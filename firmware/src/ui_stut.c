/* SPDX-License-Identifier: GPL-3.0-only */
/* The STUTTER lane on the GRID (PHEBUS, 21 Nov 2026, Ben: "would be cool if there was a stutter sequencer tbh where you
 * could set the length of the stutter pattern and then turn stutter on my step?"; picker: length "Per step", a step
 * repeats "The whole mix"). Included by ui_studio.c after ui_grid.c; the lane itself and its sound: punch.c stl_*.
 * On the GRID, ALGORITHM goes 1, 2, 3, DRUM, then STUTTER (left: back to DRUM). It is not a voice: white and grey.
 *   tap a step       an unlit one lights with the length for new steps; a lit one goes when its key comes up (unless
 *                    KNOB 1 turned meanwhile)
 *   hold a step      KNOB 1: that step's length (1/4 1/8 1/16 1/32), the steps held together
 *   KNOB 1           the length for new steps (1/8 to start); KNOB 2 the lane ON / OFF (its steps kept); KNOB 4 its
 *                    length, 1..64 steps, looping by itself under the tracks
 *   black keys 1..4  the page of steps; OCT- / OCT+ the page before / after
 * A lit step under the playhead: the mix repeats the span just played at the step's length (steps in a row with the
 * same length: one loop); FX held + a key comes first. Saved with the project and each section (project.c stl_put). */
static const char *const STL_LEN[4] = {"1/4", "1/8", "1/16", "1/32"};
static uint8_t stl_new = 1;                             /* the length a new step takes: 1/8 */
static uint16_t stl_held, stl_pend;                     /* white keys held; of those, lit steps that go when let go */
static uint8_t stl_pg;                                  /* the page of steps shown */
static uint32_t stl_sess;                               /* the undo session (one hold, or one knob's turns) */
_Static_assert(sizeof stl <= sizeof undo.st, "the lane fits the undo's room");

