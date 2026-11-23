/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Host test of the project formats (firmware/src/project.c, -DPROJ_HOST part). Format 8 ("FUN8", PHEBUS wave 2: the
 * drum trigs) is written: its round trip, its checks, and format 7 converted into it are at the end. Before them, every
 * older format into format 7 (the chain format 8 reads them through). Format 5 ("FUN5",
 * SLOOP 2.4: format 4 + per-step nudges, parameter locks and fill conditions); format 4 ("FUN4", SLOOP 2.0:
 * 10-byte steps with levels and ratchets, the drum track's 16 lanes, P_CHORD), format 3 ("FUN3", SLOOP
 * 1.x), format 2 ("FUN2", 53 parameters per track) and format 1 ("FUN1"), built byte for byte as the
 * firmware stored them, convert: every old value at its parameter, the parameters added since at their
 * defaults, the swings onto the MPC scale (x 0.8), synth steps as they were, the drum track's notes onto
 * its lanes (accent: hard), globals, selection, the engine bytes (kept; the drum track's 0), no nudge, no
 * lock, no fill condition; damaged ones are refused. Run by tests/run_tests.sh (needs build/gen). */
#define main hostsim_main
#include <stddef.h>
#include "hostsim.c"
#undef main
#define PROJ_HOST 1
static uint32_t trk_def_engine(uint32_t i)       /* ui.c TRK_DEF: ANALOG, DIGITAL, LOFI */
{
    static const uint8_t E[NPART] = {0, 1, 3};
    return i < NPART ? E[i] : 0u;
}
#include "../firmware/src/project.c"
#define ST_BIG_MAX_HOST (2u * 4096u - 256u)              /* (storage.c ST_BIG_MAX) */
static project_t q2v8_tmp;
static uint32_t dstep_mask(const dstep_t *s) { return (uint32_t)s->on[0] | (uint32_t)s->on[1] << 8; }   /* (format 7's) */

static int check(const char *what, int ok)
{
    printf("%-66s %s\n", what, ok ? "ok" : "FAIL");
    return ok ? 0 : 1;
}

/* the value parameter k (old id) of track t had in the old project */
static int16_t oldv(uint32_t t, uint32_t k) { return (int16_t)(t * 100u + k * 3u + 1u); }

static const uint8_t OLD_ENG[NTRK] = {7, 0, 6, 8};   /* WHEEL, ANALOG, TRIO; the drum track: 8 (none) */
static void fill_old_steps(step8_t *st, uint32_t t)
{
    uint32_t k;
    for (k = 0; k < NSTEP; k++) {
        step8_t *s = &st[k];
        s->note[0] = (uint8_t)(36u + (k + t) % 40u);
        s->note[1] = (uint8_t)(38u + k % 5u);
        s->n = (uint8_t)(k % 3u);
        s->time = (uint8_t)(k % 3u);
        s->flags = (uint8_t)(k & 3u);
        s->vel = (uint8_t)(64u + t);
    }
}
static void fill_v2_track(proj_trk_v2_t *d, uint32_t t)
{
    uint32_t k;
    for (k = 0; k < PROJ_NP_V2; k++)
        d->p[k] = oldv(t, k);
    d->engine = OLD_ENG[t];
    d->preset = (uint8_t)(t + 5u);
    fill_old_steps(d->step, t);
}
static void fill_v3_track(proj_trk_v3_t *d, uint32_t t)
{
    uint32_t k;
    for (k = 0; k < PROJ_NP_V3; k++)
        d->p[k] = oldv(t, k);
    d->p[P_SSWING] = 50;                           /* (swings: within 0..100) */
    d->p[P_ASWING] = 100;
    d->engine = OLD_ENG[t];
    d->preset = (uint8_t)(t + 5u);
    fill_old_steps(d->step, t);
}

/* the steps of a converted track against the old ones: synth as they were, drums onto lanes */
static int steps_ok(const proj_trk_t *n, const step8_t *o, int drum)
{
    uint32_t k, i;
    for (k = 0; k < NSTEP; k++) {
        if (drum) {
            const dstep_t *d = &n->dstep[k];
            uint32_t want = 0;
            if (o[k].time == ST_NOTE)
                for (i = 0; i < o[k].n; i++)
                    want |= 1u << lane_of_note(o[k].note[i]);
            if (dstep_mask(d) != want)
                return 0;
            for (i = 0; i < DRUM_LANES; i++)
                if ((want >> i) & 1u && dstep_lvl(d, i) != ((o[k].flags & SF_ACCENT) ? LV_HARD : vel_lvl(o[k].vel)))
                    return 0;
        } else {
            const step_t *s = &n->step[k];
            if (memcmp(s->note, o[k].note, 4) || s->n != o[k].n || s->time != o[k].time || s->flags != o[k].flags ||
                s->vel != o[k].vel || s->lvl || s->rat)
                return 0;
        }
    }
    return 1;
}

/* track t converted from format 2 / 1 has the old values where they belong */
static int track_ok_v2(const proj_trk_t *n, const proj_trk_v2_t *o, uint32_t t)
{
    uint32_t k;
    int ok = (t == TRK_DRUM ? n->engine == 0 && n->preset == 0 : n->engine == o->engine && n->preset == o->preset) &&
             steps_ok(n, o->step, t == TRK_DRUM);
    for (k = 0; k <= P_DETUNE; k++)
        if (k != P_SSWING && k != P_ASWING)
            ok &= n->p[k] == oldv(t, k);
    ok &= n->p[P_SLCR] == 0 && n->p[P_SLPAT] == TP[P_SLPAT].def && n->p[P_SLRATE] == TP[P_SLRATE].def &&
          n->p[P_SLDEPTH] == TP[P_SLDEPTH].def && n->p[P_CHORD] == 0;
    for (k = 0; k < 8u; k++)
        ok &= pt_get(n, P_E0 + k) == oldv(t, 45u + k);
    for (k = P_ITYPE; k < P_E0; k++)
        ok &= pt_get(n, k) == TP[k].def;
    return ok;
}

int main(void)
{
    static project_v3_t v3;
    static project_v2_t v2;
    static project_v1_t v1;
    static project_v4_t v4;
    static project_v7_t q, q2;
    static project_v5_t v5;
    static union {
        project_v7_t v6;
        project_v5_t v5;
        project_v4_t v4;
        project_v3_t v3;
        project_v2_t v2;
        project_v1_t v1;
    } buf;
    uint32_t i, t;
    int bad = 0, ok;

    bad += check("layout: P_CHORD, P_TFLT, P_STRUM, P_VLEAD, then the ten of 2.6 just before P_E0 (63), P_COUNT_V8 = format 5's + 10; wave 4's after P_E7",
                 P_CHORD + 1 == P_TFLT && P_TFLT + 1 == P_STRUM && P_STRUM + 1 == P_VLEAD && P_VLEAD + 1 == P_ITYPE &&
                 P_LPOL + 1 == P_E0 && P_E0 == 63 && P_COUNT_V8 == PROJ_NP_V5 + 10u && P_FCUT == P_E7 + 1 && P_COUNT <= PROJ_NP && PROJ_NP_V5 == PROJ_NP_V4 + 3u &&
                 PROJ_NP_V4 == PROJ_NP_V3 + 1u && P_SLDEPTH + 1 == P_CHORD);
    bad += check("format 7 as stored: 3840 bytes; format 8: a core 5744, a whole project 6768 (a big flash object: 7936)",
                 sizeof(project_v7_t) == 3840u && sizeof(proj_core_t) == 5744u && sizeof(project_t) == 6768u &&
                 sizeof(project_t) <= ST_BIG_MAX_HOST);
    {   /* every 2.6 parameter's range fits its byte */
        uint32_t k, fits = 1;
        for (k = P_ITYPE; k < P_E0; k++) fits &= TP[k].min >= 0 && TP[k].max <= 255;
        bad += check("format 6: the 2.6 parameters fit a byte each (0..255)", fits);
    }

    /* format 3 (SLOOP 1.x) */
    memset(&v3, 0, sizeof v3);
    v3.magic = PROJ_MAGIC_V3;
    v3.size = sizeof v3;
    for (i = 0; i < PROJ_NG_V3; i++)
        v3.g[i] = (int16_t)(300 + i);
    v3.g[G_SWING] = 50;
    v3.sel = 3;
    for (t = 0; t < NTRK; t++)
        fill_v3_track(&v3.t[t], t);
    v3.sum = proj_hash(&v3, sizeof v3 - 4u);
    memcpy(&buf, &v3, sizeof v3);
    ok = proj_import_v7(&q, &buf, (int)sizeof v3);
    bad += check("FUN3 -> FUN7: converted, valid format 6 slot", ok && proj_ok_v7(&q) && q.magic == PROJ_MAGIC_V7);
    ok = q.sel == 3 && q.g[G_SWING] == 40;
    for (i = 0; i < PROJ_NG_V3; i++)
        ok &= i == G_SWING || q.g[i] == (int16_t)(300 + i);
    for (i = PROJ_NG_V3; i < PROJ_NG; i++)
        ok &= q.g[i] == GP[i].def;
    ok &= q.drdly == 0;
    bad += check("FUN3 -> FUN7: globals (swing 50 -> 40: the MPC scale), the new ones default", ok);
    ok = 1;
    for (t = 0; t < NTRK; t++) {
        const proj_trk_t *n = &q.t[t];
        uint32_t k;
        ok &= (t == TRK_DRUM ? n->engine == 0 : n->engine == OLD_ENG[t]) && steps_ok(n, v3.t[t].step, t == TRK_DRUM);
        for (k = 0; k < PROJ_NP_V3 - 8u; k++)
            if (k != P_SSWING && k != P_ASWING)
                ok &= n->p[k] == oldv(t, k);
        ok &= n->p[P_SSWING] == 40 && n->p[P_ASWING] == 80 && n->p[P_CHORD] == 0;
        for (k = 0; k < 8u; k++)
            ok &= pt_get(n, P_E0 + k) == oldv(t, PROJ_NP_V3 - 8u + k);
        for (k = 0; k < NLOCK; k++)
            ok &= n->lock[k].step == LOCK_FREE;
        for (k = 0; k < NSTEP; k++)
            ok &= n->mf[k] == 0;
        for (k = P_ITYPE; k < P_E0; k++)
            ok &= pt_get(n, k) == TP[k].def;
    }
    bad += check("FUN3 -> FUN7: parameters (P_E0.. moved), steps, drum notes -> lanes, no lock, no fill", ok);

    /* format 2, as written before the SLICER */
    memset(&v2, 0, sizeof v2);
    v2.magic = PROJ_MAGIC_V2;
    v2.size = sizeof v2;
    for (i = 0; i < PROJ_NG_V2; i++)
        v2.g[i] = (int16_t)(500 + i);
    v2.sel = 2;
    for (t = 0; t < NTRK; t++)
        fill_v2_track(&v2.t[t], t);
    v2.sum = proj_hash(&v2, sizeof v2 - 4u);
    bad += check("FUN2 image is 2552 bytes (as stored)", sizeof v2 == 2552u);
    memcpy(&buf, &v2, sizeof v2);
    ok = proj_import_v7(&q, &buf, (int)sizeof v2);
    bad += check("FUN2 -> FUN7: converted, valid format 6 slot", ok && proj_ok_v7(&q) && q.magic == PROJ_MAGIC_V7);
    ok = q.sel == 2;
    for (i = 0; i < PROJ_NG_V2; i++)
        ok &= i == G_SWING || q.g[i] == (int16_t)(500 + i);
    bad += check("FUN2 -> FUN7: globals and selected track", ok);
    ok = 1;
    for (t = 0; t < NTRK; t++)
        ok &= track_ok_v2(&q.t[t], &v2.t[t], t);
    bad += check("FUN2 -> FUN7: every parameter mapped, SLICER OFF, CHORD OFF (4 tracks)", ok);
    bad += check("FUN2 -> FUN7: engine bytes kept (old WHEEL 7, ANALOG 0, TRIO 6), drum 0",
                 q.t[0].engine == 7 && q.t[1].engine == 0 && q.t[2].engine == 6 && q.t[3].engine == 0);
    {   /* (2026-10-19: those engines are gone: each track its own) */
        static project_t q8;
        proj_from_v7(&q8, &q);
        proj_apply(&q8.c, 1);
    }
    bad += check("FUN2 applied: a gone engine -> the track's own (ACID, ELEMENTS, PLAITS)",
                 trk[0].eng_req == ENGI_ACID && trk[1].eng_req == ENGI_ELEM && trk[2].eng_req == ENGI_PRISM);

    /* a FUN4 image (SLOOP 2.0 .. 2.3, as stored) -> FUN5: the same, zero nudges, every lock free */
    memset(&v4, 0, sizeof v4);
    v4.magic = PROJ_MAGIC_V4;
    v4.size = sizeof v4;
    memcpy(v4.g, q.g, sizeof v4.g);
    v4.sel = 1;
    for (t = 0; t < NTRK; t++) {
        for (i = 0; i < PROJ_NP_V4; i++)               /* (format 4's ids: P_E0 was P_TFLT) */
            v4.t[t].p[i] = i < P_TFLT ? q.t[t].p[i] : pt_get(&q.t[t], P_E0 + i - P_TFLT);
        v4.t[t].engine = q.t[t].engine;
        v4.t[t].preset = q.t[t].preset;
        memcpy(v4.t[t].step, q.t[t].step, sizeof v4.t[t].step);
    }
    v4.t[1].engine = 8;
    v4.t[0].step[3].lvl = 0x9C;
    v4.t[0].step[3].rat = 0x27;
    dstep_set(&v4.t[TRK_DRUM].dstep[5], 13, LV_GHOST, 2);
    v4.sum = proj_hash(&v4, sizeof v4 - 4u);
    bad += check("FUN4 image is 3112 bytes (as stored)", sizeof v4 == 3112u);
    memcpy(&buf, &v4, sizeof v4);
    ok = proj_import_v7(&q2, &buf, (int)sizeof v4) && proj_ok_v7(&q2) && q2.magic == PROJ_MAGIC_V7 && q2.sel == 1 && q2.t[1].engine == 8 &&
         q2.t[0].step[3].lvl == 0x9C && q2.t[0].step[3].rat == 0x27 && dstep_lvl(&q2.t[TRK_DRUM].dstep[5], 13) == LV_GHOST &&
         dstep_rat(&q2.t[TRK_DRUM].dstep[5], 13) == 2u && !memcmp(q2.g, v4.g, sizeof q2.g);
    for (t = 0; t < NTRK; t++) {
        ok &= !memcmp(q2.t[t].p, v4.t[t].p, P_TFLT * 2u) && !memcmp(q2.t[t].e, v4.t[t].p + P_TFLT, 16u) &&
              q2.t[t].p[P_TFLT] == 0 && q2.t[t].p[P_STRUM] == 0 && q2.t[t].p[P_VLEAD] == 0 &&
              !memcmp(q2.t[t].step, v4.t[t].step, sizeof q2.t[t].step);
        for (i = 0; i < NLOCK; i++)
            ok &= q2.t[t].lock[i].step == LOCK_FREE;
        for (i = 0; i < NSTEP; i++)
            ok &= q2.t[t].mf[i] == 0;
        for (i = P_ITYPE; i < P_E0; i++)
            ok &= pt_get(&q2.t[t], i) == TP[i].def;
    }
    bad += check("FUN4 -> FUN7: as stored (levels, ratchets, lanes, engine 8; P_E0.. moved, FILTER off), zero nudges, locks free, no fill condition", ok);
    v4.t[2].step[1].vel ^= 1u;
    memcpy(&buf, &v4, sizeof v4);
    bad += check("FUN4 with a bad checksum: refused", !proj_import_v7(&q2, &buf, (int)sizeof v4));
    v4.t[2].step[1].vel ^= 1u;

    /* a FUN5 image (SLOOP 2.4 .. 2.5, as stored) -> FUN6: by id, the 2.6 ten at their defaults, the nudges and
     * conditions into a byte a step, a lock on P_E0.. moved with it (format 5's 53.. -> 63..) */
    memset(&v5, 0, sizeof v5);
    v5.magic = PROJ_MAGIC_V5;
    v5.size = sizeof v5;
    memcpy(v5.g, q.g, sizeof v5.g);
    v5.sel = 2;
    v5.drdly = 55;
    for (t = 0; t < NTRK; t++) {
        for (i = 0; i < PROJ_NP_V5; i++)
            v5.t[t].p[i] = i < PROJ_NP_V5 - 8u ? (int16_t)(t * 10 + i) : (int16_t)(100 + t * 10 + i);
        v5.t[t].engine = (uint8_t)t;
        v5.t[t].preset = (uint8_t)(3 + t);
        memcpy(v5.t[t].step, q.t[t].step, sizeof v5.t[t].step);
        for (i = 0; i < NLOCK; i++)
            v5.t[t].lock[i].step = LOCK_FREE;
    }
    v5.t[0].micro[3] = -32, v5.t[0].micro[4] = 31, v5.t[TRK_DRUM].micro[9] = -7;
    v5.t[0].fill[0] = 0x09;                        /* steps 1 and 2: FILL ONLY, NO FILL */
    v5.t[TRK_DRUM].fill[15] = 0x40;                /* step 64: FILL ONLY */
    v5.t[1].lock[0].step = 3, v5.t[1].lock[0].param = 53 + 1, v5.t[1].lock[0].val = 9;   /* format 5's P_E1 */
    v5.t[1].lock[1].step = 4, v5.t[1].lock[1].param = P_DIST, v5.t[1].lock[1].val = 100;
    v5.sum = proj_hash(&v5, sizeof v5 - 4u);
    memcpy(&buf, &v5, sizeof v5);
    ok = proj_import_v7(&q2, &buf, (int)sizeof v5) && proj_ok_v7(&q2) && q2.magic == PROJ_MAGIC_V7 && q2.sel == 2 && q2.drdly == 55 &&
         !memcmp(q2.g, v5.g, sizeof q2.g);
    for (t = 0; t < NTRK; t++) {
        for (i = 0; i < PROJ_NP_V5 - 8u; i++)
            ok &= pt_get(&q2.t[t], i) == (int16_t)(t * 10 + i);
        for (i = 0; i < 8u; i++)
            ok &= pt_get(&q2.t[t], P_E0 + i) == (int16_t)(100 + t * 10 + PROJ_NP_V5 - 8u + i);
        for (i = P_ITYPE; i < P_E0; i++)
            ok &= pt_get(&q2.t[t], i) == TP[i].def;
        ok &= q2.t[t].engine == t && q2.t[t].preset == 3 + t && !memcmp(q2.t[t].step, v5.t[t].step, sizeof v5.t[t].step);
    }
    ok &= pt_micro(&q2.t[0], 3) == -32 && pt_micro(&q2.t[0], 4) == 31 && pt_micro(&q2.t[TRK_DRUM], 9) == -7 &&
          pt_micro(&q2.t[0], 5) == 0 && pt_fill(&q2.t[0], 0) == FC_FILL && pt_fill(&q2.t[0], 1) == FC_NOFILL &&
          pt_fill(&q2.t[0], 2) == FC_NORM && pt_fill(&q2.t[TRK_DRUM], 63) == FC_FILL &&
          q2.t[1].lock[0].param == P_E1 && q2.t[1].lock[0].val == 9 && q2.t[1].lock[1].param == P_DIST &&
          q2.t[1].lock[2].step == LOCK_FREE;
    bad += check("FUN5 -> FUN7: by id, the 2.6 parameters default, nudges and conditions kept, a lock on E2 moved with it", ok);
    v5.t[3].micro[0] ^= 1;
    memcpy(&buf, &v5, sizeof v5);
    bad += check("FUN5 with a bad checksum: refused", !proj_import_v7(&q2, &buf, (int)sizeof v5));

    /* a FUN6 round trip: stored as is (nudges, locks, the 2.6 parameters, an engine added since: 8) */
    q.t[1].engine = 8;
    q.t[0].step[3].lvl = 0x9C;
    q.t[0].step[3].rat = 0x27;
    dstep_set(&q.t[TRK_DRUM].dstep[5], 13, LV_GHOST, 2);
    pt_set_mf(&q.t[0], 3, -32, FC_FILL);
    pt_set_mf(&q.t[0], 4, 31, FC_NOFILL);
    pt_set_mf(&q.t[TRK_DRUM], 9, -7, 0);
    q.t[0].lock[0].step = 3, q.t[0].lock[0].param = P_E0, q.t[0].lock[0].val = 2;
    q.t[0].lock[1].step = 3, q.t[0].lock[1].param = P_DIST, q.t[0].lock[1].val = 100;
    q.t[2].lock[23].step = 63, q.t[2].lock[23].param = P_LEVEL, q.t[2].lock[23].val = 50;
    q.t[2].p[P_TFLT] = -30;                        /* a track FILTER (2.4) */
    pt_set(&q.t[1], P_ITYPE, 4), pt_set(&q.t[1], P_IMIX, 90), pt_set(&q.t[TRK_DRUM], P_ITYPE, 2);
    pt_set(&q.t[0], P_LSYNC, 5), pt_set(&q.t[0], P_ESYNC, 1), pt_set(&q.t[0], P_FTYPE, 2);
    q.sum = proj_sum_v7(&q);
    memcpy(&buf, &q, sizeof q);
    bad += check("FUN7 -> FUN7: as stored (levels, ratchets, lanes, nudges, locks, fill conditions, INSERT, SYNC, engine 8)",
                 proj_import_v7(&q2, &buf, (int)sizeof q) && !memcmp(&q, &q2, sizeof q) && q2.t[1].engine == 8 &&
                 pt_micro(&q2.t[0], 3) == -32 && pt_fill(&q2.t[0], 3) == FC_FILL && pt_micro(&q2.t[0], 4) == 31 &&
                 q2.t[0].lock[1].val == 100 && q2.t[2].lock[23].step == 63 && q2.t[2].p[P_TFLT] == -30 &&
                 pt_get(&q2.t[1], P_ITYPE) == 4 && pt_get(&q2.t[1], P_IMIX) == 90 && pt_get(&q2.t[TRK_DRUM], P_ITYPE) == 2 &&
                 pt_get(&q2.t[0], P_LSYNC) == 5 && pt_get(&q2.t[0], P_ESYNC) == 1 && pt_get(&q2.t[0], P_FTYPE) == 2);

    {   /* a FUN6 image (SLOOP 2.6, as stored: format 7 without the CLOUD's bytes) -> FUN7: as it was, the cloud unset */
        static project_v6_t v6;
        memcpy(&v6, &q, __builtin_offsetof(project_v6_t, sum));
        v6.magic = PROJ_MAGIC_V6;
        v6.size = sizeof v6;
        v6.sum = proj_hash(&v6, sizeof v6 - 4u);
        memcpy(&buf, &v6, sizeof v6);
        ok = proj_import_v7(&q2, &buf, (int)sizeof v6) && proj_ok_v7(&q2) && q2.magic == PROJ_MAGIC_V7 && q2.t[1].engine == 8 &&
             !memcmp(q2.t, q.t, sizeof q.t) && !memcmp(q2.g, q.g, sizeof q.g) && q2.sel == q.sel && !(q2.cloud[6] & 0x80u);
        bad += check("FUN6 -> FUN7: every track and global as stored, the CLOUD's bytes unset", ok);
        v6.t[2].mf[5] ^= 1;
        memcpy(&buf, &v6, sizeof v6);
        bad += check("FUN6 with a bad checksum: refused", !proj_import_v7(&q2, &buf, (int)sizeof v6));
    }

    /* damaged / wrong size */
    v2.t[1].p[3]++;
    memcpy(&buf, &v2, sizeof v2);
    bad += check("FUN2 with a bad checksum: refused", !proj_import_v7(&q2, &buf, (int)sizeof v2));
    v2.t[1].p[3]--;
    memcpy(&buf, &v2, sizeof v2);
    bad += check("FUN2 with a wrong length: refused", !proj_import_v7(&q2, &buf, (int)sizeof v2 - 2));
    memcpy(&buf, &q, sizeof q);
    buf.v4.magic = PROJ_MAGIC_V3;
    bad += check("FUN6 size with a FUN3 magic: refused", !proj_import_v7(&q2, &buf, (int)sizeof q));
    memcpy(&buf, &q, sizeof q);
    buf.v6.magic = PROJ_MAGIC_V5;
    bad += check("FUN6 size with a FUN5 magic: refused", !proj_import_v7(&q2, &buf, (int)sizeof q));
    memcpy(&buf, &v3, sizeof v3);
    buf.v3.t[2].step[7].vel ^= 1u;
    bad += check("FUN3 with a bad checksum: refused", !proj_import_v7(&q2, &buf, (int)sizeof v3));

    /* format 1: one instrument -> track 1, the others their defaults */
    memset(&v1, 0, sizeof v1);
    v1.magic = PROJ_MAGIC_V1;
    v1.size = sizeof v1;
    for (i = 0; i < PROJ_NG_V2; i++)
        v1.g[i] = (int16_t)(700 + i);
    fill_v2_track(&v1.t, 0);
    v1.sum = proj_hash(&v1, sizeof v1 - 4u);
    memcpy(&buf, &v1, sizeof v1);
    ok = proj_import_v7(&q, &buf, (int)sizeof v1) && proj_ok_v7(&q) && track_ok_v2(&q.t[0], &v1.t, 0) && q.g[5] == 705;
    for (t = 1; t < NTRK; t++)
        ok &= q.t[t].preset == 0xFF && q.t[t].p[P_SLCR] == 0 && q.t[t].p[P_LEVEL] == TP[P_LEVEL].def &&
              pt_get(&q.t[t], P_E0) == ENGINES[trk_def_engine(t)]->edit[0].def && q.t[t].lock[0].step == LOCK_FREE &&
              (t == TRK_DRUM ? dstep_mask(&q.t[t].dstep[0]) == 0u : q.t[t].step[0].time == ST_REST);
    bad += check("FUN1 -> FUN7: track 1 mapped, tracks 2..4 defaults", ok);

    /* ---- format 8 (PHEBUS wave 2) */
    {   /* capture / apply: the working project round trip; the drum track's pattern: every sound's trigs at their velocities,
         * their extras (nudge, condition and its chance, retrig), their sounds' locks; the drum track's own locks; each
         * sound's sends, the STUTTER lane; the synth tracks as before (nudges, locks, conditions: CHNC kept now) */
        static project_t a, b;
        int lk;
        dtx_t *x;
        host_tracks_init();
        for (t = 0; t < NTRK; t++)
            trk[t].p[P_SLEN] = (int16_t)(5 + t);
        trk[1].step[2].n = 2, trk[1].step[2].note[0] = 60, trk[1].step[2].note[1] = 64, trk[1].step[2].time = ST_NOTE;
        trk[1].step[2].lvl = 0x0D;
        trk[1].micro[2] = -20;
        lock_set(&trk[1], 2, P_ED_FLT, -30);
        lock_set(&trk[1], 2, P_E1, 5);
        step_fill_set(&trk[1], 2, FC_FILL);
        step_fill_set(&trk[1], 7, FC_CHANCE);
        trk[2].p[P_ITYPE] = 7, trk[2].p[P_IA] = 11, trk[2].p[P_IMIX] = 77, TDRUM->p[P_ITYPE] = 4;   /* 2.6 */
        trk[0].p[P_LSYNC] = 6, trk[0].p[P_LTRIG] = 1, trk[0].p[P_LPOL] = 1, trk[0].p[P_ESYNC] = 1, trk[0].p[P_FTYPE] = 1;
        song.g[G_DUST] = 33;
        song.g[G_DRDLY] = 77;
        kit_set((uint32_t)(12));
        dt_set(9, 4, 72);                              /* a hat at 72 on step 10, nudged +12, CHNC 33 %, retrig 1/32 */
        x = dt_x_make(9, 4);
        x->micro = 12, x->cond = FC_CHANCE, x->prob = 33, x->rrate = 13, x->rlen = 24, x->rfade = -100;
        dt_set(9, 0, 127);                             /* a kick at 127 on the same step, plain */
        dt_set(63, 15, 1);                             /* a cowbell at 1 on step 64 */
        dt_lk_set(9, 4, DSP_CUT, 30);                  /* the hat's cutoff and fine (signed) on step 10 */
        dt_lk_set(9, 4, DSP_FINE, (uint8_t)(int8_t)-20);
        lock_set(TDRUM, 9, P_DIST, 64);                /* the drum track's own (its bus) */
        sl_send_set(3, 0, 11), sl_send_set(3, 1, 6);
        memset(&stl, 0, sizeof stl);
        stl.len = 32, stl.act = 0, stl.on[1] = 0x81, stl.lc[3] = 0xE4, stl.lx[1] = 0x01;
        proj_capture(&a);
        bad += check("format 8: captured: \"FUN8\", the core's size and sum good, the ext the kit's 16 slots (wave 12), then zero",
                     a.c.magic == 0x46554E38u && a.c.size == 5744u && proj_ok(&a.c) && ((pkit_t *)a.x.kit)->magic == PK_MAGIC &&
                     a.x.kit[sizeof(pkit_t)] == 0 && a.x.kit[1023] == 0);
        host_tracks_init();
        memset(&stl, 0, sizeof stl);
        song.g[G_DRDLY] = 0;
        proj_kit_take((const uint8_t *)&a, (int)sizeof a, &a.c);
        proj_apply(&a.c, 1);
        lk = dt_lk(9, 4, DSP_FINE);
        ok = song.g[G_DRDLY] == 0 && sl_send(3, 0) == 11u && sl_send(3, 1) == 6u && trk[2].p[P_SLEN] == 7 && trk[1].step[2].n == 2 && trk[1].step[2].lvl == 0x0D &&
             song.g[G_DUST] == 33 && trk[1].micro[2] == -20 && trk[1].micro[3] == 0 &&
             lock_find(&trk[1], 2, P_ED_FLT, 0) >= 0 && trk[1].lock[lock_find(&trk[1], 2, P_ED_FLT, 0)].val == -30 &&
             lock_find(&trk[1], 2, P_E1, 0) >= 0 && step_fill(&trk[1], 2) == FC_FILL && step_fill(&trk[1], 7) == FC_CHANCE &&
             step_fill(&trk[1], 3) == FC_NORM && trk[2].p[P_ITYPE] == 7 && trk[2].p[P_IA] == 11 && trk[2].p[P_IMIX] == 77 &&
             TDRUM->p[P_ITYPE] == 4 && trk[0].p[P_LSYNC] == 6 && trk[0].p[P_LTRIG] == 1 && trk[0].p[P_LPOL] == 1 &&
             trk[0].p[P_ESYNC] == 1 && trk[0].p[P_FTYPE] == 1;
        bad += check("format 8: the synth tracks and globals round trip (nudges, locks, conditions with CHNC, 2.6 parameters; the drum DLY: 0, the kit-wide sends gone, a sound's own kept)", ok);
        ok = TDRUM->p[P_E0] == 12 && pkit.base == 12u && dt_vel(9, 4) == 72u && dt_vel(9, 0) == 127u && dt_vel(63, 15) == 1u && dt_mask(10) == 0u &&
             dt_x(9, 4) >= 0 && dt_x(9, 0) < 0 && dpat.x[dt_x(9, 4)].micro == 12 && dpat.x[dt_x(9, 4)].cond == FC_CHANCE &&
             dpat.x[dt_x(9, 4)].prob == 33 && dpat.x[dt_x(9, 4)].rrate == 13 && dpat.x[dt_x(9, 4)].rlen == 24 &&
             dpat.x[dt_x(9, 4)].rfade == -100 && dt_lk(9, 4, DSP_CUT) >= 0 && dpat.lk[dt_lk(9, 4, DSP_CUT)].val == 30 &&
             lk >= 0 && (int8_t)dpat.lk[lk].val == -20 && dt_lk(9, 0, DSP_CUT) < 0 && lock_find(TDRUM, 9, P_DIST, 0) >= 0 &&
             !TDRUM->micro[9] && !step_fill(TDRUM, 9) && sl_send(3, 0) == 11u && sl_send(3, 1) == 6u &&
             stl.len == 32 && !stl.act && stl.on[1] == 0x81 && stl.lc[3] == 0xE4 && stl.lx[1] == 0x01;
        bad += check("format 8: the drum pattern round trips: velocities, extras (nudge, CHNC 33 %, retrig 1/32 x1.5 curve -100), "
                     "the sound's locks (signed too), the bus lock, the kit, the sends, the STUTTER lane", ok);
        proj_capture(&a);                              /* (as a load leaves it: every value in its range) */
        memset(&dpat, 0, sizeof dpat);
        proj_kit_take((const uint8_t *)&a, (int)sizeof a, &a.c);
        proj_apply(&a.c, 1);
        proj_capture(&b);
        bad += check("format 8: loaded and saved again: the same bytes", !memcmp(&a, &b, sizeof a));
        b = a;
        b.c.d.pat.vel[9][4] ^= 1u;
        bad += check("format 8: a changed byte breaks the sum (refused)", !proj_import(&q2v8_tmp.c, &b, (int)sizeof b, PROJ_V7_TMP));
        q2v8_tmp.c.magic = 0;
        bad += check("format 8: a slot (the core alone, 5744 bytes) and a whole project (6768) both load (its core); 0 bytes: none",
                     proj_import(&q2v8_tmp.c, &a.c, (int)sizeof a.c, PROJ_V7_TMP) && !memcmp(&q2v8_tmp.c, &a.c, sizeof a.c) &&
                     (memset(&q2v8_tmp, 0, sizeof q2v8_tmp), proj_import(&q2v8_tmp.c, &a, (int)sizeof a, PROJ_V7_TMP)) &&
                     !memcmp(&q2v8_tmp.c, &a.c, sizeof a.c) && !proj_import(&q2v8_tmp.c, &a, 0, PROJ_V7_TMP) &&
                     !proj_import(&q2v8_tmp.c, &a, (int)sizeof a - 2, PROJ_V7_TMP));
        b = a;
        bad += check("format 8: checked where it is (the core imported onto itself)", proj_import(&b.c, &b, (int)sizeof b, PROJ_V7_TMP) &&
                     !memcmp(&b, &a, sizeof a));
        a.c.d.pat.vel[9][4] = 0;                       /* the hat gone, its extras and locks left behind */
        a.c.d.pat.lk[100].step = 3, a.c.d.pat.lk[100].lane = 0, a.c.d.pat.lk[100].param = 200;   /* no such parameter */
        a.c.d.pat.vel[3][0] = 200;                     /* a velocity past 127 */
        a.c.d.pat.x[60].step = 3, a.c.d.pat.x[60].lane = 0, a.c.d.pat.x[60].prob = 0, a.c.d.pat.x[60].micro = 90;
        a.c.d.pat.x[60].rrate = 99, a.c.d.pat.x[60].cond = 7;
        a.c.sum = proj_sum(&a.c);
        proj_apply(&a.c, 1);
        x = dt_x(3, 0) >= 0 ? &dpat.x[dt_x(3, 0)] : 0;
        bad += check("format 8: apply checks the pattern: extras and locks of a trig gone freed, a parameter past the list "
                     "freed, a velocity masked, a chance, nudge, rate and condition put back in range",
                     dt_x(9, 4) < 0 && dt_lk(9, 4, DSP_CUT) < 0 && dt_lk(3, 0, 200) < 0 && dt_vel(3, 0) == (200u & 127u) &&
                     x && x->prob == 1 && x->micro == MICRO_MAX && x->rrate == 0 && x->cond == 3u);
        memset(&stl, 0, sizeof stl);
        stl.len = 16, stl.act = 1;
    }
    {   /* format 7 (KARMA, PHEBUS before wave 2) -> format 8. The drum steps' lanes become trigs at their levels' velocities
         * (ghost 42, soft 72, normal 100, hard 127); a step's nudge and condition go to every trig on it (CHNC: its chance
         * 50 %); a ratchet becomes a retrig of one step at the rate nearest (1/16 steps: x2 1/32, x3 1/48, x4 1/64); the
         * drum track's locks stay its own; each sound's sends and the STUTTER lane move out of the drum track's unused
         * bytes into their places (those bytes: the defaults); the CLOUD's and TEXTURE's bytes are ignored */
        static project_v7_t v;
        static project_t c8;
        proj_trk_t *d = &v.t[TRK_DRUM];
        uint32_t k;
        memset(&v, 0, sizeof v);
        v.magic = PROJ_MAGIC_V7, v.size = sizeof v;
        for (i = 0; i < PROJ_NG; i++)
            v.g[i] = GP[i].def;
        v.g[G_BPM] = 128, v.g[G_DRCH] = (int16_t)(101 | 27 << 7);
        v.sel = 3, v.drdly = 40, v.rsv[0] = 0x40u | 0x80u, v.cloud[6] = 0x81u;
        for (t = 0; t < NTRK; t++) {
            for (k = 0; k < P_COUNT; k++)
                pt_set(&v.t[t], k, k >= P_E0 ? 0 : TP[k].def);
            for (k = 0; k < NLOCK; k++)
                v.t[t].lock[k].step = LOCK_FREE;
            for (k = 0; k < NSTEP && t < NPART; k++)
                v.t[t].step[k].time = ST_REST;
            v.t[t].engine = t < NPART ? ENGI_JUNO : 0;
        }
        v.t[0].step[1].n = 1, v.t[0].step[1].note[0] = 48, v.t[0].step[1].time = ST_NOTE;
        pt_set_mf(&v.t[0], 1, -5, 3);                 /* a synth step's CHNC (3): kept */
        pt_set(d, P_SDIV, 2);                          /* the drum track on 1/16 */
        pt_set(d, P_E0, 7);                            /* the kit */
        dstep_set(&d->dstep[0], 0, LV_HARD, 0);
        dstep_set(&d->dstep[0], 4, LV_GHOST, 1);       /* x2 */
        dstep_set(&d->dstep[2], 2, LV_SOFT, 2);        /* x3 */
        dstep_set(&d->dstep[4], 2, LV_NORM, 3);        /* x4 */
        dstep_set(&d->dstep[6], 3, LV_NORM, 0);
        dstep_set(&d->dstep[6], 5, LV_NORM, 0);
        pt_set_mf(d, 6, -9, FC_NOFILL);                /* both trigs of step 7: nudge -9, NO FILL */
        pt_set_mf(d, 8, 4, 3);                         /* step 9: CHNC, but no trig there */
        dstep_set(&d->dstep[10], 7, LV_NORM, 0);
        pt_set_mf(d, 10, 0, 3);                        /* step 11's rim: CHNC */
        d->lock[0].step = 6, d->lock[0].param = P_DIST, d->lock[0].val = 99;   /* the drum track's own lock */
        d->e[1] = 0x7F;                                /* sound 0's sends: reverb 15, delay 7 */
        d->engine = 0x53u;                             /* the STUTTER lane, as format 7 kept it */
        d->preset = (uint8_t)(31u | 64u | 128u);
        d->p[P_LRATE] = 0x1B2C, d->p[P_GLMODE] = 0x0102, d->p[P_ATK] = 0x0500;
        v.sum = proj_sum_v7(&v);
        ok = proj_import(&c8.c, &v, (int)sizeof v, PROJ_V7_TMP) && proj_ok(&c8.c) && c8.c.magic == PROJ_MAGIC;
        bad += check("FUN7 -> FUN8: converted, a valid format 8 project", ok);
        ok = c8.c.g[G_BPM] == 128 && c8.c.sel == 3 && c8.c.drdly == 40 && c8.c.d.p[P_E0] == 7 && c8.c.d.p[P_SDIV] == 2 &&
             c8.c.t[0].step[1].note[0] == 48 && (c8.c.t[0].mf[1] >> 6) == FC_CHANCE && c8.c.t[0].engine == ENGI_JUNO;
        bad += check("FUN7 -> FUN8: globals, selection, drum DLY, the kit, the synth steps (a CHNC step kept)", ok);
        {
            const dpat_t *P = &c8.c.d.pat;
            int ok2 = P->vel[0][0] == 127 && P->vel[0][4] == 42 && P->vel[2][2] == 72 && P->vel[4][2] == 100 &&
                      P->vel[6][3] == 100 && P->vel[6][5] == 100 && P->vel[10][7] == 100 && P->vel[8][0] == 0;
            uint32_t found = 0;
            for (k = 0; k < DT_NX; k++) {
                const dtx_t *e = &P->x[k];
                if (e->step == DT_FREE)
                    continue;
                found++;
                if (e->step == 0 && e->lane == 4) ok2 &= e->rrate == 13 && e->rlen == 16 && !e->micro && !e->cond;   /* 1/32 */
                else if (e->step == 2 && e->lane == 2) ok2 &= e->rrate == 15;                                        /* 1/48 */
                else if (e->step == 4 && e->lane == 2) ok2 &= e->rrate == 16;                                        /* 1/64 */
                else if (e->step == 6) ok2 &= e->micro == -9 && e->cond == FC_NOFILL && !e->rrate;
                else if (e->step == 10 && e->lane == 7) ok2 &= e->cond == FC_CHANCE && e->prob == 50 && !e->micro;
                else ok2 = 0;
            }
            if (!ok2 || found != 6u)
                for (k = 0; k < DT_NX; k++)
                    if (P->x[k].step != DT_FREE)
                        printf("  x %u: step %u lane %u cond %u prob %u micro %d rate %u len %u\n", k, P->x[k].step, P->x[k].lane,
                               P->x[k].cond, P->x[k].prob, P->x[k].micro, P->x[k].rrate, P->x[k].rlen);
            bad += check("FUN7 -> FUN8: drum lanes -> trigs at their levels' velocities; ratchets x2 x3 x4 -> retrigs 1/32 "
                         "1/48 1/64 of a step; a step's nudge and condition on each of its trigs, CHNC at 50 %", ok2 && found == 6u);
        }
        ok = c8.c.d.lock[0].step == 6 && c8.c.d.lock[0].param == P_DIST && c8.c.d.lock[0].val == 99 &&
             c8.c.d.dsend[0] == (15u | 7u << 4) && c8.c.d.dsend[1] == 0 &&
             c8.c.perf.len == 32 && c8.c.perf.act == 1 && c8.c.perf.lc[0] == 0x2C && c8.c.perf.lc[1] == 0x1B &&
             c8.c.perf.on[0] == 0x02 && c8.c.perf.on[1] == 0x01 && c8.c.perf.lx[1] == 0x05 &&
             c8.c.d.p[P_LRATE] == TP[P_LRATE].def && c8.c.d.p[P_GLMODE] == TP[P_GLMODE].def &&
             c8.c.d.p[P_ATK] == TP[P_ATK].def && c8.c.d.p[P_E1] == 0 && c8.x.kit[0] == 0;
        bad += check("FUN7 -> FUN8: the drum track's own lock kept, the sends and the STUTTER lane moved, their old bytes "
                     "back to the defaults", ok);
        host_tracks_init();
        song.g[G_DRCH] = 9;
        proj_apply(&c8.c, 1);
        bad += check("FUN7 -> FUN8 applied: the trigs play from dpat, nothing in the drum track's own steps; the drum channel "
                     "not taken from TEXTURE's bytes", dt_vel(0, 0) == 127u && dt_x(6, 3) >= 0 && song.g[G_DRCH] == 9 &&
                     !TDRUM->micro[6] && !step_fill(TDRUM, 6) && stl.len == 32);
        for (k = 0; k < NSTEP; k++) {                  /* 64 steps of 16 nudged sounds: more trigs with extras than fit */
            uint32_t l;
            for (l = 0; l < DRUM_LANES; l++)
                dstep_set(&d->dstep[k], l, LV_NORM, 0);
            pt_set_mf(d, k, 3, FC_NORM);
        }
        v.sum = proj_sum_v7(&v);
        ok = proj_import(&c8.c, &v, (int)sizeof v, PROJ_V7_TMP);
        {
            uint32_t l, trigs = 0, xs = 0;
            for (k = 0; k < NSTEP; k++) for (l = 0; l < DRUM_LANES; l++) trigs += c8.c.d.pat.vel[k][l] != 0;
            for (k = 0; k < DT_NX; k++) xs += c8.c.d.pat.x[k].step != DT_FREE;
            bad += check("FUN7 -> FUN8: more nudged trigs than the 64 extras: every trig kept, 64 nudges, the rest on the grid "
                         "(counted)", ok && trigs == 1024u && xs == DT_NX && proj_v7_lost == 1024u - DT_NX);
        }
        d->engine = 0;                                 /* (KARMA wrote it: no lane) */
        v.sum = proj_sum_v7(&v);
        ok = proj_import(&c8.c, &v, (int)sizeof v, PROJ_V7_TMP) && c8.c.perf.len == 16 && c8.c.perf.act == 1 && !c8.c.perf.on[0];
        d->engine = 0x53u, d->preset &= 0x7Fu;         /* (a PHEBUS lane saved before 1/2 and 1/1: no 3rd bits) */
        v.sum = proj_sum_v7(&v);
        ok &= proj_import(&c8.c, &v, (int)sizeof v, PROJ_V7_TMP) && !c8.c.perf.lx[1] && c8.c.perf.on[1] == 0x01;
        bad += check("FUN7 -> FUN8: no lane in it: an empty one, 16 steps, ON; a lane from before 1/2 and 1/1: no 3rd bits", ok);
        host_tracks_init();
    }
    {   /* real projects saved by the simulator before wave 2 (format 7 files): each loads, every drum hit a trig at its
         * level's velocity, the synth steps as stored */
        static const char *const F[3] = {"tests/op_build_pattern.fm1p", "tests/op_tour_pattern.fm1p", "tests/op_v3_pattern.fm1p"};
        static uint8_t raw[8192];
        static project_t r8;
        uint32_t f, k, l, okf = 1, hits = 0;
        for (f = 0; f < 3u; f++) {
            FILE *pf = fopen(F[f], "rb");
            const project_v7_t *o = (const project_v7_t *)raw;
            int n = pf ? (int)fread(raw, 1, sizeof raw, pf) : 0;
            if (pf)
                fclose(pf);
            if (n != (int)sizeof(project_v7_t) || !proj_ok_v7(o) || !proj_import(&r8.c, raw, n, PROJ_V7_TMP)) {
                okf = 0;
                continue;
            }
            for (k = 0; k < NSTEP; k++) {
                for (l = 0; l < DRUM_LANES; l++) {
                    uint32_t on = dstep_has(&o->t[TRK_DRUM].dstep[k], l);
                    okf &= on ? r8.c.d.pat.vel[k][l] == lvl_vel(dstep_lvl(&o->t[TRK_DRUM].dstep[k], l), 100) : !r8.c.d.pat.vel[k][l];
                    hits += on;
                }
                okf &= !memcmp(&r8.c.t[0].step[k], &o->t[0].step[k], sizeof(step_t)) &&
                       !memcmp(&r8.c.t[2].step[k], &o->t[2].step[k], sizeof(step_t));
            }
            okf &= r8.c.g[G_BPM] == o->g[G_BPM] && r8.c.d.p[P_E0] == pt_get(&o->t[TRK_DRUM], P_E0);
        }
        printf("  (the three format 7 files: %u drum hits)\n", hits);
        bad += check("FUN7 -> FUN8: the simulator's saved patterns (3 files) load: drum hits as trigs, synth steps, tempo, kit",
                     okf && hits > 0u);
    }
    bad += check("2.6 SAVE > PROJECT > ERASE: an object of no bytes is no project (an empty slot, older SLOOP too)",
                 !proj_import(&q2v8_tmp.c, &q, 0, PROJ_V7_TMP) && !proj_import_v7(&q2, &q, 0));
    /* PHEBUS (21 Nov, Ben: "Dot + save on stop"): autosave_due(stop, stop_quiet, quiet, idle ms, ms since the last save) */
    bad += check("save on stop: 0.3 s after STOP, silent, 5 s since the last save: saved",
                 autosave_due(1, 1, 1, AUTOSAVE_STOP_IDLE, AUTOSAVE_STOP_GAP) == 1);
    bad += check("save on stop: not while a voice sounds, nor under a tail, nor with a hand on the panel",
                 !autosave_due(1, 1, 0, 9999, 99999) && !autosave_due(1, 0, 1, 1000, 6000) &&
                 !autosave_due(1, 1, 1, AUTOSAVE_STOP_IDLE - 1u, 99999));
    bad += check("save on stop: at most every 5 s (flash wear); else waits",
                 !autosave_due(1, 1, 1, 1000, AUTOSAVE_STOP_GAP - 1u));
    bad += check("no stop: as before, 2.5 s untouched and 20 s apart (a tail does not block it)",
                 !autosave_due(0, 1, 1, 1000, 99999) && !autosave_due(0, 1, 1, 9999, 10000) &&
                 autosave_due(0, 0, 1, AUTOSAVE_IDLE, AUTOSAVE_GAP) && autosave_due(1, 0, 1, AUTOSAVE_IDLE, AUTOSAVE_GAP));
    {   /* PHEBUS wave 4: the voice filter, its envelope, OCT and TUNE (P_FCUT ..., in format 8's room) and the Type round
         * trip; a core of wave 3 (rsv[0] 0, those ids zero) is mapped onto the voice filter as it sounded */
        static project_t w;
        static const int16_t V[11] = {33, 99, 64, 12, 1, 5, 66, 20, 44, -2, 37};
        uint32_t k, ok = 1;
        int l;
        host_tracks_init();
        for (k = 0; k < 11u; k++)
            trk[0].p[P_FCUT + k] = V[k];
        trk[0].p[P_FTYPE] = VF_BP, trk[0].p[P_ED_FLT] = -20;
        vtype_switch(&trk[2], 4);                       /* (FOLD: PLAITS on track 3) */
        vtype_switch(&trk[1], 0);                       /* (ACID on track 2: every Type on every track) */
        lock_set(&trk[0], 3, P_FCUT, 10);
        lock_set(&trk[0], 3, P_TUNE, -50);
        proj_capture(&w);
        ok &= w.c.rsv[0] >= PROJ_VOICE_V && proj_ok(&w.c);   /* (wave 5: PROJ_FX_V) */
        host_tracks_init();
        proj_apply(&w.c, 1);
        for (k = 0; k < 11u; k++)
            ok &= trk[0].p[P_FCUT + k] == V[k];
        ok &= trk[0].p[P_FTYPE] == VF_BP && trk[0].p[P_ED_FLT] == -20 && trk[2].eng_req == ENGI_PRISM && vtype_of(&trk[2]) == 4u &&
              trk[1].eng_req == ENGI_ACID;
        l = lock_find(&trk[0], 3, P_FCUT, 0);
        ok &= l >= 0 && trk[0].lock[l].val == 10;
        l = lock_find(&trk[0], 3, P_TUNE, 0);
        ok &= l >= 0 && trk[0].lock[l].val == -50;
        bad += check("wave 4: the voice filter, its envelope, OCT, TUNE, their locks and each track's Type through a project", ok);
        /* a wave 3 core: ACID with its own filter values, a CUT lock; the new ids zero as wave 3 left them */
        w.c.rsv[0] = 0;
        w.c.t[0].engine = ENGI_ACID;
        w.c.t[0].p[P_E1] = 40, w.c.t[0].p[P_E2] = 100, w.c.t[0].p[P_E3] = 80, w.c.t[0].p[P_E4] = 50, w.c.t[0].p[P_E6] = 30;
        for (k = P_COUNT_V8; k < PROJ_NP; k++)
            w.c.t[0].p[k] = 0;
        w.c.t[0].p[P_FTYPE] = 0, w.c.t[0].p[P_ED_FLT] = 0, w.c.t[0].p[P_TFLT] = 0;
        for (k = 0; k < NLOCK; k++)
            w.c.t[0].lock[k].step = LOCK_FREE;
        w.c.t[0].lock[0].step = 2, w.c.t[0].lock[0].param = P_E1, w.c.t[0].lock[0].val = 70;
        w.c.t[2].p[P_TFLT] = 32;                        /* (the old track FILT: a DJ high-pass on PLAITS) */
        w.c.sum = proj_sum(&w.c);
        host_tracks_init();
        proj_apply(&w.c, 1);
        l = lock_find(&trk[0], 2, P_FCUT, 0);
        ok = trk[0].p[P_FCUT] == 40 && trk[0].p[P_FRES] == 100 && trk[0].p[P_FLINK] == 1 && trk[0].p[P_FDRV] == 30 &&
             !trk[0].p[P_FSUS] && trk[0].p[P_ED_FLT] > 0 && trk[0].p[P_FTYPE] == VF_LP24 && l >= 0 && trk[0].lock[l].val == 70 &&
             lock_find(&trk[0], 2, P_E1, 0) < 0 && trk[2].p[P_FTYPE] == VF_HP && !trk[2].p[P_TFLT] && trk[2].p[P_FCUT] > 0;
        bad += check("a core of wave 3: ACID's CUT RES ENVMOD DECAY DRV onto the voice filter (OWN), its CUT lock a CUTOFF lock; "
                     "the old FILT HP onto the voice filter's HP", ok);
    }
    printf("%s\n", bad ? "PROJECT FORMAT TEST FAILED" : "project format test passed");
    return bad != 0;
}
