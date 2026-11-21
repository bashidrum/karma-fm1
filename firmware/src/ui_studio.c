/* SPDX-License-Identifier: GPL-3.0-only */
/* LIVE screens, 240 x 240, in a teenage-engineering-like style: black, four colours (one per
 * track and per knob: blue, green, white, orange), white for what you touch, red for recording,
 * big numbers, lowercase labels, four dials at the bottom that show what KNOB 1..4 do.
 * Screens: TRACKS (the performance view), DRUMS (GRID / KIT, pads that flash on each hit), the
 * LAYERS (a function button held: what the 16 white keys and the knobs do now), HOLD (a hold to
 * confirm: clear, save), REC (armed / free take). Every band is drawn into the canvas only when its
 * signature changed. */
static uint8_t drum_page, drum_lane, drum_cursor;   /* drum_page: 0 GRID, 1 KIT */
static void trk_short_name(uint32_t c, char *b);
static void op_header(const char *tag, uint16_t tc, const char *title, uint32_t *cache);   /* ui_op.c */
static void op_knobs(const char *const lab[4], const char *const val[4], const char *const *unit, const int32_t ratio[4],
                     uint32_t dim, uint32_t sig, uint32_t *cache);
static void op_trackbar(uint32_t *cache);
static void op_drum_scene(uint32_t bank, uint32_t len);
static void op_drum_steps(uint32_t bank, uint32_t len);
static uint8_t op_steps_y;                              /* (ui_op.c) */
static void op_home_body(void);
static void op_box(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c);
static int on_drum_page(void) { return !ui.home && cur_page()->scope == SC_DRUM; }

/* ---------------------------------------------------------------- style --- */
#define TE_G1 RGB(26, 26, 30)            /* tiles */
#define TE_G2 RGB(54, 54, 60)            /* empty steps, dial tracks */
#define TE_G3 RGB(118, 118, 126)         /* labels */
#define TE_G4 RGB(196, 196, 204)         /* secondary text */
#define TE_RED RGB(255, 44, 52)          /* recording, erasing */
#define TE_MUTE RGB(255, 196, 40)        /* a muted track: a yellow M, as Ableton (SPEC.md, 20 Nov 2026) */
#define TE_SOLO RGB(150, 215, 255)       /* a soloed track: an ice-blue S, as Ableton (review 15: never the alien's blue) */
/* Ben's palette (SPEC.md, 20 Nov 2026): track 1 purple, 2 blue, 3 green, 4 (drums) coral */
static const uint16_t TE_COL[4] = {RGB(140, 118, 240), RGB(40, 160, 245), RGB(20, 196, 170), RGB(255, 128, 108)};
static const uint16_t TE_MID[4] = {RGB(92, 78, 158), RGB(26, 106, 162), RGB(13, 129, 112), RGB(168, 84, 71)};
static const uint16_t TE_DIM[4] = {RGB(48, 40, 82), RGB(14, 54, 83), RGB(7, 67, 58), RGB(87, 44, 37)};
#define TE_DRUM TE_COL[3]
/* Ben's rule (SPEC.md, 20 Nov 2026): a voice's colour means "this acts on that voice". A knob strip is tinted by what
 * each knob acts on: 0..3 a track, 4 neutral (general or global controls). knob_tint[] is set by the screen that draws
 * the strip, just before op_knobs / te_dials. */
#define TINT_NEUTRAL 4u
static const uint16_t *const TE_TINTS[3] = {TE_COL, TE_MID, TE_DIM};
static uint8_t knob_tint[4] = {0, 1, 2, 3};
static uint16_t tint_col(uint32_t k, uint32_t shade)    /* shade 0 full, 1 mid, 2 dim */
{
    static const uint16_t NEU[3] = {RGB(196, 196, 204), RGB(118, 118, 126), RGB(54, 54, 60)};
    uint32_t v = knob_tint[k & 3u];
    return v < 4u ? TE_TINTS[shade][v] : NEU[shade];
}
static void knob_tint_all(uint32_t v) { knob_tint[0] = knob_tint[1] = knob_tint[2] = knob_tint[3] = (uint8_t)v; }
static void knob_tint_voices(void) { knob_tint[0] = 0, knob_tint[1] = 1, knob_tint[2] = 2, knob_tint[3] = 3; }
static uint32_t knob_tint_sig(void) { return knob_tint[0] + knob_tint[1] * 5u + knob_tint[2] * 25u + knob_tint[3] * 125u; }

static void te_disc(int32_t cx, int32_t cy, int32_t r, uint16_t c)     /* filled circle */
{
    int32_t y, x;
    for (y = -r; y <= r; y++)
        for (x = -r; x <= r; x++)
            if (x * x + y * y <= r * r + r)
                cv_pset(cx + x, cy + y, c);
}
/* a dial: a 270-degree ring (lit up to the value), a pointer; ratio 0..1000, -1 = a plain ring */
static void te_dial(int32_t cx, int32_t cy, int32_t r, int32_t ratio, uint16_t c, uint16_t dim)
{
    int32_t i, end = ratio < 0 ? 768 : ratio * 768 / 1000;
    for (i = 0; i <= 768; i += 8) {
        uint32_t a = (uint32_t)(384 + i) & 1023u;
        int32_t co = SINE[(a + 256u) & 1023u], si = SINE[a], k;
        uint16_t col = ratio < 0 || i <= end ? c : dim;
        for (k = r - 2; k <= r; k++)
            cv_pset(cx + ((co * k) >> 15), cy + ((si * k) >> 15), col);
    }
    if (ratio >= 0) {
        uint32_t a = (uint32_t)(384 + end) & 1023u;
        int32_t co = SINE[(a + 256u) & 1023u], si = SINE[a];
        cv_line(cx, cy, cx + ((co * (r - 4)) >> 15), cy + ((si * (r - 4)) >> 15), C_WHITE);
        te_disc(cx, cy, 2, C_WHITE);
    }
}
static void te_play_icon(int32_t x, int32_t y, int playing)
{
    int32_t i;
    if (playing)
        for (i = 0; i < 12; i++)
            cv_rect(x + i, y + i / 2, 1, 14 - i, C_WHITE);   /* a triangle */
    else
        cv_rect(x, y + 1, 12, 12, TE_G3);
}
static uint32_t studio_hash(uint32_t h, const char *p)
{ while (*p) h = h * 31u + (uint8_t)*p++; return h; }
static void te_lower(char *d, const char *s, uint32_t n)
{
    uint32_t i;
    for (i = 0; i + 1u < n && s[i]; i++)
        d[i] = s[i] >= 'A' && s[i] <= 'Z' ? (char)(s[i] + 32) : s[i];
    d[i] = 0;
}
static void te_text_c(int32_t cx, int32_t y, const char *s, uint16_t c)   /* centred on cx */
{
    cv_text(cx - text_w(&FONT_S, s) / 2, y, &FONT_S, s, c);
}

/* the dial strip: KNOB 1..4, label + value under each (a dial with no label: an empty column); a
 * message replaces it */
