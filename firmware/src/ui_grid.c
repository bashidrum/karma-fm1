/* SPDX-License-Identifier: GPL-3.0-only */
/* The step GRID, on every track (Ben, SPEC.md 20 Nov 2026: "the step sequencers should copy Elektron as much as
 * possible. Press a key to enter a step. Choose a note, and place it into a step. You can param lock per step").
 * Included by ui_studio.c. The GRID is the DRUMS screen's page (the SC_DRUM page): on the drum track it is the drum
 * grid as before, on a synth track the synth's steps. With no layer held the 16 white keys are the 16 steps of the
 * page of steps shown (seq.c kb_grid: they play nothing):
 *   tap a step       an empty one is set and heard (synth: the pen, the note / chord played last or KNOB 1 picks;
 *                    drums: the sound KNOB 1 picks), a set one is cleared when its key comes up (unless a knob
 *                    changed it meanwhile). A synth note is heard whenever it changes (the pen, a step's NOTE)
 *   hold a step      KNOB 1..4 edit it (synth: NOTE in the scale, LEN, VEL, COND; SELECT: FLAG, RATCH, NUDGE;
 *                    drums, the picked sound's trig (PHEBUS wave 2): VEL, RETRIG, NUDGE, COND; SELECT: CHANCE, LENGTH,
 *                    CURVE, the last two the retrig's); ENV / LFO / FX / EDIT tapped: that button's page on the
 *                    knobs, and each knob turned LOCKS its parameter on the step (again: the next page; the drum track:
 *                    on its sound's EDIT pages, HEX OSC .. HEX AEG, the lock is the picked sound's alone; on its own
 *                    pages, FILTER, FX, INSERT, SLICER, the whole drum track's); PRESETS
 *                    and ALGORITHM do nothing (KARMA's hidden lock of the last knob touched is cut); OCT- the step's
 *                    locks go (on a lock page: that page's only; on the grid: locks, nudge and condition, a drum trig's
 *                    retrig too), OCT+ its condition
 *   black keys 1..4  the page of steps (1-16 .. 49-64); OCT- / OCT+ (no step held) the page before / after
 * The screen shows the steps in the track's colour, the playhead, the page, and what the step held (or under the
 * cursor) holds: its notes, length, velocity, flags, condition, nudge and every lock. */
/* (the bands of ui_op.c, included after this: OP_BODY_Y, OP_KNOB_Y) */
#define SG_BODY_Y 22u
#define SG_KNOB_Y 154u
#define SG_SCENE_H 96u
static uint16_t sg_held;                                /* the white keys held on the grid (steps of the page shown) */
static uint16_t sg_pend;                                /* of those, the set steps that clear when let go (untouched) */
static uint8_t sg_alt;                                  /* a synth step held: 0 NOTE LEN VEL COND, 1 FLAG RATCH NUDGE */
static uint8_t sg_lpage = 0xFF;                         /* a step held + ENV / LFO / FX / EDIT: the page that locks */
static uint8_t sg_trk;                                  /* the track the steps held are on (from the first key down) */
static void stl_key(uint32_t k, uint32_t down);         /* (ui_stut.c) */
/* (PHEBUS wave 4) the synth GRID's tabs, TRIG OSC FILT ENV FX MOD: the drawing and the knobs' values are ui_synth.c's */
static const char *const SYN_TAB[6] = {"TRIG", "OSC", "FILT", "ENV", "FX", "MOD"};
static uint8_t syn_tab = 0;                             /* the GRID's tab: 0 TRIG, 1 .. 5 the stages OSC .. MOD */
static uint8_t syn_half[6];                             /* each tab's half shown */
static uint8_t syn_tab_last = 1;                        /* the stage before TRIG (SEQ again goes back) */
static uint8_t syn_touch = 0xFF;                        /* a step held: the parameter last locked in this hold (OCT-) */
static void edit_param_pg(const page_t *pg, uint32_t slot, int32_t steps);   /* (ui_input.c) */
static void syn_tabs(uint32_t from, uint32_t sel, uint32_t h, uint32_t nh, const char *banner, uint32_t *cache);   /* (ui_synth.c) */
static void syn_trig_vals(int32_t first, uint32_t half, const char *lab[4], char v[4][12], int32_t ratio[4], uint32_t *dim);
static void syn_vals(const page_t *pg, int32_t first, const char *lab[4], char v[4][12], int32_t ratio[4], uint32_t *locked,
                     uint32_t *dim);
static void op_dials(const char *const lab[4], const char *const val[4], const int32_t ratio[4], uint32_t locked,
                     uint32_t dim, uint16_t col, uint32_t sig, uint32_t *cache);   /* (ui_drum.c) */
static void op_names_str(const track_t *t, char *b);   /* (ui_op.c) */

static uint32_t syn_halves(uint32_t tab)               /* TRIG two; a stage its pages */
{
    uint32_t n = 0;
    if (!tab)
        return 2u;
    while (n < 8u && stage_page(tab - 1u, n) < NPAGES)
        n++;
    return n ? n : 1u;
}
static uint32_t syn_hcur(void)
{
    uint32_t n = syn_halves(syn_tab);
    return syn_half[syn_tab % 6u] < n ? syn_half[syn_tab % 6u] : 0u;
}
static uint32_t syn_page(void) { return syn_tab ? stage_page(syn_tab - 1u, syn_hcur()) : NPAGES; }   /* (PAGES index) */
static void syn_go(uint32_t tab, uint32_t half)
{
    if (tab)
        syn_tab_last = (uint8_t)tab;
    syn_tab = (uint8_t)(tab % 6u);
    syn_half[syn_tab] = (uint8_t)half;
    syn_touch = 0xFF;
    ui.force = 1;
}
static void syn_walk(int32_t s)                         /* SELECT: every half of every tab, TRIG first, to both ends */
{
    int32_t tb = syn_tab, h = (int32_t)syn_hcur();
    for (; s; s += s > 0 ? -1 : 1) {
        if (s > 0) {
            if (h + 1 < (int32_t)syn_halves((uint32_t)tb))
                h++;
            else if (tb < 5)
                tb++, h = 0;
        } else {
            if (h > 0)
                h--;
            else if (tb > 0)
                tb--, h = (int32_t)syn_halves((uint32_t)tb) - 1;
        }
    }
    syn_go((uint32_t)tb, (uint32_t)h);
}
/* a page button on the synth GRID (a step held or not, the Rytm's page keys): EDIT OSC, again the next stage; ENV, FX,
 * LFO their stage, again its next half */
static void syn_open(uint32_t fam)
{
    uint32_t tab = fam == FAM_FLT ? 2u : fam == FAM_ENV ? 3u : fam == FAM_FX ? 4u : fam == FAM_MOD ? 5u : 1u;
    if (fam == FAM_EDIT && syn_tab >= 1u)
        tab = syn_tab % 5u + 1u;                        /* OSC > FILT > ENV > FX > MOD > OSC */
    if (fam != FAM_EDIT && tab == syn_tab)
        syn_go(tab, (syn_hcur() + 1u) % syn_halves(tab));
    else
        syn_go(tab, 0);
}
static void syn_seq_tap(void)                           /* SEQ on the synth GRID: TRIG, again the stage before it */
{
    if (syn_tab)
        syn_go(0, syn_half[0]);
    else
        syn_go(syn_tab_last, syn_half[syn_tab_last % 6u]);
}
/* SEQ on a synth's sound page: its GRID, on the same page's tab (the knobs stay where they were; the keys become steps) */
static void syn_grid_from_page(void)
{
    const page_t *pg = cur_page();
    int32_t st = ui.home ? -1 : fam_stage(pg->fam);
    uint32_t k;
    if (st >= 0 && !is_drum(TSEL)) {
        for (k = 0; k < 8u && stage_page((uint32_t)st, k) < NPAGES && stage_page((uint32_t)st, k) != ui.page; k++)
            ;
        syn_go((uint32_t)st + 1u, stage_page((uint32_t)st, k) == ui.page ? k : 0u);
    }
}


static int drum_grid_black(uint32_t k, uint32_t down);  /* (ui_drum.c) */
static uint8_t drum_newvel;                             /* (ui_drum.c: a new drum trig's velocity) */
static int32_t lk_val(uint32_t id, uint32_t v);         /* (ui_drum.c: a sound lock's value) */
static uint32_t stl_keys_lit(uint32_t blink);
static const char *const SG_COND[4] = {"-", "FILL", "NOFL", "CHNC"};
static const char *const SG_COND_L[4] = {"", "FILL ONLY", "NO FILL", "CHANCE"};
static const char *const SG_FLAG[4] = {"-", "ACC", "SLD", "A+S"};

