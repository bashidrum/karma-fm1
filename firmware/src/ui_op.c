/* SPDX-License-Identifier: GPL-3.0-only */
/* The three bands every LIVE screen shares, top to bottom: the header (a tag that says where you are,
 * the tempo, the transport), the four knob readouts (each knob's label and value in the colour of what it acts
 * on: a voice's colour, or neutral grey for what everyone shares) and the track strip (the four tracks).
 * The screen's own picture goes between the header and the readouts, rows OP_BODY_Y .. OP_KNOB_Y - 1.
 * Each band is its own canvas with its own signature, as the rest of ui_studio.c. */
#define OP_HEAD_H 22
#define OP_BODY_Y OP_HEAD_H
#define OP_KNOB_Y 154
#define OP_KNOB_H 58
#define OP_BAR_Y 212
#define OP_BAR_H 28
#define OP_PUR RGB(146, 152, 166)        /* line art: what is neither touched nor playing (a neutral cool grey: only */
#define OP_PUR_D RGB(60, 64, 74)         /* the voices carry colour); stands, the floor, empty steps */
static int32_t batt_shown(void);         /* ui_draw.c */
static int32_t batt_level(void);
static int32_t meter_px(int32_t a);

static void op_box(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c)   /* a 1 px outline */
{
    cv_rect(x, y, w, 1, c);
    cv_rect(x, y + h - 1, w, 1, c);
    cv_rect(x, y, 1, h, c);
    cv_rect(x + w - 1, y, 1, h, c);
}

/* the header: [tag] title | BPM, play / stop, the four beats, REC (else the unsaved dot), the battery */
static uint8_t proj_unsaved;                           /* (PHEBUS) the work differs from flash (project.c unsaved_tick) */
static void op_header(const char *tag, uint16_t tc, const char *title, uint32_t *cache)
{
    char b[12];
    uint32_t beat = clk_beat, k, playing = song.playing;
    uint32_t sig = studio_hash((uint32_t)song.g[G_BPM] * 7u + playing * 3u + (song.rec != 0) * 1999u +
                               (playing ? beat * 131u : 0u) + (ui.bpm_t != 0) * 104729u + (uint32_t)batt_shown() * 7777u + proj_unsaved * 31337u +
                               sr_on * 3331u, title);
    sig = studio_hash(sig * 31u + tc, tag);
    if (!ui.force && sig == *cache)
        return;
    *cache = sig;
    cv_begin(240, OP_HEAD_H, C_BLACK);
    {   /* the tag in its box, one width for every tag (five characters: the title always starts at x 52; review 18) */
        int32_t w = 46;
        if (str_len(tag) > 5u)                          /* (ELEMENTS, PLAITS: their five-letter tags) */
            tag = tag[0] == 'E' ? "ELEM" : tag[0] == 'P' ? "PLTS" : "SYN";
        op_box(2, 1, w, 18, tc);
        te_text_c(2 + w / 2, 2, tag, tc);
        str_cpy(b, title, 10);
        while (b[0] && w + 8 + text_w(&FONT_S, b) > 122)
            b[str_len(b) - 1u] = 0;
        cv_text(w + 8, 2, &FONT_S, b, TE_G3);
    }
    fmt_int(b, song.g[G_BPM]);
    cv_text(150 - text_w(&FONT_S, b), 2, &FONT_S, b, ui.bpm_t ? C_WHITE : TE_G4);
    if (playing) {
        int32_t i;
        for (i = 0; i < 10; i++)
            cv_rect(156 + i, 4 + i / 2, 1, 12 - i, C_WHITE);
    } else {
        cv_rect(156, 5, 10, 10, TE_G3);
    }
    for (k = 0; k < 4u; k++)                           /* the four beats of the bar */
        cv_rect(172 + (int32_t)k * 8, 7, 6, 6, playing && beat % 4u == k ? (k || !song.rec ? C_WHITE : TE_RED) : TE_G2);   /* (red: recording only) */
    if (song.rec || sr_on)                              /* (recording, or STEP REC: ui_grid.c) */
        te_disc(209, 10, 4, TE_RED);
    else if (proj_unsaved)                             /* (saved again at the next stop: project.c autosave_tick) */
        te_disc(209, 10, 2, C_WHITE);
    {   /* battery, 3 bars (filling in turn while a USB host charges it) */
        int32_t lvl = batt_shown(), i;
        op_box(217, 5, 17, 10, TE_G3);
        cv_rect(234, 8, 2, 4, TE_G3);
        for (i = 0; i < lvl; i++)
            cv_rect(219 + i * 5, 7, 3, 6, lvl == 1 && batt_level() <= 1 ? TE_RED : TE_G4);
    }
    cv_rect(0, OP_HEAD_H - 1, 240, 1, TE_G1);
    cv_blit(0, 0);
}

/* the knob readouts: under each knob its label, a 2 px gauge and the value (then its unit, small, grey), all
 * in one face on one baseline (Ben, 21 Nov 2026, review 14 "A: one bold face"): the value large when it fits its
 * column, else small, the same baseline either way; label and gauge in what the knob acts on's colour (knob_tint).
 * The knob being turned keeps its colour: its label inverted, its value underlined (review 16e). A value that does
 * nothing now (dim: a bit per knob): grey. A label left empty: an empty column. unit may be 0. A message replaces
 * the band (as te_dials) */
#define OPK_BASE 50                                     /* the values' baseline, in the band */
#if KARMA_ANN
static void opk_take(const char *const lab[4], const char *const val[4], const char *const *unit);   /* (ui_draw.c) */
#endif
static void op_knobs(const char *const lab[4], const char *const val[4], const char *const *unit, const int32_t ratio[4],
                     uint32_t dim, uint32_t sig, uint32_t *cache)
{
    uint32_t k;
#if KARMA_ANN
    opk_take(lab, val, unit);                           /* every frame, for the screen reader (ui_speak.c) */
#endif
    sig = studio_hash(sig + dim * 977u + knob_tint_sig() * 7919u, ui.msg_t ? ui.msg : "");
    for (k = 0; k < 4u; k++) {
        sig = studio_hash(sig * 7u + (uint32_t)ratio[k] + (ui.hot_t && ui.hot_col == k) * 5003u, lab[k]);
        sig = studio_hash(sig, val[k]);
        if (unit)
            sig = studio_hash(sig, unit[k]);
    }
    if (!ui.force && sig == *cache)
        return;
    *cache = sig;
    cv_begin(240, OP_KNOB_H, C_BLACK);
    cv_rect(0, 2, 240, 1, TE_G1);
    if (ui.msg_t) {                                     /* a message: a white bar */
        cv_rect(0, 14, 240, 32, C_WHITE);
        cv_text((240 - text_w(&FONT_S, ui.msg)) / 2, 22, &FONT_S, ui.msg, C_BLACK);
        cv_blit(0, OP_KNOB_Y);
        return;
    }
    for (k = 0; k < 4u; k++) {
        int32_t x0 = 4 + 58 * (int32_t)k, x;
        uint32_t hot = ui.hot_t && ui.hot_col == k;
        uint16_t c = (dim >> k) & 1u ? TE_G3 : tint_col(k, 0);
        const char *v = val[k], *u = unit ? unit[k] : "";
        int32_t uw = u[0] ? text_w(&FONT_S, u) + 2 : 0; /* (3 px after the value's ink, the unit's last column blank) */
        if (!lab[k][0])
            continue;
        if (hot) {                                      /* touched: the label inverted */
            cv_rect(x0 - 1, 5, text_w(&FONT_S, lab[k]) + 1, 16, tint_col(k, 0));
            cv_text(x0, 5, &FONT_S, lab[k], C_BLACK);
        } else {
            cv_text(x0, 5, &FONT_S, lab[k], tint_col(k, 0));
        }
        if (ratio[k] >= 0) {                            /* where the value is in its range */
            cv_rect(x0, 22, 52, 2, TE_G2);
            cv_rect(x0, 22, ratio[k] * 52 / 1000, 2, tint_col(k, 0));
        }
        {   /* large if it fits 54 px (4 px clear of the next column); a unit 3 px after it, 1 px where only that fits */
            int32_t lw = text_w(&FONT_L, v) - 2, gap = 3;   /* (FONT_L: 2 blank columns after the last letter) */
            if (lw + uw > 54 && lw + uw - 2 <= 54)
                gap = 1;
            if (lw + uw - (3 - gap) <= 54)
                x = cv_text(x0, OPK_BASE - 24, &FONT_L, v, c) - 2;
            else
                x = cv_text(x0, OPK_BASE - 12, &FONT_S, v, c) - 1, gap = 3;
            if (uw && x + uw - (3 - gap) <= x0 + 55)
                cv_text(x + gap, OPK_BASE - 12, &FONT_S, u, TE_G3);
        }
        if (hot)
            cv_rect(x0, 54, 52, 3, tint_col(k, 0));
    }
    cv_blit(0, OP_KNOB_Y);
}

#include "op_sprites.h"
/* the track strip (Ben, 21 Nov 2026, review 15 "A: heads, underline"; mute / solo "A: badges"): a status line, not
 * tabs. Each track its character's head and its number (1 2 3 DR); the selected track underlined, its number in its
 * colour (the others grey); a muted track a yellow M badge, a soloed one an ice-blue S, the one that records a red
 * dot. A 1 px rule above */
static void op_sprite_wh(const uint8_t *d, int32_t w, int32_t h, int32_t x0, int32_t y0, int flip, uint16_t c, int32_t s);
static void op_badge(int32_t x, int32_t y, char ch, uint16_t bg)   /* 10 x 14: a black letter on bg */
{
    char b[2] = {ch, 0};
    cv_rect(x, y, 10, 14, bg);
    cv_text(x + 1, y - 1, &FONT_S, b, C_BLACK);
}
static void op_trackbar(uint32_t *cache)
{
    static const char *const NAME[NTRK] = {"1", "2", "3", "DR"};
    uint32_t i, sig = 1u + song.sel + song.rec * 16u + song.solo * 256u + (uint32_t)(on_drum_page() && stl_shown()) * 4096u;
    for (i = 0; i < NTRK; i++)
        sig = sig * 3u + (trk[i].p[P_MUTE] != 0);
    if (!ui.force && sig == *cache)
        return;
    *cache = sig;
    cv_begin(240, OP_BAR_H, C_BLACK);
    cv_rect(0, 0, 240, 1, TE_G1);
    for (i = 0; i < NTRK; i++) {
        int32_t x = 4 + 58 * (int32_t)i, nx = x + OP_HM_W + 4, bx;
        uint32_t sel = song.sel == i && !(on_drum_page() && stl_shown());   /* (the STUTTER lane, after DR: no track underlined, PHEBUS) */
        op_sprite_wh(OP_SPR_HM[i], OP_HM_W, OP_HM_H, x, 5, 0, TE_COL[i], 1);
        bx = cv_text(nx, 5, &FONT_S, NAME[i], sel ? TE_COL[i] : TE_G3) + 3;
        if ((song.rec >> i) & 1u)
            te_disc(bx + 4, 12, 3, TE_RED);
        else if (trk[i].p[P_MUTE])
            op_badge(bx, 6, 'M', TE_MUTE);
        else if ((song.solo >> i) & 1u)
            op_badge(bx, 6, 'S', TE_SOLO);
        if (sel)
            cv_rect(x, 24, 54, 3, TE_COL[i]);
    }
    cv_blit(0, OP_BAR_Y);
}

