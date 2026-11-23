/* SPDX-License-Identifier: GPL-3.0-only */
/* The drum screen, the Analog Rytm MKII's way (PHEBUS wave 3; Ben, 21 Nov 2026: "Drums: the Analog Rytm MKII's easy drum
 * programming. Pick a sound, 16 trig keys, hold a trig and turn to lock, sound pages in the Rytm's order (SRC, FLTR, AMP,
 * LFO), mute, fill, retrig, chromatic"; pickers "Drum pages: B: four dials over the knobs", "Locks: Rytm way", "Make every
 * kit editable"). Included by ui_studio.c after ui_grid.c (the step keys, the steps held, the pages of steps are the GRID's).
 *
 *   the screen     the selected sound's name, large; the 4 x 4 sound map (a tile a sound: dim with trigs, lit as it
 *                  plays, framed white when selected, a yellow M muted, an ice-blue S soloed); its 16 trigs of the page
 *                  (a lock: a square in it; a retrig: lines; a condition: a notch; held: framed white; the playhead
 *                  under); what the held trig holds; the tabs TRIG SRC FLTR AMP LFO FX and the half (1/2); four dials,
 *                  each over its knob: its name and its value, one size in fixed columns, a locked value inverted
 *   the keys       the 16 steps of the page of the selected sound (as every GRID); black keys 1 .. 4 the page of steps,
 *                  F#5 FILL while held; OCT+ with F#5 held (or F#5 with OCT+ held) latches it, F#5 again lets it go
 *                  (wave 8: PERFORM's latch, one gesture; it was a double press)
 *   the pages      EDIT: SRC, again the next page (FLTR AMP LFO FX); ENV: AMP, LFO: LFO (again: the page's next half); SEQ:
 *                  TRIG (again: its next half); SELECT walks every half of every page; they change the page with a trig
 *                  held too. FX (wave 8, Ben 22 Nov 2026: "FX = whole mix only"; the drum voice's bus insert "stays its
 *                  own"): the drum voice's own FILTER, INSERT, INSERT 2, SENDS, SLICER, the Rytm's FX track; a trig held,
 *                  its knobs lock them on the step as any tab's (it was the FX button with a trig held)
 *   the knobs      nothing held: the sound's value (the first turn on a built-in kit makes the project's own copy,
 *                  drum_snd.c); a trig held: the value LOCKED on that trig (TRIG: the trig's own VEL RETRIG NUDGE COND,
 *                  CHANCE LENGTH CURVE NOTE). OCT- with a trig held: the lock of the knob last turned goes, again (or
 *                  nothing turned): every lock of the trig; on TRIG: its condition, chance, nudge, retrig and every lock
 *   SEL held       the 16 keys pick the sound (the Rytm's TRACK + pad); in MUTE mode they solo it
 *   SEL tapped     MUTE mode: the 16 keys mute / unmute the 16 sounds (again: back)
 *   ARP tapped     CHROMATIC mode: the 27 keys play the selected sound at their pitches, OCT- / OCT+ the octave; recorded
 *                  live, each trig keeps its pitch (a NOTE lock). Again: back
 *   ARP held       RETRIG: a key retrigs its sound at the rate KNOB 1 or a black key picks (1/4 .. 1/80) */
#define DPG_TABS 6u                                     /* TRIG SRC FLTR AMP LFO FX */
#define DPG_FX 5u
static uint8_t drum_pg = 1;                             /* 0 TRIG, 1 SRC, 2 FLTR, 3 AMP, 4 LFO (drum_snd.c DPG_* + 1), 5 FX */
static uint8_t drum_half[DPG_TABS];                     /* each page's half shown */
static uint8_t drum_pg_last = 1;                        /* the sound page last shown (EDIT, ENV, LFO come back to it) */
static uint8_t drum_newvel = DT_VEL_DEF;                /* the velocity a new trig takes (TRIG, nothing held: KNOB 1) */
static uint8_t dk_touch = 0xFF;                         /* a trig held: the id of the knob last turned in this hold */
static const char *const DPG_NAME[DPG_TABS] = {"TRIG", "SRC", "FLTR", "AMP", "LFO", "FX"};
/* (wave 8) the FX tab's halves: the drum voice's own pages (params.c PAGES: FILTER, INSERT, INSERT 2 and SENDS of FAM_DRM, the
 * SLICER every voice has), by name once */
#define NDBUS 8u                                        /* (wave 9: then the whole mix's, the FX group's MIX) */
static uint8_t dbus_pg[NDBUS];
static uint32_t dbus_page(uint32_t h)                   /* the FX tab's half h: its PAGES index */
{
    static const char *const T[NDBUS] = {"FILTER", "INSERT", "INSERT 2", "SENDS", "SLICER", "DELAY", "REV/CHO", "MASTER"};
    static const uint8_t F[NDBUS] = {FAM_DRM, FAM_DRM, FAM_DRM, FAM_DRM, FAM_DRM, FAM_MFX, FAM_MFX, FAM_MFX};
    if (!dbus_pg[0] && !dbus_pg[1]) {
        uint32_t i, k;
        for (k = 0; k < NDBUS; k++)
            for (i = 0; i < NPAGES; i++)
                if (PAGES[i].fam == F[k] && str_eq(PAGES[i].title, T[k]))
                    dbus_pg[k] = (uint8_t)i;
    }
    return dbus_pg[h % NDBUS];
}