static void te_dials(int32_t y0, const char *const lab[4], const char *const val[4], const int32_t ratio[4],
                     uint32_t sig, uint32_t *cache)
{
    uint32_t k;
    sig = studio_hash(sig + knob_tint_sig() * 7919u, ui.msg_t ? ui.msg : "");
    for (k = 0; k < 4u; k++) {
        sig = studio_hash(sig * 7u + (uint32_t)ratio[k] + (ui.hot_t && ui.hot_col == k) * 5003u, lab[k]);
        sig = studio_hash(sig, val[k]);
    }
    if (!ui.force && sig == *cache)
        return;
    *cache = sig;
    cv_begin(240, ui.msg_t ? 240u - (uint32_t)y0 : 40u, C_BLACK);
    if (ui.msg_t) {                                     /* a message: a white bar */
        cv_rect(0, 10, 240, 32, C_WHITE);
        cv_text((240 - text_w(&FONT_S, ui.msg)) / 2, 18, &FONT_S, ui.msg, C_BLACK);
        cv_blit(0, (uint32_t)y0);
        return;
    }
    for (k = 0; k < 4u; k++) {
        int32_t cx = 30 + 60 * (int32_t)k;
        if (!lab[k][0])
            continue;
        te_dial(cx, 12, 11, ratio[k], tint_col(k, 0), tint_col(k, 2));
        if (ui.hot_t && ui.hot_col == k) {              /* turned: the label inverted in the knob's colour (review 16e) */
            int32_t w = text_w(&FONT_S, lab[k]);
            cv_rect(cx - w / 2 - 1, 24, w + 1, 16, tint_col(k, 0));
            te_text_c(cx, 24, lab[k], C_BLACK);
        } else {
            te_text_c(cx, 24, lab[k], TE_G3);
        }
    }
    cv_blit(0, (uint32_t)y0);
    cv_begin(240, 16, C_BLACK);                         /* the values */
    for (k = 0; k < 4u; k++)
        te_text_c(30 + 60 * (int32_t)k, 0, val[k], TE_G4);
    cv_blit(0, (uint32_t)y0 + 40u);
}

static void studio_open(uint32_t scope)
{
    uint32_t i;
    if (scope == SC_SONG && rec_wait)
        rec_wait = 0;                                   /* (an arm does not follow into the song page) */
#if FELUCCA_ARRANGER
    if (scope == SC_SONG)
        song_bank = 0;                                  /* (3.0.5) the song first, its saved songs a SAVE away */
#endif
    for (i = 0; i < NPAGES; i++) if (PAGES[i].scope == scope) {
        ui.page = (uint8_t)i; ui.home = 0; song.seq_mode = 0; ui.force = 1;
        ui.msg_t = 0; ui.entry_open = 0; return;
    }
}
/* LIVE: REC, on any page. One record arm, on the selected track (it follows ALGORITHM). No
 * click (seq.c): playing, recording starts at once; stopped, REC arms and the first note
 * starts the loop (or, in an empty project, a free take that REC closes: seq.c ft_close).
 * REC again stops recording / cancels the arm (playback continues). */
static void rec_toggle(void)
{
    if (ft_owns_press())
        return;
    if (song.playing && arrangement_enabled) {
        ui_message("STOP FIRST");
        return;
    }
    if (song.rec || rec_wait) {
        song.rec = 0;
        rec_wait = 0;
        ui_message("REC OFF");
        return;
    }
    arrangement_enabled = 0;
    if (song.playing)
        rec_begin();                    /* playing: record now */
    else
        rec_wait = 1;                   /* stopped: the first note starts it */
}

/* the loop position of track t: "bar.beat" in its pattern (1.1 .. ) */
static void loop_pos(const track_t *t, char *b)
{
    uint32_t len = (uint32_t)clamp(t->p[P_SLEN], 1, NSTEP), s = t->seq_idx % len, k;
    fmt_int(b, (int32_t)(s / 16u + 1u));
    k = str_len(b);
    b[k] = '.';
    b[k + 1] = (char)('1' + (s % 16u) / 4u);
    b[k + 2] = 0;
}

/* the header of the live screens: BPM, transport, the loop position, the beat lights, REC */
static void te_header(const char *title, uint16_t tc, uint32_t *cache)
{
    char b[12];
    uint32_t beat = clk_beat, k, playing = song.playing;
    uint32_t sig = studio_hash((uint32_t)song.g[G_BPM] * 7u + playing * 3u + (song.rec != 0) * 1999u +
                               (playing ? beat * 131u + TSEL->seq_idx * 7919u : 0u) + (uint32_t)song.g[G_CLOCK] * 77u +
                               arrangement_enabled * 5u + (ui.bpm_t != 0) * 104729u + song.solo * 37u, title);
    sig = sig * 31u + tc;
    if (!ui.force && sig == *cache)
        return;
    *cache = sig;
    cv_begin(240, 40, C_BLACK);
    fmt_int(b, song.g[G_BPM]);
    cv_text(4, 4, &FONT_L, b, ui.bpm_t ? C_WHITE : TE_G4);
    cv_text(4 + text_w(&FONT_L, b) + 4, 20, &FONT_S, "bpm", TE_G3);
    te_play_icon(104, 6, (int)playing);
    if (playing) {                                     /* bar.beat in the selected track's loop */
        loop_pos(TSEL, b);
        cv_text(122, 5, &FONT_S, b, C_WHITE);
    } else {
        cv_text(122, 5, &FONT_S, arrangement_enabled ? "song" : "loop", TE_G3);
    }
    for (k = 0; k < 4u; k++)                           /* the four beats of the bar */
        cv_rect(104 + (int32_t)k * 9, 26, 7, 7, playing && beat % 4u == k ? (k || !song.rec ? C_WHITE : TE_RED) : TE_G2);   /* (red: recording only) */
    if (song.rec) {
        te_disc(224, 12, 8, TE_RED);
        cv_text(176, 4, &FONT_S, "rec", TE_RED);
        if (song.g[G_CLOCK] != 0)                       /* the click is on */
            cv_text(168, 22, &FONT_S, "click", TE_G3);
    } else {
        cv_text(240 - text_w(&FONT_S, title) - 4, 5, &FONT_S, title, tc);
        if (song.solo)
            cv_text(236 - text_w(&FONT_S, "solo"), 22, &FONT_S, "solo", C_WHITE);
    }
    cv_rect(0, 39, 240, 1, TE_G1);
    cv_blit(0, 0);
}

/* --------------------------------------------------------------- TRACKS --- */
/* swing, MPC style: "62%" */
static void swing_str(char *b, int32_t v)            /* 0 straight .. 100 the most (params.c F_SWING) */
{
    fmt_int(b, clamp(v, 0, 100));
}

/* (PHEBUS, Ben 21 Nov 2026: "having easy access to decay, filter, fx, makes for fun play on the fly"; picker "Cutoff,
 * decay, FX, level", and of the FX knob: "Would be cool if it actually just turned on and off the stutter") HOME's KNOB 1
 * and 2, for the selected voice: CUTOFF, the engine's own filter cutoff (its CUT: ACID, JUNO), else the voice's FILTER
 * (P_TFLT: PLAITS, ELEMENTS, and the drums' track filter); DECAY, the decay of the envelope that moves the sound: the
 * engine's own envelope where it has one (ACID's DECAY), else the shared ENV's DEC (JUNO, PLAITS, ELEMENTS); on the
 * drums the selected sound's DEC of a HEX kit (its HEX AEG; the other kits have none: 0). (KNOB 3 the STUTTER lane on /
 * off, KNOB 4 LEVEL: ui_input.c tracks_edit) */
