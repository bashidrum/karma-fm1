/* SPDX-License-Identifier: GPL-3.0-only */
/* HOLD TO MAP (PHEBUS wave 11). Ben, 23 Nov 2026, 10:41: "holding the LFO button and then it jumps back into the page you were
 * just on, gives you the same controls and you simply wiggle what you want mapped to the lfo. same for the ENV button? ...
 * Would be awesome if the same logic were applied to all the nice and obviously pleasantly mappable parameters"; his picker
 * answers at 10:50: which LFO / ENV "The one last shown" (its name shows while you hold, SEL flips to the other); velocity
 * and key "Matrix page only". Included by ui_synth.c.
 *
 *   On a synth voice's sound page (OSC FILT ENV FX LFO, the MATRIX too), HOLD LFO or ENV: the page stays as it is (a tap acts
 *   on its release, a hold never moves to another page); the tab row says MAP LFO 1 (or LFO 2, AMP ENV, FILT ENV) and what SEL
 *   flips to; the four dials are that source's amounts onto their knobs, bipolar (right +, left -, back to 0: gone). Let go:
 *   the page. Each amount is a cell of the mod matrix (modmx.c mx_map_set: one source of truth, the MATRIX page shows it).
 *   SEL: the other LFO / envelope. OCT-: every mapping from this source onto this page's knobs removed (SAVE + OCT- brings
 *   them back). MASTER: its pair as in wave 10 (LFO: MOD DEPTH, ENV: ENV LENGTH), the chip in the header.
 *   A knob that cannot be mapped (modmx.c mx_knob_col: a switch, a modulation amount, an envelope onto its own stages): its
 *   dial greyed, "--", and a turn says so. Outside the hold a knob with something mapped to it has a mark by its label.
 *   Not on HOME, the GRID (SEQ), the drum voice, the ARP, KEY, GLO, SAVE or MIX pages: there LFO and ENV held are MASTER's
 *   pair only, as wave 10 built them. With a step held: the step's (a page button there opens that page's locks): no map */
static uint8_t map_lfo2, map_fenv;                      /* the LFO / envelope last shown (the page seen): 0 LFO 1 / AMP */
static uint8_t map_lyr;                                 /* LY_LFO or LY_ENV while mapping (ui_input.c), else 0 */
static uint8_t map_vis;                                 /* the MAP view shows (a hold, not a tap: a knob, SEL, or SHOW_MS) */
static const char *nav_map_hint;                        /* (ui_nav.c nav_tabs) the banner is a MAP one: outlined, then "SEL" and
                                                         * this (what SEL flips to) */
