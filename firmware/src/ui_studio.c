/* SPDX-License-Identifier: GPL-3.0-only */
/* LIVE screens, 240 x 240, in a teenage-engineering-like style: black, four colours (one per
 * track and per knob: blue, green, white, orange), white for what you touch, red for recording,
 * big numbers, lowercase labels, four dials at the bottom that show what KNOB 1..4 do.
 * Screens: TRACKS (the performance view), DRUMS (GRID / KIT, pads that flash on each hit), the
 * LAYERS (a function button held: what the 16 white keys and the knobs do now), HOLD (a hold to
 * confirm: clear, save), REC (armed). Every band is drawn into the canvas only when its
 * signature changed. */
static uint8_t drum_cursor;              /* the drum GRID's step anchor (its page of steps); drum_lane, the selected sound: drums.c */
static uint32_t drum_dec_id(void) { return drum_model() == DM_SYN ? DSP_S0 + DS_DECAY : DSP_DEC; }   /* the sound's decay */
static void trk_short_name(uint32_t c, char *b);
static void op_header(const char *tag, uint16_t tc, const char *title, uint32_t *cache);   /* ui_op.c */
static void op_knobs(const char *const lab[4], const char *const val[4], const char *const *unit, const int32_t ratio[4],
                     uint32_t dim, uint32_t sig, uint32_t *cache);
static void op_trackbar(uint32_t *cache);
static void op_drum_steps(uint32_t bank, uint32_t len);   /* (ui_op.c) */
static void op_home_body(void);
static void op_box(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c);
static void op_badge(int32_t x, int32_t y, char ch, uint16_t bg);
static int on_drum_page(void) { return !ui.home && cur_page()->scope == SC_DRUM; }

/* ---------------------------------------------------------------- style --- */
#define TE_G1 RGB(26, 26, 30)            /* tiles */
#define TE_G2 RGB(54, 54, 60)            /* empty steps, dial tracks */
#define TE_G3 RGB(118, 118, 126)         /* labels */
#define TE_G4 RGB(196, 196, 204)         /* secondary text */
#define TE_RED RGB(255, 44, 52)          /* recording, erasing */
#define TE_MUTE RGB(255, 196, 40)        /* a muted track: a yellow M, as Ableton (SPEC.md, 20 Nov 2026) */
#define TE_SOLO RGB(150, 215, 255)       /* a soloed track: an ice-blue S, as Ableton (review 15: never the alien's blue) */
/* Ben's palette (SPEC.md, 20 Nov 2026): track 1 purple, 2 blue, 3 green, 4 (drums) coral */
static const uint16_t TE_COL[4] = {RGB(140, 118, 240), RGB(40, 160, 245), RGB(20, 196, 170), RGB(255, 128, 108)};
static const uint16_t TE_MID[4] = {RGB(92, 78, 158), RGB(26, 106, 162), RGB(13, 129, 112), RGB(168, 84, 71)};
static const uint16_t TE_DIM[4] = {RGB(48, 40, 82), RGB(14, 54, 83), RGB(7, 67, 58), RGB(87, 44, 37)};
#define TE_DRUM TE_COL[3]
/* Ben's rule (SPEC.md, 20 Nov 2026): a voice's colour means "this acts on that voice". A knob strip is tinted by what
 * each knob acts on: 0..3 a track, 4 neutral (general or global controls). knob_tint[] is set by the screen that draws
 * the strip, just before op_knobs / te_dials. */
#define TINT_NEUTRAL 4u
static const uint16_t *const TE_TINTS[3] = {TE_COL, TE_MID, TE_DIM};
static uint8_t knob_tint[4] = {0, 1, 2, 3};
static uint16_t tint_col(uint32_t k, uint32_t shade)    /* shade 0 full, 1 mid, 2 dim */
{
    static const uint16_t NEU[3] = {RGB(196, 196, 204), RGB(118, 118, 126), RGB(54, 54, 60)};
    uint32_t v = knob_tint[k & 3u];
    return v < 4u ? TE_TINTS[shade][v] : NEU[shade];
}
static void knob_tint_all(uint32_t v) { knob_tint[0] = knob_tint[1] = knob_tint[2] = knob_tint[3] = (uint8_t)v; }
static void knob_tint_voices(void) { knob_tint[0] = 0, knob_tint[1] = 1, knob_tint[2] = 2, knob_tint[3] = 3; }
static uint32_t knob_tint_sig(void) { return knob_tint[0] + knob_tint[1] * 5u + knob_tint[2] * 25u + knob_tint[3] * 125u; }

