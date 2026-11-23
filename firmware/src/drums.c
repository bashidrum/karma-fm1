/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* The drum part (track 4): its kit (HEX, synthesised or MY HEX; General MIDI percussion map), played by its step
 * pattern, the keys when track 4 is selected, and its own MIDI channel (GLO -> DRUMS, default 10). Its own voices
 * (outside the parts' voice budget). One-shots: note-offs are ignored; a closed or
 * pedal hi-hat chokes the open one. LEVEL / REV / DLY: GLO > DRUMS (G_DRLVL, G_DRREV, G_DRDLY);
 * PAN and MUTE: the drum track's P_PAN / P_MUTE. Rendered from the audio ISR. */
#define NDRUM 6
#include "drum_synth.c"       /* synthesised kits (DS_KITS) */
/* P_E0 was unused on the drum track: it holds the kit. 0..4: the HEX kits (hex_drums.c, the LXR-02's drum voices
 * ported in lxr_dsp.c; they took the places of the GM sample kit and its four treatments, gone with the samples),
 * 5..: the synthesised kits, then five numbers no kit has (KARMA's user sample kits USR1..USR4 and USR3+4, cut in
 * PHEBUS wave 1: the numbers stay so that a project's kit byte keeps its meaning; a project on one of them plays the
 * power-on kit), then MY HEX 1..4 */
#define DRUM_SAMPLED 5u
#define DRUM_USR (DRUM_SAMPLED + DS_NKITS)       /* (the five numbers no kit has, from here) */
#define DRUM_SYN (DRUM_USR + 5u)                 /* SLOOP 2.5: SYN1..SYN4, your synthesised kits (drum_synth.c dsu) */
#define DRUM_KITS (DRUM_SYN + DSU_N)
static const char *const DRUM_KIT_NAMES[] = {"HEX BOOM", "HEX ACID", "HEX METAL", "HEX GLITCH", "HEX ROOM", DS_KIT_NAME_LIST,
                                             "-", "-", "-", "-", "-",
                                             "MY HEX 1", "MY HEX 2", "MY HEX 3", "MY HEX 4"};
static const char *const DRUM_KIT_STYLES[] = {"ELECTRO", "TECHNO", "INDUSTRIAL", "GLITCH", "SOFT", DS_KIT_STYLE_LIST,
                                              "-", "-", "-", "-", "-",
                                              "YOUR HEX", "YOUR HEX", "YOUR HEX", "YOUR HEX"};
_Static_assert(sizeof(DRUM_KIT_NAMES) / sizeof(DRUM_KIT_NAMES[0]) == DRUM_KITS, "a name per kit");
#define DRUM_DEFAULT_KIT (DRUM_SAMPLED + 2u)   /* power-on: 606 (PHEBUS, 21 Nov 2026, Ben: "Start on 606"; gen_drumkits.py
                                                * KITS order: 808, 909, 606) */
static int drum_kit_none(uint32_t k) { return k >= DRUM_USR && k < DRUM_SYN; }
static uint32_t drum_kit(void)
{
    uint32_t k = (uint32_t)clamp(TDRUM->p[P_E0], 0, DRUM_KITS - 1);
    return drum_kit_none(k) ? DRUM_DEFAULT_KIT : k;
}
/* the kit s detents on from k (PRESETS, the KIT page's KNOB 2), past the numbers no kit has */
static int16_t drum_kit_step(int32_t k, int32_t s)
{
    k = clamp(k, 0, DRUM_KITS - 1);
    if (drum_kit_none((uint32_t)k))
        k = DRUM_DEFAULT_KIT;
    while (s) {
        int32_t d = s > 0 ? 1 : -1, n = k + d;
        while (n >= 0 && n < (int32_t)DRUM_KITS && drum_kit_none((uint32_t)n))
            n += d;
        if (n < 0 || n >= (int32_t)DRUM_KITS)
            break;
        k = n;
        s -= d;
    }
    return (int16_t)k;
}

/* each sound's own sends, on top of the drum voice's (FX > SENDS): reverb 0..15, delay 0..7 (wave 12: the slot's, drum_snd.c
 * dslot_t.send; AMP 2/2 DEL REV) */
#define DRUM_SEND_REV 15
#define DRUM_SEND_DLY 7

static struct {
    voice_t v[NDRUM];
    uint8_t lane[NDRUM];         /* the sound each voice plays (its sends) */
    uint32_t age;
    int32_t tail;                /* declick: the last output of cut voices, decaying */
    int32_t trev, tdly;          /* (3.1.2) .. and their reverb and delay sends: they stepped to 0, a click in
                                  * the echo and the reverb */
    int32_t gs[NDRUM][2];        /* each voice's reverb and delay send gains in the last block, Q15 */
    int32_t peak;                /* largest |output| since the UI last looked (TRACKS meter) */
    uint8_t trim[NDRUM];         /* (wave 12) the level trim of the slot's sound (its kit's: DRUM_KIT_TRIM's index, 7 none) */
    uint8_t synth[NDRUM];        /* the voice plays a synthesised kit (vx[].ds) */
    union {                      /* (PHEBUS wave 3: one or the other, never both: 720 bytes of RAM back) */
        dsv_t ds;
        hex_v_t hx;              /* .. or a HEX kit (synth[] 2) */
    } vx[NDRUM];
    volatile uint16_t hits;      /* bit per lane hit since the UI last looked (pads, key LEDs) */
    volatile uint8_t kick;       /* a kick was hit (fx.c DUCK) */
    int32_t a0, a1;              /* the drum track's mute / solo attenuation over this block (fx.c), Q15, 0 = heard */
} drums;

static int32_t ds_buf[CTL];

/* (3.1.3) the HEX voices on CPU1. Measured on an FM-1: they were ~0.3 ms of CPU0's 0.73 ms block, beside the effects,
 * while CPU1 (the synth tracks) idled. With two cores, CPU1 renders each sounding HEX voice's block first (fx.c
 * core1_parts: the notes of the block are already in, CPU0 leaves the drums alone until the join), and drums_mix takes
 * it from here instead of rendering it. One core, or a block without it: as before */
