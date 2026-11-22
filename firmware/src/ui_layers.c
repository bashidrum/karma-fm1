/* SPDX-License-Identifier: GPL-3.0-only */
/* LAYERS, teenage-engineering style: hold a function button, touch a key (or turn a knob). While the
 * button is held the screen shows what the 16 white keys and KNOB 1..4 do now; tapped (pressed and
 * let go without touching anything) the button opens its pages as before.
 *   FX    PERFORM: the keys are pads       knobs: the last pad's (ui_perf.c, punch.c perf_*)
 *   EDIT  erase that sound / note (seq.c)  knobs: SHIFT  LENGTH x2 / half  TRANSPOSE   OCT- undo  OCT+ redo
 *   ARP   note repeat (seq.c roll)         knobs: RATE
 *   SEQ   the keys do nothing              knobs: -  DIV  SWING  STEPS (PHEBUS, Ben 21 Nov 2026: "Remove them": the
 *                                                 GRID is the one place steps are edited); the screen draws the
 *                                                 pattern's steps in time: their spacing, the swing, how many
 *   SEL   any key: the key of the song     knobs: CHORD  SCALE  KEYS  TRANSPOSE
 *   GLO   keys 1..4 mute, 5..8 solo,       knobs: the levels of tracks 1..4
 *         9 fill while held, 10 fill the next bar, the last white key: tap tempo
 *   SAVE  keys 1..4 play section A..D (playing: on the next bar), 5..8 store the loop into A..D
 * The keys' part runs in the audio ISR (seq.c layer_now: no lag, no lost press); the SEQ, SEL and
 * GLO keys come to the UI through seq.c lk_q. HOLD: REC held clears the track, SAVE held saves the
 * project (a ring fills; let go before and nothing happens). */
static const uint8_t LAYER_BTN[LY_COUNT] = {NB, B_FX, B_EDIT, B_ARP, B_SEQ, B_SCL, B_GLO, B_SAVE};
static const char *const LAYER_NAME[LY_COUNT] = {"", "perform", "erase", "roll", "steps", "key", "mix", "pattern"};   /* (wave 7: A to D are patterns, Ben's word) */
static const char *layer_name(uint32_t l)               /* (PHEBUS wave 3: on the drum track SEL picks a sound, ARP retrigs) */
{
    if (is_drum(TSEL) && l == LY_SCALE)
        return drum_keys == DK_MUTE ? "solo" : "sound";
    if (is_drum(TSEL) && l == LY_ROLL)
        return "retrig";
    return LAYER_NAME[l % LY_COUNT];
}
static const char *const LAYER_BTN_NAME[LY_COUNT] = {"", "fx:", "edit:", "arp:", "seq:", "sel:", "glo:", "save:"};   /* the button
                                                         * held, before the layer's name: "FX: PUNCH" (review 21) */
static void section_store(uint32_t s);                  /* project.c */
static void perf_knob(uint32_t k, int32_t s);           /* ui_perf.c (wave 6: PERFORM) */
static void section_load(uint32_t s);
/* (wave 8, Ben 22 Nov 2026: "keep them few and consistent") THE TIMES, all of them, in one place (GUIDE.md section 2):
 *   TAP_MS         a press shorter than this, nothing else touched: a tap (a button's pages)
 *   SHOW_MS        a layer shows after this, so a tap does not flash it
 *   LONG_MS        the long press: a layer button let go after it, untouched, stays open; HOME held this long opens the
 *                  menu; REC held this long becomes "clear the voice"
 *   HOLD_CLEAR_MS  the ring of the confirm: REC held this long more (or, with a question showing, this long from its press)
 * No action waits for a second press: nothing is reached by pressing twice quickly */
#define TAP_MS 450u                                     /* a press shorter than this, untouched: a tap */
#define SHOW_MS 140u                                    /* the layer shows after this (a tap does not flash it) */
#define LONG_MS 700u                                    /* the long press */
static uint8_t mix_pan;                                 /* (PHEBUS) GLO + key 11: the mixer's knobs are the pans, until it
                                                         * closes (HOME's KNOB 4 was PAN; VOICE 2 has it too) */

/* the ISR's view of the panel: the layers' buttons, OCT- / OCT+ */
static void layers_init(void)
{
    uint32_t l;
    for (l = LY_FX; l < LY_COUNT; l++)
        ly_bit[l] = 1u << panel.btn[LAYER_BTN[l]];
    dyn_bit[0] = 1u << panel.btn[B_OCTDN];
    dyn_bit[1] = 1u << panel.btn[B_OCTUP];
}

static uint32_t key_of_white(uint32_t w) { return key_of_lane(w & 15u); }   /* white key w -> key index */

