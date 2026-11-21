/* SPDX-License-Identifier: GPL-3.0-only */
/* The screensavers (PHEBUS, Ben, 21 Nov 2026: "Drop all 11, new only"). On HOME (the TRACKS screen), HOME tapped
 * opens one; HOME again or any page closes it; SELECT steps through them (the name shows a second); a layer held
 * shows its screen as ever, then the screensaver comes back. The keys, PLAY and REC work as always.
 *   1 FLOW    liquid marble, wavy stripes and glitch tears, a band per voice
 *   2 EYES    four watching eyes in a turning lattice, an eye per voice
 *   3 DESERT  dunes in four strata under a ringed sun, a stratum per voice
 *   4 THIRD EYE  the three fused: a great eye as the sun on the dunes, FLOW for a sky
 * Ben: "acid psychedelic modern glitch art fused with ... psychedellic eyes and desert art vibe", "These but flowing
 * and morphing"; the four voice colours only (plus black and a little white), "each driven by its own voice's level";
 * no busier than the boot picture he set as "the limit". Original designs, not copies of anyone's artwork.
 * Everything is computed into the canvas pixel by pixel (no pictures, no frame buffer), mostly in 2x2 blocks, from
 * what the audio already leaves for the UI: the tracks' peaks (fx.c / drums.c, as the TRACKS meters read them), the
 * notes that started and the kick (voice.c vis_hit). Drawn in the main loop every other frame, in two bands of 120
 * rows: no cost to the audio. */
static const char *const VIS_NAME[VIS_N] = {"FLOW", "EYES", "DESERT", "THIRD EYE"};
/* vis_on, vis_shown_last, vis_name_t: ui_draw.c; vis_style, VIS_N: panel.c (kept with the settings) */
static struct {
    uint16_t e[4];                                  /* each voice's level, smoothed: 0..256 (fast up, slow down) */
    uint8_t flash[4], kick;                         /* a note started (8, counting down) / a kick, fading */
    uint32_t tq, last_ms, rip;                      /* the clock (1/256: ~40 a second, faster when loud); the sun's rings */
} vs;
static uint16_t vp[4][4];                           /* this picture's colours of each voice: dark, body, bright, hot */

static int vis_shown(void) { return vis_on && !ui.home && !ui.menu && cur_page()->scope == SC_TRK; }
static void vis_open(void)
{
    vis_on = 1;
    vis_name_t = 30;
    ui.force = 1;
}
static int32_t isin(uint32_t a) { return SINE[a & 1023u]; }          /* a: 1024 a turn; Q15 */
static int32_t icos(uint32_t a) { return SINE[(a + 256u) & 1023u]; }
static uint16_t mix565(uint16_t a, uint16_t b, int32_t k)              /* a -> b, k 0..256 */
{
    int32_t r = (a >> 11) + ((((b >> 11) - (a >> 11)) * k) >> 8);
    int32_t g = ((a >> 5) & 63) + (((((b >> 5) & 63) - ((a >> 5) & 63)) * k) >> 8);
    int32_t bl = (a & 31) + ((((b & 31) - (a & 31)) * k) >> 8);
    return (uint16_t)(r << 11 | g << 5 | bl);
}
/* |x| (Q15) -> 0..1000 over 48 dB (8 log2: 6 dB a step of 8) */
static int32_t vis_lvl(int32_t a)
{
    int32_t lg = 0, v;
    if (a < 0)
        a = -a;
    if (a < 16)
        return 0;
    while ((a >> lg) > 1)
        lg++;
    v = lg * 8 + (((a << 3) >> lg) & 7);           /* 120 = 0 dB */
    return clamp((v - 56) * 1000 / 64, 0, 1000);
}
static uint32_t vis_sqrt(uint32_t v)
{
    uint32_t r = 0, b = 1u << 30;
    while (b > v)
        b >>= 2;
    while (b) {
        if (v >= r + b)
            v -= r + b, r = (r >> 1) + b;
        else
            r >>= 1;
        b >>= 2;
    }
    return r;
}
static int32_t vis_abs(int32_t v) { return v < 0 ? -v : v; }
static uint32_t vis_hash(uint32_t a, uint32_t b) { a = a * 2654435761u ^ b * 40503u; return (a ^ (a >> 15)) * 2246822519u; }

/* ---- the frame's data ---- */
/* the levels (0..256 a voice, or -1: the peaks, read here), the notes that started, the kick; the clock runs when go (the
 * screensaver: always; HOME's band: while playing or a voice sounds). Returns 1 while anything moves */
