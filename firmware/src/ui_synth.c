/* SPDX-License-Identifier: GPL-3.0-only */
/* The melodic voices' screens, the MiniFreak's way (PHEBUS wave 4; Ben, 21 Nov 2026: "I'd really love the melodic parts of
 * PHEBUS to feel as similar to the minifreak as possible"; "Melodic voices: the MiniFreak's swift sound design. Oscillator
 * type plus a few macro knobs, filter, envelopes, FX slots, mod matrix last. The same layout on every voice"; pickers
 * "OSC page: A: Type + three macros", "Filter envelope: As decided: ADSR + link", "Locks: Rytm way"). Included by
 * ui_studio.c after ui_drum.c: the same visual language as the drum screen (a large name, tabs, four dials over the
 * knobs, one tall value face in fixed columns, the touched label inverted, a locked value inverted).
 *
 *   a sound page   (OSC FILT ENV FX MOD, a synth track: the keys play notes) the header ([1] and the sound's name); the
 *                  page's subject large in the voice's colour (OSC: the Type; FILT: the filter's type; ENV: AMP or FILTER,
 *                  the AMP | FILT selector; FX, MOD: the page) with a line under it; what the knobs do, drawn (the wave,
 *                  the filter's response and its envelope's sweep, the envelope(s), the sends, the LFO); the tabs OSC FILT
 *                  ENV FX MOD and the half (1/3); four dials; the strip. No character, no scene behind (Ben: "Cut them")
 *   the GRID       (a synth track: the keys are the 16 steps) the steps as tiles, what the step held (or under the cursor)
 *                  holds; the tabs TRIG OSC FILT ENV FX MOD, as the drum screen's TRIG SRC FLTR AMP LFO: EDIT opens OSC
 *                  (again: the next stage), ENV, FX, LFO (MOD) their own (again: the next half), SEQ TRIG (again: back),
 *                  SELECT every half; with a step held too. Nothing held, the knobs edit the sound (TRIG: NOTE STEP HIT VEL,
 *                  the pen); a step held, a knob LOCKS what it shows on the step (TRIG: the step's NOTE LEN VEL COND, FLAG
 *                  RETRIG NUDGE), the locked value inverted; OCT- the lock of the knob last turned, again every lock */
#include "ui_map.c"                                      /* (wave 11) hold LFO / ENV to map */
/* ---- the values */
static void syn_fmt_type(char *b, uint32_t ty) { str_cpy(b, ty < NVTYPE ? VTYPES[ty].name : "--", 8); }
/* page pg's four knobs on the selected synth track: labels, values (a step held, first: its lock, inverted), dials */
/* (review V8, Ben: "Fix all three") OSC 3/4 and 4/4: the voice's voices, not page 1's wave again: POLY eight dots, the ones
 * sounding filled; UNISON the eight stacked on one note, fanned out as far as DETUNE spreads them; MONO and LEGATO one; the
 * line under says how they are taken (PRIOR on 3/4, ALLOC on 4/4: ROT goes round, REUSE takes the first free) */
static void syn_voices(const track_t *t, uint16_t c, uint16_t cd)
{
    int32_t vm = clamp(t->p[P_VOICE], 0, 3), i, cy = 76, spread = t->p[P_DETUNE] * 24 / 127;
    char b[28];
    for (i = 0; i < NVOICE; i++) {
        int on = t->v[i].active != 0, x;
        if (vm == V_UNISON) {
            x = 120 + (2 * i - (NVOICE - 1)) * spread / 2;
            cv_rect(x - 1, cy - 12, 3, 24, on ? c : cd);
        } else if (vm == V_POLY) {
            x = 22 + i * 28;
            if (on)
                te_disc(x, cy, 7, c);
            else
                op_box(x - 7, cy - 7, 15, 15, cd);
        } else if (!i) {
            if (on)
                te_disc(120, cy, 9, c);
            else
                op_box(111, cy - 9, 19, 19, cd);
        }
    }
    str_cpy(b, vm == V_POLY ? "POLY: 8 VOICES" : vm == V_UNISON ? "UNISON: 8 ON ONE NOTE" : vm == V_MONO ? "MONO: ONE VOICE"
                                                                                                    : "LEGATO: ONE VOICE", sizeof b);
    cv_text(4, 94, &FONT_S, b, TE_G3);
    str_cpy(b, t->p[P_ALLOC] ? "REUSE" : "ROT", 8);
    cv_text(236 - text_w(&FONT_S, b), 94, &FONT_S, b, TE_G3);
}
/* (review V6, Ben: "One format with units") an envelope's value on its dial, both envelopes alike: a time in ms under a second
 * ("340MS"), then seconds ("1.2S", "12S"); the sustain in % ("80%"). 0: not an envelope's time or level */
