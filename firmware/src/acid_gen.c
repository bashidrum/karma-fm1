/* SPDX-License-Identifier: GPL-3.0-only */
/* ACID GEN: TB-3PO's acid lines (X0X's firmware/src/seq/tb3po.c, Charles Vestal, GPL-3.0: a port of
 * schwung-tb3po's generator, itself of the Phazerville Hemisphere Suite's TB_3PO applet, djphazer and
 * contributors, GPL-3.0) written into a SLOOP synth track's steps. The same random draws in the same order as
 * X0X's, so a seed and the settings give the same line as there.
 *
 * SEQ > ACID GEN, the selected synth track: DENS (how many steps play), ACC (how many are accented), SLIDE (how
 * many slide into the next), SEED (each value a line of its own); ACID GEN 2: SCALE, OCT (the octaves it spans),
 * ROOT, MUTATE (GO: a quarter of the steps rolled again). A knob turned writes the line at once, over the track's
 * PATTERN length; the first one after the page opens keeps the pattern that was there for UNDO. The settings
 * belong to the FM-1, not to a project: the line they made is the track's pattern, and that is kept. (3.1.5) Each synth
 * track has its own (Ariel: "it should remember each parameter for each track always"; they were one set, at its
 * defaults after every power-on): the pages show the selected track's, kept across power-off in the settings object
 * (project.c ag_part). */
#define AG_NSCALES 6
typedef struct { int8_t deg[7]; uint8_t len; } ag_scale_t;
static const ag_scale_t AG_SCALES[AG_NSCALES] = {    /* schwung-tb3po's SCALES, same order */
    {{0, 2, 3, 5, 7, 8, 10}, 7},                     /* Minor */
    {{0, 1, 3, 5, 7, 8, 10}, 7},                     /* Phrygian */
    {{0, 2, 3, 5, 7, 8, 11}, 7},                     /* HarmMinor */
    {{0, 3, 5, 7, 10, 0, 0}, 5},                     /* MinPent */
    {{0, 2, 3, 5, 7, 9, 10}, 7},                     /* Dorian */
    {{0, 2, 4, 5, 7, 9, 11}, 7},                     /* Major */
};
typedef struct {
    uint8_t density, accent, slide;                  /* probabilities 0..100 % */
    uint8_t oct_range;                               /* 1..3 */
    uint8_t root, scale, base_oct;                   /* note = 12 (base_oct + 1) + root + degree */
    uint32_t seed;
} ag_cfg_t;
enum { AG_REST, AG_NOTE, AG_ACC, AG_SLIDE };

/* (3.1.5) each synth track's settings: song.g[G_AGDENS ..] is the selected one's (the pages' proxies) */
typedef struct { uint8_t v[6]; uint16_t seed; } ag_trk_t;   /* DENS ACC SLIDE SCALE OCT ROOT, SEED */
static const uint8_t AG_ID[6] = {G_AGDENS, G_AGACC, G_AGSLD, G_AGSCALE, G_AGOCT, G_AGROOT};
static ag_trk_t ag_trk[NPART];
static uint8_t ag_dirty;                             /* changed since the settings object was written (project.c) */
static uint8_t ag_ready;                             /* ag_trk set: the defaults, then the settings object's (dsu_boot) */
static void ag_defaults(void)
{
    uint32_t k, i;
    ag_ready = 1;
    for (k = 0; k < NPART; k++) {
        for (i = 0; i < 6u; i++)
            ag_trk[k].v[i] = (uint8_t)GP[AG_ID[i]].def;
        ag_trk[k].seed = (uint16_t)GP[G_AGSEED].def;
    }
}
static void ag_to_g(uint32_t k)                      /* track k's into the pages */
{
    uint32_t i;
    for (i = 0; i < 6u; i++)
        song.g[AG_ID[i]] = ag_trk[k % NPART].v[i];
    song.g[G_AGSEED] = (int16_t)ag_trk[k % NPART].seed;
}
static void ag_from_g(uint32_t k)                    /* the pages into track k's */
{
    uint32_t i;
    for (i = 0; i < 6u; i++)
        ag_trk[k % NPART].v[i] = (uint8_t)clamp(song.g[AG_ID[i]], GP[AG_ID[i]].min, GP[AG_ID[i]].max);
    ag_trk[k % NPART].seed = (uint16_t)clamp(song.g[G_AGSEED], GP[G_AGSEED].min, GP[G_AGSEED].max);
    ag_dirty = 1;
}
static void ag_key_all(uint32_t root, uint32_t scale)   /* SEQ > HARMONY's key: every track's (gen_seq.c gen_key_set) */
{
    uint32_t k;
    for (k = 0; k < NPART; k++)
        ag_trk[k].v[5] = (uint8_t)root, ag_trk[k].v[3] = (uint8_t)scale;
    ag_dirty = 1;
}