static uint32_t vis_step(const int32_t *lv, uint32_t go)
{
    uint32_t i, hit, now = fm1_ms, dt, emax = 0, fl = 0;
    int32_t pk[4];
    fm1_irq_off();                                  /* the tracks' peaks since the last look, the notes that started */
    if (!lv) {
        for (i = 0; i < NPART; i++) {
            pk[i] = trk[i].peak;
            trk[i].peak = 0;
        }
        pk[3] = drums.peak;
        drums.peak = 0;
    }
    hit = vis_hit;
    vis_hit = 0;
    if (vis_kick_hit) {
        vis_kick_hit = 0;
        vs.kick = 255;
    }
    fm1_irq_on();
    for (i = 0; i < 4u; i++) {
        int32_t v = lv ? lv[i] : trk_level(i) && !trk[i].p[P_MUTE] ? vis_lvl(pk[i]) * 256 / 1000 : 0;   /* (as the TRACKS meters) */
        vs.e[i] = (uint16_t)(v > vs.e[i] ? vs.e[i] + (v - vs.e[i]) / 2 : vs.e[i] - (vs.e[i] - v) / 12);
        vs.flash[i] = (uint8_t)((hit >> i) & 1u ? 8u : vs.flash[i] ? vs.flash[i] - 1u : 0u);
        fl |= vs.flash[i];
        if (vs.e[i] > emax)
            emax = vs.e[i];
    }
    vs.kick = (uint8_t)(vs.kick > 20 ? vs.kick - 20 : 0);
    dt = now - vs.last_ms;                          /* the clock: gentle when quiet, a little quicker when loud */
    vs.last_ms = now;
    if (dt > 200u)
        dt = 200u;
    go |= emax | fl | vs.kick;
    if (go)
        vs.tq += dt * (40u * 256u + emax * 48u) / 1000u;
    vs.rip += 1u + vs.kick / 40u;
    return go != 0u;
}
static void vis_update(void) { vis_step(0, 1); }
/* the voices' colours for this picture, from their levels (a quiet voice breathes, dim, near black) */
static void vis_palette(void)
{
    uint32_t i, t = vs.tq >> 8;
    for (i = 0; i < 4u; i++) {
        int32_t e = vs.e[i] + 24 + (isin(t * 6u + i * 256u) * 18 >> 15) + vs.flash[i] * 6;
        if (e > 256) e = 256;
        vp[i][0] = mix565(C_BLACK, TE_DIM[i], 150 + e * 106 / 256);
        vp[i][1] = e < 128 ? mix565(TE_DIM[i], TE_MID[i], e * 2) : mix565(TE_MID[i], TE_COL[i], (e - 128) * 2);
        vp[i][2] = mix565(TE_MID[i], TE_COL[i], 96 + e * 160 / 256);
        vp[i][3] = mix565(TE_COL[i], C_WHITE, e > 150 ? (e - 150) * 150 / 106 : 0);
    }
}

/* ---- painting: each piece paints the band of the canvas (screen rows y0..y0+119) row pair by row pair, in 2x2
 * blocks (the top row worked out, the bottom row a copy), every pixel only where detail shows (the eyes) ---- */
static void vis_canvas(void)                        /* (cv_begin without the clear: every pixel is painted) */
{
    lcd_sync();
    cv_w = 240u;
    cv_h = 120u;
    cv_oy = 0;
}
#define VIS_ROW2(q) memcpy((q) + 240, (q), 240u * sizeof(uint16_t))
#define VIS_KEEP 0x0821u                            /* (eyes_px: no eye here, what is under it stays) */

/* ---- FLOW's field: the boot's ripples of warped sines cut into bands, a band per voice with black between. A voice
 * playing swells its band (pushing the others aside), brightens it and, loud, lights a white vein in it; a quiet
 * voice thins to a dim line. Every 15 s or so it morphs to the next kind (a wipe down the screen): liquid marble,
 * wavy stripes, marble torn sideways in rows; a kick tears a few rows in any kind. At the boot's broadest scale
 * (x2): never busier than Ben's limit. */
static uint16_t fl_lut[256];                        /* the bands along one turn of the field (byte-swapped) */
static uint32_t fl_t, fl_cyc, fl_wipe;
static uint8_t fl_noline;                           /* (HOME's strip: the morph without its white line) */
static void flow_prep(void)
{
    uint32_t T = vs.tq >> 8, j, p = 0, w[4], sum = 0, rot;
    fl_t = T;
    fl_cyc = T / 600u;
    rot = fl_cyc / 3u;
    fl_wipe = T % 600u < 40u ? T % 600u * 6u : 999u;   /* the morph: the new kind above a line going down (a second) */
    for (j = 0; j < 4u; j++)
        sum += w[j] = 26u + vs.e[(j + rot) & 3u] * 30u / 256u;
    for (j = 0; j < 4u; j++) {
        uint32_t i = (j + rot) & 3u, q, edge = w[j] / 6u + 1u, hot = vs.e[i] > 170u ? (vs.e[i] - 170u) / 22u : 0u;
        for (q = 0; q < w[j]; q++)
            fl_lut[(p + q) & 255u] = swap16(q < edge || q >= w[j] - edge ? vp[i][0]
                                            : (q >= w[j] / 2u - hot && q < w[j] / 2u + hot) ? vp[i][3] : vp[i][1]);
        p += w[j];
        for (q = 0; q < (256u - sum) / 4u; q++)     /* black between */
            fl_lut[(p + q) & 255u] = C_BLACK;
        p += (256u - sum) / 4u;
    }
    for (; p < 256u; p++)
        fl_lut[p] = C_BLACK;
}
static void flow_row(uint16_t *q, uint32_t sy, uint32_t xa, uint32_t xb, uint32_t xn)   /* (columns xa..xb-1 left: covered;
                                                                                           * xn: the row's width) */
{
    uint32_t T = fl_t, kind = sy < fl_wipe ? fl_cyc % 3u : (fl_cyc + 2u) % 3u, h = vis_hash(sy >> 2, (T >> 3) + kind) >> 24, x;
    int32_t ra = (isin(sy * 5u + T * 3u) * 150) >> 15, tear = (kind == 2u && h < 14u) || h < vs.kick / 24u ? (int32_t)(h * 4u % 56u) - 28 : 0;
    for (x = 0; x < xn; x += 2u) {
        int32_t a, b, v;
        if (x == xa)
            x = xb;
        if (x >= xn)
            break;
        a = ((int32_t)x + tear) * 2 + ra;
        b = (int32_t)sy * 2 + ((isin((uint32_t)a + T * 2u) * 110) >> 15);
        a += (isin((uint32_t)b + T * 5u) * 90) >> 15;
        v = kind == 1u ? a * 2 + b : (isin((uint32_t)a) + isin((uint32_t)(b + (a >> 1)))) >> 6;
        q[x] = q[x + 1u] = fl_lut[((uint32_t)(v + (int32_t)(T * 4u)) >> 2) & 255u];
    }
    if (sy - fl_wipe < 2u && !fl_noline)            /* (the morph's edge: a white line) */
        for (x = 0; x < xn; x++)
            q[x] = C_WHITE;
}