static int stl_shown(void) { return stl_view && song.sel == TRK_DRUM && !drum_page; }
static uint32_t stl_pages(void) { return (stl.len + 15u) / 16u; }
static void stl_mark(void)                              /* undo (EDIT + OCT-): the lane before this edit (ui.c undo_swap) */
{
    if (undo.valid && !undo.undone && undo.trk == NTRK && undo.sess == stl_sess)
        return;
    memcpy(undo.st, &stl, sizeof stl);
    undo.trk = NTRK;
    undo.sess = stl_sess;
    undo.valid = 1;
    undo.undone = 0;
}
static void stl_put_step(uint32_t s, int32_t code)      /* code < 0: off (bytes: the audio ISR reads them whole) */
{
    if (code < 0) {
        stl.on[s >> 3] &= (uint8_t)~(1u << (s & 7u));
    } else {                                            /* (its length first, then lit) */
        stl.lc[s >> 2] = (uint8_t)((stl.lc[s >> 2] & ~(3u << 2u * (s & 3u))) | (uint32_t)code << 2u * (s & 3u));
        stl.on[s >> 3] |= (uint8_t)(1u << (s & 7u));
    }
    ui.force = 1;
}
/* ALGORITHM on the GRID (ui_studio.c): right from DRUM onto the lane, left off it. 1: taken */
static int stl_algo(int32_t s)
{
    if (stl_view) {
        if (s < 0) {
            stl_view = 0;
            stl_held = stl_pend = 0;
            ui.force = 1;
        }
        return 1;
    }
    if (song.sel != TRK_DRUM || s <= 0 || drum_page)
        return 0;
    stl_view = 1;
    stl_pg = 0;
    ui.force = 1;
    return 1;
}
/* a key on the lane (ui_grid.c grid_key) */
static void stl_key(uint32_t k, uint32_t down)
{
    static const int8_t PG[12] = {-1, 0, -1, 1, -1, 2, -1, -1, 3, -1, -1, -1};
    int32_t w = punch_key(k);
    uint32_t s, b;
    if (w < 0) {                                        /* the first four black keys: the page */
        if (down && !stl_held && k < 12u && PG[k] >= 0 && (uint32_t)PG[k] < stl_pages()) {
            stl_pg = (uint8_t)PG[k];
            ui.force = 1;
        }
        return;
    }
    b = 1u << w;
    s = stl_pg * 16u + (uint32_t)w;
    if (!down) {
        if (!(stl_held & b))
            return;
        stl_held &= (uint16_t)~b;
        if ((stl_pend & b) && s < stl.len)
            stl_put_step(s, -1);                        /* a tap on a lit step: it goes */
        stl_pend &= (uint16_t)~b;
        ui.force = 1;
        return;
    }
    if (s >= stl.len)
        return;
    if (!stl_held)
        stl_sess = (undo_sess += 4u) | 3u;
    stl_mark();
    stl_held |= (uint16_t)b;
    if (stl_lit(s))
        stl_pend |= (uint16_t)b;
    else
        stl_put_step(s, stl_new);
    ui.force = 1;
}
/* the lane's knobs and buttons (ui_studio.c drum_screen_input, while it is shown) */
static void stl_input(uint32_t pressed)
{
    uint32_t k, w;
    int32_t s;
    if ((pressed & 1u << panel.btn[B_OCTDN]) | (pressed & 1u << panel.btn[B_OCTUP]) && !stl_held) {
        stl_pg = (uint8_t)((stl_pg + ((pressed >> panel.btn[B_OCTUP]) & 1u ? 1u : stl_pages() - 1u)) % stl_pages());
        ui.force = 1;
    }
    panel_enc(EN_PRESET);                               /* (nothing here: no turn waits) */
    panel_enc(EN_SELECT);
    for (k = 0; k < 4u; k++) {
        uint32_t again;
        if ((s = panel_enc(EN_K1 + k)) == 0)
            continue;
        again = ui.hot_t && ui.hot_col == k;             /* (turning on: the same undo) */
        ui.hot_col = (uint8_t)k;
        ui.hot_t = 40;
        if (stl_held) {                                 /* a step held: KNOB 1 its length */
            if (k)
                continue;
            stl_mark();
            for (w = 0; w < 16u; w++)
                if ((stl_held >> w) & 1u && stl_pg * 16u + w < stl.len && stl_lit(stl_pg * 16u + w))
                    stl_put_step(stl_pg * 16u + w, clamp((int32_t)stl_code(stl_pg * 16u + w) + (s > 0 ? 1 : -1), 0, 3));
            stl_pend &= (uint16_t)~stl_held;            /* (edited: kept when let go) */
            continue;
        }
        if (k == 0u) {
            stl_new = (uint8_t)clamp(stl_new + (s > 0 ? 1 : -1), 0, 3);
        } else if (k == 1u) {
            stl.act = (uint8_t)(s > 0);
        } else if (k == 3u) {
            if (!again)
                stl_sess = (undo_sess += 4u) | 3u;
            stl_mark();
            stl.len = (uint8_t)clamp((int32_t)stl.len + accel(EN_K4, s, 63), 1, NSTEP);
            if (stl_pg >= stl_pages())
                stl_pg = (uint8_t)(stl_pages() - 1u);
        }
        ui.force = 1;
    }
}
/* the keys' lights: the lit steps of the page shown, the playhead blinking, the keys held */
static uint32_t stl_keys_lit(uint32_t blink)
{
    uint32_t i, m = 0;
    for (i = 0; i < 16u; i++) {
        uint32_t s = stl_pg * 16u + i, on;
        if (s >= stl.len)
            continue;
        on = (uint32_t)stl_lit(s);
        if (song.playing && s == stl_pos)
            on = !on || blink;
        if (on || ((stl_held >> i) & 1u))
            m |= 1u << key_of_lane(i);
    }
    return m;
}
/* the lane: the 16 steps of the page as the synth GRID's tiles (two rows of eight), a lit step filled grey with its
 * length (4 8 16 32), the step held white, the playhead a white bar; the page slots; then ON / OFF (or the step held)
 * and the length a new step (or the step held) takes; the knobs; the tracks */
