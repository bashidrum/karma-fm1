/* SPDX-License-Identifier: GPL-3.0-only */
/* SELECT, the type knob (PHEBUS wave 9; Ben, 23 Nov 2026: "I feel the select knob would be better acting as a knob 5
 * effectively changing parameters within the category that have a type - oscillator type or type of distortion engine for
 * example"; picker "Yes, SELECT = type": "SELECT chooses the type on the current page (oscillator type, insert type, filter
 * type, LFO wave). Pages are walked only by pressing the labelled button"). Included by ui_synth.c.
 *
 *   what it turns   the page's sel (params.c page_t: V_TYPE on the OSC pages, a slot's TYPE on FX 1 to 3, the filter's TYPE
 *                   on FILT, LFO 1's WAVE on its two pages, LFO 2's, the SLICER's mode). A page with none: SELECT does nothing
 *   on the drum screen (Ben, 23 Nov 2026, picker "Drum sound pick: SELECT knob") the selected SOUND, one of the 16 a detent,
 *                   on every tab (it was SEL held + a key); the sound's own WAVE, MODEL, filter TYPE stay on its knobs
 *   with a step held the type is LOCKED on it, as a knob's value is (Ben's Rytm lock), where it can be: the oscillator's TYPE,
 *                   an FX slot's TYPE cannot (the Rytm's machine): NOT LOCKABLE. The drum screen with a step held: the
 *                   sound stays (the held step is that sound's)
 *   where it shows  the header's title place (op_header): a chip, the knob's little dial and the value, in the voice's colour;
 *                   filled for a moment after a turn, as a touched dial's label is. No chip: SELECT does nothing here */
static int vis_shown(void);                             /* (ui_vis.c) */
enum { SK_NONE, SK_PAGE, SK_SOUND };
/* the type on the screen shown: SK_PAGE (*pg its page, *id its sel) or SK_DRUM (*id a drum sound parameter), else SK_NONE */
static uint32_t sel_target(const page_t **pg, uint32_t *id)
{
    *pg = 0;
    *id = 0xFFu;
    if (ui.menu || ui.layer != LY_PLAY || ui.hold_kind || rec_wait || ui.home || vis_shown())
        return SK_NONE;
    if (on_drum_page()) {
        if (stl_shown())
            return SK_NONE;
        if (song.sel == TRK_DRUM)                       /* the drum screen: the sound (the kit's pages and the whole mix's:
                                                         * nothing, they do not act on one sound) */
            return dpg_bus() ? SK_NONE : SK_SOUND;
        if (!syn_tab || syn_page() >= NPAGES)           /* the synth GRID: its tab's page */
            return SK_NONE;
        *pg = &PAGES[syn_page()];
    } else {
        *pg = cur_page();
    }
    if (!*pg || (*pg)->sel == SEL_NONE || (is_drum(TSEL) && !page_for_drum(*pg)))
        return SK_NONE;
    *id = (*pg)->sel;
    return SK_PAGE;
}

/* the oscillator's TYPE a detent (vtype.c), as KNOB 1 turned it before wave 9 */
static void sel_vtype(int32_t steps)
{
    int32_t ty = (int32_t)vtype_of(TSEL);
    ty = clamp((ty < (int32_t)NVTYPE ? ty : 0) + (steps > 0 ? 1 : -1), 0, (int32_t)NVTYPE - 1);
    if ((uint32_t)ty != vtype_of(TSEL))
        vtype_set(TSEL, (uint32_t)ty);
}
/* an FX slot's TYPE a detent: past the FLANGER when another slot has it and past the old send types; a type picked starts at
 * its own settings (wave 5, wave 8: as KNOB 1 did) */
static void sel_xtype(const page_t *pg, int32_t steps)
{
    uint32_t k = pg_slot(pg);
    int32_t ty = TSEL->p[P_XS(k, 0)], to = ty, n;
    for (n = 0; n < (int32_t)XT_N; n++) {
        to = clamp(to + (steps > 0 ? 1 : -1), 0, (int32_t)XT_N - 1);
        if (xs_type_ok(TSEL->p, k, to) || to == 0 || to == (int32_t)XT_N - 1)
            break;
    }
    if (!xs_type_ok(TSEL->p, k, to))
        to = ty;
    ml_knob(TSEL, P_XS(k, 0));                          /* (MIDI LEARN: the TYPE too) */
    if (to != ty) {
        fm1_irq_off();
        locks_restore(TSEL);                            /* (a lock in force on the old type's knob: its base first) */
        xs_type_set(TSEL->p, k, to);
        fm1_irq_on();
    }
}
static int sel_lockable(uint32_t kind, const page_t *pg, uint32_t id)
{
    (void)kind;
    return id < P_COUNT && pg->graph != GR_XSLOT && pg->scope == SC_TRACK && p_lockable(id);
}

