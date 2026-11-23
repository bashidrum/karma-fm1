/* SPDX-License-Identifier: GPL-3.0-only */
/* The drum sounds as the Rytm-style pages edit them (PHEBUS wave 3; Ben, 21 Nov 2026: "Drums: the Analog Rytm MKII's
 * easy drum programming. Pick a sound, 16 trig keys, hold a trig and turn to lock, sound pages in the Rytm's order (SRC,
 * FLTR, AMP, LFO)"). Included by drums.c.
 *
 * THE KIT IS 16 SLOTS IN THE PROJECT (PHEBUS wave 12; Ben, 23 Nov 2026, 11:02: "couldn't see how to load in like an 808
 * snare with a 606 kit preset. Or just choose the snare engine and then do sound design"; 11:10: "Kits on HOME, sounds on
 * the drum screen", "Same kind first, then all", "Yes, an INIT per engine"). Each of the 16 slots holds one whole sound of
 * either machine, and every slot is the project's own: editing a slot edits the project's sound (no copy, no star, no
 * REPLACE question). The machines:
 *   HEX      the LXR-02's voices (hex_drums.c hex_snd_t, its 22 bytes), the HEX kits and MY HEX
 *   CLASSIC  the synthesised model (drum_synth.c dsnd_t, its 22 bytes, 23 parameters): the 606, 808, 909 and the rest
 * plus, on every slot, its own sends (AMP 2/2 DEL REV), its own LFO (8 bytes: neither machine had one; it moves PITCH,
 * CUTOFF, RESONANCE or LEVEL of the hit it belongs to), where it came from (its name: "808 SNARE"), its level trim and
 * crush (its kit's, so it sounds as it did in its kit) and its pitch offset (the 606's PEDAL is its closed hat a semitone
 * down). The machine is the slot's: a HEX kick beside an 808 snare plays each hit with its own slot's machine.
 *
 * A trig's lock holds one parameter by its DSP_* id (core.h): a HEX byte (0 .. 21), a send (22, 23), an LFO byte (24 ..
 * 31), a CLASSIC parameter (32 ..), the pitch (DSP_NOTE). A lock made for the other machine than its slot's does nothing;
 * when a slot takes a sound of the other machine its locks are carried over where the other machine has the same knob
 * (lk_carry, below) and the rest are dropped, counted on screen (ui_kit.c). */
enum { DM_HEX, DM_SYN };
static const char *const DM_NAME[2] = {"HEX", "CLASSIC"};
enum {                                       /* a synthesised sound's parameters (DSP_S0 + these), in dsnd_t's order */
    DS_WAVE, DS_NOISE, DS_TUNE, DS_FINE, DS_SWEEP, DS_SWTIME, DS_HOLD, DS_DECAY, DS_TONE, DS_PART2, DS_P2LVL, DS_CLICK,
    DS_NLVL, DS_NHOLD, DS_NDEC, DS_FTYPE, DS_RES, DS_FREQ, DS_FENV, DS_HPF, DS_CHIP, DS_DRIVE, DS_LEVEL, DS_N
};
_Static_assert(DSP_S0 + DS_N == DSP_NOTE, "the synthesised sounds' lock ids end where the pitch's starts");
typedef struct {                             /* a sound's LFO (8 bytes): the Rytm's SPD DEP DST WAV, then SYNC FADE PHASE MODE */
    uint8_t spd;                             /* RATE, 0..127 (LFO_HZ_X100), when not synced */
    uint8_t dst;                             /* DL_*: what it moves */
    uint8_t wav;                             /* SIN TRI SAW SQR S&H (params.c N_LWAVE) */
    uint8_t syn;                             /* 0 free (spd), else a note value (params.c N_LSYNC: 4BAR .. 1/32) */
    uint8_t sph;                             /* where a hit starts it, 0..127 of a cycle */
    uint8_t mode;                            /* DLM_*: TRIG restarts it at each hit, FREE runs on, ONE plays one cycle */
    int8_t dep;                              /* -64..63: 0 off */
    int8_t fad;                              /* -64..63: < 0 fades out after the hit, > 0 fades in, 0 none */
} dlfo_t;
enum { DL_PIT, DL_CUT, DL_RES, DL_LVL, DL_N };
enum { DLM_TRIG, DLM_FREE, DLM_ONE, DLM_N };
static const dlfo_t DLFO_DEF = {64, DL_PIT, 0, 0, 0, DLM_TRIG, 0, 0};
static const uint16_t DL_PULSES[11] = {0, 384, 192, 96, 48, 24, 12, 8, 6, 4, 3};   /* (voice.c LSYNC_PULSES: 24ths of a beat) */