/* ---- the eyes: almond eyes, each the lens where two circles cross, outlined, with two echo contours round it; an
 * iris of rings flowing outward (faster when loud), a pupil that opens with the level, a glint ---- */
static struct {
    int32_t x, y, c, r2, o2, c0, k1, k1e, k2, k2e, gx, gy, rp2, ri2, fi2, th[12];
    uint16_t ic[13], sc, lid, e1, e2, ph, rw;      /* (rw: the iris rings' width, 1/256 px; 3 px or more: drawn in blocks) */
} ey[4];
static uint32_t ey_n;
/* eye i at x, y: half width hw, half height open hh0 and now hh; its colours voice v's (4: the four voices in turn) */
static void eye_set(uint32_t i, int32_t x, int32_t y, int32_t hw, int32_t hh0, int32_t hh, uint32_t v, int32_t rp, int32_t ri, uint32_t e)
{
    static const uint8_t RC[4] = {2, 1, 0, 3};      /* the iris rings: bright, body, dark, hot */
    int32_t r = (hh * hh + hw * hw) / (2 * hh), r0 = (hh0 * hh0 + hw * hw) / (2 * hh0), g = hw / 5, off;
    uint32_t j;
    ey[i].x = x, ey[i].y = y;
    ey[i].c = r - hh;                               /* the lids: circles r, their centres c below and above */
    ey[i].r2 = r * r;
    ey[i].o2 = (r + 2) * (r + 2);
    ey[i].c0 = r0 - hh0;                            /* the echoes: the open lens grown by g and 2g, 3 px wide */
    ey[i].k1 = (r0 + g) * (r0 + g), ey[i].k1e = ey[i].k1 + 6 * (r0 + g);
    ey[i].k2 = (r0 + 2 * g) * (r0 + 2 * g), ey[i].k2e = ey[i].k2 + 6 * (r0 + 2 * g);
    ey[i].rp2 = rp * rp;
    ey[i].ri2 = ri * ri;
    ey[i].fi2 = ey[i].rw < 768u ? ri * ri + 30 : (rp + 5) * (rp + 5);   /* (every pixel: the iris, or round the pupil) */
    ey[i].ph = (uint16_t)((ey[i].ph + (ey[i].rw >> 8) * (3u + e / 20u) / 2u) % (4u * ey[i].rw));
    off = (int32_t)ey[i].rw * 8 - (int32_t)ey[i].ph;   /* the bounds: where r*256 + off hits a multiple of rw */
    for (j = 0; j < 12u; j++) {                     /* (the bounds squared, in 1/16 px^2) */
        int32_t k = (rp * 256 + off) / (int32_t)ey[i].rw + 1 + (int32_t)j, b = ((int32_t)ey[i].rw * k - off) / 64;
        ey[i].th[j] = b * b;
        ey[i].ic[j] = v < 4u ? vp[v][RC[(k - 1) & 3]] : vp[(k - 1) & 3][(k & 4) ? 1 : 2];
    }
    ey[i].ic[12] = ey[i].ic[11];
    v &= 3u;
    ey[i].sc = mix565(C_BLACK, TE_DIM[v], 90);
    ey[i].lid = vp[v][2], ey[i].e1 = vp[v][1], ey[i].e2 = vp[v][0];
}
static inline __attribute__((always_inline)) uint16_t eyes_px(uint32_t k, int32_t x, int32_t y)   /* (near eye k) */
{
    int32_t dx = x - ey[k].x, dy = y - ey[k].y, ay = dy < 0 ? -dy : dy, f = dx * dx + (ay + ey[k].c) * (ay + ey[k].c);
    if (f <= ey[k].r2) {                            /* in the eye */
        int32_t ix = dx - ey[k].gx, iy = dy - ey[k].gy, r2 = ix * ix + iy * iy, j = 0;
        if ((ix + 5) * (ix + 5) + (iy + 5) * (iy + 5) < 6)
            return C_WHITE;                         /* the glint */
        if (r2 < ey[k].rp2)
            return C_BLACK;
        if (r2 < ey[k].ri2) {                       /* the iris */
            r2 <<= 4;
            while (j < 12 && r2 >= ey[k].th[j])
                j++;
            return ey[k].ic[j];
        }
        return r2 < ey[k].ri2 + 30 ? C_BLACK : ey[k].sc;
    }
    if (f <= ey[k].o2)
        return ey[k].lid;                           /* the lid's line */
    f = dx * dx + (ay + ey[k].c0) * (ay + ey[k].c0);
    if (f >= ey[k].k1 && f < ey[k].k1e)
        return ey[k].e1;                            /* the echoes */
    if (f >= ey[k].k2 && f < ey[k].k2e)
        return ey[k].e2;
    return VIS_KEEP;
}
static void eyes_own(uint8_t *own, uint32_t k, int32_t a, int32_t b)   /* columns a..b near eye k (the nearer eye wins) */
{
    int32_t c;
    for (c = a < 0 ? 0 : a >> 1; c <= b >> 1 && c < 120; c++)
        if (own[c] == 4u || vis_abs(c * 2 - ey[k].x) < vis_abs(c * 2 - ey[own[c]].x))
            own[c] = (uint8_t)k;
}
static void eyes_over(uint16_t *q, int32_t sy)      /* the eyes over row sy (painted), then its copy below */
{
    int32_t fx[24];
    uint32_t nf = 0, k, x, j;
    uint8_t own[120];                               /* which eye each column pair shows (4: none) */
    memset(own, 4, sizeof own);
    for (k = 0; k < ey_n; k++) {
        int32_t a0 = vis_abs(sy - ey[k].y), a1 = vis_abs(sy + 1 - ey[k].y), ay, w2, A = (a0 + ey[k].c0) * (a0 + ey[k].c0);
        for (j = 0; j < 2u; j++) {                  /* the columns where the echoes cross this row */
            int32_t lo = j ? ey[k].k2 : ey[k].k1, hi = j ? ey[k].k2e : ey[k].k1e, wo, wi;
            if (hi <= A)
                continue;
            wo = (int32_t)vis_sqrt((uint32_t)(hi - A)) + 1;
            wi = lo > A ? (int32_t)vis_sqrt((uint32_t)(lo - A)) - 1 : 0;
            eyes_own(own, k, ey[k].x - wo, ey[k].x - wi);
            eyes_own(own, k, ey[k].x + wi, ey[k].x + wo);
        }
        ay = (a0 < a1 ? a0 : a1) + ey[k].c;         /* the eye itself, and every pixel on its lids' line and iris */
        w2 = ey[k].o2 - ay * ay;
        if (w2 > 0) {
            int32_t w = (int32_t)vis_sqrt((uint32_t)w2) + 1, wi, ai = (a0 > a1 ? a0 : a1) + ey[k].c, iy;
            eyes_own(own, k, ey[k].x - w, ey[k].x + w);
            w2 = ey[k].r2 - ai * ai;
            wi = w2 > 0 ? (int32_t)vis_sqrt((uint32_t)w2) - 1 : 0;
            fx[nf++] = (ey[k].x - w) & ~1, fx[nf++] = ey[k].x - wi + 1;
            fx[nf++] = (ey[k].x + wi - 1) & ~1, fx[nf++] = ey[k].x + w + 1;
            a0 = vis_abs(sy - ey[k].y - ey[k].gy), a1 = vis_abs(sy + 1 - ey[k].y - ey[k].gy);
            iy = a0 < a1 ? a0 : a1;
            w2 = ey[k].fi2 - iy * iy;              /* (the iris and the black ring round it) */
            if (w2 > 0) {
                w2 = (int32_t)vis_sqrt((uint32_t)w2) + 2;
                fx[nf++] = (ey[k].x + ey[k].gx - w2) & ~1, fx[nf++] = ey[k].x + ey[k].gx + w2;
            }
        }
    }
    for (x = 0; x < 240u; x += 2u)
        if (own[x >> 1] < 4u) {
            uint16_t c = eyes_px(own[x >> 1], (int32_t)x, sy);
            if (c != VIS_KEEP)
                q[x] = q[x + 1u] = swap16(c);
        }
    VIS_ROW2(q);
    for (k = 0; k < nf; k += 2u) {
        int32_t a = fx[k] < 0 ? 0 : fx[k], b = fx[k + 1u] > 240 ? 240 : fx[k + 1u], i;
        for (i = a; i < b; i += 2) {
            uint32_t o = own[i >> 1] & 3u;
            uint16_t c = eyes_px(o, i + 1, sy), d = eyes_px(o, i, sy + 1), e = eyes_px(o, i + 1, sy + 1);
            if (c != VIS_KEEP) q[i + 1] = swap16(c);
            if (d != VIS_KEEP) q[i + 240] = swap16(d);
            if (e != VIS_KEEP) q[i + 241] = swap16(e);
        }
    }
}
static uint32_t eye_blink(uint32_t T, uint32_t i, int32_t hh)   /* the lids now: a blink now and then */
{
    uint32_t bt = (T + i * 537u) % (170u + i * 41u);
    return (uint32_t)(bt < 8u ? 1 + (hh - 1) * (int32_t)(bt < 4u ? 4u - bt : bt - 4u) / 4 : hh);
}