static uint32_t sg_cur(void) { return is_drum(TSEL) ? drum_cursor : ui.cursor; }
static uint32_t sg_page(void) { return sg_cur() / 16u; }
static void sg_cur_set(uint32_t c)
{
    if (is_drum(TSEL)) {
        drum_cursor = (uint8_t)c;
    } else {
        ui.cursor = (uint8_t)c;
        ui.bank = (uint8_t)(c / 16u);
        ui.entry_open = 0;
    }
}
/* the first step held (its index in the pattern), or -1 */
static int32_t sg_first(void)
{
    uint32_t w;
    for (w = 0; w < 16u; w++)
        if ((sg_held >> w) & 1u)
            return (int32_t)(sg_page() * 16u + w);
    return -1;
}
static int sg_on(const track_t *t, uint32_t idx)
{
    return is_drum(t) ? dt_has(idx, drum_lane) : step_on(&t->step[idx]);
}
static uint32_t sg_sess;                                /* the undo session of the steps held (one hold: one undo) */
static void sg_undo(track_t *t)
{
    undo_mark(t, sg_held ? sg_sess : ui.step_sess ? ui.step_sess : (ui.step_sess = (undo_sess += 4u) | 3u));
}
/* a note moved d degrees of the track's scale (CHR: semitones), 1..127 */
static int32_t sg_scale_move(const track_t *t, int32_t n, int32_t d)
{
    uint32_t mask = scale_mask(t), root = (uint32_t)t->p[P_ROOT] % 12u;
    int32_t dir = d > 0 ? 1 : -1, guard;
    for (; d; d -= dir)
        for (guard = 0; guard < 12; guard++) {
            n += dir;
            if (n < 1 || n > 127)
                return clamp(n, 1, 127);
            if ((mask >> ((uint32_t)(n + 120 - (int32_t)root) % 12u)) & 1u)
                break;
        }
    return n;
}
/* the pen (the note / chord a new synth step takes) as text: "C4 E4 G4" */
static void sg_notes_str(char *b, const uint8_t *nt, uint32_t n, uint32_t max)
{
    uint32_t i;
    b[0] = 0;
    for (i = 0; i < n && i < 4u && str_len(b) + 5u < max; i++) {
        char nm[6];
        note_name(nm, nt[i]);
        if (i)
            str_cpy(b + str_len(b), " ", 2);
        str_cpy(b + str_len(b), nm, 6);
    }
}

/* ------------------------------------------------------------ steps --- */
static void sg_set(track_t *t, uint32_t idx)            /* an empty step set: the pen (synth) or drum_lane, NORM */
{
    sg_undo(t);
    fm1_irq_off();
    if (is_drum(t)) {
        dt_set(idx, drum_lane, drum_newvel);
    } else {
        step_t *s = &t->step[idx];
        uint32_t i, n = pen_n ? pen_n : 1u, tie = s->time == ST_TIE;
        memset(s, 0, sizeof *s);
        for (i = 0; i < n && i < 4u; i++)
            s->note[i] = pen_n ? pen_note[i] : last_note;
        s->n = (uint8_t)(t->p[P_VOICE] == V_POLY ? n : 1u);
        s->time = ST_NOTE;
        s->vel = 100;
        if (tie)                                        /* set on a TIE: the note before ends here, this one is one
                                                         * step long (the rest of the old tail goes, not to it) */
            note_set_len(t, idx, 1);
    }
    t->seq_active = 1;
    fm1_irq_on();
    if (is_drum(t))
        audition_lane(drum_lane);
    else                                                /* heard as it is placed (REVIEW.md 1: play on change) */
        audition_notes(song.sel, t->step[idx].note, t->step[idx].n);
    sync_reload = 1;
}
static void sg_clear(track_t *t, uint32_t idx)          /* a set step cleared (drums: the sound's hit) */
{
    sg_undo(t);
    fm1_irq_off();
    if (is_drum(t)) {
        dt_clr(idx, drum_lane);                         /* (the trig's extras and locks with it) */
    } else {
        note_set_len(t, idx, 1);                        /* (its TIEs go with it) */
        step_clear(&t->step[idx]);
    }
    if (!is_drum(t) || !dt_mask(idx)) {                 /* an empty step keeps no lock, no nudge, no condition */
        lock_del(t, idx, P_COUNT);
        t->micro[idx] = 0;
        step_fill_set(t, idx, FC_NORM);
    }
    fm1_irq_on();
    sync_reload = 1;
}
/* the steps held let go of without a change (the GRID left, or another track: a key let go later clears nothing) */
static void sg_drop(void)
{
    sg_held = sg_pend = 0;
    sg_lpage = 0xFF;
    sg_alt = 0;
}
/* a page of steps added after the last, a copy of it (its steps, nudges, conditions and locks; REVIEW.md 4, 21 Nov
 * 2026): the pattern 16 steps longer, the GRID on the new page, "32 STEPS" said. Never past 64; a pattern whose
 * last page is short is first made whole (its steps past the end, out of sight, emptied) */
static void sg_page_add(track_t *t)
{
    uint32_t len = trk_len(t), pages = (len + 15u) / 16u, src = (pages - 1u) * 16u, dst = pages * 16u, i, k, lost = 0;
    char b[24];
    if (pages >= 4u) {
        ui_message("64 STEPS: THE MOST");
        return;
    }
    sg_undo(t);
    fm1_irq_off();
    for (i = len; i < dst; i++) {                       /* (the short last page made whole: empty steps) */
        memset(&t->step[i], 0, sizeof t->step[i]);
        if (!is_drum(t))
            t->step[i].time = ST_REST;
        else
            for (k = 0; k < DRUM_LANES; k++)
                dt_clr(i, k);
        lock_del(t, i, P_COUNT);
        t->micro[i] = 0;
        step_fill_set(t, i, FC_NORM);
    }
    for (i = 0; i < 16u; i++) {
        t->step[dst + i] = t->step[src + i];
        t->micro[dst + i] = t->micro[src + i];
        step_fill_set(t, dst + i, step_fill(t, src + i));
        lock_del(t, dst + i, P_COUNT);
        if (is_drum(t))                                 /* (the drum trigs, their extras and sounds' locks) */
            lost += !dt_copy_step(src + i, dst + i);
    }
    for (k = 0; k < NLOCK; k++)
        if (t->lock[k].step >= src && t->lock[k].step < src + 16u)
            lost += !lock_set(t, t->lock[k].step + 16u, t->lock[k].param, t->lock[k].val);
    t->p[P_SLEN] = (int16_t)(dst + 16u);
    fm1_irq_on();
    sync_reload = 1;
    sg_cur_set(dst + sg_cur() % 16u);
    ui.force = 1;
    fmt_int(b, (int32_t)(dst + 16u));
    ui_say(b, lost ? " STEPS, LOCKS FULL" : " STEPS");
}
/* the shown page follows the playhead as it goes onto another page (a page picked by hand stays until it does) */
static void sg_follow(void)
{
    static uint8_t was = 0xFF;
    const track_t *t = TSEL;
    uint32_t pg = song.playing ? t->seq_idx % trk_len(t) / 16u : 0xFFu;
    if (pg != was && pg != 0xFFu && !sg_held && pg != sg_page()) {
        sg_cur_set(pg * 16u + sg_cur() % 16u < trk_len(t) ? pg * 16u + sg_cur() % 16u : trk_len(t) - 1u);
        ui.force = 1;
    }
    was = (uint8_t)pg;
}
/* the GRID shown, no layer held: its keys are steps (seq.c kb_grid). The drum track's MUTE and CHROMATIC modes: the
 * keys mute, play (PHEBUS wave 3) */
static int grid_keys_on(void) { return on_drum_page() && (song.sel != TRK_DRUM || drum_keys == DK_PLAY) && !ui.menu; }
/* a key of the GRID (seq.c lk_q KB_GRID), down or up: a white key is a step of the page shown, the first four
 * black keys pick the page (steps 1-16, 17-32, 33-48, 49-64). The cursor follows */
static void grid_key(uint32_t k, uint32_t down)
{
    static const int8_t PG[12] = {-1, 0, -1, 1, -1, 2, -1, -1, 3, -1, -1, -1};
    track_t *t = TSEL;
    uint32_t len = trk_len(t), idx;
    int32_t w = punch_key(k);
    if (stl_view && song.sel == TRK_DRUM) {             /* (PHEBUS) the STUTTER lane's (ui_stut.c) */
        stl_key(k, down);
        return;
    }
    if (w < 0) {
        if (drum_grid_black(k, down))                   /* (PHEBUS wave 3: the drum GRID's F#5, FILL) */
            return;
        if (!down || sg_held || k >= 12u || PG[k] < 0)
            return;
        if ((uint32_t)PG[k] * 16u < len) {
            idx = (uint32_t)PG[k] * 16u + sg_cur() % 16u;
            sg_cur_set(idx < len ? idx : len - 1u);
            ui.force = 1;
        } else if ((uint32_t)PG[k] == (len + 15u) / 16u) {
            sg_page_add(t);                             /* the slot after the last: a page added */
        } else {
            char b[4] = {(char)('1' + (len + 15u) / 16u), 0};
            ui_say("ADD PAGE ", b);                     /* (one at a time: the slot after the last first) */
        }
        return;
    }
    if (!down) {
        uint32_t b = 1u << w;
        if (!(sg_held & b))
            return;
        sg_held &= (uint16_t)~b;
        idx = sg_page() * 16u + (uint32_t)w;
        if ((sg_pend & b) && idx < len && sg_on(t, idx))
            sg_clear(t, idx);                           /* a tap on a set step: it goes */
        sg_pend &= (uint16_t)~b;
        if (!sg_held) {
            sg_lpage = 0xFF;
            sg_alt = 0;
        }
        ui.force = 1;
        return;
    }
    idx = sg_page() * 16u + (uint32_t)w;
    if (idx >= len)
        return;
    if (!sg_held) {
        sg_cur_set(idx);
        sg_trk = (uint8_t)song.sel;
        sg_sess = (undo_sess += 4u) | 3u;               /* (the step set by this press, and every edit of the hold) */
    }
    sg_held |= (uint16_t)(1u << w);
    if (sg_on(t, idx))
        sg_pend |= (uint16_t)(1u << w);                 /* (cleared on release, if untouched) */
    else
        sg_set(t, idx);
    ui.force = 1;
}

