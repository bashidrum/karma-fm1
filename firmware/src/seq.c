/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Keyboard and its layers, scale and chords, arpeggiator, note repeat, sequencer, recording and
 * transport. Runs in the audio ISR, once per CTL-sample block (events_block), and ends in
 * trk_note_on / trk_note_off / drum_on: engines never see where a note came from.
 *
 * Time: the transport clock of fx.c (clk_beat, clk_pos, in units of a sample at 1 BPM). Every track
 * reads its step from it (trk_grid): the step is the clock divided by the track's DIV, swung, modulo
 * its LEN. So the tracks, the click, the arp, the rolls and the sections never drift apart, a
 * tempo or DIV change plays at most one step a block, and a polymeter (any LEN) stays in phase.
 *
 * Four tracks: tracks 1..3 are synth parts (steps of up to 4 notes, each note a level (ghost .. hard) and a ratchet:
 * x1..x4 hits in its step), track 4 the drum track (PHEBUS wave 2: 16 sounds, one per white key, each with its own
 * trigs, core.h dpat_t: a velocity, and maybe a condition, a chance, a nudge, a retrig and locks of its own sound). The keys play the selected track; MIDI channels 1..3 play parts 1..3,
 * the DRUMS channel (GLO > DRUMS, default 10) the drum track, any other channel the selected track.
 *
 * Layers: a function button held turns the keys into something else (TE style: hold + touch):
 *   FX   punch-in effects (punch.c)       EDIT  erase that note / sound (while held, as it plays)
 *   ARP  note repeat (roll) at G_ROLL     SEQ   steps 1..16 (the UI: ui_layers.c)
 *   SEL  the key of the song (the UI)     GLO   mute / solo / fill / tap tempo (the UI)
 * On the drum track OCT- / OCT+ held play (and record) ghost / hard hits. */
static const uint16_t SCALE_MASK[] = {
    0xFFF,                                   /* CHR */
    (1 << 0) | (1 << 2) | (1 << 4) | (1 << 5) | (1 << 7) | (1 << 9) | (1 << 11),   /* MAJ */
    (1 << 0) | (1 << 2) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 8) | (1 << 10),   /* MIN */
    (1 << 0) | (1 << 2) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 9) | (1 << 10),   /* DOR */
    (1 << 0) | (1 << 2) | (1 << 4) | (1 << 5) | (1 << 7) | (1 << 9) | (1 << 10),   /* MIX */
    (1 << 0) | (1 << 2) | (1 << 4) | (1 << 7) | (1 << 9),                          /* PEN */
    (1 << 0) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 10),                         /* MPEN */
    (1 << 0) | (1 << 2) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 8) | (1 << 11),   /* HARM */
    (1 << 0) | (1 << 1) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 8) | (1 << 10),   /* PHRY */
    (1 << 0) | (1 << 2) | (1 << 4) | (1 << 6) | (1 << 7) | (1 << 9) | (1 << 11),   /* LYD */
    (1 << 0) | (1 << 1) | (1 << 3) | (1 << 5) | (1 << 6) | (1 << 8) | (1 << 10),   /* LOC */
    (1 << 0) | (1 << 2) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 9) | (1 << 11),   /* MEL (ascending) */
    (1 << 0) | (1 << 3) | (1 << 5) | (1 << 6) | (1 << 7) | (1 << 10),              /* BLUES (minor) */
    (1 << 0) | (1 << 2) | (1 << 4) | (1 << 6) | (1 << 8) | (1 << 10),              /* WHOLE */
    (1 << 0) | (1 << 1) | (1 << 3) | (1 << 4) | (1 << 6) | (1 << 7) | (1 << 9) | (1 << 10), /* DIMHW */
    (1 << 0) | (1 << 2) | (1 << 3) | (1 << 5) | (1 << 6) | (1 << 8) | (1 << 9) | (1 << 11), /* DIMWH */
};
#define NSCALES (sizeof SCALE_MASK / sizeof SCALE_MASK[0])
#define SEQ_NONE 0xFFFFFFFFu
static void drum_rt_clear(void);              /* (the drum trigs as they play, below) */
static uint32_t seq_src(const struct track *t, uint32_t nabs);

#define KB_SILENT 255u
static uint32_t kb_prev;
/* per key: what its press started, so its release ends the same (whatever the layer or track is now) */
enum { KS_NONE, KS_NOTE, KS_DRUM, KS_ROLL, KS_ERASE, KS_FX, KS_UI, KS_MOD };   /* KS_MOD: a chord modifier (2.4) */
static uint8_t kb_kind[27], kb_trk[27], kb_n[27], kb_nt[27][4], kb_root[27];   /* kb_root: a chord key's note (CHORD+) */
static int8_t kb_oct[27];                   /* the octave buttons when the key went down (arp_add: held_key, #191) */
static uint8_t last_note = 60;
static uint8_t pen_n = 1, pen_note[4] = {60};   /* the last chord / note played: the SEQ layer writes it */
/* SLOOP 2.6: the MIDI keyboard's notes. mi_pen: the notes held on the selected track (its pen, as the keys: the notes
 * down together are a chord). step_in_q: every synth note-on / note-off from MIDI, note | on << 7 | track << 8, for the
 * UI (ui_input.c midi_entry: the MIDI hint while recording): the audio side writes, the UI reads; a full queue drops and says so */
static uint32_t mi_pen[4];
#define STEP_IN_Q 32u
static volatile uint16_t step_in_q[STEP_IN_Q];
static volatile uint32_t step_in_w;
static volatile uint8_t step_in_lost;
static uint32_t step_in_r;
static volatile uint8_t pc_req[NTRK];          /* (2.6) a MIDI Program Change for the track: number + 1 (ui.c pc_poll) */
static void step_in_push(uint32_t ev)
{
    if (step_in_w - step_in_r >= STEP_IN_Q) {
        step_in_lost = 1;
        return;
    }
    step_in_q[step_in_w % STEP_IN_Q] = (uint16_t)ev;
    step_in_w++;
}
static volatile uint8_t transport_req;   /* 1 start, 2 stop (from the UI); 3 start from a MIDI START */
static volatile uint8_t panic_req;       /* bit per track: release every sounding note (preset / engine change) */

static uint32_t trk_index(const track_t *t) { return (uint32_t)(t - trk); }

static uint32_t trk_midi_ch(uint32_t i)    /* MIDI channel 0..15 of track i (keys -> MIDI out); 2.6: GLO > MIDI */
{                                          /* (2.6: OFF is OFF, in and out: 0xFF, nothing sent) */
    int32_t c = i < NPART ? song.g[G_CH1 + i] : song.g[G_DRCH];
    return c >= 1 && c <= 16 ? (uint32_t)c - 1u : 0xFFu;
}
/* a note on (vel > 0) / off to MIDI OUT on channel ch (trk_midi_ch: 0xFF, the track's MIDI OFF: nothing) */
static void midi_out_note(uint32_t ch, uint32_t note, uint32_t vel)
{
    if (ch > 15u)
        return;
    midi_out_event(vel ? 0x09u | (0x90u | ch) << 8 | (note & 127u) << 16 | (vel & 127u) << 24
                       : 0x08u | (0x80u | ch) << 8 | (note & 127u) << 16);
}

/* MIDI OUT of what the sequencer, the arp and the rolls play (GLO > SYSTEM > MIDI = SEQ; the keys always
 * go out: key_down / key_up). Notes from a computer or the jack are never echoed (no MIDI loop). A set per
 * track of the notes sent on, so a note is ended once, and STOP or MIDI = KEYS end them all */
static uint32_t mo_set[NTRK][4];
static uint8_t mo_ch[NTRK];                /* (2.6) the channel the notes of mo_set went out on: their note-offs too,
                                            * whatever GLO > MIDI says now (no note left hanging on the other gear) */
static void seq_out_off(const track_t *t, uint32_t note)
{
    uint32_t i = trk_index(t) % NTRK;
    if (note > 127u || !(mo_set[i][note >> 5] & (1u << (note & 31u))))
        return;
    mo_set[i][note >> 5] &= ~(1u << (note & 31u));
    midi_out_note(mo_ch[i], note, 0);
}
static void seq_out_track_off(const track_t *t);
static uint8_t mo_any;                     /* something was sent on since the last check (events_block) */
static void seq_out_on(const track_t *t, uint32_t note, uint32_t vel)
{
    uint32_t i = trk_index(t) % NTRK;
    if (!song.g[G_MIDI] || note > 127u)
        return;
    seq_out_off(t, note);                      /* played again while on: off first */
    if (trk_midi_ch(i) != mo_ch[i]) {          /* (its channel changed: what is on ends where it started) */
        seq_out_track_off(t);
        mo_ch[i] = (uint8_t)trk_midi_ch(i);
    }
    if (mo_ch[i] > 15u)
        return;                                /* (MIDI OFF: nothing goes out, nothing to end) */
    mo_set[i][note >> 5] |= 1u << (note & 31u);
    mo_any = 1;
    midi_out_note(mo_ch[i], note, (vel ? vel & 127u : 1u));
}
static void seq_out_track_off(const track_t *t)      /* every note of the track still on */
{
    uint32_t i = trk_index(t) % NTRK, w, b;
    for (w = 0; w < 4u; w++)
        for (b = 0; mo_set[i][w]; b++)
            if (mo_set[i][w] & (1u << b)) {
                mo_set[i][w] &= ~(1u << b);
                midi_out_note(mo_ch[i], (w * 32u + b), 0);
            }
}
static void seq_out_all_off(void)
{
    uint32_t i;
    for (i = 0; i < NTRK; i++)
        seq_out_track_off(&trk[i]);
}

static uint32_t scale_mask(const track_t *t)
{
    return SCALE_MASK[clamp(t->p[P_SCALE], 0, NSCALES - 1)];
}

/* ---------------------------------------------------------- layers --- */
enum { LY_PLAY, LY_FX, LY_ERASE, LY_ROLL, LY_STEP, LY_SCALE, LY_MIX, LY_SONG, LY_COUNT };
static uint32_t ly_bit[LY_COUNT];        /* the button (fm1_in.buttons bit) of each layer: the UI sets them */
static uint32_t dyn_bit[2];              /* OCT- / OCT+: ghost / hard on the drum track */
/* a layer locked open (its button held + HOME tapped: ui_input.c), LY_PLAY = none: the keys and knobs
 * stay in it with the button let go, as if it were held */
static volatile uint8_t ly_lock = LY_PLAY;
/* (PHEBUS, Ben 21 Nov 2026: step-record, "press REC in the GRID, play notes or chords on the keys, each lands on the next
 * step") STEP REC on a synth track's GRID (ui_grid.c sr_*): the keys play again and every note they play (and MIDI's) goes
 * to the UI through step_in_q; ly_off: the layer buttons that are not layers meanwhile (SEQ: rest, ARP: tie) */
static volatile uint8_t sr_on;
static volatile uint32_t ly_off;
static uint32_t layer_buttons(void) { return (fm1_in.buttons & ~ly_off) | (ly_lock != LY_PLAY ? ly_bit[ly_lock % LY_COUNT] : 0u); }
/* the layer the keys are in: the held function button (FX, EDIT, ARP, SEQ, SEL, GLO in that order),
 * else the locked one */
static uint32_t layer_now(void)
{
    uint32_t b = layer_buttons(), l;
    for (l = LY_FX; l < LY_COUNT; l++)
        if (b & ly_bit[l])
            return l;
    return LY_PLAY;
}
/* the GRID shown (the UI sets it every frame; every track, ui_grid.c): with no layer, the keys are its steps
 * (KB_GRID events of lk_q, ui_grid.c grid_key) and play nothing */
#define KB_GRID LY_COUNT
static volatile uint8_t kb_grid;
/* the keys of the layers the UI handles (steps, key, mix): key k down / up, in order */
#define LKQ 16u
static volatile uint16_t lk_q[LKQ];
static volatile uint32_t lk_w, lk_r;
static void lk_push(uint32_t layer, uint32_t k, uint32_t down)
{
    if (lk_w - lk_r < LKQ) {
        lk_q[lk_w % LKQ] = (uint16_t)(layer << 8 | down << 7 | k);
        RING_PUBLISH();
        lk_w++;
    }
}

/* ------------------------------------------------------------- keys --- */
/* the keys of track t play through its scale (kb_pitch): not the drum track (its own map) */
static int kb_scaled(const track_t *t)
{
    return !is_drum(t);
}
/* key k's note on a scaled track with the octave buttons at oct (KB_SILENT: none): QNT, ROOT, SCALE, TRN as they are
 * now. WHITE (and chord mode): the white keys walk the scale from C4 = the root, the black keys are silent; SNAP: every
 * key, rounded down into the scale. The keys (kb_map), and the notes the arp holds (arp_next, 2.6 after Felucca 1.5
 * #191: a latched chord follows TRN, ROOT, SCALE and QNT, in the octave its keys were pressed in) */
static uint32_t kb_pitch(const track_t *t, uint32_t k, int32_t oct)
{
    static const int8_t DEGREE[12] = {0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6};
    int32_t n = 53 + (int32_t)k;
    if (t->p[P_QUANT] == 1 && !t->p[P_CHORD]) {  /* SNAP: every key, rounded down to the scale (the old ON) */
        uint32_t mask = scale_mask(t), guard = 12;
        n += 12 * oct + t->p[P_TRANS];
        while (guard-- && !((mask >> (uint32_t)((n - t->p[P_ROOT] + 120) % 12)) & 1u))
            n--;
        return (uint32_t)clamp(n, 0, 127);
    }
    if (t->p[P_QUANT] == 2 || t->p[P_CHORD]) {   /* WHITE: white keys walk the scale, black keys are silent */
        uint32_t mask = t->p[P_CHORD] && !t->p[P_SCALE] ? SCALE_MASK[2] : scale_mask(t), i;
        int32_t count = 0, degree = DEGREE[n % 12], wo;
        if (degree < 0)
            return KB_SILENT;
        /* C4 is the root. Walk scale degrees on successive white keys, including
         * below C4; scales with 5, 6, 8 or 12 notes still have no duplicated degrees. */
        degree += (n / 12 - 5) * 7;
        for (i = 0; i < 12u; i++)
            count += (mask >> i) & 1u;
        wo = degree / count;
        degree %= count;
        if (degree < 0) {
            degree += count;
            wo--;
        }
        for (i = 0; i < 12u; i++)
            if ((mask >> i) & 1u) {
                if (!degree)
                    break;
                degree--;
            }
        n = 60 + t->p[P_ROOT] + 12 * wo + (int32_t)i;
    }
    return (uint32_t)clamp(n + 12 * oct + t->p[P_TRANS], 0, 127);
}
/* key k -> note on track t (KB_SILENT: none) */
static uint32_t kb_map(const track_t *t, uint32_t k)
{
    if (is_drum(t))
        return LANE_NOTE[lane_of_key(k)];
    return kb_pitch(t, k, song.octave);
}

/* chord mode (P_CHORD): the chord of the scale built on note n (in the scale; CHR: minor), into c[];
 * the notes it holds (<= 4, the most a step keeps) */
static const int8_t CHORD_DEG[6][4] = {
    {0, -1, -1, -1},                     /* OFF */
    {0, 2, 4, -1},                       /* TRIAD: 1 3 5 */
    {0, 2, 4, 6},                        /* 7TH: 1 3 5 7 */
    {0, 2, 6, 8},                        /* 9TH: 1 3 7 9 (the lo-fi / R&B voicing) */
    {0, 3, 4, -1},                       /* SUS4: 1 4 5 */
    {0, -1, -1, -1},                     /* POWER: 1 5 8 (semitones, below) */
};
static uint32_t chord_notes(const track_t *t, uint32_t n, uint8_t *c)
{
    uint32_t type = (uint32_t)clamp(t->p[P_CHORD], 0, 5), mask = t->p[P_SCALE] ? scale_mask(t) : SCALE_MASK[2];
    uint32_t k = 0, j;
    if (type == 5u) {
        static const uint8_t PW[3] = {0, 7, 12};
        for (j = 0; j < 3u; j++)
            if (n + PW[j] < 128u)
                c[k++] = (uint8_t)(n + PW[j]);
        return k;
    }
    for (j = 0; j < 4u && CHORD_DEG[type][j] >= 0; j++) {
        int32_t m = (int32_t)n, d = CHORD_DEG[type][j], guard = 48;
        while (d > 0 && guard--) {                       /* d scale degrees up */
            m++;
            if ((mask >> (uint32_t)((m - t->p[P_ROOT] + 120) % 12)) & 1u)
                d--;
        }
        if (m < 128)
            c[k++] = (uint8_t)m;
    }
    return k;
}

/* CHORD+ (2.4, after HiChord / minichord): in chord mode the black keys are modifiers. Held while a white key
 * plays (or pressed while it is held: the chord changes under the finger), they change its chord: F# flips its
 * third (major <-> minor), G# adds the 7th, A# makes it sus4, C# adds the 9th, D# inverts it (its lowest note an
 * octave up); several at once combine. P_VLEAD ON voices each chord nearest the last one played on the track. */