/* ---- 2 EYES: four eyes on a slowly turning, breathing square lattice, all looking at the same wandering point;
 * behind them octagonal rings flowing outward, a voice's dim colour each in turn. Each eye is a voice (its iris,
 * pupil and lines in the voice's colour, brighter as it plays); a white star at the centre jumps on the kick. */
#define EYE_HW 31                                   /* half the width of an eye, half its height open */
#define EYE_HH 17
static uint16_t ey_bg[4];                           /* the rings behind */
static int32_t ey_star, ey_ring;
static void eyes_prep(void)
{
    uint32_t T = vs.tq >> 8, i, ang = T * 3u / 7u;
    int32_t R = 64 + (isin(T * 4u) * 5 >> 15), tx = 120 + (isin(T * 5u) * 70 >> 15), ty = 120 + (isin(T * 7u + 300u) * 50 >> 15);
    ey_star = 6 + vs.kick / 32;
    ey_ring = (int32_t)((vs.tq >> 9) & 15u);
    ey_n = 4;
    for (i = 0; i < 4u; i++) {
        ey[i].rw = 512;                             /* (rings 2 px) */
        int32_t e = vs.e[i], x = 120 + (icos(ang + i * 256u) * R >> 15), y = 120 + (isin(ang + i * 256u) * R >> 15);
        eye_set(i, x, y, EYE_HW, EYE_HH, (int32_t)eye_blink(T, i, EYE_HH), i, 3 + e * 5 / 256 + vs.flash[i] / 4, 13 + e * 2 / 256, (uint32_t)e);
        ey[i].gx = clamp((tx - x) / 6, -9, 9);
        ey[i].gy = clamp((ty - y) / 9, -4, 4);
        ey_bg[i] = mix565(C_BLACK, TE_DIM[i], 130 + e / 2);
    }
}
static void eyes_back_row(uint16_t *q, int32_t sy)   /* the rings and the star, worked out where they cross the row */
{
    int32_t aw = vis_abs(sy - 120), n, a, b, x;
    memset(q, 0, 240u * sizeof(uint16_t));
    for (n = 2; n < 12; n++) {                      /* ring n: the octagonal distance d - ey_ring in [16n, 16n + 2) */
        int32_t lo = 16 * n + ey_ring, hi = lo + 2, seg;
        uint16_t c = swap16(ey_bg[n & 3]);
        for (seg = 0; seg < 2; seg++) {
            if (!seg)                               /* ax < aw: d = aw + ax * 3 / 8 */
                a = (8 * (lo - aw) + 2) / 3, b = (8 * (hi - aw) + 2) / 3, a = a < 0 ? 0 : a, b = b > aw ? aw : b;
            else                                    /* ax >= aw: d = ax + aw * 3 / 8 */
                a = lo - aw * 3 / 8, b = hi - aw * 3 / 8, a = a < aw ? aw : a;
            for (x = a; x < b && x <= 120; x++) {
                if (120 + x < 240)
                    q[120 + x] = c;
                q[120 - x] = c;
            }
        }
    }
    for (x = 0; x < ey_star - aw; x++)              /* the star */
        q[120 + x] = q[120 - x] = C_WHITE;
}