static uint32_t dpg_halves(uint32_t pg)
{
    uint32_t f;
    return pg == 0u ? TH_N : pg == DPG_FX ? NDBUS : dh_count(drum_model(), pg - 1u, &f);
}
static uint32_t dpg_half(void)
{
    uint32_t n = dpg_halves(drum_pg);
    return drum_half[drum_pg % DPG_TABS] < n ? drum_half[drum_pg % DPG_TABS] : 0u;
}
static uint32_t dpg_id(uint32_t k) { return drum_pg && drum_pg < DPG_FX ? dh_id(drum_model(), drum_pg - 1u, dpg_half(), k) : 0xFFu; }
static void dpg_go(uint32_t pg, uint32_t half)
{
    if (pg)
        drum_pg_last = (uint8_t)pg;
    drum_pg = (uint8_t)(pg % DPG_TABS);
    drum_half[drum_pg] = (uint8_t)half;
    ui.force = 1;
}
static void drum_screen_open(void)                      /* the drum screen (from anywhere), the GRID's keys */
{
    if (!on_drum_page() || song.sel != TRK_DRUM)
        studio_open(SC_DRUM);
    if (stl_view)                                       /* (wave 9: left by a page button: its held steps let go too) */
        stl_held = stl_pend = stl_fresh = 0;
    stl_view = 0;
}
/* ---- the values */
static int32_t snd_val(uint32_t l, uint32_t id)          /* sound l's value of id, as it is (no lock) */
{
    return dsp_get(drum_model(), lane_snd(l), lane_lfo(l), l, id);
}
static int32_t lk_val(uint32_t id, uint32_t v)           /* a lock's byte as the value it holds (signed ones) */
{
    return id == DSP_FINE || id == DSP_NOTE || id == DSP_LDEP || id == DSP_LFAD || id == DSP_S0 + DS_FENV ||
           id == DSP_S0 + DS_LEVEL ? (int32_t)(int8_t)v : (int32_t)v;
}
/* the knob of id turned s, nothing held: the selected sound's value. A built-in kit: its project copy made first (a copy
 * of another kit there: it asks, wave 8: hold REC until the ring fills replaces it). The sends are the sound's (not the
 * kit's) */
static void kit_replace(void)                           /* (the question confirmed: ui_input.c ask_do) */
{
    if (pkit_on() || !pkit_other())
        return;
    pkit_make();
    ui_message("YOUR COPY OF THE KIT");
}
static void drum_param_set(uint32_t id, int32_t v);
static void drum_param_turn(uint32_t id, int32_t s, uint32_t role)
{
    uint32_t l = drum_lane & 15u, model = drum_model();
    const param_desc_t *d = dsp_desc(model, lane_snd(l), id);
    if (!d)
        return;
    drum_param_set(id, clamp(snd_val(l, id) + accel(role, s, d->max - d->min), d->min, d->max));
}
/* the selected sound's id set to v (in range): a send, or the kit's own copy (made on the first edit) */
static void drum_param_set(uint32_t id, int32_t v)
{
    uint32_t l = drum_lane & 15u;
    if (id == DSP_REV || id == DSP_DEL) {
        drum_send[l][id - DSP_REV] = (uint8_t)v;
        return;
    }
    if (!pkit_on()) {
        if (pkit_other()) {
            char b[20] = "REPLACE ";
            str_cpy(b + 8, DRUM_KIT_NAMES[pkit.src % DRUM_KITS], 10);
            str_cpy(b + str_len(b), "*", 2);
            if (ui.ask != ASK_KIT)
                ask_set(ASK_KIT, 0, b);                 /* (one kit's copy a project: the other one's edits would go) */
            return;
        }
        pkit_make();                                    /* the project's own copy of this kit, then the edit in it */
        ui_message("YOUR COPY OF THE KIT");
    }
    fm1_irq_off();
    dsp_put(pkit.model, pkit.snd[l], &pkit.lfo[l], id, v);
    pkit_gen++;
    fm1_irq_on();
}
/* the trigs held (the selected sound's): the lock of id moved s (made at the sound's value first). All or none */
static void drum_lock_turn(uint32_t id, int32_t s, uint32_t role)
{
    uint32_t w, l = drum_lane & 15u, len = trk_len(TDRUM), need = 0, model = drum_model();
    const param_desc_t *d = dsp_desc(model, lane_snd(l), id);
    int32_t sk;
    if (!d)
        return;
    if (id == DSP_MODEL) {                              /* (as the Rytm's machine: not lockable) */
        ui_message("NOT LOCKABLE");
        return;
    }
    sk = accel(role, s, d->max - d->min);
    for (w = 0; w < 16u; w++) {
        uint32_t idx = sg_page() * 16u + w;
        need += ((sg_held >> w) & 1u) && idx < len && dt_has(idx, l) && dt_lk(idx, l, id) < 0;
    }
    if (need > dt_free_lk()) {
        ui_message("SOUND LOCKS FULL: 128");
        return;
    }
    sg_undo(TDRUM);
    fm1_irq_off();
    for (w = 0; w < 16u; w++) {
        uint32_t idx = sg_page() * 16u + w;
        int k;
        int32_t v;
        if (!((sg_held >> w) & 1u) || idx >= len || !dt_has(idx, l))
            continue;
        k = dt_lk(idx, l, id);
        v = clamp((k >= 0 ? lk_val(id, dpat.lk[k].val) : snd_val(l, id)) + sk, d->min, d->max);
        (void)dt_lk_set(idx, l, id, (uint32_t)(uint8_t)v);
    }
    fm1_irq_on();
    sg_pend &= (uint16_t)~sg_held;                      /* (edited: kept when let go) */
    dk_touch = (uint8_t)id;
    sync_reload = 1;
}
/* TRIG with trigs held: knob k of the half shown (0 VEL 1 RETRIG 2 NUDGE 3 COND, 4 CHANCE 5 LENGTH 6 CURVE, 7 NOTE: a
 * pitch lock, as chromatic mode records) */
