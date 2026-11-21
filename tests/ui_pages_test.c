/* SPDX-License-Identifier: GPL-3.0-only */
/* The real UI (ui.c, ui_draw.c, ui_studio.c, ui_layers.c, ui_menu.c, ui_input.c) on a
 * framebuffer with panel / flash / storage doubles, the audio (mix_block) running between frames as on
 * the device. Renders every screen to PPM for review (DIR/page-*.ppm, live-*.ppm, layer-*.ppm) and
 * drives the panel:
 *   taps    a layer button tapped opens its pages; held + a key / knob it does not
 *   FX      held + a white key: punch-in; knobs: filter, dust, duck
 *   SEQ     held: the steps on the white keys (drums: the sound played last), OCT: pages, a step key
 *           held + KNOB 1..4: sound / note, level, ratchet, nudge (2.4); PRESETS a lock of the parameter
 *           ALGORITHM picks (the sound parameter last touched first); + OCT-: its nudge, locks and fill
 *           condition go; + OCT+: the condition (normal, fill only, no fill)
 *   EDIT    held + a key: erase; OCT- / OCT+: undo / redo; KNOB 1 shift, 2 length x2
 *   ARP     held + a key: a roll; KNOB 1 the rate
 *   SEL     held + a key: the key of the song
 *   GLO     held + keys: mute, solo, fill (held) / fill the next bar, tap tempo; knobs: levels
 *   REC     press: arm / record at once; held: the clear ring, to the end: cleared (undo brings it back)
 *   SAVE    tapped: the SAVE pages (PRESETS first), on every screen; held: the sections layer (A..D: play / store)
 *   DRUMS   the grid: sound / step / hit / level knobs, GRID <-> KIT
 * then 20000 frames of random use: every draw stays on the screen. */
#define FELUCCA_ARRANGER 1
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
static uint16_t screen[240*240];
static void lcd_sync(void) {}
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *p)
{ uint32_t i,j; assert(x+w<=240 && y+h<=240); for(j=0;j<h;j++) for(i=0;i<w;i++) screen[(y+j)*240+x+i]=p[j*w+i]; }
#include "../firmware/src/gfx.c"
static void lcd_fill(uint32_t x,uint32_t y,uint32_t w,uint32_t h,uint16_t c)
{ uint32_t i,j; assert(x+w<=240&&y+h<=240); for(j=0;j<h;j++)for(i=0;i<w;i++)screen[(y+j)*240+x+i]=swap16(c); }
static int32_t encs[7];
static uint32_t fm1_ticks(void) { return fm1_ms * 1000u * 24u; }
#define FM1_TICKS_PER_US 24u
static int32_t fm1_enc_take(uint32_t e) { int32_t s = encs[e]; encs[e] = 0; return s; }
static uint8_t fm1_led[16], fm1_led_dim[16], fm1_led_bg[16];
static volatile uint16_t fm1_led_bg_ns;
#ifndef FM1_NCOL                                 /* (web/emu/sloop_web.c: the FM-1's own matrix, for the LEDs) */
#define FM1_NCOL 16u
static const int8_t FM1_KEYMAP[5][16];
#endif
static void fm1_led_key(uint32_t id, int on) { (void)id; (void)on; }
static uint32_t edges_btn, notes_seen;
static uint32_t fm1_input_edges(int x) { uint32_t e = edges_btn; (void)x; edges_btn = 0; return e; }
static uint32_t fm1_input_note_edges(void) { uint32_t e = fm1_in.notes & ~notes_seen; notes_seen = fm1_in.notes; return e; }
static uint32_t wdt_tick_ms;                       /* (stress_test: 1, the blocking PANEL setup's clock runs out) */
static void fm1_wdt_feed(void) { fm1_ms += wdt_tick_ms; }
static int32_t fm1_adc_read(int c) { (void)c; return -1; }
static struct { uint32_t magic, stage, page, home, ui_frames; } felucca_dbg;
#define FELUCCA_ICONS 1
#define SCOPE_N 512u
static int16_t scope_buf[SCOPE_N], scope_bufr[SCOPE_N];
static uint32_t scope_w;
#include "../firmware/src/panel.c"
#include "../firmware/src/ui.c"
static uint32_t saves, loads, erases;
static int project_used(uint32_t i) { return i < 2; }
static void project_save(uint32_t i) { (void)i; saves++; ui_message("SAVED"); }
static void project_load(uint32_t i) { (void)i; loads++; }
static void project_erase(uint32_t i) { (void)i; erases++; ui_message("ERASED"); }
static uint32_t arrangement_ready(void) { return 3; }
static void arrangement_apply(uint32_t s) { (void)s; }
static uint32_t sec_stores, sec_loads;
static void section_store(uint32_t s) { sec_stores++; live_sec = (int8_t)s; }
static void section_load(uint32_t s) { sec_loads++; live_sec = (int8_t)s; }
static int up_used(uint32_t k) { return k < 2; }
static int up_load(uint32_t k) { (void)k; return 0; }
static int up_load_to(track_t *t, uint32_t k) { (void)t; (void)k; return 0; }
static uint32_t up_count(void) { return 2; }
static uint32_t up_nth(uint32_t n) { return n; }
static uint32_t up_rank(uint32_t s) { return s; }
static uint32_t up_engine(uint32_t k) { return k ? ENGI_PRISM : ENGI_ACID; }   /* (MY LEAD on track 1, MY PAD on 3) */
static void up_name(uint32_t k, char *b) { str_cpy(b, k ? "MY PAD" : "MY LEAD", 13); }
static void up_slot_label(char *b, uint32_t k) { fmt_int(b, (int32_t)k + 1); }
static void up_ui(uint32_t op, uint32_t k) { (void)op; (void)k; }
static void settings_save(void) {}
#include "../firmware/src/ui_studio.c"
#include "../firmware/src/ui_op.c"
#include "../firmware/src/icons.c"
#include "../firmware/src/ui_draw.c"
#include "../firmware/src/ui_vis.c"
#include "../firmware/src/ui_layers.c"
#include "../firmware/src/ui_menu.c"
#include "../firmware/src/ui_input.c"
#include "../firmware/src/splash.c"
static const char *outdir;
static void ppm(const char *name) {
    char path[512]; snprintf(path,sizeof path,"%s/%s.ppm",outdir,name);
    FILE *f=fopen(path,"wb"); assert(f); fprintf(f,"P6\n240 240\n255\n");
    for(unsigned i=0;i<240*240;i++) { uint16_t p=swap16(screen[i]); uint8_t rgb[3]={(p>>11)*255/31,((p>>5)&63)*255/63,(p&31)*255/31}; fwrite(rgb,1,3,f); }
    fclose(f);
}
/* one UI frame (~16 ms): the audio between (as the ISR does), then input, LEDs, draw */
static void frame(void)
{
    uint32_t q;
    static int32_t o[CTL * 2];
    for (q = 0; q < 22u; q++) {
        uint32_t i;
        mix_block(o, CTL);
        for (i = 1; i < CTL; i += 2u) {             /* the scope, as audio.c fills it (the visualiser) */
            scope_bufr[scope_w & (SCOPE_N - 1u)] = vis_tap[2u * i + 1u];
            scope_buf[scope_w++ & (SCOPE_N - 1u)] = vis_tap[2u * i];
        }
    }
    ui_input(); ui_leds(); ui_draw(); fm1_ms += 16;
}
static void frames(uint32_t n) { while (n--) frame(); }
static uint32_t BT(uint32_t b) { return 1u << panel.btn[b]; }
static void press(uint32_t b) { edges_btn |= BT(b); fm1_in.buttons |= BT(b); frame(); }
static void release(uint32_t b) { fm1_in.buttons &= ~BT(b); frame(); }
static void tap(uint32_t b) { press(b); release(b); }
static void key(uint32_t k) { fm1_in.notes |= 1u << k; frame(); fm1_in.notes &= ~(1u << k); frame(); }
static int grid_voice(const track_t *t, uint32_t note)   /* a voice of t sounds note, its gate open */
{ uint32_t v; for (v = 0; v < NVOICE; v++) if (t->v[v].active && t->v[v].gate && t->v[v].note == note) return 1; return 0; }
static int fails;
static void check(int ok, const char *what) { printf("ui: %-74s %s\n", what, ok ? "ok" : "FAIL"); fails += !ok; }
static const char *sub_line(void) { return layer_sub_shown; }
static uint32_t px_count(uint32_t x0, uint32_t y0, uint32_t w, uint32_t h, uint16_t c)   /* pixels of colour c in a box */
{ uint32_t x, y, n = 0; for (y = y0; y < y0 + h; y++) for (x = x0; x < x0 + w; x++) n += swap16(screen[y * 240u + x]) == c; return n; }   /* the layer's sub line as last drawn (ui_layers.c) */

/* PHEBUS (Ben, 21 Nov 2026: "just a plain sawtooth is fine for the bass, chords, and lead"; "Keep presets, plain start";
 * "JUNO on track 2"; picker "ACID for bass"): the default sounds. Track 1 ACID INIT mono, an octave down (a saw through
 * the 303's filter, part open, a moderate envelope: AC below); track 2 JUNO polyphonic; track 3 PLAITS VA
 * mono; each a plain saw (JUNO: SAW, PW 0, SUB 0, NOISE 0, CUT 127, RES 0, ENV 0, CHORUS OFF; VA: MODEL 5, HARM 64,
 * TIMB 0, MRPH 64, ENV 0, SUB 0), the INIT preset, no send, no LFO routing, no glide, no insert, full sustain */
static int plain_defaults(void)
{
    static const int8_t JU[8] = {0, 0, 0, 0, 127, 0, 0, 0}, VA[8] = {5, 64, 0, 64, 0, 0, 0, 0}, AC[8] = {0, 45, 45, 55, 55, 0, 0, 21};
    uint32_t i, k;
    int ok = trk[0].eng_req == ENGI_ACID && trk[1].eng_req == ENGI_JUNO && trk[2].eng_req == ENGI_PRISM &&
             !strcmp(ENGINES[ENGI_ACID]->presets[trk[0].preset].name, "INIT") && trk[0].preset == ACID_INIT &&
             !strcmp(ENGINES[ENGI_JUNO]->presets[trk[1].preset].name, "INIT") &&
             !strcmp(ENGINES[ENGI_PRISM]->presets[trk[2].preset].name, "INIT") &&
             trk[0].p[P_VOICE] != V_POLY && trk[1].p[P_VOICE] == V_POLY && trk[2].p[P_VOICE] != V_POLY &&
             trk[0].p[P_TRANS] == -12 && trk[1].p[P_TRANS] == 0 && trk[2].p[P_TRANS] == 0;
    for (i = 0; i < NPART; i++) {
        const track_t *t = &trk[i];
        for (k = 0; k < 8u; k++)
            ok &= t->p[P_E0 + k] == (i == 0u ? AC[k] : i == 1u ? JU[k] : VA[k]);
        ok &= !t->p[P_DIST] && !t->p[P_CHOR] && !t->p[P_DLY] && !t->p[P_REV] && !t->p[P_ITYPE] && !t->p[P_GLIDE] &&
              !t->p[P_LD_PIT] && !t->p[P_LD_FLT] && !t->p[P_LD_SHP] && !t->p[P_LD_AMP] && !t->p[P_ED_FLT] &&
              !t->p[P_ED_PIT] && !t->p[P_ED_SHP] && t->p[P_SUS] == 127 && t->p[P_ATK] <= 10 && t->p[P_REL] <= 20 &&
              !t->p[P_SLCR] && !t->p[P_TFLT] && !t->p[P_AMODE];
    }
    return ok;
}

/* PHEBUS (review 8, Ben 21 Nov: "Sound big, character small"): the sound pages' plot as a hash (the graph canvas from
 * the plot's top, the character's corner left out), and a page of the chain by its title */
static uint32_t plot_hash(void)
{
    uint32_t x, y, h = 2166136261u;
    for (y = 22u + 20u; y < 22u + 118u; y++)
        for (x = 0; x < 240u; x++)
            if (x < 190u || y > 22u + 40u)
                h = (h ^ screen[y * 240u + x]) * 16777619u;
    return h;
}
static int chain_goto(uint32_t fam, const char *title)
{
    uint32_t j;
    open_family(fam); frame();
    for (j = 0; j < 20u && strcmp(PAGES[ui.page].title, title); j++) { encs[panel.enc[EN_SELECT]] = 1; frame(); }
    return !strcmp(PAGES[ui.page].title, title);
}

