/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Felucca UI input: LEDs, knobs and buttons, the layers (ui_layers.c), holds, SEQ step entry, panel setup. */
#define BTN_BIT(b) (1u << panel.btn[b])
static uint32_t ly_eat;                                  /* (wave 9) layer buttons pressed in the menu: no tap after it */
static void layer_unlock(void);
#include "ui_nav.c"                                     /* (wave 9) how to move around: categories, pages */
/* ----------------------------------------------------------- LEDs --- */
/* The LED picture is built off-line and copied one byte per column: clearing
 * and relighting would let the 10 kHz scan catch the dark gap and flicker. */
static uint8_t led_pos[41];                        /* (col << 3) | row bit, 0xFF = none */

static void led_pos_init(void)
{
    uint32_t id, p, r;
    for (id = 0; id < 41u; id++) {
        led_pos[id] = 0xFF;
        for (p = 0; p < FM1_NCOL; p++)
            for (r = 1; r < 5u; r++)
                if (FM1_KEYMAP[r][p] == (int8_t)id)
                    led_pos[id] = (uint8_t)((p << 3) | r);
    }
}

static void led_put(uint8_t *nl, uint32_t id, int on)
{
    uint8_t q = led_pos[id];
    if (q != 0xFF && on)
        nl[q >> 3] |= (uint8_t)(1u << (q & 7u));
}

static uint32_t cur_fam(void) { return ui.home ? FAM_HOME : cur_page()->fam; }
static uint32_t cur_btn(void)                      /* the button of the screen shown (wave 9: the group it is in) */
{
    uint32_t k, n, b = nav_btn_of(nav_cat(&k, &n));
    return b < NB ? b : B_HOME;
}

/* PLAY flashes on every beat, as heard (the sound leaves ~9 ms after it is rendered) */
static int play_led(void)
{
    uint32_t p = clk_pos, lat = HALF_FRAMES * 3u / 2u * (uint32_t)song.g[G_BPM];
    if (!song.playing)
        return 0;
    p = p >= lat ? p - lat : p + BEAT_U - lat;
    return p < BEAT_U / 4u;
}

/* the keys' lights: what the layer held does, else the keys down and the drum hits */
/* what sounds on track t, as keys: the drum hits (each lights its key a few frames) or, on a synth
 * track, the voices still held, mapped back through the keyboard (octave, scale, chords; by
 * @renebohne, PR #11). Shown at once with menu NOTES on, on every screen and layer (keys_notes_dim) */
static uint32_t keys_sounding(const track_t *t)
{
    uint32_t i, m = 0;
    if (is_drum(t)) {
        for (i = 0; i < DRUM_LANES; i++)
            if (pad_lit[i])
                m |= 1u << key_of_white(i);
        return m;
    }
    for (i = 0; i < NVOICE; i++) {
        const voice_t *v = &t->v[i];
        if (v->active && v->gate && v->stage <= 2) {
            uint32_t k, note = v->note;
            for (k = 0; k < 27u; k++)
                if (kb_map(t, k) == note)
                    m |= 1u << k;
        }
    }
    if (t == TSEL)                                 /* + the notes just started, a few frames (pads_tick) */
        for (i = 0; i < 27u; i++)
            if (key_lit[i])
                m |= 1u << i;
    return m;
}

static uint32_t scale_keys(uint32_t root_only)   /* SEL: the keys in the scale (or its roots only) */
{
    uint32_t i, m = 0, root = (uint32_t)trk[0].p[P_ROOT] % 12u, mask = SCALE_MASK[clamp(trk[0].p[P_SCALE], 0, NSCALES - 1)];
    for (i = 0; i < 27u; i++) {
        uint32_t d = (53u + i - root + 120u) % 12u;
        if ((mask >> d) & 1u && (!root_only || !d))
            m |= 1u << i;
    }
    return m;
}

static uint32_t erase_lanes(const track_t *t)   /* EDIT erase, drum track: the sounds the pattern holds */
{
    uint32_t i, m = 0;
    if (is_drum(t))
        for (i = 0; i < trk_len(t); i++) {
            uint32_t l, d = dt_mask(i);
            for (l = 0; d; l++, d >>= 1)
                if (d & 1u)
                    m |= 1u << key_of_white(l);
        }
    return m;
}

static uint32_t keys_lit(void)
{
    uint32_t m = 0, i, blink = (fm1_ms / 125u) & 1u;
    track_t *t = TSEL;
    switch (ui.layer) {
    case LY_FX:                                    /* (wave 6) PERFORM: the pads playing; latched ones blink */
        return perf_keys_lit(blink);
    case LY_STEP:                                  /* (PHEBUS) SEQ held: its keys do nothing, none lit */
        return 0u;
    case LY_SCALE:                                 /* the keys in the scale; the root blinks */
        if (lights_notes)                          /* NOTES: the scale goes dim (keys_notes_dim), what sounds lit */
            return (blink ? scale_keys(1) : 0u) | keys_sounding(t) | fm1_in.notes;
        return scale_keys(0) & ~(blink ? 0u : scale_keys(1));
    case LY_MIX:                                   /* tracks heard: 1..4; soloed: 5..8; a fill: 9, armed / on: 10; tap: the beat */
        if (is_drum(t)) {                          /* (wave 9) the drums: the sounds heard (not muted; soloed when any is) */
            for (i = 0; i < 16u; i++)
                if (drum_solo ? (drum_solo >> i) & 1u : !((drum_mute >> i) & 1u))
                    m |= 1u << key_of_white(i);
            return m;
        }
        for (i = 0; i < 4u; i++) {
            if (!trk_silent(&trk[i]))
                m |= 1u << key_of_white(i);
            if ((song.solo >> i) & 1u)
                m |= 1u << key_of_white(4u + i);
        }
        if (fill_now)
            m |= 1u << key_of_white(8);
        if (fill_arm || fill_bar_on)
            m |= 1u << key_of_white(9);
        if (play_led())
            m |= 1u << key_of_white(15);
        return m;
    case LY_ERASE:                                 /* the sounds the pattern holds (NOTES: dim, the hits lit) */
        return (lights_notes ? 0u : erase_lanes(t)) | fm1_in.notes | (lights_notes ? keys_sounding(t) : 0u);
    default:                                       /* playing, ARP roll, SAVE sections, every page and the menu */
        break;
    }
    if (kb_grid)                                   /* the GRID: the steps of the page shown (ui_grid.c) */
        return grid_keys_lit(blink) | fm1_in.notes;
    m = fm1_in.notes;
    if (is_drum(t) || lights_notes)                /* the drum track: each hit lights its key; NOTES: the synths too */
        m |= keys_sounding(t);
    return m;
}