#define HX_PRE_N NDRUM                         /* (3.1.4's test build rendered 5 here: no RAM; 3.1.5 freed it) */
static int16_t hx_pre_buf[HX_PRE_N][CTL];       /* (main RAM: the pool keeps its 8 KiB spare) */
static uint8_t hx_pre_alive[NDRUM];
static uint8_t hx_pre_mask;                    /* bit k: voice k's block is in hx_pre_buf (set by CPU1, used once) */
static void drum_hex_par(uint32_t k, lx_par_t *p);   /* (below: its sound, its LFO) */
static void drums_hex_pre(uint32_t n)          /* CPU1 */
{
    uint32_t k, m = 0;
    if (n > CTL)
        n = CTL;
    for (k = 0; k < HX_PRE_N; k++)
        if (drums.v[k].active && drums.synth[k] == 2) {
            lx_par_t p;
            drum_hex_par(k, &p);                       /* (its sound, and its LFO: wave 3) */
            hx_pre_alive[k] = (uint8_t)lx_render(&drums.vx[k].hx.lx, &p, hx_pre_buf[k], n, drums.vx[k].hx.note);
            m |= 1u << k;
        }
    hx_pre_mask = (uint8_t)m;
}

/* voice k stops (stolen, choked, ended): what it was playing fades out, its sends too (drums_mix) */
static void drum_cut(uint32_t k)
{
    const int32_t s = drums.v[k].s[7];
    drums.tail += s;
    drums.trev += mulq15(s, drums.gs[k][0]);
    drums.tdly += mulq15(s, drums.gs[k][1]);
}

/* ------------------------------------------------------------ lanes --- */
/* The drum track's 16 sounds (slots), in slot order: kicks, snare and clap, the hi-hats, rim and a second snare, the toms,
 * the cymbals, the percussion. Each plays a GM note (the synthesised kits: drum_synth.c DS_MAP). Which white key plays which
 * is the grid's (DRUM_GRID below, since the drum grid wave; until then key n played slot n) */
static const uint8_t LANE_NOTE[DRUM_LANES] = {36, 35, 38, 39, 42, 46, 44, 37, 40, 43, 48, 49, 51, 70, 63, 56};
static const char *const LANE_NAME[DRUM_LANES] = {
    "KICK", "KICK 2", "SNARE", "CLAP", "HAT", "OPEN HAT", "PEDAL", "RIM",
    "SNARE 2", "LOW TOM", "HI TOM", "CRASH", "RIDE", "SHAKER", "CONGA", "COWBELL"};
static const char *const LANE_SHORT[DRUM_LANES] = {           /* 6 characters: tiles, dials (review 21) */
    "kick", "kick 2", "snare", "clap", "hat", "op hat", "pedal", "rim",
    "snare2", "lo tom", "hi tom", "crash", "ride", "shaker", "conga", "c-bell"};
/* a GM note (MIDI in, old projects) -> its lane: the nearest sound of the 16 (35..81; below: kick, above: shaker) */
static const uint8_t LANE_OF_GM[81 - 35 + 1] = {
    /* 35 */ 1, 0, 7, 2, 3, 8, 9, 4, 9, 6,
    /* 45 */ 9, 5, 10, 10, 11, 10, 12, 11, 12, 13,
    /* 55 */ 11, 15, 11, 13, 12, 14, 14, 14, 14, 14,
    /* 65 */ 14, 14, 15, 15, 13, 13, 15, 15, 13, 13,
    /* 75 */ 7, 7, 7, 14, 14, 15, 15};
static uint32_t lane_of_note(uint32_t note)
{
    return note < 35u ? 0u : note > 81u ? 13u : LANE_OF_GM[note - 35u];
}
/* (the drum grid, Rytm style: Ben, 23 Nov 2026, 21:07 and 21:14, as he typed it) THE ONE LAYOUT of the 16 sounds, used
 * everywhere they are drawn (the mixer, the drum screen's map, the hold SEQ tiles) and everywhere the white keys choose or
 * play one (the keys, RETRIG, the mixer's mutes and solos, SELECT's order): the grid's place, row by row from the top
 * left, -> the slot.
 *     HAT     OP HAT  RIDE    CRASH
 *     PEDAL   CONGA   C-BELL  SHAKER
 *     KICK 2  LO TOM  HI TOM  CLAP
 *     KICK    RIM     SNARE 2 SNARE
 * The white keys run down the columns, top to bottom (his picker): keys 1 to 4 the left column (HAT PEDAL KICK 2 KICK),
 * 5 to 8 the second, 9 to 12 the third, 13 to 16 the right. Presentation and key order only: the slots, the project, the
 * kits, the MIDI notes, the trigs and locks, PERFORM's mutes stay by slot */
static const uint8_t DRUM_GRID[DRUM_LANES] = {4, 5, 12, 11, 6, 14, 15, 13, 1, 9, 10, 3, 0, 7, 8, 2};
/* white key w (0..15) -> its place in the grid; the same sum turns a place back into its key (the transpose) */
static uint32_t dgrid_of_white(uint32_t w) { return (w & 3u) * 4u + ((w >> 2) & 3u); }
static uint32_t lane_of_white(uint32_t w) { return DRUM_GRID[dgrid_of_white(w & 15u)]; }
static uint32_t dgrid_of_lane(uint32_t l)               /* the grid's place of slot l */
{
    uint32_t p;
    for (p = 0; p < 15u && DRUM_GRID[p] != (l & 15u); p++)
        ;
    return p;
}
static uint32_t white_of_lane(uint32_t l) { return dgrid_of_white(dgrid_of_lane(l)); }   /* slot l's white key, 0..15 */
/* the sound d keys on from slot l, in key order (SELECT, CHROMATIC's -/+); wrap: round, else it stops at either end */
static uint32_t lane_step(uint32_t l, int32_t d, int wrap)
{
    int32_t w = (int32_t)white_of_lane(l) + d;
    w = wrap ? (w & 15) : w < 0 ? 0 : w > 15 ? 15 : w;
    return lane_of_white((uint32_t)w);
}
/* the lane of key k (0 = F3 .. 26 = G5): the white key's sound (the grid's order), a black key the white key left of it */
static uint32_t lane_of_key(uint32_t k)
{
    static const int8_t W[12] = {0, -1, 1, -1, 2, -1, 3, 4, -1, 5, -1, 6};   /* from F */
    int32_t i = W[k % 12u];
    if (i < 0)
        i = W[(k - 1u) % 12u];
    return lane_of_white((uint32_t)((int32_t)(k / 12u) * 7 + i) & 15u);
}
/* white key w (0 .. 15: a step, a pad, a place) -> its key index */
static uint32_t key_of_white(uint32_t w)
{
    static const uint8_t K[7] = {0, 2, 4, 6, 7, 9, 11};      /* F G A B C D E */
    w &= 15u;
    return (w / 7u) * 12u + K[w % 7u];
}
/* the key of slot l (its sound's white key, the grid's order), for the key LEDs */
static uint32_t key_of_lane(uint32_t l) { return key_of_white(white_of_lane(l)); }

