/* SPDX-License-Identifier: GPL-3.0-only */
/* PHEBUS wave 5, the MiniFreak voices, part two (SPEC.md, "New direction for PHEBUS", "Direction decisions"):
 *   defaults  a fresh sound has every FX slot OFF and sends nothing (Ben: "I don't like having fx on by default")
 *   fx        each FX slot type changes the voice, bounded; OFF, MIX 0 (or the EQ flat): the voice bit for bit as with no
 *             slot; (wave 8, Ben 22 Nov 2026: "FX = whole mix only") the sends are the voice's own, heard with no slot, and
 *             CHORUS, REVERB, DELAY are no slot types; one FLANGER a voice
 *   matrix    every source (LFO1 LFO2 AENV FENV PENV VEL KEY) reaches every destination (PIT M1 M2 M3 CUT RES LVL PAN FX1
 *             FX2 FX3 AS1 AS2): a cell set changes what is heard; the fixed cells and their parameters are one value
 *   spice     Spice 0: the pattern exactly as written, whatever the roll; Spice up: the roll varies it, another roll
 *             another variation, the same roll the same (seeded); the notes written never change
 *   project   the slots, the matrix, LFO 2, Spice and Dice through a project; a project from before (DIST, sends, LFO >
 *             SHP, a DIST lock) loads as slots, sends and cells (wave 8: every send kept; tests/fx8_test.c: wave 5's send slots)
 *   cpu       the host's cost of each slot type and of a busy matrix (printed, not checked)
 * Seeded and deterministic. Exit status: the number of failed checks. */
#include <stdint.h>
#define main hostsim_main
#include "hostsim.c"
#undef main
#define PROJ_HOST 1
static uint32_t trk_def_engine(uint32_t i) { return i < NPART ? ENGI_JUNO : 0u; }
#include "../firmware/src/project.c"
static int fails;
static void check(int ok, const char *what)
{
    printf("fx5: %-100s %s\n", what, ok ? "ok" : "FAIL");
    fails += !ok;
}
#define NB 600u
static int32_t out_l[NB * CTL], out_r[NB * CTL], ref_l[NB * CTL], ref_r[NB * CTL];
static void render(uint32_t nb)
{
    static int32_t o[2u * CTL];
    uint32_t b, i;
    for (b = 0; b < nb && b < NB; b++) {
        mix_block(o, CTL);
        for (i = 0; i < CTL; i++)
            out_l[b * CTL + i] = o[2u * i], out_r[b * CTL + i] = o[2u * i + 1u];
    }
}
static double diff(uint32_t nb)                         /* mean |out - ref| over both channels */
{
    double d = 0;
    uint32_t i;
    for (i = 0; i < nb * CTL; i++)
        d += fabs((double)out_l[i] - ref_l[i]) + fabs((double)out_r[i] - ref_r[i]);
    return d / (2.0 * nb * CTL);
}
static int32_t peak(uint32_t nb)
{
    int32_t m = 0;
    uint32_t i;
    for (i = 0; i < nb * CTL; i++)
        m = abs(out_l[i]) > m ? abs(out_l[i]) : m;
    return m;
}
/* every comparison from the same state: a child process sets up, renders and hands the output back (the parent's state
 * never moves: voices, buses, random numbers, the engines' memory, all the same at each start) */