/* the characters (op_sprites.h, drawn by tools/gen_sprites.py): a robot (track 1), an alien (2), a crocodile (3), a
 * snake (the drums), each in its track's colour; pictures in flash, 4 bits a pixel: 1 dark, 2 the colour, 3 light
 * (shades of the track's colour), 4 white, 5 black. They dance to the beat: op_dance */
static uint8_t op_dim;                                  /* the next pictures dim (a muted, a silent track) */
static uint16_t op_mix(uint16_t c, uint16_t to, uint32_t k)   /* c, k eighths of the way to "to" */
{
    int32_t r = (int32_t)(c >> 11), g = (int32_t)((c >> 5) & 63u), b = (int32_t)(c & 31u);
    r += ((int32_t)(to >> 11) - r) * (int32_t)k / 8;
    g += ((int32_t)((to >> 5) & 63u) - g) * (int32_t)k / 8;
    b += ((int32_t)(to & 31u) - b) * (int32_t)k / 8;
    return (uint16_t)((uint32_t)r << 11 | (uint32_t)g << 5 | (uint32_t)b);
}
static void op_pal(uint16_t *pal, uint16_t c, int dim)   /* dim: c is dim already (a muted track's) */
{
    pal[0] = 0;
    pal[1] = op_mix(c, C_BLACK, dim ? 3 : 5);
    pal[2] = c;
    pal[3] = op_mix(c, C_WHITE, dim ? 2 : 4);
    pal[4] = dim ? TE_G3 : C_WHITE;
    pal[5] = C_BLACK;
}
/* a picture w x h (even), each pixel s x s; (x0, y0): the top left, flip: mirrored, c: the track's colour */
static void op_sprite_wh(const uint8_t *d, int32_t w, int32_t h, int32_t x0, int32_t y0, int flip, uint16_t c, int32_t s)
{
    uint16_t pal[6];
    int32_t x, y, i;
    op_pal(pal, c, op_dim);
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++) {
            uint32_t v = (d[(y * w + x) >> 1] >> ((x & 1) * 4)) & 15u;
            int32_t px = x0 + (flip ? w - 1 - x : x) * s;
            if (v && v < 6u)
                for (i = 0; i < s * s; i++)
                    cv_pset(px + i % s, y0 + y * s + i / s, pal[v]);
        }
}
/* a character at full size (112 x 104: its pixels doubled): pose 0 still, 1 leaning, 2 arms up (op_dance) */
static void op_char(uint32_t ti, uint32_t pose, int flip, int32_t x0, int32_t y0)
{
    if (op_strike[ti & 3u])                              /* its voice just played an accent: the STRIKE pose (Ben: "Both") */
        pose = 3u;
    op_sprite_wh(OP_SPR_BIG[ti & 3u][pose & 3u], OP_BIG_W, OP_BIG_H, x0, y0, flip, TE_COL[ti & 3u], 2);
}
/* the dance: a pose a beat while the song plays (lean right, arms up, lean left, arms up the other way); still
 * when it is stopped. *flip: mirrored */
static uint32_t op_dance(int *flip)
{
    uint32_t b = clk_beat & 3u;
    *flip = 0;
    if (!song.playing)
        return 0;
    *flip = b >= 2u;
    return b & 1u ? 2u : 1u;
}

/* ---------------------------------------------------------------- DRUMS --- */
/* the GRID page: the snake dancing to the beat (the floor flashes white on the kick), the step in large digits,
 * the 16 x 16 steps of this page in small at the right, then the 16 steps of KNOB 1's sound as the 16 white keys. */
#define OP_SCENE_H 96
#define OP_STEPS_H 36

/* an arc of an ellipse, a0 .. a1 in 1/1024 turns (0 = right, 256 = down), as 2 px chords */
static void op_arc(int32_t cx, int32_t cy, int32_t rx, int32_t ry, uint32_t a0, uint32_t a1, uint16_t c)
{
    int32_t px = 0, py = 0;
    uint32_t a, first = 1;
    for (a = a0;; a += 32u) {
        uint32_t m = (a > a1 ? a1 : a) & 1023u;
        int32_t x = cx + ((SINE[(m + 256u) & 1023u] * rx + 16384) >> 15), y = cy + ((SINE[m] * ry + 16384) >> 15);
        if (!first)
            cv_line(px, py, x, y, c);
        first = 0;
        px = x;
        py = y;
        if (a >= a1)
            break;
    }
}
static void op_ell(int32_t cx, int32_t cy, int32_t rx, int32_t ry, uint16_t c) { op_arc(cx, cy, rx, ry, 0, 1024, c); }
static void op_drum_scene(uint32_t bank, uint32_t len)
{
    uint32_t i, j, n, pose;
    int flip;
    uint16_t c;
#define LIT(a) (pad_lit[a] != 0)
#define SEL(a) (drum_lane == (a))
    cv_begin(240, OP_SCENE_H, C_BLACK);
    for (i = 40; i < 188u; i += 4u)
        cv_pset((int32_t)i, 93, LIT(0) || LIT(1) ? C_WHITE : OP_PUR_D);
    /* the step: the one that plays (white), stopped the one of KNOB 2 (green) */
    n = (song.playing ? TDRUM->seq_idx % len : drum_cursor) + 1u;
    c = song.playing ? C_WHITE : TE_COL[3];
    te_digit(4, 4, 14, 30, 2, n / 10u, c);
    te_digit(22, 4, 14, 30, 2, n % 10u, c);
    cv_text(4, 38, &FONT_S, "STEP", TE_G3);
    /* the snake, standing on the floor */
    pose = op_dance(&flip);
    op_char(TRK_DRUM, pose, flip, 58, -10);
    /* this page of steps in small: 16 sounds down, 16 steps across, the column that plays framed */
    for (i = 0; i < DRUM_LANES; i++)
        for (j = 0; j < 16u; j++) {
            uint32_t p = bank * 16u + j;
            if (p >= len)
                continue;
            cv_rect(190 + (int32_t)j * 3, 4 + (int32_t)i * 3, 2, 2,
                    dstep_has(&TDRUM->dstep[p], i) ? (SEL(i) ? TE_COL[3] : OP_PUR) : SEL(i) ? TE_DIM[3] : RGB(30, 32, 38));
        }
    if (song.playing && TDRUM->seq_idx % len / 16u == bank)
        op_box(189 + (int32_t)(TDRUM->seq_idx % len % 16u) * 3, 3, 4, 50, C_WHITE);
    for (i = 0; i < 4u; i++) {                          /* the four page slots: this one filled, the others in use
                                                         * grey, the unused ones dim (OCT+ past the last adds one) */
        if (i == bank)
            cv_rect(190 + (int32_t)i * 8, 58, 6, 6, C_WHITE);
        else
            op_box(190 + (int32_t)i * 8, 58, 6, 6, i * 16u < len ? OP_PUR : OP_PUR_D);
    }
#undef LIT
#undef SEL
    cv_blit(0, OP_BODY_Y);
}

/* the 16 steps of KNOB 1's sound: a hit filled in its level's colour (a notch per ratchet), the cursor
 * framed white, the step that plays underlined */
static uint8_t op_steps_y = OP_BODY_Y + OP_SCENE_H;     /* where: under the snake; a step held: at the top, as on a
                                                         * synth's GRID (review 19) */
static void op_drum_steps(uint32_t bank, uint32_t len)
{
    uint32_t j, r;
    cv_begin(240, OP_STEPS_H, C_BLACK);
    for (j = 0; j < 16u; j++) {
        uint32_t p = bank * 16u + j;
        const dstep_t *s = &TDRUM->dstep[p < NSTEP ? p : 0];
        int32_t x = 4 + (int32_t)j * 14 + (int32_t)(j / 4u) * 2;
        if (p >= len)
            continue;
        if (dstep_has(s, drum_lane)) {
            cv_rect(x, 4, 12, 22, lvl_col(dstep_lvl(s, drum_lane)));
            for (r = 0; r < dstep_rat(s, drum_lane); r++)
                cv_rect(x + 2, 7 + (int32_t)r * 4, 8, 2, C_BLACK);
        } else {
            op_box(x, 4, 12, 22, j % 4u ? OP_PUR_D : OP_PUR);
        }
        if (p == drum_cursor)
            op_box(x - 2, 2, 16, 26, C_WHITE);
        if (song.playing && p == TDRUM->seq_idx % len)
            cv_rect(x, 31, 12, 2, C_WHITE);
    }
    cv_blit(0, (uint32_t)op_steps_y);
}

/* ---------------------------------------------------------------- SYNTH --- */
/* a synth track's page without a graph of its own (EDIT, VOICE, the DEST pages...): the track's
 * character in its own scene (below), moved by the sound: the track's meter, its last note, each note
 * as it starts. */
static uint32_t op_ph;                                  /* the wave scrolls while there is sound */
static int32_t op_px;                                   /* PRISM's picture is at the top right: the scenes' things there go left by this */
static int32_t op_lvl;                                  /* the meter, falling */

/* one cycle of the wave, ph in 1/1024: kind 0..4 ANALOG's SAW SQR TRI SIN PWM, 5 a sine with more
 * harmonics as morph (0..1000) rises; -32767..32767 */
static int32_t op_wave(uint32_t ph, uint32_t kind, int32_t morph)
{
    int32_t p = (int32_t)(ph & 1023u);
    switch (kind) {
    case 0: return 32767 - p * 64;
    case 1: return p < 512 ? 32767 : -32767;
    case 2: return p < 512 ? -32767 + p * 128 : 32767 - (p - 512) * 128;
    case 3: return SINE[p];
    case 4: return p < 256 ? 32767 : -32767;
    default: return (SINE[p] + SINE[(p * 2) & 1023] * morph / 2000 + SINE[(p * 3) & 1023] * morph / 3000) * 2 / 3;
    }
}

/* the characters small (the mixer: op_k 2, HOME: op_k 1), still pictures: op_k says which size, op_ink dim a muted
 * or silent track */
static int32_t op_k = 3;
static uint16_t op_ink = OP_PUR;

/* the sound of the track, at the top of the graph canvas: its preset alone, in white (the engine is the header's tag,
 * review 18) */
static void op_names(const track_t *t)
{
    char b[16];
    const engine_t *e = ENGINES[t->eng_req % NENGINES];
    b[0] = 0;
    if (user_of(t) < UP_SLOTS)
        up_name(user_of(t), b);
    else if (e->npresets)
        str_cpy(b, e->presets[t->preset % e->npresets].name, sizeof b);
    cv_text(4, 2, &FONT_S, b, C_WHITE);
}

/* the voice whose envelope the ENV page follows: the one earliest in its envelope (a new note first), then
 * the loudest; 0: the track is silent */