static uint32_t ag_rng_u32(uint32_t *r)              /* xorshift32, as schwung-tb3po */
{
    uint32_t x = *r;
    if (x == 0)
        x = 1;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *r = x;
    return x;
}
static float ag_rng_f(uint32_t *r) { return (float)(ag_rng_u32(r) & 0xFFFFFFu) / (float)0x1000000; }
static uint8_t ag_note(const ag_cfg_t *g, int degree, int oct)
{
    const ag_scale_t *sc = &AG_SCALES[g->scale < AG_NSCALES ? g->scale : 0];
    return (uint8_t)clamp(12 * (g->base_oct + 1) + g->root + sc->deg[degree] + 12 * oct, 0, 127);
}
static int ag_degree(uint32_t *r, const ag_scale_t *sc)
{
    int d = (int)(ag_rng_f(r) * (float)sc->len);
    return d >= sc->len ? sc->len - 1 : d;
}
static int ag_oct(uint32_t *r, int range)
{
    int o = (int)(ag_rng_f(r) * (float)range);
    return o >= range ? range - 1 : o;
}
static uint8_t ag_kind(uint32_t *r, const ag_cfg_t *g)   /* (a SLIDE step is not also an ACCENT, as there) */
{
    uint8_t f = AG_NOTE;
    if (ag_rng_f(r) < (float)g->accent / 100.0f)
        f = AG_ACC;
    if (ag_rng_f(r) < (float)g->slide / 100.0f)
        f = AG_SLIDE;
    return f;
}
static void ag_put(step_t *s, uint8_t note, uint8_t kind)   /* a TB-3PO step as a SLOOP step */
{
    memset(s, 0, sizeof *s);
    s->note[0] = note;                               /* (a rest keeps its note too: a later VARY may play it, as there) */
    if (kind == AG_REST) {
        s->time = ST_REST;
        return;
    }
    s->n = 1;
    s->note[0] = note;
    s->time = ST_NOTE;
    s->flags = kind == AG_ACC ? SF_ACCENT : kind == AG_SLIDE ? SF_SLIDE : 0u;
}
static uint8_t ag_kind_of(const step_t *s)
{
    return !s->n || s->time == ST_REST ? AG_REST : (s->flags & SF_SLIDE) ? AG_SLIDE : (s->flags & SF_ACCENT) ? AG_ACC : AG_NOTE;
}
static void ag_fix_slides(track_t *t, int len)       /* a slide into a rest does nothing */
{
    int i;
    for (i = 0; i < len; i++)
        if ((t->step[i].flags & SF_SLIDE) && ag_kind_of(&t->step[(i + 1) % len]) == AG_REST)
            t->step[i].flags &= (uint8_t)~SF_SLIDE;
}

/* tb3po_generate: the whole line over len steps */
static void ag_generate(track_t *t, const ag_cfg_t *g, int len)
{
    const ag_scale_t *sc = &AG_SCALES[g->scale < AG_NSCALES ? g->scale : 0];
    int range = clamp(g->oct_range, 1, 3), i, any = 0, deg, oct;
    uint32_t r = g->seed ? g->seed : 1;
    uint8_t kind[NSTEP], note[NSTEP];
    len = clamp(len, 1, NSTEP);
    for (i = 0; i < NSTEP; i++) {
        kind[i] = AG_REST;
        note[i] = ag_note(g, 0, 0);
    }
    for (i = 0; i < len; i++) {
        if (ag_rng_f(&r) > (float)g->density / 100.0f)
            continue;
        if (i % 4 == 0 && ag_rng_f(&r) < 0.35f)
            deg = 0;                                 /* on the beat: the root, often */
        else
            deg = ag_degree(&r, sc);
        oct = ag_oct(&r, range);
        note[i] = ag_note(g, deg, oct);
        kind[i] = ag_kind(&r, g);
    }
    for (i = 0; i < len; i++)
        any |= kind[i] != AG_REST;
    if (!any)
        kind[0] = AG_NOTE;
    for (i = 0; i < NSTEP; i++)
        ag_put(&t->step[i], note[i], i < len ? kind[i] : AG_REST);
    ag_fix_slides(t, len);
}