/* ------------------------------------------------------------ locks --- */
/* (the drum sounds' locks, PHEBUS wave 3: the drum screen's pages, ui_drum.c drum_lock_turn) */
/* a page whose knobs can lock on the selected track: shown, a track's page, with a lockable parameter */
static int sg_lock_page_ok(const page_t *pg)
{
    uint32_t s;
    if (!page_shown(pg) || (pg->scope != SC_TRACK && pg->scope != SC_ENGINE) || (is_drum(TSEL) && !page_for_drum(pg)))
        return 0;
    for (s = 0; s < 4u; s++) {
        uint32_t id = pg_id(pg, s);
        if (id < P_COUNT && p_lockable(id) && lock_desc(TSEL, id)->max > lock_desc(TSEL, id)->min &&
            !(is_drum(TSEL) && id > P_E0 && id <= P_E7))
            return 1;
    }
    return 0;
}
/* the pages a button walks with a step held: EDIT the chain's first three stages, OSC > FILT > ENV (REVIEW.md 3,
 * 21 Nov 2026: the filter locks from the GRID), ENV / LFO / FX their own */
static int sg_fam_in(uint32_t pfam, uint32_t fam)
{
    return fam == FAM_EDIT ? pfam == FAM_OSC || pfam == FAM_FLT || pfam == FAM_ENV : pfam == fam;
}
/* the lock page's name ("FILTER 2"): the page's title, an engine page its engine's */
static const char *sg_lock_title(void)
{
    const page_t *pg = &PAGES[sg_lpage % NPAGES];
    return pg->scope == SC_ENGINE ? ENGINES[TSEL->eng_req % NENGINES]->page_title[pg->id[0] != P_E0] : pg->title;
}
/* a step held + a page button: its page on the knobs (again: the next one of that button that can lock, in the
 * chain's order, round) */
static void sg_lock_open(uint32_t fam)
{
    uint32_t i, start = 0, n;
    if (sg_lpage < NPAGES && sg_fam_in(PAGES[sg_lpage].fam, fam))
        start = sg_lpage + 1u;
    for (n = 0; n < NPAGES; n++) {
        i = (start + n) % NPAGES;
        if (sg_fam_in(PAGES[i].fam, fam) && sg_lock_page_ok(&PAGES[i])) {
            sg_lpage = (uint8_t)i;
            ui.force = 1;
            return;
        }
    }
    ui_message("NOTHING TO LOCK HERE");
}
/* the steps held (bits of page pg) can all take a lock of id: the new ones fit in the free slots. Else none is made
 * (never a half: the first steps locked, the rest not) and the screen says why */
static int locks_fit(const track_t *t, uint32_t pg, uint32_t held, uint32_t id)
{
    uint32_t w, need = 0, fr = 0;
    for (w = 0; w < NLOCK; w++)
        fr += t->lock[w].step == LOCK_FREE;
    for (w = 0; w < 16u; w++)
        need += ((held >> w) & 1u) && pg * 16u + w < trk_len(t) && lock_find(t, pg * 16u + w, id, 0) < 0;
    if (need <= fr)
        return 1;
    ui_message(fr ? "LOCKS FULL: HOLD FEWER STEPS" : "LOCKS FULL: 24 A TRACK");
    return 0;
}
/* the steps held: a lock of id moved s (made at the track's value first) */
static void sg_lock_turn(uint32_t id, int32_t s, uint32_t role)
{
    track_t *t = TSEL;
    uint32_t w, len = trk_len(t);
    int32_t sk;
    if (id >= P_COUNT || !p_lockable(id) || (is_drum(t) && id > P_E0 && id <= P_E7)) {
        ui_message("NOT LOCKABLE");
        return;
    }
    sk = accel(role, s, lock_desc(t, id)->max - lock_desc(t, id)->min);
    if (!locks_fit(t, sg_page(), sg_held, id))
        return;
    sg_undo(t);
    fm1_irq_off();
    for (w = 0; w < 16u; w++) {
        uint32_t idx = sg_page() * 16u + w;
        int k;
        if (!((sg_held >> w) & 1u) || idx >= len)
            continue;
        k = lock_find(t, idx, id, 0);
        if (!lock_set(t, idx, id, (k >= 0 ? t->lock[k].val : t->p[id]) + sk)) {
            fm1_irq_on();
            ui_message("NO LOCK LEFT");
            return;
        }
    }
    fm1_irq_on();
    sg_pend &= (uint16_t)~sg_held;                      /* (edited: kept when let go) */
    sync_reload = 1;
}

/* ------------------------------------------------------------ knobs --- */
/* (PHEBUS wave 2) a drum trig's own values, the picked sound's on step idx (none there: nothing): sl 0 VEL (1..127), 1 RETRIG
 * (OFF, 1/1 .. 1/80), 2 NUDGE, 3 COND (- FILL NOFL CHNC), 4 CHANCE (CHNC's, the Rytm's steps 1 .. 99 %), 5 LENGTH (the
 * retrig's, 1/8 step .. 16 steps, INF), 6 CURVE (the retrig's velocity, -128 fade out .. 127 fade in). The extras are made
 * when a value leaves its default and freed when all are back (core.h dtx_t); the pool full: said, nothing changed */
static const uint8_t SG_PROB[] = {1, 2, 4, 6, 9, 13, 19, 25, 33, 41, 50, 59, 67, 75, 81, 87, 91, 94, 96, 98, 99};
static const uint8_t SG_RLEN[] = {2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 255, 0};   /* (1/16 step; 0 INF) */
static const char *const SG_RATE[] = {"OFF", "1/1", "1/2", "1/3", "1/4", "1/5", "1/6", "1/8", "1/10", "1/12", "1/16",
                                      "1/20", "1/24", "1/32", "1/40", "1/48", "1/64", "1/80"};
static uint32_t sg_near(const uint8_t *tab, uint32_t n, uint32_t v)   /* the index of the entry nearest v */
{
    uint32_t i, b = 0, bd = 0xFFFFu;
    for (i = 0; i < n; i++) {
        uint32_t d = tab[i] > v ? tab[i] - v : v - tab[i];
        if (d < bd)
            bd = d, b = i;
    }
    return b;
}
static void sg_rlen_str(char *b, uint32_t v)            /* a retrig's length in steps: "1/8", "0.19", "1", "16", "INF" */
{
    if (!v) {
        str_cpy(b, "INF", 4);
    } else if (v % 16u == 0u) {
        fmt_int(b, (int32_t)(v / 16u));
    } else if (16u % v == 0u) {
        str_cpy(b, "1/", 3);
        fmt_int(b + 2, (int32_t)(16u / v));
    } else {
        uint32_t c = (v % 16u) * 100u / 16u;
        fmt_int(b, (int32_t)(v / 16u));
        b[str_len(b) + 3u] = 0;
        b[str_len(b) + 2u] = (char)('0' + c % 10u);
        b[str_len(b) + 1u] = (char)('0' + c / 10u);
        b[str_len(b)] = '.';
    }
}
static void sg_drum_knob(uint32_t idx, uint32_t sl, int32_t s)
{
    uint32_t l = drum_lane;
    int xi;
    dtx_t *x, c;
    if (!dt_has(idx, l) || sl == 7u)
        return;
    if (sl == 0u) {
        dt_set(idx, l, (uint32_t)clamp((int32_t)dt_vel(idx, l) + accel(EN_K1, s, 126), 1, 127));
        return;
    }
    xi = dt_x(idx, l);
    if (xi >= 0) {
        c = dpat.x[xi];
    } else {                                            /* (the defaults: as dt_x_make makes them) */
        memset(&c, 0, sizeof c);
        c.prob = DT_PROB_DEF;
        c.rlen = DT_RLEN_DEF;
    }
    if (sl == 1u) {
        c.rrate = (uint8_t)clamp((int32_t)c.rrate + (s > 0 ? 1 : -1), 0, (int32_t)NRTRG);
    } else if (sl == 2u) {
        c.micro = (int8_t)clamp(c.micro + s, MICRO_MIN, MICRO_MAX);
    } else if (sl == 3u) {
        c.cond = (uint8_t)clamp((int32_t)c.cond + (s > 0 ? 1 : -1), 0, 3);
    } else if (sl == 4u) {
        c.prob = SG_PROB[clamp((int32_t)sg_near(SG_PROB, sizeof SG_PROB, c.prob) + (s > 0 ? 1 : -1), 0, (int32_t)sizeof SG_PROB - 1)];
    } else if (sl == 5u) {                              /* (INF: the last) */
        int32_t i = c.rlen ? (int32_t)sg_near(SG_RLEN, sizeof SG_RLEN - 1u, c.rlen) : (int32_t)sizeof SG_RLEN - 1;
        c.rlen = SG_RLEN[clamp(i + (s > 0 ? 1 : -1), 0, (int32_t)sizeof SG_RLEN - 1)];
    } else if (sl == 6u) {
        c.rfade = (int8_t)clamp(c.rfade + accel(EN_K3, s, 255), -128, 127);
    }
    if (xi < 0 && dt_x_plain(&c))
        return;                                         /* (still all defaults: no entry needed) */
    if (!(x = xi >= 0 ? &dpat.x[xi] : dt_x_make(idx, l))) {
        ui_message("TRIG EXTRAS FULL: 64");
        return;
    }
    c.step = (uint8_t)idx;
    c.lane = (uint8_t)l;
    *x = c;
    if (dt_x_plain(x))
        x->step = DT_FREE;                              /* (all back at the defaults: freed) */
}
/* KNOB k with steps held: the lock page's parameter, else the step's own values */
static void sg_held_knob(uint32_t k, int32_t s)
{
    track_t *t = TSEL;
    uint32_t w, len = trk_len(t);
    int32_t heard = -1;                                 /* a step whose NOTE moved: heard */
    if (sg_lpage < NPAGES) {
        sg_lock_turn(pg_id(&PAGES[sg_lpage], k), s, EN_K1 + k);
        return;
    }
    sg_undo(t);
    sg_pend &= (uint16_t)~sg_held;
    fm1_irq_off();
    for (w = 0; w < 16u; w++) {
        uint32_t idx = sg_page() * 16u + w, i;
        if (!((sg_held >> w) & 1u) || idx >= len)
            continue;
        if (is_drum(t)) {                               /* the picked sound's trig: VEL RETRIG NUDGE COND, CHANCE LENGTH CURVE */
            sg_drum_knob(idx, sg_alt ? 4u + k : k, s);
            continue;
        }
        {
            step_t *st = &t->step[idx];
            uint32_t sl = sg_alt ? 4u + k : k;          /* 0 NOTE 1 LEN 2 VEL 3 COND 4 FLAG 5 RATCH 6 NUDGE */
            if (sl == 3u) {
                step_fill_set(t, idx, (uint32_t)clamp((int32_t)step_fill(t, idx) + (s > 0 ? 1 : -1), 0, 3));
                continue;
            }
            if (sl == 6u) {
                t->micro[idx] = (int8_t)clamp(t->micro[idx] + s, MICRO_MIN, MICRO_MAX);
                continue;
            }
            if (!step_on(st))
                continue;
            if (sl == 0u) {                             /* NOTE: by degrees of the scale; a chord moves whole */
                int32_t d = sg_scale_move(t, st->note[0], s) - st->note[0];
                for (i = 0; i < st->n && i < 4u; i++)
                    st->note[i] = (uint8_t)clamp(st->note[i] + d, 1, 127);
                last_note = st->note[0];
                if (d && heard < 0)
                    heard = (int32_t)idx;
            } else if (sl == 1u) {                      /* LEN: the TIEs after it (ui.c note_set_len) */
                uint32_t was = note_len(t, idx), n = note_set_len(t, idx, (uint32_t)clamp((int32_t)was + (s > 0 ? 1 : -1), 1, NSTEP));
                if (s > 0 && n == was)
                    ui.msg_t = 0, ui_message(idx + n < len ? "NOTE AHEAD" : "END OF PATTERN");
            } else if (sl == 2u) {                      /* VEL 1..127, every note at NORM (an accent goes: it is 127) */
                st->vel = (uint8_t)clamp((int32_t)step_vel_shown(st) + s, 1, 127);
                st->lvl = 0;
                st->flags &= (uint8_t)~SF_ACCENT;
            } else if (sl == 4u) {                      /* FLAG: - ACC SLD A+S */
                uint32_t f = (st->flags & SF_ACCENT ? 1u : 0u) | (st->flags & SF_SLIDE ? 2u : 0u);
                f = (uint32_t)clamp((int32_t)f + (s > 0 ? 1 : -1), 0, 3);
                st->flags = (uint8_t)((st->flags & ~(SF_ACCENT | SF_SLIDE)) | (f & 1u ? SF_ACCENT : 0u) | (f & 2u ? SF_SLIDE : 0u));
            } else if (sl == 5u) {                      /* RATCH x1..x4, every note */
                uint32_t r = (uint32_t)clamp((int32_t)(st->rat & 3u) + (s > 0 ? 1 : -1), 0, 3);
                st->rat = (uint8_t)(r * 0x55u);
            }
        }
    }
    fm1_irq_on();
    sync_reload = 1;
    if (heard >= 0)
        audition_notes(song.sel, t->step[heard].note, t->step[heard].n);
}
/* KNOB k, no step held, a synth track: NOTE (the pen, by degrees of the scale; heard), STEP (the cursor), HIT (right:
 * the pen on the cursor's step, left: cleared), VEL (of the cursor's step) */