static const voice_t *op_env_voice(const track_t *t)
{
    const voice_t *best = 0;
    uint32_t i;
    for (i = 0; i < NVOICE; i++) {
        const voice_t *v = &t->v[i];
        if (!v->active || !v->stage || v->stage > 3u)
            continue;
        if (!best || v->stage < best->stage || (v->stage == best->stage && v->env > best->env))
            best = v;
    }
    return best;
}
/* The three scenes draw into the graph canvas (240 x 124, ui_draw.c draw_graph). ratio: the four knobs,
 * 0..1000 (-1: none); each owns a part of the picture, in its colour. */

/* the scenes' character: its top in the graph canvas (screen y 36) so the sound chain's strip (y 141) leaves its feet
 * whole, the floor just under them (review 19) */
#define OP_SPR_Y 14
#define OP_FLOOR_Y 118
/* synth 1, the robot (the bass): it dances to the beat (op_dance); a note: a little hop. KNOB 1 the wave behind it
 * (ANALOG's WAVE as it is, else from round to sharp; its ripples the last note's pitch), KNOB 2 the rays round
 * its head, KNOB 3 a moon that rises, KNOB 4 sparks off the floor on a note */
static void op_scene_robot(track_t *t, const int32_t ratio[4], int analog_wave)
{
    uint32_t kind = analog_wave ? (uint32_t)clamp(t->p[P_E0], 0, 4) : 5u, pose;
    uint32_t cyc = (uint32_t)clamp(384 + ((int32_t)op_note - 36) * 13, 256, 1536), rnd = (ui.frame / 2u) * 2654435761u + 1u;
    int32_t morph = ratio[0] < 0 ? 0 : ratio[0], amp = 3 + op_lvl * 10 / 82, x, py = 0, cx = 120, i, n, hop = op_hit > 5u ? 2 : 0;
    int flip;
#define WY(xx, f, ph) (104 - ((op_wave((uint32_t)(((xx) - 8) * (int32_t)(f) / 56) + (ph), kind, morph) * amp) >> 15))
    pose = op_dance(&flip);
    if (ratio[2] >= 0)                                  /* KNOB 3: the moon */
        op_ell(222 - (op_px ? 146 : 0), 70 - ratio[2] * 34 / 1000, 5, 5, tint_col(2u, 0));
    for (x = 8; x <= 232; x += 2) {                     /* KNOB 1: the wave; white as a note starts */
        int32_t y = WY(x, cyc, op_ph);
        if (x > 8)
            cv_line(x - 2, py, x, y, op_hit > 6u ? C_WHITE : tint_col(0u, 0));
        py = y;
    }
    n = ratio[1] <= 0 ? 0 : 3 + ratio[1] * 10 / 1000;   /* KNOB 2: the rays, round its head */
    for (i = 0; i < n; i++) {
        uint32_t a = 512u + 64u + (uint32_t)i * 384u / (uint32_t)(n > 1 ? n - 1 : 1);
        int32_t hx = cx + (pose == 1u ? (flip ? -10 : 10) : 0), hy = 42 - (pose == 2u ? 4 : 0) - hop;
        int32_t co = SINE[(a + 256u) & 1023u], si = SINE[a & 1023u];
        cv_line(hx + ((co * 30) >> 15), hy + ((si * 26) >> 15), hx + ((co * 36) >> 15), hy + ((si * 32) >> 15), tint_col(1u, 0));
    }
    op_ell(120, OP_FLOOR_Y, 70, 2, op_hit > 4u ? C_WHITE : OP_PUR_D);   /* the floor: it rings under a note */
    op_char(0, pose, flip, cx - 56, OP_SPR_Y - hop);
    n = ratio[3] <= 0 ? 0 : 2 + ratio[3] * 7 / 1000;    /* KNOB 4: sparks off the floor */
    for (i = 0; op_hit > 4u && i < n; i++) {
        int32_t fx = cx + (i & 1 ? 30 : -30), d = (9 - (int32_t)op_hit) * 3;
        rnd = rnd * 1664525u + 1013904223u;
        x = (int32_t)((rnd >> 8) % 21u) - 10;
        cv_line(fx + x, 118 - d / 2, fx + x * 2, 116 - d - (int32_t)((rnd >> 20) % 5u), tint_col(3u, 0));
    }
#undef WY
}

/* synth 2, the alien (ELEMENTS): it dances to the beat (op_dance) between two speakers; a note: a little hop, and a
 * speaker thumps (they take turns). KNOB 1 the size of the speakers, KNOB 2 strings of lights from above, swinging
 * with the sound, KNOB 3 a moon that rises, KNOB 4 the rings of sound off the speaker */
static void op_scene_alien(const int32_t ratio[4])
{
    static uint8_t prev, side;                          /* the speaker that thumped last */
    int32_t big = ratio[0] < 0 ? 0 : ratio[0] * 6 / 1000, x, i, n, s, hop = op_hit > 5u ? 2 : 0;
    uint32_t pose;
    int flip;
    if (op_hit > prev)
        side ^= 1u;
    prev = op_hit;
    pose = op_dance(&flip);
    for (x = 8; x < 232; x += 8)
        cv_pset(x, OP_FLOOR_Y, OP_PUR_D);
    if (ratio[2] >= 0)                                  /* KNOB 3: the moon */
        op_ell(222 - (op_px ? 146 : 0), 70 - ratio[2] * 34 / 1000, 5, 5, tint_col(2u, 0));
    n = ratio[1] < 0 ? 0 : 1 + ratio[1] * 5 / 1000;     /* KNOB 2: strings of lights from above, swinging with the sound */
    for (i = 0; i < n; i++) {
        static const uint8_t VX[6] = {62, 190, 40, 212, 20, 230};
        uint32_t h = (uint32_t)(i + 1) * 2246822519u;
        int32_t len = 16 + (int32_t)((h >> 9) % 24u), sw = op_lvl ? (SINE[(op_ph / 2u + (uint32_t)i * 200u) & 1023u] * 4) >> 15 : 0, y0 = VX[i] < 56 ? 58 : 20, k;
        x = VX[i];
        if (op_px && x > 150)
            y0 = 46;
        cv_line(x, y0, x + sw, y0 + len, tint_col(1u, 0));
        for (k = 1; k <= 3; k++)                        /* its bulbs */
            cv_rect(x + sw * k / 3 - 1, y0 + len * k / 3 - 1, 3, 3, tint_col(1u, 0));
    }
    op_char(1, pose, flip, 64, OP_SPR_Y - hop);
    for (s = -1; s <= 1; s += 2) {                      /* KNOB 1: the speakers, at the sides */
        int32_t dx = 120 + s * 82, hw = 11 + big, top = 88 - big * 2, hot = op_hit > 3u && (s > 0) == (side != 0);
        int32_t r = 6 + big / 2;
        cv_rect(dx - hw, top, hw * 2 + 1, 122 - top, C_BLACK);
        op_box(dx - hw, top, hw * 2 + 1, 122 - top, tint_col(0u, 0));
        op_ell(dx, top + 7, 2, 2, tint_col(0u, 0));                           /* the tweeter */
        op_ell(dx, 112 - r, r + (hot ? 1 : 0), r + (hot ? 1 : 0), hot ? C_WHITE : tint_col(0u, 0));   /* the cone */
        op_ell(dx, 112 - r, 2, 2, hot ? C_WHITE : tint_col(0u, 0));
        if (op_hit && hot && ratio[3] > 0) {            /* KNOB 4: the rings, towards the middle */
            int32_t rr = 8 + (8 - (int32_t)op_hit) * (2 + ratio[3] * 3 / 1000);
            op_arc(dx, 112 - r, rr + r, rr + r, s > 0 ? 384 : 896, s > 0 ? 640 : 1152, tint_col(3u, 0));
            if (ratio[3] > 500 && rr > 14)
                op_arc(dx, 112 - r, rr + r - 7, rr + r - 7, s > 0 ? 384 : 896, s > 0 ? 640 : 1152, tint_col(3u, 0));
        }
    }
}

/* synth 3, the crocodile (PLAITS): it dances to the beat (op_dance) under a spotlight, on a floor of lights, among
 * the stars; with sound the lights run along the floor and the stars pass, faster the louder; a note: a little hop,
 * the spotlight flashes and the floor lights all light. KNOB 1 the spotlight's width, KNOB 2 a planet that grows,
 * KNOB 3 more stars, KNOB 4 sparks rising in the light */
static void op_scene_croc(const int32_t ratio[4])
{
    int32_t i, n, bw, live = op_lvl || op_hit, hop = op_hit > 5u ? 2 : 0, cx = op_px ? 92 : 120;   /* (left of PRISM's picture) */
    uint32_t pose;
    int flip;
    pose = op_dance(&flip);
    n = 8 + (ratio[2] < 0 ? 0 : ratio[2] * 26 / 1000);  /* KNOB 3: the stars, three depths, some twinkling */
    for (i = 0; i < n; i++) {
        uint32_t h = (uint32_t)(i + 1) * 2654435761u;
        int32_t x = (int32_t)(((h >> 4) % 240u + 240u * 64u - (op_ph >> (3u + (uint32_t)i % 3u)) % 240u) % 240u);
        int32_t y = 22 + (int32_t)((h >> 14) % 90u), big = i % 5 == 0 && ((ui.frame / 8u + (uint32_t)i) & 3u);
        cv_rect(x, y, 1 + big, 1, tint_col(2u, 0));
        if (big && i % 10 == 0) {
            cv_pset(x, y - 1, tint_col(2u, 0));
            cv_pset(x, y + 1, tint_col(2u, 0));
        }
    }
    if (ratio[1] >= 0) {                                /* KNOB 2: the planet and its ring */
        int32_t r = 5 + ratio[1] * 10 / 1000, px = 206, py = 96;
        op_ell(px, py, r, r, tint_col(1u, 0));
        op_arc(px, py, r + 7, 3, 0, 512, tint_col(1u, 0));
        cv_pset(px - r - 7, py, tint_col(1u, 0));
        cv_pset(px + r + 7, py, tint_col(1u, 0));
        op_arc(px - 2, py - 2, r * 2 / 3, r * 2 / 3, 576, 768, tint_col(1u, 0));
    }
    bw = 30 + (ratio[0] < 0 ? 0 : ratio[0] * 30 / 1000);   /* KNOB 1: the spotlight, from above: bright on a note */
    {
        uint16_t c = op_hit > 5u ? C_WHITE : op_hit || op_lvl ? tint_col(0u, 0) : tint_col(0u, 2);
        cv_line(cx - 6, 22, cx - bw, 120, c);
        cv_line(cx + 6, 22, cx + bw, 120, c);
        op_ell(cx, 120, bw, 3, c);
    }
    n = ratio[3] <= 0 || !live ? 0 : 2 + ratio[3] * 8 / 1000;   /* KNOB 4: the sparks, rising in the light */
    for (i = 0; i < n; i++) {
        uint32_t h = (uint32_t)(i + 1) * 2246822519u;
        int32_t up = (int32_t)(((h >> 7) + op_ph / 12u) % 64u), y = 120 - up * 96 / 64, half = 6 + (bw - 6) * (y - 22) / 98;
        cv_rect(cx - half + (int32_t)((h >> 3) % (uint32_t)(half * 2 + 1)), y, 2, 2, tint_col(3u, 0));
    }
    op_char(2, pose, flip, cx - 56, OP_SPR_Y - hop);
    for (i = 0; i < 7; i++) {                           /* the floor lights: one runs along; all of them on a note */
        int on = op_hit > 4u || (live && (uint32_t)i == (op_ph / 160u) % 7u);
        cv_rect(24 + i * 30, OP_FLOOR_Y, 12, 2, on ? tint_col(3u, 0) : tint_col(3u, 2));
    }
}