static void drum_trig_turn(uint32_t k, int32_t s)
{
    uint32_t w, len = trk_len(TDRUM), sl = dpg_half() * 4u + k;
    if (sl == 7u) {
        drum_lock_turn(DSP_NOTE, s, EN_K4);
        return;
    }
    sg_undo(TDRUM);
    sg_pend &= (uint16_t)~sg_held;
    fm1_irq_off();
    for (w = 0; w < 16u; w++) {
        uint32_t idx = sg_page() * 16u + w;
        if ((sg_held >> w) & 1u && idx < len)
            sg_drum_knob(idx, sl, s);
    }
    fm1_irq_on();
    sync_reload = 1;
}
/* OCT- with trigs held: on a sound page the lock of the knob last turned in this hold (none: every lock of the trig); on
 * TRIG all the trig holds (condition, chance, nudge, retrig, every lock, the drum voice's own locks on the step). OCT+:
 * the next condition (sg_held_oct) */
static void drum_held_oct(uint32_t up)
{
    uint32_t w, len = trk_len(TDRUM), l = drum_lane & 15u, n = 0, one = drum_pg && dk_touch != 0xFFu;
    char b[24];
    if (up)                                             /* (wave 9: OCT+ with a step held is no gesture: its condition is
                                                         * TRIG's KNOB 4 COND; a combo with no obvious link, removed) */
        return;
    if (drum_pg == DPG_FX) {                            /* (wave 8) the FX tab: the drum voice's own locks of the half shown */
        const page_t *pg = &PAGES[dbus_page(dpg_half())];
        if (!up && pg->scope != SC_TRACK) {
            ui_message("NO LOCK ON THIS PAGE");
            return;
        }
        sg_lpage = (uint8_t)(pg - PAGES);
        sg_held_oct(up);
        sg_lpage = 0xFF;
        return;
    }
    if (!drum_pg) {                                     /* (wave 10) TRIG: the hit itself goes, with all it holds */
        sg_held_erase();
        return;
    }
    if (sg_lpage < NPAGES) {
        sg_held_oct(up);
        return;
    }
    sg_undo(TDRUM);
    sg_pend &= (uint16_t)~sg_held;
    fm1_irq_off();
    for (w = 0; w < 16u; w++) {
        uint32_t idx = sg_page() * 16u + w;
        if ((sg_held >> w) & 1u && idx < len && dt_has(idx, l))
            n += dt_lk_del(idx, l, one ? dk_touch : DSP_ALL);
    }
    fm1_irq_on();
    sync_reload = 1;
    if (!n && !one) {                                   /* (wave 10) no lock left: the hit itself goes */
        sg_held_erase();
        return;
    }
    if (one && n) {
        const param_desc_t *d = dsp_desc(drum_model(), lane_snd(l), dk_touch);
        str_cpy(b, d ? d->label : "", 8);
        ui_say("LOCK OFF: ", b);
    } else {
        fmt_int(b, (int32_t)n);
        ui_say(b, n == 1u ? " LOCK OFF" : " LOCKS OFF");
    }
    dk_touch = 0xFF;                                    /* (OCT- again: every lock) */
}

/* ---- drawing */
static int32_t cv_text_tall(int32_t x, int32_t y, const char *s, uint16_t c)   /* FONT_S drawn twice as tall */
{
    const felucca_font_t *f = &FONT_S;
    for (; *s; s++) {
        uint32_t gi = glyph(f, (uint8_t)*s), gx, gy, w = f->bw[gi];
        const uint8_t *gd = f->data + f->off[gi];
        for (gy = 0; gy < f->h * 2u; gy++)
            for (gx = 0; gx < w; gx++) {
                uint32_t i = (gy >> 1) * w + gx;
                if ((gd[i >> 3] >> (7u - (i & 7u))) & 1u)
                    cv_pset(x - f->pad + (int32_t)gx, y + (int32_t)gy, c);
            }
        x += f->adv[gi];
    }
    return x;
}
static int32_t text_w_tall(const char *s) { return text_w(&FONT_S, s); }
static uint16_t drum_tile_col(uint32_t l)               /* a sound's tile in the map: lit as it plays, dim with trigs */
{
    uint32_t i, len = trk_len(TDRUM), has = 0;
    for (i = 0; i < len && !has; i++)
        has = dt_has(i, l);
    return pad_lit[l] ? TE_DRUM : has ? TE_MID[3] : TE_G1;
}
/* the 16 sounds as a 4 x 4 map at (x0, y0), tiles w x h on a pitch of px, py */
static void drum_map(int32_t x0, int32_t y0, int32_t w, int32_t h, int32_t px, int32_t py)
{
    uint32_t l;
    for (l = 0; l < DRUM_LANES; l++) {
        int32_t x = x0 + (int32_t)(l % 4u) * px, y = y0 + (int32_t)(l / 4u) * py;
        cv_rect(x, y, w, h, drum_tile_col(l));
        if ((drum_mute >> l) & 1u)
            cv_rect(x + w - 5, y + 2, 3, h - 4, TE_MUTE);
        else if ((drum_solo >> l) & 1u)
            cv_rect(x + w - 5, y + 2, 3, h - 4, TE_SOLO);
        if (l == drum_lane)
            op_box(x - 1, y - 1, w + 2, h + 2, C_WHITE);
    }
}
static uint32_t drum_map_sig(void)
{
    uint32_t l, s = drum_mute * 7u + drum_solo * 131u + drum_lane;
    for (l = 0; l < DRUM_LANES; l++)
        s = s * 3u + (drum_tile_col(l) == TE_DRUM ? 2u : drum_tile_col(l) == TE_MID[3] ? 1u : 0u);
    return s;
}
/* sound l's trigs of page bank in a row at y (cells w x h on a pitch of p from x0); held: framed; the playhead under */
static void drum_trig_row(uint32_t l, uint32_t bank, int32_t x0, int32_t y, int32_t w, int32_t h, int32_t p, int held)
{
    uint32_t j, len = trk_len(TDRUM), pl = song.playing ? TDRUM->seq_idx % len : 0xFFFFu;
    for (j = 0; j < 16u; j++) {
        uint32_t idx = bank * 16u + j;
        int32_t x = x0 + (int32_t)j * p, xi;
        if (idx >= len)
            continue;
        if (dt_has(idx, l)) {
            cv_rect(x, y, w, h, vel_col(dt_vel(idx, l)));
            if (dt_lk_n(idx, l))                        /* a lock: a square in it (the Rytm's lock trig) */
                cv_rect(x + w / 2 - 2, y + 4, 5, 5, C_BLACK);
            xi = dt_x(idx, l);
            if (xi >= 0 && dpat.x[xi].rrate) {          /* a retrig: three lines */
                uint32_t r;
                for (r = 0; r < 3u; r++)
                    cv_rect(x + 2, y + h - 10 + (int32_t)r * 3, w - 4, 1, C_BLACK);
            }
            if (xi >= 0 && dpat.x[xi].cond) {           /* a condition: a notch */
                cv_rect(x + w - 3, y, 3, 3, C_BLACK);
            }
        } else {
            op_box(x, y, w, h, j % 4u ? TE_G2 : TE_G3);
        }
        if (held && ((sg_held >> j) & 1u))
            op_box(x - 1, y - 1, w + 2, h + 2, C_WHITE);
        if (idx == pl)
            cv_rect(x, y + h + 3, w, 2, C_WHITE);
        else if (!(j % 4u))
            cv_rect(x, y + h + 3, 2, 2, TE_G3);         /* (the quarters) */
    }
}
static uint32_t drum_trig_sig(uint32_t l, uint32_t bank)
{
    uint32_t j, len = trk_len(TDRUM), s = l * 7919u + bank * 31u + len;
    for (j = 0; j < 16u; j++) {
        uint32_t idx = bank * 16u + j;
        int xi = dt_x(idx, l);
        s = s * 31u + dt_vel(idx, l) + dt_lk_n(idx, l) * 977u + (xi >= 0 ? dpat.x[xi].rrate * 13u + dpat.x[xi].cond * 7u : 0u);
    }
    return s + sg_held * 3u + (song.playing ? TDRUM->seq_idx % len + 1u : 0u) * 65537u;
}
/* the dials (PHEBUS wave 3; wave 4: every voice's, in its colour): label over each knob, a ring, the value in one tall face
 * (a locked one inverted); the touched knob's label inverted and its value underlined. Each knob a 60 px column: the ring
 * at its left, the value in the 40 px after it (DIAL_VX .. + DIAL_VW). Five letters there take a narrower step (7 px, the
 * others 8), so no value reaches the next column's ring (wave 3 had "PITCH" against it, Ben's drum LFO page); longer
 * ones are cut to five (dval_fit). dial_over counts any drawn past its column (tests/ui_pages_test.c) */