/* ------------------------------------------------------------ tools --- */
/* EDIT layer knobs; each a step of one undo (the layer's hold) */
static void layer_undo_mark(track_t *t)
{
    if (!ui.step_sess)
        ui.step_sess = (undo_sess += 4u) | 3u;
    undo_mark(t, ui.step_sess);
}
static void pattern_rotate(track_t *t, int32_t d)       /* every step one later (d > 0) / earlier */
{
    uint32_t len = trk_len(t), i;
    step_t keep;
    layer_undo_mark(t);
    fm1_irq_off();
    if (is_drum(t)) {                                   /* (PHEBUS wave 2) the drum trigs, their extras and locks with them */
        uint8_t kv[DRUM_LANES];
        uint32_t k, sh = d > 0 ? 1u : len - 1u;
        if (d > 0) {
            memcpy(kv, dpat.vel[len - 1u], sizeof kv);
            for (i = len - 1u; i > 0; i--)
                memcpy(dpat.vel[i], dpat.vel[i - 1u], sizeof kv);
            memcpy(dpat.vel[0], kv, sizeof kv);
        } else {
            memcpy(kv, dpat.vel[0], sizeof kv);
            for (i = 0; i + 1u < len; i++)
                memcpy(dpat.vel[i], dpat.vel[i + 1u], sizeof kv);
            memcpy(dpat.vel[len - 1u], kv, sizeof kv);
        }
        for (k = 0; k < DT_NX; k++)
            if (dpat.x[k].step < len)
                dpat.x[k].step = (uint8_t)((dpat.x[k].step + sh) % len);
        for (k = 0; k < DT_NLK; k++)
            if (dpat.lk[k].step < len)
                dpat.lk[k].step = (uint8_t)((dpat.lk[k].step + sh) % len);
        fm1_irq_on();
        return;
    }
    if (d > 0) {
        keep = t->step[len - 1u];
        for (i = len - 1u; i > 0; i--)
            t->step[i] = t->step[i - 1u];
        t->step[0] = keep;
    } else {
        keep = t->step[0];
        for (i = 0; i + 1u < len; i++)
            t->step[i] = t->step[i + 1u];
        t->step[len - 1u] = keep;
    }
    fm1_irq_on();
}
static void pattern_length(track_t *t, int32_t d)       /* x2 (the pattern again after itself), or half */
{
    uint32_t len = trk_len(t), i;
    layer_undo_mark(t);
    fm1_irq_off();
    if (d > 0 && len * 2u <= NSTEP) {
        for (i = 0; i < len; i++) {
            t->step[len + i] = t->step[i];
            if (is_drum(t))                             /* (the drum trigs, with their extras and locks) */
                (void)dt_copy_step(i, len + i);
        }
        t->p[P_SLEN] = (int16_t)(len * 2u);
    } else if (d < 0 && len >= 2u) {
        t->p[P_SLEN] = (int16_t)(len / 2u);
    }
    fm1_irq_on();
    {
        char b[8];
        fmt_int(b, t->p[P_SLEN]);
        ui_say("STEPS ", b);
    }
}
static void pattern_transpose(track_t *t, int32_t d)    /* every note a semitone up / down (synth parts) */
{
    uint32_t i, j;
    if (is_drum(t))
        return;
    layer_undo_mark(t);
    fm1_irq_off();
    for (i = 0; i < NSTEP; i++)
        for (j = 0; j < t->step[i].n && j < 4u; j++)
            t->step[i].note[j] = (uint8_t)clamp(t->step[i].note[j] + (d > 0 ? 1 : -1), 0, 127);
    fm1_irq_on();
}

/* ------------------------------------------------------------- GLO --- */
/* a key of the layers the UI runs (seq.c lk_q): SEL key, GLO mix (SEQ's: taken, nothing done) */
static void layer_key(uint32_t layer, uint32_t k, uint32_t down)
{
    int32_t w = punch_key(k);
    if (layer >= LY_COUNT) {                            /* KB_GRID: the GRID's (ui_grid.c); a layer held, only */
        if (!down)                                      /* its releases come: a step held let go */
            grid_key(k, 0);
        return;
    }
    if (!down) {
        if (layer == LY_MIX && w == 8)
            fill_held = 0;                                  /* GLO + key 9 let go: the fill ends */
        return;
    }
    switch (layer) {                                    /* (SEQ: its keys do nothing, PHEBUS) */
    case LY_SCALE: {                                    /* the key of the song: every synth part */
        uint32_t i, root = (53u + k) % 12u;
        if (is_drum(TSEL)) {                            /* (PHEBUS wave 3) the drum track: SEL + a key picks its sound (the
                                                         * Rytm's TRACK + pad); in MUTE mode it solos it (again: off) */
            if (w < 0)
                return;
            if (drum_keys == DK_MUTE) {
                drum_solo ^= (uint16_t)(1u << w);
                drum_lane = (uint8_t)w;
            } else {
                drum_lane = (uint8_t)w;
                audition_lane((uint32_t)w);
                sg_drop();
            }
            ui.force = 1;
            return;
        }
        for (i = 0; i < NPART; i++)
            trk[i].p[P_ROOT] = (int16_t)root;
        ui_say("KEY ", N_NOTE[root]);
        return;
    }
    case LY_SONG: {                                     /* sections A..D: play (on the next bar), store */
        char b[2] = {0, 0};
        if (w < 0)
            return;
        b[0] = (char)('A' + (w & 3));
        if (w < 4) {
            if (!((arrangement_ready() >> w) & 1u)) {
                ui_say("EMPTY ", b);
            } else if (song.playing) {
                fm1_irq_off();
                live_req = (int8_t)w;
                fm1_irq_on();
                ui_say("NEXT: ", b);
            } else {
                section_load((uint32_t)w);
                ui_say("LOADED ", b);
            }
        } else if (w < 8) {
            uint32_t s = (uint32_t)w - 4u;
            if ((arrangement_ready() >> s) & 1u) {      /* (wave 8) over a used one it asks: hold REC until the ring fills */
                char q[8] = "STORE A";
                q[6] = b[0];
                ask_set(ASK_STORE, s, q);
            } else {
                section_store(s);
                ui_say("SAVED ", b);
            }
        }
        return;
    }
    case LY_MIX:
        if (k == 1u) {                                      /* (2.6) the black key F#3: MIDI LEARN on / off */
            ml_toggle();
        } else if (w >= 0 && w < 4) {
            trk[w].p[P_MUTE] = (int16_t)!trk[w].p[P_MUTE];
        } else if (w >= 4 && w < 8) {
            song.solo ^= (uint8_t)(1u << (w - 4));
        } else if (w == 8) {
            fill_held = 1;                                  /* FILL while held */
        } else if (w == 9) {
            fill_arm = (uint8_t)!fill_arm;                  /* FILL NEXT BAR (again: cancelled) */
            ui_message(fill_arm ? "FILL: NEXT BAR" : "FILL BAR OFF");
        } else if (w == 10) {
            mix_pan = (uint8_t)!mix_pan;                    /* (PHEBUS) key 11: the knobs are the pans (again: levels) */
            ui.force = 1;
        }                                               /* (wave 8: the tempo is GLO > GLOBAL's BPM only: no tap) */
        return;
    default:
        return;
    }
}