/* (PHEBUS wave 3) the selected sound: the one the drum screen's pages, its step keys, chromatic mode and the sound pick
 * address (written by the UI only; seq.c reads it for chromatic mode) */
static uint8_t drum_lane;
#include "drum_snd.c"         /* (PHEBUS wave 3) the sounds as the pages edit them; (wave 12) the kit: 16 slots in the project */
#include "ukit.c"             /* (wave 12) the kits and sounds PRESETS browses, the user kits, the locks on a machine change */
/* each voice's LFO as it plays, and a locked send (0 none, else the value + 1: reverb, delay) */
static struct {
    dlfo_t lf[NDRUM];
    uint32_t ph[NDRUM], rnd[NDRUM];
    int32_t fade[NDRUM], val[NDRUM];             /* fade: Q15 the depth's share now; val: the LFO x the depth, Q15 */
    uint8_t one[NDRUM];                          /* ONE: the cycle is over (it holds) */
    uint8_t sov[NDRUM][2];
    uint32_t t;                                  /* samples the drums ran (FREE's phase) */
} dlr;

/* ---- the drum pattern's trigs (PHEBUS wave 2, core.h dpat_t): step i's trig of sound l, its extras, its sound's locks.
 * Every writer runs with the audio IRQ off (the sequencer reads them in the ISR) */
static uint32_t dt_vel(uint32_t i, uint32_t l) { return dpat.vel[i % NSTEP][l & 15u]; }
static int dt_has(uint32_t i, uint32_t l) { return dt_vel(i, l) != 0u; }
static uint32_t dt_mask(uint32_t i)                    /* the sounds with a trig on step i, a bit each */
{
    const uint8_t *v = dpat.vel[i % NSTEP];
    uint32_t l, m = 0;
    for (l = 0; l < DRUM_LANES; l++)
        m |= (v[l] != 0u) << l;
    return m;
}
static int dt_x(uint32_t i, uint32_t l)                /* the trig's extras: their entry, -1 = none */
{
    uint32_t k;
    for (k = 0; k < DT_NX; k++)
        if (dpat.x[k].step == i && dpat.x[k].lane == l)
            return (int)k;
    return -1;
}
static dtx_t *dt_x_make(uint32_t i, uint32_t l)        /* .. made (at the defaults) if it has none; 0: the pool is full */
{
    int k = dt_x(i, l);
    uint32_t f;
    if (k >= 0)
        return &dpat.x[k];
    for (f = 0; f < DT_NX && dpat.x[f].step != DT_FREE; f++)
        ;
    if (f == DT_NX)
        return 0;
    memset(&dpat.x[f], 0, sizeof dpat.x[f]);
    dpat.x[f].step = (uint8_t)i;
    dpat.x[f].lane = (uint8_t)l;
    dpat.x[f].prob = DT_PROB_DEF;
    dpat.x[f].rlen = DT_RLEN_DEF;
    return &dpat.x[f];
}
static int dt_x_plain(const dtx_t *x)                  /* all at the defaults: the entry is not needed */
{
    return x->cond == FC_NORM && !x->micro && !x->rrate && x->prob == DT_PROB_DEF && x->rlen == DT_RLEN_DEF && !x->rfade;
}
static void dt_x_tidy(void)                            /* entries with nothing in them (or for no trig) freed */
{
    uint32_t k;
    for (k = 0; k < DT_NX; k++)
        if (dpat.x[k].step != DT_FREE && (dpat.x[k].step >= NSTEP || dpat.x[k].lane >= DRUM_LANES ||
                                          !dt_has(dpat.x[k].step, dpat.x[k].lane) || dt_x_plain(&dpat.x[k])))
            dpat.x[k].step = DT_FREE;
}
static int dt_lk(uint32_t i, uint32_t l, uint32_t p)   /* a lock of sound l's parameter p on step i: its entry, -1 = none */
{
    uint32_t k;
    for (k = 0; k < DT_NLK; k++)
        if (dpat.lk[k].step == i && dpat.lk[k].lane == l && dpat.lk[k].param == p)
            return (int)k;
    return -1;
}
static int dt_lk_set(uint32_t i, uint32_t l, uint32_t p, uint32_t v)   /* set (or made); 0: the pool is full */
{
    int k = dt_lk(i, l, p);
    uint32_t f;
    if (k < 0) {
        for (f = 0; f < DT_NLK && dpat.lk[f].step != DT_FREE; f++)
            ;
        if (f == DT_NLK)
            return 0;
        k = (int)f;
    }
    dpat.lk[k].step = (uint8_t)i;
    dpat.lk[k].lane = (uint8_t)l;
    dpat.lk[k].param = (uint8_t)p;
    dpat.lk[k].val = (uint8_t)v;
    return 1;
}
static uint32_t dt_lk_del(uint32_t i, uint32_t l, uint32_t p)   /* p DSP_ALL: every lock of the trig; how many went */
{
    uint32_t k, n = 0;
    for (k = 0; k < DT_NLK; k++)
        if (dpat.lk[k].step == i && dpat.lk[k].lane == l && (p == DSP_ALL || dpat.lk[k].param == p)) {
            dpat.lk[k].step = DT_FREE;
            n++;
        }
    return n;
}
static uint32_t dt_lk_n(uint32_t i, uint32_t l)        /* the trig's locks */
{
    uint32_t k, n = 0;
    for (k = 0; k < DT_NLK; k++)
        n += dpat.lk[k].step == i && dpat.lk[k].lane == l;
    return n;
}
static uint32_t dt_free_x(void) { uint32_t k, n = 0; for (k = 0; k < DT_NX; k++) n += dpat.x[k].step == DT_FREE; return n; }
static uint32_t dt_free_lk(void) { uint32_t k, n = 0; for (k = 0; k < DT_NLK; k++) n += dpat.lk[k].step == DT_FREE; return n; }
static void dt_set(uint32_t i, uint32_t l, uint32_t vel)   /* a trig (or its velocity), 1..127 */
{
    dpat.vel[i % NSTEP][l & 15u] = (uint8_t)(vel < 1u ? 1u : vel > 127u ? 127u : vel);
}
static void dt_clr(uint32_t i, uint32_t l)             /* a trig gone, its extras and its sound's locks with it */
{
    int k = dt_x(i, l);
    dpat.vel[i % NSTEP][l & 15u] = 0;
    if (k >= 0)
        dpat.x[k].step = DT_FREE;
    dt_lk_del(i, l, DSP_ALL);
}
static void dpat_clear(dpat_t *d)                      /* no trig, no extra, no lock */
{
    uint32_t k;
    memset(d, 0, sizeof *d);
    for (k = 0; k < DT_NX; k++)
        d->x[k].step = DT_FREE;
    for (k = 0; k < DT_NLK; k++)
        d->lk[k].step = DT_FREE;
}
/* step src's trigs (with their extras and locks) onto step dst, whatever dst held; 0: a pool was full (some extras or
 * locks did not go) */