/* ---- waves 3 to 11: the project's copy of ONE built-in kit ("PKT1", 488 bytes in the project's ext). Read only: a project
 * that has one is converted to the 16 slots when it loads (project.c proj_kit_take) */
#define PK1_MAGIC 0x31544B50u                /* "PKT1" */
typedef struct {
    uint32_t magic;
    uint8_t src, model, crush, rsv;          /* the kit it copied (P_E0's number), DM_*, a CLASSIC kit's crush */
    uint8_t snd[DRUM_LANES][22];
    dlfo_t lfo[DRUM_LANES];
} pkit1_t;
_Static_assert(sizeof(pkit1_t) == 488u && sizeof(dsnd_t) == 22u && sizeof(hex_snd_t) == 22u && sizeof(dlfo_t) == 8u,
               "waves 3 to 11's copy of a kit: 488 bytes, each sound 22");

/* ---- (wave 12) a slot: one whole sound (36 bytes) */
#define SL_TRIM(m) (((m) >> 1) & 7u)         /* mach bits 1..3: its kit's level trim (drums.c DRUM_KIT_TRIM; 7: none) */
#define SL_CHG 0x40u                         /* mach bit 6: differs from the kit loaded (taken from elsewhere, or edited) */
#define SL_EDT 0x80u                         /* mach bit 7: a knob turned on it since it was taken */
#define DSRC_USER 0x40u                      /* src: a user kit (MY KIT 1 ..), + its number */
#define DSRC_INIT 0x7Fu                      /* src: the INIT sound of its machine */
#define UKIT_N 6u                            /* user kits (SAVE > KITS): MY KIT 1 .. 6 */
typedef struct {
    uint8_t snd[22];                         /* a hex_snd_t (HEX) or a dsnd_t (CLASSIC) */
    dlfo_t lfo;
    uint8_t mach;                            /* bit 0: DM_*; SL_TRIM; SL_CHG; SL_EDT */
    uint8_t src;                             /* where it came from: a kit's number (P_E0's), DSRC_USER + n, DSRC_INIT */
    uint8_t orig;                            /* bits 0..3: the slot it had there; 4..7: its pitch offset, semitones + 8 */
    uint8_t crush;                           /* CLASSIC: its kit's crush (dkit_t.crush) */
    uint8_t send;                            /* its sends: reverb 0..15 | delay 0..7 << 4 */
    uint8_t rsv;
} dslot_t;
#define PK_MAGIC 0x32544B50u                 /* "PKT2" (wave 12) */
#define PK_NONE 0xFFu                        /* base: no whole kit to name (never: NEW loads the 606) */
typedef struct {                             /* the project's kit (592 bytes, in the project's ext: project.c) */
    uint32_t magic;                          /* PK_MAGIC */
    uint8_t base;                            /* the whole kit last loaded: a kit's number (P_E0's), or DSRC_USER + n */
    uint8_t ver, rsv[2];
    char name[8];                            /* a user kit's name (base DSRC_USER + n), 0-padded */
    dslot_t s[DRUM_LANES];
} pkit_t;
_Static_assert(sizeof(dslot_t) == 36u && sizeof(pkit_t) == 592u, "the kit: 16 slots of 36 bytes, 592 with its head");
static pkit_t pkit;
static volatile uint8_t pkit_gen;            /* (bumped at each edit: the screen, the unsaved dot) */