static void sg_free_knob(uint32_t k, int32_t s)
{
    track_t *t = TSEL;
    uint32_t len = trk_len(t), c = sg_cur(), i;
    step_t *st = &t->step[c];
    if (k == 0u) {
        int32_t d;
        if (!pen_n) {
            pen_note[0] = last_note;
            pen_n = 1;
        }
        d = sg_scale_move(t, pen_note[0], s) - pen_note[0];
        for (i = 0; i < pen_n && i < 4u; i++)
            pen_note[i] = (uint8_t)clamp(pen_note[i] + d, 1, 127);
        last_note = pen_note[0];
        if (d)
            audition_notes(song.sel, pen_note, pen_n);  /* the note chosen, heard before it is placed */
        return;
    }
    if (k == 1u) {
        sg_cur_set((uint32_t)clamp((int32_t)c + s, 0, (int32_t)len - 1));
        return;
    }
    if (k == 2u) {
        if (s > 0 && !step_on(st))
            sg_set(t, c);
        else if (s < 0 && step_on(st))
            sg_clear(t, c);
        return;
    }
    if (step_on(st)) {
        sg_undo(t);
        fm1_irq_off();
        st->vel = (uint8_t)clamp((int32_t)step_vel_shown(st) + accel(EN_K4, s, 126), 1, 127);
        st->lvl = 0;
        st->flags &= (uint8_t)~SF_ACCENT;
        fm1_irq_on();
        sync_reload = 1;
    }
}
/* OCT- with steps held: their locks go (a lock page shown: that page's only; else every lock, the nudge and the
 * condition); OCT+: their condition, round */
static void sg_held_oct(uint32_t up)
{
    track_t *t = TSEL;
    uint32_t w, len = trk_len(t), n = 0, v = 0, first = 1;
    sg_undo(t);
    sg_pend &= (uint16_t)~sg_held;
    fm1_irq_off();
    for (w = 0; w < 16u; w++) {
        uint32_t idx = sg_page() * 16u + w, s;
        if (!((sg_held >> w) & 1u) || idx >= len)
            continue;
        if (is_drum(t)) {                               /* (PHEBUS wave 2) the picked sound's trig */
            int xi = dt_x(idx, drum_lane);
            if (!dt_has(idx, drum_lane))
                continue;
            if (up) {
                dtx_t *x;
                if (first)
                    v = ((xi >= 0 ? dpat.x[xi].cond : 0u) + 1u) % 4u;
                first = 0;
                if (v == FC_NORM && xi >= 0) {
                    dpat.x[xi].cond = FC_NORM;
                    if (dt_x_plain(&dpat.x[xi]))
                        dpat.x[xi].step = DT_FREE;
                } else if (v != FC_NORM && (x = dt_x_make(idx, drum_lane)) != 0) {
                    x->cond = (uint8_t)v;
                }
            } else if (sg_lpage < NPAGES) {
                for (s = 0; s < 4u; s++) {
                    uint32_t id = pg_id(&PAGES[sg_lpage], s);
                    if (id < P_COUNT && lock_find(t, idx, id, 0) >= 0) {
                        lock_del(t, idx, id);
                        n++;
                    }
                }
            } else {
                n += dt_lk_del(idx, drum_lane, DSP_ALL) + (xi >= 0) + step_locked(t, idx);
                if (xi >= 0)
                    dpat.x[xi].step = DT_FREE;          /* (its condition, chance, nudge, retrig) */
                lock_del(t, idx, P_COUNT);              /* (and the drum track's own locks on the step) */
            }
            continue;
        }
        if (up) {
            if (first)
                v = (step_fill(t, idx) + 1u) % 4u;
            first = 0;
            step_fill_set(t, idx, v);
        } else if (sg_lpage < NPAGES) {
            for (s = 0; s < 4u; s++)
                if (pg_id(&PAGES[sg_lpage], s) < P_COUNT && lock_find(t, idx, pg_id(&PAGES[sg_lpage], s), 0) >= 0) {
                    lock_del(t, idx, pg_id(&PAGES[sg_lpage], s));
                    n++;
                }
        } else {
            n += step_locked(t, idx) || step_fill(t, idx);
            lock_del(t, idx, P_COUNT);
            t->micro[idx] = 0;
            step_fill_set(t, idx, FC_NORM);
        }
    }
    fm1_irq_on();
    sync_reload = 1;
    if (up) {
        static const char *const MSG[4] = {"COND: NORMAL", "COND: FILL ONLY", "COND: NO FILL", "COND: CHANCE"};
        ui_message(MSG[v % 4u]);
    } else if (sg_lpage < NPAGES) {
        ui_message(n ? "PAGE LOCKS CLEARED" : "NO LOCK ON THIS PAGE");
    } else if (is_drum(t)) {
        ui_message(n ? "LOCKS, NUDGE, COND, RETRIG CLEARED" : "NOTHING TO CLEAR");
    } else {
        ui_message(n ? "LOCKS, NUDGE, COND CLEARED" : "NO LOCK, NUDGE OR COND");
    }
}
/* the GRID's buttons and knobs that are the same on every track: OCT- / OCT+, ENV / LFO, SELECT with a step held. Returns 1 when KNOB 1..4 were taken here (a step held, or a synth track) */
static int sg_input(uint32_t pressed)
{
    uint32_t k;
    int32_t s;
    if (pressed & (1u << panel.btn[B_OCTDN] | 1u << panel.btn[B_OCTUP])) {
        uint32_t up = (pressed >> panel.btn[B_OCTUP]) & 1u, pages = (trk_len(TSEL) + 15u) / 16u, pg;
        if (sg_held) {
            sg_held_oct(up);
        } else if (up && sg_page() + 1u == pages && pages < 4u) {
            sg_page_add(TSEL);                          /* past the last page: one added, a copy of it */
        } else {                                        /* the page of steps before / after (64 steps: round) */
            pg = (sg_page() + (up ? 1u : pages - 1u)) % pages;
            sg_cur_set((uint32_t)clamp((int32_t)(pg * 16u + sg_cur() % 16u), 0, (int32_t)trk_len(TSEL) - 1));
            ui.force = 1;
        }
    }
    if (sg_held && (pressed & (1u << panel.btn[B_ENV] | 1u << panel.btn[B_LFO])))
        sg_lock_open((pressed >> panel.btn[B_ENV]) & 1u ? FAM_ENV : FAM_LFO);
    else if (!sg_held && (pressed & 1u << panel.btn[B_ENV]))   /* (every track, as from HOME: the drums' ENV is the */
        open_family(FAM_ENV);                                  /* sound's HEX AEG, their LFO says there is none) */
    else if (!sg_held && (pressed & 1u << panel.btn[B_LFO]))
        open_family(FAM_LFO);
    if (!on_drum_page())
        return 1;
    if (!sg_held)
        return 0;
    /* PRESETS, ALGORITHM: nothing while a step is held (KARMA's hidden lock of the last knob touched is cut) */
    (void)panel_enc(EN_PRESET);
    (void)panel_enc(EN_ALGO);
    if ((s = panel_enc(EN_SELECT)) != 0) {
        sg_alt = (uint8_t)(s > 0);                      /* NOTE LEN VEL COND <-> FLAG RATCH NUDGE; drums: VEL RETRIG NUDGE
                                                         * COND <-> CHANCE LENGTH CURVE */
        ui.force = 1;
    }
    for (k = 0; k < 4u; k++)
        if ((s = panel_enc(EN_K1 + k)) != 0) {
            ui.hot_col = (uint8_t)k;
            ui.hot_t = 40;
            sg_held_knob(k, s);
        }
    return 1;
}
/* ------------------------------------------------------- STEP REC --- */
/* (PHEBUS, Ben 21 Nov 2026: "it's hard to input sequences that make sense when you're writing melodic parts"; picker
 * "Yes, add it": "press REC in the GRID, play notes or chords on the keys, each lands on the next step") On a synth
 * track's GRID with the transport stopped, REC starts STEP REC (again: it ends; playing, REC records live as before).
 * The keys play again (seq.c: kb_grid off) and every note they or MIDI play reaches sr_event (seq.c step_in_q):
 *   a note or a chord      (every key pressed before the last one comes up) is written on the cursor's step, whatever
 *                          was there, and the cursor moves on a step (round at the pattern's end, onto its later pages)
 *   SEQ (beside PLAY)      a rest: the step emptied, the cursor on. With keys still held: a tie (as ARP)
 *   ARP                    a tie: the note before is a step longer (the keys held: written first, then tied)
 *   OCT- / OCT+            the octave, as on HOME (both: 0)
 *   SELECT, KNOB 2         the cursor (to fix a step); KNOB 1 the octave, KNOB 3 the velocity written, KNOB 4 STEPS
 * One undo (EDIT + OCT-) takes back the whole pass, from REC to REC. Leaving the GRID (or the track) ends it */
