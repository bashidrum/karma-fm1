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
 *                    drums: LEVEL, RATCH, NUDGE, COND); ENV / LFO / FX / EDIT tapped: that button's page on the
 *                    knobs, and each knob turned LOCKS its parameter on the step (again: the next page); PRESETS a
 *                    lock of the SEQ layer's lock parameter, ALGORITHM picks it; OCT- the step's locks go (on a lock
 *                    page: that page's only; on the grid: locks, nudge and condition), OCT+ its condition
 *   black keys 1..4  the page of steps (1-16 .. 49-64); OCT- / OCT+ (no step held) the page before / after
 * The screen shows the steps in the track's colour, the playhead, the page, and what the step held (or under the
 * cursor) holds: its notes, length, velocity, flags, condition, nudge and every lock. */
/* (the bands of ui_op.c, included after this: OP_BODY_Y, OP_KNOB_Y, OP_SCENE_H) */
#define SG_BODY_Y 22u
#define SG_KNOB_Y 154u
#define SG_SCENE_H 96u
static uint16_t sg_held;                                /* the white keys held on the grid (steps of the page shown) */
static uint16_t sg_pend;                                /* of those, the set steps that clear when let go (untouched) */
static uint8_t sg_alt;                                  /* a synth step held: 0 NOTE LEN VEL COND, 1 FLAG RATCH NUDGE */
static uint8_t sg_lpage = 0xFF;                         /* a step held + ENV / LFO / FX / EDIT: the page that locks */
static uint8_t sg_trk;                                  /* the track the steps held are on (from the first key down) */
static void lock_par_step(int32_t s);                   /* (ui_layers.c) */
static void stl_key(uint32_t k, uint32_t down);         /* (ui_stut.c) */
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
    return is_drum(t) ? dstep_has(&t->dstep[idx], drum_lane) : step_on(&t->step[idx]);
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
        dstep_set(&t->dstep[idx], drum_lane, LV_NORM, 0);
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
        dstep_clr(&t->dstep[idx], drum_lane);
    } else {
        note_set_len(t, idx, 1);                        /* (its TIEs go with it) */
        step_clear(&t->step[idx]);
    }
    if (!is_drum(t) || !dstep_mask(&t->dstep[idx])) {   /* an empty step keeps no lock, no nudge, no condition */
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
    if (song.playing && arrangement_enabled) {
        ui_message("STOP FIRST");
        return;
    }
    sg_undo(t);
    fm1_irq_off();
    for (i = len; i < dst; i++) {                       /* (the short last page made whole: empty steps) */
        memset(&t->step[i], 0, sizeof t->step[i]);
        if (!is_drum(t))
            t->step[i].time = ST_REST;
        lock_del(t, i, P_COUNT);
        t->micro[i] = 0;
        step_fill_set(t, i, FC_NORM);
    }
    for (i = 0; i < 16u; i++) {
        t->step[dst + i] = t->step[src + i];            /* (a drum step is the same 10 bytes: the union) */
        t->micro[dst + i] = t->micro[src + i];
        step_fill_set(t, dst + i, step_fill(t, src + i));
        lock_del(t, dst + i, P_COUNT);
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
/* the GRID shown, no layer held: its keys are steps (seq.c kb_grid). The drum track's KIT page: its pads */
static int grid_keys_on(void) { return on_drum_page() && (song.sel != TRK_DRUM || !drum_page) && !ui.menu; }
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
    if (song.playing && arrangement_enabled) {
        ui_message("STOP FIRST");
        return;
    }
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
/* a page whose knobs can lock on the selected track: shown, a track's page, with a lockable parameter */
static int sg_lock_page_ok(const page_t *pg)
{
    uint32_t s;
    if (!page_shown(pg) || (pg->scope != SC_TRACK && pg->scope != SC_ENGINE) || (is_drum(TSEL) && !page_for_drum(pg)))
        return 0;
    for (s = 0; s < 4u; s++) {
        uint32_t id = pg->id[s];
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
/* KNOB k with steps held: the lock page's parameter, else the step's own values */
static void sg_held_knob(uint32_t k, int32_t s)
{
    track_t *t = TSEL;
    uint32_t w, len = trk_len(t);
    int32_t heard = -1;                                 /* a step whose NOTE moved: heard */
    if (sg_lpage < NPAGES) {
        sg_lock_turn(PAGES[sg_lpage].id[k], s, EN_K1 + k);
        return;
    }
    sg_undo(t);
    sg_pend &= (uint16_t)~sg_held;
    fm1_irq_off();
    for (w = 0; w < 16u; w++) {
        uint32_t idx = sg_page() * 16u + w, i;
        if (!((sg_held >> w) & 1u) || idx >= len)
            continue;
        if (is_drum(t)) {                               /* LEVEL, RATCH, NUDGE, COND */
            dstep_t *d = &t->dstep[idx];
            uint32_t lv = dstep_lvl(d, drum_lane), rt = dstep_rat(d, drum_lane);
            if (k == 2u) {
                t->micro[idx] = (int8_t)clamp(t->micro[idx] + s, MICRO_MIN, MICRO_MAX);
            } else if (k == 3u) {
                step_fill_set(t, idx, (uint32_t)clamp((int32_t)step_fill(t, idx) + (s > 0 ? 1 : -1), 0, 3));
            } else if (dstep_has(d, drum_lane)) {
                if (k == 0u)
                    lv = LV_UP[clamp((int32_t)lvl_rank(lv) + (s > 0 ? 1 : -1), 0, 3)];
                else
                    rt = (uint32_t)clamp((int32_t)rt + (s > 0 ? 1 : -1), 0, 3);
                dstep_set(d, drum_lane, lv, rt);
            }
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
    if (song.playing && arrangement_enabled) {
        ui_message("STOP FIRST");
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
        if (up) {
            if (first)
                v = (step_fill(t, idx) + 1u) % 4u;
            first = 0;
            step_fill_set(t, idx, v);
        } else if (sg_lpage < NPAGES) {
            for (s = 0; s < 4u; s++)
                if (PAGES[sg_lpage].id[s] < P_COUNT && lock_find(t, idx, PAGES[sg_lpage].id[s], 0) >= 0) {
                    lock_del(t, idx, PAGES[sg_lpage].id[s]);
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
    } else {
        ui_message(n ? "LOCKS, NUDGE, COND CLEARED" : "NO LOCK, NUDGE OR COND");
    }
}
/* the GRID's buttons and knobs that are the same on every track: OCT- / OCT+, ENV / LFO, PRESETS and ALGORITHM
 * with a step held, SELECT. Returns 1 when KNOB 1..4 were taken here (a step held, or a synth track) */
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
    if ((s = panel_enc(EN_PRESET)) != 0)                /* the SEQ layer's lock parameter, locked on the steps */
        sg_lock_turn(ui.lock_par % P_COUNT, s, EN_PRESET);
    if ((s = panel_enc(EN_ALGO)) != 0)
        lock_par_step(s);
    if ((s = panel_enc(EN_SELECT)) != 0 && !is_drum(TSEL)) {
        sg_alt = (uint8_t)(s > 0);                      /* NOTE LEN VEL COND <-> FLAG RATCH NUDGE */
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
/* the synth GRID's input (the drum track's: drum_screen_input, which calls sg_input first) */
static void sg_synth_input(uint32_t pressed)
{
    uint32_t k, total, cur;
    int32_t s;
    if (sg_input(pressed) || !on_drum_page())
        return;
    if ((s = panel_enc(EN_PRESET)) != 0 && (cur = preset_pos(&total), total))   /* the sound, as on HOME */
        preset_go((uint32_t)(((int32_t)cur + s % (int32_t)total + (int32_t)total) % (int32_t)total));
    if ((s = panel_enc(EN_SELECT)) != 0) {              /* the tempo, as on HOME */
        song.g[G_BPM] = (int16_t)clamp(song.g[G_BPM] + accel(EN_SELECT, s, 200), GP[G_BPM].min, GP[G_BPM].max);
        ui.bpm_t = 40;
    }
    for (k = 0; k < 4u; k++)
        if ((s = panel_enc(EN_K1 + k)) != 0) {
            ui.hot_col = (uint8_t)k;
            ui.hot_t = 40;
            sg_free_knob(k, s);
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
    if (is_drum(t)) {
        const dstep_t *d = &t->dstep[idx];
        if (dstep_has(d, drum_lane)) {
            ln[0][0] = 0;
            if (!held) {
                str_cpy(ln[0], "LEVEL ", 31);
                str_cpy(ln[0] + 6, LV_NAME[dstep_lvl(d, drum_lane)], 8);
                str_cpy(ln[0] + str_len(ln[0]), "  ", 3);
            }
            str_cpy(ln[0] + str_len(ln[0]), "RATCH X", 8);
            fmt_int(ln[0] + str_len(ln[0]), (int32_t)dstep_rat(d, drum_lane) + 1);
        } else {
            str_cpy(ln[0], "NO HIT", 31);
        }
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
    n = 1;
    if (f || t->micro[idx]) {                           /* the condition and the nudge */
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
            str_cpy(ln[n], "LOCK", 31);
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
/* the step detail: what the step held (or under the cursor) holds, in a w x h band at y */
static void sg_detail_draw(uint32_t y, uint32_t h, uint32_t idx, uint32_t held)
{
    const track_t *t = TSEL;
    uint16_t col = TE_COL[song.sel & 3u];
    char ln[4][31], top[32], nm[20];
    uint32_t i, n = (sg_lines_held = (uint8_t)held, sg_lines(t, idx, ln));
    int32_t ty, x;
    cv_begin(240, h, C_BLACK);
    str_cpy(top, "STEP ", 8);
    fmt_int(top + 5, (int32_t)idx + 1);
    if (held && (sg_held & (sg_held - 1u)))
        str_cpy(top + str_len(top), " +", 3);           /* (more steps held: edited together) */
    if (held && sg_lpage < NPAGES) {                    /* its knobs lock: on which page */
        str_cpy(top + str_len(top), " LOCK ", 7);
        str_cpy(top + str_len(top), sg_lock_title(), 32u - str_len(top));
    }
    if (is_drum(t)) {
        str_cpy(nm, LANE_SHORT[drum_lane], 20);
    } else if (step_on(&t->step[idx])) {
        sg_notes_str(nm, t->step[idx].note, t->step[idx].n, 20);
    } else {
        str_cpy(nm, "--", 20);
    }
    if (held) {                                          /* held: a frame in the track's colour, the notes big */
        op_box(0, 0, 240, (int32_t)h, col);
        x = cv_text(6, 4, &FONT_S, top, C_WHITE);
        if (text_w(&FONT_L, nm) <= 232 - x)
            cv_text(240 - 6 - text_w(&FONT_L, nm), 2, &FONT_L, nm, col);
        else
            cv_text(x + 8, 4, &FONT_S, nm, col);
        ty = 36;
    } else {                                            /* the cursor's step's notes (its number: the knob row's STEP,
                                                         * review 19); the pen at the right */
        char pen[24];
        cv_text(6, 2, &FONT_S, nm, col);
        if (!is_drum(t)) {
            sg_notes_str(pen, pen_n ? pen_note : &last_note, pen_n ? pen_n : 1u, 24);
            x = 234 - text_w(&FONT_S, pen);
            cv_text(x, 2, &FONT_S, pen, col);
            cv_text(x - 4 - text_w(&FONT_S, "PEN"), 2, &FONT_S, "PEN", TE_G3);
        }
        cv_rect(0, 0, 240, 1, TE_G1);
        ty = 22;
    }
    for (i = 0; i < n && ty + 16 <= (int32_t)h; i++, ty += 17)
        cv_text(6, ty, &FONT_S, ln[i], i ? TE_G4 : C_WHITE);
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
            if (step_on(st)) {
                cv_rect(x, ty, w, (int32_t)th, held ? C_WHITE : TE_COL[sel]);
                if (big) {                              /* the note without its octave, centred; the octave as 1..4
                                                         * ticks over the velocity bar (review 19) */
                    const char *nm = N_NOTE[st->note[0] % 12u];
                    int32_t o = clamp((int32_t)st->note[0] / 12 - 2, 1, 4), q;
                    uint16_t ink = held || !(st->flags & SF_ACCENT) ? C_BLACK : C_WHITE;   /* an accent: white */
                    cv_text(x + (w - text_w(&FONT_S, nm)) / 2, ty + 4, &FONT_S, nm, ink);
                    for (q = 0; q < o; q++)
                        cv_rect(x + (w - (o * 4 - 2)) / 2 + q * 4, ty + (int32_t)th - 7, 2, 2, ink);
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
            if (step_locked(t, idx))                    /* a lock / nudge: a dot top centre (top right: the slide) */
                cv_rect(x + w / 2 - 1, ty + 1, 3, 2, step_on(st) ? C_BLACK : C_WHITE);
            if (step_fill(t, idx))
                cv_rect(x + 1, ty + 1, 4, 2, step_on(st) ? C_BLACK : C_WHITE);
        }
        if (idx == sg_cur() && !sg_held)
            op_box(x - 1, ty - 1, w + 2, (int32_t)th + 2, C_WHITE);
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
/* the knobs of the GRID with steps held (both tracks), or a lock page's: its parameters, each the step's lock
 * (full colour) or the track's value (dim: no lock there) */
static void sg_knobs_held(uint32_t idx, uint32_t *footer)
{
    const track_t *t = TSEL;
    static char v[4][12];
    const char *val[4] = {v[0], v[1], v[2], v[3]}, *lab[4], *u;
    static char lb[4][8];
    int32_t ratio[4] = {-1, -1, -1, -1};
    uint32_t k, dim = 0;
    if (sg_lpage < NPAGES) {
        const page_t *pg = &PAGES[sg_lpage];
        for (k = 0; k < 4u; k++) {
            uint32_t id = pg->id[k];
            lab[k] = lb[k];
            lb[k][0] = v[k][0] = 0;
            if (id < P_COUNT && p_lockable(id) && !(is_drum(t) && id > P_E0 && id <= P_E7) &&
                lock_desc(t, id)->max > lock_desc(t, id)->min) {
                const param_desc_t *d = lock_desc(t, id);
                int l = lock_find(t, idx, id, 0);
                int32_t x = l >= 0 ? t->lock[l].val : t->p[id];
                te_lower(lb[k], d->label, 8);
                param_format(d, x, v[k], &u);
                ratio[k] = (x - d->min) * 1000 / (d->max - d->min);
                if (l < 0)
                    dim |= 1u << k;
            }
        }
    } else if (is_drum(t)) {
        static const char *const LD[4] = {"level", "ratch", "nudge", "cond"};
        const dstep_t *d = &t->dstep[idx];
        for (k = 0; k < 4u; k++)
            lab[k] = LD[k];
        str_cpy(v[0], dstep_has(d, drum_lane) ? LV_NAME[dstep_lvl(d, drum_lane)] : "--", 8);
        v[1][0] = 'x', fmt_int(v[1] + 1, (int32_t)dstep_rat(d, drum_lane) + 1);
        fmt_int(v[2], t->micro[idx]);
        str_cpy(v[3], SG_COND[step_fill(t, idx)], 8);
        ratio[0] = dstep_has(d, drum_lane) ? (int32_t)lvl_rank(dstep_lvl(d, drum_lane)) * 333 : 0;
        ratio[1] = (int32_t)dstep_rat(d, drum_lane) * 333;
        ratio[2] = (t->micro[idx] - MICRO_MIN) * 1000 / (MICRO_MAX - MICRO_MIN);
        ratio[3] = (int32_t)step_fill(t, idx) * 333;
    } else {
        static const char *const LA[4] = {"note", "len", "vel", "cond"}, *const LB[4] = {"flag", "ratch", "nudge", ""};
        const step_t *st = &t->step[idx];
        for (k = 0; k < 4u; k++)
            lab[k] = sg_alt ? LB[k] : LA[k];
        v[3][0] = 0;
        if (!sg_alt) {
            if (step_on(st))
                note_name(v[0], st->note[0]);
            else
                str_cpy(v[0], "--", 4);
            fmt_int(v[1], step_on(st) ? (int32_t)note_len(t, idx) : 0);
            fmt_int(v[2], step_on(st) ? (int32_t)step_vel_shown(st) : 0);
            str_cpy(v[3], SG_COND[step_fill(t, idx)], 8);
            ratio[0] = step_on(st) ? st->note[0] * 1000 / 127 : 0;
            ratio[1] = step_on(st) ? (int32_t)note_len(t, idx) * 1000 / (int32_t)trk_len(t) : 0;
            ratio[2] = step_on(st) ? (int32_t)step_vel_shown(st) * 1000 / 127 : 0;
            ratio[3] = (int32_t)step_fill(t, idx) * 333;
        } else {
            uint32_t f = (st->flags & SF_ACCENT ? 1u : 0u) | (st->flags & SF_SLIDE ? 2u : 0u);
            str_cpy(v[0], SG_FLAG[f], 4);
            v[1][0] = 'x', fmt_int(v[1] + 1, (int32_t)(st->rat & 3u) + 1);
            fmt_int(v[2], t->micro[idx]);
            ratio[0] = (int32_t)f * 333;
            ratio[1] = (int32_t)(st->rat & 3u) * 333;
            ratio[2] = (t->micro[idx] - MICRO_MIN) * 1000 / (MICRO_MAX - MICRO_MIN);
        }
    }
    knob_tint_all(song.sel);
    op_knobs(lab, val, 0, ratio, dim, 7u + sg_lpage * 11u + sg_alt * 3u, footer);
}
/* the synth GRID: header, the steps (or the step held, big), the knobs, the tracks */
static void sg_draw(void)
{
    static uint32_t head, tiles_sig, footer, bar;
    const track_t *t = TSEL;
    const engine_t *e = ENGINES[t->eng_req % NENGINES];
    uint32_t len = trk_len(t), i, sig = 0, c, pages = (len + 15u) / 16u;
    int32_t h0 = sg_first();
    char title[20];
    if (ui.cursor >= len)
        sg_cur_set(len - 1u);
    c = h0 >= 0 ? (uint32_t)h0 : sg_cur();
    if (sg_lpage < NPAGES) {                             /* a lock page: its name, whole (the step band says LOCK) */
        str_cpy(title, sg_lock_title(), 20);
    } else {
        str_cpy(title, "GRID ", 20);
        fmt_int(title + 5, (int32_t)sg_page() + 1);
        str_cpy(title + str_len(title), "/", 2);
        fmt_int(title + str_len(title), (int32_t)pages);
    }
    op_header(e != &ENG_GONE ? e->name : "SYN", TE_COL[song.sel & 3u], title, &head);
    for (i = 0; i < len; i++) {                          /* (what the tiles and the detail show) */
        const step_t *st = &t->step[i];
        sig = sig * 31u + st->note[0] + st->n * 131u + st->time * 7u + st->vel * 17u + st->flags * 3u + st->rat * 5u;
        sig = sig * 7u + step_fill(t, i) + (uint32_t)(t->micro[i] + 64) * 3u;
    }
    for (i = 0; i < NLOCK; i++)
        sig = sig * 31u + t->lock[i].step + t->lock[i].param * 257u + (uint32_t)t->lock[i].val * 65537u;
    sig = sig * 31u + sg_held * 3u + sg_cur() * 977u + len + song.sel * 7919u + pen_note[0] * 13u + pen_n +
          (song.playing ? t->seq_idx + 1u : 0u) * 71u;
    if (ui.force || sig != tiles_sig) {
        tiles_sig = sig;
        if (h0 >= 0) {                                   /* a step held: a slim strip, then all it holds */
            sg_tiles_draw(SG_BODY_Y, 0);
            sg_detail_draw(SG_BODY_Y + 36u, SG_KNOB_Y - SG_BODY_Y - 36u, c, 1);
        } else {
            sg_tiles_draw(SG_BODY_Y, 1);
            sg_detail_draw(SG_BODY_Y + 72u, SG_KNOB_Y - SG_BODY_Y - 72u, c, 0);
        }
    }
    if (h0 >= 0 || sg_lpage < NPAGES) {
        sg_knobs_held(c, &footer);
    } else {
        static char v[4][12];
        static const char *const LF[4] = {"note", "step", "hit", "vel"};
        const char *val[4] = {v[0], v[1], v[2], v[3]};
        const step_t *st = &t->step[c];
        int32_t ratio[4];
        note_name(v[0], pen_n ? pen_note[0] : last_note);
        fmt_int(v[1], (int32_t)c + 1);
        str_cpy(v[2], step_on(st) ? "on" : "--", 4);
        if (step_on(st))
            fmt_int(v[3], (int32_t)step_vel_shown(st));
        else
            str_cpy(v[3], "--", 4);
        ratio[0] = (pen_n ? pen_note[0] : last_note) * 1000 / 127;
        ratio[1] = (int32_t)c * 1000 / (int32_t)(len > 1u ? len - 1u : 1u);
        ratio[2] = step_on(st) ? 1000 : 0;
        ratio[3] = step_on(st) ? (int32_t)step_vel_shown(st) * 1000 / 127 : 0;
        knob_tint_all(song.sel);
        op_knobs(LF, val, 0, ratio, 0, 3u, &footer);
    }
    op_trackbar(&bar);
}