#define DIAL_VX 17                                      /* the value's start in its column */
#define DIAL_VW 40                                      /* its room */
static uint32_t dial_over;                              /* values drawn past their column (0, always) */
static uint32_t dials_y = SG_KNOB_Y;                    /* where the dials go (PHEBUS wave 6: PERFORM's lower) */
static const uint16_t *dial_tint;                       /* (wave 7) a colour a knob (HOME, the mixer); 0: col for all four */
static int32_t dial_adv(const char *s) { return str_len(s) >= 5u ? 7 : 8; }
static int32_t dial_w(const char *s) { return (int32_t)str_len(s) * dial_adv(s); }
static int32_t cv_text_tall_n(int32_t x, int32_t y, const char *s, uint16_t c, int32_t adv)   /* FONT_S x2 tall, adv a letter */
{
    const felucca_font_t *f = &FONT_S;
    for (; *s; s++) {
        uint32_t gi = glyph(f, (uint8_t)*s), gx, gy, w = f->bw[gi];
        const uint8_t *gd = f->data + f->off[gi];
        for (gy = 0; gy < f->h * 2u; gy++)
            for (gx = 0; gx < w; gx++) {
                uint32_t i = (gy >> 1) * w + gx;
                if ((gd[i >> 3] >> (7u - (i & 7u))) & 1u)
                    cv_pset(x - f->pad + (int32_t)gx, y + (int32_t)gy, c);
            }
        x += adv;
    }
    return x;
}
static void op_dials(const char *const lab[4], const char *const val[4], const int32_t ratio[4], uint32_t locked,
                     uint32_t dim, uint16_t col, uint32_t sig, uint32_t *cache)
{
    uint32_t k;
    if (dial_tint)
        col = (uint16_t)(dial_tint[0] ^ dial_tint[1] * 3u ^ dial_tint[2] * 5u ^ dial_tint[3] * 7u);
    sig = studio_hash(sig + locked * 97u + dim * 7u + col * 131u, ui.msg_t ? ui.msg : "");
    for (k = 0; k < 4u; k++) {
        sig = studio_hash(sig * 7u + (uint32_t)ratio[k] + (ui.hot_t && ui.hot_col == k) * 5003u, lab[k]);
        sig = studio_hash(sig, val[k]);
    }
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
        int32_t x0 = 2 + 60 * (int32_t)k, vx = x0 + DIAL_VX, vw;
        uint32_t hot = ui.hot_t && ui.hot_col == k, lk = (locked >> k) & 1u;
        uint16_t c = (dim >> k) & 1u ? TE_G3 : (dim >> (4u + k)) & 1u ? TE_G4 : dial_tint ? dial_tint[k] : col;   /* (wave 5: dim bits 4..7, a shared
                                                         * value on a voice's page: white-grey, not the voice's colour) */
        if (!lab[k][0])
            continue;
        if (hot) {
            cv_rect(x0, 4, text_w(&FONT_S, lab[k]) + 2, 16, c);
            cv_text(x0 + 1, 4, &FONT_S, lab[k], C_BLACK);
        } else {
            cv_text(x0 + 1, 4, &FONT_S, lab[k], c);
        }
        te_dial(x0 + 7, 38, 6, ratio[k], c, TE_G2);
        vw = dial_w(val[k]);
        dial_over += vw > DIAL_VW - 3;                  /* (its ink and a clear gap: wave 3's 40 px "PITCH" touched the next ring) */
        if (lk) {                                       /* locked on the trig held: inverted (the Rytm's way) */
            cv_rect(vx - 3, 21, vw + 5, 34, c);
            cv_text_tall_n(vx, 22, val[k], C_BLACK, dial_adv(val[k]));
        } else {
            cv_text_tall_n(vx, 22, val[k], c, dial_adv(val[k]));
        }
        if (hot)
            cv_rect(vx - 1, 56, vw + 1, 2, c);
    }
    cv_blit(0, dials_y);
}
static void drum_dials(const char *const lab[4], const char *const val[4], const int32_t ratio[4], uint32_t locked,
                       uint32_t dim, uint32_t sig, uint32_t *cache)
{
    op_dials(lab, val, ratio, locked, dim, TE_DRUM, sig, cache);
}
/* a value made to fit its column (40 px: five letters of the tall face, narrower) */
static void dval_fit(char *b)
{
    while (str_len(b) > 5u)
        b[str_len(b) - 1u] = 0;
}