/* menu NOTES, on the layers whose keys are tiles (FX effects, SEQ steps, GLO mute / solo): what
 * sounds glows dimly under the tiles, which keep their full light. SEL (the scale) and EDIT on the
 * drum track (the sounds of the pattern): those glow, and what sounds is lit (keys_lit) */
static uint32_t keys_notes_dim(void)
{
    if (!lights_notes)
        return 0u;
    switch (ui.layer) {
    case LY_FX:
    case LY_MIX:
        return keys_sounding(TSEL);
    case LY_SCALE:
        return scale_keys(0);
    case LY_ERASE:
        return erase_lanes(TSEL);
    default:
        return 0u;
    }
}

/* the keys' landmarks, dim: the first key of each row of the 4 x 4 grid on screen (white keys
 * 1, 5, 9, 13) while a layer is held, and on the drum track (its 16 sounds, 4 x 4 as on KIT) */
static uint32_t keys_guide(void)
{
    if ((ui.layer == LY_PLAY && !is_drum(TSEL) && !kb_grid) || ui.layer == LY_STEP)   /* (SEQ: no tiles, PHEBUS) */
        return 0u;
    return 1u << key_of_white(0) | 1u << key_of_white(4) | 1u << key_of_white(8) | 1u << key_of_white(12);
}

/* the keys the backlight lights (menu KEYS): the Cs, every white key, or every key; bit k = key k (0 = F3) */
static uint32_t lights_keys_mask(void)
{
    uint32_t k, m = 0;
    if (!lights_lvl || !lights_keys)
        return 0u;
    for (k = 0; k < 27u; k++) {
        uint32_t pc = (53u + k) % 12u;             /* key 0 = F3 (53) */
        if (lights_keys == KEYS_ALL || (lights_keys == KEYS_C ? pc == 0u : ((0xAB5u >> pc) & 1u) != 0u))   /* 0xAB5: C D E F G A B */
            m |= 1u << k;
    }
    return m;
}

static void ui_leds(void)
{
    uint8_t nl[FM1_NCOL] = {0}, dl[FM1_NCOL] = {0}, bl[FM1_NCOL] = {0};
    uint32_t k, c, keys, guide, back;
    static uint8_t ready;
    if (!ready) {
        led_pos_init();
        ready = 1;
    }
    led_put(nl, panel.btn[ui.layer != LY_PLAY ? LAYER_BTN[ui.layer] : cur_btn()],
            ly_lock == LY_PLAY || ((fm1_ms / 300u) & 1u) != 0u || (fm1_in.buttons & ly_bit[ly_lock % LY_COUNT]) != 0u);   /* locked: blinks */
    led_put(nl, panel.btn[B_PLAY], play_led() ||
                                      (ci_on && ci_u % BEAT_U < BEAT_U / 4u));   /* (the count-in's beats) */
    led_put(nl, panel.btn[B_REC], song.rec != 0u || (rec_wait && ((fm1_ms / 125u) & 1u)) ||
                                     (ui.hold_kind == 1u && ((fm1_ms / 60u) & 1u)));   /* blinks: armed; fast: clearing */
    if (ui.layer == LY_FX) {                       /* PERFORM: OCT- lit while something is latched (all off), OCT+ while
                                                    * a pad is held (latch it) or held itself */
        led_put(nl, panel.btn[B_OCTDN], perf.lat != 0u);
        led_put(nl, panel.btn[B_OCTUP], perf.held != 0u || (fm1_in.buttons & dyn_bit[1]) != 0u);
    } else if (is_drum(TSEL)) {                    /* the drum track: OCT- / OCT+ lit while ghost / hard */
        led_put(nl, panel.btn[B_OCTDN], (fm1_in.buttons & dyn_bit[0]) != 0u);
        led_put(nl, panel.btn[B_OCTUP], (fm1_in.buttons & dyn_bit[1]) != 0u);
    } else {
        led_put(nl, panel.btn[B_OCTDN], song.octave < 0);
        led_put(nl, panel.btn[B_OCTUP], song.octave > 0);
    }
    keys = keys_lit();
    guide = (keys_guide() | keys_notes_dim()) & ~keys;
    back = lights_keys_mask() & ~keys & ~guide;
    for (k = 0; k < 27u; k++) {
        led_put(nl, 14u + k, (int)((keys >> k) & 1u));
        led_put(dl, 14u + k, (int)((guide >> k) & 1u));
        led_put(bl, 14u + k, (int)((back >> k) & 1u));
    }
    if (lights_lvl)                                /* menu LIGHTS: every button glows, the lit ones stay full */
        for (k = 0; k < NB; k++)
            led_put(bl, panel.btn[k], 1);
    for (c = 0; c < FM1_NCOL; c++) {
        fm1_led[c] = nl[c];
        fm1_led_dim[c] = dl[c];
        fm1_led_bg[c] = (uint8_t)(bl[c] & ~nl[c]);
    }
    fm1_led_bg_ns = LIGHTS_NS[lights_lvl % LIGHTS_N];
}

/* ---------------------------------------------------------- input --- */
static int32_t accel(uint32_t role, int32_t s, int32_t range)
{
    uint32_t now = fm1_ticks(), dt = now - ui.enc_t[role];
    ui.enc_t[role] = now;
    if (range > 40 && dt < 60u * 1000u * FM1_TICKS_PER_US)
        return s * (range > 150 ? 6 : 3);
    return s;
}

/* TRACKS page (HOME, PHEBUS 21 Nov 2026): KNOB 1 CUTOFF and 2 DECAY of the selected voice (ui_studio.c home_knob),
 * 3 the STUTTER lane ON / OFF (punch.c stl.act: everyone's), 4 LEVEL (0 = mute; the drum track: GLO > DRUMS LEVEL). A track
 * muted with MUTE (GLO + key): the first turn of KNOB 4 unmutes it. (Swing and STEPS: SEQ held; PAN: VOICE 2
 * and the mixer, GLO + key 11) */
static void tracks_edit(uint32_t slot, int32_t steps)
{
    track_t *t = TSEL;
    int16_t *vp;
    const param_desc_t *d;
    if (slot == 2u) {
        stl.act = (uint8_t)(steps > 0);
        return;
    }
    if (slot == 3u) {
        if (t->p[P_MUTE]) {
            t->p[P_MUTE] = 0;
            return;
        }
        vp = is_drum(t) ? &song.g[G_DRLVL] : &t->p[P_LEVEL];
        d = is_drum(t) ? &GP[G_DRLVL] : &TP[P_LEVEL];
    } else if (slot == 1u && is_drum(t)) {
        drum_param_turn(drum_dec_id(), steps, EN_K2);   /* (PHEBUS wave 3: the selected sound's decay, any kit) */
        return;
    } else if ((vp = home_knob(slot, &d)) == 0) {
        return;
    }
    *vp = (int16_t)clamp(*vp + accel(EN_K1 + slot, steps, d->max - d->min), d->min, d->max);
    if (!is_drum(t) && vp >= &t->p[0] && vp < &t->p[P_COUNT])
        ml_knob(t, (uint32_t)(vp - t->p));              /* MIDI LEARN (2.6) */
}

