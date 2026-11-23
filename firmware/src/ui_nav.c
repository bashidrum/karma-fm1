/* SPDX-License-Identifier: GPL-3.0-only */
/* HOW TO MOVE AROUND (PHEBUS wave 9). Ben, 23 Nov 2026, 08:25 and 08:42: "Press a labelled button and obviously associated
 * features pop up on screen with knobs 1-4 controlling parameters"; "Hit the edit key, which brings Osc and Filter categories.
 * And if the Osc category has 4 pages, you cycle through those using the Sel ... if you press edit again it moves to the
 * next category of Filter"; picker "PAGE button: SEL". Included by ui_input.c. Two actions, any button can be bound to either:
 *
 *   nav_button(b)  the labelled button b: its group's first CATEGORY; on that group already, its next category (round). It
 *                  never walks pages. The same on every screen and with a step held (the Rytm's page keys)
 *   nav_page()     SEL (PAGE): the next page inside the category shown (round); a category of one page: nothing
 *
 * A category lives where its voice's pages live: a synth voice's on its sound pages (the keys play notes) or, on its GRID,
 * as the GRID's view (the keys stay steps); the drum voice's on the drum screen; the shared ones (GLO, SAVE, ARP's KEY) on
 * their pages. The tab row (nav_tabs) names the group's categories, the one shown filled, and "2/4" the page in it */
static const uint8_t NAV_SYN[NB][5] = {
    [B_EDIT] = {CT_OSC, CT_FILT},
    [B_ENV] = {CT_AMP, CT_FENV},
    [B_LFO] = {CT_LFO1, CT_LFO2, CT_MATRIX},
    [B_FX] = {CT_INS, CT_SENDS, CT_SLICER, CT_MIX},     /* (Ben: "Voice FX, then mix FX") */
    [B_SEQ] = {CT_TRIG, CT_PATT, CT_PATTS},             /* (wave 10, Ben: "in charge of patterns") */
    [B_ARP] = {CT_ARP, CT_KEY},                         /* (Ben: key, scale and chord "Under ARP") */
    [B_GLO] = {CT_GLOBAL, CT_SYSTEM, CT_MIDI, CT_DEVICE},
    [B_SAVE] = {CT_PRESETS, CT_USER, CT_TOOLS},
    [B_HOME] = {CT_HOME, CT_SAVER},
};
static const uint8_t NAV_DRUM[NB][6] = {               /* the drum voice, parallel (the Rytm's SRC FLTR AMP LFO, its FX track) */
    [B_EDIT] = {CT_SRC, CT_FLTR},
    [B_ENV] = {CT_DAMP},
    [B_LFO] = {CT_DLFO},
    [B_FX] = {CT_DFILT, CT_DINS, CT_DSENDS, CT_DSLICER, CT_MIX},
    [B_SEQ] = {CT_TRIG, CT_PATT, CT_PATTS},             /* (wave 10, Ben: "in charge of patterns") */
    [B_ARP] = {CT_CHROM},
    [B_GLO] = {CT_GLOBAL, CT_SYSTEM, CT_MIDI, CT_DEVICE},
    [B_SAVE] = {CT_PRESETS, CT_USER, CT_TOOLS},
    [B_HOME] = {CT_HOME, CT_SAVER},
};
static const char *const CT_NAME[CT_N] = {
    [CT_OSC] = "OSC", [CT_FILT] = "FILT", [CT_AMP] = "AMP", [CT_FENV] = "FILT", [CT_LFO1] = "LFO 1", [CT_LFO2] = "LFO 2",
    [CT_MATRIX] = "MATRIX", [CT_INS] = "INS", [CT_SENDS] = "SENDS", [CT_SLICER] = "SLICER", [CT_MIX] = "MIX",
    [CT_DFILT] = "FILTER", [CT_DINS] = "INS", [CT_DSENDS] = "SENDS", [CT_DSLICER] = "SLICER", [CT_ARP] = "ARP",
    [CT_KEY] = "KEY", [CT_GLOBAL] = "GLOBAL", [CT_SYSTEM] = "SYSTEM", [CT_MIDI] = "MIDI", [CT_PRESETS] = "PRESETS",
    [CT_USER] = "USER", [CT_TOOLS] = "TOOLS", [CT_HOME] = "HOME", [CT_TRIG] = "TRIG", [CT_PATT] = "PATTERN", [CT_PATTS] = "A-D", [CT_SRC] = "SRC",
    [CT_FLTR] = "FLTR", [CT_DAMP] = "AMP", [CT_DLFO] = "LFO", [CT_CHROM] = "CHROM", [CT_DEVICE] = "DEVICE",
    [CT_SAVER] = "SAVER",
};
/* the drum screen's FX tab: where each category starts among its halves (ui_drum.c dbus_page: FILTER, INSERT, INSERT 2,
 * SENDS, SLICER, then the whole mix's DELAY, REV/CHO, MASTER) */