/* the drum screen (the GRID, chromatic mode): header, name, map, trigs, tabs, dials, strip */
static void drum_main_draw(void)
{
    static uint32_t head, sa, sb, sc, foot, bar;
    uint32_t l = drum_lane & 15u, len = trk_len(TDRUM), bank = sg_page(), model = drum_model(), i, sig;
    int32_t first = sg_held ? sg_first() : -1;
    int chrom = drum_keys == DK_CHROM;
    char sub[28], info[32];
    {   /* the header: DRUM, the kit (a * when the project's copy plays), CHROM in chromatic mode */
        char nm[12];
        uint32_t kit = drum_kit();
        str_cpy(nm, DRUM_KIT_NAMES[kit], 10);
        if (kit >= DRUM_SYN) {
            memcpy(nm, dsu_kit(kit - DRUM_SYN)->name, 8);
            nm[8] = 0;
        }
        if (pkit_on())
            str_cpy(nm + str_len(nm), "*", 2);
        op_header("DRUM", TE_DRUM, nm, &head);
    }
    /* the name, what is held, the map */
    if (first >= 0) {
        uint32_t n = 0, w;
        for (w = 0; w < 16u; w++)
            n += (sg_held >> w) & 1u;
        str_cpy(sub, "STEP ", 6);                       /* (wave 7: a key is a step, as the synth GRID and the lane say) */
        fmt_int(sub + 5, first + 1);
        str_cpy(sub + str_len(sub), n > 1u ? " +" : " HELD", 6);
        if (n > 1u) {
            fmt_int(sub + str_len(sub), (int32_t)n - 1);
            str_cpy(sub + str_len(sub), " HELD", 6);
        }
    } else if (chrom) {
        str_cpy(sub, "CHROMATIC  OCT ", sizeof sub);
        fmt_int(sub + str_len(sub), drum_chr_oct);
    } else {                                            /* (wave 9) the kit here: the header's title place is SELECT's */
        uint32_t n = 0, kit = drum_kit();
        str_cpy(sub, DRUM_KIT_NAMES[kit], 10);
        if (kit >= DRUM_SYN) {
            memcpy(sub, dsu_kit(kit - DRUM_SYN)->name, 8);
            sub[8] = 0;
        }
        if (pkit_on())
            str_cpy(sub + str_len(sub), "*", 2);
        str_cpy(sub + str_len(sub), "  ", 3);
        for (i = 0; i < len; i++)
            n += (uint32_t)dt_has(i, l);
        fmt_int(sub + str_len(sub), (int32_t)n);
        str_cpy(sub + str_len(sub), n == 1u ? " TRIG" : " TRIGS", 8);
    }
    sig = studio_hash(drum_map_sig() * 31u + l, sub) + ((drum_mute >> l) & 1u) * 7u + ((drum_solo >> l) & 1u) * 11u;
    if (ui.force || sig != sa) {
        sa = sig;
        cv_begin(240, 54, C_BLACK);
        {
            char nm[10];
            str_cpy(nm, LANE_NAME[l], 9);
            while (text_w(&FONT_L, nm) > 146)
                nm[str_len(nm) - 1u] = 0;
            cv_text(4, 2, &FONT_L, nm, TE_DRUM);
        }
        i = (uint32_t)cv_text(4, 36, &FONT_S, sub, C_WHITE);
        if ((drum_mute >> l) & 1u)
            op_badge((int32_t)i + 6, 37, 'M', TE_MUTE);
        else if ((drum_solo >> l) & 1u)
            op_badge((int32_t)i + 6, 37, 'S', TE_SOLO);
        drum_map(155, 4, 18, 9, 21, 12);
        cv_blit(0, SG_BODY_Y);
    }
    /* the trigs (chromatic: the keys as a keyboard), what the held trig holds, the pages of steps */
    info[0] = 0;
    if (first >= 0) {
        int xi = dt_x((uint32_t)first, l);
        uint32_t k, n = 0;
        if (xi >= 0 && dpat.x[xi].rrate) {
            str_cpy(info, "RETRIG ", 8);               /* (wave 7: the word every screen uses; was RTRG) */
            str_cpy(info + 7, SG_RATE[dpat.x[xi].rrate % 18u], 6);
            str_cpy(info + str_len(info), "  ", 3);
        }
        for (k = 0; k < DT_NLK; k++)
            if (dpat.lk[k].step == (uint32_t)first && dpat.lk[k].lane == l) {
                const param_desc_t *d = dsp_desc(model, lane_snd(l), dpat.lk[k].param);
                if (!d)
                    continue;
                if (!n++)
                    str_cpy(info + str_len(info), "LOCK ", 6);
                if (n <= 2u) {
                    str_cpy(info + str_len(info), d->label, 7);
                    str_cpy(info + str_len(info), " ", 2);
                }
            }
        if (n > 2u) {
            str_cpy(info + str_len(info), "+", 2);
            fmt_int(info + str_len(info), (int32_t)n - 2);
        }
        if (n && text_w(&FONT_S, info) > 180) {         /* (wave 7: never into the page slots: the locks counted instead) */
            char *lp = info;
            while (*lp && memcmp(lp, "LOCK ", 5))
                lp++;
            str_cpy(lp, "LOCKS ", 7);
            fmt_int(lp + 6, (int32_t)n);
        }
    } else if (chrom) {
        str_cpy(info, "KEYS PLAY ", sizeof info);
        str_cpy(info + str_len(info), LANE_SHORT[l], 8);
    } else {
        if (drum_pg == DPG_FX) {                        /* (wave 8) the FX tab: which of the drum voice's own pages */
            str_cpy(info, PAGES[dbus_page(dpg_half())].title, 10);
            str_cpy(info + str_len(info), "  ", 3);
        }
        fmt_int(info + str_len(info), (int32_t)bank * 16 + 1);
        str_cpy(info + str_len(info), "-", 2);
        fmt_int(info + str_len(info), (int32_t)(bank * 16u + 16u < len ? bank * 16u + 16u : len));
        str_cpy(info + str_len(info), " OF ", 5);
        fmt_int(info + str_len(info), (int32_t)len);
    }
    sig = studio_hash(chrom ? fm1_in.notes * 7u + 3u : drum_trig_sig(l, bank), info) + (uint32_t)chrom * 99991u;
    if (ui.force || sig != sb) {
        sb = sig;
        cv_begin(240, 55, C_BLACK);
        if (chrom) {                                    /* the 27 keys: white 15 px, black over them; the sound's own pitch
                                                         * dotted; a key down lit */
            static const int8_t W[12] = {0, -1, 1, -1, 2, -1, 3, 4, -1, 5, -1, 6};
            uint32_t k;
            for (k = 0; k < 27u; k++)
                if (W[k % 12u] >= 0) {
                    int32_t x = ((int32_t)(k / 12u) * 7 + W[k % 12u]) * 15;
                    uint32_t on = (fm1_in.notes >> k) & 1u;
                    cv_rect(x + 1, 0, 13, 28, on ? TE_DRUM : TE_G3);
                    if (k == CHR_KEY0)
                        te_disc(x + 7, 22, 2, on ? C_BLACK : TE_DRUM);
                }
            for (k = 0; k < 27u; k++)
                if (W[k % 12u] < 0) {
                    int32_t x = ((int32_t)(k / 12u) * 7 + W[(k - 1u) % 12u]) * 15 + 10;
                    uint32_t on = (fm1_in.notes >> k) & 1u;
                    cv_rect(x, 0, 10, 17, on ? TE_DRUM : C_BLACK);
                    op_box(x, 0, 10, 17, TE_G3);
                }
        } else {
            drum_trig_row(l, bank, 1, 1, 13, 26, 15, 1);
        }
        cv_text(4, 36, &FONT_S, info, first >= 0 ? TE_DRUM : TE_G3);
        for (i = 0; i < 4u && !chrom; i++) {            /* the pages of steps: used filled, the one shown underlined */
            int32_t x = 186 + (int32_t)i * 13;
            if (i * 16u < len)
                cv_rect(x, 41, 11, 5, i == bank ? TE_DRUM : TE_MID[3]);
            else
                op_box(x, 41, 11, 5, TE_G2);
            if (i == bank)
                cv_rect(x, 48, 11, 1, C_WHITE);
        }
        cv_blit(0, 76);
    }
    /* (wave 9) the tabs: the categories of the button's group, the one shown filled, the page in it (ui_nav.c) */
    nav_tabs(TE_DRUM, 0, &sc);
    /* the dials */
    {
        static char v[4][12];
        const char *lab[4] = {"", "", "", ""}, *val[4] = {v[0], v[1], v[2], v[3]}, *u;
        int32_t ratio[4] = {-1, -1, -1, -1};
        uint32_t k, locked = 0, dim = 0;
        for (k = 0; k < 4u; k++)
            v[k][0] = 0;
        if (drum_pg == DPG_FX) {                        /* (wave 8) the FX tab: the drum voice's own pages; a trig held, the
                                                         * values locked on its step (its sends: not lockable, grey) */
            const page_t *pg = &PAGES[dbus_page(dpg_half())];
            for (k = 0; k < 4u; k++) {
                uint32_t id = pg->id[k];
                int16_t *hv;
                const param_desc_t *d = page_desc(pg, k, &hv);
                int32_t x = -1, vv;
                if (!d || !hv || id == 0xFFu)
                    continue;
                lab[k] = d->label;
                if (pg->scope == SC_TRACK && first >= 0)
                    x = lock_find(TDRUM, (uint32_t)first, id, 0);
                vv = x >= 0 ? TDRUM->lock[x].val : *hv;
                locked |= (uint32_t)(x >= 0) << k;
                if (pg->scope != SC_TRACK && first >= 0)
                    dim |= 1u << k;
                param_format(d, vv, v[k], &u);
                dval_fit(v[k]);
                ratio[k] = (vv - d->min) * 1000 / (d->max > d->min ? d->max - d->min : 1);
            }
        } else if (!drum_pg && dpg_half() >= TH_PATT) { /* (wave 9) PATTERN: GATE (--) DIV SWING STEPS (wave 10: SHIFT, A-D) */
            patt_vals(TDRUM, dpg_half() - TH_PATT, lab, v, ratio, &dim);
        } else if (!drum_pg) {                          /* TRIG: the held trig's own values (nothing held: VEL for new) */
            static const char *const TL[8] = {"VEL", "RETRIG", "NUDGE", "COND", "CHANCE", "LENGTH", "CURVE", "NOTE"};
            uint32_t h = dpg_half();
            int xi = first >= 0 ? dt_x((uint32_t)first, l) : -1;
            dtx_t c;
            memset(&c, 0, sizeof c);
            c.prob = DT_PROB_DEF;
            c.rlen = DT_RLEN_DEF;
            if (xi >= 0)
                c = dpat.x[xi];
            for (k = 0; k < 4u; k++) {
                uint32_t sl = h * 4u + k;
                lab[k] = TL[sl];
                if (first < 0 && sl) {
                    str_cpy(v[k], "--", 4);
                    dim |= 1u << k;
                    continue;
                }
                switch (sl) {
                case 0: { uint32_t vel = first >= 0 ? dt_vel((uint32_t)first, l) : drum_newvel;
                          fmt_int(v[k], (int32_t)vel); ratio[k] = (int32_t)vel * 1000 / 127; break; }
                case 1: str_cpy(v[k], SG_RATE[c.rrate % 18u], 6); ratio[k] = c.rrate * 1000 / 17; break;
                case 2: fmt_int(v[k], c.micro); if (c.micro > 0) { v[k][0] = '+'; fmt_int(v[k] + 1, c.micro); }
                        ratio[k] = (c.micro + 32) * 1000 / 63; break;
                case 3: str_cpy(v[k], SG_COND[c.cond & 3u], 6); ratio[k] = c.cond * 333; break;
                case 4: fmt_int(v[k], c.prob); str_cpy(v[k] + str_len(v[k]), "%", 2); ratio[k] = c.prob * 10;
                        dim |= (uint32_t)(c.cond != FC_CHANCE) << k; break;
                case 5: sg_rlen_str(v[k], c.rlen); ratio[k] = c.rlen ? c.rlen * 1000 / 255 : 1000;
                        dim |= (uint32_t)(!c.rrate) << k; break;
                case 6: fmt_int(v[k], c.rfade); ratio[k] = (c.rfade + 128) * 1000 / 255;
                        dim |= (uint32_t)(!c.rrate || !c.rlen) << k; break;
                default: {
                    int lk = dt_lk((uint32_t)first, l, DSP_NOTE);
                    int32_t nv = lk >= 0 ? lk_val(DSP_NOTE, dpat.lk[lk].val) : 0;
                    param_format(&DNOTE_DESC, nv, v[k], &u);
                    locked |= (uint32_t)(lk >= 0) << k;
                    ratio[k] = (nv + 48) * 1000 / 96;
                    break;
                }
                }
                dval_fit(v[k]);
            }
        } else {                                        /* a sound page: the sound's values, a held trig's locks inverted */
            for (k = 0; k < 4u; k++) {
                uint32_t id = dpg_id(k);
                const param_desc_t *d = id != 0xFFu ? dsp_desc(model, lane_snd(l), id) : 0;
                int32_t vv;
                int lk = -1;
                if (!d)
                    continue;
                lab[k] = d->label;
                vv = snd_val(l, id);
                if (first >= 0 && (lk = dt_lk((uint32_t)first, l, id)) >= 0)
                    vv = lk_val(id, dpat.lk[lk].val);
                locked |= (uint32_t)(lk >= 0) << k;
                dsp_format(id, d, vv, v[k], &u);
                dval_fit(v[k]);
                ratio[k] = (vv - d->min) * 1000 / (d->max > d->min ? d->max - d->min : 1);
                if (id == DSP_MODEL && first >= 0)
                    dim |= 1u << k;                     /* (not lockable: grey with a trig held) */
            }
        }
        if (drum_pg == DPG_FX && PAGES[dbus_page(dpg_half())].fam == FAM_MFX)   /* (the whole mix's: white-grey) */
            op_dials(lab, val, ratio, locked, dim, TE_G4, drum_pg * 977u + dpg_half() * 31u + pkit_gen, &foot);
        else
            drum_dials(lab, val, ratio, locked, dim, drum_pg * 977u + dpg_half() * 31u + pkit_gen, &foot);
    }
    op_trackbar(&bar);
}