static int dt_copy_step(uint32_t src, uint32_t dst)
{
    uint32_t l, k, ok = 1;
    for (l = 0; l < DRUM_LANES; l++)
        dt_clr(dst, l);
    memcpy(dpat.vel[dst % NSTEP], dpat.vel[src % NSTEP], DRUM_LANES);
    for (k = 0; k < DT_NX; k++)
        if (dpat.x[k].step == src) {
            dtx_t c = dpat.x[k], *x = dt_x_make(dst, c.lane);
            if (x) {
                c.step = (uint8_t)dst;
                *x = c;
            } else {
                ok = 0;
            }
        }
    for (k = 0; k < DT_NLK; k++)
        if (dpat.lk[k].step == src && !dt_lk_set(dst, dpat.lk[k].lane, dpat.lk[k].param, dpat.lk[k].val))
            ok = 0;
    return (int)ok;
}

/* (the drum steps of the project formats before PHEBUS wave 2, project.c: a lane's bit, level, ratchet) */
static int dstep_has(const dstep_t *s, uint32_t l) { return (s->on[(l >> 3) & 1u] >> (l & 7u)) & 1u; }
static uint32_t dstep_lvl(const dstep_t *s, uint32_t l) { return (s->lvl[(l >> 2) & 3u] >> ((l & 3u) * 2u)) & 3u; }
static uint32_t dstep_rat(const dstep_t *s, uint32_t l) { return (s->rat[(l >> 2) & 3u] >> ((l & 3u) * 2u)) & 3u; }
static void dstep_set(dstep_t *s, uint32_t l, uint32_t lvl, uint32_t rat)   /* lane on, with its level and ratchet */
{
    uint32_t sh = (l & 3u) * 2u, b = (l >> 2) & 3u;
    s->on[(l >> 3) & 1u] |= (uint8_t)(1u << (l & 7u));
    s->lvl[b] = (uint8_t)((s->lvl[b] & ~(3u << sh)) | (lvl & 3u) << sh);
    s->rat[b] = (uint8_t)((s->rat[b] & ~(3u << sh)) | (rat & 3u) << sh);
}

/* the velocity of a level: as played (LV_NORM: vel), ghost, soft, hard */
static uint32_t lvl_vel(uint32_t lvl, uint32_t vel)
{
    static const uint8_t V[4] = {0, 42, 72, 127};
    return lvl & 3u ? V[lvl & 3u] : vel;
}
/* a MIDI velocity -> the level it records as (keys play 100: LV_NORM) */
static uint32_t vel_lvl(uint32_t vel)
{
    return vel < 56u ? LV_GHOST : vel < 88u ? LV_SOFT : vel < 116u ? LV_NORM : LV_HARD;
}

/* the metronome (GLO > GLOBAL > CLICK, and the REC count-in), 2.4.1: a voice of its own, mixed after the drum
 * track (fx.c mix_block), so it is heard with every kit (a user kit has no wood block: it was silent), whatever
 * the drum track's mute, a solo, its filter or slicer. A short sine tick: 1568 Hz on a bar's first beat, 1047 Hz
 * on the others, about 12 ms to fade by 1/e; its level follows GLO > DRUMS > LVL (100: as loud as the old
 * wood block). It costs a few hundred cycles a block while it sounds, nothing after */
static struct { uint32_t ph, inc; int32_t env, gain; } click;   /* env: Q30, 0 = silent */
static uint32_t click_n;                         /* clicks so far, and the last one's accent (the host tests count them) */
static uint8_t click_acc;
static void click_on(int accent)
{
    click.ph = 0;
    click.inc = accent ? (uint32_t)(((uint64_t)1568u << 32) / FS) : (uint32_t)(((uint64_t)1047u << 32) / FS);
    click.gain = accent ? 20000 : 12000;
    click.env = 1 << 30;
    click_acc = (uint8_t)(accent != 0);
    click_n++;
}
static void click_render(int32_t *ml, int32_t *mr, uint32_t n)
{
    uint32_t i;
    int32_t g;
    if (!click.env)
        return;
    g = (int32_t)((int64_t)click.gain * song.g[G_DRLVL] / 100);
    for (i = 0; i < n; i++) {
        int32_t v = (int32_t)(((int64_t)sine_i(click.ph) * (click.env >> 15) >> 15) * g >> 15);
        ml[i] += v;
        mr[i] += v;
        click.ph += click.inc;
        click.env -= click.env >> 9;                 /* about 12 ms to 1/e */
    }
    if (click.env < (1 << 18))
        click.env = 0;                                /* (-72 dB: done, about 100 ms) */
}