/* a new project: every track empty, the default sounds, 90 BPM (TOOLS > NEW) */
static void project_new(void)
{
    uint32_t i;
    memset(drum_send, 0, sizeof drum_send);             /* (every drum sound dry again) */
    memset(&stl, 0, sizeof stl);                        /* (PHEBUS) the STUTTER lane empty, ON, 16 steps */
    stl.len = 16, stl.act = 1;
    transport_req = 2;
    panic_req = (1u << NTRK) - 1u;
#if FELUCCA_ARRANGER
    live_sec = -1;                                      /* (no section's any more: edits stick would keep the empty
                                                         * project in the one last played) */
#endif
    for (i = 0; i < NTRK; i++) {
        track_t *t = &trk[i];
        undo_mark(t, (undo_sess += 4u) | 3u);
        fm1_irq_off();
        if (i < NPART)
            sound_change(t);                            /* (2.6: what sounds fades out with its own sound) */
        track_defaults(t);
        if (i < NPART)
            trk_def_sound(t, i);                        /* (dry: ui.c) */
        fm1_irq_on();
    }
    TDRUM->p[P_E0] = DRUM_DEFAULT_KIT;
    fm1_irq_off();
    memset(&pkit, 0, sizeof pkit);                      /* (PHEBUS wave 3) no kit of its own: the built-in 606 */
    pkit_gen++;
    fm1_irq_on();
    drum_mute = 0;                                      /* (2.6: the live mutes) */
    drum_solo = 0;
    for (i = 0; i < G_COUNT; i++)
        if (i != G_SLOT && i != G_DRCH && i != G_SYNC && i != G_MIDI && i != G_ROUTE && (i < G_CH1 || i > G_CHOTH) &&
            i != G_BEND)                                                                                   /* (the
                                                                 * FM-1's settings: GLO > MIDI 2.6) */
            song.g[i] = GP[i].def;
    song.solo = 0;
    song.octave = 0;
    sync_reload = 1;
    ui.force = 1;
}

static void edit_param_pg(const page_t *pg, uint32_t slot, int32_t steps)   /* (wave 4: any page; the GRID's tabs) */
{
    int16_t *vp;
    const param_desc_t *d;
    uint32_t id = pg_id(pg, slot);
    int32_t v;
    if (is_drum(TSEL) && !page_for_drum(pg))
        return;                                           /* "DRUM TRACK": nothing to edit here */
    if (id == V_LEARN) {                                  /* (wave 9) GLO > MIDI 3 LEARN: on right, off left (was GLO + F#3) */
        if ((steps > 0) != (mlu.on != 0))
            ml_toggle();
        return;
    }
    if (id >= P_FATK && id <= P_FREL && !TSEL->p[P_FLINK]) {   /* (wave 4) the filter's envelope turned while LINKed: its
                                                         * own from here, starting where the amp ENV is (one turn) */
        fm1_irq_off();
        TSEL->p[P_FATK] = TSEL->p[P_ATK];
        TSEL->p[P_FDEC] = TSEL->p[P_DEC];
        TSEL->p[P_FSUS] = TSEL->p[P_SUS];
        TSEL->p[P_FREL] = TSEL->p[P_REL];
        TSEL->p[P_FLINK] = 1;
        fm1_irq_on();
        ui_message("FILTER ENV: OWN");
    }
    if (pg->scope == SC_TRK) {
        tracks_edit(slot, steps);
        return;
    }
    if (pg->graph == GR_MATRIX && !is_drum(TSEL) && (id == V_MAMT || id == V_MK4)) {   /* (wave 5) the grid's cell, TARGET */
        uint32_t ti = song.sel % NPART, r = (uint32_t)mx_row % MS_N, c = (uint32_t)mx_col % MD_N;
        if (id == V_MAMT) {
            int32_t v = clamp(mx_get(ti, r, c) + accel(EN_K1 + slot, steps, 127), -64, 63), e;
            fm1_irq_off();
            e = mx_set(ti, r, c, v);
            fm1_irq_on();
            if (e < 0)
                ui_message("MATRIX FULL: 19 CELLS");
        } else if (c >= MD_AS1) {                         /* TARGET: the next / previous parameter a column can move */
            int32_t a = mxt[ti].as[c - MD_AS1] ? mxt[ti].as[c - MD_AS1] - 1 : -1, n;
            for (n = 0; n < (int32_t)P_COUNT + 1; n++) {
                a += steps > 0 ? 1 : -1;
                if (a < -1)
                    a = (int32_t)P_COUNT - 1;
                if (a >= (int32_t)P_COUNT)
                    a = -1;
                if (a < 0 || mx_as_ok((uint32_t)a))
                    break;
            }
            mxt[ti].as[c - MD_AS1] = (uint8_t)(a + 1);
        } else if (mx_get(ti, r, c)) {                    /* CLEAR: the cell to 0 (a fixed one: its parameter's 0) */
            fm1_irq_off();
            mx_set(ti, r, c, 0);
            fm1_irq_on();
            ui_message("CELL CLEARED");
        }
        return;
    }
    if (pg->graph == GR_BROWSE) {                         /* KNOB 1: one preset, KNOB 2: the next / previous engine */
        if (slot == 0u && !is_drum(TSEL)) {
            uint32_t total, cur = preset_pos(&total);
            if (total)
                preset_go((uint32_t)(((int32_t)cur + steps % (int32_t)total + (int32_t)total) % (int32_t)total));
        } else if (slot == 1u && !is_drum(TSEL)) {
        {                                                 /* (the track's engines only: ACID <-> JUNO) */
            uint32_t e = TSEL->eng_req, j;
            for (j = 0; j < NENGINES; j++) {
                e = (e + (steps > 0 ? 1u : NENGINES - 1u)) % NENGINES;
                if (eng_allowed(TSEL, e))
                    break;
            }
            if (e != TSEL->eng_req)
                select_engine(e);
        }
        }
        return;
    }
    if (pg->graph == GR_USER) {                           /* KNOB 1 slot; LOAD / ERASE / SAVE: GO buttons */
        static const char *const UP_GO[3] = {"LOAD", "ERASE", "SAVE"};
        if (slot == 0u) {
            ui.uslot = (uint8_t)clamp((int32_t)ui.uslot + steps, 0, UP_SLOTS - 1);
            ui.arm = 0;
            return;
        }
        if (steps <= 0)
            return;
        {                                                 /* (wave 8) it asks; hold REC until the ring fills: done */
            char b[12], l[4];
            str_cpy(b, UP_GO[slot - 1u], 7);
            up_slot_label(l, ui.uslot);
            str_cpy(b + str_len(b), " ", 2);
            str_cpy(b + str_len(b), l, 4);
            ask_set(ASK_UP, (slot - 1u) | (uint32_t)ui.uslot << 2, b);
        }
        return;
    }
    if (pg->graph == GR_SLOTS && slot == 1u) {           /* 2.6: KNOB 2 ERASE, a GO button (the slot KNOB 1 picks) */
        if (steps > 0)                                    /* (wave 8: it asks; hold REC: done) */
            ask_set(ASK_GO, 0xE8u, "ERASE");
        return;
    }
    d = page_desc(pg, slot, &vp);
    if (!d || !vp || d->max == d->min)
        return;
    v = d->names == ESYNC_NAMES || d->names == N_IBITS || d->names == N_IRATE ? names_turn(d, *vp, steps)   /* (a name a detent) */
                                    : clamp(*vp + accel(EN_K1 + slot, steps, d->max - d->min), d->min, d->max);
    *vp = (int16_t)v;
    if (pg->scope != SC_GLOBAL)
        ml_knob(TSEL, id);                                /* MIDI LEARN: this parameter (2.6) */
    if (!v)
        return;
    if (pg->scope == SC_GLOBAL && (id == G_LOAD || id == G_SAVE || id == G_CLRSEQ || id == G_INITSND || id == G_NEWPRJ)) {
        *vp = 0;                                          /* (wave 8) a GO asks; hold REC until the ring fills: done */
        ask_set(ASK_GO, id, d->label);
    }
}
/* a GO done (its question confirmed: hold REC) */
static void go_do(uint32_t id)
{
    switch (id) {
    case G_LOAD:
        project_load((uint32_t)song.g[G_SLOT] - 1u);
        break;
    case G_SAVE:
        project_save((uint32_t)song.g[G_SLOT] - 1u);
        break;
    case G_CLRSEQ:
        undo_mark(TSEL, (undo_sess += 4u) | 3u);
        fm1_irq_off();
        track_defaults_steps(TSEL);
        fm1_irq_on();
        ui_message("PATTERN CLEARED");
        break;
    case G_INITSND:
        set_engine(TSEL->eng_req);                              /* engine defaults + its first preset */
        ui_message("SOUND INIT");
        ui.force = 1;
        break;
    case G_NEWPRJ:
        project_new();
        ui_message("NEW LOOP");
        break;
    case 0xE8u:
        project_erase((uint32_t)song.g[G_SLOT] - 1u);
        break;
    default:
        break;
    }
}