static int16_t *home_knob(uint32_t k, const param_desc_t **d)
{
    track_t *t = TSEL;
    uint32_t i, kit = drum_kit();
    if (is_drum(t)) {
        if (!k) {
            *d = &TP[P_TFLT];
            return &t->p[P_TFLT];
        }
        *d = &GP[G_HXDEC];
        return kit < DRUM_SAMPLED || kit >= DRUM_SYN ? &song.g[G_HXDEC] : 0;
    }
    for (i = 0; i < 8u; i++)
        if (str_eq(ENGINES[t->eng_req % NENGINES]->edit[i].label, k ? "DECAY" : "CUT")) {
            *d = track_desc(t, P_E0 + i);
            return &t->p[P_E0 + i];
        }
    *d = &TP[k ? P_DEC : P_TFLT];
    return &t->p[k ? P_DEC : P_TFLT];
}
static void studio_tracks_draw(void)
{
    static uint32_t head, footer, bar;
    op_header("HOME", TE_G3, arrangement_enabled ? "song" : "loop", &head);
    op_home_body();                                    /* the deck, the selected track, the four lanes (ui_op.c) */
    {   /* KNOB 1 cutoff, 2 decay of the selected voice, 3 the STUTTER lane on / off (everyone's), 4 the voice's level */
        track_t *t = TSEL;
        static char v[4][8];
        static const char *const lab[4] = {"cutoff", "decay", "stut", "level"};
        const char *val[4] = {v[0], v[1], v[2], v[3]}, *u;
        int32_t ratio[4];
        uint32_t lvl = is_drum(t) ? song.g[G_DRLVL] : t->p[P_LEVEL], k, dim = t->p[P_MUTE] ? 8u : 0u;
        const char *unit[4] = {"", "", "", t->p[P_MUTE] ? "MUTE" : ""};   /* muted: its real level, dim, "MUTE" (review 19) */
        for (k = 0; k < 2u; k++) {
            const param_desc_t *d;
            int16_t *vp = home_knob(k, &d);
            if (!vp) {                                  /* (a drum kit with no DEC to turn) */
                str_cpy(v[k], "--", 8);
                ratio[k] = 0;
                dim |= 1u << k;
                continue;
            }
            param_format(d, *vp, v[k], &u);
            if (d->fmt != F_PCT)                        /* (a time: its unit; a % as LEVEL: the number alone) */
                unit[k] = u;
            ratio[k] = (*vp - d->min) * 1000 / (d->max > d->min ? d->max - d->min : 1);
        }
        str_cpy(v[2], stl.act ? "ON" : "OFF", 8);
        ratio[2] = stl.act ? 1000 : 0;
        fmt_int(v[3], (int32_t)lvl * 100 / 127);
        ratio[3] = (int32_t)lvl * 1000 / 127;
        knob_tint_all(song.sel);                        /* HOME: the voice's knobs in its colour, the stutter everyone's */
        knob_tint[2] = TINT_NEUTRAL;
        op_knobs(lab, val, unit, ratio, dim, song.sel, &footer);
    }
    op_trackbar(&bar);
}

/* ---------------------------------------------------------------- DRUMS --- */
static const char *const LV_NAME[4] = {"norm", "ghost", "soft", "hard"};
static uint32_t lvl_rank(uint32_t lvl);                 /* (below) ghost 0, soft 1, norm 2, hard 3 */
/* the level order of the knobs: ghost < soft < norm < hard */
static const uint8_t LV_UP[4] = {LV_GHOST, LV_SOFT, LV_NORM, LV_HARD};
static uint16_t lvl_col(uint32_t lvl)                   /* a hit's colour by its level */
{
    return lvl == LV_GHOST ? TE_DIM[3] : lvl == LV_SOFT ? TE_MID[3] : lvl == LV_HARD ? C_WHITE : TE_DRUM;
}
static uint16_t pad_lit[DRUM_LANES];                   /* pads and key LEDs: frames left lit */
static uint8_t key_lit[27];                             /* menu NOTES, a synth track: frames its key stays lit */
static uint8_t key_lit_trk = 0xFF;                      /* (the track key_lit is for) */
static uint8_t op_note = 60, op_hit;                    /* the selected synth track: its last note, frames since (8 .. 0) */
static uint8_t op_strike[NTRK];                         /* each track's character: frames of its STRIKE pose still to show
                                                         * (an accent: pads_tick; ui_op.c op_dance) */
#define OP_STRIKE_FR 8u
#include "acid_gen.c"             /* SEQ > ACID GEN: TB-3PO's acid lines */

/* FX > CLOUD's SEND (G_CLSEND) is the selected track's send, on the drum track the selected sound's: the value on
 * the page follows the selection, and what the knob (or the editor) sets goes to that track / sound. Once a frame */
/* SEQ > GEN: ON shows and sets the selected synth track's (gen_on) and rolls a new loop at once; NEW rolls one; a
 * key played on the page (or an MPK key) turns it on with a new loop that starts on that note.
 * Turned on, and NEW, keep the pattern before for UNDO. Once a frame */
