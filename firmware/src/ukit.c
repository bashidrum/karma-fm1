/* SPDX-License-Identifier: GPL-3.0-only */
/* (PHEBUS wave 12) Whole kits and single sounds: the lists PRESETS browses, the user kits, and what happens to a slot's
 * locks when it takes a sound of the other machine. Included by drums.c after drum_snd.c (no UI here: ui_kit.c).
 *
 *   KITS   (HOME's PRESETS, SAVE > PRESETS on the drums, SAVE > KITS) the five HEX kits, the 32 CLASSIC kits, then the user
 *          kits that hold something: MY KIT 1 .. 6 (stored with SAVE > KITS). MY KIT 1 .. 4 never stored yet are the old
 *          MY HEX 1 .. 4 (their bytes in the settings object, drum_synth.c dsu): they load as they always did.
 *   SOUNDS (the drum screen's PRESETS, for the selected slot; Ben: "Same kind first, then all") INIT HEX, INIT CLASSIC
 *          first, then the slot's own kind from every kit in kit order, then every other kind, KICK to COWBELL, each from
 *          every kit. A kind is a slot's role: the 16 slots every kit shares (the HEX kits and MY HEX keep the same 16;
 *          a user kit's sound is of the kind of the slot it sits in there). The list wraps.
 *
 * The user kits are kept in flash on the FM-1 (ukit_flash.c, an object of their own: 6 kits, 3,564 bytes, read when
 * needed, never mirrored in RAM); on the host and the simulator (UKIT_RAM) in RAM. */
static uint32_t ukit_state(uint32_t n);                   /* 0 never stored, 1 stored, 2 erased */
static int ukit_get(uint32_t n, pkit_t *k);               /* the whole kit (1: read) */
static int ukit_slot(uint32_t n, uint32_t l, dslot_t *d); /* one of its slots (1: read) */
static int ukit_put(uint32_t n, const pkit_t *k);         /* stored (k 0: erased); 0 done */
enum { UK_NONE, UK_STORED, UK_ERASED };

#ifdef UKIT_RAM
static pkit_t uk_ram[UKIT_N];
static uint8_t uk_ram_st[UKIT_N];
static uint32_t ukit_state(uint32_t n) { return n < UKIT_N ? uk_ram_st[n] : UK_ERASED; }
static int ukit_get(uint32_t n, pkit_t *k)
{
    if (ukit_state(n) != UK_STORED)
        return 0;
    *k = uk_ram[n];
    return 1;
}
static int ukit_slot(uint32_t n, uint32_t l, dslot_t *d)
{
    if (ukit_state(n) != UK_STORED)
        return 0;
    *d = uk_ram[n].s[l & 15u];
    return 1;
}
static int ukit_put(uint32_t n, const pkit_t *k)
{
    if (n >= UKIT_N)
        return -1;
    if (k)
        uk_ram[n] = *k;
    else
        memset(&uk_ram[n], 0, sizeof uk_ram[n]);
    uk_ram_st[n] = (uint8_t)(k ? UK_STORED : UK_ERASED);
    return 0;
}
#endif

/* ---- names */
static void ukit_name(uint32_t n, char *b, uint32_t m)    /* "MY KIT 1" (an old MY HEX not yet stored over: its own name) */
{
    if (ukit_state(n) == UK_NONE && n < DSU_N) {
        char t[9];
        memcpy(t, dsu_kit(n)->name, 8);
        t[8] = 0;
        str_cpy(b, t, m);
        return;
    }
    str_cpy(b, "MY KIT ", m);
    if (m > 8u)
        b[7] = (char)('1' + n % 10u), b[8] = 0;
}
static void kit_name_of(uint32_t src, char *b, uint32_t n)
{
    if (src >= DSRC_USER && src < DSRC_USER + UKIT_N) {   /* (a user kit: its fixed name, MY KIT 1 .. 6) */
        str_cpy(b, "MY KIT ", n);
        if (n > 8u)
            b[7] = (char)('1' + (src - DSRC_USER)), b[8] = 0;
        return;
    }
    if (src >= DRUM_KITS || drum_kit_none(src)) {
        str_cpy(b, "?", n);
        return;
    }
    if (src >= DRUM_SYN) {
        char t[9];
        memcpy(t, dsu_kit(src - DRUM_SYN)->name, 8);
        t[8] = 0;
        str_cpy(b, t, n);
        return;
    }
    str_cpy(b, DRUM_KIT_NAMES[src], n);
}
/* the kit's name as HOME says it: the whole kit loaded, while no slot differs from it; CUSTOM once one does */
static void drum_kit_name(char *b, uint32_t n)
{
    if (kit_changed())
        str_cpy(b, "CUSTOM", n);
    else
        kit_name_of(pkit.base, b, n);
}

