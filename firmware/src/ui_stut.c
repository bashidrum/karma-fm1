/* SPDX-License-Identifier: GPL-3.0-only */
/* The STUTTER lane on the GRID (PHEBUS, 21 Nov 2026, Ben: "would be cool if there was a stutter sequencer tbh where you
 * could set the length of the stutter pattern and then turn stutter on my step?"; picker: length "Per step", a step
 * repeats "The whole mix"). Included by ui_studio.c after ui_grid.c; the lane itself and its sound: punch.c stl_*.
 * On the GRID, ALGORITHM goes 1, 2, 3, DRUM, then STUTTER (left: back to DRUM). It is not a voice: white and grey.
 *   tap a step       an unlit one lights with the length for new steps; a lit one goes when its key comes up (unless
 *                    KNOB 1 turned meanwhile)
 *   hold a step      KNOB 1: that step's length (1/1 1/2 1/4 1/8 1/16 1/32; right: shorter), the steps held together
 *   KNOB 1           the length for new steps (1/4 to start); KNOB 2 the lane ON / OFF (its steps kept); KNOB 4 its
 *                    length, 1..64 steps, looping by itself under the tracks
 *   black keys 1..4  the page of steps; OCT- / OCT+ the page before / after
 * (wave 6, Ben: "Keep, and record played FX into it") the PERFORM pads played while the sequencer records land here as
 * spans of steps (punch.c stl.ev), drawn under the steps, each named; they play with the pattern while the lane is ON.
 *   hold a step + OCT-   the recorded pads over that step go
 *   KNOB 3               FX: how many are recorded; turned twice within 1.5 s, all of them go
 * A lit step under the playhead: the mix repeats the span just played at the step's length (steps in a row with the
 * same length: one loop); 1/2 and 1/1: the sequencer replays the half bar / bar before instead (punch.c rep_*); FX held
 * + a key comes first. Saved with the project and each section (project.c stl_put). */
/* the lengths by code (punch.c stl_code: 4 and 5 the sequencer's replay), and in the knobs' order, longest first: KNOB 1
 * turned right goes shorter (as before), left longer (Ben, 21 Nov 2026: "1/4 is preferably ... 1/2 would even be nice to
 * play with ... we may as well go the full 1/1"; picker "Replay the steps": 1/4 the default) */
static const char *const STL_LEN[6] = {"1/4", "1/8", "1/16", "1/32", "1/2", "1/1"};
static const uint8_t STL_BY_LEN[6] = {5, 4, 0, 1, 2, 3}, STL_RANK[6] = {2, 3, 4, 5, 1, 0};
static uint8_t stl_new = 0;                             /* the length a new step takes: 1/4 */
static uint32_t stl_turn(uint32_t code, int32_t s)      /* a length one longer (s < 0) or shorter */
{
    return STL_BY_LEN[clamp((int32_t)STL_RANK[code % 6u] + (s > 0 ? 1 : -1), 0, 5)];
}
static uint16_t stl_held, stl_pend;                     /* white keys held; of those, lit steps that go when let go */
static uint16_t stl_fresh;                              /* (wave 6) of those, steps this press lit (OCT- to erase pads: unlit again) */
static uint8_t stl_pg;                                  /* the page of steps shown */
static uint32_t stl_sess;                               /* the undo session (one hold, or one knob's turns) */
_Static_assert(sizeof stl <= sizeof undo.st, "the lane fits the undo's room");