enum { CM_MINOR = 1, CM_SEVEN = 2, CM_SUS4 = 4, CM_NINE = 8, CM_INV = 16 };
static uint32_t chord_mod_of_key(uint32_t k)            /* key k's modifier (0: a white key) */
{
    switch ((53u + k) % 12u) {
    case 6: return CM_MINOR;                            /* F# */
    case 8: return CM_SEVEN;                            /* G# */
    case 10: return CM_SUS4;                            /* A# */
    case 1: return CM_NINE;                             /* C# */
    case 3: return CM_INV;                              /* D# */
    default: return 0;
    }
}
static uint8_t vl_prev[NPART][4], vl_n[NPART];          /* the last chord played on each part (voice leading) */
static uint32_t scale_up(const track_t *t, uint32_t n, uint32_t deg)   /* deg scale degrees above n */
{
    uint32_t mask = t->p[P_SCALE] ? scale_mask(t) : SCALE_MASK[2];
    int32_t m = (int32_t)n, guard = 48;
    while (deg > 0 && guard--) {
        m++;
        if ((mask >> (uint32_t)((m - t->p[P_ROOT] + 120) % 12)) & 1u)
            deg--;
    }
    return (uint32_t)m;
}
static void sort_notes(uint8_t *c, uint32_t n)
{
    uint32_t a, b;
    for (a = 1; a < n; a++)
        for (b = a; b > 0 && c[b - 1u] > c[b]; b--) {
            uint8_t x = c[b]; c[b] = c[b - 1u]; c[b - 1u] = x;
        }
}
/* the chord of white key n with the modifiers held, voiced (VLEAD); its notes (<= 4) into c[] */
static uint32_t chord_play_notes(track_t *t, uint32_t n, uint32_t mods, uint8_t *c)
{
    uint32_t k = chord_notes(t, n, c), j, type = (uint32_t)clamp(t->p[P_CHORD], 0, 5), part = trk_index(t);
    if (type != 5u && k >= 2u && mods) {                /* (POWER: the inversion only) */
        uint32_t third = scale_up(t, n, 2u);
        for (j = 0; j < k; j++) {
            if (c[j] == third && (mods & CM_SUS4))
                c[j] = (uint8_t)scale_up(t, n, 3u);     /* the 4th instead of the 3rd */
            else if (c[j] == third && (mods & CM_MINOR))
                c[j] = (uint8_t)(c[j] - n == 4u ? c[j] - 1u : c[j] - n == 3u ? c[j] + 1u : c[j]);
        }
        if ((mods & CM_SEVEN) && k < 4u) {
            uint32_t s7 = scale_up(t, n, 6u);
            for (j = 0; j < k && c[j] != s7; j++)
                ;
            if (j == k && s7 < 128u)
                c[k++] = (uint8_t)s7;
        }
        if (mods & CM_NINE) {
            uint32_t s9 = scale_up(t, n, 8u), s5 = scale_up(t, n, 4u);
            for (j = 0; j < k && c[j] != s9; j++)
                ;
            if (j == k && s9 < 128u) {
                if (k < 4u) {
                    c[k++] = (uint8_t)s9;
                } else {                                /* four already: the 9th for the 5th */
                    for (j = 0; j < k && c[j] != s5; j++)
                        ;
                    c[j < k ? j : k - 1u] = (uint8_t)s9;
                }
            }
        }
    }
    sort_notes(c, k);
    if (t->p[P_VLEAD] && part < NPART && vl_n[part] && k) {   /* the inversion and octave nearest the last chord */
        uint8_t best[4], cand[4];
        int32_t bcost = 0x7FFFFFFF, inv, oct;
        for (inv = 0; inv < (int32_t)k; inv++)
            for (oct = -1; oct <= 1; oct++) {
                int32_t cost = 0, ok = 1;
                for (j = 0; j < k; j++) {
                    int32_t v = c[j] + 12 * oct + (j < (uint32_t)inv ? 12 : 0);
                    if (v < 24 || v > 108)
                        ok = 0;
                    cand[j] = (uint8_t)(v < 0 ? 0 : v > 127 ? 127 : v);
                }
                if (!ok)
                    continue;
                sort_notes(cand, k);
                for (j = 0; j < k; j++) {
                    int32_t d = (int32_t)cand[j] - vl_prev[part][j < vl_n[part] ? j : vl_n[part] - 1u];
                    cost += d < 0 ? -d : d;
                }
                if (cost < bcost) {
                    bcost = cost;
                    memcpy(best, cand, k);
                }
            }
        if (bcost != 0x7FFFFFFF)
            memcpy(c, best, k);
    }
    if ((mods & CM_INV) && k >= 2u && c[0] + 12u < 128u) {   /* an inversion: the lowest an octave up */
        c[0] = (uint8_t)(c[0] + 12u);
        sort_notes(c, k);
    }
    if (part < NPART) {
        memcpy(vl_prev[part], c, k);
        vl_n[part] = (uint8_t)k;
    }
    return k;
}

/* ------------------------------------------------------------- grid --- */
/* units an odd step starts late: the track's + the global SWING (MPC: 0 = 50 %, 100 = 75 %) */
static uint32_t swing_units(int32_t pct, uint32_t u)
{
    return (uint32_t)clamp(pct, 0, 100) * u / 200u;
}
/* a position on a grid of steps u units long, odd steps sw late: the step, units into it, its length */
static uint32_t grid_swing(uint32_t abs, uint32_t frac, uint32_t u, uint32_t sw, uint32_t *into, uint32_t *len)
{
    if (abs & 1u) {                                      /* an odd step: sw late */
        if (frac < sw) {
            abs--;
            frac += u;
            *len = u + sw;
        } else {
            frac -= sw;
            *len = u - sw;
        }
    } else {
        *len = u + sw;
    }
    *into = frac;
    return abs;
}
/* the clock on a grid of den steps a beat (the rolls: ROLL_DEN) */
static uint32_t grid_den(uint32_t den, uint32_t sw, uint32_t *into, uint32_t *len)
{
    uint32_t u = BEAT_U / den;
    return grid_swing(clk_beat * den + clk_pos / u, clk_pos % u, u, sw, into, len);
}
/* the clock on the grid of a division (N_SDIV: steps inside a beat, or of 2, 4, 8 whole beats) */
static uint32_t grid_at(uint32_t div, uint32_t sw, uint32_t *into, uint32_t *len)
{
    uint32_t m;
    if (div < NDIV_SHORT)
        return grid_den(DIV_DEN[div], sw, into, len);
    m = DIV_BEATS[(div - NDIV_SHORT) % 3u];              /* whole beats: the step from the beat count */
    return grid_swing(clk_beat / m, (clk_beat % m) * BEAT_U + clk_pos, BEAT_U * m, sw, into, len);
}
/* swing is for the straight grids inside a beat: off on the triplet grids (on 8T the odd steps would
 * change from one beat to the next, as on most machines) and on steps of whole beats */
static uint32_t swings(uint32_t div) { return div < 4u; }
static uint32_t trk_div(const track_t *t) { return (uint32_t)t->p[P_SDIV] % NDIV_STEP; }
static uint32_t trk_grid(const track_t *t, uint32_t *into, uint32_t *len)
{
    uint32_t div = trk_div(t);
    return grid_at(div, swings(div) ? swing_units(t->p[P_SSWING] + song.g[G_SWING], div_units(div)) : 0u, into, len);
}
static uint32_t trk_len(const track_t *t) { return t->p[P_SLEN] > 0 ? (uint32_t)t->p[P_SLEN] : 1u; }

/* ------------------------------------------------------------- undo --- */
/* One step back (and forward again) for the pattern of one track: what it was before the last
 * recording pass, erase, step edit, tool or clear (a session: one mark). EDIT + OCT- / OCT+. The whole of
 * every step: its notes and length, and its locks, nudge and condition (REVIEW.md 7, 21 Nov 2026) */
/* (PHEBUS wave 2) the drum track: its whole pattern (dpat) and its bus locks. In .noinit (RAM is short; it is
 * garbage after a power-off: undo_init clears it at start) */
static struct {
    uint8_t valid, undone, trk;
    int16_t len;
    uint32_t sess;
    union {
        struct {
            step_t st[NSTEP];
            int8_t micro[NSTEP];
            uint8_t fill[NSTEP / 4];
        };
        dpat_t dp;                       /* (the drum track) */
    };
    plock_t lock[NLOCK];
} undo __attribute__((section(".noinit")));
static void undo_init(void) { memset(&undo, 0, sizeof undo); }
static uint32_t undo_sess = 1;           /* UI sessions (seq.c: recording passes use the track's pass) */
static void undo_mark(const track_t *t, uint32_t sess)
{
    uint32_t i = trk_index(t);
    if (undo.valid && !undo.undone && undo.trk == i && undo.sess == sess)
        return;                                          /* (this session is marked already) */
    if (is_drum(t)) {
        memcpy(&undo.dp, &dpat, sizeof undo.dp);
    } else {
        memcpy(undo.st, t->step, sizeof undo.st);
        memcpy(undo.micro, t->micro, sizeof undo.micro);
        memcpy(undo.fill, t->fill, sizeof undo.fill);
    }
    memcpy(undo.lock, t->lock, sizeof undo.lock);
    undo.len = t->p[P_SLEN];
    undo.trk = (uint8_t)i;
    undo.sess = sess;
    undo.valid = 1;
    undo.undone = 0;
}
#define UNDO_REC(t) (((t)->pass << 2) | 1u)      /* a recording pass of track t */
static uint32_t undo_erase_sess;

/* ------------------------------------------------------- recording --- */
/* key to ear, in samples: the key's debounce (~3 ms) and the audio out buffer (HALF_FRAMES to
 * 2 x HALF_FRAMES, ~9 ms on average). A note played in time with what the player hears reaches
 * the sequencer this much later than the sound it was played to: recording takes it back. */
#define REC_LAT 512u

/* the step a note played now goes into (as heard: REC_LAT earlier): the one playing, or the next one
 * when it is past the middle of the playing one; *later: it has not played yet (it must not sound twice) */
static uint32_t rec_target(const track_t *t, uint32_t *later)
{
    uint32_t into, slen, abs = trk_grid(t, &into, &slen), half = slen / 2u, lat = REC_LAT * (uint32_t)song.g[G_BPM];
    if (into > half + (lat < half ? lat : half))
        abs++;
    *later = abs != t->seq_abs;
    return abs;
}

/* note into synth step idx (overdub: a step that holds notes gets this one added, a chord of up to 4;
 * when full, the last note is replaced; MONO / LEGATO / UNISON parts keep one note per step) */
static void step_add(track_t *t, uint32_t idx, uint32_t note, uint32_t vel, uint32_t lvl, uint32_t rat)
{
    step_t *s = &t->step[idx];
    uint32_t k;
    if (s->time != ST_NOTE || !s->n || t->p[P_VOICE] != V_POLY) {
        s->n = 0;                                   /* a fresh step */
        s->flags = 0;
        s->vel = 0;
        s->lvl = 0;
        s->rat = 0;
    }
    for (k = 0; k < s->n && s->note[k] != note; k++)
        ;
    if (k == s->n) {
        if (s->n < 4u)
            s->n++;
        k = s->n - 1u;
        s->note[k] = (uint8_t)note;
    }
    s->lvl = (uint8_t)((s->lvl & ~(3u << (2u * k))) | (lvl & 3u) << (2u * k));
    s->rat = (uint8_t)((s->rat & ~(3u << (2u * k))) | (rat & 3u) << (2u * k));
    s->time = ST_NOTE;
    if (vel > s->vel)
        s->vel = (uint8_t)vel;
}

/* live recording into a synth part: the nearest step (rec_target). Held on: each further step the
 * sequencer enters while the note is held becomes a TIE (rec_hold), up to the pattern length and
 * never over a step with notes (overdub keeps them); a release before the middle of the last one
 * puts that step back (rec_release), so a short note stays one step. A note recorded into another
 * step ends the hold before (the step model ties the notes of one step only). */
static void rec_note(track_t *t, uint32_t note, uint32_t vel, uint32_t rat, int hold)
{
    uint32_t len = trk_len(t), later, abs = rec_target(t, &later), idx = abs % len, k;
    undo_mark(t, UNDO_REC(t));
    step_add(t, idx, note, vel, vel_lvl(vel), rat);
    t->seq_active = 1;
    if (later) {                                    /* it sounds now: the step must not trigger it again */
        if (t->rskip_abs != abs)
            t->rskip_n = 0;
        t->rskip_abs = abs;
        if (t->rskip_n < 4u)
            t->rskip[t->rskip_n++] = (uint8_t)note;
    }
    if (!hold)
        return;
    if (!t->rh_n || t->rh_start != idx) {           /* a new hold (one in another step ends) */
        t->rh_n = 0;
        t->rh_start = (uint8_t)idx;
        t->rh_ties = 0;
    }
    for (k = 0; k < t->rh_n && t->rh_note[k] != note; k++)
        ;
    if (k == t->rh_n && t->rh_n < 4u)
        t->rh_note[t->rh_n++] = (uint8_t)note;      /* a chord: held until its last key is up */
}

static void dt_retrig_hits(uint32_t i, uint32_t l, uint32_t hits);   /* (below) */
static void dt_retrig_rate(uint32_t i, uint32_t l, uint32_t r);
/* live recording into the drum track: the sound's trig at the velocity played (over one there: its velocity), and a roll
 * faster than the steps (rat: hits in the step - 1) as its retrig */
static uint32_t rec_hit(track_t *t, uint32_t lane, uint32_t vel, uint32_t rat)   /* the step it went on */
{
    uint32_t later, abs = rec_target(t, &later), i = abs % trk_len(t);
    undo_mark(t, UNDO_REC(t));
    dt_set(i, lane, vel);
    if (rat >= 0x80u)                                /* (PHEBUS wave 3: a live retrig's own rate, RT_DEN's index | 0x80) */
        dt_retrig_rate(i, lane, rat & 0x7Fu);
    else if (rat)
        dt_retrig_hits(i, lane, rat + 1u);
    t->seq_active = 1;
    if (later) {
        if (t->rskip_abs != abs)
            t->rskip_lanes = 0;
        t->rskip_abs = abs;
        t->rskip_lanes |= (uint16_t)(1u << lane);
    }
    return i;
}

/* the sequencer enters step idx (before playing it): a recorded note still held ties into it */
static void rec_hold(track_t *t, uint32_t idx, uint32_t len, uint32_t abs)
{
    step_t *s;
    if (!t->rh_n)
        return;
    if (!((song.rec >> trk_index(t)) & 1u) || t->rh_ties + 1u >= len) {
        t->rh_n = 0;                                /* disarmed, or the whole pattern is this note */
        return;
    }
    if (idx == t->rh_start)
        return;                                     /* (recorded ahead into the step now starting) */
    s = &t->step[idx];
    if (s->time == ST_NOTE && s->n) {
        t->rh_n = 0;                                /* a step with notes: the hold ends before it */
        return;
    }
    t->rh_bak = *s;
    t->rh_last = (uint8_t)idx;
    t->rh_last_abs = abs;
    t->rh_ties++;
    memset(s, 0, sizeof *s);
    s->time = ST_TIE;
}

/* a key of a recorded note is up: the hold ends with the last one */
static void rec_release(track_t *t, uint32_t note)
{
    uint32_t i, k = 0, into, slen, half, lat;
    for (i = 0; i < t->rh_n; i++)
        if (t->rh_note[i] != note)
            t->rh_note[k++] = t->rh_note[i];
    if (k == t->rh_n || (t->rh_n = (uint8_t)k))
        return;                                     /* not one of them, or others still held */
    if (!t->rh_ties || trk_grid(t, &into, &slen) != t->rh_last_abs)
        return;
    half = slen / 2u;
    lat = REC_LAT * (uint32_t)song.g[G_BPM];
    if (into <= half + (lat < half ? lat : half) && t->step[t->rh_last].time == ST_TIE)
        t->step[t->rh_last] = t->rh_bak;            /* released early in it: not held into this step */
}

/* LIVE recording, no click needed (REC: ui_input.c rec_toggle).
 *  playing: REC records the selected track at once (song.rec), quantised, overdub.
 *  stopped: REC arms (rec_wait). The first note played on the selected track starts the transport, that note is
 *   step 1, recording on, at the tempo set (an empty project too: KARMA's free take, which found the tempo from the
 *   playing, is cut, PHEBUS wave 1). PLAY while armed starts the transport and the recording together.
 *  The REC screen (ui_studio.c) sets how (a setting of the FM-1, panel.c lights_word):
 *   KNOB 3 START: NOTE (the first note starts the loop, as above)
 *          or COUNT (PLAY clicks one bar, 4 beats, then the loop and the recording start; notes
 *          played meanwhile only sound). */
static volatile uint8_t rec_wait;             /* 1: armed, waits for a note */
static uint8_t rec_count;                     /* REC screen START: 0 NOTE, 1 COUNT (one bar of clicks) */
static volatile uint8_t ci_on;                /* the count-in runs (armed, COUNT, PLAY) */
static volatile uint8_t ci_beat;              /* its beats clicked so far - 1 (the UI shows 4 - ci_beat) */
static uint32_t ci_u;                         /* clock units since it started */
static volatile uint8_t rec_go;               /* recording just started (the UI says so) */
static void seq_start(void);
static void rec_begin(void)
{
    song.rec = (uint8_t)(1u << (song.sel % NTRK));
    rec_wait = 0;
    rec_go = 1;
}

static int track_empty(const track_t *t)
{
    uint32_t k;
    for (k = 0; k < NSTEP; k++)
        if (is_drum(t) ? dt_mask(k) != 0u : t->step[k].time == ST_NOTE && t->step[k].n)
            return 0;
    return 1;
}
/* -------------------------------------------------- parameter locks --- */
/* A lock (track_t.lock, SLOOP 2.4): on its step the track's p[param] takes its value; the parameter goes back
 * to what it was at the next step without a lock on it (Elektron style: notes still ringing follow, the
 * engines read p[] every block). The locks in force are listed in lk_* (param, the base to go back to, the
 * value set). A knob turned while a lock is on wins: the value found is kept as the new base. Only the
 * sound parameters lock (p_lockable): the sequencer's, the arp's, the key's and the voice mode's do not. */