/* PRISM: the model's own picture, 80 x 40 at the top right, from the three controls as they stand (not from the
 * sound: it shows what the knobs set). FOLD and WAVE: a cycle of the wave, made by the model itself; HARM: its 24
 * harmonics; SWARM: the eight oscillators, as far apart as HARM spreads them, as long as MRPH makes their grains;
 * CHORD: its notes on an octave of keys (green: CHORD's knob), the inversion under it (white: INVRT's). Kept until
 * a control moves. */
static void op_prism_panel(const track_t *t, int32_t x0, int32_t y0, int32_t k)   /* (at x0, y0, k times as big) */
{
    static int8_t pts[80];
    static uint32_t key = 0xFFFFFFFFu;
    static const uint16_t TONES[11] = {0x001, 0x081, 0x0A1, 0x089, 0x489, 0x40D, 0x429, 0x215, 0x815, 0x891, 0x091};
    static const uint8_t WHITE[7] = {0, 2, 4, 5, 7, 9, 11}, BLACK[5] = {1, 3, 6, 8, 10}, BX[5] = {7, 17, 37, 47, 57};
    uint32_t model = prism_model(t), kk = (uint32_t)t->p[P_E0] | (uint32_t)t->p[P_E1] << 4 | (uint32_t)t->p[P_E2] << 12 |
                                          (uint32_t)t->p[P_E3] << 20, i;
    int32_t h = t->p[P_E1] * 258, tb = t->p[P_E2] * 258, mo = t->p[P_E3] * 258, x;
#define PK(v) ((v) * k)
    if (model == PRM_FOLD || model == PRM_WAVE || model == PRM_HARM) {
        if (kk != key) {                                /* a control moved: the picture again */
            int16_t b[32];
            key = kk;
            if (model == PRM_HARM) {
                pr_harm_t hs;
                int32_t top = 1;
                pr_harm_init(&hs);
                for (i = 0; i < 60u; i++)               /* (her spectrum takes some blocks to settle) */
                    pr_harm_render(&hs, 0x01000000u, h, tb, mo, b, 1);
                for (i = 0; i < PR_HARM_N; i++)
                    top = hs.amp[i] > top ? hs.amp[i] : top;
                for (i = 0; i < PR_HARM_N; i++) {       /* (the square root of each: the quiet ones show too) */
                    uint32_t r = (uint32_t)hs.amp[i] * 1296u / (uint32_t)top, q = 0;
                    while ((q + 1u) * (q + 1u) <= r)
                        q++;
                    pts[i] = (int8_t)q;
                }
            } else {                                    /* two cycles of 160 samples, the second one kept */
                pr_fold_t fs;
                pr_wave_t ws;
                uint32_t blk;
                pr_fold_init(&fs);
                pr_wave_init(&ws);
                for (blk = 0; blk < 10u; blk++) {
                    if (model == PRM_FOLD)
                        pr_fold_render(&fs, 0x0199999Au, h, tb, mo, b, 32);
                    else
                        pr_wave_render(&ws, 0x0199999Au, h, tb, mo, b, 32);
                    for (i = 0; blk >= 5u && i < 32u; i += 2u)
                        pts[(blk - 5u) * 16u + i / 2u] = (int8_t)(b[i] * 18 / 32768);
                }
            }
        }
        if (model == PRM_HARM) {
            for (i = 0; i < PR_HARM_N; i++)
                cv_rect(x0 + PK(4 + (int32_t)i * 3), y0 + PK(38 - pts[i]), PK(2), PK(pts[i] + 1), tint_col(0u, 0));
        } else {
            for (x = 0; x < 80; x += 4)
                cv_pset(x0 + PK(x), y0 + PK(20), OP_PUR_D);
            for (i = 1; i < 80u; i++) {
                cv_line(x0 + PK((int32_t)i - 1), y0 + PK(20 - pts[i - 1]), x0 + PK((int32_t)i), y0 + PK(20 - pts[i]), tint_col(0u, 0));
                if (k > 1)
                    cv_line(x0 + PK((int32_t)i - 1), y0 + PK(20 - pts[i - 1]) - 1, x0 + PK((int32_t)i), y0 + PK(20 - pts[i]) - 1,
                            tint_col(0u, 0));
            }
        }
    } else if (model == PRM_SWARM) {
        int32_t spread = h, len = 8 + mo * 30 / 32767, n = 2 + tb * 10 / 32767;   /* (HARM as it is: its cube is the pitch) */
        for (i = 0; i < 8u; i++) {                      /* (their ranks: -1 .. 1.33, as the model's) */
            int32_t y = y0 + PK(22 - (((int32_t)i - 3) * spread * 13 / 3) / 32767);
            cv_line(x0 + PK(40 - len), y + PK(2), x0 + PK(40 + len), y - PK(2), tint_col(0u, 0));
            cv_line(x0 + PK(40 + len), y - PK(2), x0 + PK(40 + len), y + PK(2), tint_col(0u, 0));
        }
        for (i = 0; i < (uint32_t)n; i++)               /* TIMB: the grains' rate */
            cv_rect(x0 + PK(2 + (int32_t)i * 6), y0, PK(3), PK(2), tint_col(2u, 0));
    } else {                                            /* CHORD */
        uint32_t tones = TONES[(uint32_t)t->p[P_E1] * 11u / 128u];
        for (i = 0; i < 7u; i++) {
            int32_t on = (tones >> WHITE[i]) & 1u;
            if (on)
                cv_rect(x0 + PK(5 + (int32_t)i * 10), y0 + PK(2), PK(9), PK(30), WHITE[i] ? tint_col(1u, 0) : C_WHITE);
            else
                op_box(x0 + PK(5 + (int32_t)i * 10), y0 + PK(2), PK(9), PK(30), OP_PUR);
        }
        for (i = 0; i < 5u; i++) {
            int32_t on = (tones >> BLACK[i]) & 1u;
            cv_rect(x0 + PK(5 + BX[i]), y0 + PK(2), PK(5), PK(17), on ? tint_col(1u, 0) : C_BLACK);
            if (!on)
                op_box(x0 + PK(5 + BX[i]), y0 + PK(2), PK(5), PK(17), OP_PUR);
        }
        for (i = 0; i < 20u; i++)                       /* INVRT: twenty steps up */
            cv_rect(x0 + PK(1 + (int32_t)i * 4), y0 + PK(36), PK(3), PK(2),
                    (int32_t)i <= t->p[P_E2] * 20 / 128 ? tint_col(2u, 0) : OP_PUR_D);
    }
#undef PK
}

static void op_synth_scene(track_t *t, const int32_t ratio[4], int analog_wave)
{
    int32_t m = meter_px(t->peak);
    char b[8];
    op_px = t->eng_req % NENGINES == ENGI_PRISM;
    t->peak = 0;
    op_lvl = m > op_lvl ? m : op_lvl > 4 ? op_lvl - 4 : 0;
    if (op_lvl || op_hit)
        op_ph += 40u;
    op_names(t);
    note_name(b, op_note);                              /* the last note, large */
    cv_text(4, 22, &FONT_L, b, op_hit > 5u ? C_WHITE : TE_G4);
    switch ((uint32_t)(t - trk) % 3u) {
    case 0: op_scene_robot(t, ratio, analog_wave); break;
    case 1: op_scene_alien(ratio); break;
    default: op_scene_croc(ratio); break;
    }
    if (op_px)
        op_prism_panel(t, 152, 22, 1);
}

/* the pages of the family, the one shown filled (one page: nothing) */
static void op_pages(uint32_t k, uint32_t n, uint32_t *cache)
{
    uint32_t i, sig = 1u + k + n * 16u;
    if (!ui.force && sig == *cache)
        return;
    *cache = sig;
    cv_begin(240, 8, C_BLACK);
    for (i = 0; i < n && n > 1u; i++) {
        if (i + 1u == k)
            cv_rect(4 + (int32_t)i * 9, 1, 6, 6, C_WHITE);
        else
            op_box(4 + (int32_t)i * 9, 1, 6, 6, OP_PUR);
    }
    cv_blit(0, 146);
}

/* the sound chain's signal path (Ben, 20 Nov 2026: the MiniFreak's flow): OSC > FILT > ENV > FX > MOD, always all five
 * in the same order, in the voice's colour, a stage the voice does not have dark. The stage shown is filled and holds
 * the name of the page shown (Ben, 21 Nov: "Keep, name the pages": FILTER 2, LFO SYNC, so a one-knob page says what it
 * is); at the right its place among the stage's pages, "2/2", when it has more than one. The bottom rows of the graph
 * above it give way (draw_graph) */
#define OP_CHAIN_H 13                                   /* (a row apart from the graph, then 12) */
#define OP_CHAIN_Y (OP_KNOB_Y - OP_CHAIN_H)
static void op_chain(uint32_t st, uint32_t have, uint32_t k, uint32_t n, uint32_t *cache)
{
    uint32_t v = song.sel & 3u, s, sig = 0x51u + st + have * 8u + k * 256u + n * 4096u + v * 65536u;
    const char *pn = cur_page()->title;
    char pos[6] = {(char)('0' + k % 10u), '/', (char)('0' + n % 10u), 0};
    int32_t x = 3, w;
    sig = studio_hash(sig, pn);
    if (!ui.force && sig == *cache)
        return;
    *cache = sig;
    for (s = 0, w = 0; s < NSTAGE; s++)                 /* (a name too long for the line: the stage's own) */
        w += text_w(&FONT_S, s == st ? pn : STAGE_NAME[s]) + 4 + (s ? 4 : 0);
    if (w + 3 + (n > 1u ? 5 + text_w(&FONT_S, pos) : 0) > 237)
        pn = STAGE_NAME[st];
    cv_begin(240, OP_CHAIN_H, C_BLACK);
    for (s = 0; s < NSTAGE; s++) {
        const char *nm = s == st ? pn : STAGE_NAME[s];
        w = text_w(&FONT_S, nm) + 4;
        if (s) {                                        /* the arrow between two stages */
            cv_line(x + 1, 4, x + 2, 6, TE_G2);
            cv_line(x + 2, 7, x + 1, 9, TE_G2);
            x += 4;
        }
        if (s == st)
            cv_rect(x, 1, w, OP_CHAIN_H - 1, TE_COL[v]);
        cv_text(x + 2, 0, &FONT_S, nm, s == st ? C_BLACK : (have >> s) & 1u ? TE_MID[v] : TE_G2);
        x += w;
    }
    if (n > 1u)                                         /* the page's place in the stage, right-aligned */
        cv_text(238 - text_w(&FONT_S, pos), 0, &FONT_S, pos, TE_MID[v]);
    cv_blit(0, OP_CHAIN_Y);
}

