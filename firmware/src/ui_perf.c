/* SPDX-License-Identifier: GPL-3.0-only */
/* PERFORM on the screen and the knobs (PHEBUS wave 6; wave 13: the PERFORM set). Included by ui_layers.c: it is the FX layer
 * (hold FX; held 0.7 s untouched and let go, it stays, as GLO's mixer does; FX again or any page button leaves). The engine
 * and the pads: punch.c perf_*.
 *   header    (wave 7: every screen's) FX, PERFORM (PERF and how many while pads are latched), the tempo, the transport
 *   caption   under it: the last pad pressed, in words, and its state; LOCK while it stays open; a message
 *   strip     (wave 13) the 11 black keys in key order, in the strip's place: the seven MUTES on the top row (V1 V2 V3 in their
 *             voices' colours, the four drum mutes teal: each acts on that voice only), the four extras under them (REV STOP
 *             SW DN SW UP, white and grey: the whole mix), each over a column of the pads
 *   pads      the 16 white keys as 4 x 4 (keys 1..4 the top row): 1/1 1/2 1/4 1/8 / 1/16 HALF ECHO VERB / ISO LO ISO MID
 *             ISO HI MOTION / CRUSH PHASER JET VOWEL. Held: white. Latched: grey with a white dot. Played by the lane
 *             (recorded): grey with a white bar under it. The knobs' pad: outlined
 *   dials     its four knobs, the drum and synth pages' dials (ui_drum.c op_dials; KNOB 4 is AMOUNT wherever a pad has
 *             one), white-grey
 * The effects act on the whole mix: white and grey (the colour rule); a mute acts on one voice: its colour. */