static int env_fmt(uint32_t id, const param_desc_t *d, int32_t x, char *v)
{
    int env = (id >= P_ATK && id <= P_REL) || (id >= P_FATK && id <= P_FREL);
    if (!env)
        return 0;
    if (d->fmt == F_TIME) {
        uint32_t ms10 = TIME_MS_X10[x & 127];
        if (ms10 < 9995u) {
            fmt_int(v, (int32_t)((ms10 + 5u) / 10u));
            str_cpy(v + str_len(v), "MS", 3);
        } else {
            if (ms10 < 99500u)
                fmt_fix(v, (int32_t)((ms10 + 50u) / 100u), 1);
            else
                fmt_int(v, (int32_t)((ms10 + 5000u) / 10000u));
            str_cpy(v + str_len(v), "S", 2);
        }
        return 1;
    }
    if (d->fmt == F_PCT) {
        fmt_int(v, d->max > 0 ? (x * 100 + d->max / 2) / d->max : x);
        str_cpy(v + str_len(v), "%", 2);
        return 1;
    }
    return 0;                                           /* (ENV SYNC on: the note values, as they are) */
}
static void syn_vals(const page_t *pg, int32_t first, const char *lab[4], char v[4][12], int32_t ratio[4], uint32_t *locked,
                     uint32_t *dim)
{
    track_t *t = TSEL;
    uint32_t k;
    *locked = *dim = 0;
    for (k = 0; k < 4u; k++) {
        uint32_t id = pg_id(pg, k);
        int16_t *vp;
        const param_desc_t *d = page_desc(pg, k, &vp);
        const char *u;
        int32_t x;
        lab[k] = "";
        v[k][0] = 0;
        ratio[k] = -1;
        if (!d || !vp || !d->label || d->label[0] == '-')
            continue;
        lab[k] = id == V_TYPE ? "TYPE" : pg->id[k] == V_MAMT ? "AMOUNT" : d->label;   /* (wave 5: a fixed cell too) */
        if (id == V_TYPE) {
            uint32_t ty = vtype_of(t);
            syn_fmt_type(v[k], ty);
            ratio[k] = (int32_t)(ty < NVTYPE ? ty : 0u) * 1000 / (int32_t)(NVTYPE - 1u);
            if (first >= 0)
                *dim |= 1u << k;                        /* (not lockable: the Rytm's machine, grey with a step held) */
            continue;
        }
        if (id == V_MDST) {                             /* (wave 11) the column's name (an assignable one: its parameter's) */
            uint8_t cols[MD_N];
            uint32_t n = mx_vis(song.sel % NPART, cols);
            mx_col_name(song.sel % NPART, (uint32_t)mx_col, v[k]);
            ratio[k] = n > 1u ? (int32_t)mx_vis_pos(song.sel % NPART, (uint32_t)mx_col) * 1000 / (int32_t)(n - 1u) : 0;
            if (first >= 0)
                *dim |= 1u << k;
            continue;
        }
        if (id == V_MK4) {                              /* (wave 5) TARGET: the parameter's name; CLEAR: the cell */
            if (mx_col >= MD_AS1 && mx_col < (int16_t)MD_N) {
                uint32_t a = mxt[song.sel % NPART].as[mx_col - MD_AS1];
                str_cpy(v[k], a ? track_desc(t, a - 1u)->label : "--", 8);
                ratio[k] = a ? (int32_t)a * 1000 / P_COUNT : 0;
            } else {
                str_cpy(v[k], mx_get(song.sel % NPART, (uint32_t)mx_row % MS_N, (uint32_t)mx_col % MD_N) ? "CLR" : "--", 6);
            }
            dval_fit(v[k]);
            if (first >= 0)
                *dim |= 1u << k;
            continue;
        }
        x = *vp;
        if (id >= P_FATK && id <= P_FREL && !t->p[P_FLINK]) {   /* LINK: the amp ENV's, dim (the first turn: OWN) */
            x = t->p[P_ATK + (id - P_FATK)];
            *dim |= 1u << k;
        }
        if (first >= 0 && id >= P_COUNT)                /* (wave 5: a grid cursor, a shared value: no lock) */
            *dim |= 1u << k;
        if (first >= 0 && id < P_COUNT) {
            int l = lock_find(t, (uint32_t)first, id, 0);
            if (l >= 0) {
                x = t->lock[l].val;
                *locked |= 1u << k;
            }
            if (!p_lockable(id))
                *dim |= 1u << k;
        }
        if (!env_fmt(id, d, x, v[k]))
            param_format(d, x, v[k], &u);
        dval_fit(v[k]);
        ratio[k] = d->max > d->min ? (x - d->min) * 1000 / (d->max - d->min) : -1;
    }
}
/* TRIG's knobs: a step held, its NOTE LEN VEL COND (half 2: FLAG RETRIG NUDGE); nothing held, NOTE (the pen) STEP HIT VEL */
static void syn_trig_vals(int32_t first, uint32_t half, const char *lab[4], char v[4][12], int32_t ratio[4], uint32_t *dim)
{
    static const char *const LA[4] = {"NOTE", "LEN", "VEL", "COND"}, *const LB[4] = {"FLAG", "RETRIG", "NUDGE", ""};   /* (wave 8: one repeat word, RETRIG, as the drum trig's) */
    static const char *const LF[4] = {"NOTE", "STEP", "HIT", "VEL"};
    const track_t *t = TSEL;
    uint32_t k, len = trk_len(t), c = first >= 0 ? (uint32_t)first : sg_cur();
    const step_t *st = &t->step[c];
    *dim = 0;
    for (k = 0; k < 4u; k++) {
        lab[k] = first >= 0 || !half ? (half ? LB[k] : first >= 0 ? LA[k] : LF[k]) : LB[k];
        v[k][0] = 0;
        ratio[k] = -1;
    }
    if (half) {                                         /* FLAG RETRIG NUDGE */
        uint32_t f = (st->flags & SF_ACCENT ? 1u : 0u) | (st->flags & SF_SLIDE ? 2u : 0u);
        if (first < 0) {                                /* (wave 5) nothing held: Spice and Dice, the MiniFreak's */
            const mxt_t *m = &mxt[song.sel % NPART];
            lab[0] = "SPICE", lab[1] = "DICE", lab[2] = lab[3] = "";
            fmt_int(v[0], (m->spice * 100 + 63) / 127);
            v[1][0] = '#';
            fmt_int(v[1] + 1, m->dice);
            dval_fit(v[1]);
            ratio[0] = m->spice * 1000 / 127;
            ratio[1] = m->dice % 100 * 10;
            return;
        }
        str_cpy(v[0], SG_FLAG[f], 4);
        v[1][0] = 'X', fmt_int(v[1] + 1, (int32_t)(st->rat & 3u) + 1);
        fmt_int(v[2], t->micro[c]);
        if (t->micro[c] > 0)
            v[2][0] = '+', fmt_int(v[2] + 1, t->micro[c]);
        ratio[0] = (int32_t)f * 333;
        ratio[1] = (int32_t)(st->rat & 3u) * 333;
        ratio[2] = (t->micro[c] - MICRO_MIN) * 1000 / (MICRO_MAX - MICRO_MIN);
        return;
    }
    if (first >= 0) {                                   /* NOTE LEN VEL COND of the step held */
        if (step_on(st))
            note_name(v[0], st->note[0]);
        else
            str_cpy(v[0], "--", 4);
        fmt_int(v[1], step_on(st) ? (int32_t)note_len(t, c) : 0);
        fmt_int(v[2], step_on(st) ? (int32_t)step_vel_shown(st) : 0);
        str_cpy(v[3], SG_COND[step_fill(t, c)], 8);
        ratio[0] = step_on(st) ? st->note[0] * 1000 / 127 : 0;
        ratio[1] = step_on(st) ? (int32_t)note_len(t, c) * 1000 / (int32_t)len : 0;
        ratio[2] = step_on(st) ? (int32_t)step_vel_shown(st) * 1000 / 127 : 0;
        ratio[3] = (int32_t)step_fill(t, c) * 333;
        return;
    }
    note_name(v[0], pen_n ? pen_note[0] : last_note);    /* nothing held: the pen, the cursor, its step, its velocity */
    fmt_int(v[1], (int32_t)c + 1);
    str_cpy(v[2], step_on(st) ? "ON" : "--", 4);
    if (step_on(st))
        fmt_int(v[3], (int32_t)step_vel_shown(st));
    else
        str_cpy(v[3], "--", 4);
    ratio[0] = (pen_n ? pen_note[0] : last_note) * 1000 / 127;
    ratio[1] = (int32_t)c * 1000 / (int32_t)(len > 1u ? len - 1u : 1u);
    ratio[2] = step_on(st) ? 1000 : 0;
    ratio[3] = step_on(st) ? (int32_t)step_vel_shown(st) * 1000 / 127 : 0;
}