/* (3.0.4, from SLOOP 2.8.3 by isod89) the voice for a drum hit: a free voice; all busy, the sound's own voice if it still
 * sounds (it starts over, as on a drum machine: a 16th hi-hat no longer takes every voice with its own tails); else the
 * oldest, a kick last. Before, the oldest went whatever it was: on a busy beat the kick lost its voice and was cut */
/* (3.1.2) overload: the HEX drums cost the most on CPU0 (~0.1 ms a voice a block, measured on an FM-1: six voices, the
 * reverb, the delay and the CLOUD were 30 .. 68 late halves a second, a crackle). When audio.c sheds, the drums go first
 * while 4 or more sound: their oldest tail (not a kick) fades out and fewer play at once; ~2 s without an overload gives
 * one back. Never under DRUM_CAP_MIN */
#define DRUM_CAP_MIN 3u
#define DRUM_CALM_HALVES 344u                   /* (~2 s) */
static uint8_t drum_cap = NDRUM;
static uint16_t drum_calm;
static uint32_t drum_active(void)
{
    uint32_t i, n = 0;
    for (i = 0; i < NDRUM; i++)
        n += drums.v[i].active ? 1u : 0u;
    return n;
}
static voice_t *drum_oldest(void)               /* the oldest that is not a kick; all kicks: the oldest */
{
    voice_t *v = 0;
    uint32_t i, best = 0xFFFFFFFFu;
    for (i = 0; i < NDRUM; i++) {
        uint32_t kick = drums.v[i].note == 35u || drums.v[i].note == 36u;
        uint32_t key = (kick ? 0x80000000u : 0u) | (drums.v[i].age & 0x7FFFFFFFu);
        if (drums.v[i].active && key < best) {
            best = key;
            v = &drums.v[i];
        }
    }
    return v;
}
static int drum_shed(void)                      /* audio.c's overload: 1 = a drum voice let go */
{
    uint32_t n = drum_active();
    voice_t *v;
    drum_calm = 0;
    if (n <= DRUM_CAP_MIN || !(v = drum_oldest()))
        return 0;
    v->active = 0;
    drum_cut((uint32_t)(v - drums.v));          /* (its sound and sends fade: no click) */
    drum_cap = (uint8_t)(n - 1u);
    return 1;
}
static void drum_calm_half(int over)            /* each half: no overload for a while, one voice back */
{
    if (over) {
        drum_calm = 0;
    } else if (drum_cap < NDRUM && ++drum_calm >= DRUM_CALM_HALVES) {
        drum_cap++;
        drum_calm = 0;
    }
}
static voice_t *drum_pick(uint32_t note)
{
    voice_t *v = 0;
    uint32_t i, best = 0xFFFFFFFFu, room = drum_active() < drum_cap;
    for (i = 0; i < NDRUM && room; i++)         /* (a free one while under the cap) */
        if (!drums.v[i].active)
            return &drums.v[i];
    for (i = 0; i < NDRUM; i++)
        if (drums.v[i].active && drums.v[i].note == note)   /* (its own, still sounding: it starts over) */
            return &drums.v[i];
    (void)best;
    v = drum_oldest();                              /* the oldest that is not a kick; all kicks: the oldest */
    if (v)
        return v;
    for (i = 0; i < NDRUM; i++)                     /* (none sounds: a free one) */
        if (!drums.v[i].active)
            return &drums.v[i];
    return &drums.v[0];
}

/* (PHEBUS wave 2, 3) the locks of the hit about to start (seq.c dhit sets them just before trk_note_on, and clears them
 * after; chromatic mode a pitch): the hit plays from its voice's own copy of its sound with the locked values in it, so a
 * lock changes that hit only, for as long as it rings; its LFO, its sends and its pitch the same. A hit without locks
 * plays the kit's sound itself (a knob turned is heard on it at once, as before) */