static int p_lockable(uint32_t id)
{
    return id <= P_LD_AMP || id == P_SGATE || (id >= P_DIST && id <= P_REV) || id == P_GLIDE || id == P_PAN ||
           id == P_DETUNE || (id >= P_SLCR && id <= P_SLDEPTH) || (id >= P_E0 && id <= P_E7) || id == P_TFLT ||
           (id >= P_IA && id <= P_IMIX);   /* 2.6: the INSERT's values and MIX */
}
/* the range of p[id] on track t (the engine that renders: t->engine; the drum track's P_E0: the kit) */
static const param_desc_t *lock_desc(const track_t *t, uint32_t id)
{
    if (is_drum(t) && id == P_E0)
        return &DRUM_KIT_DESC;
    if (id >= P_E0 && id <= P_E7)
        return &ENGINES[t->engine % NENGINES]->edit[id - P_E0];
    return &TP[id % P_COUNT];
}
static void lock_write(track_t *t, uint32_t id, int32_t v)
{
    const param_desc_t *d = lock_desc(t, id);
    t->p[id % P_COUNT] = (int16_t)clamp(v, d->min, d->max);
}
static void locks_restore(track_t *t)        /* every lock in force let go (STOP, a load, a cleared pattern) */
{
    uint32_t i;
    for (i = 0; i < t->lk_n && i < NLOCK; i++)
        if (t->p[t->lk_param[i] % P_COUNT] == t->lk_set[i])
            lock_write(t, t->lk_param[i], t->lk_base[i]);
    t->lk_n = 0;
}
static void locks_clear(track_t *t)          /* no lock, no nudge (an empty pattern) */
{
    uint32_t i;
    locks_restore(t);
    memset(t->micro, 0, sizeof t->micro);
    for (i = 0; i < NLOCK; i++) {
        t->lock[i].step = LOCK_FREE;
        t->lock[i].param = 0;
        t->lock[i].val = 0;
    }
}
/* the sequencer enters step idx: its locks take hold, the last step's that it does not share let go */
static void lock_step(track_t *t, uint32_t idx)
{
    uint32_t i, k, n = 0;
    for (i = 0; i < t->lk_n && i < NLOCK; i++) {      /* in force, no lock here: back to the base (or the knob) */
        uint32_t p = t->lk_param[i] % P_COUNT, has = 0;
        for (k = 0; k < NLOCK; k++)
            if (t->lock[k].step == idx && t->lock[k].param == p)
                has = 1;
        if (has) {
            t->lk_param[n] = (uint8_t)p;
            t->lk_base[n] = t->lk_base[i];
            t->lk_set[n] = t->lk_set[i];
            n++;
        } else if (t->p[p] == t->lk_set[i]) {
            lock_write(t, p, t->lk_base[i]);
        }
    }
    t->lk_n = (uint8_t)n;
    for (k = 0; k < NLOCK; k++) {                     /* this step's locks */
        const plock_t *l = &t->lock[k];
        uint32_t p = l->param;
        if (l->step != idx || p >= P_COUNT || !p_lockable(p))
            continue;
        for (i = 0; i < t->lk_n && t->lk_param[i] != p; i++)
            ;
        if (i == t->lk_n) {
            if (i >= NLOCK)
                continue;
            t->lk_param[i] = (uint8_t)p;
            t->lk_base[i] = t->p[p];
            t->lk_n++;
        } else if (t->p[p] != t->lk_set[i]) {
            t->lk_base[i] = t->p[p];                  /* turned meanwhile: that is the new base */
        }
        lock_write(t, p, l->val);
        t->lk_set[i] = t->p[p];
    }
}
/* a lock of (step, param): its slot, or a free one for it (-1: none left) */
static int lock_find(const track_t *t, uint32_t step, uint32_t param, int make)
{
    uint32_t k;
    int fr = -1;
    for (k = 0; k < NLOCK; k++) {
        if (t->lock[k].step == step && t->lock[k].param == param)
            return (int)k;
        if (fr < 0 && t->lock[k].step == LOCK_FREE)
            fr = (int)k;
    }
    return make ? fr : -1;
}
/* set (or make) the lock of (step, param) at v, clamped; 0 = no slot left or not lockable. Callers hold the IRQ off */
static int lock_set(track_t *t, uint32_t step, uint32_t param, int32_t v)
{
    int k;
    const param_desc_t *d;
    if (step >= NSTEP || param >= P_COUNT || !p_lockable(param) || (k = lock_find(t, step, param, 1)) < 0)
        return 0;
    d = lock_desc(t, param);
    t->lock[k].step = (uint8_t)step;
    t->lock[k].param = (uint8_t)param;
    t->lock[k].val = (int16_t)clamp(v, d->min, d->max);
    return 1;
}
static void lock_del(track_t *t, uint32_t step, uint32_t param)   /* param P_COUNT: every lock of the step */
{
    uint32_t k;
    for (k = 0; k < NLOCK; k++)
        if (t->lock[k].step == step && (param >= P_COUNT || t->lock[k].param == param))
            t->lock[k].step = LOCK_FREE;
}
static int step_locked(const track_t *t, uint32_t step)   /* the step carries a lock or a nudge (the UI's mark) */
{
    uint32_t k;
    if (step < NSTEP && t->micro[step])
        return 1;
    for (k = 0; k < NLOCK; k++)
        if (t->lock[k].step == step)
            return 1;
    return 0;
}

/* ---- step conditions (SLOOP 2.4 fill, core.h FC_*): 2 bits a step in t->fill; GLO + key 9 held, or key 10
 * for the next bar, makes the fill: FC_FILL steps play only then, FC_NOFILL steps are silent then */
static uint32_t step_fill(const track_t *t, uint32_t idx)
{
    idx %= NSTEP;
    return (uint32_t)(t->fill[idx / 4u] >> (2u * (idx % 4u))) & 3u;
}
static void step_fill_set(track_t *t, uint32_t idx, uint32_t v)
{
    uint32_t sh;
    idx %= NSTEP;
    sh = 2u * (idx % 4u);
    t->fill[idx / 4u] = (uint8_t)((t->fill[idx / 4u] & ~(3u << sh)) | (v & 3u) << sh);
}
static volatile uint8_t fill_held;           /* GLO + key 9 down (the UI) */
static volatile uint8_t fill_arm;            /* GLO + key 10: the next bar is a fill (the UI; the ISR clears it) */
static uint8_t fill_bar_on;                  /* that bar, while it plays (the ISR) */
static volatile uint8_t fill_lat;            /* (PHEBUS wave 3) a latched fill: on until it is unlatched or STOP (the UI) */
static uint8_t fill_now;                     /* this block is a fill: fill_held || fill_lat || fill_bar_on, read once a block */
static uint32_t fill_last_bar = 0xFFFFFFFFu; /* clk_beat / 4 of the last bar seen (events_block) */
/* a step's condition holds now. CHNC (FC_CHANCE): a synth step's is a fixed chance, 64 in 127 (about 50 %, what SEQ >
 * PLAY's CHANCE started at; PLAY is cut, PHEBUS wave 1); a drum trig has its own (dt_plays) */
#define STEP_CHANCE 64u
static uint32_t step_plays(const track_t *t, uint32_t idx)
{
    uint32_t c = step_fill(t, idx);
    if (c == FC_CHANCE)
        return rng() % 127u < STEP_CHANCE;
    return c == FC_FILL ? fill_now : c == FC_NOFILL ? !fill_now : 1u;
}
/* (PHEBUS wave 2) a drum trig's condition holds now (x: its extras' entry, -1 = none: always); CHNC: its chance */
static uint32_t dt_plays(int x)
{
    const dtx_t *e;
    if (x < 0)
        return 1u;
    e = &dpat.x[x];
    if (e->cond == FC_CHANCE)
        return rng() % 100u < e->prob;
    return e->cond == FC_FILL ? fill_now : e->cond == FC_NOFILL ? !fill_now : 1u;
}

static void steps_clear(track_t *t)           /* an empty pattern (synth: REST steps, drums: no trig); no lock, no nudge, no condition */
{
    uint32_t k;
    memset(t->step, 0, sizeof t->step);
    if (!is_drum(t))
        for (k = 0; k < NSTEP; k++)
            t->step[k].time = ST_REST;
    else
        dpat_clear(&dpat);
    locks_clear(t);
    memset(t->fill, 0, sizeof t->fill);
}

/* ------------------------------------------------------------ erase --- */
/* EDIT + key: that note / sound goes from the selected pattern. Playing: from the step playing and
 * every step the sequencer enters while the key is held (MPC style); stopped: from every step. */
static uint8_t er_trk;
static uint16_t er_lanes;                     /* drums: lanes held */
/* the drum track's live mutes (SLOOP 2.6, after Felucca 1.4.1): EDIT + a black key turns the white keys from erasing
 * into muting (er_mute; back to erasing with a black key again, and when EDIT is let go and not locked), and a white
 * key then mutes / unmutes its sound: the sequencer skips it (its ratchets too), the keys still play it. Performance
 * state: never saved, not undone; a project load or NEW clears it */
static volatile uint16_t drum_mute;
static volatile uint8_t er_mute;
/* (PHEBUS wave 3, after the Rytm's pad modes) what the keys do on the drum track when they are not the GRID's steps:
 * play the sounds (DK_PLAY), mute / unmute them (DK_MUTE, the Rytm's MUTE mode; SEL + key: solo, the UI), or play the
 * selected sound (drums.c drum_lane) across the keys at pitches (DK_CHROM, the Rytm's CHROMATIC mode; recorded, the
 * pitch is the trig's DSP_NOTE lock). drum_solo: the sounds soloed (none: all play). Performance state, as drum_mute */
enum { DK_PLAY, DK_MUTE, DK_CHROM };
static volatile uint8_t drum_keys;
static volatile uint16_t drum_solo;
static volatile int8_t drum_chr_oct;          /* chromatic mode: the keys' octave, -2..+2 (OCT- / OCT+, the UI) */
#define CHR_KEY0 7u                           /* chromatic mode: the key that plays the sound at its own pitch (C4) */
static volatile uint8_t dlk_full;             /* a recorded pitch found the locks' pool full (the UI says it) */
static int drum_silenced(uint32_t l)          /* the sequencer skips sound l: muted live, or another one soloed */
{
    return ((drum_mute >> l) & 1u) || (drum_solo && !((drum_solo >> l) & 1u));
}
static uint32_t er_notes[4];                  /* synth: notes held (bit per MIDI note) */
static volatile uint8_t er_flash;             /* something was erased (the UI flashes) */

static int er_has(uint32_t note) { return (er_notes[(note >> 5) & 3u] >> (note & 31u)) & 1u; }
static void erase_step(track_t *t, uint32_t idx)
{
    if (is_drum(t)) {
        uint32_t l, m = dt_mask(idx) & er_lanes;
        for (l = 0; m; l++, m >>= 1)
            if (m & 1u) {
                dt_clr(idx, l);                       /* (its extras and locks with it) */
                er_flash = 1;
            }
    } else {
        step_t *s = &t->step[idx];
        uint32_t i, k = 0, lv = 0, rt = 0;
        if (s->time != ST_NOTE)
            return;
        for (i = 0; i < s->n; i++)
            if (!er_has(s->note[i])) {
                lv |= ((s->lvl >> (2u * i)) & 3u) << (2u * k);
                rt |= ((s->rat >> (2u * i)) & 3u) << (2u * k);
                s->note[k++] = s->note[i];
            }
        if (k == s->n)
            return;
        er_flash = 1;
        s->n = (uint8_t)k;
        s->lvl = (uint8_t)lv;
        s->rat = (uint8_t)rt;
        if (!k) {
            s->time = ST_REST;
            s->flags = 0;
            s->vel = 0;
        }
    }
}
static void erase_now(track_t *t)             /* a key just went down */
{
    uint32_t i, len = trk_len(t);
    undo_mark(t, undo_erase_sess);
    if (song.playing && t->seq_abs != SEQ_NONE) {
        erase_step(t, t->seq_idx % len);
    } else {
        for (i = 0; i < len; i++)
            erase_step(t, i);
    }
}
static int erasing(const track_t *t) { return trk_index(t) == er_trk && (er_lanes || er_notes[0] || er_notes[1] || er_notes[2] || er_notes[3]); }

/* ------------------------------------------------------------- arp --- */
/* 2.6 (after Felucca 1.5, #191): where each track's held[i] came from, so the arp maps it again as it plays (arp_next:
 * TRN, ROOT, SCALE, QNT as they are now). held_key: the key + 1 (bits 0..4) and the octave buttons then + 3 (bits
 * 5..7); 0: not a key of the track's scale (MIDI IN, the drum track), held[i] plays as it is.
 * held_dt: held[i] above the key's own note (a chord's other notes). in_key: the key (+ 1) whose notes go into
 * input_on now (key_down, chord_revoice), 0: none */
static uint8_t held_key[NTRK][16];
static int8_t held_dt[NTRK][16];
static uint8_t in_key;
static void arp_add(track_t *t, uint32_t note)
{
    uint32_t i;
    if (t->p[P_AHOLD] && t->arp_phys == 0u)
        t->nheld = 0;                               /* new chord replaces the latched one */
    t->arp_phys++;                                  /* every key-down: arp_remove counts every key-up */
    for (i = 0; i < t->nheld; i++)
        if (t->held[i] == note)
            return;                                 /* repeated note-on: not a new note */
    if (t->nheld < 16u) {
        uint32_t k = in_key - 1u, ti = trk_index(t);
        held_key[ti][t->nheld] = in_key && kb_scaled(t) ? (uint8_t)(in_key | (uint32_t)(kb_oct[k % 27u] + 3) << 5) : 0u;   /* (the
                                                       * key's octave: a CHORD+ note added after OCT moved, too) */
        held_dt[ti][t->nheld] = (int8_t)(held_key[ti][t->nheld] ? (int32_t)note - kb_root[k % 27u] : 0);
        t->held[t->nheld++] = (uint8_t)note;
    }
    if (t->nheld == 1u) {
        t->arp_new = 1;                             /* the first note: now (or on the grid just ahead) */
        t->arp_idx = 0xFFFFFFFFu;
    }
}

static void arp_remove(track_t *t, uint32_t note)
{
    uint32_t i, k = 0;
    if (t->arp_phys)
        t->arp_phys--;
    if (t->p[P_AHOLD])
        return;
    for (i = 0; i < t->nheld; i++)
        if (t->held[i] != note) {
            held_key[trk_index(t)][k] = held_key[trk_index(t)][i];
            held_dt[trk_index(t)][k] = held_dt[trk_index(t)][i];
            t->held[k++] = t->held[i];
        }
    t->nheld = (uint8_t)k;
}

/* the next note of the arp: the held notes (sorted or as played) over OCT octaves, by MODE */
static uint32_t arp_next(track_t *t)
{
    uint32_t cnt = 0, list[64], len = 0, i, j, o, ti = trk_index(t);
    for (i = 0; i < t->nheld; i++) {               /* the notes as they play now (#191), each once */
        uint32_t x = t->held[i], s = held_key[ti][i];
        if (s) {                                    /* a key: through the scale as it is now */
            x = kb_pitch(t, (s & 31u) - 1u, (int32_t)(s >> 5) - 3);
            if (x == KB_SILENT)
                continue;                           /* (silent now: a black key under WHITE) */
            x = (uint32_t)clamp((int32_t)x + held_dt[ti][i], 0, 127);
        }
        for (j = 0; j < cnt && list[j] != x; j++)
            ;
        if (j == cnt)
            list[cnt++] = x;
    }
    if (!cnt)
        return KB_SILENT;                           /* (every key silent now: a black key under WHITE since) */
    if (!t->p[P_AORDER])
        for (i = 1; i < cnt; i++)
            for (j = i; j > 0 && list[j - 1] > list[j]; j--) {
                uint32_t x = list[j];
                list[j] = list[j - 1];
                list[j - 1] = x;
            }
    for (o = 0; o < (uint32_t)t->p[P_AOCT]; o++)
        for (i = 0; i < cnt && len < 64u; i++)
            list[len++] = (uint32_t)clamp((int32_t)list[i] + 12 * (int32_t)o, 0, 127);
    t->arp_idx++;
    switch (t->p[P_AMODE]) {
    case 2:
        j = len - 1u - t->arp_idx % len;
        break;
    case 3: {
        uint32_t cyc = len > 1u ? 2u * len - 2u : 1u, k = t->arp_idx % cyc;
        j = k < len ? k : cyc - k;
        break;
    }
    case 4:
        j = rng() % len;
        break;
    default:
        j = t->arp_idx % len;
        break;
    }
    return list[j];
}

/* the arp, once per block: on the transport's grid of RATE (with its SWING) while playing, from the
 * first key while stopped. A new chord starts at once unless the grid is just ahead. While recording,
 * each note it plays is recorded (what you hear) */
static void arp_tick(track_t *t, uint32_t adv)
{
    uint32_t div = (uint32_t)t->p[P_ARATE] % NDIV_SHORT, u = div_units(div), into, slen, abs = 0, fire = 0;
    if (t->arp_note) {
        if (t->arp_off <= adv) {
            trk_note_off(t, t->arp_note);
            seq_out_off(t, t->arp_note);
            t->arp_note = 0;
        } else {
            t->arp_off -= adv;
        }
    }
    if (!t->p[P_AMODE] || !t->nheld) {
        if (!t->nheld && t->arp_note) {
            trk_note_off(t, t->arp_note);
            seq_out_off(t, t->arp_note);
            t->arp_note = 0;
        }
        t->arp_new = 0;
        return;
    }
    if (song.playing) {
        abs = grid_at(div, swings(div) ? swing_units(t->p[P_ASWING], u) : 0u, &into, &slen);
        if (t->arp_new) {
            t->arp_new = 0;
            t->arp_abs = into * 4u >= slen * 3u ? abs : abs - 1u;   /* the last quarter: the grid plays it */
        } else if (div != t->arp_den) {
            t->arp_abs = abs;                       /* RATE changed: the next step of the new grid plays */
        }
        t->arp_den = (uint8_t)div;
        if (abs + 1u == t->arp_abs)
            abs = t->arp_abs;                       /* ARP SWG turned up inside an odd step: no replay */
        fire = abs != t->arp_abs;
        t->arp_abs = abs;
    } else {
        if (t->arp_new) {
            t->arp_new = 0;
            t->arp_pos = u;
        }
        slen = u;
        t->arp_pos += adv;
        if (t->arp_pos >= u) {
            t->arp_pos = t->arp_pos - u < u ? t->arp_pos - u : 0;
            fire = 1;
        }
    }
    if (!fire)
        return;
    if (t->arp_note) {
        trk_note_off(t, t->arp_note);
        seq_out_off(t, t->arp_note);
    }
    t->arp_note = 0;
    if ((uint32_t)(rng() & 127u) <= (uint32_t)t->p[P_APROB]) {
        uint32_t n = arp_next(t);
        if (n == KB_SILENT)
            return;
        t->arp_note = (uint8_t)n;
        t->arp_off = slen * (uint32_t)t->p[P_AGATE] / 128u;
        trk_note_on(t, n, 100);
        seq_out_on(t, n, 100);
        if (((song.rec >> trk_index(t)) & 1u) && song.playing)
            rec_note(t, n, 100, 0, 0);
    }
}