static uint32_t map_src_of(uint32_t lyr) { return lyr == LY_LFO ? (map_lfo2 ? MS_LFO2 : MS_LFO1) : (map_fenv ? MS_FENV : MS_AENV); }
static uint32_t map_src(void) { return map_src_of(map_lyr); }
static const char *map_src_name(uint32_t s)
{
    static const char *const N[MS_N] = {"LFO 1", "LFO 2", "AMP ENV", "FILT ENV", "", "", ""};
    return N[s % MS_N];
}
static const char *map_other_name(uint32_t s)           /* what SEL flips to, short */
{
    return s == MS_LFO1 ? "LFO 2" : s == MS_LFO2 ? "LFO 1" : s == MS_AENV ? "FILT" : "AMP";
}
static void map_chip(uint32_t l);
static int vis_shown(void);                             /* (ui_vis.c) */                       /* (ui_layers.c: MASTER's chip in the header, ui_master.c's pair) */
/* the screen can map: a synth voice's sound page, nothing else on top */
static int map_page_ok(void)
{
    return !ui.home && !ui.menu && !vis_shown() && !rec_wait && !is_drum(TSEL) && on_chain() && !kb_grid && !sg_held &&
           !sr_on && !ui.ask;
}
/* (wave 12) the drums: LFO held on a drum sound page (SRC FLTR AMP LFO), no step held, the keys steps (not CHROMATIC) */
static int map_drum_ok(void)
{
    return !ui.home && !ui.menu && !vis_shown() && !rec_wait && is_drum(TSEL) && on_drum_page() && !stl_shown() &&
           dpg_snd() && drum_keys == DK_PLAY && !sg_held && !ui.ask;   /* (the drum effects under FX: its SENDS too) */
}
static int map_ok_for(uint32_t held) { return held == LY_LFO ? map_page_ok() || map_drum_ok() : held == LY_ENV && map_page_ok(); }
/* the page seen: which LFO / envelope is "the one last shown" (every frame, ui_input.c) */
static void map_seen(void)
{
    uint32_t c;
    if (ui.home || ui.menu || is_drum(TSEL) || map_lyr)  /* (while held: SEL's flip stands) */
        return;
    c = cur_page()->cat;
    if (c == CT_LFO1 || c == CT_LFO2)
        map_lfo2 = (uint8_t)(c == CT_LFO2);
    else if (c == CT_AMP || c == CT_FENV)
        map_fenv = (uint8_t)(c == CT_FENV);
}
/* knob k of page pg while mapping source s: 0 an empty knob, 1 not mappable, 2 mappable (*amt its amount) */
static uint32_t map_knob(const page_t *pg, uint32_t k, uint32_t s, int32_t *amt, uint32_t *idp)
{
    int16_t *vp;
    uint32_t id = pg_id(pg, k);
    const param_desc_t *d = page_desc(pg, k, &vp);
    *amt = 0;
    *idp = id;
    if (!d || !d->label || d->label[0] == '-')
        return 0;
    if (id >= P_COUNT || mx_self(s, id) || mx_knob_col(TSEL, id) == -1)
        return 1;
    *amt = mx_map_get(song.sel % NPART, s, id);
    return 2;
}
/* a knob turned while mapping: the source's amount onto it (-64..63; 0 removes) */
static void map_turn(uint32_t k, int32_t steps)
{
    if (is_drum(TSEL)) {                                /* (wave 12) the drums: the slot's LFO */
        dmap_turn(k, steps);
        return;
    }
    const page_t *pg = cur_page();
    uint32_t s = map_src(), id, st;
    int32_t a, e;
    char b[32];
    st = map_knob(pg, k, s, &a, &id);
    if (st == 0u)
        return;
    if (st == 1u) {                                     /* refused, visibly: the dial is grey and it says so */
        int16_t *vp;
        const param_desc_t *d = page_desc(pg, k, &vp);
        te_upper(b, id == V_TYPE ? "TYPE" : d->label, 8);
        str_cpy(b + str_len(b), mx_self(s, id) ? ": NOT FROM ITSELF" : ": NOT MAPPABLE", 20);
        ui_message(b);
        return;
    }
    a = clamp(a + accel(EN_K1 + k, steps, 127), -64, 63);
    fm1_irq_off();
    e = mx_map_set(song.sel % NPART, s, id, a);
    fm1_irq_on();
    if (e == -1)
        ui_message(MX_FULL_MSG);
    else if (e == -2)
        ui_message(MX_NAS_MSG);
}
/* the voice's matrix before a removal, for SAVE + OCT- (the undo's room, as the STUTTER lane uses it: no RAM of its own) */
static void mx_undo_keep(uint32_t ti)
{
    uint32_t i;
    int16_t *al = (int16_t *)((uint8_t *)undo.st + 64);
    _Static_assert(sizeof mxt[0].c + sizeof mxt[0].as <= 64u && 64u + 2u * NELEM(MX_ALIAS) <= sizeof undo.st,
                   "a matrix (and its fixed cells' parameters) fits the undo's room");
    memcpy(undo.st, mxt[ti % NPART].c, sizeof mxt[0].c);
    memcpy((uint8_t *)undo.st + sizeof mxt[0].c, mxt[ti % NPART].as, sizeof mxt[0].as);
    for (i = 0; i < NELEM(MX_ALIAS); i++)               /* (the fixed cells are parameters: theirs too) */
        al[i] = trk[ti % NPART].p[MX_ALIAS[i][2]];
    undo.trk = (uint8_t)(NTRK + 1u + ti % NPART);
    undo.sess = 0;
    undo.valid = 1;
    undo.undone = 0;
}
/* OCT- while mapping: every mapping from the source onto this page's knobs, gone (one undo: SAVE + OCT-) */
static void map_remove(void)
{
    if (is_drum(TSEL)) {
        dmap_remove();
        return;
    }
    const page_t *pg = cur_page();
    uint32_t ids[4], k, n, s = map_src();
    int32_t a;
    char b[32];
    for (k = 0, n = 0; k < 4u; k++) {
        if (map_knob(pg, k, s, &a, &ids[k]) != 2u)
            ids[k] = P_COUNT;
        n += ids[k] < P_COUNT && a;
    }
    if (!n) {                                           /* (nothing: the undo before stays) */
        str_cpy(b, "NOTHING FROM ", sizeof b);
        str_cpy(b + str_len(b), map_src_name(s), 10);
        ui_message(b);
        return;
    }
    mx_undo_keep(song.sel % NPART);
    fm1_irq_off();
    n = mx_map_clear(song.sel % NPART, s, ids, 4);
    fm1_irq_on();
    str_cpy(b, map_src_name(s), sizeof b);
    str_cpy(b + str_len(b), n == 1u ? ": 1 MAPPING OFF" : ": MAPPINGS OFF", 16);
    ui_message(b);
}