#define PF_Y 40                                         /* (wave 7: the frame every screen has) the chips and pads, 114 rows */
#define PF_CAP_Y 22                                     /* the caption, under the header */
static const char *const VOW_NAME[5] = {"U", "O", "A", "E", "I"};
static const char *pf_label(uint32_t p)                 /* a pad's short name (the lane's repeat: its length) */
{
    if (p == PF_LSTUT)
        return LST_NAME[clamp(perf.par[PF_LSTUT][0], 0, (int32_t)LST_N - 1)];
    return PFD[p % PF_NPAD].pad;
}
static uint16_t pf_col(uint32_t p)                      /* a pad's family colour: a mute its voice's, an effect white */
{
    if (p >= PF_MV1 && p <= PF_MV3)
        return TE_COL[p - PF_MV1];
    if (p >= PF_MKICK && p <= PF_MHTSN)
        return TE_COL[TRK_DRUM];
    return C_WHITE;
}
static uint32_t pf_knob_pad(uint32_t p) { return p == PF_ISOM || p == PF_ISOH ? PF_ISOL : p; }   /* (the isolator: one set) */
static void pf_knob_str(uint32_t p, uint32_t k, char *v, int32_t *ratio)   /* a knob's value, as shown, and where it sits */
{
    const pk_t *d = &PFD[p % PF_NPAD].k[k & 3u];
    int32_t x = perf.par[pf_knob_pad(p % PF_NPAD)][k & 3u];
    v[0] = 0;
    *ratio = -1;
    switch (d->kind) {
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
    case KK_VOW: {                                      /* (wave 13) the vowel, between two: both ("A-E") */
        int32_t a = clamp(x, 0, 40) / 10, f = clamp(x, 0, 40) % 10;
        str_cpy(v, VOW_NAME[f >= 7 && a < 4 ? a + 1 : a], 4);
        if (f >= 3 && f < 7 && a < 4) {
            str_cpy(v + 1, "-", 2);
            str_cpy(v + 2, VOW_NAME[a + 1], 2);
        }
        break;
    }
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
    uint32_t p = perf.last % PF_NPAD, q = pf_knob_pad(p), i;
    const pk_t *d = &PFD[p].k[k & 3u];
    int32_t x;
    if (d->kind == KK_NONE)
        return;
    x = perf.par[q][k] + (d->hi - d->lo > 20 ? accel(EN_K1 + k, s, d->hi - d->lo) : (s > 0 ? 1 : -1));
    x = clamp(x, d->lo, d->hi);
    if (q == PF_ISOL)                                   /* (wave 13: the three isolator pads share their crossovers) */
        for (i = PF_ISOL; i <= PF_ISOH; i++)
            perf.par[i][k] = (int8_t)x;
    perf.par[q][k] = (int8_t)x;
    if (((perf.held | perf.lat) >> p) & 1u) {           /* (recorded with the pad's span, if a take is on) */
        perf.knv[p] = perf.par[q][k];
        perf.kn[p] = (uint8_t)(k + 1u);
    }
}
static uint32_t perf_keys_lit(uint32_t blink)           /* the keys: held and recorded pads lit, latched ones blink */
{
    uint32_t m = 0, on = perf.held | perf.lane | (blink ? perf.lat : 0u), p;
    for (p = 0; p < PF_NKEY; p++)
        if ((on >> p) & 1u)
            m |= 1u << punch_key_of_fx(p);
    return m;
}
static uint32_t perf_shown_sig(void)
{
    return perf.held * 3u + perf.lat * 7u + perf.lane * 13u + perf.last * 104729u;
}
/* a pad or a chip: its state's fill, its label, a dot when latched, a bar when the lane plays it. col: its family (white: an
 * effect on the whole mix; a voice's colour: a mute of that voice) */
static void pf_pad(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t p, int chip, int bright)
{
    uint32_t b = PFB(p), held = (perf.held & b) != 0u, lat = (perf.lat & b) != 0u, lane = (perf.lane & b) != 0u;
    uint16_t col = pf_col(p), mid = col == C_WHITE ? TE_G4 : TE_MID[p >= PF_MKICK ? TRK_DRUM : p - PF_MV1];
    const char *n = PFD[p].pad;
    int32_t ty = chip ? y - 2 : y + 2;
    if (held) {
        cv_rect(x, y, w, h, col);
        te_text_c(x + w / 2, ty, n, C_BLACK);
        return;
    }
    if (lat || lane) {
        cv_rect(x, y, w, h, mid);
        cv_rect(x + 1, y + 1, w - 2, h - 2, col == C_WHITE ? TE_G2 : TE_DIM[p >= PF_MKICK ? TRK_DRUM : p - PF_MV1]);
        te_text_c(x + w / 2, ty, n, C_WHITE);
        if (lat)
            cv_rect(x + w - 7, y + 3, 4, 4, C_WHITE);
        if (lane && !lat)
            cv_rect(x + 3, y + h - 3, w - 6, 2, C_WHITE);
        return;
    }
    cv_rect(x, y, w, h, TE_G1);
    if (col != C_WHITE) {                               /* (a mute: its voice's colour, a band on its left, its name) */
        cv_rect(x, y, 3, h, col);
        te_text_c(x + 1 + w / 2, ty, n, col);
        return;
    }
    if (p == perf.last)                                 /* the knobs' pad */
        op_box(x, y, w, h, TE_G3);
    te_text_c(x + w / 2, ty, n, bright ? TE_G4 : TE_G3);
}
/* (wave 13) the black keys' chips: the seven mutes on the top row (V1 V2 V3 narrow, the drum mutes wide), the four extras on
 * the second, over the pads' columns; key order left to right, top then bottom (F#3 .. G#4, A#4 .. F#5) */
static void pf_chip_box(uint32_t i, int32_t *x, int32_t *y, int32_t *w)
{
    static const uint8_t X[7] = {2, 28, 54, 84, 123, 162, 201}, W[7] = {24, 24, 24, 37, 37, 37, 37};
    if (i < 7u)
        *x = X[i], *y = 0, *w = W[i];
    else
        *x = 2 + (int32_t)(i - 7u) * 60, *y = 13, *w = 56;
}
static void perf_screen_draw(void)
{
    static uint32_t head, body, cap, foot, bar;
    static char v[4][10];
    const char *lab[4], *val[4] = {v[0], v[1], v[2], v[3]};
    int32_t ratio[4];
    uint32_t i, p = perf.last % PF_NPAD, locked = ly_lock == LY_FX, sig;
    {   /* (wave 10) MASTER's chip: FX MIX */
        mpair_t mp;
        if (master_pair(LY_FX, &mp))
            hdr_mchip = mp.chip, hdr_mcol = mp.col, hdr_mratio = (mp.v - mp.lo) * 1000 / (mp.hi - mp.lo);
        op_header("FX", C_WHITE, "PERFORM", &head);
        hdr_mchip = 0;
    }
    /* the chips (black keys) and the pads (white keys) */
    bk_drawn = 0x7FFu, bk_drawn_f = ui.frame;           /* (wave 11: the strip's live keys, for the lights; wave 13: all 11) */
    sig = perf_shown_sig();
    if (ui.force || sig != body) {
        body = sig;
        cv_begin(240, SG_KNOB_Y - PF_Y, C_BLACK);
        for (i = 0; i < 11u; i++) {
            int32_t x, y, w;
            pf_chip_box(i, &x, &y, &w);
            pf_pad(x, y, w, 12, PF_MV1 + i, 1, 0);
        }
        for (i = 0; i < 16u; i++)
            pf_pad(2 + (int32_t)(i % 4u) * 60, 27 + (int32_t)(i / 4u) * 22, 56, 20, i, 0, i < 8u);
        cv_blit(0, PF_Y);
    }
    /* the caption: the knobs' pad and what it is doing; LOCK while it stays open; a message in a white bar */
    {
        uint32_t c = perf.cap % PF_NPAD, b = PFB(c);
        const char *st = perf.held & b ? "HELD" : perf.lat & b ? "LATCHED" : perf.lane & b ? "FROM LANE" : "LAST PAD";
        sig = studio_hash(c * 31u + (perf.held & b ? 1u : 0u) + (perf.lat & b ? 2u : 0u) + (perf.lane & b ? 4u : 0u) + locked * 8u, st);
        if (ui.msg_t)
            sig = studio_hash(sig * 3u + 1u, ui.msg);
        if (ui.force || sig != cap) {
            int32_t x = 236;
            cap = sig;
            cv_begin(240, PF_Y - PF_CAP_Y, C_BLACK);
            if (ui.msg_t) {
                cv_rect(0, 0, 240, 17, C_WHITE);
                te_text_c(120, 0, ui.msg, C_BLACK);
            } else {
                if (locked) {                           /* it stays open: FX again (or any page button) leaves */
                    int32_t w = (int32_t)text_w(&FONT_S, "LOCK") + 6;
                    cv_rect(x - w, 0, w, 16, C_WHITE);
                    cv_text(x - w + 3, 0, &FONT_S, "LOCK", C_BLACK);
                    x -= w + 6;
                }
                x = pm_draw(x, 0, "OFF", "LATCH") - 8;   /* (wave 10) -/+: every latch off (the held pads' only), latch */
                cv_text(4, 0, &FONT_S, PFD[c].name, pf_col(c) == C_WHITE ? TE_G4 : pf_col(c));
                if (x - text_w(&FONT_S, st) > 8 + text_w(&FONT_S, PFD[c].name))
                    cv_text(x - text_w(&FONT_S, st), 0, &FONT_S, st, TE_G3);
            }
            cv_blit(0, PF_CAP_Y);
        }
    }
    for (i = 0; i < 4u; i++) {
        lab[i] = PFD[p].k[i].lab;
        pf_knob_str(p, i, v[i], &ratio[i]);
    }
    {   /* the synth and drum pages' dials (a ring, the value in the tall face), white-grey: the whole mix's */
        uint8_t m = ui.msg_t;                           /* (a message shows in the caption: the dials stay) */
        ui.msg_t = 0;
        op_dials(lab, val, ratio, 0, 0, TE_G4, 0x50E7u + p * 977u, &foot);
        ui.msg_t = m;
    }
    op_trackbar(&bar);
}