/* ------------------------------------------------------- note input --- */
/* armed and stopped, a note on the selected track: the note is the downbeat (the transport starts, recording on) */
static void arm_start(track_t *t)
{
    if (!rec_wait || t != TSEL || song.playing || ci_on)
        return;
    if (rec_count) {
        return;                                   /* COUNT: PLAY counts in; a note only sounds */
    } else {
        seq_start();                              /* the note is the downbeat */
        if (song.playing)
            rec_begin();
    }
}

static void drum_input(uint32_t lane, uint32_t vel, uint32_t rat, int rec);
static const uint8_t *in_chord;                    /* input_on's note is note in_chord_i of in_chord (CHORD+) */
static uint32_t in_chord_n, in_chord_i;
/* SLOOP 2.6: the sustain pedal (MIDI CC 64, on the track its channel plays): while it is down, a synth track's notes let
 * go keep sounding (their note-off waits: the arp keeps them, a recording holds them) until it is up; a note played
 * again meanwhile is held by its key again. Per track: the pedal, the notes waiting */
static uint8_t sus_on[NTRK];
static uint32_t sus_wait[NTRK][4];
static void input_off_now(track_t *t, uint32_t note);
static void input_on(track_t *t, uint32_t note, uint32_t vel)
{
    if (is_drum(t)) {                             /* (a GM note on the drum track: its lane) */
        drum_input(lane_of_note(note), vel, 0, 1);   /* (PHEBUS wave 2: at its velocity, recorded so) */
        return;
    }
    last_note = (uint8_t)note;
    {   /* (2.6) played again under the pedal: the note-off it waited with first (the arp's key count, a recording's
         * note end), then this one held by its key */
        uint32_t *w = &sus_wait[trk_index(t) % NTRK][(note >> 5) & 3u], bit = 1u << (note & 31u);
        if (*w & bit) {
            *w &= ~bit;
            input_off_now(t, note);
        }
    }
    arm_start(t);
    if (t->p[P_AMODE]) {
        arp_add(t, note);                         /* (the arp records the notes it plays) */
        return;
    }
    if (((song.rec >> trk_index(t)) & 1u) && song.playing)
        rec_note(t, note, vel, 0, 1);
    if (in_chord)                                 /* a chord from the keys: maybe strummed (voice.c) */
        trk_note_chord(t, in_chord, in_chord_n, in_chord_i, vel);
    else
        trk_note_on(t, note, vel);
}

static void input_off(track_t *t, uint32_t note)
{
    if (is_drum(t))
        return;
    if (sus_on[trk_index(t) % NTRK]) {               /* the pedal down: it waits for the pedal (2.6) */
        sus_wait[trk_index(t) % NTRK][(note >> 5) & 3u] |= 1u << (note & 31u);
        return;
    }
    input_off_now(t, note);
}
static void input_off_now(track_t *t, uint32_t note)
{
    rec_release(t, note);
    arp_remove(t, note);                            /* both: the note may have started in the */
    trk_note_off(t, note);                          /* other mode (ARP switched while held) */
}

/* a drum hit from a key, MIDI or a roll: lane, velocity; rat: its retrig when recorded (rolls: hits in the step - 1);
 * rec: it may be recorded (a roll records one hit a step) */
static void drum_input_x(uint32_t lane, uint32_t vel, uint32_t rat, int rec, int chr, int32_t xs);
static void drum_input(uint32_t lane, uint32_t vel, uint32_t rat, int rec) { drum_input_x(lane, vel, rat, rec, 0, 0); }
/* .. chr: from chromatic mode, xs semitones from the sound's pitch: heard so, and recorded as the trig's pitch lock (PHEBUS
 * wave 3, the Rytm's CHROMATIC mode records the note of each trig) */
static void drum_input_x(uint32_t lane, uint32_t vel, uint32_t rat, int rec, int chr, int32_t xs)
{
    track_t *t = TDRUM;
    lane &= 15u;
    vel = vel < 1u ? 1u : vel > 127u ? 127u : vel;
    arm_start(t);
    if (rec && ((song.rec >> TRK_DRUM) & 1u) && song.playing) {
        uint32_t i = rec_hit(t, lane, vel, rat);
        if (chr && xs) {
            if (!dt_lk_set(i, lane, DSP_NOTE, (uint32_t)(uint8_t)(int8_t)xs))
                dlk_full = 1;
        } else if (chr) {
            dt_lk_del(i, lane, DSP_NOTE);            /* (the sound's own pitch: no lock) */
        }
    }
    if (chr && xs) {
        drum_lk_n = 1;
        drum_lk_p[0] = DSP_NOTE;
        drum_lk_v[0] = (uint8_t)(int8_t)xs;
    }
    trk_note_on(t, LANE_NOTE[lane], vel);
    drum_lk_n = 0;
}

/* -------------------------------------------------------------- roll --- */
/* ARP + key: note repeat. The key plays at G_ROLL (1/8, 1/16, 1/32, 32T, 1/64) on the transport's grid
 * (stopped: from the press) until it or ARP is up. Recorded, a roll faster than the track's steps
 * becomes ratchets (1/32 on 1/16 steps: x2, 32T: x3, 1/64: x4). */
static const uint8_t ROLL_DEN[5] = {2, 4, 8, 12, 16};
#define NROLL 4u
static struct {
    uint8_t on, key, trk, note, lvl;    /* note: the synth note, or the lane */
    uint32_t last;                      /* playing: the roll step last played; stopped: units since */
    uint32_t off;                       /* synth: units to its note-off, 0 = not sounding */
    uint32_t rec_abs;                   /* the step its last recorded hit went into */
} roll[NROLL];
static uint32_t roll_den(void) { return ROLL_DEN[(uint32_t)clamp(song.g[G_ROLL], 0, 4)]; }
/* (PHEBUS wave 3, the Rytm's live RETRIG: hold RETRIG + a pad) on the drum track ARP + a key retrigs the sound at
 * drt_live, one of the per-trig retrig's rates (RT_DEN below: 1/1 .. 1/80 of a bar; the black keys or KNOB 1 pick it,
 * the UI), not G_ROLL's. A roll's unit and its grid: */
static volatile uint8_t drt_live = 10;          /* (RT_DEN's index: 10 = 1/16) */
static uint32_t rt_units(uint32_t r);
static uint32_t roll_unit(uint32_t ti) { return ti == TRK_DRUM ? rt_units(drt_live) : BEAT_U / roll_den(); }
static uint32_t roll_grid(uint32_t ti, uint32_t *into, uint32_t *len)
{
    uint64_t a;
    uint32_t u;
    if (ti != TRK_DRUM)
        return grid_den(roll_den(), 0, into, len);
    u = roll_unit(ti);
    a = (uint64_t)clk_beat * BEAT_U + clk_pos;
    *into = (uint32_t)(a % u);
    *len = u;
    return (uint32_t)(a / u);
}
/* the lanes / notes a roll plays live on track t: the pattern does not play them meanwhile (no flam
 * with what was recorded, nor with what this roll is recording) */
static uint32_t roll_lanes(const track_t *t)
{
    uint32_t r, m = 0;
    for (r = 0; r < NROLL; r++)
        if (roll[r].on && roll[r].trk == trk_index(t))
            m |= 1u << (roll[r].note & 15u);
    return m;
}
static int roll_has(const track_t *t, uint32_t note)
{
    uint32_t r;
    for (r = 0; r < NROLL; r++)
        if (roll[r].on && roll[r].trk == trk_index(t) && roll[r].note == note)
            return 1;
    return 0;
}

static void roll_hit(uint32_t r)
{
    track_t *t = &trk[roll[r].trk % NTRK];
    uint32_t u = roll_unit(roll[r].trk), su = div_units((uint32_t)t->p[P_SDIV]);
    uint32_t hits = su / u, rat = hits > 4u ? 3u : hits > 1u ? hits - 1u : 0u;
    if (is_drum(t))                                  /* (a retrig faster than a step: recorded at its own rate) */
        rat = hits > 1u ? 0x80u | drt_live : 0u;
    uint32_t armed = ((song.rec >> trk_index(t)) & 1u) && song.playing, later, abs = 0, rec = 0;
    if (armed) {                                     /* one recorded hit a step: the ratchet does the rest */
        abs = rec_target(t, &later);
        rec = abs != roll[r].rec_abs;
        roll[r].rec_abs = abs;
    }
    if (is_drum(t)) {
        drum_input(roll[r].note, lvl_vel(roll[r].lvl, 100), rat, rec || !armed);
        seq_out_on(t, LANE_NOTE[roll[r].note & 15u], lvl_vel(roll[r].lvl, 100));
        return;
    }
    arm_start(t);
    if (roll[r].off)
        trk_note_off(t, roll[r].note);
    trk_note_on(t, roll[r].note, lvl_vel(roll[r].lvl, 100));
    seq_out_on(t, roll[r].note, lvl_vel(roll[r].lvl, 100));
    roll[r].off = u / 2u;
    if (rec)
        rec_note(t, roll[r].note, 100, rat, 0);
}

static void roll_start(uint32_t k, track_t *t, uint32_t note, uint32_t lvl)
{
    uint32_t r, into, slen, abs;
    for (r = 0; r < NROLL && roll[r].on; r++)
        ;
    if (r == NROLL)
        return;
    roll[r].on = 1;
    roll[r].key = (uint8_t)k;
    roll[r].trk = (uint8_t)trk_index(t);
    roll[r].note = (uint8_t)note;
    roll[r].lvl = (uint8_t)lvl;
    roll[r].off = 0;
    roll[r].rec_abs = SEQ_NONE;
    if (song.playing) {
        abs = roll_grid(roll[r].trk, &into, &slen);
        roll[r].last = abs;
        if (into * 4u >= slen * 3u)
            return;                                  /* the grid is just ahead: it starts there */
    } else {
        roll[r].last = 0;
    }
    roll_hit(r);
}

static void roll_end(uint32_t r)
{
    if (roll[r].on && roll[r].off && roll[r].trk != TRK_DRUM) {
        trk_note_off(&trk[roll[r].trk % NTRK], roll[r].note);
        seq_out_off(&trk[roll[r].trk % NTRK], roll[r].note);
    }
    if (roll[r].on && roll[r].trk == TRK_DRUM)
        seq_out_off(&trk[TRK_DRUM], LANE_NOTE[roll[r].note & 15u]);
    roll[r].on = 0;
}

static void roll_block(uint32_t adv)
{
    uint32_t r, u, into, slen;
    for (r = 0; r < NROLL; r++) {
        if (!roll[r].on)
            continue;
        u = roll_unit(roll[r].trk);
        if (roll[r].off) {
            if (roll[r].off <= adv) {
                trk_note_off(&trk[roll[r].trk % NTRK], roll[r].note);
                seq_out_off(&trk[roll[r].trk % NTRK], roll[r].note);
                roll[r].off = 0;
            } else {
                roll[r].off -= adv;
            }
        }
        if (song.playing) {
            uint32_t abs = roll_grid(roll[r].trk, &into, &slen);
            if (abs != roll[r].last) {
                roll[r].last = abs;
                roll_hit(r);
            }
        } else {
            roll[r].last += adv;
            if (roll[r].last >= u) {
                roll[r].last = roll[r].last - u < u ? roll[r].last - u : 0;
                roll_hit(r);
            }
        }
    }
}

/* ---------------------------------------------------------- keyboard --- */
/* the level of a key on the drum track: OCT- held ghost, OCT+ held hard */
static uint32_t key_lvl(void)
{
    uint32_t b = fm1_in.buttons;
    return (b & dyn_bit[0]) ? LV_GHOST : (b & dyn_bit[1]) ? LV_HARD : LV_NORM;
}

/* CHORD+: the modifiers held (the black keys down in chord mode) */
static uint32_t chord_mods(void)
{
    uint32_t k, m = 0;
    for (k = 0; k < 27u; k++)
        if (kb_kind[k] == KS_MOD)
            m |= chord_mod_of_key(k);
    return m;
}
/* a modifier went down or up: every chord held on part sel changes under the finger (the notes it loses end,
 * the ones it gains start; the ones it keeps ring on) */
static void chord_revoice(uint32_t sel)
{
    uint32_t k, i, j, mods = chord_mods(), mc = trk_midi_ch(sel);
    track_t *t = &trk[sel % NTRK];
    for (k = 0; k < 27u; k++) {
        uint8_t nw[4];
        uint32_t nn;
        if (kb_kind[k] != KS_NOTE || kb_trk[k] != sel || !t->p[P_CHORD] || is_drum(t))
            continue;
        nn = chord_play_notes(t, kb_root[k], mods, nw);
        for (i = 0; i < kb_n[k]; i++) {             /* the notes it loses */
            for (j = 0; j < nn && nw[j] != kb_nt[k][i]; j++)
                ;
            if (j == nn) {
                input_off(t, kb_nt[k][i]);
                midi_out_note(mc, (uint32_t)kb_nt[k][i], 0);
            }
        }
        for (j = 0; j < nn; j++) {                  /* the notes it gains */
            for (i = 0; i < kb_n[k] && kb_nt[k][i] != nw[j]; i++)
                ;
            if (i == kb_n[k]) {
                in_key = (uint8_t)(k + 1u);         /* (a chord's new note: of this key, #191) */
                input_on(t, nw[j], 100);
                in_key = 0;
                midi_out_note(mc, (uint32_t)nw[j], 100u);
            }
        }
        memcpy(kb_nt[k], nw, nn);
        kb_n[k] = (uint8_t)nn;
    }
}

static void key_down(uint32_t k)
{
    uint32_t layer = layer_now(), sel = song.sel % NTRK, i, mc;
    track_t *t = &trk[sel];
    kb_kind[k] = KS_NONE;
    kb_trk[k] = (uint8_t)sel;
    kb_n[k] = 0;
    switch (layer) {
    case LY_FX: {                                     /* FX held: the keys pick a punch-in effect (white and black) */
        int32_t fx = punch_fx_of_key(k);
        kb_kind[k] = KS_FX;
        if (fx >= 0 && fx < (int32_t)PUNCH_NFX) {
            punch.req = (int8_t)fx;
            punch.keybit = 1u << k;
        }
        return;
    }
    case LY_STEP:
    case LY_SCALE:
    case LY_MIX:
    case LY_SONG:                                     /* the UI's: steps, the key, the mix, the sections */
        kb_kind[k] = KS_UI;
        kb_nt[k][0] = (uint8_t)layer;                 /* (its key-up goes to the same layer) */
        lk_push(layer, k, 1);
        return;
    case LY_ERASE:
        if (is_drum(t) && punch_key(k) < 0) {        /* (2.6) a black key: erase <-> mute */
            er_mute = (uint8_t)!er_mute;
            return;
        }
        if (is_drum(t) && er_mute) {                  /* MUTE: the white key's sound off / on in the sequencer */
            drum_mute ^= (uint16_t)(1u << lane_of_key(k));
            return;
        }
        kb_kind[k] = KS_ERASE;
        if (!erasing(t) || er_trk != sel)
            undo_erase_sess = (undo_sess += 4u) | 2u;  /* a new erase: one undo */
        if (er_trk != sel) {
            er_lanes = 0;
            er_notes[0] = er_notes[1] = er_notes[2] = er_notes[3] = 0;
            er_trk = (uint8_t)sel;
        }
        if (is_drum(t)) {
            kb_nt[k][0] = (uint8_t)lane_of_key(k);
            kb_n[k] = 1;
            er_lanes |= (uint16_t)(1u << kb_nt[k][0]);
        } else {
            uint32_t n = kb_map(t, k);
            if (n == KB_SILENT)
                return;
            kb_n[k] = (uint8_t)(t->p[P_CHORD] ? chord_notes(t, n, kb_nt[k]) : 1u);
            if (!t->p[P_CHORD])
                kb_nt[k][0] = (uint8_t)n;
            for (i = 0; i < kb_n[k]; i++)
                er_notes[(kb_nt[k][i] >> 5) & 3u] |= 1u << (kb_nt[k][i] & 31u);
        }
        erase_now(t);
        return;
    default:
        break;
    }
    if (layer == LY_PLAY && kb_grid) {                /* the GRID (every track): a step key */
        kb_kind[k] = KS_UI;
        kb_nt[k][0] = (uint8_t)KB_GRID;
        lk_push(KB_GRID, k, 1);
        return;
    }
    if (is_drum(t)) {                                 /* the drum track: the key's lane */
        uint32_t lane = lane_of_key(k), lvl = key_lvl();
        kb_nt[k][0] = (uint8_t)lane;
        kb_n[k] = 1;
        if (layer == LY_ROLL) {
            if (punch_key(k) < 0) {                   /* (PHEBUS wave 3) RETRIG: a black key picks the rate */
                static const uint8_t RATE_OF_BLACK[11] = {4, 6, 7, 9, 10, 12, 13, 14, 15, 16, 17};   /* 1/4 .. 1/80 */
                uint32_t b;
                for (b = 0; b < 11u && PUNCH_BLACK[b] != k; b++)
                    ;
                if (b < 11u)
                    drt_live = RATE_OF_BLACK[b];
                return;
            }
            kb_kind[k] = KS_ROLL;
            roll_start(k, t, lane, lvl);
            return;
        }
        if (layer == LY_PLAY && drum_keys == DK_MUTE) {   /* (wave 3) MUTE mode: a white key mutes / unmutes its sound */
            if (punch_key(k) >= 0)
                drum_mute ^= (uint16_t)(1u << lane);
            return;
        }
        if (layer == LY_PLAY && drum_keys == DK_CHROM) {  /* (wave 3) CHROMATIC: the selected sound at the key's pitch */
            int32_t xs = (int32_t)k - (int32_t)CHR_KEY0 + 12 * drum_chr_oct;
            lane = drum_lane & 15u;
            kb_nt[k][0] = (uint8_t)lane;
            kb_kind[k] = KS_DRUM;
            drum_input_x(lane, 100, 0, 1, 1, xs);
            midi_out_note(trk_midi_ch(sel), (uint32_t)LANE_NOTE[lane], 100u);
            return;
        }
        kb_kind[k] = KS_DRUM;
        drum_input(lane, lvl_vel(lvl, 100), 0, 1);
        mc = trk_midi_ch(sel);
        midi_out_note(mc, (uint32_t)LANE_NOTE[lane], lvl_vel(lvl, 100));
        return;
    }
    {
        uint32_t n = kb_map(t, k);
        if (n == KB_SILENT) {
            if (t->p[P_CHORD] && layer == LY_PLAY && chord_mod_of_key(k)) {   /* CHORD+: a modifier key */
                kb_kind[k] = KS_MOD;
                chord_revoice(sel);
            }
            return;
        }
        if (layer == LY_ROLL) {
            kb_kind[k] = KS_ROLL;
            kb_nt[k][0] = (uint8_t)n;
            kb_n[k] = 1;
            roll_start(k, t, n, LV_NORM);
            return;
        }
        kb_kind[k] = KS_NOTE;
        kb_root[k] = (uint8_t)n;
        kb_oct[k] = (int8_t)song.octave;
        if (t->p[P_CHORD]) {
            kb_n[k] = (uint8_t)chord_play_notes(t, n, chord_mods(), kb_nt[k]);   /* (the modifiers held, voiced) */
        } else {
            kb_nt[k][0] = (uint8_t)n;
            kb_n[k] = 1;
        }
        mc = trk_midi_ch(sel);
        in_key = (uint8_t)(k + 1u);                 /* (the arp keeps the key: arp_add, #191) */
        for (i = 0; i < kb_n[k]; i++) {
            in_chord = kb_n[k] > 1u ? kb_nt[k] : 0;
            in_chord_n = kb_n[k];
            in_chord_i = i;
            input_on(t, kb_nt[k][i], 100);
            in_chord = 0;
            midi_out_note(mc, (uint32_t)kb_nt[k][i], 100u);
            if (sr_on)                              /* (STEP REC: the UI writes it, ui_grid.c sr_event) */
                step_in_push(kb_nt[k][i] | 0x80u | sel << 8);
        }
        in_key = 0;
        /* the pen of the SEQ layer: the keys down now (a chord), else this note */
        if (!(kb_prev & ~(1u << k)) || pen_n >= 4u)
            pen_n = 0;
        for (i = 0; i < kb_n[k] && pen_n < 4u; i++)
            pen_note[pen_n++] = kb_nt[k][i];
    }
}