static uint32_t dfx_first(uint32_t c) { return c == CT_DFILT ? 0u : c == CT_DINS ? 1u : c == CT_DSENDS ? 3u : c == CT_DSLICER ? 4u : 5u; }
static uint32_t dfx_cat(uint32_t h) { return h < 1u ? CT_DFILT : h < 3u ? CT_DINS : h < 4u ? CT_DSENDS : h < 5u ? CT_DSLICER : CT_MIX; }
static int nav_grid(void) { return on_drum_page() && !stl_shown(); }   /* the GRID or the drum screen shown */
static int nav_voice_cat(uint32_t c) { return c >= CT_OSC && c <= CT_MIX; }   /* a synth voice's (and the mix's) PAGES */

static const uint8_t *nav_group(uint32_t b, uint32_t *n)
{
    const uint8_t *g = is_drum(TSEL) ? NAV_DRUM[b % NB] : NAV_SYN[b % NB];
    uint32_t m = is_drum(TSEL) ? 6u : 5u;
    for (*n = 0; *n < m && g[*n]; (*n)++)
        ;
    return g;
}
/* the PAGES of category c the selected voice has, in order; idx may be 0 (count only) */
static uint32_t nav_list(uint32_t c, uint8_t *idx)
{
    uint32_t i, n = 0;
    for (i = 0; i < NPAGES; i++)
        if (PAGES[i].cat == c && page_shown(&PAGES[i])) {
            if (idx)
                idx[n] = (uint8_t)i;
            n++;
        }
    return n;
}
/* the category shown, *k its page shown, *n how many pages it has */
static uint32_t nav_cat(uint32_t *k, uint32_t *n)
{
    uint32_t c = CT_NONE, i;
    uint8_t idx[8];
    *k = 0;
    *n = 1;
    if (ui.menu) {
        *k = mi_sec(ui.menu_sel % MI_COUNT);
        *n = MS_COUNT;
        return CT_DEVICE;
    }
    if (vis_shown()) {
        *k = vis_style % VIS_N;
        *n = VIS_N;
        return CT_SAVER;
    }
    if (ui.home || cur_page()->scope == SC_TRK)
        return CT_HOME;
    if (on_drum_page()) {
        if (stl_shown())
            return CT_NONE;
        if (!is_drum(TSEL)) {                           /* the synth GRID */
            if (!syn_tab) {
                if (syn_half[0] >= TH_AD)
                    return CT_PATTS;
                if (syn_half[0] >= TH_PATT) {
                    *k = syn_half[0] - TH_PATT;
                    *n = 2;
                    return CT_PATT;
                }
                *k = syn_half[0];
                *n = 2;
                return CT_TRIG;
            }
            c = PAGES[syn_pg % NPAGES].cat;
        } else {                                        /* the drum screen */
            if (drum_keys == DK_CHROM)
                return CT_CHROM;
            if (!drum_pg) {
                if (dpg_half() >= TH_AD)
                    return CT_PATTS;
                if (dpg_half() >= TH_PATT) {
                    *k = dpg_half() - TH_PATT;
                    *n = 2;
                    return CT_PATT;
                }
                *k = dpg_half();
                *n = 2;
                return CT_TRIG;
            }
            if (drum_pg < DPG_FX) {
                uint32_t f;
                *k = dpg_half();
                *n = dh_count(drum_model(), drum_pg - 1u, &f);
                return CT_SRC + drum_pg - 1u;
            }
            c = dfx_cat(dpg_half());
            *k = dpg_half() - dfx_first(c);
            *n = c == CT_DINS ? 2u : c == CT_MIX ? 3u : 1u;
            return c;
        }
    } else {
        c = cur_page()->cat;
    }
    *n = nav_list(c, idx);
    for (i = 0; i < *n; i++)
        if (idx[i] == (on_drum_page() ? syn_pg : ui.page))
            *k = i;
    if (!*n)
        *n = 1;
    return c;
}

