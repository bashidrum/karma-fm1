/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Felucca UI drawing: status bar (top), columns + gauges, graphs, focus readout,
 * footer (steps + engine / preset / page). */
static void draw_about(void);
static int dev_dim(uint32_t id);                       /* (review U5: ui_menu.c) */
static uint32_t str_hash(uint32_t h, const char *s);
static uint32_t projpg_sig(void);                       /* (the bank: SAVE > PROJECT, below) */
/* (the knob band shows no icon: the look-ups by label, icons.c, are not built in) */
#define param_icon(d, v) ICON_NONE
#define engine_icon(n) ICON_NONE

/* --------------------------------------------------------- drawing --- */
static int is_eng_name(const char *s)                 /* one of the ENGINES[]->name strings */
{
    uint32_t e;
    for (e = 0; e < NENGINES; e++)
        if (s == ENGINES[e]->name)
            return 1;
    return 0;
}

/* at most 6 characters, and no wider than maxw (review 21: CHANCE, STRIDE, BOUNCE whole) */
static void fit(char *d, const char *src, const felucca_font_t *f, int32_t maxw)
{
    str_cpy(d, src, is_eng_name(src) ? 8 : 7);           /* engine names are kept whole */
    while (d[0] && text_w(f, d) > maxw)
        d[str_len(d) - 1u] = 0;
}

static int32_t batt_level(void)                         /* ADC ch3 thresholds */
{
    return song.batt_raw >= 591 ? 3 : song.batt_raw >= 561 ? 2 : song.batt_raw >= 531 ? 1 : 0;
}
/* charging (a USB host powers us; there is no charger status line): bars fill 1, 2, 3
 * every 0.6 s, so the header is redrawn twice a second at most; else the level */
static int32_t batt_shown(void)
{
    if (usb.config && !usb.suspended)
        return 1 + (int32_t)((fm1_ms / 600u) % 3u);
    return batt_level();
}

/* a page every voice shares (Ben's colour rule, review 16b): GLO's GLOBAL, SYSTEM, MIDI; (wave 8) the FX button's DELAY,
 * REV/CHO, MASTER, the whole mix's */
static int shared_page(const page_t *pg)
{
    return !ui.home && (pg->fam == FAM_GLO || pg->fam == FAM_MFX) && pg->scope == SC_GLOBAL;
}
static int drums_page(const page_t *pg) { (void)pg; return 0; }   /* (GLO > DRUMS: gone in wave 8, its four elsewhere) */

/* the header (ui_op.c): the track's tag, the page's title; a message shows in the knob band. A shared page: "GLO" in
 * neutral grey; GLO > DRUMS: the drum track's */
static void draw_head(void)
{
    const page_t *pg = cur_page();
    const engine_t *e = ENGINES[TSEL->eng_req % NENGINES];
    const char *title = ui.home ? "HOME" : pg->scope == SC_ENGINE && !is_drum(TSEL) ? e->page_title[pg->id[0] != P_E0] : pg->title;
    if (mlu.on)                                       /* MIDI LEARN on (2.6, GLO + F#3) */
        title = "LEARN";
    if ((pg->graph == GR_SPATT || pg->graph == GR_PROJ) && !ui.home)   /* (wave 11) SAVE > PATTERNS (the bank: SAVE >
                                                         * PROJECT): the whole project's, no voice's tag */
        op_header("SAVE", TE_G4, title, &ui.head_sig);
    else if (pg->graph == GR_UKIT && !ui.home)          /* (wave 12) SAVE > KITS: the drum voice's kit, whichever is selected */
        op_header("DRUM", TE_DRUM, title, &ui.head_sig);
    else if (shared_page(pg))
        op_header(pg->fam == FAM_MFX ? "MIX" : "GLO", TE_G4, title, &ui.head_sig);   /* (wave 9: the whole mix's, after a
                                                         * voice's FX in the FX group: MIX, never a voice's tag) */   /* (wave 7: the button that opens them) */
    else if (drums_page(pg))
        op_header("DRUM", TE_COL[TRK_DRUM], title, &ui.head_sig);
    else
    {   /* (PHEBUS wave 4) a synth track: its number, as the sound pages and the GRID (the Type is the engine now) */
        char tg[4] = {(char)('1' + (song.sel & 3u)), 0};
        op_header(!is_drum(TSEL) ? tg : "DRUM", TE_COL[song.sel & 3u], title, &ui.head_sig);
        (void)e;
    }
}
/* (the bands cover the screen: header, graph, the pages strip, the knob band, the track bar) */
static void draw_frame(void) {}

/* one knob: label, value, unit, gauge (ratio 0..1000, -1 = none), kept for the knob band (ui_op.c op_knobs),
 * which draw_columns draws when the four are set; vc C_DIM: a value that does nothing now. (icon: unused,
 * the labels carry the knob's colour) */
static struct {
    char lab[4][8], val[4][10], unit[4][8];
    int32_t ratio[4];
    uint8_t dim;
} opk;
static void draw_column(uint32_t c, const char *label, const char *val, const char *unit, uint16_t vc,
                        int32_t ratio, uint32_t icon)
{
    char l[8], v[10], u[8];
    (void)icon;
    fit(l, label, &FONT_S, 54);
    fit(v, val, &FONT_S, is_eng_name(val) ? 56 : 54);   /* engine names whole; (review V2) six letters */
    fit(u, unit, &FONT_S, 54 - text_w(&FONT_S, v) - 3);
    if (c < 4u) {                                       /* (the big values: their own fit) */
        str_cpy(ui.big_l[c], l, 8);
        fit(ui.big_v[c], val, &FONT_S, 112);            /* (review V3: printed in its row, small type) */
        str_cpy(ui.big_u[c], unit, 8);
        ui.big_c[c] = vc;
        str_cpy(opk.lab[c], l, 8);
        str_cpy(opk.val[c], v, 10);
        str_cpy(opk.unit[c], u, 8);
        opk.ratio[c] = ratio;
        opk.dim = (uint8_t)((opk.dim & ~(1u << c)) | (uint32_t)(vc == C_DIM) << c);
    }
    if (c == ui.hot_col) {
        str_cpy(ui.focus_l, l, 8);
        str_cpy(ui.focus_v, v, 8);
        str_cpy(ui.focus_u, u, 8);
    }
}

/* ENV: the envelope, a segment per knob in its colour (ATK DEC SUS REL). Matches voice.c: attack is linear,
 * decay and release are exponential (env += (target - env) * k each tick, ~99 % after the set time); the
 * time axis is the parameter value (the times themselves are exponential). It follows the voice that
 * sounds (ui_op.c op_env_voice): up the attack, down to the sustain, along it while the note is held,
 * down the release; its level in large digits, GATE lit while a key is down. */