static void key_up(uint32_t k)
{
    uint32_t i, mc, kind = kb_kind[k];
    track_t *t = &trk[kb_trk[k] % NTRK];
    kb_kind[k] = KS_NONE;
    switch (kind) {
    case KS_FX:
        if (punch.keybit == 1u << k) {                /* its key is up: the mix comes back */
            punch.keybit = 0;
            punch.req = -1;
        }
        return;
    case KS_UI:
        lk_push(kb_nt[k][0], k, 0);
        return;
    case KS_MOD:                                    /* a CHORD+ modifier let go: the chords held change back */
        chord_revoice(kb_trk[k]);
        return;
    case KS_ERASE:
        if (is_drum(t))
            er_lanes &= (uint16_t)~(1u << kb_nt[k][0]);
        else
            for (i = 0; i < kb_n[k]; i++)
                er_notes[(kb_nt[k][i] >> 5) & 3u] &= ~(1u << (kb_nt[k][i] & 31u));
        return;
    case KS_ROLL:
        for (i = 0; i < NROLL; i++)
            if (roll[i].on && roll[i].key == k)
                roll_end(i);
        return;
    case KS_NOTE:
        mc = trk_midi_ch(kb_trk[k] % NTRK);
        for (i = 0; i < kb_n[k]; i++) {
            input_off(t, kb_nt[k][i]);
            midi_out_note(mc, (uint32_t)kb_nt[k][i], 0);
            if (sr_on)
                step_in_push(kb_nt[k][i] | (uint32_t)kb_trk[k] << 8);
        }
        return;
    case KS_DRUM:
        mc = trk_midi_ch(TRK_DRUM);
        midi_out_note(mc, (uint32_t)LANE_NOTE[kb_nt[k][0] & 15u], 0);
        return;
    default:
        return;
    }
}

/* the UI asks to hear drum sounds (a sound or a step picked with a knob, a step set from a key):
 * aud_lanes the lanes, played here, in the audio context: at their trigs' velocities on step aud_step (as they are
 * then), or at 100 (aud_step NSTEP) */
static volatile uint32_t aud_lanes, aud_step;
static void audition_req(uint32_t lanes, uint32_t step)
{
    fm1_irq_off();
    aud_step = step;
    aud_lanes = lanes & 0xFFFFu;
    fm1_irq_on();
}
static void audition_lane(uint32_t lane) { audition_req(1u << (lane & 15u), NSTEP); }
static void audition_step(uint32_t i) { audition_req(dt_mask(i % NSTEP), i % NSTEP); }   /* every sound of a drum step */
/* the UI asks to hear a synth step (the GRID: a step set, its NOTE turned; REVIEW.md 1, 21 Nov 2026): its notes on
 * track aud_syn_trk, played here and let go AUD_SYN_BLOCKS blocks (~150 ms) later. Only the voices: nothing recorded,
 * nothing to the arp or MIDI out. A new one ends the one before */
#define AUD_SYN_BLOCKS 200u
static volatile uint8_t aud_syn_trk = 0xFF, aud_syn_n, aud_syn_nt[4];
static uint8_t aud_on_trk, aud_on_n, aud_on_nt[4];
static uint16_t aud_on_left;
static void audition_notes(uint32_t ti, const uint8_t *nt, uint32_t n)
{
    uint32_t i;
    fm1_irq_off();
    for (i = 0; i < n && i < 4u; i++)
        aud_syn_nt[i] = nt[i];
    aud_syn_n = (uint8_t)i;
    aud_syn_trk = (uint8_t)ti;
    fm1_irq_on();
}
static void audition_syn_block(void)
{
    uint32_t i;
    if (aud_on_left && (--aud_on_left == 0u || aud_syn_trk < NPART))
        for (i = 0; i < aud_on_n; i++)
            trk_note_off(&trk[aud_on_trk % NTRK], aud_on_nt[i]);
    if (aud_syn_trk >= NPART)
        return;
    aud_on_trk = aud_syn_trk;
    aud_on_n = trk[aud_on_trk].p[P_VOICE] == V_POLY ? aud_syn_n : (aud_syn_n ? 1u : 0u);
    for (i = 0; i < aud_on_n; i++) {
        aud_on_nt[i] = aud_syn_nt[i];
        trk_note_on(&trk[aud_on_trk], aud_on_nt[i], 100);
    }
    aud_on_left = (uint16_t)AUD_SYN_BLOCKS;
    aud_syn_trk = 0xFF;
}
static void audition_block(void)
{
    uint32_t m = aud_lanes, st = aud_step, l;
    audition_syn_block();
    if (!m)
        return;
    aud_lanes = 0;
    for (l = 0; m; l++, m >>= 1)
        if (m & 1u)
            trk_note_on(TDRUM, LANE_NOTE[l], st < NSTEP && dt_vel(st, l) ? dt_vel(st, l) : 100u);
}

static void keyboard_block(void)
{
    uint32_t cur = fm1_in.notes, ch = cur ^ kb_prev, k, r;
    audition_block();
    if (er_mute && layer_now() != LY_ERASE)
        er_mute = 0;                                  /* EDIT let go: its keys erase again (2.6) */
    if (!(layer_buttons() & ly_bit[LY_ROLL]))         /* ARP up (and not locked): the rolls end (the keys stay silent) */
        for (r = 0; r < NROLL; r++)
            if (roll[r].on)
                roll_end(r);
    if (!ch)
        return;
    for (k = 0; k < 27u; k++) {
        if (!((ch >> k) & 1u))
            continue;
        if ((cur >> k) & 1u)
            key_down(k);
        else
            key_up(k);
        kb_prev ^= 1u << k;                           /* (key_down sees the keys down before it) */
    }
}

/* LIVE: one record arm at most, on the selected track; selecting another track moves it there
 * (an arm follows too: it waits for a note on the track selected) */
static void rec_follow(uint32_t sel)
{
    if (song.rec)
        song.rec = (uint8_t)(1u << (sel % NTRK));
}

/* -------------------------------------------------------- metronome --- */
/* A wood block on every beat, louder on the first of the bar, from the drum kit's voices
 * (not recorded, not muted with the drum track; GLO > DRUMS LVL sets its level; drums.c click_on: its own voice).
 * GLO > GLOBAL CLICK: OFF (0, default: LIVE needs none), REC = while a track records,
 * ON = while playing. */
#define CLICK_REC 1
#define CLICK_ON 2
static uint32_t click_last = SEQ_NONE;          /* the beat it last played */
static void click_tick(void)
{
    uint32_t mode = (uint32_t)song.g[G_CLOCK];
    if (!song.playing || clk_beat == click_last)
        return;
    click_last = clk_beat;
    if (mode == CLICK_ON || (mode == CLICK_REC && song.rec))
        click_on(clk_beat % 4u == 0u);
}

/* -------------------------------------------------------- sequencer --- */
#if FELUCCA_ARRANGER
/* ---- LIVE SECTIONS (SAVE held + key, ui_layers.c): a section asked for while playing starts on the
 * next bar, every track from its step 0 */
static uint32_t arrangement_ready(void);           /* project.c (arranger_scene.c): the sections stored, a bit each */
static void arrangement_apply(uint32_t scene);     /* .. one into the tracks */
static volatile int8_t live_req = -1;              /* section asked for (UI), applied on the next bar */
static volatile int8_t live_sec = -1;              /* the section playing: last jumped to, loaded or stored */
static volatile int8_t sec_fresh = -1;             /* (3.0.5, issue #3) the part just stored (SAVE + 5..8): edits do
                                                    * not stick to it until a part is picked or played again */
static uint32_t live_bar = 0xFFFFFFFFu;            /* clk_beat / 4 of the last bar seen */
static void seq_reset_tracks(uint32_t pos);
static void live_block(void)                       /* once a block while playing */
{
    if ((clk_beat & 3u) || (clk_beat >> 2) == live_bar)
        return;
    live_bar = clk_beat >> 2;                       /* a new bar */
    if (live_req >= 0) {
        uint32_t s = (uint32_t)live_req;
        live_req = -1;
        {                                           /* (the UI asked for a section it checked: no hash here) */
            arrangement_apply(s);
            song.rec = 0;                           /* (a take does not run on into another section) */
            live_sec = (int8_t)s;
            sec_fresh = -1;                         /* (a part picked) */
            seq_reset_tracks(clk_pos);              /* on the bar: every track from its step 0 */
            live_bar = 0;
        }
    }
}
#endif

/* every track from its step 0, together, at pos units into beat 0 (a section: on its bar) */
static void seq_reset_tracks(uint32_t pos)
{
    uint32_t i;
    for (i = 0; i < NTRK; i++) {
        track_t *t = &trk[i];
        t->seq_abs = SEQ_NONE;
        t->seq_idx = 0;
        t->rskip_n = 0;
        t->rskip_lanes = 0;
        t->rskip_abs = SEQ_NONE;
        t->rh_n = 0;
        t->arp_new = t->nheld != 0;
    }
    for (i = 0; i < NROLL; i++)
        roll[i].last = SEQ_NONE - 1u;               /* (a roll held over the start: on the grid from here) */
    drum_rt_clear();
    clk_beat = 0;
    clk_pos = pos;
#if FELUCCA_ARRANGER
    live_bar = 0xFFFFFFFFu;                        /* (bar 0 is a new bar) */
#endif
    fill_last_bar = 0xFFFFFFFFu;                   /* (the same for an armed fill bar) */
    click_last = SEQ_NONE;
    song.tick = 0;
    song.playing = 1;
    slicer_start(pos);                             /* slicer.c: its step 0 with the sequencer's */
}

static void seq_start(void)
{
    if (!song.playing)
        guard_start();                             /* (3.1.5 test 11) the overload guard as STOP left it */
    seq_reset_tracks(0);
}

static void seq_release(track_t *t)
{
    uint32_t i;
    for (i = 0; i < t->seq_n; i++) {
        trk_note_off(t, t->seq_notes[i]);
        seq_out_off(t, t->seq_notes[i]);
    }
    if (is_drum(t))
        seq_out_track_off(t);                       /* the drum hits of the step: ended with it */
    t->seq_n = 0;
    t->seq_hold = 0;
    t->slide_glide = 0;                             /* live MONO / LEG keys must not glide after it */
}

static void seq_stop(void)
{
    uint32_t i;
#if FELUCCA_ARRANGER
    live_req = -1;
#endif
    if (song.playing)
        guard_stop();                               /* (3.1.5 test 11) how far the overload guard had trimmed */
    fill_held = 0;                                  /* STOP ends a fill, held, latched or armed */
    fill_lat = 0;
    fill_arm = 0;
    fill_bar_on = 0;
    song.playing = 0;
    for (i = 0; i < NTRK; i++) {
        seq_release(&trk[i]);
        locks_restore(&trk[i]);                    /* the parameters back to their base */
        trk[i].rh_n = 0;                           /* a recorded note held over the stop: as far as it got */
    }
    seq_out_all_off();
    drum_rt_clear();
}

/* the velocity of note i of synth step s */
static uint32_t step_vel(const step_t *s, uint32_t i)
{
    uint32_t base = (s->flags & SF_ACCENT) ? 127u : (s->vel ? s->vel : 96u);
    return lvl_vel((s->lvl >> (2u * i)) & 3u, base);
}

/* play one synth step: TIE extends, REST releases, NOTE (re)triggers; a SLIDE on the previous
 * step makes this one legato with a glide (acid style). skip: bit k = note k already sounds
 * from live recording (not triggered, not released here). len: the step's length (units). */
static void seq_step(track_t *t, const step_t *s, uint32_t slen, uint32_t skip)
{
    uint32_t i, j, gate = slen * (uint32_t)t->p[P_SGATE] / 128u;
    uint32_t slide_in = t->seq_hold && t->seq_n;
    uint32_t next_tie = t->step[(t->seq_idx + 1u) % trk_len(t)].time == ST_TIE;
    if (s->time == ST_TIE) {
        if (t->seq_n) {
            t->seq_off = gate + slen / 2u;
            t->seq_hold = (s->flags & SF_SLIDE) != 0 || next_tie;   /* chains hold at any GATE / swing */
        }
        return;
    }
    if (s->time == ST_REST || !s->n) {
        seq_release(t);
        return;
    }
    if (s->rat) {                                   /* ratchets: each hit its share of the step */
        uint32_t hits = 1u + ((s->rat >> 0) & 3u);
        for (i = 1; i < s->n; i++)
            if (1u + ((s->rat >> (2u * i)) & 3u) > hits)
                hits = 1u + ((s->rat >> (2u * i)) & 3u);
        gate /= hits;
    }
    t->slide_glide = (uint8_t)slide_in;
    if (!slide_in)
        seq_release(t);
    for (i = 0; i < s->n; i++)
        if (roll_has(t, s->note[i]))
            skip |= 1u << i;                        /* (a roll plays it) */
    for (i = 0; i < s->n; i++)
        if (!((skip >> i) & 1u)) {
            if (slide_in)
                trk_note_on(t, s->note[i], step_vel(s, i));
            else                                    /* (STRUM: a chord's notes one after the other) */
                trk_note_chord(t, s->note, s->n, i, step_vel(s, i));
            if (!slide_in || !(mo_set[trk_index(t) % NTRK][s->note[i] >> 5] & (1u << (s->note[i] & 31u))))
                seq_out_on(t, s->note[i], step_vel(s, i));   /* (a slide into the same note: one MIDI note) */
        }
    if (slide_in)                                   /* release what is not held over */
        for (i = 0; i < t->seq_n; i++) {
            for (j = 0; j < s->n && s->note[j] != t->seq_notes[i]; j++)
                ;
            if (j == s->n) {
                trk_note_off(t, t->seq_notes[i]);
                seq_out_off(t, t->seq_notes[i]);
            }
        }
    t->seq_n = 0;
    for (i = 0; i < s->n; i++)
        if (!((skip >> i) & 1u))
            t->seq_notes[t->seq_n++] = s->note[i];
    t->seq_off = gate;
    t->seq_hold = !s->rat && ((s->flags & SF_SLIDE) != 0 || next_tie);   /* next step a TIE: keep the notes to it */
}

/* ---- the drum trigs as they play (PHEBUS wave 2, core.h dpat_t). On each step, each sound's trig: its condition
 * (CHNC: its own chance), then its hit at its velocity with its sound's locks (drums.c drum_lk_*), on the grid or nudged:
 * late, it waits in dq; early (before its step), it is queued when the step before it plays. Its retrig repeats it at
 * the retrig's rate over its length (drt), each hit its velocity through the curve, its locks again; the sound's next
 * trig ends it. Everything runs a block at a time, like the steps (a hit lands in the block it falls in) */
static const uint8_t RT_DEN[18] = {0, 1, 2, 3, 4, 5, 6, 8, 10, 12, 16, 20, 24, 32, 40, 48, 64, 80};   /* 1/1 .. 1/80 note */
#define NRTRG 17u
static uint32_t rt_units(uint32_t r) { return BEAT_U * 4u / RT_DEN[r < 1u ? 1u : r > NRTRG ? NRTRG : r]; }   /* (exact) */
#define DQ_N 32u
static struct { int32_t left; uint8_t step, lane; } dq[DQ_N];   /* nudged hits waiting: units from the block's start to
                                                                  * when each is due (as a step: it plays in the first block
                                                                  * that starts at or after that), the trig */