/* category c, its k-th page (round), where the voice's pages are */
static void nav_go(uint32_t c, uint32_t k)
{
    uint8_t idx[8];
    uint32_t n, drum = is_drum(TSEL), grid = nav_grid();
    if (ui.menu && c != CT_DEVICE)
        menu_close();
    if (c != CT_SAVER)
        vis_on = 0;
    if (drum && c != CT_CHROM && drum_keys == DK_CHROM)
        drum_keys = DK_PLAY;                            /* (another category: the keys are steps again) */
    switch (c) {
    case CT_TRIG:
    case CT_PATT:
    case CT_PATTS:
        if (!on_drum_page() || stl_shown()) {
            studio_open(SC_DRUM);
            if (stl_view)
                stl_held = stl_pend = stl_fresh = 0;
            stl_view = 0;
        }
        if (drum)
            dpg_go(0u, c == CT_PATTS ? TH_AD : c == CT_PATT ? TH_PATT + k % 2u : k % 2u);
        else
            syn_go(0u, c == CT_PATTS ? TH_AD : c == CT_PATT ? TH_PATT + k % 2u : k % 2u);
        ui.msg_t = 0;
        return;
    case CT_SRC:
    case CT_FLTR:
    case CT_DAMP:
    case CT_DLFO: {
        uint32_t f, m = dh_count(drum_model(), c - CT_SRC, &f);
        drum_screen_open();
        dpg_go(c - CT_SRC + 1u, m ? k % m : 0u);
        return;
    }
    case CT_CHROM:
        drum_screen_open();
        drum_keys = DK_CHROM;
        if (!drum_pg)
            dpg_go(drum_pg_last, drum_half[drum_pg_last % DPG_TABS]);
        ui.force = 1;
        return;
    case CT_DEVICE:
        if (!ui.menu) {
            layer_unlock();
            ui.menu = 1;
            ui.confirm = 0;
            song.seq_mode = 0;
        }
        ui.menu_sel = MS_FIRST[k % MS_COUNT];
        ui.force = 1;
        return;
    case CT_SAVER:
        go_home();
        vis_style = (uint8_t)(k % VIS_N);
        vis_open();
        return;
    case CT_HOME:
        go_home();
        return;
    default:
        break;
    }
    if (drum && (c == CT_MIX || (c >= CT_DFILT && c <= CT_DSLICER))) {   /* the drum voice's FX group: its drum screen's FX tab */
        uint32_t m = c == CT_DINS ? 2u : c == CT_MIX ? 3u : 1u;
        drum_screen_open();
        dpg_go(DPG_FX, dfx_first(c) + k % m);
        return;
    }
    n = nav_list(c, idx);
    if (!n) {
        ui_message("NOT ON THIS VOICE");
        return;
    }
    if (grid && !drum && nav_voice_cat(c) && sg_held)   /* (wave 10, Ben 10:33: "the sequence stays on screen even when you go
                                                         * to the edit page") on the GRID only with a step held: that page's
                                                         * knobs under the steps, a lock view; nothing held: its real page */
        syn_view(idx[k % n]);
    else
        page_go(idx[k % n]);
}