static void graph_adsr(const track_t *t, uint16_t c)
{
    static int32_t plat;                                                /* px along the sustain so far */
    int32_t a = 4 + t->p[P_ATK] * 50 / 127, d = 6 + t->p[P_DEC] * 50 / 127, r = 6 + t->p[P_REL] * 60 / 127;
    int32_t top = 38, bot = 92, sus = t->p[P_SUS] * 1000 / 127;          /* 0..1000 */
    int32_t x0 = 8, x1 = x0 + a, x2 = x1 + d, x3 = 230 - r, i, px, py, y, tx = x0, ty, dpos = -1, rpos = -1;
    int32_t e = 32768;                                                  /* exp(-4.6 u), Q15 */
    const voice_t *v = op_env_voice(t);
    int32_t stage = v ? v->stage : 0, cur = v ? clamp(((v->env >> 14) * 1000) >> 10, 0, 1000) : 0, gate = stage == 1 || stage == 2;
    (void)c;
#define EGY(lvl) (bot - (lvl) * (bot - top) / 1000)
#define EGL(ax, ay, bx, by, col) (cv_line(ax, ay, bx, by, col), cv_line((ax) + 1, ay, (bx) + 1, by, col), \
                                 cv_line(ax, (ay) - 1, bx, (by) - 1, col))   /* the curve 2 px (review 19) */
    ty = bot;
    /* (Ben, 21 Nov 2026: no character here: "I'd rather something more chill there ... Maybe not even the character") */
    cv_rect(0, bot + 1, 240, 1, TE_G1);
    for (y = top; y < bot; y += 3)                                      /* the corners, down to the floor */
        cv_pset(x1, y, OP_PUR_D);
    for (y = EGY(sus); y < bot; y += 3) {
        cv_pset(x2, y, OP_PUR_D);
        cv_pset(x3, y, OP_PUR_D);
    }
    EGL(x0, bot, x1, top, tint_col(0u, 0));                                   /* attack: linear */
    px = x1;
    py = top;
    for (i = 1; i <= d; i++) {                                          /* decay: exponential to SUS */
        int32_t lvl;
        e = (e * (32768 - 150733 / d)) >> 15;           /* k^d = exp(-4.6) */
        lvl = sus + ((1000 - sus) * e >> 15);
        EGL(px, py, x1 + i, EGY(lvl), tint_col(1u, 0));
        px = x1 + i;
        py = EGY(lvl);
        if (dpos < 0 && lvl <= cur)
            dpos = px;
    }
    EGL(px, py, x3, EGY(sus), tint_col(2u, 0));                                /* sustain */
    px = x3;
    py = EGY(sus);
    e = 32768;
    for (i = 1; i <= r; i++) {                                          /* release: exponential to 0 */
        e = (e * (32768 - 150733 / r)) >> 15;
        EGL(px, py, x3 + i, EGY(sus * e >> 15), tint_col(3u, 0));
        px = x3 + i;
        py = EGY(sus * e >> 15);
        if (rpos < 0 && (sus * e >> 15) <= cur)
            rpos = px;
    }
    cv_rect(x1 - 1, top - 1, 3, 3, C_WHITE);                                   /* a handle where the next knob takes over */
    cv_rect(x2 - 1, EGY(sus) - 1, 3, 3, C_WHITE);
    cv_rect(x3 - 1, EGY(sus) - 1, 3, 3, C_WHITE);
    if (stage == 1) {                                                   /* where the voice is on the line */
        plat = 0;
        tx = x0 + a * cur / 1000;
        ty = EGY(cur);
    } else if (stage == 2 && cur > sus + 20 && dpos >= 0) {
        tx = dpos;
        ty = EGY(cur);
    } else if (stage == 2) {                                            /* held: along the sustain, to its end */
        if (plat < x3 - x2)
            plat += 2;
        tx = x2 + (plat < x3 - x2 ? plat : x3 - x2);
        ty = EGY(sus);
    } else if (stage == 3) {
        tx = cur > sus || rpos < 0 ? x3 : rpos;
        ty = EGY(cur > sus ? sus : cur);
    } else {
        plat = 0;
    }
    if (v) {                                                            /* a note playing: its level, GATE (hidden
                                                                         * until then, review 19), where it is */
        te_digit(4, -2, 12, 26, 2, (uint32_t)(cur * 99 / 1000) / 10u, C_WHITE);
        te_digit(20, -2, 12, 26, 2, (uint32_t)(cur * 99 / 1000) % 10u, C_WHITE);
        op_box(196, 0, 40, 18, gate ? C_WHITE : TE_G4);
        te_text_c(216, 1, "GATE", gate ? C_WHITE : TE_G4);
        te_disc(tx, ty, 3, C_WHITE);
    }
#undef EGY
#undef EGL
}

/* LFO: two cycles of the wave, running (KARMA 3.0): it scrolls with the track's LFO, "now" at x = LFO_NOW with a dot
 * on the value it gives (a dim line there), what it did to the left, what comes to the right */
#define LFO_NOW 160
static void graph_lfo(const track_t *t, uint16_t c)
{
    int32_t x, py = 50, ny = 50;
    uint32_t now = t->lfo_ph, step = 0xFFFFFFFFu / 120u;
    cv_line(0, 50, 239, 50, C_LINE);
    cv_line(LFO_NOW, 4, LFO_NOW, 96, C_LINE);
    for (x = 0; x < 240; x++) {                      /* (lfo_wave only reads the track) */
        uint32_t ph = now + (uint32_t)(x - LFO_NOW) * step;
        int32_t y = 50 - lfo_wave((track_t *)t, ph) * 38 / 32768;
        if (t->p[P_LWAVE] == 4)                      /* RANDOM: a held value a sixth of a cycle, scrolling with it */
            y = 50 - ((int32_t)(((ph / 0x2AAAAAABu) * 2654435761u) >> 16) - 32768) * 38 / 32768;
        if (x)
            cv_line(x - 1, py, x, y, c);
        if (x == LFO_NOW)
            ny = y;
        py = y;
    }
    cv_rect(LFO_NOW - 3, ny - 3, 7, 7, C_WHITE);    /* now */
}

static void graph_steps(const track_t *t, uint16_t c)
{
    uint32_t i, len = (uint32_t)t->p[P_SLEN];
    for (i = 0; i < NSTEP; i++) {                    /* 4 rows of 16 thin bars */
        int32_t x = 6 + (int32_t)(i % 16u) * 14 + (int32_t)(i % 16u) / 4 * 4, y = 6 + (int32_t)(i / 16u) * 24;
        const step_t *st = &t->step[i];
        if (i >= len)
            continue;
        if (step_on(st))
            cv_rect(x, y, 1, 14, c);
        else if (st->time == ST_TIE)
            cv_rect(x, y + 6, 1, 8, C_DIM);
        else
            cv_rect(x, y + 13, 1, 1, C_DIM);
        if (st->flags & SF_ACCENT)
            cv_rect(x - 1, y - 2, 3, 1, c);
        if ((song.playing && i == t->seq_idx) || i == ui.cursor)
            cv_rect(x - 1, y + 16, 3, 3, C_WHITE);
    }
}
static void graph_scale(const track_t *t, uint16_t c)
{
    static const uint8_t BLACK[12] = {0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0};
    uint32_t i, mask = scale_mask(t);
    for (i = 0; i < 12u; i++) {
        uint32_t deg = (i + 12u - (uint32_t)t->p[P_ROOT]) % 12u;
        int32_t x = 6 + (int32_t)i * 19;
        uint16_t col = (mask >> deg) & 1u ? (deg == 0u ? C_WHITE : c) : C_DIM;
        cv_rect(x, BLACK[i] ? 10 : 40, 16, 1, col);
        cv_rect(x + 7, BLACK[i] ? 10 : 40, 1, 40, col);
    }
}

static void graph_fx(const track_t *t, uint16_t c)
{
    uint32_t i;
    for (i = 0; i < 4u; i++) {
        int32_t h = t->p[P_DIST + i] * 80 / 127, x = (int32_t)i * 60 + 28;
        cv_rect(x, 10, 1, 80, C_LINE);
        cv_rect(x, 90 - h, 1, h, c);
        cv_rect(x - 3, 90 - h, 7, 1, c);
    }
}
/* SLICER page: the pattern's 16 steps, a 'x' step a full bar; a '.' step: GATE a bar as high as
 * it stays open (DEPTH), STUT hatched (it repeats the last 'x'); the step playing underlined.
 * Grey when the SLICER is OFF. */
static void graph_slicer(const track_t *t, uint16_t c)
{
    uint32_t i, pat = sl_pattern(t), mode = (uint32_t)t->p[P_SLCR], cur = sl[t - trk].idx;
    int32_t open = 70 - t->p[P_SLDEPTH] * 70 / 127;      /* px a closed GATE step keeps */
    uint16_t col = mode == SL_OFF ? C_DIM : c;
    for (i = 0; i < 16u; i++) {
        int32_t x = 4 + (int32_t)i * 14 + (int32_t)(i / 4u) * 2, y;
        if ((pat >> i) & 1u) {
            cv_rect(x, 10, 11, 70, col);
        } else if (mode == SL_STUT) {
            for (y = 10; y < 80; y += 4)
                cv_rect(x, y, 11, 1, col);
        } else {
            cv_rect(x, 79, 11, 1, C_LINE);
            if (open)
                cv_rect(x, 80 - open, 11, open, C_DIM);
        }
        if (mode != SL_OFF && i == cur)
            cv_rect(x, 85, 11, 3, C_WHITE);
    }
}
static uint32_t steps_hash(const track_t *t)
{
    uint32_t h = 2166136261u, i;
    for (i = 0; i < NSTEP; i++) {
        const step_t *st = &t->step[i];
        h = (h ^ (st->note[0] + st->n * 128u + st->time * 1024u + st->flags * 4096u + st->note[1] * 65536u)) *
            16777619u;
    }
    return h;
}

static uint32_t str_hash(uint32_t h, const char *s)
{
    while (*s)
        h = (h ^ (uint8_t)*s++) * 16777619u;
    return h;
}