static void te_disc(int32_t cx, int32_t cy, int32_t r, uint16_t c)     /* filled circle */
{
    int32_t y, x;
    for (y = -r; y <= r; y++)
        for (x = -r; x <= r; x++)
            if (x * x + y * y <= r * r + r)
                cv_pset(cx + x, cy + y, c);
}
/* a dial: a 270-degree ring (lit up to the value), a pointer; ratio 0..1000, -1 = a plain ring */
static void te_dial(int32_t cx, int32_t cy, int32_t r, int32_t ratio, uint16_t c, uint16_t dim)
{
    int32_t i, end = ratio < 0 ? 768 : ratio * 768 / 1000;
    for (i = 0; i <= 768; i += 8) {
        uint32_t a = (uint32_t)(384 + i) & 1023u;
        int32_t co = SINE[(a + 256u) & 1023u], si = SINE[a], k;
        uint16_t col = ratio < 0 || i <= end ? c : dim;
        for (k = r - 2; k <= r; k++)
            cv_pset(cx + ((co * k) >> 15), cy + ((si * k) >> 15), col);
    }
    if (ratio >= 0) {
        uint32_t a = (uint32_t)(384 + end) & 1023u;
        int32_t co = SINE[(a + 256u) & 1023u], si = SINE[a];
        cv_line(cx, cy, cx + ((co * (r - 4)) >> 15), cy + ((si * (r - 4)) >> 15), C_WHITE);
        te_disc(cx, cy, 2, C_WHITE);
    }
}
static void te_play_icon(int32_t x, int32_t y, int playing)
{
    int32_t i;
    if (playing)
        for (i = 0; i < 12; i++)
            cv_rect(x + i, y + i / 2, 1, 14 - i, C_WHITE);   /* a triangle */
    else
        cv_rect(x, y + 1, 12, 12, TE_G3);
}
static uint32_t studio_hash(uint32_t h, const char *p)
{ while (*p) h = h * 31u + (uint8_t)*p++; return h; }
static void te_lower(char *d, const char *s, uint32_t n)
{
    uint32_t i;
    for (i = 0; i + 1u < n && s[i]; i++)
        d[i] = s[i] >= 'A' && s[i] <= 'Z' ? (char)(s[i] + 32) : s[i];
    d[i] = 0;
}
static void te_text_c(int32_t cx, int32_t y, const char *s, uint16_t c)   /* centred on cx */
{
    cv_text(cx - text_w(&FONT_S, s) / 2, y, &FONT_S, s, c);
}

/* the dial strip: KNOB 1..4, label + value under each (a dial with no label: an empty column); a
 * message replaces it */
static void te_dials(int32_t y0, const char *const lab[4], const char *const val[4], const int32_t ratio[4],
                     uint32_t sig, uint32_t *cache)
{
    uint32_t k;
    sig = studio_hash(sig + knob_tint_sig() * 7919u, ui.msg_t ? ui.msg : "");
    for (k = 0; k < 4u; k++) {
        sig = studio_hash(sig * 7u + (uint32_t)ratio[k] + (ui.hot_t && ui.hot_col == k) * 5003u, lab[k]);
        sig = studio_hash(sig, val[k]);
    }
    if (!ui.force && sig == *cache)
        return;
    *cache = sig;
    cv_begin(240, ui.msg_t ? 240u - (uint32_t)y0 : 40u, C_BLACK);
    if (ui.msg_t) {                                     /* a message: a white bar */
        cv_rect(0, 10, 240, 32, C_WHITE);
        cv_text((240 - text_w(&FONT_S, ui.msg)) / 2, 18, &FONT_S, ui.msg, C_BLACK);
        cv_blit(0, (uint32_t)y0);
        return;
    }
    for (k = 0; k < 4u; k++) {
        int32_t cx = 30 + 60 * (int32_t)k;
        if (!lab[k][0])
            continue;
        te_dial(cx, 12, 11, ratio[k], tint_col(k, 0), tint_col(k, 2));
        if (ui.hot_t && ui.hot_col == k) {              /* turned: the label inverted in the knob's colour (review 16e) */
            int32_t w = text_w(&FONT_S, lab[k]);
            cv_rect(cx - w / 2 - 1, 24, w + 1, 16, tint_col(k, 0));
            te_text_c(cx, 24, lab[k], C_BLACK);
        } else {
            te_text_c(cx, 24, lab[k], TE_G3);
        }
    }
    cv_blit(0, (uint32_t)y0);
    cv_begin(240, 16, C_BLACK);                         /* the values */
    for (k = 0; k < 4u; k++)
        te_text_c(30 + 60 * (int32_t)k, 0, val[k], TE_G4);
    cv_blit(0, (uint32_t)y0 + 40u);
}