static uint8_t dq_n;
static uint32_t dq_ahead = SEQ_NONE;            /* the grid step whose early trigs are queued (dq_early: their sounds) */
static uint16_t dq_early;
#define RT_INF 0xFFFFFFFFu
/* a retrig as it runs: units since its trig (at the block's start), when its next hit is due, its rate and length */
static struct { uint32_t el, nx, per, len; uint8_t on, step, vel, now; int8_t fade; } drt[DRUM_LANES];   /* (now: it started
                                                                  * in this block: its hits from the next) */
static void drum_rt_clear(void)                 /* nothing waits, nothing retrigs (STOP, PLAY, a section, a load) */
{
    uint32_t l;
    dq_n = 0;
    dq_ahead = SEQ_NONE;
    dq_early = 0;
    for (l = 0; l < DRUM_LANES; l++)
        drt[l].on = 0;
}
/* a trig of hits x1..x4 in its step (an old ratchet, a roll recorded): a retrig of a step's length at the rate nearest a
 * step / hits on the drum track's DIV */
static void dt_retrig_hits(uint32_t i, uint32_t l, uint32_t hits)
{
    uint32_t want = div_units(trk_div(TDRUM)) / (hits ? hits : 1u), r, best = 1, bd = 0xFFFFFFFFu;
    dtx_t *x;
    for (r = 1; r <= NRTRG; r++) {
        uint32_t u = rt_units(r), d = u > want ? u - want : want - u;
        if (d < bd)
            bd = d, best = r;
    }
    if (hits > 1u && (x = dt_x_make(i, l)) != 0) {
        x->rrate = (uint8_t)best;
        x->rlen = DT_RLEN_DEF;
        x->rfade = 0;
    }
}
/* (PHEBUS wave 3) a trig recorded from a live retrig faster than its step: a retrig of one step at rate r (RT_DEN) */
static void dt_retrig_rate(uint32_t i, uint32_t l, uint32_t r)
{
    dtx_t *x;
    if (r >= 1u && r <= NRTRG && (x = dt_x_make(i, l)) != 0) {
        x->rrate = (uint8_t)r;
        x->rlen = DT_RLEN_DEF;
        x->rfade = 0;
    }
}
/* one hit of trig (i, l) at vel: its sound's locks, unless the sound is muted live (or another soloed) or rolling */
static void dhit(track_t *t, uint32_t i, uint32_t l, uint32_t vel)
{
    uint32_t k, n = 0;
    if (((roll_lanes(t) >> l) & 1u) || drum_silenced(l))
        return;
    for (k = 0; k < DT_NLK && n < DLK_MAX; k++)
        if (dpat.lk[k].step == i && dpat.lk[k].lane == l && dpat.lk[k].param < DSP_NIDS) {
            drum_lk_p[n] = dpat.lk[k].param;
            drum_lk_v[n++] = dpat.lk[k].val;
        }
    drum_lk_n = (uint8_t)n;
    trk_note_on(t, LANE_NOTE[l], vel < 1u ? 1u : vel);
    drum_lk_n = 0;
    seq_out_on(t, LANE_NOTE[l], vel < 1u ? 1u : vel);
}
/* a retrig's velocity at the hit units after its start (len: its length): the curve, fade out (< 0) to fade in (> 0) */
static uint32_t rt_vel(uint32_t vel, int32_t fade, uint32_t at, uint32_t len)
{
    int32_t p, v;
    if (!fade || len == RT_INF || !len)
        return vel;
    p = (int32_t)((uint64_t)(at < len ? at : len) * 1024u / len);   /* 0..1024 */
    v = fade < 0 ? (int32_t)vel * (131072 + fade * p) / 131072
                 : (int32_t)vel * (131072 - fade * (1024 - p)) / 131072;
    return (uint32_t)(v < 1 ? 1 : v > 127 ? 127 : v);
}
/* trig (i, l) plays now, late units after it was due (a step of slen units): its hit, its retrig started (the sound's
 * last one ends) */
static void dtrig(track_t *t, uint32_t i, uint32_t l, uint32_t slen, uint32_t late)
{
    int x = dt_x(i, l);
    uint32_t vel = dt_vel(i, l);
    drt[l].on = 0;
    if (!vel)
        return;                                     /* (cleared while it waited) */
    if (x >= 0 && dpat.x[x].rrate) {
        const dtx_t *e = &dpat.x[x];
        drt[l].per = rt_units(e->rrate);
        drt[l].len = e->rlen ? (uint32_t)((uint64_t)slen * e->rlen / 16u) : RT_INF;
        drt[l].el = late;
        drt[l].nx = drt[l].per;
        drt[l].vel = (uint8_t)vel;
        drt[l].fade = e->rfade;
        drt[l].step = (uint8_t)i;
        drt[l].on = 1;
        drt[l].now = 1;
        vel = rt_vel(vel, e->rfade, 0, drt[l].len);
    }
    dhit(t, i, l, vel);
}
/* a nudged hit of trig (i, l) due left units from this block's start: now when it is due already, else queued */
static void dq_push(track_t *t, int32_t left, uint32_t i, uint32_t l, uint32_t slen)
{
    if (left <= 0) {
        dtrig(t, i, l, slen, (uint32_t)-left);
    } else if (dq_n < DQ_N) {
        dq[dq_n].left = left;
        dq[dq_n].step = (uint8_t)i;
        dq[dq_n].lane = (uint8_t)l;
        dq_n++;
    }
}
/* once a block while playing (adv units since the last block's start), before a step fires: the nudged hits due */
static void drum_block(track_t *t, uint32_t adv, uint32_t slen)
{
    uint32_t k, l;
    for (k = 0; k < dq_n;) {
        dq[k].left -= (int32_t)adv;
        if (dq[k].left > 0) {
            k++;
            continue;
        }
        l = dq[k].lane;
        dtrig(t, dq[k].step, l, slen, (uint32_t)-dq[k].left);
        dq[k] = dq[--dq_n];
    }
}
/* .. and after it: the retrigs' hits (after: a sound's next trig in this block ends its retrig first, no double hit) */
static void drum_retrigs(track_t *t, uint32_t adv)
{
    uint32_t l;
    for (l = 0; l < DRUM_LANES; l++) {
        if (!drt[l].on || drt[l].now) {
            drt[l].now = 0;
            continue;
        }
        drt[l].el += adv;
        if (drt[l].el < drt[l].nx)
            continue;
        if (drt[l].len != RT_INF && drt[l].nx >= drt[l].len) {
            drt[l].on = 0;                          /* its length is over */
            continue;
        }
        dhit(t, drt[l].step, l, rt_vel(drt[l].vel, drt[l].fade, drt[l].nx, drt[l].len));
        drt[l].nx += drt[l].per;                    /* (the rate is longer than a block: one hit a block at most) */
        if (drt[l].len == RT_INF) {                 /* (until the sound's next trig: kept small) */
            drt[l].el -= drt[l].per;
            drt[l].nx -= drt[l].per;
        }
    }
}
/* the drum notes sent on MIDI out end here, but those of the step's trigs that played early */
static void seq_out_drum_off(const track_t *t, uint32_t keep)
{
    uint32_t l;
    for (l = 0; l < DRUM_LANES; l++)
        if (!((keep >> l) & 1u))
            seq_out_off(t, LANE_NOTE[l]);
}
/* play drum step idx (grid step nabs, slen units long, the grid into units past its start now, the pattern len steps):
 * each trig whose condition holds, on the grid or nudged late (skip: sounds already played by live recording); then the
 * next step's trigs nudged early are queued */
static void drum_step(track_t *t, uint32_t idx, uint32_t nabs, uint32_t slen, uint32_t into, uint32_t len, uint32_t skip)
{
    uint32_t l, m = dt_mask(idx) & ~skip, nxt, ahead = dq_ahead == nabs;
    seq_out_drum_off(t, ahead ? dq_early : 0u);
    for (l = 0; m; l++, m >>= 1) {
        int x;
        int32_t mu;
        if (!(m & 1u))
            continue;
        x = dt_x(idx, l);
        mu = x >= 0 ? dpat.x[x].micro : 0;
        if (mu < 0 && ahead)
            continue;                               /* (queued a step ahead) */
        if (!dt_plays(x))
            continue;
        if (mu <= 0)
            dtrig(t, idx, l, slen, mu < 0 ? 0u : into);   /* (nudged early but not queued: PLAY, a jump: now) */
        else
            dq_push(t, (int32_t)(slen / 64u) * mu - (int32_t)into, idx, l, slen);
    }
    nxt = seq_src(t, nabs + 1u) % (len ? len : 1u);
    dq_early = 0;
    for (l = 0, m = dt_mask(nxt); m; l++, m >>= 1) {
        int x;
        if (!(m & 1u) || (x = dt_x(nxt, l)) < 0 || dpat.x[x].micro >= 0 || !dt_plays(x))
            continue;
        dq_early |= (uint16_t)(1u << l);
        dq_push(t, (int32_t)slen + (int32_t)(slen / 64u) * dpat.x[x].micro - (int32_t)into, nxt, l, slen);
    }
    dq_ahead = nabs + 1u;
}

/* ratchets: the further hits of the playing synth step's notes, each at its share of the step */
static void seq_ratchets(track_t *t, uint32_t into, uint32_t slen)
{
    uint32_t i;
    if (t->seq_skip)
        return;                                     /* (its condition failed: no hit at all) */
    if (is_drum(t))
        return;                                     /* (the drum trigs' retrigs: drum_retrigs) */
    {
        const step_t *s = &t->step[t->seq_idx % NSTEP];
        if (s->time != ST_NOTE || !s->rat)
            return;
        for (i = 0; i < s->n; i++) {
            uint32_t hits = 1u + ((s->rat >> (2u * i)) & 3u), h;
            if (hits == 1u || roll_has(t, s->note[i]))
                continue;
            h = into * hits / slen;
            if (h > t->rat_done[i] && h < hits) {
                uint32_t j;
                t->rat_done[i] = (uint8_t)h;
                trk_note_off(t, s->note[i]);
                trk_note_on(t, s->note[i], step_vel(s, i));
                seq_out_on(t, s->note[i], step_vel(s, i));
                t->seq_off = slen / hits * (uint32_t)t->p[P_SGATE] / 128u;
                for (j = 0; j < t->seq_n && t->seq_notes[j] != s->note[i]; j++)
                    ;
                if (j == t->seq_n && t->seq_n < 4u)
                    t->seq_notes[t->seq_n++] = s->note[i];   /* (its gate ends it) */
            }
        }
    }
}

/* the nudge of grid step abs of track t, in units of a step slen long: where in its own step it fires
 * (micro >= 0), or how far before its step (micro < 0, as a negative number) */
static int32_t micro_units(const track_t *t, uint32_t abs, uint32_t slen)
{
    int32_t m = t->micro[abs % trk_len(t) % NSTEP];
    return (int32_t)(slen / 64u) * m;                /* |m| <= 32: fits */
}

/* The steps fire in order, one a block at most, each at its nudged time (micro: 1/64 of a step early
 * or late): the step after the last one played (seq_abs) is due when the grid is in its own step past
 * its nudge, or, nudged early, in the previous grid step past (length - |nudge|). So a step is never
 * skipped or played twice, whatever its neighbours' nudges (two that cross play in order, a block
 * apart), and a step nudged late past the next one's early nudge still plays first. Ratchets and
 * the recording stay on the grid (rec_target); seq_ratchets gets the time since the step fired. */

/* (PHEBUS) the grid step whose content plays at grid step nabs: itself, or during a 1/2 or 1/1 run of the STUTTER lane
 * (punch.c rep_*) one of the steps of the span before the run, in a loop. The run's start and span in this track's steps
 * (its DIV): a span shorter than one of its steps (a bar's steps on 1/2: 0) replays nothing */
static uint32_t seq_src(const track_t *t, uint32_t nabs)
{
    uint32_t div = trk_div(t), st, len;
    if (!rep_l16)
        return nabs;
    if (div < NDIV_SHORT) {                          /* den steps a beat: den / 4 a 1/16 */
        st = (rep_g0 * DIV_DEN[div] + 3u) / 4u;
        len = rep_l16 * DIV_DEN[div] / 4u;
    } else {                                         /* m beats a step: 4 m 1/16s */
        uint32_t m4 = 4u * DIV_BEATS[(div - NDIV_SHORT) % 3u];
        st = (rep_g0 + m4 - 1u) / m4;
        len = rep_l16 / m4;
    }
    if (!len || nabs < st || st < len)
        return nabs;                                 /* (before the run; nothing played before it yet) */
    return st - len + (nabs - st) % len;
}

static void seq_tick(track_t *t, uint32_t adv)
{
    uint32_t len = trk_len(t), into, slen, abs, idx, nabs, fire = 0;
    int32_t rel;
    if (t->seq_n && !t->seq_hold) {
        if (t->seq_off <= adv)
            seq_release(t);
        else
            t->seq_off -= adv;
    }
    if (!song.playing)
        return;
    abs = trk_grid(t, &into, &slen);
    if (is_drum(t))
        drum_block(t, adv, slen);                    /* (nudged hits and retrigs due in this block) */
    {
        uint32_t div = trk_div(t);
        if (t->seq_abs != SEQ_NONE && div != t->seq_den)
            t->seq_abs = abs;                        /* DIV changed: the next step of the new grid plays */
        t->seq_den = (uint8_t)div;
    }
    if (t->seq_abs == SEQ_NONE) {                    /* PLAY: the step the grid is in (nudged late: once there) */
        nabs = abs;
        fire = micro_units(t, nabs, slen) <= (int32_t)into;
    } else {
        int32_t mu;
        nabs = t->seq_abs + 1u;
        mu = micro_units(t, nabs, slen);
        if (abs == nabs)
            fire = mu <= (int32_t)into;              /* its own step: past its nudge (early: due already) */
        else if (abs + 1u == nabs)
            fire = mu < 0 && (int32_t)slen + mu <= (int32_t)into;   /* the step before: nudged early into it */
        else if ((int32_t)(abs - nabs) > 0)
            fire = 1;                                /* the grid jumped ahead: catch up, a step a block */
        /* (abs + 1 == seq_abs: SWING turned up inside a played odd step: nothing until the grid is back) */
    }
    if (fire) {                                      /* a new step: one a block at most */
        t->seq_abs = nabs;
        idx = seq_src(t, nabs) % (len ? len : 1u);   /* (a 1/2 or 1/1 STUTTER run: replayed) */
        t->seq_idx = (uint16_t)idx;
        t->rat_done[0] = t->rat_done[1] = t->rat_done[2] = t->rat_done[3] = 0;
        if (!idx) {
            t->pass++;                               /* a new pass of the loop (recording: one undo) */
            if (t->p[P_LTRIG] == 2 && !is_drum(t))   /* (PHEBUS, Ben 21 Nov 2026, picker "Add PATTERN") LFO SYNC > TRIG
                                                      * PATTERN: the LFO starts again as the pattern does (and at PLAY) */
                t->lfo_ph = (uint32_t)t->p[P_LPHASE] << 25;
        }
        if (erasing(t))
            erase_step(t, idx);                      /* EDIT + key held: gone as it passes */
        t->seq_skip = (uint8_t)!step_plays(t, idx);
        if (t->seq_skip) {                           /* its fill condition fails: as a REST with no lock */
            lock_step(t, NSTEP);                     /* (no step has locks there: the bases are back) */
            t->rskip_lanes = 0;
            t->rskip_n = 0;
            if (is_drum(t)) {
                seq_out_track_off(t);
            } else {
                rec_hold(t, idx, len, nabs);
                seq_release(t);
            }
        } else if (is_drum(t)) {
            uint32_t skip = t->rskip_abs == nabs ? t->rskip_lanes : 0u;
            lock_step(t, idx);                       /* its bus's locks, before the block renders */
            t->rskip_lanes = 0;
            drum_step(t, idx, nabs, slen, abs == nabs ? into : 0u, len, skip);
        } else {
            const step_t *s = &t->step[idx];
            uint32_t skip = 0, i, k;
            lock_step(t, idx);
            rec_hold(t, idx, len, nabs);
            if (t->rskip_n && t->rskip_abs == nabs)
                for (i = 0; i < s->n; i++)
                    for (k = 0; k < t->rskip_n; k++)
                        if (s->note[i] == t->rskip[k])
                            skip |= 1u << i;
            t->rskip_n = 0;
            seq_step(t, s, slen, skip);
        }
    }
    /* the ratchets of the step playing, timed from where it fired (its hits ride with its nudge) */
    if (t->seq_abs == abs)
        rel = (int32_t)into - micro_units(t, abs, slen);
    else if (t->seq_abs == abs + 1u)
        rel = (int32_t)into - ((int32_t)slen + micro_units(t, abs + 1u, slen));
    else
        rel = (int32_t)into;
    if (t->seq_abs != SEQ_NONE)
        seq_ratchets(t, rel < 0 ? 0u : (uint32_t)rel, slen);
    if (is_drum(t))
        drum_retrigs(t, adv);
}

/* MIDI in: the track that has channel ch (0..15) as its own: the drum track's GLO > DRUMS > CH, synth track i's
 * GLO > MIDI > CHi (2.6; 1, 2, 3 as before; a channel two tracks have: the first of them), else 0 */
static track_t *midi_own(uint32_t ch)
{
    uint32_t i;
    if (song.g[G_DRCH] && ch + 1u == (uint32_t)song.g[G_DRCH])
        return TDRUM;
    for (i = 0; i < NPART; i++)
        if (ch + 1u == (uint32_t)song.g[G_CH1 + i])
            return &trk[i];
    return 0;
}
/* the track a channel plays: its own, or (GLO > MIDI > OTHER = SEL) the selected one; 0: none (OTHER = OFF) */
static track_t *midi_track(uint32_t ch)
{
    track_t *t = midi_own(ch);
    return t ? t : song.g[G_CHOTH] ? 0 : TSEL;
}

/* a channel that plays the selected track: its note-off goes to the track its note-on went to,
 * even when another track was selected in between (else that note would hang) */