static uint8_t drum_lk_n, drum_lk_p[DLK_MAX], drum_lk_v[DLK_MAX];
static hex_snd_t drum_own[NDRUM];                /* (a hex_snd_t, or a dsnd_t: both 22 bytes) */
static void dsu_fix_sound(hex_snd_t *d);
/* the slot a note plays (wave 12: the project's kit, whatever machine each slot is) */
static const dslot_t *drum_note_slot(uint32_t note) { return &pkit.s[(note < 128u ? lane_of_note(note) : 0u) & 15u]; }
static void drum_lfo_val(uint32_t k)             /* voice k's LFO x its depth now (dlr.val) */
{
    const dlfo_t *f = &dlr.lf[k];
    int32_t w = dlfo_wave(f->wav, dlr.ph[k], dlr.rnd[k]);
    dlr.val[k] = f->dep ? mulq15(w, mulq15(dlr.fade[k], f->dep * 516)) : 0;   /* (63 x 516: about 1.0) */
}
static void drum_lfo_start(uint32_t k, const dlfo_t *f)
{
    dlr.lf[k] = *f;
    dlr.one[k] = 0;
    dlr.rnd[k] = rng();
    dlr.ph[k] = (f->mode == DLM_FREE ? dlr.t * (dlfo_inc(f, 1u)) : 0u) + ((uint32_t)f->sph << 25);
    dlr.fade[k] = f->fad > 0 ? 0 : 32767;
    drum_lfo_val(k);
}
static void drum_lfo_tick(uint32_t k, uint32_t n)   /* a block of n samples later */
{
    const dlfo_t *f = &dlr.lf[k];
    uint32_t old = dlr.ph[k];
    if (!f->dep)
        return;
    if (!dlr.one[k]) {
        dlr.ph[k] += dlfo_inc(f, n);
        if (dlr.ph[k] < old) {
            dlr.rnd[k] = rng();
            if (f->mode == DLM_ONE) {
                dlr.one[k] = 1;
                dlr.ph[k] = 0xFFFFFFFFu;            /* (holds at the cycle's end) */
            }
        }
    }
    if (f->fad) {                                   /* FADE: about 2 ms .. 8 s to its end */
        int32_t step = (int32_t)(32767u * n / (uint32_t)(88u + 5500u * (uint32_t)((f->fad < 0 ? -f->fad : f->fad) *
                                                                                  (f->fad < 0 ? -f->fad : f->fad)) / 64u));
        dlr.fade[k] = clamp(dlr.fade[k] + (f->fad > 0 ? step : -step), 0, 32767);
    }
    drum_lfo_val(k);
}
/* voice k's LFO as its machine's modulation: pitch (1/16 semitone), cutoff, resonance (in the machine's steps), level (Q14) */
static void drum_lfo_mod(uint32_t k, int32_t *p16, int32_t *cut, int32_t *res, int32_t *lvl)
{
    int32_t v = dlr.val[k];
    *p16 = *cut = *res = 0;
    *lvl = 16384;
    if (!v)
        return;
    switch (dlr.lf[k].dst) {
    case DL_PIT: *p16 = v * 192 / 32767; break;      /* +-12 semitones */
    case DL_CUT: *cut = v * 64 / 32767; break;       /* +-64 of 127 */
    case DL_RES: *res = v * 64 / 32767; break;
    case DL_LVL: *lvl = clamp(16384 + (v >> 1), 0, 32767); break;
    default: break;                                 /* (wave 12: another knob, drum_hex_par / drum_synth_on) */
    }
}
static void drum_hex_par(uint32_t k, lx_par_t *p)   /* a HEX voice's parameters this block: its sound's, its LFO on them */
{
    int32_t p16, cut, res, lvl;
    uint32_t id = dl_id(DM_HEX, dlr.lf[k].dst);
    if (id < DSP_COUNT && dlr.val[k]) {             /* (wave 12) the LFO on another knob: a copy of the sound, moved */
        hex_snd_t t = *drums.vx[k].hx.d;
        const param_desc_t *d = dsp_desc(DM_HEX, (const uint8_t *)&t, id);
        int32_t v = (id == DSP_FINE ? (int32_t)(int8_t)((uint8_t *)&t)[id] : (int32_t)((uint8_t *)&t)[id]) +
                    dlr.val[k] * (d->max - d->min) / 65534;
        ((uint8_t *)&t)[id] = (uint8_t)clamp(v, d->min, d->max);
        hex_par(&t, p);
    } else {
        hex_par(drums.vx[k].hx.d, p);
    }
    drum_lfo_mod(k, &p16, &cut, &res, &lvl);
    if (p16) {                                      /* (COARSE in semitones, FINE +-half a semitone; DRUM / SNARE keep
                                                     * their FINE at 64: theirs is the slope) */
        int32_t semi = (p16 + (p16 < 0 ? -8 : 8)) / 16, fr = p16 - semi * 16;
        p->coarse = (uint8_t)clamp((int32_t)p->coarse + semi, 0, 127);
        p->fine = (uint8_t)clamp((int32_t)p->fine + fr * 8, 0, 126);
    }
    if (cut)
        p->fcut = (uint8_t)clamp((int32_t)p->fcut + cut, 0, 127);
    if (res)
        p->fres = (uint8_t)clamp((int32_t)p->fres + res, 0, 127);
}
/* a voice: note on the kit the drum track is on (its sound, the project's copy or the kit's), the oldest voice stolen */
static void __attribute__((noinline)) drum_synth_on(uint32_t note, uint32_t vel)
{
    voice_t *v = &drums.v[0];
    uint32_t i, k, lane = note < 128u ? lane_of_note(note) : 0u, own = 0;
    int32_t xs = 0, ls, ns;
    const dslot_t *sl = drum_note_slot(note);
    const uint8_t *snd = sl->snd;
    uint32_t model = sl->mach & 1u, crush = model == DM_SYN ? sl->crush : 0u;
    dlfo_t lf = sl->lfo;
    /* (wave 12) its pitch: the slot's sound's own offset (the 606's PEDAL: its closed hat a semitone down), and a GM note
     * other than the slot's own its distance from it (the engines add the note's offset themselves: taken back here) */
    (void)ds_lane(LANE_NOTE[lane & 15u], &ls);
    (void)ds_lane(note, &ns);
    (void)ns;
    xs = sl_semi(sl) - ls;
    if (note == 42u || note == 44u)                 /* hi-hat choke (the slots HAT and PEDAL cut OPEN HAT, whatever sound) */
        for (i = 0; i < NDRUM; i++)
            if (drums.v[i].active && drums.v[i].note == 46u) {
                drums.v[i].active = 0;
                drum_cut(i);
            }
    v = drum_pick(note);                            /* (3.0.4, SLOOP 2.8.3: a free one, its own, else the oldest but a kick) */
    if (v->active)
        drum_cut((uint32_t)(v - drums.v));
    i = (uint32_t)(v - drums.v);
    v->note = (uint8_t)note;
    v->vel = (uint8_t)vel;
    v->active = 1;
    v->s[7] = 0;
    v->age = ++drums.age;
    drums.synth[i] = model == DM_HEX ? 2 : 1;
    drums.lane[i] = (uint8_t)lane;
    drums.trim[i] = (uint8_t)SL_TRIM(sl->mach);
    dlr.sov[i][0] = dlr.sov[i][1] = 0;
    for (k = 0; k < drum_lk_n && k < DLK_MAX; k++) {   /* the locks: into the voice's own copy, its LFO, sends, pitch */
        uint32_t id = drum_lk_p[k];
        int32_t val = id == DSP_FINE || id == DSP_NOTE || id == DSP_LDEP || id == DSP_LFAD ||
                      id == DSP_S0 + DS_FENV || id == DSP_S0 + DS_LEVEL ? (int32_t)(int8_t)drum_lk_v[k] : (int32_t)drum_lk_v[k];
        if (id == DSP_NOTE) {
            xs += clamp(val, -48, 48);
        } else if (id == DSP_REV || id == DSP_DEL) {
            dlr.sov[i][id - DSP_REV] = (uint8_t)(clamp(val, 0, id == DSP_REV ? DRUM_SEND_REV : DRUM_SEND_DLY) + 1);
        } else if (id >= DSP_LSPD && id <= DSP_LMOD) {
            dsp_put(model, 0, &lf, id, val);
        } else if ((id < DSP_COUNT && model == DM_HEX && id != DSP_MODEL) ||
                   (id >= DSP_S0 && id < DSP_S0 + DS_N && model == DM_SYN)) {
            if (!own)
                memcpy(&drum_own[i], snd, 22);
            own = 1;
            dsp_put(model, (uint8_t *)&drum_own[i], &lf, id, val);
        }
    }
    drum_lfo_start(i, &lf);
    {   /* (wave 12) the LFO on another knob: a CLASSIC knob or a send takes its value as the hit starts (HEX: each block) */
        uint32_t id = dl_id(model, lf.dst);
        if (id != 0xFFu && dlr.val[i]) {
            const param_desc_t *d = dsp_desc(model, own ? (const uint8_t *)&drum_own[i] : snd, id);
            if (d && (id == DSP_REV || id == DSP_DEL)) {
                int32_t base = dlr.sov[i][id - DSP_REV] ? dlr.sov[i][id - DSP_REV] - 1 : (int32_t)sl_send(lane, id - DSP_REV);
                dlr.sov[i][id - DSP_REV] = (uint8_t)(clamp(base + dlr.val[i] * (d->max - d->min) / 65534, d->min, d->max) + 1);
            } else if (d && model == DM_SYN) {
                if (!own)
                    memcpy(&drum_own[i], snd, 22);
                own = 1;
                dsp_put(model, (uint8_t *)&drum_own[i], &lf,
                        id, dsp_get(model, (const uint8_t *)&drum_own[i], &lf, lane, id) + dlr.val[i] * (d->max - d->min) / 65534);
            }
        }
    }
    if (model == DM_HEX)
        hex_on_snd_x(&drums.vx[i].hx, own ? &drum_own[i] : (const hex_snd_t *)snd, note, vel, xs);
    else
        ds_on_snd(&drums.vx[i].ds, own ? (const dsnd_t *)&drum_own[i] : (const dsnd_t *)snd, crush, note, vel, xs);
}