/* ---- what the knobs do, drawn (the body's lower rows: sp_t .. sp_b) */
/* an ADSR (the four ids) as voice.c makes it: the attack linear, decay and release exponential; x0 .. x1, top .. bot */
static void syn_adsr(const track_t *t, uint32_t a_id, int32_t x0, int32_t x1, int32_t top, int32_t bot, uint16_t c, int w2)
{
    const int16_t *p = t->p;
    int32_t span = x1 - x0, a = 2 + p[a_id] * span / 5 / 127, d = 3 + p[a_id + 1] * span / 5 / 127;
    int32_t r = 3 + p[a_id + 3] * span / 4 / 127, sus = p[a_id + 2] * 1000 / 127, xa = x0 + a, xd = xa + d, xr = x1 - r;
    int32_t i, px, py, e = 32768;
#define SY(l) (bot - (l) * (bot - top) / 1000)
#define SL(ax, ay, bx, by) (cv_line(ax, ay, bx, by, c), w2 ? (cv_line(ax, (ay) - 1, bx, (by) - 1, c), 0) : 0)
    SL(x0, bot, xa, top);
    px = xa, py = top;
    for (i = 1; i <= d; i++) {
        int32_t l;
        e = (e * (32768 - 150733 / d)) >> 15;
        l = sus + ((1000 - sus) * e >> 15);
        SL(px, py, xa + i, SY(l));
        px = xa + i, py = SY(l);
    }
    SL(px, py, xr, SY(sus));
    px = xr, py = SY(sus), e = 32768;
    for (i = 1; i <= r; i++) {
        e = (e * (32768 - 150733 / r)) >> 15;
        SL(px, py, xr + i, SY(sus * e >> 15));
        px = xr + i, py = SY(sus * e >> 15);
    }
    if (w2) {                                           /* the handles where the next knob takes over */
        cv_rect(xa - 1, top - 1, 3, 3, C_WHITE);
        cv_rect(xd - 1, SY(sus) - 1, 3, 3, C_WHITE);
        cv_rect(xr - 1, SY(sus) - 1, 3, 3, C_WHITE);
    }
#undef SY
#undef SL
}
/* the filter's response at TYPE: CUTOFF (0..127), RESO, the ENV amount's sweep (shaded, as far as it opens it). (Review 26
 * Nov, U3) by value, so the drum screen's FLTR and KIT FLT draw the same picture: ty VF_NTYPE an open filter (OFF), flat */
static void syn_filter_at(uint32_t ty, int32_t cut, int32_t res, int32_t env, int32_t drv, int32_t key, uint16_t c, uint16_t cd)
{
    int32_t w = SP_R - SP_L - 20, y0 = SP_T + 12 - drv * 8 / 127, xc = SP_L + 10 + cut * w / 127;   /* (DRIVE:
                                                         * the passband lifted, louder into the ladder) */
    int32_t r = res * (SP_B - SP_T - 8) / 127, xe = clamp(xc + env * w / 64, SP_L, SP_R), x;
    int32_t k = ty == VF_LP24 ? 10 : ty == VF_LP12 || ty == VF_HP ? 5 : 6;
    gp_dots(y0);
    for (x = xe; x != xc; x += xe > xc ? -3 : 3) {      /* the sweep: from where the envelope takes it, back */
        if ((xe > xc && x < xc) || (xe < xc && x > xc))
            break;
        cv_rect(x, y0 + 4, 1, SP_B - y0 - 4, cd);
    }
    gp_go = 0;
    for (x = SP_L; x <= SP_R; x += 2) {
        int32_t dd = x - xc, v = 0;
        if (ty < VF_NTYPE) {
            if (ty == VF_HP || ty == VF_BP)
                v += dd < 0 ? -dd * k / 8 : 0;
            if (ty != VF_HP)
                v += dd > 0 ? dd * k / 8 : 0;
            if (ty == VF_BP)
                v += 6;
            if (dd > -16 && dd < 16)
                v -= r * (256 - dd * dd) / 256;
        }
        gp_to(x, y0 + v, c);
    }
    if (ty >= VF_NTYPE)
        return;
    cv_rect(xc - 1, SP_B - 3, 3, 4, C_WHITE);           /* where CUTOFF is */
    if (key)                                            /* KEY: the cutoff follows the note, a slope through it */
        cv_line(xc - 14, SP_B - 2, xc + 14, SP_B - 2 - key * 12 / 127, C_WHITE);
}
static void syn_filter(const track_t *t, uint16_t c, uint16_t cd)
{
    const int16_t *p = t->p;
    syn_filter_at((uint32_t)p[P_FTYPE] % VF_NTYPE, p[P_FCUT], p[P_FRES], p[P_ED_FLT], p[P_FDRV], p[P_FKEY], c, cd);
}
/* the LFO: two cycles of its wave, running, "now" a dot (as graph_lfo, in the body's rows) */
static void syn_lfo(track_t *t, uint16_t c)
{
    int32_t x, py = SP_M, ny = SP_M, a = (SP_B - SP_T) / 2 - 2;
    uint32_t now = t->lfo_ph, step = 0xFFFFFFFFu / 120u;
    gp_dots(SP_M);
    for (x = SP_L; x <= SP_R; x++) {
        uint32_t ph = now + (uint32_t)(x - 160) * step;
        int32_t y = SP_M - lfo_wave(t, ph) * a / 32768;
        if (x > SP_L)
            cv_line(x - 1, py, x, y, c);
        if (x == 160)
            ny = y;
        py = y;
    }
    cv_rect(157, ny - 3, 7, 7, C_WHITE);
}
/* the SLICER's pattern of 16 (as graph_slicer, in the body's rows) */
static void syn_slicer(const track_t *t, uint16_t c)
{
    uint32_t i, pat = sl_pattern(t), mode = (uint32_t)t->p[P_SLCR], cur = sl[t - trk].idx;
    int32_t h = SP_B - SP_T - 6, open = h - t->p[P_SLDEPTH] * h / 127, y;
    uint16_t col = mode == SL_OFF ? TE_G3 : c;
    for (i = 0; i < 16u; i++) {
        int32_t x = 4 + (int32_t)i * 14 + (int32_t)(i / 4u) * 2;
        if ((pat >> i) & 1u)
            cv_rect(x, SP_T, 11, h, col);
        else if (mode == SL_STUT)
            for (y = SP_T; y < SP_T + h; y += 4)
                cv_rect(x, y, 11, 1, col);
        else if (open)
            cv_rect(x, SP_T + h - open, 11, open, TE_G2);
        if (mode != SL_OFF && i == cur)
            cv_rect(x, SP_B - 3, 11, 3, C_WHITE);
    }
}
/* (PHEBUS wave 5) the FX page (mock-synth-fx-A): the three slots as boxes, the one on the knobs filled, a square lit when the
 * slot runs; under them what the slot's knobs do and a picture of its effect */
