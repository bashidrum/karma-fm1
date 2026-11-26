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

static uint16_t te_dim_of(uint16_t c)                  /* (wave 9) a colour's dark shade: a voice's TE_DIM, else dark grey */
{
    uint32_t v;
    for (v = 0; v < 4u; v++)
        if (TE_COL[v] == c)
            return TE_DIM[v];
    return TE_G1;
}
static void op_box(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c)   /* a 1 px outline */
{
    cv_rect(x, y, w, 1, c);
    cv_rect(x, y + h - 1, w, 1, c);
    cv_rect(x, y, 1, h, c);
    cv_rect(x + w - 1, y, 1, h, c);
}

/* the header: [tag] title | BPM, play / stop, the four beats, REC (else the unsaved dot), the battery */
static uint8_t proj_unsaved;                           /* (PHEBUS) the work differs from flash (project.c unsaved_tick) */
#define OP_BPM_R 154                                   /* the tempo's right edge */
static uint8_t sel_t;                                   /* (wave 9) frames the SELECT chip stays filled after a turn */
static int sel_chip(char *v, int32_t *ratio, uint16_t *col);   /* (ui_sel.c) */
/* (wave 10, Ben 23 Nov 2026) THE -/+ LEGEND: what a single tap of OCT- and OCT+ does on the screen shown, in one small place (the
 * right end of the tab row on page screens, of the line under the header on the layers): a boxed minus and its word, a boxed
 * plus and its word; one word between them when both move the same thing (- PAGE +). White-grey: the octave buttons are
 * everyone's. Not drawn where -/+ do nothing */
static int32_t pm_glyph(int32_t x, int32_t y, int plus)   /* a 10 x 12 box, its sign in white */
{
    op_box(x, y + 3, 10, 12, TE_G3);
    cv_rect(x + 2, y + 8, 6, 2, C_WHITE);
    if (plus)
        cv_rect(x + 4, y + 6, 2, 6, C_WHITE);
    return x + 11;
}
static int32_t pm_width(const char *m, const char *p)
{
    if (m && p && str_eq(m, p))
        return 11 + text_w(&FONT_S, m) + 1 + 10;
    return (m ? 11 + text_w(&FONT_S, m) : 0) + (m && p ? 6 : 0) + (p ? 11 + text_w(&FONT_S, p) : 0);
}
static int32_t pm_draw(int32_t xr, int32_t y, const char *m, const char *p)   /* right-aligned at xr; returns its left */
{
    int32_t x0 = xr - pm_width(m, p), x = x0;
    if (!m && !p)
        return xr;
    if (m && p && str_eq(m, p)) {
        x = pm_glyph(x, y, 0);
        x = cv_text(x, y, &FONT_S, m, TE_G4) + 1;
        pm_glyph(x, y, 1);
        return x0;
    }
    if (m) {
        x = pm_glyph(x, y, 0);
        x = cv_text(x, y, &FONT_S, m, TE_G4) + 6;
    }
    if (p) {
        x = pm_glyph(x, y, 1);
        cv_text(x, y, &FONT_S, p, TE_G4);
    }
    return x0;
}
/* (wave 10, Ben: black keys "Only live when drawn") THE BLACK-KEY STRIP, PERFORM's (ui_perf.c), in the same place on every
 * screen that gives a black key a job: y 40, two rows, F#3 .. F#4 above, G#4 .. F#5 below, 37 x 12 a key. A key with a job
 * has its word (lit: white, filled); a key with none here is an empty outline: it does nothing */
#define BK_Y 40
#define BK_H 26
static uint16_t bk_drawn;                               /* (wave 11) the black keys with a job in the strip drawn this frame */
static uint32_t bk_drawn_f;                             /* (the frame it was drawn: ui.frame; the lights read both) */
static void bk_strip(const char *const lab[11], uint32_t on, uint32_t *cache)
{
    uint32_t i, sig = on * 7u + 11u;
    bk_drawn = 0;
    bk_drawn_f = ui.frame;
    for (i = 0; i < 11u; i++) {
        sig = studio_hash(sig * 3u, lab[i] ? lab[i] : "\1");
        bk_drawn |= (uint16_t)((lab[i] != 0) << i);
    }
    if (!ui.force && sig == *cache)
        return;
    *cache = sig;
    cv_begin(240, BK_H, C_BLACK);
    for (i = 0; i < 11u; i++) {
        int32_t r = i < 6u ? 0 : 1, c = i < 6u ? (int32_t)i : (int32_t)i - 6, x = 2 + c * 40, y = r * 13;
        if (!lab[i]) {
            op_box(x, y, 37, 12, TE_G1);
        } else if ((on >> i) & 1u) {
            cv_rect(x, y, 37, 12, C_WHITE);
            te_text_c(x + 18, y - 2, lab[i], C_BLACK);
        } else {
            cv_rect(x, y, 37, 12, TE_G1);
            te_text_c(x + 18, y - 2, lab[i], TE_G4);
        }
    }
    cv_blit(0, BK_Y);
}
/* (wave 10) MASTER's chip: on a layer the header's title place says what MASTER turns while the button is held (a little
 * dial, the pair's word, in what it acts on's colour; filled a moment after a turn), as SELECT's chip says what SELECT turns.
 * Set by the layer's drawing for one op_header call */