static void drum_on_(uint32_t note, uint32_t vel);
static void drum_on(uint32_t note, uint32_t vel)   /* (3.1.3: timed in the measuring build) */
{
    PC_T0();
    drum_on_(note, vel);
    PC_MARK(PC_EV_DRUM);
}
static void drum_on_(uint32_t note, uint32_t vel)
{
    if (note != 76u && note != 77u)                 /* the pads and key LEDs (not the click's wood block) */
        drums.hits |= (uint16_t)(1u << lane_of_note(note));
    if ((note == 35u || note == 36u) && !((perf_dmute >> lane_of_note(note)) & 1u))
        drums.kick = 1;                             /* (DUCK; wave 13: not from a kick PERFORM mutes) */
    drum_synth_on(note, vel);
}

/* the reverb and delay sends of voice k, Q15: the kit's plus its sound's own; own_only: the sound's alone */
static inline void drum_voice_sends(uint32_t k, int own_only, int32_t *rs, int32_t *ds)
{
    uint32_t ln = drums.lane[k] % DRUM_LANES;
    int32_t r = (dlr.sov[k][0] ? dlr.sov[k][0] - 1 : (int32_t)sl_send(ln, 0)) * 127 / DRUM_SEND_REV;   /* (a locked send: */
    int32_t d = (dlr.sov[k][1] ? dlr.sov[k][1] - 1 : (int32_t)sl_send(ln, 1)) * 127 / DRUM_SEND_DLY;  /* the hit's own) */
    if (own_only) {
        *rs = r * 258;
        *ds = d * 258;
        return;
    }
    r += song.g[G_DRREV];
    d += song.g[G_DRDLY];
    *rs = send_thr((r > 127 ? 127 : r) * 258, thr_rev);   /* (2.6: the THROWs) */
    *ds = send_thr((d > 127 ? 127 : d) * 258, thr_dly);
}

/* adds the drums into the dry mix and the reverb and delay sends; mono != 0: into mono instead, before
 * the pan and the kit's sends (the SLICER, slicer.c slicer_drums, does those after it), each sound's own
 * sends from here (so they stay the sound's with the SLICER on) */
/* the synthesised kits (HEX, the SYN ones) as loud as the synth tracks (2026-10-20: they were 5 .. 13 dB under):
 * a gain, then a soft knee rounds each hit's peak instead of the master limiter pumping the mix */
#ifndef DRUM_BOOST_Q8
#define DRUM_BOOST_Q8 1024                      /* x 4 (+12 dB) */
#endif
#ifndef DRUM_KNEE
#define DRUM_KNEE 9000                          /* (where the soft knee starts) */
#endif
/* and each kit level with the others (Q8; measured on a house beat, tests: about -16.5 LUFS): HEX BOOM, ACID,
 * METAL, GLITCH, ROOM, then 808, 909 (the rest: 1). The HEX kits' (2026-10-26, the LXR's voices): their grooves as
 * loud as the HEX kits' before (tests/hex_demo.c), each sound's LEVEL set by tools/level_hexkits.py */
static const uint16_t DRUM_KIT_TRIM[7] = {160, 145, 96, 149, 183, 215, 256};
/* (PHEBUS wave 13) PERFORM's drum mutes (punch.c perf_dmute): each slot's fade, Q15 (0 heard, 32767 silent), 8 blocks end to
 * end as a track's mute (fx.c MUTE_STEP); a muted slot's voices go on ringing unheard, so it comes back exactly in place */