/* KNOB 1..4 while a layer is held: what the layer gives them (the page does not see them) */
static void layer_knobs(uint32_t layer)
{
    uint32_t k;
    int32_t s;
    track_t *t = TSEL;
    for (k = 0; k < 4u; k++) {
        if ((s = panel_enc(EN_K1 + k)) == 0)
            continue;
        ui.layer_used = 1;
        ui.hot_col = (uint8_t)k;
        ui.hot_t = 40;
        switch (layer) {
        case LY_FX:                                     /* (wave 6) PERFORM: the last pad's knobs */
            perf_knob(k, s);
            break;
        case LY_ERASE:
            if (k == 0u)
                pattern_rotate(t, s);
            else if (k == 1u)
                pattern_length(t, s);
            else if (k == 2u)
                pattern_transpose(t, s);
            break;
        case LY_ROLL:
            if (k == 0u && is_drum(t))                  /* (PHEBUS wave 3) the drum track's live RETRIG rate, 1/1 .. 1/80 */
                drt_live = (uint8_t)clamp((int32_t)drt_live + s, 1, (int32_t)NRTRG);
            else if (k == 0u)
                song.g[G_ROLL] = (int16_t)clamp(song.g[G_ROLL] + s, 0, 4);
            break;
        case LY_STEP:                                   /* GATE  DIV  SWING  STEPS (wave 5: GATE, a call for Ben) */
            if (k == 0u && !is_drum(t))
                t->p[P_SGATE] = (int16_t)clamp(t->p[P_SGATE] + accel(EN_K1, s, 126), 1, 127);
            else if (k == 1u)
                t->p[P_SDIV] = (int16_t)clamp(t->p[P_SDIV] + s, 0, (int32_t)NDIV_STEP - 1);   /* (up to 2BAR) */
            else if (k == 2u)
                t->p[P_SSWING] = (int16_t)clamp(t->p[P_SSWING] + accel(EN_K3, s, 100), 0, 100);
            else if (k == 3u)
                t->p[P_SLEN] = (int16_t)clamp(t->p[P_SLEN] + accel(EN_K4, s, 63), 1, NSTEP);
            break;
        case LY_SCALE:
            if (is_drum(t))                             /* (wave 3: the sound pick; its knobs do nothing) */
                break;
            if (k == 0u) {
                if (!is_drum(t))
                    t->p[P_CHORD] = (int16_t)clamp(t->p[P_CHORD] + s, 0, 5);
            } else if (k == 1u) {
                uint32_t i;
                int16_t v = (int16_t)clamp(trk[0].p[P_SCALE] + s, 0, NSCALES - 1);
                for (i = 0; i < NPART; i++)
                    trk[i].p[P_SCALE] = v;
            } else if (k == 2u) {
                if (!is_drum(t))
                    t->p[P_QUANT] = (int16_t)clamp(t->p[P_QUANT] + s, 0, 2);
            } else if (!is_drum(t)) {
                t->p[P_TRANS] = (int16_t)clamp(t->p[P_TRANS] + s, -24, 24);
            }
            break;
        case LY_MIX: {
            int16_t *lv = mix_pan ? &trk[k].p[P_PAN] : k == TRK_DRUM ? &song.g[G_DRLVL] : &trk[k].p[P_LEVEL];
            *lv = (int16_t)clamp(*lv + accel(EN_K1 + k, s, 127), mix_pan ? -64 : 0, mix_pan ? 63 : 127);
            break;
        }
        default:
            break;
        }
    }
}

/* --------------------------------------------------------- drawing --- */
/* 16 tiles (the white keys, 4 x 4) under a title band, the knobs' dials at the bottom */
typedef struct {
    char lab[8];
    uint16_t bg, fg, top;        /* fill, text, the 3-pixel top band (0 = none) */
} tile_t;

