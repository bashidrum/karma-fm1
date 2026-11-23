/* SPDX-License-Identifier: GPL-3.0-only */
/* The mod matrix (PHEBUS wave 5; Ben, 21 Nov 2026: "Melodic voices: the MiniFreak's swift sound design ... FX slots, mod
 * matrix last"; picker "Mod matrix: A: grid"). As the MiniFreak's LED matrix: sources in rows, destinations in columns, a cell
 * a bipolar amount. Included by params.c (the pages show it; voice.c and fx.c apply it).
 *
 * Sources (core.h MS_*), each a Q15 value: LFO1 (the LFO page's, faded, BI or UNI), LFO2 (its own RATE WAVE SYNC TRIG), AENV
 * (the amp envelope, 0..1), FENV (the filter's envelope: LINK the amp ENV, OWN its ADSR), PENV (the pitch envelope: a fast
 * fall from each note's start, ~40 ms, as ENV > PIT always was), VEL (the note's velocity, 0..1), KEY (the note from C4,
 * -1..+1 over 64 semitones).
 * Destinations (MD_*): PIT (+-12 semitones at full), M1 M2 M3 (the OSC page's three macros, as the Type names them: +-64 steps
 * at full), CUT (the voice filter: the envelopes +-128 steps (ten octaves), the others +-64), RES (+-64), LVL (the voice's
 * level: x0 .. x2), PAN (+-64), FX1 FX2 FX3 (the insert slot's AMOUNT, its mix, +-128; wave 8: the sends have no column of
 * their own, an assignable column takes one: the MPC treats a send as a mix level), AS1 AS2 (any
 * sound parameter picked on the page: +- its whole range). PIT M1 M2 M3 CUT RES LVL act on each voice with its own envelopes;
 * PAN, FX and AS on the whole voice part (the LFOs as they run, the envelopes of the note started last).
 *
 * One source of truth: six cells ARE the parameters that were fixed routings before (mx_alias), so the FILT page's ENV and
 * KEY and the grid show one value, and an old project's or preset's routings are the grid's cells as they were: LFO1 > PIT
 * (P_LD_PIT), LFO1 > CUT (P_LD_FLT), LFO1 > LVL (P_LD_AMP: the tremolo depth, one way, 0..100 %), FENV > CUT (P_ED_FLT, the
 * FILT page's ENV), PENV > PIT (P_ED_PIT), KEY > CUT (P_FKEY, FILT 2's KEY: 0..100 %, 100 % the cutoff follows the note).
 * The old LFO > SHP and ENV > SHP (P_LD_SHP, P_ED_SHP: the engine's "shape", which is one of its macros) become cells of that
 * macro's column when a sound from before is loaded (mx_from_legacy); the two stay in voice.c's sum, at 0. */
static const uint8_t MX_ALIAS[][3] = {   /* source, destination, P_* */
    {MS_LFO1, MD_PIT, P_LD_PIT}, {MS_LFO1, MD_CUT, P_LD_FLT}, {MS_LFO1, MD_LVL, P_LD_AMP},
    {MS_FENV, MD_CUT, P_ED_FLT}, {MS_PENV, MD_PIT, P_ED_PIT}, {MS_KEY, MD_CUT, P_FKEY},
};
/* the cell (s, d)'s parameter when it is one (P_*), else -1 */
static int32_t mx_alias(uint32_t s, uint32_t d)
{
    uint32_t i;
    for (i = 0; i < NELEM(MX_ALIAS); i++)
        if (MX_ALIAS[i][0] == s && MX_ALIAS[i][1] == d)
            return MX_ALIAS[i][2];
    return -1;
}
static uint32_t mx_at(uint32_t s, uint32_t d) { return s * MD_N + d + 1u; }
/* the amount of cell (s, d) on synth track ti (an alias: its parameter's value) */
static int32_t mx_get(uint32_t ti, uint32_t s, uint32_t d)
{
    int32_t a = mx_alias(s, d);
    uint32_t i, at = mx_at(s, d);
    if (ti >= NPART || s >= MS_N || d >= MD_N)
        return 0;
    if (a >= 0)
        return trk[ti].p[a];
    for (i = 0; i < MX_NCELL; i++)
        if (mxt[ti].c[i].at == at)
            return mxt[ti].c[i].amt;
    return 0;
}
/* cell (s, d) set to v (-64..63; an alias: its parameter, in its range). 0 = done, -1 = no entry left (the cells a voice can
 * set besides the fixed ones are MX_NCELL). Callers hold the audio IRQ off */