/* ---- (wave 8, Ben 22 Nov 2026: "One rule everywhere: one latch gesture, one keep-open gesture, one confirm gesture, no
 * double presses") THE CONFIRM. Whatever erases or replaces asks first, in a white bar on the screen it was asked on: HOLD
 * REC: LOAD U07, HOLD REC: NEW, HOLD REC: REPLACE 606*, HOLD REC: CLEAR PADS, HOLD REC: STORE B. Hold REC: a ring fills
 * (HOLD_CLEAR_MS, as hold REC to clear the voice); full, it is done; let go before, nothing (the question stays). Any other
 * button, or another screen, drops the question; REC tapped drops it too (and records nothing). No second turn, no timing
 * to hit: it replaced "turn again within 1.5 s" (GO, USER, the kit), "the key again within 3 s" (SAVE + 5..8) and "KNOB 3
 * twice" (the lane's pads) */
static void stl_pads_clear(void);                         /* (ui_stut.c) */
static void kit_replace(void);                            /* (ui_drum.c) */
static void ask_set(uint32_t kind, uint32_t arg, const char *what)
{
    ui.ask = (uint8_t)kind;
    ui.ask_arg = (uint8_t)arg;
    ui_say("HOLD REC: ", what);
}
static void ask_drop(void)
{
    if (!ui.ask)
        return;
    ui.ask = 0;
    ui.msg_t = 0;
    ui.force = 1;
}
static void ask_do(void)
{
    uint32_t kind = ui.ask, a = ui.ask_arg;
    ui.ask = 0;
    ui.msg_t = 0;
    switch (kind) {
    case ASK_GO:
        go_do(a);
        break;
    case ASK_UP:
        up_ui(a & 3u, a >> 2);
        break;
    case ASK_KIT:
        kit_replace();
        break;
    case ASK_PADS:
        stl_pads_clear();
        break;
    case ASK_HALVE:                                       /* (wave 9) SEQ + OCT-: the second half dropped */
        seq_halve_do(a);
        break;
    case ASK_STORE: {
        char b[2] = {(char)('A' + (a & 3u)), 0};
        section_store(a & 3u);
        ui_say("SAVED ", b);
        break;
    }
    default:
        break;
    }
    ui.force = 1;
}

static void edit_param(uint32_t slot, int32_t steps) { edit_param_pg(cur_page(), slot, steps); }

/* SLOOP 2.6: the MIDI keyboard's notes as the UI sees them (seq.c step_in_q), every frame: recording, armed or in a free
 * take, a note for another track than the one recording says which one it plays (once a take). The notes themselves
 * sound, and set the GRID's pen, in seq.c (the old STEP page's entry is gone: REVIEW.md 2, 21 Nov 2026) */
static void midi_entry(void)                              /* (top of ui_input: every frame) */
{
    int rec_arm = song.rec || rec_wait;                   /* (2.6) recording or armed: on which track */
    uint32_t rec_trk = song.sel % NTRK;
    static uint8_t rh_st, rh_trk;                         /* its hint: 0 off, 1 not said yet, 2 said (or not needed) */
    if (!rec_arm)
        rh_st = 0;
    else if (!rh_st || rh_trk != rec_trk)
        rh_st = 1, rh_trk = (uint8_t)rec_trk;
    if (step_in_lost) {                                   /* (the queue overflowed: what is in it is unknown: dropped) */
        step_in_lost = 0;
        step_in_r = step_in_w;
    }
    while (step_in_r != step_in_w) {
        uint32_t ev = step_in_q[step_in_r % STEP_IN_Q], tr = (ev >> 8) % NTRK;
        step_in_r++;
        if (sr_on && tr == song.sel)                      /* (PHEBUS) STEP REC: the note goes on the GRID (ui_grid.c) */
            sr_event(ev & 127u, (ev >> 7) & 1u);
        if (!(ev & 0x80u))
            continue;
        if (rh_st == 1u && tr != rec_trk) {               /* (2.6, a YouTube comment) the keyboard plays another track
                                                         * than the one recording: nothing is recorded, say why (once a
                                                         * take, and not when MIDI plays that track too: an external
                                                         * sequencer on the other tracks does not flash it) */
            char b[2] = {(char)('1' + tr), 0};
            ui_say("MIDI PLAYS TRACK ", b);
            rh_st = 2;
        } else if (rec_arm && tr == rec_trk) {
            rh_st = 2;
        }
    }
}