/* SELECT turned s: the type on the screen shown (none: nothing). 1: it had one */
static int sel_turn(int32_t s)
{
    const page_t *pg;
    uint32_t id, kind = sel_target(&pg, &id);
    int held = on_drum_page() && sg_held;
    if (kind == SK_NONE || !s)
        return 0;
    sel_t = 40;
    ui.force = 1;
    if (kind == SK_SOUND) {                             /* the drum screen: the next / previous of the 16 sounds, in key order
                                                         * (the drum grid: down each column, HAT PEDAL KICK 2 KICK, OP HAT ...),
                                                         * round from SNARE to HAT */
        int32_t l = (int32_t)lane_step(drum_lane, s > 0 ? 1 : -1, 1);
        if (held) {
            ui_message("LET GO OF THE STEP");
            return 1;
        }
        if ((uint32_t)l != drum_lane) {
            drum_lane = (uint8_t)l;
            if (!song.playing)
                audition_lane((uint32_t)l);             /* (heard, stopped: as SEL + key did) */
            dk_touch = 0xFF;
        }
        return 1;
    }
    if (held) {                                         /* a step held: the type locked on it (where it can be) */
        if (!sel_lockable(kind, pg, id)) {
            ui_message("TYPE: NOT LOCKABLE");
            return 1;
        }
        sg_lock_turn(id, s, EN_SELECT);
        syn_touch = (uint8_t)id;                        /* (OCT-: this lock first) */
        return 1;
    }
    if (id == V_TYPE) {
        sel_vtype(s);
    } else if (pg->graph == GR_XSLOT) {
        sel_xtype(pg, s);
    } else {                                            /* a named type: one name a detent */
        int16_t *vp = pg->scope == SC_GLOBAL ? &song.g[id] : &TSEL->p[id];
        const param_desc_t *d = pg->scope == SC_GLOBAL ? &GP[id] : track_desc(TSEL, id);
        *vp = (int16_t)clamp(*vp + (s > 0 ? 1 : -1), d->min, d->max);
        if (pg->scope != SC_GLOBAL)
            ml_knob(TSEL, id);
    }
    return 1;
}

/* the chip's value: 1 and v (six characters at most), *ratio where it sits in its range (the dial), *col the voice's colour */
static int sel_chip(char *v, int32_t *ratio, uint16_t *col)
{
    const page_t *pg;
    uint32_t id, kind = sel_target(&pg, &id);
    int32_t first = on_drum_page() && sg_held ? sg_first() : -1, x;
    const char *u;
    v[0] = 0;
    *ratio = 0;
    *col = TE_COL[song.sel & 3u];
    if (kind == SK_NONE)
        return 0;
    if (kind == SK_SOUND) {
        str_cpy(v, LANE_SHORT[drum_lane & 15u], 8);
        *ratio = (int32_t)white_of_lane(drum_lane) * 1000 / (DRUM_LANES - 1);   /* (its key's place) */
        *col = TE_DRUM;
    } else if (id == V_TYPE) {
        uint32_t ty = vtype_of(TSEL);
        str_cpy(v, ty < NVTYPE ? VTYPES[ty].name : "--", 8);
        *ratio = (int32_t)(ty < NVTYPE ? ty : 0u) * 1000 / (int32_t)(NVTYPE - 1u);
    } else {
        const param_desc_t *d = pg->scope == SC_GLOBAL ? &GP[id] : track_desc(TSEL, id);
        int l;
        x = pg->scope == SC_GLOBAL ? song.g[id] : TSEL->p[id];
        if (pg->graph == GR_XSLOT && xt_send(x))
            x = XT_OFF;                                 /* (wave 8: no slot type: shown as it plays, OFF) */
        if (first >= 0 && pg->scope == SC_TRACK && id < P_COUNT && (l = lock_find(TSEL, (uint32_t)first, id, 0)) >= 0)
            x = TSEL->lock[l].val;
        param_format(d, x, v, &u);
        *ratio = d->max > d->min ? (x - d->min) * 1000 / (d->max - d->min) : 0;
    }
    v[7] = 0;
    return 1;
}