static void syn_fx_slots(const track_t *t, uint32_t cur, uint16_t c, uint16_t cd)
{
    uint32_t k;
    for (k = 0; k < NXSLOT; k++) {
        int32_t x = 2 + (int32_t)k * 79, ty = clamp(t->p[P_XS(k, 0)], 0, XT_N - 1);
        if (xt_send(ty))
            ty = XT_OFF;                                /* (wave 8: no slot type: shown as it plays, OFF) */
        char lb[5] = {'F', 'X', ' ', (char)('1' + k), 0};   /* (review V2: FX 1) */
        uint16_t fg = ty ? (k == cur ? C_WHITE : c) : TE_G3;
        if (k == cur)
            cv_rect(x, 2, 77, 40, cd);
        op_box(x, 2, 77, 40, k == cur ? C_WHITE : TE_G2);
        cv_text(x + 4, 3, &FONT_S, lb, k == cur ? C_WHITE : TE_G3);
        if (ty)
            cv_rect(x + 64, 7, 7, 7, k == cur ? C_WHITE : c);   /* on */
        cv_text(x + 4, 21, &FONT_S, N_XBIG[ty], fg);
    }
}
static void syn_fx_pic(const track_t *t, uint32_t k, uint16_t c)
{
    int32_t ty = clamp(t->p[P_XS(k, 0)], 0, XT_N - 1), x, i;
    int32_t tm = t->p[P_XS(k, 1)], in = t->p[P_XS(k, 2)], am = t->p[P_XS(k, 3)];
    switch (ty) {
    case XT_PHASER: case XT_FLANGER: {                   /* the sweep: its notches moving (wave 8: no shared effect here) */
        int32_t py = SP_M, rate = 1 + tm / 16;
        for (x = SP_L; x <= SP_R; x++) {
            int32_t n = ty == XT_PHASER ? 2 : 6, q = ((x - SP_L) * n * 256 / (SP_R - SP_L) + (int32_t)ui.frame * rate) & 255;
            int32_t y = SP_T + 4 + (q < 128 ? q : 255 - q) * (SP_B - SP_T - 8) * clamp(in + 30, 0, 127) / 127 / 128;
            if (x > SP_L)
                cv_line(x - 1, py, x, y, c);
            py = y;
        }
        break;
    }
    case XT_DIST: case XT_SOFT: case XT_HARD: case XT_FOLD: case XT_FUZZ: case XT_CRUSH: {   /* the curve in -> out */
        int32_t R = (SP_B - SP_T) / 2 - 2, py = SP_M;
        gp_dots(SP_M);
        for (x = 120 - 2 * R; x <= 120 + 2 * R; x++) {
            int32_t in32 = (x - 120) * 32767 / (2 * R), g = 4096 + in * in * 3, v = (int32_t)(((int64_t)in32 * g) >> 12), o, y;
            int32_t it = ty == XT_DIST ? IT_SOFT : XT_IT[ty];
            o = it == IT_CRUSH ? ins_shape(IT_CRUSH, in32, ins_bits_sh(in)) : ins_shape((uint32_t)it, clamp(v, -98000, 98000), 0);
            o = clamp(o, -32767, 32767);
            o = in32 + (o - in32) * am / 127;              /* (wave 7: 32 bits; |o - in32| <= 65534, am <= 127) */
            y = SP_M - o * R / 32767;
            if (x > 120 - 2 * R)
                cv_line(x - 1, py, x, y, c);
            py = y;
        }
        break;
    }
    case XT_EQ: {                                        /* LOW MID HIGH as a curve through three points */
        int32_t y0 = SP_M - (tm - 64) * (SP_B - SP_T) / 140, y1 = SP_M - (in - 64) * (SP_B - SP_T) / 140;
        int32_t y2 = SP_M - (am - 64) * (SP_B - SP_T) / 140, py = y0;
        gp_dots(SP_M);
        for (x = SP_L; x <= SP_R; x++) {
            int32_t u = (x - SP_L) * 256 / (SP_R - SP_L), y;
            y = u < 128 ? y0 + (y1 - y0) * u / 128 : y1 + (y2 - y1) * (u - 128) / 128;
            if (x > SP_L)
                cv_line(x - 1, py, x, y, c);
            py = y;
        }
        break;
    }
    default:
        gp_dots(SP_M);
        break;
    }
}
/* (wave 8) SENDS: how much of this voice goes to the whole mix's DELAY, REVERB and CHORUS, three boxes over the knobs: the
 * echoes, the tail, the doubled wave, each as tall as its send (their settings are the FX button's pages: not drawn here) */
