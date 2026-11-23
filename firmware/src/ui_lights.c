/* SPDX-License-Identifier: GPL-3.0-only */
/* THE LIGHTS (PHEBUS wave 11). Included by ui_input.c.
 *
 * Ben, 23 Nov 2026, 10:45: "all buttons that are related combinations illuminate to make it obvious that there are "hidden"
 * controls under those buttons". While a button (or a step, a key, a pad) is held, every button and key that does something
 * together with it is lit and every other one is dark, backlight too; the held button keeps its light. While a question
 * waits for the confirm, SAVE alone. The controls that work under a hold are decided once, here (combo_live), and the input
 * asks the same function before it acts (ui_input.c layers_input, ui_layers.c layer_key, seq.c's keys through it): the
 * lights cannot say something the buttons do not do. tests/ui_pages_test.c presses every control under every hold and checks.
 *
 * Ben, 10:46: "settings for how much of a disco you can put on" (panel.c disco): QUIET, NORMAL, SHOW, a ceiling over LIGHTS,
 * KEYS and NOTES. Every light is a level (off, backlight, glow, lit) with what it is: STATE (needed to work: the page's
 * button, the held button, a latch, the confirm, the steps and the step held, the octave, REC), HINT (a control live under a
 * hold), MOTION (follows the music or time: the beat on PLAY, the play head, hits, notes, blinking, the count-in, the
 * landmarks' guide), BACK (the menu's LIGHTS and KEYS backlight).
 *   QUIET   STATE at the glow (a twenty-fourth of lit), HINT at the backlight's pulse (at least QUIET_HINT_NS), no MOTION (a
 *           blink stands still, on), BACK at most LOW. Nothing changes while a pattern plays.
 *   NORMAL  as built before wave 11 (and the hints of a hold).
 *   SHOW    NORMAL, every button's backlight at HIGH, and with nothing held on a screen whose keys play notes the keys dance:
 *           the notes of every voice and the drum hits lit, the step running along the white keys and the level of the mix
 *           sweeping up from the lowest key, at the glow. A hold always wins: its hints, nothing else. */
enum { LL_OFF, LL_BG, LL_GLOW, LL_LIT };
enum { LK_STATE, LK_HINT, LK_MOTION, LK_BACK };
#define QUIET_HINT_NS 1500u
static int combo_live(uint32_t h, uint32_t c);                             /* (QUIET's hints: the backlight's pulse, LIGHTS off or not) */
static uint8_t led_lv[41];                              /* the level each control got last frame (the tests read it) */
static uint8_t led_ctx;                                 /* the hold the lights showed (HC_*) */

/* ---- the holds, and what works under each */
enum { HC_NONE, HC_ASK, HC_STEP, HC_KEY, HC_OCTDN, HC_OCTUP, HC_LY0 };   /* HC_LY0 + LY_*: a layer held (or kept open) */
_Static_assert(HC_LY0 == HC_LY(0) && HC_STEP == HC_STEPS, "HC_LY, HC_STEPS (ui_layers.c) are these");
static uint32_t black_keys(void);
/* (Ben: black keys "Only live when drawn") the black keys with no job here: the GRID, the drum voice's sounds (not CHROMATIC),
 * a chord mode where the modifiers are not drawn */