static void studio_open(uint32_t scope)
{
    uint32_t i;
    for (i = 0; i < NPAGES; i++) if (PAGES[i].scope == scope) {
        ui.page = (uint8_t)i; ui.home = 0; song.seq_mode = 0; ui.force = 1;
        ui.msg_t = 0; ui.entry_open = 0; return;
    }
}
/* LIVE: REC, on any page. One record arm, on the selected track (it follows ALGORITHM). No
 * click (seq.c): playing, recording starts at once; stopped, REC arms and the first note
 * starts the loop. REC again stops recording / cancels the arm (playback continues). */
static void rec_toggle(void)
{
    if (song.rec || rec_wait) {
        song.rec = 0;
        rec_wait = 0;
        ui_message("REC OFF");
        return;
    }
    if (song.playing)
        rec_begin();                    /* playing: record now */
    else
        rec_wait = 1;                   /* stopped: the first note starts it */
}

/* the loop position of track t: "bar.beat" in its pattern (1.1 .. ) */
static void loop_pos(const track_t *t, char *b)
{
    uint32_t len = (uint32_t)clamp(t->p[P_SLEN], 1, NSTEP), s = t->seq_idx % len, k;
    fmt_int(b, (int32_t)(s / 16u + 1u));
    k = str_len(b);
    b[k] = '.';
    b[k + 1] = (char)('1' + (s % 16u) / 4u);
    b[k + 2] = 0;
}

/* the header of the live screens: BPM, transport, the loop position, the beat lights, REC */
static void te_header(const char *title, uint16_t tc, uint32_t *cache)
{
    char b[12];
    uint32_t beat = clk_beat, k, playing = song.playing;
    uint32_t sig = studio_hash((uint32_t)song.g[G_BPM] * 7u + playing * 3u + (song.rec != 0) * 1999u +
                               (playing ? beat * 131u + TSEL->seq_idx * 7919u : 0u) + (uint32_t)song.g[G_CLOCK] * 77u +
                               (ui.bpm_t != 0) * 104729u + song.solo * 37u, title);
    sig = sig * 31u + tc;
    if (!ui.force && sig == *cache)
        return;
    *cache = sig;
    cv_begin(240, 40, C_BLACK);
    fmt_int(b, song.g[G_BPM]);
    cv_text(4, 4, &FONT_L, b, ui.bpm_t ? C_WHITE : TE_G4);
    cv_text(4 + text_w(&FONT_L, b) + 4, 20, &FONT_S, "bpm", TE_G3);
    te_play_icon(104, 6, (int)playing);
    if (playing) {                                     /* bar.beat in the selected track's loop */
        loop_pos(TSEL, b);
        cv_text(122, 5, &FONT_S, b, C_WHITE);
    }
    for (k = 0; k < 4u; k++)                           /* the four beats of the bar */
        cv_rect(104 + (int32_t)k * 9, 26, 7, 7, playing && beat % 4u == k ? (k || !song.rec ? C_WHITE : TE_RED) : TE_G2);   /* (red: recording only) */
    if (song.rec) {
        te_disc(224, 12, 8, TE_RED);
        cv_text(176, 4, &FONT_S, "rec", TE_RED);
        if (song.g[G_CLOCK] != 0)                       /* the click is on */
            cv_text(168, 22, &FONT_S, "click", TE_G3);
    } else {
        cv_text(240 - text_w(&FONT_S, title) - 4, 5, &FONT_S, title, tc);
        if (song.solo)
            cv_text(236 - text_w(&FONT_S, "solo"), 22, &FONT_S, "solo", C_WHITE);
    }
    cv_rect(0, 39, 240, 1, TE_G1);
    cv_blit(0, 0);
}

/* --------------------------------------------------------------- TRACKS --- */
/* swing, MPC style: "62%" */
static void swing_str(char *b, int32_t v)            /* 0 straight .. 100 the most (params.c F_SWING) */
{
    fmt_int(b, clamp(v, 0, 100));
}

