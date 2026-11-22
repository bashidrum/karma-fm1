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
 *                  RATCH NUDGE), the locked value inverted; OCT- the lock of the knob last turned, again every lock */
/* ---- the values */
static void syn_fmt_type(char *b, uint32_t ty) { str_cpy(b, ty < NVTYPE ? VTYPES[ty].name : "--", 8); }
/* page pg's four knobs on the selected synth track: labels, values (a step held, first: its lock, inverted), dials */
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
        lab[k] = id == V_TYPE ? "TYPE" : d->label;
        if (id == V_TYPE) {
            uint32_t ty = vtype_of(t);
            syn_fmt_type(v[k], ty);
            ratio[k] = (int32_t)(ty < NVTYPE ? ty : 0u) * 1000 / (int32_t)(NVTYPE - 1u);
            if (first >= 0)
                *dim |= 1u << k;                        /* (not lockable: the Rytm's machine, grey with a step held) */
            continue;
        }
        x = *vp;
        if (id >= P_FATK && id <= P_FREL && !t->p[P_FLINK]) {   /* LINK: the amp ENV's, dim (the first turn: OWN) */
            x = t->p[P_ATK + (id - P_FATK)];
            *dim |= 1u << k;
        }
        if (first >= 0 && id < P_COUNT) {
            int l = lock_find(t, (uint32_t)first, id, 0);
            if (l >= 0) {
                x = t->lock[l].val;
                *locked |= 1u << k;
            }
            if (!p_lockable(id))
                *dim |= 1u << k;
        }
        param_format(d, x, v[k], &u);
        dval_fit(v[k]);
        ratio[k] = d->max > d->min ? (x - d->min) * 1000 / (d->max - d->min) : -1;
    }
}
/* TRIG's knobs: a step held, its NOTE LEN VEL COND (half 2: FLAG RATCH NUDGE); nothing held, NOTE (the pen) STEP HIT VEL */
static void syn_trig_vals(int32_t first, uint32_t half, const char *lab[4], char v[4][12], int32_t ratio[4], uint32_t *dim)
{
    static const char *const LA[4] = {"NOTE", "LEN", "VEL", "COND"}, *const LB[4] = {"FLAG", "RATCH", "NUDGE", ""};
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
    if (half) {                                         /* FLAG RATCH NUDGE */
        uint32_t f = (st->flags & SF_ACCENT ? 1u : 0u) | (st->flags & SF_SLIDE ? 2u : 0u);
        if (first < 0) {
            for (k = 0; k < 3u; k++)
                str_cpy(v[k], "--", 4), *dim |= 1u << k;
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

/* ---- the tabs (as the drum screen's): the names, the one shown inverted in the voice's colour, the half at the right;
 * from: the first tab drawn (1: no TRIG, a sound page) */
static void syn_tabs(uint32_t from, uint32_t sel, uint32_t h, uint32_t nh, const char *banner, uint32_t *cache)
{
    uint32_t i, v = song.sel & 3u, sig = from * 7u + sel * 31u + h * 131u + nh * 977u + v * 7919u;
    uint16_t col = TE_COL[v];
    int32_t x = 4;
    sig = studio_hash(sig, banner ? banner : "");
    if (!ui.force && sig == *cache)
        return;
    *cache = sig;
    cv_begin(240, 23, C_BLACK);
    cv_rect(0, 0, 240, 1, TE_G1);
    if (banner) {                                       /* (STEP REC: what the knobs are) */
        cv_rect(2, 4, text_w(&FONT_S, banner) + 4, 17, col);
        cv_text(4, 5, &FONT_S, banner, C_BLACK);
    } else {
        for (i = from; i < 6u; i++) {
            int32_t w = text_w(&FONT_S, SYN_TAB[i]);
            if (i == sel) {
                cv_rect(x - 2, 4, w + 4, 17, col);
                cv_text(x, 5, &FONT_S, SYN_TAB[i], C_BLACK);
            } else {
                cv_text(x, 5, &FONT_S, SYN_TAB[i], i ? TE_G4 : TE_G3);
            }
            x += w + (from ? 12 : 9);             /* (the GRID's six: closer) */
        }
        if (nh > 1u) {
            char b[6];
            fmt_int(b, (int32_t)h + 1);
            str_cpy(b + str_len(b), "/", 2);
            fmt_int(b + str_len(b), (int32_t)nh);
            cv_text(236 - text_w(&FONT_S, b), 5, &FONT_S, b, TE_G3);
        }
    }
    cv_blit(0, 131);
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
/* the filter's response at TYPE: CUTOFF (0..127), RESO, the ENV amount's sweep (shaded, as far as it opens it) */
static void syn_filter(const track_t *t, uint16_t c, uint16_t cd)
{
    const int16_t *p = t->p;
    uint32_t ty = (uint32_t)p[P_FTYPE] % VF_NTYPE;
    int32_t w = SP_R - SP_L - 20, y0 = SP_T + 12 - p[P_FDRV] * 8 / 127, xc = SP_L + 10 + p[P_FCUT] * w / 127;   /* (DRIVE:
                                                         * the passband lifted, louder into the ladder) */
    int32_t r = p[P_FRES] * (SP_B - SP_T - 8) / 127, env = p[P_ED_FLT], xe = clamp(xc + env * w / 64, SP_L, SP_R), x;
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
        if (ty == VF_HP || ty == VF_BP)
            v += dd < 0 ? -dd * k / 8 : 0;
        if (ty != VF_HP)
            v += dd > 0 ? dd * k / 8 : 0;
        if (ty == VF_BP)
            v += 6;
        if (dd > -16 && dd < 16)
            v -= r * (256 - dd * dd) / 256;
        gp_to(x, y0 + v, c);
    }
    cv_rect(xc - 1, SP_B - 3, 3, 4, C_WHITE);           /* where CUTOFF is */
    if (p[P_FKEY])                                      /* KEY: the cutoff follows the note, a slope through it */
        cv_line(xc - 14, SP_B - 2, xc + 14, SP_B - 2 - p[P_FKEY] * 12 / 127, C_WHITE);
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
/* the body of a sound page (240 x 109 at y 22): the subject large, a line, the drawing */
static void syn_body(track_t *t, const page_t *pg, uint32_t half, uint32_t *cache)
{
    uint32_t v = song.sel & 3u, st = (uint32_t)fam_stage(pg->fam), i, sig = 2166136261u, ty = vtype_of(t);
    uint16_t c = TE_COL[v];
    char big[16], sub[40];
    for (i = 0; i < P_COUNT; i++)
        sig = (sig ^ (uint32_t)(uint16_t)t->p[i]) * 16777619u;
    sig ^= (uint32_t)(pg - PAGES) * 7919u + half * 131u + t->eng_req * 31u + v * 104729u;
    if (pg->graph == GR_LFO || (pg->graph == GR_SLCR && t->p[P_SLCR]))
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
            str_cpy(sub + str_len(sub), "  NOT ON THIS TRACK", 20);   /* (the device build's limit: engines.c) */
        if (half == 1u) {                               /* OCT TUNE GLIDE VOICE: the pitch on a scale of +-3 octaves, the
                                                         * voice's ways */
            int32_t px = 120 + (t->p[P_OCT] * 1200 + t->p[P_TUNE]) * 108 / 3600, o;
            cv_rect(12, SP_T - 6, 216, 1, TE_G2);
            for (o = -3; o <= 3; o++)
                cv_rect(120 + o * 36, SP_T - 8, 1, o ? 3 : 5, TE_G3);
            cv_rect(px - 2, SP_T - 9, 5, 7, c);
            gp_voice(t, c);
        }
        else if (t->eng_req % NENGINES == ENGI_PRISM && prism_model(t) < PRM_PLX)
            op_prism_panel(t, 80, SP_T - 6, 1);         /* (FOLD, SWARM, WAVE, HARM, CHORD: the model's own picture) */
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
        x = 236 - text_w(&FONT_S, "FILT") - 6;          /* AMP | FILT, the one shown filled */
        if (filt) {
            op_box(x - text_w(&FONT_S, "AMP") - 12, 4, text_w(&FONT_S, "AMP") + 8, 18, TE_G3);
            cv_text(x - text_w(&FONT_S, "AMP") - 8, 5, &FONT_S, "AMP", TE_G3);
            cv_rect(x, 4, text_w(&FONT_S, "FILT") + 6, 18, c);
            cv_text(x + 3, 5, &FONT_S, "FILT", C_BLACK);
        } else {
            cv_rect(x - text_w(&FONT_S, "AMP") - 12, 4, text_w(&FONT_S, "AMP") + 8, 18, c);
            cv_text(x - text_w(&FONT_S, "AMP") - 8, 5, &FONT_S, "AMP", C_BLACK);
            op_box(x, 4, text_w(&FONT_S, "FILT") + 6, 18, TE_G3);
            cv_text(x + 3, 5, &FONT_S, "FILT", TE_G3);
        }
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
        if (pg->graph == GR_INS) {
            str_cpy(sub, N_ITYPE[clamp(t->p[P_ITYPE], 0, IT_N - 1)], 12);
            gp_insert(t, c);
        } else if (pg->graph == GR_SLCR) {
            str_cpy(sub, N_SLCR[clamp(t->p[P_SLCR], 0, 2)], 12);
            syn_slicer(t, c);
        } else {
            str_cpy(sub, "DIST CHORUS DELAY REVERB", 40);
            gp_sends(t, c);
        }
        break;
    default:                                            /* MOD: the LFO, where it and the ENV go */
        str_cpy(big, pg->graph == GR_LFO ? "LFO" : pg->id[0] == P_ED_PIT ? "ENV" : "LFO", 16);
        if (pg->graph == GR_LFO) {
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
    op_header(tag, TE_COL[v], mlu.on ? "LEARN" : name, &head);
    syn_body(t, pg, h, &body);
    syn_tabs(1, st + 1u, h, n, 0, &tabs);
    syn_vals(pg, -1, lab, vb, ratio, &locked, &dim);
    op_dials(lab, val, ratio, locked, dim, TE_COL[v], ui.page * 977u + song.sel * 31u, &dials);
    op_trackbar(&bar);
}
