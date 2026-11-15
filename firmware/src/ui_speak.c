/* SPDX-License-Identifier: GPL-3.0-only */
/* The screen reader (KARMA 3.1, GitHub issue #2): what the screen shows, in words, for Charles Vestal's FM-1
 * Screen Reader (github.com/charlesvestal/fm1-reader: a page that speaks it, or hands it to VoiceOver / NVDA).
 * announce.c keeps the protocol and sends nothing until a reader says HELLO. This says what changed, once a
 * UI frame, from what the screen just drew: the page (its name, the track, the sound and the four knobs; the
 * knobs as draw_columns left them in opk), a knob turning, the sound changing, play / stop, the octave. The
 * screen's own short words ("CUT", "ENG") are put into words by the reader with KARMA's words file
 * (firmware/words.tsv, published with the site). Messages (ui_say) are sent where they are shown. */

static char sp_buf[ANN_MAX + 1];                      /* the sentence being built (not on the stack) */
static struct {
    uint32_t ctx, snd;                                 /* the page and the sound last said */
    char kv[4][24];                                    /* each knob as last said: "CUT 13%" */
    uint8_t started, playing, cols;
    int8_t oct;
    int16_t bpm;
    uint8_t mix[NTRK];                                 /* (3.1.3) GLO's MIX: each track's mute | solo << 1, and its level */
    uint8_t mixlv[NTRK];
    uint8_t fbar;
    uint32_t sng;                                      /* the SONG screen's entry, the saved song selected */
} sp;

static void sp_cat(const char *s)
{
    uint32_t n = str_len(sp_buf);
    str_cpy(sp_buf + n, s, sizeof sp_buf - n);
}
static void sp_sep(void)                               /* ", " between items, "" at the start */
{
    uint32_t n = str_len(sp_buf);
    if (n && sp_buf[n - 1u] != ' ')
        sp_cat(n && sp_buf[n - 1u] != '.' ? ", " : " ");
}

/* knob c as the screen shows it: "CUT 13%", "TRACK 1 of 4" ("/4" would be read as a quarter); "" if blank */
static void sp_knob(uint32_t c, char *d, uint32_t cap)
{
    const char *l = opk.lab[c], *v = opk.val[c], *u = opk.unit[c];
    d[0] = 0;
    if (!l[0] || !v[0])
        return;
    str_cpy(d, l, cap);
    if (str_eq(v, "--"))
        return;                                         /* a button (GO) or nothing to show: its name only */
    if (str_eq(v, "-"))
        v = "none";
    str_cpy(d + str_len(d), " ", cap - str_len(d));
    str_cpy(d + str_len(d), v, cap - str_len(d));
    if (u[0] == '/') {
        str_cpy(d + str_len(d), " of ", cap - str_len(d));
        str_cpy(d + str_len(d), u + 1, cap - str_len(d));
    } else if (u[0] && !str_eq(u, l)) {                /* ("BPM 120 BPM": once) */
        if (u[0] != '%')
            str_cpy(d + str_len(d), " ", cap - str_len(d));
        str_cpy(d + str_len(d), u, cap - str_len(d));
    }
}

static void sp_track(void)                             /* "track 2", "drums" */
{
    char n[4];
    if (song.sel == TRK_DRUM) {
        sp_cat("drums");
        return;
    }
    fmt_int(n, (int32_t)song.sel + 1);
    sp_cat("track ");
    sp_cat(n);
}

static void sp_sound_of(const track_t *t)              /* "JUNO SUB 106" (the engine, then the sound) */
{
    const engine_t *e = ENGINES[t->eng_req % NENGINES];
    char b[16];
    if (is_drum(t))
        return;
    b[0] = 0;
    if (user_of(t) < UP_SLOTS)
        up_name(user_of(t), b);
    else if (e->npresets)
        str_cpy(b, e->presets[t->preset % e->npresets].name, sizeof b);
    {                                                   /* the engine, unless the sound's name starts with it */
        uint32_t n = str_len(e->name), i, same = 1;
        for (i = 0; i < n; i++)
            same &= b[i] == e->name[i];
        same &= b[n] == ' ' || b[n] == 0;
        if (!same || !b[0]) {
            sp_cat(e->name);
            if (b[0])
                sp_cat(" ");
        }
        sp_cat(b);
    }
}
static void sp_sound(void) { sp_sound_of(TSEL); }