/* MUTE mode (the mock "drum mute"): the 16 sounds as tiles, a muted one dark with a yellow M (a soloed one an ice-blue S),
 * the keys' jobs, the selected sound's trigs */
static void drum_mute_draw(void)
{
    static uint32_t head, sa, sb, bar;
    uint32_t l, sig = drum_map_sig() * 7u + drum_trig_sig(drum_lane & 15u, sg_page());
    op_header("DRUM", TE_DRUM, "MUTE", &head);
    if (ui.force || sig != sa) {
        sa = sig;
        cv_begin(240, 124, C_BLACK);
        for (l = 0; l < DRUM_LANES; l++) {
            int32_t x = 2 + (int32_t)(l % 4u) * 60, y = 3 + (int32_t)(l / 4u) * 25;
            uint32_t m = (drum_mute >> l) & 1u, so = (drum_solo >> l) & 1u, sel = l == drum_lane, i, j;
            char nm[8];
            for (i = j = 0; LANE_SHORT[l][i] && j < 7u; i++)    /* (the name without its space when a badge needs the room) */
                if (LANE_SHORT[l][i] != ' ' || !(m || so))
                    nm[j++] = LANE_SHORT[l][i];
            nm[j] = 0;
            if (m) {                                    /* muted: dark, the name grey, a yellow M */
                cv_rect(x, y, 56, 22, TE_G1);
                cv_text(x + 1, y + 3, &FONT_S, nm, TE_G3);
            } else {
                if (sel || pad_lit[l])
                    cv_rect(x, y, 56, 22, sel ? TE_DRUM : TE_MID[3]);
                else
                    op_box(x, y, 56, 22, TE_DRUM);
                if (so)
                    cv_text(x + 1, y + 3, &FONT_S, nm, sel || pad_lit[l] ? C_BLACK : TE_DRUM);
                else
                    te_text_c(x + 28, y + 3, nm, sel || pad_lit[l] ? C_BLACK : TE_DRUM);
            }
            if (m || so)                                /* the badge at the tile's right edge (as the strip's) */
                op_badge(x + 45, y + 4, m ? 'M' : 'S', m ? TE_MUTE : TE_SOLO);
        }
        cv_text(4, 104, &FONT_S, "KEYS: MUTE", TE_G3);
        cv_text(236 - text_w(&FONT_S, "SEL+KEY: SOLO"), 104, &FONT_S, "SEL+KEY: SOLO", TE_G3);
        cv_blit(0, SG_BODY_Y);
    }
    if (ui.force || sig != sb) {
        sb = sig;
        cv_begin(240, 66, C_BLACK);
        cv_rect(0, 0, 240, 1, TE_G1);
        drum_trig_row(drum_lane & 15u, sg_page(), 1, 7, 13, 26, 15, 0);
        cv_text(4, 45, &FONT_S, LANE_NAME[drum_lane & 15u], TE_DRUM);
        cv_blit(0, 146);
    }
    op_trackbar(&bar);
}