static uint8_t midi_sel_on[16][64];                   /* per channel and note: track + 1, 0 = none (a nibble a note: PHEBUS
                                                       * wave 3, 1 KB of RAM back) */
static uint32_t msel_get(uint32_t ch, uint32_t n) { return (midi_sel_on[ch & 15u][(n & 127u) >> 1] >> ((n & 1u) * 4u)) & 15u; }
static void msel_set(uint32_t ch, uint32_t n, uint32_t v)
{
    uint8_t *b = &midi_sel_on[ch & 15u][(n & 127u) >> 1];
    uint32_t sh = (n & 1u) * 4u;
    *b = (uint8_t)((*b & ~(15u << sh)) | (v & 15u) << sh);
}
static track_t *midi_route(uint32_t ch, uint32_t note, int on)
{
    track_t *t = midi_own(ch);
    if (t)
        return t;                                     /* a track's own channel (GLO > MIDI, GLO > DRUMS > CH) */
    if (song.g[G_CHOTH] && (on || !msel_get(ch, note)))
        return 0;                                     /* OTHER = OFF: nothing (a note-off only ends what a note-on of
                                                       * that channel started on the selected track, before) */
    t = TSEL;
    if (on)
        msel_set(ch, note, song.sel + 1u);
    else if (msel_get(ch, note)) {
        t = &trk[(msel_get(ch, note) - 1u) % NTRK];
        msel_set(ch, note, 0);
    }
    return t;
}

/* (2.6) the pedal and the wheel of a channel: the track they act on is remembered (track + 1), so a pedal let up or a
 * wheel let go after another track got selected (a SEL channel, IN = CH n) still lets go of the one they held */
static uint8_t ch_sus[16], ch_bend[16];
static void sus_set(track_t *t, int on);
static void midi_pedal(uint32_t ch, track_t *t, uint32_t value)
{
    uint32_t was = ch_sus[ch & 15u];
    if (was && was - 1u != trk_index(t))
        sus_set(&trk[(was - 1u) % NTRK], 0);           /* (the pedal's other track: let go there) */
    ch_sus[ch & 15u] = (uint8_t)(value >= 64u ? trk_index(t) + 1u : 0u);
}
static void midi_bend(uint32_t ch, track_t *t, uint32_t d1, uint32_t d2)
{
    uint32_t was = ch_bend[ch & 15u];
    if (was && was - 1u != trk_index(t))
        trk[(was - 1u) % NTRK].bend = 0;                /* (the wheel's other track: back to its pitch, gliding) */
    t->bend = (int16_t)((int32_t)(d1 | d2 << 7) - 8192);
    ch_bend[ch & 15u] = (uint8_t)(trk_index(t) + 1u);
}

/* SLOOP 2.5: MIDI CCs set track parameters, after Felucca 1.1.5's standard CC map (#103, Leo Kuroshita).
 * A CC acts on the track its channel plays, as the notes do (1-3 the synths, the drum channel the drum
 * track, 4-16 the selected track), and sets its parameter as a knob would: 0..127 over the parameter's
 * range, 64 the middle of a bipolar one. 5 GLIDE, 7 LEVEL, 10 PAN, 71 the engine's resonance (RES or Q;
 * an engine without one ignores it), 72 / 73 / 75 release / attack / decay, 74 the track's FILTER (64 off,
 * below a low-pass, above a high-pass: on every engine and the drums), 91 / 93 / 94 the reverb, chorus and
 * delay sends. The drum track takes 7, 91 and 94 as GLO > DRUMS LVL, REV and DLY (2.5), and 10 and 74. */
#define MCC_RES 0xFFu
static const uint8_t MIDI_CC_MAP[][2] = {
    {5, P_GLIDE}, {7, P_LEVEL}, {10, P_PAN}, {71, MCC_RES}, {72, P_REL}, {73, P_ATK}, {74, P_TFLT}, {75, P_DEC},
    {91, P_REV}, {93, P_CHOR}, {94, P_DLY},
};
/* MIDI LEARN (SLOOP 2.6, after Felucca 1.5 by Leo Kuroshita, Discussion #170): up to ML_N CCs, each set to one
 * parameter of one track, whatever channel it comes on (a channel IN ignores stays ignored) and whichever track that
 * channel plays. Kept with the settings (project.c settings_write, after the SYN kits), an entry 16 bits: bits 0..6
 * the CC, 7..8 the track, 9..15 the parameter's code, 0 = an empty entry: a common parameter its P_* id + 1 (their
 * ids never move, new ones go before P_E0), an engine parameter ML_E0 + k for P_E0 + k (P_E0 moves when common ones
 * are added). A learned CC no longer reaches the standard map on any channel: the controller is the user's now. The
 * CCs with a meaning of their own (0 / 32 bank select, 1 mod wheel, 6 / 38 data entry, 11 expression, 64 sustain,
 * 96..101, 120..127 the channel modes) are never learned. The UI (ui_input.c ml_*) writes the entries with the
 * interrupts off; ml_arm / ml_heard hand it a CC heard while learning */
#define ML_N 16u
#define ML_E0 120u
_Static_assert(P_E0 < ML_E0 && P_COUNT - P_E0 == 8, "MIDI LEARN: a code per parameter");
static volatile uint16_t ml_tab[ML_N];
static volatile uint8_t ml_arm, ml_heard;  /* the UI waits for a CC; the CC that came meanwhile + 1 (0 none) */
static uint32_t ml_id(uint32_t code)        /* an entry's code -> P_* id, P_COUNT: none this firmware knows */
{
    return code >= ML_E0 ? (code - ML_E0 < 8u ? P_E0 + code - ML_E0 : P_COUNT) : code && code <= P_E0 ? code - 1u : P_COUNT;
}
static int ml_free_cc(uint32_t cc)
{
    return cc && cc < 120u && cc != 1u && cc != 6u && cc != 11u && cc != 32u && cc != 38u && cc != 64u &&
           (cc < 96u || cc > 101u);
}
/* parameter id of track t from a CC value, as a knob over its range */
/* the pedal on track t: up, its waiting notes end now */
static void sus_set(track_t *t, int on)
{
    uint32_t ti = trk_index(t) % NTRK, n;
    if (is_drum(t))
        return;
    sus_on[ti] = (uint8_t)(on != 0);
    if (on)
        return;
    for (n = 0; n < 128u; n++)
        if ((sus_wait[ti][n >> 5] >> (n & 31u)) & 1u) {
            sus_wait[ti][n >> 5] &= ~(1u << (n & 31u));
            input_off(t, n);
        }
}
static void midi_cc_set(track_t *t, uint32_t id, uint32_t value)
{
    const param_desc_t *d = is_drum(t) && id == P_E0 ? &DRUM_KIT_DESC
                            : id >= P_E0 ? &ENGINES[t->eng_req % NENGINES]->edit[(id - P_E0) & 7u] : &TP[id % P_COUNT];
    if (d->max > d->min)
        t->p[id % P_COUNT] = (int16_t)(d->min + ((int32_t)(value & 127u) * (d->max - d->min) + 63) / 127);
}
/* 1: cc is learned (set here) or taken by MIDI LEARN waiting for one */
static int __attribute__((noinline)) midi_learned(uint32_t cc, uint32_t value)
{
    uint32_t i, e, id, hit = 0;
    if (!ml_free_cc(cc)) {
        if (ml_arm && !ml_heard)
            ml_heard = (uint8_t)(cc + 1u);              /* (the UI says CC NOT LEARNABLE; the CC does its own job) */
        return 0;
    }
    if (ml_arm) {
        if (!ml_heard)
            ml_heard = (uint8_t)(cc + 1u);
        return 1;
    }
    for (i = 0; i < ML_N; i++) {
        e = ml_tab[i];
        if ((e >> 9) && (e & 127u) == cc) {
            if ((id = ml_id(e >> 9)) < P_COUNT)
                midi_cc_set(&trk[(e >> 7) & 3u], id, value);
            hit = 1;
        }
    }
    return hit;
}

/* KNOBS PANEL (2026-10-20): a controller's 8 knobs (CC 1-8 or 70-77) are the FM-1's rotaries: 1 PRESETS, 2 SELECT,
 * 3 ALGORITHM, 4 MASTER (the level, at once), 5..8 KNOB 1..4. The others turn in detents: a knob's travel becomes
 * their steps (panel.c panel_enc adds them): KNOB 1..4 a step a CC unit, PRESETS one in 4, SELECT in 8, ALGORITHM
 * in 16. midi_enc in panel.c's order: SELECT, ALGORITHM, PRESETS, KNOB 1..4 */
static volatile int16_t midi_enc[7];
static int16_t midi_knob_last[8] = {-1, -1, -1, -1, -1, -1, -1, -1}, midi_knob_acc[8];
static int midi_panel_knob(uint32_t cc, uint32_t value)
{
    static const uint8_t ROLE[8] = {2, 0, 1, 0xFF, 3, 4, 5, 6}, UNITS[8] = {4, 8, 16, 1, 1, 1, 1, 1};
    uint32_t k;
    int32_t d;
    if (song.g[G_KNOBS] != 4 || !((cc >= 1u && cc <= 8u) || (cc >= 70u && cc <= 77u)))
        return 0;
    k = cc >= 70u ? cc - 70u : cc - 1u;
    if (k == 3u) {                                    /* MASTER: the level, as the pot (main.c: squared) */
        uint32_t k10 = value * 1023u / 127u;
        song.master_q12 = (k10 * k10) >> 8;
        return 1;
    }
    if (midi_knob_last[k] < 0) {                      /* (the first word from it: where it is, no step) */
        midi_knob_last[k] = (int16_t)value;
        return 1;
    }
    d = midi_knob_acc[k] + (int32_t)value - midi_knob_last[k];
    midi_knob_last[k] = (int16_t)value;
    midi_enc[ROLE[k]] = (int16_t)(midi_enc[ROLE[k]] + d / UNITS[k]);
    midi_knob_acc[k] = (int16_t)(d % UNITS[k]);
    return 1;
}

static void __attribute__((noinline)) midi_cc(track_t *t, uint32_t cc, uint32_t value)
{
    const param_desc_t *d = 0;
    int16_t *slot = 0;
    uint32_t i, id = 0xFFFFu;
    if (cc >= 120u) {                                  /* (3.1.3) the channel mode messages (never learned, never mapped):
                                                        * ALL SOUND OFF (120) and ALL NOTES OFF (123), as a DAW sends them
                                                        * on stop or panic: the track's notes end (with their release) and
                                                        * the pedal lets go; RESET ALL CONTROLLERS (121): the wheel and the
                                                        * pedal back. A track's notes from the FM-1's own keys end too */
        if (cc == 120u || cc == 123u)
            panic_req = (uint8_t)(panic_req | 1u << (trk_index(t) % NTRK));
        else if (cc == 121u) {
            sus_set(t, 0);
            t->bend = 0;
        }
        return;
    }
    if (midi_learned(cc, value))                       /* (a learned CC is the user's: not the map's) */
        return;
    if (midi_panel_knob(cc, value))                    /* KNOBS PANEL: the FM-1's rotaries */
        return;
    if (cc == 64u) {                                   /* the sustain pedal (2.6): >= 64 down */
        sus_set(t, value >= 64u);
        return;
    }
    if (!is_drum(t) && (((song.g[G_KNOBS] & 1) && cc >= 70u && cc <= 77u) ||
                        ((song.g[G_KNOBS] & 2) && cc >= 1u && cc <= 8u))) {   /* KNOBS: a controller's 8 knobs are the
                                                                     * track's EDIT knobs, page 1 then page 2 */
        midi_cc_set(t, P_E0 + (cc >= 70u ? cc - 70u : cc - 1u), value);
        return;
    }
    for (i = 0; i < NELEM(MIDI_CC_MAP); i++)
        if (MIDI_CC_MAP[i][0] == cc)
            id = MIDI_CC_MAP[i][1];
    if (id == 0xFFFFu)
        return;
    if (is_drum(t)) {
        if (id == P_LEVEL || id == P_REV || id == P_DLY) {
            id = id == P_LEVEL ? G_DRLVL : id == P_REV ? G_DRREV : G_DRDLY;
            d = &GP[id];
            slot = &song.g[id];
        } else if (id == P_PAN || id == P_TFLT) {
            d = &TP[id];
            slot = &t->p[id];
        }
    } else if (id == MCC_RES) {
        const engine_t *e = ENGINES[t->eng_req % NENGINES];
        for (i = 0; i < 8u && !d; i++)
            if (str_eq(e->edit[i].label, "RES") || str_eq(e->edit[i].label, "Q")) {
                d = &e->edit[i];
                slot = &t->p[P_E0 + i];
            }
    } else {
        d = &TP[id];
        slot = &t->p[id];
    }
    if (!d || d->max <= d->min)
        return;
    *slot = (int16_t)(d->min + ((int32_t)value * (d->max - d->min) + 63) / 127);
}

/* MIDI clock in (GLO > SYSTEM > SYNC = USB or TRS; after Felucca 1.0's midi_clock.c, from contributions by
 * ChanceTheMaker and keremimo): 24 pulses a beat. While the clock runs, the sequencer advances by the
 * pulses (a pulse = BEAT_U / 24 units), interpolated up to the next one from the last interval but never
 * past it, so it follows the master's tempo changes and cannot drift; BPM shows the master's tempo
 * (the slicer, delay and arp follow it). START restarts from the top, CONTINUE carries on where it
 * stopped, STOP stops. With no pulse for 0.5 s, the internal tempo takes over (PLAY works as ever). */
#define MCLK_PULSE_U (BEAT_U / 24u)
static struct {
    uint32_t pos, done;          /* units: the master's position (pulses since START), ours */
    uint32_t last_ms, iv_ms;     /* the last pulse, the interval between pulses (smoothed) */
    uint16_t bt[9];              /* (2026-10-31) the last beats' times (ms, low 16 bits: spans of seconds): the tempo
                                  * over up to 8 beats */
    uint8_t nbt;                 /* beats in bt */
    uint8_t skip;                /* (3.0.4) pulses after START / CONTINUE not yet timed (the first beat: see mclk_event) */
    int16_t mbpm;                /* (3.0.4) the master's tempo as last measured (0: none yet): START / CONTINUE start
                                  * at it */
    uint8_t have, n24;           /* a pulse since START; pulses towards the next tempo reading */
    uint8_t alive;               /* pulses are coming (from the SYNC source) */
} mclk;

static int mclk_on(void)                          /* the clock drives the sequencer */
{
    return song.g[G_SYNC] && mclk.alive && fm1_ms - mclk.last_ms < 500u;
}

/* Song Position Pointer (KARMA 3.0; 2026-10-28, Ariel: Ableton loops 16 bars): the master's position in 16ths.
 * Playing: every track jumps there at once. Stopped: the next CONTINUE starts from there */
static uint8_t spp_pend;
static uint16_t spp_pos;
static void seq_jump(uint32_t pos16)
{
    uint32_t bar = pos16 / 16u, in = (pos16 % 16u) * (BEAT_U / 4u), i;
    for (i = 0; i < NTRK; i++)
        seq_release(&trk[i]);
    seq_reset_tracks(in % BEAT_U);
    clk_beat = bar * 4u + in / BEAT_U;
    mclk.done = mclk.pos;                          /* (the master's next pulse is from here) */
}

static void mclk_event(uint32_t st, uint32_t src, uint32_t val)   /* a realtime message (or F2, val its position); src 1 USB, 2 TRS */
{
    uint32_t now = fm1_ms;
    if (!song.g[G_SYNC] || src != (uint32_t)song.g[G_SYNC])
        return;
    if (st == 0xF2u) {                             /* SONG POSITION */
        if (song.playing && !ci_on) {
            seq_jump(val);
        } else {
            spp_pos = (uint16_t)val;
            spp_pend = 1;
        }
        return;
    }
    if (st == 0xFAu || st == 0xFBu || st == 0xFCu)
        mclk.nbt = 0, mclk.skip = (uint8_t)(st == 0xFCu ? 0u : 24u);                              /* (2026-10-31) the tempo measured afresh: a pause in the pulses is
                                                    * never part of a beat (STOP, PLAY quickly: 104, then 122) */
    if (st == 0xFAu || st == 0xFBu) {              /* START: from the top; CONTINUE: on from where it stopped */
        if (mclk.mbpm && fm1_ms - mclk.last_ms < 30000u)   /* (3.0.4, the Ableton test: a STOP in song mode brought the project's
                                                    * 125 back, and the first beats after PLAY ran at it before the
                                                    * master's 120 was timed again) the master's tempo, at once (heard in
                                                    * the last 30 s: an old one is not the master's any more) */
            song.g[G_BPM] = mclk.mbpm;
        mclk.pos = mclk.done = 0;
        mclk.have = 0;
        if (st == 0xFAu) {
            spp_pend = 0;
            transport_req = 3;                     /* (not 1: the master counts, never a count-in) */
        } else if (spp_pend && !song.playing) {
            transport_req = 3;                     /* from the position the master sent (seq_jump after the start) */
        } else if (!song.playing) {
            song.playing = 1;
        }
        return;
    }
    if (st == 0xFCu) {                             /* STOP */
        transport_req = 2;
        return;
    }
    if (st != 0xF8u)
        return;
    if (mclk.alive && now - mclk.last_ms < 200u) { /* the interval, smoothed (a gap is not a tempo) */
        uint32_t iv = now - mclk.last_ms;
        mclk.iv_ms = mclk.iv_ms ? (mclk.iv_ms * 3u + iv + 2u) / 4u : iv;
    }
    if (mclk.skip) {                               /* (3.0.4, 2026-11-06, the Ableton test: after each STOP / PLAY the
                                                    * FM-1 read 144, 131, 127 BPM and rushed for a beat or two: the first
                                                    * pulses after START come in a clump, the render late while the section
                                                    * is set up, and the tempo was timed from the first of them.) The first
                                                    * beat after START / CONTINUE is not timed: the tempo from the next */
        mclk.skip--;
        mclk.nbt = 0;
    } else if (!mclk.alive || now - mclk.last_ms >= 500u || !mclk.nbt) {   /* (re)started: count a fresh beat */
        mclk.n24 = 0;
        mclk.bt[0] = (uint16_t)now;
        mclk.nbt = 1;
    } else if (++mclk.n24 == 24u) {                /* a beat: the tempo */
        /* (2026-10-31, Ariel: Ableton at 120, the FM-1 showed 119, 120, 119 ..) the pulses come a few ms early or late
         * (Ableton's per audio buffer: up to ~10 ms), and one beat at 120 is only 500 ms: 503 read as 119. The tempo over
         * the last 8 beats, changed only when it is clearly away (thr): the master's 120 stays 120, real changes follow */
        uint32_t span, beats, mbpm, cur;
        mclk.n24 = 0;
        if (mclk.nbt == 9u) {
            uint32_t j;
            for (j = 0; j < 8u; j++)
                mclk.bt[j] = mclk.bt[j + 1u];
            mclk.nbt = 8;
        }
        mclk.bt[mclk.nbt++] = (uint16_t)now;
        beats = mclk.nbt - 1u;
        span = (uint16_t)((uint16_t)now - mclk.bt[0]);
        if (beats && span >= 250u * beats && span <= 1500u * beats) {   /* 40..240 BPM */
            mbpm = (uint32_t)(((uint64_t)60000000u * beats + span / 2u) / span);   /* milli-BPM */
            cur = (uint32_t)song.g[G_BPM] * 1000u;
            /* how far it must be to count: the fewer beats measured, the more the pulses' jitter (~8 ms each end) weighs
             * (1 beat at 120: +-2 BPM; 8 beats: +-0.25), so 8, 3, 1.5, 0.6 BPM; 8 beats still follow a change of 1 */
            uint32_t thr = beats >= 6u ? 600u : beats >= 3u ? 1500u : beats == 2u ? 3000u : 8000u;
            if (mbpm > cur + thr || mbpm + thr < cur)
                song.g[G_BPM] = (int16_t)clamp((int32_t)((mbpm + 500u) / 1000u), 40, 240);
            mclk.mbpm = song.g[G_BPM];             /* (the master's, kept for the next START) */
        }
    }
    mclk.alive = 1;
    mclk.last_ms = now;
    if (song.playing || transport_req == 1u || transport_req == 3u) {   /* (a START queued with it: the next block starts) */
        if (mclk.have)
            mclk.pos += MCLK_PULSE_U;
        mclk.have = 1;                             /* the first pulse after START is the downbeat */
    }
}