#ifndef UI_PAGES_HARNESS_ONLY                       /* (tests/stress_test.c: the harness, its own main) */
int main(int argc, char **argv)
{
    uint32_t i;
    outdir = argc > 1 ? argv[1] : "build/host";
    {   /* the preset list by kind (ui.c BANK): every factory preset of every engine once, every name found */
        uint32_t e, k, n, hits;
        bank_resolve();
        for (n = 0; n < NBANK; n++)
            if (bank_pi[n] == 0xFF) {
                printf("ui: BANK %s: no such preset in engine %u\n", BANK[n].name, BANK[n].e);
                fails++;
            }
        for (e = 0; e < NENGINES; e++)
            for (k = 0; k < ENGINES[e]->npresets && ENGINES[e] != &ENG_GONE; k++) {
                for (hits = 0, n = 0; n < NBANK; n++)
                    hits += BANK[n].e == e && bank_pi[n] == k;
                if (hits != 1) {
                    printf("ui: preset %s (engine %u) is %u times in BANK\n", ENGINES[e]->presets[k].name, e, hits);
                    fails++;
                }
            }
    }
    {   /* each track's list: its engines only (track 1 ACID + JUNO, 2 ELEMENTS, 3 PLAITS) */
        uint32_t t, n, k, ok = 1, seen[NENGINES] = {0};
        for (t = 0; t < NPART; t++)
            for (n = 0; n < pl_total(&trk[t]); n++) {
                uint32_t e = preset_at_of(&trk[t], n, &k);
                if (e < NENGINES) {
                    ok &= eng_allowed(&trk[t], e);
                    seen[e] |= 1u << t;
                }
            }
        check(ok && seen[ENGI_ACID] == 1u && seen[ENGI_JUNO] == 3u && seen[ENGI_ELEM] == 2u && seen[ENGI_PRISM] == 4u,
              "PRESETS: track 1 lists ACID and JUNO, track 2 JUNO and ELEMENTS, track 3 PLAITS, nothing else");
    }
    panel = PANEL_DEFAULT;
    layers_init();
    settings.palette = 4;
    palette_set(4);
    host_tracks_init();
    for (i = 0; i < NPART; i++) { trk_def_sound(&trk[i], i); trk[i].engine = trk[i].eng_req; }   /* (as main.c felucca_init) */
    TDRUM->p[P_E0] = DRUM_DEFAULT_KIT;
    {   /* PHEBUS (21 Nov, Ben: "I don't like having fx on by default"): power-on is dry */
        uint32_t ok = 1, l;
        for (i = 0; i < NPART; i++)
            ok &= !trk[i].p[P_DIST] && !trk[i].p[P_CHOR] && !trk[i].p[P_DLY] && !trk[i].p[P_REV] && !trk[i].p[P_ITYPE] &&
                  !trk[i].p[P_SSWING];
        for (l = 0; l < DRUM_LANES; l++)
            ok &= !drum_send[l][0] && !drum_send[l][1];
        ok &= !TDRUM->p[P_DIST] && !TDRUM->p[P_DLY] && !TDRUM->p[P_REV] && !TDRUM->p[P_ITYPE] && !song.g[G_DRREV] &&
              !song.g[G_DRDLY] && !song.g[G_SWING] && !song.g[G_DUST] && !song.g[G_DUCK] && !song.g[G_FILT];
        check(ok, "power-on: every send 0 (tracks, drum sounds), no DIST / INSERT, no swing, master FX off");
        check(plain_defaults(), "power-on: plain saws: ACID INIT bass (mono, -12), JUNO chords (poly), PLAITS VA lead (mono)");
        check(ACID_INIT == NELEM(ACID_PRESETS) - 1u && ACID_INIT == 6u && !strcmp(ACID_PRESETS[ACID_INIT].name, "INIT") &&
              ACID_PRESETS[ACID_INIT].e[0] == 0 && !ACID_PRESETS[ACID_INIT].e[5] && !ACID_PRESETS[ACID_INIT].e[6] &&
              ACID_PRESETS[ACID_INIT].mono && preset_trim(ENGI_ACID, ACID_INIT) == 18 && preset_trim(ENGI_ACID, 0) == 23,
              "ACID INIT: appended last (the others keep their numbers and trims), SAW, no ACC, no DRIVE, mono, level-trimmed");
    }
    sloop_splash(); ppm("page-splash");
    { static const uint32_t SEED[6] = {0u, 0x1231u, 0x2462u, 0x7733u, 0x51A4u, 0x9C55u}; uint32_t k;   /* the boot: FLOW, PHEBUS black in it */
      for (k = 0; k < 6u; k++) { char nm[16]; snprintf(nm, sizeof nm, "boot-%u", (unsigned)k); flow_seed = SEED[k]; boot_frame(60u + k * 9u); ppm(nm); } }
    ui.menu = 2; ui.force = 1; frame(); ppm("page-about"); ui.menu = 0;
    go_home(); ui.force = 1; frame(); ppm("page-tracks");
    open_family(FAM_EDIT); ui.force = 1; frame(); ppm("page-osc-default");
    /* (the rest of this test was written on the old power-on sounds, ACID TB ACID and ELEMENTS PICKED BAR: those again) */
    set_engine_of(&trk[0], ENGI_ACID); trk[0].engine = trk[0].eng_req;
    set_engine_of(&trk[1], ENGI_ELEM); apply_preset_to(&trk[1], 3); trk[1].engine = trk[1].eng_req;
    go_home(); ui.force = 1; frame();
    {   /* PHEBUS (21 Nov, Ben: "Dot + save on stop"): the header's unsaved dot (project.c unsaved_tick sets proj_unsaved),
         * between the beats and the battery; REC's red disc takes its place while recording; gone once saved */
        uint16_t was = swap16(screen[10 * 240 + 209]);
        proj_unsaved = 1; frame();
        check(was != C_WHITE && swap16(screen[10 * 240 + 209]) == C_WHITE,
              "unsaved work: a white dot in the header, clear of the tempo and the battery");
        ppm("home-unsaved");
        open_family(FAM_ENV); ui.force = 1; frame();
        check(swap16(screen[10 * 240 + 209]) == C_WHITE, "unsaved work: the dot on a sound page's header too");
        go_home(); ui.force = 1; frame();
        proj_unsaved = 0; frame();
        check(swap16(screen[10 * 240 + 209]) == was, "saved: the dot goes");
        check(!strcmp(DRUM_KIT_NAMES[DRUM_DEFAULT_KIT], "606"), "power-on drum kit: the 606 (Ben, 21 Nov)");
    }
    {   /* review 21: a message stays on the screen it showed on; another screen, it goes */
        go_home(); ui.force = 1; frame();
        ui_message("STOP FIRST"); frame(); frame();
        check(ui.msg_t != 0, "a message shows on HOME");
        open_family(FAM_ENV); frame(); frame();
        check(ui.msg_t == 0, "... and clears when the screen changes (review 21)");
        open_family(FAM_ENV); ui_message("NOT ON THIS SOUND"); frame(); frame();
        check(ui.msg_t != 0, "a message made as the screen changes stays on the new screen");
        go_home(); ui.force = 1; frame();
    }
    {   /* review 16d: the downbeat light white while playing, red only while recording */
        uint32_t c = 0, b = clk_beat, pl = song.playing, rc = song.rec;
        clk_beat = 4u; song.playing = 1; song.rec = 0; ui.force = 1;
        op_header("HOME", TE_G3, "loop", &c);
        check(swap16(screen[9 * 240 + 174]) == C_WHITE, "header: the downbeat white while playing");
        song.rec = 1; ui.force = 1;
        op_header("HOME", TE_G3, "loop", &c);
        check(swap16(screen[9 * 240 + 174]) == TE_RED, "header: the downbeat red while recording");
        clk_beat = b; song.playing = pl; song.rec = rc;
        go_home(); ui.force = 1; frame();
    }
    open_family(FAM_ENV); ui.force = 1; ui.hot_col = 1; ui.hot_t = 30; frame(); ppm("page-env");
    check(px_count(0, Y_GRAPH + G_OY, 40, 26, C_WHITE) + px_count(0, Y_GRAPH + G_OY, 40, 26, TE_G2) == 0 &&
          px_count(190, Y_GRAPH + G_OY, 50, 20, TE_G2) == 0,
          "ENV, no note playing: the level and GATE readout hidden (review 19)");
    check(px_count(0, OP_KNOB_Y + 5, 240, 16, C_WHITE) == 0, "ENV: the touched knob keeps the voice colour (no white, review 16e)");
    {   /* review 19: HOME, a muted track: its real level, dim, with MUTE (not LEVEL 0) */
        int16_t m = trk[0].p[P_MUTE];
        song.sel = 0; trk[0].p[P_MUTE] = 1; go_home(); ui.force = 1; frame();
        check(px_count(178, OP_KNOB_Y + 26, 56, 26, TE_G3) > 40 && px_count(178, OP_KNOB_Y + 26, 56, 26, TE_COL[0]) == 0,
              "HOME, track 1 muted: LEVEL (KNOB 4, PHEBUS) shows its level in grey, with MUTE");
        trk[0].p[P_MUTE] = m; go_home(); ui.force = 1; frame();
        open_family(FAM_ENV); ui.force = 1; frame();
    }
    {   /* PHEBUS (Ben, 21 Nov 2026, picker "Cutoff, decay, FX, level"; FX: "just turned on and off the stutter"): HOME's
         * knobs: CUTOFF and DECAY of the selected voice (its engine's CUT / DECAY, else FILTER / the ENV's DEC; drums: the
         * track filter, a HEX sound's DEC), STUTTER (the lane on / off, drawn neutral), LEVEL; pan on the mixer (GLO + 11) */
        static track_t keep[NTRK];
        const param_desc_t *d;
        int16_t lv, *vp, tf = trk[2].p[P_TFLT], kit = TDRUM->p[P_E0];
        uint32_t act = stl.act, k, ok;
        memcpy(keep, trk, sizeof keep);
        set_engine_of(&trk[0], ENGI_ACID); song.sel = 0; frame();
        check(home_knob(0, &d) == &trk[0].p[P_E1] && home_knob(1, &d) == &trk[0].p[P_E4] && str_eq(d->label, "DECAY"),
              "HOME knobs, ACID: CUTOFF its CUT, DECAY its own DECAY (the 303's envelope)");
        set_engine_of(&trk[0], ENGI_JUNO); frame();
        check(home_knob(0, &d) == &trk[0].p[P_E4] && str_eq(d->label, "CUT") && home_knob(1, &d) == &trk[0].p[P_DEC],
              "HOME knobs, JUNO: CUTOFF its VCF CUT, DECAY the ENV's DEC");
        song.sel = 2; frame();
        check(trk[2].eng_req == ENGI_PRISM && home_knob(0, &d) == &trk[2].p[P_TFLT] && home_knob(1, &d) == &trk[2].p[P_DEC],
              "HOME knobs, PLAITS: CUTOFF the voice's FILTER, DECAY the ENV's DEC");
        set_engine_of(&trk[1], ENGI_ELEM); song.sel = 1; frame();
        check(home_knob(0, &d) == &trk[1].p[P_TFLT] && home_knob(1, &d) == &trk[1].p[P_DEC], "HOME knobs, ELEMENTS: FILTER, DEC");
        song.sel = TRK_DRUM; TDRUM->p[P_E0] = 0; frame();
        check(home_knob(0, &d) == &TDRUM->p[P_TFLT] && home_knob(1, &d) == &song.g[G_HXDEC],
              "HOME knobs, drums (HEX kit): CUTOFF the drum track's FILTER, DECAY the selected sound's DEC");
        TDRUM->p[P_E0] = DRUM_DEFAULT_KIT; frame();
        check(home_knob(1, &d) == 0, "HOME knobs, drums (606): no DEC to turn (drawn --)");
        go_home(); ui.force = 1; frame(); ppm("home-knobs-drum");
        song.sel = 2; go_home(); frames(16);
        trk[2].p[P_TFLT] = 0;
        encs[panel.enc[EN_K1]] = -5; frame();
        check(trk[2].p[P_TFLT] < 0, "HOME KNOB 1 on PLAITS: the FILTER goes low-pass");
        lv = trk[2].p[P_DEC]; encs[panel.enc[EN_K2]] = 3; frame();
        check(trk[2].p[P_DEC] > lv, "HOME KNOB 2: DECAY (the ENV's DEC) up");
        stl.act = 1; encs[panel.enc[EN_K3]] = -1; frame();
        ok = !stl.act; encs[panel.enc[EN_K3]] = 1; frame();
        check(ok && stl.act, "HOME KNOB 3: the STUTTER lane OFF (left), ON (right)");
        lv = trk[2].p[P_LEVEL]; encs[panel.enc[EN_K4]] = -4; frame();
        check(trk[2].p[P_LEVEL] < lv, "HOME KNOB 4: LEVEL");
        trk[2].p[P_MUTE] = 1; lv = trk[2].p[P_LEVEL]; encs[panel.enc[EN_K4]] = 3; frame();
        check(!trk[2].p[P_MUTE] && trk[2].p[P_LEVEL] == lv, "HOME KNOB 4 on a muted voice: the first turn unmutes (as before)");
        ui.force = 1; frame(); ppm("home-knobs");
        for (ok = 1, k = 0; k < 4u; k++) {
            uint32_t c = px_count(4 + 58 * k, OP_KNOB_Y, 54, OP_KNOB_H - 4, TE_COL[2]);
            ok &= k == 2u ? c == 0u : c > 30u;
        }
        check(ok, "HOME knobs: CUTOFF, DECAY, LEVEL in the voice's colour; STUTTER neutral (shared)");
        press(B_GLO); frames(10); key(key_of_lane(10)); frames(2);
        lv = trk[0].p[P_PAN]; encs[panel.enc[EN_K1]] = 5; frame();
        check(mix_pan && trk[0].p[P_PAN] == lv + 5, "pan: GLO held + key 11: the mixer's knobs are the pans (KNOB 1: voice 1's)");
        ui.force = 1; frame(); ppm("layer-mix-pan");
        release(B_GLO); frames(2);
        check(!mix_pan, "pan: GLO let go: the mixer's knobs are the levels again");
        for (ok = 0, k = 0; k < NPAGES; k++)
            ok |= !strcmp(PAGES[k].title, "VOICE 2") && PAGES[k].id[2] == P_PAN;
        check(ok, "pan: still on VOICE 2 (KNOB 3) too");
        memcpy(trk, keep, sizeof keep); trk[2].p[P_TFLT] = tf; TDRUM->p[P_E0] = kit; stl.act = (uint8_t)act; sync_reload = 1;
        song.sel = 0; go_home(); frames(16);
    }
    {   /* PHEBUS (Ben, 21 Nov 2026, picker "Add PATTERN"): LFO SYNC > TRIG is NOTE, FREE or PATTERN */
        int16_t tr = trk[0].p[P_LTRIG];
        song.sel = 0; go_home(); frames(16);
        trk[0].p[P_LTRIG] = 0;
        check(chain_goto(FAM_LFO, "LFO SYNC") && cur_page()->id[1] == P_LTRIG, "LFO SYNC: KNOB 2 is TRIG");
        encs[panel.enc[EN_K2]] = 1; frame(); encs[panel.enc[EN_K2]] = 1; frame(); encs[panel.enc[EN_K2]] = 1; frame();
        check(trk[0].p[P_LTRIG] == 2 && !strcmp(TP[P_LTRIG].names[2], "PATT") && TP[P_LTRIG].max == 2,
              "LFO SYNC: TRIG NOTE -> FREE -> PATT (PATTERN, the last)");
        ui.force = 1; frame(); ppm("page-lfo-sync-pattern");
        trk[0].p[P_LTRIG] = tr; go_home(); frames(16);
    }
    {   /* PHEBUS (Ben, 21 Nov 2026: "I don't like how the tape reel looks"; picker "A strip of the flowing backdrop"): HOME's
         * deck: a strip of FLOW (2 x 2 blocks) in the voices' colours where the reels were; still when nothing plays or
         * sounds, flowing while the song plays, each voice's band swelling with its level */
        static uint16_t a[HB_W * HB_H];
        static track_t keep[NTRK];
        uint32_t x, y, same, n0, voice = 0, pur = 0, blocks = 1, k;
        memcpy(keep, trk, sizeof keep);
        song.sel = 0; go_home(); frames(80);              /* (silent: the levels sink, the strip stops) */
        ui.force = 1; frame();
        for (y = 0; y < HB_H; y++) for (x = 0; x < HB_W; x++) a[y * HB_W + x] = screen[(OP_BODY_Y + HB_Y + y) * 240 + HB_X + x];
        frames(6);
        for (same = 1, y = 0; y < HB_H; y++) for (x = 0; x < HB_W; x++) same &= a[y * HB_W + x] == screen[(OP_BODY_Y + HB_Y + y) * 240 + HB_X + x];
        check(same && !song.playing, "HOME strip: stopped and silent: it stands still (not redrawn)");
        for (y = 0; y < HB_H; y += 2) for (x = 0; x < HB_W; x += 2) {
            uint16_t c = screen[(OP_BODY_Y + HB_Y + y) * 240 + HB_X + x];
            blocks &= c == screen[(OP_BODY_Y + HB_Y + y) * 240 + HB_X + x + 1] && c == screen[(OP_BODY_Y + HB_Y + y + 1) * 240 + HB_X + x];
        }
        check(blocks, "HOME strip: drawn in 2 x 2 blocks");
        pur = px_count(84, OP_BODY_Y, 80, 44, OP_PUR);
        check(pur == 0u, "HOME: the tape reels are gone (no reel line art in the deck)");
        ppm("home-band-still");
        for (k = 0; k < 16u; k++) {                       /* a bass line, chords, a lead and a beat */
            uint8_t nb = (uint8_t)(36 + (k % 3u) * 7u), ch[3] = {60, 64, 67}, ld = (uint8_t)(72 + (k % 4u) * 2u), kk = 36, hh = 42;
            put_step(&trk[0], k, k % 2u ? 0u : 1u, &nb, k % 2u ? ST_REST : ST_NOTE, 0);
            put_step(&trk[1], k, k % 8u ? 0u : 3u, ch, k % 8u ? (k % 8u < 6u ? ST_TIE : ST_REST) : ST_NOTE, 0);
            put_step(&trk[2], k, k % 4u == 2u ? 1u : 0u, &ld, k % 4u == 2u ? ST_NOTE : ST_REST, 0);
            put_step(TDRUM, k, 1, k % 4u ? &hh : &kk, ST_NOTE, 0);
            trk[0].p[P_SLEN] = trk[1].p[P_SLEN] = trk[2].p[P_SLEN] = TDRUM->p[P_SLEN] = 16;
        }
        for (k = 0; k < NTRK; k++) trk[k].seq_active = 1;
        transport_req = 1; frames(60);
        for (y = 0; y < HB_H; y++) for (x = 0; x < HB_W; x++) a[y * HB_W + x] = screen[(OP_BODY_Y + HB_Y + y) * 240 + HB_X + x];
        frames(4);
        for (n0 = 0, y = 0; y < HB_H; y++) for (x = 0; x < HB_W; x++) n0 += a[y * HB_W + x] != screen[(OP_BODY_Y + HB_Y + y) * 240 + HB_X + x];
        for (k = 0; k < 4u; k++) voice += px_count(HB_X, OP_BODY_Y + HB_Y, HB_W, HB_H, vp[k][1]) + px_count(HB_X, OP_BODY_Y + HB_Y, HB_W, HB_H, vp[k][2]);
        check(n0 > HB_W * HB_H / 8u && voice > 0u, "HOME strip: playing, it flows (redrawn), in the voices' colours");
        check(vs.e[0] > 0u && vs.e[3] > 0u, "HOME strip: the voices' levels drive it (bass and drums sounding)");
        ui.force = 1; frame(); ppm("home-band-play");
        transport_req = 2; frames(4);
        memcpy(trk, keep, sizeof keep); sync_reload = 1;
        go_home(); frames(4);
    }
    go_home(); open_family(FAM_EDIT); ui.force = 1; frame(); ppm("page-edit");   /* (EDIT: the chain's start, OSC) */
    open_family(FAM_FX); ui.force = 1; frame(); ppm("page-fx");
    open_family(FAM_SEQ); ui.force = 1; frame(); ppm("page-pattern");
    open_family(FAM_GLO); ui.force = 1; frame(); ppm("page-global");
    {   /* review 16b (Ben, 21 Nov: "Fix all"): a page every voice shares is neutral: the tag "ALL" in grey, no voice colour
         * in the header, no character, the knobs grey */
        uint32_t k, voice = 0;
        for (k = 0; k < 4u; k++)
            voice += px_count(0, 0, 240, OP_HEAD_H, TE_COL[k]) + px_count(0, OP_BODY_Y, 240, OP_KNOB_Y - OP_BODY_Y, TE_COL[k]) +
                     px_count(0, OP_KNOB_Y, 240, OP_KNOB_H, TE_COL[k]);
        check(!strcmp(cur_page()->title, "GLOBAL") && px_count(2, 1, 46, 18, TE_G4) > 60 && voice == 0,
              "GLO > GLOBAL: tag ALL in grey, no voice colour above the strip (no character, neutral knobs)");
    }
    open_family(FAM_GLO); ui.force = 1; frame(); ppm("page-master");
    open_family(FAM_SCL); ui.force = 1; frame(); ppm("page-scale");
    {   /* (2.4) GLO > SYSTEM: KNOB 1 is MIDI OUT (KEYS / SEQ), shown as such; the USB status in the third column */
        uint32_t i, m0 = (uint32_t)song.g[G_MIDI];
        for (i = 0; i < NPAGES; i++) if (!strcmp(PAGES[i].title, "SYSTEM")) break;
        open_family(FAM_GLO); ui.page = (uint8_t)i; ui.fam_last[FAM_GLO] = (uint8_t)i; page_entered(); ui.force = 1; frame();
        song.g[G_MIDI] = 0; ui.force = 1; frame();
        encs[panel.enc[EN_K1]] = 1; frame();
        check(i < NPAGES && song.g[G_MIDI] == 1, "GLO > SYSTEM: KNOB 1 sets MIDI = SEQ");
        {
            char val[12]; const char *unit = "";
            int16_t *vp; const param_desc_t *d = page_desc(cur_page(), 0, &vp);
            param_format(d, *vp, val, &unit);
            check(d && !strcmp(d->label, "OUT") && !strcmp(val, "SEQ"), "GLO > SYSTEM: the first column reads OUT SEQ (MIDI OUT)");
        }
        ui.force = 1; frame(); ppm("page-system");
        {   /* (2.4) KNOB 3: IN = CLOCK (notes ignored), kept in the settings word; back to NOTES */
            int16_t r0 = song.g[G_ROUTE];
            int16_t *vp; const param_desc_t *d = page_desc(cur_page(), 2, &vp);
            char val[12]; const char *unit = "";
            encs[panel.enc[EN_K3]] = 1; frame();
            param_format(d, *vp, val, &unit);
            check(song.g[G_ROUTE] == 1 && !strcmp(d->label, "IN") && !strcmp(val, "CLOCK"), "GLO > SYSTEM: KNOB 3 sets IN = CLOCK");
            lights_min = 1;
            check((lights_word() >> 15 & 1u) == 1u, "IN = CLOCK kept in the settings word (bit 15)");
            lights_from_word(lights_word() & ~(1u << 15));
            check(lights_min == 0u, "IN read back from the settings word");
            encs[panel.enc[EN_K3]] = 12; frame();             /* (2.6) on to CH 12 */
            param_format(d, *vp, val, &unit);
            lights_min = (uint8_t)song.g[G_ROUTE];
            {
                uint32_t w = lights_word();
                lights_min = 0;
                lights_from_word(w);
                check(song.g[G_ROUTE] == 13 && !strcmp(val, "CH 12") && lights_min == 13u && !(w >> 15 & 1u) && (w >> 21 & 31u) == 12u
                      && d->max == 17, "2.6: IN = CH 12 (CH 1..16 after CLOCK), kept in the settings word (bits 21-25), read back");
                lights_from_word(w & ~(31u << 21));
                check(lights_min == 0u, "2.6: a settings word from 2.5 (no channel bits): IN = NOTES");
            }
            encs[panel.enc[EN_K3]] = -13; frame();
            song.g[G_ROUTE] = r0;
        }
        song.g[G_MIDI] = (int16_t)m0; go_home(); frame();
    }

    /* ---- taps open pages, holds are layers */
    go_home(); ui.force = 1; frame();
    { uint8_t was = song.sel; song.sel = 0; check(keys_guide() == 0u, "synth track, no layer: no landmarks (a piano)"); song.sel = was; }
    /* (Ben's SPEC.md, 20 Nov 2026: step keys on every track. SEQ tapped on HOME is the track's GRID, as on the drum
     *  track; SEQ tapped again: the SEQ pages. Before: straight to the SEQ pages) */
    tap(B_SEQ); check(on_drum_page(), "SEQ tapped on HOME: the track's GRID");
    tap(B_SEQ); check(cur_fam() == FAM_SEQ, "SEQ tapped again: the SEQ pages");
    go_home(); frame();
    tap(B_FX); check(cur_fam() == FAM_FX, "FX tapped: the FX pages");
    go_home(); frame();
    press(B_FX); frames(12); check(ui.layer == LY_FX && punch.hold, "FX held: the punch layer shows");
    check(keys_guide() == (1u << key_of_white(0) | 1u << key_of_white(4) | 1u << key_of_white(8) | 1u << key_of_white(12)),
          "FX held: keys 1, 5, 9, 13 lit dim (the rows of the grid)");
    ppm("layer-punch");
    fm1_in.notes = 1u << 4; frame(); check(punch.req == 2, "FX + the 3rd white key: punch effect 3");
    ppm("layer-punch-on");
    fm1_in.notes = 0; frame(); check(punch.req == -1, "key up: the mix comes back");
    fm1_in.notes = 1u << 17; frame(); check(punch.req == PX_FREEZE, "FX + the black key A#4: FREEZE (2.6: 11 black-key effects)");
    check(keys_lit() == 1u << 17, "its key lights");
    ppm("layer-punch-black");
    fm1_in.notes = 0; frame(); check(punch.req == -1, "black key up: the mix comes back");
    encs[panel.enc[EN_K2]] = 10; frame(); check(song.g[G_DUST] > 0, "FX + KNOB 2: DUST");
    encs[panel.enc[EN_K1]] = -10; frame(); check(song.g[G_FILT] < 0, "FX + KNOB 1: the filter (low-pass)");
    encs[panel.enc[EN_K4]] = 12; frame(); check(TSEL->p[P_TFLT] > 0 && song.g[G_FILT] < 0, "FX + KNOB 4: the selected track's FILTER (high-pass)");
    ui.force = 1; frame(); ppm("layer-punch-trkflt");
    TSEL->p[P_TFLT] = 0;
    release(B_FX); check(cur_page()->scope == SC_TRK && ui.layer == LY_PLAY, "FX used then let go: no FX page");
    song.g[G_DUST] = 0; song.g[G_FILT] = 0;

    /* ---- a layer locked open: held + HOME tapped; any other button (not PLAY, REC, OCT) lets it go */
    go_home(); frame();
    press(B_FX); frames(3); tap(B_HOME); release(B_FX); frames(3);
    check(ly_lock == LY_FX && ui.layer == LY_FX && punch.hold, "FX held + HOME: locked open, FX let go");
    check(cur_page()->scope == SC_TRK, "FX + HOME: no FX page, no HOME jump");
    fm1_in.notes = 1u << 4; frame(); check(punch.req == 2, "locked FX + the 3rd white key: punch effect 3 (no hands on FX)");
    fm1_in.notes = 0; frame(); check(punch.req == -1, "locked FX, key up: the mix comes back");
    encs[panel.enc[EN_K2]] = 6; frame(); check(song.g[G_DUST] > 0, "locked FX + KNOB 2: DUST");
    ui.force = 1; frame(); ppm("layer-locked");
    { uint8_t was = song.playing; tap(B_PLAY); frames(2);
      check(ly_lock == LY_FX && song.playing != was, "locked: PLAY plays and keeps the lock"); tap(B_PLAY); frames(2); }
    tap(B_ENV); frames(2);
    check(ly_lock == LY_PLAY && ui.layer == LY_PLAY && !punch.hold && cur_page()->scope == SC_TRK,
          "locked, ENV pressed: unlocked, and only that (no ENV page)");
    press(B_FX); frames(3); tap(B_HOME); release(B_FX); frames(3);
    tap(B_HOME); frames(2);
    check(ly_lock == LY_PLAY && ui.layer == LY_PLAY && cur_page()->scope == SC_TRK, "locked, HOME tapped: unlocked");
    press(B_FX); frames(3); tap(B_HOME); release(B_FX); frames(3);
    tap(B_FX); frames(2);
    check(ly_lock == LY_PLAY && ui.layer == LY_PLAY && cur_fam() != FAM_FX, "locked, FX tapped: unlocked, no FX page");
    go_home(); frame();
    press(B_SEQ); frames(3); tap(B_HOME); release(B_SEQ); frames(3);
    press(B_FX); frames(12);
    check(ly_lock == LY_PLAY && ui.layer == LY_FX, "locked SEQ, FX held: unlocked, the FX layer");
    release(B_FX); frames(2); check(ui.layer == LY_PLAY, "and FX let go: back to playing");
    press(B_FX); frames(3); tap(B_HOME); release(B_FX); frames(3);
    press(B_HOME); frames(50); release(B_HOME); frames(2);
    check(ui.menu && ly_lock == LY_PLAY, "locked, HOME held: the menu, unlocked");
    ui.menu = 0; ui.force = 1; frames(2);
    song.g[G_DUST] = 0; song.g[G_FILT] = 0;

    /* ---- SEQ layer (PHEBUS, Ben 21 Nov 2026: "Remove them"): held, its keys set no step and play nothing; KNOB 2..4
     * are DIV, SWING, STEPS; the drum steps are made on the GRID (one hold of a step key: one undo) */
    song.sel = TRK_DRUM; go_home(); frame();
    press(B_SEQ); frames(10);
    {
        uint32_t ac = 0, v2;
        key(0); key(7); key(14);
        for (i = 0; i < 16u; i++) ac |= dstep_mask(&TDRUM->dstep[i]);
        for (v2 = 0; v2 < NVOICE; v2++) ac |= TDRUM->v[v2].active;
        check(!ac && keys_lit() == 0u && keys_guide() == 0u, "SEQ held + white keys: no step set, no sound, no key lit");
    }
    ppm("layer-steps");
    release(B_SEQ); check(ui.layer == LY_PLAY && !on_drum_page(), "SEQ used then let go: no page change");
    studio_open(SC_DRUM); frames(16); drum_lane = 2;   /* (the knobs quiet ~250 ms after a layer: #39) */
    key(0); key(7); key(14);                          /* steps 1, 5, 9 on the GRID */
    check(dstep_has(&TDRUM->dstep[0], 2) && dstep_has(&TDRUM->dstep[4], 2) && dstep_has(&TDRUM->dstep[8], 2) &&
          !dstep_has(&TDRUM->dstep[1], 2), "GRID + white keys 1, 5, 9: snare steps");
    fm1_in.notes = 1u << 7; frame();                  /* step 5 held + KNOB 1 / 2: level, ratchet (the GRID's) */
    encs[panel.enc[EN_K1]] = 1; frame();
    encs[panel.enc[EN_K2]] = 1; frame(); encs[panel.enc[EN_K2]] = 1; frame();
    fm1_in.notes = 0; frame();
    check(dstep_lvl(&TDRUM->dstep[4], 2) == LV_HARD && dstep_rat(&TDRUM->dstep[4], 2) == 2u,
          "GRID: step 5 held + KNOB 1 / 2: hard, x3");
    key(0); check(!dstep_has(&TDRUM->dstep[0], 2), "step 1 again: off");
    drum_lane = 0; go_home(); frame();

    /* ---- EDIT: undo / redo, erase, length */
    press(B_EDIT); frames(10);
    edges_btn |= BT(B_OCTDN); fm1_in.buttons |= BT(B_OCTDN); frame(); fm1_in.buttons &= ~BT(B_OCTDN); frame();
    check(dstep_has(&TDRUM->dstep[0], 2) && dstep_lvl(&TDRUM->dstep[4], 2) == LV_HARD, "EDIT + OCT-: undo (step 1 back)");
    edges_btn |= BT(B_OCTUP); fm1_in.buttons |= BT(B_OCTUP); frame(); fm1_in.buttons &= ~BT(B_OCTUP); frame();
    check(!dstep_has(&TDRUM->dstep[0], 2) && dstep_lvl(&TDRUM->dstep[4], 2) == LV_HARD, "EDIT + OCT+: redo (back, as left)");
    ppm("layer-erase");
    key(4);                                           /* stopped: every snare goes */
    check(!dstep_has(&TDRUM->dstep[4], 2) && !dstep_has(&TDRUM->dstep[8], 2), "EDIT + snare (stopped): every snare erased");
    encs[panel.enc[EN_K2]] = 1; frame();
    check(TDRUM->p[P_SLEN] == 32, "EDIT + KNOB 2: length x2");
    release(B_EDIT);
    edges_btn |= BT(B_EDIT); fm1_in.buttons |= BT(B_EDIT); frame();      /* undo the length too */
    edges_btn |= BT(B_OCTDN); fm1_in.buttons |= BT(B_OCTDN); frame(); fm1_in.buttons &= ~BT(B_OCTDN); frame();
    release(B_EDIT);
    check(TDRUM->p[P_SLEN] == 16, "EDIT + OCT-: the length back to 16");

    /* ---- ARP layer: a roll, rate knob */
    press(B_ARP); frames(10);
    encs[panel.enc[EN_K1]] = 1; frame();
    check(song.g[G_ROLL] == 2, "ARP + KNOB 1: the roll rate (1/32)");
    fm1_in.notes = 1u << 7; frames(3); check(roll[0].on, "ARP + a key: it rolls");
    ppm("layer-roll");
    fm1_in.notes = 0; frame(); check(!roll[0].on, "key up: the roll ends");
    release(B_ARP);

    /* ---- SEL: the key of the song; GLO: mute, solo, tap */
    song.sel = 0; go_home(); frame();
    press(B_SCL); frames(10);
    key(9);                                           /* D4 */
    check(trk[0].p[P_ROOT] == 2 && trk[1].p[P_ROOT] == 2 && trk[2].p[P_ROOT] == 2, "SEL + D: every part in D");
    encs[panel.enc[EN_K1]] = 2; frame();
    check(trk[0].p[P_CHORD] == 2, "SEL + KNOB 1: chords (7TH) on the track");
    ppm("layer-key");
    release(B_SCL);
    press(B_GLO); frames(10);
    key(0); check(trk[0].p[P_MUTE] == 1, "GLO + key 1: track 1 muted");
    key(9); check(song.solo == 2u, "GLO + key 6: track 2 soloed");
    ppm("layer-mix");
    {   /* review 15 (Ben, 21 Nov: "A: heads, underline", "A: badges"): the track strip under the knobs: a yellow M badge
         * on the muted track, an ice-blue S on the soloed one, the selected track underlined in its colour; the mixer's
         * S in the same ice blue, never the alien's */
        check(px_count(4, OP_BAR_Y, 56, OP_BAR_H, TE_MUTE) >= 60 && px_count(62, OP_BAR_Y, 56, OP_BAR_H, TE_MUTE) == 0,
              "strip: a yellow M badge on track 1 (muted), none on track 2");
        check(px_count(62, OP_BAR_Y, 56, OP_BAR_H, TE_SOLO) >= 60 && px_count(4, OP_BAR_Y, 56, OP_BAR_H, TE_SOLO) == 0,
              "strip: an ice-blue S badge on track 2 (soloed)");
        check(TE_SOLO == RGB(150, 215, 255) && TE_SOLO != TE_COL[1] && px_count(0, OP_BODY_Y, 240, 96, TE_SOLO) >= 60,
              "mixer: its S badge ice blue (150,215,255), not the alien's blue");
        check(px_count(4, OP_BAR_Y + 24, 54, 3, TE_COL[0]) == 54 * 3 && px_count(62, OP_BAR_Y + 24, 54, 3, TE_COL[1]) == 0,
              "strip: the selected track underlined in its colour, 3 px");
        check(px_count(0, OP_BAR_Y + 1, 240, OP_BAR_H - 1, C_WHITE) < 40, "strip: no filled tab (the old bar's look)");
    }
    for (i = 0; i < 4u; i++) { fm1_in.notes = 1u << 26; frame(); fm1_in.notes = 0; frames(36); }   /* ~0.6 s apart */
    check(song.g[G_BPM] >= 95 && song.g[G_BPM] <= 105, "GLO + the last key, tapped at ~0.6 s: ~100 BPM");
    key(0); key(9);
    check(!trk[0].p[P_MUTE] && !song.solo, "again: unmuted, no solo");
    /* (2.4) key 9 held: a fill; key 10: the next bar is one (again: cancelled) */
    fm1_in.notes = 1u << key_of_white(8); frame(); frame();
    check(fill_held == 1u && fill_now == 1u && (keys_lit() & (1u << key_of_white(8))) != 0, "GLO + key 9 held: FILL (the key lit)");
    ui.force = 1; frame(); ppm("layer-mix-fill");
    fm1_in.notes = 0; frame(); frame();
    check(fill_held == 0u && fill_now == 0u, "key 9 let go: the fill ends");
    key(key_of_white(9)); check(fill_arm == 1u && (keys_lit() & (1u << key_of_white(9))) != 0, "GLO + key 10: FILL NEXT BAR armed (the key lit)");
    key(key_of_white(9)); check(fill_arm == 0u, "GLO + key 10 again: cancelled");
    release(B_GLO);
    fm1_in.notes = 1u << key_of_white(8); frame();     /* (no layer: a plain key, no fill) */
    check(fill_held == 0u, "key 9 without GLO: no fill");
    fm1_in.notes = 0; frames(2);
    press(B_GLO); frames(45); release(B_GLO); frames(2);           /* (2026-10-28) ~0.7 s, nothing touched */
    check(ui.layer == LY_MIX && ly_lock == LY_MIX, "GLO held 0.7 s, untouched, let go: the mixer stays");
    key(0); check(trk[0].p[P_MUTE] == 1 && ui.layer == LY_MIX, "... its keys still mute (track 1)");
    key(0); check(!trk[0].p[P_MUTE], "... and unmute");
    tap(B_GLO); frames(2);
    check(ui.layer == LY_PLAY && ly_lock == LY_PLAY, "... GLO again: it goes (and no GLO pages open)");
    press(B_GLO); frames(45); encs[panel.enc[EN_K1]] = 1; frame(); release(B_GLO); frames(2);
    check(ly_lock == LY_PLAY && ui.layer == LY_PLAY, "GLO held with a knob turned: it goes on release, as before");
    press(B_GLO); frames(32); release(B_GLO); frames(2);
    check(ly_lock == LY_PLAY && ui.layer == LY_PLAY, "GLO held 0.5 s: a look, it goes on release");
    fm1_in.notes = 0; frame();
    trk[0].p[P_CHORD] = 0;

    /* ---- the lock parameter (PRESETS / ALGORITHM with GRID steps held, ui_grid.c): the sound knob last touched */
    check(ui.lock_par == P_ED_FLT, "the lock parameter starts as ENV > FLT");
    song.sel = 0; go_home(); frames(16);              /* (KNOB 1..4 quiet ~250 ms after a layer: #39) */
    open_family(FAM_FX); frame();                     /* the FX page: KNOB 1 = DIST */
    encs[panel.enc[EN_K1]] = 2; frame();
    check(ui.lock_par == P_DIST && trk[0].p[P_DIST] > 0, "a sound knob turned (FX > DIST): the lock parameter");
    open_family(FAM_SEQ); frame();                    /* PATTERN page knobs (LEN, DIV..) do not lock */
    open_family(FAM_SEQ); frame();
    if (cur_page()->id[1] == P_SDIV) { encs[panel.enc[EN_K2]] = 1; frame(); encs[panel.enc[EN_K2]] = -1; frame(); }
    check(ui.lock_par == P_DIST, "a pattern knob (DIV) is not a lock parameter");
    go_home(); frame();
    {   /* SEQ held (PHEBUS): KNOB 1 nothing, 2 DIV, 3 SWING, 4 STEPS; PRESETS / ALGORITHM / OCT change nothing of the
         * steps; the screen draws the steps in time and says how many bars they make */
        int16_t d0 = trk[0].p[P_SDIV], s0 = trk[0].p[P_SSWING], l0 = trk[0].p[P_SLEN], n0 = (int16_t)pen_note[0];
        uint32_t sel0 = song.sel, eng0 = trk[0].eng_req, pr0 = trk[0].preset, lit0, lit1;
        steps_clear(&trk[0]); trk[0].step[0].n = 1; trk[0].step[0].note[0] = 60; trk[0].step[0].time = ST_NOTE;
        press(B_SEQ); frames(10);
        ui.force = 1; frame();
        lit0 = px_count(0, 46, 240, 124, TE_COL[0]);
        check(lit0 > 0 && px_count(0, 46, 240, 124, TE_G2) > 0, "SEQ held: the steps in time (a set one filled, empty ones outlined)");
        check(!strcmp(sub_line(), "16 steps = 1 bar"), "SEQ held: the sub line says 16 steps = 1 bar");
        encs[panel.enc[EN_K1]] = 3; frame();
        check(trk[0].p[P_SDIV] == d0 && trk[0].p[P_SSWING] == s0 && trk[0].p[P_SLEN] == l0 && pen_note[0] == (uint8_t)n0,
              "SEQ held + KNOB 1: nothing (no pen note any more)");
        encs[panel.enc[EN_K2]] = -1; frame();
        check(trk[0].p[P_SDIV] == d0 - 1, "SEQ held + KNOB 2: DIV");
        ui.force = 1; frame(); ppm("layer-steps-div8");
        check(!strcmp(sub_line(), "16 steps = 2 bars"), "  DIV 1/8: 16 steps = 2 bars, drawn on two rows");
        encs[panel.enc[EN_K2]] = 1; frame();
        {   /* the swing moves the odd steps: step 2's left edge (one row: its middle at y 107) later than straight */
            uint32_t x, x0 = 0, x1 = 0;
            ui.force = 1; frame();
            for (x = 16; x < 60u && !x0; x++) if (swap16(screen[107 * 240 + x]) == TE_G2) x0 = x;
            encs[panel.enc[EN_K3]] = 50; frame();
            check(trk[0].p[P_SSWING] > s0, "SEQ held + KNOB 3: SWING");
            ui.force = 1; frame(); ppm("layer-steps-swing");
            for (x = 16; x < 60u && !x1; x++) if (swap16(screen[107 * 240 + x]) == TE_G2) x1 = x;
            check(x0 >= 18u && x0 <= 20u && x1 >= x0 + 3u, "  the odd steps drawn late by the swing (step 2 moves right)");
        }
        encs[panel.enc[EN_K4]] = 16; frame();
        check(trk[0].p[P_SLEN] > l0, "SEQ held + KNOB 4: STEPS");
        ui.force = 1; frame(); ppm("layer-steps-len");
        lit1 = px_count(0, 46, 240, 124, TE_G2);
        check(lit1 > 0 && strstr(sub_line(), "steps = 2 bars") != 0, "  more steps: drawn on, and the bars counted");
        encs[panel.enc[EN_PRESET]] = 1; frame(); encs[panel.enc[EN_ALGO]] = 1; frame();
        edges_btn |= BT(B_OCTUP); fm1_in.buttons |= BT(B_OCTUP); frame(); fm1_in.buttons &= ~BT(B_OCTUP); frame();
        check(trk[0].eng_req == eng0 && trk[0].preset == pr0 && song.sel == sel0 && !step_locked(&trk[0], 0) &&
              step_fill(&trk[0], 0) == FC_NORM, "SEQ held + PRESETS / ALGORITHM / OCT+: no sound, track, lock or condition change");
        release(B_SEQ); frames(2);
        trk[0].p[P_SDIV] = d0; trk[0].p[P_SSWING] = s0; trk[0].p[P_SLEN] = l0;
        steps_clear(&trk[0]);
    }
    fm1_irq_off(); steps_clear(TDRUM); steps_clear(&trk[0]); fm1_irq_on();
    trk[0].p[P_DIST] = 0;
    ui.lock_par = P_ED_FLT;
    song.sel = 0; go_home(); frame();

    /* ---- REC: press arms; held: the ring; to the end: cleared */
    go_home(); frame();
    song.playing = 0; song.rec = 0; rec_wait = 0;
    tap(B_REC); check(rec_wait == 1, "REC tapped (stopped): armed");
    ui.force = 1; frame(); ppm("live-rec-ready");
    tap(B_REC); check(rec_wait == 0, "REC again: cancelled");
    trk[0].step[3].n = 1, trk[0].step[3].note[0] = 60, trk[0].step[3].time = ST_NOTE;
    press(B_REC); frames(50);                         /* 0.8 s: the ring */
    check(ui.hold_kind == 1u && rec_wait == 0, "REC held: the press undone, the clear ring");
    ppm("hold-clear");
    frames(90);                                       /* to the end */
    check(!trk[0].step[3].n && ui.hold_kind == 0, "REC held to the end: track 1 cleared");
    release(B_REC);
    press(B_EDIT); frames(10);
    edges_btn |= BT(B_OCTDN); fm1_in.buttons |= BT(B_OCTDN); frame(); fm1_in.buttons &= ~BT(B_OCTDN); frame();
    release(B_EDIT);
    check(trk[0].step[3].n == 1, "EDIT + OCT-: the cleared track back");
    press(B_REC); frames(60); release(B_REC);         /* let go before the end: nothing */
    check(trk[0].step[3].n == 1 && !rec_wait, "REC let go before the end: nothing cleared");

    /* ---- SAVE: tap = the SAVE pages (PRESETS / USER / TOOLS) everywhere; held = the sections layer (A..D: play / store) */
    go_home(); frame();
    song.playing = 0; live_sec = -1; live_req = -1;
    press(B_SAVE); frames(15); check(ui.layer == LY_SONG, "SAVE held: the sections layer");
    key(key_of_white(4)); check(sec_stores == 0 && sec_armed == 1u, "store over a used A: asks again");
    key(key_of_white(4)); check(sec_stores == 1 && live_sec == 0, "again: the loop stored in A");
    key(key_of_white(6)); check(sec_stores == 2 && live_sec == 2, "an empty C: stored at once");
    key(key_of_white(1)); check(sec_loads == 1 && live_sec == 1, "stopped: B loaded as the loop");
    key(key_of_white(3)); check(sec_loads == 1, "an empty D: not played");
    song.playing = 1; clk_beat = 1; clk_pos = 0;
    key(key_of_white(0)); check(live_req == 0, "playing: A asked for the next bar");
    key(key_of_white(1)); check(live_req == 1, "B tapped in the same hold: B asked for instead (no chain)");
    ui.force = 1; frame(); ppm("layer-song");
    {   /* (PHEBUS wave 1: song mode, SONG REC and the SONG screen are cut) keys 13, 14, 16 do nothing */
        uint32_t st0 = sec_stores, ld0 = sec_loads, pg0 = ui.page;
        key(key_of_white(12)); key(key_of_white(13)); key(key_of_white(15));
        check(sec_stores == st0 && sec_loads == ld0 && live_req == 1 && ui.page == pg0 && ui.layer == LY_SONG,
              "SAVE held + keys 13, 14, 16: nothing (song mode, SONG REC and the song page are gone)");
        check(str_eq(LAYER_NAME[LY_SONG], "section") && str_eq(sub_line(), "next bar"), "the layer reads \"section\", playing: \"next bar\"");
    }
    release(B_SAVE); frames(2);
    check(live_req == 1 && saves == 0, "SAVE let go: B still asked for, nothing saved");
    live_req = -1; song.playing = 0;
    go_home(); frames(2);
    tap(B_SAVE); frames(2);
    check(!ui.home && cur_page()->fam == FAM_SAVE && str_eq(cur_page()->title, "PRESETS"), "SAVE tapped on HOME: PRESETS (one job a button)");
    song.sel = 0; studio_open(SC_DRUM); frames(2);
    tap(B_SAVE); frames(2);
    check(cur_page()->fam == FAM_SAVE && str_eq(cur_page()->title, "PRESETS"), "SAVE tapped on a synth GRID: PRESETS");
    song.sel = TRK_DRUM; studio_open(SC_DRUM); frames(2);
    tap(B_SAVE); frames(2);
    check(cur_page()->fam == FAM_SAVE && str_eq(cur_page()->title, "PRESETS"), "SAVE tapped on the drum GRID: PRESETS");
    {   /* no page is the old SONG screen */
        uint32_t i, none = 1;
        for (i = 0; i < NPAGES; i++) none &= !str_eq(PAGES[i].title, "SONG");
        check(none, "no SONG page left");
    }
    go_home(); frames(16);

    /* ---- the drum screen */
    song.sel = TRK_DRUM; studio_open(SC_DRUM); ui.force = 1; frames(16);   /* (after SAVE: the knobs quiet, #39) */
    check(on_drum_page(), "the drum screen");
    encs[panel.enc[EN_K3]] = 1; frame(); check(dstep_has(&TDRUM->dstep[0], 0), "DRUMS: KNOB 3 adds the kick on step 1");
    encs[panel.enc[EN_K4]] = -1; frame(); check(dstep_lvl(&TDRUM->dstep[0], 0) == LV_SOFT, "DRUMS: KNOB 4 softer");
    encs[panel.enc[EN_K1]] = 100; encs[panel.enc[EN_K2]] = 100; frame();
    check(drum_lane == 15 && drum_cursor == 15, "DRUMS: KNOB 1 / 2 bounded (16 sounds, 16 steps)");
    {   /* a scene to look at */
        static const uint8_t BEAT[16] = {0x11, 0x10, 0x10, 0x10, 0x14, 0x10, 0x10, 0x21, 0x11, 0x10, 0x01, 0x10, 0x1C, 0x10, 0x10, 0x20};
        uint32_t j;
        for (j = 0; j < 16u; j++) {
            memset(&TDRUM->dstep[j], 0, sizeof(dstep_t));
            if (BEAT[j] & 1u) dstep_set(&TDRUM->dstep[j], 0, LV_HARD, 0);
            if (BEAT[j] & 4u) dstep_set(&TDRUM->dstep[j], 2, LV_NORM, 0);
            if (BEAT[j] & 8u) dstep_set(&TDRUM->dstep[j], 3, LV_GHOST, 0);
            if (BEAT[j] & 0x10u) dstep_set(&TDRUM->dstep[j], 4, j % 4u ? LV_SOFT : LV_NORM, j == 14u ? 2u : 0u);
            if (BEAT[j] & 0x20u) dstep_set(&TDRUM->dstep[j], 5, LV_NORM, 0);
        }
        for (j = 0; j < 16u; j += 3u) { trk[0].step[j].n = 1; trk[0].step[j].note[0] = 36; trk[0].step[j].time = ST_NOTE; }
        for (j = 0; j < 16u; j += 4u) { trk[1].step[j].n = 3; trk[1].step[j].time = ST_NOTE; }
        drum_lane = 4; drum_cursor = 6; drum_page = 0; ui.force = 1; ui.msg_t = 0;
        drums.hits = 1u | 1u << 4; frame(); ppm("live-grid");
        drum_page = 1; ui.force = 1; drums.hits = 1u | 1u << 4; frame(); ppm("live-kit");
        drum_page = 0; song.sel = 1; go_home(); ui.force = 1;
        transport_req = 1; frames(30); song.rec = 2u; frame(); ui.force = 1; frame(); ppm("live-tracks");
        song.rec = 0; transport_req = 2; frames(2);
        song.sel = TRK_DRUM;
    }
    {   /* ---- the step GRID on every track (SPEC.md, 20 Nov 2026: Elektron style; ui_grid.c) */
        track_t *t = &trk[0];
        static track_t keep[NTRK];
        uint32_t k5 = key_of_lane(4), k1 = key_of_lane(0), a0, a1;
        memcpy(keep, trk, sizeof keep);
        song.sel = 0; go_home(); frames(16);
        steps_clear(t);
        t->p[P_SLEN] = 32; t->p[P_SCALE] = 2; t->p[P_ROOT] = 0;   /* (C minor) */
        pen_n = 1; pen_note[0] = 48;
        tap(B_SEQ); frames(2);
        check(on_drum_page() && kb_grid && grid_keys_on(), "GRID: HOME + SEQ on a synth track opens its GRID, the keys are steps");
        aud_on_left = 0;
        key(k1);
        check(step_on(&t->step[0]) && t->step[0].note[0] == 48 && t->step[0].vel == 100 && t->step[0].n == 1,
              "GRID: tap an empty step: set with the pen (C3), VEL 100");
        check(aud_on_left && aud_on_trk == 0u && aud_on_n == 1u && aud_on_nt[0] == 48u && grid_voice(t, 48),
              "GRID: ... and heard: C3 sounds on track 1 (REVIEW 1, play on change)");
        frames(16);
        check(!aud_on_left && !grid_voice(t, 48), "GRID: ... a short note: let go after ~150 ms");
        key(k1);
        check(!step_on(&t->step[0]), "GRID: tap a set step: cleared");
        fm1_in.notes |= 1u << k5; frame();                /* step 5 held */
        check(step_on(&t->step[4]) && sg_held == 1u << 4, "GRID: step 5 held: set, held");
        frames(16); aud_on_left = 0;
        encs[panel.enc[EN_K1]] = 1; frame();
        check(t->step[4].note[0] == 50, "GRID: step held + KNOB 1: NOTE one degree of the scale up (C3 -> D3)");
        frame(); check(aud_on_left && aud_on_nt[0] == 50u && grid_voice(t, 50), "GRID: ... D3 heard as it changes");
        encs[panel.enc[EN_K1]] = 1; frame();
        check(t->step[4].note[0] == 51, "GRID: KNOB 1 again: D3 -> D#3 (C minor)");
        encs[panel.enc[EN_K2]] = 1; frame();
        check(note_len(t, 4) == 2u && t->step[5].time == ST_TIE, "GRID: step held + KNOB 2: LEN 2 (a TIE after it)");
        encs[panel.enc[EN_K3]] = -10; frame();
        check(t->step[4].vel == 90, "GRID: step held + KNOB 3: VEL 90");
        encs[panel.enc[EN_K4]] = 1; frame();
        check(step_fill(t, 4) == FC_FILL, "GRID: step held + KNOB 4: COND fill only");
        ui.force = 1; frame(); ppm("grid-synth-held");
        a0 = (uint32_t)t->p[P_ATK];
        tap(B_ENV); frame();
        check(sg_lpage < NPAGES && PAGES[sg_lpage].fam == FAM_ENV && on_drum_page(),
              "GRID: step held + ENV: the ENV page locks, the GRID stays");
        encs[panel.enc[EN_K1]] = 3; frame();
        {
            int l = lock_find(t, 4, P_ATK, 0);
            check(l >= 0 && t->lock[l].val == (int32_t)a0 + 3 && t->p[P_ATK] == (int16_t)a0,
                  "GRID: step held + ENV + KNOB 1: an ATK lock on step 5, the track's ATK unchanged");
        }
        tap(B_FX); frame();
        check(sg_lpage < NPAGES && PAGES[sg_lpage].fam == FAM_FX, "GRID: step held + FX tapped: the FX page locks");
        a1 = (uint32_t)PAGES[sg_lpage].id[0];
        encs[panel.enc[EN_K1]] = 2; frame();
        check(lock_find(t, 4, a1, 0) >= 0, "GRID: FX + KNOB 1: its lock on step 5");
        ui.force = 1; frame(); ppm("grid-synth-lock");
        press(B_OCTDN); release(B_OCTDN); frame();
        check(lock_find(t, 4, a1, 0) < 0 && lock_find(t, 4, P_ATK, 0) >= 0, "GRID: OCT- on the FX lock page: its locks go, ATK's stays");
        {   /* REVIEW 3: EDIT with a step held walks OSC > FILT > ENV (the filter locks), the page named whole */
            uint32_t seen = 0, j, cut = 0;
            for (j = 0; j < 10u; j++) {
                tap(B_EDIT); frame();
                if (sg_lpage >= NPAGES) break;
                seen |= 1u << (PAGES[sg_lpage].fam == FAM_OSC ? 0 : PAGES[sg_lpage].fam == FAM_FLT ? 1 : PAGES[sg_lpage].fam == FAM_ENV ? 2 : 3);
                if (!cut && !strcmp(sg_lock_title(), "FILTER")) {
                    cut = 1;
                    encs[panel.enc[EN_K1]] = 5; frame();
                    ui.force = 1; frame(); ppm("grid-synth-lock-filter");
                }
            }
            check(seen == 7u, "GRID: step held + EDIT, again and again: OSC, FILTER and ENV pages (nothing else)");
            check(cut && lock_find(t, 4, P_E1, 0) >= 0 && t->lock[lock_find(t, 4, P_E1, 0)].val != t->p[P_E1],
                  "GRID: on FILTER, KNOB 1: a CUT lock on the step (the filter locks from the GRID)");
            {
                uint32_t j2;
                for (j2 = 0, j = 1; j2 < NPAGES; j2++)
                    if (sg_fam_in(PAGES[j2].fam, FAM_EDIT) || PAGES[j2].fam == FAM_FX || PAGES[j2].fam == FAM_LFO)
                        j &= text_w(&FONT_S, PAGES[j2].title) <= 122 - 46;
                check(j, "GRID: every lock page's name fits the header whole (no \"LOCK VOIC\")");
            }
        }
        ui.msg_t = 0;
        fm1_in.notes &= ~(1u << k5); frame();
        check(step_on(&t->step[4]) && !sg_held && sg_lpage == 0xFF, "GRID: step let go after edits: kept, the knobs back");
        {   /* a scene: an acid line over two pages */
            static const uint8_t NT[16] = {36, 0, 48, 36, 0, 39, 36, 0, 43, 0, 36, 46, 0, 36, 48, 0};
            uint32_t j;
            for (j = 0; j < 16u; j++) if (NT[j] && j != 4u) {
                memset(&t->step[j], 0, sizeof(step_t));
                t->step[j].n = 1; t->step[j].note[0] = NT[j]; t->step[j].time = ST_NOTE; t->step[j].vel = (uint8_t)(j % 4u ? 80 : 120);
                if (j == 11u) t->step[j].flags = SF_ACCENT | SF_SLIDE;
            }
            t->step[20].n = 3; t->step[20].note[0] = 48; t->step[20].note[1] = 51; t->step[20].note[2] = 55; t->step[20].time = ST_NOTE;
            t->micro[8] = -6;
        }
        ui.cursor = 4; ui.force = 1; frame(); ppm("grid-synth");
        transport_req = 1; frames(20); ui.force = 1; frame(); ppm("grid-synth-play");
        transport_req = 2; frames(2);
        fm1_in.notes |= 1u << k5; frame(); encs[panel.enc[EN_SELECT]] = 1; frame();
        check(sg_alt == 1u, "GRID: step held + SELECT: FLAG RATCH NUDGE on the knobs");
        encs[panel.enc[EN_K1]] = 1; frame(); encs[panel.enc[EN_K3]] = -4; frame();
        check((t->step[4].flags & SF_ACCENT) && t->micro[4] == -4, "GRID: FLAG ACC, NUDGE -4");
        ui.force = 1; frame(); ppm("grid-synth-flag");
        press(B_OCTDN); release(B_OCTDN);
        check(!step_locked(t, 4) && !step_fill(t, 4), "GRID: step held + OCT-: its locks, nudge and condition go");
        fm1_in.notes &= ~(1u << k5); frame(); ui.msg_t = 0;
        key(3);                                           /* the second black key: steps 17-32 */
        check(ui.cursor / 16u == 1u, "GRID: black key 2: the page of steps 17-32");
        ui.force = 1; frame(); ppm("grid-synth-p2");
        ui.msg_t = 0;
        press(B_OCTUP); release(B_OCTUP);                 /* REVIEW 4: past the last page, one added (a copy) */
        check(t->p[P_SLEN] == 48 && ui.cursor / 16u == 2u && !memcmp(&t->step[36], &t->step[20], sizeof(step_t)) &&
              t->step[36].n == 3u && !strcmp(ui.msg, "48 STEPS"),
              "GRID: OCT+ on the last page: a page added, a copy of the last, shown, \"48 STEPS\"");
        ui.force = 1; frame(); ppm("grid-synth-page-add");
        press(B_OCTUP); release(B_OCTUP);
        check(t->p[P_SLEN] == 64 && ui.cursor / 16u == 3u && !strcmp(ui.msg, "64 STEPS"), "GRID: OCT+ again: 64 STEPS");
        press(B_OCTUP); release(B_OCTUP);
        check(t->p[P_SLEN] == 64 && ui.cursor / 16u == 0u, "GRID: OCT+ at 64 steps: round to the first page");
        t->p[P_SLEN] = 32; ui.cursor = 4; frame();
        frames(16); aud_on_left = 0; pen_n = 1; pen_note[0] = 48;
        encs[panel.enc[EN_K1]] = 1; frames(2);
        check(pen_note[0] == 50 && aud_on_left && aud_on_nt[0] == 50u && grid_voice(t, 50),
              "GRID: KNOB 1 (no step held): the pen one degree up (D3), heard before it is placed");
        encs[panel.enc[EN_K1]] = -1; frames(16);
        encs[panel.enc[EN_K2]] = -3; frame();
        check(ui.cursor == 1u, "GRID: KNOB 2 (no step held): the cursor (step 5 -> step 2)");
        encs[panel.enc[EN_K3]] = 1; frame();
        check(step_on(&t->step[1]) && t->step[1].note[0] == 48, "GRID: KNOB 3 right: the pen on the cursor's step");
        encs[panel.enc[EN_K3]] = -1; frame();
        check(!step_on(&t->step[1]), "GRID: KNOB 3 left: cleared");
        tap(B_SEQ); frames(2);
        check(!on_drum_page() && !strcmp(PAGES[ui.page].title, "PATTERN"), "GRID, SEQ: the SEQ pages (PATTERN; the STEP page is gone)");
        open_family(FAM_ENV); frames(2);
        tap(B_SEQ); frames(2);
        check(on_drum_page() && kb_grid && song.sel == 0u, "REVIEW 2: SEQ on a sound page (ENV): back to the GRID");
        go_home(); frames(2);
        check(!kb_grid, "HOME: the keys play notes again");
        /* the drum grid: hold a step, as on the synths */
        song.sel = TRK_DRUM; studio_open(SC_DRUM); drum_page = 0; drum_lane = 0; drum_cursor = 0; frames(16);
        memset(&TDRUM->dstep[1], 0, sizeof(dstep_t));
        fm1_in.notes |= 1u << key_of_lane(1); frame();
        check(dstep_has(&TDRUM->dstep[1], 0) && sg_held == 2u, "DRUM GRID: an empty step pressed: set at once (as before)");
        encs[panel.enc[EN_K2]] = 1; frame();
        encs[panel.enc[EN_K3]] = 5; frame();
        check(dstep_rat(&TDRUM->dstep[1], 0) == 1u && TDRUM->micro[1] == 5, "DRUM GRID: step held + KNOB 2 RATCH x2, KNOB 3 NUDGE 5");
        ui.force = 1; frame(); ppm("grid-drum-held");
        fm1_in.notes &= ~(1u << key_of_lane(1)); frame();
        check(dstep_has(&TDRUM->dstep[1], 0), "DRUM GRID: let go after an edit: kept");
        key(key_of_lane(1));
        check(!dstep_has(&TDRUM->dstep[1], 0), "DRUM GRID: tapped again: cleared");
        tap(B_ENV); frames(2);                            /* REVIEW U14: ENV / LFO on the drum GRID as from HOME */
        check(!on_drum_page() && !strcmp(PAGES[ui.page].title, "HEX AEG"), "DRUM GRID: ENV opens the sound's envelope (HEX AEG), as from HOME");
        tap(B_SEQ); frames(2);
        check(on_drum_page() && drum_page == 0u, "DRUM: SEQ on HEX AEG: back to the GRID");
        ui.msg_t = 0; tap(B_LFO); frames(2);
        check(on_drum_page() && !strcmp(ui.msg, "NO LFO ON DRUMS"), "DRUM GRID: LFO says there is none (as from HOME)");
        drum_page = 1; tap(B_FX); frames(2);
        tap(B_SEQ); frames(2);
        check(on_drum_page() && drum_page == 0u, "DRUM: KIT left for FX, then SEQ: the GRID (not the KIT, no dead STEP page)");
        encs[panel.enc[EN_ALGO]] = -1; frames(2);
        check(song.sel == 2u && on_drum_page() && kb_grid, "GRID: ALGORITHM to a synth track: its GRID");
        song.sel = 0; go_home(); frames(16);
        memcpy(trk, keep, sizeof keep); sync_reload = 1;
        song.sel = TRK_DRUM;
    }
    {   /* ---- the four sequencer bugs of the review (REVIEW.md 11) and the full lock table (ENG-9) */
        track_t *t = &trk[0];
        static track_t keep[NTRK];
        uint32_t k3 = key_of_lane(2), k5 = key_of_lane(4), k6 = key_of_lane(5), k7 = key_of_lane(6), j, n, ok;
        memcpy(keep, trk, sizeof keep);
        song.playing = 0; song.sel = 0; go_home(); frames(16);
        steps_clear(t); for (j = 0; j < NLOCK; j++) t->lock[j].step = LOCK_FREE;
        t->p[P_SLEN] = 16; pen_n = 1; pen_note[0] = 48;
        {   /* (a) a step with every lock it can take: its lines stay inside their 31 bytes and the screen */
            char ln[4][31];
            for (j = 0, n = 0; j < P_COUNT && n < NLOCK; j++)
                if (p_lockable(j) && lock_desc(t, j)->max > lock_desc(t, j)->min) {
                    lock_set(t, 4, j, lock_desc(t, j)->max); n++;
                }
            n = sg_lines(t, 4, ln);
            for (j = 0, ok = n == 4u; j < n; j++)
                ok &= strlen(ln[j]) <= 30u && text_w(&FONT_S, ln[j]) <= 228;
            check(ok && strstr(ln[3], " +") != 0, "bug (a): 24 locks on a step: 4 lines, each <= 30 characters and 228 px, \"+\" for more");
            tap(B_SEQ); frames(2);
            fm1_in.notes |= 1u << k5; frame(); ui.force = 1; frame(); ppm("grid-synth-many-locks");
            fm1_in.notes &= ~(1u << k5); frame();
            go_home(); frames(2);
            for (j = 0; j < NLOCK; j++) t->lock[j].step = LOCK_FREE;
        }
        /* (b) a set step held, the GRID left (HOME), another track, the key let go: nothing cleared anywhere */
        trk[1].p[P_SLEN] = 16; steps_clear(&trk[1]);
        memset(&trk[1].step[2], 0, sizeof(step_t)); trk[1].step[2].n = 1; trk[1].step[2].note[0] = 60; trk[1].step[2].time = ST_NOTE; trk[1].step[2].vel = 100;
        memset(&t->step[2], 0, sizeof(step_t)); t->step[2].n = 1; t->step[2].note[0] = 50; t->step[2].time = ST_NOTE; t->step[2].vel = 100;
        tap(B_SEQ); frames(2);
        fm1_in.notes |= 1u << k3; frame();
        tap(B_HOME); frames(2);
        encs[panel.enc[EN_ALGO]] = 1; frames(2);
        fm1_in.notes &= ~(1u << k3); frames(2);
        check(song.sel == 1u && step_on(&trk[0].step[2]) && step_on(&trk[1].step[2]) && !sg_held,
              "bug (b): step held, GRID left, track changed, key let go: no step cleared on either track");
        song.sel = 0; go_home(); frames(4);
        /* (c) a TIE tapped: a note of one step there, the note before ends there (no tail taken) */
        steps_clear(t);
        tap(B_SEQ); frames(2);
        fm1_in.notes |= 1u << k5; frame();
        encs[panel.enc[EN_K2]] = 1; frame(); encs[panel.enc[EN_K2]] = 1; frame();
        fm1_in.notes &= ~(1u << k5); frame();
        check(note_len(t, 4) == 3u, "bug (c): set up: a note of 3 steps on step 5");
        key(k6);
        check(note_len(t, 4) == 1u && step_on(&t->step[5]) && note_len(t, 5) == 1u && t->step[6].time == ST_REST,
              "bug (c): its TIE (step 6) tapped: a note of 1 step there, step 5 ends before it, step 7 empty");
        /* (d) a layer button held until its layer shows (past SHOW_MS, before TAP_MS), let go: no tap */
        {
            uint32_t pg = ui.page;
            press(B_SEQ); frames((SHOW_MS + 40u) / 16u);
            check(ui.layer == LY_STEP, "bug (d): SEQ held 180 ms: its layer shows");
            release(B_SEQ); frames(2);
            check(on_drum_page() && ui.page == pg, "bug (d): ... let go: no tap (the GRID stays)");
            press(B_FX); frames((SHOW_MS + 40u) / 16u); release(B_FX); frames(2);
            check(on_drum_page() && ui.page == pg, "bug (d): FX held until shown, let go: no FX page");
            press(B_EDIT); frames((SHOW_MS + 40u) / 16u); release(B_EDIT); frames(2);
            check(on_drum_page() && ui.page == pg, "bug (d): EDIT held until shown, let go: no EDIT page");
            tap(B_FX); frames(2);
            check(!on_drum_page() && cur_page()->fam == FAM_FX, "bug (d): FX tapped quickly: still its page");
            go_home(); frames(2);
        }
        /* ENG-9: two steps held, one free lock slot: no lock made on either, and the screen says so */
        steps_clear(t); for (j = 0; j < NLOCK; j++) t->lock[j].step = LOCK_FREE;
        for (j = 0; j < NLOCK - 1u; j++) {
            t->lock[j].step = (uint8_t)(8u + j % 8u); t->lock[j].param = (uint8_t)(j < 8u ? P_DEC : j < 16u ? P_SUS : P_REL); t->lock[j].val = 1;
        }
        tap(B_SEQ); frames(2);
        fm1_in.notes |= 1u << k5 | 1u << k7; frames(2);
        tap(B_ENV); frame(); ui.msg_t = 0;
        encs[panel.enc[EN_K1]] = 3; frame();
        check(lock_find(t, 4, P_ATK, 0) < 0 && lock_find(t, 6, P_ATK, 0) < 0 && strstr(ui.msg, "LOCKS FULL") != 0,
              "ENG-9: lock table one short for two steps held: neither locked, \"LOCKS FULL\" shown");
        fm1_in.notes = 0; frames(2);
        go_home(); frames(4);
        /* REVIEW 7, full undo: a step with a note, length, locks, nudge and condition cleared by a tap; EDIT shows
         * "oct- undo"; EDIT + OCT- brings all of it back, EDIT + OCT+ clears it again */
        {
            step_t st0;
            int8_t m0;
            uint32_t f0, nl0, nl = 0;
            steps_clear(t); for (j = 0; j < NLOCK; j++) t->lock[j].step = LOCK_FREE;
            tap(B_SEQ); frames(2);
            fm1_in.notes |= 1u << k5; frame();
            encs[panel.enc[EN_K1]] = 2; frame(); encs[panel.enc[EN_K2]] = 1; frame();
            press(B_OCTUP); release(B_OCTUP);
            encs[panel.enc[EN_SELECT]] = 1; frame(); encs[panel.enc[EN_K3]] = 5; frame(); encs[panel.enc[EN_SELECT]] = -1; frame();
            tap(B_ENV); frame(); encs[panel.enc[EN_K1]] = 4; frame(); encs[panel.enc[EN_K2]] = 4; frame();
            fm1_in.notes &= ~(1u << k5); frames(2); ui.msg_t = 0;
            st0 = t->step[4]; m0 = t->micro[4]; f0 = step_fill(t, 4);
            for (j = 0, nl0 = 0; j < NLOCK; j++) nl0 += t->lock[j].step == 4u;
            check(step_on(&st0) && note_len(t, 4) == 2u && m0 == 5 && f0 == FC_FILL && nl0 == 2u, "undo: set up: note, LEN 2, NUDGE 5, FILL, 2 locks");
            key(k5);
            check(!step_on(&t->step[4]) && !t->micro[4] && !step_fill(t, 4) && !step_locked(t, 4), "undo: the step tapped: cleared, all of it");
            press(B_EDIT); frames((SHOW_MS + 40u) / 16u); ui.force = 1; frame();
            check(!strcmp(sub_line(), "oct- undo"), "undo: EDIT held: the screen says \"oct- undo\"");
            ppm("layer-erase-undo");
            edges_btn |= BT(B_OCTDN); fm1_in.buttons |= BT(B_OCTDN); frame(); fm1_in.buttons &= ~BT(B_OCTDN); frame();
            ui.force = 1; frame();
            for (j = 0; j < NLOCK; j++) nl += t->lock[j].step == 4u;
            check(!memcmp(&t->step[4], &st0, sizeof st0) && note_len(t, 4) == 2u && t->micro[4] == m0 && step_fill(t, 4) == f0 && nl == nl0,
                  "undo: EDIT + OCT-: the whole step back (note, LEN, locks, nudge, condition)");
            check(!strcmp(sub_line(), "oct+ redo"), "undo: then the screen says \"oct+ redo\"");
            edges_btn |= BT(B_OCTUP); fm1_in.buttons |= BT(B_OCTUP); frame(); fm1_in.buttons &= ~BT(B_OCTUP); frame();
            check(!step_on(&t->step[4]) && !step_locked(t, 4) && !step_fill(t, 4), "undo: EDIT + OCT+: redo, cleared again");
            release(B_EDIT); frames(20);                   /* (past LY_QUIET_MS) */
            go_home(); frames(4);
        }
        memcpy(trk, keep, sizeof keep); sync_reload = 1;
        song.sel = TRK_DRUM;
    }
    {   /* REVIEW 4: the GRID follows the playhead onto the next page; a page picked by hand stays until it moves on */
        track_t *t = &trk[0];
        static track_t keep[NTRK];
        uint32_t j;
        memcpy(keep, trk, sizeof keep);
        song.sel = 0; go_home(); frames(16);
        steps_clear(t); t->p[P_SLEN] = 32;
        tap(B_SEQ); frames(2);
        ui.cursor = 3;
        transport_req = 1;
        for (j = 0; j < 400u && t->seq_idx % 32u < 17u; j++) frame();
        check(song.playing && t->seq_idx % 32u >= 16u && ui.cursor / 16u == 1u, "GRID, playing: the view follows the playhead onto page 2");
        ui.force = 1; frame(); ppm("grid-synth-follow");
        key(1);                                           /* (F#3: the first black key: page 1 by hand) */
        frames(4);
        check(t->seq_idx % 32u >= 16u && ui.cursor / 16u == 0u, "GRID, playing: page 1 picked by hand stays while the playhead is on 2");
        for (j = 0; j < 400u && t->seq_idx % 32u >= 16u; j++) frame();
        frames(2);
        check(t->seq_idx % 32u < 16u && ui.cursor / 16u == 0u, "  ... and the playhead back on page 1: page 1");
        for (j = 0; j < 400u && t->seq_idx % 32u < 17u; j++) frame();
        check(ui.cursor / 16u == 1u, "  ... onto page 2 again: page 2");
        transport_req = 2; frames(4);
        go_home(); frames(4);
        memcpy(trk, keep, sizeof keep); sync_reload = 1;
        song.sel = TRK_DRUM;
    }
    {   /* PHEBUS (Ben, 21 Nov 2026, picker "Yes, add it"): STEP REC on a synth track's GRID. REC, stopped: the keys play,
         * a note or a chord lands on the cursor's step and the cursor moves on; SEQ a rest, ARP a tie (or a key held + SEQ);
         * round at the end; SELECT / KNOB 2 move back; one undo the whole pass; REC again leaves; playing: live */
        track_t *t = &trk[1];
        static track_t keep[NTRK];
        uint32_t ka = key_of_lane(0), kb = key_of_lane(2), kc = key_of_lane(4), na, nb, nc, j, empty = 1;
        memcpy(keep, trk, sizeof keep);
        song.sel = 1; go_home(); frames(16);
        steps_clear(t); t->p[P_SLEN] = 16; t->p[P_CHORD] = 0; song.octave = 0;
        tap(B_SEQ); frames(2);
        ui.cursor = 0; frame();                           /* (STEP REC starts at the GRID's cursor) */
        tap(B_REC); frames(2);
        check(sr_on && !kb_grid && !song.rec && !rec_wait && on_drum_page() && !strcmp(ui.msg_t ? "" : "x", "x") ,
              "STEP REC: REC on a synth GRID, stopped: STEP REC on, the keys are notes again, nothing armed");
        ui.msg_t = 0;
        na = kb_map(t, ka); nb = kb_map(t, kb); nc = kb_map(t, kc);
        fm1_in.notes |= 1u << ka; frames(2);
        check(grid_voice(t, na) && !step_on(&t->step[0]), "STEP REC: a key held sounds; nothing written until it comes up");
        fm1_in.notes &= ~(1u << ka); frames(2);
        check(step_on(&t->step[0]) && t->step[0].note[0] == na && t->step[0].n == 1u && t->step[0].vel == 100u && ui.cursor == 1u,
              "STEP REC: a note let go: written on step 1 (VEL 100), the cursor on step 2");
        fm1_in.notes |= 1u << kb; frame(); fm1_in.notes |= 1u << kc; frames(2);
        ui.force = 1; frame(); ppm("grid-step-rec-held");
        fm1_in.notes &= ~(1u << kb); frames(2);
        check(!step_on(&t->step[1]) && ui.cursor == 1u, "STEP REC: a chord: one key up, the other held: not written yet");
        fm1_in.notes &= ~(1u << kc); frames(2);
        check(step_on(&t->step[1]) && t->step[1].n == 2u && t->step[1].note[0] == nb && t->step[1].note[1] == nc && ui.cursor == 2u,
              "STEP REC: ... the last key up: the chord (2 notes, polyphonic voice) on step 2, the cursor on 3");
        t->step[2].time = ST_NOTE; t->step[2].n = 1; t->step[2].note[0] = 70;
        tap(B_SEQ); frames(2);
        check(!step_on(&t->step[2]) && t->step[2].time == ST_REST && ui.cursor == 3u && sr_on && on_drum_page(),
              "STEP REC: SEQ: a rest (step 3's note gone), the cursor on 4; the SEQ pages do not open");
        tap(B_ARP); frames(2);
        check(ui.cursor == 3u && !strcmp(ui.msg, "NO NOTE TO TIE") && ui.layer == LY_PLAY, "STEP REC: ARP after a rest: \"NO NOTE TO TIE\", no move, no ROLL layer");
        ui.msg_t = 0;
        key(ka); frames(2);
        tap(B_ARP); frames(2);
        check(t->step[3].note[0] == na && t->step[4].time == ST_TIE && note_len(t, 3) == 2u && ui.cursor == 5u,
              "STEP REC: a note then ARP: tied, two steps long, the cursor on 6");
        fm1_in.notes |= 1u << kb; frames(2);
        tap(B_SEQ); frames(2);
        check(t->step[5].note[0] == nb && t->step[6].time == ST_TIE && ui.cursor == 7u, "STEP REC: a key held + SEQ: written, then tied");
        fm1_in.notes &= ~(1u << kb); frames(2);
        check(ui.cursor == 7u && !step_on(&t->step[7]), "STEP REC: ... let go: nothing more written");
        press(B_OCTUP); release(B_OCTUP);
        check(song.octave == 1, "STEP REC: OCT+: the octave, as on HOME");
        key(ka); frames(2);
        check(t->step[7].note[0] == na + 12u && ui.cursor == 8u, "STEP REC: ... the note an octave up on step 8");
        press(B_OCTDN); release(B_OCTDN);
        encs[panel.enc[EN_K2]] = 20; frame();
        check(ui.cursor == 15u, "STEP REC: KNOB 2: the cursor (to the last step, no further)");
        key(kc); frames(2);
        check(t->step[15].note[0] == nc && ui.cursor == 0u, "STEP REC: a note on the last step: the cursor round to step 1");
        t->p[P_SLEN] = 32; ui.cursor = 15; frame();
        key(kb); frames(2);
        check(t->step[15].note[0] == nb && ui.cursor == 16u && sg_page() == 1u, "STEP REC: 32 steps: on from 16 onto page 2");
        ui.force = 1; frame(); ppm("grid-step-rec-p2");
        encs[panel.enc[EN_SELECT]] = -1; frame();
        check(ui.cursor == 15u && sg_page() == 0u, "STEP REC: SELECT left: the cursor back a step (page 1)");
        encs[panel.enc[EN_K2]] = -15; frame();
        check(ui.cursor == 0u, "STEP REC: KNOB 2 left: back to step 1 to fix it");
        key(kc); frames(2);
        check(t->step[0].note[0] == nc && t->step[0].n == 1u && ui.cursor == 1u, "STEP REC: ... the step replaced");
        encs[panel.enc[EN_K3]] = -20; frame();
        check(sr_vel == 80u, "STEP REC: KNOB 3: the velocity written (80)");
        encs[panel.enc[EN_K4]] = -16; frame();
        check(t->p[P_SLEN] == 16, "STEP REC: KNOB 4: STEPS (32 -> 16)");
        sr_vel = 100;
        ui.force = 1; frame(); ppm("grid-step-rec");
        press(B_EDIT); press(B_OCTDN); release(B_OCTDN); release(B_EDIT); frames(2);
        for (j = 0; j < 32u; j++) empty &= !step_on(&t->step[j]) && t->step[j].time != ST_TIE;
        check(empty && t->p[P_SLEN] == 16 && !strcmp(ui.msg, "UNDO"), "STEP REC: EDIT + OCT-: the whole pass undone (the empty 16 steps it started from)");
        press(B_EDIT); press(B_OCTUP); release(B_OCTUP); release(B_EDIT); frames(2);
        check(t->step[0].note[0] == nc && t->step[5].note[0] == nb, "STEP REC: EDIT + OCT+: redone");
        ui.msg_t = 0;
        tap(B_REC); frames(2);
        check(!sr_on && kb_grid && !song.rec && !rec_wait, "STEP REC: REC again: off, the keys are steps again");
        key(ka); frames(2);
        check(!step_on(&t->step[1]) || t->step[1].n == 2u, "STEP REC: ... a key is a step again (step 2 untouched by a note)");
        transport_req = 1; frames(4);
        tap(B_REC); frames(2);
        check(song.rec && !sr_on && !kb_grid && !strcmp("LIVE REC", "LIVE REC"), "STEP REC: playing, REC: a live take as before (the keys play notes on the GRID)");
        fm1_in.notes |= 1u << ka; frames(6); fm1_in.notes &= ~(1u << ka); frames(2);
        ui.force = 1; frame(); ppm("grid-live-rec");
        tap(B_REC); frames(2);
        check(!song.rec && !sr_on && kb_grid, "STEP REC: playing, REC again: the take ends, the keys are steps");
        transport_req = 2; frames(4);
        tap(B_REC); frames(2);
        transport_req = 1; frames(8);
        check(sr_on && song.playing && !song.rec, "STEP REC: PLAY while in STEP REC: it plays, STEP REC stays");
        transport_req = 2; frames(4);
        go_home(); frames(2);
        check(!sr_on && !ly_off, "STEP REC: the GRID left: STEP REC ends");
        tap(B_SEQ); frames(2);
        press(B_REC); frames(50);
        check(!sr_on && ui.hold_kind == 1u, "STEP REC: REC held: the clear ring, STEP REC undone");
        release(B_REC); frames(2);
        song.octave = 0; go_home(); frames(4);
        memcpy(trk, keep, sizeof keep); sync_reload = 1;
        song.sel = TRK_DRUM;
    }
    {   /* REVIEW 13: the STRIKE pose on the sequencer's ACC steps: UNISON (velocity scaled under the threshold) and a
         * legato ACC + SLIDE (no new voice) too */
        track_t *t = &trk[0];
        static track_t keep[NTRK];
        uint32_t j, hit_uni = 0, hit_leg = 0, hit_plain = 0, ti = 0;
        memcpy(keep, trk, sizeof keep);
        for (j = 0; j < NPART; j++) {                     /* (a track whose UNISON stacks voices: their velocity scaled) */
            int16_t v0 = trk[j].p[P_VOICE];
            trk[j].p[P_VOICE] = V_UNISON;
            if (trk_nvoice(&trk[j]) > 1u) { ti = j; trk[j].p[P_VOICE] = v0; break; }
            trk[j].p[P_VOICE] = v0;
        }
        t = &trk[ti];
        song.sel = (uint8_t)ti; go_home(); frames(4);
        steps_clear(t); t->p[P_SLEN] = 16;
        for (j = 0; j < 16u; j += 4u) {
            memset(&t->step[j], 0, sizeof(step_t));
            t->step[j].n = 1; t->step[j].note[0] = 48; t->step[j].time = ST_NOTE; t->step[j].vel = 80;
        }
        t->p[P_VOICE] = V_UNISON;
        frames(4);
        transport_req = 1;
        for (j = 0; j < 140u; j++) { frame(); hit_plain |= op_strike[ti] != 0; }
        transport_req = 2; frames(16);
        t->step[4].flags = SF_ACCENT;
        transport_req = 1;
        for (j = 0; j < 140u; j++) { frame(); hit_uni |= op_strike[ti] != 0; }
        transport_req = 2; frames(16);
        t->p[P_VOICE] = V_LEGATO;
        t->step[4].flags = SF_SLIDE;                      /* step 5 slides into an ACC step 6 */
        memset(&t->step[5], 0, sizeof(step_t));
        t->step[5].n = 1; t->step[5].note[0] = 55; t->step[5].time = ST_NOTE; t->step[5].vel = 80; t->step[5].flags = SF_ACCENT | SF_SLIDE;
        for (j = 0; j < 16u; j += 4u) t->step[j].vel = 80;
        transport_req = 1;
        for (j = 0; j < 140u; j++) { frame(); hit_leg |= op_strike[ti] != 0; }
        transport_req = 2; frames(16);
        check(!hit_plain, "STRIKE: no ACC step, soft notes: no pose");
        check(hit_uni, "STRIKE: UNISON, an ACC step: the pose (from the sequencer's accent)");
        check(hit_leg, "STRIKE: LEGATO, a slide into an ACC + SLIDE step: the pose");
        go_home(); frames(4);
        memcpy(trk, keep, sizeof keep); sync_reload = 1;
        song.sel = TRK_DRUM;
    }
    {   /* the REC screen (the free take is cut, PHEBUS wave 1: an empty project records at the tempo set, no MODE) */
        static track_t keep[NTRK];
        uint32_t lw;
        memcpy(keep, trk, sizeof keep);
        for (i = 0; i < NTRK; i++) steps_clear(&trk[i]);
        rec_count = 0;
        rec_wait = 1; ui.force = 1; frame(); ppm("live-rec-empty");
        check(px_count(0, 42, 240, 64, 0) < 240u * 64u, "REC screen, empty project: drawn (play a note: it starts the loop)");
        /* the REC screen's dials (2.3): KNOB 2 LENGTH, KNOB 3 START; KNOB 1 nothing */
        lw = lights_word();
        encs[panel.enc[EN_K1]] = 1; frame();
        check(lights_word() == lw && rec_wait, "REC screen, empty project: KNOB 1 does nothing (no MODE)");
        trk[song.sel].p[P_SLEN] = 16;
        encs[panel.enc[EN_K2]] = 1; frame();
        encs[panel.enc[EN_K2]] = 1; frame();
        check(trk[song.sel].p[P_SLEN] == 64, "REC screen: KNOB 2 -> LENGTH 1, 2, 4 bars");
        encs[panel.enc[EN_K2]] = -1; frame();
        check(trk[song.sel].p[P_SLEN] == 32, "REC screen: KNOB 2 back -> 2 bars");
        encs[panel.enc[EN_K3]] = 1; frame();
        check(rec_count == 1u, "REC screen: KNOB 3 -> START count");
        ui.force = 1; frame(); ppm("live-rec-count");
        check((lights_word() >> 9 & 3u) == 2u, "REC screen: START saved with the settings (bit 9, KARMA's MODE, written 0)");
        lights_from_word(lights_word() | 1u << 9);
        check((lights_word() >> 9 & 1u) == 0u, "the settings' old MODE bit: read and ignored");
        ci_on = 1; ci_beat = 1; ui.force = 1; frame(); ppm("live-rec-countin");
        encs[panel.enc[EN_K3]] = -1; frame();
        check(rec_count == 1u, "REC screen: the count-in running, the dials wait");
        ci_on = 0; ci_beat = 0;
        trk[1].step[0].time = ST_NOTE; trk[1].step[0].n = 1; trk[1].step[0].note[0] = 60;
        ui.force = 1; frame(); ppm("live-rec-notes");
        steps_clear(&trk[1]);
        encs[panel.enc[EN_K3]] = -1; frame();
        check(rec_count == 0u, "REC screen: KNOB 3 back -> note");
        trk[song.sel].p[P_SLEN] = 16;
        memcpy(trk, keep, sizeof keep);
        rec_wait = 0; frame();
    }
    for (i = 0; i < DRUM_KITS; i++) { TDRUM->p[P_E0] = (int16_t)i; ui.force = 1; drum_page = 1; frame(); }
    drum_page = 0;

    {   /* menu NOTES (PR #11 by @renebohne): sounding synth voices light their keys */
        song.sel = 0; go_home(); ui.force = 1; frame();
        trk[0].p[P_CHORD] = 0; trk[0].p[P_QUANT] = 0; trk[0].p[P_ROOT] = 0; trk[0].p[P_TRANS] = 0;
        song.octave = 0;
        lights_notes = 0;
        trk[0].v[0].active = 1; trk[0].v[0].gate = 1; trk[0].v[0].stage = 1; trk[0].v[0].note = 60;
        frame();
        check((keys_lit() & (1u << 7)) == 0, "NOTES off: a sounding synth voice lights no key");
        ui.menu = 1; ui.menu_sel = MI_NOTES; ui.force = 1; frame();
        encs[panel.enc[EN_K1 + mi_row(ui.menu_sel)]] = 1; frame();
        check(lights_notes == 1u, "menu NOTES: its knob (KNOB 3) right: ON");
        ui.force = 1; frame(); ppm("page-menu-notes");
        ui.menu = 0; ui.force = 1; go_home(); frame();
        check((keys_lit() & (1u << 7)) != 0, "NOTES on: a sounding synth voice lights key 7 (C4)");
        {   /* a short note (a sequencer 1/16) that starts and ends between two frames still lights its key */
            int32_t o[CTL * 2];
            uint32_t n = 0, f;
            trk[0].v[0].active = 0; trk[0].v[0].gate = 0;
            frame(); frame();
            check((keys_lit() >> 9 & 1u) == 0, "NOTES on: D4 not lit before it plays");
            trk_note_on(&trk[0], 62, 100);
            mix_block(o, CTL);
            trk_note_off(&trk[0], 62);
            for (f = 0; f < 20; f++) mix_block(o, CTL);
            frame();
            check((keys_lit() >> 9 & 1u) != 0, "NOTES on: a note over before the frame still lights its key (D4)");
            for (f = 0; f < 10; f++) { frame(); n += keys_lit() >> 9 & 1u; }
            check(n >= 4u && n <= 7u && (keys_lit() >> 9 & 1u) == 0, "NOTES on: ... for about 6 frames, then off");
            song.sel = 1; frame(); song.sel = 0; frame();
            trk[0].v[0].active = 1; trk[0].v[0].gate = 1; trk[0].v[0].stage = 1; trk[0].v[0].note = 60;
        }
        {   /* every screen and layer: lit where the keys are notes, a glow under the tiles */
            static const struct { uint32_t ly; int lit; const char *name; } L[] = {
                {LY_ERASE, 1, "EDIT erase"}, {LY_ROLL, 1, "ARP roll"}, {LY_SCALE, 1, "SEL key"}, {LY_SONG, 1, "SAVE song"},
                {LY_FX, 0, "FX punch"}, {LY_MIX, 0, "GLO mix"}};
            uint32_t k, ok = 1, ly0 = ui.layer;
            char what[96];
            for (k = 0; k < sizeof L / sizeof L[0]; k++) {
                ui.layer = (uint8_t)L[k].ly;
                if (L[k].lit)
                    ok = (keys_lit() >> 7 & 1u) != 0;
                else
                    ok = (keys_notes_dim() >> 7 & 1u) != 0 && (keys_lit() >> 7 & 1u) == 0;
                snprintf(what, sizeof what, "NOTES on, %s layer: the sounding C4 %s", L[k].name, L[k].lit ? "lit" : "glows under the tiles");
                check(ok, what);
            }
            ui.layer = LY_STEP;
            check(keys_lit() == 0u && keys_notes_dim() == 0u, "NOTES on, SEQ held: no key lit or glowing (its keys do nothing)");
            ui.layer = LY_SCALE;
            check((keys_notes_dim() & scale_keys(0)) == scale_keys(0), "NOTES on, SEL: the scale glows");
            lights_notes = 0;
            ui.layer = LY_ERASE;
            check((keys_lit() >> 7 & 1u) == 0 && keys_notes_dim() == 0u, "NOTES off, EDIT erase: as before (no note lights)");
            ui.layer = LY_SCALE;
            check(keys_notes_dim() == 0u, "NOTES off, SEL: as before");
            lights_notes = 1;
            ui.layer = (uint8_t)ly0;
        }
        song.octave = -1;
        frame();
        check((keys_lit() & (1u << 19)) != 0 && (keys_lit() & (1u << 7)) == 0, "NOTES on: the octave moves the lit key");
        song.octave = 0;
        trk[0].v[0].gate = 0; trk[0].v[0].stage = 3;
        frame();
        check((keys_lit() & (1u << 7)) == 0, "NOTES on: the release turns the key off");
        trk[0].v[0].active = 0;
        check((lights_word() >> 8 & 1u) == 1u, "NOTES: saved with the light settings");
        ui.menu = 1; ui.menu_sel = MI_USB; ui.force = 1; frame(); ppm("menu-usb");
        usb_full = 0;
        encs[panel.enc[EN_K1 + mi_row(ui.menu_sel)]] = 1; frame();
        check(usb_full == 1u && (lights_word() >> 11 & 1u) == 1u, "menu USB AUDIO: its knob (KNOB 2) -> FULL, saved with the settings");
        tap(B_OCTUP);
        check(usb_full == 0u && ui.menu == 1, "menu USB AUDIO: OCT+ toggles back to MASTER");
        ui.menu_sel = MI_SERIAL; ui.force = 1; frame();
        check(usb_serial == 0u && !(lights_word() >> 16 & 1u), "menu USB SERIAL: OFF by default (2.3 settings read so)");
        encs[panel.enc[EN_K1 + mi_row(ui.menu_sel)]] = 1; frame();
        check(usb_serial == 1u && (lights_word() >> 16 & 1u), "menu USB SERIAL: its knob (KNOB 3) -> ON, saved with the settings");
        ui.force = 1; frame(); ppm("menu-serial");
        lights_from_word(lights_word());
        check(usb_serial == 1u, "menu USB SERIAL: read back");
        tap(B_OCTUP);
        check(usb_serial == 0u && ui.menu == 1, "menu USB SERIAL: OCT+ toggles back to OFF");
        ui.menu = 0; ui.force = 1; go_home(); frame();
        ui.menu = 1; ui.menu_sel = MI_NOTES; ui.force = 1; frame();
        tap(B_OCTUP);
        check(lights_notes == 0u && ui.menu == 1, "menu NOTES: OCT+ toggles it off");
        ui.menu = 0; ui.force = 1; go_home(); frame();
    }

    {   /* menu LIGHTS / KEYS: the backlight for playing in the dark */
        uint32_t m, nbits;
        go_home(); ui.force = 1; frame();
        check(lights_lvl == LIGHTS_OFF && fm1_led_bg_ns == 0u && lights_keys_mask() == 0u, "LIGHTS: off by default");
        ui.menu = 1; ui.menu_sel = MI_LIGHTS; ui.force = 1; frame();
        encs[panel.enc[EN_K1 + mi_row(ui.menu_sel)]] = 1; frame();
        check(lights_lvl == LIGHTS_LOW && fm1_led_bg_ns == 500u, "LIGHTS: KNOB 1 right: LOW (a 0.5 us pulse a frame)");
        encs[panel.enc[EN_K1 + mi_row(ui.menu_sel)]] = 1; frame();
        check(lights_lvl == LIGHTS_MID && fm1_led_bg_ns == 1000u, "LIGHTS: MID (1 us)");
        encs[panel.enc[EN_K1 + mi_row(ui.menu_sel)]] = 1; frame(); encs[panel.enc[EN_K1 + mi_row(ui.menu_sel)]] = 1; frame();
        check(lights_lvl == LIGHTS_HIGH && fm1_led_bg_ns == 2000u, "LIGHTS: HIGH (2 us, under the 4 us glow), and it stops there");
        ui.force = 1; frame(); ppm("page-menu-lights");
        encs[panel.enc[EN_K1 + mi_row(ui.menu_sel)]] = -1; frame();
        check(lights_lvl == LIGHTS_MID, "LIGHTS: KNOB 1 left: dimmer");
        tap(B_OCTUP);
        check(lights_lvl == LIGHTS_HIGH && ui.menu == 1, "LIGHTS: OCT+ steps round, the menu stays");
        tap(B_OCTUP);
        check(lights_lvl == LIGHTS_OFF && fm1_led_bg_ns == 0u, "LIGHTS: ... back to OFF");
        ui.menu_sel = MI_KEYS; ui.force = 1; frame();
        encs[panel.enc[EN_K1 + mi_row(ui.menu_sel)]] = 1; frame();
        m = lights_keys_mask();
        check(lights_keys == KEYS_C && lights_lvl == LIGHTS_LOW && m == (1u << 7 | 1u << 19),
              "KEYS (KNOB 2): C KEYS (C4, C5), LIGHTS raised to LOW");
        encs[panel.enc[EN_K1 + mi_row(ui.menu_sel)]] = 1; frame();
        for (m = lights_keys_mask(), nbits = 0; m; m &= m - 1u)
            nbits++;
        check(lights_keys == KEYS_WHITE && nbits == 16u && !(lights_keys_mask() & (1u << 1)),
              "KEYS: WHITE KEYS (16 of 27, F#3 dark)");
        encs[panel.enc[EN_K1 + mi_row(ui.menu_sel)]] = 1; frame(); encs[panel.enc[EN_K1 + mi_row(ui.menu_sel)]] = 1; frame();
        check(lights_keys == KEYS_ALL && lights_keys_mask() == 0x7FFFFFFu, "KEYS: ALL KEYS (every one of the 27 keys), and it stops there");
        ui.force = 1; frame(); ppm("page-menu-keys-all");
        tap(B_OCTUP);
        check(lights_keys == KEYS_OFF, "KEYS: OCT+ from ALL KEYS steps round to OFF");
        lights_keys = KEYS_ALL;
        check(lights_word() >> 4 & 15u, "KEYS: ALL KEYS kept in the settings word");
        lights_from_word(lights_word());
        check(lights_keys == KEYS_ALL, "KEYS: ALL KEYS read back");
        lights_lvl = LIGHTS_OFF; frame();
        check(lights_keys_mask() == 0u && fm1_led_bg_ns == 0u, "KEYS: nothing lit while LIGHTS is OFF");
        lights_keys = KEYS_OFF;
        ui.menu = 0; ui.force = 1; go_home(); frame();
    }

    {   /* the menu in sections (2.4): SELECT the section, KNOB 1..3 its rows, PRESETS the cursor, OCT+ opens */
        uint32_t sec;
        ui.menu = 1; ui.menu_sel = 0; ui.force = 1; frame(); ppm("menu-screen");
        check(mi_sec(ui.menu_sel) == MS_SCREEN, "menu: opens on SCREEN (COLOR, ZOOM)");
        encs[panel.enc[EN_SELECT]] = 1; frame();
        check(mi_sec(ui.menu_sel) == MS_LIGHTS && ui.menu_sel == MI_LIGHTS, "menu: SELECT right: LIGHTS, the cursor on its first row");
        encs[panel.enc[EN_K2]] = 1; frame(); encs[panel.enc[EN_K2]] = 1; frame();
        check(lights_keys == KEYS_WHITE && ui.menu_sel == MI_KEYS, "menu LIGHTS: KNOB 2 sets KEYS (row 2), the cursor follows");
        encs[panel.enc[EN_K3]] = 1; frame();
        check(lights_notes == 1u && ui.menu_sel == MI_NOTES, "menu LIGHTS: KNOB 3 sets NOTES (row 3)");
        encs[panel.enc[EN_K4]] = 1; frame();
        check(ui.menu_sel == MI_NOTES, "menu: KNOB 4 on a section of three rows does nothing");
        ui.force = 1; frame(); ppm("menu-lights");
        encs[panel.enc[EN_PRESET]] = 1; frame();
        check(ui.menu_sel == MI_LIGHTS, "menu: PRESETS moves the cursor round the section's rows");
        encs[panel.enc[EN_SELECT]] = 1; frame(); ui.force = 1; frame(); ppm("menu-audio");
        check(mi_sec(ui.menu_sel) == MS_AUDIO, "menu: SELECT right: AUDIO (LOWCUT, USB AUDIO, USB SERIAL)");
        encs[panel.enc[EN_SELECT]] = 5; frame();
        sec = mi_sec(ui.menu_sel);
        check(sec == MS_SYSTEM, "menu: SELECT stops at the last section, SYSTEM");
        encs[panel.enc[EN_K2]] = 1; frame();
        check(ui.menu == 1 && ui.menu_sel == MI_LCLEAR, "menu SYSTEM: KNOB 2 is MIDI LEARN CLEAR (2.6), nothing cleared by a turn");
        ui.force = 1; frame(); ppm("menu-system");
        encs[panel.enc[EN_K3]] = 1; frame();
        check(ui.menu == 1 && ui.menu_sel == MI_ABOUT, "menu SYSTEM: a knob does not open ABOUT, it moves the cursor");
        tap(B_OCTUP);
        check(ui.menu == 2, "menu SYSTEM: OCT+ on ABOUT opens it");
        tap(B_OCTDN);
        check(ui.menu == 1 && ui.menu_sel == MI_ABOUT, "menu: OCT- from ABOUT: back to SYSTEM");
        tap(B_OCTDN);
        check(ui.menu == 0, "menu: OCT- closes");
        lights_keys = KEYS_OFF; lights_notes = 0; lights_lvl = LIGHTS_OFF;
        go_home(); ui.force = 1; frame();
    }

    {   /* SELECT (2.4): the pages of the family shown; the tempo on HOME (TRACKS), in a layer, on REC */
        int16_t bpm;
        #define PT() (PAGES[ui.page].title)
        #define SEL(n) do { encs[panel.enc[EN_SELECT]] = (n); frame(); } while (0)
        {   /* PHEBUS, 21 Nov 2026 (Ben: "Merge and move"): each voice's chain without the shared effects, LFO 2 named
             * LFO SYNC; DLY, REV/CHO under GLO, neutral; every parameter still on a page (CLOUD, TEXTURE: cut, wave 1) */
            static const char *const WANT[5] = {
                "OSC|VOICE|VOICE 2|FILTER|FILTER 2|ENV|ENV DEST|FX|INSERT|INSERT 2|SLICER|LFO|LFO DEST|LFO SYNC",
                "OSC|VOICE|VOICE 2|FILTER|ENV|ENV DEST|FX|CHORUS|INSERT|INSERT 2|SLICER|LFO|LFO DEST|LFO SYNC",
                "OSC|OSC 2|VOICE|VOICE 2|FILTER|ENV|ENV DEST|FX|INSERT|INSERT 2|SLICER|LFO|LFO DEST|LFO SYNC",
                "OSC|OSC 2|VOICE|VOICE 2|FILTER|ENV|ENV DEST|FX|INSERT|INSERT 2|SLICER|LFO|LFO DEST|LFO SYNC",
                "HEX OSC|HEX MOD|HEX CLICK|HEX MORE|HEX FILT|FILTER|HEX AEG|INSERT|INSERT 2|SLICER"};
            static const char *const NM[5] = {"ACID", "JUNO", "ELEMENTS", "PLAITS", "drums (HEX)"};
            static const uint8_t SEL_OF[5] = {0, 0, 1, 2, 3};
            uint8_t e0 = trk[0].eng_req, pre0 = trk[0].preset, idx[40], seen[G_COUNT] = {0};
            uint32_t v, n, i, g_glo = 0, glob_ok = 1;
            for (v = 0; v < 5u; v++) {
                char got[400] = "", what[96];
                if (v == 1u) { set_engine_of(&trk[0], ENGI_JUNO); apply_preset_to(&trk[0], 0); }
                if (v == 2u) { set_engine_of(&trk[0], e0); apply_preset_to(&trk[0], pre0); }
                song.sel = SEL_OF[v];
                n = chain_list(idx, sizeof idx);
                for (i = 0; i < n; i++) {
                    glob_ok &= PAGES[idx[i]].graph == GR_HEX || PAGES[idx[i]].scope != SC_GLOBAL;
                    if (i) strcat(got, "|");
                    strcat(got, PAGES[idx[i]].title);
                }
                snprintf(what, sizeof what, "chain on %s: the page list as decided (%u pages)", NM[v], (unsigned)n);
                if (strcmp(got, WANT[v])) printf("ui:   got  %s\nui:   want %s\n", got, WANT[v]);
                check(!strcmp(got, WANT[v]), what);
            }
            song.sel = 0;
            check(glob_ok, "no shared (global) effect page left in any voice's chain");
            for (i = 0; i < NPAGES; i++)
                if (PAGES[i].fam == FAM_GLO) {
                    uint32_t k;
                    for (k = 0; k < 4u; k++) if (PAGES[i].id[k] != 0xFFu) seen[PAGES[i].id[k]] = 1;
                    g_glo |= (!strcmp(PAGES[i].title, "DLY") ? 1u : 0u) | (!strcmp(PAGES[i].title, "REV/CHO") ? 2u : 0u) |
                             (!strncmp(PAGES[i].title, "CLOUD", 5) || !strncmp(PAGES[i].title, "TEXTURE", 7) ? 4u : 0u);
                }
            check(g_glo == 3u && seen[G_DTIME] && seen[G_DFDBK] && seen[G_DCOLOR] && seen[G_DMIX] && seen[G_RSIZE] &&
                  seen[G_RDAMP] && seen[G_CRATE] && seen[G_CDEPTH],
                  "GLO: DLY, REV/CHO with all eight of their controls; no CLOUD or TEXTURE page");
            go_home(); open_family(FAM_GLO); frame();
            for (i = 0; i < 16u; i++) { encs[panel.enc[EN_SELECT]] = -1; frame(); }
            for (i = 0; i < 16u && strcmp(PAGES[ui.page].title, "DLY"); i++) { encs[panel.enc[EN_SELECT]] = 1; frame(); }
            ui.force = 1; frame(); ppm("page-glo-dly");
            check(!strcmp(PAGES[ui.page].title, "DLY"), "GLO, SELECT: reaches DLY");
            encs[panel.enc[EN_SELECT]] = 1; frame();
            ui.force = 1; frame(); ppm("page-glo-rev");
        }
        song.playing = 0; song.sel = 0; go_home(); ui.force = 1; frame();
        bpm = song.g[G_BPM];
        SEL(1);
        check(song.g[G_BPM] > bpm, "SELECT on HOME (TRACKS): the tempo");
        song.g[G_BPM] = bpm;
        tap(B_LFO);
        check(!strcmp(PT(), "LFO"), "SELECT: LFO opens its first page");
        /* (the sound chain, Ben 20 Nov 2026: OSC > FILTER > ENV > FX > MOD; MOD is LFO, LFO DEST, LFO SYNC (21 Nov)) */
        SEL(1);
        check(!strcmp(PT(), "LFO DEST") && song.g[G_BPM] == bpm, "SELECT on LFO: next page LFO DEST, the tempo untouched");
        SEL(1);
        check(!strcmp(PT(), "LFO SYNC") && song.g[G_BPM] == bpm, "SELECT: then LFO SYNC (2.6's LFO 2, named for what it is)");
        ui.force = 1; frame(); ppm("page-lfo2");
        SEL(1);
        check(!strcmp(PT(), "LFO SYNC") && song.g[G_BPM] == bpm, "SELECT: stops at the end of the chain (MOD's last page)");
        SEL(-1);
        check(!strcmp(PT(), "LFO DEST"), "SELECT left: the page before");
        SEL(-1);
        check(!strcmp(PT(), "LFO"), "SELECT left: LFO");
        SEL(-1);
        check(!strcmp(PT(), "SLICER"), "SELECT left from MOD's first page: FX's last, SLICER (one chain)");
        tap(B_FX); SEL(1);
        check(!strcmp(PT(), "INSERT"), "SELECT on FX: next page, INSERT");
        go_home(); tap(B_EDIT); tap(B_EDIT); SEL(1);
        check(!strcmp(PT(), "FILTER 2"), "EDIT, EDIT: FILTER; SELECT: FILTER 2 (ACID: the track FILTER)");
        TSEL->p[P_TFLT] = -32; ui.force = 1; frames(2); ppm("page-filter");
        {   /* Ben, 21 Nov ("Keep, name the pages"): the strip's FILT stage holds the page's name, its place at the right */
            uint32_t box = px_count(0, OP_CHAIN_Y + OP_CHAIN_H - 1, 240, 1, TE_COL[0]);
            check(box == text_w(&FONT_S, "FILTER 2") + 4u && px_count(200, OP_CHAIN_Y, 40, OP_CHAIN_H, TE_MID[0]) > 10u,
                  "strip: the stage shown holds the page's name (FILTER 2), 2/2 at the right");
            SEL(-1); ui.force = 1; frames(2);
            check(px_count(0, OP_CHAIN_Y + OP_CHAIN_H - 1, 240, 1, TE_COL[0]) == text_w(&FONT_S, "FILTER") + 4u,
                  "strip: back a page, FILTER (the name follows the page)");
            SEL(1); frames(2);
        }
        check(!strcmp(ui.big_l[0], "FILT") && !strcmp(ui.big_v[0], "LP50") && !ui.big_l[1][0],
              "FILTER page: its one value drawn large (LP50 %)");
        {   /* the value large in the knob band (ui_op.c): pixels lit under KNOB 1's label */
            uint32_t x, y, lit = 0;
            for (y = OP_KNOB_Y + 24; y < OP_KNOB_Y + 56; y++)
                for (x = 4; x < 60; x++)
                    lit += screen[y * 240u + x] != 0;
            check(lit > 100u, "FILTER page: the knob band shows it");
        }
        TSEL->p[P_TFLT] = 0;
        tap(B_FX); SEL(3);
        check(!strcmp(PT(), "SLICER"), "SELECT on FX: on to its last page, SLICER (the shared effects are GLO's)");
        SEL(1);
        check(!strcmp(PT(), "LFO"), "SELECT on FX's last page: on to MOD (LFO)");
        go_home(); tap(B_EDIT); SEL(-9);
        check(!strcmp(PT(), "OSC"), "EDIT: the chain's start, OSC; SELECT left stops there");
        SEL(2);
        check(!strcmp(PT(), "VOICE 2"), "SELECT on OSC: up to VOICE 2 (ACID: OSC, VOICE, VOICE 2)");
        tap(B_EDIT);
        check(!strcmp(PT(), "FILTER"), "EDIT on the chain: the next stage, FILTER");
        tap(B_EDIT);
        check(!strcmp(PT(), "ENV"), "EDIT: then ENV");
        tap(B_EDIT);
        check(!strcmp(PT(), "FX"), "EDIT: then FX");
        tap(B_EDIT);
        check(!strcmp(PT(), "LFO"), "EDIT: then MOD (LFO)");
        tap(B_EDIT);
        check(!strcmp(PT(), "OSC"), "EDIT on MOD: round to OSC");
        tap(B_SEQ); frames(2);
        check(on_drum_page() && kb_grid, "SEQ on a sound page (OSC): the GRID");
        tap(B_SEQ); SEL(9);
        check(!strcmp(PT(), "PATTERN"), "SEQ again: SEQ > PATTERN; SELECT: PATTERN is the SEQ family's one page (the generators and PLAY are cut)");
        tap(B_ENV);
        check(!strcmp(PT(), "ENV"), "the button still opens its family");
        tap(B_ENV);
        check(!strcmp(PT(), "ENV DEST"), "... and a second tap still steps the page");
        press(B_GLO); frames(3);
        SEL(1);
        check(!strcmp(PT(), "ENV DEST") && song.g[G_BPM] > bpm, "SELECT with a layer held (GLO): the tempo");
        release(B_GLO); frames(2);
        song.g[G_BPM] = bpm;
        go_home(); frame(); SEL(-1);
        check(song.g[G_BPM] < bpm, "back on HOME: SELECT is the tempo again");
        song.g[G_BPM] = bpm;
        #undef SEL
        #undef PT
    }

    {   /* Felucca #39 / #102 (2.4): a knob turned as a layer lets go, or while PLAY is pressed in a layer, never
         * edits the page under the layer */
        int16_t r0, w0;
        song.playing = 0; song.sel = 0; go_home(); ui.force = 1; frames(3);
        tap(B_LFO); frames(2);
        r0 = trk[0].p[P_LRATE]; w0 = trk[0].p[P_LWAVE];
        press(B_FX); frames(10);
        encs[panel.enc[EN_K2]] = 3; frame();
        encs[panel.enc[EN_K1]] = 4;                         /* turned in the frame FX comes up */
        fm1_in.buttons &= ~BT(B_FX); frame();
        check(trk[0].p[P_LRATE] == r0, "#39: KNOB 1 turned as FX lets go: the LFO page under it is untouched");
        frames(5); encs[panel.enc[EN_K1]] = 2; frame();
        check(trk[0].p[P_LRATE] == r0, "#39: ... and for a moment after (the knob still turning)");
        frames(20); encs[panel.enc[EN_K1]] = 2; frame();
        check(trk[0].p[P_LRATE] > r0, "#39: then KNOB 1 is the page's again");
        trk[0].p[P_LRATE] = r0;
        press(B_FX); frames(10);
        edges_btn |= BT(B_PLAY); fm1_in.buttons |= BT(B_PLAY);
        encs[panel.enc[EN_K2]] = 2; frame();
        fm1_in.buttons &= ~BT(B_PLAY);
        check(trk[0].p[P_LWAVE] == w0 && song.g[G_DUST] > 0 && transport_req, "#102: PLAY pressed in the FX layer plays, KNOB 2 is DUST only");
        transport_req = 0; song.g[G_DUST] = 0;
        release(B_FX); frames(20);
        song.g[G_BPM] = 120;
    }

    {   /* DRUMS grid (2.4): the keys are the steps of the sound; the sound and the step heard; SELECT grid / kit */
        uint32_t i;
        song.playing = 0; song.sel = TRK_DRUM; go_home(); frame();
        memset(TDRUM->dstep, 0, sizeof TDRUM->dstep);
        TDRUM->p[P_SLEN] = 32;
        studio_open(SC_DRUM); drum_page = 0; drum_lane = 0; drum_cursor = 0; frames(2);
        memset(pad_lit, 0, sizeof pad_lit);
        key(key_of_white(2)); frame();
        check(dstep_has(&TDRUM->dstep[2], 0) && drum_cursor == 2 && pad_lit[0], "grid: white key 3 sets step 3 of the sound (heard), the cursor on it");
        check((keys_lit() >> key_of_white(2)) & 1u, "grid: its key lit");
        key(key_of_white(2));
        check(!dstep_mask(&TDRUM->dstep[2]), "grid: the key again clears it");
        key(3);                                                     /* (G#3: the second black key) */
        check(drum_cursor / 16u == 1u, "grid: the black keys pick the page of steps (17-32)");
        key(key_of_white(0));
        check(dstep_has(&TDRUM->dstep[16], 0), "grid: ... and the white keys are its steps (step 17)");
        ui.msg_t = 0;
        key(8);                                                     /* (C#4: the fourth: steps 49-64, none) */
        check(drum_cursor / 16u == 1u && TDRUM->p[P_SLEN] == 32 && !strcmp(ui.msg, "ADD PAGE 3"),
              "grid: a page two past LENGTH is not picked, and says so (\"ADD PAGE 3\")");
        dstep_set(&TDRUM->dstep[17], 6, LV_HARD, 1); TDRUM->micro[17] = 3;
        key(5);                                                     /* (A#3: the third: the slot after the last) */
        check(drum_cursor / 16u == 2u && TDRUM->p[P_SLEN] == 48 && dstep_has(&TDRUM->dstep[33], 6) &&
              dstep_lvl(&TDRUM->dstep[33], 6) == LV_HARD && TDRUM->micro[33] == 3 && dstep_has(&TDRUM->dstep[32], 0),
              "grid: the page key after the last: a page added, a copy of the last (hits, levels, nudges)");
        ui.force = 1; frame(); ppm("grid-drum-page-add");
        memset(&TDRUM->dstep[17], 0, sizeof(dstep_t)); TDRUM->micro[17] = 0; TDRUM->micro[33] = 0;
        TDRUM->p[P_SLEN] = 32; drum_cursor = 16; frame();
        memset(pad_lit, 0, sizeof pad_lit);
        encs[panel.enc[EN_K1]] = 3; frames(2);
        check(drum_lane == 3 && pad_lit[3], "grid: KNOB 1 picks the sound and plays it");
        dstep_set(&TDRUM->dstep[20], 5, LV_NORM, 0);
        dstep_set(&TDRUM->dstep[20], 7, LV_SOFT, 0);
        memset(pad_lit, 0, sizeof pad_lit);
        drum_cursor = 19; encs[panel.enc[EN_K2]] = 1; frames(2);
        check(drum_cursor == 20 && pad_lit[5] && pad_lit[7] && !pad_lit[3], "grid: KNOB 2 onto a step plays what it holds");
        {   /* REVIEW 5: SELECT right on the drum GRID: the full view (a row a sound in use), the keys still the steps */
            uint32_t j, bpm = (uint32_t)song.g[G_BPM];
            for (j = 0; j < 32u; j += 4u) dstep_set(&TDRUM->dstep[j], 0, LV_NORM, 0);
            for (j = 4; j < 32u; j += 8u) dstep_set(&TDRUM->dstep[j], 2, LV_HARD, 0);
            for (j = 2; j < 32u; j += 2u) dstep_set(&TDRUM->dstep[j], 4, j % 4u ? LV_SOFT : LV_NORM, 0);
            encs[panel.enc[EN_SELECT]] = 1; frame();
            check(drum_full && drum_page == 0u && grid_keys_on() && song.g[G_BPM] == (int16_t)bpm,
                  "DRUMS: SELECT right on the GRID: the full view (the keys still steps, the tempo untouched)");
            {
                uint8_t row[DRUM_LANES];
                uint32_t n = dg_rows(row);
                check(n == 6u && row[0] == 0u && row[1] == 2u && row[2] == 3u && row[3] == 4u && row[4] == 5u && row[5] == 7u,
                      "full view: a row for each sound in use and the one picked (KICK SNARE CLAP HAT OPEN RIM)");
            }
            ui.force = 1; frame(); ppm("grid-drum-full");
            transport_req = 1; frames(12); ui.force = 1; frame(); ppm("grid-drum-full-play");
            transport_req = 2; frames(2);
            drum_cursor = 20; frame();
            key(key_of_white(9)); frame();
            check(dstep_has(&TDRUM->dstep[25], 3), "full view: a white key sets the step of the sound picked");
            key(key_of_white(9)); frame();
            encs[panel.enc[EN_SELECT]] = 1; frame();
            check(drum_full && drum_page == 0u, "full view: SELECT right again: stays");
            encs[panel.enc[EN_SELECT]] = -1; frame();
            check(!drum_full && drum_page == 0u, "full view: SELECT left: the snake view again");
            for (j = 0; j < 32u; j++) { dstep_clr(&TDRUM->dstep[j], 0); dstep_clr(&TDRUM->dstep[j], 2); dstep_clr(&TDRUM->dstep[j], 4); }
            ui.force = 1; frame();
        }
        tap(B_SEQ); frames(2);
        check(drum_page == 1, "DRUMS: SEQ on the GRID: kit");
        frame();
        memset(pad_lit, 0, sizeof pad_lit);
        key(key_of_white(9)); frame();
        check(pad_lit[9] && !dstep_has(&TDRUM->dstep[25], 3), "kit: the keys play the pads again, no step set");
        encs[panel.enc[EN_SELECT]] = -1; frame();
        check(drum_page == 0, "DRUMS: SELECT left: grid");
        {   /* PHEBUS (21 Nov 2026): the STUTTER lane, after DRUM on the GRID (ui_stut.c, punch.c stl) */
            uint32_t j, kw0 = key_of_white(0), kw12 = key_of_white(12);
            check(stl_new == 0u && !strcmp(STL_LEN[stl_new], "1/4"),
                  "STUTTER (Ben, 21 Nov: \"1/4 is preferably\"): new steps start at 1/4");
            memset(&stl, 0, sizeof stl); stl.len = 16; stl.act = 1; stl_new = 1;
            ui.force = 1; frame();
            encs[panel.enc[EN_ALGO]] = 1; frames(2);
            check(stl_shown() && song.sel == TRK_DRUM && grid_keys_on() && kb_grid,
                  "STUTTER: ALGORITHM right of DRUM on the GRID: the lane, the keys its steps");
            ui.force = 1; frame(); ppm("grid-stutter-empty");
            check(px_count(0, OP_BAR_Y + 24, 240, 3, TE_COL[3]) == 0 && px_count(0, OP_BAR_Y + 24, 240, 3, TE_COL[0]) == 0,
                  "STUTTER: the track strip underlines no track (the lane is not DR)");
            check(px_count(2, 1, 46, 18, TE_G4) > 40u && px_count(48, 0, 4, 20, TE_G4) == 0,
                  "STUTTER: its header tag drawn in the box (STUT, not SYN)");
            key(kw12);
            check(stl_lit(12) && stl_code(12) == 1u, "STUTTER: tap step 13: lit, 1/8 (the length for new steps)");
            check(!dstep_mask(&TDRUM->dstep[12]), "STUTTER: ... and no drum hit set");
            key(kw12);
            check(!stl_lit(12), "STUTTER: tap it again: off");
            encs[panel.enc[EN_K1]] = -1; frame();
            check(stl_new == 0u, "STUTTER: KNOB 1 left: new steps 1/4");
            for (j = 12; j < 16u; j++) key(key_of_white(j));
            check(stl_lit(12) && stl_lit(15) && stl_code(13) == 0u, "STUTTER: steps 13-16 lit at 1/4");
            fm1_in.notes |= 1u << key_of_white(14); frame();
            encs[panel.enc[EN_K1]] = 1; frame(); encs[panel.enc[EN_K1]] = 1; frame();
            ui.force = 1; frame(); ppm("grid-stutter-held");
            fm1_in.notes &= ~(1u << key_of_white(14)); frame();
            check(stl_lit(14) && stl_code(14) == 2u && stl_code(13) == 0u, "STUTTER: hold step 15 + KNOB 1: that step 1/16, the others kept");
            check(stl_new == 0u, "STUTTER: ... the length for new steps unchanged");
            press(B_EDIT); press(B_OCTDN); release(B_OCTDN); release(B_EDIT); frames(2);
            check(stl_code(14) == 0u, "STUTTER: EDIT + OCT-: undo (step 15 back to 1/4)");
            press(B_EDIT); press(B_OCTUP); release(B_OCTUP); release(B_EDIT); frames(2);
            check(stl_code(14) == 2u, "STUTTER: EDIT + OCT+: redo");
            {   /* (PHEBUS, picker "Replay the steps") six lengths, KNOB 1 right shorter, left longer: 1/1 1/2 1/4 1/8 1/16 1/32 */
                static const char *const ORD[6] = {"1/1", "1/2", "1/4", "1/8", "1/16", "1/32"};
                uint32_t q, ok = 1;
                frames(16);                             /* (the knobs quiet ~250 ms after the EDIT layer: #39) */
                fm1_in.notes |= 1u << key_of_white(14); frame();
                for (q = 0; q < 8u; q++) { encs[panel.enc[EN_K1]] = -1; frame(); }
                ok &= stl_code(14) == 5u && !strcmp(STL_LEN[stl_code(14)], "1/1");
                ui.force = 1; frame(); ppm("grid-stutter-long");
                for (q = 1; q < 6u; q++) { encs[panel.enc[EN_K1]] = 1; frame(); ok &= !strcmp(STL_LEN[stl_code(14)], ORD[q]); }
                encs[panel.enc[EN_K1]] = 1; frame();
                ok &= stl_code(14) == 3u;
                for (q = 0; q < 4u; q++) { encs[panel.enc[EN_K1]] = -1; frame(); }
                ok &= stl_code(14) == 4u;
                fm1_in.notes &= ~(1u << key_of_white(14)); frame();
                check(ok && stl_lit(14), "STUTTER: hold a step + KNOB 1 left to 1/1 (no further), right 1/2 1/4 1/8 1/16 1/32 (no further)");
                ui.force = 1; frame(); ppm("grid-stutter-half");
                stl_put_step(14, 2);                    /* (back to 1/16 for what follows) */
            }
            j = (uint32_t)TDRUM->p[P_SLEN];
            for (i = 0; i < 64u && stl.len < 32u; i++) { encs[panel.enc[EN_K4]] = 1; frame(); }
            check(stl.len == 32u && TDRUM->p[P_SLEN] == (int16_t)j, "STUTTER: KNOB 4: its own length, 32 steps (the drums keep theirs)");
            key(3);                                      /* (A3, the second black key: steps 17-32) */
            check(stl_pg == 1u, "STUTTER: black key 2: steps 17-32");
            encs[panel.enc[EN_K1]] = 1; frame();         /* (1/8) */
            key(kw12);
            check(stl_lit(28) && stl_code(28) == 1u, "STUTTER: on page 2 the keys are steps 17-32 (step 29 lit, 1/8)");
            ui.msg_t = 0; ui.force = 1; frame(); ppm("grid-stutter-p2");
            encs[panel.enc[EN_K2]] = -1; frame();
            check(!stl.act && stl_lit(28), "STUTTER: KNOB 2 left: the lane OFF, its steps kept");
            ui.msg_t = 0; ui.force = 1; frame(); ppm("grid-stutter-off");
            encs[panel.enc[EN_K2]] = 1; frame();
            check(stl.act, "STUTTER: KNOB 2 right: ON");
            /* played: the steps lit ask for their loop as the playhead reaches them (punch.c stl_want) */
            {
                uint32_t seen13 = 0, seen29 = 0, other = 0, n = 0, cur29 = 0;
                transport_req = 1;
                while (n++ < 400u) {
                    frame();
                    if (stl_pos == 12u || stl_pos == 13u) seen13 |= stl_want == PX_LOOP4;
                    else if (stl_pos == 14u) seen13 |= (stl_want == PX_LOOP16) << 1;
                    else if (stl_pos == 28u) { seen29 |= stl_want == PX_LOOP8; cur29 |= punch.cur == PX_LOOP8; }
                    else other |= stl_want >= 0 && stl_pos != 15u;
                    if (stl_pos == 29u) { ui.force = 1; frame(); ppm("grid-stutter-play"); break; }
                }
                check(seen13 == 3u && seen29 && !other && cur29,
                      "STUTTER: playing: steps 13-16 ask for LOOP 4 (15: LOOP 16), 29 LOOP 8, no other step; the punch plays it");
                transport_req = 2; frames(3);
                check(stl_want < 0, "STUTTER: stopped: no stutter");
            }
            /* (saved with the project and its sections: tests/project_test.c) */
            encs[panel.enc[EN_ALGO]] = -1; frames(2);
            check(!stl_shown() && song.sel == TRK_DRUM && !drum_page, "STUTTER: ALGORITHM left: the drum GRID again");
            encs[panel.enc[EN_ALGO]] = 1; frames(2);
            check(stl_shown(), "STUTTER: right again: the lane");
            track_select(0); frames(1); track_select(TRK_DRUM); frames(1);
            check(!stl_shown(), "STUTTER: another track picked and back: the drum GRID");
            memset(&stl, 0, sizeof stl); stl.len = 16; stl.act = 1;
            ui.force = 1; frame();
        }
        go_home(); frame();
        memset(pad_lit, 0, sizeof pad_lit);
        press(B_SEQ); frames(3);
        encs[panel.enc[EN_K1]] = 2; frames(2);
        check(!pad_lit[2] && !pad_lit[1], "SEQ layer, drum track: KNOB 1 does nothing (no sound picked or played)");
        release(B_SEQ); frames(2);
        song.sel = 1; go_home(); frame();
        trk[1].p[P_SDIV] = 5;
        press(B_SEQ); frames(3);
        for (i = 0; i < 5u; i++) { encs[panel.enc[EN_K2]] = 1; frame(); }
        check(trk[1].p[P_SDIV] == (int16_t)(NDIV_STEP - 1u), "SEQ layer: KNOB 2 DIV goes on to 1/2, 1BAR, 2BAR");
        release(B_SEQ); frames(2);
        trk[1].p[P_SDIV] = 2; TDRUM->p[P_SLEN] = 16; memset(TDRUM->dstep, 0, sizeof TDRUM->dstep);
        song.sel = 0; go_home(); frame();
    }

    {   /* the screensavers (PHEBUS: FLOW, EYES, DESERT, THIRD EYE): HOME on HOME opens one, SELECT steps through them, HOME / a
         * page closes it; a layer shows its screen over it; the choice is kept with the settings */
        uint32_t st, j, x, lit, k;
        static const uint8_t BASS[2] = {38, 34};
        song.playing = 0; song.sel = 0;
        for (j = 0; j < NTRK; j++) { steps_clear(&trk[j]); trk[j].p[P_SLEN] = 16; }
        for (j = 0; j < 16u; j++) {                 /* a groove to look at */
            step_t *sp;
            if (!(j & 1u)) { sp = &trk[0].step[j]; sp->n = 1; sp->note[0] = (uint8_t)(BASS[j / 8u] + ((j & 2u) ? 12 : 0)); sp->time = ST_NOTE; sp->vel = 100; }
            if ((j % 4u) == 2u) { sp = &trk[1].step[j]; sp->n = 3; sp->note[0] = 65; sp->note[1] = 69; sp->note[2] = 72; sp->time = ST_NOTE; sp->vel = 100; }
            if (!(j % 8u)) { sp = &trk[2].step[j]; sp->n = 2; sp->note[0] = 57; sp->note[1] = 64; sp->time = ST_NOTE; sp->vel = 100; }
            memset(&TDRUM->dstep[j], 0, sizeof(dstep_t));
            if (!(j % 4u)) dstep_set(&TDRUM->dstep[j], 0, LV_NORM, 0);
            if ((j % 8u) == 4u) dstep_set(&TDRUM->dstep[j], 3, LV_NORM, 0);
            dstep_set(&TDRUM->dstep[j], 4, (j & 1u) ? LV_SOFT : LV_NORM, 0);
            for (k = 0; k < 4u; k++) trk[k].seq_active = 1;
        }
        go_home(); frames(20);                      /* (the knobs quiet after the layers above) */
        transport_req = 1; frames(30);
        check(!vis_shown(), "visualiser: closed on the TRACKS screen");
        tap(B_HOME); frames(2);
        check(vis_shown() && vis_on, "visualiser: HOME on HOME opens it");
        vis_style = 0; ui.force = 1;
        for (st = 0; st < VIS_N; st++) {
            char nm[80];
            frames(st == 0u ? 40u : 70u);           /* (past the name's second) */
            ui.force = 1; frames(2);
            for (lit = 0, x = 0; x < 240u * 240u; x++)
                lit += screen[x] != 0;
            snprintf(nm, sizeof nm, "vis-%02u-%s", st + 1u, VIS_NAME[st]);
            for (x = 0; nm[x]; x++) if (nm[x] == ' ') nm[x] = '-';
            ppm(nm);
            snprintf(nm, sizeof nm, "screensaver %u %-9s draws (%u px lit)", st + 1u, VIS_NAME[st], lit);
            check(lit > 1500u && vis_style == st, nm);
            encs[panel.enc[EN_SELECT]] = 1; frame();
        }
        {   /* MASTER at 0: silence out, the picture as if MASTER were all the way up */
            uint32_t m0 = song.master_q12, pk = 0, op = 0, b;
            static int32_t o2[CTL * 2];
            song.master_q12 = 0; frames(20);
            for (j = 0; j < SCOPE_N; j++) { uint32_t a = (uint32_t)(scope_buf[j] < 0 ? -scope_buf[j] : scope_buf[j]); if (a > pk) pk = a; }
            for (b = 0; b < 200u; b++) { mix_block(o2, CTL); for (j = 0; j < CTL * 2u; j++) { uint32_t a = (uint32_t)(o2[j] < 0 ? -o2[j] : o2[j]); if (a > op) op = a; } }
            ui.force = 1; frames(2);
            for (lit = 0, x = 0; x < 240u * 240u; x++) lit += screen[x] != 0;
            check(op < 64u && pk > 2000u && lit > 1500u, "visualiser: MASTER at 0: no sound, the picture as at full volume");
            song.master_q12 = m0; frames(4);
        }
        check(vis_style == 0u, "visualiser: SELECT goes round (THIRD EYE -> FLOW)");
        encs[panel.enc[EN_SELECT]] = -1; frames(2);
        check(vis_style == VIS_N - 1u && (lights_word() >> 17 & 15u) == VIS_N - 1u, "visualiser: SELECT left: THIRD EYE (4), kept in the settings word");
        lights_from_word(lights_word());
        check(vis_style == VIS_N - 1u, "visualiser: the style read back");
        {   /* each screensaver follows each voice: silent, one voice loud (each in turn), all loud with a kick; the
             * pictures at several moments (ss-NAME-STATE) */
            static const char *const SN[6] = {"silent", "v1", "v2", "v3", "v4", "all"};
            uint32_t q, v, pix[6], diff;
            char nm[64];
            uint8_t st0 = vis_style;
            static uint16_t ref[240 * 240];
            for (st = 0; st < VIS_N; st++) {
                vis_style = (uint8_t)st;
                for (q = 0; q < 6u; q++) {
                    for (v = 0; v < 4u; v++) {
                        vs.e[v] = (uint16_t)(q == 5u || q == v + 1u ? 250u : 0u);
                        vs.flash[v] = 0;
                    }
                    vs.kick = q == 5u ? 200u : 0u;
                    vs.tq = (st * 1700u + q * 260u + 80u) << 8;
                    vs.rip = q * 9u;
                    memset(ey, 0, sizeof ey);
                    vis_paint();
                    for (pix[q] = 0, x = 0; x < 240u * 240u; x++)
                        pix[q] += (uint32_t)((swap16(screen[x]) >> 11) + ((swap16(screen[x]) >> 5) & 63u) / 2u + (swap16(screen[x]) & 31u));
                    snprintf(nm, sizeof nm, "ss-%s-%s", VIS_NAME[st], SN[q]);
                    for (x = 0; nm[x]; x++) if (nm[x] == ' ') nm[x] = '-';
                    ppm(nm);
                }
                snprintf(nm, sizeof nm, "screensaver %s: each voice brightens it, all four most", VIS_NAME[st]);
                check(pix[1] > pix[0] && pix[2] > pix[0] && pix[3] > pix[0] && pix[4] > pix[0] && pix[5] > pix[1] &&
                      pix[5] > pix[4], nm);
                vs.e[0] = vs.e[1] = vs.e[2] = vs.e[3] = 0; vs.kick = 0;   /* it moves while nothing plays */
                vs.tq = 4000u << 8; vis_paint(); memcpy(ref, screen, sizeof ref);
                vs.tq = 4012u << 8; vis_paint();
                for (diff = 0, x = 0; x < 240u * 240u; x++) diff += screen[x] != ref[x];
                snprintf(nm, sizeof nm, "screensaver %s: flows while silent (%u px changed)", VIS_NAME[st], diff);
                check(diff > 2000u, nm);
            }
            vis_style = st0; ui.force = 1; frames(2);
        }
        { int16_t bpm = song.g[G_BPM], lv = trk[0].p[P_LEVEL];
          encs[panel.enc[EN_K2]] = 5; frame();
          check(trk[0].p[P_LEVEL] == lv && song.g[G_BPM] == bpm, "visualiser: KNOB 2 (the hidden TRACKS screen) edits nothing, SELECT is not the tempo"); }
        press(B_GLO); frames(32);                  /* (held past a tap, not long enough to keep it) */
        check(ui.layer == LY_MIX, "visualiser: a layer held shows its screen");
        release(B_GLO); frames(4);
        check(vis_shown(), "visualiser: ... and it comes back");
        tap(B_HOME); frames(2);
        check(!vis_shown() && !vis_on, "visualiser: HOME again closes it");
        tap(B_HOME); frames(2);
        tap(B_LFO); frames(2);
        check(!vis_shown() && !vis_on && !strcmp(PAGES[ui.page].title, "LFO"), "visualiser: a page button closes it and opens its page");
        transport_req = 2; frames(4);
        for (j = 0; j < NTRK; j++) steps_clear(&trk[j]);
        vis_style = 0; song.sel = 0; go_home(); frames(20);
    }

    {   /* 2.6, after Felucca 1.4 / 1.5 (Leo Kuroshita): STEP LEN, SEQ VEL, the drum mutes (EDIT + a black key), MIDI
         * LEARN (GLO + F#3, a knob, a CC; LEARN CLEAR in the menu), the INSERT pages (the drum track too), ANALOG's FILTER */
        uint32_t j, w, ins_d = 0, flt_a = 0, flt_o = 0;
        #define K(n, v) do { encs[panel.enc[n]] = (v); frames(5); } while (0)
        #define WALK(fa_, ti_, hit_) do { open_family(fa_); frame(); \
            for (j = 0; j < 16u; j++) { encs[panel.enc[EN_SELECT]] = -1; frame(); } \
            for (j = 0; j < 16u; j++) { if (PAGES[ui.page].fam == (fa_) && !strcmp(PAGES[ui.page].title, ti_)) hit_ = 1; \
                encs[panel.enc[EN_SELECT]] = 1; frame(); } } while (0)
        song.playing = 0;
        song.sel = 0; steps_clear(&trk[0]); trk[0].p[P_SLEN] = 16; go_home(); frames(16);
        trk[0].step[4].n = 1; trk[0].step[4].note[0] = 60; trk[0].step[4].time = ST_NOTE; trk[0].step[4].vel = 100;   /* step 5
                                                         * (held SEQ set it before PHEBUS took the step keys off) */
        tap(B_SEQ); frames(2); tap(B_SEQ); frames(2);
        check(!strcmp(PAGES[ui.page].title, "PATTERN"), "SEQ pages: PATTERN first (the old STEP page is gone: the GRID edits steps)");
        tap(B_SEQ); frames(2);                            /* (REVIEW 2: SEQ from a SEQ page: back to the GRID) */
        check(on_drum_page() && kb_grid, "SEQ on SEQ > PATTERN: back to the GRID");
        fm1_in.notes = 1u << key_of_lane(4); frame();     /* LEN on the GRID (a step held, KNOB 2), as STEP's KNOB 3 was */
        K(EN_K2, 1); K(EN_K2, 1);
        check(note_len(&trk[0], 4) == 3u && trk[0].step[5].time == ST_TIE && trk[0].step[6].time == ST_TIE &&
              trk[0].step[7].time != ST_TIE, "GRID, step held, KNOB 2 on a note: LEN 1 -> 3 (two TIEs after it)");
        K(EN_K2, -1);
        check(note_len(&trk[0], 4) == 2u && trk[0].step[6].time == ST_REST, "KNOB 2 back: LEN 2 (its last TIE a REST)");
        trk[0].step[5].n = 1, trk[0].step[5].note[0] = 64;   /* a TIE holding a note (TIME, or an older SLOOP) */
        K(EN_K2, -1);
        check(note_len(&trk[0], 4) == 1u && trk[0].step[5].time == ST_REST && trk[0].step[5].n == 1u &&
              trk[0].step[5].note[0] == 64u, "LEN shorter over a TIE holding a note: a REST, the note kept (as TIME)");
        fm1_in.notes = 0; frames(2);
        go_home(); frames(16);
        press(B_EDIT); frames(10);
        edges_btn |= BT(B_OCTDN); fm1_in.buttons |= BT(B_OCTDN); frame(); fm1_in.buttons &= ~BT(B_OCTDN); frame();
        release(B_EDIT); frames(16);
        check(note_len(&trk[0], 4) == 1u && trk[0].step[5].n == 0u && trk[0].step[4].n == 1u,
              "EDIT + OCT-: the LEN turns of that hold undone at once (the steps as before the step was held)");
        {   /* 2.6: a MIDI keyboard's notes (CH 1 = track 1, selected) set the GRID's pen, the chord held together; a
             * note for another track while recording says which track it plays */
            #define MIN(c, n, v) (midi_in_q[mi_w++ % MQ] = ((v) ? 0x09u | (0x90u | (c)) << 8 : 0x08u | (0x80u | (c)) << 8) | \
                                  (uint32_t)(n) << 16 | (uint32_t)(v) << 24)
            uint32_t k9;
            int16_t v0 = trk[0].p[P_VOICE];
            trk[0].p[P_VOICE] = V_POLY;
            tap(B_SEQ); frames(2);
            MIN(0, 50, 100); MIN(0, 54, 90); frame();
            check(pen_n == 2u && pen_note[0] == 50u && pen_note[1] == 54u, "GRID: two MIDI notes held (CH 1): the pen, that chord");
            MIN(0, 50, 0); MIN(0, 54, 0); frame();
            MIN(0, 62, 100); MIN(0, 62, 0); frame();
            check(pen_n == 1u && pen_note[0] == 62u && step_in_r == step_in_w, "  a MIDI note tapped: the pen that note, the queue read");
            go_home(); frame();
            ui.msg[0] = 0; song.rec = 1;                  /* (2.6) recording track 1: a keyboard on track 2's channel */
            MIN(1, 60, 100); MIN(1, 60, 0); frame();
            check(!strcmp(ui.msg, "MIDI PLAYS TRACK 2"), "REC on track 1, a MIDI note on CH 2: \"MIDI PLAYS TRACK 2\" (not recorded)");
            ui.msg[0] = 0;
            MIN(1, 62, 100); MIN(1, 62, 0); frame();
            check(!ui.msg[0], "  said once a take (an external sequencer on track 2 does not flash it)");
            song.rec = 0; frame(); song.rec = 1; ui.msg[0] = 0;   /* a new take; the keyboard on track 1's channel */
            MIN(0, 60, 100); MIN(0, 60, 0); frame();
            MIN(1, 62, 100); MIN(1, 62, 0); frame();
            check(!ui.msg[0], "  a new take, a note on CH 1 (track 1's) first: no message, even with CH 2 after");
            song.rec = 0; ui.msg[0] = 0;
            for (k9 = 0; k9 < NSTEP; k9++) if (trk[0].step[k9].time == ST_NOTE && trk[0].step[k9].note[0] == 60) { trk[0].step[k9].time = ST_REST; trk[0].step[k9].n = 0; }
            trk[0].p[P_VOICE] = v0;
            #undef MIN
        }
        {   /* 2.6, after Felucca 1.5.1: MIDI Program Change (queued by seq.c, loaded here) and FX + OCT- / OCT+: the THROWs */
            uint8_t e0 = trk[1].eng_req, p0 = trk[1].preset;
            int16_t k0 = TDRUM->p[P_E0];
            uint32_t k, tot = pl_total(&trk[1]), x0 = pl_entry(&trk[1], 0);
            go_home(); frame();
            pc_req[1] = 1;                                /* PC 0 on track 2's channel: the first sound of the list */
            frame();
            check(trk[1].eng_req == BANK[x0].e && trk[1].preset == bank_pi[x0] && !strncmp(ui.msg, "TRACK 2 PROG 0", 14),
                  "MIDI Program Change 0: track 2 takes the first sound of its PRESETS list (TRACK 2 PROG 0)");
            pc_req[3] = 3;                                /* PC 2 on the drum channel: kit 2 */
            frame();
            check(TDRUM->p[P_E0] == 2, "MIDI Program Change on the drum channel: kit 2");
            pc_req[1] = (uint8_t)(tot + 1u);              /* past the end: nothing */
            k = trk[1].eng_req;
            frame();
            check(trk[1].eng_req == k && trk[1].preset == bank_pi[x0], "MIDI Program Change past the list: ignored (no wrap)");
            TDRUM->p[P_E0] = k0;
            set_engine_of(&trk[1], e0); apply_preset_to(&trk[1], p0); trk[1].engine = trk[1].eng_req;
            frames(4);
            press(B_FX); frames(10);
            fm1_in.buttons |= BT(B_OCTDN); edges_btn |= BT(B_OCTDN); frame(); frame();
            k = thr_req;
            ui.force = 1; frame(); ppm("layer-fx-throw");
            fm1_in.buttons &= ~BT(B_OCTDN); frame();
            fm1_in.buttons |= BT(B_OCTUP); edges_btn |= BT(B_OCTUP); frame(); frame();
            check(k == 1u && thr_req == 2u && !strcmp(sub_line(), "echo throw"), "FX + OCT-: REV THROW while held, FX + OCT+: ECHO THROW");
            fm1_in.buttons &= ~BT(B_OCTUP); frame();
            release(B_FX); frames(4);
            check(thr_req == 0u && song.octave == 0, "  let go: no THROW, the octave untouched");
        }
        steps_clear(&trk[0]); go_home(); frames(16);
        {   /* the GRID: two steps held + KNOB 3 (VEL): both one up (the turn accelerated once, not once a step; held SEQ
             * had this test until PHEBUS took its step keys off) */
            uint32_t v0, v1;
            song.sel = 0; trk[0].p[P_SLEN] = 16; go_home(); frames(16);
            studio_open(SC_DRUM); frames(2);
            key(key_of_lane(0)); key(key_of_lane(4));
            fm1_in.notes = 1u << key_of_lane(0) | 1u << key_of_lane(4); frame();
            v0 = step_vel_shown(&trk[0].step[0]), v1 = step_vel_shown(&trk[0].step[4]);
            K(EN_K3, 1);
            check(step_on(&trk[0].step[0]) && trk[0].step[0].vel == v0 + 1u && trk[0].step[4].vel == v1 + 1u,
                  "GRID: two steps held + KNOB 3: both velocities one up");
            fm1_in.notes = 0; frame();
            steps_clear(&trk[0]); go_home(); frames(16);
        }

        song.sel = TRK_DRUM; go_home(); frames(16);
        dstep_set(&TDRUM->dstep[0], 0, LV_NORM, 0);
        press(B_EDIT); frames(10);
        key(1);                                           /* a black key: mute mode */
        check(er_mute == 1u, "EDIT + a black key (drum track): MUTE mode");
        key(0);                                           /* the kick's key */
        check(drum_mute == 1u && dstep_has(&TDRUM->dstep[0], 0), "MUTE mode + the kick's key: the kick muted, its steps kept");
        ui.force = 1; frame(); ppm("layer-erase-mute");
        key(0);
        check(drum_mute == 0u, "the kick's key again: unmuted");
        key(0);
        release(B_EDIT); frames(2);
        check(er_mute == 0u && drum_mute == 1u, "EDIT let go: out of MUTE mode, the mute stays");
        drum_mute = 0;
        TDRUM->dstep[0] = (dstep_t){0};
        WALK(FAM_FX, "INSERT", ins_d);
        check(ins_d, "the drum track: the INSERT page in FX");
        go_home(); frames(16);

        song.sel = 0; set_engine_of(&trk[0], 0); trk[0].engine = trk[0].eng_req; go_home(); frame();
        for (j = 0; j < NPAGES; j++) flt_a |= PAGES[j].id[0] == P_FTYPE && PAGES[j].fam == FAM_FLT && page_shown(&PAGES[j]);
        open_family(FAM_EDIT); frame();
        for (j = 0; j < 30u && strcmp(PAGES[ui.page].title, "FILTER"); j++) { encs[panel.enc[EN_SELECT]] = 1; frame(); }
        ui.force = 1; frame(); ppm("page-filter");
        {   /* on EDIT > FILTER, ALGORITHM to a track that is not ANALOG: not left on a page it does not have */
            int16_t bpm0 = song.g[G_BPM];
            encs[panel.enc[EN_ALGO]] = 1; frames(2);
            check(song.sel == 1u && trk[1].eng_req != 0u && PAGES[ui.page].fam == FAM_FLT &&
                  PAGES[ui.page].id[0] == P_TFLT && page_shown(cur_page()),
                  "FILTER (ANALOG's TYPE), then a track that is not ANALOG: its own FILTER page (FILT), not TYPE");
            encs[panel.enc[EN_SELECT]] = 1; frame();
            check(song.g[G_BPM] == bpm0, "  and SELECT turns pages there, not the tempo");
            song.sel = 0; go_home(); frame();
        }
        set_engine_of(&trk[0], 1); trk[0].engine = trk[0].eng_req; go_home(); frame();
        for (j = 0; j < NPAGES; j++) flt_o |= PAGES[j].id[0] == P_FTYPE && page_shown(&PAGES[j]);
        check(flt_a && !flt_o, "FILTER: the TYPE page with ANALOG only");
        set_engine_of(&trk[0], TRK_DEF[0][0]); apply_preset_to(&trk[0], TRK_DEF[0][1]); trk[0].engine = trk[0].eng_req;
        go_home(); frames(16);

        for (w = 0; w < ML_N; w++) ml_tab[w] = 0;
        press(B_GLO); frames(10);
        key(1);                                           /* F#3 */
        release(B_GLO); frames(16);
        check(mlu.on == 1u && !mlu.pick, "GLO + F#3: MIDI LEARN on");
        open_family(FAM_FX); frame();                     /* FX: its first page, KNOB 1 = DIST */
        frames(16);
        ui.force = 1; frame(); ppm("header-learn");
        K(EN_K1, 1);
        check(mlu.pick && mlu.id == P_DIST && mlu.trk == 0u && ml_arm, "LEARN: a knob turned picks its parameter (DIST)");
        midi_cc(&trk[0], 74, 10); frame();
        check(ml_count() == 1u && (ml_tab[0] & 127u) == 74u && (ml_tab[0] >> 9) == ml_code(P_DIST) && !mlu.pick,
              "LEARN: a CC sent: CC 74 = track 1 DIST");
        go_home(); frames(16);
        press(B_GLO); frames(10); key(1); release(B_GLO); frames(4);
        check(!mlu.on, "GLO + F#3 again: LEARN off");
        midi_cc(&trk[1], 74, 127);
        check(trk[0].p[P_DIST] == TP[P_DIST].max, "CC 74 (any channel): track 1's DIST to its top");
        trk[0].p[P_DIST] = 0;
        ui.menu = 1; ui.menu_sel = 0; ui.force = 1; frame();
        for (j = 0; j < 6u; j++) { encs[panel.enc[EN_SELECT]] = 1; frame(); }
        encs[panel.enc[EN_K2]] = 1; frame();
        tap(B_OCTUP);
        check(ui.menu_sel == MI_LCLEAR && ml_count() == 0u, "menu SYSTEM > MIDI LEARN CLEAR, OCT+: nothing learned any more");
        ui.menu = 0; ui.force = 1; go_home(); frames(16);
        {   /* GLO > MIDI: CH1 CH2 CH3 OTHER (2.6) */
            uint32_t hit = 0;
            WALK(FAM_GLO, "MIDI", hit);
            open_family(FAM_GLO); frame();
            for (j = 0; j < 12u && strcmp(PAGES[ui.page].title, "MIDI"); j++) { encs[panel.enc[EN_SELECT]] = 1; frame(); }
            frames(16);
            K(EN_K2, 9);                                  /* CH2: 2 -> 11 */
            check(hit && !strcmp(PAGES[ui.page].title, "MIDI") && song.g[G_CH2] == 11, "GLO > MIDI: the page, KNOB 2 sets track 2's channel (11)");
            ui.force = 1; frame(); ppm("page-midi");
            {   /* 2.6: every MIDI setting in MIDI and MIDI 2; no channel on two tracks */
                const page_t *m = cur_page(), *m2 = &PAGES[ui.page + 1u];
                const char *u;
                char vv[12];
                param_format(&GP[G_DRCH], 0, vv, &u);
                check(m->id[0] == G_CH1 && m->id[1] == G_CH2 && m->id[2] == G_CH3 && m->id[3] == G_DRCH && !strcmp(m2->title, "MIDI 2") &&
                      m2->id[0] == G_ROUTE && m2->id[1] == G_CHOTH && m2->id[2] == G_MIDI && m2->id[3] == G_BEND &&
                      !strcmp(GP[G_DRCH].label, "CH4") && !strcmp(vv, "OFF"),
                      "GLO > MIDI: CH1 CH2 CH3 CH4 (the drums); MIDI 2: IN OTHER OUT BEND; CH4 0 reads OFF");
                song.g[G_CH2] = 2, frame();
                K(EN_K2, 1);                              /* CH2 2 -> 3: track 3's: on to 4 */
                check(song.g[G_CH2] == 4 && !strcmp(ui.msg, "CH 3 = TRACK 3"), "GLO > MIDI: CH2 turned onto track 3's channel: on to 4, \"CH 3 = TRACK 3\"");
                K(EN_K2, -1);                             /* back: 3 taken, on to 2 */
                check(song.g[G_CH2] == 2, "  turned back: over 3 again, to 2");
                K(EN_K4, -7);                             /* DR CH 10 -> 3: track 3's, then 2, 1 (taken): OFF */
                check(song.g[G_DRCH] == 0, "  the drums turned down onto 3, 2, 1 (all taken): OFF");
                song.g[G_DRCH] = 10, frame();
                song.g[G_ROUTE] = 6;                      /* IN = CH 5 */
                go_home(); frame();
                open_family(FAM_GLO); frame();
                for (j = 0; j < 16u && strcmp(PAGES[ui.page].title, "MIDI"); j++) { encs[panel.enc[EN_SELECT]] = 1; frame(); }
                check(!strcmp(ui.msg, "IN CH 5: SELECTED TRK"), "GLO > MIDI with IN = CH 5: says that channel plays the selected track");
                song.g[G_ROUTE] = 0;
            }
            song.g[G_CH2] = 2;
            go_home(); frames(16);
        }
        #undef K
        #undef WALK
    }

    {   /* 3.0.5: SAVE > PROJECT (PARTS) and SONGS left the SAVE pages (the SONG screen has them) */
        uint32_t i, n = 0;
        for (i = 0; i < NPAGES; i++) n += PAGES[i].fam == FAM_SAVE && (PAGES[i].graph == GR_SLOTS || !strcmp(PAGES[i].title, "SONGS"));
        check(n == 0u, "SAVE pages: the sound's only (PRESETS, USER, TOOLS)");
    }
    {   /* fuzz: 20000 frames of random buttons (held or tapped), knobs and keys, with the audio running
         * between frames; every draw stays on the screen (lcd_blit / lcd_fill assert it) */
        uint32_t f, seed = 777, held = 0;
        #define R(n) ((seed = seed * 1664525u + 1013904223u) >> 8) % (n)
        song.sel = 0; go_home(); ui.force = 1;
        for (f = 0; f < 20000u; f++) {
            if (R(6) == 0) {
                uint32_t bt = R(NB);
                edges_btn |= 1u << panel.btn[bt];
                if (bt != B_HOME && R(3) == 0) held ^= 1u << panel.btn[bt];
            }
            if (R(30) == 0) held = 0;
            if (R(700) == 0) ly_lock = (uint8_t)R(LY_COUNT);              /* (a layer locked open, at random) */
            fm1_in.buttons = held & ~(1u << panel.btn[B_HOME]);
            if (R(4) == 0) encs[R(7)] += (int32_t)R(5) - 2;
            fm1_in.notes = R(10) == 0 ? (1u << R(27)) : (R(3) ? fm1_in.notes : 0);
            frame();
            if (ui.menu) { ui.menu = 0; ui.force = 1; }               /* (the menu is tested above) */
        }
        fm1_in.buttons = 0; fm1_in.notes = 0; frames(4);
        #undef R
    }
    {   /* (PHEBUS wave 1: the generators and PLAY are cut) no GEN, ACID GEN, HARMONY, DRUM GEN or PLAY page; DRUMS:
         * SEQ -> GRID -> KIT -> GRID */
        static const char *const GONE[9] = {"GEN", "GEN 2", "ACID GEN", "ACID GEN 2", "HARMONY", "DRUM GEN", "DRUM GEN 2", "PLAY", "SONG"};
        uint32_t k, j, none = 1;
        for (k = 0; k < NPAGES; k++)
            for (j = 0; j < 9u; j++)
                none &= strcmp(PAGES[k].title, GONE[j]) != 0;
        check(none, "no generator, PLAY or SONG page left");
        go_home(); song.playing = 0; ly_lock = LY_PLAY;
        track_select(TRK_DRUM); frames(2);
        studio_open(SC_DRUM); drum_page = 0; frames(2);
        tap(B_SEQ); frames(2);
        check(on_drum_page() && drum_page == 1u, "DRUMS: SEQ, GRID -> KIT");
        tap(B_SEQ); frames(2);
        check(on_drum_page() && drum_page == 0u, "DRUMS: SEQ on KIT -> back to the GRID");
        track_select(0); frames(2);
        go_home(); frames(2);
    }
    {   /* the PRESETS page on each track: its engines' sounds only */
        uint32_t t, k;
        static const char *const NM[3] = {"browse-t1", "browse-t2", "browse-t3"};
        for (t = 0; t < 3u; t++) {
            go_home(); frames(2);
            track_select(t); frames(2);
            for (k = 0; k < NPAGES; k++)
                if (PAGES[k].graph == GR_BROWSE)
                    ui.page = (uint8_t)k;
            ui.home = 0; ui.force = 1; frames(3);
            ppm(NM[t]);
        }
        go_home(); frames(2);
    }
    {   /* JUNO on track 1: its two EDIT pages */
        go_home(); song.playing = 0; ly_lock = LY_PLAY;
        track_select(0); frames(2);
        host_preset(&trk[0], ENGI_JUNO, 1); frames(2);
        check(!strcmp(ENGINES[trk[0].engine]->name, "JUNO"), "JUNO: the preset loads the engine");
        trk[0].p[P_MUTE] = 0; ui.fam_last[FAM_EDIT] = 0;
        open_family(FAM_EDIT); ui.force = 1; frames(3);
        ppm("juno-dco");
        encs[panel.enc[EN_SELECT]] = 1; ui.force = 1; frames(3);
        ppm("juno-vcf");
        go_home(); frames(2);
    }
    {   /* the sound chain on each voice (Ben, 20 Nov 2026: OSC > FILTER > ENV > FX > MOD, as the MiniFreak): EDIT opens
         * OSC, SELECT walks every page of it in stage order to MOD's last; every stage the voice has is visited, none
         * shown empty (DRUM TRACK); each page shot as chain-<voice>-<n> */
        static const char *const VN[5] = {"acid", "juno", "elem", "plaits", "drum"};
        uint32_t v, j, n, st, last, seen, want, empty;
        char nm[40];
        for (v = 0; v < 5u; v++) {
            go_home(); song.playing = 0; ly_lock = LY_PLAY; frames(2);
            track_select(v < 2u ? 0u : v - 1u); frames(2);
            if (v < 2u)
                host_preset(&trk[0], v ? ENGI_JUNO : ENGI_ACID, 0);
            if (v == 2u)                                 /* (track 2 starts on JUNO since 21 Nov: ELEMENTS for its shots) */
                host_preset(&trk[1], ENGI_ELEM, 3);
            frames(2);
            if (v == 4u) {
                studio_open(SC_DRUM); frames(2);
            }
            tap(B_EDIT); frames(2);
            check(on_chain() && fam_stage(cur_page()->fam) == 0 && stage_page(0, 0) == ui.page,
                  v == 4u ? "chain (drums): EDIT on the DRUM screen opens OSC (HEX OSC)" : "chain: EDIT opens OSC, its first page");
            for (want = 0, st = 0; st < NSTAGE; st++)
                want |= (uint32_t)(stage_page(st, 0) < NPAGES) << st;
            for (n = 0, last = 0, seen = 0, empty = 0, j = ~0u; n < 30u && j != ui.page; n++) {
                j = ui.page;
                st = (uint32_t)fam_stage(cur_page()->fam);
                if (st < last) last = 99u;
                else last = st;
                seen |= 1u << st;
                empty |= is_drum(TSEL) && !page_for_drum(cur_page());
                ui.force = 1; frames(2);
                snprintf(nm, sizeof nm, "chain-%s-%02u", VN[v], (unsigned)n);
                ppm(nm);
                printf("chain %-6s %2u %-5s %-9s %-6s %-6s %-6s %-6s ok\n", VN[v], (unsigned)n, STAGE_NAME[st], cur_page()->title,
                       opk.lab[0], opk.lab[1], opk.lab[2], opk.lab[3]);
                encs[panel.enc[EN_SELECT]] = 1; frame();
            }
            check(last != 99u && seen == want && !empty && (v == 4u ? want == 15u : want == 31u),
                  v == 4u ? "chain (drums): OSC FILT ENV FX in order, no MOD, nothing empty" : "chain: OSC FILT ENV FX MOD in order");
        }
        track_select(TRK_DRUM); frames(2); studio_open(SC_DRUM); frames(2); tap(B_EDIT); frames(2);
        j = ui.page; tap(B_LFO); frames(1);
        check(ui.page == j && !strcmp(ui.msg, "NO LFO ON DRUMS"), "chain (drums): LFO: NO LFO ON DRUMS, the page stays");
        tap(B_ENV); frames(1);
        check(!strcmp(cur_page()->title, "HEX AEG"), "chain (drums): ENV opens the sound's envelope (HEX AEG)");
        encs[panel.enc[EN_ALGO]] = -1; frames(2);
        check(song.sel == 2u && !strcmp(cur_page()->title, "ENV"), "chain: ALGORITHM to a synth on ENV: its ENV");
        tap(B_LFO); frames(1); encs[panel.enc[EN_ALGO]] = 1; frames(2);
        check(song.sel == TRK_DRUM && on_chain() && fam_stage(cur_page()->fam) == 3, "chain: on MOD, to the drums: their FX (the stage before)");
        track_select(0); host_preset(&trk[0], ENGI_ACID, 0); go_home(); frames(2);
    }
    {   /* PHEBUS (review 8, Ben 21 Nov: "Sound big, character small"): each sound page draws what its knobs do, big, and
         * every knob moves the picture; the character small at the top right; each page shot at three settings */
        static const struct { uint8_t trk, eng, pre, fam; const char *title, *shot; } SP[] = {
            {0, ENGI_JUNO, 11, FAM_OSC, "OSC", "osc-juno"}, {0, ENGI_JUNO, 11, FAM_FLT, "FILTER", "filter-juno"},
            {0, ENGI_JUNO, 11, FAM_OSC, "VOICE", "voice"}, {0, ENGI_JUNO, 11, FAM_OSC, "VOICE 2", "voice2"},
            {0, ENGI_JUNO, 11, FAM_ENV, "ENV DEST", "envdest"}, {0, ENGI_JUNO, 11, FAM_FX, "FX", "fx-sends"},
            {0, ENGI_JUNO, 11, FAM_FX, "CHORUS", "chorus"}, {0, ENGI_JUNO, 11, FAM_FX, "INSERT", "insert"},
            {0, ENGI_JUNO, 11, FAM_FX, "INSERT 2", "insert2"}, {0, ENGI_JUNO, 11, FAM_MOD, "LFO DEST", "lfodest"},
            {0, ENGI_ACID, 0, FAM_OSC, "OSC", "osc-acid"}, {0, ENGI_ACID, 0, FAM_FLT, "FILTER", "filter-acid"},
            {0, ENGI_ACID, 0, FAM_FLT, "FILTER 2", "filter-trk"}, {2, ENGI_PRISM, 58, FAM_OSC, "OSC", "osc-va"},
            {2, ENGI_PRISM, 0, FAM_OSC, "OSC", "osc-fold"}, {2, ENGI_PRISM, 58, FAM_OSC, "OSC 2", "osc2-plaits"},
            {1, ENGI_ELEM, 3, FAM_OSC, "OSC", "osc-elem"}, {3, 0, 0, FAM_FLT, "FILTER", "filter-drum"},
            {3, 0, 0, FAM_FX, "INSERT", "insert-drum"}};
        uint32_t c, k, ok_all = 1, chr = 1;
        char nm[40], what[120];
        song.playing = 0;
        for (c = 0; c < sizeof SP / sizeof SP[0]; c++) {
            track_t *t = &trk[SP[c].trk];
            const page_t *pg;
            uint32_t ok = 1, set;
            go_home(); track_select(SP[c].trk); frames(2);
            if (SP[c].trk < 3u) host_preset(t, SP[c].eng, SP[c].pre);
            t->p[P_ITYPE] = 1; t->p[P_IA] = 64; t->p[P_IMIX] = 100;   /* (INSERT SOFT: A, B, C and MIX at work) */
            if (SP[c].eng == ENGI_JUNO) t->p[P_E0] = 2;              /* (S+P: PW at work) */
            if (SP[c].eng == ENGI_ACID) t->p[P_E3] = 60;             /* (ENV: DEC at work) */
            t->p[P_VOICE] = V_MONO; t->p[P_GLIDE] = 60;              /* (PRIO, GLMODE at work) */
            if (!is_drum(t)) t->p[P_TFLT] = 0;
            frames(16);
            if (!chain_goto(SP[c].fam, SP[c].title)) {
                snprintf(what, sizeof what, "sound page %s: reached", SP[c].shot); check(0, what); continue;
            }
            pg = cur_page();
            ui.force = 1; frames(2);
            chr &= px_count(196, 22, 44, 38, TE_COL[SP[c].trk & 3u]) > 40u;
            for (k = 0; k < 4u; k++) {                  /* every knob: the picture moves */
                const param_desc_t *d;
                int16_t v0;
                uint32_t h0, h1;
                int32_t st;
                if (pg->id[k] >= P_COUNT || (d = track_desc(t, pg->id[k]))->max <= d->min) continue;
                v0 = t->p[pg->id[k]];
                st = (d->max - d->min) / 3 > 1 ? (d->max - d->min) / 3 : 1;
                ui.force = 1; frames(2); h0 = plot_hash();
                t->p[pg->id[k]] = (int16_t)(v0 + st <= d->max ? v0 + st : v0 - st);
                ui.force = 1; frames(2); h1 = plot_hash();
                t->p[pg->id[k]] = v0;
                if (h0 == h1) { printf("ui:   %s: knob %u (%s) moves nothing\n", SP[c].shot, (unsigned)k + 1, d->label); ok = 0; }
            }
            snprintf(what, sizeof what, "sound page %s: each knob moves its picture", SP[c].shot);
            check(ok, what);
            ok_all &= ok;
            for (set = 0; set < 3u; set++) {            /* three settings: as it is, a third and five sixths of each */
                for (k = 0; set && k < 4u; k++) {
                    const param_desc_t *d;
                    if (pg->id[k] >= P_COUNT || pg->id[k] == P_E0 && SP[c].eng == ENGI_PRISM) continue;
                    d = track_desc(t, pg->id[k]);
                    t->p[pg->id[k]] = (int16_t)(d->min + (d->max - d->min) * (set == 1u ? 2 : 5) / 6);
                }
                ui.force = 1; frames(2);
                snprintf(nm, sizeof nm, "snd-%s-%c", SP[c].shot, 'a' + (int)set);
                ppm(nm);
            }
        }
        check(chr, "sound pages: the voice's character small at the top right");
        go_home(); track_select(0); host_preset(&trk[0], ENGI_ACID, 0); frames(2);
        trk[0].p[P_VOICE] = V_LEGATO; trk[0].p[P_GLIDE] = 0; trk[0].p[P_ITYPE] = 0;
    }
    {   /* PHEBUS (21 Nov 2026): TOOLS > NEW: every send and effect off, the STUTTER lane empty */
        uint32_t ok = 1, l;
        for (i = 0; i < NPART; i++) trk[i].p[P_REV] = 50, trk[i].p[P_DLY] = 40, trk[i].p[P_DIST] = 20;
        drum_send[3][0] = 2; song.g[G_SWING] = 30; song.g[G_DRREV] = 20; stl.on[0] = 5; stl.len = 40;
        project_new(); frames(4);
        for (i = 0; i < NPART; i++)
            ok &= !trk[i].p[P_DIST] && !trk[i].p[P_CHOR] && !trk[i].p[P_DLY] && !trk[i].p[P_REV] && !trk[i].p[P_ITYPE] &&
                  !trk[i].p[P_SSWING] && trk[i].eng_req == TRK_DEF[i][0] && trk[i].preset == TRK_DEF[i][1];
        for (l = 0; l < DRUM_LANES; l++)
            ok &= !drum_send[l][0] && !drum_send[l][1];
        ok &= !TDRUM->p[P_DIST] && !TDRUM->p[P_DLY] && !TDRUM->p[P_REV] && !TDRUM->p[P_ITYPE] && !song.g[G_DRREV] &&
              !song.g[G_DRDLY] && !song.g[G_SWING] && !song.g[G_DUST] && !song.g[G_DUCK] && !song.g[G_FILT];
        check(ok, "TOOLS > NEW: the default sounds dry, every send 0, no swing, master FX off");
        check(plain_defaults(), "TOOLS > NEW: the plain saws again (ACID INIT bass, JUNO chords, PLAITS VA lead)");
        check(!stl.on[0] && stl.len == 16u && stl.act, "TOOLS > NEW: the STUTTER lane empty, 16 steps, ON");
    }
    printf("ui: %s\n", fails ? "FAILED" : "pages, layers (punch, steps, erase, roll, key, mix), layer lock, song layer, REC hold, drums, REC, 20000-frame fuzz PASS");
    return fails;
}
#endif