/* ---- the kits PRESETS loads, in order: HEX (5), CLASSIC (32), the user kits there are */
#define KIT_BUILTIN (DRUM_SAMPLED + DS_NKITS)
static uint32_t kit_src_n(void)
{
    uint32_t n = KIT_BUILTIN, u;
    for (u = 0; u < UKIT_N; u++)
        n += ukit_state(u) == UK_STORED || (u < DSU_N && ukit_state(u) == UK_NONE);
    return n;
}
static uint32_t kit_src(uint32_t i)          /* the i-th kit: a kit's number (P_E0's: MY HEX too) or DSRC_USER + n */
{
    uint32_t u;
    if (i < KIT_BUILTIN)
        return i;
    i -= KIT_BUILTIN;
    for (u = 0; u < UKIT_N; u++) {
        if (ukit_state(u) == UK_STORED) {
            if (!i--)
                return DSRC_USER + u;
        } else if (u < DSU_N && ukit_state(u) == UK_NONE) {
            if (!i--)
                return DRUM_SYN + u;
        }
    }
    return DRUM_DEFAULT_KIT;
}
static const char *kit_group(uint32_t src)   /* the group a kit is shown in */
{
    return src < DRUM_SAMPLED ? "HEX KITS" : src < DRUM_USR ? "CLASSIC KITS" : "MY KITS";
}
/* the kit list's place of src, -1 none */
static int32_t kit_pos(uint32_t src)
{
    uint32_t i, n = kit_src_n();
    for (i = 0; i < n; i++)
        if (kit_src(i) == src)
            return (int32_t)i;
    return -1;
}
/* whole kit src into k (a built-in kit, MY HEX, a user kit); 0: there is none */
static int kit_make(uint32_t src, pkit_t *k)
{
    uint32_t l;
    if (src >= DSRC_USER && src < DSRC_USER + UKIT_N) {
        if (!ukit_get(src - DSRC_USER, k) || !pkit_fix(k))
            return 0;
        k->base = (uint8_t)src;
    } else {
        if (src >= DRUM_KITS || drum_kit_none(src))
            return 0;
        memset(k, 0, sizeof *k);
        for (l = 0; l < DRUM_LANES; l++)
            slot_from_kit(&k->s[l], src, l);
        k->magic = PK_MAGIC;
        k->base = (uint8_t)src;
        k->ver = 1;
    }
    for (l = 0; l < DRUM_LANES; l++)
        k->s[l].mach &= (uint8_t)~(SL_CHG | SL_EDT);
    return 1;
}

/* ---- the sounds for slot L: INIT HEX, INIT CLASSIC, L's kind from every kit, then every other kind from every kit */
#define SND_NINIT 2u
static uint32_t snd_n(void) { return SND_NINIT + DRUM_LANES * kit_src_n(); }
static uint32_t snd_kind(uint32_t L, uint32_t g)   /* the g-th kind of the list for slot L */
{
    return g == 0u ? L & 15u : g - 1u + (g - 1u >= (L & 15u));
}
static uint32_t snd_group_of(uint32_t L, uint32_t kind)
{
    return kind == (L & 15u) ? 0u : kind + 1u - (kind > (L & 15u));
}
/* entry e of slot L's list: *src (DSRC_INIT: *kind the machine) and *kind */
static void snd_at(uint32_t L, uint32_t e, uint32_t *src, uint32_t *kind)
{
    uint32_t s = kit_src_n();
    if (e < SND_NINIT) {
        *src = DSRC_INIT;
        *kind = e;
        return;
    }
    e -= SND_NINIT;
    *src = kit_src(e % s);
    *kind = snd_kind(L, e / s);
}
/* where slot L's sound stands in its list (its place, of the list's n), its group's name and its place in the group; -1:
 * not in it (edited, or a sound that has no entry) */