static int mx_set(uint32_t ti, uint32_t s, uint32_t d, int32_t v)
{
    int32_t a = mx_alias(s, d);
    uint32_t i, at = mx_at(s, d), fr = MX_NCELL;
    if (ti >= NPART || s >= MS_N || d >= MD_N)
        return 0;
    if (a >= 0) {
        trk[ti].p[a] = (int16_t)clamp(v, TP[a].min, TP[a].max);
        return 0;
    }
    v = clamp(v, -64, 63);
    for (i = 0; i < MX_NCELL; i++) {
        if (mxt[ti].c[i].at == at) {
            if (!v)
                mxt[ti].c[i].at = 0;
            mxt[ti].c[i].amt = (int8_t)v;
            return 0;
        }
        if (fr == MX_NCELL && !mxt[ti].c[i].at)
            fr = i;
    }
    if (!v)
        return 0;
    if (fr == MX_NCELL)
        return -1;
    mxt[ti].c[fr].at = (uint8_t)at;
    mxt[ti].c[fr].amt = (int8_t)v;
    return 0;
}
/* the cells in use besides the fixed ones */
static uint32_t mx_used(uint32_t ti)
{
    uint32_t i, n = 0;
    for (i = 0; i < MX_NCELL && ti < NPART; i++)
        n += mxt[ti].c[i].at != 0;
    return n;
}
/* the matrix of track ti as a fresh sound has it: no cell, no assignable column, LFO 2 free, unsynced (the fixed cells are
 * parameters: they go with the sound's). Spice and Dice are the pattern's: kept */
static void mx_clear_sound(uint32_t ti)
{
    if (ti >= NPART)
        return;
    memset(mxt[ti].c, 0, sizeof mxt[ti].c);
    memset(mxt[ti].as, 0, sizeof mxt[ti].as);
    mxt[ti].l2sync = 0;
    mxt[ti].l2trig = 1;                               /* (FREE: the MiniFreak's LFO 2 runs free unless asked) */
}
/* a parameter an assignable column can move: a sound parameter that locks (seq.c p_lockable), not a fixed cell's own, not a
 * TYPE (core.h: the slots' are not lockable either) */
static int p_lockable(uint32_t id);
static int mx_as_ok(uint32_t id)
{
    uint32_t i;
    if (id >= P_COUNT || !p_lockable(id) || id == P_LEVEL || id == P_SGATE)
        return 0;
    for (i = 0; i < NELEM(MX_ALIAS); i++)
        if (MX_ALIAS[i][2] == id)
            return 0;
    return 1;
}

/* ---- (wave 11, Ben 23 Nov 2026, 10:41: "holding the LFO button and then it jumps back into the page you were just on, gives
 * you the same controls and you simply wiggle what you want mapped to the lfo. same for the ENV button") HOLD TO MAP. A knob's
 * parameter as a destination of the grid: one source of truth, the knob's cell IS the MATRIX page's cell.
 *   the voice filter's CUTOFF and RESO, the OSC page's three macros, an FX slot's AMOUNT: their fixed columns (CUT RES M1 M2
 *   M3 FX1..3), acting on each note (PIT, M, CUT, RES) or the voice (FX); TUNE: PIT (a semitone a step of the amount, +-12 at
 *   full, each note: vibrato, the way a synth's pitch destination is); every other knob: an assignable column for its
 *   parameter (made on the first mapping, freed when its last cell goes), acting on the whole voice once a block.
 * Not mappable (a switch: named positions or fewer than 25; or a modulation amount itself; or nothing to move): TYPE, OCT,
 * GLMODE, VOICE, PRIOR, ALLOC, the filter's ENV and KEY and LINK, the LFO's POL SYNC TRIG ESYNC, LFO 2's SYNC TRIG, the
 * SLICER's PAT and RATE, the MATRIX page's knobs. An envelope does not map to its own times and level (AENV: ATTACK ..
 * RELEASE; FENV: the filter envelope's): "the amp envelope's own attack from itself is not useful". An LFO may move its own
 * RATE, PHASE and FADE (synths allow it: a rate that wobbles) */