static uint32_t kit_model(uint32_t k) { return k >= DRUM_SAMPLED && k < DRUM_USR ? DM_SYN : DM_HEX; }
static uint32_t sl_mach(uint32_t l) { return pkit.s[l & 15u].mach & 1u; }
static int32_t sl_semi(const dslot_t *d) { return (int32_t)(d->orig >> 4) - 8; }
static uint32_t sl_lane(const dslot_t *d) { return d->orig & 15u; }
static uint32_t drum_model(void) { return sl_mach(drum_lane); }   /* the selected slot's machine (the pages show its own) */
static const uint8_t *lane_snd(uint32_t l) { return pkit.s[l & 15u].snd; }
static const dlfo_t *lane_lfo(uint32_t l) { return &pkit.s[l & 15u].lfo; }
static uint32_t sl_send(uint32_t l, uint32_t which)   /* 0: its reverb send (0..15), 1: its delay send (0..7) */
{
    return which ? (pkit.s[l & 15u].send >> 4) & 7u : pkit.s[l & 15u].send & 15u;
}
static void sl_send_set(uint32_t l, uint32_t which, uint32_t v)
{
    dslot_t *d = &pkit.s[l & 15u];
    d->send = (uint8_t)(which ? (d->send & 15u) | (v & 7u) << 4 : (d->send & 0x70u) | (v & 15u));
}
/* the level trim of kit k's sounds (drums.c DRUM_KIT_TRIM: HEX BOOM .. ROOM, 808, 909; MY HEX: the HEX kit it started from) */
static uint32_t kit_trim(uint32_t k)
{
    if (k >= DRUM_SYN && k < DRUM_KITS)
        return dsu.k[(k - DRUM_SYN) % DSU_N].src % DSU_NSRC;
    return k < 7u ? k : 7u;
}
/* lane l's sound in built-in kit k (as the kit plays the lane's note), or a MY HEX kit's */
static const uint8_t *kit_snd(uint32_t k, uint32_t l)
{
    int32_t semi;
    uint32_t s = ds_lane(LANE_NOTE[l & 15u], &semi) % DS_LANES;
    if (k >= DRUM_SYN)
        return (const uint8_t *)&dsu_kit(k - DRUM_SYN)->s[s];
    if (k < DRUM_SAMPLED)
        return (const uint8_t *)&HEX_KITS[k].s[s];
    return (const uint8_t *)&DS_KITS[(k - DRUM_SAMPLED) % DS_NKITS].s[s];
}
/* slot d made lane l's sound of kit k (a built-in kit or MY HEX): exactly as that kit plays it */
static void slot_from_kit(dslot_t *d, uint32_t k, uint32_t l)
{
    int32_t semi;
    (void)ds_lane(LANE_NOTE[l & 15u], &semi);
    memset(d, 0, sizeof *d);
    memcpy(d->snd, kit_snd(k, l), 22);
    d->lfo = DLFO_DEF;
    d->mach = (uint8_t)(kit_model(k) | kit_trim(k) << 1);
    d->src = (uint8_t)k;
    d->orig = (uint8_t)((l & 15u) | (uint32_t)(semi + 8) << 4);
    d->crush = (uint8_t)(kit_model(k) == DM_SYN ? DS_KITS[(k - DRUM_SAMPLED) % DS_NKITS].crush : 0u);
}

/* ---- (wave 12, Ben: "Yes, an INIT per engine") the INIT sound of each machine: plain, for the kind of slot it goes into
 * (the slots' three families: TONE a sine thump, the kicks, toms, conga, cowbell, at the slot's pitch; HIT a tone and a
 * noise burst, the snares, clap, rim; NOISE a plain noise burst, high-passed, the hats, cymbals, shaker). No transient, no
 * FM, no drive, no filter sweep: the starting point for sound design */