/* ---- 3 DESERT: a horizon at dusk. A ringed sun half set on it, its rings flowing outward (a kick pushes a ripple
 * through them) and a faint halo; a few stars; four strata of dunes drifting at their own speeds (the nearer, the
 * faster), lit on the faces turned to the low sun, with a bright crest and a contour under it; heat shimmer near the
 * horizon and, now and then (and on a kick), a glitch scanline torn sideways. The sun's rings and the strata are the
 * voices, purple farthest, coral nearest: a voice playing raises its dunes and brightens them and its rings. */
#define DS_HOR 116
static uint8_t ds_r[4][121];                        /* each stratum's crest, every 2 columns (bit 0: the face is lit) */
static uint16_t ds_sh[4];                           /* each stratum's shadow */
static uint8_t ds_top[4];                           /* each stratum's highest crest */
static void desert_prep(void)
{
    static const uint8_t BASE[4] = {DS_HOR + 8, DS_HOR + 30, DS_HOR + 62, DS_HOR + 100}, SP[4] = {1, 2, 4, 7};
    uint32_t T = vs.tq >> 8, s, i;
    for (s = 0; s < 4u; s++) {
        int32_t amp = 5 + (int32_t)s * 3 + (int32_t)vs.e[s] * (6 + (int32_t)s * 3) / 256;
        ds_sh[s] = mix565(vp[s][0], vp[s][1], 100);
        ds_top[s] = 255;
        for (i = 0; i <= 120u; i++) {
            uint32_t ph = i * (9u - s) + T * SP[s] / 2u + s * 300u, p2 = ph * 2u + 170u + T / 3u;
            int32_t y = (BASE[s] * 32 - (isin(ph) * amp >> 10) - (isin(p2) * amp >> 11) - amp * 16 + 32) >> 6 << 1;   /* (even, rounded once: no jitter) */
            int32_t dy = -(icos(ph) * 2 + icos(p2) * 2);   /* (the slope: going down to the right when > 0) */
            y = clamp(y, DS_HOR, 238);
            ds_r[s][i] = (uint8_t)(y | (dy > 0));   /* lit: the faces turned to the left, the sun low there */
            if (y < ds_top[s])
                ds_top[s] = (uint8_t)y;
        }
    }
}
static void desert_row(uint16_t *q, int32_t sy, int sky)   /* (sky 0: the sky left black, no sun or stars) */
{
    uint32_t T = vs.tq >> 8, x, h = vis_hash((uint32_t)sy / 3u, T / 5u) >> 24;
    int32_t rs = 44 + vs.kick / 40, rh = rs + 22, rip = (int32_t)(vs.rip % 96u) / 4, dh = sy - DS_HOR, dx = 0, r = 0, xs, s0 = 3;
    while (s0 >= 0 && sy < ds_top[s0])            /* (the strata that reach up to this row) */
        s0--;
    if (dh > -14 && dh < 24)                        /* the shimmer, the glitch scanlines */
        dx = isin((uint32_t)sy * 90u + T * 24u) * 2 >> 15;
    if (h < 4u + vs.kick / 28u)
        dx += (int32_t)(h * 37u % 48u) - 24;
    for (x = 0; x < 240u; x += 2u) {
        int32_t sx = dx ? clamp((int32_t)x + dx, 0, 239) : (int32_t)x, s = s0, cx = sx - 120;
        uint32_t xi = (uint32_t)sx >> 1;
        uint16_t c = C_BLACK;
        for (; s >= 0; s--)                         /* the nearest stratum whose crest is above */
            if (sy >= (ds_r[s][xi] & ~1)) {
                int32_t d = sy - (ds_r[s][xi] & ~1);
                uint32_t xl = xi + (uint32_t)d / 2u;   /* (the shadow's edge slants down from each crest) */
                c = d < 2 ? vp[s][3] : d >= 8 + s * 3 && d < 10 + s * 3 ? vp[s][0] : (ds_r[s][xl < 120u ? xl : 120u] & 1u) ? vp[s][1] : ds_sh[s];
                break;
            }
        if (s < 0 && sky) {                         /* the sky: the sun, its halo, the stars */
            int32_t r2 = cx * cx + dh * dh;
            if (r2 < rh * rh) {
                if (!r)                             /* (the radius followed along the row) */
                    r = (int32_t)vis_sqrt((uint32_t)r2);
                while (r * r > r2)
                    r--;
                while ((r + 1) * (r + 1) <= r2)
                    r++;
                xs = r - rip + 48;
                c = r < 7 ? C_WHITE : r < rs ? (xs % 6 == 5 ? C_BLACK : vp[(xs / 6) & 3][1 + ((xs / 6) & 1)])
                                    : xs % 6 < 2 ? vp[(xs / 6) & 3][0] : C_BLACK;
            } else if (sy < DS_HOR - 6) {
                uint32_t hs = vis_hash(xi, (uint32_t)sy >> 1);
                if ((hs >> 24) < 2u)
                    c = ((hs >> 8) + T) % 40u < 4u ? C_WHITE : vp[hs & 3u][2];
            }
        }
        q[x] = q[x + 1u] = swap16(c);
    }
}