/* (Ben's way 1) a labelled button: its group's first category; on it already, the next one, round */
static void nav_button(uint32_t b)
{
    uint32_t k, n, ng, c = nav_cat(&k, &n), i;
    const uint8_t *g = nav_group(b, &ng);
    if (!ng)
        return;
    if (is_drum(TSEL) && b == B_ARP && c == CT_CHROM) { /* (the drum voice's ARP: CHROMATIC; again: the keys are steps again) */
        drum_keys = DK_PLAY;
        ui.force = 1;
        return;
    }
    for (i = 0; i < ng && g[i] != c; i++)
        ;
    nav_go(i < ng ? g[(i + 1u) % ng] : g[0], 0);
}
/* (wave 11) button b pressed now would show another page (the lights: a page button with nothing to move to stays dark) */
static int nav_moves(uint32_t b)
{
    uint32_t k, n, ng, c = nav_cat(&k, &n), i;
    const uint8_t *g = nav_group(b, &ng);
    if (!ng)
        return 0;
    for (i = 0; i < ng && g[i] != c; i++)
        ;
    return (i < ng ? g[(i + 1u) % ng] : g[0]) != c || k != 0u;
}
static int nav_page_moves(void)
{
    uint32_t k, n, c = nav_cat(&k, &n);
    return c != CT_NONE && n >= 2u;
}
/* (Ben, 08:42) SEL, the PAGE button: the next page of the category shown, round; one page: nothing */
static void nav_page(void)
{
    uint32_t k, n, c = nav_cat(&k, &n);
    if (c == CT_NONE || n < 2u)
        return;
    nav_go(c, (k + 1u) % n);
}

/* the group the category shown belongs to (the button whose presses step through it); NB: none */
static uint32_t nav_btn_of(uint32_t c)
{
    uint32_t b, i, n;
    for (b = 0; b < NB; b++) {
        const uint8_t *g = nav_group(b, &n);
        for (i = 0; i < n; i++)
            if (g[i] == c)
                return b;
    }
    return NB;
}
static uint32_t nav_tabs_shown;                         /* (the host tests: the categories last drawn, CT_* a byte) */
static uint8_t nav_tabs_fit = 1;                        /* (the host tests: the names ended before the page and the legend) */
static const char *pm_shown_m, *pm_shown_p;             /* (the host tests: the -/+ legend last drawn on a page screen) */
static void pm_words(const char **m, const char **p);   /* (ui_input.c) */
/* the tab row (y 131, 23 px): the group's categories, the one shown filled in col (the whole mix's: white, after a divider:
 * Ben, "Voice FX, then mix FX"), "2/4": the page in the category (SEL's), and (wave 10) at the right end the -/+ legend: what
 * a tap of OCT- and OCT+ does here. banner: STEP REC's line instead of the categories. When the names do not fit beside the
 * legend, the gap narrows, then the names not shown are cut to their first letters (four, then three): the one shown stays
 * whole */