#define MX_FULL_MSG "MATRIX FULL: 19 CELLS"   /* (MX_NCELL: the one limit, said where it is met) */
#define MX_NAS_MSG "MAP FULL: ALL COLUMNS USED"  /* (MX_NAS below MX_NCELL only: a device build's limit) */
static int32_t mx_as_find(uint32_t ti, uint32_t id)    /* the assignable column of parameter id, -1 none */
{
    uint32_t c;
    for (c = 0; c < MX_NAS && ti < NPART; c++)
        if (mxt[ti].as[c] == id + 1u)
            return (int32_t)(MD_AS1 + c);
    return -1;
}
static uint32_t mx_col_cells(uint32_t ti, uint32_t d)  /* the cells on column d (the fixed ones' parameters not counted) */
{
    uint32_t i, n = 0;
    for (i = 0; i < MX_NCELL && ti < NPART; i++)
        n += mxt[ti].c[i].at && (mxt[ti].c[i].at - 1u) % MD_N == d;
    return n;
}
static const param_desc_t *track_desc(const track_t *t, uint32_t id);   /* (params.c) */
/* the column parameter id (a knob's, pg_id resolved) maps to on synth track t; -1: not mappable (a switch, see above),
 * -2: mappable on an assignable column that is not there yet */
static int32_t mx_knob_col(const track_t *t, uint32_t id)
{
    uint32_t ti = (uint32_t)(t - trk), k;
    const param_desc_t *d;
    int32_t c;
    if (ti >= NPART || id >= P_COUNT)
        return -1;
    if (id == P_FCUT)
        return MD_CUT;
    if (id == P_FRES)
        return MD_RES;
    if (id == P_TUNE)
        return MD_PIT;
    for (k = 0; k < 3u; k++)
        if (vtype_macro(t, k) < 8u && id == P_E0 + vtype_macro(t, k))
            return (int32_t)(MD_M1 + k);
    for (k = 0; k < NXSLOT; k++)
        if (id == P_XS(k, 3))
            return (int32_t)(MD_FX1 + k);
    if (!mx_as_ok(id))
        return -1;
    d = track_desc(t, id);
    if (!d || d->names || d->max - d->min < 24)
        return -1;
    c = mx_as_find(ti, id);
    return c >= 0 ? c : -2;
}
static int mx_self(uint32_t s, uint32_t id)            /* an envelope onto its own stages: refused */
{
    return (s == MS_AENV && id >= P_ATK && id <= P_REL) || (s == MS_FENV && id >= P_FATK && id <= P_FREL);
}
/* the amount source s gives knob parameter id on synth track ti (0: none or not mappable) */
static int32_t mx_map_get(uint32_t ti, uint32_t s, uint32_t id)
{
    int32_t c = mx_knob_col(&trk[ti % NTRK], id);
    return c >= 0 ? mx_get(ti, s, (uint32_t)c) : 0;
}
/* source s onto knob parameter id at v (-64..63, 0 removes; an assignable column with no cell left is freed). 0 done, -1 the
 * matrix full (MX_NCELL cells), -2 every assignable column taken (MX_NAS), -3 not mappable. Callers hold the audio IRQ off */
static int mx_map_set(uint32_t ti, uint32_t s, uint32_t id, int32_t v)
{
    int32_t c, e;
    uint32_t k;
    if (ti >= NPART || s >= MS_N || mx_self(s, id))
        return -3;
    c = mx_knob_col(&trk[ti], id);
    if (c == -1)
        return -3;
    if (c == -2) {
        if (!v)
            return 0;
        for (k = 0; k < MX_NAS && mxt[ti].as[k]; k++)
            ;
        if (k == MX_NAS)
            return -2;
        if (mx_used(ti) >= MX_NCELL)
            return -1;
        mxt[ti].as[k] = (uint8_t)(id + 1u);
        c = (int32_t)(MD_AS1 + k);
    }
    e = mx_set(ti, s, (uint32_t)c, v);
    if (c >= MD_AS1 && !mx_col_cells(ti, (uint32_t)c))
        mxt[ti].as[c - MD_AS1] = 0;                   /* (its last cell gone: the column too) */
    return e;
}
/* every mapping from source s onto the parameters ids[0..n) (a page's knobs): removed (hold ENV / LFO + OCT-). Returns how many */
static uint32_t mx_map_clear(uint32_t ti, uint32_t s, const uint32_t *ids, uint32_t n)
{
    uint32_t k, r = 0;
    for (k = 0; k < n; k++)
        if (ids[k] < P_COUNT && !mx_self(s, ids[k]) && mx_map_get(ti, s, ids[k])) {
            mx_map_set(ti, s, ids[k], 0);
            r++;
        }
    return r;
}
/* is anything mapped onto knob parameter id of track ti (the mark on its dial): a bit a source */
static uint32_t mx_map_srcs(uint32_t ti, uint32_t id)
{
    uint32_t s, m = 0;
    for (s = 0; s < MS_N; s++)
        if (mx_map_get(ti, s, id))
            m |= 1u << s;
    return m;
}