static void syn_send_box(uint32_t k, int32_t a, uint16_t c, uint16_t cd)   /* (U3: one box, k 0 DELAY 1 REVERB 2 CHORUS) */
{
    int32_t i, x, x0 = 4 + (int32_t)k * 60, hh = SP_B - SP_T - 4, h;
    a = clamp(a, 0, 127), h = hh * a / 127;
    op_box(x0, SP_T, 52, SP_B - SP_T + 1, a ? cd : TE_G2);
    if (!a) {
        cv_rect(x0 + 4, SP_B - 3, 44, 1, TE_G2);
        return;
    }
    if (k == 0u)                                        /* DELAY: the sound, then its echoes */
        for (i = 0, x = h; i < 5 && x > 0; i++, x = x * 2 / 3)
            cv_rect(x0 + 5 + i * 9, SP_B - 2 - x, 4, x, i ? cd : c);
    else if (k == 1u)                                   /* REVERB: a tail */
        for (i = 0; i < 44; i += 2)
            cv_rect(x0 + 4 + i, SP_B - 2 - h * (44 - i) / 44, 1, h * (44 - i) / 44 + 1, c);
    else {                                              /* CHORUS: the wave and its second voice */
        int32_t py = SP_M, py2 = SP_M;
        for (i = 0; i <= 44; i++) {
            int32_t v = SINE[((uint32_t)i * 23u) & 1023u], v2 = SINE[((uint32_t)i * 23u + 90u) & 1023u];
            int32_t y = SP_M - ((v * (h / 2)) >> 15), y2 = SP_M - ((v2 * (h / 2)) >> 15);
            if (i) {
                cv_line(x0 + 3 + i, py2, x0 + 4 + i, y2, cd);
                cv_line(x0 + 3 + i, py, x0 + 4 + i, y, c);
            }
            py = y, py2 = y2;
        }
    }
}
static void syn_sends(const track_t *t, uint16_t c, uint16_t cd)
{
    static const uint8_t ID[3] = {P_DLY, P_REV, P_CHOR};
    uint32_t k;
    for (k = 0; k < 3u; k++)
        syn_send_box(k, t->p[ID[k]], c, cd);
}
/* (wave 5) the grid (mock-synth-mod-A): the sources down, the destinations across (seven at a time, the cursor's half), a dot
 * a cell, as large as its amount, a ring when negative, the cursor framed white */
static void syn_matrix(const track_t *t, uint16_t c)
{
    uint8_t cols[MD_N];
    uint32_t ti = (uint32_t)(t - trk) % NPART, r, j, n = mx_vis(ti, cols), cp = mx_vis_pos(ti, (uint32_t)mx_col);
    uint32_t j0 = (uint32_t)clamp((int32_t)cp - 5, 0, (int32_t)n - 6);
    uint16_t cm = TE_MID[song.sel & 3u];                /* (review V8) the voice's colour, dim: the heads not at the cursor */
    char nm[8];
    for (j = j0; j < j0 + 6u && j < n; j++) {           /* (wave 11: an assignable column is named by its parameter, white-grey
                                                         * as every head; "+" the empty one that makes another) */
        mx_col_name(ti, cols[j], nm);
        cv_text_tight(38 + (int32_t)(j - j0) * 32 + (32 - cv_text_tight(0, 0, nm, 0, 0)) / 2, 0, nm,
                      j == cp ? c : cm, 1);   /* (review V8: the voice's colour, the cursor's bright, the rest dim) */
    }
    if (j0)                                             /* (more columns either side: the cursor slides the view) */
        cv_text(31, 0, &FONT_S, "<", cm);
    if (j0 + 6u < n)
        cv_text(232, 0, &FONT_S, ">", cm);
    for (r = 0; r < MS_N; r++) {
        int32_t y = 16 + (int32_t)r * 13;
        cv_text_tight(2, y - 2, N_MSRC[r], r == (uint32_t)mx_row ? c : cm, 1);
        for (j = j0; j < j0 + 6u && j < n; j++) {
            uint32_t d = cols[j];
            int32_t x = 38 + (int32_t)(j - j0) * 32 + 16, a = mx_get(ti, r, d), cy = y + 5, rad;
            if ((r == MS_LFO1 && d == MD_LVL) || (r == MS_KEY && d == MD_CUT))
                a = a / 2;                              /* (the one-way fixed cells: 0..127) */
            rad = a ? 1 + (a < 0 ? -a : a) * 4 / 64 : 0;
            if (!a)
                cv_rect(x, cy, 1, 1, TE_G2);
            else if (a > 0)
                cv_rect(x - rad, cy - rad, 2 * rad + 1, 2 * rad + 1, c);
            else
                op_box(x - rad, cy - rad, 2 * rad + 1, 2 * rad + 1, c);
            if (r == (uint32_t)mx_row && j == cp)
                op_box(x - 13, y - 1, 27, 13, c);
        }
    }
}

/* ---- (review 26 Nov, U3, Ben's picker "Each page its own picture") the drum screen's pictures: the synth pages' drawings,
 * in the drums' teal, in the drum screen's lower band (its rows 20 to 52, under the line that names the sound or the kit):
 * FLTR the filter's response (syn_filter_at), LFO the wave, SENDS the send boxes (syn_send_box) and the kit's KIT FX, the
 * kit's INS its tile and curve (gp_insert_in), SLICER the pattern (syn_slicer), KIT FLT the kit filter's curve. Worked out
 * from the values, as the synth pages' (no RAM, no rendering) */