#define LY_SUB_Y 22                                     /* (wave 7) under the header: what the layer does now, or a message */
#define LY_TILES_Y 40                                   /* the 16 tiles, 4 rows */
#define LY_TILE_H 28
static void tiles_draw(const tile_t *tl, uint32_t *cache)
{
    uint32_t r, c, sig = 7u;
    for (r = 0; r < 16u; r++)
        sig = studio_hash(sig * 31u + tl[r].bg * 3u + tl[r].fg * 5u + tl[r].top * 7u, tl[r].lab);
    if (!ui.force && sig == *cache)
        return;
    *cache = sig;
    for (r = 0; r < 4u; r++) {                          /* (wave 7: 28 px rows between the sub line and the dials, so every
                                                         * layer has the pages' dials and the strip) */
        cv_begin(240, LY_TILE_H, C_BLACK);
        for (c = 0; c < 4u; c++) {
            const tile_t *t = &tl[r * 4u + c];
            int32_t x = 2 + (int32_t)c * 60;
            if (!t->lab[0] && t->bg == TE_G1)           /* a key that does nothing here: an outline (review 19) */
                op_box(x, 1, 56, LY_TILE_H - 2, TE_G1);
            else
                cv_rect(x, 1, 56, LY_TILE_H - 2, t->bg);
            if (t->top)
                cv_rect(x, 1, 56, 3, t->top);
            te_text_c(x + 28, 6, t->lab, t->fg);
        }
        cv_blit(0, LY_TILES_Y + (int32_t)r * LY_TILE_H);
    }
}

/* (wave 7) a layer's top: the header every screen has (the button's name as the tag, in the layer's colour; the layer's
 * name as the title; the tempo, the transport, the unsaved dot), then a line: what it does now in grey, a message in a
 * white bar, LOCK at the right while it stays open */
static const char *const LAYER_TAG[LY_COUNT] = {"", "FX", "EDIT", "ARP", "SEQ", "SEL", "GLO", "SAVE"};
static void layer_title(const char *name, const char *sub, uint16_t col, uint32_t *cache)
{
    static uint32_t hc;
    uint32_t locked = ly_lock != LY_PLAY;
    uint32_t sig = studio_hash(studio_hash(col, name), sub) + (ui.msg_t ? studio_hash(3u, ui.msg) : 0u) + locked * 7919u;
    char ti[12];
    te_upper(ti, name, sizeof ti);
    op_header(LAYER_TAG[ui.layer % LY_COUNT], col, ti, &hc);
    if (!ui.force && sig == *cache)
        return;
    *cache = sig;
    cv_begin(240, LY_TILES_Y - LY_SUB_Y, C_BLACK);
    if (ui.msg_t) {
        cv_rect(0, 0, 240, 17, C_WHITE);
        te_text_c(120, 0, ui.msg, C_BLACK);
    } else {
        cv_text(4, 0, &FONT_S, sub, TE_G3);
        if (locked) {                                   /* locked open (HOME): any button lets it go */
            int32_t w = (int32_t)text_w(&FONT_S, "LOCK") + 6;
            cv_rect(236 - w, 0, w, 16, C_WHITE);
            cv_text(239 - w, 0, &FONT_S, "LOCK", C_BLACK);
        }
    }
    cv_blit(0, LY_SUB_Y);
}

/* the SEQ layer (PHEBUS): the pattern's steps in time, a bar a row (more bars a row when it is longer than four): each
 * step a bar where it starts and as long as it lasts (DIV), the odd ones late by the swing, as many as STEPS; a step
 * that plays filled in the voice's colour, an empty one outlined; the playing one white. What the three knobs do,
 * seen as they turn */
#define BAR_U (4u * BEAT_U)
/* (wave 7) r 231 / (BAR_U per) rounded down, r <= BAR_U per, in 32 bits: BAR_U is a multiple of 64, so r 231 / 64 first
 * (exact as an integer part and a remainder part), then / (BAR_U / 64 per): the same as the 64-bit form */