/* ---- the second LFO (the audio ISR, once a block a part: voice.c track_render) */
static int32_t lfo_shape(uint32_t wave, uint32_t ph, uint32_t rnd)
{
    switch (wave) {
    case 1:
        return osc_tri(ph);
    case 2:
        return (int32_t)(ph >> 16) - 32768;
    case 3:
        return ph < 0x80000000u ? 32767 : -32767;
    case 4:
        return (int32_t)(rnd >> 16) - 32768;
    default:
        return osc_sine(ph);
    }
}
static uint32_t lsync_inc(uint32_t sync);            /* (voice.c: a cycle a note value of the tempo) */
static void lfo2_tick(track_t *t)
{
    uint32_t ti = (uint32_t)(t - trk), old;
    mxt_t *m;
    if (ti >= NPART)
        return;
    m = &mxt[ti];
    old = m->l2ph;
    m->l2ph += m->l2sync > 0 ? lsync_inc((uint32_t)m->l2sync) : LFO_INC[t->p[P_L2RATE] & 127];
    if (m->l2ph < old)                                  /* (its own random numbers: the shared ones stay the voices') */
        m->l2rnd = m->l2rnd * 1664525u + 1013904223u + ti;
    m->l2val = lfo_shape((uint32_t)t->p[P_L2WAVE] % 5u, m->l2ph, m->l2rnd);
}

/* ---- the sound's routings of before as cells (a project, user preset or factory preset from before wave 5): LFO > SHP and
 * ENV > SHP moved the engine's "shape", which is one of its macros (PLAITS' M3, JUNO's PW (M2), ELEMENTS' E6 (M4: on an
 * assignable column)); ACID had none (they did nothing there). p: the sound's parameters, e its engine, ti the track */
static void mx_from_legacy(uint32_t ti, int16_t *p, uint32_t e)
{
    int32_t shp = e == ENGI_PRISM ? 3 : e == ENGI_JUNO ? 1 : e == ENGI_ELEM ? 6 : -1, col = -1, k;
    if (ti >= NPART)
        return;
    if (shp >= 0 && (p[P_LD_SHP] || p[P_ED_SHP])) {
        uint32_t ty = vtype_find(e, (uint32_t)p[P_E0]);
        for (k = 0; k < 3 && ty < NVTYPE; k++)
            if ((int32_t)VTYPES[ty].m[k] == shp)
                col = MD_M1 + k;
        if (col < 0) {                                /* (not on the page: an assignable column, at its scale) */
            uint32_t a = 0;
            while (a + 1u < MX_NAS && mxt[ti].as[a] && mxt[ti].as[a] != P_E0 + (uint32_t)shp + 1u)
                a++;
            mxt[ti].as[a] = (uint8_t)(P_E0 + (uint32_t)shp + 1u);
            col = (int32_t)(MD_AS1 + a);
        }
        if (col >= MD_AS1) {                          /* (an assignable column moves +- the whole range at full: half) */
            mx_set(ti, MS_LFO1, (uint32_t)col, p[P_LD_SHP] / 2);
            mx_set(ti, MS_AENV, (uint32_t)col, p[P_ED_SHP] / 2);
        } else {
            mx_set(ti, MS_LFO1, (uint32_t)col, p[P_LD_SHP]);
            mx_set(ti, MS_AENV, (uint32_t)col, p[P_ED_SHP]);
        }
    }
    p[P_LD_SHP] = p[P_ED_SHP] = 0;
}

/* ---- (wave 5) the FX slots: a TYPE picked on slot k of the sound p (the MiniFreak's way: a type starts at its own settings).
 * OFF: the knobs at 0. (Wave 8, Ben 22 Nov 2026: "FX = whole mix only") every slot type is this voice's insert; the old send
 * types (CHORUS, REVERB, DELAY: the stores' numbers) are never picked, the sends are the SENDS page's (P_DLY P_REV P_CHOR) */
