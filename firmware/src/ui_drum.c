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
static uint8_t map_lyr, map_vis;                        /* (ui_map.c: LFO held to map) */
static const char *nav_map_hint;
static const char *hdr_mchip;                          /* (ui_op.c) */
static void map_chip(uint32_t l);
static void dmap_dials(uint32_t *cache);                /* (ui_kit.c) */
static uint32_t dmap_marks(void);
/* (drum_newvel, the velocity a new trig and a drum key take: seq.c; TRIG, nothing held: KNOB 1) */
static uint8_t dk_touch = 0xFF;                         /* a trig held: the id of the knob last turned in this hold */
static const char *const DPG_NAME[DPG_TABS] = {"TRIG", "SRC", "FLTR", "AMP", "LFO", "FX"};
/* (wave 8) the FX tab's halves: the drum voice's own pages (params.c PAGES: INSERT, INSERT 2, FILTER of FAM_DRM, the SLICER
 * every voice has), by name once. (The drum effects under FX, Ben 23 Nov 2026, 21:30 to 21:45, "Just mirror the synths as
 * closely as possible") in a synth voice's FX order: INS (the kit's INSERT, INSERT 2), SENDS (the SELECTED SOUND's own:
 * DELAY, REVERB, KIT FX; a sound page, not a PAGES one), SLICER (the kit's), then the whole mix's DELAY, REV/CHO, MASTER.
 * The kit's FILTER is the last half: EDIT's third category (Ben 21:40, "FILTER should live under Edit"), not FX's. The
 * kit's pages say KIT in the header and SELECT does nothing there (it picks a sound they do not act on) */