static int32_t snd_pos(uint32_t L)
{
    const dslot_t *d = &pkit.s[L & 15u];
    int32_t k;
    if (d->mach & SL_EDT)
        return -1;
    if (d->src == DSRC_INIT)
        return (int32_t)(d->mach & 1u);
    if ((k = kit_pos(d->src)) < 0)
        return -1;
    return (int32_t)(SND_NINIT + snd_group_of(L, sl_lane(d)) * kit_src_n() + (uint32_t)k);
}
static void snd_group(uint32_t L, uint32_t e, char *grp, uint32_t gn, uint32_t *ipos, uint32_t *isize)
{
    uint32_t s = kit_src_n();
    if (e < SND_NINIT) {
        str_cpy(grp, "INIT", gn);
        *ipos = e + 1u;
        *isize = SND_NINIT;
        return;
    }
    str_cpy(grp, KIND_NAME[snd_kind(L, (e - SND_NINIT) / s)], gn);
    *ipos = (e - SND_NINIT) % s + 1u;
    *isize = s;
}
/* entry e (for slot L) as a slot; 0: none */
static int snd_make(uint32_t L, uint32_t e, dslot_t *d)
{
    uint32_t src, kind;
    snd_at(L, e, &src, &kind);
    if (src == DSRC_INIT) {
        slot_init(d, kind, L);
        return 1;
    }
    if (src >= DSRC_USER) {
        if (!ukit_slot(src - DSRC_USER, kind, d))
            return 0;
        slot_fix(d);
        d->src = (uint8_t)src;
        d->orig = (uint8_t)((d->orig & 0xF0u) | kind);
    } else {
        slot_from_kit(d, src, kind);
    }
    d->mach &= (uint8_t)~SL_EDT;
    return 1;
}
static void snd_name_at(uint32_t L, uint32_t e, char *b, uint32_t n)
{
    dslot_t d;
    uint32_t src, kind;
    snd_at(L, e, &src, &kind);
    memset(&d, 0, sizeof d);
    d.src = (uint8_t)src;
    d.mach = (uint8_t)(src == DSRC_INIT ? kind : 0u);
    d.orig = (uint8_t)(src == DSRC_INIT ? L & 15u : kind);
    slot_name(&d, b, n);
}

/* ---- (wave 12) LOCKS WHEN A SLOT CHANGES MACHINE. A lock on a send, the LFO or the pitch (NOTE) is every machine's: it
 * stays. A lock on one machine's own parameter goes over to the other machine's knob of the same name, at the same share
 * of its range (a named value, a filter TYPE or a WAVE, only where the other has the same name); the rest are dropped,
 * and the screen says how many (ui_kit.c). The same machine again: every lock as it was */
static const uint8_t LK_PAIR[][2] = {                     /* {HEX, CLASSIC}: the knobs both machines have */
    {DSP_COARSE, DSP_S0 + DS_TUNE}, {DSP_CUT, DSP_S0 + DS_FREQ}, {DSP_RES, DSP_S0 + DS_RES},
    {DSP_DRIVE, DSP_S0 + DS_DRIVE}, {DSP_DEC, DSP_S0 + DS_DECAY}, {DSP_LEVEL, DSP_S0 + DS_LEVEL},
    {DSP_FTYPE, DSP_S0 + DS_FTYPE}, {DSP_WAVE, DSP_S0 + DS_WAVE}};