static void xs_defaults(int16_t *p, uint32_t k, int32_t ty)
{
    uint32_t f;
    ty = clamp(ty, 0, XT_N - 1);
    if (xt_send(ty))
        ty = XT_OFF;
    p[P_XS(k, 0)] = (int16_t)ty;
    for (f = 1; f <= 3u; f++)
        p[P_XS(k, f)] = (int16_t)(ty ? XS_DESC[ty][f - 1u].def : 0);
}
/* slot k of p may take type ty: any insert, the FLANGER once a voice (its line); never an old send type */
static int xs_type_ok(const int16_t *p, uint32_t k, int32_t ty)
{
    uint32_t j;
    if (xt_send(ty))
        return 0;
    if (ty != XT_FLANGER)
        return 1;
    for (j = 0; j < NXSLOT; j++)
        if (j != k && p[P_XS(j, 0)] == ty)
            return 0;
    return 1;
}
static void xs_type_set(int16_t *p, uint32_t k, int32_t ty)
{
    if (p[P_XS(k, 0)] != ty)
        xs_defaults(p, k, ty);
}

/* ---- (wave 8) a sound of waves 5 to 7 (a project, a user preset record 3): a CHORUS, REVERB or DELAY slot was the voice's send
 * to that shared effect (its AMOUNT the send, P_CHOR P_REV P_DLY: the very parameter the SENDS page shows), and a send without
 * its slot was silent. Now the sends are always heard and every slot is an insert: such a slot becomes OFF in its place (the
 * inserts keep their order, their locks stay on their knobs), its send stays as it was; a send the sound had no slot for goes
 * to 0 (as it sounded; its locks, which did nothing, freed). A matrix cell on such a slot's FX column moves to a free
 * assignable column aimed at that send (the same amount: +-128 there, +- the range on an assignable one); none free: dropped
 * (fx8_lost counts them). lk: the sound's locks (0: none), ti the synth track (-1: no matrix) */
static uint32_t fx8_lost;                            /* (the last conversion: tests/fx8_test.c) */
static void fx_sends_from_slots(int16_t *p, plock_t *lk, int32_t ti)
{
    static const uint8_t SID[3] = {P_CHOR, P_REV, P_DLY};
    uint32_t k, had = 0, i, j;
    for (k = 0; k < NXSLOT; k++) {
        int32_t ty = p[P_XS(k, 0)];
        uint32_t sid;
        if (!xt_send(ty))
            continue;
        sid = xs_send_id(ty);
        had |= sid == P_CHOR ? 1u : sid == P_REV ? 2u : 4u;
        if (ti >= 0 && ti < (int32_t)NPART) {        /* its FX column's cells onto an assignable column for its send */
            for (i = 0; i < MS_N; i++) {
                int32_t a = mx_get((uint32_t)ti, i, MD_FX1 + k);
                uint32_t c;
                if (!a)
                    continue;
                for (c = 0; c < MX_NAS; c++)
                    if (!mxt[ti].as[c] || mxt[ti].as[c] == sid + 1u)
                        break;
                mx_set((uint32_t)ti, i, MD_FX1 + k, 0);
                if (c < MX_NAS) {
                    mxt[ti].as[c] = (uint8_t)(sid + 1u);
                    if (mx_set((uint32_t)ti, i, MD_AS1 + c, a) < 0)
                        fx8_lost++;
                } else {
                    fx8_lost++;
                }
            }
        }
        for (j = 0; lk && j < NLOCK; j++)            /* (its knobs were the shared settings and the send: no lock of its own) */
            if (lk[j].step != LOCK_FREE && lk[j].param >= P_XS(k, 1) && lk[j].param <= P_XS(k, 3))
                lk[j].step = LOCK_FREE, lk[j].param = 0, lk[j].val = 0;
        xs_defaults(p, k, XT_OFF);
    }
    for (i = 0; i < 3u; i++) {
        if ((had >> i) & 1u)
            continue;
        p[SID[i]] = 0;
        for (j = 0; lk && j < NLOCK; j++)
            if (lk[j].step != LOCK_FREE && lk[j].param == SID[i])
                lk[j].step = LOCK_FREE, lk[j].param = 0, lk[j].val = 0;
    }
}

/* ---- a sound from before wave 5 (a project, user preset, factory preset): its DIST and INSERT as slots, in their order (wave
 * 8: its sends stay its sends, as they always were, all three heard). Exact but for: an INSERT's fourth value (the drives'
 * LEVEL: into the sound's trim, exact at MIX 100 %; CRUSH's LPF and PHASER's / FLANGER's DEPTH: their defaults), and the
 * INSERT's mono CHORUS (the shared CHORUS now, as a send). lk: the sound's locks (NLOCK, 0: none), moved with what they lock */