static uint8_t sr_vel = 100, sr_n, sr_nt[4], sr_down, sr_done, sr_trk;
static uint32_t sr_sess;
static int sr_here(void) { return grid_keys_on() && !is_drum(TSEL); }   /* a synth track's GRID shown */
static void sr_set(uint32_t on)
{
    if (on && !sr_on) {
        sr_sess = (undo_sess += 4u) | 3u;               /* (the pass: one undo) */
        sr_trk = (uint8_t)song.sel;
        sr_n = sr_down = sr_done = 0;
        sg_drop();
    }
    sr_on = (uint8_t)(on != 0u);
    ly_off = on ? ly_bit[LY_STEP] | ly_bit[LY_ROLL] : 0u;
    ui.force = 1;
}
static void sr_next(const track_t *t)                   /* the cursor a step on, round at the end */
{
    uint32_t c = sg_cur() + 1u;
    sg_cur_set(c < trk_len(t) ? c : 0u);
    ui.force = 1;
}
static void sr_tail_rest(track_t *t, uint32_t c)        /* the TIEs after step c (a note's tail cut there): rests */
{
    uint32_t j, len = trk_len(t);
    for (j = c + 1u; j < len && t->step[j].time == ST_TIE && !t->step[j].n; j++)
        step_clear(&t->step[j]);
}
static void sr_note(track_t *t)                         /* the notes held, onto the cursor's step */
{
    uint32_t c = sg_cur(), i;
    step_t *st = &t->step[c];
    undo_mark(t, sr_sess);
    fm1_irq_off();
    sr_tail_rest(t, c);                                 /* (what was there goes: its tail too) */
    memset(st, 0, sizeof *st);
    for (i = 0; i < sr_n && i < 4u; i++)
        st->note[i] = sr_nt[i];
    st->n = (uint8_t)(t->p[P_VOICE] == V_POLY ? sr_n : 1u);   /* (a chord only on a polyphonic voice) */
    st->time = ST_NOTE;
    st->vel = sr_vel;
    t->seq_active = 1;
    fm1_irq_on();
    last_note = st->note[0];
    sync_reload = 1;
    sr_done = 1;
    sr_next(t);
}
static void sr_rest(track_t *t)
{
    uint32_t c = sg_cur();
    undo_mark(t, sr_sess);
    fm1_irq_off();
    sr_tail_rest(t, c);
    step_clear(&t->step[c]);
    lock_del(t, c, P_COUNT);                            /* (an empty step keeps no lock, nudge or condition) */
    t->micro[c] = 0;
    step_fill_set(t, c, FC_NORM);
    fm1_irq_on();
    sync_reload = 1;
    sr_next(t);
}
static void sr_tie(track_t *t)                          /* the note before the cursor one step longer */
{
    uint32_t c = sg_cur(), p = c;
    while (p && t->step[p - 1u].time == ST_TIE && !t->step[p - 1u].n)
        p--;
    if (!p || !step_on(&t->step[p - 1u])) {
        ui_message("NO NOTE TO TIE");
        return;
    }
    undo_mark(t, sr_sess);
    fm1_irq_off();
    sr_tail_rest(t, c);
    memset(&t->step[c], 0, sizeof t->step[c]);
    t->step[c].time = ST_TIE;
    lock_del(t, c, P_COUNT);
    t->micro[c] = 0;
    step_fill_set(t, c, FC_NORM);
    fm1_irq_on();
    sync_reload = 1;
    sr_next(t);
}
/* a note of the selected track down / up (ui_input.c midi_entry: the keys' and MIDI's, seq.c step_in_q): a chord is
 * every note down before the last comes up */
static void sr_event(uint32_t note, uint32_t on)
{
    uint32_t i;
    if (on) {
        if (!sr_down)
            sr_n = sr_done = 0;
        sr_down++;
        for (i = 0; i < sr_n && sr_nt[i] != note; i++)
            ;
        if (i == sr_n && sr_n < 4u)
            sr_nt[sr_n++] = (uint8_t)note;
        ui.force = 1;
        return;
    }
    if (sr_down && !--sr_down) {
        if (sr_n && !sr_done)
            sr_note(TSEL);
        sr_n = sr_done = 0;
        ui.force = 1;
    }
}
static void sr_input(uint32_t pressed)
{
    track_t *t = TSEL;
    uint32_t k, len = trk_len(t), total, cur;
    int32_t s;
    if (pressed & (1u << panel.btn[B_OCTDN] | 1u << panel.btn[B_OCTUP])) {   /* the octave, as on HOME */
        uint32_t both = (1u << panel.btn[B_OCTDN]) | (1u << panel.btn[B_OCTUP]);
        if ((fm1_in.buttons & both) == both)
            song.octave = 0;
        else
            song.octave += (pressed >> panel.btn[B_OCTDN]) & 1u ? (song.octave > -3 ? -1 : 0) : (song.octave < 3 ? 1 : 0);
        ui.force = 1;
    }
    if (pressed & ly_bit[LY_ROLL]) {                    /* ARP: a tie (keys held: their notes first) */
        if (sr_down && sr_n && !sr_done)
            sr_note(t);
        sr_tie(t);
    } else if (pressed & ly_bit[LY_STEP]) {             /* SEQ: a rest; keys held: a tie */
        if (sr_down) {
            if (sr_n && !sr_done)
                sr_note(t);
            sr_tie(t);
        } else {
            sr_rest(t);
        }
    }
    if (pressed & 1u << panel.btn[B_ENV])
        open_family(FAM_ENV);
    else if (pressed & 1u << panel.btn[B_LFO])
        open_family(FAM_LFO);
    if ((s = panel_enc(EN_PRESET)) != 0 && (cur = preset_pos(&total), total))   /* the sound, as on HOME */
        preset_go((uint32_t)(((int32_t)cur + s % (int32_t)total + (int32_t)total) % (int32_t)total));
    if ((s = panel_enc(EN_SELECT)) != 0)                /* SELECT: the cursor */
        sg_cur_set((uint32_t)clamp((int32_t)sg_cur() + (s > 0 ? 1 : -1), 0, (int32_t)len - 1));
    for (k = 0; k < 4u; k++) {
        if ((s = panel_enc(EN_K1 + k)) == 0)
            continue;
        ui.hot_col = (uint8_t)k;
        ui.hot_t = 40;
        if (k == 0u)
            song.octave = (int8_t)clamp(song.octave + (s > 0 ? 1 : -1), -3, 3);
        else if (k == 1u)
            sg_cur_set((uint32_t)clamp((int32_t)sg_cur() + s, 0, (int32_t)len - 1));
        else if (k == 2u)
            sr_vel = (uint8_t)clamp((int32_t)sr_vel + accel(EN_K3, s, 126), 1, 127);
        else {                                          /* STEPS, as SEQ held + KNOB 4 */
            t->p[P_SLEN] = (int16_t)clamp(t->p[P_SLEN] + accel(EN_K4, s, 63), 1, NSTEP);
            if (sg_cur() >= trk_len(t))
                sg_cur_set(trk_len(t) - 1u);
            sync_reload = 1;
        }
    }
}