static const param_desc_t *dsp_desc(uint32_t model, const uint8_t *snd, uint32_t id);   /* (drum_par.c) */
static int lk_signed(uint32_t id)
{
    return id == DSP_FINE || id == DSP_NOTE || id == DSP_LDEP || id == DSP_LFAD || id == DSP_S0 + DS_FENV ||
           id == DSP_S0 + DS_LEVEL;
}
static int lk_mine(uint32_t id, uint32_t mach)            /* a lock id of machine mach's own (not shared) */
{
    return mach == DM_HEX ? id < DSP_COUNT : id >= DSP_S0 && id < DSP_S0 + DS_N;
}
static int lk_shared(uint32_t id) { return !(id < DSP_COUNT) && !(id >= DSP_S0 && id < DSP_S0 + DS_N); }
/* lock (id, byte) made for machine from (sound fs) carried onto machine to (sound ts): 1 with *nid, *nv */
static int lk_carry(uint32_t id, uint32_t v, uint32_t from, const uint8_t *fs, uint32_t to, const uint8_t *ts,
                    uint32_t *nid, uint32_t *nv)
{
    uint32_t i;
    const param_desc_t *a, *b;
    int32_t x;
    if (lk_shared(id) || from == to) {
        *nid = id, *nv = v;
        return 1;
    }
    for (i = 0; i < NELEM(LK_PAIR); i++)
        if (LK_PAIR[i][from == DM_SYN] == id)
            break;
    if (i == NELEM(LK_PAIR))
        return 0;
    *nid = LK_PAIR[i][to == DM_SYN];
    a = dsp_desc(from, fs, id);
    b = dsp_desc(to, ts, *nid);
    if (!a || !b)
        return 0;
    x = lk_signed(id) ? (int32_t)(int8_t)v : (int32_t)v;
    if (a->names || b->names) {                           /* a named value: the same name, or nothing */
        int32_t k;
        if (!a->names || !b->names || x < a->min || x > a->max)
            return 0;
        for (k = b->min; k <= b->max; k++)
            if (str_eq(a->names[x - a->min], b->names[k - b->min]))
                break;
        if (k > b->max)
            return 0;
        x = k;
    } else {                                              /* the same share of its range, rounded */
        int32_t ra = a->max - a->min, rb = b->max - b->min;
        x = b->min + ((x - a->min) * rb * 2 + (ra > 0 ? ra : 1)) / (2 * (ra > 0 ? ra : 1));
        x = clamp(x, b->min, b->max);
    }
    *nv = (uint32_t)(uint8_t)x;
    return 1;
}
/* slot L's locks made again from org[] (the pattern's locks before the change, the slot then of machine om, sound os):
 * this slot's own ones replaced; *kept, *dropped: of the ones the machine had of its own. IRQ off (the sequencer) */
static void lk_rebuild(uint32_t L, const dlk_t *org, uint32_t om, const uint8_t *os, uint32_t *kept, uint32_t *dropped)
{
    uint32_t k, f = 0, nm = sl_mach(L);
    *kept = *dropped = 0;
    for (k = 0; k < DT_NLK; k++)
        if (dpat.lk[k].step != DT_FREE && dpat.lk[k].lane == (L & 15u))
            dpat.lk[k].step = DT_FREE;
    for (k = 0; k < DT_NLK; k++) {
        uint32_t nid, nv;
        const dlk_t *o = &org[k];
        if (o->step == DT_FREE || o->lane != (L & 15u))
            continue;
        if (!lk_carry(o->param, o->val, om, os, nm, lane_snd(L), &nid, &nv)) {
            *dropped += lk_mine(o->param, om);
            continue;
        }
        *kept += om != nm && lk_mine(o->param, om);
        while (f < DT_NLK && dpat.lk[f].step != DT_FREE)
            f++;
        if (f == DT_NLK)
            break;
        dpat.lk[f].step = o->step, dpat.lk[f].lane = o->lane, dpat.lk[f].param = (uint8_t)nid, dpat.lk[f].val = (uint8_t)nv;
    }
}

/* slot L's voices cut (it takes another sound: what rings stops, as a Rytm track does; its sends fade). IRQ off */
static void lane_cut(uint32_t L)
{
    uint32_t k;
    for (k = 0; k < NDRUM; k++)
        if (drums.v[k].active && drums.lane[k] == (L & 15u)) {
            drums.v[k].active = 0;
            drum_cut(k);
        }
}
/* slot L takes sound d (it differs from the kit loaded from now on); its locks from org[] (machine om, sound os) */
static void slot_take(uint32_t L, const dslot_t *d, const dlk_t *org, uint32_t om, const uint8_t *os, uint32_t *kept,
                      uint32_t *dropped)
{
    fm1_irq_off();
    lane_cut(L);
    pkit.s[L & 15u] = *d;
    lk_rebuild(L, org, om, os, kept, dropped);
    pkit_gen++;
    fm1_irq_on();
}