static uint32_t fx_legacy_lost, fx_legacy_approx;      /* (the last conversion: tests/fx5_test.c; nothing is lost since wave 8) */
static void fx_from_legacy(int16_t *p, plock_t *lk)
{
    int32_t it = clamp(p[P_ITYPE], 0, 8), mix = clamp(p[P_IMIX], 0, 127), n = 0, k;
    int32_t ty[3], kd = -1, ki = -1;
    static const int8_t IT_XT[9] = {XT_OFF, XT_SOFT, XT_HARD, XT_FOLD, XT_FUZZ, XT_CRUSH, XT_PHASER, XT_FLANGER, XT_OFF};
    int16_t ia = p[P_IA], ib = p[P_IB], ic = p[P_IC];
    for (k = 0; k < (int32_t)NXSLOT; k++)
        xs_defaults(p, (uint32_t)k, XT_OFF);
    if (p[P_DIST] > 0)
        kd = n, ty[n++] = XT_DIST;
    if (it && mix && it != 8)
        ki = n, ty[n++] = IT_XT[it];
    if (it == 8 && mix && !p[P_CHOR])                   /* (the INSERT's mono chorus: the shared one, a send) */
        p[P_CHOR] = (int16_t)(mix * 24 / 127 > 0 ? mix * 24 / 127 : 1), fx_legacy_approx++;
    for (k = 0; k < n && k < (int32_t)NXSLOT; k++) {
        p[P_XS(k, 0)] = (int16_t)ty[k];
        if (k == kd)
            p[P_XS(k, 1)] = 64, p[P_XS(k, 2)] = p[P_DIST], p[P_XS(k, 3)] = 127;
        else if (k == ki && it <= 4)                    /* SOFT .. FUZZ: TONE DRIVE MIX; LEVEL into the trim */
            p[P_XS(k, 1)] = ib, p[P_XS(k, 2)] = ia, p[P_XS(k, 3)] = (int16_t)mix;
        else if (k == ki && it == 5)                    /* CRUSH: RATE BITS MIX */
            p[P_XS(k, 1)] = ib, p[P_XS(k, 2)] = ia, p[P_XS(k, 3)] = (int16_t)mix;
        else if (k == ki)                               /* PHASER, FLANGER: RATE FDBK MIX */
            p[P_XS(k, 1)] = ia, p[P_XS(k, 2)] = ic, p[P_XS(k, 3)] = (int16_t)mix;
    }
    if (ki >= 0 && it <= 4 && ic != 96) {               /* the drives' LEVEL (96 the slots'): the trim, in 1/2 dB */
        int32_t d5 = (LEVEL_DB_X10[clamp(ic, 1, 127)] - LEVEL_DB_X10[96]);
        p[P_ED_FX] = (int16_t)clamp(p[P_ED_FX] + (d5 >= 0 ? d5 + 2 : d5 - 2) / 5, -64, 63);
        fx_legacy_approx += mix < 127;
    }
    if (ki >= 0 && ((it == 5 && ic != 96) || (it >= 6 && ib != 96)))
        fx_legacy_approx++;
    for (k = 0; lk && k < (int32_t)NLOCK; k++) {        /* the locks, onto the slots' knobs */
        uint32_t id = lk[k].param, to = 0xFFu;
        if (lk[k].step == LOCK_FREE)
            continue;
        if (id == P_DIST)
            to = kd >= 0 ? P_XS(kd, 2) : 0xFEu;
        else if (id >= P_IA && id <= P_IMIX) {
            if (ki < 0)
                to = 0xFEu;
            else if (id == P_IMIX)
                to = P_XS(ki, 3);
            else if (it <= 5)
                to = id == P_IA ? P_XS(ki, 2) : id == P_IB ? P_XS(ki, 1) : 0xFEu;
            else
                to = id == P_IA ? P_XS(ki, 1) : id == P_IC ? P_XS(ki, 2) : 0xFEu;
        }
        if (to == 0xFEu)
            lk[k].step = LOCK_FREE, lk[k].param = 0, lk[k].val = 0;
        else if (to != 0xFFu)
            lk[k].param = (uint8_t)to;
    }
    p[P_DIST] = 0;
    p[P_ITYPE] = 0;
    p[P_IA] = TP[P_IA].def, p[P_IB] = TP[P_IB].def, p[P_IC] = TP[P_IC].def, p[P_IMIX] = TP[P_IMIX].def;
}