static const char *sp_title(void)                      /* the screen's name, as its header says it */
{
    const page_t *pg = cur_page();
    const engine_t *e = ENGINES[TSEL->eng_req % NENGINES];
    if (ui.menu)
        return "menu";
    if (ui.layer != LY_PLAY)
        return LAYER_NAME[ui.layer % LY_COUNT];
#if FELUCCA_ARRANGER
    if (on_song_page())
        return song_bank ? "songs" : "song";
#endif
    if (ui.home)
        return "home";
    if (vis_shown())
        return "visualiser. HOME: back";                /* (HOME on HOME: the full-screen visualiser) */
    if (pg->scope == SC_TRK)
        return "home";                                  /* (the TRACKS page: its header says HOME) */
    if (on_drum_page())
        return "drum kit";
    return pg->scope == SC_ENGINE && !is_drum(TSEL) ? e->page_title[pg->id[0] != P_E0] : pg->title;
}

static void sp_kit(void)                               /* the drum track's kit: "HEX BOOM" */
{
    char v[12];
    const char *u;
    param_format(&DRUM_KIT_DESC, TDRUM->p[P_E0], v, &u);
    sp_cat(v);
}

/* (3.1.3, issue #2) GLO's MIX layer: "track 2, muted, level 63" ("drums" for track 4; the level as its dial, %) */
static uint32_t sp_mix_state(uint32_t i) { return (uint32_t)(trk[i].p[P_MUTE] != 0) | ((song.solo >> i) & 1u) << 1; }
static uint32_t sp_mix_level(uint32_t i) { return trk_level(i) * 100u / 127u; }
static void sp_mix_name(uint32_t i)
{
    char n[4];
    if (i == TRK_DRUM) {
        sp_cat("drums");
        return;
    }
    sp_cat("track ");
    fmt_int(n, (int32_t)i + 1);
    sp_cat(n);
}
static void sp_mix_trk(uint32_t i)
{
    char n[8];
    uint32_t st = sp_mix_state(i);
    sp_mix_name(i);
    if (st & 1u)
        sp_cat(", muted");
    if (st & 2u)
        sp_cat(", solo");
    sp_cat(", level ");
    fmt_int(n, (int32_t)sp_mix_level(i));
    sp_cat(n);
}

#if FELUCCA_ARRANGER
/* (3.1.3, issue #2) the SONG screen's entry at the cursor: "entry 2 of 4, part B, 4 bars" */
static void sp_entry(void)
{
    char n[8];
    const arr_entry_t *e = &arrangement.entry[song_cursor % ARR_STEPS];
    sp_cat("entry ");
    fmt_int(n, (int32_t)song_cursor + 1);
    sp_cat(n);
    sp_cat(" of ");
    fmt_int(n, (int32_t)arrangement.count);
    sp_cat(n);
    sp_cat(", part ");
    n[0] = (char)('A' + (e->scene & 3u));
    n[1] = 0;
    sp_cat(n);
    if (!project_used(e->scene)) {
        sp_cat(", empty");
        return;
    }
    if (e->bars) {
        sp_cat(", ");
        fmt_int(n, e->bars);
        sp_cat(n);
        sp_cat(e->bars == 1u ? " bar" : " bars");
    } else {
        sp_cat(", loop");
    }
}
/* a saved song: "song 2, parts A, B, D" / "song 2, empty" */
static void sp_song_slot(uint32_t k)
{
    char n[4];
    uint32_t inf = song_info_of(k), j, first = 1;
    sp_cat("song ");
    fmt_int(n, (int32_t)k + 1);
    sp_cat(n);
    if (!inf) {
        sp_cat(", empty");
        return;
    }
    sp_cat(", parts ");
    for (j = 0; j < 4u; j++)
        if ((inf >> j) & 1u) {
            char p[2] = {(char)('A' + j), 0};
            if (!first)
                sp_cat(", ");
            sp_cat(p);
            first = 0;
        }
}
static uint32_t sp_song_sig(void)                      /* what the SONG / SONGS screen's knobs change */
{
    const arr_entry_t *e = &arrangement.entry[song_cursor % ARR_STEPS];
    if (song_bank)
        return 0x80000000u | ui.sslot | song_info_of(ui.sslot) << 4;
    return song_cursor | (uint32_t)e->scene << 8 | (uint32_t)e->bars << 10 | arrangement.count << 18 |
           (uint32_t)arrangement_enabled << 26 | (uint32_t)project_used(e->scene) << 27;
}
#endif