/* tb3po_mutate: about a quarter of the steps rolled again, the random stream going on from *rng */
static void ag_mutate(track_t *t, const ag_cfg_t *g, int len, uint32_t *rng)
{
    const ag_scale_t *sc = &AG_SCALES[g->scale < AG_NSCALES ? g->scale : 0];
    int range = clamp(g->oct_range, 1, 3), i;
    uint32_t r = *rng ? *rng : (g->seed ? g->seed : 1);
    len = clamp(len, 1, NSTEP);
    for (i = 0; i < len; i++) {
        step_t *s = &t->step[i];
        if (ag_rng_f(&r) >= 0.25f)
            continue;
        if (ag_rng_f(&r) < 0.5f) {                   /* the step */
            uint8_t k = ag_rng_f(&r) > (float)g->density / 100.0f ? AG_REST : ag_kind(&r, g);
            ag_put(s, s->note[0], k);
        } else {                                     /* the note */
            int deg = ag_degree(&r, sc), oct = ag_oct(&r, range);
            s->note[0] = ag_note(g, deg, oct);
        }
    }
    ag_fix_slides(t, len);
    *rng = r;
}

/* the page's knobs -> the selected synth track's line, once a frame (ui_studio.c pads_tick) */
static void acid_gen_sync(void)
{
    static int16_t was[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
    static uint8_t open_was, sel_was = 0xFFu;
    static uint32_t mut_rng;
    const int16_t *g = song.g;
    const char *ti = cur_page()->title, *pre = "ACID GEN";
    uint32_t i, open = !ui.home && song.sel < NPART, changed = 0, mut = 0;
    for (i = 0; pre[i] && open; i++)                 /* (its two pages: the titles start so) */
        open = ti[i] == pre[i];
    if (!ag_ready)                                   /* (a build without flash: the defaults) */
        ag_defaults();
    if (song.sel < NPART && song.sel != sel_was) {   /* (3.1.5) another track: its settings on the pages, no line written */
        sel_was = song.sel;
        ag_to_g(sel_was);
        for (i = 0; i < 7u; i++)
            was[i] = g[G_AGDENS + i];
    }
    for (i = 0; i < 8u; i++) {
        if (g[G_AGDENS + i] != was[i]) {
            if (i == 7u)
                mut = g[G_AGMUT] != 0;
            else
                changed = 1;
            was[i] = g[G_AGDENS + i];
        }
    }
    if (changed && sel_was < NPART)
        ag_from_g(sel_was);                          /* (3.1.5: the track's own, kept) */
    if (open && !open_was)
        undo_sess++;                                 /* (the first line written here keeps the old pattern for UNDO) */
    open_was = (uint8_t)open;
    if (!open || (!changed && !mut))
        return;
    {
        track_t *t = TSEL;
        ag_cfg_t c;
        int len = (int)trk_len(t);
        c.density = (uint8_t)(g[G_AGDENS] * 100 / 127);
        c.accent = (uint8_t)(g[G_AGACC] * 100 / 127);
        c.slide = (uint8_t)(g[G_AGSLD] * 100 / 127);
        c.seed = 0x9E3779B9u * (uint32_t)(g[G_AGSEED] + 1);
        c.scale = (uint8_t)g[G_AGSCALE];
        c.oct_range = (uint8_t)g[G_AGOCT];
        c.root = (uint8_t)g[G_AGROOT];
        c.base_oct = 2;                              /* C2.. (TB-3PO's: C1; a bass track transposes as it likes) */
        undo_mark(t, undo_sess);
        if (mut) {
            ag_mutate(t, &c, len, &mut_rng);
            song.g[G_AGMUT] = 0;
            was[7] = 0;
        } else {
            ag_generate(t, &c, len);
            mut_rng = 0;
        }
        t->seq_active = 1;
        ui.force = 1;
    }
}