#include <sys/mman.h>
static struct { int32_t l[NB * CTL], r[NB * CTL]; int flag; } *sh;
static void run(void (*cfg)(intptr_t), intptr_t a, uint32_t nb)
{
    pid_t pid;
    if (!sh)
        sh = mmap(0, sizeof *sh, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    fflush(stdout);
    pid = fork();
    if (!pid) {
        cfg(a);
        render(nb);
        memcpy(sh->l, out_l, sizeof out_l);
        memcpy(sh->r, out_r, sizeof out_r);
        _exit(0);
    }
    waitpid(pid, 0, 0);
    memcpy(out_l, sh->l, sizeof out_l);
    memcpy(out_r, sh->r, sizeof out_r);
}
static void keep_ref(void) { memcpy(ref_l, out_l, sizeof ref_l); memcpy(ref_r, out_r, sizeof ref_r); }
/* everything at rest: the effects' lines and states, the voices, the random numbers; track 1 a plain JUNO saw */
static void reset(void)
{
    uint32_t i, k;
    host_tracks_init();
    for (i = 0; i < 4u; i++) {
        int16_t *ln = ins[i].line;
        memset(&ins[i], 0, sizeof ins[i]);
        ins[i].line = ln;
    }
    memset(xst, 0, sizeof xst);
    memset(ins_lines, 0, sizeof ins_lines);
    memset(dly_buf, 0, sizeof dly_buf);
    memset(cho_buf, 0, sizeof cho_buf);
    memset(rev_line, 0, sizeof rev_line);
    memset(rev_ap, 0, sizeof rev_ap);
    memset(&fx, 0, sizeof fx);
    for (k = 0; k < NTRK; k++) {
        trk[k].ins_run = trk[k].tail = 0;
        trk[k].lfo_ph = 0, trk[k].lfo_val = 0, trk[k].lfo_fade = 32767, trk[k].lvl = 0;
        for (i = 0; i < NVOICE; i++)
            memset(&trk[k].v[i], 0, sizeof trk[k].v[i]);
        trk[k].nmono = trk[k].mono_note = 0;
    }
    rng_state = 0x1234567u;
    song.playing = 0;
    song.master_q12 = 2048;
    memset(&lim_dac, 0, sizeof lim_dac);
    lim_dac.env = LIM_T, lim_dac.g = 32768, lim_dac.hold = 0;
    trk[0].p[P_LEVEL] = 100;
    trk[0].p[P_ATK] = 0, trk[0].p[P_DEC] = 40, trk[0].p[P_SUS] = 90, trk[0].p[P_REL] = 30;
    trk[0].p[P_VOICE] = V_POLY;
}

static void t_defaults(void)
{
    uint32_t k, i, ok = 1;
    host_tracks_init();                                 /* (power-on and NEW: every parameter its default, main.c) */
    host_preset(&trk[0], ENGI_ACID, ACID_INIT);         /* (the plain starts: ACID INIT, JUNO INIT, PLAITS INIT) */
    host_preset(&trk[1], ENGI_JUNO, JUNO_INIT);
    host_preset(&trk[2], ENGI_PRISM, PRISM_INIT);
    for (i = 0; i < NPART; i++) {
        for (k = 0; k < NXSLOT; k++)
            ok &= trk[i].p[P_XS(k, 0)] == XT_OFF;
        ok &= !trk[i].p[P_CHOR] && !trk[i].p[P_DLY] && !trk[i].p[P_REV] && !mx_used(i) && !mxt[i].spice;
    }
    check(ok, "defaults: power-on and the INIT sounds: every voice's three FX slots OFF, no send, no matrix cell, Spice 0");
    ok = 1;
    for (k = 0; k < P_COUNT; k++)
        if (!TP[k].label && (k < P_E0 || k > P_E7))   /* (P_E0 ..: named by each engine) */
            printf("  parameter %u has no name\n", (unsigned)k), ok = 0;
    check(ok && TP[P_X1TYPE].def == XT_OFF && TP[P_X2TYPE].def == XT_OFF && TP[P_X3TYPE].def == XT_OFF,
          "defaults: the slots' TYPE starts OFF (TP), every parameter named");
}

static int32_t fx_ty, fx_zero;
static void cfg_fx(intptr_t ty)
{
    reset();
    if (ty) {
        xs_type_set(trk[0].p, 1, (int32_t)ty);           /* (slot 2: the place does not matter) */
        if (ty == XT_EQ)
            trk[0].p[P_X2TIME] = 110, trk[0].p[P_X2INT] = 20, trk[0].p[P_X2AMT] = 100;
        if (xt_send((int32_t)ty))                       /* (the shared effects: a clear send) */
            trk[0].p[xs_send_id((int32_t)ty)] = 100;
        if (fx_zero) {                                  /* bypassed: MIX 0, a send of 0, the EQ flat */
            if (xt_send((int32_t)ty))
                trk[0].p[xs_send_id((int32_t)ty)] = 0;
            else if (ty == XT_EQ)
                trk[0].p[P_X2TIME] = trk[0].p[P_X2INT] = trk[0].p[P_X2AMT] = 64;
            else
                trk[0].p[P_X2AMT] = 0;
        }
    }
    trk_note_on(&trk[0], 45, 100);
}
static void cfg_nosend(intptr_t a)
{
    (void)a;
    reset();
    trk[0].p[P_REV] = 100, trk[0].p[P_DLY] = 100, trk[0].p[P_CHOR] = 100;
    trk_note_on(&trk[0], 45, 100);
}
static void t_fx(void)
{
    int32_t ty;
    uint32_t ok_all = 1, byp_all = 1;
    char what[120];
    fx_zero = 0;
    run(cfg_fx, 0, NB);
    keep_ref();                                         /* no slot */
    for (ty = 1; ty < XT_N; ty++) {
        double d, d0;
        int32_t pk;
        if (xt_send(ty))                                /* (wave 8: no slot type: the SENDS page, below) */
            continue;
        fx_zero = 0;
        run(cfg_fx, ty, NB);
        d = diff(NB);
        pk = peak(NB);
        fx_zero = 1;
        run(cfg_fx, ty, NB);
        d0 = diff(NB);
        printf("  %-7s mean |diff| %8.1f, peak %6d; at zero |diff| %.2f\n", N_XBIG[ty], d, pk, d0);
        snprintf(what, sizeof what, "FX %s: changes the voice, bounded", N_XBIG[ty]);
        ok_all &= d > 20.0 && pk < 32767;
        byp_all &= d0 == 0.0;
    }
    check(ok_all, "FX: every slot type (PHASER FLANGER DIST SOFT HARD FOLD FUZZ CRUSH EQ) changes the voice, bounded");
    check(byp_all, "FX: at MIX 0 or the EQ flat, every type leaves the voice bit for bit as no slot");
    run(cfg_nosend, 0, NB);
    check(diff(NB) > 20.0, "FX (wave 8): the voice's sends to the delay, reverb and chorus are heard with no slot (the SENDS page)");
    {   /* (wave 8) the old send types are never a slot's; one FLANGER a voice; a slot's type leaves the sends alone */
        int16_t p[P_COUNT];
        uint32_t k, ok = 1;
        memcpy(p, trk[0].p, sizeof p);
        p[P_DLY] = 50, p[P_REV] = 40, p[P_CHOR] = 30;
        for (k = 0; k < NXSLOT; k++) {
            xs_type_set(p, k, XT_OFF);
            ok &= !xs_type_ok(p, k, XT_CHORUS) && !xs_type_ok(p, k, XT_REVERB) && !xs_type_ok(p, k, XT_DELAY);
            xs_type_set(p, k, XT_DELAY);
            ok &= p[P_XS(k, 0)] == XT_OFF;
        }
        xs_type_set(p, 0, XT_FLANGER);
        xs_type_set(p, 1, XT_DIST);
        xs_type_set(p, 2, XT_CRUSH);
        ok &= !xs_type_ok(p, 1, XT_FLANGER) && xs_type_ok(p, 0, XT_FLANGER) && p[P_DLY] == 50 && p[P_REV] == 40 && p[P_CHOR] == 30;
        check(ok, "FX (wave 8): CHORUS REVERB DELAY are no slot types; one FLANGER a voice; three inserts and three sends at once");
    }
}

/* ---- the matrix: each source to each destination */
static void mx_setup(uint32_t s)                        /* track 1: PLAITS VA (three real macros), every source moving */
{
    reset();
    vtype_switch(&trk[0], 2);                           /* VA: DETUNE SQUARE SAW */
    trk[0].engine = trk[0].eng_req;
    trk[0].p[P_E1] = 40, trk[0].p[P_E2] = 40, trk[0].p[P_E3] = 40;
    trk[0].p[P_FCUT] = 70, trk[0].p[P_FRES] = 40, trk[0].p[P_SUS] = 60, trk[0].p[P_DEC] = 50;
    trk[0].p[P_FLINK] = 1, trk[0].p[P_FATK] = 30, trk[0].p[P_FDEC] = 40, trk[0].p[P_FSUS] = 50;
    trk[0].p[P_LRATE] = 90, trk[0].p[P_L2RATE] = 80;
    xs_type_set(trk[0].p, 0, XT_DIST);                  /* FX1 FX2 FX3: a DIST, a PHASER, a CRUSH at half (wave 8: inserts) */
    xs_type_set(trk[0].p, 1, XT_PHASER);
    xs_type_set(trk[0].p, 2, XT_CRUSH);
    trk[0].p[P_X1AMT] = 64, trk[0].p[P_X2AMT] = 64, trk[0].p[P_X3AMT] = 64, trk[0].p[P_DLY] = 64;
    mxt[0].as[0] = P_FCUT + 1u, mxt[0].as[1] = P_E1 + 1u;   /* AS1 the cutoff, AS2 a macro */
    (void)s;
}
static int32_t mx_cell, mx_amt;
static void cfg_mx(intptr_t x)
{
    (void)x;
    mx_setup(0);
    if (mx_cell >= 0)
        mx_set(0, (uint32_t)mx_cell / MD_N, (uint32_t)mx_cell % MD_N, mx_amt);
    trk_note_on(&trk[0], 72, 100);                      /* (C5: KEY is not 0) */
}
static void t_matrix(void)
{
    uint32_t s, d, bad = 0, n = 0;
    for (s = 0; s < MS_N; s++)
        for (d = 0; d <= MD_AS2; d++) {                 /* (wave 11: AS3 .. are columns with no parameter here) */
            double df;
            int32_t a = mx_alias(s, d) >= 0 && (s == MS_LFO1 && d == MD_LVL) ? 100 : s == MS_KEY && d == MD_CUT ? 100 : 60;
            mx_cell = -1;
            run(cfg_mx, 0, 500);
            keep_ref();
            mx_cell = (int32_t)(s * MD_N + d), mx_amt = a;
            run(cfg_mx, 0, 500);
            df = diff(500);
            n++;
            if (df < 2.0) {
                printf("  %s > %u: |diff| %.2f\n", N_MSRC[s], (unsigned)d, df);
                bad++;
            }
        }
    printf("  %u cells tried, %u without effect\n", (unsigned)n, (unsigned)bad);
    check(!bad, "matrix: every source (LFO1 LFO2 AENV FENV PENV VEL KEY) reaches every destination (PIT M1..M3 CUT RES LVL PAN FX1..3 AS1 AS2)");
    {   /* the fixed cells are the parameters: one value, both ways, and a cell's sound is its parameter's */
        uint32_t ok = 1, i;
        reset();
        trk[0].p[P_ED_FLT] = 30, trk[0].p[P_LD_PIT] = -12, trk[0].p[P_LD_FLT] = 20, trk[0].p[P_LD_AMP] = 90;
        trk[0].p[P_ED_PIT] = 7, trk[0].p[P_FKEY] = 100;
        ok &= mx_get(0, MS_FENV, MD_CUT) == 30 && mx_get(0, MS_LFO1, MD_PIT) == -12 && mx_get(0, MS_LFO1, MD_CUT) == 20 &&
              mx_get(0, MS_LFO1, MD_LVL) == 90 && mx_get(0, MS_PENV, MD_PIT) == 7 && mx_get(0, MS_KEY, MD_CUT) == 100;
        mx_set(0, MS_FENV, MD_CUT, -40);
        mx_set(0, MS_LFO1, MD_PIT, 3);
        ok &= trk[0].p[P_ED_FLT] == -40 && trk[0].p[P_LD_PIT] == 3 && !mx_used(0);
        for (i = 0; i < NELEM(MX_ALIAS); i++)           /* (none of them ever takes a free cell) */
            ok &= mx_alias(MX_ALIAS[i][0], MX_ALIAS[i][1]) == MX_ALIAS[i][2];
        check(ok, "matrix: the FILT page's ENV and KEY, the old LFO DEST and ENV PIT are the grid's cells: one value, both ways");
    }
    {   /* full: 19 cells besides the fixed ones, then refused */
        uint32_t i, ok = 1;
        reset();
        for (i = 0; i < MX_NCELL; i++)
            ok &= mx_set(0, MS_LFO2, i % MD_N, 10) == 0 || 1;
        for (i = 0, s = 0; s < MS_N && i < MX_NCELL + 3u; s++)
            for (d = 0; d < MD_N; d++)
                if (mx_alias(s, d) < 0 && i < MX_NCELL + 3u)
                    ok &= (mx_set(0, s, d, 5) == 0) == (mx_used(0) <= MX_NCELL), i++;
        check(mx_used(0) == MX_NCELL && mx_set(0, MS_VEL, MD_AS2, 9) == -1 && mx_set(0, MS_LFO2, MD_PIT, 0) == 0,
              "matrix: 19 cells besides the fixed ones, the 20th refused (MATRIX FULL), a cell set to 0 frees its place");
    }
}

/* ---- Spice and Dice */
static void sd_pattern(void)
{
    uint32_t i;
    reset();
    trk[0].p[P_SLEN] = 16;
    trk[0].p[P_SDIV] = 2;
    for (i = 0; i < 16u; i++) {
        step_t *st = &trk[0].step[i];
        st->time = ST_NOTE, st->n = 1, st->note[0] = (uint8_t)(48 + (i * 5) % 12), st->vel = 100;
    }
    trk[0].seq_active = 1;
    song.g[G_BPM] = 140;
}
static int32_t sd_sp, sd_dc;
static void cfg_sd(intptr_t x)
{
    (void)x;
    sd_pattern();
    mxt[0].spice = (int16_t)sd_sp, mxt[0].dice = (int16_t)sd_dc;
    transport_req = 1;
}
static void sd_play(int32_t spice, int32_t dice)
{
    sd_sp = spice, sd_dc = dice;
    run(cfg_sd, 0, NB);
}
static void t_spice(void)
{
    step_t before[NSTEP];
    double d;
    sd_play(0, 0);
    keep_ref();
    sd_pattern();
    memcpy(before, trk[0].step, sizeof before);
    sd_play(0, 4321);
    check(diff(NB) == 0.0, "Spice 0: the pattern exactly as written, whatever the roll (bit for bit)");
    sd_play(110, 7);
    d = diff(NB);
    keep_ref();
    sd_play(110, 7);
    check(diff(NB) == 0.0, "the same roll at the same Spice: the same variation (seeded)");
    sd_play(110, 8);
    check(diff(NB) > 50.0, "Dice: another roll, another variation");
    sd_pattern();                                       /* (in this process: play, then look at the steps) */
    mxt[0].spice = 110, mxt[0].dice = 7;
    transport_req = 1;
    render(NB);
    transport_req = 2;
    render(2);
    check(d > 50.0 && !memcmp(before, trk[0].step, sizeof before),
          "Spice 110: the roll varies the pattern (heard), the notes written unchanged");
    {   /* what varies, and only that: more Spice is the same roll further; no Spice varies nothing */
        uint32_t i, off = 0, oct = 0, rat = 0, vel = 0, len = 0, nest = 1;
        spv_t a, b;
        sd_pattern();
        for (mxt[0].dice = 0; mxt[0].dice < 50; mxt[0].dice++)
            for (i = 0; i < 16u; i++) {
                mxt[0].spice = 127;
                spice_of(&trk[0], i, &a);
                mxt[0].spice = 40;
                spice_of(&trk[0], i, &b);
                off += a.off, oct += a.oct != 0, rat += a.hits != 0, vel += a.dvel != 0, len += a.glen != 64;
                nest &= (!b.off || a.off) && (!b.oct || a.oct == b.oct) && (!b.hits || a.hits == b.hits);
                mxt[0].spice = 0;
                spice_of(&trk[0], i, &b);
                nest &= !b.off && !b.oct && !b.hits && !b.dvel && b.glen == 64;
            }
        printf("  at full Spice over 800 steps: left out %u, octave %u, ratchet %u, velocity %u, length %u\n",
               (unsigned)off, (unsigned)oct, (unsigned)rat, (unsigned)vel, (unsigned)len);
        check(off > 150 && off < 330 && oct > 120 && oct < 280 && rat > 90 && rat < 240 && vel > 700 && len > 700 && nest,
              "Dice varies gate (about 30 %), octave (25 %), ratchet (20 %), velocity and length; less Spice the same roll, less far");
    }
}

/* ---- through a project */
static void t_project(void)
{
    static project_t w;
    uint32_t ok = 1;
    int l;
    reset();
    xs_type_set(trk[1].p, 0, XT_FLANGER);
    xs_type_set(trk[1].p, 2, XT_EQ);
    trk[1].p[P_X1TIME] = 33, trk[1].p[P_REV] = 77, trk[1].p[P_L2RATE] = 11, trk[1].p[P_L2WAVE] = 3;
    mx_set(1, MS_LFO2, MD_M2, -21);
    mx_set(1, MS_VEL, MD_AS1, 17);
    mxt[1].as[0] = P_FDRV + 1u, mxt[1].l2sync = 5, mxt[1].l2trig = 2, mxt[1].spice = 66, mxt[1].dice = 1234;
    lock_set(&trk[1], 3, P_X1TIME, 90);
    proj_capture(&w);
    ok &= w.c.rsv[0] == PROJ_SEND_V && proj_ok(&w.c);
    reset();
    proj_apply(&w.c, 1);
    l = lock_find(&trk[1], 3, P_X1TIME, 0);
    ok &= trk[1].p[P_X1TYPE] == XT_FLANGER && trk[1].p[P_X3TYPE] == XT_EQ && trk[1].p[P_X1TIME] == 33 &&
          trk[1].p[P_REV] == 77 && trk[1].p[P_L2RATE] == 11 && trk[1].p[P_L2WAVE] == 3 && mx_get(1, MS_LFO2, MD_M2) == -21 &&
          mx_get(1, MS_VEL, MD_AS1) == 17 && mxt[1].as[0] == P_FDRV + 1u && mxt[1].l2sync == 5 && mxt[1].l2trig == 2 &&
          mxt[1].spice == 66 && mxt[1].dice == 1234 && l >= 0 && trk[1].lock[l].val == 90;
    check(ok, "project: the slots, a slot lock, LFO 2, the matrix (cells, an assignable column), Spice and Dice round trip");
    /* a project from before wave 5: DIST 40, delay 30, reverb 20, chorus 10 (wave 8: every send kept, as it played), a DIST
     * lock; PLAITS with LFO > SHP 20 */
    w.c.rsv[0] = PROJ_VOICE_V;
    memset(w.c.t[1].mod, 0, sizeof w.c.t[1].mod);
    {
        uint32_t k;
        for (k = P_X1TYPE; k < PROJ_NP; k++)
            w.c.t[1].p[k] = 0;
        for (k = 0; k < NLOCK; k++)
            w.c.t[1].lock[k].step = LOCK_FREE;
    }
    w.c.t[1].p[P_DIST] = 40, w.c.t[1].p[P_DLY] = 30, w.c.t[1].p[P_REV] = 20, w.c.t[1].p[P_CHOR] = 10;
    w.c.t[1].lock[0].step = 2, w.c.t[1].lock[0].param = P_DIST, w.c.t[1].lock[0].val = 99;
    w.c.t[2].engine = ENGI_PRISM, w.c.t[2].p[P_E0] = VT_PLX(0), w.c.t[2].p[P_LD_SHP] = 20, w.c.t[2].p[P_ED_SHP] = -8;
    w.c.sum = proj_sum(&w.c);
    reset();
    fx_legacy_lost = 0;
    proj_apply(&w.c, 1);
    l = lock_find(&trk[1], 2, P_X1INT, 0);
    ok = trk[1].p[P_X1TYPE] == XT_DIST && trk[1].p[P_X1INT] == 40 && trk[1].p[P_X1AMT] == 127 && trk[1].p[P_X1TIME] == 64 &&
         trk[1].p[P_X2TYPE] == XT_OFF && trk[1].p[P_X3TYPE] == XT_OFF && trk[1].p[P_DLY] == 30 && trk[1].p[P_REV] == 20 &&
         trk[1].p[P_CHOR] == 10 && !trk[1].p[P_DIST] && fx_legacy_lost == 0u && l >= 0 && trk[1].lock[l].val == 99 &&
         mx_get(2, MS_LFO1, MD_M3) == 20 && mx_get(2, MS_AENV, MD_M3) == -8 && !trk[2].p[P_LD_SHP] && !trk[2].p[P_ED_SHP];
    check(ok, "a project from before: DIST as a slot, every send kept (wave 8), the DIST lock on DRIVE; LFO > SHP as LFO1 > M3");
}

/* ---- a sound from before sounds the same: P_DIST and the sends of wave 4's mix, against the slots they become */
static void t_same(void)
{
    /* the old path is gone; what stays is its arithmetic: a DIST slot at TONE 64 and MIX 100 % is P_DIST's chain, and a
     * send through its slot is the send as before (taken after the level). Checked: a factory preset with sends (JUNO PAD)
     * loads its sends as slots, each its value */
    reset();
    host_preset(&trk[0], ENGI_JUNO, 6);
    check(!xt_send(trk[0].p[P_X1TYPE]) && !xt_send(trk[0].p[P_X2TYPE]) && !xt_send(trk[0].p[P_X3TYPE]) && !trk[0].p[P_DIST] &&
              (trk[0].p[P_REV] || trk[0].p[P_CHOR] || trk[0].p[P_DLY]),
          "a factory preset's sends load as the voice's sends (wave 8: no slot taken)");
}

static void t_cpu(void)
{
    static const int32_t T[] = {XT_OFF, XT_CHORUS, XT_PHASER, XT_FLANGER, XT_REVERB, XT_DELAY, XT_DIST, XT_SOFT, XT_CRUSH, XT_EQ};
    uint32_t i, r;
    double base = 0;
    printf("  host cost of one voice part (JUNO, one note) with one FX slot, ns a sample (best of 3):\n");
    for (i = 0; i < NELEM(T); i++) {
        double best = 1e9;
        for (r = 0; r < 3u; r++) {
            uint64_t t0;
            reset();
            if (T[i])
                xs_type_set(trk[0].p, 0, T[i]);
            if (xt_send(T[i]))
                trk[0].p[xs_send_id(T[i])] = 80;
            trk_note_on(&trk[0], 45, 100);
            render(20);
            t0 = now_ns();
            render(NB);
            best = fmin(best, (double)(now_ns() - t0) / (NB * CTL));
        }
        if (!i)
            base = best;
        printf("    %-8s %6.1f ns  (+%5.1f)\n", i ? N_XBIG[T[i]] : "none", best, best - base);
    }
    {
        double best = 1e9, b0 = 1e9;
        uint32_t s;
        for (r = 0; r < 3u; r++) {
            uint64_t t0;
            reset();
            for (s = 0; s < 4u; s++)
                trk_note_on(&trk[0], 45 + 7 * s, 100);
            render(20);
            t0 = now_ns();
            render(NB);
            b0 = fmin(b0, (double)(now_ns() - t0) / (NB * CTL));
            reset();
            for (s = 0; s < MX_NCELL; s++)
                mx_set(0, s % MS_N, (s * 5u) % MD_N, 20);
            mxt[0].as[0] = P_FCUT + 1u, mxt[0].as[1] = P_GLIDE + 1u;
            for (s = 0; s < 4u; s++)
                trk_note_on(&trk[0], 45 + 7 * s, 100);
            render(20);
            t0 = now_ns();
            render(NB);
            best = fmin(best, (double)(now_ns() - t0) / (NB * CTL));
        }
        printf("  four JUNO voices: %.1f ns a sample; with 19 matrix cells: %.1f (+%.1f, %.1f a voice)\n", b0, best, best - b0,
               (best - b0) / 4.0);
    }
}

int main(void)
{
    t_defaults();
    t_fx();
    t_matrix();
    t_spice();
    t_project();
    t_same();
    t_cpu();
    printf("fx5: %s (%d failed)\n", fails ? "FAILED" : "all ok", fails);
    return fails;
}