/* (wave 4) OCT- with steps held on a sound tab of the synth GRID (as the drum screen's): the lock of the knob last turned in
 * this hold goes; again (or nothing turned) every lock of the steps. OCT+: their condition, as on TRIG */
static void syn_held_oct(uint32_t up)
{
    track_t *t = TSEL;
    uint32_t w, len = trk_len(t), n = 0, one = syn_touch != 0xFFu;
    char b[24];
    if (up || !syn_tab) {
        sg_held_oct(up);
        return;
    }
    sg_undo(t);
    sg_pend &= (uint16_t)~sg_held;
    fm1_irq_off();
    for (w = 0; w < 16u; w++) {
        uint32_t idx = sg_page() * 16u + w;
        if (!((sg_held >> w) & 1u) || idx >= len)
            continue;
        if (one) {
            if (lock_find(t, idx, syn_touch, 0) >= 0)
                lock_del(t, idx, syn_touch), n++;
        } else {
            uint32_t k;
            for (k = 0; k < NLOCK; k++)
                n += t->lock[k].step == idx;
            lock_del(t, idx, P_COUNT);
        }
    }
    fm1_irq_on();
    sync_reload = 1;
    if (one && n) {
        str_cpy(b, lock_desc(t, syn_touch)->label, 8);
        ui_say("LOCK OFF: ", b);
    } else {
        fmt_int(b, (int32_t)n);
        ui_say(b, n == 1u ? " LOCK OFF" : " LOCKS OFF");
    }
    syn_touch = 0xFF;                                   /* (OCT- again: every lock) */
}
/* the synth GRID's input (the drum track's: ui_drum.c drum_grid_input). (PHEBUS wave 4, the Rytm's way, as the drum screen)
 * the page buttons change the tab, a step held or not (EDIT, SEQ, FX: ui_input.c layer_tap; ENV, LFO here); SELECT walks
 * every half; KNOB 1..4: nothing held, the sound (TRIG: NOTE STEP HIT VEL); a step held, a lock of what the knob shows
 * (TRIG: the step's own values) */
static void sg_synth_input(uint32_t pressed)
{
    uint32_t k, total, cur;
    int32_t s;
    if (sr_on) {                                        /* STEP REC: its own controls */
        sr_input(pressed);
        return;
    }
    if (!sg_held)
        syn_touch = 0xFF;
    if (pressed & (1u << panel.btn[B_OCTDN] | 1u << panel.btn[B_OCTUP])) {
        uint32_t up = (pressed >> panel.btn[B_OCTUP]) & 1u, pages = (trk_len(TSEL) + 15u) / 16u, pg;
        if (sg_held) {
            syn_held_oct(up);
        } else if (up && sg_page() + 1u == pages && pages < 4u) {
            sg_page_add(TSEL);                          /* past the last page: one added, a copy of it */
        } else {                                        /* the page of steps before / after (64 steps: round) */
            pg = (sg_page() + (up ? 1u : pages - 1u)) % pages;
            sg_cur_set((uint32_t)clamp((int32_t)(pg * 16u + sg_cur() % 16u), 0, (int32_t)trk_len(TSEL) - 1));
            ui.force = 1;
        }
    }
    if (pressed & (1u << panel.btn[B_ENV]))
        syn_open(FAM_ENV);
    if (pressed & (1u << panel.btn[B_LFO]))
        syn_open(FAM_MOD);
    if (!on_drum_page())
        return;
    if ((s = panel_enc(EN_PRESET)) != 0 && !sg_held && (cur = preset_pos(&total), total))   /* the sound, as on HOME */
        preset_go((uint32_t)(((int32_t)cur + s % (int32_t)total + (int32_t)total) % (int32_t)total));
    if (sg_held)
        (void)panel_enc(EN_ALGO);
    if ((s = panel_enc(EN_SELECT)) != 0)                /* every half of every tab (the tempo: HOME, the pages) */
        syn_walk(s);
    sg_alt = (uint8_t)(syn_tab ? 0u : syn_hcur());      /* (TRIG's halves: NOTE LEN VEL COND, FLAG RATCH NUDGE) */
    for (k = 0; k < 4u; k++) {
        if ((s = panel_enc(EN_K1 + k)) == 0)
            continue;
        ui.hot_col = (uint8_t)k;
        ui.hot_t = 40;
        if (!syn_tab) {
            if (sg_held)
                sg_held_knob(k, s);
            else if (!syn_hcur())
                sg_free_knob(k, s);
            else if (k == 0u)                           /* (wave 5) TRIG 2/2, nothing held: SPICE, DICE (the MiniFreak's) */
                mxt[song.sel % NPART].spice = (int16_t)clamp(mxt[song.sel % NPART].spice + accel(EN_K1, s, 127), 0, 127);
            else if (k == 1u) {                         /* each detent a roll: right a new one, left back to the last */
                char b[12];
                mxt[song.sel % NPART].dice = (int16_t)((mxt[song.sel % NPART].dice + (s > 0 ? 1 : 9999)) % 10000);
                b[0] = '#';
                fmt_int(b + 1, mxt[song.sel % NPART].dice);
                ui_say(mxt[song.sel % NPART].spice ? "DICE " : "DICE (SPICE 0) ", b);
            }
        } else if (syn_page() < NPAGES) {
            const page_t *pg = &PAGES[syn_page()];
            uint32_t id = pg_id(pg, k);
            if (id == 0xFFu)
                continue;
            if (sg_held) {
                sg_lock_turn(id == V_TYPE ? P_COUNT : id, s, EN_K1 + k);   /* (TYPE: NOT LOCKABLE, the Rytm's machine) */
                if (id < P_COUNT && p_lockable(id))
                    syn_touch = (uint8_t)id;
            } else {
                edit_param_pg(pg, k, s);
            }
        }
    }
}

/* the keys' lights on the GRID: the steps set on the page shown (drums: of the sound KNOB 1 picks), the playhead
 * blinking, the keys held */
static uint32_t grid_keys_lit(uint32_t blink)
{
    const track_t *t = TSEL;
    uint32_t i, m = 0, len = trk_len(t), b0 = sg_page() * 16u;
    if (stl_view && song.sel == TRK_DRUM)
        return stl_keys_lit(blink);                     /* (PHEBUS) the STUTTER lane's (ui_stut.c) */
    for (i = 0; i < 16u; i++) {
        uint32_t idx = b0 + i, on;
        if (idx >= len)
            continue;
        on = (uint32_t)sg_on(t, idx);
        if (song.playing && idx == t->seq_idx)
            on = !on || blink;
        if (on || ((sg_held >> i) & 1u))
            m |= 1u << key_of_lane(i);
    }
    return m;
}