/* (PHEBUS, Ben 21 Nov 2026: "having easy access to decay, filter, fx, makes for fun play on the fly"; picker "Cutoff,
 * decay, FX, level", and of the FX knob: "Would be cool if it actually just turned on and off the stutter") HOME's KNOB 1
 * and 2, for the selected voice: CUTOFF, the engine's own filter cutoff (its CUT: ACID, JUNO), else the voice's FILTER
 * (P_TFLT: PLAITS, ELEMENTS, and the drums' track filter); DECAY, the decay of the envelope that moves the sound: the
 * engine's own envelope where it has one (ACID's DECAY), else the shared ENV's DEC (JUNO, PLAITS, ELEMENTS); on the
 * drums the selected sound's decay on every kit (PHEBUS wave 3: its AMP page's DEC / DECAY; a built-in kit's first turn
 * makes the project's copy, ui_drum.c). (KNOB 3 the STUTTER lane on /
 * off, KNOB 4 LEVEL: ui_input.c tracks_edit) */
static int16_t *home_knob(uint32_t k, const param_desc_t **d)
{
    track_t *t = TSEL;
    uint32_t i, kit = drum_kit();
    if (is_drum(t)) {
        static int16_t dec;                             /* (PHEBUS wave 3: the selected sound's DECAY, every kit: a mirror;
                                                         * ui_input.c tracks_edit turns it through the drum pages') */
        if (!k) {
            *d = &TP[P_TFLT];
            return &t->p[P_TFLT];
        }
        (void)kit;
        if (!(*d = dsp_desc(drum_model(), lane_snd(drum_lane), drum_dec_id())))
            return 0;
        dec = (int16_t)dsp_get(drum_model(), lane_snd(drum_lane), lane_lfo(drum_lane), drum_lane, drum_dec_id());
        return &dec;
    }
    for (i = 0; i < 8u; i++)
        if (str_eq(ENGINES[t->eng_req % NENGINES]->edit[i].label, k ? "DECAY" : "CUT")) {
            *d = track_desc(t, P_E0 + i);
            return &t->p[P_E0 + i];
        }
    *d = &TP[k ? P_DEC : P_TFLT];
    return &t->p[k ? P_DEC : P_TFLT];
}
static void studio_tracks_draw(void)
{
    static uint32_t head, footer, bar;
    {   /* (PHEBUS wave 2: the header's title was empty since the song cut) the pattern playing, A..D, as the Rytm names
         * its pattern; none stored or picked yet: the instrument's name */
        char ti[12] = "PATTERN A";
        ti[8] = (char)('A' + (live_sec & 3));
        op_header("HOME", TE_G3, live_sec >= 0 && live_sec <= 3 ? ti : "PHEBUS", &head);
    }
    op_home_body();                                    /* the deck, the selected track, the four lanes (ui_op.c) */
    {   /* KNOB 1 cutoff, 2 decay of the selected voice, 3 the STUTTER lane on / off (everyone's), 4 the voice's level */
        track_t *t = TSEL;
        static char v[4][8];
        static const char *const lab[4] = {"cutoff", "decay", "stut", "level"};
        const char *val[4] = {v[0], v[1], v[2], v[3]}, *u;
        int32_t ratio[4];
        uint32_t lvl = is_drum(t) ? song.g[G_DRLVL] : t->p[P_LEVEL], k, dim = t->p[P_MUTE] ? 8u : 0u;
        const char *unit[4] = {"", "", "", t->p[P_MUTE] ? "MUTE" : ""};   /* muted: its real level, dim, "MUTE" (review 19) */
        for (k = 0; k < 2u; k++) {
            const param_desc_t *d;
            int16_t *vp = home_knob(k, &d);
            if (!vp) {                                  /* (a drum kit with no DEC to turn) */
                str_cpy(v[k], "--", 8);
                ratio[k] = 0;
                dim |= 1u << k;
                continue;
            }
            param_format(d, *vp, v[k], &u);
            if (d->fmt != F_PCT)                        /* (a time: its unit; a % as LEVEL: the number alone) */
                unit[k] = u;
            ratio[k] = (*vp - d->min) * 1000 / (d->max > d->min ? d->max - d->min : 1);
        }
        str_cpy(v[2], stl.act ? "ON" : "OFF", 8);
        ratio[2] = stl.act ? 1000 : 0;
        fmt_int(v[3], (int32_t)lvl * 100 / 127);
        ratio[3] = (int32_t)lvl * 1000 / 127;
        knob_tint_all(song.sel);                        /* HOME: the voice's knobs in its colour, the stutter everyone's */
        knob_tint[2] = TINT_NEUTRAL;
        op_knobs(lab, val, unit, ratio, dim, song.sel, &footer);
    }
    op_trackbar(&bar);
}