/* a page drawn with its values large (graph_big) */
static int big_page(const page_t *pg)            /* (2.6: the INSERT pages too: their middle stayed empty) */
{
    return (pg->graph == GR_NONE || pg->graph == GR_INS) && pg->scope != SC_DRUM;
}
static uint32_t graph_signature(void)
{
    const page_t *pg = cur_page();
    const track_t *t = TSEL;
    uint32_t h = 2166136261u, i;
    if (ui.hot_t && settings.zoom)
        h = str_hash(str_hash(str_hash(h ^ 0x5555u, ui.focus_v), ui.focus_l), ui.focus_u);
    if (!ui.home && big_page(pg))                    /* the big values: as the columns show them */
        for (i = 0; i < 4u; i++)
            h = str_hash(str_hash(str_hash(h ^ ui.big_c[i] * 31u, ui.big_v[i]), ui.big_l[i]), ui.big_u[i]);
    if (ui.home)
        return h ^ (ui.frame / 2u);                  /* scope: redraw every other frame */
    h ^= (uint32_t)pg->graph * 131u + TSEL->eng_req + song.sel * 7777u;
    if (fam_stage(pg->fam) >= 0 && (pg->graph == GR_FX || big_page(pg))) {   /* a sound page (op_sound): as its knobs move
                                                     * it; a swept INSERT's notches run */
        if (pg->graph == GR_INS && t->p[P_ITYPE] >= IT_PHASER)
            h ^= (ui.frame + 1u) * 40503u;
    } else if (big_page(pg) && !is_drum(t))          /* the track's scene (ui_op.c): moves while the track sounds */
        h ^= (t->peak > 64 || op_lvl || op_hit ? ui.frame / 2u : 0u) * 40503u + op_note * 977u + user_of(t) * 31u;
    for (i = 0; i < P_COUNT; i++)
        h = (h ^ (uint32_t)t->p[i]) * 16777619u;
    h ^= (uint32_t)TSEL->preset * 7u + (uint32_t)song.g[G_SLOT] * 13u + TSEL->user * 257u + up_gen * 7919u + ui.uslot * 104729u;
    if (pg->graph == GR_ARP && !is_drum(t))          /* ARP: the stones as the keys change, the hops */
        h ^= (t->nheld * 31u + t->arp_note * 977u + (op_arp_hop ? ui.frame + 1u : 0u)) * 40503u;
    if (pg->graph == GR_LFO && !is_drum(t))          /* LFO: the wave runs (every other frame) */
        h ^= (ui.frame / 2u + 1u) * 40503u;
    if (pg->graph == GR_ADSR && !is_drum(t) && op_env_voice(t))   /* ENV: it follows the voice */
        h ^= (ui.frame / 2u + 1u) * 40503u;
    if (pg->graph == GR_SLCR && t->p[P_SLCR])        /* the SLICER's step playing */
        h ^= (sl[song.sel].idx + 1u) * 2654435761u;
    if (pg->fam == FAM_MFX && pg->id[0] == G_DUST)  /* (review V3) MASTER's meter runs */
        h ^= (ui.frame / 2u + 1u) * 40503u;
    if (pg->graph == GR_PROJ)                        /* (the bank) the two summaries, the live state */
        h ^= projpg_sig() * 2654435761u;
    if (pg->graph == GR_SLOTS)                       /* (a checksum over each slot) */
        for (i = 0; i < 4u; i++)
            h ^= (uint32_t)project_used(i) << (20u + i);
    if (pg->graph == GR_STEPS) {
        uint32_t ph = song.playing ? t->seq_idx : 0xFFFFu;
        h ^= steps_hash(t) + ph * 31u + ui.cursor * 7919u;
    }
    return h;
}
/* preset browser: the list by kind (ui.c BANK), the current one in white */
static void graph_browse(void)
{
    uint32_t total, cur, e, k;
    int32_t row;
    if (is_drum(TSEL)) {                                 /* (wave 12) the drums: the whole kits, the one loaded teal (CUSTOM:
                                                         * a kit changed from it, first) */
        int32_t p = kit_changed() ? -1 : kit_pos(pkit.base);
        uint32_t n = kit_src_n();
        for (row = -3; row <= 3; row++) {
            int32_t y = 4 + (row + 3) * 17, q = (p < 0 ? 0 : p) + row;
            char nm[13];
            int sel = row == 0;
            if (p < 0 && row == 0) {
                str_cpy(nm, "CUSTOM", sizeof nm);
            } else {
                if (p < 0 && row > 0)
                    q--;
                q = (q % (int32_t)n + (int32_t)n) % (int32_t)n;
                kit_name_of(kit_src((uint32_t)q), nm, sizeof nm);
            }
            if (sel)
                cv_rect(4, y + 6, 3, 3, C_WHITE);
            cv_text(14, y, &FONT_S, p < 0 && sel ? "" : kit_src((uint32_t)q) < DRUM_SAMPLED ? "HEX" : kit_src((uint32_t)q) < DRUM_USR ? "CLS" : "MY", sel ? C_GRAY : C_DIM);
            cv_text(54, y, &FONT_S, nm, sel ? TE_DRUM : C_GRAY);
        }
        return;
    }
    cur = preset_pos(&total);
    if (!total)
        return;
    for (row = -3; row <= 3; row++) {
        int32_t y = 4 + (row + 3) * 17;
        char tag[5], nm[13];
        int sel = row == 0;
        uint32_t n = (cur + total * 4u + (uint32_t)row) % total;
        e = preset_at(n, &k);
        if (e == NENGINES) {                             /* user preset: "U07" and its name */
            up_slot_label(tag, k);
            up_name(k, nm);
        } else {                                         /* its kind: BASS, KEYS, PAD... */
            str_cpy(tag, preset_kind(n), sizeof tag);
            str_cpy(nm, ENGINES[e]->presets[k].name, sizeof nm);
        }
        if (sel)
            cv_rect(4, y + 6, 3, 3, C_WHITE);
        cv_text(14, y, &FONT_S, tag, sel ? C_GRAY : C_DIM);
        cv_text(54, y, &FONT_S, nm, sel ? C_WHITE : C_GRAY);
    }
}

/* user preset slots around the selected one: "U07  NAME" / EMPTY, the selected one in white */
static void graph_user(void)
{
    int32_t row, first = clamp((int32_t)ui.uslot - 3, 0, UP_SLOTS - 7);
    for (row = 0; row < 7; row++) {
        uint32_t k = (uint32_t)(first + row);
        int32_t y = 4 + row * 17;
        char tag[4], nm[13];
        int sel = k == ui.uslot, used = up_used(k);
        up_slot_label(tag, k);
        if (used)
            up_name(k, nm);
        else
            str_cpy(nm, "EMPTY", sizeof nm);
        if (sel)
            cv_rect(4, y + 6, 3, 3, C_WHITE);
        cv_text(14, y, &FONT_S, tag, sel ? C_WHITE : C_GRAY);
        cv_text(54, y, &FONT_S, nm, used ? (sel ? C_WHITE : C_HI) : C_DIM);
    }
}

/* (wave 11) SAVE > PATTERN: the four patterns, stored or empty, the one playing marked, the one KNOB 1 picks in white */
static uint8_t spatt_sel;                               /* the pattern KNOB 1 picks (0..3) */
static int32_t patt_shown(void);                        /* (ui_layers.c) */
static void graph_spatt(void)
{
    uint32_t i, ready = arrangement_ready();
    int32_t pl = patt_shown();
    for (i = 0; i < 4u; i++) {
        int32_t y = 6 + (int32_t)i * 26;
        char b[2] = {(char)('A' + i), 0};
        int sel = i == spatt_sel, used = (ready >> i) & 1u;
        if (sel)
            cv_rect(4, y + 6, 3, 3, C_WHITE);
        cv_text(14, y, &FONT_S, b, sel ? C_WHITE : C_GRAY);
        cv_text(40, y, &FONT_S, used ? "STORED" : "EMPTY", used ? (sel ? C_WHITE : C_HI) : C_DIM);
        if (pl == (int32_t)i)
            cv_text(130, y, &FONT_S, "PLAYING", TE_G4);
    }
}
/* (the bank, Ben 22:45) SAVE > PROJECT: what the live project and the bank hold, in plain words, so the two can be told
 * apart (no names: Ben asked for "just enough"); whole-instrument, white and grey. A pattern count, the tempo, the kit:
 *   LIVE                 SAVED          (or NOT SAVED YET: the unsaved dot's state; playing: STOP TO SAVE)
 *   4 PATTERNS  92 BPM
 *   KIT: CUSTOM
 *   BANK
 *   EMPTY: NOTHING KEPT YET             (or as LIVE's two lines)
 * The knobs: KEEP, SWAP, NEW (a turn right; the question that follows says HOLD SAVE) */