static int key_strip_on(void);
static uint32_t keys_dead(void)
{
    return kb_grid || (is_drum(TSEL) && drum_keys != DK_CHROM) || (TSEL->p[P_CHORD] && !key_strip_on() && !is_drum(TSEL))
               ? black_keys() : 0u;
}
static uint32_t hold_ctx(void)
{
    uint32_t ob = 1u << panel.btn[B_OCTDN], pb = 1u << panel.btn[B_OCTUP], b = fm1_in.buttons;
    if (ui.menu)
        return HC_NONE;
    if (ui.ask || ui.hold_kind == 3u)
        return HC_ASK;
    if (ly_held_now != LY_PLAY)
        return HC_LY0 + ly_held_now;
    if (kb_grid && sg_held)
        return HC_STEP;
    if ((b & ob) && !(b & pb))
        return HC_OCTDN;
    if ((b & pb) && !(b & ob))
        return HC_OCTUP;
    if ((fm1_in.notes & ~keys_dead()) && !kb_grid && !sr_on && !rec_wait)
        return HC_KEY;
    return HC_NONE;
}
/* the keys play sounds here (no steps, no STEP REC): the drums' ghost / hard, a key's erase */
static int keys_are_notes(void) { return !kb_grid && !sr_on && !ui.menu; }
/* control c (0..13 a button B_*, 14..40 key c - 14) does something together with hold h, now */
static int combo_live(uint32_t h, uint32_t c)
{
    track_t *t = TSEL;
    uint32_t k = c - 14u, drum = (uint32_t)is_drum(t), pd = (fm1_in.buttons >> panel.btn[B_PLAY]) & 1u;
    int32_t w = c >= 14u ? punch_key(k) : -1, bi = c >= 14u ? black_idx(k) : -1;
    if (h == HC_NONE || c > 40u)
        return 0;
    switch (h) {
    case HC_ASK:                                        /* the confirm: SAVE, nothing else */
        return c == B_SAVE;
    case HC_STEP:                                       /* a step: the page buttons its locks are made from, SEL, OCT- erases;
                                                         * the other steps (held together, one lock for all) */
        if (c < 14u)                                    /* (a page button or SEL: only if there is another page to show) */
            return ((c == B_EDIT || c == B_ENV || c == B_LFO || c == B_FX || c == B_SEQ) && nav_moves(c)) ||
                   (c == B_SCL && nav_page_moves()) || c == B_OCTDN;
        return w >= 0 && sg_page() * 16u + (uint32_t)w < trk_len(t) && !((sg_held >> w) & 1u);
    case HC_KEY:                                        /* a note or a drum sound held: OCT- erases it from the pattern */
        return c == B_OCTDN;
    case HC_OCTDN:
    case HC_OCTUP:                                      /* the other: octave 0 (wave 12: no ghost / hard hits on the keys) */
        return c == (h == HC_OCTDN ? B_OCTUP : B_OCTDN);
    case HC_LY0 + LY_FX:                                /* PERFORM: the pads; OCT+ latches the held ones, OCT- lets latches go */
        if (c == B_OCTUP)
            return perf.held != 0u;
        if (c == B_OCTDN)
            return perf.lat != 0u;
        return c >= 14u && punch_fx_of_key(k) >= 0 && punch_fx_of_key(k) < (int32_t)PF_NPAD;
    case HC_LY0 + LY_SEQ:                               /* the rhythm: PLAY the fill, -/+ halve / double (PLAY held: the fill's
                                                         * latch), the white keys roll (drums: retrig), A B C D BAR */
        if (c == B_PLAY)
            return 1;
        if (c == B_OCTDN)
            return pd ? fill_lat != 0u : !stl_shown();
        if (c == B_OCTUP)
            return pd ? !fill_lat : !stl_shown();
        if (c < 14u)
            return 0;
        if (w >= 0)
            return drum || kb_map(t, k) != KB_SILENT;
        return bi >= 0 && bi < 5;
    case HC_LY0 + LY_ARP:                               /* the pattern a semitone (synth voices) */
        return !drum && (c == B_OCTDN || c == B_OCTUP);
    case HC_LY0 + LY_EDIT:                              /* clear the voice (asks) */
        return c == B_OCTDN;
    case HC_LY0 + LY_SAVE:                              /* undo, redo; keys 1 to 4 store A to D */
        return c == B_OCTDN || c == B_OCTUP || (w >= 0 && w < 4);
    case HC_LY0 + LY_GLO:                               /* the mixer: mutes and solos (drums: the 16 sounds), PAN, SOLO */
        if (c < 14u)
            return 0;
        if (w >= 0)
            return drum ? 1 : w < 8;
        return bi == 1 || (bi == 0 && drum);
    case HC_LY0 + LY_ENV:
    case HC_LY0 + LY_LFO:                               /* mapping (ui_map.c): SEL the other source, OCT- remove */
        return map_lyr && ((c == B_SCL && !is_drum(TSEL)) || c == B_OCTDN);   /* (wave 12: the drums' one LFO: no SEL) */
    default:
        return 0;
    }
}
/* the button held of hold h (its own light), NB none */
static uint32_t hold_btn(uint32_t h)
{
    if (h >= HC_LY0)
        return LAYER_BTN[(h - HC_LY0) % LY_COUNT];
    return h == HC_OCTDN ? B_OCTDN : h == HC_OCTUP ? B_OCTUP : NB;
}