static uint32_t row_px(uint32_t r, uint32_t per)
{
    return ((r >> 6) * 231u + ((r & 63u) * 231u >> 6)) / (BAR_U / 64u * per);
}
static void seq_time_draw(const track_t *t, uint16_t col, uint32_t *cache)
{
    uint32_t len = trk_len(t), div = trk_div(t), u = div_units(div), i, k;
    uint32_t sw = swings(div) ? swing_units(t->p[P_SSWING], u) : 0u;
    uint32_t bars = (len * u + BAR_U - 1u) / BAR_U, per = 1, rows, sig = len * 977u + div * 31u + sw;
    int32_t play = song.playing ? (int32_t)t->seq_idx : -1;
    while (bars > per * 4u)                             /* bars a row: 1, 2, 4 .. so that four rows hold them */
        per *= 2u;
    rows = (bars + per - 1u) / per;
    for (i = 0; i < len; i++)
        sig = sig * 3u + (is_drum(t) ? dt_mask(i) != 0 : step_on(&t->step[i]) ? 1u : t->step[i].time == ST_TIE ? 2u : 0u);
    sig += (uint32_t)(play + 1) * 7919u + col;
    if (!ui.force && sig == *cache)
        return;
    *cache = sig;
    cv_begin(240, LY_TILE_H * 4, C_BLACK);
    {
        int32_t pitch = rows <= 2u ? 46 : 28, top = (LY_TILE_H * 4 - (int32_t)rows * pitch + 3) / 2, h = pitch - 11;
        for (k = 0; k < rows; k++) {                    /* the rows: a line at each beat, brighter at each bar */
            int32_t y = top + (int32_t)k * pitch, b;
            for (b = 0; b <= (int32_t)per * 4; b++)
                cv_rect(4 + b * 231 / ((int32_t)per * 4), y, 1, h + 8, b % 4 ? TE_G1 : TE_G2);
        }
        for (i = 0; i < len; i++) {                     /* the steps: from where they start (the odd ones swung), three
                                                         * fifths of a step long, so the swing shows as pairs */
            uint32_t a = i * u + (i & 1u ? sw : 0u), e = a + (u * 3u + 4u) / 5u;   /* (wave 7: 32 bits, the device has no
                                                         * 64-bit divide; 64 steps of 2BAR: 1.4e9 units) */
            uint32_t on = is_drum(t) ? dt_mask(i) != 0
                        : step_on(&t->step[i]) ? 1u : t->step[i].time == ST_TIE ? 2u : 0u;
            uint16_t c = (int32_t)i == play ? C_WHITE : on == 1u ? col : on ? TE_DIM[song.sel & 3u] : TE_G2;
            if (t->step[i].time == ST_TIE && !is_drum(t))
                e = a + u;                              /* (a tie runs on to the next step) */
            while (a < e) {                             /* (a step longer than a row: on into the next) */
                uint32_t row = a / (BAR_U * per), re = (row + 1u) * BAR_U * per, b2 = e < re ? e : re;
                int32_t x0 = 4 + (int32_t)row_px(a - row * BAR_U * per, per) + 1;
                int32_t x1 = 4 + (int32_t)row_px(b2 - row * BAR_U * per, per) - 1, y = top + 4 + (int32_t)row * pitch;
                if (row >= 4u)
                    break;
                if (x1 < x0)
                    x1 = x0;
                if (on || (int32_t)i == play)
                    cv_rect(x0, y, x1 - x0 + 1, h, c);
                else
                    op_box(x0, y, x1 - x0 + 1, h, c);
                a = b2;
            }
        }
    }
    cv_blit(0, LY_TILES_Y);
}