static void stl_draw(void)
{
    static uint32_t head, sig_was, footer, bar;
    static char v[4][8];
    const char *lab[4] = {"len", "on", "", "steps"}, *val[4] = {v[0], v[1], v[2], v[3]};
    int32_t ratio[4] = {0, 0, -1, 0};
    uint32_t j, sig, hs = 0xFFu, c;
    char t[16];
    for (j = 16u; j--;)                                 /* (the first step held) */
        if ((stl_held >> j) & 1u)
            hs = stl_pg * 16u + j;
    c = hs < NSTEP ? stl_code(hs) : stl_new;
    fmt_int(t, (int32_t)stl_pg + 1);
    str_cpy(t + str_len(t), "/", 2);
    fmt_int(t + str_len(t), (int32_t)stl_pages());
    op_header("STUT", TE_G4, t, &head);       /* (the tag box holds five letters: STUT, not SYN) */
    sig = studio_hash(stl.len * 7u + stl.act * 131u + stl_pg * 977u + stl_held * 3u + stl_new * 7919u +
                      (song.playing ? stl_pos + 1u : 0u) * 71u, "");
    for (j = 0; j < 24u; j++)
        sig = sig * 31u + ((const uint8_t *)&stl)[j];
    if (ui.force || sig != sig_was) {
        sig_was = sig;
        cv_begin(240, 124, C_BLACK);                    /* (the tiles as ui_grid.c sg_tiles_draw, big) */
        for (j = 0; j < 16u; j++) {
            uint32_t s = stl_pg * 16u + j, h = (stl_held >> j) & 1u;
            int32_t x = 4 + (int32_t)(j % 8u) * 29, y = 4 + (int32_t)(j / 8u) * 30;
            if (s >= stl.len)
                continue;
            if (stl_lit(s)) {
                const char *n = STL_LEN[stl_code(s)] + 2;   /* ("1/16" -> "16") */
                cv_rect(x, y, 27, 26, h ? C_WHITE : stl.act ? TE_G4 : TE_G2);
                cv_text(x + (27 - text_w(&FONT_S, n)) / 2, y + 5, &FONT_S, n, stl.act || h ? C_BLACK : TE_G4);
            } else {
                op_box(x, y, 27, 26, h ? C_WHITE : j % 4u ? TE_G2 : TE_G3);
            }
            if (song.playing && s == stl_pos)
                cv_rect(x, y + 27, 27, 2, C_WHITE);
        }
        for (j = 0; j < 4u; j++)                        /* the page slots: the one shown light, the others in use grey */
            cv_rect(4 + (int32_t)j * 58, 64, 54, 5, j >= stl_pages() ? TE_G1 : j == stl_pg ? TE_G4 : TE_G2);
        cv_rect(0, 72, 240, 1, TE_G1);
        if (hs < NSTEP) {                               /* a step held: its number, its length big */
            str_cpy(t, "STEP ", 16);
            fmt_int(t + 5, (int32_t)hs + 1);
            cv_text(6, 80, &FONT_S, t, C_WHITE);
        } else {                                        /* ON / OFF big, the length new steps take */
            cv_text(6, 96, &FONT_L, stl.act ? "ON" : "OFF", stl.act ? C_WHITE : TE_G3);
            cv_text(6, 80, &FONT_S, stl.act ? "LIT STEPS REPEAT" : "STEPS KEPT, SILENT", TE_G3);
            cv_text(234 - text_w(&FONT_S, "NEW STEPS"), 80, &FONT_S, "NEW STEPS", TE_G3);
        }
        cv_text(234 - text_w(&FONT_L, STL_LEN[c]), 96, &FONT_L, STL_LEN[c], C_WHITE);
        cv_blit(0, SG_BODY_Y);
        if (ui.force) {                                 /* (the rows under it, to the knobs) */
            cv_begin(240, SG_KNOB_Y - SG_BODY_Y - 124u, C_BLACK);
            cv_blit(0, SG_BODY_Y + 124u);
        }
    }
    str_cpy(v[0], STL_LEN[c], 8);
    ratio[0] = (int32_t)c * 333;
    str_cpy(v[1], stl.act ? "ON" : "OFF", 8);
    ratio[1] = stl.act ? 1000 : 0;
    fmt_int(v[3], (int32_t)stl.len);
    ratio[3] = ((int32_t)stl.len - 1) * 1000 / 63;
    if (hs < NSTEP)
        lab[1] = lab[3] = "";
    knob_tint_all(TINT_NEUTRAL);
    op_knobs(lab, val, 0, ratio, 0, 101u + (hs < NSTEP), &footer);
    op_trackbar(&bar);
}