/* SEQ > PLAY: its four values are the selected track's (seq.c pl_*), shown and set here. Once a frame */
static void play_ui_sync(void)
{
    static uint8_t sel_was = 0xFF;
    uint32_t s = song.sel % NTRK;
    int16_t *g = song.g;
    if (s != sel_was) {
        sel_was = (uint8_t)s;
        g[G_PLDIR] = pl_dir[s], g[G_PLCHN] = pl_chance[s], g[G_PLJMP] = pl_jump[s], g[G_PLSTR] = pl_stride[s];
    }
    if (g[G_PLDIR] != pl_dir[s] && pl_dir[s] == PD_WALK)
        pl_pos[s] = (uint8_t)(trk[s].seq_idx % NSTEP);  /* (WALK starts where the track is) */
    pl_dir[s] = (uint8_t)clamp(g[G_PLDIR], 0, PD_COUNT - 1);
    pl_chance[s] = (uint8_t)clamp(g[G_PLCHN], 0, 127);
    pl_jump[s] = (uint8_t)clamp(g[G_PLJMP], 0, 127);
    pl_stride[s] = (uint8_t)clamp(g[G_PLSTR], 1, 7);
}
static void gen_ui_sync(void)
{
    static uint8_t sel_was = 0xFF;
    static int16_t on_was = -1;
    uint32_t s = song.sel, bit = s < NPART ? 1u << s : 0u;
    if (s != sel_was) {
        sel_was = (uint8_t)s;
        song.g[G_GNON] = (int16_t)((gen_on & bit) != 0u);
        on_was = song.g[G_GNON];
    }
    gen_keys = (uint8_t)((str_eq(cur_page()->title, "GEN") ? 1u : str_eq(cur_page()->title, "HARMONY") ? 2u : 0u) * !ui.menu);
    {   /* SEQ > HARMONY: KEY and MODE are track 1's ROOT and SCL (a project loaded shows its key); turned, they set the
         * three synth tracks and ACID GEN; any of the four turned: every GEN track's part again, the chords said;
         * BARS also sets their loops (GEN never changes LEN otherwise: 2026-10-27) */
        static int16_t kr = -1, km = -1, kp = -1, kb = -1;
        int16_t *g = song.g;
        int changed = 0;
        if (g[G_GKROOT] != kr || g[G_GKMODE] != km) {
            if (kr >= 0) {
                gen_key_set((uint32_t)g[G_GKROOT], (uint32_t)g[G_GKMODE]);
                changed = 1;
            }
        } else {
            g[G_GKROOT] = (int16_t)(trk[0].p[P_ROOT] % 12);
            g[G_GKMODE] = (int16_t)gk_mode_of(trk[0].p[P_SCALE]);
        }
        if (kp >= 0 && (g[G_GKPROG] != kp || g[G_GKBARS] != kb))
            changed = 1;
        if (kb >= 0 && g[G_GKBARS] != kb) {          /* BARS turned: every GEN track's loop that long, together */
            uint32_t i;
            for (i = 0; i < NPART; i++)
                if ((gen_on >> i) & 1u)
                    trk[i].p[P_SLEN] = (int16_t)(16 << clamp(g[G_GKBARS], 0, 2));
        }
        kr = g[G_GKROOT], km = g[G_GKMODE], kp = g[G_GKPROG], kb = g[G_GKBARS];
        gk_load();
        if (changed) {
            static const char *const RN[12] = {"I", "bII", "II", "bIII", "III", "IV", "bV", "V", "bVI", "VI", "bVII", "VII"};
            char m[40];
            uint32_t i, j = 0, q;
            undo_sess++;
            for (i = 0; i < NPART; i++)
                if ((gen_on >> i) & 1u)
                    undo_mark(&trk[i], undo_sess);
            gen_again();
            for (i = 0; i < gk.n && j < 24u; i++) {   /* the chords: "i bVII", minor in small letters */
                const char *r = RN[(uint32_t)(gk_note(gk.deg[i]) - gk.root + 12) % 12u];
                int minor = gk_note(gk.deg[i] + 2) - gk_note(gk.deg[i]) == 3;
                if (i)
                    m[j++] = ' ';
                for (q = 0; r[q] && j < 30u; q++)
                    m[j++] = (char)(minor && r[q] != 'b' ? r[q] - 'A' + 'a' : r[q]);
            }
            if (gen_on & 7u) {                       /* the tracks rewritten: GEN 1 2 3 */
                static const char T[] = " GEN";
                for (q = 0; T[q] && j < 28u; q++)
                    m[j++] = T[q];
                for (i = 0; i < NPART && j < 30u; i++)
                    if ((gen_on >> i) & 1u)
                        m[j++] = ' ', m[j++] = (char)('1' + i);
            }
            m[j] = 0;
            ui_message(m);
            ui.force = 1;
        }
    }
    if (gen_seed) {                                  /* a key on the GEN page: GEN on, a new loop from that note */
        uint32_t n = gen_seed - 1u;
        gen_seed = 0;
        if (bit && gen_keys == 1u) {                 /* GEN: this track's GEN on, a new part (and the KEY) */
            undo_sess++;
            undo_mark(TSEL, undo_sess);
            gen_on |= (uint8_t)bit;
            song.g[G_GNON] = on_was = 1;
            gen_new_from(TSEL, n);
            ui.force = 1;
        } else if (gen_keys == 2u) {                 /* HARMONY: the KEY only (GEN stays as it is: 2026-10-26) */
            song.g[G_GKROOT] = (int16_t)(n % 12u);
        }
    }
    if (song.g[G_GNON] != on_was) {
        on_was = song.g[G_GNON];
        if (!bit) {
            song.g[G_GNON] = on_was = 0;
        } else if (on_was) {
            undo_sess++;
            undo_mark(TSEL, undo_sess);
            gen_on |= (uint8_t)bit;
            gen_new(TSEL);                           /* (a new loop at once: DRIFT alone took seconds to be heard) */
            ui.force = 1;
        } else {
            gen_on &= (uint8_t)~bit;
        }
    }
    {                                                /* DRUM GEN: a knob turned, the lanes written now */
        static int16_t dg_was[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
        uint32_t i, ch = 0;
        for (i = 0; i < 8u; i++)
            if (song.g[G_DGMODE + i] != dg_was[i]) {
                ch |= 1u << i;
                dg_was[i] = song.g[G_DGMODE + i];
            }
        if (ch && song.g[G_DGMODE]) {
            if (ch & 1u) {                           /* MODE on (or MAP <-> EUCL): the pattern before kept for UNDO */
                undo_sess++;
                undo_mark(TDRUM, undo_sess);
                dg_roll_chaos();
            }
            drum_gen_fill(TDRUM);
            ui.force = 1;
        }
    }
    if (song.g[G_GNNEW]) {
        song.g[G_GNNEW] = 0;
        if (bit) {
            undo_sess++;
            undo_mark(TSEL, undo_sess);
            gen_new(TSEL);
            ui.force = 1;
        }
    }
}
static void cloud_ui_sync(void)
{
    static uint16_t was = 0xFFFF;
    uint16_t now = (uint16_t)(song.sel == TRK_DRUM ? 0x100u | (drum_lane % DRUM_LANES) : song.sel % NPART);
    uint8_t *v = now & 0x100u ? &drum_cloud[now & 0xFFu] : &cloud_send[now];
    if (now != was || cloud_resync) {
        song.g[G_CLSEND] = *v;
        was = now;
        cloud_resync = 0;
    } else {
        *v = (uint8_t)clamp(song.g[G_CLSEND], 0, 127);
    }
}
/* the drum track's EDIT pages (HEX OSC .. HEX MORE): G_HXMODEL.. show the selected sound (drum_lane) of the
 * drum track's kit, if it is a HEX kit, and what the knobs set goes into it. A factory kit (HEX BOOM ..) is not
 * changed: the first turn copies it into the first MY HEX kit not edited yet and the drum track takes that one
 * (all four edited: a message, nothing changed). An edit is stored with the settings when all is quiet
 * (project.c). On these pages a key played picks its sound, as on KIT. Once a frame */
static uint8_t hex_resync;
static uint32_t hex_gen;                                /* (the preview, ui_op.c: redrawn when it moves) */
static void hex_ui_sync(void)
{
    static uint16_t was_sel = 0xFFFF;
    static int16_t was[22];
    uint32_t kit = drum_kit(), lane, i, k, on_page = cur_page()->graph == GR_HEX && !ui.home && song.sel == TRK_DRUM;
    int changed = 0;
    const hex_snd_t *src;
    hex_snd_t *dst;
    if (on_page)
        for (k = 0; k < 27u; k++)
            if ((fm1_in.notes >> k) & 1u) {
                drum_lane = (uint8_t)lane_of_key(k);
                break;
            }
    {                                                   /* the track's lane -> the kit's sound (PEDAL: the HAT's) */
        int32_t semi;
        lane = ds_lane(LANE_NOTE[drum_lane % DRUM_LANES], &semi) % DS_LANES;
    }
    if (kit >= DRUM_SAMPLED && kit < DRUM_SYN)
        return;                                         /* not a HEX kit: nothing to show */
    src = kit < DRUM_SAMPLED ? &HEX_KITS[kit].s[lane] : &dsu_kit(kit - DRUM_SYN)->s[lane];
    if ((uint16_t)(kit << 4 | lane) != was_sel || hex_resync) {
        for (i = 0; i < 22u; i++)
            song.g[G_HXMODEL + i] = was[i] = i == 3u ? (int16_t)(int8_t)((const uint8_t *)src)[i] : ((const uint8_t *)src)[i];
        was_sel = (uint16_t)(kit << 4 | lane);
        hex_resync = 0;
        hex_gen++;
        return;
    }
    for (i = 0; i < 22u; i++)
        changed |= song.g[G_HXMODEL + i] != was[i];
    if (!changed)
        return;
    if (kit < DRUM_SAMPLED) {                           /* a factory kit: into a MY HEX kit */
        char m[20] = "MY HEX ";
        for (k = 0; k < DSU_N && dsu_kit(k)->rsv[0]; k++)
            ;
        if (k == DSU_N) {
            ui_message("4 MY HEX USED");
            hex_resync = 1;
            return;
        }
        dsu_from_factory(k, kit);
        dsu.k[k].rsv[0] = 1;
        TDRUM->p[P_E0] = (int16_t)(DRUM_SYN + k);
        kit = DRUM_SYN + k;
        m[7] = (char)('1' + k);
        m[8] = 0;
        ui_message(m);
    }
    dst = &dsu.k[kit - DRUM_SYN].s[lane];
    for (i = 0; i < 22u; i++)
        ((uint8_t *)dst)[i] = (uint8_t)song.g[G_HXMODEL + i];
    dsu_fix_sound(dst);                                 /* (a value the model has no use for: back in its range) */
    dsu.k[kit - DRUM_SYN].rsv[0] = 1;
    dsu_dirty = 1;
    for (i = 0; i < 22u; i++)
        song.g[G_HXMODEL + i] = was[i] = i == 3u ? (int16_t)(int8_t)((const uint8_t *)dst)[i] : ((const uint8_t *)dst)[i];
    was_sel = (uint16_t)(kit << 4 | lane);
    hex_gen++;
}
static void pads_tick(void)                             /* once a frame: the hits since the last one */
{
    uint32_t i, hits, n[NPART][4], k, w;
    fm1_irq_off();
    hits = drums.hits;
    drums.hits = 0;
    for (i = 0; i < NPART; i++)
        for (w = 0; w < 4u; w++) {
            n[i][w] = note_hits[i][w];
            note_hits[i][w] = 0;
        }
    fm1_irq_on();
    cloud_ui_sync();
    acid_gen_sync();
    gen_ui_sync();
    play_ui_sync();
    hex_ui_sync();
    for (i = 0; i < DRUM_LANES; i++) {
        if ((hits >> i) & 1u) pad_lit[i] = 6;
        else if (pad_lit[i]) pad_lit[i]--;
    }
    /* a synth track: each note it started lights its key 6 frames (~0.1 s), so a short sequencer note
     * (a 1/16 at GATE 50 %: 60..80 ms) is seen even when it ends between two frames */
    if (song.sel != key_lit_trk) {
        memset(key_lit, 0, sizeof key_lit);
        key_lit_trk = song.sel;
    }
    for (k = 0; k < 27u; k++)
        if (key_lit[k])
            key_lit[k]--;
    if (op_hit)
        op_hit--;
    if (song.sel < NPART)                               /* the character of the track jumps on its notes (ui_op.c) */
        for (k = 0; k < 128u; k++)
            if ((n[song.sel][k >> 5] >> (k & 31u)) & 1u) {
                op_note = (uint8_t)k;
                op_hit = 8;
            }
    {   /* an accent, a note or a hit started at LV_HARD's velocity (116 and up: an ACC step, a HARD hit, a key
         * played hard): its track's character strikes a pose for a few frames. Read from the voices the notes
         * started since the last frame (their ages), nothing in the sound is changed */
        static uint32_t seen_v, seen_d;
        uint32_t top_v = seen_v, top_d = seen_d, j;
        for (i = 0; i < NTRK; i++)
            if (op_strike[i])
                op_strike[i]--;
        {   /* the sequencer's ACC steps (seq.c seq_acc): UNISON, a legato ACC + SLIDE too */
            uint32_t a;
            fm1_irq_off();
            a = seq_acc;
            seq_acc = 0;
            fm1_irq_on();
            for (i = 0; i < NPART; i++)
                if ((a >> i) & 1u)
                    op_strike[i] = OP_STRIKE_FR;
        }
        for (i = 0; i < NPART; i++)
            for (j = 0; j < NVOICE; j++) {
                const voice_t *v = &trk[i].v[j];
                uint32_t age = v->age;
                if (v->active && (int32_t)(age - seen_v) > 0) {
                    if (vel_lvl(v->vel) == LV_HARD)
                        op_strike[i] = OP_STRIKE_FR;
                    if ((int32_t)(age - top_v) > 0)
                        top_v = age;
                }
            }
        for (j = 0; j < NDRUM; j++) {
            const voice_t *v = &drums.v[j];
            uint32_t age = v->age;
            if (v->active && (int32_t)(age - seen_d) > 0) {
                if (vel_lvl(v->vel) == LV_HARD)
                    op_strike[TRK_DRUM] = OP_STRIKE_FR;
                if ((int32_t)(age - top_d) > 0)
                    top_d = age;
            }
        }
        seen_v = top_v;
        seen_d = top_d;
    }
    if (song.sel < NPART && (n[song.sel][0] | n[song.sel][1] | n[song.sel][2] | n[song.sel][3]))
        for (k = 0; k < 27u; k++) {
            uint32_t note = kb_map(TSEL, k);
            if (note < 128u && (n[song.sel][note >> 5] >> (note & 31u)) & 1u)
                key_lit[k] = 6;
        }
}

#include "ui_grid.c"               /* the step GRID on every track (Elektron style) */
#include "ui_stut.c"               /* (PHEBUS) the GRID's STUTTER lane, after DRUM */

/* the drum GRID's full view (SELECT right; left: the snake again. REVIEW.md 5, 21 Nov 2026): a row a sound, 16 steps
 * of the page across. The sounds the pattern uses and the one KNOB 1 picks (white), up to 7, the picked one always in
 * view; a hit in its level's colour, the cursor framed white, the step playing underlined, the four page slots */
static uint8_t drum_full;
static uint32_t dg_rows(uint8_t *row)
{
    uint32_t used = 1u << drum_lane, i, l, n = 0, sel = 0, len = trk_len(TDRUM), st;
    for (i = 0; i < len; i++)
        used |= dstep_mask(&TDRUM->dstep[i]);
    for (l = 0; l < DRUM_LANES; l++)
        if ((used >> l) & 1u) {
            if (l == drum_lane)
                sel = n;
            row[n++] = (uint8_t)l;
        }
    if (n <= 7u)
        return n;
    st = sel > 3u ? sel - 3u : 0u;
    st = st > n - 7u ? n - 7u : st;
    for (i = 0; i < 7u; i++)
        row[i] = row[st + i];
    return 7;
}
/* rows r0..r1-1 of row[] in a band of h at y (16 a row); slots: the page slots under them */
static void dg_band(uint32_t y, uint32_t h, const uint8_t *row, uint32_t r0, uint32_t r1, uint32_t bank, uint32_t len, int slots)
{
    uint32_t r, j, pl = song.playing ? TDRUM->seq_idx % len : 0xFFFFu;
    cv_begin(240, h, C_BLACK);
    for (r = r0; r < r1; r++) {
        int32_t yy = (int32_t)(r - r0) * 16;
        uint32_t l = row[r];
        cv_text(2, yy, &FONT_S, LANE_SHORT[l], l == drum_lane ? C_WHITE : TE_G3);
        for (j = 0; j < 16u; j++) {
            uint32_t p = bank * 16u + j;
            const dstep_t *s = &TDRUM->dstep[p % NSTEP];
            int32_t x = 52 + (int32_t)j * 11 + (int32_t)(j / 4u);   /* (room for the 6-letter names, review 21) */
            if (p >= len)
                continue;
            if (dstep_has(s, l))
                cv_rect(x, yy + 1, 10, 13, lvl_col(dstep_lvl(s, l)));
            else
                op_box(x, yy + 1, 10, 13, j % 4u ? TE_G2 : TE_G3);
            if (l == drum_lane && p == drum_cursor)
                op_box(x - 1, yy, 12, 15, C_WHITE);
            if (p == pl)
                cv_rect(x, yy + 14, 10, 2, C_WHITE);
        }
    }
    for (j = 0; j < 4u && slots; j++) {
        int32_t x = 52 + (int32_t)j * 45;
        if (j * 16u >= len)
            op_box(x, 26, 42, 5, TE_G2);
        else
            cv_rect(x, 26, 42, 5, j == bank ? TE_DRUM : TE_G2);
    }
    cv_blit(0, y);
}

static void drum_screen_draw(void)
{
    static uint32_t head, scene_sig, steps_sig, body_sig, footer, bar;
    uint32_t i, len = (uint32_t)clamp(TDRUM->p[P_SLEN], 1, 64), kit = drum_kit(), sig, bank, pat = 0, lit = 0;
    if (song.sel != TRK_DRUM || stl_shown()) {          /* a synth track: its GRID (ui_grid.c); the STUTTER lane */
        if (song.sel != TRK_DRUM)
            sg_draw();
        else
            stl_draw();
        head = scene_sig = steps_sig = body_sig = footer = 0;
        return;
    }
    if (drum_cursor >= len) drum_cursor = (uint8_t)(len - 1u);
    bank = drum_cursor / 16u;
    {
        char nm[9];
        const char *name = DRUM_KIT_NAMES[kit];
        if (kit >= DRUM_SYN) {                          /* SYN1..4: the name you gave it (editor DRUM SYNTH) */
            memcpy(nm, dsu_kit(kit - DRUM_SYN)->name, 8);
            nm[8] = 0;
            if (nm[0])
                name = nm;
        }
        op_header("DRUM", TE_DRUM, name, &head);
    }
    for (i = 0; i < DRUM_LANES; i++) lit = lit * 2u + (pad_lit[i] != 0);
    for (i = 0; i < len; i++) {
        const dstep_t *s = &TDRUM->dstep[i];
        pat = pat * 31u + dstep_mask(s);
        pat = pat * 31u + s->lvl[0] + s->lvl[1] * 7u + s->lvl[2] * 49u + s->lvl[3] * 343u;
        pat = pat * 31u + s->rat[0] + s->rat[1] * 7u + s->rat[2] * 49u + s->rat[3] * 343u;
    }
    sig = pat * 31u + drum_lane * 977u + drum_cursor * 101u + len * 3u + (song.playing ? TDRUM->seq_idx + 1u : 0u) * 71u;
    if (!drum_page) {                                  /* GRID: the snake dancing, then the 16 steps (ui_op.c) */
        if (sg_held) {                                 /* a step held: all it holds, where the snake was */
            uint32_t ls = 0;
            for (i = 0; i < NLOCK; i++)
                ls = ls * 31u + TDRUM->lock[i].step + TDRUM->lock[i].param * 257u + (uint32_t)TDRUM->lock[i].val * 65537u;
            ls = ls * 31u + sig + sg_held + (uint32_t)TDRUM->micro[drum_cursor] * 7u + step_fill(TDRUM, drum_cursor) * 3u;
            if (ui.force || ls * 131u + 1u != scene_sig) {
                scene_sig = ls * 131u + 1u;
                sg_detail_draw(SG_BODY_Y + 36u, SG_SCENE_H, (uint32_t)sg_first(), 1);   /* (the steps above it) */
            }
        } else if (drum_full) {                        /* the full view: every sound in use, a row each */
            if (ui.force || sig * 131u + 2u != scene_sig) {
                uint8_t row[DRUM_LANES];
                uint32_t n = dg_rows(row);
                scene_sig = sig * 131u + 2u;
                dg_band(SG_BODY_Y, SG_SCENE_H, row, 0, n < 6u ? n : 6u, bank, len, 0);
                dg_band(SG_BODY_Y + SG_SCENE_H, SG_KNOB_Y - SG_BODY_Y - SG_SCENE_H, row, 6, n, bank, len, 1);
            }
            steps_sig = sig + 1u;                      /* (the steps strip: drawn again when it comes back) */
        } else if (ui.force || sig * 131u + lit + (op_strike[TRK_DRUM] ? 977u : 0u) != scene_sig) {   /* (the snake's STRIKE) */
            scene_sig = sig * 131u + lit + (op_strike[TRK_DRUM] ? 977u : 0u);
            op_drum_scene(bank, len);
        }
        if ((!drum_full || sg_held) && (ui.force || sig + (sg_held != 0) * 7u != steps_sig)) {
            steps_sig = sig + (sg_held != 0) * 7u;
            op_steps_y = (uint8_t)(sg_held ? SG_BODY_Y : SG_BODY_Y + SG_SCENE_H);
            op_drum_steps(bank, len);
        }
        body_sig = 0;
    } else {                                           /* KIT: 16 pads, lit on each hit */
        if (ui.force) {
            lcd_fill(0, 22, 240, 16, C_BLACK);
            lcd_fill(0, 138, 240, 16, C_BLACK);
        }
        for (i = 0; i < DRUM_LANES; i++)
            lit = lit * 131u + drum_send[i][0] + drum_send[i][1] * 16u;
        if (ui.force || lit * 31u + drum_lane + 1u != body_sig) {
            body_sig = lit * 31u + drum_lane + 1u;
            cv_begin(240, 100, C_BLACK);
            for (i = 0; i < DRUM_LANES; i++) {             /* under each name its own sends: reverb, delay */
                int32_t x = 2 + (int32_t)(i % 4u) * 60, y = (int32_t)(i / 4u) * 25;
                cv_rect(x, y, 56, 23, pad_lit[i] ? TE_DRUM : TE_G1);
                if (i == drum_lane)
                    op_box(x, y, 56, 23, C_WHITE);
                te_text_c(x + 28, y + 2, LANE_SHORT[i], pad_lit[i] ? C_BLACK : i == drum_lane ? C_WHITE : TE_G3);
                cv_rect(x + 3, y + 19, drum_send[i][0] * 24 / DRUM_SEND_REV, 2, pad_lit[i] ? C_BLACK : TE_COL[3]);
                cv_rect(x + 30, y + 19, drum_send[i][1] * 24 / DRUM_SEND_DLY, 2, pad_lit[i] ? C_BLACK : TE_COL[3]);
            }
            cv_blit(0, 38);
        }
        scene_sig = steps_sig = 0;
    }
    {
        static char v[4][12];
        const char *val[4] = {v[0], v[1], v[2], v[3]};
        static const char *const LG[4] = {"sound", "step", "hit", "level"}, *const LK[4] = {"sound", "kit", "reverb", "delay"};
        int32_t ratio[4];
        if (!drum_page && (sg_held || sg_lpage < NPAGES)) {
            sg_knobs_held((uint32_t)(sg_held ? sg_first() : (int32_t)drum_cursor), &footer);
        } else if (!drum_page) {
            const dstep_t *s = &TDRUM->dstep[drum_cursor];
            str_cpy(v[0], LANE_SHORT[drum_lane], 8);
            fmt_int(v[1], drum_cursor + 1);
            str_cpy(v[2], dstep_has(s, drum_lane) ? "on" : "--", 4);
            str_cpy(v[3], dstep_has(s, drum_lane) ? LV_NAME[dstep_lvl(s, drum_lane)] : "--", 8);
            ratio[0] = (int32_t)drum_lane * 1000 / (DRUM_LANES - 1);
            ratio[1] = (int32_t)drum_cursor * 1000 / (int32_t)(len > 1u ? len - 1u : 1u);
            ratio[2] = v[2][0] == 'o' ? 1000 : 0;
            ratio[3] = dstep_has(s, drum_lane) ? (int32_t)lvl_rank(dstep_lvl(s, drum_lane)) * 333 : 0;   /* ghost left ..
                                                                     * hard right (2.5: it followed the stored order) */
            knob_tint_all(TRK_DRUM);
            op_knobs(LG, val, 0, ratio, 0, 1u, &footer);
        } else {
            str_cpy(v[0], LANE_SHORT[drum_lane], 8);   /* KIT: the sound, the kit, the sound's own reverb and delay */
            str_cpy(v[1], DRUM_KIT_NAMES[kit], sizeof v[1]);   /* (the kit by name, as the header: review 21) */
            fmt_int(v[2], drum_send[drum_lane][0] * 100 / DRUM_SEND_REV);
            fmt_int(v[3], drum_send[drum_lane][1] * 100 / DRUM_SEND_DLY);
            ratio[0] = (int32_t)drum_lane * 1000 / (DRUM_LANES - 1);
            ratio[1] = (int32_t)kit * 1000 / (int32_t)(DRUM_KITS - 1u);
            ratio[2] = drum_send[drum_lane][0] * 1000 / DRUM_SEND_REV;
            ratio[3] = drum_send[drum_lane][1] * 1000 / DRUM_SEND_DLY;
            knob_tint_all(TRK_DRUM);
            op_knobs(LK, val, 0, ratio, 0, 2u, &footer);
        }
    }
    op_trackbar(&bar);
}

static uint32_t lvl_rank(uint32_t lvl) { return lvl == LV_GHOST ? 0u : lvl == LV_SOFT ? 1u : lvl == LV_NORM ? 2u : 3u; }

/* DRUMS: SEQ -> GRID -> KIT -> SEQ > DRUM GEN (SEQ there: back to the GRID) */
static void drum_seq_tap(void)
{
    uint32_t i;
    if (!drum_page) {
        drum_page = 1;
        ui.msg_t = 0;
        ui.force = 1;
        return;
    }
    drum_page = 0;
    for (i = 0; i < NPAGES; i++)
        if (str_eq(PAGES[i].title, "DRUM GEN")) {
            ui.page = (uint8_t)i;
            ui.fam_last[FAM_SEQ] = (uint8_t)i;
            ui.home = 0;
            page_entered();
            ui.force = 1;
        }
}

static void drum_screen_input(uint32_t pressed, uint32_t home)
{
    uint32_t k, b;
    int32_t s;
    if (home == 1u) {
        go_home();
        return;
    }
    for (k = 0; k < NB; k++) if ((pressed >> panel.btn[k]) & 1u) {
        b = k;
        if (b == B_PLAY) {
            if (ft_owns_press()) ;
            else if (!song.playing && arrangement_enabled && !arr_valid(&arrangement, arrangement_ready())) ui_message("EMPTY SECTION: REC");
            else transport_req = song.playing ? 2 : 1;
        } else if (b == B_SAVE) {
            studio_open(SC_SONG);
            return;
        }
    }
    if (!sg_held && !stl_held && (s = panel_enc(EN_ALGO)) && !ft_on && !stl_algo(s))   /* the track: the GRID stays, on
                                                         * the track chosen (after DRUM: the STUTTER lane, ui_stut.c) */
        track_select((uint32_t)clamp((int32_t)song.sel + s, 0, 3));
    if (stl_shown()) {
        stl_input(pressed);
        return;
    }
    sg_follow();                                        /* (playing: the page the playhead goes onto) */
    if (song.sel != TRK_DRUM) {                         /* a synth track's GRID (ui_grid.c) */
        sg_synth_input(pressed);
        return;
    }
    if (!drum_page && sg_input(pressed))                /* (a step held: its knobs; OCT- / OCT+, ENV, LFO) */
        return;
    if ((s = panel_enc(EN_SELECT)) != 0) {              /* SELECT: GRID: right the full view, left the snake (REVIEW.md
                                                         * 5); KIT: left back to the GRID. (SEQ: GRID -> KIT) */
        if (drum_page ? s < 0 : (uint32_t)(s > 0) != drum_full) {
            if (drum_page)
                drum_page = 0;
            else
                drum_full = (uint8_t)(s > 0);
            ui.msg_t = 0;
            ui.force = 1;
        }
    }
    if ((s = panel_enc(EN_PRESET))) TDRUM->p[P_E0] = (int16_t)clamp(TDRUM->p[P_E0] + s, 0, DRUM_KITS - 1);
    if (drum_page)                                      /* KIT: a key played picks its sound (KNOB 3, 4: its sends) */
        for (k = 0; k < 27u; k++)
            if ((fm1_in.notes >> k) & 1u) {
                drum_lane = (uint8_t)lane_of_key(k);
                break;
            }
    for (k = 0; k < 4u; k++) if ((s = panel_enc(EN_K1 + k))) {
        ui.hot_col = (uint8_t)k;
        ui.hot_t = 40;
        if (!drum_page) {
            dstep_t *st = &TDRUM->dstep[drum_cursor];
            if (k == 0) {                              /* the sound: heard */
                uint8_t l = (uint8_t)clamp(drum_lane + s, 0, DRUM_LANES - 1);
                if (l != drum_lane) audition_lane(l);
                drum_lane = l;
            }
            if (k == 1) {                              /* the step: what it holds, heard */
                uint8_t c = (uint8_t)clamp(drum_cursor + s, 0, TDRUM->p[P_SLEN] - 1);
                if (c != drum_cursor) audition_step(&TDRUM->dstep[c]);
                drum_cursor = c;
            }
            if (k >= 2) {
                if (song.playing && arrangement_enabled) { ui_message("STOP FIRST"); continue; }
                undo_mark(TDRUM, ui.step_sess ? ui.step_sess : (ui.step_sess = (undo_sess += 4u) | 3u));
                fm1_irq_off();
                if (k == 2) {
                    if (s > 0) dstep_set(st, drum_lane, LV_NORM, 0);
                    else dstep_clr(st, drum_lane);
                } else if (dstep_has(st, drum_lane)) {
                    uint32_t r = (uint32_t)clamp((int32_t)lvl_rank(dstep_lvl(st, drum_lane)) + (s > 0 ? 1 : -1), 0, 3);
                    dstep_set(st, drum_lane, LV_UP[r], dstep_rat(st, drum_lane));
                }
                fm1_irq_on();
                sync_reload = 1;
            }
        } else {
            if (k == 0) {                              /* the sound: heard */
                uint8_t l = (uint8_t)clamp(drum_lane + s, 0, DRUM_LANES - 1);
                if (l != drum_lane) audition_lane(l);
                drum_lane = l;
            }
            if (k == 1) TDRUM->p[P_E0] = (int16_t)clamp(TDRUM->p[P_E0] + s, 0, DRUM_KITS - 1);
            if (k == 2) drum_send[drum_lane][0] = (uint8_t)clamp(drum_send[drum_lane][0] + s, 0, DRUM_SEND_REV);
            if (k == 3) drum_send[drum_lane][1] = (uint8_t)clamp(drum_send[drum_lane][1] + s, 0, DRUM_SEND_DLY);
        }
    }
}

/* ---------------------------------------------------------------- REC --- */
/* a big seven-segment digit, TE style: w x h, segments t thick */
static void te_digit(int32_t x, int32_t y, int32_t w, int32_t h, int32_t t, uint32_t d, uint16_t c)
{
    static const uint8_t SEG[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};   /* gfedcba */
    uint32_t m = SEG[d % 10u];
    int32_t hh = h / 2;
    if (d % 10u == 1u) {                                /* a real 1 (review 14): a centred stem, a two-step flag, a foot */
        int32_t sx = x + (w - t) / 2;
        cv_rect(sx, y, t, h, c);
        cv_rect(sx - t, y + t, t, t, c);
        cv_rect(sx - 2 * t, y + 2 * t, t, t, c);
        cv_rect(x + w / 6, y + h - t, w - w / 3, t, c);
        return;
    }
    if (m & 0x01) cv_rect(x + t, y, w - 2 * t, t, c);                       /* a */
    if (m & 0x02) cv_rect(x + w - t, y + t, t, hh - t, c);                  /* b */
    if (m & 0x04) cv_rect(x + w - t, y + hh, t, hh - t, c);                 /* c */
    if (m & 0x08) cv_rect(x + t, y + h - t, w - 2 * t, t, c);               /* d */
    if (m & 0x10) cv_rect(x, y + hh, t, hh - t, c);                         /* e */
    if (m & 0x20) cv_rect(x, y + t, t, hh - t, c);                          /* f */
    if (m & 0x40) cv_rect(x + t, y + hh - t / 2, w - 2 * t, t, c);          /* g */
}

static uint8_t rec_shown;
/* the four tracks, compact, from y0: the one that records is framed in red */
static void rec_rows(uint32_t rt, uint32_t take, uint32_t y0, uint32_t *cache)
{
    uint32_t sig = rt * 3u + rec_wait + take * 5u + drum_kit() * 977u + y0, i, j;
    for (i = 0; i < NTRK; i++) {
        char b[16];
        if (i == TRK_DRUM) str_cpy(b, DRUM_KIT_NAMES[drum_kit()], sizeof b);
        else trk_short_name(i, b);
        sig = studio_hash(sig, b) + (uint32_t)trk[i].p[P_SLEN] * 31u;
        for (j = 0; j < NSTEP; j++) sig = sig * 3u + (uint32_t)trk_step_on(&trk[i], j);
    }
    if (!ui.force && sig == *cache)
        return;
    *cache = sig;
    cv_begin(240, 76, C_BLACK);
    for (i = 0; i < NTRK; i++) {
        const track_t *t = &trk[i];
        uint32_t len = (uint32_t)clamp(t->p[P_SLEN], 1, 64), armed = i == rt;
        int32_t y = (int32_t)i * 19;
        char b[16];
        if (armed) cv_rect(0, y, 240, 18, TE_RED), cv_rect(1, y + 1, 238, 16, C_BLACK);
        cv_rect(4, y + 3, 12, 12, armed ? TE_COL[i] : TE_DIM[i]);
        if (i == TRK_DRUM) str_cpy(b, DRUM_KIT_NAMES[drum_kit()], sizeof b);
        else trk_short_name(i, b);
        b[10] = 0;
        cv_text(22, y + 1, &FONT_S, b, armed ? C_WHITE : TE_G3);
        for (j = 0; j < 16u; j++) {
            uint32_t a = j * len / 16u, z = (j + 1u) * len / 16u, k, on = 0;
            if (z == a) z = a + 1u;
            for (k = a; k < z && k < NSTEP; k++) if (trk_step_on(t, k)) on = 1;
            cv_rect(106 + (int32_t)j * 8, y + 5, 6, 8, on ? (armed ? TE_COL[i] : TE_DIM[i]) : TE_G1);
        }
    }
    cv_blit(0, y0);
}

/* REC armed (stopped: waiting for the first note, or PLAY), its count-in, or a free take running
 * (seq.c). Armed, the dials say how it records: KNOB 1 MODE free / tempo (an empty project),
 * KNOB 2 LENGTH (1, 2 or 4 bars), KNOB 3 START note / count (ui_input.c rec_knobs). The four tracks
 * stay in view; the REC LED blinks while armed, is lit during the take. */
static void rec_screen_draw(void)
{
    static uint32_t head, body, rows, foot, dials;
    static uint8_t layout;
    uint32_t sig, take = ft_on, empty = take || project_empty(), rt = take ? ft_trk % NTRK : song.sel;
    uint32_t blink = (fm1_ms / 250u) & 1u, secs = take ? ft_t * CTL / FS : 0u, bars = 0, bpm = 0;
    uint32_t lay = take ? 1u : 2u, free = empty && !rec_tempo && !take, count = ci_on;
    if (take)
        bars = ft_fit(ft_t, &bpm);
    if (!rec_shown || lay != layout) {
        lcd_fill(0, 0, 240, 240, C_BLACK);
        ui.force = 1;
    }
    rec_shown = 1;
    layout = (uint8_t)lay;
    te_header(take ? "free take" : count ? "count-in" : "rec ready", TE_RED, &head);
    if (take) {                                         /* free take: the time, the loop it makes */
        sig = 1000003u + rt * 7919u + blink * 31u + secs * 131u + bars * 17u + bpm * 3u;
        if (ui.force || sig != body) {
            char b[24];
            uint32_t n = secs > 99u ? 99u : secs;
            body = sig;
            cv_begin(240, 106, C_BLACK);
            te_disc(18, 30, 9, blink ? TE_RED : TE_G2);
            if (n >= 10u)
                te_digit(36, 4, 32, 56, 7, n / 10u, C_WHITE);
            te_digit(76, 4, 32, 56, 7, n % 10u, C_WHITE);
            cv_text(112, 44, &FONT_S, "s", TE_G3);
            cv_rect(132, 6, 1, 54, TE_G1);
            if (bars) {
                fmt_int(b, (int32_t)bars);
                cv_text(146, 4, &FONT_L, b, TE_COL[rt & 3u]);
                cv_text(146 + text_w(&FONT_L, b) + 6, 18, &FONT_S, bars == 1u ? "bar" : "bars", TE_G3);
                fmt_int(b, (int32_t)bpm);
                cv_text(146, 42, &FONT_S, b, C_WHITE);
                cv_text(146 + text_w(&FONT_S, b) + 4, 42, &FONT_S, "bpm", TE_G3);
            } else {
                cv_text(146, 4, &FONT_L, "-", TE_G3);
            }
            te_text_c(120, 80, "press rec on the 1", TE_RED);
            cv_blit(0, 42);
        }
        rec_rows(rt, take, 148u, &rows);
        if (ui.force || foot != 1u) {
            foot = 1u;
            cv_begin(240, 16, C_BLACK);
            cv_text(4, 0, &FONT_S, "rec: close", TE_G3);
            cv_text(236 - text_w(&FONT_S, "play: drop"), 0, &FONT_S, "play: drop", TE_G3);
            cv_blit(0, 224);
        }
        return;
    }
    foot = 0;
    /* armed / counting in: what happens next */
    sig = 2000003u + empty * 7u + free * 11u + rec_count * 13u + count * 17u + ci_beat * 19u + blink * 31u;
    if (ui.force || sig != body) {
        static const char *const L1[3] = {"play freely", "play a note", "press play"};
        static const char *const L2[3] = {"then rec on the 1", "it starts the loop", "4 clicks, then rec"};
        static const char *const L3[3] = {"the tempo follows you", "play: go  rec: cancel", "rec: cancel"};
        uint32_t m = free ? 0u : rec_count ? 2u : 1u;
        body = sig;
        cv_begin(240, 64, C_BLACK);
        if (count) {                                    /* the count-in: 4, 3, 2, 1 */
            te_digit(96, 4, 30, 54, 6, 4u - (ci_beat > 3u ? 3u : ci_beat), C_WHITE);
            te_disc(60, 31, 12, TE_RED);
            cv_text(144, 14, &FONT_S, "count-in", TE_G4);
            cv_text(144, 34, &FONT_S, "rec: cancel", TE_G3);
        } else {
            te_disc(28, 30, 18, blink ? TE_RED : TE_G2);
            te_disc(28, 30, 7, C_BLACK);
            cv_text(60, 6, &FONT_S, L1[m], C_WHITE);
            cv_text(60, 24, &FONT_S, L2[m], TE_G4);
            cv_text(60, 42, &FONT_S, L3[m], TE_G3);
        }
        cv_blit(0, 42);
    }
    rec_rows(rt, 0u, 106u, &rows);
    {   /* the dials: how it records */
        static char v[3][10];
        static const char *const LAB_E[4] = {"mode", "steps", "start", ""};   /* (STEPS: the pattern's length, review 21) */
        static const char *const LAB_F[4] = {"mode", "", "", ""};
        static const char *const LAB_N[4] = {"", "steps", "start", ""};
        static const char *const LAB_0[4] = {"", "", "", ""};
        const char *val[4] = {v[0], v[1], v[2], ""};
        int32_t ratio[4] = {0, 0, 0, 0};
        uint32_t len = (uint32_t)clamp(TSEL->p[P_SLEN], 1, 64);
        str_cpy(v[0], rec_tempo ? "tempo" : "free", sizeof v[0]);
        if (len % 16u == 0u) {
            fmt_int(v[1], (int32_t)(len / 16u));
            str_cpy(v[1] + str_len(v[1]), len == 16u ? " bar" : " bars", 6);
        } else {
            fmt_int(v[1], (int32_t)len);
            str_cpy(v[1] + str_len(v[1]), " st", 4);
        }
        str_cpy(v[2], rec_count ? "count" : "note", sizeof v[2]);
        ratio[0] = rec_tempo ? 1000 : 0;
        ratio[1] = (int32_t)(len - 1u) * 1000 / 63;
        ratio[2] = rec_count ? 1000 : 0;
        {
            const char *const *lab = count ? LAB_0 : !empty ? LAB_N : free ? LAB_F : LAB_E;
            uint32_t k;
            for (k = 0; k < 4u; k++)
                if (!lab[k][0])
                    val[k] = "";                        /* (a dial not shown: no value either) */
            knob_tint_all(song.sel);
            te_dials(184, lab, val, ratio, rt * 7u + count * 3u, &dials);
        }
    }
}