/* ---------------------------------------------------------------- DRUMS --- */
static const char *const LV_NAME[4] = {"norm", "ghost", "soft", "hard"};
static uint32_t lvl_rank(uint32_t lvl);                 /* (below) ghost 0, soft 1, norm 2, hard 3 */
/* the level order of the knobs: ghost < soft < norm < hard */
static const uint8_t LV_UP[4] = {LV_GHOST, LV_SOFT, LV_NORM, LV_HARD};
static uint16_t lvl_col(uint32_t lvl)                   /* a hit's colour by its level */
{
    return lvl == LV_GHOST ? TE_DIM[3] : lvl == LV_SOFT ? TE_MID[3] : lvl == LV_HARD ? C_WHITE : TE_DRUM;
}
static uint16_t vel_col(uint32_t vel) { return lvl_col(vel_lvl(vel)); }   /* .. a drum trig's by its velocity's band */
static uint16_t pad_lit[DRUM_LANES];                   /* pads and key LEDs: frames left lit */
static uint8_t key_lit[27];                             /* menu NOTES, a synth track: frames its key stays lit */
static uint8_t key_lit_trk = 0xFF;                      /* (the track key_lit is for) */
static uint8_t op_note = 60, op_hit;                    /* the selected synth track: its last note, frames since (8 .. 0) */

static void pads_tick(void)                             /* once a frame: the hits since the last one */
{
    uint32_t i, hits, n[NPART][4], k, w;
    fm1_irq_off();
    hits = drums.hits;
    drums.hits = 0;
    for (i = 0; i < NPART; i++)
        for (w = 0; w < 4u; w++) {
            n[i][w] = note_hits[i][w];
            note_hits[i][w] = 0;
        }
    fm1_irq_on();
    for (i = 0; i < DRUM_LANES; i++) {
        if ((hits >> i) & 1u) pad_lit[i] = 6;
        else if (pad_lit[i]) pad_lit[i]--;
    }
    /* a synth track: each note it started lights its key 6 frames (~0.1 s), so a short sequencer note
     * (a 1/16 at GATE 50 %: 60..80 ms) is seen even when it ends between two frames */
    if (song.sel != key_lit_trk) {
        memset(key_lit, 0, sizeof key_lit);
        key_lit_trk = song.sel;
    }
    for (k = 0; k < 27u; k++)
        if (key_lit[k])
            key_lit[k]--;
    if (op_hit)
        op_hit--;
    if (song.sel < NPART)                               /* the track's scene answers its notes (ui_op.c) */
        for (k = 0; k < 128u; k++)
            if ((n[song.sel][k >> 5] >> (k & 31u)) & 1u) {
                op_note = (uint8_t)k;
                op_hit = 8;
            }
    if (song.sel < NPART && (n[song.sel][0] | n[song.sel][1] | n[song.sel][2] | n[song.sel][3]))
        for (k = 0; k < 27u; k++) {
            uint32_t note = kb_map(TSEL, k);
            if (note < 128u && (n[song.sel][note >> 5] >> (note & 31u)) & 1u)
                key_lit[k] = 6;
        }
}

#include "ui_grid.c"               /* the step GRID on every track (Elektron style) */
#include "ui_stut.c"               /* (PHEBUS) the GRID's STUTTER lane, after DRUM */
#include "ui_drum.c"               /* (PHEBUS wave 3) the drum screen, the Rytm's way */

static uint32_t lvl_rank(uint32_t lvl) { return lvl == LV_GHOST ? 0u : lvl == LV_SOFT ? 1u : lvl == LV_NORM ? 2u : 3u; }

/* the DRUMS screen (the SC_DRUM page): a synth track's GRID (ui_grid.c), the STUTTER lane (ui_stut.c), the drum screen or
 * its MUTE mode (ui_drum.c) */
static void drum_screen_draw(void)
{
    uint32_t len = trk_len(TDRUM);
    if (song.sel != TRK_DRUM) {
        sg_draw();
        return;
    }
    if (stl_shown()) {
        stl_draw();
        return;
    }
    if (drum_cursor >= len)
        drum_cursor = (uint8_t)(len - 1u);
    if (drum_keys == DK_MUTE)
        drum_mute_draw();
    else
        drum_main_draw();
}