#define DP_T 20
#define DP_B 52
static int32_t dpic_val(uint32_t l, uint32_t id, int32_t lo, int32_t hi)   /* sound l's id on a scale of lo..hi */
{
    const param_desc_t *d = dsp_desc(sl_mach(l), lane_snd(l), id);
    if (!d)
        return lo;
    return lo + (snd_val(l, id) - d->min) * (hi - lo) / (d->max > d->min ? d->max - d->min : 1);
}
static void dpic_filter(uint32_t l)
{
    uint32_t ty;
    int32_t cut, res, env = 0, drv;
    sp_t = DP_T, sp_b = DP_B;
    if (sl_mach(l)) {                                   /* CLASSIC: OFF LP BP HP, LP+ BP+ HP+ (the tone through it too) */
        static const uint8_t TY[7] = {VF_NTYPE, VF_LP12, VF_BP, VF_HP, VF_LP12, VF_BP, VF_HP};
        ty = TY[(uint32_t)snd_val(l, DSP_S0 + DS_FTYPE) % 7u];
        cut = dpic_val(l, DSP_S0 + DS_FREQ, 0, 127), res = dpic_val(l, DSP_S0 + DS_RES, 0, 127);
        env = snd_val(l, DSP_S0 + DS_FENV), drv = dpic_val(l, DSP_S0 + DS_DRIVE, 0, 127);
    } else {                                            /* HEX: LP HP BP UBP NTCH PEAK LP2 OFF (the band ones drawn as BP) */
        static const uint8_t TY[8] = {VF_LP12, VF_HP, VF_BP, VF_BP, VF_BP, VF_BP, VF_LP24, VF_NTYPE};
        ty = TY[(uint32_t)snd_val(l, DSP_FTYPE) % 8u];
        cut = dpic_val(l, DSP_CUT, 0, 127), res = dpic_val(l, DSP_RES, 0, 127), drv = dpic_val(l, DSP_DRIVE, 0, 127);
    }
    syn_filter_at(ty, cut, res, env, drv, 0, TE_DRUM, TE_DIM[3]);
    sp_t = 40, sp_b = 114;
}
static void dpic_lfo(uint32_t l)                        /* two cycles of the sound's LFO from where a hit starts it (the white
                                                         * square), as tall as its DEPTH (none: the shape, dim) */
{
    const dlfo_t *f = &pkit.s[l & 15u].lfo;
    int32_t x, py = 0, a, dep = f->dep, mag = dep < 0 ? -dep : dep;
    uint32_t ph0 = (uint32_t)f->sph << 25, step = 0xFFFFFFFFu / (uint32_t)((SP_R - SP_L) / 2);
    sp_t = DP_T, sp_b = DP_B;
    a = (SP_B - SP_T) / 2 - 1;
    if (mag)
        a = a * (mag < 16 ? 16 : mag) / 64;             /* (a quarter at least: the shape stays readable) */
    gp_dots(SP_M);
    for (x = SP_L; x <= SP_R; x++) {
        uint32_t ph = ph0 + (uint32_t)(x - SP_L) * step;
        int32_t w = f->wav % 5u == 4u ? gp_noise((x - SP_L) * 16 / (SP_R - SP_L) + 1)   /* (S&H: held steps) */
                                      : dlfo_wave(f->wav % 5u, ph, 0);
        int32_t y = SP_M - (dep < 0 ? -w : w) * a / 32768;
        if (x > SP_L)
            cv_line(x - 1, py, x, y, mag ? TE_DRUM : TE_DIM[3]);
        else
            cv_rect(x - 2, y - 2, 5, 5, C_WHITE);       /* (drawn first: the line runs from it) */
        py = y;
    }
    sp_t = 40, sp_b = 114;
}
static void dpic_sends(uint32_t l)                      /* DELAY, REVERB as a synth voice's; KIT FX: the kit's 16 sounds, those
                                                         * through its effects lit, this one bright (OFF: its tile empty) */
{
    uint32_t i, off = sl_kfx_off(l);
    int32_t x0 = 124;
    sp_t = DP_T, sp_b = DP_B;
    syn_send_box(0, (int32_t)sl_send(l, 1), TE_DRUM, TE_DIM[3]);
    syn_send_box(1, (int32_t)sl_send(l, 0), TE_DRUM, TE_DIM[3]);
    op_box(x0, SP_T, 52, SP_B - SP_T + 1, off ? TE_G2 : TE_DIM[3]);
    for (i = 0; i < DRUM_LANES; i++) {                  /* (the drum grid's places, as the map above) */
        uint32_t l2 = DRUM_GRID[i];
        int32_t x = x0 + 4 + (int32_t)(i % 4u) * 11, y = SP_T + 4 + (int32_t)(i / 4u) * 7;
        if (l2 == (l & 15u))
            (off ? op_box : cv_rect)(x, y, 9, 5, off ? TE_G3 : TE_DRUM);
        else
            cv_rect(x, y, 9, 5, sl_kfx_off(l2) ? TE_G1 : TE_DIM[3]);
    }
    sp_t = 40, sp_b = 114;
}
static void dpic_kit(uint32_t h)                        /* the kit's FX tab half h: INSERT, SLICER, FILTER */
{
    const track_t *t = TDRUM;
    sp_t = DP_T, sp_b = DP_B;
    if (h == 0u) {                                      /* INS: its tile (a synth FX slot's, the one there is), its curve */
        int32_t ty = clamp(t->p[P_ITYPE], 0, IT_N - 1);
        cv_rect(2, SP_T - 1, 77, SP_B - SP_T + 3, TE_DIM[3]);
        op_box(2, SP_T - 1, 77, SP_B - SP_T + 3, C_WHITE);
        cv_text(6, SP_T, &FONT_S, "INS", C_WHITE);
        if (ty)
            cv_rect(66, SP_T + 4, 7, 7, C_WHITE);       /* on */
        cv_text(6, SP_T + 16, &FONT_S, N_ITYPE[ty], ty ? C_WHITE : TE_G3);
        gp_insert_in(t, TE_DRUM, 92, SP_R);
    } else if (h == DBH_SLICER) {
        syn_slicer(t, TE_DRUM);
    } else {                                            /* KIT FLT: < 0 a low-pass closing, > 0 a high-pass opening */
        int32_t tf = t->p[P_TFLT];
        syn_filter_at(tf < 0 ? VF_LP12 : tf > 0 ? VF_HP : VF_NTYPE, tf < 0 ? 127 + tf * 127 / 64 : tf * 127 / 63, 0, 0, 0, 0,
                      TE_DRUM, TE_DIM[3]);
    }
    sp_t = 40, sp_b = 114;
}
static uint32_t dpic_sig(uint32_t l)                    /* what the pictures show, for the redraw */
{
    const track_t *t = TDRUM;
    uint32_t s = 0, i;
    const uint8_t *lf = (const uint8_t *)&pkit.s[l & 15u].lfo;
    for (i = 0; i < sizeof(dlfo_t); i++)
        s = s * 31u + lf[i];
    for (i = 0; i < DRUM_LANES; i++)
        s = s * 3u + sl_kfx_off(i);
    for (i = P_ITYPE; i <= P_IMIX; i++)
        s = s * 131u + (uint32_t)(uint16_t)t->p[i];
    for (i = P_SLCR; i <= P_SLDEPTH; i++)
        s = s * 131u + (uint32_t)(uint16_t)t->p[i];
    s = s * 131u + (uint32_t)(uint16_t)t->p[P_TFLT];
    if (drum_pg == DPG_FX && ((dpg_half() == 0u && t->p[P_ITYPE] >= IT_PHASER) || (dpg_half() == DBH_SLICER && t->p[P_SLCR])))
        s ^= (ui.frame / 2u + 1u) * 40503u;             /* (the INSERT's sweep runs, the SLICER's step) */
    return s;
}
#undef DP_T
#undef DP_B