/* ---- the lights, a level and a kind each, then the DISCO setting over them */
static uint8_t ll_lv[41], ll_kind[41];
static void ll_set(uint32_t c, uint32_t lv, uint32_t kind)
{
    if (c <= 40u && lv >= ll_lv[c]) {
        if (lv > ll_lv[c] || kind < ll_kind[c])
            ll_kind[c] = (uint8_t)kind;               /* (the same level from two: the needed one counts) */
        ll_lv[c] = (uint8_t)lv;
    }
}
static void ll_keys(uint32_t m, uint32_t lv, uint32_t kind)
{
    uint32_t k;
    for (k = 0; k < 27u; k++)
        if ((m >> k) & 1u)
            ll_set(14u + k, lv, kind);
}
/* SHOW: the keys dance (nothing held, the keys play notes) */
static void ll_show_keys(uint32_t dead)
{
    uint32_t i, m = 0, k, lvl = 0;
    for (i = 0; i < NPART; i++)                         /* every voice's notes */
        m |= keys_sounding(&trk[i]);
    for (i = 0; i < DRUM_LANES; i++)                    /* the drum hits */
        if (pad_lit[i])
            m |= 1u << key_of_white(i);
    ll_keys(m & ~dead, LL_LIT, LK_MOTION);
    if (song.playing) {                                 /* the step, along the white keys */
        uint32_t st = (clk_beat * 4u + clk_pos / (BEAT_U / 4u)) & 15u;   /* (no 64-bit divide) */
        ll_set(14u + key_of_white(st), LL_GLOW, LK_MOTION);
    }
    for (i = 0; i < NTRK; i++)                          /* the mix's level, up from the lowest key */
        lvl = op_mx[i] > lvl ? op_mx[i] : lvl;
    for (k = 0; k < 27u && k * 6u < lvl; k++)
        if (!((dead >> k) & 1u))
            ll_set(14u + k, LL_GLOW, LK_MOTION);
}
/* the black keys as key bits (F#3 G#3 A#3 C#4 ...: PUNCH_BLACK) */
static uint32_t black_keys(void)
{
    uint32_t i, m = 0;
    for (i = 0; i < 11u; i++)
        m |= 1u << PUNCH_BLACK[i];
    return m;
}
static void ui_leds(void)
{
    uint8_t nl[FM1_NCOL] = {0}, dl[FM1_NCOL] = {0}, bl[FM1_NCOL] = {0};
    uint32_t k, c, h = hold_ctx(), blink = (fm1_ms / 125u) & 1u, bgns = LIGHTS_NS[lights_lvl % LIGHTS_N], quiet = disco == DISCO_QUIET;
    static uint8_t ready;
    if (!ready) {
        led_pos_init();
        ready = 1;
    }
    memset(ll_lv, 0, sizeof ll_lv);
    memset(ll_kind, LK_BACK, sizeof ll_kind);
    led_ctx = (uint8_t)h;
    if (h != HC_NONE) {                                 /* ---- a hold: what works with it lit, the rest dark */
        uint32_t hb = hold_btn(h);
        for (c = 0; c <= 40u; c++)
            if (combo_live(h, c))
                ll_set(c, LL_LIT, LK_HINT);
        if (hb < NB) {                                  /* the held button; kept open (let go): it blinks */
            if (ly_lock != LY_PLAY && h == HC_LY0 + ly_lock && !(fm1_in.buttons & (1u << panel.btn[hb])) && !quiet)
                ll_set(hb, (fm1_ms / 300u) & 1u ? LL_LIT : LL_OFF, LK_MOTION);
            else
                ll_set(hb, LL_LIT, LK_STATE);
        }
        if (h == HC_ASK)                                /* the confirm: SAVE pulses (the one thing to press) */
            ll_set(B_SAVE, quiet || ((fm1_ms / 250u) & 1u) ? LL_LIT : LL_GLOW, quiet ? LK_STATE : LK_MOTION);
        if (h == HC_LY0 + LY_FX) {                      /* PERFORM: a pad held lit, a latched one blinks (QUIET: lit) */
            uint32_t p;
            for (p = 0; p < PF_NPAD; p++)
                if (((perf.held | perf.lat) >> p) & 1u)
                    ll_set(14u + punch_key_of_fx(p), (perf.held >> p) & 1u || quiet || blink ? LL_LIT : LL_GLOW, LK_STATE);
        }
        if (h == HC_STEP)                               /* the steps held (and the others: hints) */
            for (k = 0; k < 16u; k++)
                if ((sg_held >> k) & 1u)
                    ll_set(14u + key_of_lane(k), LL_LIT, LK_STATE);
        if (h == HC_KEY)                                /* the keys held */
            ll_keys(fm1_in.notes & ~keys_dead(), LL_LIT, LK_STATE);
    } else {                                            /* ---- nothing held: as before wave 11, then the DISCO */
        uint32_t keys, guide, back, lnotes = lights_notes && !quiet;
        uint32_t lk = ly_lock == LY_PLAY, bk = black_keys();
        /* (Ben: black keys "Only live when drawn") a black key with no job here (the GRID, the drum voice's sounds) stays dark,
         * pressed or not; the KEY page's chord modifiers (drawn) glow */
        uint32_t dead = keys_dead();
        ll_set(ui.layer != LY_PLAY ? LAYER_BTN[ui.layer] : cur_btn(), lk || ((fm1_ms / 300u) & 1u) ? LL_LIT : LL_OFF,
               lk ? LK_STATE : LK_MOTION);              /* (kept open: blinks) */
        if (play_led() || (ci_on && ci_u % BEAT_U < BEAT_U / 4u))
            ll_set(B_PLAY, LL_LIT, LK_MOTION);          /* the beat, the count-in's beats */
        if (song.playing)
            ll_set(B_PLAY, LL_GLOW, LK_STATE);
        if (song.rec)
            ll_set(B_REC, LL_LIT, LK_STATE);
        if (rec_wait)
            ll_set(B_REC, blink ? LL_LIT : LL_OFF, LK_MOTION), ll_set(B_REC, LL_GLOW, LK_STATE);
        {                                               /* the octave (wave 12: the drums' ghost / hard lights gone) */
            if (song.octave < 0)
                ll_set(B_OCTDN, LL_LIT, LK_STATE);
            if (song.octave > 0)
                ll_set(B_OCTUP, LL_LIT, LK_STATE);
        }
        if (kb_grid && stl_shown()) {                   /* the STUTTER lane's steps (what blinks: MOTION) */
            uint32_t a = stl_keys_lit(0), b = stl_keys_lit(1);
            ll_keys(a & b, LL_LIT, LK_STATE);
            ll_keys((a ^ b) & (blink ? b : a), LL_LIT, LK_MOTION);
            ll_keys(fm1_in.notes, LL_LIT, LK_STATE);
        } else if (kb_grid) {                           /* the steps; the play head blinks over them (MOTION) */
            const track_t *t = TSEL;
            uint32_t i, len = trk_len(t), b0 = sg_page() * 16u, ph = song.playing ? t->seq_idx : 0xFFFFu;
            for (i = 0; i < 16u; i++) {
                uint32_t idx = b0 + i, on, key = 14u + key_of_lane(i);
                if (idx >= len)
                    continue;
                on = (uint32_t)sg_on(t, idx);
                if (idx == ph && !quiet)
                    ll_set(key, !on || blink ? LL_LIT : LL_OFF, LK_MOTION);
                else if (on)
                    ll_set(key, LL_LIT, LK_STATE);
                if ((sg_held >> i) & 1u)
                    ll_set(key, LL_LIT, LK_STATE);
            }
            ll_keys(fm1_in.notes & ~dead, LL_LIT, LK_STATE);
        } else {
            keys = fm1_in.notes & ~dead;
            ll_keys(keys, LL_LIT, LK_STATE);
            if (key_strip_on())                         /* the chord modifiers, drawn: live */
                ll_keys(bk, LL_GLOW, LK_HINT);
            if (is_drum(TSEL) || lnotes)                /* the drum hits; NOTES: the synths' notes too */
                ll_keys(keys_sounding(TSEL) & ~keys & ~dead, LL_LIT, LK_MOTION);
            if (disco == DISCO_SHOW && !ui.menu && !rec_wait)
                ll_show_keys(dead);
        }
        guide = keys_guide() & 0x7FFFFFFu;              /* the landmarks: dim */
        ll_keys(guide, LL_GLOW, LK_BACK);
        back = lights_keys_mask() & ~dead;              /* the menu's KEYS (not a dead black key) */
        ll_keys(back, LL_BG, LK_BACK);
        if (lights_lvl || disco == DISCO_SHOW)          /* the menu's LIGHTS (SHOW: every button's backlight) */
            for (k = 0; k < NB; k++)
                ll_set(k, LL_BG, LK_BACK);
    }
    /* ---- the DISCO setting over it all */
    if (disco == DISCO_SHOW && h == HC_NONE)
        bgns = LIGHTS_NS[LIGHTS_HIGH];
    if (quiet) {
        uint32_t lim = LIGHTS_NS[LIGHTS_LOW];
        bgns = bgns > lim ? lim : bgns;
        for (c = 0; c <= 40u; c++) {
            uint32_t lv = ll_lv[c], kd = ll_kind[c];
            if (kd == LK_MOTION)
                lv = LL_OFF;                            /* (nothing that moves) */
            else if (kd == LK_STATE && lv == LL_LIT)
                lv = LL_GLOW;                           /* (dim) */
            else if (kd == LK_HINT || (kd == LK_BACK && lv > LL_BG))
                lv = LL_BG;
            ll_lv[c] = (uint8_t)lv;
        }
        if (h != HC_NONE && bgns < QUIET_HINT_NS)
            bgns = QUIET_HINT_NS;
    }
    for (c = 0; c <= 40u; c++) {
        uint32_t id = c < 14u ? panel.btn[c] : c;
        led_lv[c] = ll_lv[c];
        if (ll_lv[c] != LL_OFF)
            led_put(ll_lv[c] == LL_LIT ? nl : ll_lv[c] == LL_GLOW ? dl : bl, id, 1);
    }
    for (c = 0; c < FM1_NCOL; c++) {
        fm1_led[c] = nl[c];
        fm1_led_dim[c] = (uint8_t)(dl[c] & ~nl[c]);
        fm1_led_bg[c] = (uint8_t)(bl[c] & ~nl[c] & ~dl[c]);
    }
    fm1_led_bg_ns = (uint16_t)bgns;
}
