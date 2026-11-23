/* SPDX-License-Identifier: GPL-3.0-only */
/* (PHEBUS wave 12) CHANGING A DRUM SOUND, the screens' side (Ben, 23 Nov 2026, 11:02: "Kind of confusing how to change the sound
 * of individual drums ... couldn't see how to load in like an 808 snare with a 606 kit preset. Or just choose the snare engine
 * and then do sound design"; 11:10: "Kits on HOME, sounds on the drum screen", "Same kind first, then all", "Yes, an INIT per
 * engine"). Included by ui_studio.c after ui_drum.c. The lists themselves: ukit.c.
 *
 *   PRESETS on the drum screen (any tab)  the selected slot's SOUND: each detent the slot takes the next sound of its list at
 *          once and you hear it (stopped: once a detent; playing: the pattern plays it). Turning back to where you started
 *          gives the sound you had back, whatever you had done to it (and its locks); SAVE + OCT- undoes the whole turn.
 *   PRESETS on HOME (the drums selected), SAVE > PRESETS, TRACKS  the WHOLE KIT: the 16 slots. The one thing here that asks
 *          (HOLD SAVE: LOAD 808), and only when the kit has been changed from the kit loaded (nothing to lose otherwise).
 *   The readout, in the same place both times (the dials' band, while the knob turns and a moment after): KIT 808 / SOUND 808
 *          SNARE, then the group and the place in it (CLASSIC KITS 2/32, SNARE 3/41), and the locks a machine change kept or
 *          dropped.
 *   SAVE > KITS  the user kits: KNOB 1 the kit (MY KIT 1 .. 6), LOAD, ERASE, SAVE (GO knobs: right). */
static void ask_drop(void);                             /* (ui_input.c) */
_Static_assert(sizeof(pkit_t) + sizeof(dpat.lk) <= sizeof(dpat_t), "the kit and the locks fit the undo's room");
static uint32_t kit_undo_sess = 1;
static void kit_undo_mark(void)                         /* the kit and the sounds' locks as they are, for SAVE + OCT- */
{
    uint8_t *r = (uint8_t *)&undo.dp;
    memcpy(r, &pkit, sizeof pkit);
    memcpy(r + sizeof pkit, dpat.lk, sizeof dpat.lk);
    undo.trk = UNDO_KIT;
    undo.sess = ++kit_undo_sess;
    undo.valid = 1;
    undo.undone = 0;
}
static const pkit_t *kit_undo_kit(void) { return (const pkit_t *)(const void *)&undo.dp; }
static const dlk_t *kit_undo_lk(void) { return (const dlk_t *)(const void *)((const uint8_t *)&undo.dp + sizeof(pkit_t)); }
static void kit_undo_swap(void)                         /* (ui.c undo_swap) the kit and the locks and the room swap places */
{
    uint8_t *a = (uint8_t *)&pkit, *b = (uint8_t *)&undo.dp, *c = (uint8_t *)dpat.lk, x;
    uint32_t i, k;
    fm1_irq_off();
    for (k = 0; k < NDRUM; k++)
        if (drums.v[k].active) {
            drums.v[k].active = 0;
            drum_cut(k);
        }
    for (i = 0; i < sizeof pkit; i++)
        x = a[i], a[i] = b[i], b[i] = x;
    for (i = 0; i < sizeof dpat.lk; i++)
        x = c[i], c[i] = b[sizeof pkit + i], b[sizeof pkit + i] = x;
    pkit_gen++;
    fm1_irq_on();
}

/* ---- the browse: one turn of PRESETS after another on the same slot (or kit) is one session, one undo */
static struct {
    uint8_t kind;                                       /* 0 none, 1 a slot's sound, 2 the whole kit */
    uint8_t lane, in, asked;                            /* the slot; the start is in the list; the kit question was asked */
    uint16_t pos, start, n;                             /* the place (n: the start, when it is not in the list) */
    uint32_t sess;
    uint8_t gen;
} brw;
static uint8_t brw_t;                                   /* frames the readout stays */
static char brw_l1[28], brw_l2[24], brw_r2[20];         /* the readout: KIT 808 / SOUND 808 SNARE; group and place; locks */
static uint16_t brw_col;
static int brw_live(uint32_t kind)
{
    return brw.kind == kind && undo.valid && !undo.undone && undo.trk == UNDO_KIT && undo.sess == brw.sess &&
           brw.gen == pkit_gen && (kind != 1u || brw.lane == (drum_lane & 15u));
}
static void brw_locks(uint32_t kept, uint32_t dropped)
{
    brw_r2[0] = 0;
    if (dropped) {
        fmt_int(brw_r2, (int32_t)dropped);
        str_cpy(brw_r2 + str_len(brw_r2), dropped == 1u ? " LOCK OFF" : " LOCKS OFF", 11);
    } else if (kept) {
        fmt_int(brw_r2, (int32_t)kept);
        str_cpy(brw_r2 + str_len(brw_r2), kept == 1u ? " LOCK MOVED" : " LOCKS MOVED", 13);
    }
}
static void brw_place(const char *grp, uint32_t i, uint32_t n)
{
    char b[8];
    str_cpy(brw_l2, grp, sizeof brw_l2);
    str_cpy(brw_l2 + str_len(brw_l2), " ", 2);
    fmt_int(b, (int32_t)i);
    str_cpy(brw_l2 + str_len(brw_l2), b, 6);
    str_cpy(brw_l2 + str_len(brw_l2), "/", 2);
    fmt_int(b, (int32_t)n);
    str_cpy(brw_l2 + str_len(brw_l2), b, 6);
}
static uint32_t brw_step(int32_t s, uint32_t ring)      /* the place s detents on, round */
{
    int32_t p = ((int32_t)brw.pos + s % (int32_t)ring + (int32_t)ring) % (int32_t)ring;
    return (uint32_t)p;
}