/* the four tracks' meters, for the screens that show the whole band (the mixer, HOME), once a frame: up at
 * once, falling; a rise: the track's character hops for 6 frames */
static uint8_t op_mx[NTRK], op_hop[NTRK];
static void op_band_tick(void)
{
    uint32_t i;
    for (i = 0; i < NTRK; i++) {
        int32_t pk = i == TRK_DRUM ? drums.peak : trk[i].peak, m;
        if (i == TRK_DRUM)
            drums.peak = 0;
        else
            trk[i].peak = 0;
        m = trk_silent(&trk[i]) ? 0 : meter_px(pk);
        if (m > op_mx[i] + 8)
            op_hop[i] = 6;
        else if (op_hop[i])
            op_hop[i]--;
        op_mx[i] = (uint8_t)(m > op_mx[i] ? m : op_mx[i] > 3 ? op_mx[i] - 3 : 0);
    }
}
/* a track's character for the mixer (op_k 2: dancing, pose 0 still or 1 leaning, flip: the other way) or HOME's
 * lanes (op_k 1: the head), standing on (cx, fy), dim if op_ink is (a muted, a silent track) */
static void op_band_member(uint32_t i, int32_t cx, int32_t fy, uint32_t pose, int flip, uint16_t c)
{
    uint32_t sm = op_k == 1, k = i & 3u;
    int32_t w = sm ? OP_HM_W : OP_MX_W, h = sm ? OP_HM_H : OP_MX_H;
    op_dim = op_ink == OP_PUR_D;
    op_sprite_wh(sm ? OP_SPR_HM[k] : OP_SPR_MX[k][op_strike[k] ? 2u : pose & 1u], w, h, cx - w / 2, fy - h + 1, flip, c, 1);
    op_dim = 0;
}

/* ---------------------------------------------------------------- ARP --- */
/* the track's character at half size (its own pixels: 56 x 52), still, standing on (cx, fy) */
static void op_sprite_scaled(uint32_t ti, int32_t cx, int32_t fy, uint16_t c)
{
    op_sprite_wh(OP_SPR_BIG[ti & 3u][0], OP_BIG_W, OP_BIG_H, cx - OP_BIG_W / 2, fy - OP_BIG_H + 1, 0, c, 1);
}

/* the ARP page: the arp as stepping stones, one a note in the order it plays them (the keys held, else C E G to
 * show the shape), high for a high note; the track's character hops to the stone of each note it plays. KNOB 1
 * (MODE, blue) the stones' order: up, down, up and down, shuffled, as played; KNOB 2 (RATE, green) the grid under
 * them; KNOB 3 (OCT, white) the octaves, a dashed line between them; KNOB 4 (GATE, orange) how much of each stone
 * is lit */
static uint8_t op_arp_hop;                             /* frames of the hop still to go (graph_signature) */
static void op_arp_scene(const track_t *t)
{
    static uint8_t pos, prev, last;
    static const uint8_t TICKS[6] = {4, 8, 16, 32, 6, 12};
    uint8_t base[16], seq[32];
    uint32_t mode = (uint32_t)clamp(t->p[P_AMODE], 0, 5), oct = (uint32_t)clamp(t->p[P_AOCT], 1, 4), nb = 0, n = 0, i, j, k;
    int32_t gate = clamp(t->p[P_AGATE], 1, 127), lo = 127, hi = 0, sp, x0, cx, fy, live = t->nheld && mode;
    uint32_t ti = (uint32_t)(t - trk) & 3u;
    for (i = 0; i < t->nheld && nb < 16u; i++)
        base[nb++] = t->held[i];
    if (!nb) {                                      /* nothing held: the shape on a C major chord */
        base[0] = 60, base[1] = 64, base[2] = 67;
        nb = 3;
    }
    if (mode != 5u)                                 /* (ORD: as pressed; the others from the lowest) */
        for (i = 1; i < nb; i++)
            for (j = i; j > 0 && base[j - 1] > base[j]; j--) {
                uint8_t s = base[j];
                base[j] = base[j - 1];
                base[j - 1] = s;
            }
    for (k = 0; k < oct; k++)
        for (i = 0; i < nb && n < 24u; i++)
            seq[n++] = (uint8_t)clamp(base[i] + 12 * (int32_t)k, 0, 127);
    if (mode == 2u)                                 /* DN */
        for (i = 0; i < n / 2u; i++) {
            uint8_t s = seq[i];
            seq[i] = seq[n - 1u - i];
            seq[n - 1u - i] = s;
        }
    else if (mode == 3u && n > 2u)                  /* UPDN: up, then down without the ends twice */
        for (i = n - 2u; i > 0 && n < 32u; i--)
            seq[n++] = seq[i];
    else if (mode == 4u) {                          /* RND: a shuffle (a new one each round would flicker: one) */
        uint32_t r = 0x9E3779B9u * (n + 7u);
        for (i = n - 1u; i > 0; i--) {
            uint8_t s;
            r = r * 1664525u + 1013904223u;
            j = (r >> 8) % (i + 1u);
            s = seq[i];
            seq[i] = seq[j];
            seq[j] = s;
        }
    }
    if (n > 16u)
        n = 16u;                                    /* (as many stones as fit) */
    for (i = 0; i < n; i++) {
        lo = seq[i] < lo ? seq[i] : lo;
        hi = seq[i] > hi ? seq[i] : hi;
    }
    if (hi - lo < 12)
        hi = lo + 12;
#define AY(nt) (112 - ((int32_t)(nt) - lo) * 34 / (hi - lo))
    sp = 216 / (int32_t)n;
    x0 = 12 + (216 - sp * (int32_t)n) / 2;
    for (k = 1; k * 12u <= (uint32_t)(hi - lo); k++)      /* KNOB 3: the octaves */
        for (i = 8; i < 232u; i += 6u)
            cv_pset((int32_t)i, AY(lo + 12 * (int32_t)k) + 3, tint_col(2u, 0));
    for (i = 0; i < TICKS[(uint32_t)t->p[P_ARATE] % 6u]; i++)   /* KNOB 2: the grid */
        cv_rect(12 + (int32_t)i * 216 / TICKS[(uint32_t)t->p[P_ARATE] % 6u], 120, 2, 2, tint_col(1u, 0));
    if (live && t->arp_note && t->arp_note != last) {   /* a new note: to its stone (the next one with it) */
        for (k = 1; k <= n; k++)
            if (seq[(pos + k) % n] == t->arp_note)
                break;
        if (k <= n) {
            prev = pos;
            pos = (uint8_t)((pos + k) % n);
            op_arp_hop = 6;
        }
    }
    last = live ? t->arp_note : 0;
    if (!live)
        pos = prev = 0;
    if (pos >= n)
        pos = prev = 0;
    for (i = 0; i < n; i++) {                       /* KNOB 1: the stones; KNOB 4: how much of each is lit */
        int32_t x = x0 + (int32_t)i * sp + 1, y = AY(seq[i]), w = sp - 3, lit = w * gate / 127;
        uint16_t c = !mode ? OP_PUR_D : tint_col(0u, 0);
        if (i == pos && live && op_arp_hop > 3u)
            cv_rect(x, y, w, 4, C_WHITE);
        else {
            op_box(x, y, w, 4, c);
            if (mode)
                cv_rect(x + 1, y + 1, lit > 2 ? lit - 2 : 1, 2, i == pos && live ? C_WHITE : tint_col(3u, 0));
        }
    }
    {                                               /* the character: on its stone, or on the way there */
        int32_t xa = x0 + (int32_t)prev * sp + sp / 2, ya = AY(seq[prev]) - 1;
        int32_t xb = x0 + (int32_t)pos * sp + sp / 2, yb = AY(seq[pos]) - 1, u = 6 - (int32_t)op_arp_hop;
        cx = op_arp_hop ? xa + (xb - xa) * u / 6 : xb;
        fy = op_arp_hop ? ya + (yb - ya) * u / 6 - (SINE[u * 512 / 6] * 12 >> 15) : yb;
        op_sprite_scaled(ti, cx, fy, TE_COL[ti]);
    }
    if (op_arp_hop)
        op_arp_hop--;
    if (!mode)
        cv_text(170, 24, &FONT_S, "ARP OFF", TE_G3);
    else if (!t->nheld)
        cv_text(150, 24, &FONT_S, "HOLD KEYS", TE_G3);
#undef AY
}

/* ---------------------------------------------------------------- HEX --- */
/* the drum track's EDIT pages: the sound of the selected lane, played once into a picture (its first 0.75 s, the
 * louder half of each column), redrawn when it changes; the sound's name and its kit */
static void op_hex_scene(void)
{
    static int8_t col[200][2];
    static uint32_t made = 0xFFFFFFFFu;
    uint32_t kit = drum_kit(), i, k;
    const hex_snd_t *kits;
    if (kit >= DRUM_SAMPLED && kit < DRUM_SYN) {
        cv_text(28, 50, &FONT_S, "A HEX KIT ONLY:", TE_G3);
        cv_text(28, 68, &FONT_S, "PRESETS: KIT 1-5", TE_G3);
        return;
    }
    kits = kit < DRUM_SAMPLED ? HEX_KITS[kit].s : dsu_kit(kit - DRUM_SYN)->s;
    if (made != hex_gen * 64u + kit * 16u + drum_lane) {
        static hex_v_t v;
        static int32_t buf[166];
        hex_on(&v, kits, LANE_NOTE[drum_lane % DRUM_LANES], 110);
        for (i = 0; i < 200u; i++) {
            int32_t lo = 0, hi = 0;
            hex_render(&v, buf, 166);
            for (k = 0; k < 166u; k++) {
                lo = buf[k] < lo ? buf[k] : lo;
                hi = buf[k] > hi ? buf[k] : hi;
            }
            col[i][0] = (int8_t)clamp(lo / 1300, -32, 0);
            col[i][1] = (int8_t)clamp(hi / 1300, 0, 32);
        }
        made = hex_gen * 64u + kit * 16u + drum_lane;
    }
    for (i = 0; i < 200u; i += 4u)
        cv_pset(20 + (int32_t)i, 70, OP_PUR_D);
    for (i = 0; i < 200u; i++)
        if (col[i][1] - col[i][0] > 0)
            cv_line(20 + (int32_t)i, 70 - col[i][1], 20 + (int32_t)i, 70 - col[i][0], i < 6u ? C_WHITE : TE_DRUM);
    cv_text(4, 2, &FONT_S, LANE_NAME[drum_lane % DRUM_LANES], C_WHITE);
    cv_text(4 + text_w(&FONT_S, LANE_NAME[drum_lane % DRUM_LANES]) + 8, 2, &FONT_S, DRUM_KIT_NAMES[kit], TE_G3);
    cv_text(4, 106, &FONT_S, "KEY: PICK SOUND", TE_G2);
}

/* ------------------------------------------------------------------- FX --- */
/* ---------------------------------------------------------- SOUND PAGES --- */
/* (Ben, 21 Nov 2026, review 8: "Sound big, character small") a sound page (OSC, VOICE, FILTER, the DEST pages, FX,
 * CHORUS, INSERT) draws what its four knobs do, big, as they turn, in the voice's colour (tint_col: a shared page
 * neutral): the wave the oscillator makes, the filter's response, the sends, the INSERT's curve, the voice's ways. Its
 * character small (the mixer's size) at the top right, still dancing and striking; the sound's name at the top left.
 * MiniFreak's rule: the picture shows the consequence the instant a knob moves, not the number. One 2 px line
 * helper (gp_to) draws every curve in the plot SP_L..SP_R x SP_T..SP_B (graph canvas rows) */