/* (3.0.4, 2026-11-06, Ariel, synced to Ableton: "the bpm lost it in a few points") the gap was closed a quarter a block
 * (0.73 ms) at up to 1/8 of the tempo: every pulse's jitter (Ableton's +-8 ms) and every few pulses held back while the
 * render ran late were chased at once, the tempo swung +-13 % over 20 ms (seq2 t_mclk_smooth). Now a 128th a block (~0.1 s)
 * at up to 1/32 (3 %): the jitter averages out, a real drift is still closed in a fraction of a beat */
#define MCLK_GAIN 128
#define MCLK_LIM 64
static uint32_t mclk_adv(uint32_t n)               /* units to advance this block (mclk_on) */
{
    /* (2026-10-28, Ariel: freezes and rushes with SYNC = USB on the FM-1) the master's tempo (G_BPM, measured over its
     * 24 pulses), and the gap to where its pulses say it is closed a quarter a block, at most an eighth of the tempo
     * faster or slower: pulses that come in a burst (the render late, USB) are smoothed out, not caught up at twice
     * the tempo (a rush) or waited for (a freeze). More than a beat behind (a long stall): up to twice the tempo */
    uint32_t el, off = 0, tgt;
    int32_t rate = (int32_t)(n * (uint32_t)song.g[G_BPM]), err, corr, lim = rate / MCLK_LIM;
    if (!mclk.have)
        return 0;                                  /* START seen: wait for the downbeat */
    el = fm1_ms - mclk.last_ms;
    if (mclk.iv_ms) {
        if (el > mclk.iv_ms)
            el = mclk.iv_ms;
        off = el * MCLK_PULSE_U / mclk.iv_ms;      /* (<= 200 x 110250: fits 32 bits) */
        if (off >= MCLK_PULSE_U)
            off = MCLK_PULSE_U - 1u;
    }
    tgt = mclk.pos + off;
    err = (int32_t)(tgt - mclk.done);
    if (err > (int32_t)BEAT_U)
        lim = rate;                                /* (a beat or more behind: catch up faster) */
    else if (err > (int32_t)(BEAT_U / 4u) || err < -(int32_t)(BEAT_U / 4u))
        lim = rate / 8;                            /* (a quarter of a beat out: a real stall, never jitter: as before) */
    corr = (err > (int32_t)(BEAT_U / 4u) || err < -(int32_t)(BEAT_U / 4u)) ? err / 4 : err / MCLK_GAIN;
    corr = corr > lim ? lim : corr < -lim ? -lim : corr;
    err = rate + corr;
    mclk.done += (uint32_t)(err > 0 ? err : 0);
    return (uint32_t)(err > 0 ? err : 0);
}

/* everything that happens between two rendered blocks: transport, input, the steps of every
 * track at the clock, the click, the rolls and the arps; then the clock moves on by n samples */
static uint32_t mi_trk_held[NTRK][4];          /* (3.0.4) the notes MIDI in holds on each track (midi_in_lost) */
static void events_block(uint32_t n)
{
    uint32_t i, pr, adv;
    PC_T0();                                    /* (3.1.5 test 14: the measuring build times the parts) */
    if (mo_any && !song.g[G_MIDI]) {            /* MIDI = KEYS again: end what the sequencer had sent */
        seq_out_all_off();
        mo_any = 0;
    }
    if (transport_req == 1u || transport_req == 3u) {
        uint32_t ext = transport_req == 3u;         /* a MIDI START: the master counts; cut a count-in short */
        transport_req = 0;
        if (ext && ci_on)
            ci_on = 0;
        if (ci_on) {
            ci_on = 0;                              /* PLAY again during the count-in: back to armed */
        } else if (!ext && rec_wait && rec_count && !song.playing && !mclk_on()) {
            ci_on = 1;                              /* COUNT: one bar of clicks first (below) */
            ci_u = 0;
            ci_beat = 0;
            click_on(1);
        } else {
            seq_start();
            if (ext && spp_pend && song.playing)
                seq_jump(spp_pos);                  /* (a CONTINUE after a SONG POSITION: from there) */
            spp_pend = 0;
            if (rec_wait && song.playing)
                rec_begin();                        /* PLAY while armed: record from the top */
        }
    } else if (transport_req == 2u) {
        seq_stop();
        transport_req = 0;
        song.rec = 0;                               /* STOP ends the take (and the wait) */
        rec_wait = 0;
    }
    if (ci_on) {                                    /* the count-in: 4 beats at the tempo, then go */
        if (!rec_wait || song.playing) {
            ci_on = 0;                              /* (REC cancelled it, or it started some other way) */
        } else {
            uint32_t b;
            ci_u += n * (uint32_t)song.g[G_BPM];
            b = ci_u / BEAT_U;
            if (b >= 4u) {
                ci_on = 0;
                seq_start();
                if (song.playing)
                    rec_begin();
            } else if (b != ci_beat) {
                ci_beat = (uint8_t)b;
                click_on(0);
            }
        }
    }
    if (rec_wait && song.playing)
        rec_begin();                                /* started some other way: record now */
    adv = mclk_on() && song.playing ? mclk_adv(n) : n * (uint32_t)song.g[G_BPM];   /* (after a START) */
#if FELUCCA_ARRANGER
    if (song.playing)
        live_block();
#endif
    if (song.playing && !(clk_beat & 3u) && (clk_beat >> 2) != fill_last_bar) {   /* a new bar: the armed fill bar
                                                                                    * (after a section: its bar 0) */
        fill_last_bar = clk_beat >> 2;
        fill_bar_on = fill_arm;
        fill_arm = 0;
    }
    fill_now = (uint8_t)(fill_held || fill_lat || fill_bar_on);
    {   /* GLO > SYSTEM > IN changed to or from CH n, or a channel of GLO > MIDI / DRUMS (2.6): what the other routing
         * started is released (its note-offs would go elsewhere now: stuck notes), the pedals and wheels too */
        static uint8_t route_was;
        static uint32_t chans_was = 0xFFFFFFFFu;
        uint32_t rt = (uint32_t)song.g[G_ROUTE];
        uint32_t chans = (uint32_t)(song.g[G_CH1] & 31) | (uint32_t)(song.g[G_CH2] & 31) << 5 |
                         (uint32_t)(song.g[G_CH3] & 31) << 10 | (uint32_t)(song.g[G_CHOTH] & 1) << 15 |
                         (uint32_t)(song.g[G_DRCH] & 31) << 16;
        if ((rt != route_was && (rt >= 2u || route_was >= 2u)) || (chans != chans_was && chans_was != 0xFFFFFFFFu)) {
            panic_req = (uint8_t)((1u << NTRK) - 1u);
            seq_out_all_off();                        /* (MIDI OUT: the notes on end on the channels they went out on) */
            memset(midi_sel_on, 0, sizeof midi_sel_on);
            memset(ch_sus, 0, sizeof ch_sus);
            memset(ch_bend, 0, sizeof ch_bend);
            for (i = 0; i < NTRK; i++)
                trk[i].bend = 0;
        }
        route_was = (uint8_t)rt;
        chans_was = chans;
    }
    PC_MARK(PC_EV_TOP);
    pr = panic_req;
    panic_req = 0;
    if (pr)
        mi_pen[0] = mi_pen[1] = mi_pen[2] = mi_pen[3] = 0;   /* (the MIDI pen's notes held: never stuck either) */
    if (pr == (1u << NTRK) - 1u) {                    /* (2.6) all of it (a project, NEW, the MIDI routing): the
                                                       * wheels back to the middle too */
        memset(ch_bend, 0, sizeof ch_bend);
        memset(ch_sus, 0, sizeof ch_sus);
        for (i = 0; i < NTRK; i++)
            trk[i].bend = 0;
    }
    for (i = 0; i < NTRK; i++) {
        track_t *t = &trk[i];
        if ((pr >> i) & 1u) {
            if (t->pold) {                            /* (2.6) a preset loaded: the notes keep sounding as they are
                                                       * and fade out with the old sound (voice.c sound_change):
                                                       * no release cut short (an organ's key-off) under the fade */
                strum_cancel(t, 0, 1);
                t->nmono = 0, t->mono_note = 0, t->xp_n = 0;
            } else {
                trk_all_off(t);
            }
            sus_wait[i][0] = sus_wait[i][1] = sus_wait[i][2] = sus_wait[i][3] = 0;   /* (what the pedal held: gone too, */
            sus_on[i] = 0;                                                         /* and the pedal: never stuck) */
            t->nheld = 0;
            t->arp_phys = 0;
            t->arp_note = 0;
        }
        if (i < NPART)
            engine_block(t);                          /* engine switch: fade, then switch (voice.c) */
        /* ARP turned off, or HOLD released with no key down: drop the latched chord */
        if ((t->armp && !t->p[P_AMODE]) || (t->aholdp && !t->p[P_AHOLD] && !t->arp_phys)) {
            t->nheld = 0;
            if (!t->p[P_AMODE])
                t->arp_phys = 0;
            if (t->arp_note) {
                trk_note_off(t, t->arp_note);
                seq_out_off(t, t->arp_note);
                t->arp_note = 0;
            }
        }
        t->armp = t->p[P_AMODE];
        t->aholdp = t->p[P_AHOLD];
    }
    PC_MARK(PC_EV_ENG);
    keyboard_block();
    strum_block(n);                                   /* (voice.c: the strummed notes due) */
    PC_MARK(PC_EV_KEYS);
    while (mi_r != mi_w) {                            /* USB-MIDI (and TRS) in */
        uint32_t pkt = midi_in_q[mi_r % MQ], st = (pkt >> 8) & 0xF0u, ch = (pkt >> 8) & 0x0Fu;
        uint32_t d1 = (pkt >> 16) & 0x7Fu, d2 = (pkt >> 24) & 0x7Fu;
        track_t *t;
        mi_r++;
        if ((pkt & 15u) == 0xFu) {                    /* clock / transport: cable 0 USB, 1 TRS */
            mclk_event((pkt >> 8) & 0xFFu, ((pkt >> 4) & 15u) ? 2u : 1u, d1 | d2 << 7);
            continue;
        }
        if (song.g[G_ROUTE] >= 2 && ch == (uint32_t)song.g[G_ROUTE] - 2u) {   /* IN = CH n: that channel plays the selected
                                                       * track (notes and CCs); 2026-10-20: the other channels as with IN =
                                                       * NOTES (their tracks, the drums' channel), the default CH 1 */
            if (st == 0xB0u) {
                if (d1 == 64u)
                    midi_pedal(ch, TSEL, d2);
                midi_cc(TSEL, d1, d2);
                continue;
            }
            if (st == 0xE0u) {                        /* pitch bend (2.6) */
                if (!is_drum(TSEL))
                    midi_bend(ch, TSEL, d1, d2);
                continue;
            }
            if (st == 0xC0u) {                        /* Program Change (2.6): the selected track's sound */
                pc_req[song.sel % NTRK] = (uint8_t)(d1 + 1u);
                continue;
            }
            if (st != 0x90u && st != 0x80u)
                continue;
            if (st == 0x90u && d2) {
                msel_set(ch, d1, song.sel + 1u);
                t = TSEL;
            } else {                                  /* the note-off: the track its note-on went to */
                t = msel_get(ch, d1) ? &trk[(msel_get(ch, d1) - 1u) % NTRK] : TSEL;
                msel_set(ch, d1, 0);
            }
        } else {
            if (st == 0xB0u) {                            /* a CC (IN = CLOCK: none) */
                if (song.g[G_ROUTE] != 1 && (t = midi_track(ch)) != 0) {
                    if (d1 == 64u)
                        midi_pedal(ch, t, d2);
                    midi_cc(t, d1, d2);
                }
                else if (song.g[G_ROUTE] != 1)
                    (void)midi_learned(d1, d2);           /* (OTHER = OFF: a learned CC still works, on any channel) */
                continue;
            }
            if (st == 0xC0u) {                            /* Program Change (2.6): the sound of the track its channel
                                                           * plays (IN = CLOCK, OTHER = OFF and nobody's channel: none) */
                if (song.g[G_ROUTE] != 1 && (t = midi_track(ch)) != 0)
                    pc_req[trk_index(t) % NTRK] = (uint8_t)(d1 + 1u);
                continue;
            }
            if (st == 0xE0u) {                            /* pitch bend (2.6): the track its channel plays (IN = CLOCK:
                                                           * none) */
                if (song.g[G_ROUTE] != 1 && (t = midi_track(ch)) != 0 && !is_drum(t))
                    midi_bend(ch, t, d1, d2);
                continue;
            }
            if (st != 0x90u && st != 0x80u)
                continue;
            if (song.g[G_ROUTE] == 1 && st == 0x90u && d2)
                continue;                                 /* GLO > SYSTEM > IN = CLOCK: no notes (the note-offs still
                                                           * end what was held when it was set) */
            t = midi_route(ch, d1, st == 0x90u && d2);
            if (!t)
                continue;                                 /* (GLO > MIDI > OTHER = OFF: a channel no track has) */
        }
        if (is_drum(t)) {
            if (st == 0x90u && d2)
                drum_input(lane_of_note(d1), d2, 0, 1);
        } else if (st == 0x90u && d2) {
            if (t == TSEL) {                          /* the SEQ layer's pen: this note, or the chord held (2.6) */
                if (!(mi_pen[0] | mi_pen[1] | mi_pen[2] | mi_pen[3]) || pen_n >= 4u)
                    pen_n = 0;
                pen_note[pen_n++] = (uint8_t)d1;
                mi_pen[d1 >> 5] |= 1u << (d1 & 31u);
            }
            step_in_push(d1 | 0x80u | trk_index(t) << 8);
            mi_trk_held[trk_index(t) % NTRK][d1 >> 5] |= 1u << (d1 & 31u);
            input_on(t, d1, d2);
        } else {
            mi_pen[d1 >> 5] &= ~(1u << (d1 & 31u));
            mi_trk_held[trk_index(t) % NTRK][d1 >> 5] &= ~(1u << (d1 & 31u));
            step_in_push(d1 | trk_index(t) << 8);
            input_off(t, d1);
        }
    }
    if (midi_in_lost) {                               /* (3.0.4, after SLOOP 2.8.3) a note message was lost (the ring
                                                       * full), once what came is read: the notes MIDI holds end, none
                                                       * stays stuck for a lost note-off */
        uint32_t ti, w, b;
        midi_in_lost = 0;
        for (ti = 0; ti < NTRK; ti++)
            for (w = 0; w < 4u; w++)
                for (b = 0; b < 32u; b++)
                    if ((mi_trk_held[ti][w] >> b) & 1u) {
                        mi_trk_held[ti][w] &= ~(1u << b);
                        input_off(&trk[ti], w * 32u + b);
                    }
        mi_pen[0] = mi_pen[1] = mi_pen[2] = mi_pen[3] = 0;
    }
    PC_MARK(PC_EV_MIDI);
    stl_seq();                                        /* (PHEBUS) a 1/2 or 1/1 STUTTER run: the tracks replay (punch.c) */
    for (i = 0; i < NTRK; i++)
        seq_tick(&trk[i], adv);
    stl_tick();                                       /* (PHEBUS) the STUTTER lane's step (punch.c) */
    click_tick();
    roll_block(adv);
    PC_MARK(PC_EV_SEQ);
    for (i = 0; i < NPART; i++)
        arp_tick(&trk[i], adv);
    if (song.playing) {
        song.tick++;
        clk_pos += adv;
        while (clk_pos >= BEAT_U) {
            clk_pos -= BEAT_U;
            clk_beat++;
        }
    }
    PC_MARK(PC_EV_ARP);
}