static uint32_t lane_family(uint32_t l)      /* 0 TONE, 1 HIT, 2 NOISE */
{
    static const uint8_t F[DRUM_LANES] = {0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 2, 2, 2, 0, 0};
    return F[l & 15u];
}
static void slot_init(dslot_t *d, uint32_t mach, uint32_t l)
{
    /* per slot: the TONE family's pitch (MIDI note; HEX: its COARSE), the NOISE family's decay */
    static const uint8_t PIT[DRUM_LANES] = {36, 33, 52, 52, 60, 60, 60, 64, 55, 45, 52, 60, 60, 60, 57, 69};
    static const uint8_t NDEC[DRUM_LANES] = {0, 0, 0, 0, 34, 84, 30, 0, 0, 0, 0, 100, 96, 40, 0, 0};
    uint32_t f = lane_family(l);
    memset(d, 0, sizeof *d);
    d->lfo = DLFO_DEF;
    d->src = DSRC_INIT;
    d->orig = (uint8_t)((l & 15u) | 8u << 4);
    if (mach == DM_HEX) {
        hex_snd_t *h = (hex_snd_t *)d->snd;
        d->mach = (uint8_t)(DM_HEX | 0u << 1);  /* (HEX BOOM's trim: the HEX voices' level as the HEX kits have it) */
        h->click = HC_SNAP;
        h->clvl = 0;                         /* (no transient) */
        h->ctone = 25;
        h->ftype = HF_OFF;
        h->cut = 127;
        h->drive = 40;                       /* (unity into the filter) */
        h->slope = 40;
        if (f == 0) {                        /* a sine with a short pitch drop: a thump */
            h->model = HM_DRUM, h->wave = HW_SINE, h->coarse = (uint8_t)(PIT[l & 15u] - 5u), h->fine = -30;
            h->dec = l == 15u ? 14 : 34, h->level = 120, h->pamt = 30, h->pdec = 30, h->fmratio = 43;
        } else if (f == 1) {                 /* a sine and a noise: a plain snare */
            h->model = HM_SNARE, h->wave = HW_SINE, h->coarse = 52, h->fine = -20;
            h->dec = 18, h->slope = 30, h->level = 110, h->pamt = 20, h->pdec = 22, h->fmratio = 120, h->fmamt = 80;
        } else {                             /* a noise burst, high-passed */
            h->model = HM_HAT, h->wave = HW_NOISE, h->coarse = 120;
            h->dec = (uint8_t)(NDEC[l & 15u] / 4u + 4u), h->level = 100, h->fmratio = 60, h->ftype = HF_HP, h->cut = 110;
        }
    } else {
        dsnd_t *s = (dsnd_t *)d->snd;
        d->mach = (uint8_t)(DM_SYN | 7u << 1);
        s->btime = 26, s->decay = 57, s->ndec = 44, s->fcut = 127, s->level = 136, s->pitch = 60;
        if (f == 0) {                        /* a sine with a short pitch drop: a thump */
            s->wave = DW_SINE, s->pitch = PIT[l & 15u], s->bend = 12, s->btime = 31, s->hold = 3, s->decay = 70;
            s->tlev = 124;
            if (l == 15u)
                s->bend = 0, s->decay = 40;
        } else if (f == 1) {                 /* a sine and a white noise: a plain snare */
            s->wave = DW_SINE, s->src = DN_WHITE, s->pitch = 52, s->bend = 3, s->btime = 13, s->decay = 50, s->tlev = 80;
            s->nlev = 100, s->ndec = 60, s->level = 140;
        } else {                             /* a white noise burst, high-passed */
            s->src = DN_WHITE, s->nlev = 110, s->ndec = NDEC[l & 15u], s->flt = DF_HP, s->fcut = 100, s->level = 150;
        }
    }
}