#define SP_L 8
#define SP_R 232
#define SP_T 40
#define SP_B 114
#define SP_M ((SP_T + SP_B) / 2)
static int32_t gp_x, gp_y;
static uint8_t gp_go;                                   /* 0: the next point starts a line */
static void gp_to(int32_t x, int32_t y, uint16_t c)
{
    y = clamp(y, SP_T, SP_B);
    if (gp_go) {
        cv_line(gp_x, gp_y, x, y, c);
        cv_line(gp_x, gp_y - 1, x, y - 1, c);
    }
    gp_x = x, gp_y = y, gp_go = 1;
}
static void gp_dots(int32_t y)                          /* a dotted rule across the plot */
{
    int32_t x;
    for (x = SP_L; x <= SP_R; x += 4)
        cv_pset(x, y, OP_PUR_D);
}
/* a filter's response in px down from 0 dB at x: cutoff at xc, resonance r px of peak, k/8 px down a px past it, hp */
static int32_t gp_resp(int32_t x, int32_t xc, int32_t r, int32_t k, int hp)
{
    int32_t d = hp ? xc - x : x - xc, v = d > 0 ? d * k / 8 : 0;
    if (d > -16 && d < 16)
        v -= r * (256 - d * d) / 256;
    return v;
}
/* the filter page: the response (cutoff 0..127, resonance 0..127, the slope k), the envelope's sweep (env 0..127) as a
 * shaded wedge out to where it opens, fading as fast as dec (0..127, -1: no decay shown), and the track's own LP / HP
 * filter (tf, -64..63) on top of it */
static void gp_filter(int32_t cut, int32_t res, int32_t env, int32_t dec, int32_t k, int32_t tf, uint16_t c)
{
    const int32_t y0 = SP_T + 18, w = SP_R - SP_L - 20;
    int32_t xc = SP_L + 10 + cut * w / 127, xe = clamp(xc + env * w / 2 / 127, SP_L, SP_R), x, h = (SP_B - y0) << 8;
    int32_t xt = tf < 0 ? SP_R - (-tf) * (SP_R - SP_L) / 64 : SP_L + tf * (SP_R - SP_L) / 63;
    gp_dots(y0);
    for (x = xe; x > xc; x -= 3) {                      /* the sweep: from where the envelope opens it, back */
        cv_rect(x, SP_B - (h >> 8), 1, h >> 8, tint_col(2u, 2));
        if (dec >= 0)
            h -= h * 3 / (8 + dec);
    }
    gp_go = 0;
    for (x = SP_L; x <= SP_R; x += 2)
        gp_to(x, y0 + gp_resp(x, xc, res * 26 / 127, k, 0) + (tf ? gp_resp(x, xt, 0, 5, tf > 0) : 0), c);
    cv_rect((tf && cut == 127 ? xt : xc) - 1, SP_B - 3, 3, 4, C_WHITE);   /* where the cutoff is */
}
/* the oscillators: two cycles across the plot (x: 0..2047 a phase), each engine's own way */
static int32_t gp_noise(int32_t x) { uint32_t h = (uint32_t)x * 2654435761u; h ^= h >> 13; h *= 2246822519u; return (int32_t)(h >> 16) - 32768; }
static void gp_osc(const track_t *t, uint16_t c)
{
    const int16_t *p = t->p;
    uint32_t e = t->eng_req % NENGINES, m = e == ENGI_PRISM ? prism_model(t) : 0u;
    int32_t x, a = SP_M - SP_T - 4;
    gp_dots(SP_M);
    if (e == ENGI_PRISM && m < PRM_PLX) {               /* FOLD, WAVE, HARM, SWARM, CHORD: the model's own picture, big */
        op_prism_panel(t, 40, SP_T - 4, 2);
        return;
    }
    gp_go = 0;
    for (x = SP_L; x <= SP_R; x += 2) {
        uint32_t ph = (uint32_t)(x - SP_L) * 2048u / (SP_R - SP_L), q = ph & 1023u;
        int32_t saw = (int32_t)q * 64 - 32768, v;
        if (e == ENGI_JUNO) {                           /* WAVE (saw, pulse, both), PW, SUB an octave down, NOISE */
            int32_t wv = clamp(p[P_E0], 0, 2), pw = 512 + clamp(p[P_E1], 0, 127) * 460 / 127;
            int32_t sub = clamp(p[P_E2], 0, 127), noi = clamp(p[P_E3], 0, 127);
            int32_t sum = (wv != 1 ? saw * 127 : 0) + (wv ? ((int32_t)q < pw ? 32767 : -32767) * 127 : 0) +
                          (ph & 1024u ? -32767 : 32767) * sub + gp_noise(x) * noi;
            v = sum / ((wv == 2 ? 254 : 127) + sub + noi);
        } else if (e == ENGI_ACID) {                    /* WAVE (saw, square) through DRV */
            v = p[P_E0] ? ((int32_t)q < 512 ? 32767 : -32767) : saw;
            v = softclip(v * (8 + p[P_E6] / 4) / 8) * 5 / 6;
        } else if (e == ENGI_PRISM && m == PRM_PLX) {   /* PLAITS VA (a schematic): MRPH a triangle to a saw (then a
                                                         * notch), TIMB a pulse from narrow to square, HARM a second
                                                         * wave drifting off, SUB an octave down */
            int32_t mo = p[P_E3], tb = p[P_E2];
            uint32_t q2 = (q + (uint32_t)((p[P_E1] - 64) * (int32_t)(ph >> 4) / 64)) & 1023u;
            int32_t tri = q2 < 512u ? (int32_t)q2 * 128 - 32768 : 32767 - ((int32_t)q2 - 512) * 128;
            v = tri + ((int32_t)q2 * 64 - 32768 - tri) * (mo > 64 ? 64 : mo) / 64;
            if (mo > 64 && q2 + (uint32_t)(mo - 64) * 6u > 512u && q2 < 512u)
                v = 0;
            v = (v * 2 + ((int32_t)q < 40 + tb * 4 ? 32767 : -32767) * (tb > 16 ? 16 : tb) / 16) / 3;
            v += p[P_E5] * (ph & 1024u ? -128 : 128);
        } else if (e == ENGI_PRISM) {                   /* the other PLAITS models: brighter with TIMB, HARM the cycles */
            v = op_wave((uint32_t)(ph * (2u + (uint32_t)p[P_E1] / 32u) / 2u), 5u, p[P_E2] * 8);
            v = v * (128 - p[P_E3] / 3) / 128 + op_wave(ph * 2u, 3u, 0) * p[P_E3] / 384;
        } else {                                        /* ELEMENTS (a schematic): a struck body ringing down, BRIT its
                                                         * edge, DAMP how soon it dies, STRC its partials */
            int32_t env = p[P_E0] == EM_BOW || p[P_E0] == EM_BLOW ? 32767   /* (BOW, BLOW: held on; the others ring down) */
                        : 32767 - (x - SP_L) * (32 + (127 - clamp(p[P_E3], 0, 127)) / 2) * 4;
            v = op_wave((uint32_t)(x - SP_L) * (24u + (uint32_t)p[P_E1] / 8u), 5u, p[P_E2] * 8);
            v = (int32_t)((int64_t)v * (env < 0 ? 0 : env) >> 15);
        }
        gp_to(x, SP_M - v * a / 32768, c);
    }
    if (e == ENGI_ACID) {                               /* ACC and SLIDE, small, top left: a note, then one slid up
                                                         * to (as long as SLIDE), accented (taller, as ACC) */
        int32_t sl = p[P_E7] * 24 / 127, ac = 3 + p[P_E5] * 6 / 127;
        cv_rect(SP_L, 35, 24, 4, tint_col(1u, 1));
        gp_go = 0;
        gp_to(SP_L + 24, 37, tint_col(2u, 0));
        gp_to(SP_L + 26 + sl, 30, tint_col(2u, 0));
        cv_rect(SP_L + 26 + sl, 30 - ac, 24, ac, c);
    }
}
/* the four knobs as bars under their columns (OSC 2, the DEST pages, VOICE 2), each from its value; a value that goes
 * below 0 (the DEST amounts, PAN) from the middle */
static void gp_bars(const track_t *t, const page_t *pg)
{
    uint32_t k;
    for (k = 0; k < 4u; k++) {
        const param_desc_t *d;
        int32_t x = 6 + (int32_t)k * 60, h, y, bi;
        if (pg->id[k] >= P_COUNT || (d = track_desc(t, pg->id[k]))->max <= d->min)
            continue;
        bi = d->min < 0;
        op_box(x, SP_T, 48, SP_B - SP_T + 1, tint_col(k, 2));
        h = (t->p[pg->id[k]] - (bi ? 0 : d->min)) * (SP_B - SP_T) / (d->max - (bi ? 0 : d->min)) / (bi ? 2 : 1);
        y = bi ? SP_M - (h > 0 ? h : 0) : SP_B - h;
        cv_rect(x + 3, y, 42, h < 0 ? -h : h > 0 ? h : 1, tint_col(k, 0));
        if (bi)
            cv_rect(x, SP_M, 48, 1, OP_PUR);
    }
}
/* FX: the four sends as they sound, each in a box under its knob, as strong as the knob: DIST a wave clipped harder,
 * CHORUS a second voice further off, DELAY its echoes, longer lasting, REVERB its tail */
static void gp_sends(const track_t *t, uint16_t c)
{
    uint32_t k;
    int32_t x, i;
    for (k = 0; k < 4u; k++) {
        int32_t x0 = 6 + (int32_t)k * 60, a = clamp(t->p[P_DIST + k], 0, 127), h = (SP_B - SP_T - 8) << 8;
        op_box(x0, SP_T, 48, SP_B - SP_T + 1, tint_col(k, 2));
        for (i = k == 1u ? 1 : 0; i >= 0; i--) {        /* DIST, CHORUS: a cycle (CHORUS: and its second voice) */
            if (k > 1u)
                break;
            gp_go = 0;
            for (x = 0; x <= 40; x += 2) {
                int32_t v = SINE[((uint32_t)x * 25u + (uint32_t)(i * a * 2)) & 1023u];
                if (!k)
                    v = clamp(v * (16 + a) / 16, -32767, 32767);
                gp_to(x0 + 4 + x, SP_M - ((v * 28) >> 15), i ? tint_col(k, 1) : c);
            }
        }
        if (k == 2u)                                    /* DELAY: the sound, then its echoes, each FDBK of the last */
            for (i = 0; i < 5; i++, h = h * a / 127)
                cv_rect(x0 + 5 + i * 9, SP_B - 2 - (h >> 8), 3, h >> 8, i ? c : tint_col(k, 1));
        if (k == 3u)                                    /* REVERB: the tail, longer as more goes in */
            for (x = 0; x < 40; x += 2, h -= h * 10 / (10 + a))
                cv_rect(x0 + 4 + x, SP_B - 2 - (h >> 8) * a / 127, 1, (h >> 8) * a / 127 + 1, c);
    }
}
/* the INSERT: its curve, the output for an input from -1 to 1 (SOFT .. CRUSH: the shape itself, fx.c ins_shape, after
 * DRIVE, at LEVEL), mixed with the dry line by MIX; the swept ones (PHASR, FLANG, CHOR): the notches they comb into
 * the sound, as deep as DEPTH, peaked by FDBK, moving at RATE; OFF: the dry line */