static void drum_screen_input(uint32_t pressed, uint32_t home)
{
    uint32_t k, b;
    int32_t s;
    if (home == 1u) {
        go_home();
        return;
    }
    for (k = 0; k < NB; k++) if ((pressed >> panel.btn[k]) & 1u) {
        b = k;
        if (b == B_PLAY) {
            transport_req = song.playing ? 2 : 1;
        }
    }
    if (!sg_held && !stl_held && (s = panel_enc(EN_ALGO)) && !stl_algo(s))   /* the track: the GRID stays, on
                                                         * the track chosen (after DRUM: the STUTTER lane, ui_stut.c) */
        track_select((uint32_t)clamp((int32_t)song.sel + s, 0, 3));
    if (stl_shown()) {
        stl_input(pressed);
        return;
    }
    sg_follow();                                        /* (playing: the page the playhead goes onto) */
    if (song.sel != TRK_DRUM) {                         /* a synth track's GRID (ui_grid.c) */
        sg_synth_input(pressed);
        return;
    }
    drum_grid_input(pressed);                           /* (PHEBUS wave 3: ui_drum.c) */
}

/* ---------------------------------------------------------------- REC --- */
/* a big seven-segment digit, TE style: w x h, segments t thick */
static void te_digit(int32_t x, int32_t y, int32_t w, int32_t h, int32_t t, uint32_t d, uint16_t c)
{
    static const uint8_t SEG[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};   /* gfedcba */
    uint32_t m = SEG[d % 10u];
    int32_t hh = h / 2;
    if (d % 10u == 1u) {                                /* a real 1 (review 14): a centred stem, a two-step flag, a foot */
        int32_t sx = x + (w - t) / 2;
        cv_rect(sx, y, t, h, c);
        cv_rect(sx - t, y + t, t, t, c);
        cv_rect(sx - 2 * t, y + 2 * t, t, t, c);
        cv_rect(x + w / 6, y + h - t, w - w / 3, t, c);
        return;
    }
    if (m & 0x01) cv_rect(x + t, y, w - 2 * t, t, c);                       /* a */
    if (m & 0x02) cv_rect(x + w - t, y + t, t, hh - t, c);                  /* b */
    if (m & 0x04) cv_rect(x + w - t, y + hh, t, hh - t, c);                 /* c */
    if (m & 0x08) cv_rect(x + t, y + h - t, w - 2 * t, t, c);               /* d */
    if (m & 0x10) cv_rect(x, y + hh, t, hh - t, c);                         /* e */
    if (m & 0x20) cv_rect(x, y + t, t, hh - t, c);                          /* f */
    if (m & 0x40) cv_rect(x + t, y + hh - t / 2, w - 2 * t, t, c);          /* g */
}

static uint8_t rec_shown;
/* the four tracks, compact, from y0: the one that records is framed in red */
static void rec_rows(uint32_t rt, uint32_t y0, uint32_t *cache)
{
    uint32_t sig = rt * 3u + rec_wait + drum_kit() * 977u + y0, i, j;
    for (i = 0; i < NTRK; i++) {
        char b[16];
        if (i == TRK_DRUM) str_cpy(b, DRUM_KIT_NAMES[drum_kit()], sizeof b);
        else trk_short_name(i, b);
        sig = studio_hash(sig, b) + (uint32_t)trk[i].p[P_SLEN] * 31u;
        for (j = 0; j < NSTEP; j++) sig = sig * 3u + (uint32_t)trk_step_on(&trk[i], j);
    }
    if (!ui.force && sig == *cache)
        return;
    *cache = sig;
    cv_begin(240, 76, C_BLACK);
    for (i = 0; i < NTRK; i++) {
        const track_t *t = &trk[i];
        uint32_t len = (uint32_t)clamp(t->p[P_SLEN], 1, 64), armed = i == rt;
        int32_t y = (int32_t)i * 19;
        char b[16];
        if (armed) cv_rect(0, y, 240, 18, TE_RED), cv_rect(1, y + 1, 238, 16, C_BLACK);
        cv_rect(4, y + 3, 12, 12, armed ? TE_COL[i] : TE_DIM[i]);
        if (i == TRK_DRUM) str_cpy(b, DRUM_KIT_NAMES[drum_kit()], sizeof b);
        else trk_short_name(i, b);
        b[10] = 0;
        cv_text(22, y + 1, &FONT_S, b, armed ? C_WHITE : TE_G3);
        for (j = 0; j < 16u; j++) {
            uint32_t a = j * len / 16u, z = (j + 1u) * len / 16u, k, on = 0;
            if (z == a) z = a + 1u;
            for (k = a; k < z && k < NSTEP; k++) if (trk_step_on(t, k)) on = 1;
            cv_rect(106 + (int32_t)j * 8, y + 5, 6, 8, on ? (armed ? TE_COL[i] : TE_DIM[i]) : TE_G1);
        }
    }
    cv_blit(0, y0);
}

