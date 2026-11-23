/* SPDX-License-Identifier: GPL-3.0-only */
/* LAYERS: hold a button, touch a key or a knob (or turn MASTER). While the button is held the screen shows what the keys,
 * KNOB 1..4, OCT- / OCT+ and MASTER do now; tapped (pressed and let go without touching anything) the button opens its
 * categories. (Wave 10, Ben 23 Nov 2026: "every control has ONE idea") each hold is its button's idea, a shortcut or a
 * performance layer, never the only way:
 *   FX    PERFORM: the keys are pads; knobs: the last pad's; OCT+ latch, OCT- latches off; MASTER: FX MIX (ui_perf.c)
 *   SEQ   RHYTHM: the white keys ROLL (drums: RETRIG); black keys F#3..C#4 play pattern A..D, D#4 the fill bar (drawn
 *         in the strip); PLAY: FILL while held, OCT+ with it latches; knobs: RATE SHIFT SWING STEPS; OCT- halve, OCT+
 *         double; MASTER: SWING
 *   ARP   NOTES: OCT- / OCT+ transpose the voice's pattern a semitone; MASTER: TRANSP (KEY 2's TRANS)
 *   GLO   the MIXER: levels, pans, mutes, solos (drums: the 16 sounds'); MASTER: TEMPO
 *   SAVE  keys 1..4 store the playing state into A..D; OCT- undo, OCT+ redo; with a question showing, the confirm's ring
 *   EDIT  OCT- clears the voice (after the confirm); MASTER: LEVEL. ENV: MASTER ENV LENGTH. LFO: MASTER MOD DEPTH
 *   SEL   no layer (the PAGE button); HOME, REC: none
 * The keys' part runs in the audio ISR (seq.c layer_now); the GLO, SAVE and SEQ black keys come to the UI through seq.c
 * lk_q */
static const uint8_t LAYER_BTN[LY_COUNT] = {NB, B_FX, B_EDIT, B_ARP, B_SEQ, B_SCL, B_GLO, B_SAVE, B_ENV, B_LFO};
static const char *const LAYER_NAME[LY_COUNT] = {"", "perform", "voice", "notes", "rhythm", "", "mix", "save", "envelope",
                                                 "modulation"};
static const char *layer_name(uint32_t l) { return LAYER_NAME[l % LY_COUNT]; }
static void section_store(uint32_t s);                  /* project.c */
static void perf_knob(uint32_t k, int32_t s);           /* ui_perf.c (wave 6: PERFORM) */
static void section_load(uint32_t s);
#include "ui_master.c"                                  /* (wave 10) MASTER with a button held: its pair */
static void map_chip(uint32_t l)                        /* (wave 11) mapping: MASTER's pair of the held button, as a chip */
{
    mpair_t m;
    if (master_pair(l, &m))
        hdr_mchip = m.chip, hdr_mcol = m.col, hdr_mratio = (m.v - m.lo) * 1000 / (m.hi - m.lo);
}
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
static uint8_t mix_dsolo;                               /* (wave 9) the mixer, the drums selected: the white keys solo (F#3,
                                                         * drawn in the strip: wave 10) */
static uint8_t mix_pan;                                 /* (PHEBUS) the mixer's knobs are the pans (G#3, drawn in the strip:
                                                         * wave 10), until it closes */

