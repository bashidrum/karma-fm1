/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Projects: four slots in RAM (the song sections A..D; two of them in .noinit, so they survive resets and UBOOT
 * entry). With FELUCCA_FLASH every save also goes to flash through storage.c, and an empty RAM slot
 * is filled from flash on load. The working project is also kept in flash by itself (autosave, when
 * the transport is stopped and nothing sounds) and comes back at power-on: SLOOP starts where you
 * left it.
 *
 * Formats: 8 ("FUN8", PHEBUS wave 2, written: below, proj_core_t). Read and converted (to 8 through 7): 7 ("FUN7",
 * KARMA and PHEBUS until wave 2): format 6 and the 24 bytes it left of a flash object, where KARMA kept its CLOUD (the
 * cloud's settings and every send; PHEBUS cut the CLOUD: read and ignored); 3840 bytes.
 * Read and converted: 6 ("FUN6", SLOOP 2.6: the cloud at its defaults, nothing sent to it): format 5 with the ten parameters of 2.6 (P_ITYPE .. P_LPOL, just
 * before P_E0: the INSERT, the ANALOG filter TYPE, ENV SYNC, LFO 2) in a byte each, and each step's nudge (6 bits)
 * and fill condition (2 bits) in one byte: format 5 was full (3840 bytes, one flash object), this is 3816.
 * Read and converted: 5 ("FUN5", SLOOP 2.4 .. 2.5: format 4 plus, per track, the nudge of each step (micro), NLOCK
 * parameter locks, the steps' fill conditions (2 bits each) and P_TFLT, one parameter more just before P_E0;
 * converted by id, the 2.6 parameters at their defaults, a lock on P_E0.. moved with it), 4 ("FUN4", SLOOP 2.0 .. 2.3: PROJ_NP_V4 parameters, today's G_COUNT, 10-byte
 * steps with levels and ratchets, the drum track's 16 lanes; no nudge, no lock), 3 ("FUN3", SLOOP 1.x: 8-byte
 * steps, the drum track's notes become its lanes, the swings x 0.8 for the MPC scale), 2 ("FUN2") and 1
 * ("FUN1"), which held PROJ_NP_V2 parameters per track, mapped by count as user presets are (the first
 * PROJ_NP_V2 - 8 are P_LEVEL.. in order, the last 8 P_E0..P_E7; the parameters added since take their
 * defaults). Their engine bytes are kept: formats 1 and 2 had engines 0..7 (ANALOG .. WHEEL), and the
 * engines added since were appended, no index moved; the drum track's byte (it has no engine) becomes 0.
 *
 * Built on the host too (tests/project_test.c, -DPROJ_HOST): the part above the #ifndef
 * PROJ_HOST needs core.h, params.c (TP), drums.c (the lanes), the engines and trk_def_engine (ui.c). */
#define PROJ_MAGIC 0x46554E38u                 /* "FUN8" (PHEBUS wave 2): the drum trigs, room for waves 4 to 6 */
#define PROJ_MAGIC_V7 0x46554E37u              /* "FUN7": format 6 + KARMA's CLOUD bytes; read only */
#define PROJ_MAGIC_V6 0x46554E36u              /* "FUN6": format 5 + the 2.6 parameters, nudge and condition in a byte; read only */
#define PROJ_MAGIC_V5 0x46554E35u              /* "FUN5": format 4 + per-step nudge, parameter locks, fill conditions (SLOOP 2.4) */
#define PROJ_MAGIC_V4 0x46554E34u              /* "FUN4": four tracks, P_COUNT parameters each, 10-byte steps; read only */
#define PROJ_MAGIC_V3 0x46554E33u              /* "FUN3": SLOOP 1.x; read only */
#define PROJ_MAGIC_V2 0x46554E32u              /* "FUN2": four tracks, PROJ_NP_V2 parameters; read only */
#define PROJ_MAGIC_V1 0x46554E31u              /* "FUN1": one instrument; loads into track 1 */
#define PROJ_NP_V5 61u                         /* P_COUNT of format 5 (P_E0 was 53) */
#define PROJ_NP_V4 58u                         /* P_COUNT of format 4 (P_E0 was 50) */
#define PROJ_NP_V3 57u                         /* P_COUNT of format 3 (P_E0 was 49) */
#define PROJ_NG_V3 27u                         /* G_COUNT of formats 1..3 */
#define PROJ_NP_V2 53u                         /* P_COUNT of formats 1 and 2 (P_E0 was 45) */
#define PROJ_NG_V2 27u                         /* G_COUNT of formats 1 and 2 */
#define PROJ_NG 32u                            /* the globals of formats 4 and 5 (G_COUNT until 2.4). SLOOP 2.5's
                                                * G_DRDLY rides in the byte after sel (0 in older projects: off),
                                                * so the format and its size do not change */
_Static_assert(G_DRDLY == PROJ_NG && G_BEND == PROJ_NG + 5u && G_COUNT == G_BEND + 24u,   /* (+23: the HEX sound, KNOBS: not in a project) */
               "the globals a project holds, then G_DRDLY (then 2.6's GLO > MIDI: settings of the FM-1)");
#define PROJ_NC6 ((uint32_t)P_ITYPE)           /* format 6: the parameters kept in 16 bits, P_LEVEL .. P_VLEAD (as 5), */
#define PROJ_NX6 ((uint32_t)(P_E0 - P_ITYPE))  /* the 2.6 ones in a byte (0..127), then P_E0 .. P_E7 */
_Static_assert(PROJ_NC6 == PROJ_NP_V5 - 8u && PROJ_NX6 == 10u, "format 6: format 5's parameters, then ten bytes");
typedef struct {                               /* one track; the drum track ignores engine / preset */
    int16_t p[PROJ_NC6];                       /* P_LEVEL .. P_VLEAD (pt_get, pt_set) */
    int16_t e[8];                              /* P_E0 .. P_E7 */
    uint8_t x[PROJ_NX6];                       /* P_ITYPE .. P_LPOL (2.6) */
    uint8_t engine, preset;
    union {
        step_t step[NSTEP];
        dstep_t dstep[NSTEP];                  /* (the drum track: 16 lanes, the same size) */
    };
    uint8_t mf[NSTEP];                         /* each step's nudge (core.h MICRO_*, 6 bits) | its fill condition << 6 */
    plock_t lock[NLOCK];                       /* and its parameter locks (step LOCK_FREE = none) */
} proj_trk_t;
typedef struct {
    uint32_t magic, size;
    int16_t g[PROJ_NG];
    uint8_t sel, drdly, rsv[2];                /* the selected track; 2.5: G_DRDLY */
    proj_trk_t t[NTRK];
    uint8_t cloud[24];                         /* format 7: KARMA's CLOUD (cut: zeros, ignored) */
    uint32_t sum;
} project_v7_t;
typedef struct {                               /* format 6 (SLOOP 2.6), read only: format 7 without the CLOUD */
    uint32_t magic, size;
    int16_t g[PROJ_NG];
    uint8_t sel, drdly, rsv[2];
    proj_trk_t t[NTRK];
    uint32_t sum;
} project_v6_t;
_Static_assert(sizeof(project_v6_t) == 3816u && sizeof(project_v7_t) == 3840u &&
               __builtin_offsetof(project_v7_t, cloud) == __builtin_offsetof(project_v6_t, sum),
               "format 7: format 6, then the CLOUD's 24 bytes, then the sum");
static int16_t pt_get(const proj_trk_t *t, uint32_t id)   /* p[id] of a stored track */
{
    return id < PROJ_NC6 ? t->p[id] : id < P_E0 ? (int16_t)t->x[id - PROJ_NC6] : t->e[(id - P_E0) & 7u];
}
static void pt_set(proj_trk_t *t, uint32_t id, int32_t v)
{
    if (id < PROJ_NC6)
        t->p[id] = (int16_t)v;
    else if (id < P_E0)
        t->x[id - PROJ_NC6] = (uint8_t)clamp(v, 0, 255);
    else
        t->e[(id - P_E0) & 7u] = (int16_t)v;
}
static int32_t pt_micro(const proj_trk_t *t, uint32_t i) { int32_t m = t->mf[i] & 63; return m >= 32 ? m - 64 : m; }
static uint32_t pt_fill(const proj_trk_t *t, uint32_t i) { return (uint32_t)t->mf[i] >> 6; }
static void pt_set_mf(proj_trk_t *t, uint32_t i, int32_t micro, uint32_t fill)
{
    t->mf[i] = (uint8_t)(((uint32_t)micro & 63u) | (fill & 3u) << 6);
}
typedef struct {                               /* a track of format 5 (SLOOP 2.4 .. 2.5), read only */
    int16_t p[PROJ_NP_V5];
    uint8_t engine, preset;
    union {
        step_t step[NSTEP];
        dstep_t dstep[NSTEP];
    };
    int8_t micro[NSTEP];                       /* each step's nudge (core.h) */
    plock_t lock[NLOCK];                       /* and its parameter locks (step LOCK_FREE = none) */
    uint8_t fill[NSTEP / 4];                   /* and its fill condition, 2 bits a step (FC_*) */
} proj_trk_v5_t;
typedef struct {                               /* format 5, read only */
    uint32_t magic, size;
    int16_t g[PROJ_NG];
    uint8_t sel, drdly, rsv[2];
    proj_trk_v5_t t[NTRK];
    uint32_t sum;
} project_v5_t;
typedef struct {                               /* a track of format 4 (SLOOP 2.0 .. 2.3), read only */
    int16_t p[PROJ_NP_V4];
    uint8_t engine, preset;
    union {
        step_t step[NSTEP];
        dstep_t dstep[NSTEP];
    };
} proj_trk_v4_t;
typedef struct {                               /* format 4, read only */
    uint32_t magic, size;
    int16_t g[PROJ_NG];
    uint8_t sel, rsv[3];
    proj_trk_v4_t t[NTRK];
    uint32_t sum;
} project_v4_t;
typedef struct { uint8_t note[4], n, time, flags, vel; } step8_t;   /* the steps of formats 1..3 */
typedef struct {                               /* a track of format 3, read only */
    int16_t p[PROJ_NP_V3];
    uint8_t engine, preset;
    step8_t step[NSTEP];
} proj_trk_v3_t;
typedef struct {                               /* format 3 (SLOOP 1.x), read only */
    uint32_t magic, size;
    int16_t g[PROJ_NG_V3];
    uint8_t sel, rsv[3];
    proj_trk_v3_t t[NTRK];
    uint32_t sum;
} project_v3_t;
typedef struct {                               /* a track of formats 1 and 2, read only */
    int16_t p[PROJ_NP_V2];
    uint8_t engine, preset;
    step8_t step[NSTEP];
} proj_trk_v2_t;
typedef struct {                               /* format 2 (until 0.9), read only */
    uint32_t magic, size;
    int16_t g[PROJ_NG_V2];
    uint8_t sel, rsv[3];
    proj_trk_v2_t t[NTRK];
    uint32_t sum;
} project_v2_t;
typedef struct {                               /* format 1 (until 0.5 beta), read only */
    uint32_t magic, size;
    int16_t g[PROJ_NG_V2];
    proj_trk_v2_t t;
    uint32_t sum;
} project_v1_t;
_Static_assert(sizeof(project_v2_t) == 2552u && sizeof(project_v1_t) == 688u && sizeof(project_v3_t) == 2584u &&
               sizeof(project_v4_t) == 3112u && sizeof(project_v5_t) == 3840u, "formats 1 / 2 / 3 / 4 / 5 as they were stored");

static uint32_t proj_hash(const void *p, uint32_t n)   /* FNV-1a over n bytes */
{
    const uint8_t *b = (const uint8_t *)p;
    uint32_t i, s = 0x811C9DC5u;
    for (i = 0; i < n; i++)
        s = (s ^ b[i]) * 16777619u;
    return s;
}
static uint32_t proj_sum_v7(const project_v7_t *p) { return proj_hash(p, sizeof *p - 4u); }
static int proj_ok_v7(const project_v7_t *q) { return q->magic == PROJ_MAGIC_V7 && q->size == sizeof *q && q->sum == proj_sum_v7(q); }

/* ---- old formats -> format 5 */
/* an old step into a synth step (no level, no ratchet) */
static void step_from8(step_t *d, const step8_t *s)
{
    memcpy(d->note, s->note, 4);
    d->n = s->n;
    d->time = s->time;
    d->flags = s->flags;
    d->vel = s->vel;
    d->lvl = d->rat = 0;
}
/* an old drum step (GM notes) into the drum track's lanes; its velocity / accent -> their level */
static void dstep_from8(dstep_t *d, const step8_t *s)
{
    uint32_t i, lvl = (s->flags & SF_ACCENT) || s->vel > 115u ? LV_HARD : !s->vel ? LV_NORM : vel_lvl(s->vel);
    memset(d, 0, sizeof *d);
    if (s->time != ST_NOTE)
        return;
    for (i = 0; i < s->n && i < 4u; i++)
        dstep_set(d, lane_of_note(s->note[i] & 127u), lvl, 0);
}
static int16_t swing_from_v3(int32_t v) { return (int16_t)clamp((v * 4 + 2) / 5, 0, 100); }   /* /250 -> /200 */

/* the globals of formats 1..3 (G_* unchanged since; any added later: their defaults) */
static void proj_g_from_old(int16_t *g, const int16_t *g2)
{
    uint32_t i;
    for (i = 0; i < PROJ_NG; i++)
        g[i] = i < PROJ_NG_V3 ? g2[i] : GP[i].def;
    g[G_SWING] = swing_from_v3(g[G_SWING]);
}

/* no nudge, no lock, no condition (formats 1..4); the 2.6 parameters at their defaults (formats 1..5) */
static void proj_trk_plain(proj_trk_t *d)
{
    uint32_t k;
    memset(d->mf, 0, sizeof d->mf);
    for (k = PROJ_NC6; k < P_E0; k++)
        pt_set(d, k, TP[k].def);
    for (k = 0; k < NLOCK; k++) {
        d->lock[k].step = LOCK_FREE;
        d->lock[k].param = 0;
        d->lock[k].val = 0;
    }
}

/* a track of format 3 -> today's (by id up to P_SLDEPTH; P_E0.. moved) */
static void proj_trk_from_v3(proj_trk_t *d, const proj_trk_v3_t *s, int drum)
{
    uint32_t k, nc = PROJ_NP_V3 - 8u;
    proj_trk_plain(d);
    for (k = 0; k < PROJ_NC6; k++)
        d->p[k] = k < nc ? s->p[k] : TP[k].def;
    for (k = 0; k < 8u; k++)
        d->e[k] = s->p[nc + k];
    d->p[P_SSWING] = swing_from_v3(d->p[P_SSWING]);
    d->p[P_ASWING] = swing_from_v3(d->p[P_ASWING]);
    d->engine = drum ? 0u : s->engine;
    d->preset = drum ? 0u : s->preset;
    for (k = 0; k < NSTEP; k++) {
        if (drum)
            dstep_from8(&d->dstep[k], &s->step[k]);
        else
            step_from8(&d->step[k], &s->step[k]);
    }
}

/* a track of formats 1 and 2 -> format 3 (mapped by count, see the top) */
static void proj_trk_v2_to_v3(proj_trk_v3_t *d, const proj_trk_v2_t *s, int drum)
{
    uint32_t k, nc = PROJ_NP_V2 - 8u;
    for (k = 0; k < PROJ_NP_V3 - 8u; k++)
        d->p[k] = k < nc ? s->p[k] : TP[k].def;
    for (k = 0; k < 8u; k++)
        d->p[PROJ_NP_V3 - 8u + k] = s->p[nc + k];
    d->engine = drum ? 0u : s->engine;          /* (indices 0..7 as they were) */
    d->preset = drum ? 0u : s->preset;
    memcpy(d->step, s->step, sizeof d->step);
}

/* a format 4 project (n bytes in *v4) -> slot q as format 6: the same, no nudge, no lock, no condition, no FILTER */
static int proj_from_v4(project_v7_t *q, const project_v4_t *v4, int n)
{
    uint32_t i;
    if (n != (int)sizeof *v4 || v4->magic != PROJ_MAGIC_V4 || v4->size != sizeof *v4 ||
        v4->sum != proj_hash(v4, sizeof *v4 - 4u))
        return 0;
    memset(q, 0, sizeof *q);
    q->magic = PROJ_MAGIC_V7;
    q->size = sizeof *q;
    memcpy(q->g, v4->g, sizeof q->g);
    q->sel = v4->sel;
    for (i = 0; i < NTRK; i++) {
        uint32_t k;
        proj_trk_plain(&q->t[i]);
        for (k = 0; k < PROJ_NC6; k++)                 /* by id up to P_CHORD, the parameters added since: */
            q->t[i].p[k] = k < PROJ_NP_V4 - 8u ? v4->t[i].p[k] : TP[k].def;   /* their defaults; P_E0.. moved */
        for (k = 0; k < 8u; k++)
            q->t[i].e[k] = v4->t[i].p[PROJ_NP_V4 - 8u + k];
        q->t[i].engine = v4->t[i].engine;
        q->t[i].preset = v4->t[i].preset;
        memcpy(q->t[i].step, v4->t[i].step, sizeof q->t[i].step);
    }
    q->sum = proj_sum_v7(q);
    return 1;
}

/* a format 5 project (n bytes in *v5) -> slot q as format 6: by id, the 2.6 parameters at their defaults, each step's
 * nudge and condition into its byte, the locks as they were (a lock on P_E0.. follows its parameter's new id) */
static int proj_from_v5(project_v7_t *q, const project_v5_t *v5, int n)
{
    uint32_t i, k;
    if (n != (int)sizeof *v5 || v5->magic != PROJ_MAGIC_V5 || v5->size != sizeof *v5 ||
        v5->sum != proj_hash(v5, sizeof *v5 - 4u))
        return 0;
    memset(q, 0, sizeof *q);
    q->magic = PROJ_MAGIC_V7;
    q->size = sizeof *q;
    memcpy(q->g, v5->g, sizeof q->g);
    q->sel = v5->sel;
    q->drdly = v5->drdly;
    for (i = 0; i < NTRK; i++) {
        const proj_trk_v5_t *s = &v5->t[i];
        proj_trk_t *d = &q->t[i];
        proj_trk_plain(d);
        for (k = 0; k < PROJ_NC6; k++)
            d->p[k] = s->p[k];
        for (k = 0; k < 8u; k++)
            d->e[k] = s->p[PROJ_NP_V5 - 8u + k];
        d->engine = s->engine;
        d->preset = s->preset;
        memcpy(d->step, s->step, sizeof d->step);
        for (k = 0; k < NSTEP; k++)
            pt_set_mf(d, k, clamp(s->micro[k], MICRO_MIN, MICRO_MAX), (uint32_t)s->fill[k / 4u] >> (2u * (k % 4u)));
        for (k = 0; k < NLOCK; k++) {
            d->lock[k] = s->lock[k];
            if (s->lock[k].step != LOCK_FREE && s->lock[k].param >= PROJ_NP_V5 - 8u)   /* P_E0.. of format 5 */
                d->lock[k].param = (uint8_t)(s->lock[k].param - (PROJ_NP_V5 - 8u) + P_E0);
        }
    }
    q->sum = proj_sum_v7(q);
    return 1;
}

/* a format 6 project -> slot q as format 7: as it is, the CLOUD's bytes 0 */
static int proj_from_v6(project_v7_t *q, const project_v6_t *v6, int n)
{
    if (n != (int)sizeof *v6 || v6->magic != PROJ_MAGIC_V6 || v6->size != sizeof *v6 ||
        v6->sum != proj_hash(v6, sizeof *v6 - 4u))
        return 0;
    memset(q, 0, sizeof *q);
    memcpy(q, v6, __builtin_offsetof(project_v6_t, sum));
    q->magic = PROJ_MAGIC_V7;
    q->size = sizeof *q;
    q->sum = proj_sum_v7(q);
    return 1;
}

/* a format 3 project -> slot q as format 6 */
static void proj_from_v3_ok(project_v7_t *q, const project_v3_t *v3)
{
    uint32_t i;
    memset(q, 0, sizeof *q);
    q->magic = PROJ_MAGIC_V7;
    q->size = sizeof *q;
    proj_g_from_old(q->g, v3->g);
    q->sel = v3->sel;
    for (i = 0; i < NTRK; i++)
        proj_trk_from_v3(&q->t[i], &v3->t[i], i == TRK_DRUM);
    q->sum = proj_sum_v7(q);
}
static int proj_from_v3(project_v7_t *q, const project_v3_t *v3, int n)
{
    if (n != (int)sizeof *v3 || v3->magic != PROJ_MAGIC_V3 || v3->size != sizeof *v3 ||
        v3->sum != proj_hash(v3, sizeof *v3 - 4u))
        return 0;
    proj_from_v3_ok(q, v3);
    return 1;
}

/* the RAM a load stages a project in (format 8 as stored: its core, then its ext; or an older one's bytes), and the
 * backup page's (editor.c ED_BK_RAW). Formats 1, 2 go through format 3 in its second half (their bytes, 688 and 2552, in
 * its first; format 7 is made elsewhere, proj_import) */
static union {
    uint8_t raw[6768];                         /* (sizeof (project_t), below) */
    struct { uint8_t old[3840]; project_v3_t v3; } o;
} proj_tmp;
#define proj_v3_tmp (proj_tmp.o.v3)            /* (formats 1, 2: through format 3) */
/* a format 2 project (n bytes in *v2) -> slot q as format 5 */
static int proj_from_v2(project_v7_t *q, const project_v2_t *v2, int n)
{
    project_v3_t *v3 = &proj_v3_tmp;
    uint32_t i;
    if (n != (int)sizeof *v2 || v2->magic != PROJ_MAGIC_V2 || v2->size != sizeof *v2 ||
        v2->sum != proj_hash(v2, sizeof *v2 - 4u))
        return 0;
    memset(v3, 0, sizeof *v3);
    memcpy(v3->g, v2->g, sizeof v3->g);
    v3->sel = v2->sel;
    for (i = 0; i < NTRK; i++)
        proj_trk_v2_to_v3(&v3->t[i], &v2->t[i], i == TRK_DRUM);
    proj_from_v3_ok(q, v3);
    return 1;
}

/* a format 1 project (n bytes in *v1) -> slot q as format 5: the instrument becomes track 1,
 * tracks 2..4 start empty (their sounds as at power-on) */
static int proj_from_v1(project_v7_t *q, const project_v1_t *v1, int n)
{
    project_v3_t *v3 = &proj_v3_tmp;
    uint32_t i;
    if (n != (int)sizeof *v1 || v1->magic != PROJ_MAGIC_V1 || v1->size != sizeof *v1 ||
        v1->sum != proj_hash(v1, sizeof *v1 - 4u))
        return 0;
    memset(v3, 0, sizeof *v3);
    memcpy(v3->g, v1->g, sizeof v3->g);
    proj_trk_v2_to_v3(&v3->t[0], &v1->t, 0);
    proj_from_v3_ok(q, v3);
    for (i = 1; i < NTRK; i++) {               /* the other tracks: their defaults, no steps */
        uint32_t k;
        for (k = 0; k < P_COUNT_V8; k++)
            pt_set(&q->t[i], k, k >= P_E0 ? ENGINES[trk_def_engine(i)]->edit[k - P_E0].def : TP[k].def);
        q->t[i].engine = (uint8_t)trk_def_engine(i);
        q->t[i].preset = 0xFF;                 /* 0xFF: its default preset (project_load) */
        memset(q->t[i].step, 0, sizeof q->t[i].step);
        if (i != TRK_DRUM)
            for (k = 0; k < NSTEP; k++)
                q->t[i].step[k].time = ST_REST;
        proj_trk_plain(&q->t[i]);
    }
    q->sum = proj_sum_v7(q);
    return 1;
}

/* n bytes of a stored project of format 7 or older -> q as format 7; 0 = not one */
static int proj_import_v7(project_v7_t *q, const void *b, int n)
{
    if (n == (int)sizeof *q && proj_ok_v7((const project_v7_t *)b)) {
        memcpy(q, b, sizeof *q);
        return 1;
    }
    return proj_from_v6(q, (const project_v6_t *)b, n) ||
           proj_from_v5(q, (const project_v5_t *)b, n) || proj_from_v4(q, (const project_v4_t *)b, n) ||
           proj_from_v3(q, (const project_v3_t *)b, n) ||
           proj_from_v2(q, (const project_v2_t *)b, n) || proj_from_v1(q, (const project_v1_t *)b, n);
}

/* ---- format 8 ("FUN8", PHEBUS wave 2, written; Ben, 21 Nov 2026: "Save format: Accept (new format; PHEBUS reads old
 * projects; stock KARMA cannot read new ones)"). A project is two parts:
 *   proj_core_t  what a section (A..D) holds and switches: the globals, each track's parameters by id, its steps, nudges,
 *                conditions and locks, the drum track's pattern (core.h dpat_t: every sound's trigs, their extras and
 *                their sounds' locks), each drum sound's sends, the STUTTER lane. 5,744 bytes.
 *   proj_ext_t   the project's own, not switched with the sections: its own kit (PHEBUS wave 3, drum_snd.c pkit_t: the copy
 *                of the built-in kit it edited, 488 bytes; then zero). 1,024 bytes.
 * The working project (autosave) is stored whole (core + ext, 6,768 bytes); a slot (a section) its core. Both are storage.c
 * "big" objects (two sectors a copy). A reader takes either length (a slot written whole by a later wave loads too).
 * Room kept, zero, so the waves to come need no new format (each named where it is):
 *   - every track: parameters by id up to PROJ_NP (96; P_COUNT is 71): the new P_ ids of waves 4 and 5 (the filter
 *     envelope's ADSR and LINK / OWN switch, the resonant filter's CUT RES TYPE ENV, the FX slots' TYPE TIME INTENSITY
 *     AMOUNT) are appended and land there, read by id; the engine choice per track (wave 4) is the engine byte, as now
 *   - every synth track: mod[46], for wave 5's mod matrix grid (its routings) or whatever a track keeps that is no
 *     parameter
 *   - the globals by id up to PROJ_NG8 (48; a project keeps G_BPM .. G_NEWPRJ, 32 of them, and G_DRDLY in its byte)
 *   - perf: the STUTTER lane as punch.c keeps it, then fx[124] for wave 6's performance lane (the played effects
 *     recorded into it)
 *   - the drum pattern's lock ids from DSP_COUNT (core.h): wave 3's sends, LFO, synthesised sounds and pitch (to DSP_NIDS)
 *   - ext.kit: the project's own kit (wave 3: 16 sounds and their LFOs, copy-on-write from the built-in kits) */
#define PROJ_NP 96u                            /* parameters a track keeps, by id (P_COUNT and up: 0, waves 4 and 5) */
#define PROJ_NG8 48u                           /* globals a project keeps, by id (PROJ_NG and up: 0, later waves) */
_Static_assert(P_COUNT <= PROJ_NP && PROJ_NG <= PROJ_NG8, "format 8 keeps every parameter and global by id");
typedef struct {                               /* the STUTTER lane (punch.c stl), and wave 6's room: 160 bytes */
    uint8_t on[8], lc[16], lx[8];              /* as punch.c: lit steps, length codes (2 bits a step), the codes' third bit */
    uint8_t len, act, rsv[2];                  /* steps (1..64), ON */
    uint8_t fx[124];                           /* wave 6: the pads played into the lane (punch.c pev_t: 31 of 4 bytes) */
} proj_perf_t;
typedef struct {                               /* a synth track: 1,040 bytes */
    int16_t p[PROJ_NP];                        /* by id (P_COUNT ..: reserved, zero) */
    uint8_t engine, preset;
    uint8_t mod[46];                           /* reserved, zero: wave 5's mod matrix */
    step_t step[NSTEP];
    uint8_t mf[NSTEP];                         /* each step's nudge (core.h MICRO_*, 6 bits) | its condition << 6 (FC_*) */
    plock_t lock[NLOCK];
} proj_syn_t;
typedef struct {                               /* the drum track: 2,352 bytes */
    int16_t p[PROJ_NP];                        /* by id (P_E0: the kit; P_COUNT ..: reserved, zero) */
    uint8_t dsend[DRUM_LANES];                 /* each sound's own sends (drums.c drum_send): reverb 0..15 | delay 0..7 << 4 */
    plock_t lock[NLOCK];                       /* the drum track's own parameters' locks (its bus) */
    dpat_t pat;                                /* its pattern: every sound's trigs, extras, locks */
} proj_drm_t;
typedef struct {
    uint32_t magic, size;                      /* PROJ_MAGIC, sizeof (proj_core_t) */
    int16_t g[PROJ_NG8];                       /* by id */
    uint8_t sel, drdly, rsv[2];                /* the selected track, G_DRDLY; rsv[0] (PHEBUS wave 4): PROJ_VOICE_V once the
                                                * synth tracks' parameters are wave 4's (0: before, mapped when it loads) */
    proj_perf_t perf;
    proj_syn_t t[NPART];
    proj_drm_t d;
    uint32_t sum;                              /* FNV-1a of the bytes before it */
} proj_core_t;
typedef struct {
    uint8_t kit[1024];                         /* the project's own kit (drum_snd.c pkit_t, 488 bytes; magic 0: none), then zero */
} proj_ext_t;
typedef struct {                               /* the working project as stored */
    proj_core_t c;
    proj_ext_t x;
} project_t;
_Static_assert(sizeof(proj_perf_t) == 160u && sizeof(proj_syn_t) == 1040u && sizeof(proj_drm_t) == 2352u &&
               sizeof(proj_core_t) == 5744u && sizeof(proj_ext_t) == 1024u && sizeof(project_t) == 6768u,
               "format 8 as stored");
_Static_assert(sizeof(project_t) <= 2u * 4096u - 256u, "format 8 fits a big flash object (storage.c ST_BIG_MAX)");
static uint32_t proj_sum(const proj_core_t *c) { return proj_hash(c, sizeof *c - 4u); }
/* (PHEBUS wave 4) the core's voice layout: 1 = the voice filter, its envelope, OCT and TUNE (P_FCUT ..) are in it and the
 * engines' own filter values are not read. A core of wave 2 or 3 (0, its room for them zero) is mapped when it is applied
 * (ui.c voice_from_engine), as is every older format (they come through format 7 with 0). Format 8 still: a wave 3
 * firmware reads it, without the new values */
#define PROJ_VOICE_V 1u
/* (PHEBUS wave 5) 2 = the FX slots and the mod matrix too: the slots' and LFO 2's parameters in the room by id, the matrix, LFO
 * 2's SYNC and TRIG, Spice and Dice in mod[46] (mx_pack). Below 2: the sound's DIST, INSERT and sends are made slots, its
 * LFO > SHP and ENV > SHP cells, when it is applied (modmx.c fx_from_legacy, mx_from_legacy) */
#define PROJ_FX_V 2u
/* (PHEBUS wave 8, Ben 22 Nov 2026: "FX = whole mix only"; "cut the spare filters"; "tempo in one place") 3 = the sends are the
 * SENDS page's, always heard, and every FX slot is an insert; no hidden DJ filter on a synth track, none on the master; the
 * swing is each pattern's only. Below 3, when it is applied: a CHORUS / REVERB / DELAY slot becomes OFF with its send kept, a
 * send with no slot 0 (modmx.c fx_sends_from_slots); a synth track's FILT (CC 74's DJ filter) onto its voice filter; GLO >
 * MASTER FILT dropped; GLO > GLOBAL SWING added to every track's own (proj_apply) */
#define PROJ_SEND_V 3u
static int16_t proj_old_swing;                 /* (wave 8) the GLOBAL SWING of the last project of before loaded: a section
                                                * stored then borrowed it, so it is added to such a section's tracks too */
/* (wave 11) mod[46], layout 2: byte 7 is MXL2 (layout 1, waves 5 to 10, always had 0 there); the other 45 bytes a stream of
 * bits, low bit first: LFO 2's SYNC (4) and TRIG (2), Spice (7), Dice (14), how many cells (5), then each cell: its source (3),
 * its destination (7: a fixed column 0..10, or 11 + the parameter id of an assignable column), its amount (7, -64..63). 32 +
 * 19 x 17 = 355 of 360 bits. The columns are not stored: each parameter a cell names gets one again (in the order met).
 * Layout 1 (as[0] as[1], SYNC, TRIG, Spice, Dice in bytes 0..6; 19 cells of two bytes, the place s x 13 + d + 1) is read
 * exactly: every cell keeps its source, column and amount (AS1 AS2 are columns 11 and 12 there and here) */
#define MXL2 0x5Bu
#define MD_N_V1 13u                                    /* (layout 1's columns: AS1, AS2 only) */
_Static_assert(32u + 17u * MX_NCELL <= 360u && P_COUNT + MD_NFIX < 128u, "the matrix fits mod[46], layout 2");
static void mxb_put(uint8_t *b, uint32_t *pos, uint32_t v, uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; i++, (*pos)++) {
        uint32_t q = *pos < 56u ? *pos : *pos + 8u;    /* (byte 7 is the layout's) */
        if ((v >> i) & 1u)
            b[q >> 3] |= (uint8_t)(1u << (q & 7u));
    }
}
static uint32_t mxb_get(const uint8_t *b, uint32_t *pos, uint32_t n)
{
    uint32_t i, v = 0;
    for (i = 0; i < n; i++, (*pos)++) {
        uint32_t q = *pos < 56u ? *pos : *pos + 8u;
        v |= (uint32_t)((b[q >> 3] >> (q & 7u)) & 1u) << i;
    }
    return v;
}
static void mx_pack(uint8_t *b, uint32_t ti)       /* track ti's matrix, LFO 2, Spice and Dice -> mod[46] */
{
    const mxt_t *m = &mxt[ti % NPART];
    uint32_t i, pos = 0, n = 0;
    memset(b, 0, 46);
    b[7] = MXL2;
    mxb_put(b, &pos, (uint32_t)m->l2sync & 15u, 4);
    mxb_put(b, &pos, (uint32_t)m->l2trig & 3u, 2);
    mxb_put(b, &pos, (uint32_t)m->spice & 127u, 7);
    mxb_put(b, &pos, (uint32_t)(uint16_t)m->dice & 0x3FFFu, 14);
    for (i = 0; i < MX_NCELL; i++)
        n += m->c[i].at != 0;
    mxb_put(b, &pos, n, 5);
    for (i = 0; i < MX_NCELL; i++) {
        uint32_t at = m->c[i].at, d, code;
        if (!at)
            continue;
        d = (at - 1u) % MD_N;
        code = d < MD_NFIX ? d : m->as[d - MD_AS1] ? MD_NFIX + m->as[d - MD_AS1] - 1u : 127u;
        mxb_put(b, &pos, (at - 1u) / MD_N, 3);
        mxb_put(b, &pos, code, 7);
        mxb_put(b, &pos, (uint32_t)m->c[i].amt & 127u, 7);
    }
}
/* a cell (s, d) of amount a read back into track ti (an assignable one: the column of parameter p, made if new; p
 * not a parameter a column can move, or no column left: dropped). 1 = kept */
static int mx_cell_in(uint32_t ti, uint32_t s, uint32_t d, int32_t p, int32_t a)
{
    mxt_t *m = &mxt[ti];
    uint32_t i;
    a = clamp(a, -64, 63);
    if (s >= MS_N || !a)
        return 0;
    if (d >= MD_NFIX) {
        int32_t c = p >= 0 && mx_as_ok((uint32_t)p) ? mx_as_find(ti, (uint32_t)p) : -1;
        if (p < 0 || !mx_as_ok((uint32_t)p))
            return 0;
        if (c < 0) {
            for (i = 0; i < MX_NAS && m->as[i]; i++)
                ;
            if (i == MX_NAS)
                return 0;
            m->as[i] = (uint8_t)(p + 1);
            c = (int32_t)(MD_AS1 + i);
        }
        d = (uint32_t)c;
    }
    if (mx_alias(s, d) >= 0)
        return 0;
    for (i = 0; i < MX_NCELL; i++)
        if (!m->c[i].at) {
            m->c[i].at = (uint8_t)mx_at(s, d);
            m->c[i].amt = (int8_t)a;
            return 1;
        }
    return 0;
}
static uint32_t mx_unpack_lost;                        /* (the last read: cells dropped; tests) */
static void mx_unpack(const uint8_t *b, uint32_t ti)   /* .. back, every value inside its range */
{
    mxt_t *m = &mxt[ti % NPART];
    uint32_t i;
    ti %= NPART;
    memset(m->c, 0, sizeof m->c);
    memset(m->as, 0, sizeof m->as);
    mx_unpack_lost = 0;
    if (b[7] == MXL2) {                               /* layout 2 (wave 11) */
        uint32_t pos = 0, n, l2s = mxb_get(b, &pos, 4), l2t = mxb_get(b, &pos, 2);
        m->l2sync = (int16_t)(l2s < NELEM(N_LSYNC) ? l2s : 0u);
        m->l2trig = (int16_t)(l2t < 3u ? l2t : 1u);
        m->spice = (int16_t)mxb_get(b, &pos, 7);
        m->dice = (int16_t)clamp((int32_t)mxb_get(b, &pos, 14), 0, 9999);
        n = mxb_get(b, &pos, 5);
        for (i = 0; i < n && i < MX_NCELL; i++) {
            uint32_t s = mxb_get(b, &pos, 3), code = mxb_get(b, &pos, 7), a7 = mxb_get(b, &pos, 7);
            int32_t a = (int32_t)(a7 & 63u) - (int32_t)(a7 & 64u);
            if (!mx_cell_in(ti, s, code < MD_NFIX ? code : MD_NFIX, code < MD_NFIX ? -1 : (int32_t)(code - MD_NFIX), a) && a)
                mx_unpack_lost++;
        }
        return;
    }
    m->l2sync = (int16_t)(b[2] < NELEM(N_LSYNC) ? b[2] : 0u);   /* layout 1 (waves 5 to 10) */
    m->l2trig = (int16_t)(b[3] < 3u ? b[3] : 1u);
    m->spice = (int16_t)(b[4] & 127u);
    m->dice = (int16_t)clamp((int32_t)(b[5] | (uint32_t)b[6] << 8), 0, 9999);
    for (i = 0; i < 2u; i++)                          /* (its two columns first, in their places: AS1 AS2 as they were) */
        m->as[i] = (uint8_t)(b[i] && mx_as_ok(b[i] - 1u) ? b[i] : 0u);
    for (i = 0; i < MX_NCELL; i++) {
        uint32_t at = b[8u + 2u * i], s, d;
        int32_t a = (int8_t)b[9u + 2u * i];
        if (!at || at > MS_N * MD_N_V1 || !a)
            continue;
        s = (at - 1u) / MD_N_V1, d = (at - 1u) % MD_N_V1;
        if (d >= MD_NFIX && !m->as[d - MD_NFIX])
            continue;                                 /* (a column with no parameter: it moved nothing) */
        if (!mx_cell_in(ti, s, d < MD_NFIX ? d : MD_NFIX, d < MD_NFIX ? -1 : (int32_t)m->as[d - MD_NFIX] - 1, a))
            mx_unpack_lost++;
    }
}
static int proj_ok(const proj_core_t *c) { return c->magic == PROJ_MAGIC && c->size == sizeof *c && c->sum == proj_sum(c); }

/* a pattern's extras and locks checked: a free entry at the free mark, one for no trig (or out of range) freed, the
 * velocities 0..127 */
static void pat_fix(dpat_t *d)
{
    uint32_t k, i, l;
    for (i = 0; i < NSTEP; i++)
        for (l = 0; l < DRUM_LANES; l++)
            d->vel[i][l] &= 127u;
    for (k = 0; k < DT_NX; k++) {
        dtx_t *x = &d->x[k];
        if (x->step >= NSTEP || x->lane >= DRUM_LANES || !d->vel[x->step][x->lane]) {
            x->step = DT_FREE;
            continue;
        }
        x->cond &= 3u;
        x->prob = (uint8_t)(x->prob < 1u ? 1u : x->prob > 100u ? 100u : x->prob);
        x->micro = (int8_t)clamp(x->micro, MICRO_MIN, MICRO_MAX);
        if (x->rrate > NRTRG)
            x->rrate = 0;
    }
    for (k = 0; k < DT_NLK; k++) {
        dlk_t *q = &d->lk[k];
        if (q->step >= NSTEP || q->lane >= DRUM_LANES || q->param >= DSP_NIDS || !d->vel[q->step][q->lane])
            q->step = DT_FREE;
    }
}
/* the trig (i, l) of pattern d: its extras' entry, made at the defaults if it has none (0: the pool is full) */
static dtx_t *pat_x(dpat_t *d, uint32_t i, uint32_t l)
{
    uint32_t k, f = DT_NX;
    for (k = 0; k < DT_NX; k++) {
        if (d->x[k].step == i && d->x[k].lane == l)
            return &d->x[k];
        if (f == DT_NX && d->x[k].step == DT_FREE)
            f = k;
    }
    if (f == DT_NX)
        return 0;
    memset(&d->x[f], 0, sizeof d->x[f]);
    d->x[f].step = (uint8_t)i;
    d->x[f].lane = (uint8_t)l;
    d->x[f].prob = DT_PROB_DEF;
    d->x[f].rlen = DT_RLEN_DEF;
    return &d->x[f];
}

/* ---- format 7 (and older, through it) -> format 8 */
#define STL_MARK 0x53u                         /* format 7: the STUTTER lane is in the drum track's unused bytes */
_Static_assert(P_LD_AMP - P_LRATE == 7 && P_DETUNE - P_GLMODE == 3 && P_DETUNE < PROJ_NC6 && P_REL - P_ATK == 3 &&
               P_REL < PROJ_NC6, "format 7: the lane's bytes");
static uint32_t proj_v7_lost;                  /* (the last conversion: drum trigs whose nudge, condition or ratchet did
                                                * not fit the extras' pool; tests/project_test.c) */
/* a ratchet of hits x2..x4 in a step on the drum track's DIV div: the retrig rate nearest a step / hits (seq.c RT_DEN) */
static uint32_t proj_rt_rate(uint32_t div, uint32_t hits)
{
    uint32_t want = div_units(div % NDIV_STEP) / hits, r, best = 1, bd = 0xFFFFFFFFu;
    for (r = 1; r <= NRTRG; r++) {
        uint32_t u = rt_units(r), d = u > want ? u - want : want - u;
        if (d < bd)
            bd = d, best = r;
    }
    return best;
}
static void proj_from_v7(proj_core_t *c, const project_v7_t *v)
{
    const proj_trk_t *od = &v->t[TRK_DRUM];
    uint32_t i, k, l, div;
    memset(c, 0, sizeof *c);
    c->magic = PROJ_MAGIC;
    c->size = sizeof *c;
    for (i = 0; i < PROJ_NG; i++)
        c->g[i] = v->g[i];
    c->sel = v->sel;
    c->drdly = v->drdly;
    for (i = 0; i < NPART; i++) {                       /* the synth tracks: as they were */
        proj_syn_t *d = &c->t[i];
        for (k = 0; k < P_COUNT_V8; k++)                /* (wave 4's are not in format 7: mapped when applied, rsv[0] 0) */
            d->p[k] = pt_get(&v->t[i], k);
        d->engine = v->t[i].engine;
        d->preset = v->t[i].preset;
        memcpy(d->step, v->t[i].step, sizeof d->step);
        memcpy(d->mf, v->t[i].mf, sizeof d->mf);
        memcpy(d->lock, v->t[i].lock, sizeof d->lock);
    }
    /* the drum track: its parameters, but those format 7 used for the sends and the lane (unused on the drum track: their
     * defaults); its locks (they were on its own parameters, its bus, and stay so: no sound's parameter is one of them) */
    for (k = 0; k < P_COUNT; k++)
        c->d.p[k] = (k > P_E0 && k <= P_E7) ? 0 : (k >= P_LRATE && k <= P_LD_AMP) || (k >= P_GLMODE && k <= P_DETUNE) ||
                                                      (k >= P_ATK && k <= P_REL) || k >= P_COUNT_V8 ? TP[k].def : pt_get(od, k);
    memcpy(c->d.lock, od->lock, sizeof c->d.lock);
    for (l = 0; l < DRUM_LANES; l++) {                  /* each sound's sends: 7 bits a sound in P_E1 .. P_E7 (rsv[0] 0x40) */
        uint32_t b, w = 0;
        for (b = 0; b < 7u && (v->rsv[0] & 0x40u); b++)
            w |= (((uint16_t)od->e[1u + (l * 7u + b) / 16u] >> ((l * 7u + b) % 16u)) & 1u) << b;
        c->d.dsend[l] = (uint8_t)w;
    }
    if (od->engine == STL_MARK) {                       /* the STUTTER lane */
        for (i = 0; i < 16u; i++)
            c->perf.lc[i] = (uint8_t)((uint16_t)od->p[P_LRATE + i / 2u] >> (8u * (i & 1u)));
        for (i = 0; i < 8u; i++) {
            c->perf.on[i] = (uint8_t)((uint16_t)od->p[P_GLMODE + i / 2u] >> (8u * (i & 1u)));
            c->perf.lx[i] = (od->preset & 128u) ? (uint8_t)((uint16_t)od->p[P_ATK + i / 2u] >> (8u * (i & 1u))) : 0u;
        }
        c->perf.len = (uint8_t)((od->preset & 63u) + 1u);
        c->perf.act = (uint8_t)((od->preset & 64u) != 0u);
    } else {                                            /* (none: empty, 16 steps, ON) */
        c->perf.len = 16;
        c->perf.act = 1;
    }
    /* the drum steps: each lane's hit a trig of its sound at its level's velocity; the step's nudge and condition go to
     * every trig of the step, a ratchet becomes a retrig of one step at the rate nearest it */
    dpat_clear(&c->d.pat);
    proj_v7_lost = 0;
    div = (uint32_t)clamp(pt_get(od, P_SDIV), 0, NDIV_STEP - 1);
    for (i = 0; i < NSTEP; i++) {
        const dstep_t *s = &od->dstep[i];
        int32_t mu = clamp(pt_micro(od, i), MICRO_MIN, MICRO_MAX);
        uint32_t f = pt_fill(od, i);                    /* (3: CHNC, which format 7 stored but its loader read as
                                                         * normal: CHNC again, at 50 %, as it played) */
        for (l = 0; l < DRUM_LANES; l++) {
            uint32_t rt;
            dtx_t *x;
            if (!dstep_has(s, l))
                continue;
            c->d.pat.vel[i][l] = (uint8_t)lvl_vel(dstep_lvl(s, l), DT_VEL_DEF);
            rt = dstep_rat(s, l);
            if (!mu && f == FC_NORM && !rt)
                continue;
            if (!(x = pat_x(&c->d.pat, i, l))) {
                proj_v7_lost++;
                continue;
            }
            x->micro = (int8_t)mu;
            x->cond = (uint8_t)f;
            if (rt)
                x->rrate = (uint8_t)proj_rt_rate(div, rt + 1u);
        }
    }
    c->sum = proj_sum(c);
}

/* n bytes of a stored project (any format: format 8 a slot's core alone or a whole project, its ext reserved) -> its core
 * q; 0 = not a project. q may be b (format 8: checked where it is). An older one goes through format 7 in v7 (RAM of its
 * own: it must not overlap b, nor q; the main loop uses autosave_buf), its bytes read whole before q is written */
static int proj_import(proj_core_t *q, const void *b, int n, project_v7_t *v7)
{
    if ((n == (int)sizeof(proj_core_t) || n == (int)sizeof(project_t)) && proj_ok((const proj_core_t *)b)) {
        if ((const void *)q != b)
            memcpy(q, b, sizeof *q);
        return 1;
    }
    if (n <= 0 || n > (int)sizeof(project_v7_t) || !proj_import_v7(v7, b, n))
        return 0;
    proj_from_v7(q, v7);
    return 1;
}

/* ---- the working project <-> a project */
static void proj_capture_core(proj_core_t *c)  /* what is playing now, as a project's core */
{
    uint32_t i, k;
    memset(c, 0, sizeof *c);
    c->magic = PROJ_MAGIC;
    c->size = sizeof *c;
    for (i = 0; i < PROJ_NG; i++)
        c->g[i] = song.g[i];
    c->sel = song.sel;
    c->drdly = (uint8_t)song.g[G_DRDLY];
    c->rsv[0] = PROJ_SEND_V;                            /* (wave 4: the voice filter's values are in it; wave 5: the FX
                                                         * slots and the matrix; wave 8: the sends their own) */
    memcpy(c->perf.on, stl.on, sizeof c->perf.on);
    memcpy(c->perf.lc, stl.lc, sizeof c->perf.lc);
    memcpy(c->perf.lx, stl.lx, sizeof c->perf.lx);
    c->perf.len = stl.len;
    c->perf.act = stl.act;
    _Static_assert(sizeof c->perf.fx == sizeof stl.ev, "wave 6: the recorded pads in format 8's room");
    memcpy(c->perf.fx, stl.ev, sizeof c->perf.fx);     /* (wave 6) the pads recorded into the lane */
    c->perf.rsv[0] = PEV_V13;                           /* (wave 13) .. in the PERFORM set's numbering (punch.c) */
    for (i = 0; i < NPART; i++) {
        proj_syn_t *d = &c->t[i];
        for (k = 0; k < P_COUNT; k++)
            d->p[k] = trk[i].p[k];
        d->engine = trk[i].eng_req;
        d->preset = trk[i].preset;
        memcpy(d->step, trk[i].step, sizeof d->step);
        for (k = 0; k < NSTEP; k++)
            d->mf[k] = (uint8_t)(((uint32_t)trk[i].micro[k] & 63u) | (((uint32_t)trk[i].fill[k / 4u] >> (2u * (k % 4u))) & 3u) << 6);
        memcpy(d->lock, trk[i].lock, sizeof d->lock);
        mx_pack(d->mod, i);                             /* (wave 5) */
    }
    for (k = 0; k < P_COUNT; k++)
        c->d.p[k] = TDRUM->p[k];
    c->d.p[P_E0] = (int16_t)(pkit.base < DRUM_KITS && !drum_kit_none(pkit.base) ? pkit.base : DRUM_DEFAULT_KIT);   /* (wave 12:
                                                         * the kit is the project's 16 slots, in its ext; P_E0 and dsend only
                                                         * say which kit and sends, for a firmware before wave 12) */
    for (k = 0; k < DRUM_LANES; k++)
        c->d.dsend[k] = pkit.s[k].send;
    memcpy(c->d.lock, TDRUM->lock, sizeof c->d.lock);
    c->d.pat = dpat;
    c->sum = proj_sum(c);
}
static void proj_capture(project_t *p)        /* .. as a whole project, as stored (the ext: the project's kit, wave 12's 16 slots) */
{
    memset(&p->x, 0, sizeof p->x);
    memcpy(p->x.kit, &pkit, sizeof pkit);
    proj_capture_core(&p->c);
}
/* (PHEBUS wave 3) the working project's sum as stored: its core's and its kit's (the autosave, the unsaved dot) */
static uint32_t work_sum(uint32_t core_sum) { return core_sum ^ pkit_hash() * 0x9E3779B1u; }
/* a working project of n bytes as stored (b: its core, then its ext; q: its core, converted) -> the project's kit. Wave 12's
 * ext holds the 16 slots ("PKT2"). Anything older is converted to the 16 slots, the same sound exactly (drum_snd.c
 * pkit_from_old): the kit it played (its core's P_E0), each sound's sends (its dsend), and waves 3 to 11's edited copy where
 * it was of that kit. A copy of another kit (it did not play) is kept as a user kit, in the first free one (pkit_old_kept) */
static void proj_kit_take(const uint8_t *b, int n, const proj_core_t *q)
{
    const pkit_t *x = (const pkit_t *)(b + sizeof(proj_core_t));
    const pkit1_t *c1 = (const pkit1_t *)(b + sizeof(proj_core_t));
    int other = 0;
    _Static_assert(sizeof(pkit_t) <= sizeof(proj_ext_t) && sizeof(pkit1_t) <= sizeof(proj_ext_t), "the kit fits the ext");
    if (n == (int)sizeof(project_t) && x->magic == PK_MAGIC) {
        pkit_t k;
        memcpy(&k, x, sizeof k);
        if (pkit_fix(&k)) {
            fm1_irq_off();
            pkit = k;
            pkit_gen++;
            fm1_irq_on();
            return;
        }
    }
    pkit_from_old((uint32_t)clamp(q->d.p[P_E0], 0, DRUM_KITS - 1), q->d.dsend,
                  n == (int)sizeof(project_t) ? c1 : 0, &other);
    if (other) {                                   /* (the copy that did not play: into a free user kit) */
        pkit_t k;
        uint32_t u, l;
        if (kit_make(c1->src, &k)) {
            for (l = 0; l < DRUM_LANES; l++) {
                dlfo_t lf = c1->lfo[l];
                memcpy(k.s[l].snd, c1->snd[l], 22);
                dlfo_fix(&lf);
                k.s[l].lfo = lf;
                slot_fix(&k.s[l]);
            }
            for (u = 0; u < UKIT_N; u++)
                if (ukit_state(u) == UK_ERASED || (u >= DSU_N && ukit_state(u) == UK_NONE))
                    break;
            k.base = (uint8_t)(DSRC_USER + u);
            pkit_old_kept = (uint8_t)(u < UKIT_N && !ukit_put(u, &k) ? u + 1u : 0xFFu);
        }
    }
}
_Static_assert(sizeof proj_tmp == sizeof(project_t) && sizeof(project_v3_t) <= sizeof proj_tmp - 3840u,
               "proj_tmp holds a whole project, or an old one's bytes and format 3");

/* a project's tracks (and its globals, all: a load; or only the drum level / reverb / delay: a song
 * section) into the working one, every value back inside its range. The audio ISR must not run
 * meanwhile (the song sections: called from it; a load: IRQ off) */
static void proj_apply(const proj_core_t *p, int all)
{
    uint32_t i, k;
    for (i = 0; i < PROJ_NG; i++)
        if (all ? i != G_SLOT && i != G_LOAD && i != G_SAVE && i != G_SYNC && i != G_MIDI && i != G_ROUTE && i != G_DRCH &&
                      i != G_INFO
                : i == G_DRLVL || i == G_DRREV)                 /* (2.6: the drum channel a setting of the FM-1, GLO > MIDI;
                                                                 * its slot and G_INFO's held KARMA's TEXTURE) */
            song.g[i] = (int16_t)clamp(p->g[i], GP[i].min, GP[i].max);
    song.g[G_DRDLY] = (int16_t)clamp(p->drdly, GP[G_DRDLY].min, GP[G_DRDLY].max);   /* (a load and a section, as REV) */
    if (all && p->rsv[0] < PROJ_SEND_V)                 /* (wave 8) the old GLOBAL SWING, onto every track's (below) */
        proj_old_swing = (int16_t)clamp(song.g[G_SWING], 0, 100);
    else if (all)
        proj_old_swing = 0;
    song.g[G_SWING] = 0;                                /* (no page: the pattern's swing is the one) */
    song.g[G_FILT] = 0;                                 /* (wave 8: the master DJ filter is gone, PERFORM's LP / HP) */
    /* (wave 12) each drum sound's own sends are its slot's: the kit is the project's (proj_kit_take), a section never
     * changes it (its dsend and P_E0 only say what they were) */
    memcpy(stl.on, p->perf.on, sizeof stl.on);          /* (PHEBUS) the STUTTER lane: a load and a section */
    memcpy(stl.lc, p->perf.lc, sizeof stl.lc);
    memcpy(stl.lx, p->perf.lx, sizeof stl.lx);
    stl.len = (uint8_t)(p->perf.len >= 1u && p->perf.len <= 64u ? p->perf.len : 16u);
    stl.act = (uint8_t)(p->perf.act != 0u);
    memcpy(stl.ev, p->perf.fx, sizeof stl.ev);          /* (wave 6) the pads recorded (zero before: none) */
    if (p->perf.rsv[0] != PEV_V13) {                    /* (wave 13) saved by waves 6 to 12: onto the PERFORM set (punch.c) */
        uint32_t dr = pev_convert_old(stl.ev);
        pev_drop_n = (uint8_t)(pev_drop_n + dr > 99u ? 99u : pev_drop_n + dr);
    }
    for (k = 0; k < PEV_N; k++)                         /* (a span that cannot be: dropped) */
        if ((stl.ev[k].fx & 31u) > PF_NPAD || stl.ev[k].st >= NSTEP || !stl.ev[k].len || stl.ev[k].len > NSTEP)
            stl.ev[k].fx = 0;
    perf_rec_drop();
    for (k = 0; k < NTRK; k++) {
        track_t *t = &trk[k];
        int drum = k == TRK_DRUM;
        const int16_t *pp = drum ? p->d.p : p->t[k].p;
        const plock_t *pl = drum ? p->d.lock : p->t[k].lock;
        uint32_t e = drum ? 0u : p->t[k].engine % NENGINES;
        t->eng_req = (uint8_t)e;
        t->user = 0;                                    /* (no user preset slot is saved) */
        t->lk_n = 0;                                    /* (the locks in force: the values come from the project) */
        for (i = 0; i < P_COUNT; i++) {                 /* every value back inside its range */
            const param_desc_t *d = drum && i == P_E0 ? &DRUM_KIT_DESC :   /* the drum kit */
                                    i >= P_E0 && i <= P_E7 ? &ENGINES[e]->edit[i - P_E0] : &TP[i];
            t->p[i] = (int16_t)clamp(i >= P_COUNT_V8 && drum ? TP[i].def : pp[i], d->min, d->max);
        }
        if (!drum && p->rsv[0] < PROJ_VOICE_V)          /* (wave 4) saved before the voice filter: mapped onto it */
            voice_from_engine(t->p, e, 1);
        else if (!drum && p->rsv[0] < PROJ_SEND_V)      /* (wave 8) CC 74's hidden DJ filter: onto the voice filter */
            tflt_onto_voice(t->p, e);
        if (p->rsv[0] < PROJ_SEND_V && proj_old_swing)  /* (wave 8) the GLOBAL SWING it played with, now the pattern's */
            t->p[P_SSWING] = (int16_t)clamp(t->p[P_SSWING] + proj_old_swing, 0, 100);
        if (drum) {
            t->preset = 0;
            memset(t->step, 0, sizeof t->step);         /* (its pattern: dpat; no step, nudge or condition of its own) */
            memset(t->micro, 0, sizeof t->micro);
            memset(t->fill, 0, sizeof t->fill);
            memcpy(&dpat, &p->d.pat, sizeof dpat);
            pat_fix(&dpat);
        } else {
            const proj_syn_t *s = &p->t[k];
            t->preset = (uint8_t)(ENGINES[e]->npresets ? (s->preset == 0xFFu ? 0u : s->preset >= ENGINES[e]->npresets ? ENGINES[e]->npresets - 1u : s->preset) : 0u);   /* (3.1.3: a removed
                                                         * sound, OMEN BELL: the engine's last, not its first) */
            memcpy(t->step, s->step, sizeof t->step);
            memset(t->fill, 0, sizeof t->fill);
            for (i = 0; i < NSTEP; i++) {               /* the nudges and conditions */
                int32_t m = s->mf[i] & 63;
                t->micro[i] = (int8_t)clamp(m >= 32 ? m - 64 : m, MICRO_MIN, MICRO_MAX);
                t->fill[i / 4u] |= (uint8_t)(((uint32_t)s->mf[i] >> 6) << (2u * (i % 4u)));
            }
            for (i = 0; i < NSTEP; i++) {
                step_t *st = &t->step[i];
                uint32_t j;
                if (st->n > 4u)
                    st->n = 4;
                if (st->time > ST_REST)
                    st->time = ST_REST;
                for (j = 0; j < 4u; j++)
                    st->note[j] &= 127u;
            }
        }
        for (i = 0; i < NLOCK; i++) {
            const plock_t *l = &pl[i];
            uint32_t id = l->param;
            if (l->step < NSTEP && id < P_COUNT && p_lockable(id)) {
                const param_desc_t *d = drum && id == P_E0 ? &DRUM_KIT_DESC :
                                        id >= P_E0 && id <= P_E7 ? &ENGINES[e]->edit[id - P_E0] : &TP[id];
                t->lock[i].step = l->step;
                t->lock[i].param = (uint8_t)id;
                t->lock[i].val = (int16_t)clamp(l->val, d->min, d->max);
            } else {
                t->lock[i].step = LOCK_FREE;            /* no such step or parameter: the slot is free */
                t->lock[i].param = 0;
                t->lock[i].val = 0;
            }
        }
        if (!drum && p->rsv[0] < PROJ_VOICE_V)          /* (wave 4) its filter's locks onto the voice filter's */
            voice_locks_from_engine(t, e);
        if (!drum && p->rsv[0] < PROJ_SEND_V)           /* (wave 8) a lock of the old FILT: the filter is gone (it had no
                                                         * page to lock it from: only a lock the editor made) */
            for (i = 0; i < NLOCK; i++)
                if (t->lock[i].step != LOCK_FREE && t->lock[i].param == P_TFLT)
                    t->lock[i].step = LOCK_FREE, t->lock[i].param = 0, t->lock[i].val = 0;
        if (!drum && p->rsv[0] >= PROJ_FX_V) {          /* (wave 5) its matrix, LFO 2, Spice and Dice */
            mx_unpack(p->t[k].mod, k);
            if (p->rsv[0] < PROJ_SEND_V)                /* (wave 8) its send slots: OFF, the sends their own */
                fx_sends_from_slots(t->p, t->lock, (int32_t)k);
        } else if (!drum) {                             /* saved before: its DIST, INSERT and sends as FX slots (their
                                                         * locks with them), LFO > SHP and ENV > SHP as cells */
            mx_clear_sound(k);
            mxt[k].spice = mxt[k].dice = 0;
            t->p[P_L2RATE] = TP[P_L2RATE].def;
            t->p[P_L2WAVE] = TP[P_L2WAVE].def;
            fx_from_legacy(t->p, t->lock);
            mx_from_legacy(k, t->p, e);
        }
        if (!drum && !eng_allowed(t, t->eng_req)) {     /* an engine gone, or not this track's: its own, as it
                                                         * starts (not the drum track: it has none, and this put its
                                                         * kit back to HEX BOOM at every load, power-on and section
                                                         * until 2026-10-28) */
            uint32_t k2;
            t->eng_req = (uint8_t)eng_default(t);
            for (k2 = 0; k2 < 8u; k2++)
                t->p[P_E0 + k2] = ENGINES[t->eng_req]->edit[k2].def;
            t->preset = 0;
        }
    }
    drum_rt_clear();                                    /* (nothing of the last pattern waits or retrigs) */
}

/* (PHEBUS wave 2) the four slots (the sections A..D) in RAM, a project's core each (5,744 bytes): A and B in .noinit (they
 * survive a reset, as all four did), C and D in main RAM (the .noinit region holds no more: after a reset C and D are read
 * from flash again, which an update or UPDATE MODE writes first, persist_flush_now; only a section stored while playing
 * and not yet in flash is lost if the FM-1 crashes). (Wave 3: C moved out of the pool into main RAM, which wave 3's
 * clean-up freed 4.8 KB of: the pool, which tools/build.py keeps 8 KiB of spare, has 5.7 KB more for waves 4 and 5) */
static proj_core_t proj_sec_a[2] __attribute__((section(".noinit")));
static proj_core_t proj_sec_c;
static proj_core_t proj_sec_d;
static proj_core_t *const proj_slots[4] = {&proj_sec_a[0], &proj_sec_a[1], &proj_sec_c, &proj_sec_d};
#define PSLOT(s) (proj_slots[(s) & 3u])
/* the main loop's scratch for a project: the working one captured (its sum: autosave_hash), a slot fetched, format 7 made
 * of an older one (proj_import) */
static proj_core_t autosave_buf __attribute__((section(".pool")));
#define PROJ_V7_TMP ((project_v7_t *)&autosave_buf)
_Static_assert(sizeof(project_v7_t) <= sizeof(proj_core_t), "format 7 is made in autosave_buf");

/* ---- when the working project goes to flash (autosave_tick below; on the host: tests/project_test.c). PHEBUS, 21 Nov
 * 2026 (Ben: "Dot + save on stop"): each time the transport stops, once the panel has rested AUTOSAVE_STOP_IDLE and the
 * output is silent (stop_quiet: a flash erase stalls the audio ~50 ms, so never under a tail), at most every
 * AUTOSAVE_STOP_GAP (the two sectors' wear); else, as before, AUTOSAVE_IDLE untouched and AUTOSAVE_GAP apart (the
 * output then need not be silent: a long tail may hold it). quiet: no voice sounds */
#define AUTOSAVE_IDLE 2500u                    /* ms without input */
#define AUTOSAVE_GAP 20000u                    /* ms between two saves at least */
#define AUTOSAVE_STOP_IDLE 300u                /* after a stop: ms without input (the STOP press itself is input) */
#define AUTOSAVE_STOP_GAP 5000u                /* after a stop: ms since the last save at least */
static int autosave_due(uint32_t stop, uint32_t stop_quiet, uint32_t quiet, uint32_t idle, uint32_t gap)
{
    if (!quiet)
        return 0;
    if (stop && stop_quiet && idle >= AUTOSAVE_STOP_IDLE && gap >= AUTOSAVE_STOP_GAP)
        return 1;
    return idle >= AUTOSAVE_IDLE && gap >= AUTOSAVE_GAP;
}

#ifndef PROJ_HOST
#if FELUCCA_ARRANGER
#include "arranger_scene.c"
#endif
static uint8_t sec_dirty, song_dirty;           /* live sections / the settings: in RAM, not yet in flash */
#if FELUCCA_FLASH
/* slot from flash into RAM: format 8 (a big object); else, once, the old one converted (format 7 and older, never written
 * again) */
static void proj_fetch(uint32_t slot)
{
    int n = st_load(OBJ_PRJ0 + (slot & 3u), &proj_tmp, sizeof proj_tmp);
    if (n < 0)
        n = st_load(OBJ_PROJECT0 + (slot & 3u), &proj_tmp, sizeof(project_v7_t));
    if (!proj_import(PSLOT(slot), &proj_tmp, n, PROJ_V7_TMP))
        PSLOT(slot)->magic = 0;
}
#endif

static void persist_flush_now(void);               /* (below) */
static void project_save(uint32_t slot)
{
    proj_core_t *p = PSLOT(slot);
#if FELUCCA_ARRANGER
    if (song.playing || transport_req) { ui_message("STOP BEFORE SAVE"); return; }
#endif
    proj_capture_core(p);
#if FELUCCA_ARRANGER
    live_sec = -1;                                      /* (3.0.3: saved from the menu or restored: no section playing) */
#endif
#if FELUCCA_FLASH
    if (flash_ok) {
        int err = st_save_big(OBJ_PRJ0 + (slot & 3u), p, sizeof *p, 0);
        if (!err) {                                 /* (2026-10-28, Ariel: the kit came back as before) what power-on */
            if (dsu_dirty)                          /* loads, now too: the working project (else only autosaved when */
                song_dirty = 1;                     /* idle, 20 s apart) and an edited HEX kit with the settings */
            persist_flush_now();
        }
        ui_message(err ? "SAVE ERROR" : "SAVED");
        return;
    }
#endif
    ui_message("SAVED (RAM)");
}

/* SAVE > PROJECT > ERASE (SLOOP 2.6, after a YouTube comment): the slot (section A..D) empty again. In flash as the
 * backup page's restore leaves an empty slot (project_restore): an object of no bytes, which proj_fetch (older SLOOP too)
 * reads as no project. Stopped only: a flash erase stops the audio ~50 ms */
static void project_erase(uint32_t slot)
{
    slot &= 3u;
#if FELUCCA_ARRANGER
    if (song.playing || transport_req) { ui_message("STOP BEFORE ERASE"); return; }
#endif
    if (!proj_ok(PSLOT(slot))) {
        ui_message("EMPTY PATTERN");
        return;
    }
#if FELUCCA_FLASH
    if (flash_ok && (st_save_big(OBJ_PRJ0 + slot, PSLOT(slot), 0, 0) ||
                     (st_load(OBJ_PROJECT0 + slot, &proj_tmp, 4) >= 0 && st_save(OBJ_PROJECT0 + slot, &proj_tmp, 0)))) {
        ui_message("ERASE ERROR");                      /* (an old object there too: emptied, so it does not come back) */
        return;
    }
#endif
    fm1_irq_off();
    memset(PSLOT(slot), 0, sizeof *PSLOT(slot));
    sec_dirty &= (uint8_t)~(1u << slot);
    if (live_sec == (int8_t)slot)
        live_sec = -1;
    fm1_irq_on();
    ui_message("ERASED");
}

/* a project into the working one: the transport stops, everything sounding is released */
static void project_apply(const proj_core_t *p)
{
    uint32_t k;
    transport_req = 2;
    panic_req = (1u << NTRK) - 1u;
    drum_mute = 0;                                      /* (the live mutes: a performance's, not the project's) */
    fm1_irq_off();                                      /* the audio ISR must not see half a project */
    proj_apply(p, 1);
    song.sel = (uint8_t)(p->sel < NTRK ? p->sel : 0u);
    fm1_irq_on();
    for (k = 0; k < NPART; k++)                         /* a format 1 project: the default sounds of tracks 2, 3 */
        if (p->t[k].preset == 0xFFu) {
            apply_preset_to(&trk[k], TRK_DEF[k][1]);
            steps_clear(&trk[k]);
        }
    sync_reload = 1;
    ui.force = 1;
}

static void project_load(uint32_t slot)
{
    proj_core_t *p = PSLOT(slot);
#if FELUCCA_ARRANGER
    if (song.playing || transport_req) { ui_message("STOP BEFORE LOAD"); return; }
#endif
#if FELUCCA_FLASH
    if (flash_ok && !proj_ok(p))
        proj_fetch(slot);
#endif
    if (!proj_ok(p)) {
        ui_message("EMPTY PATTERN");
        return;
    }
    project_apply(p);
#if FELUCCA_ARRANGER
    live_sec = -1;                                      /* (3.0.3; 2026-10-31, Ariel: every slot came to play the same song)
                                                         * a project loaded from the menu or restored is no section
                                                         * playing: edits stick kept it in the section last played, a
                                                         * moment after any touch. Only SAVE + key loads a section */
#endif
    ui_message("LOADED");
}

/* ---- the working project, kept in flash by itself: saved when it changed, the transport is stopped,
 * nothing sounds and the panel was not touched for AUTOSAVE_IDLE (a flash erase stops the audio for
 * ~50 ms: never while something plays); loaded at power-on (autosave_resume). autosave_buf (above) is a scratch: what
 * is in flash is known by its sum (autosave_hash), so the buffer also serves proj_fetch and a backup's restore */
static uint32_t autosave_hash, autosave_ms, autosave_checked, unsaved_ms;
static uint8_t stop_save, was_running;         /* (PHEBUS) the transport stopped, not saved since; it ran last frame */

static int audio_quiet(void)
{
    uint32_t p, i;
    for (p = 0; p < NPART; p++)
        for (i = 0; i < NVOICE; i++)
            if (trk[p].v[i].active)
                return 0;
    for (i = 0; i < NDRUM; i++)
        if (drums.v[i].active)
            return 0;
    return 1;
}
#if FELUCCA_FLASH
static int out_silent(void)                    /* (PHEBUS) the last 512 samples of the mix (every other one) under -60 dB */
{
    return out_quiet >= SCOPE_N;                /* (audio.c counts them: wave 3, its scope buffers gone) */
}

/* (PHEBUS) the header's unsaved dot (ui_op.c proj_unsaved): the working project as it is now against the one in flash,
 * twice a second; a lock in force counts as its base (seq.c lk_*: the knob's value, not the step's); the live sections,
 * the settings and an edited kit not yet in flash count too */
static void unsaved_tick(void)
{
    uint32_t i, k;
    if (!flash_ok || fm1_ms - unsaved_ms < 500u)
        return;
    unsaved_ms = fm1_ms;
    proj_capture_core(&autosave_buf);
    for (i = 0; i < NTRK; i++)
        for (k = 0; k < trk[i].lk_n && k < NLOCK; k++)
            if (trk[i].p[trk[i].lk_param[k] % P_COUNT] == trk[i].lk_set[k])
                (i == TRK_DRUM ? autosave_buf.d.p : autosave_buf.t[i].p)[trk[i].lk_param[k] % P_COUNT] = trk[i].lk_base[k];
    proj_unsaved = (uint8_t)(work_sum(proj_sum(&autosave_buf)) != autosave_hash || sec_dirty || song_dirty || dsu_dirty);
}
#endif

static void autosave_tick(void)                /* main loop */
{
#if FELUCCA_FLASH
    uint32_t h, now = fm1_ms, run = song.playing || transport_req;
    if (was_running && !run)
        stop_save = 1;                         /* (PHEBUS) stopped: saved as soon as it is silent */
    was_running = (uint8_t)run;
    unsaved_tick();
    if (!flash_ok || run || rec_wait || ui.menu || now - autosave_checked < (stop_save ? 100u : 1000u) ||
        !autosave_due(stop_save, stop_save && out_silent(), 1, now - ui_input_ms, now - autosave_ms))
        return;
    autosave_checked = now;
    proj_capture_core(&autosave_buf);
    h = work_sum(autosave_buf.sum);
    if (h == autosave_hash) {
        stop_save = 0;                         /* (nothing changed since the last save) */
        return;
    }
    if (!audio_quiet())
        return;
    if (st_save_big2(OBJ_WORK, &autosave_buf, sizeof autosave_buf, &pkit, sizeof pkit,
                     sizeof(proj_ext_t) - sizeof pkit) == 0)   /* (the ext: the project's kit, then zeros) */
        autosave_hash = h;
    autosave_ms = fm1_ms;
    stop_save = 0;
    unsaved_ms = fm1_ms - 500u;                /* (the dot: again at the next frame) */
#endif
}

static uint8_t drch_seed;                           /* a settings object as 2.5 wrote it: the drum channel was the
                                                     * project's then (autosave_resume takes it from a 2.5 autosave) */
static void autosave_resume(void)              /* power-on: the project as it was left (felucca_init) */
{
#if FELUCCA_FLASH
    proj_core_t *q = (proj_core_t *)proj_tmp.raw;
    int n, old = 0;
    if (!flash_ok)
        return;
    n = st_load(OBJ_WORK, &proj_tmp, sizeof proj_tmp);
    if (n < 0) {                                       /* (PHEBUS wave 2) none yet: the old one, once, converted */
        n = st_load(OBJ_AUTOSAVE, &proj_tmp, sizeof(project_v7_t));
        old = 1;
        if (n == (int)sizeof(project_v7_t) || n == (int)sizeof(project_v6_t)) {   /* (2.6) updated from 2.5: GLO > DRUMS
            * > CH as it was there (a project's then, a setting of the FM-1 now), kept in the settings */
            const project_v6_t *o = (const project_v6_t *)proj_tmp.raw;
            if (drch_seed && !(o->rsv[0] & 0x80u) && o->g[G_DRCH] >= 0 && o->g[G_DRCH] <= 16 && o->g[G_DRCH] != midi_set[3]) {
                midi_set[3] = (uint8_t)o->g[G_DRCH];
                settings_later = 1;
            }
        }
    }
    drch_seed = 0;
    if (!proj_import(q, &proj_tmp, n, PROJ_V7_TMP))
        return;
    proj_kit_take(proj_tmp.raw, old ? 0 : n, q);        /* (wave 12) the project's kit, from its ext (older: converted) */
    autosave_hash = old ? 0u : work_sum(q->sum);       /* (converted: saved in the new format when next quiet) */
    proj_apply(q, 1);
    song.sel = (uint8_t)(q->sel < NTRK ? q->sel : 0u);
    for (n = 0; n < NPART; n++)
        trk[n].engine = trk[n].eng_req;        /* (nothing sounds yet: no fade) */
#endif
}

/* settings + learned panel table: one flash object. The flash copy wins at
 * boot (the .noinit copies are garbage after a power-off). */
typedef struct {
    uint32_t magic, palette, lowcut, zoom;
    panel_t panel;
    uint8_t song_rsv[36];                          /* (PHEBUS wave 1) where the song order was (KARMA's SONG screen, cut):
                                                    * kept so the layout stays; read and written as zeros, ignored */
    uint32_t lights;                               /* SLOOP 2.3: the backlight (panel.c lights_word); appended,
                                                    * so 2.2 still reads its part (st_load cuts at its size) */
} persist_t;
#define PERSIST_SIZE_V22 __builtin_offsetof(persist_t, lights)   /* the settings as 2.2 wrote them (no lights) */
_Static_assert(sizeof(persist_t) == PERSIST_SIZE_V22 + 4u, "lights: the last word, no padding before it");
_Static_assert(__builtin_offsetof(persist_t, song_rsv) == 48u, "the old song order: bytes 48..83 of the settings");
#define PERSIST_MAGIC 0x50455233u                  /* "PER3" (with the old song order's 36 bytes) */
#if FELUCCA_FLASH
static persist_t persist_saved;
static void dsu_boot(void);
#ifndef UKIT_RAM
#include "ukit_flash.c"                            /* (wave 12) the user drum kits in flash */
#endif
#endif

static void persist_boot(void)                    /* before settings_init / panel_init */
{
#if FELUCCA_FLASH
    persist_t p;
    uint32_t f = irq_save();
    flash_ok = FL_JEDEC_OK(FL_FAR(fl_jedec_ram)());      /* a known 1 MiB part (fm1_flash_id.h; 3.0.4 after SLOOP 2.8.5:
                                                          * the UC25HQ80 of FM-1s sold from late 2026 too), else RAM-only */
    irq_restore(f);
    if (!flash_ok)
        return;
    fl_plain_window_init();                        /* flash above 0x93000 reads as plaintext through XIP (KARMA's user
                                                    * sample sets were played from there; kept as it was) */
    {
        int n = st_load(OBJ_SETTINGS, &p, sizeof p);
        if (n == (int)PERSIST_SIZE_V22 && p.magic == PERSIST_MAGIC)
            p.lights = 0;                          /* from 2.2: backlight off */
        if (((n == (int)sizeof p || n == (int)PERSIST_SIZE_V22) && p.magic == PERSIST_MAGIC) ||
            (n == (int)(16u + sizeof(panel_t)) && p.magic == 0x50455232u)) {
            settings.magic = SETTINGS_MAGIC;
            settings.palette = p.palette;
            settings.lowcut = p.lowcut;
            settings.zoom = p.zoom;
            if (p.panel.magic == PANEL_MAGIC)
                panel = p.panel;
            if (p.magic == PERSIST_MAGIC)
                lights_from_word(p.lights);
            else
                p.lights = 0;
            memset(p.song_rsv, 0, sizeof p.song_rsv);   /* (an old song order: ignored, cleared at the next save) */
            persist_saved = p;
        } else if (n == (int)(8u + sizeof(panel_t)) && p.magic == 0x50455231u) {   /* "PER1": palette, panel */
            const uint32_t *w = (const uint32_t *)&p;
            panel_t old;
            memcpy(&old, w + 2, sizeof old);
            settings.magic = SETTINGS_MAGIC;
            settings.palette = w[1];
            settings.lowcut = 0;
            settings.zoom = 0;
            if (old.magic == PANEL_MAGIC)
                panel = old;
        }
    }
    {   /* projects: fill empty RAM slots from flash, so the slot list is right after power-on. A slot
         * still valid in RAM (a warm reset: an update, UPDATE MODE, a crash) may never have reached
         * flash (a live section stored while playing): marked to be written when quiet */
        uint32_t i;
        for (i = 0; i < 4u; i++)
            if (!proj_ok(PSLOT(i))) {
                proj_fetch(i);
                if (proj_ok(PSLOT(i)) && st_load(OBJ_PRJ0 + i, &proj_tmp, 4) < 0)
                    sec_dirty |= (uint8_t)(1u << i);   /* (converted from the old format: written in the new one) */
            } else {
                int n = st_load(OBJ_PRJ0 + i, &proj_tmp, sizeof proj_tmp);
                if (n != (int)sizeof *PSLOT(i) || memcmp(proj_tmp.raw, PSLOT(i), sizeof *PSLOT(i)))
                    sec_dirty |= (uint8_t)(1u << i);
            }
    }
    dsu_boot();                                    /* SLOOP 2.5: the SYN kits (after the settings) */
#ifndef UKIT_RAM
    ukit_boot();                                   /* (wave 12) the user kits' states */
#endif
    up_boot();                                     /* user presets */
#endif
}

static int project_used(uint32_t slot) { return proj_ok(PSLOT(slot)); }

#if FELUCCA_FLASH
/* SLOOP 2.5: the settings object holds the SYN kits after persist_t (drum_synth.c dsu): firmware before 2.5 reads
 * its persist_t and leaves the rest (st_load cuts at its size); its own next save drops them. 2.6: then the MIDI LEARN
 * map (seq.c ml_tab, ML_N entries), only while something is learned; 2.5 reads an object with the map as one without
 * its SYN kits (their defaults), one without it as its own */
/* the settings object's third part (2.6): the MIDI LEARN map (seq.c ml_tab, ML_N entries, 2 bytes each, low first),
 * then GLO > MIDI: CH1 CH2 CH3, the drum channel | OTHER << 7, GLO > MIDI 2's BEND, 3 spare. Written only while it is not
 * the default (nothing learned, channels 1 2 3 10, OTHER SEL, BEND 2): else the object is exactly 2.5's. 2.6 test
 * builds wrote 32 (the map) or 36 bytes (and the channels) */
#define MIDI_PART (2u * ML_N + 8u)
_Static_assert(sizeof(persist_t) + sizeof(dsu_bank_t) + MIDI_PART <= ST_PAYLOAD_MAX, "the settings object fits its sector");
static uint8_t midi_saved[MIDI_PART];
static uint8_t drch_seed;                           /* a settings object as 2.5 wrote it: the drum channel was the
                                                     * project's then (autosave_resume takes it from a 2.5 autosave) */               /* the part as flash has it (settings_save: written when it differs) */
static void midi_part_fill(uint8_t *b)
{
    uint32_t k;
    for (k = 0; k < ML_N; k++)
        b[2u * k] = (uint8_t)ml_tab[k], b[2u * k + 1u] = (uint8_t)(ml_tab[k] >> 8);
    b[2u * ML_N] = midi_set[0], b[2u * ML_N + 1u] = midi_set[1], b[2u * ML_N + 2u] = midi_set[2];
    b[2u * ML_N + 3u] = (uint8_t)(midi_set[3] | midi_set[4] << 7);
    b[2u * ML_N + 4u] = midi_set[5];
    b[2u * ML_N + 5u] = b[2u * ML_N + 6u] = b[2u * ML_N + 7u] = 0;
}
static int midi_part_default(const uint8_t *b)
{
    static const uint8_t def[6] = MIDI_SET_DEF;
    uint32_t k;
    for (k = 0; k < 2u * ML_N; k++)
        if (b[k])
            return 0;
    return b[2u * ML_N] == def[0] && b[2u * ML_N + 1u] == def[1] && b[2u * ML_N + 2u] == def[2] &&
           b[2u * ML_N + 3u] == (uint8_t)(def[3] | def[4] << 7) && b[2u * ML_N + 4u] == def[5];
}
/* (3.1.5) ACID GEN's settings, 8 bytes a synth track, came after the MIDI part (cut, PHEBUS wave 1): an object with them
 * still reads, the part is skipped; it is no longer written */
#define AG_PART (8u * NPART)
/* n: 32 (the map), 36 (and the channels) or MIDI_PART; every entry checked (a CC that learns, a parameter this knows, a
 * channel 0..16, a range 1..12) */
static void midi_part_read(const uint8_t *b, uint32_t n)
{
    uint32_t k, e;
    fm1_irq_off();                                  /* (the ISR reads the map and the channels: never half of them) */
    for (k = 0; k < ML_N; k++) {
        e = (uint32_t)b[2u * k] | (uint32_t)b[2u * k + 1u] << 8;
        ml_tab[k] = (uint16_t)((e >> 9) && ml_free_cc(e & 127u) && ml_id(e >> 9) < P_COUNT ? e : 0u);
    }
    if (n >= 2u * ML_N + 4u) {
        for (k = 0; k < 3u; k++)
            midi_set[k] = (uint8_t)(b[2u * ML_N + k] <= 16u ? b[2u * ML_N + k] : k + 1u);
        midi_set[3] = (uint8_t)((b[2u * ML_N + 3u] & 31u) <= 16u ? b[2u * ML_N + 3u] & 31u : 10u);
        midi_set[4] = (uint8_t)(b[2u * ML_N + 3u] >> 7);
        song.g[G_CH1] = midi_set[0], song.g[G_CH2] = midi_set[1], song.g[G_CH3] = midi_set[2];
        song.g[G_DRCH] = midi_set[3], song.g[G_CHOTH] = midi_set[4];
    }
    if (n >= MIDI_PART) {
        midi_set[5] = (uint8_t)(b[2u * ML_N + 4u] >= 1u && b[2u * ML_N + 4u] <= 12u ? b[2u * ML_N + 4u] : 2u);
        song.g[G_BEND] = midi_set[5];
    }
    fm1_irq_on();
}
static int settings_write(const persist_t *p)
{
    uint8_t mp[MIDI_PART];
    uint32_t n3;
    if (!dsu_valid(&dsu))
        dsu_defaults();
    midi_part_fill(mp);
    /* the third part only when it is not the default: without, the object is exactly 2.5's (2.5 reads it whole, SYN
     * kits included; with it, it takes the object for one without them) */
    n3 = midi_part_default(mp) ? 0u : MIDI_PART;
    if (st_save3(OBJ_SETTINGS, p, sizeof *p, &dsu, sizeof dsu, mp, n3))
        return -1;
    dsu_dirty = 0;
    memcpy(midi_saved, mp, sizeof midi_saved);
    return 0;
}
static void dsu_boot(void)                          /* the SYN kits (and the MIDI part) from the settings object, else */
{                                                   /* the defaults */
    st_hdr_t h;
    uint32_t len = sizeof(persist_t) + sizeof(dsu_bank_t), k;
    dsu_defaults();
    for (k = 0; k < ML_N; k++)
        ml_tab[k] = 0;
    if (st_current(OBJ_SETTINGS, &h) >= 0 &&
        (h.len == len || h.len == len + 2u * ML_N || h.len == len + 2u * ML_N + 4u || h.len == len + MIDI_PART ||
         h.len == len + MIDI_PART + AG_PART) &&
        dsu_valid((const dsu_bank_t *)(st_buf + sizeof(persist_t)))) {
        memcpy(&dsu, st_buf + sizeof(persist_t), sizeof dsu);
        for (k = 0; k < DSU_N; k++)
            dsu_fix_kit(&dsu.k[k]);
        if (h.len > len)
            midi_part_read(st_buf + len, h.len - len > MIDI_PART ? MIDI_PART : h.len - len);
        drch_seed = h.len == len;
    }
    midi_part_fill(midi_saved);
    dsu_dirty = 0;
}
#endif

static void persist_fill(persist_t *p)              /* the settings as they are now */
{
    memset(p, 0, sizeof *p);
    p->magic = PERSIST_MAGIC;
    p->palette = settings.palette;
    p->lowcut = settings.lowcut;
    p->zoom = settings.zoom;
    p->panel = panel;
    p->lights = lights_word();
}

static void settings_save(void)
{
#if FELUCCA_FLASH
    persist_t p;
    if (!flash_ok)
        return;
    persist_fill(&p);
    {
        uint8_t mp[MIDI_PART];
        midi_part_fill(mp);
        if (!memcmp(&p, &persist_saved, sizeof p) && !memcmp(mp, midi_saved, sizeof mp) && !dsu_dirty)
            return;                                /* unchanged: no erase cycle */
    }
    if (settings_write(&p) == 0)
        persist_saved = p;
#endif
}

#if FELUCCA_FLASH
_Static_assert(sizeof(project_t) <= ST_BIG_MAX, "a project fits a big flash object");

/* ---- backup restore (editor.c BK_PUT): each object checked as a load checks it, then written through the
 * same A/B commit as a save. rc: 0 ok, 2 not a valid object, 3 stop first, 4 flash */
static int panel_valid(const panel_t *q)           /* a permutation of the buttons and of the knobs */
{
    uint32_t i, b = 0, e = 0;
    if (q->magic != PANEL_MAGIC)
        return 0;
    for (i = 0; i < NB; i++) {
        if (q->btn[i] >= 14u || (b >> q->btn[i]) & 1u)
            return 0;
        b |= 1u << q->btn[i];
    }
    for (i = 0; i < NE; i++) {
        if (q->enc[i] >= 7u || (e >> q->enc[i]) & 1u || (q->dir[i] != 1 && q->dir[i] != -1))
            return 0;
        e |= 1u << q->enc[i];
    }
    return 1;
}

static uint32_t settings_restore(const void *raw, uint32_t n)
{
    persist_t p;
    if (n != sizeof p && n != PERSIST_SIZE_V22)
        return 2;
    memset(&p, 0, sizeof p);
    memcpy(&p, raw, n);
    if (p.magic != PERSIST_MAGIC || p.palette >= NPALETTES || p.lowcut > 1u || p.zoom > 1u || !panel_valid(&p.panel))
        return 2;
    memset(p.song_rsv, 0, sizeof p.song_rsv);       /* (a backup's old song order: ignored) */
    if (!flash_ok || settings_write(&p))
        return 4;
    persist_saved = p;
    settings.palette = p.palette;
    settings.lowcut = p.lowcut;
    settings.zoom = p.zoom;
    panel = p.panel;
    lights_from_word(p.lights);
    song.g[G_SYNC] = (int16_t)lights_sync;
    song.g[G_MIDI] = (int16_t)lights_mout;
    song.g[G_ROUTE] = (int16_t)lights_min;
    song.g[G_KNOBS] = (int16_t)midi_knobs;
    palette_set(settings.palette);
    fx_lowcut = (uint8_t)(settings.lowcut != 0);
    ui.force = 1;
    return 0;
}

/* slot 0..3 (n 0: empty), or 4: the working project (loaded now) */
static uint32_t project_restore(uint32_t slot, const void *raw, uint32_t n)
{
    if (song.playing || transport_req)
        return 3;
    if (slot < 4u && !n) {
        if (!flash_ok || st_save_big(OBJ_PRJ0 + slot, raw, 0, 0))
            return 4;
        memset(PSLOT(slot), 0, sizeof *PSLOT(slot));
        sec_dirty &= (uint8_t)~(1u << slot);
        return 0;
    }
    {   /* (raw: the staging RAM, proj_tmp) checked where it is; an older one through format 7 in autosave_buf, back here */
        proj_core_t *q = (proj_core_t *)proj_tmp.raw;
        if ((const void *)raw != (const void *)proj_tmp.raw || !proj_import(q, raw, (int)n, PROJ_V7_TMP))
            return 2;
        if (slot == 4u) {
            proj_kit_take(proj_tmp.raw, (int)n, q);     /* (wave 12: the kit in its ext; older: converted) */
            project_apply(q);
            return 0;
        }
        if (!flash_ok || st_save_big(OBJ_PRJ0 + slot, q, sizeof *q, 0))
            return 4;
        *PSLOT(slot) = *q;
    }
    sec_dirty &= (uint8_t)~(1u << slot);
    return 0;
}
#endif
#if FELUCCA_ARRANGER
/* ---- live sections (SAVE + key, ui_layers.c). A section is a project slot (A..D = 1..4): stored into RAM
 * at once (playing too), written to flash once the transport is stopped and nothing sounds (an erase
 * stops the audio for ~50 ms). */
static void section_store(uint32_t s)
{
    s &= 3u;
    fm1_irq_off();                                      /* (the audio ISR may be applying a section) */
    proj_capture_core(PSLOT(s));
    live_sec = (int8_t)s;
    sec_fresh = (int8_t)s;                              /* (3.0.5) the next edits: the loop only, not this part */
    fm1_irq_on();
    sec_dirty |= (uint8_t)(1u << s);
}
static void section_load(uint32_t s)                    /* stopped: the section is the loop now */
{
    s &= 3u;
    project_apply(PSLOT(s));
    live_sec = (int8_t)s;
    sec_fresh = -1;                                     /* (picked: edits stick to it again) */
}
static void sections_write(void)                        /* the dirty sections and the settings into flash */
{
    uint32_t i;
#if FELUCCA_FLASH
    if (flash_ok)
        for (i = 0; i < 4u; i++)
            if (((sec_dirty >> i) & 1u) && st_save_big(OBJ_PRJ0 + i, PSLOT(i), sizeof *PSLOT(i), 0) == 0)
                sec_dirty &= (uint8_t)~(1u << i);       /* (a failed write stays dirty: tried again later) */
    if (!flash_ok)
#endif
        sec_dirty = 0;
    (void)i;
    if (song_dirty) {
        song_dirty = 0;
        settings_save();
    }
}
/* before an intentional reset (an update, UPDATE MODE, UBOOT from the host): the audio is stopped, so
 * whatever is only in RAM goes to flash now: the live sections, the settings, the working project */
static void persist_flush_now(void)
{
    sections_write();
#if FELUCCA_FLASH
    if (flash_ok) {
        proj_capture_core(&autosave_buf);
        if (work_sum(autosave_buf.sum) != autosave_hash &&
            st_save_big2(OBJ_WORK, &autosave_buf, sizeof autosave_buf, &pkit, sizeof pkit,
                         sizeof(proj_ext_t) - sizeof pkit) == 0)
            autosave_hash = work_sum(autosave_buf.sum);
    }
#endif
}
static void sections_flush(void)                        /* main loop */
{
    static uint32_t tried;
    if ((uint32_t)song.g[G_SYNC] != lights_sync) {      /* GLO > SYSTEM > SYNC: kept with the settings */
        lights_sync = (uint8_t)song.g[G_SYNC];
        settings_later = 1;
    }
    if ((uint32_t)(song.g[G_MIDI] != 0) != lights_mout) {   /* GLO > SYSTEM > MIDI: the same */
        lights_mout = (uint8_t)(song.g[G_MIDI] != 0);
        settings_later = 1;
    }
    if ((uint32_t)song.g[G_ROUTE] != lights_min) {        /* GLO > SYSTEM > IN: the same (2.6: CH n too) */
        lights_min = (uint8_t)song.g[G_ROUTE];
        settings_later = 1;
    }
    if ((uint32_t)song.g[G_KNOBS] != midi_knobs) {        /* GLO > MIDI 3 > KNOBS: the same */
        midi_knobs = (uint8_t)song.g[G_KNOBS];
        settings_later = 1;
    }
    {   /* 2.6: GLO > MIDI (CH1 CH2 CH3 OTHER) and GLO > DRUMS > CH: the same */
        static const uint8_t gid[6] = {G_CH1, G_CH2, G_CH3, G_DRCH, G_CHOTH, G_BEND};
        uint32_t k;
        for (k = 0; k < 6u; k++)
            if ((uint32_t)song.g[gid[k]] != midi_set[k]) {
                midi_set[k] = (uint8_t)song.g[gid[k]];
                settings_later = 1;
            }
    }
    if (dsu_dirty)                                      /* a HEX kit edited (ui_studio.c hex_ui_sync): kept when quiet */
        settings_later = 1;
    if (settings_later) {                               /* the menu closed while playing */
        settings_later = 0;
        song_dirty = 1;                                 /* (settings_save when quiet) */
    }
#if FELUCCA_ARRANGER
    section_keep_ui(fm1_ms, ui_input_ms);              /* edits stick (arranger_scene.c) */
    if (sec_kept) {                                     /* sections that kept edits while they played (the ISR) */
        fm1_irq_off();
        sec_dirty |= sec_kept;
        sec_kept = 0;
        fm1_irq_on();
    }
#endif
    if ((!sec_dirty && !song_dirty) || song.playing || transport_req || !audio_quiet() || fm1_ms - ui_input_ms < 1500u ||
        fm1_ms - tried < 5000u)
        return;
    tried = fm1_ms;                                     /* (a failed write: again in 5 s, not every frame) */
    sections_write();
    if (sec_dirty)
        ui_message("SAVE ERROR: RETRYING");
}
#endif
#endif /* PROJ_HOST */