/* ---- 4 THIRD EYE (the three fused): one great eye as the sun on the desert's horizon, the flowing field for a sky.
 * The sky is FLOW (its bands the voices), the dunes DESERT's (a stratum per voice), the eye's iris rings the four
 * voices in turn, its pupil opening with the loudest; its lines and echoes cut black through the sky. */
static void third_prep(void)
{
    uint32_t T = vs.tq >> 8, i;
    int32_t emax = 0;
    flow_prep();
    desert_prep();
    for (i = 0; i < 4u; i++)
        if (vs.e[i] > emax)
            emax = vs.e[i];
    ey_n = 1;
    ey[0].rw = 768;                                 /* (rings 3 px, in blocks) */
    eye_set(0, 120, 100, 64, 30, (int32_t)eye_blink(T / 2u, 3u, 30), 4u, 5 + emax * 8 / 256 + vs.kick / 64, 25, (uint32_t)emax);
    ey[0].gx = isin(T * 5u) * 14 >> 15;
    ey[0].gy = isin(T * 3u + 200u) * 5 >> 15;
    ey[0].sc = C_BLACK;
    ey[0].lid = C_WHITE;
    ey[0].e1 = ey[0].e2 = C_BLACK;
}

/* the screensaver, every other frame: the update, then both bands */
static void vis_paint(void)
{
    uint32_t pass, st = vis_style % VIS_N, y;
    vis_palette();
    if (st == 0u)
        flow_prep();
    else if (st == 1u)
        eyes_prep();
    else if (st == 2u)
        desert_prep();
    else
        third_prep();
    for (pass = 0; pass < 2u; pass++) {
        vis_canvas();
        for (y = 0; y < 120u; y += 2u) {
            uint16_t *q = &cv_px[y * 240u];
            int32_t sy = (int32_t)(pass * 120u + y);
            if (st == 0u)
                flow_row(q, (uint32_t)sy, 1u, 1u, 240u), VIS_ROW2(q);
            else if (st == 1u)
                eyes_back_row(q, sy), eyes_over(q, sy);
            else if (st == 2u)
                desert_row(q, sy, 1), VIS_ROW2(q);
            else {
                if (sy < DS_HOR) {                  /* (not under the eye's black) */
                    int32_t ay = vis_abs(sy - ey[0].y) + ey[0].c, w2 = ey[0].r2 - (ay + 1) * (ay + 1), w;
                    w = w2 > 0 ? (int32_t)vis_sqrt((uint32_t)w2) - 2 : 0;
                    w = w > 0 ? w : 0;
                    flow_row(q, (uint32_t)sy, (uint32_t)(ey[0].x - w + 1) & ~1u, (uint32_t)(ey[0].x + w) & ~1u, 240u);
                }
                else
                    desert_row(q, sy, 0);
                eyes_over(q, sy);
            }
        }
        if (!pass && (vis_name_t || ui.msg_t)) {    /* the piece's name a second (or a message) */
            char n[8];
            const char *s = ui.msg_t ? ui.msg : VIS_NAME[st];
            cv_rect(0, 0, 240, 20, C_BLACK);
            cv_text(120 - text_w(&FONT_S, s) / 2, 2, &FONT_S, s, C_WHITE);
            if (!ui.msg_t) {
                fmt_int(n, (int32_t)st + 1);
                str_cpy(n + str_len(n), "/", 2);
                fmt_int(n + str_len(n), (int32_t)VIS_N);
                cv_text(4, 2, &FONT_S, n, C_DIM);
            }
        }
        cv_blit(0, pass * 120u);
    }
}
static void vis_draw(void)
{
    if (!vis_shown_last || ui.force) {
        vis_shown_last = 1;
        lcd_fill(0, 0, 240, 240, C_BLACK);
    } else if (ui.frame & 1u) {
        return;
    }
    vis_update();
    vis_paint();
    if (vis_name_t)
        vis_name_t--;
}
/* SELECT on the screensaver: the next / previous one (round) */
static void vis_select(int32_t s)
{
    vis_style = (uint8_t)((vis_style + (s > 0 ? 1u : VIS_N - 1u)) % VIS_N);
    vis_name_t = 30;
    ui.force = 1;
    settings_later = 1;                                 /* (kept with the settings, once stopped) */
}