/* PRESETS on the drum screen: the selected slot's sound, s detents on */
static void snd_browse(int32_t s)
{
    uint32_t L = drum_lane & 15u, kept = 0, dropped = 0, n, ring;
    const pkit_t *o;
    dslot_t d;
    if (!s)
        return;
    if (!brw_live(1u)) {
        int32_t p = snd_pos(L);
        kit_undo_mark();
        brw.kind = 1, brw.lane = (uint8_t)L, brw.sess = undo.sess;
        brw.n = (uint16_t)snd_n();
        brw.in = (uint8_t)(p >= 0);
        brw.pos = brw.start = (uint16_t)(p >= 0 ? (uint32_t)p : brw.n);
    }
    o = kit_undo_kit();
    n = brw.n;
    ring = brw.in ? n : n + 1u;
    brw.pos = (uint16_t)brw_step(s, ring);
    if (brw.pos == brw.start) {                         /* back where it started: the sound it had, exactly */
        d = o->s[L];
    } else if (!snd_make(L, brw.pos, &d)) {
        return;
    } else {
        d.mach |= SL_CHG;
    }
    slot_take(L, &d, kit_undo_lk(), o->s[L].mach & 1u, o->s[L].snd, &kept, &dropped);
    brw.gen = pkit_gen;
    str_cpy(brw_l1, "SOUND ", sizeof brw_l1);
    slot_name(&pkit.s[L], brw_l1 + 6, sizeof brw_l1 - 6u);
    if (brw.pos >= n) {
        str_cpy(brw_l2, "AS IT WAS", sizeof brw_l2);
    } else {
        char g[12];
        uint32_t i, sz;
        snd_group(L, brw.pos, g, sizeof g, &i, &sz);
        brw_place(g, i, sz);
    }
    brw_locks(kept, dropped);
    brw_col = TE_DRUM;
    brw_t = 90;
    if (!song.playing)
        audition_lane(L);                               /* (heard once a detent, stopped; playing, the pattern plays it) */
    ui.force = 1;
}
/* the whole kit made kit list entry e (the start when e is the list's n), the locks carried from the start */
static void kit_take(uint32_t e)
{
    const pkit_t *o = kit_undo_kit();
    pkit_t k;
    uint32_t l, kept = 0, dropped = 0;
    if (e == brw.start && !brw.in)
        k = *o;
    else if (!kit_make(kit_src(e), &k))
        return;
    fm1_irq_off();
    for (l = 0; l < NDRUM; l++)
        if (drums.v[l].active) {
            drums.v[l].active = 0;
            drum_cut(l);
        }
    pkit = k;
    for (l = 0; l < DRUM_LANES; l++) {
        uint32_t a, b;
        lk_rebuild(l, kit_undo_lk(), o->s[l].mach & 1u, o->s[l].snd, &a, &b);
        kept += a, dropped += b;
    }
    pkit_gen++;
    fm1_irq_on();
    brw.gen = pkit_gen;
    brw_locks(kept, dropped);
}
static void kit_readout(uint32_t e)
{
    str_cpy(brw_l1, "KIT ", sizeof brw_l1);
    if (e >= brw.n) {
        str_cpy(brw_l1 + 4, "CUSTOM", sizeof brw_l1 - 4u);
        str_cpy(brw_l2, "AS IT WAS", sizeof brw_l2);
    } else {
        uint32_t src = kit_src(e), i, first = e, last = e;
        kit_name_of(src, brw_l1 + 4, sizeof brw_l1 - 4u);
        while (first && str_eq(kit_group(kit_src(first - 1u)), kit_group(src)))
            first--;
        while (last + 1u < brw.n && str_eq(kit_group(kit_src(last + 1u)), kit_group(src)))
            last++;
        i = e - first + 1u;
        brw_place(kit_group(src), i, last - first + 1u);
    }
    brw_col = TE_DRUM;
    brw_t = 90;
    ui.force = 1;
}
/* PRESETS where it loads whole kits: the kit s detents on; asks first when the kit has been changed (HOLD SAVE: LOAD 808) */
static void kit_browse(int32_t s)
{
    uint32_t ring;
    if (!s)
        return;
    if (!brw_live(2u)) {
        int32_t p = kit_changed() ? -1 : kit_pos(pkit.base);
        kit_undo_mark();
        brw.kind = 2, brw.sess = undo.sess, brw.gen = pkit_gen;
        brw.n = (uint16_t)kit_src_n();
        brw.in = (uint8_t)(p >= 0);
        brw.pos = brw.start = (uint16_t)(p >= 0 ? (uint32_t)p : brw.n);
        brw.asked = 0;
    }
    ring = brw.in ? brw.n : brw.n + 1u;
    brw.pos = (uint16_t)brw_step(s, ring);
    brw_r2[0] = 0;
    kit_readout(brw.pos);
    if (!brw.in && !brw.asked && brw.pos != brw.start) {   /* a changed kit: nothing loads until the question is answered */
        char q[24] = "LOAD ";
        kit_name_of(kit_src(brw.pos), q + 5, sizeof q - 5u);
        ask_set(ASK_KIT, brw.pos, q);
        return;
    }
    if (ui.ask == ASK_KIT)
        ask_drop();
    kit_take(brw.pos);
}
static void kit_ask_do(uint32_t e)                      /* (ui_input.c ask_do) the question answered: that kit loads */
{
    if (!brw_live(2u))
        return;
    brw.asked = 1;
    brw.pos = (uint16_t)e;
    kit_take(e);
    kit_readout(e);
}
/* MIDI Program Change on the drum channel: kit n of the list (as PRESETS: asks first over a changed kit). 0: done or asked */
static int kit_pc(uint32_t n)
{
    if (n >= kit_src_n())
        return 1;
    brw.kind = 0;
    if (!kit_changed()) {
        kit_undo_mark();
        brw.kind = 2, brw.sess = undo.sess, brw.gen = pkit_gen, brw.n = (uint16_t)kit_src_n(), brw.in = 1;
        brw.start = brw.pos = (uint16_t)(kit_pos(pkit.base) >= 0 ? (uint32_t)kit_pos(pkit.base) : 0u);
        brw.pos = (uint16_t)n;
        kit_take(n);
        kit_readout(n);
        return 0;
    }
    kit_undo_mark();
    brw.kind = 2, brw.sess = undo.sess, brw.gen = pkit_gen, brw.n = (uint16_t)kit_src_n(), brw.in = 0;
    brw.start = brw.n, brw.pos = (uint16_t)n, brw.asked = 0;
    kit_readout(n);
    {
        char q[24] = "LOAD ";
        kit_name_of(kit_src(n), q + 5, sizeof q - 5u);
        ask_set(ASK_KIT, n, q);
    }
    return 0;
}