static void proj_text(char *a, char *b, uint32_t np, int32_t bpm, const char *kit)
{
    if (!np)
        str_cpy(a, "NO PATTERNS", 24);
    else {
        fmt_int(a, (int32_t)np);
        str_cpy(a + str_len(a), np == 1u ? " PATTERN" : " PATTERNS", 12);
    }
    str_cpy(a + str_len(a), "  ", 3);
    fmt_int(a + str_len(a), bpm);
    str_cpy(a + str_len(a), " BPM", 5);
    str_cpy(b, "KIT: ", 6);
    str_cpy(b + 5, kit[0] ? kit : "?", 10);
}
static uint32_t proj_live_npat(void)
{
    uint32_t r = arrangement_ready(), n = 0;
    for (; r; r &= r - 1u)
        n++;
    return n;
}
static const char *proj_live_state(void)               /* the live project against flash, in words (the unsaved dot) */
{
    return !proj_unsaved ? "SAVED" : song.playing ? "STOP TO SAVE" : "NOT SAVED YET";
}
static uint32_t projpg_sig(void)
{
    uint32_t np;
    int32_t bpm;
    char kit[9], k2[12];
    uint32_t h = bank_summary(&np, &bpm, kit) * 7u + np * 31u + (uint32_t)bpm * 977u;
    drum_kit_name(k2, sizeof k2);
    return str_hash(str_hash(h + proj_live_npat() * 13u + (uint32_t)song.g[G_BPM] * 7919u + proj_unsaved * 104729u +
                             (uint32_t)song.playing * 3u, kit), k2);
}
static void graph_proj(void)
{
    char a[24], b[24], kit[12];
    uint32_t np;
    int32_t bpm;
    const char *st = proj_live_state();
    cv_text(4, 0, &FONT_S, "LIVE", C_WHITE);
    cv_text(236 - text_w(&FONT_S, st), 0, &FONT_S, st, proj_unsaved ? C_WHITE : TE_G4);
    drum_kit_name(kit, sizeof kit);
    proj_text(a, b, proj_live_npat(), song.g[G_BPM], kit);
    cv_text(4, 16, &FONT_S, a, C_HI);
    cv_text(4, 32, &FONT_S, b, C_HI);
    cv_rect(4, 50, 232, 1, C_LINE);
    cv_text(4, 53, &FONT_S, "BANK", C_WHITE);
    if (bank_summary(&np, &bpm, kit)) {
        proj_text(a, b, np, bpm, kit);
        cv_text(4, 69, &FONT_S, a, C_HI);
        cv_text(4, 85, &FONT_S, b, C_HI);
    } else {
        cv_text(4, 69, &FONT_S, "EMPTY: NOTHING KEPT YET", C_DIM);
    }
}
/* the song's parts A..D (SAVE > PARTS): used / empty, the selected one in white */
static void graph_slots(void)
{
    uint32_t i;
    for (i = 0; i < 4u; i++) {
        int32_t y = 8 + (int32_t)i * 26;
        char b[4];
        int sel = (int32_t)i + 1 == song.g[G_SLOT];
        b[0] = (char)('A' + i);
        b[1] = 0;
        if (sel)
            cv_rect(4, y + 6, 3, 3, C_WHITE);
        cv_text(14, y, &FONT_S, b, sel ? C_WHITE : C_GRAY);
        cv_text(40, y, &FONT_S, project_used(i) ? "USED" : "EMPTY", project_used(i) ? (sel ? C_WHITE : C_HI) : C_DIM);
    }
}

/* TRACKS page: four channel strips (mixer style), one under each column: number +
 * REC / ARM / MUTE, the sound's short name, then a level fader with the output meter
 * (note activity), the pattern over its LEN (time running down; bar width = notes in
 * the step) and the play head. The selected track is drawn bright. Every part is
 * its own small canvas with its own signature: while the transport runs only the
 * head markers and the meters move. (No ZOOM focus here.) */
#define TS_HEAD_Y (Y_GRAPH + 2)
#define TS_NAME_Y (Y_GRAPH + 20)
#define TS_Y (Y_GRAPH + 38)
#define TS_H 84                                      /* body: rows Y_GRAPH + 38 .. + 121 */
static struct {
    uint32_t head[NTRK], name[NTRK], fader[NTRK], ov[NTRK], mark[NTRK];
    uint8_t meter[NTRK];
} ts;

static int32_t meter_px(int32_t a)                   /* |sample| (Q15) -> px: 6 dB = TS_H / 10 */
{
    int32_t lg = 0, v;
    if (a < 64)
        return 0;
    while ((a >> lg) > 1)
        lg++;
    v = lg * 8 + (((a << 3) >> lg) & 7);             /* 8 log2(a): 48 (-54 dB) .. 128 (+6 dB) */
    return clamp((v - 48) * (TS_H - 2) / 80, 0, TS_H - 2);
}

static uint32_t trk_level(uint32_t c)                /* LEVEL 0..127 (the drum track: GLO > DRUMS LEVEL) */
{
    return (uint32_t)(c == TRK_DRUM ? song.g[G_DRLVL] : trk[c].p[P_LEVEL]) & 127u;
}

static void trk_short_name(uint32_t c, char *b)      /* the track's sound, b holds 13 */
{
    const track_t *t = &trk[c];
    const engine_t *e = ENGINES[t->eng_req % NENGINES];
    if (c == TRK_DRUM)
        str_cpy(b, "DRUM", 13);
    else if (user_of(t) < UP_SLOTS)
        up_name(user_of(t), b);
    else if (e->npresets)
        str_cpy(b, e->presets[t->preset % e->npresets].name, 13);
    else
        str_cpy(b, e->name, 13);
}

static void draw_tracks(void)
{
    uint32_t c;
    if (ui.force) {
        lcd_fill(0, Y_GRAPH, 240, H_GRAPH, C_BLACK);
        for (c = 0; c < NTRK; c++)
            ts.meter[c] = 0;
    }
    for (c = 0; c < NTRK; c++) {
        track_t *t = &trk[c];
        uint32_t x0 = c * 60u + 4u, sel = c == song.sel, lvl = trk_level(c), mute = !lvl || t->p[P_MUTE];
        uint32_t arm = (song.rec >> c) & 1u, st = arm ? (song.playing ? 1u : 2u) : mute ? 3u : 0u, sig;
        uint32_t len = t->p[P_SLEN] > 0 ? (uint32_t)t->p[P_SLEN] : 1u, row = 0xFFFFu;
        int32_t pk, m;
        char b[16];
        if (c == TRK_DRUM) {
            pk = drums.peak;
            drums.peak = 0;
        } else {
            pk = t->peak;
            t->peak = 0;
        }
        /* head: number (white = selected), REC / ARM / MUTE */
        sig = 1u + sel + st * 2u;
        if (ui.force || sig != ts.head[c]) {
            ts.head[c] = sig;
            b[0] = (char)('1' + c);
            b[1] = 0;
            cv_begin(54, 16, C_BLACK);
            cv_text(0, 0, &FONT_S, b, sel ? C_WHITE : C_GRAY);
            if (sel)
                cv_rect(0, 15, 8, 1, C_WHITE);
            if (st == 1u || st == 2u) {
                cv_rect(14, 5, 6, 6, st == 1u ? C_WHITE : C_AMB);
                cv_text(24, 0, &FONT_S, st == 1u ? "REC" : "ARM", st == 1u ? C_WHITE : C_AMB);
            } else if (st) {
                cv_text(14, 0, &FONT_S, "MUTE", C_DIM);
            }
            cv_blit(x0, TS_HEAD_Y);
        }
        /* the sound: preset name (user preset, DRUM), cut to the column */
        trk_short_name(c, b);
        while (b[0] && text_w(&FONT_S, b) > 54)
            b[str_len(b) - 1u] = 0;
        sig = str_hash(2u + sel, b);
        if (ui.force || sig != ts.name[c]) {
            ts.name[c] = sig;
            cv_begin(54, 16, C_BLACK);
            cv_text(0, 0, &FONT_S, b, sel ? C_HI : C_DIM);
            cv_blit(x0, TS_NAME_Y);
        }
        /* fader (LEVEL) + meter of the output */
        m = mute ? 0 : meter_px(pk);
        if (m < ts.meter[c] - 3)
            m = ts.meter[c] - 3;                     /* falls ~20 dB/s */
        ts.meter[c] = (uint8_t)(m < 0 ? 0 : m);
        sig = 1u + lvl + ts.meter[c] * 128u + sel * 65536u + (sel && ui.hot_t && ui.hot_col == 1u) * 131072u +
              (t->p[P_MUTE] != 0) * 262144u;
        if (ui.force || sig != ts.fader[c]) {
            int32_t fy = (int32_t)(TS_H - 2u) - (int32_t)lvl * (TS_H - 4) / 127;
            ts.fader[c] = sig;
            cv_begin(12, TS_H, C_BLACK);
            cv_rect(3, 0, 1, TS_H, C_LINE);
            if (lvl) {
                cv_rect(2, fy, 3, TS_H - fy, C_DIM);
                cv_rect(0, fy - 1, 7, 2, (sig & 131072u) ? C_WHITE : sel ? C_HI : C_GRAY);
            } else {
                cv_rect(0, TS_H - 2, 7, 2, C_DIM);
            }
            if (ts.meter[c])
                cv_rect(9, TS_H - ts.meter[c], 3, ts.meter[c], C_AMB);
            cv_blit(x0, TS_Y);
        }
        /* the pattern: one band per step over LEN, a note step as wide as its notes */
        sig = steps_hash(t) * 31u + len * 7u + sel;
        if (ui.force || sig != ts.ov[c]) {
            uint32_t i;
            ts.ov[c] = sig;
            cv_begin(34, TS_H, C_BLACK);
            cv_rect(17, 0, 1, TS_H, C_LINE);
            for (i = 0; i < len; i++) {
                const step_t *s = &t->step[i];
                int32_t r0 = (int32_t)(i * TS_H / len), r1 = (int32_t)((i + 1u) * TS_H / len), h = r1 - r0;
                if (h > 2)
                    h--;                             /* a gap between steps when there is room */
                if (h < 1)
                    h = 1;
                if (step_on(s)) {
                    int32_t w = 2 + 6 * (int32_t)s->n;
                    cv_rect(17 - w / 2, r0, w, h, (s->flags & SF_ACCENT) && sel ? C_WHITE : sel ? C_HI : C_GRAY);
                } else if (s->time == ST_TIE) {
                    cv_rect(16, r0, 3, r1 - r0, C_DIM);
                }
            }
            cv_blit(x0 + 14u, TS_Y);
        }
        /* play head: a small arrow right of the pattern (white while recording) */
        if (song.playing)
            row = ((t->seq_idx % len) * 2u + 1u) * TS_H / (2u * len);
        sig = 1u + row + (st == 1u) * 65536u;
        if (ui.force || sig != ts.mark[c]) {
            int32_t i;
            ts.mark[c] = sig;
            cv_begin(5, TS_H, C_BLACK);
            if (row != 0xFFFFu)
                for (i = -2; i <= 2; i++) {
                    int32_t w = 5 - 2 * (i < 0 ? -i : i);
                    cv_rect(5 - w, (int32_t)row + i, w, 1, st == 1u ? C_WHITE : C_AMB);
                }
            cv_blit(x0 + 49u, TS_Y);
        }
    }
}