static void gp_insert(const track_t *t, uint16_t c)
{
    const int16_t *p = t->p;
    int32_t ty = clamp(p[P_ITYPE], 0, IT_N - 1), mix = ins_mixq(p[P_IMIX]), x;
    gp_dots(SP_M);
    gp_go = 0;
    if (ty >= IT_PHASER) {
        int32_t n = ty == IT_PHASER ? 4 : ty == IT_FLANGER ? 9 : 2, sh = (int32_t)(ui.frame * (uint32_t)(1 + p[P_IA] / 8)) % 64;
        for (x = SP_L; x <= SP_R; x += 2) {
            int32_t ph = ((x - SP_L) * n * 1024 / (SP_R - SP_L) + sh * 16) & 1023, s = SINE[ph];
            int32_t g = s > 0 ? s * p[P_IC] / 127 : s * p[P_IB] / 127;   /* FDBK the peaks, DEPTH the notches */
            gp_to(x, SP_M - 16 - ((g * 28) >> 15) * mix / 32768, c);
        }
        return;
    }
    for (x = 120 - 37; x <= 120 + 37; x += 1) {         /* a square, 75 px: in across, out up */
        int32_t in = (x - 120) * 32767 / 37, out = in, g = 4096 + p[P_IA] * p[P_IA] * 3;
        if (ty == IT_CRUSH)
            out = ins_shape(IT_CRUSH, in, ins_bits_sh(p[P_IA]));
        else if (ty)
            out = (int32_t)((int64_t)ins_shape((uint32_t)ty, (in * (g >> 4)) >> 8, 0) * LEVEL_Q12[p[P_IC] & 127] >> 12);
        out = ins_mix(in, out, mix);
        gp_to(x, SP_M - clamp(out, -32767, 32767) * 37 / 32768, c);
    }
    cv_line(120 - 37, SP_M + 37, 120 + 37, SP_M - 37, OP_PUR_D);   /* the dry line */
    if (!ty)
        return;
    gp_go = 0;                                          /* right: TONE (CRUSH: LPF), the low-pass after it */
    for (x = 172; x <= SP_R; x += 2)
        gp_to(x, SP_M - 10 + gp_resp(x, 176 + p[ty == IT_CRUSH ? P_IC : P_IB] * 52 / 127, 0, 6, 0), tint_col(2u, 0));
    if (ty == IT_CRUSH) {                               /* left: RATE, a wave held at the rate */
        int32_t step = 2 + (127 - p[P_IB]) / 8;
        gp_go = 0;
        for (x = 0; x <= 60; x++)
            gp_to(SP_L + x, SP_M - ((SINE[(uint32_t)(x - x % step) * 17u & 1023u] * 20) >> 15), tint_col(1u, 0));
    }
}
/* VOICE: three keys held (low first, high next, the middle last): POLY plays them all, the others one, by PRIO (LAST,
 * LOW, HIGH); MONO starts it apart from the note before, LEGATO tied on; UNISON thick. GLIDE how long the pitch
 * slides, GLMODE RATE (the longer jump slides longer) or TIME (both the same) */
static void gp_voice(const track_t *t, uint16_t c)
{
    int32_t vm = clamp(t->p[P_VOICE], 0, 3), pr = clamp(t->p[P_PRIO], 0, 2), gl = t->p[P_GLIDE] * 50 / 127, i;
    for (i = 0; i < 3; i++) {                           /* the notes, left: high, middle, low */
        int32_t y = SP_T + 8 + i * 26, xs = SP_L + (i == 0 ? 20 : i == 1 ? 40 : 0);
        if (vm == V_POLY || i == (pr == 0 ? 1 : pr == 1 ? 2 : 0)) {
            cv_rect(xs, y - (vm == V_UNISON ? 4 : 0), SP_L + 104 - xs, vm == V_UNISON ? 14 : 6, c);
            if (vm == V_MONO || vm == V_LEGATO)         /* the note before: a gap (MONO), tied on (LEGATO) */
                cv_rect(xs - 20, y, vm == V_LEGATO ? 20 : 15, 6, tint_col(0u, 1));
        } else {
            op_box(xs, y, SP_L + 104 - xs, 6, tint_col(0u, 2));
        }
    }
    for (i = 0; i < 2; i++) {                           /* the slides, right: a step of 1, then of 3 */
        int32_t y0 = SP_B - 4 - i * 40, len = t->p[P_GLMODE] ? gl : gl * (1 + i * 2) / 3;
        gp_go = 0;
        gp_to(128, y0, c);
        gp_to(152, y0, c);
        gp_to(152 + len, y0 - 10 - i * 20, c);
        gp_to(SP_R, y0 - 10 - i * 20, c);
    }
}
/* JUNO's CHORUS: OFF one line; I, II, I+II two, three, four, wider apart */
static void gp_chorus(const track_t *t, uint16_t c)
{
    int32_t n = clamp(t->p[P_E7], 0, 3), i, x;
    for (i = n; i >= 0; i--) {
        gp_go = 0;
        for (x = SP_L; x <= SP_R; x += 3)
            gp_to(x, SP_M - ((SINE[((uint32_t)(x - SP_L) * 9u + (uint32_t)(i * (40 + n * 30))) & 1023u] * 30) >> 15),
                  i ? tint_col(0u, 1) : c);
    }
}
static void op_sound(const track_t *t, const page_t *pg)
{
    uint32_t e = t->eng_req % NENGINES, si = song.sel & 3u, pose;
    uint16_t c = tint_col(0u, 0);
    int flip;
    const int16_t *p = t->p;
    if (!is_drum(t))
        op_names(t);
    pose = op_dance(&flip);                             /* the character, small, top right */
    op_k = 3;
    op_band_member(si, 218, 35, pose, flip, TE_COL[si]);
    if (pg->fam == FAM_FLT) {
        if (pg->id[0] == P_TFLT || is_drum(t))
            gp_filter(127, 0, 0, -1, 0, p[P_TFLT], c);
        else if (e == ENGI_ACID)                        /* CUT RES ENV DEC: 18 dB */
            gp_filter(p[P_E1], p[P_E2], p[P_E3], p[P_E4], 8, 0, c);
        else                                            /* JUNO: CUT RES ENV, 24 dB, then FILT */
            gp_filter(p[P_E4], p[P_E5], p[P_E6], -1, 10, p[P_TFLT], c);
    } else if (pg->graph == GR_INS) {
        gp_insert(t, c);
    } else if (pg->id[0] == P_VOICE) {
        gp_voice(t, c);
    } else if (pg->id[0] == P_E7 && pg->fam == FAM_FX) {
        gp_chorus(t, c);
    } else if (pg->graph == GR_FX) {
        gp_sends(t, c);
    } else if (pg->fam == FAM_OSC && pg->id[0] == P_E0) {
        gp_osc(t, c);
    } else {                                            /* OSC 2, ENV DEST, LFO DEST (from the middle), VOICE 2 */
        gp_bars(t, pg);
    }
}

/* ---------------------------------------------------------------- MIXER --- */
/* GLO held: the band on stage, a track a column (as the knobs and the track bar under it). A podium as
 * high as the track's level, its meter inside; its character on top, dancing to the beat (still when muted),
 * hopping as the track sounds. Muted: dashed and dim, a red M; soloed: a white S. Under the stage the 16 white keys and what they do here:
 * M mute 1..4, S solo 1..4, F a fill while held, B fill the next bar, T tap the tempo (lit on the beat).
 * flags: 1 a fill now, 2 the next bar is one, 4 it is asked for, 8 the beat. */
static void op_mix_draw(const char *const val[4], const int32_t ratio[4], uint32_t flags)
{
    static const char *const LAB[4] = {"1", "2", "3", "DR"};   /* (the tracks by number, as the strip: review 21) */
    static const char KEYS[17] = "123D123DFBP    T";   /* (PHEBUS: P, key 11, the knobs are the pans) */
    static uint32_t head, scene, keys, knobs, bar;
    const uint8_t *mx = op_mx, *hop = op_hop;
    uint32_t i, sig = 7u;
    op_header("GLO", C_WHITE, ui.msg_t ? ui.msg : flags & 16u ? "PAN" : "MIX", &head);   /* (GLO: MIX, the button and the layer, review 21; a
                                                         * message: in the title, the levels stay) */
    op_band_tick();
    for (i = 0; i < NTRK; i++)
        sig = sig * 131u + mx[i] + hop[i] * 97u + (uint32_t)ratio[i] * 7u + (trk[i].p[P_MUTE] != 0) * 3u;
    sig = sig * 31u + song.solo + (song.playing ? clk_beat & 3u : 4u) * 64u;   /* (the dance: a pose a beat) */
    sig = sig * 31u + (op_strike[0] != 0) + (op_strike[1] != 0) * 2u + (op_strike[2] != 0) * 4u + (op_strike[3] != 0) * 8u;
    if (ui.force || sig != scene) {
        scene = sig;
        cv_begin(240, 96, C_BLACK);
        cv_rect(0, 94, 240, 1, TE_G1);
        op_k = 2;
        for (i = 0; i < NTRK; i++) {
            int32_t cx = 4 + 58 * (int32_t)i + 27, h = 6 + ratio[i] * 46 / 1000, top = 94 - h, y, n = (h - 3) / 4, lit;
            int32_t air = hop[i] ? (SINE[(6 - hop[i]) * 85] * 5) >> 15 : 0;
            uint32_t mute = trk[i].p[P_MUTE] != 0;
            uint16_t c = mute ? TE_G3 : TE_COL[i];   /* (muted: grey, never darker, review 19) */
            if (mute) {                                 /* a muted podium: dashed */
                for (y = top; y < 94; y += 4) {
                    cv_rect(cx - 17, y, 1, 2, c);
                    cv_rect(cx + 16, y, 1, 2, c);
                }
                for (y = -17; y < 17; y += 4)
                    cv_rect(cx + y, top, 2, 1, c);
            } else {
                op_box(cx - 17, top, 34, h + 1, c);
            }
            lit = n * mx[i] / 82;
            for (y = 0; y < n; y++)
                cv_rect(cx - 13, 91 - y * 4, 26, y < lit ? 2 : 1, y < lit ? TE_COL[i] : TE_G1);
            op_ink = mute ? OP_PUR_D : OP_PUR;
            {                                           /* dancing to the beat (still if muted) */
                int flip = 0;
                uint32_t pose = mute ? 0u : op_dance(&flip);
                op_band_member(i, cx, top - 1 - air, pose, flip, c);
            }
            if (mute || (song.solo >> i) & 1u) {
                cv_rect(cx + 14, 2, 12, 14, mute ? TE_MUTE : TE_SOLO);
                cv_text(cx + 16, 1, &FONT_S, mute ? "M" : "S", C_BLACK);
            }
        }
        op_ink = OP_PUR;
        op_k = 3;
        cv_blit(0, OP_BODY_Y);
    }
    sig = flags + song.solo * 64u;
    for (i = 0; i < NTRK; i++)
        sig = sig * 2u + (trk[i].p[P_MUTE] != 0);
    if (ui.force || sig != keys) {                      /* the 16 white keys: the track's number on its mute and solo
                                                         * keys, what each group does under it (review 21) */
        static const char *const GRP[4] = {"MUTE", "SOLO", "FILL", "TAP"};
        static const uint8_t GX[4] = {0, 4, 8, 15};
        keys = sig;
        cv_begin(240, 36, C_BLACK);
        for (i = 0; i < 16u; i++) {
            int32_t x = 4 + (int32_t)i * 14 + (int32_t)(i / 4u) * 2;
            uint32_t on = i < 4u ? trk[i].p[P_MUTE] != 0 : i < 8u ? (song.solo >> (i - 4u)) & 1u : i == 8u ? flags & 1u :
                          i == 9u ? (flags >> 1) & 1u : i == 10u ? (flags >> 4) & 1u : i == 15u ? (flags >> 3) & 1u : 0u;
            uint16_t c = i < 4u ? TE_COL[i] : i < 8u ? TE_MID[i - 4u] : KEYS[i] == ' ' ? OP_PUR_D : TE_G4;
            char ch[2] = {KEYS[i], 0};
            if (on)
                cv_rect(x, 2, 12, 20, i < 4u ? TE_MUTE : i < 8u ? TE_SOLO : C_WHITE);
            else
                op_box(x, 2, 12, 20, c);
            if (i == 9u && (flags & 4u))
                cv_rect(x, 2, 12, 2, C_WHITE);
            cv_text(x + 2, 4, &FONT_S, ch, on ? C_BLACK : c);
        }
        for (i = 0; i < 4u; i++) {
            int32_t x = 4 + (int32_t)GX[i] * 14 + (int32_t)(GX[i] / 4u) * 2;
            cv_text(i == 3u ? 236 - text_w(&FONT_S, GRP[i]) : x, 21, &FONT_S, GRP[i], TE_G3);
        }
        cv_blit(0, OP_BODY_Y + 96);
    }
    {
        uint8_t m = ui.msg_t;
        ui.msg_t = 0;
        knob_tint_voices();                             /* the mixer: one knob per voice */
        op_knobs(LAB, val, 0, ratio, 0, 4242u, &knobs);
        ui.msg_t = m;
    }
    op_trackbar(&bar);
}