/* the ISR's view of the panel: the layers' buttons, OCT- / OCT+ */
static void layers_init(void)
{
    uint32_t l;
    for (l = LY_FX; l < LY_COUNT; l++)
        ly_bit[l] = l == LY_SEL ? 0u : 1u << panel.btn[LAYER_BTN[l]];   /* (wave 9, Ben 23 Nov 2026: SEL is the PAGE button;
                                                         * held, nothing: "Leave it unused") */
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
/* (wave 9, Ben 23 Nov 2026: "-oct and sequence could halve the sequence. Or +oct and sequence could double it as a clone") SEQ
 * held + OCT- / OCT+ on the selected voice's pattern (SEQ the constant button, the pattern its feature; DOUBLE on EDIT held,
 * the same two jobs, is gone). Halving drops the second half: when it holds anything (a note, a tie, a drum trig, a lock, a
 * nudge, a condition) it asks first, HOLD REC: HALVE TO 8 (the one confirm; SEQ stays held while REC is held, as SAVE does for
 * a pattern store). Doubling clones the steps after themselves, with their locks, nudges and conditions (the drum trigs with
 * their extras and sound locks). One undo each */
static int steps_used(const track_t *t, uint32_t from, uint32_t to)
{
    uint32_t i, k;
    for (i = from; i < to && i < NSTEP; i++) {
        if (is_drum(t) ? dt_mask(i) != 0u : t->step[i].time == ST_TIE || step_on(&t->step[i]))
            return 1;
        if (t->micro[i] || step_fill(t, i))
            return 1;
        for (k = 0; k < NLOCK; k++)
            if (t->lock[k].step == i)
                return 1;
    }
    return 0;
}
static void seq_halve_do(uint32_t ti)                   /* (the question confirmed, or nothing to lose) */
{
    track_t *t = &trk[ti % NTRK];
    uint32_t len = trk_len(t), n = len / 2u, i, l;
    char b[8];
    if (len < 2u)
        return;
    undo_mark(t, (undo_sess += 4u) | 3u);
    fm1_irq_off();
    for (i = n; i < len; i++) {
        if (is_drum(t))
            for (l = 0; l < DRUM_LANES; l++)
                dt_clr(i, l);
        else
            step_clear(&t->step[i]);
        lock_del(t, i, P_COUNT);
        t->micro[i] = 0;
        step_fill_set(t, i, FC_NORM);
    }
    t->p[P_SLEN] = (int16_t)n;
    fm1_irq_on();
    sync_reload = 1;
    fmt_int(b, (int32_t)n);
    ui_say("STEPS ", b);
}
static void seq_halve(void)
{
    track_t *t = TSEL;
    uint32_t len = trk_len(t);
    char q[16] = "HALVE TO ";
    if (len < 2u) {
        ui_message("1 STEP: NO HALF");
        return;
    }
    if (!steps_used(t, len / 2u, len)) {
        seq_halve_do(song.sel);
        return;
    }
    fmt_int(q + 9, (int32_t)(len / 2u));
    ask_set(ASK_HALVE, song.sel, q);
}
static void seq_double(void)
{
    track_t *t = TSEL;
    uint32_t len = trk_len(t), i, k, lost = 0;
    char b[8];
    if (len * 2u > NSTEP) {
        ui_message("64 STEPS: FULL");
        return;
    }
    undo_mark(t, (undo_sess += 4u) | 3u);
    fm1_irq_off();
    for (i = 0; i < len; i++) {
        lock_del(t, len + i, P_COUNT);
        if (is_drum(t))
            lost |= (uint32_t)!dt_copy_step(i, len + i);
        else
            t->step[len + i] = t->step[i];
        t->micro[len + i] = t->micro[i];
        step_fill_set(t, len + i, step_fill(t, i));
    }
    for (k = 0; k < NLOCK; k++)                         /* (the locks: a copy each, as many as the 24 hold) */
        if (t->lock[k].step < len && !lock_set(t, t->lock[k].step + len, t->lock[k].param, t->lock[k].val))
            lost = 1;
    t->p[P_SLEN] = (int16_t)(len * 2u);
    fm1_irq_on();
    sync_reload = 1;
    if (lost) {
        ui_message("DOUBLED: SOME LOCKS LEFT OUT");
        return;
    }
    fmt_int(b, (int32_t)len * 2);
    ui_say("CLONED: STEPS ", b);
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

/* ------------------------------------------------------- patterns --- */
/* (wave 10, Ben: "The sequence button should be in charge of patterns") play pattern w (0..3, A..D): playing, on the next bar
 * (NEXT: A); stopped, loaded (LOADED A); empty: EMPTY A. SEQ's A-D category (KNOB 1, -/+) and hold SEQ's black keys 1 to 4 */
static void patt_pick(int32_t w)
{
    char b[2] = {(char)('A' + (w & 3)), 0};
    if (w < 0 || w > 3)
        return;
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
    ui.force = 1;
}
static int32_t patt_shown(void) { return live_req >= 0 ? live_req : live_sec; }
/* (wave 10) storing stays SAVE's (Ben's "save keeps"): hold SAVE + key 1..4 stores the playing state into A..D; over a used
 * one it asks, and SAVE is held already, so the ring fills at once (let go before: nothing) */
static void patt_store(uint32_t w)
{
    char b[2] = {(char)('A' + (w & 3u)), 0};
    if ((arrangement_ready() >> (w & 3u)) & 1u) {
        char q[8] = "STORE A";
        q[6] = b[0];
        ask_set(ASK_STORE, w & 3u, q);
    } else {
        section_store(w & 3u);
        ui_say("SAVED ", b);
    }
}
/* (wave 10, the black-key strip: Ben, "Only live when drawn") the 11 black keys, F#3 .. F#5, by their key numbers */
static int32_t black_idx(uint32_t k)
{
    uint32_t i;
    for (i = 0; i < 11u; i++)
        if (PUNCH_BLACK[i] == k)
            return (int32_t)i;
    return -1;
}

/* ------------------------------------------------------------ keys --- */
/* a key of the layers the UI runs (seq.c lk_q): GLO the mixer, SAVE the store, SEQ's black keys */
static int combo_live(uint32_t h, uint32_t c);          /* (wave 11, ui_lights.c) */
#define HC_LY(l) (6u + (l))                             /* (ui_lights.c HC_LY0 + a layer) */
static void layer_key(uint32_t layer, uint32_t k, uint32_t down)
{
    int32_t w = punch_key(k), bi = black_idx(k);
    if (layer >= LY_COUNT) {                            /* KB_GRID: the GRID's (ui_grid.c); a layer held, only */
        if (!down)                                      /* its releases come: a step held let go */
            grid_key(k, 0);
        return;
    }
    if (!down || !combo_live(HC_LY(layer), 14u + k))    /* (wave 11: what the lights say works, works; nothing else) */
        return;
    switch (layer) {
    case LY_SEQ:                                        /* black keys 1 to 4: A to D; D#4: FILL on the next bar (again: off) */
        if (bi >= 0 && bi < 4)
            patt_pick(bi);
        else if (bi == 4) {
            fill_arm = (uint8_t)!fill_arm;
            ui_message(fill_arm ? "FILL: NEXT BAR" : "FILL BAR OFF");
        }
        return;
    case LY_SAVE:                                       /* keys 1 to 4: store into A to D */
        if (w >= 0 && w < 4)
            patt_store((uint32_t)w);
        return;
    case LY_GLO:                                        /* the mixer: keys 1 to 4 mute the voices, 5 to 8 solo them; the drums
                                                         * selected, the 16 keys mute the sounds and F#3 SOLO turns them into
                                                         * solo keys. G#3 PAN: the knobs the pans. Both drawn in its strip.
                                                         * Mutes and solos are performance state */
        if (bi == 0 && is_drum(TSEL)) {
            mix_dsolo = (uint8_t)!mix_dsolo;
        } else if (bi == 1) {
            mix_pan = (uint8_t)!mix_pan;
        } else if (is_drum(TSEL)) {                     /* (wave 9: the drums selected, the 16 sounds) */
            if (w >= 0 && w < 16) {
                if (mix_dsolo)
                    drum_solo ^= (uint16_t)(1u << w);
                else
                    drum_mute ^= (uint16_t)(1u << w);
                drum_lane = (uint8_t)w;
            }
        } else if (w >= 0 && w < 4) {
            trk[w].p[P_MUTE] = (int16_t)!trk[w].p[P_MUTE];
        } else if (w >= 4 && w < 8) {
            song.solo ^= (uint8_t)(1u << (w - 4));
        }
        ui.force = 1;
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
        case LY_SEQ:                                    /* (wave 10) RATE (the roll's; drums the RETRIG's), SHIFT, SWING, STEPS:
                                                         * the rhythm's quick ones (GATE and DIV: SEQ's PATTERN page) */
            if (k == 0u && is_drum(t))
                drt_live = (uint8_t)clamp((int32_t)drt_live + s, 1, (int32_t)NRTRG);
            else if (k == 0u)
                song.g[G_ROLL] = (int16_t)clamp(song.g[G_ROLL] + s, 0, 4);
            else if (k == 1u)
                pattern_rotate(t, s);
            else
                patt_turn(t, 0, k, s);
            break;
        case LY_LFO:
        case LY_ENV:                                    /* (wave 11) mapping: the source's amount onto the knob (ui_map.c) */
            if (map_lyr)
                map_turn(k, s);
            break;
        case LY_GLO: {
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
/* (wave 10) the tiles under the black-key strip (as PERFORM's pads): 4 rows of 22 from y 67 */
#define LY_STILES_Y (BK_Y + BK_H + 1)
#define LY_STILE_H 22
static void tiles_draw(const tile_t *tl, int strip, uint32_t *cache)
{
    uint32_t r, c, sig = 7u + (uint32_t)strip;
    int32_t h = strip ? LY_STILE_H : LY_TILE_H, y0 = strip ? LY_STILES_Y : LY_TILES_Y;
    for (r = 0; r < 16u; r++)
        sig = studio_hash(sig * 31u + tl[r].bg * 3u + tl[r].fg * 5u + tl[r].top * 7u, tl[r].lab);
    if (!ui.force && sig == *cache)
        return;
    *cache = sig;
    for (r = 0; r < 4u; r++) {                          /* (wave 7: rows between the sub line and the dials, so every layer
                                                         * has the pages' dials and the strip) */
        cv_begin(240, h, C_BLACK);
        for (c = 0; c < 4u; c++) {
            const tile_t *t = &tl[r * 4u + c];
            int32_t x = 2 + (int32_t)c * 60;
            if (!t->lab[0] && t->bg == TE_G1)           /* a key that does nothing here: an outline (review 19) */
                op_box(x, 1, 56, h - 2, TE_G1);
            else
                cv_rect(x, 1, 56, h - 2, t->bg);
            if (t->top)
                cv_rect(x, 1, 56, 3, t->top);
            te_text_c(x + 28, strip ? 2 : 6, t->lab, t->fg);
        }
        cv_blit(0, y0 + (int32_t)r * h);
    }
}

/* (wave 7) a layer's top: the header every screen has (the button's name as the tag, in the layer's colour; wave 10: MASTER's
 * chip in the title's place when the button has a pair, else the layer's name; the tempo, the transport, the unsaved dot), then
 * a line: what it does now in grey, the -/+ legend at its right, LOCK at the far right while it stays open; a message in a
 * white bar */
static const char *const LAYER_TAG[LY_COUNT] = {"", "FX", "EDIT", "ARP", "SEQ", "SEL", "GLO", "SAVE", "ENV", "LFO"};
static const char *pm_lm, *pm_lp;                       /* the layer's -/+ words (layer_pm) */
/* the words of OCT- and OCT+ while layer l is held (0: nothing) */
static void layer_pm(uint32_t l, const char **m, const char **p)
{
    *m = *p = 0;
    switch (l) {
    case LY_FX:
        *m = "OFF", *p = "LATCH";
        break;
    case LY_SEQ:
        if (fm1_in.buttons & (1u << panel.btn[B_PLAY]))
            *m = "UNLATCH", *p = "LATCH";
        else
            *m = "HALVE", *p = "DOUBLE";
        break;
    case LY_ARP:
        if (!is_drum(TSEL))
            *m = *p = "SEMITONE";
        break;
    case LY_EDIT:
        *m = "CLEAR";
        break;
    case LY_SAVE:
        *m = "UNDO", *p = "REDO";
        break;
    default:
        break;
    }
}
/* the header with MASTER's chip (its pair on the selected voice), or the layer's name where the button has none */
static void layer_header(uint32_t l, uint16_t col, const char *name, uint32_t *cache)
{
    mpair_t m;
    char ti[12];
    te_upper(ti, name, sizeof ti);
    if (master_pair(l, &m)) {
        hdr_mchip = m.chip;
        hdr_mcol = m.col;
        hdr_mratio = (m.v - m.lo) * 1000 / (m.hi - m.lo);
    }
    op_header(LAYER_TAG[l % LY_COUNT], col, ti, cache);
    hdr_mchip = 0;
}
static void layer_title(const char *name, const char *sub, uint16_t col, uint32_t *cache)
{
    static uint32_t hc;
    uint32_t locked = ly_lock != LY_PLAY;
    uint32_t sig = studio_hash(studio_hash(col, name), sub) + (ui.msg_t ? studio_hash(3u, ui.msg) : 0u) + locked * 7919u;
    layer_header(ui.layer, col, name, &hc);
    layer_pm(ui.layer, &pm_lm, &pm_lp);
    sig = studio_hash(studio_hash(sig, pm_lm ? pm_lm : ""), pm_lp ? pm_lp : "");
    if (!ui.force && sig == *cache)
        return;
    *cache = sig;
    cv_begin(240, LY_TILES_Y - LY_SUB_Y, C_BLACK);
    if (ui.msg_t) {
        cv_rect(0, 0, 240, 17, C_WHITE);
        te_text_c(120, 0, ui.msg, C_BLACK);
    } else {
        int32_t x = 236;
        if (locked) {                                   /* locked open: any button lets it go */
            int32_t w = (int32_t)text_w(&FONT_S, "LOCK") + 6;
            cv_rect(x - w, 0, w, 16, C_WHITE);
            cv_text(x - w + 3, 0, &FONT_S, "LOCK", C_BLACK);
            x -= w + 6;
        }
        x = pm_draw(x, 0, pm_lm, pm_lp) - 8;
        if (4 + (int32_t)text_w(&FONT_S, sub) <= x)
            cv_text(4, 0, &FONT_S, sub, TE_G3);
    }
    cv_blit(0, LY_SUB_Y);
}
/* (wave 10) a hold whose button has only MASTER's pair (EDIT, ENV, LFO; ARP's semitones): the body says it large, its value
 * and a gauge, in what it acts on's colour */
static void pair_body_draw(uint32_t l, uint32_t *cache)
{
    mpair_t m;
    char vs[12];
    int has = master_pair(l, &m);
    uint32_t sig = has ? studio_hash((uint32_t)m.v * 7u + m.col, m.chip) : 1u;
    if (!ui.force && sig == *cache)
        return;
    *cache = sig;
    cv_begin(240, 114, C_BLACK);
    if (has) {
        int32_t r = (m.v - m.lo) * 200 / (m.hi - m.lo), w;
        master_val_str(l, &m, vs);
        te_text_c(120, 8, "TURN MASTER", TE_G3);
        w = (int32_t)text_w(&FONT_L, m.chip);
        cv_text(120 - w / 2, 30, &FONT_L, m.chip, m.col);
        w = (int32_t)text_w(&FONT_L, vs);
        cv_text(120 - w / 2, 64, &FONT_L, vs, m.col);
        op_box(20, 102, 200, 6, te_dim_of(m.col));
        cv_rect(20, 102, r, 6, m.col);
    } else {
        te_text_c(120, 40, "NOTHING ON MASTER", TE_G3);
        te_text_c(120, 60, "FOR THE DRUMS HERE", TE_G3);
    }
    cv_blit(0, LY_TILES_Y);
}

#include "ui_perf.c"                                    /* (wave 6) PERFORM, the FX layer */
static uint8_t layer_shown;                              /* the screen holds a layer (or a hold) */
static const char *layer_sub_shown = "";                 /* the sub line last drawn (the host tests read it) */
static const char *const SEQ_BK[11] = {"A", "B", "C", "D", "BAR", 0, 0, 0, 0, 0, 0};   /* hold SEQ's black keys */
static uint32_t bk_shown;                               /* (the host tests: the strip's keys with a job, a bit each) */
static void layer_screen_draw(void)
{
    static uint32_t head, tiles, foot, bar, strip;
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
    bk_shown = 0;
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
    sub[0] = 0;
    switch (layer) {
    case LY_SEQ: {                                      /* (wave 10) the rhythm: the keys ROLL (drums RETRIG), lit = held */
        uint32_t held = fm1_in.notes;
        str_cpy(sub, fill_now ? "fill on" : "play: fill", sizeof sub);
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
            tl[i].bg = down ? C_WHITE : TE_G1;
            tl[i].fg = down ? C_BLACK : present ? C_WHITE : TE_G3;
            tl[i].top = present && !down ? col : 0;
        }
        lab[0] = "rate", lab[1] = "shift", lab[2] = "swing", lab[3] = "steps";
        if (is_drum(t)) {
            str_cpy(v[0], SG_RATE[drt_live % 18u], 8);
            ratio[0] = ((int32_t)drt_live - 1) * 1000 / ((int32_t)NRTRG - 1);
        } else {
            str_cpy(v[0], N_ROLL[clamp(song.g[G_ROLL], 0, 4)], 8);
            ratio[0] = song.g[G_ROLL] * 250;
        }
        str_cpy(v[1], "<  >", 8);
        swing_str(v[2], t->p[P_SSWING]);
        ratio[2] = t->p[P_SSWING] * 10;
        fmt_int(v[3], t->p[P_SLEN]);
        ratio[3] = (t->p[P_SLEN] - 1) * 1000 / 63;
        break;
    }
    case LY_SAVE: {                                     /* (wave 10) store into A..D; the undo, the redo */
        uint32_t ready = arrangement_ready();
        col = C_WHITE;
        str_cpy(sub, "keys: store", sizeof sub);
        for (i = 0; i < 4u; i++) {
            int asked = ui.ask == ASK_STORE && ui.ask_arg == i;
            str_cpy(tl[i].lab, "save A", 8);           /* (six letters: the tile's width with room) */
            tl[i].lab[5] = (char)('A' + i);
            tl[i].bg = asked ? TE_RED : (ready >> i) & 1u ? TE_G2 : TE_G1;
            tl[i].fg = asked ? C_BLACK : (ready >> i) & 1u ? C_WHITE : TE_G4;
            tl[i].top = live_sec == (int8_t)i ? C_WHITE : 0;
        }
        for (i = 4; i < 16u; i++) {                     /* (keys 5..16 do nothing here: their rows left out) */
            tl[i].lab[0] = 0;
            tl[i].bg = tl[i].fg = C_BLACK;
        }
        break;
    }
    case LY_ARP:
        str_cpy(sub, is_drum(t) ? "" : "pattern", sizeof sub);
        break;
    case LY_EDIT:
        str_cpy(sub, "the voice", sizeof sub);
        break;
    case LY_ENV:
    case LY_LFO:
        str_cpy(sub, is_drum(t) ? "the sound" : "the voice", sizeof sub);
        break;
    case LY_GLO: {                                      /* the mixer: levels or pans; the strip SOLO (drums) and PAN */
        static const char *BK[11];
        BK[0] = is_drum(t) ? "SOLO" : 0;
        BK[1] = "PAN";
        for (i = 0; i < 4u; i++) {
            uint32_t lv = trk_level(i);
            static const char *const L[4] = {"1", "2", "3", "dr"};   /* (the strip's numbers) */
            lab[i] = L[i];
            fmt_int(v[i], (int32_t)lv * 100 / 127);
            ratio[i] = (int32_t)lv * 1000 / 127;
            if (mix_pan) {                              /* (G#3: the pans, L 64 .. C .. R 63) */
                int32_t pn = trk[i].p[P_PAN];
                v[i][0] = pn < 0 ? 'L' : pn > 0 ? 'R' : 'C';
                if (pn)
                    fmt_int(v[i] + 1, pn < 0 ? -pn : pn);
                else
                    v[i][1] = 0;
                ratio[i] = (pn + 64) * 1000 / 127;
            }
        }
        {
            mpair_t m;
            if (!ui.msg_t && master_pair(LY_GLO, &m))   /* (a message shows in the title instead) */
                hdr_mchip = m.chip, hdr_mcol = m.col, hdr_mratio = (m.v - m.lo) * 1000 / (m.hi - m.lo);
        }
        op_mix_draw(val, ratio, (uint32_t)(song.playing && clk_pos < BEAT_U / 4u) << 3 | (uint32_t)mix_pan << 4 |
                    (uint32_t)mix_dsolo << 5);
        hdr_mchip = 0;
        bk_strip(BK, (uint32_t)(mix_dsolo && is_drum(t)) | (uint32_t)mix_pan << 1, &strip);
        bk_shown = (is_drum(t) ? 1u : 0u) | 2u;
        layer_sub_shown = "";
        return;
    }
    default:
        break;
    }
    layer_sub_shown = sub;
    layer_title(layer_name(layer), sub, col, &head);
    if (layer == LY_SEQ) {
        int32_t p = patt_shown();
        uint32_t on = (p >= 0 ? 1u << p : 0u) | (uint32_t)(fill_arm || fill_bar_on) << 4;
        bk_strip(SEQ_BK, on, &strip);
        bk_shown = 0x1Fu;
        tiles_draw(tl, 1, &tiles);
    } else if (layer == LY_SAVE) {
        tiles_draw(tl, 0, &tiles);
    } else {
        pair_body_draw(layer, &tiles);
    }
    {   /* (a message shows in the title: the dials stay) */
        uint8_t m = ui.msg_t;
        ui.msg_t = 0;
        for (i = 0; i < 4u; i++) {                      /* (the pages' dials: five letters at most, upper case) */
            te_upper(v[i], v[i], sizeof v[i]);
            dval_fit(v[i]);
            te_upper(lu[i], lab[i], sizeof lu[i]);
            lab[i] = lu[i];
        }
        op_dials(lab, val, ratio, 0, 0, layer == LY_SAVE ? TE_G4 : TE_COL[sel & 3u], layer * 7919u + sel, &foot);
        ui.msg_t = m;
    }
    op_trackbar(&bar);
}

/* (wave 10, Ben 23 Nov 2026: "wouldn't save be the logical confirm button") THE CONFIRM: a ring fills while SAVE is held with a
 * question showing (HOLD SAVE: ...); full, it is done. Red when it takes something away (every question but one), white when
 * it only keeps (SAVE into an empty user preset slot) */
#define HOLD_CLEAR_MS 1300u                             /* the ring */
static uint8_t ask_keeps;                               /* the question showing only keeps: a white ring */
static void hold_screen_draw(void)
{
    static uint32_t cache;
    uint32_t span = HOLD_CLEAR_MS, el = fm1_ms - ui.hold_t0;
    const char *what = ui.msg + 11;                     /* (the question's action, after "HOLD SAVE: ") */
    uint16_t rc = ask_keeps ? C_WHITE : TE_RED;
    int32_t ratio = el >= span ? 1000 : (int32_t)(el * 1000u / span);
    uint32_t sig = studio_hash(ui.hold_kind * 7u + (uint32_t)ratio / 20u + rc, what);
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
    te_text_c(120, 4, "let go: no", TE_G4);
    te_text_c(120, 30, "keep holding save", TE_G3);
    cv_blit(0, 160);
}