/* ---------------------------------------------------------- drawing --- */
/* what step idx holds, as up to 4 lines of text (the knobs show the values being turned; this shows all of it) */
static uint8_t sg_lines_held;                           /* a step held: the knobs show LEVEL, the lines leave it out */
static uint32_t sg_lines(const track_t *t, uint32_t idx, char ln[4][31])
{
    uint32_t held = sg_lines_held;
    uint32_t n = 0, k, nl = 0, f = step_fill(t, idx);
    char v[12];
    const char *u;
    ln[0][0] = ln[1][0] = ln[2][0] = ln[3][0] = 0;
    if (is_drum(t)) {                                   /* (PHEBUS wave 2) the picked sound's trig */
        int xi = dt_x(idx, drum_lane);
        const dtx_t *x = xi >= 0 ? &dpat.x[xi] : 0;
        if (dt_has(idx, drum_lane)) {
            ln[0][0] = 0;
            if (!held) {
                str_cpy(ln[0], "VEL ", 31);
                fmt_int(ln[0] + 4, (int32_t)dt_vel(idx, drum_lane));
                str_cpy(ln[0] + str_len(ln[0]), "  ", 3);
            }
            str_cpy(ln[0] + str_len(ln[0]), "RETRIG ", 8);
            str_cpy(ln[0] + str_len(ln[0]), SG_RATE[x ? x->rrate % (NRTRG + 1u) : 0u], 6);
            if (x && x->rrate) {                        /* (its length, in steps) */
                str_cpy(ln[0] + str_len(ln[0]), " LEN ", 6);
                sg_rlen_str(ln[0] + str_len(ln[0]), x->rlen);
            }
            if (x && (x->cond || x->micro)) {           /* the condition, its chance, the nudge */
                str_cpy(ln[1], SG_COND_L[x->cond & 3u], 31);
                if (x->cond == FC_CHANCE) {
                    str_cpy(ln[1] + str_len(ln[1]), " ", 2);
                    fmt_int(ln[1] + str_len(ln[1]), x->prob);
                    str_cpy(ln[1] + str_len(ln[1]), "%", 2);
                }
                if (x->micro) {
                    str_cpy(ln[1] + str_len(ln[1]), x->cond ? "  NUDGE " : "NUDGE ", 9);
                    fmt_int(ln[1] + str_len(ln[1]), x->micro);
                }
                n = 2;
            } else {
                n = 1;
            }
            for (k = 0; k < DT_NLK && n < 4u; k++) {    /* the sound's locks */
                const dlk_t *q = &dpat.lk[k];
                char e[18];
                uint32_t el;
                const param_desc_t *qd = q->step == idx && q->lane == drum_lane ? dsp_desc(drum_model(), lane_snd(drum_lane), q->param) : 0;
                if (!qd)
                    continue;
                e[0] = ' ';
                str_cpy(e + 1, qd->label, 7);
                el = str_len(e);
                e[el++] = ' ';
                dsp_format(q->param, qd, lk_val(q->param, q->val), v, &u);
                str_cpy(e + el, v, 8);
                if (!nl)
                    str_cpy(ln[n], "LOCK", 31);
                else if (str_len(ln[n]) + str_len(e) > 30u || text_w(&FONT_S, ln[n]) + text_w(&FONT_S, e) > 228) {
                    if (++n >= 4u) {
                        str_cpy(ln[3] + (str_len(ln[3]) > 28u ? 28u : str_len(ln[3])), " +", 3);
                        break;
                    }
                    str_cpy(ln[n], "    ", 31);
                }
                str_cpy(ln[n] + str_len(ln[n]), e, 31u - str_len(ln[n]));
                nl++;
            }
        } else {
            str_cpy(ln[0], "NO TRIG", 31);
            n = 1;
        }
        f = 0;                                          /* (a drum step has no condition or nudge of its own) */
    } else {
        const step_t *st = &t->step[idx];
        if (step_on(st)) {
            str_cpy(ln[0], "LEN ", 31);                /* (VEL: the knob row has it, review 19) */
            fmt_int(ln[0] + 4, (int32_t)note_len(t, idx));
            if (st->flags & (SF_ACCENT | SF_SLIDE)) {
                str_cpy(ln[0] + str_len(ln[0]), "  ", 3);
                str_cpy(ln[0] + str_len(ln[0]), SG_FLAG[(st->flags & SF_ACCENT ? 1u : 0u) | (st->flags & SF_SLIDE ? 2u : 0u)], 4);
            }
            if (st->rat & 3u) {
                str_cpy(ln[0] + str_len(ln[0]), "  X", 4);
                fmt_int(ln[0] + str_len(ln[0]), (int32_t)(st->rat & 3u) + 1);
            }
        } else {
            str_cpy(ln[0], st->time == ST_TIE ? "TIE" : "EMPTY", 31);
        }
    }
    if (!is_drum(t))
        n = 1;
    if (!is_drum(t) && (f || t->micro[idx])) {          /* the condition and the nudge */
        str_cpy(ln[1], SG_COND_L[f], 31);
        if (t->micro[idx]) {
            str_cpy(ln[1] + str_len(ln[1]), f ? "  NUDGE " : "NUDGE ", 9);
            fmt_int(ln[1] + str_len(ln[1]), t->micro[idx]);
        }
        n = 2;
    }
    for (k = 0; k < NLOCK && n < 4u; k++) {             /* the locks, as many a line as fit (30 characters, 228 px) */
        const plock_t *l = &t->lock[k];
        char e[18];
        uint32_t el;
        if (l->step != idx || l->param >= P_COUNT)
            continue;
        e[0] = ' ';
        str_cpy(e + 1, lock_desc(t, l->param)->label, 7);
        el = str_len(e);
        e[el++] = ' ';
        param_format(lock_desc(t, l->param), l->val, v, &u);
        str_cpy(e + el, v, 8);
        if (!nl)
            str_cpy(ln[n], is_drum(t) ? "DRUMS" : "LOCK", 31);   /* (the drum track's own: not the sound's) */
        else if (str_len(ln[n]) + str_len(e) > 30u || text_w(&FONT_S, ln[n]) + text_w(&FONT_S, e) > 228) {
            if (++n >= 4u) {                            /* (no room left: "+" says there are more) */
                str_cpy(ln[3] + (str_len(ln[3]) > 28u ? 28u : str_len(ln[3])), " +", 3);
                break;
            }
            str_cpy(ln[n], "    ", 31);
        }
        str_cpy(ln[n] + str_len(ln[n]), e, 31u - str_len(ln[n]));
        nl++;
    }
    if (nl && n < 4u)
        n++;
    return n > 4u ? 4u : n;
}
/* STEP REC's band under the tiles: which step the next note goes on, the notes held (or the last written), how to
 * rest, tie and leave */
static void sr_detail_draw(uint32_t y, uint32_t h)
{
    const track_t *t = TSEL;
    uint16_t col = TE_COL[song.sel & 3u];
    char b[24], nm[20];
    int32_t x;
    cv_begin(240, h, C_BLACK);
    cv_rect(0, 0, 240, 1, TE_G1);
    str_cpy(b, "NEXT: STEP ", 24);
    fmt_int(b + str_len(b), (int32_t)sg_cur() + 1);
    x = cv_text(6, 4, &FONT_S, b, C_WHITE);
    (void)x;
    if (sr_down && sr_n)                                /* the keys held: what will be written */
        sg_notes_str(nm, sr_nt, t->p[P_VOICE] == V_POLY ? sr_n : 1u, 20);
    else
        str_cpy(nm, "PLAY A NOTE", 20);
    cv_text(234 - text_w(&FONT_S, nm), 4, &FONT_S, nm, sr_down && sr_n ? col : TE_G3);
    x = cv_text(6, 24, &FONT_S, "SEQ", C_WHITE);       /* the controls, white on grey words */
    x = cv_text(x + 4, 24, &FONT_S, "REST", TE_G3);
    x = cv_text(x + 12, 24, &FONT_S, "ARP", C_WHITE);
    x = cv_text(x + 4, 24, &FONT_S, "TIE", TE_G3);
    x = cv_text(x + 12, 24, &FONT_S, "REC", TE_RED);
    cv_text(x + 4, 24, &FONT_S, "DONE", TE_G3);
    cv_blit(0, y);
}
/* the 16 steps of the page: two rows of eight tiles (keys 1..8, 9..16), a set step filled in the track's colour
 * with its note, a TIE as a bar from the note before, velocity as a bar under it; marks: a lock / nudge (dot,
 * top right), a condition (notch, top left); the cursor framed white, the playhead a white bar, a step held white */
static void sg_tiles_draw(uint32_t y, uint32_t big)
{
    const track_t *t = TSEL;
    uint32_t j, len = trk_len(t), b0 = sg_page() * 16u, pages = (len + 15u) / 16u, sel = song.sel & 3u;
    uint32_t th = big ? 26u : 22u, row = big ? 30u : 0u, h = big ? 72u : 36u;
    cv_begin(240, h, C_BLACK);
    for (j = 0; j < 16u; j++) {
        uint32_t idx = b0 + j, held = (sg_held >> j) & 1u;
        int32_t x = big ? 4 + (int32_t)(j % 8u) * 29 : 4 + (int32_t)j * 14 + (int32_t)(j / 4u) * 2;
        int32_t ty = big ? 4 + (int32_t)(j / 8u) * (int32_t)row : 4, w = big ? 27 : 12;
        if (idx >= len)
            continue;
        {                                               /* (synth tracks: the drum grid has its own, ui_op.c) */
            const step_t *st = &t->step[idx];
            int32_t q;
            if (step_on(st)) {
                cv_rect(x, ty, w, (int32_t)th, held ? C_WHITE : TE_COL[sel]);
                if (big) {                              /* the note without its octave, centred; the octave as 1..4
                                                         * ticks over the velocity bar (review 19) */
                    const char *nm = N_NOTE[st->note[0] % 12u];
                    int32_t o = clamp((int32_t)st->note[0] / 12 - 2, 1, 4);
                    uint16_t ink = held || !(st->flags & SF_ACCENT) ? C_BLACK : C_WHITE;   /* an accent: white */
                    cv_text(x + (w - text_w(&FONT_S, nm)) / 2, ty + 4, &FONT_S, nm, ink);
                    for (q = 0; q < o; q++)
                        cv_rect(x + (w - (o * 4 - 2)) / 2 + q * 4, ty + (int32_t)th - 7, 2, 2, ink);
                }
                if (big && !is_drum(t) && mxt[song.sel % NPART].spice) {   /* (wave 5) Spice: the roll's variation of the
                                                         * step, drawn over it: left out dim, an octave an arrow at the
                                                         * left, a ratchet ticks at the right, the length a line */
                    spv_t sv;
                    spice_of(t, idx, &sv);
                    if (sv.off) {
                        cv_rect(x, ty, w, (int32_t)th, held ? C_WHITE : TE_DIM[sel]);
                        cv_line(x, ty + (int32_t)th - 1, x + w - 1, ty, C_BLACK);
                    }
                    if (sv.oct > 0)
                        cv_rect(x + 2, ty + 2, 3, 1, C_WHITE), cv_rect(x + 3, ty + 1, 1, 1, C_WHITE);
                    else if (sv.oct < 0)
                        cv_rect(x + 2, ty + 1, 3, 1, C_WHITE), cv_rect(x + 3, ty + 2, 1, 1, C_WHITE);
                    for (q = 0; q + 1 < (int32_t)sv.hits; q++)
                        cv_rect(x + w - 2, ty + 6 + q * 4, 2, 2, C_BLACK);
                    if (!sv.off)
                        cv_rect(x + 2, ty + (int32_t)th - 1, (w - 4) * clamp((int32_t)sv.glen, 16, 112) / 112, 1, C_WHITE);
                }
                if (st->flags & SF_SLIDE)                   /* a slide: a 3 x 3 mark, 2 px in from the top right */
                    cv_rect(x + w - 5, ty + 2, 3, 3, held ? C_BLACK : C_WHITE);
                cv_rect(x + 2, ty + (int32_t)th - 3, (w - 4) * (int32_t)step_vel_shown(st) / 127, 2, C_BLACK);   /* velocity */
                if (st->n > 1u)                         /* a chord: a second line */
                    cv_rect(x + 2, ty + 2, 2, (int32_t)th - 8, C_BLACK);
            } else if (st->time == ST_TIE) {
                cv_rect(x - 2, ty + (int32_t)th / 2 - 3, w + 2, 6, held ? C_WHITE : TE_MID[sel]);
                op_box(x, ty, w, (int32_t)th, TE_DIM[sel]);
            } else {
                op_box(x, ty, w, (int32_t)th, held ? C_WHITE : j % 4u ? TE_DIM[sel] : TE_MID[sel]);
            }
            {                                           /* (wave 4, as the drum screen) a lock: a square in it; a nudge: a
                                                         * dot top centre (top right: the slide) */
                uint32_t k, lk = 0;
                for (k = 0; k < NLOCK; k++)
                    lk |= t->lock[k].step == idx;
                if (lk && big)
                    cv_rect(x + w - 6, ty + (int32_t)th - 7, 5, 5, step_on(st) || held ? C_BLACK : TE_COL[sel]);
                else if (lk)
                    cv_rect(x + w / 2 - 1, ty + 1, 3, 2, step_on(st) ? C_BLACK : C_WHITE);
                if (t->micro[idx])
                    cv_rect(x + w / 2 - 1, ty + 1, 3, 2, step_on(st) ? C_BLACK : C_WHITE);
            }
            if (step_fill(t, idx))
                cv_rect(x + 1, ty + 1, 4, 2, step_on(st) ? C_BLACK : C_WHITE);
        }
        if (idx == sg_cur() && sr_on) {                 /* STEP REC: the step the next note goes on, framed red */
            op_box(x - 1, ty - 1, w + 2, (int32_t)th + 2, TE_RED);
            op_box(x - 2, ty - 2, w + 4, (int32_t)th + 4, TE_RED);
        } else if (idx == sg_cur() && !sg_held) {
            op_box(x - 1, ty - 1, w + 2, (int32_t)th + 2, C_WHITE);
        }
        if (song.playing && idx == t->seq_idx % len)
            cv_rect(x, ty + (int32_t)th + 1, w, 2, C_WHITE);
    }
    for (j = 0; j < 4u && big; j++) {                    /* the four page slots: the one shown filled, the others in
                                                         * use grey, the unused ones an outline (OCT+ past the last
                                                         * adds one); the one playing marked */
        int32_t x = 4 + (int32_t)j * 58;
        if (j >= pages) {
            op_box(x, 64, 54, 5, TE_G2);
            continue;
        }
        cv_rect(x, 64, 54, 5, j == sg_page() ? TE_COL[sel] : TE_G2);
        if (song.playing && t->seq_idx / 16u == j)
            cv_rect(x + 22, 70, 10, 2, C_WHITE);
    }
    cv_blit(0, y);
}
/* (wave 4) the band under the tiles: two lines. A step held: "STEP 5 +1" and its notes; then its locks (else its length,
 * flags, condition). Nothing held: the cursor's notes and the pen; then its length and the page of steps. STEP REC: its own */
