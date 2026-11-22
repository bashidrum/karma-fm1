/* SPDX-License-Identifier: GPL-3.0-only */
/* PERFORM on the screen and the knobs (PHEBUS wave 6; Ben's picker "A: pad map, trimmed set", review/direction
 * mock-perf-A). Included by ui_layers.c: it is the FX layer (hold FX; held 0.7 s untouched and let go, it stays, as
 * GLO's mixer does; FX again or any page button leaves). The engine and the pads: punch.c perf_*.
 *   title     FX: PERFORM, how many latched, LOCK while it stays open
 *   chips     the 11 black keys in two rows, in key order (F#3 .. F#5); the four that have a pad, and two triplets
 *   pads      the 16 white keys as 4 x 4 (keys 1..4 the top row): 1/1 1/2 1/4 1/8 / 1/16 1/32 REVRS HALF /
 *             LP HP CRUSH GATE / STOP DROP ECHO VERB. Held: white. Latched: grey with a white dot. Played by the lane
 *             (recorded): grey with a white bar under it. The knobs' pad: outlined
 *   caption   the last pad pressed, in words, and its state
 *   dials     its four knobs, the drum and synth pages' dials (KNOB 4 is AMOUNT wherever a pad has one)
 * Everything here acts on the whole mix: white and grey only (the colour rule). */