/* ---- HOME's band (PHEBUS, Ben 21 Nov 2026: "I don't like how the tape reel looks"; picker "A strip of the flowing
 * backdrop": "a band of the FLOW pattern moving with the four voices' levels"): FLOW's voice-driven field in a strip of the
 * deck, each voice's band swelling with its own level (HOME's meters, ui_op.c op_mx), sinking dim when quiet. It flows
 * while the song plays or a voice sounds and stands still otherwise. home_band_tick once a frame (1: it moves, draw it
 * again, every other frame), home_band into the deck's canvas: w x h at (x0, y0), in 2 x 2 blocks */
static uint32_t home_band_tick(void)
{
    int32_t lv[4];
    uint32_t i;
    for (i = 0; i < 4u; i++)
        lv[i] = (int32_t)op_mx[i] * 256 / (TS_H - 2);
    return vis_step(lv, song.playing);
}
static void home_band(int32_t x0, int32_t y0, uint32_t w, uint32_t h)
{
    uint32_t y;
    vis_palette();
    flow_prep();
    fl_noline = 1;                                  /* (a white line standing still across it, stopped: no) */
    for (y = 0; y < h; y += 2u) {
        uint16_t *q = &cv_px[(uint32_t)(y0 + (int32_t)y) * cv_w + (uint32_t)x0];
        flow_row(q, y, w, w, w);                    /* (the screensaver's scale: no busier than Ben's limit) */
        memcpy(q + cv_w, q, w * sizeof(uint16_t));
    }
    fl_noline = 0;
}

/* ---- the boot's FLOW (Ben, 20 Nov 2026: "acid psychedelic modern glitch art ... flowing and morphing"; the four voices'
 * colours only): a field of warped sines, posterised into bands of the voices' colours and black. Three kinds by the
 * seed: 0 liquid marble, 1 wavy stripes (two voices), 2 marble torn sideways in rows (glitch). The boot draws it at
 * full resolution (Ben: "Keep 3 s full detail"); the FLOW screensaver above is its voice-driven cousin. */