#if !FELUCCA_ARRANGER                                   /* (PHEBUS wave 3: HOME is the TRACKS page, ui.home is never set:
                                                         * the scope and its 1 KB snapshot only in a build without it) */
/* oscilloscope of the output, triggered on a rising zero crossing */
static void graph_scope(uint16_t c)
{
    static int16_t snap[SCOPE_N];
    uint32_t w = scope_w, i, trig = 0;
    int32_t py = 50, x, peak = 1500;
    for (i = 0; i < SCOPE_N; i++) {
        snap[i] = scope_buf[(w + i) & (SCOPE_N - 1u)];
        if (snap[i] > peak)
            peak = snap[i];
        else if (-snap[i] > peak)
            peak = -snap[i];
    }
    for (i = 1; i < SCOPE_N - 240u; i++)
        if (snap[i - 1] < 0 && snap[i] >= 0) {
            trig = i;
            break;
        }
    cv_line(0, 50, 239, 50, C_LINE);
    for (x = 0; x < 240; x++) {
        int32_t y = 50 - snap[trig + (uint32_t)x] * 44 / peak;   /* auto-scaled */
        if (x)
            cv_line(x - 1, py, x, y, c);
        py = y;
    }
}
#endif

/* SLOOP 2.4: a page without a graph (EDIT, VOICE, the DEST pages, GLOBAL, MASTER, SYSTEM...): its values
 * large in the empty space, 2 x 2 as the knobs (KNOB 1 2 / KNOB 3 4), the one turned in white; one value
 * alone (FILTER) across the width. The columns (draw_columns, drawn first) fill ui.big_* */
static void graph_big(void)
{
    uint32_t c, n = 0;
    for (c = 0; c < 4u; c++)
        n += ui.big_l[c][0] != 0;
    if (n > 1u) {
        cv_rect(119, 6, 1, 112, C_LINE);
        cv_rect(6, 62, 228, 1, C_LINE);
    }
    for (c = 0; c < 4u; c++) {
        int32_t x0 = c & 1u ? 126 : 6, y0 = c & 2u ? 64 : 2, x;
        if (!ui.big_l[c][0])
            continue;
        if (n == 1u) {                                  /* one value: in the middle */
            x0 = (240 - text_w(&FONT_L, ui.big_v[c]) - (ui.big_u[c][0] ? text_w(&FONT_S, ui.big_u[c]) + 4 : 0)) / 2;
            y0 = 30;
        }
        cv_text(x0, y0 + 2, &FONT_S, ui.big_l[c], tint_col(c, 0));
        (void)x;
        if (opk.ratio[c] >= 0) {                        /* (wave 4: the value once, in the dials; here where it sits) */
            cv_rect(x0, y0 + 30, 100, 3, TE_G2);
            cv_rect(x0, y0 + 30, opk.ratio[c] * 100 / 1000, 3, ui.big_c[c] == C_DIM ? C_DIM : tint_col(c, 0));
        }
    }
}

/* the settings pages (GLO, SAVE > TOOLS; 2026-10-29, Ariel: they should read as a menu): the four knobs as rows, each
 * its knob's number and colour, its name, its value large (the track's character at the side is cut) */
static int menu_page(const page_t *pg)
{
    return pg->scope == SC_GLOBAL && pg->graph == GR_NONE && (pg->fam == FAM_GLO || pg->fam == FAM_SAVE || pg->fam == FAM_MFX);
}
static int dev_restart(uint32_t id);                    /* (ui_menu.c) */
/* (review V3, Ben 26 Nov 2026: "plain rows on GLO") each knob a row: its number in a chip, its name, its value printed
 * right-aligned (no bar: the dials under give the rest); COLOUR shows the palette's colours, USB and SERIAL say RESTART
 * while the change waits for the next start */
static void graph_menu(const track_t *t)
{
    uint32_t c;
    (void)t;
    for (c = 0; c < 4u; c++) {
        int32_t y = 2 + (int32_t)c * 26, xv;
        uint32_t id = cur_page()->id[c];
        uint16_t vc = ui.big_c[c] == C_DIM ? C_DIM : C_WHITE;
        char n[2] = {(char)('1' + c), 0}, v[16];
        if (!ui.big_l[c][0])
            continue;
        cv_rect(4, y + 6, 12, 18, tint_col(c, 0));      /* the knob: its number, in the page's tint */
        cv_text(6, y + 7, &FONT_S, n, C_BLACK);
        cv_text(22, y + 7, &FONT_S, ui.big_l[c], tint_col(c, 0));
        str_cpy(v, ui.big_v[c], sizeof v);
        if (ui.big_u[c][0] && str_len(v) + str_len(ui.big_u[c]) + 1u < sizeof v) {
            if (ui.big_u[c][0] != '%')
                str_cpy(v + str_len(v), " ", 2);
            str_cpy(v + str_len(v), ui.big_u[c], sizeof v - str_len(v));
        }
        xv = 228 - text_w(&FONT_S, v);
        cv_text(xv, y + 7, &FONT_S, v, vc);
        if (id == V_COLOR) {                            /* the palette's colours, left of its name */
            uint32_t q;
            for (q = 0; q < 5u; q++)
                cv_rect(xv - 8 - (5 - (int32_t)q) * 12, y + 10, 9, 9, pal[q]);
        } else if (id >= V_DEV0 && id <= V_DEVN && dev_restart(id)) {
            cv_text(xv - 8 - text_w(&FONT_S, "RESTART"), y + 7, &FONT_S, "RESTART", C_AMB);
        }
        cv_rect(22, y + 25, 206, 1, C_LINE);
    }
}

/* (review V3, Ben 26 Nov 2026: "Pictures on MIX") the whole mix's pages draw what their knobs do, white and grey as the
 * whole mix is, as a voice's pages do: DELAY its taps (spaced by TIME, falling by FDBK, as loud as MIX, duller by COLOUR),
 * REV/CHO the reverb's tail (as long as SIZE, darker sooner with DAMP) over the chorus's wave (RATE, DEPTH), MASTER the
 * output's meter, the DUCK's dip on each beat and the DUST */