/* HOME: tap on release, hold 0.7 s fires once. t0 = press time | 1,
 * bit 1 = fired (or swallowed: then the release is no tap either) */
enum { BT_NONE, BT_TAP, BT_HOLD };
static uint32_t btn_hold(uint32_t *t0, uint32_t label, uint32_t now, int hold_ok)
{
    uint32_t tap;
    if ((fm1_in.buttons >> panel.btn[label]) & 1u) {
        if (!*t0)
            *t0 = (now | 1u) & ~2u;
        else if (hold_ok && !(*t0 & 2u) && now - (*t0 & ~3u) > 700u * 1000u * FM1_TICKS_PER_US) {
            *t0 |= 2u;
            return BT_HOLD;
        }
        return BT_NONE;
    }
    tap = *t0 && !(*t0 & 2u);
    *t0 = 0;
    return tap ? BT_TAP : BT_NONE;
}

/* a layer button tapped (pressed and let go, nothing touched): (wave 9, Ben 23 Nov 2026) its group's first category, on it
 * already its next category (ui_nav.c). Held, the layer (PERFORM, ERASE, ROLL, STEPS, MIX, PATTERN): a shortcut or a
 * performance layer only */
static void layer_tap(uint32_t layer)
{
    static const uint8_t BTN[LY_COUNT] = {NB, B_FX, B_EDIT, B_ARP, B_SEQ, B_SCL, B_GLO, B_SAVE};
    if (layer == LY_STEP && sr_on)
        return;                                           /* (STEP REC: SEQ is the rest, ui_grid.c sr_input) */
    nav_button(BTN[layer % LY_COUNT]);
}

/* a layer locked open (wave 8, Ben 22 Nov 2026: "one keep-open gesture ... nothing deeper than a hold"): ANY layer button
 * held KEEP_MS (0.7 s, the long press) without touching anything, then let go: the layer stays (LOCK), both hands free for the
 * keys and the knobs. Any other button but PLAY, REC and OCT- / OCT+ lets it go, the same button too (and does only that:
 * its press is eaten). (Until wave 7 GLO and FX did this and every layer also stayed with HOME tapped while held: that
 * second gesture, a chord, is gone) */
static uint8_t home_eat;                                  /* HOME pressed to unlock: its tap is eaten */
static void layer_unlock(void)
{
    if (ly_lock != LY_PLAY) {
        ly_lock = LY_PLAY;
        ui.force = 1;
    }
}

/* #39 of Felucca: a layer that showed lets go; KNOB 1..4 belong to no page for LY_QUIET_MS after (the knob still
 * turning as the button comes up is the layer's, not the page's under it). fm1_ms | 1, 0 = none */
#define LY_QUIET_MS 250u
#define GLO_KEEP_MS LONG_MS                               /* a layer held this long, untouched, then let go: it stays */
static uint32_t ly_quiet_t;
static int ly_quiet(void)
{
    if (ly_quiet_t && fm1_ms - (ly_quiet_t & ~1u) >= LY_QUIET_MS)
        ly_quiet_t = 0;
    return ly_quiet_t != 0;
}
/* the layers, once a frame: which one is held (or locked), the taps on release, its keys and knobs.
 * Returns 1 while one is held or locked (the page does not take the knobs then) */
static int layers_input(uint32_t note_edges, uint32_t *pressed, uint32_t home)
{
    static uint8_t down[LY_COUNT], used[LY_COUNT], shown;   /* shown: a bit a layer, shown in this hold (no tap then) */
    static uint32_t t0[LY_COUNT];
    uint32_t l, now = fm1_ms, held = LY_PLAY, eat = 0;
    if (ly_lock != LY_PLAY) {
        uint32_t keep = 1u << panel.btn[B_PLAY] | 1u << panel.btn[B_REC] | 1u << panel.btn[B_OCTDN] | 1u << panel.btn[B_OCTUP];
        eat = *pressed & ~keep;
        if (eat) {
            layer_unlock();
            if (eat & 1u << panel.btn[B_HOME])
                home_eat = 1;
            *pressed &= ~eat;
        }
    }
    for (l = LY_FX; l < LY_COUNT; l++) {
        uint32_t d = (fm1_in.buttons & ly_bit[l] & ~ly_off) != 0u;   /* (STEP REC: SEQ and ARP are no layers) */
        if (d && !down[l]) {
            t0[l] = now;
            shown &= (uint8_t)~(1u << l);
            used[l] = (uint8_t)((eat & ly_bit[l]) != 0u || (ly_eat & ly_bit[l]) != 0u);   /* (the press that unlocked, or
                                                         * one the menu took: not a tap) */
            ly_eat &= ~ly_bit[l];
        }
        if (d && note_edges)
            used[l] = 1;                                  /* a key while held: not a tap */
        if (!d && down[l] && !used[l] && !((shown >> l) & 1u) && now - t0[l] < TAP_MS && !ui.menu && !ui.confirm)
            layer_tap(l);
        if (!d && down[l] && !used[l] && now - t0[l] >= GLO_KEEP_MS && ly_lock == LY_PLAY && !ui.menu && !ui.confirm &&
            !ui.hold_kind) {
            ly_lock = (uint8_t)l;                         /* held a while, nothing touched: it stays (GLO's mixer since
                                                           * 2026-10-28, Ariel; PERFORM wave 6; every layer wave 8); any
                                                           * other button, or the same again, lets it go */
            ui.force = 1;
        }
        down[l] = (uint8_t)d;
        if (d && held == LY_PLAY)
            held = l;
    }
    if (held == LY_PLAY && ly_lock != LY_PLAY) {
        held = ly_lock;
        used[held] = 1;
    }
    if (held != LY_PLAY && held != ui.layer && ui.layer != LY_PLAY)
        ui.layer = (uint8_t)held;                         /* (from one layer straight to another) */
    if (ui.layer != LY_PLAY)
        shown |= (uint8_t)(1u << ui.layer);               /* (seen: its button let go is no tap) */
    if (held == LY_PLAY) {
        while (lk_r != lk_w) {                            /* a key let go after its layer: its release only */
            uint32_t e = lk_q[lk_r % LKQ];
            lk_r++;
            if ((e >> 8) == KB_GRID) {                    /* the GRID: a step key (its release always) */
                if (!((e >> 7) & 1u) || grid_keys_on())
                    grid_key(e & 31u, (e >> 7) & 1u);
            } else if (!((e >> 7) & 1u)) {
                layer_key(e >> 8, e & 31u, 0);
            }
        }
        mix_pan = 0;                                      /* (the mixer closed: its knobs are the levels again) */
        if (ui.layer != LY_PLAY) {
            ly_quiet_t = fm1_ms | 1u;                     /* a layer shown lets go: KNOB 1..4 quiet a while (#39) */
            ui.layer = LY_PLAY;
            ui.force = 1;                                 /* the page comes back */
        }
        ui.step_sess = 0;
        return 0;
    }
    ui.layer_used = used[held];
    while (lk_r != lk_w) {                                /* the keys of SEQ, SEL, GLO (seq.c) */
        uint32_t e = lk_q[lk_r % LKQ];
        lk_r++;
        used[held] = 1;
        layer_key(e >> 8, e & 31u, (e >> 7) & 1u);
    }
    layer_knobs(held);
    if (ui.layer_used)
        used[held] = 1;
    if (ui.layer == LY_PLAY && (used[held] || now - t0[held] >= SHOW_MS)) {
        ui.layer = (uint8_t)held;                         /* show it (not for a quick tap) */
        shown |= (uint8_t)(1u << held);
    }
    if (held == LY_FX) {   /* (wave 6) PERFORM: OCT+ latches the pads held (a pad pressed with OCT+ held: seq.c), OCT- lets
                            * every latch go */
        uint32_t ob = 1u << panel.btn[B_OCTDN], pb = 1u << panel.btn[B_OCTUP];
        static uint32_t prev;
        uint32_t b = fm1_in.buttons & (ob | pb), press = b & ~prev;
        prev = b;
        if (press)
            used[held] = 1;
        if ((press & pb) && perf.held) {
            fm1_irq_off();
            perf_latch_held();
            fm1_irq_on();
            ui.force = 1;
        }
        if ((press & ob) && perf.lat) {
            fm1_irq_off();
            perf_unlatch();
            fm1_irq_on();
            ui_message("LATCH OFF");
        }
    }
    if (held == LY_STEP && !stl_shown()) {                 /* (wave 9) SEQ + OCT- / OCT+: halve / double the pattern (a clone) */
        uint32_t ob = 1u << panel.btn[B_OCTDN], pb = 1u << panel.btn[B_OCTUP];
        static uint32_t prev;
        uint32_t b = fm1_in.buttons & (ob | pb), press = b & ~prev;
        prev = b;
        if (press) {
            used[held] = 1;
            if (press & ob)
                seq_halve();
            else
                seq_double();
        }
    }
    if (held == LY_ERASE) {                               /* EDIT + OCT- / OCT+: undo / redo */
        uint32_t ob = 1u << panel.btn[B_OCTDN], pb = 1u << panel.btn[B_OCTUP];
        static uint32_t prev;
        uint32_t b = fm1_in.buttons & (ob | pb), press = b & ~prev;
        prev = b;
        if (press) {
            used[held] = 1;
            if (press & ob)
                ui_message(undo_swap(0) ? "UNDO" : "NOTHING TO UNDO");
            else
                ui_message(undo_swap(1) ? "REDO" : "NOTHING TO REDO");
        }
    }
    return 1;
}