/* REC armed (stopped: waiting for the first note, or PLAY) or its count-in (seq.c). Armed, the dials say how it
 * records: KNOB 2 LENGTH (1, 2 or 4 bars), KNOB 3 START note / count (ui_input.c rec_knobs). The four tracks stay in
 * view; the REC LED blinks while armed */
static void rec_screen_draw(void)
{
    static uint32_t head, body, rows, dials;
    uint32_t sig, rt = song.sel;
    uint32_t blink = (fm1_ms / 250u) & 1u, count = ci_on;
    if (!rec_shown) {
        lcd_fill(0, 0, 240, 240, C_BLACK);
        ui.force = 1;
    }
    rec_shown = 1;
    te_header(count ? "count-in" : "rec ready", TE_RED, &head);
    /* armed / counting in: what happens next */
    sig = 2000003u + rec_count * 13u + count * 17u + ci_beat * 19u + blink * 31u;
    if (ui.force || sig != body) {
        static const char *const L1[2] = {"play a note", "press play"};
        static const char *const L2[2] = {"it starts the loop", "4 clicks, then rec"};
        static const char *const L3[2] = {"play: go  rec: cancel", "rec: cancel"};
        uint32_t m = rec_count ? 1u : 0u;
        body = sig;
        cv_begin(240, 64, C_BLACK);
        if (count) {                                    /* the count-in: 4, 3, 2, 1 */
            te_digit(96, 4, 30, 54, 6, 4u - (ci_beat > 3u ? 3u : ci_beat), C_WHITE);
            te_disc(60, 31, 12, TE_RED);
            cv_text(144, 14, &FONT_S, "count-in", TE_G4);
            cv_text(144, 34, &FONT_S, "rec: cancel", TE_G3);
        } else {
            te_disc(28, 30, 18, blink ? TE_RED : TE_G2);
            te_disc(28, 30, 7, C_BLACK);
            cv_text(60, 6, &FONT_S, L1[m], C_WHITE);
            cv_text(60, 24, &FONT_S, L2[m], TE_G4);
            cv_text(60, 42, &FONT_S, L3[m], TE_G3);
        }
        cv_blit(0, 42);
    }
    rec_rows(rt, 106u, &rows);
    {   /* the dials: how it records */
        static char v[3][10];
        static const char *const LAB_N[4] = {"", "steps", "start", ""};   /* (STEPS: the pattern's length, review 21) */
        static const char *const LAB_0[4] = {"", "", "", ""};
        const char *val[4] = {v[0], v[1], v[2], ""};
        int32_t ratio[4] = {0, 0, 0, 0};
        uint32_t len = (uint32_t)clamp(TSEL->p[P_SLEN], 1, 64);
        v[0][0] = 0;
        if (len % 16u == 0u) {
            fmt_int(v[1], (int32_t)(len / 16u));
            str_cpy(v[1] + str_len(v[1]), len == 16u ? " bar" : " bars", 6);
        } else {
            fmt_int(v[1], (int32_t)len);
            str_cpy(v[1] + str_len(v[1]), " st", 4);
        }
        str_cpy(v[2], rec_count ? "count" : "note", sizeof v[2]);
        ratio[1] = (int32_t)(len - 1u) * 1000 / 63;
        ratio[2] = rec_count ? 1000 : 0;
        {
            const char *const *lab = count ? LAB_0 : LAB_N;
            uint32_t k;
            for (k = 0; k < 4u; k++)
                if (!lab[k][0])
                    val[k] = "";                        /* (a dial not shown: no value either) */
            knob_tint_all(song.sel);
            te_dials(184, lab, val, ratio, rt * 7u + count * 3u, &dials);
        }
    }
}