/* the body of a sound page (240 x 109 at y 22): the subject large, a line, the drawing */
static void syn_body(track_t *t, const page_t *pg, uint32_t half, uint32_t *cache)
{
    uint32_t v = song.sel & 3u, st = (uint32_t)fam_stage(pg->fam), i, sig = 2166136261u, ty = vtype_of(t);
    uint16_t c = TE_COL[v];
    char big[16], sub[40];
    for (i = 0; i < P_COUNT; i++)
        sig = (sig ^ (uint32_t)(uint16_t)t->p[i]) * 16777619u;
    sig ^= (uint32_t)(pg - PAGES) * 7919u + half * 131u + t->eng_req * 31u + v * 104729u;
    sig ^= (uint32_t)mx_row * 3001u + (uint32_t)mx_col * 70001u + studio_hash(0, "") * 0u;   /* (wave 5: the grid's cursor, */
    {
        const mxt_t *m = &mxt[song.sel % NPART];                                         /* its cells, the globals shown) */
        for (i = 0; i < MX_NCELL; i++)
            sig = (sig ^ ((uint32_t)m->c[i].at << 8 | (uint8_t)m->c[i].amt)) * 16777619u;
        sig ^= (uint32_t)m->l2sync * 77u + (uint32_t)m->l2trig * 999u;
        for (i = 0; i < MX_NAS; i++)
            sig = (sig ^ m->as[i]) * 16777619u;
    }
    if (pg->graph == GR_LFO || pg->graph == GR_LFO2 || (pg->graph == GR_SLCR && t->p[P_SLCR]) ||
        (pg->graph == GR_XSLOT && (t->p[P_XS(pg_slot(pg), 0)] == XT_PHASER || t->p[P_XS(pg_slot(pg), 0)] == XT_FLANGER)))
        sig ^= (ui.frame / 2u + 1u) * 40503u;           /* (the LFO runs, the SLICER's step) */
    if (pg->graph == GR_INS && t->p[P_ITYPE] >= IT_PHASER)
        sig ^= (ui.frame + 1u) * 40503u;
    if (!ui.force && sig == *cache)
        return;
    *cache = sig;
    big[0] = sub[0] = 0;
    cv_begin(240, 109, C_BLACK);
    sp_t = 58, sp_b = 104;
    switch (st) {
    case 0:                                             /* OSC: the Type, its place in the list */
        str_cpy(big, ty < NVTYPE ? VTYPES[ty].big : "--", 16);
        str_cpy(sub, "TYPE ", 6);
        fmt_int(sub + 5, (int32_t)(ty < NVTYPE ? ty + 1u : 0u));
        str_cpy(sub + str_len(sub), "/", 2);
        fmt_int(sub + str_len(sub), (int32_t)NVTYPE);
        if (ty < NVTYPE && VTYPES[ty].eng == ENGI_PRISM && prism_model(t) >= PRM_PLX &&
            plx_stride(prism_model(t) - PRM_PLX) > eng_arena_size(t))
            str_cpy(sub + str_len(sub), "  NOT ON THIS VOICE", 20);   /* (the device build's limit: engines.c) */
        if (half == 1u) {                               /* OCT TUNE GLIDE VOICE: the pitch on a scale of +-3 octaves, the
                                                         * voice's ways */
            int32_t px = 120 + (t->p[P_OCT] * 1200 + t->p[P_TUNE]) * 108 / 3600, o;
            cv_rect(12, SP_T - 6, 216, 1, TE_G2);
            for (o = -3; o <= 3; o++)
                cv_rect(120 + o * 36, SP_T - 8, 1, o ? 3 : 5, TE_G3);
            cv_rect(px - 2, SP_T - 9, 5, 7, c);
            gp_voice(t, c);
        }
        else if (half >= 2u)                            /* (review V8) OSC 3/4 and 4/4: the voices, not page 1's wave again */
            syn_voices(t, c, TE_DIM[v]);
        else if (t->eng_req % NENGINES == ENGI_PRISM && (prism_model(t) == PRM_FOLD || prism_model(t) == PRM_WAVE ||
                                                         prism_model(t) == PRM_HARM))
            op_prism_panel(t, 6, SP_T - 6, 0);          /* (FOLD, WAVE, HARM: the model's own picture, across the width as
                                                         * every wave, review V8) */
        else if (t->eng_req % NENGINES == ENGI_PRISM && prism_model(t) < PRM_PLX)
            op_prism_panel(t, 80, SP_T - 6, 1);         /* (SWARM, CHORD: the model's own picture) */
        else
            gp_osc(t, c);
        break;
    case 1: {                                           /* FILT: its type, which ladder, which envelope */
        uint32_t f = (uint32_t)t->p[P_FTYPE] % VF_NTYPE, e = t->eng_req % NENGINES;
        static const char *const BIG[VF_NTYPE] = {"LP 24", "LP 12", "BP", "HP"};
        str_cpy(big, BIG[f], 16);
        str_cpy(sub, f == VF_LP24 && e == ENGI_ACID ? "303 LADDER" : f == VF_LP24 && e == ENGI_JUNO ? "IR3109 LADDER" : "LADDER",
                24);
        str_cpy(sub + str_len(sub), t->p[P_FLINK] ? "  ENV: OWN" : "  ENV: AMP", 12);
        syn_filter(t, c, TE_DIM[v]);                  /* (the envelope it follows: the line above says, mock-synth-filt-A) */
        break;
    }
    case 2: {                                           /* ENV: AMP or FILTER, the selector, the curve(s) */
        uint32_t filt = pg->id[0] == P_FATK, link = !t->p[P_FLINK];
        int32_t x;
        str_cpy(big, filt ? "FILTER" : "AMP", 16);
        str_cpy(sub, filt ? (link ? "LINKED TO THE AMP ENV" : "ITS OWN ADSR") : (link ? "THE FILTER FOLLOWS IT" : "THE VOICE'S LEVEL"),
                40);
        (void)x;                                        /* (review V4: no AMP | FILT chips: the tab row says it) */
        cv_rect(0, SP_B + 1, 240, 1, TE_G1);
        if (filt && !link)                              /* the other envelope faint behind (both shapes) */
            syn_adsr(t, P_ATK, SP_L, SP_R, SP_T, SP_B, TE_DIM[v], 0);
        if (!filt && !link)
            syn_adsr(t, P_FATK, SP_L, SP_R, SP_T, SP_B, TE_DIM[v], 0);
        syn_adsr(t, filt && !link ? P_FATK : P_ATK, SP_L, SP_R, SP_T, SP_B, filt && link ? TE_MID[v] : c, 1);
        break;
    }
    case 3:                                             /* FX: the page, what it does */
        str_cpy(big, pg->graph == GR_FX ? "SENDS" : pg->title, 16);
        if (pg->graph == GR_XSLOT) {                    /* (wave 5) the three slots, the one on the knobs, its effect */
            uint32_t k = pg_slot(pg);
            int32_t ty = clamp(t->p[P_XS(k, 0)], 0, XT_N - 1);
            syn_fx_slots(t, k, c, TE_DIM[v]);
            sp_t = 64, sp_b = 106;
            syn_fx_pic(t, k, c);
            if (!ty)
                str_cpy(sub, "EMPTY: SELECT PICKS A TYPE", 40);
            else
                str_cpy(sub, "IN SERIES", 40);
            cv_text(4, 45, &FONT_S, sub, TE_G4);
            sp_t = 40, sp_b = 114;
            cv_blit(0, 22);
            return;
        }
        if (pg->graph == GR_INS) {
            str_cpy(sub, N_ITYPE[clamp(t->p[P_ITYPE], 0, IT_N - 1)], 12);
            gp_insert(t, c);
        } else if (pg->graph == GR_SLCR) {
            str_cpy(sub, N_SLCR[clamp(t->p[P_SLCR], 0, 2)], 12);
            syn_slicer(t, c);
        } else if (pg->graph == GR_SENDS) {             /* (wave 8) the voice's own sends to the whole mix's effects */
            str_cpy(sub, "TO THE MIX'S FX", 40);
            syn_sends(t, c, TE_DIM[v]);
        } else {
            str_cpy(sub, "DIST CHORUS DELAY REVERB", 40);
            gp_sends(t, c);
        }
        break;
    default:                                            /* MOD: the LFOs, the matrix */
        if (pg->graph == GR_MATRIX) {                   /* (wave 5) the grid fills the body */
            syn_matrix(t, c);
            sp_t = 40, sp_b = 114;
            cv_blit(0, 22);
            return;
        }
        str_cpy(big, pg->graph == GR_LFO2 ? "LFO 2" : W_LFO1, 16);   /* (review V2: LFO 1) */
        if (pg->graph == GR_LFO2) {                     /* (wave 5) the second LFO, running */
            const mxt_t *m = &mxt[song.sel % NPART];
            int32_t x, py = SP_M, ny = SP_M, a;
            const char *u;
            char b[12];
            str_cpy(sub, N_LWAVE[clamp(t->p[P_L2WAVE], 0, 4)], 8);
            str_cpy(sub + str_len(sub), "  ", 3);
            if (m->l2sync > 0) {
                str_cpy(sub + str_len(sub), N_LSYNC[clamp(m->l2sync, 0, 10)], 8);
            } else {
                param_format(&TP[P_L2RATE], t->p[P_L2RATE], b, &u);
                str_cpy(sub + str_len(sub), b, 8);
                str_cpy(sub + str_len(sub), u, 4);
            }
            str_cpy(sub + str_len(sub), m->l2trig == 0 ? "  NOTE" : m->l2trig == 2 ? "  PATT" : "  FREE", 8);
            a = (SP_B - SP_T) / 2 - 2;
            gp_dots(SP_M);
            for (x = SP_L; x <= SP_R; x++) {
                uint32_t ph = m->l2ph + (uint32_t)(x - 160) * (0xFFFFFFFFu / 120u);
                int32_t y = SP_M - lfo_shape((uint32_t)t->p[P_L2WAVE] % 5u, ph, m->l2rnd) * a / 32768;
                if (x > SP_L)
                    cv_line(x - 1, py, x, y, c);
                if (x == 160)
                    ny = y;
                py = y;
            }
            cv_rect(157, ny - 3, 7, 7, C_WHITE);
        } else if (pg->graph == GR_LFO) {
            const char *u;
            char b[12];
            str_cpy(sub, N_LWAVE[clamp(t->p[P_LWAVE], 0, 4)], 8);
            str_cpy(sub + str_len(sub), "  ", 3);
            if (t->p[P_LSYNC]) {
                str_cpy(sub + str_len(sub), N_LSYNC[clamp(t->p[P_LSYNC], 0, 10)], 8);
            } else {
                param_format(&TP[P_LRATE], t->p[P_LRATE], b, &u);
                str_cpy(sub + str_len(sub), b, 8);
                str_cpy(sub + str_len(sub), u, 4);
            }
            syn_lfo(t, c);
        } else {
            str_cpy(sub, pg->id[0] == P_ED_PIT ? "WHERE THE ENV GOES" : "WHERE THE LFO GOES", 40);
            gp_bars(t, pg);
        }
        break;
    }
    cv_text(4, 2, &FONT_L, big, c);
    cv_text(4, 36, &FONT_S, sub, TE_G4);
    sp_t = 40, sp_b = 114;
    cv_blit(0, 22);
}

