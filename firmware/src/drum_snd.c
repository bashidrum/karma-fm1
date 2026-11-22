/* SPDX-License-Identifier: GPL-3.0-only */
/* The drum sounds as the Rytm-style pages edit them (PHEBUS wave 3; Ben, 21 Nov 2026: "Drums: the Analog Rytm MKII's
 * easy drum programming. Pick a sound, 16 trig keys, hold a trig and turn to lock, sound pages in the Rytm's order (SRC,
 * FLTR, AMP, LFO)"; picker "Make every kit editable" (keep the 606 start; turning a knob on any kit makes your own
 * editable copy)). Included by drums.c.
 *
 * The sound model. Every one of the 16 sounds of whatever kit is loaded has the same four pages, SRC FLTR AMP LFO, of
 * four knobs a half (SRC 1/3 .. 3/3, FLTR, AMP 1/2 2/2, LFO 1/2 2/2). A kit is one of two machines, and a page holds the
 * machine's own parameters in the Rytm's places:
 *   HEX   (the HEX kits, MY HEX): the LXR-02's voices (hex_drums.c hex_snd_t, its 22 bytes)
 *   SYN   (the 32 synthesised kits, 606 among them): drum_synth.c dsnd_t (its 22 bytes, 23 parameters)
 * plus, on every sound, its own sends (drums.c drum_send: AMP 2/2 DEL REV, as the Rytm's AMP page) and its own LFO (8
 * bytes, below: neither machine had one; it moves PITCH, CUTOFF, RESONANCE or LEVEL of the hit it belongs to).
 *
 * Copy-on-write: the built-in kits (and MY HEX) stay as they are; the first knob turned on a sound makes the project's own
 * copy of the whole kit (pkit, 488 bytes, kept in the project's ext: project.c) and edits that. The copy plays whenever
 * the drum track is on the kit it came from (its name then has a *). Each of the 16 sounds is its own in the copy (the
 * built-in kits give PEDAL the closed hat's sound: the copy gives it its own). One kit's copy at a time: editing another
 * kit replaces it, after the UI asked to turn again (ui_drum.c).
 *
 * A trig's lock holds one of these by its DSP_* id (core.h): a HEX byte (0 .. 21), a send (22, 23), an LFO byte (24 ..
 * 31), a SYN parameter (32 ..), the pitch (DSP_NOTE). A lock for the other machine than the kit's does nothing. */
enum { DM_HEX, DM_SYN };
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

#define PK_MAGIC 0x31544B50u                 /* "PKT1" */
typedef struct {                             /* the project's own kit (488 bytes, in the project's ext) */
    uint32_t magic;                          /* PK_MAGIC: there is one */
    uint8_t src;                             /* the kit it is a copy of (P_E0's number) */
    uint8_t model;                           /* DM_* */
    uint8_t crush;                           /* SYN: the kit's crush (dkit_t.crush) */
    uint8_t rsv;
    uint8_t snd[DRUM_LANES][22];             /* each lane's sound: a hex_snd_t or a dsnd_t */
    dlfo_t lfo[DRUM_LANES];
} pkit_t;
_Static_assert(sizeof(pkit_t) == 488u && sizeof(dsnd_t) == 22u && sizeof(hex_snd_t) == 22u && sizeof(dlfo_t) == 8u,
               "the project's kit: 488 bytes, each sound 22");
static pkit_t pkit;
static volatile uint8_t pkit_gen;            /* (bumped at each edit: the screen, the unsaved dot) */

static uint32_t kit_model(uint32_t k) { return k >= DRUM_SAMPLED && k < DRUM_USR ? DM_SYN : DM_HEX; }
static int pkit_on(void) { return pkit.magic == PK_MAGIC && pkit.src == drum_kit(); }
static int pkit_other(void) { return pkit.magic == PK_MAGIC && pkit.src != drum_kit(); }   /* a copy of another kit */
static uint32_t drum_model(void) { return pkit_on() ? pkit.model : kit_model(drum_kit()); }
/* lane l's sound in built-in kit k (as the kit plays the lane's note) */
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
static const uint8_t *lane_snd(uint32_t l) { return pkit_on() ? pkit.snd[l & 15u] : kit_snd(drum_kit(), l); }
static const dlfo_t *lane_lfo(uint32_t l) { return pkit_on() ? &pkit.lfo[l & 15u] : &DLFO_DEF; }

/* a synthesised sound's values back in their ranges (from flash, a backup: the voices only ever see what they expect) */
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
static void pkit_fix(pkit_t *p)              /* a kit from outside: checked, else none */
{
    uint32_t l;
    if (p->magic != PK_MAGIC || p->model > DM_SYN || p->src >= DRUM_KITS || drum_kit_none(p->src) ||
        kit_model(p->src) != p->model) {
        memset(p, 0, sizeof *p);
        return;
    }
    for (l = 0; l < DRUM_LANES; l++) {
        if (p->model == DM_HEX)
            dsu_fix_sound((hex_snd_t *)p->snd[l]);
        else
            ds_fix_sound((dsnd_t *)p->snd[l]);
        dlfo_fix(&p->lfo[l]);
    }
    p->rsv = 0;
}
/* the project's copy of the kit the drum track is on, made now (a copy of another kit before: replaced). IRQ off: the
 * sequencer and the voices read it */
static void pkit_make(void)
{
    uint32_t k = drum_kit(), l;
    fm1_irq_off();
    pkit.magic = 0;                          /* (until whole: the kit plays as built in) */
    pkit.src = (uint8_t)k;
    pkit.model = (uint8_t)kit_model(k);
    pkit.crush = (uint8_t)(pkit.model == DM_SYN ? DS_KITS[(k - DRUM_SAMPLED) % DS_NKITS].crush : 0u);
    pkit.rsv = 0;
    for (l = 0; l < DRUM_LANES; l++) {
        memcpy(pkit.snd[l], kit_snd(k, l), 22);
        pkit.lfo[l] = DLFO_DEF;
    }
    pkit.magic = PK_MAGIC;
    pkit_gen++;
    fm1_irq_on();
}
static uint32_t pkit_hash(void)              /* (0: none; the autosave's and the unsaved dot's: FNV-1a) */
{
    const uint8_t *b = (const uint8_t *)&pkit;
    uint32_t h = 2166136261u, i;
    if (pkit.magic != PK_MAGIC)
        return 0;
    for (i = 0; i < sizeof pkit; i++)
        h = (h ^ b[i]) * 16777619u;
    return h | 1u;
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