/* the REC screen while armed (ui_studio.c rec_screen_draw): KNOB 3 START note / count (a setting of the FM-1: panel.c
 * lights_word, saved once stopped). (Wave 8: its KNOB 2 STEPS was the pattern's length a second time, hold SEQ's: gone) */
static void rec_knobs(void)
{
    int32_t s;
    panel_enc(EN_K1);
    panel_enc(EN_K2);
    if ((s = panel_enc(EN_K3)) != 0) {
        rec_count = (uint8_t)(s > 0);
        settings_later = 1;
        ui.hot_col = 2, ui.hot_t = 40;
    }
    panel_enc(EN_K4);
}

/* REC: acts on the press (no lag). Held 0.7 s the press is undone and a ring fills: held on to the
 * end, the selected track is cleared (EDIT + OCT- brings it back); let go before, nothing happens.
 * (SAVE is the sections layer: ui_layers.c; tapped, layer_tap) */
static void holds_input(uint32_t pressed, uint32_t now_ms)
{
    static uint8_t prev_rec, prev_wait, prev_sr, rec_on, save_on;   /* *_on: pressed, *_t0 its time */
    static uint32_t rec_t0, save_t0;
    uint32_t rb = 1u << panel.btn[B_REC], sb = 1u << panel.btn[B_SAVE];
    uint32_t rec_down = (fm1_in.buttons & rb) != 0u, save_down = (fm1_in.buttons & sb) != 0u;
    /* REC */
    if ((pressed & rb) && ui.ask) {                       /* (wave 8) a question showing: REC held is yes (the ring fills from
                                                          * the press); tapped, no. Nothing records */
        rec_on = 0;
        ui.hold_kind = 3;
        ui.hold_t0 = now_ms;
        ui.force = 1;
    } else if (pressed & rb) {
        prev_rec = song.rec;
        prev_wait = rec_wait;
        prev_sr = sr_on;
        rec_t0 = now_ms;
        rec_on = 1;
        if (sr_on)                                        /* (PHEBUS) a synth track's GRID: STEP REC ends; */
            sr_set(0);
        else if (sr_here() && !song.playing && !song.rec && !rec_wait)
            sr_set(1);                                    /* stopped, it starts (playing: live, as ever) */
        else
            rec_toggle();
    }
    if (ui.hold_kind == 3u) {                             /* the confirm's ring */
        if (rec_down && now_ms - ui.hold_t0 >= HOLD_CLEAR_MS) {
            ui.hold_kind = 0;
            ask_do();
        } else if (!rec_down) {
            ui.hold_kind = 0;                             /* let go before: nothing (a tap: the question goes too) */
            if (now_ms - ui.hold_t0 < TAP_MS)
                ask_drop();
            ui.force = 1;
        }
        return;
    }
    if (rec_down && rec_on && !ui.hold_kind && now_ms - rec_t0 >= LONG_MS) {
        song.rec = prev_rec;                              /* a hold: the press is undone */
        rec_wait = prev_wait;
        if (sr_on != prev_sr)
            sr_set(prev_sr);
        ui.msg_t = 0;
        ui.hold_kind = 1;
        ui.hold_t0 = now_ms;
        ui.hold_trk = song.sel;
        ui.force = 1;
    }
    if (ui.hold_kind == 1u && rec_down && now_ms - ui.hold_t0 >= HOLD_CLEAR_MS) {
        track_t *t = &trk[ui.hold_trk % NTRK];
        char b[12] = "1 CLEARED";
        undo_mark(t, (undo_sess += 4u) | 3u);
        fm1_irq_off();
        track_defaults_steps(t);
        t->nheld = 0;                                     /* and the latched arp chord */
        t->arp_phys = 0;
        fm1_irq_on();
        b[0] = (char)('1' + ui.hold_trk);
        ui_say("TRACK ", b);
        ui.hold_kind = 0;
        rec_on = 0;                                       /* (until REC is up: nothing more) */
        ui.force = 1;
    }
    if (!rec_down) {
        if (ui.hold_kind == 1u) {
            ui.hold_kind = 0;                             /* let go before the end: nothing */
            ui.force = 1;
        }
        rec_on = 0;
    }
    (void)sb, (void)save_down, (void)save_on, (void)save_t0;   /* (SAVE is a layer now: ui_layers.c) */
}