static void sg_info_draw(uint32_t y, uint32_t h, uint32_t idx, uint32_t held)
{
    const track_t *t = TSEL;
    uint16_t col = TE_COL[song.sel & 3u];
    char ln[4][31], top[32], nm[20], pr[24];
    uint32_t i, n = (sg_lines_held = (uint8_t)held, sg_lines(t, idx, ln)), len = trk_len(t), b0 = sg_page() * 16u;
    const char *two = ln[0];
    cv_begin(240, h, C_BLACK);
    if (step_on(&t->step[idx]))
        sg_notes_str(nm, t->step[idx].note, t->step[idx].n, 20);
    else
        str_cpy(nm, "--", 20);
    for (i = 1; i < n && held; i++)                     /* (held: its locks first, when it has some; else its length) */
        if (ln[i][0] == 'L')
            two = ln[i];
    if (held) {
        str_cpy(top, "STEP ", 8);
        fmt_int(top + 5, (int32_t)idx + 1);
        if (sg_held & (sg_held - 1u))
            str_cpy(top + str_len(top), " +", 3);       /* (more steps held: edited together) */
        str_cpy(top + str_len(top), " HELD", 6);
        cv_text(4, 2, &FONT_S, top, C_WHITE);
        cv_text(236 - text_w(&FONT_S, nm), 2, &FONT_S, nm, col);
        cv_text(4, 20, &FONT_S, two, two != ln[0] ? col : TE_G4);
    } else {
        cv_text(4, 2, &FONT_S, nm, col);
        sg_notes_str(pr, pen_n ? pen_note : &last_note, pen_n ? pen_n : 1u, 24);
        cv_text(236 - text_w(&FONT_S, pr), 2, &FONT_S, pr, col);
        cv_text(236 - text_w(&FONT_S, pr) - 4 - text_w(&FONT_S, "PEN"), 2, &FONT_S, "PEN", TE_G3);
        fmt_int(pr, (int32_t)b0 + 1);
        str_cpy(pr + str_len(pr), "-", 2);
        fmt_int(pr + str_len(pr), (int32_t)(b0 + 16u < len ? b0 + 16u : len));
        str_cpy(pr + str_len(pr), " OF ", 5);
        fmt_int(pr + str_len(pr), (int32_t)len);
        {                                               /* (its length, flags, locks: as far as the page's range leaves room) */
            char b[31];
            str_cpy(b, two, 31);
            while (b[0] && text_w(&FONT_S, b) > 226 - text_w(&FONT_S, pr))
                b[str_len(b) - 1u] = 0;
            cv_text(4, 20, &FONT_S, b, TE_G4);
        }
        cv_text(236 - text_w(&FONT_S, pr), 20, &FONT_S, pr, TE_G3);
    }
    cv_blit(0, y);
}
/* the synth GRID (wave 4: as the drum screen): header ([1] and the sound's name; STEP REC, LIVE REC), the steps, what the
 * step held (or the cursor's) holds, the tabs TRIG OSC FILT ENV FX MOD, four dials, the tracks */
static void sg_draw(void)
{
    static uint32_t head, tiles_sig, tabs, footer, bar;
    static char v[4][12];
    const track_t *t = TSEL;
    uint32_t len = trk_len(t), i, sig = 0, c, vv = song.sel & 3u;
    int32_t h0 = sg_first();
    char title[20], tag[4] = {(char)('1' + vv), 0};
    const char *lab[4], *val[4] = {v[0], v[1], v[2], v[3]};
    int32_t ratio[4];
    uint32_t locked = 0, dim = 0;
    if (ui.cursor >= len)
        sg_cur_set(len - 1u);
    c = h0 >= 0 ? (uint32_t)h0 : sg_cur();
    if (sr_on || (song.rec && sr_here()))               /* (PHEBUS) the state: STEP REC, or a live take on the GRID */
        str_cpy(title, sr_on ? "STEP REC" : "LIVE REC", 20);
    else
        op_names_str(t, title);
    op_header(tag, TE_COL[vv], title, &head);
    for (i = 0; i < len; i++) {                          /* (what the tiles and the detail show) */
        const step_t *st = &t->step[i];
        sig = sig * 31u + st->note[0] + st->n * 131u + st->time * 7u + st->vel * 17u + st->flags * 3u + st->rat * 5u;
        sig = sig * 7u + step_fill(t, i) + (uint32_t)(t->micro[i] + 64) * 3u;
    }
    for (i = 0; i < NLOCK; i++)
        sig = sig * 31u + t->lock[i].step + t->lock[i].param * 257u + (uint32_t)t->lock[i].val * 65537u;
    sig = sig * 31u + sg_held * 3u + sg_cur() * 977u + len + song.sel * 7919u + pen_note[0] * 13u + pen_n +
          (song.playing ? t->seq_idx + 1u : 0u) * 71u;
    sig = sig * 31u + sr_on * 5u + sr_down * 11u + sr_n * 13u + sr_nt[0] * 17u + sr_nt[1] * 19u + sr_nt[2] * 23u +
          sr_nt[3] * 29u;
    sig = sig * 31u + (uint32_t)mxt[song.sel % NPART].spice * 101u + (uint32_t)mxt[song.sel % NPART].dice * 7u;   /* (wave 5) */
    if (ui.force || sig != tiles_sig) {
        tiles_sig = sig;
        sg_tiles_draw(SG_BODY_Y, 1);
        if (sr_on)
            sr_detail_draw(SG_BODY_Y + 72u, 131u - SG_BODY_Y - 72u);
        else
            sg_info_draw(SG_BODY_Y + 72u, 131u - SG_BODY_Y - 72u, c, h0 >= 0);
    }
    if (sr_on) {                                        /* STEP REC: OCT STEP VEL STEPS */
        static const char *const LS[4] = {"OCT", "STEP", "VEL", "STEPS"};
        if (song.octave > 0)
            v[0][0] = '+', fmt_int(v[0] + 1, song.octave);
        else
            fmt_int(v[0], song.octave);
        fmt_int(v[1], (int32_t)c + 1);
        fmt_int(v[2], sr_vel);
        fmt_int(v[3], (int32_t)len);
        ratio[0] = (song.octave + 3) * 1000 / 6;
        ratio[1] = (int32_t)c * 1000 / (int32_t)(len > 1u ? len - 1u : 1u);
        ratio[2] = sr_vel * 1000 / 127;
        ratio[3] = ((int32_t)len - 1) * 1000 / 63;
        for (i = 0; i < 4u; i++)
            lab[i] = LS[i];
        syn_tabs(0, 0, 0, 1, "STEP REC", &tabs);
    } else if (!syn_tab) {
        syn_trig_vals(h0, syn_hcur(), lab, v, ratio, &dim);
        syn_tabs(0, 0, syn_hcur(), 2, 0, &tabs);
    } else {
        uint32_t pi = syn_page();
        syn_vals(&PAGES[pi < NPAGES ? pi : 0u], h0, lab, v, ratio, &locked, &dim);
        syn_tabs(0, syn_tab, syn_hcur(), syn_halves(syn_tab), 0, &tabs);
    }
    op_dials(lab, val, ratio, locked, dim, TE_COL[vv], syn_tab * 977u + syn_hcur() * 31u + sr_on * 7u + sg_held, &footer);
    op_trackbar(&bar);
}