static void graph_mix(const page_t *pg)
{
    int32_t base = 98, top = 10, i;
    cv_rect(6, base + 1, 228, 1, TE_G2);
    if (pg->id[0] == G_DTIME) {
        static const uint8_t Q[9] = {72, 36, 18, 9, 24, 12, 54, 27, 108};   /* (N_DLY: px a tap, a quarter = 72) */
        int32_t dx = Q[clamp(song.g[G_DTIME], 0, 8)], h = (base - top) * song.g[G_DMIX] / 127, x = 10 + dx;
        int32_t fb = clamp(song.g[G_DFDBK], 0, 120), dull = 127 - song.g[G_DCOLOR];
        cv_rect(8, top, 4, base - top + 1, C_WHITE);    /* the dry hit */
        cv_text(16, top - 8, &FONT_S, "DRY", TE_G3);
        for (i = 0; x < 232 && h > 0; i++, x += dx) {
            int32_t g = i * dull / 40;
            cv_rect(x - 1, base + 1 - h, 3, h, g > 2 ? TE_G3 : g > 0 ? TE_G4 : C_WHITE);
            h = fb >= 100 ? h : h * fb / 100;
        }
    } else if (pg->id[0] == G_RSIZE) {
        int32_t len = 40 + song.g[G_RSIZE] * 150 / 127, damp = song.g[G_RDAMP], wy = 84;
        int32_t amp = 2 + song.g[G_CDEPTH] * 10 / 127, cyc = 1 + song.g[G_CRATE] * 7 / 127, py = wy;
        uint32_t r = 0x2545F491u;
        for (i = 0; i < len; i += 2) {                  /* the tail: dense lines, falling to nothing at SIZE's end */
            int32_t e = (len - i) * 1000 / len, eh, h;
            e = e * e / 1000;
            r = r * 1664525u + 1013904223u;
            eh = (wy - 6 - top) * e / 1000;
            h = eh / 2 + (int32_t)((r >> 24) * (uint32_t)eh / 512u);
            cv_rect(12 + i, wy - 6 - h, 1, h, i * 127 / len > 127 - damp ? TE_G3 : TE_G4);
        }
        cv_rect(8, top, 2, wy - 6 - top, C_WHITE);      /* the hit */
        for (i = 0; i <= 224; i += 2) {                 /* the chorus: its wave, RATE's cycles, DEPTH high */
            int32_t y = wy - SINE[(uint32_t)(i * cyc * 1024 / 224) & 1023u] * amp / 32767;
            if (i)
                cv_line(6 + i - 2, py, 6 + i, y, TE_G4);
            py = y;
        }
    } else {                                            /* MASTER: the output, the duck, the dust */
        int32_t pk = drums.peak, m, w, x;
        uint32_t r = 0x9E3779B9u, k;
        for (k = 0; k < NPART; k++)
            pk = trk[k].peak > pk ? trk[k].peak : pk;
        for (k = 0; k < NPART; k++)
            trk[k].peak = 0;
        drums.peak = 0;
        m = meter_px(pk) * 228 / (TS_H - 2);
        op_lvl = m > op_lvl - 6 ? m : op_lvl - 6;       /* (falls, as the voices' meters) */
        cv_text(6, 0, &FONT_S, "OUT", TE_G3);
        for (x = 0; x < 57; x++)                        /* 57 segments of 4 px */
            cv_rect(6 + x * 4, 16, 3, 12, x * 4 < op_lvl ? (x >= 52 ? C_WHITE : TE_G4) : TE_G2);
        cv_text(6, 34, &FONT_S, "DUCK", TE_G3);
        w = song.g[G_DUCK] * 22 / 127;                  /* a beat's dip: down by DUCK, back up over the beat */
        for (k = 0; k < 4u; k++) {
            int32_t x0 = 6 + (int32_t)k * 57, j, py = 52;
            for (j = 0; j < 57; j += 3) {
                int32_t y = 52 + w - w * j / 56;
                if (j)
                    cv_line(x0 + j - 3, py, x0 + j, y, song.g[G_DUCK] ? C_WHITE : TE_G3);
                py = y;
            }
        }
        cv_text(6, 78, &FONT_S, "DUST", TE_G3);
        for (k = 0; k < (uint32_t)song.g[G_DUST] * 2u; k++) {
            r = r * 1664525u + 1013904223u;
            cv_rect(50 + (int32_t)((r >> 8) % 182u), 80 + (int32_t)((r >> 20) % 16u), 1, 1, TE_G4);
        }
        (void)base;
    }
}
static uint32_t key_strip_c;                            /* (wave 10: the KEY page's chord strip, drawn: draw_foot) */
static void draw_graph(void)
{
    const page_t *pg = cur_page();
    const track_t *t = TSEL;
    uint16_t c = TE_COL[song.sel & 3u];               /* LIVE: the curves in the track's colour */
    uint32_t sig, top, drum_note = !ui.home && is_drum(t) && !page_for_drum(pg);
    sig = graph_signature();
    if (!ui.force && sig == ui.graph_sig)
        return;
    ui.graph_sig = sig;
    cv_begin(240, H_GRAPH, C_BLACK);
    cv_oy = G_OY;
    if (ui.home) {
#if !FELUCCA_ARRANGER
        graph_scope(c);
#endif
    } else if (drum_note) {                          /* a page the drum track has no use for */
        static const char *const L[2] = {"DRUM VOICE", "SEQ  TRACKS  GLO DRUMS"};
        cv_text((240 - text_w(&FONT_S, L[0])) / 2, 26, &FONT_S, L[0], C_HI);
        cv_text((240 - text_w(&FONT_S, L[1])) / 2, 52, &FONT_S, L[1], C_DIM);
    } else {
        switch (pg->graph) {
        case GR_ADSR:
            graph_adsr(t, c);
            break;
        case GR_LFO:
            graph_lfo(t, c);
            break;
        case GR_STEPS:
            graph_steps(t, c);
            break;
        case GR_SCALE:
            graph_scale(t, c);
            break;
        case GR_ARP:
            cv_oy = 0;
            op_arp_scene(t);                         /* the stepping stones (ui_op.c) */
            break;
        case GR_FX:
            cv_oy = 0;
            op_sound(t, pg);              /* what the knobs do, big (ui_op.c, review 8) */
            break;
        case GR_SLCR:
            graph_slicer(t, c);
            break;
        case GR_BROWSE:
            cv_oy = 0;
            graph_browse();
            break;
        case GR_SLOTS:
            cv_oy = 0;
            graph_slots();
            break;
        case GR_USER:
            cv_oy = 0;
            graph_user();
            break;
        case GR_SPATT:
            cv_oy = 0;
            graph_spatt();
            break;
        case GR_UKIT:
            cv_oy = 0;
            graph_ukit();
            break;
        case GR_PROJ:
            cv_oy = 0;
            graph_proj();
            break;
        default:
            if (big_page(pg)) {
                cv_oy = 0;
                if (pg->fam == FAM_MFX) {
                    graph_mix(pg);
                } else if (menu_page(pg)) {
                    graph_menu(t);
                } else if (fam_stage(pg->fam) >= 0) {        /* a sound page: what its knobs do, big (review 8) */
                    op_sound(t, pg);
                } else if (is_drum(t)) {
                    graph_big();
                } else {
                    op_synth_scene((track_t *)t, opk.ratio, pg->id[0] == P_E0 && t->eng_req % NENGINES == 0u);
                }
            }
            break;
        }
    }
    if (!ui.home && !drum_note && !is_drum(t) && !big_page(pg) && pg->graph != GR_BROWSE && pg->graph != GR_SLOTS &&
        pg->graph != GR_USER && pg->graph != GR_SPATT && pg->graph != GR_UKIT && pg->graph != GR_PROJ) {                      /* over a graph (they keep to the lower rows): the track's sound */
        cv_oy = 0;
        op_names(t);
    }
    top = !ui.home && !drum_note && (pg->graph == GR_BROWSE || pg->graph == GR_SLOTS || pg->graph == GR_USER || pg->graph == GR_SPATT || pg->graph == GR_UKIT ||
                                     pg->graph == GR_PROJ || big_page(pg));   /* these draw from the top */
    cv_oy = 0;
    if (ui.hot_t && settings.zoom && !(top && big_page(pg) && !drum_note)) {   /* (the big values show it already) */                 /* focus (menu ZOOM): the touched value, large and white */
        int32_t x;
        top = 1;
        cv_rect(0, 0, 150, 50, C_BLACK);
        cv_text(4, 0, &FONT_S, ui.focus_l, C_GRAY);
        x = cv_text(4, 16, &FONT_L, ui.focus_v, C_WHITE);
        cv_text(x + 4, 30, &FONT_S, ui.focus_u, C_DIM);
    }
    /* graphs keep out of the top G_OY rows: skip them unless something is (or was) there; on the sound chain the
     * bottom rows are the signal path's (draw_foot) */
    if (!ui.home)
        cv_h = 131 - Y_GRAPH;                           /* (wave 9: the tab row under it, ui_nav.c) */
    cv_blit_from(0, Y_GRAPH, top || ui.graph_top || ui.force ? 0u : G_OY);
    ui.graph_top = (uint8_t)top;
    key_strip_c = ~0u;                                  /* (wave 10: the chord strip over it, again) */
}