/* the screens that are not a page of four knobs: what a glance at them tells */
static int sp_special(void)
{
    uint32_t i;
    char n[8];
    if (!ui.menu && ui.layer == LY_MIX) {              /* (3.1.3, issue #2) GLO's mixer: the four, then its keys */
        sp_cat(". ");
        for (i = 0; i < NTRK; i++) {
            sp_mix_trk(i);
            sp_cat(". ");
        }
        sp_cat("keys 1 to 4: mute, 5 to 8: solo, 9: fill, 10: fill bar, 16: tap tempo. knobs: the levels");
        return 1;
    }
#if FELUCCA_ARRANGER
    if (!ui.menu && ui.layer == LY_SONG) {             /* (3.1.3, issue #2) SAVE held: the parts, the mode, its keys */
        uint32_t ready = arrangement_ready();
        sp_cat(arrangement_enabled ? ". song mode. " : ". loop mode. ");
        for (i = 0; i < 4u; i++) {
            char p[2] = {(char)('A' + i), 0};
            if (i)
                sp_cat(", ");
            sp_cat("part ");
            sp_cat(p);
            if (!((ready >> i) & 1u))
                sp_cat(" empty");
            else if (live_sec == (int8_t)i && !arrangement_clock.running)
                sp_cat(" playing");
        }
        sp_cat(". keys 1 to 4: play a part, 5 to 8: save to a part, 13: song or loop, 14: song rec, 16: the song screen");
        return 1;
    }
#endif
    if (ui.menu || ui.layer != LY_PLAY)
        return 0;
#if FELUCCA_ARRANGER
    if (on_song_page()) {
        if (song_bank) {                                /* (3.0.5) the saved songs */
            sp_cat(". song ");                          /* (3.1.3, issue #2) "songs. song 1 selected. song 1, parts .." */
            fmt_int(n, (int32_t)ui.sslot + 1);
            sp_cat(n);
            sp_cat(" selected. ");
            for (i = 0; i < NSONG; i++) {
                sp_song_slot(i);
                sp_cat(". ");
            }
            sp_cat("knob 1: song, 2: erase, 3: load, 4: save, twice. SAVE: back");
            return 1;
        }
        sp_cat(arrangement_enabled ? ". song mode. " : ". loop mode. ");
        for (i = 0; i < arrangement.count && i < ARR_STEPS; i++) {
            char sec[2] = {(char)('A' + (arrangement.entry[i].scene & 3u)), 0};
            if (i)
                sp_cat(", ");
            sp_cat("part ");
            sp_cat(sec);
            if (arrangement.entry[i].bars) {
                sp_cat(" ");
                fmt_int(n, arrangement.entry[i].bars);
                sp_cat(n);
                sp_cat(arrangement.entry[i].bars == 1u ? " bar" : " bars");
            } else {
                sp_cat(" loop");
            }
            if (str_len(sp_buf) > ANN_MAX - 64u)
                break;
        }
        sp_cat(". ");                                   /* (3.1.3) where the cursor is */
        sp_entry();
        return 1;
    }
#endif
    if (vis_shown())
        return 1;
    if (!ui.home && cur_page()->scope == SC_TRK) {     /* HOME (the TRACKS page): the selected track, its knobs, the four */
        sp_cat(". ");                               /* "home. track 3 selected." */
        sp_track();
        sp_cat(" selected. ");
        if (sp.cols) {                                  /* its knobs first: SWING LEVEL STEPS PAN (issue #2) */
            char k[24];
            uint32_t c;
            for (c = 0; c < 4u; c++) {
                sp_knob(c, k, sizeof k);
                if (k[0]) {
                    if (c)
                        sp_cat(", ");
                    sp_cat(k);
                }
            }
            sp_cat(". ");
        }
        for (i = 0; i < NTRK; i++) {
            if (i)
                sp_cat(", ");
            if (i == TRK_DRUM) {
                sp_cat("drums ");
                sp_kit();
            } else {
                fmt_int(n, (int32_t)i + 1);
                sp_cat(n);
                sp_cat(" ");
                sp_sound_of(&trk[i]);
            }
        }
        return 1;
    }
    if (on_drum_page()) {                              /* the drum kit screen: the kit */
        sp_cat(". kit ");
        sp_kit();
        return 1;
    }
    return 0;
}