static int stl_shown(void) { return stl_view && song.sel == TRK_DRUM; }
static uint32_t stl_pages(void) { return (stl.len + 15u) / 16u; }
static void stl_mark(void)                              /* undo (EDIT + OCT-): the lane before this edit (ui.c undo_swap) */
{
    if (undo.valid && !undo.undone && undo.trk == NTRK && undo.sess == stl_sess)
        return;
    fm1_irq_off();                                      /* (wave 6: a take marks it from the audio ISR too) */
    memcpy(undo.st, &stl, sizeof stl);
    undo.trk = NTRK;
    undo.sess = stl_sess;
    undo.valid = 1;
    undo.undone = 0;
    fm1_irq_on();
}
/* (wave 6) the recorded pads */
static uint32_t pev_count(void)
{
    uint32_t i, n = 0;
    for (i = 0; i < PEV_N; i++)
        n += stl.ev[i].fx != 0u;
    return n;
}
static int pev_covers(const pev_t *e, uint32_t s)       /* the span plays at the lane's step s */
{
    uint32_t L = stl.len ? stl.len : 16u, ln = e->len < L ? e->len : L;
    return e->fx && e->st < L && s < L && (s + L - e->st) % L < ln;
}
static uint32_t pev_erase_held(void)                    /* hold a step + OCT-: the spans over the steps held go */
{
    uint32_t i, w, n = 0;
    for (i = 0; i < PEV_N; i++)
        for (w = 0; w < 16u; w++)
            if ((stl_held >> w) & 1u && pev_covers(&stl.ev[i], stl_pg * 16u + w)) {
                if (!n)
                    stl_mark();
                stl.ev[i].fx = 0;                       /* (a byte: the ISR reads it whole) */
                n++;
                break;
            }
    return n;
}
static uint32_t pev_clear_ms;                           /* KNOB 3 turned once: again within 1.5 s clears them all */
static void stl_put_step(uint32_t s, int32_t code)      /* code < 0: off (bytes: the audio ISR reads them whole) */
{
    if (code < 0) {
        stl.on[s >> 3] &= (uint8_t)~(1u << (s & 7u));
    } else {                                            /* (its length first, then lit) */
        stl.lc[s >> 2] = (uint8_t)((stl.lc[s >> 2] & ~(3u << 2u * (s & 3u))) | ((uint32_t)code & 3u) << 2u * (s & 3u));
        stl.lx[s >> 3] = (uint8_t)((stl.lx[s >> 3] & ~(1u << (s & 7u))) | ((uint32_t)code >> 2 & 1u) << (s & 7u));
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
            stl_held = stl_pend = stl_fresh = 0;
            ui.force = 1;
        }
        return 1;
    }
    if (song.sel != TRK_DRUM || s <= 0)
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
        stl_fresh &= (uint16_t)~b;
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
    if (stl_lit(s)) {
        stl_pend |= (uint16_t)b;
    } else {
        stl_put_step(s, stl_new);
        stl_fresh |= (uint16_t)b;
    }
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
    } else if ((pressed & 1u << panel.btn[B_OCTDN]) && stl_held) {   /* (wave 6) a step held + OCT-: its recorded pads go */
        uint32_t n = pev_erase_held(), w;
        for (w = 0; w < 16u; w++)                       /* (a step lit by this very press: unlit, the press was for this) */
            if ((stl_fresh >> w) & 1u && stl_pg * 16u + w < stl.len)
                stl_put_step(stl_pg * 16u + w, -1);
        stl_fresh = 0;
        stl_pend &= (uint16_t)~stl_held;                /* (a lit step stays when let go) */
        ui_message(n ? "FX ERASED" : "NO FX ON THIS STEP");
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
                    stl_put_step(stl_pg * 16u + w, (int32_t)stl_turn(stl_code(stl_pg * 16u + w), s));
            stl_pend &= (uint16_t)~stl_held;            /* (edited: kept when let go) */
            stl_fresh = 0;
            continue;
        }
        if (k == 0u) {
            stl_new = (uint8_t)stl_turn(stl_new, s);
        } else if (k == 1u) {
            stl.act = (uint8_t)(s > 0);
        } else if (k == 2u) {                           /* (wave 6) FX: twice within 1.5 s clears the recorded pads */
            static uint32_t last;                       /* (a new turn: 0.2 s since the last click) */
            uint32_t gap = fm1_ms - last;
            last = fm1_ms;
            if (!pev_count()) {
                ui_message("NO FX RECORDED");
            } else if (pev_clear_ms && gap > 200u && fm1_ms - pev_clear_ms < 1500u) {
                uint32_t i;
                stl_sess = (undo_sess += 4u) | 3u;
                stl_mark();
                for (i = 0; i < PEV_N; i++)
                    stl.ev[i].fx = 0;
                pev_clear_ms = 0;
                ui_message("FX CLEARED");
            } else if (!pev_clear_ms || fm1_ms - pev_clear_ms >= 1500u) {
                pev_clear_ms = fm1_ms | 1u;
                ui_message("AGAIN: CLEAR FX");
            }
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
 * length (4 8 16 32), the step held white, the playhead a white bar; the page slots; then what the lit steps do (or the
 * step held) and the length a new step takes; the recorded pads as spans (wave 6); the knobs; the tracks */
static void stl_draw(void)
{
    static uint32_t head, sig_was, footer, bar;
    static char v[4][8];
    const char *lab[4] = {"len", "on", "fx", "steps"}, *val[4] = {v[0], v[1], v[2], v[3]};
    int32_t ratio[4] = {0, 0, 0, 0};
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
    for (j = 0; j < sizeof stl; j++)
        sig = sig * 31u + ((const uint8_t *)&stl)[j];
    if (ui.force || sig != sig_was) {
        sig_was = sig;
        cv_begin(240, 124, C_BLACK);                    /* (the tiles as ui_grid.c sg_tiles_draw, big) */
        for (j = 0; j < 16u; j++) {
            uint32_t s = stl_pg * 16u + j, h = (stl_held >> j) & 1u;
            int32_t x = 4 + (int32_t)(j % 8u) * 29, y = 2 + (int32_t)(j / 8u) * 24;   /* (wave 6: 22 px, room for the pads) */
            if (s >= stl.len)
                continue;
            if (stl_lit(s)) {                           /* (wave 7, "stutter lengths as spans": lit steps in a row with one
                                                         * length are one loop, drawn as one bar across them, its length
                                                         * named once at its start; a held step white) */
                uint32_t cd = stl_code(s), first = !(j % 8u) || !stl_lit(s - 1u) || stl_code(s - 1u) != cd, n = 1, q;
                int join = j % 8u != 7u && s + 1u < stl.len && stl_lit(s + 1u) && stl_code(s + 1u) == cd;
                uint16_t bg = h ? C_WHITE : stl.act ? TE_G4 : TE_G2;
                cv_rect(x, y, join ? 29 : 27, 21, bg);
                if (h && join)
                    cv_rect(x + 27, y, 2, 21, stl.act ? TE_G4 : TE_G2);
                if (first) {
                    const char *nm;
                    for (q = j + 1u; q % 8u && stl_pg * 16u + q < stl.len && stl_lit(stl_pg * 16u + q) &&
                         stl_code(stl_pg * 16u + q) == cd; q++)
                        n++;
                    nm = n > 1u ? STL_LEN[cd % 6u] : STL_LEN[cd % 6u] + 2;   /* (one tile: "16", "4"; a span: "1/16") */
                    cv_text(x + (n > 1u ? 3 : (27 - text_w(&FONT_S, nm)) / 2), y + 3, &FONT_S, nm,
                            stl.act || h ? C_BLACK : TE_G4);
                }
            } else {
                op_box(x, y, 27, 21, h ? C_WHITE : j % 4u ? TE_G2 : TE_G3);
            }
            if (song.playing && s == stl_pos)
                cv_rect(x, y + 21, 27, 2, C_WHITE);
        }
        for (j = 0; j < 4u; j++)                        /* the page slots: the one shown light, the others in use grey */
            cv_rect(4 + (int32_t)j * 58, 50, 54, 4, j >= stl_pages() ? TE_G1 : j == stl_pg ? TE_G4 : TE_G2);
        cv_rect(0, 57, 240, 1, TE_G1);
        if (hs < NSTEP) {                               /* a step held: its number and length; OCT- erases its pads */
            str_cpy(t, "STEP ", 16);
            fmt_int(t + 5, (int32_t)hs + 1);
            if (stl_lit(hs)) {
                str_cpy(t + str_len(t), "  ", 3);
                str_cpy(t + str_len(t), STL_LEN[c % 6u], 6);
            }
            cv_text(4, 59, &FONT_S, t, C_WHITE);
            for (j = 0; j < PEV_N && !pev_covers(&stl.ev[j], hs); j++)
                ;
            if (j < PEV_N)                              /* (pads recorded over it: OCT- erases them) */
                cv_text(236 - text_w(&FONT_S, "OCT-: ERASE FX"), 59, &FONT_S, "OCT-: ERASE FX", TE_G3);
        } else {                                        /* what the lit steps do; the length new steps take */
            cv_text(4, 59, &FONT_S, !stl.act ? "OFF: KEPT, SILENT" : c >= 4u ? "THE STEPS REPLAY" : "LIT STEPS REPEAT",
                    stl.act ? TE_G4 : TE_G3);
        }
        {   /* (wave 6) the recorded pads over the page's steps: a bar a span, named, two rows (more: "+n"); the ones
             * playing now light, the ones over a held step white (OCT- erases them) */
            uint32_t rows[3] = {0, 0, 0}, more = 0, i, any = 0;
            for (i = 0; i < PEV_N; i++) {
                const pev_t *e = &stl.ev[i];
                uint32_t m = 0, r, w, held = 0;
                if (!e->fx)
                    continue;
                any = 1;
                for (w = 0; w < 16u; w++)
                    if (pev_covers(e, stl_pg * 16u + w)) {
                        m |= 1u << w;
                        held |= (stl_held >> w) & 1u;
                    }
                if (!m)
                    continue;
                for (r = 0; r < 3u && (rows[r] & m); r++)
                    ;
                if (r == 3u) {
                    more++;
                    continue;
                }
                rows[r] |= m;
                for (w = 0; w < 16u; w++) {             /* each run of steps it covers */
                    uint32_t e0 = w, p = (e->fx & 31u) - 1u, now;
                    int32_t x, wd;
                    uint16_t bg, fg;
                    if (!((m >> w) & 1u))
                        continue;
                    while (w + 1u < 16u && (m >> (w + 1u)) & 1u)
                        w++;
                    x = (int32_t)e0 * 15, wd = (int32_t)(w - e0 + 1u) * 15 - 1;
                    now = song.playing && stl.act && pev_covers(e, stl_pos);
                    bg = held ? C_WHITE : now ? TE_G4 : TE_G2, fg = held || now ? C_BLACK : C_WHITE;
                    cv_rect(x, 77 + (int32_t)r * 16, wd, 14, bg);
                    if (p < PF_NPAD && (int32_t)text_w(&FONT_S, PFD[p].pad) + 3 <= wd)
                        cv_text(x + 2, 76 + (int32_t)r * 16, &FONT_S, PFD[p].pad, fg);
                }
            }
            if (hs >= NSTEP) {                          /* (the line's right: the length new steps take, or how many more
                                                         * pads there are than three rows show) */
                t[0] = '+';
                fmt_int(t + 1, (int32_t)more);
                str_cpy(t + str_len(t), " MORE", 6);
                if (!more) {
                    str_cpy(t, "NEW ", 16);
                    str_cpy(t + 4, STL_LEN[c % 6u], 6);
                }
                cv_text(236 - text_w(&FONT_S, t), 59, &FONT_S, t, TE_G4);
            }
            if (!any)
                cv_text(4, 92, &FONT_S, "FX: REC + PADS LAND HERE", TE_G3);
        }
        cv_blit(0, SG_BODY_Y);
        if (ui.force) {                                 /* (the rows under it, to the knobs) */
            cv_begin(240, SG_KNOB_Y - SG_BODY_Y - 124u, C_BLACK);
            cv_blit(0, SG_BODY_Y + 124u);
        }
    }
    str_cpy(v[0], STL_LEN[c % 6u], 8);
    ratio[0] = (int32_t)STL_RANK[c % 6u] * 200;
    str_cpy(v[1], stl.act ? "ON" : "OFF", 8);
    ratio[1] = stl.act ? 1000 : 0;
    fmt_int(v[3], (int32_t)stl.len);
    fmt_int(v[2], (int32_t)pev_count());                /* (wave 6) the recorded pads */
    ratio[2] = (int32_t)pev_count() * 1000 / (int32_t)PEV_N;
    ratio[3] = ((int32_t)stl.len - 1) * 1000 / 63;
    if (hs < NSTEP)
        lab[1] = lab[2] = lab[3] = "";
    {   /* (wave 7) the pages' dials, white-grey (the lane is everyone's) */
        static char lu[4][8];
        for (j = 0; j < 4u; j++) {
            te_upper(lu[j], lab[j], sizeof lu[j]);
            lab[j] = lu[j];
        }
        op_dials(lab, val, ratio, 0, 0, TE_G4, 101u + (hs < NSTEP), &footer);
    }
    op_trackbar(&bar);
}