/* ---- a knob turned on a slot: the project's sound itself (it now differs from the kit loaded) */
static void slot_edited(uint32_t l)
{
    pkit.s[l & 15u].mach |= SL_CHG | SL_EDT;
    pkit_gen++;
}

/* ---- SAVE > KITS: the user kits */
static uint8_t ukit_sel;                                /* the kit KNOB 1 picks (0..UKIT_N - 1) */
static int ukit_has(uint32_t n) { return ukit_state(n) == UK_STORED || (n < DSU_N && ukit_state(n) == UK_NONE); }
static void ukit_label(uint32_t n, char *b, uint32_t m)  /* what slot n holds: MY KIT 1, MY HEX 1 (old), EMPTY */
{
    if (ukit_has(n))
        ukit_name(n, b, m);
    else
        str_cpy(b, "EMPTY", m);
}
enum { UKOP_LOAD, UKOP_ERASE, UKOP_SAVE };
static void ukit_do(uint32_t op, uint32_t n)            /* done (asked where it erases or replaces) */
{
    char b[24], nm[12];
    ukit_label(n, nm, sizeof nm);
    if (op == UKOP_SAVE) {
        pkit_t k = pkit;
        uint32_t l;
        for (l = 0; l < DRUM_LANES; l++)
            k.s[l].mach &= (uint8_t)~(SL_CHG | SL_EDT);
        k.base = (uint8_t)(DSRC_USER + n);
        memset(k.name, 0, sizeof k.name);
        str_cpy(k.name, "MY KIT ", 8), k.name[7] = (char)('1' + n);
        if (ukit_put(n, &k)) {
            ui_message("NOT SAVED: FLASH");
            return;
        }
        fm1_irq_off();
        for (l = 0; l < DRUM_LANES; l++)
            pkit.s[l].mach &= (uint8_t)~(SL_CHG | SL_EDT);
        pkit.base = k.base;
        pkit_gen++;
        fm1_irq_on();
        str_cpy(b, "KIT SAVED: MY KIT ", sizeof b), b[18] = (char)('1' + n), b[19] = 0;
        ui_message(b);
    } else if (op == UKOP_ERASE) {
        if (ukit_put(n, 0)) {
            ui_message("NOT ERASED: FLASH");
            return;
        }
        ui_say("ERASED ", nm);
    } else {
        uint32_t src = ukit_state(n) == UK_STORED ? DSRC_USER + n : DRUM_SYN + n;
        int32_t e;
        brw.kind = 0;
        if ((e = kit_pos(src)) < 0)
            return;
        kit_undo_mark();
        brw.kind = 2, brw.sess = undo.sess, brw.gen = pkit_gen, brw.n = (uint16_t)kit_src_n(), brw.in = 1;
        brw.start = brw.pos = (uint16_t)e;
        kit_take((uint32_t)e);
        ui_say("LOADED ", nm);
    }
    ui.force = 1;
}
static void ukit_turn(uint32_t k, int32_t s)            /* SAVE > KITS: KNOB 1 the kit; LOAD ERASE SAVE: turned right */
{
    char b[24], nm[12];
    uint32_t n = ukit_sel;
    if (k == 0u) {
        ukit_sel = (uint8_t)clamp((int32_t)ukit_sel + s, 0, UKIT_N - 1);
        return;
    }
    if (s <= 0)
        return;
    ukit_label(n, nm, sizeof nm);
    if (k == 1u) {                                      /* LOAD: asks only over a changed kit (as PRESETS) */
        if (!ukit_has(n)) {
            ui_message("EMPTY");
            return;
        }
        if (!kit_changed()) {
            ukit_do(UKOP_LOAD, n);
            return;
        }
        str_cpy(b, "LOAD ", sizeof b);
    } else if (k == 2u) {                               /* ERASE: asks */
        if (!ukit_has(n)) {
            ui_message("EMPTY");
            return;
        }
        str_cpy(b, "ERASE ", sizeof b);
    } else {                                            /* SAVE: at once into an empty one, asks over one that holds a kit */
        if (!ukit_has(n)) {
            ukit_do(UKOP_SAVE, n);
            return;
        }
        str_cpy(b, "SAVE OVER ", sizeof b);
    }
    str_cpy(b + str_len(b), nm, sizeof b - str_len(b));
    ask_set(ASK_UKIT, (k - 1u) | n << 2, b);
}
static void graph_ukit(void)                            /* the six user kits, what each holds, the one KNOB 1 picks white */
{
    uint32_t n;
    for (n = 0; n < UKIT_N; n++) {
        int32_t y = 4 + (int32_t)n * 19;
        char tag[3] = {(char)('1' + n), 0, 0}, nm[12];
        int sel = n == ukit_sel, used = ukit_has(n);
        ukit_label(n, nm, sizeof nm);
        if (sel)
            cv_rect(4, y + 6, 3, 3, C_WHITE);
        cv_text(14, y, &FONT_S, tag, sel ? C_WHITE : C_GRAY);
        cv_text(40, y, &FONT_S, nm, used ? (sel ? TE_DRUM : TE_MID[3]) : C_DIM);
        if (used && ukit_state(n) == UK_NONE)
            cv_text(150, y, &FONT_S, "OLD", TE_G3);
    }
}

/* ---- each frame (ui_input): the readout's time; what a project's load had to say */
static void kit_ui_tick(void)
{
    if (brw_t)
        brw_t--;
    if (pkit_old_kept) {
        char b[30] = "OLD KIT COPY: MY KIT ";
        if (pkit_old_kept == 0xFFu)
            ui_message("OLD KIT COPY: NO ROOM, LOST");
        else
            b[21] = (char)('0' + pkit_old_kept), b[22] = 0, ui_message(b);
        pkit_old_kept = 0;
    }
}
/* the readout in the dials' band (ui_drum.c op_dials): KIT 808 / SOUND 808 SNARE large, the group and place, the locks */
static int brw_draw(void)
{
    if (!brw_t || ui.msg_t)
        return 0;
    cv_text_tall_n(4, 6, brw_l1, brw_col, str_len(brw_l1) > 26u ? 7 : 8);
    cv_text(4, 40, &FONT_S, brw_l2, TE_G4);
    if (brw_r2[0])
        cv_text(236 - text_w(&FONT_S, brw_r2), 40, &FONT_S, brw_r2, C_WHITE);
    return 1;
}