static uint32_t sp_sound_sig(void)
{
    const track_t *t = TSEL;
    return (uint32_t)t->eng_req * 7919u + (uint32_t)t->preset * 31u + user_of(t) * 104729u + song.sel;
}

/* the whole screen: the page, the track and its sound, the knobs, the tempo, playing or not */
static void sp_screen(int all)
{
    uint32_t c;
    char k[24], n[8];
    sp_buf[0] = 0;
    sp_cat(sp_title());
    if (sp_special()) {
    } else if (ui.layer == LY_PLAY && !ui.menu) {
        if (!(is_drum(TSEL) && (str_eq(sp_title(), "DRUMS") || str_eq(sp_title(), "drum kit")))) {
            sp_cat(". ");                               /* (not "drums. drums.") */
            sp_track();
            if (!is_drum(TSEL)) {
                sp_cat(", ");
                sp_sound();
            }
        }
        sp_cat(".");
        if (sp.cols)
            for (c = 0; c < 4u; c++) {
                sp_knob(c, k, sizeof k);
                if (k[0]) {
                    sp_sep();
                    sp_cat(k);
                }
            }
    }
    if (all) {
        sp_sep();
        fmt_int(n, song.g[G_BPM]);
        sp_cat(n);
        sp_cat(" B P M, ");
        sp_cat(song.playing ? "playing" : "stopped");
    }
    if (sp_buf[0] && sp_buf[str_len(sp_buf) - 1u] != '.')
        sp_cat(".");
}