/* a synth's sound page, whole (ui_draw.c: on the chain, a synth track): header, body, tabs, dials, strip */
static void syn_page_draw(void)
{
    static uint32_t head, body, tabs, dials, bar;
    track_t *t = TSEL;
    const page_t *pg = cur_page();
    uint32_t st = (uint32_t)fam_stage(pg->fam), k, n = 0, h = 0, locked, dim, v = song.sel & 3u;
    static char vb[4][12];
    const char *lab[4], *val[4] = {vb[0], vb[1], vb[2], vb[3]};
    int32_t ratio[4];
    char tag[4] = {(char)('1' + v), 0}, name[16];
    for (k = 0; k < 8u && stage_page(st, k) < NPAGES; k++, n++)
        if (stage_page(st, k) == ui.page)
            h = k;
    op_names_str(t, name);
    if (map_lyr && map_vis) {                           /* (wave 11) HOLD TO MAP: the page as it is, its dials the amounts */
        static uint32_t mhead, mtabs, mdials;
        uint32_t s = map_src();
        char bn[16];
        map_chip(map_lyr);                              /* (MASTER's pair, as the wave 10 hold: its chip) */
        op_header(map_lyr == LY_LFO ? "LFO" : "ENV", TE_COL[v], name, &mhead);
        hdr_mchip = 0;
        syn_body(t, pg, h, &body);
        str_cpy(bn, "MAP ", sizeof bn);
        str_cpy(bn + 4, map_src_name(s), 10);
        nav_map_hint = map_other_name(s);
        nav_tabs(TE_COL[v], bn, &mtabs);
        nav_map_hint = 0;
        map_dials(pg, s, TE_COL[v], &mdials);
        op_trackbar(&bar);
        head = tabs = dials = 0;
        return;
    }
    op_header(tag, TE_COL[v], mlu.on ? "LEARN" : name, &head);
    syn_body(t, pg, h, &body);
    nav_tabs(TE_COL[v], 0, &tabs);                    /* (wave 9: the group's categories, the page in it) */
    syn_vals(pg, -1, lab, vb, ratio, &locked, &dim);
    dial_mark = (uint8_t)map_marks(pg);               /* (wave 11) a knob something is mapped onto: its mark */
    op_dials(lab, val, ratio, locked, dim, TE_COL[v], ui.page * 977u + song.sel * 31u, &dials);
    dial_mark = 0;
    op_trackbar(&bar);
}

#include "ui_sel.c"                                      /* (wave 9) SELECT: the type knob; on the drum screen the sound */