/* ----------------------------------------------------------------- HOME --- */
static uint32_t home_band_tick(void);                   /* ui_vis.c */
static void home_band(int32_t x0, int32_t y0, uint32_t w, uint32_t h);
/* TRACKS, between the header and the knobs. The deck: the bar in large digits, a strip of the FLOW backdrop moving with
 * the four voices' levels (PHEBUS: it replaced the tape reels; ui_vis.c home_band), the song's sections A..D (the one
 * playing filled). Then the selected track's sound and state, and a lane a track: its character in small (it hops as
 * the track sounds), the 16 steps in view (a synth: each note at its height; the drums: a bar, taller with more
 * sounds), the step that plays, its level. */
#define HB_X 74                                         /* the strip: x, y in the deck, its size (2 x 2 blocks) */
#define HB_Y 6
#define HB_W 106u
#define HB_H 32u
static void op_home_body(void)
{
    static uint32_t top_sig, name_sig, lane_sig[NTRK];
    track_t *ts = TSEL;
    uint32_t i, j, playing = song.playing, slen = trk_len(ts), spos = ts->seq_idx % slen, sig, moving;
    uint32_t ready = arrangement_ready();
    op_band_tick();
    moving = home_band_tick();                          /* (the voices' levels into the strip; 1: it flows) */
    sig = playing * 3u + (playing ? spos / 16u * 31u : 0u) + ready * 5u + (uint32_t)(live_sec + 1) * 77u + slen * 13u +
          (moving ? ui.frame / 2u * 7919u + 1u : 0u);
    if (ui.force || sig != top_sig) {                   /* the deck */
        uint32_t bar = playing ? spos / 16u + 1u : 1u;
        top_sig = sig;
        cv_begin(240, 44, C_BLACK);
        te_digit(4, 7, 12, 28, 2, bar / 10u, playing ? C_WHITE : TE_G3);
        te_digit(20, 7, 12, 28, 2, bar % 10u, playing ? C_WHITE : TE_G3);
        cv_text(38, 20, &FONT_S, "BAR", TE_G3);
        home_band(HB_X, HB_Y, HB_W, HB_H);
        for (i = 0; i < 4u; i++) {                      /* the sections */
            char ch[2] = {(char)('A' + i), 0};
            int32_t x = 188 + (int32_t)i * 12;
            int on = live_sec == (int8_t)i, used = (ready >> i) & 1u;
            if (on)
                cv_rect(x, 12, 11, 18, C_WHITE);
            else
                op_box(x, 12, 11, 18, used ? OP_PUR : OP_PUR_D);
            cv_text(x + 2, 13, &FONT_S, ch, on ? C_BLACK : used ? OP_PUR : OP_PUR_D);
        }
        cv_blit(0, OP_BODY_Y);
    }
    {   /* the selected track: its sound, its engine; REC / SOLO / MUTE */
        char b[24], e[12];
        uint32_t rec = (song.rec >> song.sel) & 1u, solo = (song.solo >> song.sel) & 1u, silent = trk_silent(ts) != 0;
        if (song.sel == TRK_DRUM) {
            str_cpy(b, DRUM_KIT_NAMES[drum_kit()], sizeof b);
            str_cpy(e, "DRUM", sizeof e);
        } else {
            uint32_t k, n;
            trk_short_name(song.sel, b);
            str_cpy(e, ENGINES[ts->eng_req % NENGINES]->name, sizeof e);
            for (k = 0, n = str_len(e); b[k]; k++)       /* the engine's name already in the sound's: once (review 21) */
                if (!memcmp(b + k, e, n)) {
                    e[0] = 0;
                    break;
                }
        }
        b[13] = 0;
        sig = studio_hash(studio_hash(song.sel + rec * 8u + solo * 16u + silent * 32u, b), e);
        if (ui.force || sig != name_sig) {
            int32_t x;
            name_sig = sig;
            cv_begin(240, 16, C_BLACK);
            x = cv_text(4, 0, &FONT_S, b, C_WHITE);
            if (e[0] && x + 8 + text_w(&FONT_S, e) < 196)
                cv_text(x + 8, 0, &FONT_S, e, TE_COL[song.sel & 3u]);
            if (rec)
                cv_text(236 - text_w(&FONT_S, "REC"), 0, &FONT_S, "REC", TE_RED);
            else if (solo)
                cv_text(236 - text_w(&FONT_S, "SOLO"), 0, &FONT_S, "SOLO", TE_SOLO);
            else if (silent)
                cv_text(236 - text_w(&FONT_S, "MUTE"), 0, &FONT_S, "MUTE", TE_MUTE);
            cv_blit(0, OP_BODY_Y + 44);
        }
    }
    for (i = 0; i < NTRK; i++) {                        /* the lanes */
        track_t *t = &trk[i];
        uint32_t sel = song.sel == i, len = trk_len(t), pos = t->seq_idx % len, bank = playing ? pos / 16u : 0u;
        uint32_t level = i == TRK_DRUM ? (uint32_t)song.g[G_DRLVL] : (uint32_t)t->p[P_LEVEL], silent = trk_silent(t) || !level;
        uint32_t rec = (song.rec >> i) & 1u;
        int32_t lo = 127, hi = 0, air = op_hop[i] > 2u ? 2 : 0;
        uint16_t col = silent ? TE_G3 : sel ? TE_COL[i] : TE_MID[i];   /* (muted: grey, never darker, review 19) */
        sig = sel + level * 7u + silent * 997u + rec * 1999u + len * 37u + playing * 3u + (playing ? pos * 71u : 0u) + bank * 13u +
              (uint32_t)air * 5003u + ((song.solo >> i) & 1u) * 4999u;
        for (j = 0; j < len; j++) {
            if (i == TRK_DRUM) {
                sig = sig * 31u + dstep_mask(&t->dstep[j]);
            } else {
                const step_t *st = &t->step[j];
                sig = sig * 31u + (step_on(st) ? st->note[0] + 1u : 0u);
                if (step_on(st) && st->note[0] < lo)
                    lo = st->note[0];
                if (step_on(st) && st->note[0] > hi)
                    hi = st->note[0];
            }
        }
        if (!ui.force && sig == lane_sig[i])
            continue;
        lane_sig[i] = sig;
        if (hi - lo < 4)
            hi = lo + 4;
        cv_begin(240, 18, C_BLACK);
        if (sel)
            op_box(24, 0, 214, 18, TE_COL[i]);
        op_k = 1;                                       /* the character, half size */
        op_ink = silent ? OP_PUR_D : sel ? C_WHITE : OP_PUR;
        op_band_member(i, 12, 17 - air, 0, 0, silent ? TE_G3 : TE_COL[i]);
        op_ink = OP_PUR;
        op_k = 3;
        for (j = 0; j < 16u; j++) {
            uint32_t p = bank * 16u + j;
            int32_t x = 28 + (int32_t)j * 11;
            if (p >= len)
                continue;
            if (i == TRK_DRUM) {                        /* a bar: 3 px a sound, up to 4 */
                uint32_t m = dstep_mask(&t->dstep[p]), n = 0;
                for (; m; m &= m - 1u)
                    n++;
                if (n)
                    cv_rect(x + 2, 14 - (int32_t)(n > 4u ? 4u : n) * 3, 5, (int32_t)(n > 4u ? 4u : n) * 3, col);
                else
                    cv_rect(x + 3, 12, 2, 2, TE_G3);    /* (an empty step: a 2 x 2 dot, grey, review 19) */
            } else if (step_on(&t->step[p])) {          /* a note at its height (the first of a chord) */
                cv_rect(x, 13 - (t->step[p].note[0] - lo) * 9 / (hi - lo), 9, 2, col);
                if (t->step[p].n > 1)
                    cv_rect(x, 9 - (t->step[p].note[0] - lo) * 9 / (hi - lo), 9, 1, col);
            } else if (t->step[p].time == ST_TIE) {
                cv_rect(x, 13, 9, 1, TE_G3);
            } else {
                cv_rect(x + 3, 12, 2, 2, TE_G3);
            }
            if (playing && p == pos)
                cv_rect(x, 15, 9, 2, C_WHITE);
        }
        cv_rect(207, 8, 28, 3, TE_G1);                  /* the level; REC: a red dot */
        cv_rect(207, 8, (int32_t)level * 28 / 127, 3, col);   /* (muted: its real level, grey) */
        if (rec)
            te_disc(232, 4, 2, TE_RED);
        cv_blit(0, OP_BODY_Y + 60 + i * 18);
    }
}