static int16_t dm_att[DRUM_LANES];
static inline void drums_mix(int32_t *ml, int32_t *mr, int32_t *rev, int32_t *dly, int32_t *mono, uint32_t n)
{
    uint32_t k, i;
    int32_t lvl = song.g[G_DRLVL] * 200, send, dsend, pk = drums.peak;
    int32_t kboost;
    int16_t la0[DRUM_LANES], la1[DRUM_LANES];
    uint32_t lmask = perf_dmute, lany = 0;
    for (k = 0; k < DRUM_LANES; k++) {          /* (wave 13) the slots' fades this block */
        int32_t a = dm_att[k], to = (lmask >> k) & 1u ? 32767 : 0;
        la0[k] = (int16_t)a;
        a = a < to ? (to - a > 4096 ? a + 4096 : to) : (a - to > 4096 ? a - 4096 : to);
        la1[k] = dm_att[k] = (int16_t)a;
        lany |= (uint32_t)(la0[k] | la1[k]);
    }
    int32_t pan = trk[TRK_DRUM].p[P_PAN], gl = 4096 - (pan > 0 ? pan * 64 : 0), gr = 4096 + (pan < 0 ? pan * 64 : 0);
    if (drums.a0 >= 32767 && drums.a1 >= 32767 && !((perf_kill >> TRK_DRUM) & 1u)) {   /* (3.1.3) muted (or another track soloed) all this block: nothing
                                                       * is heard, so nothing is computed (measured on an FM-1: a muted
                                                       * drum track still cost ~0.38 ms of a 0.73 ms block). Its sounds end
                                                       * (unmuted, the next hits play). (wave 6) Not for PERFORM's KILL
                                                       * DRUMS: it plays on unheard and comes back as it was */
        for (k = 0; k < NDRUM; k++)
            drums.v[k].active = 0;
        drums.tail = drums.trev = drums.tdly = 0;
        drums.peak = 0;
        hx_pre_mask = 0;
        return;
    }
#define DRUM_FADE(x) ((x) -= (x) / 16 + ((x) > 0 ? 1 : (x) < 0 ? -1 : 0))
    for (i = 0; i < n && (drums.tail | drums.trev | drums.tdly); i++) {   /* declick tails, ~0.4 ms */
        if (mono) {
            mono[i] += drums.tail;
        } else {
            ml[i] += drums.tail;
            mr[i] += drums.tail;
        }
        rev[i] += drums.trev;
        dly[i] += drums.tdly;
        DRUM_FADE(drums.tail);
        DRUM_FADE(drums.trev);
        DRUM_FADE(drums.tdly);
    }
#undef DRUM_FADE
    for (k = 0; k < NDRUM; k++) {               /* synthesised voices: render, then as below */
        voice_t *v = &drums.v[k];
        uint32_t m = n < CTL ? n : CTL;             /* (the mix runs in blocks of CTL) */
        int32_t lp16, lcut, lres, llvl;
        if (!v->active || !drums.synth[k])
            continue;
        drum_voice_sends(k, mono != 0, &send, &dsend);
        drums.gs[k][0] = send;                      /* (3.1.2: for drum_cut) */
        drums.gs[k][1] = dsend;
        drum_lfo_mod(k, &lp16, &lcut, &lres, &llvl);   /* (PHEBUS wave 3) the sound's LFO */
        if (drums.synth[k] == 2 && ((hx_pre_mask >> k) & 1u)) {   /* (3.1.3: CPU1 rendered it) */
            for (i = 0; i < m; i++)
                ds_buf[i] = hx_pre_buf[k][i];
            if (!hx_pre_alive[k])
                v->active = 0;
        } else if (drums.synth[k] == 2) {
            lx_par_t hp;
            drum_hex_par(k, &hp);
            if (!hex_render_p(&drums.vx[k].hx, ds_buf, m, &hp))
                v->active = 0;
        } else {
            drums.vx[k].ds.lp16 = (int16_t)lp16;
            drums.vx[k].ds.lcut = (int16_t)(lcut * 16);
            drums.vx[k].ds.lres = (int16_t)(lres / 4);
            if (!ds_render(&drums.vx[k].ds, ds_buf, m))
                v->active = 0;
        }
        if (llvl != 16384)
            for (i = 0; i < m; i++)
                ds_buf[i] = (ds_buf[i] * llvl) >> 14;
        drum_lfo_tick(k, m);
        kboost = (int32_t)((DRUM_BOOST_Q8 * (drums.trim[k] < 7u ? DRUM_KIT_TRIM[drums.trim[k]] : 256u)) >> 8);   /* (wave 12: the
                                                         * slot's kit's trim, a voice each: a mixed kit as each sound was) */
        {   /* (wave 13) a slot PERFORM mutes: its fade with the track's (else the track's alone, as before) */
            uint32_t ln = drums.lane[k] % DRUM_LANES;
            if (lany && (la0[ln] | la1[ln])) {
                int32_t g0 = 32767 - la0[ln], g1 = 32767 - la1[ln];
                for (i = 0; i < m; i++)                 /* (>> 2: no 32-bit overflow on a loud hit) */
                    ds_buf[i] = ((ds_buf[i] >> 2) * (g0 + (((g1 - g0) * (int32_t)i) >> CTL_LOG2))) >> 13;
            }
        }
        for (i = 0; i < m; i++) {
            int32_t s = mulq15(ds_buf[i], mulq15(lvl, 32767 - drums.a0 - (((drums.a1 - drums.a0) * (int32_t)i) >> CTL_LOG2)));
            s = soft_knee((s * kboost) >> 8, DRUM_KNEE);   /* louder, the hits' tips rounded (2026-10-20) */
            v->s[7] = s;
            if (s > pk || -s > pk)
                pk = s < 0 ? -s : s;
            if (mono) {
                mono[i] += s;
                if (send)
                    rev[i] += mulq15(s, send);
                if (dsend)
                    dly[i] += mulq15(s, dsend);
                continue;
            }
            ml[i] += (s * gl) >> 12;
            mr[i] += (s * gr) >> 12;
            if (send)
                rev[i] += mulq15(s, send);
            if (dsend)
                dly[i] += mulq15(s, dsend);
        }
        if (!v->active) {
            drum_cut(k);                            /* ended: no step at the end */
            v->s[7] = 0;
        }
    }
    drums.peak = pk;
    dlr.t += n;
    hx_pre_mask = 0;                                /* (3.1.3: used: the next block is CPU1's again, or rendered here) */
}
static void drums_render(int32_t *ml, int32_t *mr, int32_t *rev, int32_t *dly, uint32_t n) { drums_mix(ml, mr, rev, dly, 0, n); }
static void drums_render_mono(int32_t *rev, int32_t *dly, int32_t *mono, uint32_t n) { drums_mix(0, 0, rev, dly, mono, n); }