static uint32_t flow_seed;
static uint8_t flow_lit;                                /* 1: no black here (behind the black wordmark): dim voices instead */
static uint16_t flow_px(int32_t x, int32_t y, uint32_t t)
{
    static const uint8_t BAND[3][16] = {{0, 4, 1, 1, 4, 2, 6, 4, 3, 3, 4, 0, 5, 4, 2, 4},      /* 0..3 a voice, 4 black, */
                                        {0, 0, 0, 4, 4, 1, 1, 1, 0, 0, 0, 4, 4, 1, 1, 1},      /* 5 white, 6 a voice dim */
                                        {4, 0, 4, 4, 2, 5, 4, 1, 4, 4, 3, 4, 0, 4, 6, 4}};
    uint32_t s = flow_seed, kind = s % 3u, rot = (s >> 4) & 3u, k;
    int32_t a, b, v;
    if (kind == 2u) {                                   /* rows of 4 torn sideways, a new tear every few frames */
        uint32_t h = ((uint32_t)(y >> 2) * 2654435761u + (t >> 3) * 40503u + s) >> 24;
        if (h < 14u)                                    /* (few tears, short: the field stays readable) */
            x += (int32_t)(h * 4u) - 28;
    }
    /* Ben, 20 Nov: seed 0 (x3, y3) "is also the limit for how busy these should get": scales 2..3, never finer */
    a = x * (int32_t)(2u + ((s >> 8) & 1u)) + ((isin((uint32_t)(y * 5) + t * 3u + (s >> 10)) * 150) >> 15);
    b = y * (int32_t)(2u + ((s >> 12) & 1u)) + ((isin((uint32_t)a + t * 2u + (s >> 14)) * 110) >> 15);
    a += (isin((uint32_t)b + t * 5u) * 90) >> 15;
    v = kind == 1u ? a * 2 + b : (isin((uint32_t)a) + isin((uint32_t)(b + (a >> 1)))) >> 6;
    k = BAND[kind][(((uint32_t)(v + (int32_t)(t * 4u))) >> 6) & 15u];
    if (k == 4u && flow_lit)
        return TE_MID[((((uint32_t)(v + (int32_t)(t * 4u))) >> 7) + rot) & 3u];
    return k < 4u ? TE_COL[(k + rot) & 3u] : k == 4u ? C_BLACK : k == 5u ? C_WHITE : TE_MID[rot];
}
static void flow_band(uint32_t y0, uint32_t rows, uint32_t t, int32_t fade, uint32_t st, uint32_t lit0, uint32_t lit1)
{   /* into the canvas: rows y0.. of the screen, in st x st blocks (1: smooth); rows lit0..lit1-1 of the screen: no black */
    uint32_t x, y, i, j;
    for (y = 0; y < rows; y += st) {
        flow_lit = y0 + y >= lit0 && y0 + y < lit1;
        for (x = 0; x < 240u; x += st) {
            uint16_t c = flow_px((int32_t)x, (int32_t)(y0 + y), t);
            if (fade)
                c = mix565(c, C_BLACK, fade);
            c = swap16(c);
            for (j = 0; j < st && y + j < rows; j++)
                for (i = 0; i < st; i++)
                    cv_px[(y + j) * 240u + x + i] = c;
        }
    }
    flow_lit = 0;
}
static void sloop_logo_knock(uint32_t y0, uint32_t band_y, uint32_t rows);   /* splash.c */
/* ---- the boot animation (PHEBUS; Ben, 20 Nov 2026): a FLOW backdrop, a different one each start, and the wordmark
 * black in it: "the phebus is all black so it appears as the negative space in the trippy background visual". ~3 s,
 * a button or a key skips it. boot_t: started at 0 by the boot; 0xFFFF: none (the host tests) */
static uint16_t boot_t = 0xFFFFu;
#define BOOT_END 180u
static void boot_frame(uint32_t t)
{
    int32_t fade = t < 16u ? 256 - (int32_t)t * 16 : t > BOOT_END - 20u ? (int32_t)(t - (BOOT_END - 20u)) * 256 / 20 : 0;
    uint32_t pass;
    for (pass = 0; pass < 2u; pass++) {
        cv_begin(240, 120, C_BLACK);
        flow_band(pass * 120u, 120u, t, fade, 1u, 0u, 240u);   /* (the wordmark fills the square: the only black is its letters) */
        sloop_logo_knock(0, pass * 120u, 120u);
        cv_blit(0, pass * 120u);
    }
}
static int boot_draw(void)
{
    uint32_t t = boot_t;
    if (t >= BOOT_END)
        return 0;
    if (fm1_in.buttons || fm1_in.notes || ++boot_t >= BOOT_END) {
        boot_t = 0xFFFFu;
        lcd_fill(0, 0, 240, 240, C_BLACK);
        ui.force = 1;
        return 0;
    }
    if (!t)                                             /* a different one each start: the clock, the battery's noise */
        flow_seed = (fm1_ticks() * 2654435761u) ^ (fm1_ms * 40503u) ^ ((uint32_t)batt_level() << 7) ^ 0x9E3779B9u;
    boot_frame(t);
    return 1;
}