/* ---- the kit */
static void pkit_load_kit(uint32_t k)       /* the 16 slots made built-in (or MY HEX) kit k (IRQ off: the voices read it) */
{
    uint32_t l;
    fm1_irq_off();
    memset(&pkit, 0, sizeof pkit);
    for (l = 0; l < DRUM_LANES; l++)
        slot_from_kit(&pkit.s[l], k, l);
    pkit.magic = PK_MAGIC;
    pkit.base = (uint8_t)k;
    pkit.ver = 1;
    pkit_gen++;
    fm1_irq_on();
}
static int kit_changed(void)                 /* a slot differs from the kit loaded: a whole kit's load would lose it */
{
    uint32_t l;
    for (l = 0; l < DRUM_LANES; l++)
        if (pkit.s[l].mach & SL_CHG)
            return 1;
    return 0;
}
/* a sound's values back in their ranges (from flash, a backup: the voices only ever see what they expect) */
static void ds_fix_sound(dsnd_t *d)
{
    if (d->wave > DW_BELL) d->wave = DW_SINE;
    if ((d->src & 15u) > DN_CHIP || (d->src & ~(15u | DN_CLAP))) d->src = DN_WHITE;
    if ((d->flt & 7u) == DF_ALL) d->flt &= (uint8_t)~7u;
    d->pitch &= 127u; d->fine &= 15u; d->btime &= 127u; d->decay &= 127u; d->tlev &= 127u; d->t2lev &= 127u;
    d->click &= 127u; d->nlev &= 127u; d->ndec &= 127u; d->fcut &= 127u; d->hpf &= 127u; d->chip &= 127u; d->drive &= 127u;
    if (d->bend > 96u) d->bend = 96;
    d->hold &= 127u; d->t2 &= 127u; d->nhold &= 127u;
    d->fenv = (int8_t)clamp(d->fenv, -64, 63);
}
static void dlfo_fix(dlfo_t *f)
{
    f->spd &= 127u; f->sph &= 127u;
    if (f->dst >= DL_N) f->dst = DL_PIT;
    if (f->wav > 4u) f->wav = 0;
    if (f->syn > 10u) f->syn = 0;
    if (f->mode >= DLM_N) f->mode = DLM_TRIG;
    f->dep = (int8_t)clamp(f->dep, -64, 63);
    f->fad = (int8_t)clamp(f->fad, -64, 63);
}
static void dsu_fix_sound(hex_snd_t *d);
static void slot_fix(dslot_t *d)
{
    if (d->mach & 1u)
        ds_fix_sound((dsnd_t *)d->snd);
    else
        dsu_fix_sound((hex_snd_t *)d->snd);
    dlfo_fix(&d->lfo);
    d->send &= 0x7Fu;
    if (d->src != DSRC_INIT && (d->src & 0x80u))
        d->src = DSRC_INIT;
    if (((d->orig >> 4) & 15u) < 4u)        /* (its pitch offset: -4 .. +7) */
        d->orig = (uint8_t)((d->orig & 15u) | 8u << 4);
    d->rsv = 0;
}
static int pkit_fix(pkit_t *p)               /* a kit from outside: checked (0: not one) */
{
    uint32_t l;
    if (p->magic != PK_MAGIC)
        return 0;
    for (l = 0; l < DRUM_LANES; l++)
        slot_fix(&p->s[l]);
    for (l = 0; l < 8u; l++)
        if (p->name[l] && (p->name[l] < 32 || p->name[l] > 126))
            p->name[l] = ' ';
    p->ver = 1;
    p->rsv[0] = p->rsv[1] = 0;
    return 1;
}
static uint32_t pkit_hash(void)              /* (the autosave's and the unsaved dot's: FNV-1a) */
{
    const uint8_t *b = (const uint8_t *)&pkit;
    uint32_t h = 2166136261u, i;
    for (i = 0; i < sizeof pkit; i++)
        h = (h ^ b[i]) * 16777619u;
    return h | 1u;
}
/* waves 3 to 11's project (its kit number k0, each sound's sends, and its copy of a kit if it had one) -> the 16 slots, the
 * same sound exactly: the kit it played (its copy where the copy was of that kit). *other: 1 when the copy was of another
 * kit (it did not play; ukit.c keeps it as a user kit when there is room) */