static void ui_input(void)
{
    uint32_t pressed = fm1_input_edges(0), notes = fm1_input_note_edges(), now = fm1_ticks(), id, b, k;
    uint32_t home = btn_hold(&ui.home_t0, B_HOME, now, 1);
    int32_t s;
    int layered;
    enc_hold = 0;                                       /* (panel.c: every knob readable again this pass) */
    if (pressed || notes)
        ui_input_ms = fm1_ms;
    pc_poll();                                          /* MIDI Program Change: a sound (2.6, ui.c) */
    midi_chan_fix();                                    /* GLO > MIDI: no channel on two tracks (2.6, ui.c) */
    ml_poll();                                          /* MIDI LEARN: a CC came for the picked parameter (2.6) */
    page_fix();                                         /* (a page the track does not have: its neighbour, 2.6) */
    drum_keys_fix();                                    /* (PHEBUS wave 3: MUTE / CHROMATIC end with the drum screen) */
    midi_entry();                                       /* (the MIDI notes' queue: read every frame, 2.6) */
    if (perf.full) {                                    /* (wave 6) a pad recorded found the STUTTER lane full */
        perf.full = 0;
        ui_message("LANE FULL: 31 PADS");
    }
    if (home == BT_HOLD) {                              /* HOME held: open the menu, or leave it */
        if (ui.menu) {
            menu_close();
        } else {
            layer_unlock();
            ui.menu = 1;
            ui.menu_sel = 0;
            ui.confirm = 0;
            ui.force = 1;
            song.seq_mode = 0;
        }
    }
    if (sr_on && (!sr_here() || song.sel != sr_trk || ui.menu))
        sr_set(0);                                      /* (STEP REC: the GRID or its track left) */
    kb_grid = (uint8_t)(grid_keys_on() && !sr_on && !(song.rec && sr_here()));   /* (seq.c: the keys are the grid's
                                                         * steps; STEP REC and a live take on a synth GRID: notes) */
    if (sg_held && (!kb_grid || song.sel != sg_trk))
        sg_drop();                                      /* (the GRID left with steps held: their keys clear nothing) */
    if (ui.menu) {                                      /* (wave 9) the menu is GLO's DEVICE category: SEL its next section,
                                                         * GLO the next category (GLOBAL), HOME tapped HOME; another page
                                                         * button closes it and opens its own group */
        uint32_t other = pressed & (BTN_BIT(B_FX) | BTN_BIT(B_ENV) | BTN_BIT(B_LFO) | BTN_BIT(B_EDIT) | BTN_BIT(B_ARP) |
                                    BTN_BIT(B_SEQ) | BTN_BIT(B_SAVE));
        if (home == BT_TAP) {
            menu_close();
            return;
        }
        if (!other) {
            if (pressed & BTN_BIT(B_SCL))
                nav_page();
            else if (pressed & BTN_BIT(B_GLO)) {
                ly_eat |= BTN_BIT(B_GLO);               /* (its tap, after the menu, is not one more press) */
                nav_button(B_GLO);
            } else if (!ui.home_t0)
                menu_input(pressed);
            return;
        }
        menu_close();
    }
    if (ui.ask && (pressed & ~(1u << panel.btn[B_REC])))   /* (wave 8) another button: the question goes */
        ask_drop();
    layered = layers_input(notes, &pressed, home);
    if (home_eat && !((fm1_in.buttons >> panel.btn[B_HOME]) & 1u)) {   /* (the HOME that unlocked: let go) */
        if (home == BT_TAP)
            home = BT_NONE;
        home_eat = 0;
    }
    pressed &= ~((ly_bit[LY_FX] | ly_bit[LY_ERASE] | ly_bit[LY_ROLL] | ly_bit[LY_STEP] | ly_bit[LY_SCALE] | ly_bit[LY_MIX] |
                  ly_bit[LY_SONG]) & ~ly_off);           /* (STEP REC: SEQ and ARP reach the GRID, ui_grid.c sr_input) */
    holds_input(pressed, fm1_ms);
    pressed &= ~((1u << panel.btn[B_REC]) | (1u << panel.btn[B_SAVE]));
    if (layered || ui.hold_kind) {                      /* a layer / a hold: the rest waits */
        panel_enc(EN_SELECT);                           /* (wave 8: no tempo here; the tempo is GLO > GLOBAL's BPM only) */
        panel_enc(EN_ALGO);                             /* (track and sound wait: no jump afterwards) */
        panel_enc(EN_PRESET);
        if (ui.hold_kind)
            for (k = 0; k < 4u; k++)
                panel_enc(EN_K1 + k);
        pressed &= ~((1u << panel.btn[B_OCTDN]) | (1u << panel.btn[B_OCTUP]));   /* (undo, pages, ghost / hard) */
        if (!(pressed & (1u << panel.btn[B_PLAY])))
            return;
        pressed &= 1u << panel.btn[B_PLAY];             /* PLAY still plays */
        enc_hold = (1u << NE) - 1u;                     /* #102: the knobs the layer took are not read again this
                                                         * pass (a detent counted since went to the page too) */
    } else if (ly_quiet()) {                            /* #39: the turns as a layer lets go are dropped */
        for (k = 0; k < 4u; k++)
            panel_enc(EN_K1 + k);
        enc_hold |= 15u << EN_K1;
    }
    if (rec_wait && !ci_on) {                           /* the REC screen, armed: how it records */
        rec_knobs();                                    /* (tempo and sound still work; the track too: */
    } else if (rec_wait) {                              /* the arm follows) */
        for (k = 0; k < 4u; k++)                        /* the count-in: KNOB 1..4 edit nothing */
            panel_enc(EN_K1 + k);
    }
    /* (wave 9, Ben 23 Nov 2026) the page buttons, the same on every screen, a step held too: ENV, LFO and HOME their group's
     * next category (the layer buttons: on release, layer_tap); SEL (PAGE) the next page of the category shown. HOME acts on
     * release (held: the menu, its shortcut) */
    if (pressed & BTN_BIT(B_SCL))
        nav_page();
    if (pressed & BTN_BIT(B_ENV))
        nav_button(B_ENV);
    if (pressed & BTN_BIT(B_LFO))
        nav_button(B_LFO);
    if (home == BT_TAP)
        nav_button(B_HOME);
    pressed &= ~(BTN_BIT(B_SCL) | BTN_BIT(B_ENV) | BTN_BIT(B_LFO));
#if FELUCCA_ARRANGER
    if (on_drum_page()) {
        drum_screen_input(pressed, 0);
        return;
    }
#endif
    cursor_fix();                                       /* LEN may have changed (knob, editor, load) */
    for (id = 0; id < 14u; id++) {
        if (!((pressed >> id) & 1u))
            continue;
        b = panel_btn_of(id);
        switch (b) {
        case B_PLAY:
            transport_req = song.playing ? 2 : 1;
            break;
        case B_OCTDN:
        case B_OCTUP: {
            uint32_t both = (1u << panel.btn[B_OCTDN]) | (1u << panel.btn[B_OCTUP]);
            if (is_drum(TSEL))
                break;                                  /* the drum track: ghost / hard while held (seq.c) */
            if ((fm1_in.buttons & both) == both)
                song.octave = 0;
            else
                song.octave += b == B_OCTDN ? (song.octave > -3 ? -1 : 0) : (song.octave < 3 ? 1 : 0);
            break;
        }
        default:
            break;
        }
    }

    if ((s = panel_enc(EN_PRESET)) != 0 && (ui.home || cur_page()->graph == GR_BROWSE || cur_fam() == FAM_TRK)) {
        /* PRESETS browses the selected part's presets (all engines, then user presets) on HOME, the PRESETS
         * page and TRACKS only (the drum track: its kits); elsewhere a stray turn would throw away the sound
         * being edited. Every detent counts */
        uint32_t total, cur = preset_pos(&total);
        if (is_drum(TSEL))
            TDRUM->p[P_E0] = drum_kit_step(TDRUM->p[P_E0], s);
        else if (total)
            preset_go((uint32_t)(((int32_t)cur + s % (int32_t)total + (int32_t)total) % (int32_t)total));
    }
    if ((s = panel_enc(EN_ALGO)) != 0)              /* ALGORITHM: the selected track, on every page */
        track_select((uint32_t)clamp((int32_t)song.sel + (s > 0 ? 1 : -1), 0, NTRK - 1));
    if (vis_shown() && !rec_wait) {                     /* a screensaver: KNOB 1..4 (the HOME screen's, out of sight) and
                                                         * SELECT do nothing (its style: SEL, the page button) */
        panel_enc(EN_SELECT);
        for (k = 0; k < 4u; k++)
            panel_enc(EN_K1 + k);
    }
    if ((s = panel_enc(EN_SELECT)) != 0 && !rec_wait)  /* (wave 9) SELECT: the type of the page shown (none: nothing) */
        sel_turn(s);
    for (k = 0; k < 4u; k++) {
        const page_t *pg = cur_page();
        int16_t *hv;
        if ((s = panel_enc(EN_K1 + k)) == 0)
            continue;
        if (ui.home || pg->scope == SC_TRK || page_desc(pg, k, &hv) ||
            (pg->graph == GR_USER && k == 0u)) {     /* (not an empty column, nor "DRUM TRACK") */
            ui.hot_col = (uint8_t)k;
            ui.hot_t = 40;
        }
        if (ui.home) {
            int16_t *vp;
            const param_desc_t *d = home_param(k, &vp);
            *vp = (int16_t)clamp(*vp + accel(EN_K1 + k, s, d->max - d->min), d->min, d->max);
            if (!is_drum(TSEL))
                ml_knob(TSEL, ENGINES[TSEL->eng_req % NENGINES]->macro[k & 3u]);   /* MIDI LEARN (2.6) */
        } else {
            edit_param(k, s);
        }
    }
}