#include "ui_perf.c"                                    /* (wave 6) PERFORM, the FX layer */
static uint8_t layer_shown;                              /* the screen holds a layer (or a hold) */
static const char *layer_sub_shown = "";                 /* the sub line last drawn (the host tests read it) */
static void layer_screen_draw(void)
{
    static uint32_t head, tiles, foot, bar;
    static tile_t tl[16];
    static char v[4][10], sub[24], lu[4][8];
    const char *lab[4] = {"", "", "", ""}, *val[4] = {v[0], v[1], v[2], v[3]};
    int32_t ratio[4] = {-1, -1, -1, -1};
    uint32_t i, layer = ui.layer, sel = song.sel;
    track_t *t = TSEL;
    uint16_t col = TE_COL[sel & 3u];
    if (!layer_shown) {
        lcd_fill(0, 0, 240, 240, C_BLACK);
        ui.force = 1;
        layer_shown = 1;
    }
    if (layer == LY_FX) {                               /* (wave 6) PERFORM: its own screen (ui_perf.c) */
        perf_screen_draw();
        layer_sub_shown = "";
        return;
    }
    for (i = 0; i < 4u; i++)
        v[i][0] = 0;
    memset(tl, 0, sizeof tl);
    for (i = 0; i < 16u; i++) {
        tl[i].bg = TE_G1;
        tl[i].fg = TE_G3;
    }
    str_cpy(sub, "track ", sizeof sub);                 /* (in words: TRACK 1, the DRUM TRACK, review 21) */
    sub[6] = (char)('1' + sel);
    sub[7] = 0;
    if (sel == TRK_DRUM)
        str_cpy(sub, "drum track", sizeof sub);
    switch (layer) {
    case LY_ERASE:
    case LY_ROLL: {                                     /* the keys' sounds: lit = held */
        uint32_t held = fm1_in.notes;
        int mute = layer == LY_ERASE && is_drum(t) && er_mute;   /* (2.6) EDIT + a black key: the mutes */
        col = layer == LY_ERASE ? TE_RED : col;
        str_cpy(sub, mute ? "keys mute" : layer == LY_ERASE ? (song.playing ? "as it plays" : "every step") : "hold + key", sizeof sub);
        if (layer == LY_ROLL && is_drum(t)) {           /* (PHEBUS wave 3) RETRIG: its rate, the black keys pick one */
            str_cpy(sub, "black keys: rate", sizeof sub);
        }
        for (i = 0; i < 16u; i++) {
            uint32_t k = key_of_white(i), down = (held >> k) & 1u, present = 0, j;
            if (is_drum(t)) {
                str_cpy(tl[i].lab, LANE_SHORT[i], 8);
                for (j = 0; j < trk_len(t); j++)
                    present |= (uint32_t)dt_has(j, i);
            } else {
                uint32_t n = kb_map(t, k);
                if (n == KB_SILENT) {
                    str_cpy(tl[i].lab, "-", 8);
                } else {
                    note_name(tl[i].lab, n);
                    for (j = 0; j < trk_len(t); j++)
                        if (t->step[j].time == ST_NOTE) {
                            uint32_t q;
                            for (q = 0; q < t->step[j].n; q++)
                                present |= t->step[j].note[q] == n;
                        }
                }
            }
            tl[i].bg = down ? (layer == LY_ERASE ? TE_RED : C_WHITE) : TE_G1;
            tl[i].fg = down ? C_BLACK : present ? C_WHITE : TE_G3;
            tl[i].top = present && !down ? (layer == LY_ERASE ? TE_RED : col) : 0;
            if (mute) {                                 /* MUTE: a muted sound dark, with the yellow of M (the colour rule:
                                                         * yellow is mute, red only records and erases) */
                uint32_t off = (drum_mute >> i) & 1u;
                tl[i].bg = TE_G1;
                tl[i].fg = off ? TE_G3 : present ? C_WHITE : TE_G3;
                tl[i].top = off ? TE_MUTE : 0;
            }
        }
        if (layer == LY_ERASE && is_drum(t) && !mute && !undo.valid)
            str_cpy(sub, "black = mute", sizeof sub);
        if (layer == LY_ERASE) {
            lab[0] = "shift", lab[1] = "steps", lab[2] = is_drum(t) ? "" : "transp";   /* (STEPS: the pattern's length) */
            str_cpy(v[0], "<  >", 8);
            fmt_int(v[1], t->p[P_SLEN]);
            str_cpy(v[2], is_drum(t) ? "" : "-  +", 8);
            if (!mute)                                  /* what OCT- / OCT+ do now (one step: undo, then redo) */
                str_cpy(sub, undo.valid ? (undo.undone ? "oct+ redo" : "oct- undo") : is_drum(t) ? sub : "no undo yet",
                        sizeof sub);
        } else if (is_drum(t)) {
            lab[0] = "rate";
            str_cpy(v[0], SG_RATE[drt_live % 18u], 8);
            ratio[0] = ((int32_t)drt_live - 1) * 1000 / ((int32_t)NRTRG - 1);
        } else {
            lab[0] = "rate";
            str_cpy(v[0], N_ROLL[clamp(song.g[G_ROLL], 0, 4)], 8);
            ratio[0] = song.g[G_ROLL] * 250;
        }
        break;
    }
    case LY_STEP: {                                     /* DIV  SWING  STEPS, the pattern drawn in time below */
        uint32_t len = trk_len(t), bars = (len * div_units(trk_div(t)) + BAR_U - 1u) / BAR_U;
        fmt_int(sub, (int32_t)len);                     /* "16 steps = 1 bar" */
        str_cpy(sub + str_len(sub), len == 1u ? " step = " : " steps = ", 10);
        fmt_int(sub + str_len(sub), (int32_t)bars);
        str_cpy(sub + str_len(sub), bars == 1u ? " bar" : " bars", 6);
        lab[0] = "gate", lab[1] = "div", lab[2] = "swing", lab[3] = "steps";
        if (is_drum(t))                                 /* (a drum hit is one-shot: no gate) */
            str_cpy(v[0], "--", 4), ratio[0] = 0;
        else
            fmt_int(v[0], (t->p[P_SGATE] * 100 + 63) / 127), ratio[0] = t->p[P_SGATE] * 1000 / 127;
        str_cpy(v[1], N_SDIV[t->p[P_SDIV] % NDIV_STEP], 8);
        swing_str(v[2], t->p[P_SSWING]);
        fmt_int(v[3], t->p[P_SLEN]);
        ratio[1] = t->p[P_SDIV] * 1000 / (NDIV_STEP - 1);
        ratio[2] = t->p[P_SSWING] * 10;
        ratio[3] = (t->p[P_SLEN] - 1) * 1000 / 63;
        break;
    }
    case LY_SCALE: {                                    /* the white keys' notes / chords; the root lit */
        uint32_t root = (uint32_t)trk[0].p[P_ROOT] % 12u, mask = SCALE_MASK[clamp(trk[0].p[P_SCALE], 0, NSCALES - 1)];
        if (is_drum(t)) {                               /* (PHEBUS wave 3) the drum track: the 16 sounds, the key picks one
                                                         * (MUTE mode: solos it); the selected one filled, a muted one with
                                                         * the yellow of M, a soloed one ice blue */
            int solo = drum_keys == DK_MUTE;
            col = TE_DRUM;
            str_cpy(sub, solo ? "key: solo" : "key: pick a sound", sizeof sub);
            for (i = 0; i < 16u; i++) {
                uint32_t j, present = 0, m = (drum_mute >> i) & 1u, so = (drum_solo >> i) & 1u;
                for (j = 0; j < trk_len(t); j++)
                    present |= (uint32_t)dt_has(j, i);
                str_cpy(tl[i].lab, LANE_SHORT[i], 8);
                tl[i].bg = i == drum_lane ? TE_DRUM : TE_G1;
                tl[i].fg = i == drum_lane ? C_BLACK : m ? TE_G3 : present ? TE_DRUM : TE_G3;
                tl[i].top = so ? TE_SOLO : m ? TE_MUTE : 0;
            }
            break;
        }
        col = TE_G4;
        str_cpy(sub, "key: ", sizeof sub);
        str_cpy(sub + 5, N_NOTE[root], 4);
        str_cpy(sub + str_len(sub), " ", 2);
        te_lower(sub + str_len(sub), N_SCALE[clamp(trk[0].p[P_SCALE], 0, NSCALES - 1)], 8);
        for (i = 0; i < 16u; i++) {
            uint32_t k = key_of_white(i), pc = (53u + k) % 12u, in = (mask >> ((pc + 12u - root) % 12u)) & 1u;
            if (!is_drum(t) && t->p[P_CHORD]) {       /* chord mode: the chord this key plays (the i chord lit) */
                uint8_t c[4];
                uint32_t n = kb_map(t, k), m;
                if (n != KB_SILENT && (m = chord_notes(t, n, c)) != 0u) {
                    uint32_t third = m > 1u ? (uint32_t)(c[1] - c[0]) : 4u;
                    str_cpy(tl[i].lab, N_NOTE[c[0] % 12u], 8);
                    if (t->p[P_CHORD] == 5)
                        str_cpy(tl[i].lab + str_len(tl[i].lab), "5", 2);
                    else if (third == 3u)
                        str_cpy(tl[i].lab + str_len(tl[i].lab), "m", 2);
                    pc = c[0] % 12u;
                    in = 1;
                }
            } else {
                str_cpy(tl[i].lab, N_NOTE[pc], 8);
            }
            tl[i].bg = pc == root ? col : TE_G1;
            tl[i].fg = pc == root ? C_BLACK : in ? C_WHITE : TE_G2;
        }
        lab[0] = "chord", lab[1] = "scale", lab[2] = "keys", lab[3] = "transp";
        te_lower(v[0], N_CHORD[clamp(t->p[P_CHORD], 0, 5)], 8);
        te_lower(v[1], N_SCALE[clamp(trk[0].p[P_SCALE], 0, NSCALES - 1)], 8);
        te_lower(v[2], N_QUANT[clamp(t->p[P_QUANT], 0, 2)], 8);
        fmt_int(v[3], t->p[P_TRANS]);
        if (is_drum(t))
            v[0][0] = v[2][0] = v[3][0] = 0;
        ratio[0] = t->p[P_CHORD] * 200;
        ratio[1] = trk[0].p[P_SCALE] * 1000 / (NSCALES - 1);
        ratio[2] = t->p[P_QUANT] * 500;
        ratio[3] = (t->p[P_TRANS] + 24) * 1000 / 48;
        break;
    }
    case LY_MIX: {                                      /* mute 1..4, solo 1..4, fill / fill bar, tap */
        col = C_WHITE;
        str_cpy(sub, "mute  solo  fill  tap", sizeof sub);
        for (i = 0; i < 4u; i++) {
            int m = trk[i].p[P_MUTE] != 0, so = (song.solo >> i) & 1u;
            str_cpy(tl[i].lab, "mute 1", 8);
            tl[i].lab[5] = (char)('1' + i);
            tl[i].bg = m ? TE_G2 : TE_COL[i];
            tl[i].fg = m ? TE_G3 : C_BLACK;
            str_cpy(tl[4 + i].lab, "solo 1", 8);
            tl[4 + i].lab[5] = (char)('1' + i);
            tl[4 + i].bg = so ? C_WHITE : TE_G1;
            tl[4 + i].fg = so ? C_BLACK : TE_G3;
            tl[4 + i].top = TE_DIM[i];
        }
        str_cpy(tl[8].lab, "fill", 8);                  /* key 9: held = a fill; key 10: the next bar is one */
        tl[8].bg = fill_now ? C_WHITE : TE_G1;
        tl[8].fg = fill_now ? C_BLACK : TE_G4;
        str_cpy(tl[9].lab, "bar", 8);
        tl[9].bg = fill_bar_on ? C_WHITE : TE_G1;
        tl[9].fg = fill_bar_on ? C_BLACK : TE_G4;
        tl[9].top = fill_arm ? C_WHITE : 0;
        fmt_int(tl[15].lab, song.g[G_BPM]);
        tl[15].bg = song.playing && clk_pos < BEAT_U / 4u ? C_WHITE : TE_G2;
        tl[15].fg = tl[15].bg == C_WHITE ? C_BLACK : C_WHITE;
        str_cpy(tl[14].lab, "tap>", 8);
        tl[14].bg = C_BLACK;
        for (i = 0; i < 4u; i++) {
            uint32_t lv = trk_level(i);
            static const char *const L[4] = {"1", "2", "3", "dr"};   /* (the strip's numbers) */
            lab[i] = L[i];
            fmt_int(v[i], (int32_t)lv * 100 / 127);
            ratio[i] = (int32_t)lv * 1000 / 127;
            if (mix_pan) {                              /* (key 11: the pans, L 64 .. C .. R 63) */
                int32_t pn = trk[i].p[P_PAN];
                v[i][0] = pn < 0 ? 'L' : pn > 0 ? 'R' : 'C';
                if (pn)
                    fmt_int(v[i] + 1, pn < 0 ? -pn : pn);
                else
                    v[i][1] = 0;
                ratio[i] = (pn + 64) * 1000 / 127;
            }
        }
        break;
    }
    case LY_SONG: {                                     /* A..D (playing lit, next one framed), store */
        uint32_t ready = arrangement_ready();
        static const char *const SL[4] = {"A", "B", "C", "D"};
        col = C_WHITE;
        str_cpy(sub, song.playing ? "next bar" : "", sizeof sub);   /* (a pick while playing: on the next bar) */
        for (i = 0; i < 4u; i++) {
            int used = (ready >> i) & 1u, playing = live_sec == (int8_t)i;
            int next = live_req == (int8_t)i;
            str_cpy(tl[i].lab, SL[i], 8);
            tl[i].bg = used ? (playing ? TE_G4 : TE_G2) : TE_G1;   /* sections are everyone's: neutral */
            tl[i].fg = used ? (playing ? C_BLACK : C_WHITE) : TE_G3;
            tl[i].top = next ? C_WHITE : 0;
            str_cpy(tl[4 + i].lab, "save A", 8);
            tl[4 + i].lab[5] = (char)('A' + i);
            tl[4 + i].bg = ui.ask == ASK_STORE && ui.ask_arg == i ? TE_RED : TE_G1;
            tl[4 + i].fg = ui.ask == ASK_STORE && ui.ask_arg == i ? C_BLACK : TE_G4;
            tl[4 + i].top = 0;
        }
        for (i = 8; i < 16u; i++) {                     /* (PHEBUS wave 2) keys 9..16 do nothing here since the song cut:
                                                         * their two rows left out, not eight empty outlines */
            tl[i].lab[0] = 0;
            tl[i].bg = tl[i].fg = C_BLACK;
            tl[i].top = 0;
        }
        break;
    }
    default:
        break;
    }
    layer_sub_shown = sub;
    if (layer == LY_MIX) {                              /* the mixer: the band on stage (ui_op.c) */
        op_mix_draw(val, ratio, (uint32_t)(fill_now != 0) | (uint32_t)(fill_bar_on != 0) << 1 | (uint32_t)(fill_arm != 0) << 2 |
                    (uint32_t)(song.playing && clk_pos < BEAT_U / 4u) << 3 | (uint32_t)mix_pan << 4);
        return;
    }
    layer_title(layer_name(layer), sub, col, &head);
    if (layer == LY_STEP)
        seq_time_draw(t, col, &tiles);
    else
        tiles_draw(tl, &tiles);
    {   /* (a message shows in the title: the dials stay) */
        uint8_t m = ui.msg_t;
        ui.msg_t = 0;
        for (i = 0; i < 4u; i++) {                      /* (the pages' dials: five letters at most, upper case) */
            te_upper(v[i], v[i], sizeof v[i]);
            dval_fit(v[i]);
            te_upper(lu[i], lab[i], sizeof lu[i]);
            lab[i] = lu[i];
        }
        op_dials(lab, val, ratio, 0, 0, layer == LY_SCALE && !is_drum(t) ? TE_G4 : layer == LY_SONG ? TE_G4 :
                 layer == LY_ERASE ? TE_COL[sel & 3u] : col, layer * 7919u + sel, &foot);   /* (the key, the patterns:
                                                         * everyone's, white-grey; ERASE's knobs edit the voice: its colour) */
        ui.msg_t = m;
    }
    op_trackbar(&bar);
}