static void pkit_from_old(uint32_t k0, const uint8_t *dsend, const pkit1_t *c, int *other)
{
    uint32_t l;
    *other = 0;
    if (k0 >= DRUM_KITS || drum_kit_none(k0))
        k0 = DRUM_DEFAULT_KIT;
    pkit_load_kit(k0);
    if (c && c->magic == PK1_MAGIC && c->src < DRUM_KITS && !drum_kit_none(c->src) && kit_model(c->src) == c->model) {
        if (c->src != k0) {
            *other = 1;
        } else {
            for (l = 0; l < DRUM_LANES; l++) {
                dslot_t *d = &pkit.s[l];
                dlfo_t lf = c->lfo[l];
                memcpy(d->snd, c->snd[l], 22);
                dlfo_fix(&lf);
                d->lfo = lf;
                d->crush = c->model == DM_SYN ? c->crush : 0u;
                slot_fix(d);
                if (memcmp(d->snd, kit_snd(k0, l), 22) || memcmp(&d->lfo, &DLFO_DEF, sizeof d->lfo))
                    d->mach |= SL_CHG | SL_EDT;
            }
        }
    }
    for (l = 0; dsend && l < DRUM_LANES; l++) {
        pkit.s[l].send = (uint8_t)(dsend[l] & 0x7Fu);
        if (pkit.s[l].send)
            pkit.s[l].mach |= SL_CHG;
    }
    pkit_gen++;
}

/* ---- names */
static void kit_name_of(uint32_t src, char *b, uint32_t n);   /* a whole kit's name (below, with the user kits) */
static const char *const KIND_NAME[DRUM_LANES] = {     /* the 16 slots' kinds (Ben's slot roles), as the lists say them */
    "KICK", "KICK 2", "SNARE", "CLAP", "HAT", "OPEN HAT", "PEDAL", "RIM",
    "SNARE 2", "LOW TOM", "HI TOM", "CRASH", "RIDE", "SHAKER", "CONGA", "COWBELL"};
/* a sound's name: its kit and its slot there ("808 SNARE", "HEX BOOM KICK 2", "MY KIT 1 RIDE"), or "INIT HEX" */
static void slot_name(const dslot_t *d, char *b, uint32_t n)
{
    if (d->src == DSRC_INIT) {
        str_cpy(b, "INIT ", n);
        str_cpy(b + str_len(b), DM_NAME[d->mach & 1u], n - str_len(b));
        return;
    }
    kit_name_of(d->src, b, n);
    if (str_len(b) + 2u < n) {
        str_cpy(b + str_len(b), " ", 2);
        str_cpy(b + str_len(b), KIND_NAME[sl_lane(d)], n - str_len(b));
    }
}

/* (drum_par.c, after params.c: what each id is, its range) */
static void dsp_put(uint32_t model, uint8_t *snd, dlfo_t *lf, uint32_t id, int32_t v);

/* ---- the LFO of a hit as it plays (drums.c: each voice's lf, ph, fade, val). Once a block of n samples */
static int32_t dlfo_wave(uint32_t wav, uint32_t ph, uint32_t rnd)
{
    switch (wav) {
    case 1: return osc_tri(ph);
    case 2: return (int32_t)(ph >> 16) - 32768;
    case 3: return ph < 0x80000000u ? 32767 : -32767;
    case 4: return (int32_t)(rnd >> 16) - 32768;
    default: return sine_i(ph);
    }
}
static uint32_t dlfo_inc(const dlfo_t *f, uint32_t n)   /* the phase a block of n samples moves */
{
    if (f->syn) {
        uint32_t beat = (uint32_t)FS * 60u / (uint32_t)clamp(song.g[G_BPM], 20, 300), cyc;
        cyc = beat * DL_PULSES[f->syn % 11u] / 24u;
        return (0xFFFFFFFFu / (cyc ? cyc : 1u)) * n;
    }
    return LFO_INC[f->spd & 127u] / CTL * n;
}