/* ---------------------------------------------------- panel setup --- */
/* 30 s without input: give up and keep the old table (a stuck key cannot hang the boot) */
#define SETUP_IDLE_MS 30000u
static void panel_setup(void)
{
    uint32_t i, used = 0, t0 = fm1_ms;
    const panel_t old = panel;
    lcd_fill(0, 0, 240, 240, C_BLACK);
    draw_text_box(0, 10, 240, &FONT_S, "HARDWARE CALIBRATION", C_WHITE, 1);
    draw_text_box(0, 30, 240, &FONT_S, "TEACH EACH BUTTON AND KNOB", C_GRAY, 1);
    while (fm1_in.buttons) {                             /* wait for OCT-/OCT+ release */
        fm1_wdt_feed();
        if (fm1_ms - t0 > SETUP_IDLE_MS)
            goto timeout;
    }
    fm1_input_edges(0);
    for (i = 0; i < NB; i++) {
        uint32_t p = 0, id;
        draw_text_box(0, 80, 240, &FONT_S, "PRESS", C_GRAY, 1);
        draw_text_box(0, 100, 240, &FONT_L, B_NAME[i], C_WHITE, 1);
        t0 = fm1_ms;
        while (!(p & ~used)) {
            fm1_wdt_feed();
            p |= fm1_input_edges(0);
            if (fm1_ms - t0 > SETUP_IDLE_MS)
                goto timeout;
        }
        for (id = 0; id < 14u; id++)
            if (((p & ~used) >> id) & 1u)
                break;
        panel.btn[i] = (uint8_t)id;
        used |= 1u << id;
    }
    used = 0;
    for (i = 0; i < NE; i++) {
        uint32_t e;
        int32_t st = 0;
        draw_text_box(0, 80, 240, &FONT_S, "TURN RIGHT", C_GRAY, 1);
        draw_text_box(0, 100, 240, &FONT_L, E_NAME[i], C_WHITE, 1);
        for (e = 0; e < 7u; e++)
            fm1_enc_take(e);
        t0 = fm1_ms;
        for (;;) {
            fm1_wdt_feed();
            if (fm1_ms - t0 > SETUP_IDLE_MS)
                goto timeout;
            for (e = 0; e < 7u; e++)
                if (!((used >> e) & 1u) && (st = fm1_enc_take(e)) != 0)
                    break;
            if (e < 7u)
                break;
        }
        panel.enc[i] = (uint8_t)e;
        panel.dir[i] = (int8_t)(st > 0 ? 1 : -1);
        used |= 1u << e;
        fm1_delay_ms(300);
        fm1_enc_take(e);
    }
    panel.magic = PANEL_MAGIC;
    lcd_fill(0, 0, 240, 240, C_BLACK);
    layers_init();
    ui.force = 1;
    return;
timeout:
    panel = old;
    lcd_fill(0, 0, 240, 240, C_BLACK);
    ui.force = 1;
    ui_message("SETUP CANCELLED");
}