/* ---- drawing */
/* a bipolar ring: the track dim, from the top an arc to the amount (right +, left -) in c, a tick at the top */
static void map_ring(int32_t cx, int32_t cy, int32_t r, int32_t amt, uint16_t c, uint16_t dim)
{
    int32_t i, end = 384 + amt * 384 / 64, lo = end < 384 ? end : 384, hi = end < 384 ? 384 : end;
    for (i = 0; i <= 768; i += 8) {
        uint32_t a = (uint32_t)(384 + i) & 1023u;
        int32_t co = SINE[(a + 256u) & 1023u], si = SINE[a], k;
        uint16_t col = amt && i >= lo && i <= hi ? c : dim;
        for (k = r - 2; k <= r; k++)
            cv_pset(cx + ((co * k) >> 15), cy + ((si * k) >> 15), col);
    }
    cv_rect(cx, cy - r - 2, 1, 3, C_WHITE);              /* (0: the top) */
}
/* the four dials while mapping (the pages' columns, ui_drum.c op_dials): an amount set in the voice's colour in an outline
 * box (a lock is inverted; a map is boxed), 0 grey, a knob that cannot be mapped dark grey "--" */
static void map_dials(const page_t *pg, uint32_t s, uint16_t col, uint32_t *cache)
{
    uint32_t k, st[4], id, sig = 0x6d6170u + s * 7919u + col;
    int32_t amt[4];
    const char *lab[4];
    char v[4][8];
    for (k = 0; k < 4u; k++) {
        int16_t *vp;
        const param_desc_t *d;
        st[k] = map_knob(pg, k, s, &amt[k], &id);
        d = page_desc(pg, k, &vp);
        lab[k] = st[k] && d ? (id == V_TYPE ? "TYPE" : d->label) : "";
        if (st[k] == 2u && amt[k])
            v[k][0] = amt[k] > 0 ? '+' : 0, fmt_int(v[k] + (amt[k] > 0), amt[k]);
        else
            str_cpy(v[k], st[k] == 2u ? "0" : st[k] ? "--" : "", 4);
        sig = studio_hash(sig * 31u + st[k] * 7u + (uint32_t)(amt[k] + 64) * 131u + (ui.hot_t && ui.hot_col == k) * 5003u, lab[k]);
    }
    sig = studio_hash(sig, ui.msg_t ? ui.msg : "");
    if (!ui.force && sig == *cache)
        return;
    *cache = sig;
    cv_begin(240, 58, C_BLACK);
    cv_rect(0, 1, 240, 1, TE_G1);
    if (ui.msg_t) {                                     /* a message: a white bar */
        cv_rect(0, 14, 240, 32, C_WHITE);
        cv_text((240 - text_w(&FONT_S, ui.msg)) / 2, 22, &FONT_S, ui.msg, C_BLACK);
        cv_blit(0, dials_y);
        return;
    }
    for (k = 0; k < 4u; k++) {
        int32_t x0 = 2 + 60 * (int32_t)k, vx = x0 + dial_vx(v[k]), vw = dial_w(v[k]);
        uint32_t hot = ui.hot_t && ui.hot_col == k;
        uint16_t lc = st[k] == 2u ? col : TE_G2, vc = st[k] == 2u ? (amt[k] ? col : TE_G3) : TE_G2;
        if (!st[k])
            continue;
        if (hot && st[k] == 2u) {
            cv_rect(x0, 4, text_w(&FONT_S, lab[k]) + 2, 16, lc);
            cv_text(x0 + 1, 4, &FONT_S, lab[k], C_BLACK);
        } else {
            cv_text(x0 + 1, 4, &FONT_S, lab[k], lc);
        }
        if (st[k] == 2u)
            map_ring(x0 + 7, 38, 6, amt[k], col, TE_G2);
        else
            te_dial(x0 + 7, 38, 6, -1, TE_G1, TE_G1);
        dial_over += dial_wide(v[k]);
        if (st[k] == 2u && amt[k])                      /* set: boxed (a lock is inverted) */
            op_box(vx - 3, 21, vw + 5, 34, col);
        cv_text_tall_n(vx, 22, v[k], vc, dial_adv(v[k]));
        if (hot)
            cv_rect(vx - 1, 56, vw + 1, 2, lc);
    }
    cv_blit(0, dials_y);
}
/* knobs of page pg with something mapped onto them, from any source (the matrix's fixed cells too: the FILT page's ENV is
 * FENV > CUTOFF): a bit a knob, the mark outside the hold */
static uint32_t map_marks(const page_t *pg)
{
    uint32_t k, m = 0, s, id;
    int32_t a;
    if (is_drum(TSEL))
        return dmap_marks();
    for (k = 0; k < 4u; k++)
        for (s = 0; s < MS_N; s++)
            if (map_knob(pg, k, s, &a, &id) == 2u && a) {
                m |= 1u << k;
                break;
            }
    return m;
}