/* ---- input (the drum track on the drum screen; the synth GRID's and the STUTTER lane's: ui_studio.c) */
static void drum_keys_fix(void)                         /* once a frame: the modes are the drum screen's */
{
    if (drum_keys != DK_PLAY && (!on_drum_page() || song.sel != TRK_DRUM || stl_shown()))
        drum_keys = DK_PLAY;
    if (!sg_held)
        dk_touch = 0xFF;
    if (dlk_full) {
        dlk_full = 0;
        ui_message("SOUND LOCKS FULL: 128");
    }
}
static void drum_grid_input(uint32_t pressed)
{
    uint32_t k, ob = 1u << panel.btn[B_OCTDN], pb = 1u << panel.btn[B_OCTUP];
    int32_t s;
    if (drum_keys == DK_CHROM) {                        /* (wave 10) chromatic: -/+ the sound before / after (its octave: a
                                                         * double tap, ui_input.c pm_input) */
        if (pressed & (ob | pb)) {
            int32_t l = clamp((int32_t)(drum_lane & 15u) + ((pressed & pb) ? 1 : -1), 0, DRUM_LANES - 1);
            drum_lane = (uint8_t)l;
            ui.force = 1;
        }
    } else if (drum_keys == DK_PLAY && (pressed & (ob | pb))) {
        uint32_t up = (pressed & pb) != 0u, pages = (trk_len(TDRUM) + 15u) / 16u, pg;
        if (sg_held) {
            drum_held_oct(up);
        } else if (!drum_pg && dpg_half() == TH_AD) {   /* (wave 10) SEQ > A-D: the pattern before / after */
            int32_t p = patt_shown();
            patt_pick(clamp((p < 0 ? 0 : p) + (up ? 1 : -1), 0, 3));
        } else if (up && sg_page() + 1u == pages && pages < 4u) {
            sg_page_add(TDRUM);
        } else {
            pg = (sg_page() + (up ? 1u : pages - 1u)) % pages;
            sg_cur_set((uint32_t)clamp((int32_t)(pg * 16u + sg_cur() % 16u), 0, (int32_t)trk_len(TDRUM) - 1));
            ui.force = 1;
        }
    }
    if ((s = panel_enc(EN_SELECT)) != 0)                /* (wave 9) SELECT: the sound (ui_sel.c) */
        sel_turn(s);
    if ((s = panel_enc(EN_PRESET)) != 0 && !sg_held)
        TDRUM->p[P_E0] = drum_kit_step(TDRUM->p[P_E0], s);
    if (sg_held)
        (void)panel_enc(EN_ALGO);
    for (k = 0; k < 4u; k++) {
        uint32_t id;
        if ((s = panel_enc(EN_K1 + k)) == 0)
            continue;
        ui.hot_col = (uint8_t)k;
        ui.hot_t = 40;
        if (drum_pg == DPG_FX) {                        /* (wave 8) the FX tab: the drum voice's own pages */
            const page_t *pg = &PAGES[dbus_page(dpg_half())];
            if (pg->id[k] == 0xFFu)
                continue;
            if (sg_held && pg->scope != SC_TRACK)
                ui_message("NOT LOCKABLE");
            else if (sg_held)
                sg_lock_turn(pg->id[k], s, EN_K1 + k);
            else
                edit_param_pg(pg, k, s);
        } else if (!drum_pg && dpg_half() >= TH_PATT) { /* (wave 9) PATTERN: the pattern's settings */
            if (sg_held)
                ui_message("NOT LOCKABLE");
            else
                patt_turn(TDRUM, dpg_half() - TH_PATT, k, s);
        } else if (!drum_pg) {
            if (sg_held)
                drum_trig_turn(k, s);
            else if (!k)
                drum_newvel = (uint8_t)clamp(drum_newvel + accel(EN_K1, s, 126), 1, 127);
            else
                ui_message("HOLD A STEP");             /* (wave 7: a key is a step) */
        } else if ((id = dpg_id(k)) != 0xFFu) {
            if (sg_held)
                drum_lock_turn(id, s, EN_K1 + k);
            else
                drum_param_turn(id, s, EN_K1 + k);
        }
    }
}