/* once a UI frame, after ui_draw: say what changed */
static void speak(uint32_t now)
{
    uint32_t c, ctx, snd;
    char k[24];
    int all = ann_where();
    if (boot_t < BOOT_END) {                           /* the start-up animation: the screen comes after it */
        opk_drawn = 0;
        if (all)
            sp.started = 0;
        return;
    }
    if (!ann_on()) {
        sp.started = 0;
        opk_drawn = 0;
        return;
    }
    sp.cols = opk_drawn && ui.layer == LY_PLAY && !ui.menu;
    opk_drawn = 0;
    ctx = (uint32_t)vis_shown() * 50021u + (uint32_t)ui.page * 31u + ui.home * 7u + (uint32_t)ui.layer * 131u + ui.menu * 977u + song.sel * 3u +
          (uint32_t)sp.cols * 15485863u;
#if FELUCCA_ARRANGER
    ctx += (uint32_t)on_song_page() * 7919u + (uint32_t)song_bank * 104729u;   /* (3.1.3: SAVE there: the songs) */
#endif
    snd = sp_sound_sig();
    if (!sp.started || all) {                          /* a new reader, or it asked: everything */
        sp_screen(1);
        ann_say(sp_buf, 0, ANN_NOW);
    } else if (ctx != sp.ctx) {                        /* a new page: its name and knobs, once it settles */
        sp_screen(0);
        ann_later(sp_buf, ANN_NOW, 200u, now);
    } else if (snd != sp.snd && !is_drum(TSEL)) {      /* PRESETS turned: the sound, and its place in the list */
        uint32_t tot, pos = preset_pos(&tot);
        char n[8];
        sp_buf[0] = 0;
        sp_sound();
        sp_cat(", ");
        fmt_int(n, (int32_t)pos + 1);
        sp_cat(n);
        sp_cat(" of ");
        fmt_int(n, (int32_t)tot);
        sp_cat(n);
        ann_later(sp_buf, ANN_NOW, 120u, now);
    } else if (sp.cols && ui.hot_t) {                  /* a knob turning: its name and value (only while a knob is
                                                        * turned: a drum key played on the KIT screen picks its
                                                        * sound, and is not said) */
        for (c = 0; c < 4u; c++) {
            sp_knob(c, k, sizeof k);
            if (k[0] && !str_eq(k, sp.kv[c])) {
                ann_later(k, ANN_NOW, 120u, now);
                break;
            }
        }
    }
    if (sp.started && ctx == sp.ctx && !ui.menu && ui.layer == LY_MIX) {   /* (3.1.3, issue #2) a mute, a solo, a level */
        for (c = 0; c < NTRK; c++) {
            uint32_t st = sp_mix_state(c), lv = sp_mix_level(c);
            if (st != sp.mix[c]) {
                sp_buf[0] = 0;
                sp_mix_name(c);
                if ((st ^ sp.mix[c]) & 1u)
                    sp_cat(st & 1u ? " muted" : " on");
                if ((st ^ sp.mix[c]) & 2u)
                    sp_cat(st & 2u ? " solo" : " solo off");
                ann_later(sp_buf, ANN_NOW, 120u, now);
                break;
            }
            if (lv != sp.mixlv[c]) {
                char n[8];
                sp_buf[0] = 0;
                sp_mix_name(c);
                sp_cat(" level ");
                fmt_int(n, (int32_t)lv);
                sp_cat(n);
                ann_later(sp_buf, ANN_NOW, 120u, now);
                break;
            }
        }
        if (c == NTRK && (fill_bar_on != 0) != sp.fbar)
            ann_say(fill_bar_on ? "fill bar on" : "fill bar off", 0, ANN_NOW);
    }
#if FELUCCA_ARRANGER
    if (sp.started && ctx == sp.ctx && !ui.menu && ui.layer == LY_PLAY && on_song_page() && sp_song_sig() != sp.sng) {
        sp_buf[0] = 0;                                 /* (3.1.3, issue #2) KNOB 1-4 on SONG / SONGS */
        if (song_bank)
            sp_song_slot(ui.sslot);
        else if (((sp_song_sig() ^ sp.sng) >> 26) & 1u)
            sp_cat(arrangement_enabled ? "song mode" : "loop mode");
        else
            sp_entry();
        ann_later(sp_buf, ANN_NOW, 120u, now);
    }
#endif
    if (sp.started && song.g[G_BPM] != sp.bpm && !mclk_on()) {   /* SELECT, tap tempo: the tempo (not while a
                                                                    * clock from outside sets it) */
        char n[8];
        sp_buf[0] = 0;
        fmt_int(n, song.g[G_BPM]);
        sp_cat(n);
        sp_cat(" B P M");
        ann_later(sp_buf, ANN_NOW, 200u, now);
    }
    if (sp.started && song.playing != sp.playing)
        ann_say(song.playing ? "play" : "stop", 0, ANN_NOW);
    if (sp.started && song.octave != sp.oct) {
        char n[8];
        sp_buf[0] = 0;
        sp_cat("octave ");
        if (song.octave > 0)
            sp_cat("plus ");
        else if (song.octave < 0)
            sp_cat("minus ");
        fmt_int(n, song.octave < 0 ? -song.octave : song.octave);
        sp_cat(n);
        ann_say(sp_buf, 0, ANN_NOW);
    }
    sp.started = 1;
    sp.ctx = ctx;
    sp.snd = snd;
    sp.playing = song.playing;
    sp.oct = song.octave;
    sp.bpm = song.g[G_BPM];
    for (c = 0; c < NTRK; c++) {
        sp.mix[c] = (uint8_t)sp_mix_state(c);
        sp.mixlv[c] = (uint8_t)sp_mix_level(c);
    }
    sp.fbar = fill_bar_on != 0;
#if FELUCCA_ARRANGER
    sp.sng = sp_song_sig();
#endif
    if (sp.cols)
        for (c = 0; c < 4u; c++)
            sp_knob(c, sp.kv[c], sizeof sp.kv[c]);
}