static void nav_tabs(uint16_t col, const char *banner, uint32_t *cache)
{
    uint32_t k, n, c = nav_cat(&k, &n), b = nav_btn_of(c), ng = 0, i, v = song.sel & 3u;
    uint32_t sig = c * 7u + k * 131u + n * 977u + col * 31u + v * 7919u;
    const uint8_t *g = b < NB ? nav_group(b, &ng) : 0;
    const char *pm, *pp;
    char nm[6][8];
    int32_t x = 4, gap = 9, xr = 236;
    pm_words(&pm, &pp);
    pm_shown_m = pm, pm_shown_p = pp;
    sig = studio_hash(studio_hash(studio_hash(sig, banner ? banner : ""), pm ? pm : "-"), pp ? pp : "+");
    sig = studio_hash(sig * 3u + (ly_lock != LY_PLAY), nav_map_hint ? nav_map_hint : "");
    if (!ui.force && sig == *cache)
        return;
    *cache = sig;
    nav_tabs_shown = 0;
    cv_begin(240, 23, C_BLACK);
    cv_rect(0, 0, 240, 1, TE_G1);
    if (pm || pp)
        xr = pm_draw(236, 5, pm, pp) - 6;
    if (banner && nav_map_hint) {                       /* (wave 11) MAP: outlined in the voice's colour (a LOCK banner is
                                                         * filled), then what SEL flips to, grey; LOCK while kept open */
        int32_t x = 2 + text_w(&FONT_S, banner) + 6;
        op_box(2, 4, x - 2, 17, col);
        cv_text(5, 5, &FONT_S, banner, col);
        x = cv_text(x + 6, 5, &FONT_S, "SEL", TE_G4);
        x = cv_text(x + 4, 5, &FONT_S, nav_map_hint, TE_G3);
        if (ly_lock != LY_PLAY && x + 8 + text_w(&FONT_S, "LOCK") <= xr) {
            cv_rect(xr - text_w(&FONT_S, "LOCK") - 4, 4, text_w(&FONT_S, "LOCK") + 4, 17, C_WHITE);
            cv_text(xr - text_w(&FONT_S, "LOCK") - 2, 5, &FONT_S, "LOCK", C_BLACK);
        }
        cv_blit(0, 131);
        return;
    }
    if (banner) {
        cv_rect(2, 4, text_w(&FONT_S, banner) + 4, 17, col);
        cv_text(4, 5, &FONT_S, banner, C_BLACK);
        cv_blit(0, 131);
        return;
    }
    if (n > 1u) {
        char p[6];
        fmt_int(p, (int32_t)k + 1);
        str_cpy(p + str_len(p), "/", 2);
        fmt_int(p + str_len(p), (int32_t)n);
        xr -= (int32_t)text_w(&FONT_S, p);
        cv_text(xr, 5, &FONT_S, p, c == CT_MIX ? C_WHITE : col == TE_G4 ? TE_G4 : col);
        xr -= 6;
    }
    if (ng > 6u)
        ng = 6u;
    for (i = 0; i < ng; i++)
        str_cpy(nm[i], CT_NAME[g[i]] ? CT_NAME[g[i]] : "", sizeof nm[i]);
    {   /* the gap between names: 9 px, less when the row is long; then the names not shown shortened */
        int32_t tw, room = xr - 4, cut = 5;
        for (;;) {
            tw = 0;
            for (i = 0; i < ng; i++)
                tw += text_w(&FONT_S, nm[i]) + (g[i] == CT_MIX && i ? 5 : 0);
            for (gap = 9; gap > 3 && tw + gap * (int32_t)(ng ? ng - 1u : 0u) > room; gap--)
                ;
            if (tw + gap * (int32_t)(ng ? ng - 1u : 0u) <= room || cut < 3)
                break;
            for (i = 0; i < ng; i++)
                if (g[i] != c && str_len(nm[i]) > (uint32_t)cut)
                    nm[i][cut] = 0;
            cut--;
        }
    }
    for (i = 0; i < ng; i++) {
        int32_t w = text_w(&FONT_S, nm[i]);
        int mix = g[i] == CT_MIX;
        uint16_t cc = mix ? C_WHITE : col;
        if (i < 4u)
            nav_tabs_shown |= (uint32_t)g[i] << (8u * i);
        if (mix && i) {                                 /* the boundary: this voice | the whole mix */
            cv_rect(x - 2, 3, 2, 19, TE_G4);
            x += 5;
        }
        if (g[i] == c) {
            cv_rect(x - 2, 4, w + 4, 17, cc);
            cv_text(x, 5, &FONT_S, nm[i], C_BLACK);
        } else {
            cv_text(x, 5, &FONT_S, nm[i], mix ? TE_G4 : col == TE_G4 ? TE_G3 : TE_MID[v]);
        }
        x += w + gap;
    }
    nav_tabs_fit = (uint8_t)(x - gap <= xr + 2);
    cv_blit(0, 131);
}