/* under the graph (wave 9): the tab row every screen has, the categories of the button's group and the page in it (ui_nav.c) */
static void nav_tabs(uint16_t col, const char *banner, uint32_t *cache);
/* (wave 10, Ben: black keys "Only live when drawn") the chord modifiers (seq.c CHORD+): drawn in the black-key strip on ARP's
 * KEY pages while a chord mode is on, and live only there (seq.c chord_mods_drawn); held: lit */
static int key_strip_on(void)
{
    const page_t *pg = cur_page();
    return !ui.home && !ui.menu && ui.layer == LY_PLAY && !vis_shown() && !rec_wait && !on_drum_page() && pg->cat == CT_KEY &&
           !is_drum(TSEL) && TSEL->p[P_CHORD];
}
static void draw_foot(void)
{
    const page_t *pg = cur_page();
    if (ui.home)
        return;
    nav_tabs(pg->scope == SC_GLOBAL ? TE_G4 : TE_COL[(is_drum(TSEL) ? TRK_DRUM : song.sel) & 3u], 0, &ui.foot_sig);
    if (key_strip_on()) {
        static const char *const CM[11] = {"3RD", "7TH", "SUS4", "9TH", "INV", "3RD", "7TH", "SUS4", "9TH", "INV", "3RD"};
        uint32_t i, on = 0;
        for (i = 0; i < 11u; i++)
            on |= ((fm1_in.notes >> PUNCH_BLACK[i]) & 1u) << i;
        bk_strip(CM, on, &key_strip_c);
    }
}
static void draw_columns_set(void);
static void draw_columns(void)                         /* the four knobs of the page, then their band */
{
    const char *lab[4], *val[4], *unit[4];
    uint32_t c;
    draw_columns_set();
    for (c = 0; c < 4u; c++) {
        lab[c] = opk.lab[c];
        val[c] = opk.val[c];
        unit[c] = opk.unit[c];
    }
    {                                                   /* a track's page: its colour; a song or global page: neutral */
        uint32_t sc = cur_page()->scope;
        knob_tint_all(drums_page(cur_page()) ? TRK_DRUM
                      : sc == SC_GLOBAL ? TINT_NEUTRAL : sc == SC_DRUM ? TRK_DRUM : song.sel);
    }
    {   /* (PHEBUS wave 4) the dials every page has now: one tall face in fixed columns (the drum and synth pages'); a shared
         * page's neutral, a voice's in its colour */
        static char fv[4][12];
        const char *fvp[4] = {fv[0], fv[1], fv[2], fv[3]};
        uint32_t sc = cur_page()->scope, tr = drums_page(cur_page()) || sc == SC_DRUM ? TRK_DRUM : song.sel;
        (void)unit;
        for (c = 0; c < 4u; c++) {
            str_cpy(fv[c], val[c], 12);
            dval_fit(fv[c]);
        }
        op_dials(lab, fvp, opk.ratio, 0, opk.dim, sc == SC_GLOBAL && !drums_page(cur_page()) ? TE_G4 : TE_COL[tr & 3u],
                 song.sel * 7u + ui.page * 31u + ui.home, &ui.col_sig);
    }
    {                                                   /* the track bar under them: it follows the track selected here */
        static uint32_t bar;
        op_trackbar(&bar);
    }
}
static void draw_columns_set(void)
{
    uint32_t c;
    char val[12];
    const char *unit;
    if (ui.home) {
        for (c = 0; c < 4u; c++) {
            int16_t *vp;
            const param_desc_t *d = home_param(c, &vp);
            param_format(d, *vp, val, &unit);
            draw_column(c, d->label, val, unit, VAL(c), RATIO(d, *vp), param_icon(d, *vp));
        }
        return;
    }
    if (is_drum(TSEL) && !page_for_drum(cur_page())) {   /* "DRUM TRACK" (the graph says so) */
        for (c = 0; c < 4u; c++)
            draw_column(c, "", "", "", C_HI, -1, ICON_AUTO);
        return;
    }
    if (cur_page()->scope == SC_TRK) {                 /* TRACK LEVEL LEN PAN of the selected track */
        const track_t *t = TSEL;
        uint32_t lvl = trk_level(song.sel);
        fmt_int(val, (int32_t)song.sel + 1);
        draw_column(0, W_VOICE, val, "/4", VAL(0u), (int32_t)song.sel * 1000 / (NTRK - 1), ICON_AUTO);
        if (!lvl || t->p[P_MUTE]) {                    /* (MUTE: a turn of KNOB 2 unmutes, tracks_edit) */
            str_cpy(val, "MUTE", 12);
            unit = "";
        } else if (is_drum(t)) {
            fmt_int(val, (int32_t)lvl);
            unit = "";
        } else {
            param_format(&TP[P_LEVEL], (int32_t)lvl, val, &unit);
        }
        draw_column(1, "LEVEL", val, unit, lvl && !t->p[P_MUTE] ? VAL(1u) : C_DIM, (int32_t)lvl * 1000 / 127, ICON_AUTO);
        param_format(&TP[P_SLEN], t->p[P_SLEN], val, &unit);
        draw_column(2, "STEPS", val, unit, VAL(2u), RATIO(&TP[P_SLEN], t->p[P_SLEN]), ICON_AUTO);
        param_format(&TP[P_PAN], t->p[P_PAN], val, &unit);
        draw_column(3, "PAN", val, unit, VAL(3u), RATIO(&TP[P_PAN], t->p[P_PAN]), param_icon(&TP[P_PAN], t->p[P_PAN]));
        return;
    }
    if (cur_page()->graph == GR_UKIT) {                  /* (wave 12) KIT, then three GO knobs */
        int used = ukit_has(ukit_sel);
        fmt_int(val, (int32_t)ukit_sel + 1);
        draw_column(0, "KIT", val, "", VAL(0u), (int32_t)ukit_sel * 1000 / (int32_t)(UKIT_N - 1u), ICON_AUTO);
        draw_column(1, "LOAD", "--", "", used ? C_HI : C_DIM, -1, ICON_AUTO);
        draw_column(2, "ERASE", "--", "", used ? C_HI : C_DIM, -1, ICON_AUTO);
        draw_column(3, "SAVE", "--", "", C_HI, -1, ICON_AUTO);
        return;
    }
    if (cur_page()->graph == GR_BROWSE && is_drum(TSEL)) {   /* (wave 12) the drums: the whole kit's place in the list */
        int32_t p = kit_changed() ? -1 : kit_pos(pkit.base);
        char u[8];
        if (p < 0)
            str_cpy(val, "--", 12);
        else
            fmt_int(val, p + 1);
        str_cpy(u, "/", 8);
        fmt_int(u + 1, (int32_t)kit_src_n());
        draw_column(0, "KIT", val, u, VAL(0u), -1, ICON_NONE);
        draw_column(1, "", "", "", C_HI, -1, ICON_AUTO);
        draw_column(2, "", "", "", C_HI, -1, ICON_AUTO);
        draw_column(3, "", "", "", C_HI, -1, ICON_AUTO);
        return;
    }
    if (cur_page()->graph == GR_BROWSE) {
        uint32_t total, cur = preset_pos(&total);
        char u[8];
        fmt_int(val, (int32_t)cur + 1);
        str_cpy(u, "/", 8);
        fmt_int(u + 1, (int32_t)total);
        draw_column(0, "No.", val, u, VAL(0u), -1, ICON_NONE);
        draw_column(1, "ENG", word_short(ENGINES[TSEL->eng_req]->name), "", VAL(1u), -1, engine_icon(ENGINES[TSEL->eng_req]->name));
        draw_column(2, "", "", "", C_HI, -1, ICON_AUTO);
        draw_column(3, "", "", "", C_HI, -1, ICON_AUTO);
        return;
    }
    if (cur_page()->graph == GR_PROJ) {                  /* (the bank) KEEP, SWAP, -, NEW: GO knobs */
        uint32_t np;
        int32_t bpm;
        char kit[9];
        int full = bank_summary(&np, &bpm, kit);
        draw_column(0, "KEEP", "--", "", C_HI, -1, ICON_AUTO);
        draw_column(1, "SWAP", "--", "", full ? C_HI : C_DIM, -1, ICON_AUTO);
        draw_column(2, "", "", "", C_HI, -1, ICON_AUTO);
        draw_column(3, "NEW", "--", "", C_HI, -1, ICON_AUTO);
        return;
    }
    if (cur_page()->graph == GR_SPATT) {                 /* (wave 11) PATT, then STORE (a GO) */
        val[0] = (char)('A' + spatt_sel % 4u), val[1] = 0;
        draw_column(0, "PATT", val, "", VAL(0u), (int32_t)spatt_sel * 333, ICON_AUTO);
        draw_column(1, "STORE", "--", "", C_HI, -1, ICON_AUTO);
        draw_column(2, "", "", "", C_HI, -1, ICON_AUTO);
        draw_column(3, "", "", "", C_HI, -1, ICON_AUTO);
        return;
    }
    if (cur_page()->graph == GR_USER) {                  /* SLOT, then three GO buttons */
        int used = up_used(ui.uslot);
        up_slot_label(val, ui.uslot);
        draw_column(0, "SLOT", val, "", VAL(0u), (int32_t)ui.uslot * 1000 / (int32_t)(UP_SLOTS - 1u), ICON_AUTO);
        draw_column(1, "LOAD", "--", "", used ? C_HI : C_DIM, -1, ICON_AUTO);
        draw_column(2, "ERASE", "--", "", used ? C_HI : C_DIM, -1, ICON_AUTO);
        draw_column(3, "SAVE", "--", "", C_HI, -1, ICON_AUTO);
        return;
    }
    for (c = 0; c < 4u; c++) {
        int16_t *vp;
        const param_desc_t *d = page_desc(cur_page(), c, &vp);
        if (cur_page()->graph == GR_SLOTS && c == 1u) {   /* 2.6: ERASE, lit while the slot holds a project */
            draw_column(1, "ERASE", "--", "", project_used((uint32_t)song.g[G_SLOT] - 1u) ? C_HI : C_DIM, -1, ICON_AUTO);
            continue;
        }
        if (!d || !d->label || d->label[0] == '-') {
            draw_column(c, "", "", "", C_HI, -1, ICON_AUTO);
            continue;
        }
        if (cur_page()->id[c] == G_INFO && cur_page()->scope == SC_GLOBAL && !usb.config) {   /* not connected: the
                                                       * USB status instead of the CPU (2.4: MIDI is MIDI OUT, ROUT is IN) */
            str_cpy(val, !usb.up ? "OFF" : usb.config ? "MIDI" : usb.setups ? "ENUM" : usb.sof_seen ? "BUS" : "WAIT", 12);
            unit = "USB";
            draw_column(c, "USB", val, unit, C_HI, -1, ICON_AUTO);
            continue;
        }
        if (cur_page()->id[c] == V_LCLEAR) {            /* (review U5) GLO > MIDI 3 > CLEAR: the CCs learned, a turn right
                                                       * clears them (it asks) */
            uint32_t nl = ml_count();
            if (nl) {
                fmt_int(val, (int32_t)nl);
                str_cpy(val + str_len(val), " CC", 4);
            }
            draw_column(c, d->label, nl ? val : "NONE", "", nl ? VAL(c) : C_DIM, -1, ICON_AUTO);
            continue;
        }
        if (cur_page()->id[c] == G_INFO && cur_page()->scope == SC_GLOBAL) {
            fmt_int(val, (int32_t)(song.cpu_q8 * 100u / 256u));
            unit = "%";
        } else {
            param_format(d, *vp, val, &unit);
        }
        draw_column(c, d->label, val, unit, dev_dim(cur_page()->id[c]) ? C_DIM : VAL(c), d->fmt == F_ENUM && d->max < 2 ? -1 : RATIO(d, *vp),
                    param_icon(d, *vp));
    }
}