/* HOLD to confirm: a ring fills while REC is held: clear the track (1), or (wave 8) yes to the question showing (3) */
#define HOLD_CLEAR_MS 1300u                             /* after the 0.7 s that make REC a hold */
#define HOLD_SAVE_MS 1000u
static void hold_screen_draw(void)
{
    static uint32_t cache;
    uint32_t span = ui.hold_kind == 2u ? HOLD_SAVE_MS : HOLD_CLEAR_MS, el = fm1_ms - ui.hold_t0;
    const char *what = ui.hold_kind == 1u ? "clear" : ui.hold_kind == 3u ? ui.msg + 10 : "save";   /* (3: the question's
                                                         * action, after "HOLD REC: ") */
    uint16_t rc = ui.hold_kind == 2u ? C_WHITE : TE_RED;
    int32_t ratio = el >= span ? 1000 : (int32_t)(el * 1000u / span);
    uint32_t sig = studio_hash(ui.hold_kind * 7u + (uint32_t)ratio / 20u + ui.hold_trk * 131u, what);
    char b[24];
    if (!layer_shown) {
        lcd_fill(0, 0, 240, 240, C_BLACK);
        layer_shown = 1;
        cache = ~sig;
    }
    if (sig == cache && !ui.force)
        return;
    cache = sig;
    cv_begin(240, 124, C_BLACK);
    if (text_w(&FONT_L, what) <= 236)
        cv_text(120 - text_w(&FONT_L, what) / 2, 4, &FONT_L, what, rc);
    else
        te_text_c(120, 12, what, rc);
    {   /* the ring: 36 px, 7 thick, filling clockwise from the top */
        int32_t a, rr, end = ratio * 1024 / 1000;
        for (a = 0; a < 1024; a += 2) {
            int32_t co = SINE[((uint32_t)a + 768u) & 1023u], si = SINE[(uint32_t)a & 1023u];   /* from 12 o'clock */
            uint16_t c = a <= end ? rc : TE_G2;
            for (rr = 30; rr <= 36; rr++)
                cv_pset(120 + ((si * rr) >> 15), 80 + ((co * rr) >> 15), c);
        }
    }
    cv_blit(0, 20);
    cv_begin(240, 60, C_BLACK);
    if (ui.hold_kind == 3u) {
        str_cpy(b, "let go: no", sizeof b);
    } else if (ui.hold_kind == 1u) {
        str_cpy(b, "track 1", sizeof b);
        b[6] = (char)('1' + ui.hold_trk);
        if (ui.hold_trk == TRK_DRUM)
            str_cpy(b, "drum track", sizeof b);
    } else {
        str_cpy(b, "into pattern A", sizeof b);
        b[13] = (char)('A' - 1 + clamp(song.g[G_SLOT], 1, 4));
    }
    te_text_c(120, 4, b, TE_G4);
    te_text_c(120, 30, "keep holding rec", TE_G3);
    cv_blit(0, 160);
}