static const char *hdr_mchip;
static uint8_t master_t;                                /* MASTER's chip filled a moment after a turn (frames: ui_timers) */
static uint16_t hdr_mcol;
static int32_t hdr_mratio;
static void op_header(const char *tag, uint16_t tc, const char *title, uint32_t *cache)
{
    char b[12];
    uint32_t beat = clk_beat, k, playing = song.playing;
    char cv_[10];
    int32_t cr = 0;
    uint16_t cc = 0;
    int chip = sel_chip(cv_, &cr, &cc);
    uint32_t sig = studio_hash((uint32_t)song.g[G_BPM] * 7u + playing * 3u + (song.rec != 0) * 1999u +
                               (playing ? beat * 131u : 0u) + (ui.bpm_t != 0) * 104729u + (uint32_t)batt_shown() * 7777u + proj_unsaved * 31337u +
                               sr_on * 3331u + fill_now * 7331u + pf_count(perf.lat) * 13331u, title);
    sig = studio_hash(sig * 31u + tc, tag);
    if (chip)
        sig = studio_hash(sig * 131u + (uint32_t)cr * 7u + cc + (sel_t != 0u) * 99991u, cv_);
    if (hdr_mchip)
        sig = studio_hash(sig * 137u + (uint32_t)hdr_mratio * 3u + hdr_mcol + (master_t != 0u) * 7717u, hdr_mchip);
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
        {   /* (wave 7) the title's room: to 4 px before the tempo (three digits: "PATTERN A" fits; it was cut to "PATTERN") */
            char t[8];
            int32_t room;
            fmt_int(t, song.g[G_BPM]);
            room = OP_BPM_R - text_w(&FONT_S, t) - 6;
            while (b[0] && w + 6 + text_w(&FONT_S, b) > room)
                b[str_len(b) - 1u] = 0;
        }
        if (fill_now || perf.lat) {                     /* (PHEBUS wave 3) a fill on (held, latched, the bar): FILL inverted,
                                                         * white: it is everyone's (the conditions of every track). (wave 6)
                                                         * PERFORM pads latched: PERF and how many, the same way */
            int32_t x = w + 6;
            if (fill_now) {
                cv_rect(x, 2, text_w(&FONT_S, "FILL") + 4, 16, C_WHITE);
                x = cv_text(x + 2, 2, &FONT_S, "FILL", C_BLACK) + 4;
            }
            if (perf.lat) {                             /* (PERF, not FX: never read as a voice's FX slot) */
                char f[8] = "PERF ";
                if (fill_now)
                    str_cpy(f, "P", 8);
                fmt_int(f + str_len(f), (int32_t)pf_count(perf.lat));
                cv_rect(x, 2, text_w(&FONT_S, f) + 4, 16, C_WHITE);
                cv_text(x + 2, 2, &FONT_S, f, C_BLACK);
            }
        } else if (hdr_mchip) {                         /* (wave 10) what MASTER turns while this button is held */
            int32_t cx = w + 5, cw = 18 + text_w(&FONT_S, hdr_mchip) + 3;
            uint32_t hot = master_t != 0u;
            cv_rect(cx, 1, cw, 18, hot ? hdr_mcol : te_dim_of(hdr_mcol));
            te_dial(cx + 9, 10, 6, hdr_mratio, hot ? C_BLACK : hdr_mcol, hot ? hdr_mcol : C_BLACK);
            cv_text(cx + 18, 2, &FONT_S, hdr_mchip, hot ? C_BLACK : hdr_mcol);
        } else if (chip) {                              /* (wave 9) WHAT SELECT TURNS: its little dial and the value, in the
                                                         * colour of what it acts on, where the title sits (the SELECT knob is
                                                         * at the screen's upper left); filled a moment after a turn */
            int32_t cx = w + 5, cw = 18 + text_w(&FONT_S, cv_) + 3;
            uint32_t hot = sel_t != 0u;
            cv_rect(cx, 1, cw, 18, hot ? cc : te_dim_of(cc));
            te_dial(cx + 9, 10, 6, cr, hot ? C_BLACK : cc, hot ? cc : C_BLACK);
            cv_text(cx + 18, 2, &FONT_S, cv_, hot ? C_BLACK : cc);
        } else {
            cv_text(w + 6, 2, &FONT_S, b, TE_G3);
        }
    }
    fmt_int(b, song.g[G_BPM]);
    cv_text(OP_BPM_R - text_w(&FONT_S, b), 2, &FONT_S, b, ui.bpm_t ? C_WHITE : TE_G4);
    if (playing) {
        int32_t i;
        for (i = 0; i < 10; i++)
            cv_rect(OP_BPM_R + 6 + i, 4 + i / 2, 1, 12 - i, C_WHITE);
    } else {
        cv_rect(OP_BPM_R + 6, 5, 10, 10, TE_G3);
    }
    for (k = 0; k < 4u; k++)                           /* the four beats of the bar (wave 7: 7 px apart, 4 px right, so the
                                                         * title PATTERN A clears the tempo) */
        cv_rect(OP_BPM_R + 22 + (int32_t)k * 7, 7, 6, 6, playing && beat % 4u == k ? (k || !song.rec ? C_WHITE : TE_RED) : TE_G2);   /* (red: recording only) */
    if (song.rec || sr_on)                              /* (recording, or STEP REC: ui_grid.c) */
        te_disc(209, 10, 4, TE_RED);
    else if (proj_unsaved)                             /* (saved again at the next stop: project.c autosave_tick) */
        te_disc(209, 10, 3, C_WHITE);                  /* (the bank: a size bigger, harder to miss: work not yet in flash) */
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
static void op_knobs(const char *const lab[4], const char *const val[4], const char *const *unit, const int32_t ratio[4],
                     uint32_t dim, uint32_t sig, uint32_t *cache)
{
    uint32_t k;
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
 * tabs. Each track its head and its number (1 2 3 DR); the selected track underlined, its number in its
 * colour (the others grey); a muted track a yellow M badge, a soloed one an ice-blue S, the one that records a red
 * dot. A 1 px rule above */
static void op_sprite_wh(const uint8_t *d, int32_t w, int32_t h, int32_t x0, int32_t y0, uint16_t c);
static void op_badge(int32_t x, int32_t y, char ch, uint16_t bg)   /* 10 x 14: a black letter on bg */
{
    char b[2] = {ch, 0};
    cv_rect(x, y, 10, 14, bg);
    cv_text(x + 1, y - 1, &FONT_S, b, C_BLACK);
}
static void op_trackbar(uint32_t *cache)
{
    static const char *const NAME[NTRK] = {"1", "2", "3", "DR"};
    uint32_t i, sig = 1u + song.sel + song.rec * 16u + song.solo * 256u;
    for (i = 0; i < NTRK; i++)
        sig = sig * 3u + (trk[i].p[P_MUTE] != 0);
    if (!ui.force && sig == *cache)
        return;
    *cache = sig;
    cv_begin(240, OP_BAR_H, C_BLACK);
    cv_rect(0, 0, 240, 1, TE_G1);
    for (i = 0; i < NTRK; i++) {
        int32_t x = 4 + 58 * (int32_t)i, nx = x + OP_HM_W + 4, bx;
        uint32_t sel = song.sel == i;                   /* (wave 14: on the STUTTER lane too: the voice stays selected) */
        op_sprite_wh(OP_SPR_HM[i], OP_HM_W, OP_HM_H, x, 5, TE_COL[i]);
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

/* the tracks' heads (op_sprites.h, drawn by tools/gen_sprites.py): a robot (track 1), an alien (2), a snake (3), a
 * crocodile (the drums; wave 12, Ben 23 Nov 13:35: "the crocodile the new drummer"), each in its track's colour; 18 x 16 in flash, 4 bits a pixel: 1 dark, 2 the colour, 3 light
 * (shades of the track's colour), 4 white, 5 black. Only in the track strip and HOME's lanes (KARMA's full-size
 * characters, their dance and STRIKE poses and the mixer's figures are cut) */
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
/* a picture w x h (even); (x0, y0): the top left, c: the track's colour */
static void op_sprite_wh(const uint8_t *d, int32_t w, int32_t h, int32_t x0, int32_t y0, uint16_t c)
{
    uint16_t pal[6];
    int32_t x, y;
    op_pal(pal, c, op_dim);
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++) {
            uint32_t v = (d[(y * w + x) >> 1] >> ((x & 1) * 4)) & 15u;
            if (v && v < 6u)
                cv_pset(x0 + x, y0 + y, pal[v]);
        }
}
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

/* ---------------------------------------------------------------- DRUMS --- */
#define OP_STEPS_H 36

/* the 16 steps of KNOB 1's sound: a trig filled in its velocity's colour (notches: a retrig), the cursor
 * framed white, the step that plays underlined */
static void op_drum_steps(uint32_t bank, uint32_t len)   /* (a step held: at the top, as on a synth's GRID, review 19) */
{
    uint32_t j, r;
    cv_begin(240, OP_STEPS_H, C_BLACK);
    for (j = 0; j < 16u; j++) {
        uint32_t p = bank * 16u + j;
        int32_t x = 4 + (int32_t)j * 14 + (int32_t)(j / 4u) * 2, xi;
        if (p >= len)
            continue;
        if (dt_has(p, drum_lane)) {
            cv_rect(x, 4, 12, 22, vel_col(dt_vel(p, drum_lane)));
            xi = dt_x(p, drum_lane);
            for (r = 0; xi >= 0 && dpat.x[xi].rrate && r < 3u; r++)
                cv_rect(x + 2, 7 + (int32_t)r * 4, 8, 2, C_BLACK);
        } else {
            op_box(x, 4, 12, 22, j % 4u ? OP_PUR_D : OP_PUR);
        }
        if (p == drum_cursor)
            op_box(x - 2, 2, 16, 26, C_WHITE);
        if (song.playing && p == TDRUM->seq_idx % len)
            cv_rect(x, 31, 12, 2, C_WHITE);
    }
    cv_blit(0, OP_BODY_Y);
}

/* ---------------------------------------------------------------- SYNTH --- */
/* a synth track's page without a graph of its own (EDIT, VOICE, the DEST pages...): the track's
 * scene (below), moved by the sound: the track's meter, its last note, each note
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

/* the sound of the track, at the top of the graph canvas: its preset alone, in white (the engine is the header's tag,
 * review 18) */
static void op_names_str(const track_t *t, char *b)    /* (b holds 16) */
{
    const engine_t *e = ENGINES[t->eng_req % NENGINES];
    b[0] = 0;
    if (user_of(t) < UP_SLOTS)
        up_name(user_of(t), b);
    else if (e->npresets)
        str_cpy(b, e->presets[t->preset % e->npresets].name, 16);
}
static void op_names(const track_t *t)
{
    char b[16];
    op_names_str(t, b);
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

/* the scenes' floor, over the sound chain's strip (y 141, review 19) */
#define OP_FLOOR_Y 118
/* synth 1 (the bass): KNOB 1 the wave (ANALOG's WAVE as it is, else from round to sharp; its ripples the last
 * note's pitch), KNOB 2 rays over the middle, KNOB 3 a moon that rises, KNOB 4 sparks off the floor on a note.
 * (The robot that stood here is cut: the tracks' characters are heads in the strip only) */
static void op_scene_robot(track_t *t, const int32_t ratio[4], int analog_wave)
{
    uint32_t kind = analog_wave ? (uint32_t)clamp(t->p[P_E0], 0, 4) : 5u;
    uint32_t cyc = (uint32_t)clamp(384 + ((int32_t)op_note - 36) * 13, 256, 1536), rnd = (ui.frame / 2u) * 2654435761u + 1u;
    int32_t morph = ratio[0] < 0 ? 0 : ratio[0], amp = 3 + op_lvl * 10 / 82, x, py = 0, cx = 120, i, n, hop = op_hit > 5u ? 2 : 0;
#define WY(xx, f, ph) (104 - ((op_wave((uint32_t)(((xx) - 8) * (int32_t)(f) / 56) + (ph), kind, morph) * amp) >> 15))
    if (ratio[2] >= 0)                                  /* KNOB 3: the moon */
        op_ell(222 - (op_px ? 146 : 0), 70 - ratio[2] * 34 / 1000, 5, 5, tint_col(2u, 0));
    for (x = 8; x <= 232; x += 2) {                     /* KNOB 1: the wave; white as a note starts */
        int32_t y = WY(x, cyc, op_ph);
        if (x > 8)
            cv_line(x - 2, py, x, y, op_hit > 6u ? C_WHITE : tint_col(0u, 0));
        py = y;
    }
    n = ratio[1] <= 0 ? 0 : 3 + ratio[1] * 10 / 1000;   /* KNOB 2: the rays */
    for (i = 0; i < n; i++) {
        uint32_t a = 512u + 64u + (uint32_t)i * 384u / (uint32_t)(n > 1 ? n - 1 : 1);
        int32_t hx = cx, hy = 42 - hop;
        int32_t co = SINE[(a + 256u) & 1023u], si = SINE[a & 1023u];
        cv_line(hx + ((co * 30) >> 15), hy + ((si * 26) >> 15), hx + ((co * 36) >> 15), hy + ((si * 32) >> 15), tint_col(1u, 0));
    }
    op_ell(120, OP_FLOOR_Y, 70, 2, op_hit > 4u ? C_WHITE : OP_PUR_D);   /* the floor: it rings under a note */
    n = ratio[3] <= 0 ? 0 : 2 + ratio[3] * 7 / 1000;    /* KNOB 4: sparks off the floor */
    for (i = 0; op_hit > 4u && i < n; i++) {
        int32_t fx = cx + (i & 1 ? 30 : -30), d = (9 - (int32_t)op_hit) * 3;
        rnd = rnd * 1664525u + 1013904223u;
        x = (int32_t)((rnd >> 8) % 21u) - 10;
        cv_line(fx + x, 118 - d / 2, fx + x * 2, 116 - d - (int32_t)((rnd >> 20) % 5u), tint_col(3u, 0));
    }
#undef WY
}

/* synth 2 (ELEMENTS): two speakers; a note: a speaker thumps (they take turns). (The alien that danced between
 * them is cut.) KNOB 1 the size of the speakers, KNOB 2 strings of lights from above, swinging
 * with the sound, KNOB 3 a moon that rises, KNOB 4 the rings of sound off the speaker */
static void op_scene_alien(const int32_t ratio[4])
{
    static uint8_t prev, side;                          /* the speaker that thumped last */
    int32_t big = ratio[0] < 0 ? 0 : ratio[0] * 6 / 1000, x, i, n, s;
    if (op_hit > prev)
        side ^= 1u;
    prev = op_hit;
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

/* synth 3 (PLAITS): a spotlight on a floor of lights, among the stars (the crocodile that danced in it is cut);
 * with sound the lights run along the floor and the stars pass, faster the louder; a note: the spotlight flashes
 * and the floor lights all light. KNOB 1 the spotlight's width, KNOB 2 a planet that grows,
 * KNOB 3 more stars, KNOB 4 sparks rising in the light */
static void op_scene_croc(const int32_t ratio[4])
{
    int32_t i, n, bw, live = op_lvl || op_hit, cx = op_px ? 92 : 120;   /* (left of PRISM's picture) */
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
 * once, falling; a rise: the track's head on HOME hops for 6 frames */
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
/* a track's head for HOME's lanes, standing on (cx, fy), dim (a muted, a silent track) */
static void op_lane_head(uint32_t i, int32_t cx, int32_t fy, int dim, uint16_t c)
{
    op_dim = (uint8_t)dim;
    op_sprite_wh(OP_SPR_HM[i & 3u], OP_HM_W, OP_HM_H, cx - OP_HM_W / 2, fy - OP_HM_H + 1, c);
    op_dim = 0;
}

/* ---------------------------------------------------------------- ARP --- */
/* the ARP page: the arp as stepping stones, one a note in the order it plays them (the keys held, else C E G to
 * show the shape), high for a high note; the stone of each note it plays lights white. KNOB 1
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
    int32_t gate = clamp(t->p[P_AGATE], 1, 127), lo = 127, hi = 0, sp, x0, live = t->nheld && mode;
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
    if (op_arp_hop)
        op_arp_hop--;
    if (!mode)
        cv_text(170, 24, &FONT_S, "ARP OFF", TE_G3);
    else if (!t->nheld)
        cv_text(150, 24, &FONT_S, "HOLD KEYS", TE_G3);
#undef AY
}

/* ------------------------------------------------------------------- FX --- */
/* ---------------------------------------------------------- SOUND PAGES --- */
/* (Ben, 21 Nov 2026, review 8: "Sound big, character small") a sound page (OSC, VOICE, FILTER, the DEST pages, FX,
 * CHORUS, INSERT) draws what its four knobs do, big, as they turn, in the voice's colour (tint_col: a shared page
 * neutral): the wave the oscillator makes, the filter's response, the sends, the INSERT's curve, the voice's ways. The
 * sound's name at the top left (the character small at the top right is cut).
 * MiniFreak's rule: the picture shows the consequence the instant a knob moves, not the number. One 2 px line
 * helper (gp_to) draws every curve in the plot SP_L..SP_R x SP_T..SP_B (graph canvas rows) */
#define SP_L 8
#define SP_R 232
static int32_t sp_t = 40, sp_b = 114;                   /* (wave 4: the plot's rows, set by who draws: ui_synth.c) */
#define SP_T sp_t
#define SP_B sp_b
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
        int32_t sl = p[P_E7] * 24 / 127, ac = 3 + p[P_E5] * 6 / 127, y = SP_T + 12;   /* (wave 4: inside the plot) */
        cv_rect(SP_L, y, 24, 4, tint_col(1u, 1));
        gp_go = 0;
        gp_to(SP_L + 24, y + 2, tint_col(2u, 0));
        gp_to(SP_L + 26 + sl, y - 5, tint_col(2u, 0));
        cv_rect(SP_L + 26 + sl, y - 5 - ac, 24, ac, c);
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
        if (pg_id(pg, k) >= P_COUNT)
            continue;
        d = track_desc(t, pg_id(pg, k));                /* (wave 7: assigned before it is read; was one unsequenced line) */
        if (d->max <= d->min)
            continue;
        bi = d->min < 0;
        op_box(x, SP_T, 48, SP_B - SP_T + 1, tint_col(k, 2));
        h = (t->p[pg_id(pg, k)] - (bi ? 0 : d->min)) * (SP_B - SP_T) / (d->max - (bi ? 0 : d->min)) / (bi ? 2 : 1);
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
    const int32_t R = (SP_B - SP_T) / 2;                /* (a square as tall as the plot: 37 px in the old one) */
    for (x = 120 - R; x <= 120 + R; x += 1) {           /* a square: in across, out up */
        int32_t in = (x - 120) * 32767 / R, out = in, g = 4096 + p[P_IA] * p[P_IA] * 3;
        if (ty == IT_CRUSH)
            out = ins_shape(IT_CRUSH, in, ins_bits_sh(p[P_IA]));
        else if (ty)
            out = (int32_t)((int64_t)ins_shape((uint32_t)ty, (in * (g >> 4)) >> 8, 0) * LEVEL_Q12[p[P_IC] & 127] >> 12);
        out = ins_mix(in, out, mix);
        gp_to(x, SP_M - clamp(out, -32767, 32767) * R / 32768, c);
    }
    cv_line(120 - R, SP_M + R, 120 + R, SP_M - R, OP_PUR_D);   /* the dry line */
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
        int32_t y = SP_T + 6 + i * (SP_B - SP_T - 6) / 3, xs = SP_L + (i == 0 ? 20 : i == 1 ? 40 : 0);
        if (vm == V_POLY || i == (pr == 0 ? 1 : pr == 1 ? 2 : 0)) {
            cv_rect(xs, y - (vm == V_UNISON ? 4 : 0), SP_L + 104 - xs, vm == V_UNISON ? 14 : 6, c);
            if (vm == V_MONO || vm == V_LEGATO)         /* the note before: a gap (MONO), tied on (LEGATO) */
                cv_rect(xs - 20, y, vm == V_LEGATO ? 20 : 15, 6, tint_col(0u, 1));
        } else {
            op_box(xs, y, SP_L + 104 - xs, 6, tint_col(0u, 2));
        }
    }
    for (i = 0; i < 2; i++) {                           /* the slides, right: a step of 1, then of 3 */
        int32_t hh = (SP_B - SP_T) / 4, y0 = SP_B - 4 - i * 2 * hh, len = t->p[P_GLMODE] ? gl : gl * (1 + i * 2) / 3;
        gp_go = 0;
        gp_to(128, y0, c);
        gp_to(152, y0, c);
        gp_to(152 + len, y0 - hh / 2 - i * hh, c);
        gp_to(SP_R, y0 - hh / 2 - i * hh, c);
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
    uint32_t e = t->eng_req % NENGINES;
    uint16_t c = tint_col(0u, 0);
    const int16_t *p = t->p;
    if (!is_drum(t))
        op_names(t);
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
/* GLO held: a track a column (as the knobs and the track bar under it). A bar as high as the track's level, its
 * meter inside (the characters that danced on top are cut: the bars are taller). Muted: dashed and grey, a yellow
 * M; soloed: an S. Under the stage the 16 white keys and what they do here:
 * M mute 1..4, S solo 1..4, F a fill while held, B fill the next bar, P the knobs are the pans (wave 8: no tap tempo, the
 * tempo is GLO > GLOBAL's BPM only).
 * flags: 1 a fill now, 2 the next bar is one, 4 it is asked for, 8 the beat. */
#define OPM_H (OP_BODY_Y + 96 - (BK_Y + BK_H + 1))       /* (wave 10) the podiums' band, between the strip and the keys */
static void op_mix_draw(const char *const val[4], const int32_t ratio[4], uint32_t flags)
{
    /* (wave 13, Ben 23 Nov 2026, 14:19: "This global screen is kind of hard to figure out ... What voice is the keys?") THE
     * MIXER, one map whichever voice is selected (hold GLO):
     *   line     what a key does, in words: TAP A KEY: MUTE; the -/+ legend: [-] CLEAR (every mute and solo off), [+] SOLO
     *            (held with a key: solo it, "plus = only this")
     *   voices   the four voices on the black keys F#3 G#3 A#3 C#4, in the strip's place, each over its knob's column: its
     *            name in its colour, its level as a bar with the meter in it; muted a yellow M (the cell yellow), soloed an
     *            ice-blue S (the cell ice blue), silenced by another's solo dim. The other seven black keys do nothing here:
     *            not drawn
     *   drums    the 16 drum sounds on the 16 white keys, 4 x 4 in the drum grid (drums.c DRUM_GRID, Ben 21:14: cymbals and
     *            hats at the top, KICK bottom left, SNARE bottom right; keys 1 .. 4 the left column top to bottom), each named (KICK, SNARE, OP HAT ...) in teal (the drum voice's); muted: yellow with a black M; soloed: ice
     *            blue with a black S; silenced by a solo: dark grey
     *   dials    the four levels (SEL: the pans), each in its voice's colour, LEVEL or PAN over it */
    static const char *const VN[4] = {"V1", "V2", "V3", "DRUM"};
    static uint32_t head, line, vo, dr, knobs, bar;
    const uint8_t *mx = op_mx;
    uint32_t i, sig = 7u, lk = ly_lock != LY_PLAY;
    op_header("GLO", C_WHITE, ui.msg_t ? ui.msg : flags & 16u ? "PANS" : "MIXER", &head);   /* (a message: in the title) */
    op_band_tick();
    sig = studio_hash(lk * 3u + 5u, ui.msg_t ? ui.msg : "");
    if (ui.force || sig != line) {                      /* the line: what a key does; -/+; LOCK while it stays open */
        int32_t x = 236;
        line = sig;
        cv_begin(240, BK_Y - OP_HEAD_H, C_BLACK);
        if (lk) {
            int32_t w = (int32_t)text_w(&FONT_S, "LOCK") + 6;
            cv_rect(x - w, 0, w, 16, C_WHITE);
            cv_text(x - w + 3, 0, &FONT_S, "LOCK", C_BLACK);
            x -= w + 6;
        }
        x = pm_draw(x, 0, "CLEAR", "SOLO") - 8;
        cv_text(4, 0, &FONT_S, x >= 4 + (int32_t)text_w(&FONT_S, "TAP A KEY: MUTE") ? "TAP A KEY: MUTE" : "TAP: MUTE", C_WHITE);
        cv_blit(0, OP_HEAD_H);
    }
    sig = 11u + (flags & 16u);
    for (i = 0; i < NTRK; i++)
        sig = sig * 131u + mx[i] / 4u + (uint32_t)ratio[i] * 7u + (uint32_t)trk_silent(&trk[i]) * 3u + (trk[i].p[P_MUTE] != 0) * 5u;
    sig = sig * 31u + song.solo + drum_solo * 3u;
    if (ui.force || sig != vo) {                        /* the four voices, in the strip's place (y 40 .. 71) */
        vo = sig;
        cv_begin(240, 32, C_BLACK);
        for (i = 0; i < NTRK; i++) {
            int32_t x = 1 + 60 * (int32_t)i, w = 58, lv = ratio[i] < 0 ? 0 : ratio[i], m = (int32_t)mx[i];
            uint32_t mute = trk[i].p[P_MUTE] != 0, solo = (song.solo >> i) & 1u, quiet = trk_silent(&trk[i]) && !mute;
            uint16_t c = TE_COL[i], bg = mute ? TE_MUTE : solo ? TE_SOLO : TE_G1, fg = mute || solo ? C_BLACK : quiet ? TE_G2 : c;
            if (flags & 16u)                            /* (the pans' page: the bar is the level still) */
                lv = (int32_t)(i == TRK_DRUM ? song.g[G_DRLVL] : trk[i].p[P_LEVEL]) * 1000 / 127;
            cv_rect(x, 0, w, 31, bg);
            if (!mute && !solo)
                op_box(x, 0, w, 31, quiet ? TE_G2 : TE_MID[i]);
            cv_text(x + 4, 0, &FONT_S, VN[i], fg);
            if (mute || solo)                           /* (the cell yellow or ice blue: its letter black at the right) */
                cv_text(x + w - 12, 0, &FONT_S, mute ? "M" : "S", C_BLACK);
            cv_rect(x + 4, 20, w - 8, 7, mute || solo ? C_BLACK : TE_G1);   /* the level, the meter in it */
            cv_rect(x + 4, 20, (w - 8) * lv / 1000, 7, mute || solo ? TE_G2 : quiet ? TE_G2 : TE_DIM[i]);
            if (!mute && !quiet)
                cv_rect(x + 4, 22, (w - 8) * lv / 1000 * (m > 82 ? 82 : m) / 82, 3, solo ? C_BLACK : c);
        }
        cv_blit(0, BK_Y);
    }
    bk_drawn = 0xFu, bk_drawn_f = ui.frame;            /* (the lights: F#3 .. C#4 live, the rest dark) */
    sig = 13u + drum_mute * 7u + (uint32_t)drum_solo * 131071u + (uint32_t)trk_silent(TDRUM) * 5u;
    if (ui.force || sig != dr) {                        /* the 16 drum sounds, 4 x 4 (y 74 .. 152) */
        dr = sig;
        cv_begin(240, 80, C_BLACK);
        for (i = 0; i < 16u; i++) {                     /* (i: the grid's place; l: the sound there) */
            int32_t x = 1 + (int32_t)(i % 4u) * 60, y = (int32_t)(i / 4u) * 20;
            uint32_t l = DRUM_GRID[i], m = (drum_mute >> l) & 1u, so = (drum_solo >> l) & 1u,
                     quiet = !m && (trk_silent(TDRUM) || drum_silenced(l));
            char nm[8];
            te_upper(nm, LANE_SHORT[l], sizeof nm);
            if (m || so) {                              /* (the tile yellow / ice blue, the name black; at its right end the
                                                         * letter, M or S, in a black tab. The drum grid: the tab flush with
                                                         * the tile's right edge, 8 px, so a six-letter name (OP HAT, SHAKER,
                                                         * C-BELL, 48 px) keeps a pixel clear of the tile's edge and the tab) */
                cv_rect(x, y, 58, 18, m ? TE_MUTE : TE_SOLO);
                te_text_c(x + 25, y + 1, nm, C_BLACK);
                cv_rect(x + 50, y + 2, 8, 14, C_BLACK);
                cv_text(x + 51, y + 1, &FONT_S, m ? "M" : "S", m ? TE_MUTE : TE_SOLO);
            } else {
                cv_rect(x, y, 58, 18, TE_G1);
                te_text_c(x + 29, y + 1, nm, quiet ? TE_G2 : TE_COL[TRK_DRUM]);
            }
        }
        cv_blit(0, BK_Y + 34);
    }
    {
        uint8_t m = ui.msg_t;
        static const char *const LV[4] = {"LEVEL", "LEVEL", "LEVEL", "LEVEL"}, *const PN[4] = {"PAN", "PAN", "PAN", "PAN"};
        ui.msg_t = 0;
        dial_tint = TE_COL;                             /* the mixer: one knob per voice, in its colour */
        op_dials(flags & 16u ? PN : LV, val, ratio, 0, 0, C_WHITE, 4242u + (flags & 16u), &knobs);
        dial_tint = 0;
        ui.msg_t = m;
    }
    op_trackbar(&bar);
}

/* ----------------------------------------------------------------- HOME --- */
static uint32_t home_band_tick(void);                   /* ui_vis.c */
static void home_band(int32_t x0, int32_t y0, uint32_t w, uint32_t h);
/* HOME (PHEBUS wave 7; Ben's picker "HOME: B: three zones", review/direction/mock-home-B: SYNTH, DRUM and PERFORM panels,
 * one per pillar). Between the header (PATTERN A..D while one plays, the unsaved dot) and the knobs:
 *   deck     the bar of the selected track's loop, a strip of the FLOW backdrop moving with the four voices' levels (Ben
 *            kept it: ui_vis.c home_band), the patterns A..D (the one playing filled, the stored ones outlined bright)
 *   SYNTH    the three melodic voices side by side: the sound's name in its voice's colour, the selected one inverted;
 *            M (yellow) muted, S (ice blue) soloed, a red dot recording; under each its 16 steps in view (a note lit in the
 *            voice's colour, a tie dim, the step that plays white)
 *   DRUM     the kit (a * when the project's copy plays), M / S for the drum track, how many sounds are muted (M) or
 *            soloed (S) in MUTE mode, the selected sound's name; the 16 trigs in view: the selected sound's in teal, the
 *            other sounds' dim teal, only muted sounds' grey, the playhead white. Inverted DRUM: the drums are selected
 *   PERFORM  the STUTTER lane ON / OFF and the PERFORM pads latched (their names, or how many), white and grey (everyone's);
 *            the lane's 16 steps in view as spans (lit steps in a row with one length: one loop, one bar), the playhead
 * The colour rule: a voice's colour only on that voice's things; the deck, PERFORM and the zone names white and grey. */
#define HB_X 64                                         /* the FLOW strip: x, y in the deck, its size (2 x 2 blocks) */
#define HB_Y 2
#define HB_W 104u                                       /* (wave 8: 12 px less, for the ">" after it: SELECT right opens the
                                                         * screensaver, HOME's next page) */
#define HB_H 14u
#define HZ_DECK 18                                      /* the deck's height, then three zones of HZ_H rows */
#define HZ_H 38
static uint32_t home_zone_sig[4];                       /* (the host tests read what each zone last drew) */
static uint8_t home_inv_zone;                           /* the zone whose name or voice is inverted: 0..2 a voice, 3 DRUM */
static void hz_label(int32_t x, int32_t y, const char *s, uint16_t c, int inv)   /* a zone's or voice's name; inverted: selected */
{
    if (inv) {
        cv_rect(x - 2, y, text_w(&FONT_S, s) + 3, 16, c);
        cv_text(x, y, &FONT_S, s, C_BLACK);
    } else {
        cv_text(x, y, &FONT_S, s, c);
    }
}
static int32_t hz_badge(int32_t x, int32_t y, uint32_t muted, uint32_t solo, uint32_t rec)   /* M, S or a red dot; the x after */
{
    if (rec) {
        te_disc(x + 4, y + 7, 3, TE_RED);
        return x + 10;
    }
    if (muted || solo) {
        op_badge(x, y + 1, muted ? 'M' : 'S', muted ? TE_MUTE : TE_SOLO);
        return x + 12;
    }
    return x;
}
static void home_kit_name(char *nm)                     /* (wave 12) the kit's name; CUSTOM once a slot differs from it */
{
    drum_kit_name(nm, 11);
}
static void op_home_body(void)
{
    static uint32_t top_sig, zsig[3];
    track_t *ts = TSEL;
    uint32_t i, j, playing = song.playing, slen = trk_len(ts), spos = ts->seq_idx % slen, sig, moving;
    uint32_t ready = arrangement_ready();
    op_band_tick();
    moving = disco == DISCO_QUIET ? 0u : home_band_tick();   /* (the voices' levels into the strip; 1: it flows. Wave 11: QUIET,
                                                         * it stands still) */
    home_inv_zone = (uint8_t)(song.sel & 3u);
    sig = playing * 3u + (playing ? spos / 16u * 31u : 0u) + ready * 5u + (uint32_t)(live_sec + 1) * 77u + slen * 13u +
          (uint32_t)(live_req + 1) * 7u + (moving ? ui.frame / 2u * 7919u + 1u : 0u);
    if (ui.force || sig != top_sig) {                   /* the deck */
        char b[8] = "BAR ";
        uint32_t bar = playing ? spos / 16u + 1u : 1u;
        top_sig = sig;
        cv_begin(240, HZ_DECK, C_BLACK);
        cv_text(4, 1, &FONT_S, "BAR", TE_G3);
        b[0] = (char)('0' + bar / 10u % 10u), b[1] = (char)('0' + bar % 10u), b[2] = 0;
        cv_text(4 + text_w(&FONT_S, "BAR") + 4, 1, &FONT_S, b, playing ? C_WHITE : TE_G3);
        home_band(HB_X, HB_Y, HB_W, HB_H);
        cv_text(HB_X + (int32_t)HB_W + 2, 1, &FONT_S, ">", TE_G3);   /* (SELECT right: this band full screen) */
        for (i = 0; i < 4u; i++) {                      /* the patterns */
            char ch[2] = {(char)('A' + i), 0};
            int32_t x = 188 + (int32_t)i * 12;
            int on = live_sec == (int8_t)i, used = (ready >> i) & 1u, next = live_req == (int8_t)i && !on;
            if (on)
                cv_rect(x, 1, 11, 16, C_WHITE);
            else
                op_box(x, 1, 11, 16, next ? C_WHITE : used ? TE_G3 : TE_G1);
            cv_text(x + 2, 1, &FONT_S, ch, on ? C_BLACK : used || next ? TE_G4 : TE_G2);
        }
        cv_blit(0, OP_BODY_Y);
    }
    /* SYNTH: the three voices */
    sig = song.sel * 7u + playing * 3u;
    for (i = 0; i < NPART; i++) {
        const track_t *t = &trk[i];
        uint32_t len = trk_len(t), pos = t->seq_idx % len;
        char b[16];
        trk_short_name(i, b);
        sig = studio_hash(sig * 31u + (t->p[P_MUTE] != 0) * 3u + ((song.solo >> i) & 1u) * 5u + ((song.rec >> i) & 1u) * 11u +
                          len * 13u + (playing ? pos + 1u : 0u) * 977u, b);
        for (j = 0; j < len; j++)
            sig = sig * 3u + (step_on(&t->step[j]) ? 1u : t->step[j].time == ST_TIE ? 2u : 0u);
    }
    home_zone_sig[0] = sig;
    if (ui.force || sig != zsig[0]) {
        zsig[0] = sig;
        cv_begin(240, HZ_H, C_BLACK);
        cv_rect(0, 0, 240, 1, TE_G1);
        cv_text(4, 1, &FONT_S, "SYNTH", TE_G3);
        for (i = 0; i < NPART; i++) {
            const track_t *t = &trk[i];
            uint32_t len = trk_len(t), pos = t->seq_idx % len, bank = playing ? pos / 16u : 0u, muted = t->p[P_MUTE] != 0;
            uint32_t sel = song.sel == i, solo = (song.solo >> i) & 1u, rec = (song.rec >> i) & 1u, n;
            int32_t x0 = 4 + (int32_t)i * 79, room = 70;
            uint16_t col = muted ? TE_G3 : TE_COL[i];
            char b[16];
            trk_short_name(i, b);
            if (muted || solo || rec)
                room -= 12;
            for (n = str_len(b); n && text_w(&FONT_S, b) > room; n--)   /* (cut to its room: never into the next voice) */
                b[n - 1u] = 0;
            hz_label(x0, 17, b, col, (int)sel);
            hz_badge(x0 + 2 + text_w(&FONT_S, b), 16, muted, solo, rec);
            for (j = 0; j < 16u; j++) {                 /* its steps in view: 4 px a step */
                uint32_t p = bank * 16u + j;
                int32_t x = x0 + (int32_t)j * 4 + (int32_t)(j / 4u);
                uint16_t c;
                if (p >= len)
                    continue;
                c = playing && p == pos ? C_WHITE : step_on(&t->step[p]) ? col
                  : t->step[p].time == ST_TIE ? (muted ? TE_G2 : TE_DIM[i]) : TE_G1;
                cv_rect(x, 34, 3, 3, c);
            }
        }
        cv_blit(0, OP_BODY_Y + HZ_DECK);
    }
    /* DRUM */
    {
        uint32_t len = trk_len(TDRUM), pos = TDRUM->seq_idx % len, bank = playing ? pos / 16u : 0u, l = drum_lane & 15u;
        uint32_t muted = TDRUM->p[P_MUTE] != 0, solo = (song.solo >> TRK_DRUM) & 1u, rec = (song.rec >> TRK_DRUM) & 1u;
        uint32_t dm = drum_mute, ds = drum_solo;
        char nm[12];
        home_kit_name(nm);
        sig = studio_hash(l * 7u + muted * 3u + solo * 5u + rec * 11u + dm * 13u + ds * 17u + len * 19u + (song.sel == TRK_DRUM) * 23u +
                          (playing ? pos + 1u : 0u) * 977u + bank, nm);
        for (j = 0; j < 16u; j++)
            sig = sig * 31u + (bank * 16u + j < len ? dt_mask(bank * 16u + j) : 0u);
        home_zone_sig[1] = sig;
        if (ui.force || sig != zsig[1]) {
            int32_t x;
            uint32_t audible = 0xFFFFu & ~dm;
            zsig[1] = sig;
            if (ds)
                audible = ds;
            cv_begin(240, HZ_H, C_BLACK);
            cv_rect(0, 0, 240, 1, TE_G1);
            hz_label(4, 1, "DRUM", muted ? TE_G3 : TE_DRUM, song.sel == TRK_DRUM);
            x = hz_badge(4 + text_w(&FONT_S, "DRUM") + 3, 0, muted, solo, rec);
            x = cv_text(x + 4, 1, &FONT_S, nm, TE_G3) + 4;
            if (dm || ds) {                             /* the sounds muted or soloed in MUTE mode, how many */
                uint32_t m = ds ? ds : dm, n = 0;
                char c[4];
                for (; m; m &= m - 1u)
                    n++;
                op_badge(x, 2, ds ? 'S' : 'M', ds ? TE_SOLO : TE_MUTE);
                fmt_int(c, (int32_t)n);
                cv_text(x + 12, 1, &FONT_S, c, ds ? TE_SOLO : TE_MUTE);
            }
            cv_text(236 - text_w(&FONT_S, LANE_SHORT[l]), 1, &FONT_S, LANE_SHORT[l], muted || !((audible >> l) & 1u) ? TE_G3 : TE_DRUM);
            for (j = 0; j < 16u; j++) {                 /* the trigs in view */
                uint32_t p = bank * 16u + j, m;
                int32_t xx = 4 + (int32_t)j * 14 + (int32_t)(j / 4u) * 2;
                if (p >= len)
                    continue;
                m = dt_mask(p);
                if (m) {
                    uint16_t c = muted || !(m & audible) ? TE_G3 : (m >> l) & 1u ? TE_DRUM : TE_DIM[3];
                    cv_rect(xx, 19, 12, 13, c);
                } else {
                    op_box(xx, 19, 12, 13, j % 4u ? TE_G1 : TE_G2);
                }
                if (playing && p == pos)
                    cv_rect(xx, 34, 12, 2, C_WHITE);
            }
            cv_blit(0, OP_BODY_Y + HZ_DECK + HZ_H);
        }
    }
    /* PERFORM: the STUTTER lane and the latched pads */
    {
        uint32_t L = stl.len ? stl.len : 16u, pg = playing ? stl_pos / 16u : 0u, lat = perf.lat;
        sig = stl.act * 3u + lat * 7u + L * 13u + (playing ? stl_pos + 1u : 0u) * 977u + pg;
        for (j = 0; j < sizeof stl.on; j++)
            sig = sig * 31u + stl.on[j];
        for (j = 0; j < sizeof stl.lc; j++)
            sig = sig * 31u + stl.lc[j];
        home_zone_sig[2] = sig;
        if (ui.force || sig != zsig[2]) {
            char t[32];
            int32_t x;
            zsig[2] = sig;
            cv_begin(240, HZ_H, C_BLACK);
            cv_rect(0, 0, 240, 1, TE_G1);
            x = cv_text(4, 1, &FONT_S, "PERFORM", TE_G3) + 6;
            cv_text(x, 1, &FONT_S, stl.act ? "LANE ON" : "LANE OFF", stl.act ? TE_G4 : TE_G3);
            str_cpy(t, "FX>STUT", sizeof t);             /* (wave 14) where the lane lives, while no pad is latched */
            if (lat) {                                  /* the latched pads by name, else how many */
                uint32_t p;
                t[0] = 0;
                for (p = 0; p < PF_NPAD; p++)
                    if ((lat >> p) & 1u && str_len(t) + str_len(PFD[p].pad) + 1u < sizeof t) {
                        if (t[0])
                            str_cpy(t + str_len(t), " ", 2);
                        str_cpy(t + str_len(t), PFD[p].pad, 8);
                    }
                if (236 - text_w(&FONT_S, t) < 150) {
                    fmt_int(t, (int32_t)pf_count(lat));
                    str_cpy(t + str_len(t), " LATCHED", 10);
                }
            }
            cv_text(236 - text_w(&FONT_S, t), 1, &FONT_S, t, lat ? C_WHITE : TE_G3);
            for (j = 0; j < 16u; j++) {                 /* the lane's steps in view: a run of lit steps of one length, one span */
                uint32_t s = pg * 16u + j;
                int32_t xx = 4 + (int32_t)j * 14 + (int32_t)(j / 4u) * 2;
                if (s >= L)
                    continue;
                if (stl_lit(s)) {
                    int join = j < 15u && s + 1u < L && stl_lit(s + 1u) && stl_code(s + 1u) == stl_code(s);
                    cv_rect(xx, 19, join ? (j % 4u == 3u ? 16 : 14) : 12, 13, stl.act ? TE_G4 : TE_G2);
                } else {
                    op_box(xx, 19, 12, 13, j % 4u ? TE_G1 : TE_G2);
                }
                if (playing && s == stl_pos)
                    cv_rect(xx, 34, 12, 2, C_WHITE);
            }
            cv_blit(0, OP_BODY_Y + HZ_DECK + 2 * HZ_H);
        }
    }
}

#include "ui_synth.c"            /* (PHEBUS wave 4) the melodic voices' screens, the MiniFreak's way */