/* the UI's timers, once a frame whatever the screen (a message, the BPM highlight, an armed GO, the
 * white value of the knob just turned) */
static void ui_timers(void)
{
    if (ui.ask) {                                       /* (wave 8) a question stays up while its screen does; another
                                                         * screen or another message: it goes (ui_input.c ask_set) */
        if (!ui.msg_t || memcmp(ui.msg, "HOLD SAVE: ", 11))
            ui.ask = 0;
        else
            ui.msg_t = 40;
    }
    if (ui.msg_t)
        ui.msg_t--;
    if (ui.bpm_t)
        ui.bpm_t--;
    if (ui.arm_t && !--ui.arm_t)
        ui.arm = 0;
    if (ui.hot_t)
        ui.hot_t--;
    if (sel_t)
        sel_t--;
    if (master_t)
        master_t--;
}

static void vis_draw(void);                     /* ui_vis.c */
static int boot_draw(void);                     /* ui_vis.c: the boot animation */
static int vis_shown(void);
static uint8_t vis_on, vis_shown_last, vis_name_t;   /* the visualiser (ui_vis.c): on, drawn last frame, its name's time */
static void ui_draw(void)
{
    ui.frame++;
    if (boot_draw())                                    /* (KARMA 3.0: the boot animation first) */
        return;
    pads_tick();
    knob_tint_all(song.sel & 3u);                       /* (wave 7) the graphs' tints are the selected voice's unless a
                                                         * screen says otherwise (HOME, the layers and the lane no longer
                                                         * set them: a stale per-voice tint drew coral on voice 1's OSC) */
    {   /* a message belongs to the screen it showed on: another screen, it goes (review 21) */
        uint16_t k = (uint16_t)((ui.menu * 1000003u + (ui.ask ? 0u : ui.layer) * 7919u + rec_wait * 17u +   /* (wave 8: not
                                                         * the hold's ring: the question it answers stays with its screen;
                                                         * wave 10: nor the layer that asked letting go, so SAVE can be held
                                                         * after EDIT or SEQ) */
                                 ui.home * 3u + ui.page * 977u + drum_keys * 5u + vis_on * 11u) % 65521u);
        if (!ui.msg_t)
            ;
        else if (msg_scr == 0xFFFFu)
            msg_scr = k;
        else if (msg_scr != k)
            ui.msg_t = 0;
    }
    if (rec_go) {                                       /* the take started: say so */
        rec_go = 0;
        ui_message("RECORDING");
    }
    if (er_flash) {                                     /* a key + OCT- took something out (wave 10) */
        er_flash = 0;
        if (!ui.msg_t)
            ui_message("ERASED");
    }
    if (!ui.menu && (ui.layer != LY_PLAY || ui.hold_kind)) {   /* a layer held / a hold to confirm */
        if (ui.hold_kind)
            hold_screen_draw();
        else
            layer_screen_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
    if (layer_shown) {                                  /* back to the page */
        layer_shown = 0;
        lcd_fill(0, 0, 240, 240, C_BLACK);
        ui.force = 1;
    }
    if (rec_wait && !ui.menu) {                         /* armed */
        rec_screen_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
    if (rec_shown) {                                    /* back to the page */
        rec_shown = 0;
        lcd_fill(0, 0, 240, 240, C_BLACK);
        ui.force = 1;
    }
    if (ui.menu) {
        draw_about();
        ui_timers();
        ui.force = 0;
        return;
    }
    if (vis_shown()) {                                  /* the visualiser over the TRACKS screen */
        vis_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
    if (vis_shown_last) {                               /* back from it */
        vis_shown_last = 0;
        vis_on = 0;
        lcd_fill(0, 0, 240, 240, C_BLACK);
        ui.force = 1;
    }
    if (!ui.home && cur_page()->scope == SC_TRK) {
        studio_tracks_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
    if (on_drum_page()) {
        drum_screen_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
    cursor_fix();
    if (on_chain() && !is_drum(TSEL)) {                 /* (wave 4) a synth's sound page, the MiniFreak's way (ui_synth.c) */
        syn_page_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
    if (ui.force)
        draw_frame();
    felucca_dbg.stage = 3;
    draw_head();
    felucca_dbg.stage = 4;
    draw_columns();
    felucca_dbg.stage = 5;
    draw_graph();
    ui_timers();
    felucca_dbg.stage = 6;
    draw_foot();
    ui.force = 0;
}