#define NDBUS 8u                                        /* (wave 9: then the whole mix's, the FX group's MIX) */
#define DBH_SENDS 2u                                    /* the selected sound's SENDS */
#define DBH_MIX 4u                                      /* the whole mix's first (DELAY) */
#define DBH_FILTER 7u                                   /* the kit's FILTER (EDIT's KIT FLT) */
static uint8_t dbus_pg[NDBUS];
static uint32_t dbus_page(uint32_t h)                   /* the FX tab's half h: its PAGES index (SENDS: none, 0) */
{
    static const char *const T[NDBUS] = {"INSERT", "INSERT 2", "", "SLICER", "DELAY", "REV/CHO", "MASTER", "FILTER"};
    static const uint8_t F[NDBUS] = {FAM_DRM, FAM_DRM, FAM_DRM, FAM_DRM, FAM_MFX, FAM_MFX, FAM_MFX, FAM_DRM};
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
static int dpg_sends(void) { return drum_pg == DPG_FX && dpg_half() == DBH_SENDS; }   /* the sound's SENDS (FX) */
static int dpg_snd(void) { return (drum_pg && drum_pg < DPG_FX) || dpg_sends(); }    /* a page of the selected sound */
static int dpg_bus(void) { return drum_pg == DPG_FX && dpg_half() != DBH_SENDS; }     /* a PAGES page: the kit's, the mix's */
static int dpg_kit(void) { return dpg_bus() && (dpg_half() < DBH_MIX || dpg_half() == DBH_FILTER); }   /* the KIT's */
static uint32_t dpg_id(uint32_t k)
{
    static const uint8_t SENDS[4] = {DSP_DEL, DSP_REV, DSP_KFX, 0xFF};   /* (a synth voice's SENDS: DELAY REVERB CHORUS) */
    if (dpg_sends())
        return SENDS[k & 3u];
    return drum_pg && drum_pg < DPG_FX ? dh_id(drum_model(), drum_pg - 1u, dpg_half(), k) : 0xFFu;
}
static uint32_t kfx_count(void)                         /* the sounds through the kit's effects (KIT FX ON) */
{
    uint32_t l, n = 0;
    for (l = 0; l < DRUM_LANES; l++)
        n += !sl_kfx_off(l);
    return n;
}
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
/* the knob of id turned s, nothing held: the selected slot's value (wave 12: the project's own sound, edited directly; the
 * sends are the slot's too) */
static void slot_edited(uint32_t l);                    /* (ui_kit.c) */
static void snd_browse(int32_t s);
static int brw_draw(void);
static uint8_t brw_t;
static char brw_l1[28];
static void drum_param_set(uint32_t id, int32_t v);
static void drum_param_turn(uint32_t id, int32_t s, uint32_t role)
{
    uint32_t l = drum_lane & 15u, model = drum_model();
    const param_desc_t *d = dsp_desc(model, lane_snd(l), id);
    if (!d)
        return;
    drum_param_set(id, clamp(snd_val(l, id) + accel(role, s, d->max - d->min), d->min, d->max));
}
/* the selected slot's id set to v (in range) */
static void drum_param_set(uint32_t id, int32_t v)
{
    uint32_t l = drum_lane & 15u;
    fm1_irq_off();
    if (id == DSP_REV || id == DSP_DEL)
        sl_send_set(l, id - DSP_REV, (uint32_t)v);
    else if (id == DSP_KFX)
        sl_kfx_set(l, v == 0);
    else
        dsp_put(sl_mach(l), pkit.s[l].snd, &pkit.s[l].lfo, id, v);
    slot_edited(l);
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
    if (id == DSP_MODEL || id == DSP_KFX) {             /* (as the Rytm's machine: not lockable; KIT FX: the slot's) */
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
    if (dpg_bus()) {                                    /* (wave 8) the FX tab: the drum voice's own locks of the half shown */
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
static uint8_t drum_map_nosel;                          /* (a kit page: the map frames no sound) */
/* the 16 sounds as a 4 x 4 map at (x0, y0), tiles w x h on a pitch of px, py: the drum grid (drums.c DRUM_GRID) */
static void drum_map(int32_t x0, int32_t y0, int32_t w, int32_t h, int32_t px, int32_t py)
{
    uint32_t i;
    for (i = 0; i < DRUM_LANES; i++) {
        uint32_t l = DRUM_GRID[i];
        int32_t x = x0 + (int32_t)(i % 4u) * px, y = y0 + (int32_t)(i / 4u) * py;
        cv_rect(x, y, w, h, drum_tile_col(l));
        if ((drum_mute >> l) & 1u)
            cv_rect(x + w - 5, y + 2, 3, h - 4, TE_MUTE);
        else if ((drum_solo >> l) & 1u)
            cv_rect(x + w - 5, y + 2, 3, h - 4, TE_SOLO);
        if (l == drum_lane && !drum_map_nosel)
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
 * ones are cut to six, set a pixel closer (dval_fit, review V2). dial_over counts any drawn past its column (tests/ui_pages_test.c) */
#define DIAL_VX 17                                      /* the value's start in its column */
#define DIAL_VW 40                                      /* its room */
static uint32_t dial_over;                              /* values drawn past their column (0, always) */
static uint32_t dials_y = SG_KNOB_Y;                    /* where the dials go (PHEBUS wave 6: PERFORM's lower) */
static const uint16_t *dial_tint;                       /* (wave 7) a colour a knob (HOME, the mixer); 0: col for all four */
static uint8_t dial_mark;                               /* (wave 11) a bit a knob: something is mapped onto it (ui_map.c) */
static int32_t dial_adv(const char *s) { return str_len(s) >= 5u ? 7 : 8; }
static int32_t dial_w(const char *s) { return (int32_t)str_len(s) * dial_adv(s); }
/* (review V2: the dictionary's words have six letters at most) six letters start 2 px nearer the ring and keep 4 px clear of
 * the next one; fewer keep the old 7 */
static int32_t dial_vx(const char *s) { return str_len(s) >= 6u ? DIAL_VX - 2 : DIAL_VX; }
static int dial_wide(const char *s) { return dial_vx(s) + dial_w(s) > (str_len(s) >= 6u ? 57 : DIAL_VX + DIAL_VW - 3); }
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
/* the 27 keys as a piano (CHROMATIC's; review V8: ARP > KEY's too): white 15 px, black over them; the keys in on lit in col,
 * the unlit white ones grey; a dot on the keys in dot (CHROMATIC: the sound's own pitch; KEY: the root) */
static void piano_draw(int32_t y0, int32_t wh, int32_t bh, uint32_t on, uint32_t dot, uint16_t col, uint16_t off)
{
    static const int8_t W[12] = {0, -1, 1, -1, 2, -1, 3, 4, -1, 5, -1, 6};
    uint32_t k;
    for (k = 0; k < 27u; k++)
        if (W[k % 12u] >= 0) {
            int32_t x = ((int32_t)(k / 12u) * 7 + W[k % 12u]) * 15;
            uint32_t o = (on >> k) & 1u;
            cv_rect(x + 1, y0, 13, wh, o ? col : off);
            if ((dot >> k) & 1u)
                te_disc(x + 7, y0 + wh - 6, 2, o ? C_BLACK : col);
        }
    for (k = 0; k < 27u; k++)
        if (W[k % 12u] < 0) {
            int32_t x = ((int32_t)(k / 12u) * 7 + W[(k - 1u) % 12u]) * 15 + 10;
            uint32_t o = (on >> k) & 1u;
            cv_rect(x, y0, 10, bh, o ? col : C_BLACK);
            op_box(x, y0, 10, bh, off);
            if ((dot >> k) & 1u)
                te_disc(x + 5, y0 + bh - 5, 2, o ? C_BLACK : col);
        }
}
static void op_dials(const char *const lab[4], const char *const val[4], const int32_t ratio[4], uint32_t locked,
                     uint32_t dim, uint16_t col, uint32_t sig, uint32_t *cache)
{
    uint32_t k;
    if (dial_tint)
        col = (uint16_t)(dial_tint[0] ^ dial_tint[1] * 3u ^ dial_tint[2] * 5u ^ dial_tint[3] * 7u);
    sig = studio_hash(sig + locked * 97u + dim * 7u + col * 131u + dial_mark * 40503u, ui.msg_t ? ui.msg : "");
    sig = studio_hash(sig * 3u + (brw_t != 0u), brw_t ? brw_l1 : "");   /* (wave 12: the PRESETS readout) */
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
    if (brw_draw()) {                                   /* (wave 12) PRESETS turning: what it browses, here on every screen */
        cv_blit(0, dials_y);
        return;
    }
    for (k = 0; k < 4u; k++) {
        int32_t x0 = 2 + 60 * (int32_t)k, vx = x0 + dial_vx(val[k]), vw;
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
        if ((dial_mark >> k) & 1u) {                    /* (wave 11) mapped: a small wave mark under the ring, the voice's */
            cv_rect(x0 + 3, 49, 2, 2, c), cv_rect(x0 + 5, 47, 2, 2, c), cv_rect(x0 + 7, 49, 2, 2, c), cv_rect(x0 + 9, 51, 2, 2, c);
            cv_rect(x0 + 11, 49, 2, 2, c);
        }
        vw = dial_w(val[k]);
        dial_over += dial_wide(val[k]);                 /* (its ink and a clear gap: wave 3's 40 px "PITCH" touched the next ring) */
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
/* a value made to fit its column (40 px: six letters of the tall face at most, narrower) */
static void dval_fit(char *b)
{
    while (str_len(b) > 6u)
        b[str_len(b) - 1u] = 0;
}

/* (wave 12, Ben 23 Nov 13:35: "I don't need to see the sequence on screen, it's shown on my keys") a picture of the selected
 * slot's sound, in the synth pages' language: its level over the first half second (filled, the drums' teal; a noise layer of its
 * own in grey), its pitch sweep (a white line from the top, the drop in semitones). Worked out from the sound's values (the
 * machines' envelope laws, approximately for HEX), not rendered: no RAM, a few hundred operations */
#define SP_W 232
static void snd_picture(const dslot_t *sl, int32_t x0, int32_t y0, int32_t h)
{
    int32_t c, amp[SP_W / 2], nz[SP_W / 2], pit[SP_W / 2];
    uint32_t n = SP_W / 2u, i;
    if (sl->mach & 1u) {                               /* CLASSIC: hold, then DECAY_K a sample (ds_k32 a block); the noise
                                                        * its own; the pitch BEND semitones falling at BTIME */
        const dsnd_t *d = (const dsnd_t *)sl->snd;
        uint32_t ka = ds_k32(d->decay), kn = ds_k32(d->ndec), kp = ds_k32(d->btime), b;
        int32_t a = d->wave ? (int32_t)d->tlev * 258 : 0, z = (d->src & 15u) ? (int32_t)d->nlev * 258 : 0, pe = 32767;
        uint32_t hold = ds_blocks(d->hold), nhold = ds_blocks(d->nhold), per = (uint32_t)FS / 2u / CTL / n + 1u;
        for (i = 0; i < n; i++) {
            amp[i] = a, nz[i] = z, pit[i] = d->bend ? (pe * (int32_t)d->bend) >> 15 : 0;
            for (b = 0; b < per; b++) {
                if (hold) hold--; else a = (int32_t)(((uint32_t)a * ka) >> 16);
                if (nhold) nhold--; else z = (int32_t)(((uint32_t)z * kn) >> 16);
                pe = (int32_t)(((uint32_t)pe * kp) >> 16);
            }
        }
    } else {                                           /* HEX (the LXR's laws, from its own tables' marks, hex_drums.c):
                                                        * DEC 5 ~60 ms .. 60 ~1.3 s to silence; PAMT 20 ~10 semitones; PDEC
                                                        * 10 ~13 ms */
        const hex_snd_t *d = (const hex_snd_t *)sl->snd;
        int32_t ms_dec = 12 * (int32_t)d->dec + 2 * (int32_t)d->dec * (int32_t)d->dec / 10 + 10;
        int32_t ms_atk = (int32_t)d->atk / 2, semis = (int32_t)d->pamt * 6 / 10, ms_p = (int32_t)d->pdec * 17 / 10 + 2;
        int32_t lv = (int32_t)d->level * 258;
        for (i = 0; i < n; i++) {
            int32_t t = (int32_t)i * 500 / (int32_t)n, e;
            e = t < ms_atk ? lv * t / (ms_atk ? ms_atk : 1) : t - ms_atk >= ms_dec ? 0
                : lv - lv * (t - ms_atk) / ms_dec;   /* (32766 x 1000: 32 bits) */
            e = e * e / (lv ? lv : 1);                   /* (a curve: the LXR's fall is exponential-ish; 32766^2 < 2^31) */
            amp[i] = e, nz[i] = 0;
            pit[i] = d->model <= HM_CLAP && semis ? semis * 64 / (64 + t * 64 / ms_p) : 0;
        }
    }
    op_box(x0 - 1, y0 - 1, SP_W + 2, h + 2, TE_G1);
    for (c = 1; c < 4; c++)                            /* an eighth of a second a mark */
        cv_rect(x0 + c * SP_W / 4, y0, 1, h, TE_G1);
    for (i = 0; i < n; i++) {
        int32_t ha = amp[i] * h / 32767, hn = nz[i] * h / 32767, x = x0 + (int32_t)i * 2;
        if (hn > 0)
            cv_rect(x, y0 + h - (hn > h ? h : hn), 2, hn > h ? h : hn, TE_G2);
        if (ha > 0)
            cv_rect(x, y0 + h - (ha > h ? h : ha), 2, ha > h ? h : ha, TE_MID[3]);
        if (pit[i] > 0) {
            int32_t py = y0 + h - 1 - (pit[i] > 24 ? 24 : pit[i]) * (h - 2) / 24;
            cv_rect(x, py, 2, 1, C_WHITE);
        }
    }
}
static uint32_t snd_pic_sig(const dslot_t *sl)
{
    uint32_t h = 2166136261u, i;
    for (i = 0; i < 22u; i++)
        h = (h ^ sl->snd[i]) * 16777619u;
    return h ^ sl->mach;
}

/* the drum screen (the GRID, chromatic mode): header, name, map, the sound (wave 12: no step row), tabs, dials, strip */
static void drum_main_draw(void)
{
    static uint32_t head, sa, sb, sc, foot, bar;
    uint32_t l = drum_lane & 15u, len = trk_len(TDRUM), bank = sg_page(), model = drum_model(), i, sig;
    int32_t first = sg_held ? sg_first() : -1;
    int chrom = drum_keys == DK_CHROM;
    char sub[28], info[32];
    if (map_lyr && map_vis) {                           /* (wave 12) HOLD LFO: the header says LFO with MASTER's chip */
        map_chip(map_lyr);
        op_header("LFO", TE_DRUM, "", &head);
        hdr_mchip = 0;
    } else {
        op_header("DRUM", TE_DRUM, dpg_kit() ? "KIT" : "", &head);   /* (the title place is SELECT's chip: the slot; the
                                                         * kit's pages: KIT, no chip, SELECT does nothing) */
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
    } else if (dpg_kit()) {                             /* (the drum effects under FX) a kit page: the kit, not a sound */
        drum_kit_name(sub, 11);
    } else {                                            /* (wave 12) the kit (CUSTOM once a slot differs) and the slot's trigs */
        uint32_t n = 0;
        drum_kit_name(sub, 11);
        str_cpy(sub + str_len(sub), "  ", 3);
        for (i = 0; i < len; i++)
            n += (uint32_t)dt_has(i, l);
        fmt_int(sub + str_len(sub), (int32_t)n);
        str_cpy(sub + str_len(sub), n == 1u ? " TRIG" : " TRIGS", 8);
    }
    sig = studio_hash(drum_map_sig() * 31u + l, sub) + ((drum_mute >> l) & 1u) * 7u + ((drum_solo >> l) & 1u) * 11u +
          (uint32_t)dpg_kit() * 977u;
    if (ui.force || sig != sa) {
        sa = sig;
        cv_begin(240, 54, C_BLACK);
        {
            char nm[10];
            str_cpy(nm, dpg_kit() ? "KIT" : LANE_NAME[l], 9);   /* (a kit page: KIT, no sound's name) */
            while (text_w(&FONT_L, nm) > 146)
                nm[str_len(nm) - 1u] = 0;
            cv_text(4, 2, &FONT_L, nm, TE_DRUM);
        }
        i = (uint32_t)cv_text(4, 36, &FONT_S, sub, C_WHITE);
        if (dpg_kit())
            ;                                           /* (a kit page: no sound's mute or solo badge) */
        else if ((drum_mute >> l) & 1u)
            op_badge((int32_t)i + 6, 37, 'M', TE_MUTE);
        else if ((drum_solo >> l) & 1u)
            op_badge((int32_t)i + 6, 37, 'S', TE_SOLO);
        drum_map_nosel = (uint8_t)dpg_kit();            /* (a kit page: no sound framed: the page is all 16's) */
        drum_map(155, 4, 18, 9, 21, 12);
        drum_map_nosel = 0;
        cv_blit(0, SG_BODY_Y);
    }
    /* (wave 12) the sound (chromatic: the keys as a keyboard). The keys are the steps and their lights show the pattern: the
     * screen no longer repeats them (Ben, 23 Nov 13:35). A sound page: SOUND 606 SNARE (what PRESETS browses), its place in
     * the list, its picture; a step held, which and its locks on that line. TRIG, PATTERN, A-D: the held step's values, the
     * page of steps the keys are on, the pattern's length */
    info[0] = 0;
    if (first >= 0) {
        int xi = dt_x((uint32_t)first, l);
        uint32_t k, n = 0, w, nh = 0;
        for (w = 0; w < 16u; w++)
            nh += (sg_held >> w) & 1u;
        (void)nh;                                       /* (which step: the line above says it, STEP 5 HELD) */
        if (drum_pg) {
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
            if (!n)
                str_cpy(info + str_len(info), "NO LOCK", 8);
            if (text_w(&FONT_S, info) > 232) {
                char *lp = info;
                while (*lp && memcmp(lp, "LOCK ", 5))
                    lp++;
                str_cpy(lp, "LOCKS ", 7);
                fmt_int(lp + 6, (int32_t)n);
            }
        }
        (void)xi;
    } else if (chrom) {
        str_cpy(info, "KEYS PLAY ", sizeof info);
        str_cpy(info + str_len(info), LANE_SHORT[l], 8);
    }
    sig = studio_hash(chrom ? fm1_in.notes * 7u + 3u : snd_pic_sig(&pkit.s[l]) + drum_pg * 7u + bank * 131u + len * 977u +
                      (first >= 0 ? drum_trig_sig(l, bank) : 0u), info) + (uint32_t)chrom * 99991u + pkit_gen * 3u + l +
          dpg_half() * 7919u + pkit.s[l].send * 131u + kfx_count() * 17u;
    if (ui.force || sig != sb) {
        sb = sig;
        cv_begin(240, 55, C_BLACK);
        if (chrom) {                                    /* the 27 keys: white 15 px, black over them; the sound's own pitch
                                                         * dotted; a key down lit */
            piano_draw(0, 28, 17, fm1_in.notes, 1u << CHR_KEY0, TE_DRUM, TE_G3);
            cv_text(4, 36, &FONT_S, info, TE_G3);
        } else if (dpg_sends()) {                       /* (the drum effects under FX) the selected sound's own sends */
            char a[28] = "SOUND ";
            slot_name(&pkit.s[l], a + 6, sizeof a - 6u);
            cv_text(4, 0, &FONT_S, a, TE_DRUM);
            cv_text_tall(4, 20, "SENDS", TE_DRUM);
            cv_text(236 - text_w(&FONT_S, sl_kfx_off(l) ? "KIT FX OFF" : "KIT FX ON"), 26, &FONT_S,
                    sl_kfx_off(l) ? "KIT FX OFF" : "KIT FX ON", sl_kfx_off(l) ? TE_G4 : TE_DRUM);
            if (first >= 0)
                cv_text(236 - text_w(&FONT_S, "LOCK ON THE STEP"), 0, &FONT_S, "LOCK ON THE STEP", TE_DRUM);
        } else if (drum_pg == DPG_FX) {                 /* the FX tab: the kit's own pages (the whole mix's: below) */
            char a[28];
            if (dpg_kit()) {                            /* (how many sounds go through: KIT FX ON) */
                str_cpy(a, "THE KIT: ", sizeof a);
                fmt_int(a + str_len(a), (int32_t)kfx_count());
                str_cpy(a + str_len(a), " OF 16 SOUNDS", 14);
            } else {
                str_cpy(a, "THE WHOLE MIX", sizeof a);
            }
            cv_text(4, 0, &FONT_S, a, dpg_kit() ? TE_G4 : TE_G4);
            cv_text_tall(4, 20, PAGES[dbus_page(dpg_half())].title, dpg_kit() ? TE_DRUM : C_WHITE);
            if (first >= 0)
                cv_text(236 - text_w(&FONT_S, "LOCK ON THE STEP"), 0, &FONT_S, "LOCK ON THE STEP", TE_DRUM);
        } else if (!drum_pg) {                          /* TRIG, PATTERN, A-D: the step held (its values), the keys' page */
            char a[32], b2[32];
            int xi = first >= 0 ? dt_x((uint32_t)first, l) : -1;
            fmt_int(a, (int32_t)bank * 16 + 1);
            str_cpy(a + str_len(a), "-", 2);
            fmt_int(a + str_len(a), (int32_t)(bank * 16u + 16u < len ? bank * 16u + 16u : len));
            str_cpy(a + str_len(a), " OF ", 5);
            fmt_int(a + str_len(a), (int32_t)len);
            str_cpy(a + str_len(a), " STEPS", 7);
            cv_text(4, 2, &FONT_S, "KEYS", TE_G3);
            cv_text(48, 2, &FONT_S, a, C_WHITE);
            if (first >= 0) {
                dtx_t c;
                uint32_t nl = dt_lk_n((uint32_t)first, l);
                int nk = dt_lk((uint32_t)first, l, DSP_NOTE);
                memset(&c, 0, sizeof c);
                c.prob = DT_PROB_DEF;
                if (xi >= 0)
                    c = dpat.x[xi];
                (void)b2;
                str_cpy(a, "VEL ", 5);                  /* line 2: the hit; line 3: when it plays, its pitch, its locks */
                fmt_int(a + 4, (int32_t)dt_vel((uint32_t)first, l));
                if (c.rrate)
                    str_cpy(a + str_len(a), "  RETRIG ", 10), str_cpy(a + str_len(a), SG_RATE[c.rrate % 18u], 6);
                cv_text(4, 20, &FONT_S, a, TE_DRUM);
                a[0] = 0;
                if (c.micro) {
                    str_cpy(a, "NUDGE ", 7);
                    if (c.micro > 0)
                        str_cpy(a + str_len(a), "+", 2);
                    fmt_int(a + str_len(a), c.micro);
                    str_cpy(a + str_len(a), "  ", 3);
                }
                if (c.cond) {
                    str_cpy(a + str_len(a), SG_COND[c.cond & 3u], 6);
                    if (c.cond == FC_CHANCE)
                        str_cpy(a + str_len(a), " ", 2), fmt_int(a + str_len(a), c.prob), str_cpy(a + str_len(a), "%", 2);
                    str_cpy(a + str_len(a), "  ", 3);
                }
                if (nk >= 0) {
                    int32_t nv = lk_val(DSP_NOTE, dpat.lk[nk].val);
                    str_cpy(a + str_len(a), "NOTE ", 6);
                    if (nv > 0)
                        str_cpy(a + str_len(a), "+", 2);
                    fmt_int(a + str_len(a), nv);
                    str_cpy(a + str_len(a), "  ", 3);
                }
                if (text_w(&FONT_S, a) + text_w(&FONT_S, "9 LOCKS") > 232)
                    a[0] = 0;                           /* (no room: the dials show them) */
                fmt_int(a + str_len(a), (int32_t)nl);
                str_cpy(a + str_len(a), nl == 1u ? " LOCK" : " LOCKS", 7);
                cv_text(4, 38, &FONT_S, a, TE_G4);
            } else {
                str_cpy(a, "NEW HITS AT VELOCITY ", sizeof a);
                fmt_int(a + str_len(a), drum_newvel);
                cv_text(4, 20, &FONT_S, a, TE_G4);
                cv_text(4, 38, &FONT_S, "HOLD A STEP FOR ITS VALUES", TE_G3);
            }
        } else {                                        /* a sound page: (review V4, "say each once": the title names the
                                                         * sound) where it comes from and its place, "606 \xB7 8/41", its picture */
            char a[28], g[12];
            int32_t p = snd_pos_of(l, 1);
            if (first >= 0) {                           /* (a step held: its locks; the line above says which step) */
                cv_text(4, 0, &FONT_S, info, TE_DRUM);
            } else {
                const dslot_t *sd = &pkit.s[l];
                if (sd->src == DSRC_INIT)
                    str_cpy(a, "INIT", sizeof a);
                else
                    kit_name_of(sd->src, a, 13);
                if (p >= 0) {
                    uint32_t i2, sz;
                    snd_group(l, (uint32_t)p, g, sizeof g, &i2, &sz);
                    str_cpy(a + str_len(a), " \xB7 ", 4);
                    fmt_int(a + str_len(a), (int32_t)i2);
                    str_cpy(a + str_len(a), "/", 2);
                    fmt_int(a + str_len(a), (int32_t)sz);
                }
                cv_text(4, 0, &FONT_S, a, TE_DRUM);
            }
            snd_picture(&pkit.s[l], 4, 20, 32);
        }
        cv_blit(0, 76);
    }
    /* (wave 9) the tabs: the categories of the button's group, the one shown filled, the page in it (ui_nav.c) */
    if (map_lyr && map_vis) {                           /* (wave 12) MAP LFO, outlined (wave 11's look); one LFO: no SEL */
        nav_map_hint = "";
        nav_tabs(TE_DRUM, "MAP LFO", &sc);
        nav_map_hint = 0;
        dmap_dials(&foot);
        op_trackbar(&bar);
        return;
    }
    nav_tabs(TE_DRUM, 0, &sc);
    /* the dials */
    {
        static char v[4][12];
        const char *lab[4] = {"", "", "", ""}, *val[4] = {v[0], v[1], v[2], v[3]}, *u;
        int32_t ratio[4] = {-1, -1, -1, -1};
        uint32_t k, locked = 0, dim = 0;
        for (k = 0; k < 4u; k++)
            v[k][0] = 0;
        if (dpg_bus()) {                                /* (wave 8) the FX tab: the drum voice's own pages; a trig held, the
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
        dial_mark = (uint8_t)(dpg_snd() ? dmap_marks() : 0u);   /* (wave 12: the knob the LFO moves) */
        if (dpg_bus() && PAGES[dbus_page(dpg_half())].fam == FAM_MFX)   /* (the whole mix's: white-grey) */
            op_dials(lab, val, ratio, locked, dim, TE_G4, drum_pg * 977u + dpg_half() * 31u + pkit_gen, &foot);
        else
            drum_dials(lab, val, ratio, locked, dim, drum_pg * 977u + dpg_half() * 31u + pkit_gen, &foot);
        dial_mark = 0;
    }
    op_trackbar(&bar);
}

/* MUTE mode (the mock "drum mute"): the 16 sounds as tiles, a muted one dark with a yellow M (a soloed one an ice-blue S),
 * the keys' jobs, the selected sound's trigs */
static void drum_mute_draw(void)
{
    static uint32_t head, sa, sb, bar;
    uint32_t p, sig = drum_map_sig() * 7u + drum_trig_sig(drum_lane & 15u, sg_page());
    op_header("DRUM", TE_DRUM, "MUTE", &head);
    if (ui.force || sig != sa) {
        sa = sig;
        cv_begin(240, 124, C_BLACK);
        for (p = 0; p < DRUM_LANES; p++) {             /* (the drum grid: place p, the sound l there) */
            uint32_t l = DRUM_GRID[p];
            int32_t x = 2 + (int32_t)(p % 4u) * 60, y = 3 + (int32_t)(p / 4u) * 25;
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
        cv_text(4, 104, &FONT_S, "TAP A KEY: MUTE", TE_G3);
        cv_text(236 - text_w(&FONT_S, "HOLD GLO: SOLO"), 104, &FONT_S, "HOLD GLO: SOLO", TE_G3);   /* (wave 14: it said
                                                         * SEL+KEY, gone in wave 9; solos are the mixer's, OCT+ with a key) */
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
        if (pressed & (ob | pb)) {                      /* (the drum grid: in key order, stopping at either end) */
            drum_lane = (uint8_t)lane_step(drum_lane, (pressed & pb) ? 1 : -1, 0);
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
    if ((s = panel_enc(EN_PRESET)) != 0 && !sg_held)   /* (wave 12, Ben: "sounds on the drum screen") the slot's sound */
        snd_browse(s);
    if (sg_held)
        (void)panel_enc(EN_ALGO);
    for (k = 0; k < 4u; k++) {
        uint32_t id;
        if ((s = panel_enc(EN_K1 + k)) == 0)
            continue;
        ui.hot_col = (uint8_t)k;
        ui.hot_t = 40;
        if (dpg_bus()) {                                /* (wave 8) the FX tab: the drum voice's own pages */
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