#define PF_Y 32                                         /* the chips and pads' canvas, 122 rows */
#define PF_CAP_Y 155                                    /* the caption */
#define PF_KNOB_Y 172                                   /* the dials */
static void pf_knob_str(uint32_t p, uint32_t k, char *v, int32_t *ratio)   /* a knob's value, as shown, and where it sits */
{
    const pk_t *d = &PFD[p % PF_NPAD].k[k & 3u];
    int32_t x = perf.par[p % PF_NPAD][k & 3u];
    v[0] = 0;
    *ratio = -1;
    switch (d->kind) {
    case KK_GLO: {                                      /* a shared global (GLO's DLY, REV/CHO page) */
        const char *u;
        const param_desc_t *g = &GP[d->gid];
        x = song.g[d->gid];
        param_format(g, x, v, &u);
        *ratio = g->max > g->min ? (x - g->min) * 1000 / (g->max - g->min) : 0;
        return;
    }
    case KK_NONE:
        return;
    case KK_LIST:
        str_cpy(v, d->names[clamp(x, d->lo, d->hi)], 8);
        break;
    case KK_SEMI:
        if (x > 0)
            v[0] = '+', fmt_int(v + 1, x);
        else
            fmt_int(v, x);
        break;
    case KK_CUT: {                                      /* the filters' cutoff, in Hz */
        uint32_t hz = CUTOFF_HZ[clamp(x, 0, 127)];
        if (hz >= 1000u) {
            fmt_int(v, (int32_t)(hz / 1000u));
            str_cpy(v + str_len(v), ".", 2);
            fmt_int(v + str_len(v), (int32_t)(hz % 1000u / 100u));
            str_cpy(v + str_len(v), "K", 2);
        } else {
            fmt_int(v, (int32_t)hz);
        }
        break;
    }
    case KK_X:                                          /* crush's RATE: 1 = every sample (OFF) */
        if (x <= 1)
            str_cpy(v, "OFF", 4);
        else
            fmt_int(v, x);
        break;
    default:
        fmt_int(v, x);
        break;
    }
    *ratio = d->hi > d->lo ? (x - d->lo) * 1000 / (d->hi - d->lo) : 0;
}
/* KNOB k turned s on PERFORM: the last pad's knob. Turned while the pad plays: kept for a recording (perf.kn) */
static void perf_knob(uint32_t k, int32_t s)
{
    uint32_t p = perf.last % PF_NPAD;
    const pk_t *d = &PFD[p].k[k & 3u];
    int32_t x;
    if (d->kind == KK_NONE)
        return;
    if (d->kind == KK_GLO) {
        const param_desc_t *g = &GP[d->gid];
        song.g[d->gid] = (int16_t)clamp(song.g[d->gid] + accel(EN_K1 + k, s, g->max - g->min), g->min, g->max);
        return;
    }
    x = perf.par[p][k] + (d->hi - d->lo > 20 ? accel(EN_K1 + k, s, d->hi - d->lo) : (s > 0 ? 1 : -1));
    perf.par[p][k] = (int8_t)clamp(x, d->lo, d->hi);
    if (((perf.held | perf.lat) >> p) & 1u) {           /* (recorded with the pad's span, if a take is on) */
        perf.knv[p] = perf.par[p][k];
        perf.kn[p] = (uint8_t)(k + 1u);
    }
}
static uint32_t perf_keys_lit(uint32_t blink)           /* the keys: held and recorded pads lit, latched ones blink */
{
    uint32_t m = 0, on = perf.held | perf.lane | (blink ? perf.lat : 0u), p;
    for (p = 0; p < PF_NPAD; p++)
        if ((on >> p) & 1u)
            m |= 1u << punch_key_of_fx(p);
    return m;
}
static uint32_t perf_shown_sig(void)
{
    return perf.held * 3u + perf.lat * 7u + perf.lane * 13u + perf.last * 104729u;
}
/* a pad or a chip: its state's fill, its label, a dot when latched, a bar when the lane plays it */
static void pf_pad(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t p, int chip, int bright)
{
    uint32_t b = PFB(p), held = (perf.held & b) != 0u, lat = (perf.lat & b) != 0u, lane = (perf.lane & b) != 0u;
    const char *n = PFD[p].pad;
    int32_t ty = chip ? y - 1 : y + 3;
    if (held) {
        cv_rect(x, y, w, h, C_WHITE);
        te_text_c(x + w / 2, ty, n, C_BLACK);
        return;
    }
    if (lat || lane) {
        cv_rect(x, y, w, h, TE_G4);
        cv_rect(x + 1, y + 1, w - 2, h - 2, TE_G2);
        te_text_c(x + w / 2, ty, n, C_WHITE);
        if (lat)
            cv_rect(x + w - 7, y + 3, 4, 4, C_WHITE);
        if (lane && !lat)
            cv_rect(x + 3, y + h - 3, w - 6, 2, C_WHITE);
        return;
    }
    cv_rect(x, y, w, h, TE_G1);
    if (p == perf.last)                                 /* the knobs' pad */
        op_box(x, y, w, h, TE_G3);
    te_text_c(x + w / 2, ty, n, bright ? TE_G4 : TE_G3);
}
static void perf_screen_draw(void)
{
    static uint32_t head, body, cap, foot;
    static char v[4][10];
    const char *lab[4], *val[4] = {v[0], v[1], v[2], v[3]};
    int32_t ratio[4];
    uint32_t i, p = perf.last % PF_NPAD, nl = pf_count(perf.lat), locked = ly_lock == LY_FX, sig;
    char t[16];
    /* the title */
    sig = nl * 7u + locked * 131u + (ui.msg_t ? studio_hash(3u, ui.msg) : 0u);
    if (ui.force || sig != head) {
        int32_t x;
        head = sig;
        cv_begin(240, PF_Y, C_BLACK);
        x = cv_text(4, 12, &FONT_S, "FX:", TE_G3) + 2;
        cv_text(x, -2, &FONT_L, "PERFORM", C_WHITE);
        if (ui.msg_t) {
            cv_rect(0, 4, 240, 24, C_WHITE);
            te_text_c(120, 8, ui.msg, C_BLACK);
        } else {
            x = 236;
            if (locked) {                               /* it stays open: FX again (or any page button) leaves */
                int32_t w = (int32_t)text_w(&FONT_S, "LOCK") + 6;
                cv_rect(x - w, 6, w, 16, C_WHITE);
                cv_text(x - w + 3, 6, &FONT_S, "LOCK", C_BLACK);
                x -= w + 4;
            }
            if (nl) {
                fmt_int(t, (int32_t)nl);
                str_cpy(t + str_len(t), locked ? " LAT" : " LATCHED", 10);
                cv_text(x - text_w(&FONT_S, t), 6, &FONT_S, t, TE_G4);
            }
        }
        cv_rect(0, PF_Y - 2, 240, 1, TE_G1);
        cv_blit(0, 0);
    }
    /* the chips (black keys) and the pads (white keys) */
    sig = perf_shown_sig();
    if (ui.force || sig != body) {
        body = sig;
        cv_begin(240, 122, C_BLACK);
        for (i = 0; i < 11u; i++) {
            int32_t r = i < 6u ? 0 : 1, c = i < 6u ? (int32_t)i : (int32_t)i - 6, x = 2 + c * 40, y = r * 15;
            if (PF_BLACK[i] < 0)
                op_box(x, y, 37, 14, TE_G1);            /* a key that does nothing here: an outline (review 19) */
            else
                pf_pad(x, y, 37, 14, (uint32_t)PF_BLACK[i], 1, 0);
        }
        for (i = 0; i < 16u; i++)
            pf_pad(2 + (int32_t)(i % 4u) * 60, 31 + (int32_t)(i / 4u) * 23, 56, 21, i, 0, i < 8u);
        cv_blit(0, PF_Y);
    }
    /* the caption: the knobs' pad and what it is doing */
    {
        uint32_t b = PFB(p);
        const char *st = perf.held & b ? "HELD" : perf.lat & b ? "LATCHED" : perf.lane & b ? "FROM THE LANE" : "LAST PAD";
        sig = studio_hash(p * 31u + (perf.held & b ? 1u : 0u) + (perf.lat & b ? 2u : 0u) + (perf.lane & b ? 4u : 0u), st);
        if (ui.force || sig != cap) {
            cap = sig;
            cv_begin(240, PF_KNOB_Y - PF_CAP_Y, C_BLACK);
            cv_text(4, 0, &FONT_S, PFD[p].name, TE_G4);
            cv_text(236 - text_w(&FONT_S, st), 0, &FONT_S, st, TE_G3);
            cv_blit(0, PF_CAP_Y);
        }
    }
    for (i = 0; i < 4u; i++) {
        lab[i] = PFD[p].k[i].lab;
        pf_knob_str(p, i, v[i], &ratio[i]);
    }
    knob_tint_all(TINT_NEUTRAL);
    {
        uint8_t m = ui.msg_t;                           /* (a message shows in the title: the dials stay) */
        ui.msg_t = 0;
        opk_y = PF_KNOB_Y;
        op_knobs(lab, val, 0, ratio, 0, 0x50E7u + p * 977u, &foot);
        opk_y = OP_KNOB_Y;
        ui.msg_t = m;
    }
    if (ui.force) {                                     /* (the rows under the dials) */
        cv_begin(240, 240 - PF_KNOB_Y - OP_KNOB_H, C_BLACK);
        cv_blit(0, PF_KNOB_Y + OP_KNOB_H);
    }
}
