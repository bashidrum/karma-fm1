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
#define PHEBUS_UI 1                             /* (the UI is in: ui_menu.c's dev_desc) */
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
#ifndef REAL_PROJECT                               /* (tests/bank_test.c: project.c itself, on a simulated NOR) */
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
/* (the bank) a stand-in: the bank in RAM. The harness keeps a flag and the summary; the simulator (web/emu/sloop_web.c) sets
 * bank_hook to keep and swap the real working project in RAM (project.c's capture and apply) */
static uint32_t bank_ops[3];
static uint8_t bank_has, bank_np;
static int16_t bank_bpm;
static char bank_kit[9];
static int (*bank_hook)(uint32_t op);              /* (the simulator: does the action; 0 done) */
static void project_new(void);                     /* (ui_input.c) */
static int bank_op(uint32_t op)
{
    if (song.playing || transport_req)
        return 2;
    bank_ops[op % 3u]++;
    if (op == 1u && !bank_has)
        return 1;
    if (bank_hook)
        return bank_hook(op);
    if (op == 0u) {
        bank_has = 1, bank_np = 2, bank_bpm = song.g[G_BPM];
        str_cpy(bank_kit, "CUSTOM", sizeof bank_kit);
    } else if (op == 2u) {
        project_new();
    }
    return 0;
}
static int bank_summary(uint32_t *npat, int32_t *bpm, char *kit)
{
    *npat = bank_np, *bpm = bank_bpm;
    str_cpy(kit, bank_kit, 9);
    return bank_has;
}
#endif
static int up_used(uint32_t k) { return k < 2; }
static int up_load(uint32_t k) { (void)k; return 0; }
static int up_load_to(track_t *t, uint32_t k) { (void)t; (void)k; return 0; }
static uint32_t up_count(void) { return 2; }
static uint32_t up_nth(uint32_t n) { return n; }
static uint32_t up_rank(uint32_t s) { return s; }
static uint32_t up_engine(uint32_t k) { return k ? ENGI_PRISM : ENGI_ACID; }   /* (MY LEAD on track 1, MY PAD on 3) */
static void up_name(uint32_t k, char *b) { str_cpy(b, k ? "MY PAD" : "MY LEAD", 13); }
static void up_slot_label(char *b, uint32_t k) { fmt_int(b, (int32_t)k + 1); }
static uint32_t up_ops[3];
static void up_ui(uint32_t op, uint32_t k) { up_ops[op % 3u]++; (void)k; }
#ifndef REAL_PROJECT
static void settings_save(void) {}
#endif
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
static uint32_t frame_blocks = 22u;                /* (wave 11: the lights' trials run fewer audio blocks a frame) */
static void frame(void)
{
    uint32_t q;
    static int32_t o[CTL * 2];
    for (q = 0; q < frame_blocks; q++) {
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
/* (wave 9) what the old buttons' helpers did, for the tests that walk every page (the buttons now step through categories,
 * ui_nav.c): the chain's pages in signal order; a drum screen page; the families EDIT reached */
static uint32_t chain_list(uint8_t *idx, uint32_t max)
{
    uint32_t st, k, i, n = 0;
    for (st = 0; st < NSTAGE; st++)
        for (k = 0; n < max && (i = stage_page(st, k)) < NPAGES; k++)
            idx[n++] = (uint8_t)i;
    return n;
}
static void drum_open(uint32_t fam) { drum_screen_open(); dpg_go(fam == FAM_ENV ? 3u : fam == FAM_LFO ? 4u : 1u, 0); }
static int sg_fam_in(uint32_t pfam, uint32_t fam)
{
    return fam == FAM_EDIT ? pfam == FAM_OSC || pfam == FAM_FLT || pfam == FAM_ENV : pfam == fam;
}
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
              !t->p[P_LD_PIT] && !t->p[P_LD_FLT] && !t->p[P_LD_SHP] && !t->p[P_LD_AMP] &&
              !t->p[P_ED_PIT] && !t->p[P_ED_SHP] && t->p[P_SUS] == 127 && t->p[P_ATK] <= 10 && t->p[P_REL] <= 20 &&
              !t->p[P_SLCR] && !t->p[P_TFLT] && !t->p[P_AMODE] && t->p[P_FTYPE] == VF_LP24 && !t->p[P_OCT] && !t->p[P_TUNE];
        /* (wave 4) the voice filter: ACID's 303 speaks (its envelope, OWN, a decay, a little ENV amount: the ENVMOD 55 it
         * had), JUNO and PLAITS open, no envelope on it */
        ok &= i == 0u ? t->p[P_ED_FLT] > 0 && t->p[P_FLINK] == 1 && t->p[P_FCUT] == 45 && t->p[P_FRES] == 45 && !t->p[P_FSUS]
                      : !t->p[P_ED_FLT] && t->p[P_FCUT] == 127 && !t->p[P_FRES] && !t->p[P_FLINK];
    }
    return ok;
}

/* PHEBUS (review 8, Ben 21 Nov: "Sound big, character small"): the sound pages' plot as a hash (the graph canvas from
 * the plot's top, the top-right corner (where the character was) left out), and a page of the chain by its title */
static uint32_t plot_hash(void)
{
    uint32_t x, y, h = 2166136261u;
    for (y = 22u + 20u; y < 22u + 118u; y++)
        for (x = 0; x < 240u; x++)
            if (x < 190u || y > 22u + 40u)
                h = (h ^ screen[y * 240u + x]) * 16777619u;
    return h;
}
/* (wave 9) a page reached the way Ben moves: its group's button until its category shows, then SEL (PAGE) until it does */
static uint32_t cur_cat(void) { uint32_t k, n; return nav_cat(&k, &n); }
static int chain_goto(uint32_t fam, const char *title)
{
    uint32_t j, i, b;
    for (i = 0; i < NPAGES && !(PAGES[i].fam == fam && !strcmp(PAGES[i].title, title) && page_shown(&PAGES[i])); i++)
        ;
    if (i >= NPAGES)
        return 0;
    if (on_drum_page()) { go_home(); frame(); }
    b = nav_btn_of(PAGES[i].cat);
    if (b >= NB)
        return 0;
    for (j = 0; j < 8u && cur_cat() != PAGES[i].cat; j++) { tap(b); }
    for (j = 0; j < 8u && ui.page != i; j++) { tap(B_SCL); }
    return ui.page == i && !ui.home;
}

static void lane_set_ui(uint32_t s, uint32_t code)   /* a STUTTER lane step lit at a length code */
{
    stl.on[s >> 3] |= (uint8_t)(1u << (s & 7u));
    stl.lc[s >> 2] = (uint8_t)((stl.lc[s >> 2] & ~(3u << 2u * (s & 3u))) | (code & 3u) << 2u * (s & 3u));
}
#ifndef UI_PAGES_HARNESS_ONLY                       /* (tests/stress_test.c: the harness, its own main) */

/* ---- (wave 11) the lights against the behaviour: what a control does, as state (UI, sequencer, matrix, mixer, the keys'
 * jobs, the voices sounding) */
static uint32_t h32(uint32_t h, const void *p, uint32_t n)
{
    const uint8_t *b = p;
    while (n--) h = (h ^ *b++) * 16777619u;
    return h;
}
static uint32_t st_part[6];
static uint32_t lt_hold_k = 99u;                    /* (the lights' trials: the key held, its own job not counted) */                         /* (the parts of the last hash: which one differs) */
static uint32_t st_hash(void)
{
    uint32_t h = 2166136261u, i, v[26];
    uint8_t kk[27];
    v[0] = (uint32_t)song.octave + 7u, v[1] = song.playing | transport_req << 2, v[2] = fill_held | fill_lat << 1 | fill_arm << 2;
    v[3] = perf.held, v[4] = perf.lat, v[5] = ui.page | ui.home << 8 | ui.ask << 9 | ui.menu << 12, v[6] = (uint32_t)live_req + 3u;
    v[7] = (uint32_t)live_sec + 3u, v[8] = sec_stores, v[9] = undo.valid | undo.undone << 1 | undo.trk << 2;
    v[10] = drum_mute | (uint32_t)drum_solo << 16, v[11] = song.solo | mix_pan << 8, v[12] = map_lfo2 | map_fenv << 1;
    v[13] = ly_lock, v[14] = fx_wet, v[15] = (uint32_t)(mac_env[0] + 100) | (uint32_t)(mac_dep[0] + 100) << 8;
    v[16] = syn_tab | syn_pg << 8, v[17] = drum_lane, v[18] = sg_held, v[19] = (uint32_t)song.sel | drum_keys << 4;
    v[20] = cur_cat(), v[21] = sr_on, v[22] = syn_half[0] | (uint32_t)syn_hcur() << 8, v[23] = drum_pg | dpg_half() << 8;
    v[24] = stl_pg | (uint32_t)stl_new << 8 | (uint32_t)stl_view << 16 | (uint32_t)stl_held << 17, v[25] = song.rec | (uint32_t)rec_wait << 8;   /* (wave 14) */
    st_part[0] = h32(h, v, sizeof v);
    h = 2166136261u;
    for (i = 0; i < NTRK; i++) {
        h = h32(h, trk[i].p, sizeof trk[i].p);
        h = h32(h, trk[i].step, sizeof trk[i].step);
        h = h32(h, trk[i].lock, sizeof trk[i].lock);
    }
    h = h32(h, &dpat, sizeof dpat);
    h = h32(h, &stl, sizeof stl);                   /* (wave 14: the STUTTER lane's steps and pads) */
    for (i = 0; i < NPART; i++)                     /* (the stored part: not LFO 2's phase, the last voice's sources) */
        h = h32(h, &mxt[i], (uint32_t)((const uint8_t *)&mxt[0].l2ph - (const uint8_t *)&mxt[0]));
    st_part[1] = h;
    for (i = 0; i < 27u; i++)                       /* (a key the UI took and did nothing with: none) */
        kk[i] = kb_kind[i] == KS_UI || (kb_kind[i] == KS_FX && !kb_n[i]) || i + 14u == lt_hold_k ? KS_NONE : kb_kind[i];   /* (FX: a key
                                                    * with no pad; the key held itself: not what is pressed) */
    h = h32(2166136261u, kk, sizeof kk);
    for (i = 0; i < NROLL; i++)
        h = h32(h, &roll[i].on, 1);
    st_part[2] = h;
    h = 2166136261u;
    for (i = 0; i < NPART; i++) {
        uint32_t j;
        for (j = 0; j < NVOICE; j++)
            if (trk[i].v[j].active)
                h = h32(h, &trk[i].v[j].note, 1), h = h32(h, &trk[i].v[j].vel, sizeof trk[i].v[j].vel);
    }
    st_part[3] = h;
    st_part[4] = h32(2166136261u, &drums, sizeof drums);
    st_part[5] = ui.msg_t ? h32(2166136261u, ui.msg, (uint32_t)strlen(ui.msg)) : 0u;
    for (i = 0, h = 0; i < 6u; i++)
        h = h * 31u + st_part[i];
    return h;
}
static void ctl_down(uint32_t c) { if (c < 14u) { edges_btn |= BT(c); fm1_in.buttons |= BT(c); } else fm1_in.notes |= 1u << (c - 14u); frame(); }
static void ctl_up(uint32_t c) { if (c < 14u) fm1_in.buttons &= ~BT(c); else fm1_in.notes &= ~(1u << (c - 14u)); frame(); }
static void (*lt_setup)(void);                     /* a screen, a sound, a pattern: before the hold */
static uint32_t lt_hold;                           /* what is held: a control (0..40) */
static uint32_t lt_nosound, lt_step;
/* mode 0: the hold alone; 1: the hold, then c pressed and let go, then the hold let go; 2: the hold let go, then c (at the same
 * moment as 1). Returns the state's hash (at c's press and at the end); *lv: c's light while the hold is on (mode 1) */
static uint32_t lt_trial(uint32_t c, uint32_t mode, uint32_t *lv, uint32_t *ctx)
{
    int fd[2];
    pid_t pid;
    uint32_t r[3] = {0, 0, 0};
    fflush(stdout);
    if (pipe(fd))
        return 0;
    pid = fork();
    if (!pid) {
        uint32_t h1, i;
        close(fd[0]);
        frame_blocks = 3u;
        lt_setup();
        ctl_down(lt_hold);
        frames(12);
        r[1] = led_lv[c], r[2] = led_ctx;
        if (mode == 2u)
            ctl_up(lt_hold);
        else
            frame();
        if (mode)
            ctl_down(c);
        else
            frame();
        frame();
        st_hash();
        h1 = ((st_part[0] * 31u + st_part[1]) * 31u + st_part[2]) * 31u + st_part[5];   /* (at the press: no sound yet) */
        if (getenv("LT_DEBUG"))
            printf("    press parts %08x %08x %08x %08x kinds", st_part[0], st_part[1], st_part[2], st_part[5]);
        if (getenv("LT_DEBUG")) { for (i = 0; i < 27u; i++) printf(" %u", kb_kind[i]); printf(" page %u cat %u\n", ui.page, cur_cat()); fflush(stdout); }
        if (mode)
            ctl_up(c);
        else
            frame();
        frames(3);
        st_hash();                                     /* (c let go, the hold still on: what a tap of c did) */
        h1 = h1 * 31u + ((st_part[0] * 31u + st_part[1]) * 31u + st_part[2]) * 31u + st_part[5];
        if (getenv("LT_DEBUG"))
            printf("    after parts %08x %08x %08x %08x\n", st_part[0], st_part[1], st_part[2], st_part[5]), fflush(stdout);
        if (mode != 2u)
            ctl_up(lt_hold);
        else
            frame();
        frames(6);
        for (i = 0; i < 6u; i++)
            if (fm1_in.buttons || fm1_in.notes) break;
        st_hash();
        if (lt_nosound)                                /* (a key held: its own note sounds differently long; not compared) */
            st_part[3] = st_part[4] = 0;
        r[0] = h1 * 31u + ((((st_part[0] * 31u + st_part[1]) * 31u + st_part[2]) * 31u + st_part[3]) * 31u + st_part[4]) * 31u + st_part[5];
        if (getenv("LT_DEBUG")) {
            uint32_t q;
            for (q = 0; q < NTRK; q++)
                printf("    trk%u p %08x st %08x lk %08x\n", (unsigned)q, h32(1, trk[q].p, sizeof trk[q].p), h32(1, trk[q].step, sizeof trk[q].step), h32(1, trk[q].lock, sizeof trk[q].lock));
            printf("    dpat %08x mxt %08x\n", h32(1, &dpat, sizeof dpat), h32(1, mxt, sizeof mxt));
        }
        if (getenv("LT_DEBUG"))
            printf("    [%u %u] parts %08x %08x %08x %08x %08x %08x msg '%s'\n", (unsigned)c, (unsigned)mode, st_part[0], st_part[1], st_part[2], st_part[3], st_part[4], st_part[5], ui.msg_t ? ui.msg : ""), fflush(stdout);
        if (write(fd[1], r, sizeof r) != (ssize_t)sizeof r) _exit(1);
        _exit(0);
    }
    close(fd[1]);
    if (read(fd[0], r, sizeof r) != (ssize_t)sizeof r) r[0] = 1u;
    close(fd[0]);
    waitpid(pid, 0, 0);
    if (lv) *lv = r[1];
    if (ctx) *ctx = r[2];
    return r[0];
}

static void lt_pattern(void)                       /* voice 1: notes on C4 E4 G4; the drums: kick, snare, hat */
{
    uint32_t j;
    track_t *t = &trk[0];
    steps_clear(t);
    t->p[P_SLEN] = 16;
    for (j = 0; j < 16u; j += 2u) { t->step[j].n = 1; t->step[j].note[0] = (uint8_t)(60 + (j % 3u) * 4u); t->step[j].time = ST_NOTE; t->step[j].vel = 100; }
    dpat_clear(&dpat);
    for (j = 0; j < 16u; j += 4u) { dt_set(j, 0, 100); dt_set(j + 2u, 2, 90); }
    ly_lock = LY_PLAY; perf_unlatch(); fill_lat = 0; song.octave = 0; ui.ask = 0; ui.msg_t = 0;
}
static void su_filt(void) { lt_pattern(); song.sel = 0; go_home(); frames(2); chain_goto(FAM_FLT, "FILT"); frames(2); }
static void su_lfo2(void) { lt_pattern(); song.sel = 0; go_home(); frames(2); chain_goto(FAM_MOD, "LFO 2"); frames(2); }
static void su_home(void) { lt_pattern(); song.sel = 0; go_home(); frames(4); }
static void su_home_drum(void) { lt_pattern(); song.sel = TRK_DRUM; go_home(); frames(4); }
static void su_grid(void) { lt_pattern(); song.sel = 0; go_home(); frames(2); tap(B_SEQ); frames(3); }
static void su_dscreen(void) { lt_pattern(); song.sel = TRK_DRUM; go_home(); frames(2); drum_screen_open(); frames(3); }
static void su_perf_lat(void) { lt_pattern(); song.sel = 0; go_home(); frames(2); perf.lat = 1u; }
/* (wave 14) FX's STUT: the STUTTER lane on voice 1 (the voice stays selected), a lit step and a recorded pad on it */
static void su_lane(void)
{
    lt_pattern(); memset(&stl, 0, sizeof stl); stl.len = 16; stl.act = 1; lane_set_ui(4, 1);
    stl.ev[0] = (pev_t){PF_S8 + 1u, 1, 2, 0};
    song.sel = 0; go_home(); frames(2); nav_go(CT_STUT, 0); frames(3);
}
static void su_lane_s(void) { su_lane(); }          /* (the same, for a lane step held: lt_step's comparison) */
static void su_lane_drum(void) { su_lane(); track_select(TRK_DRUM); frames(3); }
static uint32_t lt_bad;
static void lights_dump(const char *name)          /* (wave 11) a scenario's 41 levels into DIR/lights.txt (tools/lights_sheet.py) */
{
    char path[512];
    uint32_t c;
    FILE *f;
    snprintf(path, sizeof path, "%s/lights.txt", outdir);
    f = fopen(path, "a");
    if (!f) return;
    fprintf(f, "%s:", name);
    for (c = 0; c <= 40u; c++) fprintf(f, "%u", led_lv[c]);
    fprintf(f, ":%u\n", (unsigned)disco);
    fclose(f);
}
/* every control under the hold of setup su, held control hc: lit <=> it does something with it (not what it does alone) */
static uint32_t lt_check(const char *name, void (*su)(void), uint32_t hc, uint32_t *litset)
{
    uint32_t c, h0, bad = 0, nlit = 0, ctx = 0;
    lt_setup = su, lt_hold = hc, lt_nosound = hc >= 14u, lt_hold_k = hc, lt_step = su == su_grid || su == su_dscreen || su == su_lane_s;
    if (getenv("LT_ONLY") && !strstr(name, getenv("LT_ONLY")))
        return 0;
    h0 = lt_trial(0, 0, 0, &ctx);
    if (getenv("LT_DEBUG")) printf("  ^ none\n");
    for (c = 0; c <= 40u; c++) {
        uint32_t lv, hw, ha, did, lit;
        if (c == hc || c == B_HOME)                    /* (the held one; HOME: acts on its release, never under a hold) */
            continue;
        if (getenv("LT_CTL") && (uint32_t)atoi(getenv("LT_CTL")) != c)
            continue;
        if ((c == B_PLAY && hc != B_SEQ) || (c == B_REC && lt_step))   /* (PLAY is the transport under every hold but SEQ's, REC
                                                       * records: what they do alone, not combos) */
            continue;
        hw = lt_trial(c, 1, &lv, 0);
        ha = lt_step ? 0u : lt_trial(c, 2, 0, 0);     /* (a step held: what a button does is its lock page; against none) */
        did = hw != h0 && (lt_step || hw != ha);
        lit = lv == LL_LIT;
        nlit += lit;
        if (lit)
            litset[c >> 5] |= 1u << (c & 31u);
        if (did != lit) {
            printf("  %s: %s%s %s\n", name, c < 14u ? B_NAME[c] : "key ", c < 14u ? "" : (char[3]){(char)('0' + (c - 14u) / 10u), (char)('0' + (c - 14u) % 10u), 0},
                   lit ? "lit but does nothing with it" : "does something with it but dark");
            bad++;
        }
    }
    lt_bad += bad;
    return nlit;
}

/* (PHEBUS, Ben 23 Nov 2026, 14:22: the screensavers' lines "come over very harsh ... super infrequent ... I don't like the
 * white one at all ... a different one at random every time") the tears are rare and gentle, no picture holds a line of one
 * bright colour across it, the pick is random, the caption says where the others are. Each picture is held still (levels,
 * clock) so that a frame differs from its reference only by a tear: the reference is painted at the start of a 32.8 s slot,
 * where the slot's own tear never is (it comes 10 to 22 s in) */
static uint32_t vr_line(void)                       /* 1: a row of the screen of one bright colour, full width (230 of 240 pixels) */
{
    uint32_t y, x, n;
    for (y = 0; y < 240u; y++) {
        uint16_t c = screen[y * 240u];
        uint32_t cc = swap16(c), lum = (cc >> 11) * 2u + ((cc >> 5) & 63u) + (cc & 31u) * 2u;   /* 0..187 */
        if (lum < 90u)
            continue;
        for (n = 0, x = 0; x < 240u; x++)
            n += screen[y * 240u + x] == c;
        if (n >= 230u)
            return 1;
    }
    return 0;
}
static uint32_t vr_loud;
static void vr_paint(uint32_t st, uint32_t ms)
{
    uint32_t i;
    vis_style = (uint8_t)st;
    fm1_ms = ms;
    for (i = 0; i < 4u; i++) { vs.e[i] = (uint16_t)(vr_loud ? 256u : 60u + i * 40u); vs.flash[i] = 0; }
    vs.kick = vr_loud ? 200u : 0u;
    vs.tq = (st * 900u + 200u) << 8;
    vs.rip = 17u;
    memset(ey, 0, sizeof ey);
    vis_name_t = 0;
    vis_paint();
}
/* n frames of picture st, 32 ms apart from ms0; kicks: a kick every 500 ms. Returns the frames with a torn row; *runs: how
 * many separate events; *kt: the kick-started tears; *line: frames with a bright full-width row */
static uint32_t vr_run(uint32_t st, uint32_t n, uint32_t kicks, uint32_t ms0, uint32_t *runs, uint32_t *kt, uint32_t *line)
{
    static uint16_t ref[240 * 240];
    uint32_t f, torn = 0, prev = 0, i;
    int32_t lv[4] = {60, 100, 140, 180};
    vs.kt = 0;
    vr_paint(st, ms0 & ~32767u);
    memcpy(ref, screen, sizeof ref);
    *runs = *kt = *line = 0;
    for (f = 0; f < n; f++) {
        uint32_t ms = ms0 + f * 32u, d = 0, k0 = vs.kt;
        if (kicks && f % 16u == 0u) {               /* (a kick on every beat: 120 BPM) */
            vis_kick_hit = 1;
            fm1_ms = ms;
            vis_step(lv, 1u);                       /* (it may start a tear: vs.kt) */
            *kt += vs.kt != k0;
        }
        vr_paint(st, ms);
        for (i = 0; i < 240u * 240u; i++)
            d += screen[i] != ref[i];
        torn += d != 0u;
        *runs += d != 0u && !prev;
        prev = d != 0u;
        *line += vr_line();
    }
    return torn;
}
static void vis_rare_checks(void)
{
    uint32_t st, torn, runs, kt, line, i, seen, n;
    char nm[96];
    uint32_t ms0 = fm1_ms, vl = vis_last, vrng = vis_rng;
    uint8_t vst = vis_style, von = vis_on, vnt = vis_name_t, bv = boot_vis;
    uint32_t kt_all = 0, runs_all = 0;
    for (st = 0; st < VIS_N; st++) {                /* 2000 frames (64 s) of each picture, nothing playing */
        torn = vr_run(st, 2000u, 0u, 200000u, &runs, &kt, &line);
        snprintf(nm, sizeof nm, "tears %-9s idle: %u of 2000 frames torn in %u event(s) (a few rows, one event at most)", VIS_NAME[st], torn, runs);
        check(torn <= 16u && runs <= 3u, nm);       /* (64 s: one or two events of 5 frames; at most 0.8% of the frames) */
        snprintf(nm, sizeof nm, "lines %-9s idle: no frame has a bright full-width row (%u)", VIS_NAME[st], line);
        check(line == 0u, nm);
    }
    for (st = 0; st < VIS_N; st++) {                /* playing: a kick on every beat for 4000 frames (128 s, 64 bars) */
        torn = vr_run(st, 4000u, 1u, 300000u, &runs, &kt, &line);
        kt_all += kt;
        runs_all += runs;
        snprintf(nm, sizeof nm, "tears %-9s kick on every beat: %u of 4000 frames torn, %u events, %u from kicks", VIS_NAME[st], torn, runs, kt);
        check(torn <= 60u && runs <= 12u && kt <= 4u, nm);   /* (kicks: under 1 in 16 bars; 8 s apart at the closest) */
        snprintf(nm, sizeof nm, "lines %-9s kick on every beat: no bright full-width row (%u)", VIS_NAME[st], line);
        check(line == 0u, nm);
    }
    printf("ui:   (tears: kick-started %u in 4 x 64 bars, events %u in 4 x 128 s playing)\n", kt_all, runs_all);
    {   /* the morph and the field's rows too: the clock run through a whole cycle (15 s each) of FLOW and THIRD EYE */
        uint32_t cyc, bad = 0;
        for (st = 0; st < VIS_N; st += 3u)
            for (cyc = 0; cyc < 12u; cyc++) {
                vr_paint(st, 200000u);
                vs.tq = (uint32_t)(cyc * 300u + 10u) << 8;
                vis_paint();
                bad += vr_line();
            }
        check(bad == 0u, "lines FLOW, THIRD EYE: the morph (a dissolve, it was a white wipe) leaves no bright full-width row");
    }
    {   /* pictures for the review: the dissolve half way, and a tear (FLOW, the first torn frame found) */
        static uint16_t r0[240 * 240];
        uint32_t ms;
        vr_paint(0, 200000u); vs.tq = (uint32_t)(600u + 60u) << 8; vis_paint(); ppm("vis-flow-dissolve");
        vr_paint(0, 200000u & ~32767u); memcpy(r0, screen, sizeof r0);
        for (ms = 200000u; ms < 300000u; ms += 16u) {
            vr_paint(0, ms);
            if (memcmp(r0, screen, sizeof r0)) { ppm("vis-flow-tear"); break; }
        }
    }
    {   /* loud: every voice at the top and a kick; the clock through a few moments of each picture */
        uint32_t bad = 0, k;
        vr_loud = 1;
        for (st = 0; st < VIS_N; st++)
            for (k = 0; k < 40u; k++) {
                vr_paint(st, 200000u);
                vs.tq = (uint32_t)(k * 97u + st * 13u) << 8;
                vis_paint();
                bad += vr_line();
            }
        vr_loud = 0;
        check(bad == 0u, "lines, all five pictures, all voices loud with a kick: no bright full-width row");
    }
    for (st = 0; st < VIS_N; st++) {                /* the boot's pictures too */
        uint32_t t, bad = 0;
        boot_vis = (uint8_t)st; flow_seed = 0x2462u;
        for (t = 0; t < 180u; t += 9u) { boot_frame(t); bad += vr_line(); }
        snprintf(nm, sizeof nm, "boot %-9s: no bright full-width row in any frame", VIS_NAME[st]);
        check(bad == 0u, nm);
    }
    {   /* the pick at boot: from the seed, never the one shown last, all four come up */
        uint32_t cnt[VIS_N] = {0}, same = 0, a;
        for (i = 1; i <= 64u; i++) {
            vis_style = (uint8_t)(i % VIS_N);
            boot_choose(i * 2654435761u);
            same += boot_vis == (uint8_t)(i % VIS_N);
            cnt[boot_vis]++;
            if (vis_style != boot_vis || vis_last != boot_vis) same += 100u;
        }
        for (i = 1; i <= 64u; i++) {
            vis_style = 0;
            boot_choose(i * 2654435761u + 77u);
            same += boot_vis == 0u;
            cnt[boot_vis]++;
        }
        for (seen = 0, a = 0; a < VIS_N; a++) seen += cnt[a] != 0u;
        snprintf(nm, sizeof nm, "boot pick: seeds 1..64 give %u/%u/%u/%u/%u (FLOW/EYES/DESERT/THIRD EYE/BEACH), never the last shown", cnt[0], cnt[1], cnt[2], cnt[3], cnt[4]);
        check(!same && cnt[1] && cnt[2] && cnt[3] && cnt[4], nm);      /* (never FLOW here: FLOW was the last shown) */
        for (i = 0; i < 64u; i++) {                 /* the last shown moves round: every one of the four comes up */
            vis_style = (uint8_t)(i % VIS_N);
            boot_choose(i * 40503u + 5u);
            cnt[boot_vis]++;
        }
        for (seen = 0, a = 0; a < VIS_N; a++) seen += cnt[a] > 8u;
        check(seen == VIS_N, "boot pick: with the last shown moving, all five come up, each many times");
        {   uint32_t b1, b2, d = 0;
            for (i = 1; i <= 32u; i++) {            /* the same last shown, two seeds: they differ */
                vis_style = 1; boot_choose(i); b1 = boot_vis;
                vis_style = 1; boot_choose(i + 1000u); b2 = boot_vis;
                d += b1 != b2;
            }
            snprintf(nm, sizeof nm, "boot pick: different seeds, different pictures (%u of 32 pairs differ)", d);
            check(d >= 12u, nm);
        }
    }
    {   /* the screensaver opened from HOME: at random, never the same twice running */
        uint32_t cnt[VIS_N] = {0}, rep = 0, a;
        uint8_t last;
        vis_last = 0;
        for (i = 0; i < 200u; i++) {
            last = vis_last;
            vis_on = 0;
            fm1_ms = 1000u + i * 977u;               /* (a person's presses: no two moments alike) */
            vis_open();
            rep += vis_style == last;
            cnt[vis_style % VIS_N]++;
            rep += !(vis_on && vis_name_t == VIS_CAPTION && vis_last == vis_style);
        }
        for (i = 0; i < 200u; i++) {
            last = vis_last;
            vis_on = 0;
            fm1_ms = 5000u + i * 1237u;
            vis_open();
            rep += vis_style == last;
            cnt[vis_style % VIS_N]++;
        }
        snprintf(nm, sizeof nm, "screensaver from HOME: 200 openings %u/%u/%u/%u/%u, never the same twice running", cnt[0], cnt[1], cnt[2], cnt[3], cnt[4]);
        check(!rep && cnt[0] > 20u && cnt[1] > 20u && cnt[2] > 20u && cnt[3] > 20u && cnt[4] > 20u, nm);
        for (n = 0, i = 1; i <= 16u; i++) {          /* (the same moment, other dice: other pictures) */
            vis_last = 0; vis_on = 0; fm1_ms = 123456u; vis_rng = i * 7919u;
            vis_open();
            n |= 1u << vis_style;
        }
        check((n & (n - 1u)) != 0u, "screensaver from HOME: the same moment with other dice gives other pictures");
        vis_style = 1; vis_last = 1; vis_on = 1;     /* SELECT's walk: opened on, it goes where it was sent */
        vis_style = 2; vis_open();
        check(vis_style == 2u && vis_last == 2u, "screensaver, SEL: goes where it was sent (not re-rolled)");
    }
    {   /* the caption: where the others are, grey, fading, gone; not a bar, not white */
        static uint16_t a[240 * 240], b[240 * 240];
        uint32_t diff = 0, white = 0, low = 0, x;
        vis_on = 0; vis_last = 3; fm1_ms = 400000u; vis_open();
        vr_paint(vis_style, 400000u); memcpy(b, screen, sizeof b);                 /* (no caption) */
        vis_name_t = VIS_CAPTION; vis_paint(); memcpy(a, screen, sizeof a);        /* (the caption) */
        for (x = 0; x < 240u * 240u; x++) {
            if (a[x] != b[x]) {
                diff++;
                white += swap16(a[x]) == 0xFFFFu;
                low += x >= 240u * 20u;
            }
        }
        snprintf(nm, sizeof nm, "caption: %u pixels over the picture's top 20 rows only, none white", diff);
        check(diff > 150u && diff < 6000u && !low && !white, nm);
        ppm("vis-caption");
        vis_name_t = 10; vis_paint();
        for (diff = 0, x = 0; x < 240u * 20u; x++) diff += screen[x] != a[x];
        vis_name_t = 0; vis_paint();
        for (x = 0, low = 0; x < 240u * 240u; x++) low += screen[x] != b[x];
        check(diff > 50u && !low, "caption: fades (10 draws left: dimmer), and goes (0: the picture, nothing else)");
        ui.force = 1;
    }
    fm1_ms = ms0; vis_last = (uint8_t)vl; vis_rng = vrng; vis_style = vst; vis_on = von; vis_name_t = vnt; boot_vis = bv; vs.kt = 0;
}
/* (PHEBUS, Ben 23 Nov 2026, 20:56: THIRD EYE "no white outline ... slightly bigger ... remove the 2 black outlines",
 * the dunes of both deserts without their outlines and thin black lines, DESERT's sun "a tiny bit bigger"; BEACH, the
 * four characters dancing by a trippy ocean) the pictures held still, painted at the start of a tear slot (no tear) */
static void art_paint(uint32_t st, uint32_t tq, uint32_t lvl, uint32_t rip)   /* lvl: every voice's (0: a mix, 1: silent) */
{
    uint32_t i;
    vis_style = (uint8_t)st;
    fm1_ms = 300u * 32768u;
    for (i = 0; i < 4u; i++) { vs.e[i] = (uint16_t)(lvl == 1u ? 0u : lvl ? lvl : 60u + i * 40u); vs.flash[i] = 0; }
    vs.kick = 0; vs.tq = tq << 8; vs.rip = rip; vs.kt = 0;
    memset(ey, 0, sizeof ey);
    vis_name_t = 0;
    vis_paint();
}
static uint32_t art_cell(uint32_t i, const uint16_t *a, const uint16_t *b, uint16_t col)   /* character i's cell: pixels that
                                                                                            * differ (b), or of colour col */
{
    uint32_t x, y, n = 0;
    for (y = BH_FEET - 58u; y <= BH_FEET + 1u; y++)
        for (x = 2u + i * 60u; x < 2u + i * 60u + 58u && x < 240u; x++)
            n += b ? a[y * 240u + x] != b[y * 240u + x] : a[y * 240u + x] == swap16(col);
    return n;
}
static uint32_t art_pose(uint32_t i)               /* the pose character i was drawn in (its picture read to the end) */
{
    uint32_t p, r;
    for (p = 0; p < 3u; p++) {
        const uint8_t *q = OP_BCH[i][p];
        for (r = 0; r < OP_BCH_H; q++)
            r += *q == 255u;
        if (q == bh_p[i])
            return p;
    }
    return 9u;
}
static void art_checks(void)
{
    uint32_t ms0 = fm1_ms, st, k, x, y, i, bad, white;
    uint8_t vst = vis_style, dsc = disco, pl = song.playing;
    uint32_t cb = clk_beat;
    char nm[160];
    {   /* THIRD EYE: no white line round the eye (near-white only in the glint, inside the iris), and it is bigger */
        int32_t w = 0, h = 0;
        for (white = 0, k = 0; k < 24u; k++) {
            int32_t x0 = 240, x1 = -1, y0 = 240, y1 = -1;
            art_paint(3u, 300u + k * 211u, k & 1u ? 256u : 0u, 17u);
            for (y = 0; y < 240u; y++)
                for (x = 0; x < 240u; x++) {
                    uint16_t c = swap16(screen[y * 240u + x]);
                    int32_t dx = (int32_t)x - ey[0].x - ey[0].gx, dy = (int32_t)y - ey[0].y - ey[0].gy;
                    if ((c >> 11) >= 28u && ((c >> 5) & 63u) >= 56u && (c & 31u) >= 28u && dx * dx + dy * dy > ey[0].ri2 + 40)
                        white++;
                    (void)x0; (void)x1; (void)y0; (void)y1;
                }
        }
        w = TE_HW * 2 - 1, h = TE_HH * 2;               /* (the eye's body is black again, so it is measured from its shape) */
        snprintf(nm, sizeof nm, "THIRD EYE: no white outline (%u near-white pixels outside the iris in 24 frames)", white);
        check(!white, nm);
        snprintf(nm, sizeof nm, "THIRD EYE: the eye %d x %d px, was 129 x 61 (+%d %%, +%d %%: 10 to 15 asked)", w, h, (w * 100 + 64) / 129 - 100, (h * 100 + 30) / 61 - 100);
        check(w * 100 >= 129 * 110 && w * 100 <= 129 * 116 && h * 100 >= 61 * 110 && h * 100 <= 61 * 116, nm);
        check(ey[0].sc == C_BLACK, "THIRD EYE: the eye's body is black (Ben: \"Make it black again\")");
    }
    for (st = 2; st <= 3u; st++) {                  /* the dunes, both pictures: flat faces only (no crest line, no contour) */
        for (bad = 0, white = 0, k = 0; k < 16u; k++) {
            art_paint(st, 500u + k * 377u, k & 1u ? 256u : 0u, 17u);
            for (y = DS_HOR; y < 240u; y++)
                for (x = 3; x < 237u; x++) {
                    uint16_t c = swap16(screen[y * 240u + x]);
                    uint32_t s, j, in = 1, ok = 0;
                    for (j = x - 3u; j <= x + 3u && in; j++) {   /* (under a crest here and 3 px each side: shimmer aside) */
                        for (s = 0; s < 4u && y < (uint32_t)(ds_r[s][j >> 1] & ~1); s++)
                            ;
                        in = s < 4u;
                    }
                    if (!in)
                        continue;
                    if (st == 3u) {                 /* (not the eye over them) */
                        int32_t dx = (int32_t)x - ey[0].x, dy = vis_abs((int32_t)y - ey[0].y) + ey[0].c;
                        if (dx * dx + dy * dy <= ey[0].r2 + 8 * TE_HW)
                            continue;
                    }
                    for (s = 0; s < 4u; s++)
                        ok |= c == vp[s][1] || c == ds_sh[s];
                    bad += !ok;
                    white += (c >> 11) >= 28u && ((c >> 5) & 63u) >= 56u && (c & 31u) >= 28u;
                }
        }
        snprintf(nm, sizeof nm, "%s: the dunes flat colour only (%u pixels of a crest line or contour, %u near-white, 16 frames)", VIS_NAME[st], bad, white);
        check(!bad && !white, nm);
    }
    {   /* DESERT's sun: its outermost ring's reach over the horizon, as the rings flow, the largest */
        int32_t r = 0;
        for (k = 0; k < 96u; k += 3u) {
            art_paint(2u, 2000u, 0u, k);
            for (y = DS_HOR - 90; y < DS_HOR; y++) {
                uint32_t n = 0;
                for (x = 116; x <= 124u; x += 2u)
                    n += screen[y * 240u + x] != 0u;
                if (n >= 3u) {
                    if (DS_HOR - (int32_t)y > r) r = DS_HOR - (int32_t)y;
                    break;
                }
            }
        }
        snprintf(nm, sizeof nm, "DESERT: the sun reaches %d px over the horizon, was 66 (+%d %%: 8 to 12 asked)", r, (r * 100 + 33) / 66 - 100);
        check(r * 100 >= 66 * 107 && r * 100 <= 66 * 113, nm);
    }
    {   /* BEACH: the four characters, each in its voice's colour in its place; each dances with its own voice only;
         * stopped they sway; QUIET: still */
        static uint16_t a[240 * 240], b[240 * 240];
        uint32_t n[4], own = 1, moves = 1, sway = 0;
        song.playing = 0; disco = DISCO_NORMAL;
        art_paint(4u, 2000u, 0u, 17u);
        for (i = 0; i < 4u; i++) {
            n[i] = art_cell(i, screen, 0, TE_COL[i]);
            own &= n[i] >= 200u && art_cell(i, screen, 0, TE_COL[(i + 1u) & 3u]) < 10u;
        }
        snprintf(nm, sizeof nm, "BEACH: the robot, alien, snake, crocodile in their voices' colours (%u/%u/%u/%u px)", n[0], n[1], n[2], n[3]);
        check(own, nm);
        song.playing = 1;
        for (i = 0; i < 4u; i++) {                  /* one voice sounding: the beat moves its character, no other */
            uint32_t j;
            clk_beat = 0; art_paint(4u, 2000u, 1u, 17u); vs.e[i] = 200u; vis_paint(); memcpy(a, screen, sizeof a);
            clk_beat = 1; vis_paint();
            for (j = 0; j < 4u; j++)
                moves &= j == i ? art_cell(j, screen, a, 0) > 100u : art_cell(j, screen, a, 0) == 0u;
        }
        check(moves, "BEACH: each character dances on the beat with its own voice (the crocodile the drums), the others stay");
        for (k = 0, i = 0; i < 4u; i++) {           /* all playing: never two side by side in the same pose, on any beat */
            uint32_t j;
            clk_beat = i; art_paint(4u, 2000u, 220u, 17u);
            for (j = 0; j < 3u; j++)
                k += art_pose(j) > 2u || (art_pose(j) == art_pose(j + 1u) && ((bh_f >> j) & 1u) == ((bh_f >> (j + 1u)) & 1u));
        }
        clk_beat = 0; art_paint(4u, 2000u, 220u, 17u);
        check(!k, "BEACH: all playing, each a beat on from its neighbour: no two side by side in the same pose");
        ppm("beach-all-playing");
        song.playing = 0;
        art_paint(4u, 2000u, 0u, 17u); memcpy(a, screen, sizeof a);
        art_paint(4u, 2043u, 0u, 17u);
        for (i = 0; i < 4u; i++)
            sway += art_cell(i, screen, a, 0) != 0u;
        snprintf(nm, sizeof nm, "BEACH: stopped, they sway gently (%u of 4 moved in a second)", sway);
        check(sway >= 3u, nm);
        disco = DISCO_QUIET; song.playing = 1;
        clk_beat = 0; art_paint(4u, 2000u, 220u, 17u); for (i = 0; i < 4u; i++) vs.flash[i] = 8u; vis_paint(); memcpy(a, screen, sizeof a);
        clk_beat = 3; art_paint(4u, 5000u, 220u, 30u); for (i = 0; i < 4u; i++) vs.flash[i] = 8u; vis_paint(); memcpy(b, screen, sizeof b);
        check(!memcmp(a, b, sizeof a), "BEACH: DISCO QUIET, playing loud: the picture stands still (no dance, no hop, no waves)");
        disco = DISCO_NORMAL; song.playing = 0;
        check(VIS_N == 5u && !strcmp(VIS_NAME[VIS_N - 1u], "BEACH"), "BEACH: the fifth screensaver (HOME's random pick, SEL's walk, SHOW, the boot)");
    }
    fm1_ms = ms0; vis_style = vst; disco = dsc; song.playing = pl; clk_beat = cb; vs.kt = 0;
    memset(vs.e, 0, sizeof vs.e); memset(vs.flash, 0, sizeof vs.flash);
}
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
        check(ok && seen[ENGI_ACID] == 7u && seen[ENGI_JUNO] == 7u && seen[ENGI_ELEM] == 7u && seen[ENGI_PRISM] == 7u,
              "PRESETS: every track lists every engine's sounds (wave 4: every Type on every track)");
    }
    panel = PANEL_DEFAULT;
    layers_init();
    settings.palette = 4;
    palette_set(4);
    host_tracks_init();
    for (i = 0; i < NPART; i++) { trk_def_sound(&trk[i], i); trk[i].engine = trk[i].eng_req; }   /* (as main.c felucca_init) */
    kit_set((uint32_t)(DRUM_DEFAULT_KIT));
    {   /* PHEBUS (21 Nov, Ben: "I don't like having fx on by default"): power-on is dry */
        uint32_t ok = 1, l;
        for (i = 0; i < NPART; i++)
            ok &= !trk[i].p[P_DIST] && !trk[i].p[P_CHOR] && !trk[i].p[P_DLY] && !trk[i].p[P_REV] && !trk[i].p[P_ITYPE] &&
                  !trk[i].p[P_SSWING];
        for (l = 0; l < DRUM_LANES; l++)
            ok &= !sl_send(l, 0) && !sl_send(l, 1);
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
      boot_vis = 0;
      for (k = 0; k < 6u; k++) { char nm[16]; snprintf(nm, sizeof nm, "boot-%u", (unsigned)k); flow_seed = SEED[k]; boot_frame(60u + k * 9u); ppm(nm); }
      for (k = 0; k < VIS_N; k++) {                /* (23 Nov: a screensaver's picture behind the wordmark, one of the four at random) */
          char nm[40]; uint32_t w, t;
          boot_vis = (uint8_t)k; flow_seed = SEED[2];
          snprintf(nm, sizeof nm, "boot-pic-%s", VIS_NAME[k]);
          for (w = 0; nm[w]; w++) if (nm[w] == ' ') nm[w] = '-';
          for (t = 0; t < 3u; t++) { boot_frame(40u + t * 50u); if (t == 1u) ppm(nm); }
      }
      boot_vis = 0; } 
    ui.menu = 2; ui.force = 1; frame(); ppm("page-about"); ui.menu = 0;
    go_home(); ui.force = 1; frame(); ppm("page-tracks");
    {   /* (PHEBUS wave 2) HOME's header title: the pattern playing (A..D), else PHEBUS; never empty */
        static uint16_t h0[240 * 20];
        int8_t ls = live_sec;
        uint32_t k, lit = 0, diff = 0;
        live_sec = -1; ui.force = 1; frame();
        memcpy(h0, screen, sizeof h0);
        for (k = 52u * 1u; k < 240u * 20u; k++) lit += (k % 240u) >= 60u && (k % 240u) < 150u && h0[k] != 0;
        live_sec = 2; ui.force = 1; frame();
        for (k = 0; k < 240u * 20u; k++) diff += h0[k] != screen[k];
        check(lit > 20u && diff > 20u, "HOME: the header's title says PHEBUS, then PATTERN C while C plays");
        live_sec = ls; ui.force = 1; frame();
    }
    open_family(FAM_EDIT); ui.force = 1; frame(); ppm("page-osc-default");
    /* (the rest of this test was written on the old power-on sounds, ACID TB ACID and ELEMENTS PICKED BAR: those again) */
    set_engine_of(&trk[0], ENGI_ACID); trk[0].engine = trk[0].eng_req;
    set_engine_of(&trk[1], ENGI_ELEM); apply_preset_to(&trk[1], 3); trk[1].engine = trk[1].eng_req;
    go_home(); ui.force = 1; frame();
    {   /* PHEBUS (21 Nov, Ben: "Dot + save on stop"): the header's unsaved dot (project.c unsaved_tick sets proj_unsaved),
         * between the beats and the battery; REC's red disc takes its place while recording; gone once saved */
        uint16_t was = swap16(screen[10 * 240 + 208]);
        proj_unsaved = 1; frame();
        check(was != C_WHITE && swap16(screen[10 * 240 + 208]) == C_WHITE,
              "unsaved work: a white dot in the header, clear of the tempo and the battery");
        ppm("home-unsaved");
        open_family(FAM_ENV); ui.force = 1; frame();
        check(swap16(screen[10 * 240 + 208]) == C_WHITE, "unsaved work: the dot on a sound page's header too");
        go_home(); ui.force = 1; frame();
        proj_unsaved = 0; frame();
        check(swap16(screen[10 * 240 + 208]) == was, "saved: the dot goes");
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
        check(swap16(screen[9 * 240 + 178]) == C_WHITE, "header: the downbeat white while playing");
        song.rec = 1; ui.force = 1;
        op_header("HOME", TE_G3, "loop", &c);
        check(swap16(screen[9 * 240 + 178]) == TE_RED, "header: the downbeat red while recording");
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
        static dpat_t keep_dp;                          /* (PHEBUS wave 2: the drum pattern is not in trk[]) */
        const param_desc_t *d;
        int16_t lv, *vp, tf = trk[2].p[P_TFLT], kit = TDRUM->p[P_E0];
        uint32_t act = stl.act, k, ok;
        memcpy(keep, trk, sizeof keep); keep_dp = dpat;
        set_engine_of(&trk[0], ENGI_ACID); song.sel = 0; frame();
        /* every voice the same: CUTOFF the voice filter's, DECAY always the amp ENV's (Ben, 23 Nov 2026: "it should be the
         * amp envelope decay"), even while the filter's envelope is OWN, as on ACID INIT */
        check(home_knob(0, &d) == &trk[0].p[P_FCUT] && home_knob(1, &d) == &trk[0].p[P_DEC],
              "HOME knobs, ACID (filter envelope OWN): CUTOFF the voice filter's, DECAY the amp ENV's");
        set_engine_of(&trk[0], ENGI_JUNO); frame();
        check(home_knob(0, &d) == &trk[0].p[P_FCUT] && str_eq(d->label, "CUTOFF") && home_knob(1, &d) == &trk[0].p[P_DEC],
              "HOME knobs, JUNO: CUTOFF the voice filter's, DECAY the amp ENV's (LINK)");
        song.sel = 2; frame();
        check(trk[2].eng_req == ENGI_PRISM && home_knob(0, &d) == &trk[2].p[P_FCUT] && home_knob(1, &d) == &trk[2].p[P_DEC],
              "HOME knobs, PLAITS: CUTOFF the voice filter's, DECAY the ENV's DEC");
        set_engine_of(&trk[1], ENGI_ELEM); song.sel = 1; frame();
        check(home_knob(0, &d) == &trk[1].p[P_FCUT] && home_knob(1, &d) == &trk[1].p[P_DEC], "HOME knobs, ELEMENTS: CUTOFF, DEC");
        song.sel = TRK_DRUM; kit_set((uint32_t)(0)); drum_lane = 2; frame();
        check(home_knob(0, &d) == &TDRUM->p[P_TFLT] && (vp = home_knob(1, &d)) && str_eq(d->label, "DEC") &&
              *vp == HEX_KITS[0].s[1].dec,
              "HOME knobs, drums (HEX kit): CUTOFF the drum track's FILTER, DECAY the selected sound's DEC");
        kit_set((uint32_t)(DRUM_DEFAULT_KIT)); frame();
        check((vp = home_knob(1, &d)) && str_eq(d->label, "DECAY") && *vp == DS_KITS[DRUM_DEFAULT_KIT - DRUM_SAMPLED].s[1].decay,
              "HOME knobs, drums (606, PHEBUS wave 3: every kit editable): DECAY the selected sound's DECAY");
        {   /* turned: the project's own slot takes it (wave 12), the built-in kit is untouched */
            int32_t d0 = DS_KITS[DRUM_DEFAULT_KIT - DRUM_SAMPLED].s[1].decay;
            go_home(); frames(16);
            encs[panel.enc[EN_K2]] = 3; frame();
            check(dsp_get(DM_SYN, pkit.s[2].snd, &pkit.s[2].lfo, 2, DSP_S0 + DS_DECAY) == d0 + 3 &&
                  DS_KITS[DRUM_DEFAULT_KIT - DRUM_SAMPLED].s[1].decay == d0 && kit_changed(),
                  "HOME KNOB 2 on the drums (606): the snare's DECAY +3 in the project's slot, the 606 itself untouched");
            kit_set(DRUM_DEFAULT_KIT);
            ui.msg_t = 0;
        }
        go_home(); ui.force = 1; frame(); ppm("home-knobs-drum");
        song.sel = 2; go_home(); frames(16);
        trk[2].p[P_TFLT] = 0; trk[2].p[P_FCUT] = 127;
        encs[panel.enc[EN_K1]] = -5; frame();
        check(trk[2].p[P_FCUT] < 127 && !trk[2].p[P_TFLT], "HOME KNOB 1 on PLAITS: the voice filter's CUTOFF down (wave 4)");
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
        press(B_GLO); frames(10); tap(B_SCL); frames(2);
        lv = trk[0].p[P_PAN]; encs[panel.enc[EN_K1]] = 5; frame();
        check(mix_pan && trk[0].p[P_PAN] == lv + 5, "pan: GLO held + SEL (wave 13: the mixer's second page): the knobs are the pans (KNOB 1: voice 1's)");
        ui.force = 1; frame(); ppm("layer-mix-pan");
        release(B_GLO); frames(2);
        check(!mix_pan, "pan: GLO let go: the mixer's knobs are the levels again");
        for (ok = 1, k = 0; k < NPAGES; k++)
            ok &= !(fam_stage(PAGES[k].fam) >= 0 && (PAGES[k].id[0] == P_PAN || PAGES[k].id[1] == P_PAN ||
                                                       PAGES[k].id[2] == P_PAN || PAGES[k].id[3] == P_PAN));
        check(ok, "pan: the mixer's (wave 4: no voice page holds it: one layout, VOICE 2 is gone)");
        memcpy(trk, keep, sizeof keep); dpat = keep_dp; trk[2].p[P_TFLT] = tf; kit_set((uint32_t)(kit)); stl.act = (uint8_t)act; sync_reload = 1;
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
        static dpat_t keep_dp;                          /* (PHEBUS wave 2: the drum pattern is not in trk[]) */
        uint32_t x, y, same, n0, voice = 0, pur = 0, blocks = 1, k;
        memcpy(keep, trk, sizeof keep); keep_dp = dpat;
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
        memcpy(trk, keep, sizeof keep); dpat = keep_dp; sync_reload = 1;
        go_home(); frames(4);
    }
    go_home(); open_family(FAM_EDIT); ui.force = 1; frame(); ppm("page-edit");   /* (EDIT: the chain's start, OSC) */
    open_family(FAM_FX); ui.force = 1; frame(); ppm("page-fx");
    open_family(FAM_SEQ); ui.force = 1; frame(); ppm("page-pattern");
    open_family(FAM_GLO); ui.force = 1; frame(); ppm("page-global");
    check(px_count(100, Y_GRAPH + 2 + 13, 128, 3, TE_G2) == 0u && px_count(150, Y_GRAPH + 2 + 7, 78, 13, C_WHITE) > 10u,
          "V3: GLO rows: no bar, the value printed right-aligned in its row");
    {   /* review 16b (Ben, 21 Nov: "Fix all"): a page every voice shares is neutral: the tag "ALL" in grey, no voice colour
         * in the header, no character, the knobs grey */
        uint32_t k, voice = 0;
        for (k = 0; k < 4u; k++)
            voice += px_count(0, 0, 240, OP_HEAD_H, TE_COL[k]) + px_count(0, OP_BODY_Y, 240, OP_KNOB_Y - OP_BODY_Y, TE_COL[k]) +
                     px_count(0, OP_KNOB_Y, 240, OP_KNOB_H, TE_COL[k]);
        check(!strcmp(cur_page()->title, "GLOBAL") && px_count(2, 1, 46, 18, TE_G4) > 60 && voice == 0,
              "GLO > GLOBAL: tag ALL in grey, no voice colour above the strip (no character, neutral knobs)");
    }
    {   /* every global parameter a page shows has its description (a label): none lost with the cuts */
        uint32_t pg, k, ok = 1;
        for (pg = 0; pg < NPAGES; pg++)
            for (k = 0; k < 4u; k++)
                if (PAGES[pg].scope == SC_GLOBAL && PAGES[pg].id[k] < G_COUNT && !GP[PAGES[pg].id[k]].label) {
                    printf("ui:   page %s knob %u: global %u has no description\n", PAGES[pg].title, (unsigned)k + 1, (unsigned)PAGES[pg].id[k]);
                    ok = 0;
                }
        check(ok, "every global parameter on a page has a description (GLOBAL CLICK, TOOLS CLEAR ...)");
    }
    open_family(FAM_GLO); ui.force = 1; frame(); ppm("page-master");
    open_family(FAM_SCL); ui.force = 1; frame(); ppm("page-scale");
    {   /* (2.4) MIDI OUT (KEYS / SEQ) and IN; (review U6: "One home each") on GLO > MIDI 2 only, SYSTEM has SYNC and USB */
        uint32_t i, m0 = (uint32_t)song.g[G_MIDI];
        for (i = 0; i < NPAGES; i++) if (!strcmp(PAGES[i].title, "SYSTEM")) break;
        check(i < NPAGES && PAGES[i].id[0] == G_SYNC && PAGES[i].id[1] == G_INFO && PAGES[i].id[2] == 0xFF, "U6: GLO > SYSTEM: SYNC, USB; no OUT, no IN");
        open_family(FAM_GLO); ui.page = (uint8_t)i; ui.fam_last[FAM_GLO] = (uint8_t)i; page_entered(); ui.force = 1; frame(); ppm("page-system");
        for (i = 0; i < NPAGES; i++) if (!strcmp(PAGES[i].title, "MIDI 2")) break;
        open_family(FAM_GLO); ui.page = (uint8_t)i; ui.fam_last[FAM_GLO] = (uint8_t)i; page_entered(); ui.force = 1; frame();
        song.g[G_MIDI] = 0; ui.force = 1; frame();
        encs[panel.enc[EN_K3]] = 1; frame();
        check(i < NPAGES && song.g[G_MIDI] == 1, "GLO > MIDI 2: KNOB 3 sets OUT = SEQ");
        {
            char val[12]; const char *unit = "";
            int16_t *vp; const param_desc_t *d = page_desc(cur_page(), 2, &vp);
            param_format(d, *vp, val, &unit);
            check(d && !strcmp(d->label, "OUT") && !strcmp(val, "SEQ"), "GLO > MIDI 2: the third column reads OUT SEQ (MIDI OUT)");
        }
        {   /* (2.4) KNOB 1 (MIDI 2): IN = CLOCK (notes ignored), kept in the settings word; back to NOTES */
            int16_t r0 = song.g[G_ROUTE];
            int16_t *vp; const param_desc_t *d = page_desc(cur_page(), 0, &vp);
            char val[12]; const char *unit = "";
            encs[panel.enc[EN_K1]] = 1; frame();
            param_format(d, *vp, val, &unit);
            check(song.g[G_ROUTE] == 1 && !strcmp(d->label, "IN") && !strcmp(val, "CLOCK"), "GLO > MIDI 2: KNOB 1 sets IN = CLOCK");
            lights_min = 1;
            check((lights_word() >> 15 & 1u) == 1u, "IN = CLOCK kept in the settings word (bit 15)");
            lights_from_word(lights_word() & ~(1u << 15));
            check(lights_min == 0u, "IN read back from the settings word");
            encs[panel.enc[EN_K1]] = 12; frame();             /* (2.6) on to CH 12 */
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
            encs[panel.enc[EN_K1]] = -13; frame();
            song.g[G_ROUTE] = r0;
        }
        song.g[G_MIDI] = (int16_t)m0; go_home(); frame();
    }

    /* ---- taps open pages, holds are layers */
    go_home(); ui.force = 1; frame();
    { uint8_t was = song.sel; song.sel = 0; check(keys_guide() == 0u, "synth track, no layer: no landmarks (a piano)"); song.sel = was; }
    /* (Ben's SPEC.md, 20 Nov 2026: step keys on every track. SEQ tapped on HOME is the track's GRID, as on the drum
     *  track; SEQ tapped again: the SEQ pages. Before: straight to the SEQ pages) */
    /* (wave 9, Ben 23 Nov 2026, 08:42) a labelled button steps through its group's categories, SEL (PAGE) through the pages */
    tap(B_SEQ); check(on_drum_page() && syn_tab == 0u && syn_hcur() == 0u && cur_cat() == CT_TRIG, "SEQ tapped on HOME: the track's GRID, on TRIG 1/2");
    tap(B_SCL); check(on_drum_page() && syn_tab == 0u && syn_hcur() == 1u, "SEL (PAGE) on TRIG: TRIG 2/2");
    tap(B_SCL); check(on_drum_page() && syn_tab == 0u && syn_hcur() == 0u, "SEL again: round to TRIG 1/2");
    tap(B_SEQ); check(on_drum_page() && cur_cat() == CT_PATT, "SEQ again: its next category, PATTERN (the pattern's settings)");
    ui.force = 1; frame(); ppm("grid-pattern");
    tap(B_SCL); check(cur_cat() == CT_PATT && syn_hcur() == TH_PATT + 1u, "SEL on PATTERN (wave 10: two pages): PATTERN 2/2, SHIFT");
    tap(B_SEQ); check(on_drum_page() && cur_cat() == CT_PATTS, "SEQ again (wave 10): A-D, the patterns");
    tap(B_SEQ); check(on_drum_page() && cur_cat() == CT_TRIG && syn_hcur() == 0u, "SEQ again: round to TRIG");
    go_home(); frame();
    tap(B_FX); check(cur_cat() == CT_INS && !strcmp(PAGES[ui.page].title, "FX 1"),
                     "FX tapped: the voice's FX first, INS (FX 1) (Ben: \"Voice FX, then mix FX\")");
    tap(B_SCL); check(!strcmp(PAGES[ui.page].title, "FX 2"), "SEL: FX 2");
    tap(B_SCL); tap(B_SCL); check(!strcmp(PAGES[ui.page].title, "FX 1"), "SEL, SEL: FX 3, round to FX 1");
    tap(B_FX); check(cur_cat() == CT_SENDS, "FX again: SENDS");
    tap(B_FX); check(cur_cat() == CT_SLICER, "FX again: SLICER");
    tap(B_FX); check(cur_cat() == CT_MIX && !strcmp(PAGES[ui.page].title, "DELAY"), "FX again: MIX, the whole mix's DELAY");
    ui.force = 1; frame(); ppm("page-fx-delay");
    check(px_count(8, Y_GRAPH + 10, 4, 80, C_WHITE) > 200u && px_count(100, Y_GRAPH + 2 + 13, 128, 3, TE_G2) == 0u,
          "V3: MIX > DELAY: a picture (the dry hit, the taps), not the numbered bar list");
    check(px_count(0, 0, 50, 20, TE_COL[0]) == 0u && px_count(0, 0, 240, 240, TE_COL[0]) < 600u,
          "MIX pages drawn white and grey (the voice's colour only in the tab row's voice side and the strip)");
    tap(B_SCL); ui.force = 1; frame(); ppm("page-fx-revcho");
    check(!strcmp(PAGES[ui.page].title, "REV/CHO"), "MIX, SEL: REV/CHO (SIZE DAMP, the chorus's RATE DEPTH)");
    tap(B_SCL); ui.force = 1; frame(); ppm("page-fx-master");
    check(!strcmp(PAGES[ui.page].title, "MASTER") && PAGES[ui.page].id[0] == G_DUST && PAGES[ui.page].id[1] == G_DUCK,
          "MIX, SEL: MASTER (DUST, DUCK)");
    tap(B_FX); check(cur_cat() == CT_STUT && stl_shown() && song.sel == 0u, "FX again (wave 14): STUT, the STUTTER lane, voice 1 still selected");
    tap(B_SCL); check(cur_cat() == CT_STUT && stl_shown(), "STUT, SEL: one page, nothing moves");
    tap(B_FX); check(cur_cat() == CT_INS && !strcmp(PAGES[ui.page].title, "FX 1") && !stl_view, "FX again: round to INS, FX 1");
    go_home(); frame(); track_select(TRK_DRUM); frame(); tap(B_FX); check(on_drum_page() && cur_cat() == CT_DINS, "FX on the drum voice: INS first, as a synth voice (the drum effects under FX)");
    tap(B_FX); tap(B_FX); tap(B_FX);
    check(!on_drum_page() && cur_cat() == CT_MIX && PAGES[ui.page].fam == FAM_MFX && song.sel == TRK_DRUM,
          "FX on the drum voice, then MIX: the one white MIX screen (the PAGES' DELAY), as on a synth voice (review 26 Nov, U1)");
    {   /* (U1, Ben's picker "One white MIX screen") the same MIX pages whichever voice is selected: pixel for pixel */
        static uint16_t mixd[3][240 * OP_BAR_Y];
        uint32_t i, same = 1;
        for (i = 0; i < 3u; i++) {
            ui.hot_t = 0; ui.msg_t = 0; ui.force = 1; frame(); memcpy(mixd[i], screen, sizeof mixd[i]);
            same &= px_count(0, 0, 240, OP_BAR_Y, TE_DRUM) == 0u;
            tap(B_SCL); frames(2);
        }
        check(same && cur_cat() == CT_MIX && !strcmp(PAGES[ui.page].title, "DELAY"),
              "U1: MIX on the drum voice: SEL walks DELAY, REV/CHO, MASTER, round; no teal above the strip");
        track_select(0); frame(); nav_go(CT_MIX, 0); frames(2);
        for (i = 0; i < 3u; i++) {
            ui.hot_t = 0; ui.msg_t = 0; ui.force = 1; frame();
            same &= !memcmp(mixd[i], screen, sizeof mixd[i]);
            tap(B_SCL); frames(2);
        }
        check(same, "U1: the drum voice's three MIX pages are the synth voice's, pixel for pixel above the strip");
        track_select(TRK_DRUM); frame();
    }
    track_select(0);
    go_home(); frame();
    press(B_FX); frames(12); check(ui.layer == LY_FX, "FX held: PERFORM shows (wave 6)");
    check(keys_guide() == (1u << key_of_white(0) | 1u << key_of_white(4) | 1u << key_of_white(8) | 1u << key_of_white(12)),
          "FX held: keys 1, 5, 9, 13 lit dim (the rows of the grid)");
    ppm("layer-punch");
    fm1_in.notes = 1u << 4; frame(); check(perf.held == 1u << PF_S4 && perf.last == PF_S4, "FX + the 3rd white key: the 1/4 pad, held");
    ppm("layer-punch-on");
    fm1_in.notes = 0; frame(); check(perf.held == 0u, "key up: the mix comes back");
    fm1_in.notes = 1u << 20; frame(); check(perf.held == 1u << PF_STOP, "FX + the black key C#5: TAPE STOP (wave 13)");
    check(keys_lit() == 1u << 20, "its key lights");
    ppm("layer-punch-black");
    fm1_in.notes = 0; frame(); check(perf.held == 0u, "black key up: the mix comes back");
    encs[panel.enc[EN_K1]] = 1; frame(); check(perf.par[PF_STOP][0] == PFD[PF_STOP].k[0].def + 1, "FX + KNOB 1: the last pad's first knob (TAPE STOP's TIME)");
    ui.force = 1; frame(); ppm("layer-punch-trkflt");
    perf.par[PF_STOP][0] = PFD[PF_STOP].k[0].def;
    release(B_FX); check(cur_page()->scope == SC_TRK && ui.layer == LY_PLAY, "FX used then let go: no FX page");

    /* ---- a layer locked open (wave 8: one keep-open gesture): held 0.7 s untouched, let go; any other button (not PLAY,
     * REC, OCT) lets it go. Held + HOME (the chord of before) does nothing */
    go_home(); frame();
    press(B_FX); frames(3); tap(B_HOME); release(B_FX); frames(3);
    check(ly_lock == LY_PLAY && ui.layer == LY_PLAY, "FX held + HOME (the old chord): nothing kept open (wave 8)");
    go_home(); frame();
    press(B_FX); frames(50); release(B_FX); frames(3);
    check(ly_lock == LY_FX && ui.layer == LY_FX, "FX held 0.7 s untouched, let go: kept open (LOCK)");
    check(cur_page()->scope == SC_TRK, "kept open: no FX page, no HOME jump");
    fm1_in.notes = 1u << 4; frame(); check(perf.held == 1u << PF_S4, "locked FX + the 3rd white key: the 1/4 pad (no hands on FX)");
    fm1_in.notes = 0; frame(); check(perf.held == 0u, "locked FX, key up: the mix comes back");
    encs[panel.enc[EN_K2]] = 6; frame(); check(perf.par[PF_S4][1] > 0, "locked FX + KNOB 2: the 1/4 pad's DECAY");
    perf.par[PF_S4][1] = 0;
    ui.force = 1; frame(); ppm("layer-locked");
    { uint8_t was = song.playing; tap(B_PLAY); frames(2);
      check(ly_lock == LY_FX && song.playing != was, "locked: PLAY plays and keeps the lock"); tap(B_PLAY); frames(2); }
    tap(B_ENV); frames(2);
    check(ly_lock == LY_PLAY && ui.layer == LY_PLAY && cur_page()->scope == SC_TRK,
          "locked, ENV pressed: unlocked, and only that (no ENV page)");
    press(B_FX); frames(50); release(B_FX); frames(3);
    tap(B_HOME); frames(2);
    check(ly_lock == LY_PLAY && ui.layer == LY_PLAY && cur_page()->scope == SC_TRK, "locked, HOME tapped: unlocked");
    press(B_FX); frames(50); release(B_FX); frames(3);
    tap(B_FX); frames(2);
    check(ly_lock == LY_PLAY && ui.layer == LY_PLAY && cur_fam() != FAM_MFX, "locked, FX tapped: unlocked, no FX page");
    {   /* (wave 8) the same keep-open on EVERY layer: EDIT, ARP, SEQ, SEL, GLO, SAVE */
        static const uint8_t LB[5] = {B_EDIT, B_ARP, B_SEQ, B_GLO, B_SAVE}, LL[5] = {LY_EDIT, LY_ARP, LY_SEQ, LY_GLO, LY_SAVE};
        uint32_t q, ok = 1;
        for (q = 0; q < 5u; q++) {
            go_home(); frame();
            press(LB[q]); frames(50); release(LB[q]); frames(3);
            ok &= ly_lock == LL[q] && ui.layer == LL[q];
            tap(LB[q]); frames(2);
            ok &= ly_lock == LY_PLAY && ui.layer == LY_PLAY;
            press(LB[q]); frames(30); release(LB[q]); frames(3);       /* (0.5 s: a look, it goes) */
            ok &= ly_lock == LY_PLAY && ui.layer == LY_PLAY;
        }
        check(ok, "every layer (EDIT ARP SEQ GLO SAVE): held 0.7 s untouched stays open, the button again lets it go, 0.5 s does not");
        go_home(); frame();
        press(B_SCL); frames(50); check(ui.layer == LY_PLAY && ly_lock == LY_PLAY, "SEL held (wave 9, Ben: \"Leave it unused\"): no layer");
        release(B_SCL); frames(3); check(ly_lock == LY_PLAY && ui.layer == LY_PLAY, "SEL held and let go: nothing stays open");
    }
    go_home(); frame();
    press(B_SEQ); frames(50); release(B_SEQ); frames(3);
    press(B_FX); frames(12);
    check(ly_lock == LY_PLAY && ui.layer == LY_FX, "locked SEQ, FX held: unlocked, the FX layer");
    release(B_FX); frames(2); check(ui.layer == LY_PLAY, "and FX let go: back to playing");
    press(B_FX); frames(50); release(B_FX); frames(3);
    press(B_HOME); frames(50); release(B_HOME); frames(2);
    check(!ui.menu && ly_lock == LY_PLAY, "locked, HOME held (wave 10: held, nothing): unlocked, no menu");
    ui.menu = 0; ui.force = 1; frames(2);
    song.g[G_DUST] = 0;

    /* ---- SEQ layer (wave 10, Ben: "SEQ is rhythm"): held, its white keys RETRIG (drums) / ROLL, its keys set no step;
     * KNOB 1 RATE, 2 SHIFT, 3 SWING, 4 STEPS; the drum steps are made on the GRID (one hold of a step key: one undo) */
    song.sel = TRK_DRUM; go_home(); frame();
    press(B_SEQ); frames(10);
    {
        uint32_t ac = 0;
        key(0); key(7); key(14);
        for (i = 0; i < 16u; i++) ac |= dt_mask(i);
        check(!ac, "SEQ held + white keys: no step set (they RETRIG, wave 10)");
    }
    ppm("layer-steps");
    release(B_SEQ);
    {   /* (wave 10) hold SEQ on a synth voice: (review U6, Ben: "One home each") KNOB 3 the pattern's GATE; the swing is
         * MASTER's on this screen */
        int16_t g0, w0;
        song.sel = 0; go_home(); frame();
        g0 = trk[0].p[P_SGATE]; w0 = trk[0].p[P_SSWING];
        press(B_SEQ); frames(10);
        encs[panel.enc[EN_K3]] = 10; frames(2);
        check(trk[0].p[P_SGATE] > g0 && trk[0].p[P_SSWING] == w0, "U6: SEQ held + KNOB 3 (a synth voice): GATE up, no second swing");
        fm1_in.notes = 1u << key_of_white(0); frames(2); ui.force = 1; frame();   /* (review R2) the lowest white key held */
        check(px_count(2, LY_STILES_Y + 3 * LY_STILE_H + 2, 56, 10, C_WHITE) > 100u && px_count(2, LY_STILES_Y + 2, 56, 10, C_WHITE) < 20u,
              "R2: hold SEQ on a synth voice: the tiles read the keys in rows from the bottom (the lowest key bottom left)");
        fm1_in.notes = 0; frames(2);
        ui.force = 1; frame(); ppm("layer-steps-gate");
        release(B_SEQ); frames(2);
        trk[0].p[P_SGATE] = g0;
        song.sel = TRK_DRUM; go_home(); frame();
    } check(ui.layer == LY_PLAY && !on_drum_page(), "SEQ used then let go: no page change");
    studio_open(SC_DRUM); frames(16); drum_lane = 2;   /* (the knobs quiet ~250 ms after a layer: #39) */
    key(0); key(7); key(14);                          /* steps 1, 5, 9 on the GRID */
    check(dt_has(0, 2) && dt_has(4, 2) && dt_has(8, 2) &&
          !dt_has(1, 2), "GRID + white keys 1, 5, 9: snare steps");
    tap(B_SEQ); frames(2);                            /* (PHEBUS wave 3) SEQ on the drum screen: its TRIG page */
    check(on_drum_page() && drum_pg == 0u, "drum screen: SEQ opens TRIG (the trig's own values on the knobs)");
    fm1_in.notes = 1u << 7; frame();                  /* step 5 held + KNOB 1 / 2: VEL, RETRIG (PHEBUS wave 2: the trig's) */
    encs[panel.enc[EN_K1]] = 27; frame();
    for (i = 0; i < 13u; i++) { encs[panel.enc[EN_K2]] = 1; frame(); }   /* (OFF 1/1 1/2 .. : the 13th, 1/32) */
    fm1_in.notes = 0; frame();
    check(dt_vel(4, 2) == 127u && dtest_lvl(4, 2) == LV_HARD && dt_x(4, 2) >= 0 && dpat.x[dt_x(4, 2)].rrate == 13u &&
          dtest_rat(4, 2) == 1u && dt_vel(8, 2) == DT_VEL_DEF && dt_x(8, 2) < 0,
          "GRID: step 5 held + KNOB 1 / 2: velocity 127, retrig 1/32 (two hits in the step); step 9 untouched");
    key(0); check(!dt_has(0, 2), "step 1 again: off");
    tap(B_SCL); frames(2); check(drum_pg == 0u && dpg_half() == 1u, "drum screen: SEL (PAGE): TRIG 2/2");
    tap(B_SEQ); frames(2); check(drum_pg == 0u && dpg_half() == 2u && cur_cat() == CT_PATT, "drum screen: SEQ again: PATTERN");
    tap(B_SEQ); frames(2); check(drum_pg == 0u && dpg_half() == TH_AD && cur_cat() == CT_PATTS, "drum screen: SEQ again (wave 10): A-D");
    tap(B_SEQ); frames(2); check(drum_pg == 0u && dpg_half() == 0u, "drum screen: SEQ again: round to TRIG 1/2");
    drum_lane = 0; go_home(); frame();

    /* ---- (wave 10) SAVE: undo / redo; a sound's key + OCT-: erase; SEQ: length */
    press(B_SAVE); frames(10);
    edges_btn |= BT(B_OCTDN); fm1_in.buttons |= BT(B_OCTDN); frame(); fm1_in.buttons &= ~BT(B_OCTDN); frame();
    check(dt_has(0, 2) && dtest_lvl(4, 2) == LV_HARD, "SAVE + OCT-: undo (step 1 back)");
    edges_btn |= BT(B_OCTUP); fm1_in.buttons |= BT(B_OCTUP); frame(); fm1_in.buttons &= ~BT(B_OCTUP); frame();
    check(!dt_has(0, 2) && dtest_lvl(4, 2) == LV_HARD, "SAVE + OCT+: redo (back, as left)");
    ppm("layer-erase");
    release(B_SAVE); frames(2);
    fm1_in.notes |= 1u << key_of_lane(2); frames(2);  /* stopped: the snare's key (the drum grid: key 16) held + OCT-: every snare goes */
    edges_btn |= BT(B_OCTDN); fm1_in.buttons |= BT(B_OCTDN); frame(); fm1_in.buttons &= ~BT(B_OCTDN); frame();
    fm1_in.notes &= ~(1u << key_of_lane(2)); frames(2);
    check(!dt_has(4, 2) && !dt_has(8, 2), "the snare's key held + OCT- (stopped): every snare erased");
    press(B_SEQ); frames(10);                         /* (wave 9) SEQ + OCT+: the pattern doubled, a clone (DOUBLE on EDIT: gone) */
    edges_btn |= BT(B_OCTUP); fm1_in.buttons |= BT(B_OCTUP); frame(); fm1_in.buttons &= ~BT(B_OCTUP); frame();
    check(TDRUM->p[P_SLEN] == 32, "SEQ + OCT+: length x2");
    release(B_SEQ); frames(2);
    edges_btn |= BT(B_SAVE); fm1_in.buttons |= BT(B_SAVE); frame();      /* undo the length too */
    edges_btn |= BT(B_OCTDN); fm1_in.buttons |= BT(B_OCTDN); frame(); fm1_in.buttons &= ~BT(B_OCTDN); frame();
    release(B_SAVE);
    check(TDRUM->p[P_SLEN] == 16, "SAVE + OCT-: the length back to 16");

    /* ---- (wave 10) SEQ layer: a roll (drums RETRIG), the rate knob */
    press(B_SEQ); frames(10);
    encs[panel.enc[EN_K1]] = 1; frame();
    check(drt_live == 11u && song.g[G_ROLL] == 1, "SEQ + KNOB 1 (the drum track): the RETRIG rate a step up (1/20)");
    drt_live = 10;
    fm1_in.notes = 1u << 7; frames(3); check(roll[0].on, "SEQ + a key: it rolls");
    ppm("layer-roll");
    fm1_in.notes = 0; frame(); check(!roll[0].on, "key up: the roll ends");
    release(B_SEQ);

    /* ---- (wave 9, Ben 23 Nov 2026: key, scale and chord "Under ARP") ARP's KEY category; GLO: mute, solo */
    song.sel = 0; go_home(); frame();
    tap(B_ARP); check(cur_cat() == CT_ARP, "ARP: the arpeggiator first");
    tap(B_ARP); check(cur_cat() == CT_KEY && !strcmp(PAGES[ui.page].title, "KEY"), "ARP again: KEY (ROOT SCALE QNT CHORD)");
    trk[0].p[P_ROOT] = trk[1].p[P_ROOT] = trk[2].p[P_ROOT] = 0; trk[0].p[P_CHORD] = 0; frames(20);
    encs[panel.enc[EN_K1]] = 2; frame();
    check(trk[0].p[P_ROOT] == 2 && trk[1].p[P_ROOT] == 2 && trk[2].p[P_ROOT] == 2, "KEY, KNOB 1 ROOT: every part in D (the song's key)");
    encs[panel.enc[EN_K4]] = 2; frame();
    check(trk[0].p[P_CHORD] == 2, "KEY, KNOB 4: chords (7TH) on the track");
    ui.force = 1; frame(); ppm("page-key");
    tap(B_SCL); check(!strcmp(PAGES[ui.page].title, "KEY 2"), "KEY, SEL: KEY 2 (TRANS STRUM VLEAD)");
    tap(B_ARP); check(cur_cat() == CT_ARP, "ARP again: round to the arpeggiator");
    go_home(); frame();
    press(B_GLO); frames(10);
    key(PUNCH_BLACK[0]); check(trk[0].p[P_MUTE] == 1, "GLO + F#3 (wave 13: the voices on the black keys): voice 1 muted");
    press(B_OCTUP); key(PUNCH_BLACK[1]); release(B_OCTUP);
    check(song.solo == 2u && !trk[1].p[P_MUTE], "GLO + OCT+ held + G#3: voice 2 soloed (plus = only this)");
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
    {   int16_t b0 = song.g[G_BPM];
        for (i = 0; i < 4u; i++) { fm1_in.notes = 1u << 26; frame(); fm1_in.notes = 0; frames(36); }   /* ~0.6 s apart */
        check(song.g[G_BPM] == b0, "GLO + the last key: nothing (wave 8: no tap tempo; the tempo is GLO > GLOBAL's BPM only)"); }
    key(PUNCH_BLACK[0]); press(B_OCTUP); key(PUNCH_BLACK[1]); release(B_OCTUP);
    check(!trk[0].p[P_MUTE] && !song.solo, "again: unmuted, no solo");
    /* (wave 10, Ben: "Global should have midi learn. But not fill") key 9 and 10: no fill on the mixer (SEQ + PLAY) */
    fm1_in.notes = 1u << key_of_white(8); frame(); frame();
    check(fill_held == 0u && fill_now == 0u, "GLO + key 9 held (wave 10): no FILL (it is SEQ + PLAY)");
    ui.force = 1; frame(); ppm("layer-mix-fill");
    fm1_in.notes = 0; frame(); frame();
    key(key_of_white(9)); check(fill_arm == 0u, "GLO + key 10 (wave 10): no FILL NEXT BAR (SEQ + D#4)");
    check(drum_mute == (1u << 12 | 1u << 15), "GLO + keys 9 and 10 (the drum grid): RIDE and C-BELL muted, whichever voice is selected");
    tap(B_OCTDN); frames(2);
    check(!drum_mute && !drum_solo && !song.solo && !trk[0].p[P_MUTE], "GLO + OCT- (wave 13): every mute and solo off");
    release(B_GLO);
    fm1_in.notes = 1u << key_of_white(8); frame();     /* (no layer: a plain key, no fill) */
    check(fill_held == 0u, "key 9 without GLO: no fill");
    fm1_in.notes = 0; frames(2);
    press(B_GLO); frames(45); release(B_GLO); frames(2);           /* (2026-10-28) ~0.7 s, nothing touched */
    check(ui.layer == LY_GLO && ly_lock == LY_GLO, "GLO held 0.7 s, untouched, let go: the mixer stays");
    key(PUNCH_BLACK[0]); check(trk[0].p[P_MUTE] == 1 && ui.layer == LY_GLO, "... its keys still mute (voice 1)");
    key(PUNCH_BLACK[0]); check(!trk[0].p[P_MUTE], "... and unmute");
    tap(B_GLO); frames(2);
    check(ui.layer == LY_PLAY && ly_lock == LY_PLAY, "... GLO again: it goes (and no GLO pages open)");
    press(B_GLO); frames(45); encs[panel.enc[EN_K1]] = 1; frame(); release(B_GLO); frames(2);
    check(ly_lock == LY_PLAY && ui.layer == LY_PLAY, "GLO held with a knob turned: it goes on release, as before");
    press(B_GLO); frames(32); release(B_GLO); frames(2);
    check(ly_lock == LY_PLAY && ui.layer == LY_PLAY, "GLO held 0.5 s: a look, it goes on release");
    fm1_in.notes = 0; frame();
    trk[0].p[P_CHORD] = 0;

    song.sel = 0; go_home(); frames(16);              /* (KNOB 1..4 quiet ~250 ms after a layer: #39) */
    open_family(FAM_FX); frame();                     /* the FX page: KNOB 1 = DIST */
    encs[panel.enc[EN_K1]] = 2; frame();
    go_home(); frame();
    {   /* (wave 10) SEQ held: KNOB 1 the roll's RATE, 2 SHIFT, 3 SWING, 4 STEPS; the black keys A B C D BAR drawn in the strip;
         * PRESETS / ALGORITHM change nothing */
        int16_t s0 = trk[0].p[P_SSWING], l0 = trk[0].p[P_SLEN], r0 = song.g[G_ROLL];
        uint32_t sel0 = song.sel, eng0 = trk[0].eng_req, pr0 = trk[0].preset;
        steps_clear(&trk[0]); trk[0].step[0].n = 1; trk[0].step[0].note[0] = 60; trk[0].step[0].time = ST_NOTE;
        press(B_SEQ); frames(10);
        ui.force = 1; frame();
        check(bk_shown == 0x1Fu && px_count(2, BK_Y, 37 * 4 + 120, 12, TE_G1) > 200u, "SEQ held: the strip draws its black keys (A B C D BAR)");
        check(str_eq(sub_line(), "play: fill"), "SEQ held: the sub line says what PLAY does (play: fill)");
        encs[panel.enc[EN_K1]] = 1; frame();
        check(song.g[G_ROLL] == r0 + 1, "SEQ held + KNOB 1: the roll's RATE");
        encs[panel.enc[EN_K2]] = 1; frame();
        check(!step_on(&trk[0].step[0]) && step_on(&trk[0].step[1]), "SEQ held + KNOB 2: SHIFT, the pattern a step later");
        ui.force = 1; frame(); ppm("layer-steps-div8");
        {   int16_t g0 = trk[0].p[P_SGATE];
            encs[panel.enc[EN_K3]] = 20; frame();
            check(trk[0].p[P_SGATE] > g0 && trk[0].p[P_SSWING] == s0, "SEQ held + KNOB 3: GATE (U6: the swing is MASTER's only here)");
            ui.force = 1; frame(); ppm("layer-steps-swing");
            trk[0].p[P_SGATE] = g0; }
        encs[panel.enc[EN_K4]] = 16; frame();
        check(trk[0].p[P_SLEN] > l0, "SEQ held + KNOB 4: STEPS");
        ui.force = 1; frame(); ppm("layer-steps-len");
        encs[panel.enc[EN_PRESET]] = 1; frame(); encs[panel.enc[EN_ALGO]] = 1; frame();
        check(trk[0].eng_req == eng0 && trk[0].preset == pr0 && song.sel == sel0, "SEQ held + PRESETS / ALGORITHM: no sound or track change");
        release(B_SEQ); frames(2);
        trk[0].p[P_SSWING] = s0; trk[0].p[P_SLEN] = l0; song.g[G_ROLL] = r0;
        steps_clear(&trk[0]);
    }
    fm1_irq_off(); steps_clear(TDRUM); steps_clear(&trk[0]); fm1_irq_on();
    trk[0].p[P_DIST] = 0;
    song.sel = 0; go_home(); frame();

    /* ---- REC: press arms; held: the ring; to the end: cleared */
    go_home(); frame();
    song.playing = 0; song.rec = 0; rec_wait = 0;
    tap(B_REC); check(rec_wait == 1, "REC tapped (stopped): armed");
    ui.force = 1; frame(); ppm("live-rec-ready");
    tap(B_REC); check(rec_wait == 0, "REC again: cancelled");
    trk[0].step[3].n = 1, trk[0].step[3].note[0] = 60, trk[0].step[3].time = ST_NOTE;
    press(B_REC); frames(140);                        /* (wave 10, Ben: "I don't like the record button taking things away") */
    check(ui.hold_kind == 0u && rec_wait == 1 && trk[0].step[3].n == 1, "REC held 2 s (wave 10): armed, no ring, nothing cleared");
    release(B_REC); tap(B_REC); frames(2);
    check(!rec_wait, "REC again: cancelled");
    press(B_EDIT); frames(10);                        /* (wave 10) the voice cleared: EDIT + OCT-, then hold SAVE */
    edges_btn |= BT(B_OCTDN); fm1_in.buttons |= BT(B_OCTDN); frame(); fm1_in.buttons &= ~BT(B_OCTDN); frame();
    release(B_EDIT);
    press(B_SAVE); frames(30);
    check(ui.hold_kind == 3u, "EDIT + OCT-, SAVE held: the ring");
    ppm("hold-clear");
    frames(70);
    check(!trk[0].step[3].n && ui.hold_kind == 0, "SAVE held to the end: track 1 cleared");
    release(B_SAVE);
    press(B_SAVE); frames(10);
    edges_btn |= BT(B_OCTDN); fm1_in.buttons |= BT(B_OCTDN); frame(); fm1_in.buttons &= ~BT(B_OCTDN); frame();
    release(B_SAVE);
    check(trk[0].step[3].n == 1, "SAVE + OCT-: the cleared track back");
    press(B_EDIT); frames(10);
    edges_btn |= BT(B_OCTDN); fm1_in.buttons |= BT(B_OCTDN); frame(); fm1_in.buttons &= ~BT(B_OCTDN); frame();
    release(B_EDIT);
    press(B_SAVE); frames(60); release(B_SAVE);       /* let go before the end: nothing */
    check(trk[0].step[3].n == 1 && !rec_wait, "SAVE let go before the end: nothing cleared");
    tap(B_HOME); frames(2); check(!ui.ask, "(another button: the question goes)");

    /* ---- SAVE: tap = the SAVE pages (PRESETS / USER / TOOLS) everywhere; held = the sections layer (A..D: play / store) */
    go_home(); frame();
    song.playing = 0; live_sec = -1; live_req = -1;
    press(B_SAVE); frames(15); check(ui.layer == LY_SAVE, "SAVE held: its layer (store, undo, redo)");
    key(key_of_white(0)); frames(2);
    check(sec_stores == 0 && ui.ask == ASK_STORE && !strcmp(ui.msg, "HOLD SAVE: STORE A") && ui.hold_kind == 3u,
          "store over a used A: it asks (HOLD SAVE: STORE A), the ring at once (SAVE is held)");
    release(B_SAVE); frames(2);
    check(sec_stores == 0 && ui.ask == ASK_STORE, "SAVE let go before the ring is full: nothing, the question stays");
    press(B_SAVE); frames(90); release(B_SAVE);
    check(sec_stores == 1 && live_sec == 0 && !ui.ask && !song.rec && !rec_wait, "SAVE held again until the ring fills: the loop stored in A");
    press(B_SAVE); frames(15);
    key(key_of_white(2)); check(sec_stores == 2 && live_sec == 2, "an empty C: stored at once");
    release(B_SAVE); frames(2);
    press(B_SEQ); frames(12);                         /* (wave 10) playing a pattern is SEQ's: black keys 1 to 4 */
    key(PUNCH_BLACK[1]); check(sec_loads == 1 && live_sec == 1, "SEQ + G#3, stopped: B loaded as the loop");
    key(PUNCH_BLACK[3]); check(sec_loads == 1, "SEQ + C#4: an empty D: not played");
    song.playing = 1; clk_beat = 1; clk_pos = 0;
    key(PUNCH_BLACK[0]); check(live_req == 0, "SEQ + F#3, playing: A asked for the next bar");
    key(PUNCH_BLACK[1]); check(live_req == 1, "SEQ + G#3 in the same hold: B asked for instead (no chain)");
    ui.force = 1; frame(); ppm("layer-song");
    release(B_SEQ); frames(2);
    press(B_SAVE); frames(15);
    ui.force = 1; frame();
    check(px_count(0, 96, 240, 56, TE_G1) == 0u && px_count(0, 40, 240, 28, TE_G1) > 100u,   /* (wave 7: 28 px rows from y 40) */
          "SAVE layer: keys 5..16 do nothing: their rows left out (no empty outlines); STORE A..D drawn");
    {   /* (PHEBUS wave 1: song mode, SONG REC and the SONG screen are cut) keys 13, 14, 16 do nothing */
        uint32_t st0 = sec_stores, ld0 = sec_loads, pg0 = ui.page;
        key(key_of_white(12)); key(key_of_white(13)); key(key_of_white(15));
        check(sec_stores == st0 && sec_loads == ld0 && live_req == 1 && ui.page == pg0 && ui.layer == LY_SAVE,
              "SAVE held + keys 13, 14, 16: nothing (song mode, SONG REC and the song page are gone)");
        check(str_eq(LAYER_NAME[LY_SAVE], "patterns") && str_eq(sub_line(), ""), "V4: the layer reads \"patterns\" (not SAVE SAVE); the line under the header only the -/+ legend");
    }
    release(B_SAVE); frames(2);
    check(live_req == 1 && saves == 0, "SAVE let go: B still asked for, nothing saved");
    live_req = -1; song.playing = 0;
    go_home(); frames(2);
    tap(B_SAVE); frames(2);
    check(!ui.home && cur_page()->fam == FAM_SAVE && str_eq(cur_page()->title, "PROJECT"), "SAVE tapped on HOME: PROJECT (the bank: first)");
    tap(B_SAVE); frames(2);
    check(cur_page()->fam == FAM_SAVE && str_eq(cur_page()->title, "PRESETS"), "SAVE tapped again: PRESETS");
    song.sel = 0; studio_open(SC_DRUM); frames(2);
    tap(B_SAVE); frames(2);
    check(cur_page()->fam == FAM_SAVE && str_eq(cur_page()->title, "PROJECT"), "SAVE tapped on a synth GRID: PROJECT");
    song.sel = TRK_DRUM; studio_open(SC_DRUM); frames(2);
    tap(B_SAVE); frames(2);
    check(cur_page()->fam == FAM_SAVE && str_eq(cur_page()->title, "PROJECT"), "SAVE tapped on the drum GRID: PROJECT");
    {   /* no page is the old SONG screen */
        uint32_t i, none = 1;
        for (i = 0; i < NPAGES; i++) none &= !str_eq(PAGES[i].title, "SONG");
        check(none, "no SONG page left");
    }
    go_home(); frames(16);

    /* ---- the drum screen (PHEBUS wave 3, the Rytm's way: Ben, 21 Nov 2026, "Drums: the Analog Rytm MKII's easy drum
     * programming. Pick a sound, 16 trig keys, hold a trig and turn to lock, sound pages in the Rytm's order (SRC, FLTR, AMP,
     * LFO), mute, fill, retrig, chromatic"; pickers "Drum pages: B: four dials over the knobs", "Make every kit editable") */
    song.sel = TRK_DRUM; studio_open(SC_DRUM); ui.force = 1; frames(16);   /* (after SAVE: the knobs quiet, #39) */
    check(on_drum_page() && grid_keys_on(), "the drum screen: the keys are the selected sound's steps");
    memset(&pkit, 0, sizeof pkit);
    kit_set((uint32_t)(DRUM_DEFAULT_KIT));
    {   /* a scene to look at */
        static const uint8_t BEAT[16] = {0x11, 0x10, 0x10, 0x10, 0x14, 0x10, 0x10, 0x21, 0x11, 0x10, 0x01, 0x10, 0x1C, 0x10, 0x10, 0x20};
        uint32_t j;
        for (j = 0; j < 16u; j++) {
            dtest_clear(j);
            if (BEAT[j] & 1u) dtest_set(j, 0, LV_HARD, 0);
            if (BEAT[j] & 4u) dtest_set(j, 2, LV_NORM, 0);
            if (BEAT[j] & 8u) dtest_set(j, 3, LV_GHOST, 0);
            if (BEAT[j] & 0x10u) dtest_set(j, 4, j % 4u ? LV_SOFT : LV_NORM, j == 14u ? 2u : 0u);
            if (BEAT[j] & 0x20u) dtest_set(j, 5, LV_NORM, 0);
        }
        for (j = 0; j < 16u; j += 3u) { trk[0].step[j].n = 1; trk[0].step[j].note[0] = 36; trk[0].step[j].time = ST_NOTE; }
        for (j = 0; j < 16u; j += 4u) { trk[1].step[j].n = 3; trk[1].step[j].time = ST_NOTE; }
        dt_lk_set(12, 2, DSP_S0 + DS_DECAY, 30);
        drum_lane = 2; drum_cursor = 0; ui.force = 1; ui.msg_t = 0;
        go_home(); frames(2); tap(B_EDIT); frames(2);
        check(on_drum_page() && drum_pg == 1u && drum_keys == DK_PLAY, "drums: EDIT from HOME opens the drum screen at SRC");
        drums.hits = 1u | 1u << 4; ui.force = 1; frame(); ppm("live-grid");
        check(px_count(4, 24, 120, 32, TE_DRUM) > 150u, "drum screen: the selected sound's name large, in the drum voice's colour");
        check(px_count(155, 26, 84, 46, C_WHITE) > 30u, "drum screen: the 4 x 4 sound map, the selected sound framed white");
        song.sel = 1; go_home(); ui.force = 1;
        transport_req = 1; frames(30); song.rec = 2u; frame(); ui.force = 1; frame(); ppm("live-tracks");
        song.rec = 0; transport_req = 2; frames(2);
        song.sel = TRK_DRUM;
    }
    {   /* the pages: EDIT walks SRC FLTR AMP LFO; ENV: AMP (again: its next half); LFO: LFO; SELECT every half; SEQ: TRIG */
        uint32_t ok;
        go_home(); frames(2); tap(B_EDIT); frames(2);
        ok = drum_pg == 1u && dpg_half() == 0u && cur_cat() == CT_SRC;
        tap(B_SCL); frames(2); ok &= drum_pg == 1u && dpg_half() == 1u;        /* SEL: SRC 2/3, 3/3, round */
        tap(B_SCL); frames(2); ok &= drum_pg == 1u && dpg_half() == 2u;
        tap(B_SCL); frames(2); ok &= drum_pg == 1u && dpg_half() == 0u;
        tap(B_EDIT); frames(2); ok &= drum_pg == 2u && dpg_half() == 0u && cur_cat() == CT_FLTR;
        tap(B_EDIT); frames(2); ok &= cur_cat() == CT_DFILT && dpg_kit();   /* (Ben 21:40) the kit's FILTER, under EDIT */
        tap(B_EDIT); frames(2); ok &= drum_pg == 1u && dpg_half() == 0u;
        check(ok && on_drum_page(), "drum pages: EDIT steps SRC > FLTR > KIT FLT > SRC (its categories), SEL the pages of SRC (1/3 .. 3/3, round)");
        tap(B_FX); frames(2); ok = drum_pg == 5u && cur_cat() == CT_DINS && dpg_half() == 0u;
        ui.force = 1; frame(); ppm("drum-fx");
        tap(B_SCL); frames(2); ok &= cur_cat() == CT_DINS && dpg_half() == 1u;   /* INSERT 2 */
        tap(B_FX); frames(2); ok &= cur_cat() == CT_DSENDS && dpg_sends();
        tap(B_FX); frames(2); ok &= cur_cat() == CT_DSLICER;
        tap(B_FX); frames(2); ok &= cur_cat() == CT_MIX && !on_drum_page() && PAGES[ui.page].fam == FAM_MFX;   /* (U1) */
        ui.force = 1; frame(); ppm("drum-fx-mix");
        tap(B_FX); frames(2); ok &= cur_cat() == CT_STUT && stl_shown() && song.sel == TRK_DRUM;
        tap(B_FX); frames(2); ok &= cur_cat() == CT_DINS && !stl_view;
        check(ok, "drum pages: FX steps INS (SEL: INSERT 2) > SENDS > SLICER > MIX > STUT > round (a synth voice's order; no FILTER)");
        tap(B_ENV); frames(2); ok = drum_pg == 3u && dpg_half() == 0u;
        tap(B_SCL); frames(2); ok &= drum_pg == 3u && dpg_half() == 1u;
        tap(B_ENV); frames(2); ok &= drum_pg == 3u && dpg_half() == 0u;   /* (one category: again, its first page) */
        ui.force = 1; frame(); ppm("drum-amp");
        tap(B_LFO); frames(2); ok &= drum_pg == 4u && dpg_half() == 0u;
        ui.force = 1; frame(); ppm("drum-lfo");
        dpg_go(2, 0); ui.force = 1; frame(); ppm("drum-fltr");      /* (wave 8: CUTOFF RESO ENV TYPE, as the synth's FILT) */
        dpg_go(2, 1); ui.force = 1; frame(); ppm("drum-fltr2");
        kit_set((uint32_t)(0)); dpg_go(2, 0); frames(2); ui.force = 1; frame(); ppm("drum-fltr-hex");
        kit_set((uint32_t)(DRUM_DEFAULT_KIT)); frames(2); dpg_go(4, 0); frame();
        check(ok, "drum pages: ENV opens AMP (SEL its 2/2; ENV again its first page); LFO opens LFO");
        {   /* (review 26 Nov, U3, Ben's picker "Each page its own picture") the drum sound pages: SRC and AMP the level over
             * time, FLTR the filter's curve, LFO the wave, SENDS the send boxes; each its own, teal, no giant word */
            static uint16_t band[5][240 * 33];
            static const uint8_t PG[5] = {1, 2, 3, 4, DPG_FX};
            uint32_t i, j, diff = 1, other = 0, lsp, o;
            for (i = 0; i < 5u; i++) {
                dpg_go(PG[i], i == 4u ? DBH_SENDS : 0u); ui.msg_t = 0; ui.force = 1; frame();
                for (j = 0; j < 33u; j++) memcpy(band[i] + j * 240u, screen + (96u + j) * 240u, 240u * 2u);
                for (o = 0; o < NPART; o++) other += px_count(0, 76, 240, 55, TE_COL[o]) + px_count(0, 76, 240, 55, TE_MID[o]);
            }
            for (i = 0; i < 5u; i++)
                for (j = i + 1u; j < 5u; j++)
                    diff &= (i == 0u && j == 2u) || memcmp(band[i], band[j], sizeof band[i]) != 0;
            check(diff && !other, "U3: the drum sound pages each draw their own picture (SRC and AMP the level over time; FLTR, LFO, SENDS their own); no other voice's colour");
            dpg_go(DPG_FX, DBH_SENDS); ui.force = 1; frame();
            check(px_count(4, 96, 52, 33, TE_G2) + px_count(4, 96, 52, 33, TE_DIM[3]) > 40u &&
                  px_count(64, 96, 52, 33, TE_G2) + px_count(64, 96, 52, 33, TE_DIM[3]) > 40u &&
                  px_count(124, 96, 52, 33, TE_DIM[3]) + px_count(124, 96, 52, 33, TE_DRUM) > 40u,
                  "U3: SENDS: DELAY, REVERB and KIT FX as three boxes over their knobs (a synth voice's send boxes), no giant SENDS");
            sl_send_set(drum_lane, 1, 100); ui.force = 1; frame();
            check(px_count(4, 96, 52, 33, TE_DRUM) > 20u, "U3: SENDS: a delay send drawn as echoes, teal");
            ppm("u3-drum-sends");
            sl_send_set(drum_lane, 1, 0);
            pkit.s[drum_lane].lfo.dep = 40; pkit.s[drum_lane].lfo.wav = 2;
            dpg_go(4, 0); ui.force = 1; frame(); ppm("u3-drum-lfo-depth");
            lsp = px_count(0, 96, 240, 33, TE_DRUM);
            pkit.s[drum_lane].lfo.dep = 0; pkit.s[drum_lane].lfo.wav = 0;
            ui.force = 1; frame();
            check(lsp > 100u && px_count(0, 96, 240, 33, TE_DRUM) == 0u && px_count(0, 96, 240, 33, TE_DIM[3]) > 100u,
                  "U3: LFO: the wave in teal as tall as DEPTH; DEPTH 0: its shape, dim");
            {   /* SRC: the time across scaled to the sound: every slot of the 606 and a HEX kit fills a quarter of it at least */
                uint32_t k2, l2, bad = 0, ln = drum_lane;
                for (k2 = 0; k2 < 2u; k2++) {
                    kit_set(k2 ? 0u : (uint32_t)DRUM_DEFAULT_KIT); frames(2);
                    for (l2 = 0; l2 < 16u; l2++) {
                        int32_t x, last = -1;
                        uint32_t sp = snd_span_ms(&pkit.s[l2]);
                        drum_lane = (uint8_t)l2; dpg_go(1, 0); ui.force = 1; frame();
                        for (x = 0; x < 240; x++) if (px_count((uint32_t)x, 96, 1, 33, TE_MID[3]) + px_count((uint32_t)x, 96, 1, 33, TE_G2) > 1u) last = x;
                        if (sp > 125u && last < 4 + 232 / 4) { bad++; printf("ui:   %s: %u ms, drawn to x %d\n", LANE_NAME[l2], (unsigned)sp, (int)last); }
                    }
                }
                drum_lane = (uint8_t)ln; kit_set((uint32_t)(DRUM_DEFAULT_KIT)); frames(2);
                check(!bad && snd_span_ms(&pkit.s[0]) >= 125u, "U3: SRC, AMP: the level's time axis scaled to the sound (125 MS to 2 S), each sound a quarter of it at least");
            }
            dpg_go(4, 0); frame();
        }
        dpg_go(1, 0); frame();
        { uint32_t ln = drum_lane, bp = (uint32_t)song.g[G_BPM];
          encs[panel.enc[EN_SELECT]] = 1; frame(); ok = drum_pg == 1u && dpg_half() == 0u && drum_lane == lane_step(ln, 1, 1);
          encs[panel.enc[EN_SELECT]] = -1; frame(); ok &= drum_lane == ln && (uint32_t)song.g[G_BPM] == bp;
          check(ok, "drum screen: SELECT steps the selected sound (Ben: \"SELECT knob\"), no page, no tempo"); }
        tap(B_SEQ); frames(2);
        ui.force = 1; frame(); ppm("drum-trig");
        dpg_go(1, 0); frame();
        check(dh_id(DM_SYN, DPG_SRC, 0, 0) == DSP_S0 + DS_TUNE && dh_id(DM_HEX, DPG_SRC, 0, 0) == DSP_COARSE &&
              dh_id(DM_SYN, DPG_AMP, 1, 3) == DSP_S0 + DS_LEVEL && dh_id(DM_HEX, DPG_AMP, 1, 3) == DSP_LEVEL &&
              dh_id(DM_SYN, DPG_LFO, 0, 1) == DSP_LDEP && dh_id(DM_HEX, DPG_LFO, 0, 1) == DSP_LDEP,
              "drum pages: the same four pages on every sound (SRC TUNE first, AMP 2/2 LEVEL, LFO SPEED DEPTH ..)");
    }
    {   /* nothing held: a knob edits the selected slot's sound, the project's own (wave 12: no copy, no star, no question) */
        int32_t t0 = DS_KITS[DRUM_DEFAULT_KIT - DRUM_SAMPLED].s[1].pitch;
        uint32_t m;
        frames(16);
        encs[panel.enc[EN_K1]] = 2; frame();
        check(dsp_get(DM_SYN, pkit.s[2].snd, &pkit.s[2].lfo, 2, DSP_S0 + DS_TUNE) == t0 + 2 && !ui.ask &&
              DS_KITS[DRUM_DEFAULT_KIT - DRUM_SAMPLED].s[1].pitch == t0 && (pkit.s[2].mach & SL_EDT),
              "drum SRC, KNOB 1 (TUNE) on the 606: the snare's slot +2 at once, nothing asked, the built-in 606 untouched");
        ui.msg_t = 0; ui.force = 1; frame();
        m = px_count(4, 58, 120, 16, C_WHITE);
        check(m > 20u, "drum screen: the slot's sound named under the slot (606 SNARE; the header's title place is SELECT's)");
        ppm("drum-src");
        kit_set(DRUM_SAMPLED + 1u); frame();            /* another kit's sound edited: no question either */
        encs[panel.enc[EN_K1]] = 1; frame();
        check(!ui.ask && (pkit.s[2].mach & SL_EDT), "drum screen: on the 909 too a turn edits its slot at once (no REPLACE)");
        kit_set(DRUM_DEFAULT_KIT); ui.msg_t = 0; frames(2);
    }
    {   /* hold a trig and turn: the value locked on that trig (the 606: every kit's sounds lock), inverted; OCT- removes it */
        uint32_t k5 = key_of_white(4);
        int32_t dec = DS_KITS[DRUM_DEFAULT_KIT - DRUM_SAMPLED].s[1].decay;
        tap(B_ENV); frames(2); dpg_go(3, 0); frames(16);    /* AMP 1/2: HOLD DECAY N.HOLD N.DEC */
        fm1_in.notes = 1u << k5; frame();                    /* step 5 held (the snare's trig there) */
        encs[panel.enc[EN_K2]] = 3; frame();
        check(dt_lk(4, 2, DSP_S0 + DS_DECAY) >= 0 && (int32_t)dpat.lk[dt_lk(4, 2, DSP_S0 + DS_DECAY)].val == dec + 3 && !kit_changed(),
              "drum lock: trig 5 held + KNOB 2 (DECAY) on the 606: locked on that trig (+3), the kit as it was");
        ui.force = 1; frame(); ppm("drum-held-lock");
        check(px_count(80, SG_KNOB_Y + 21, 40, 34, TE_DRUM) > 300u, "drum lock: the locked value drawn inverted (a filled box)");
        check(px_count(4, 76, 120, 16, TE_DRUM) > 60u, "drum lock: the held step's locks named under STEP 5 HELD (LOCK DECAY)");
        {   /* (wave 12, Ben 23 Nov 13:35: "I don't need to see the sequence on screen, it's shown on my keys") no step row */
            uint32_t j2, rowpx = 0;
            for (j2 = 0; j2 < 16u; j2++)
                rowpx += px_count(1u + j2 * 15u, 77, 13, 26, vel_col(100)) > 200u;
            check(rowpx == 0u, "w12: the drum screen draws no row of 16 steps (the keys and their lights are the steps)");
        }
        encs[panel.enc[EN_K4]] = 2; frame();                 /* N.DEC locked too */
        press(B_OCTDN); release(B_OCTDN);
        check(dt_lk(4, 2, DSP_S0 + DS_NDEC) < 0 && dt_lk(4, 2, DSP_S0 + DS_DECAY) >= 0 && str_eq(ui.msg, "LOCK OFF: NS DEC"),
              "drum lock: OCT- removes the lock of the knob last turned (N.DEC), the other stays");
        press(B_OCTDN); release(B_OCTDN);
        check(dt_lk_n(4, 2) == 0u && dt_has(4, 2), "drum lock: OCT- again: every lock of the trig goes (the trig stays)");
        tap(B_EDIT); frames(2);
        check(drum_pg == 1u && sg_held, "drum lock: a page button changes the category with the trig held (EDIT: SRC, the Rytm's way)");
        fm1_in.notes = 0; frames(2);
        check(dt_has(4, 2), "drum lock: the trig edited stays when let go");
    }
    {   /* (wave 9, Ben 23 Nov 2026: "Drum mutes: On the mixer"; wave 13: one map) hold GLO: the white keys mute the 16 sounds,
         * OCT+ held with a key solos */
        drum_mute = drum_solo = 0;
        press(B_GLO); frames(12);
        key(key_of_white(5)); key(key_of_white(11));
        check(drum_mute == (1u << 14 | 1u << 8) && dt_has(7, 5) && !trk[0].p[P_MUTE],
              "mixer, the drums selected: keys 6 and 12 mute CONGA and SNARE 2 (the drum grid; trigs kept; no voice muted)");
        press(B_OCTUP); key(key_of_white(0)); release(B_OCTUP);
        check(drum_solo == 1u << 4 && drum_mute == (1u << 14 | 1u << 8), "mixer: OCT+ held + key 1: the HAT soloed (the drum grid; the mutes stay)");
        ui.force = 1; frame(); ppm("layer-mix-drums");
        check(px_count(0, BK_Y + 34, 240, 80, TE_MUTE) > 400u && px_count(0, BK_Y + 34, 240, 80, TE_SOLO) > 200u &&
              px_count(0, 0, 240, 240, TE_RED) == 0u, "mixer: muted sounds yellow, the soloed one ice blue, no red");
        key(3); frames(2); check(!fill_now && !fill_held && !mix_pan, "mixer: G#3 (wave 13: voice 2's key): no PAN, no FILL");
        key(3); frames(2);
        release(B_GLO); frames(2);
        check(drum_mute == (1u << 14 | 1u << 8), "mixer let go: the mutes stay (performance state)");
        song.sel = 0; press(B_GLO); frames(12); key(key_of_white(3)); release(B_GLO); frames(2);
        check(!trk[0].p[P_MUTE] && drum_mute == (1u << 0 | 1u << 14 | 1u << 8),
              "mixer, a synth voice selected (wave 13: the same map): key 4 mutes the KICK, not voice 1");
        trk[0].p[P_MUTE] = 0; song.sel = TRK_DRUM; drum_mute = drum_solo = 0;
        tap(B_SCL); frames(2); check(drum_keys == DK_PLAY, "SEL on the drum screen: no MUTE mode any more (SEL is PAGE)");
        studio_open(SC_DRUM); tap(B_EDIT); frames(2);
    }
    {   /* ARP tapped: CHROMATIC mode, the selected sound across the keys; OCT+ an octave up */
        tap(B_ARP); frames(2);
        check(drum_keys == DK_CHROM && !grid_keys_on() && on_drum_page(), "CHROMATIC mode: ARP tapped on the drum track");
        fm1_in.notes = 1u << (CHR_KEY0 + 4u); frame(); ui.force = 1; frame(); ppm("drum-chrom");
        fm1_in.notes = 0; frame();
        {
            uint32_t l0 = drum_lane;
            tap(B_OCTUP); frames(20);
            check(drum_chr_oct == 0 && drum_lane == lane_step(l0, 1, 0), "CHROMATIC mode (wave 10): OCT+ tapped: the next sound (key order), the octave unchanged");
            tap(B_OCTDN); frames(20);
            tap(B_OCTUP); tap(B_OCTUP); frames(20);
            check(drum_chr_oct == 1 && drum_lane == l0, "CHROMATIC mode: OCT+ twice quickly: the keys an octave up");
            tap(B_OCTDN); tap(B_OCTDN); frames(20);
        }
        tap(B_ARP); frames(2);
        check(drum_keys == DK_PLAY && grid_keys_on() && drum_chr_oct == 0, "CHROMATIC mode: ARP again: the steps again");
    }
    {   /* (wave 10) SEQ held: RETRIG, a key retrigs its sound at the rate KNOB 1 picks (a black key: a pattern, not a rate) */
        uint32_t d0 = drt_live;
        press(B_SEQ); frames(30);
        key(PUNCH_BLACK[2]);                           /* A#3: pattern C, not a rate */
        ui.force = 1; frame(); ppm("drum-retrig");
        check(drt_live == d0 && str_eq(layer_name(LY_SEQ), "rhythm"), "RETRIG: SEQ held + the black key A#3: no rate (pattern C)");
        encs[panel.enc[EN_K1]] = 3; frame();
        check(drt_live == d0 + 3u, "RETRIG: KNOB 1 the rate");
        drt_live = 10;
        release(B_SEQ); frames(20);
    }
    {   /* (wave 10, Ben: "Maybe hold arp or seq and press play/stop") FILL: hold SEQ and PLAY; OCT+ with them latches; F#5 on
         * the drum GRID does nothing (not drawn) */
        uint32_t pg0 = sg_page(), pl = song.playing;
        fm1_in.notes = 1u << 25; frame();
        check(!fill_held, "FILL: F#5 on the drum GRID (wave 10): nothing (not drawn)");
        fm1_in.notes = 0; frame();
        press(B_SEQ); frames(12);
        press(B_PLAY); frames(2);
        check(fill_held && song.playing == pl, "FILL: SEQ held + PLAY held: a fill (no start)");
        tap(B_OCTUP);
        release(B_PLAY); frames(2);
        check(fill_lat && sg_page() == pg0, "FILL: SEQ + PLAY + OCT+: latched (the one latch), no page turned");
        release(B_SEQ); frames(2);
        ui.msg_t = 0; ui.force = 1; frame(); ppm("drum-fill");
        check(px_count(OP_BPM_R + 18, 2, 34, 16, C_WHITE) > 200u && px_count(52, 2, 70, 16, TE_DRUM) > 20u,
              "FILL: shown inverted in the header while on (V5: in the beat lights' place, the chip kept)");
        press(B_SEQ); frames(12); tap(B_PLAY); release(B_SEQ); frames(2);
        check(!fill_lat && song.playing == pl, "FILL: SEQ + PLAY again: off (as a latched pad)");
    }
    {   /* a HEX kit: the same pages, the HEX machine's parameters */
        kit_set((uint32_t)(0)); dpg_go(1, 0); frames(2); ui.msg_t = 0; ui.force = 1; frame(); ppm("drum-hex");
        check(drum_model() == DM_HEX && dpg_halves(1) == 3u, "a HEX kit: SRC 1/3 .. 3/3 of the LXR machine's parameters");
        kit_set((uint32_t)(DRUM_DEFAULT_KIT)); frames(2);
    }
    {   /* ---- the step GRID on every track (SPEC.md, 20 Nov 2026: Elektron style; ui_grid.c) */
        track_t *t = &trk[0];
        static track_t keep[NTRK];
        static dpat_t keep_dp;                          /* (PHEBUS wave 2: the drum pattern is not in trk[]) */
        uint32_t k5 = key_of_white(4), k1 = key_of_white(0), a0, a1;
        memcpy(keep, trk, sizeof keep); keep_dp = dpat;
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
        tap(B_ENV); frame();                            /* (wave 4, the Rytm's way: the page buttons change the GRID's tab) */
        check(syn_tab == 3u && syn_page() < NPAGES && PAGES[syn_page()].fam == FAM_ENV && on_drum_page(),
              "GRID: step held + ENV: the ENV tab on the knobs, the GRID stays");
        encs[panel.enc[EN_K1]] = 3; frame();
        {
            int l = lock_find(t, 4, P_ATK, 0);
            check(l >= 0 && t->lock[l].val == (int32_t)a0 + 3 && t->p[P_ATK] == (int16_t)a0,
                  "GRID: step held + ENV + KNOB 1: an ATK lock on step 5, the track's ATK unchanged");
        }
        ui.force = 1; frame();
        check(px_count(2 + 60 * 0 + DIAL_VX - 3, SG_KNOB_Y + 21, 30, 34, TE_COL[0]) > 300u,
              "GRID: the locked value drawn inverted (a box in the voice's colour)");
        tap(B_FX); frame();
        check(syn_tab == 4u && PAGES[syn_page()].fam == FAM_FX && cur_cat() == CT_INS && sg_held && ui.layer == LY_PLAY,
              "GRID: step held + FX tapped: the FX tab, INS (FX 1), the voice's own (wave 9: FX = voice FX, then MIX)");
        xs_type_set(t->p, 0, XT_PHASER);               /* (wave 5: FX 1 a PHASER; its TYPE cannot lock, its RATE can) */
        a1 = pg_id(&PAGES[syn_page()], 1);
        encs[panel.enc[EN_K2]] = 2; frame();
        check(lock_find(t, 4, a1, 0) >= 0, "GRID: FX + KNOB 2: its lock on step 5 (FX 1's RATE)");
        ui.force = 1; frame(); ppm("grid-synth-lock");
        press(B_OCTDN); release(B_OCTDN); frame();
        check(lock_find(t, 4, a1, 0) < 0 && lock_find(t, 4, P_ATK, 0) >= 0,
              "GRID: OCT-: the lock of the knob last turned goes (FX's), ATK's stays (as the drum screen)");
        {   /* (wave 9) EDIT with a step held steps its categories OSC > FILT > OSC; SEL the pages (the filter locks from the GRID) */
            uint32_t j, seen = 0, cut = 0;
            for (j = 0; j < 3u; j++) {
                tap(B_EDIT); frame();
                seen |= 1u << syn_tab;
                if (!cut && syn_tab == 2u) {
                    cut = 1;
                    encs[panel.enc[EN_K1]] = -5; frame();
                    ui.force = 1; frame(); ppm("grid-synth-lock-filter");
                }
            }
            tap(B_SCL); frame();
            check(seen == 0x06u && cur_cat() == CT_OSC && syn_hcur() == 1u && sg_held,
                  "GRID: step held + EDIT, again and again: OSC, FILT, OSC (round); SEL: OSC 2 (the step still held)");
            check(cut && lock_find(t, 4, P_FCUT, 0) >= 0 && t->lock[lock_find(t, 4, P_FCUT, 0)].val != t->p[P_FCUT],
                  "GRID: on FILT, KNOB 1: a CUTOFF lock on the step (the voice filter locks from the GRID)");
            {
                uint32_t j2;
                for (j2 = 0, j = 1; j2 < NPAGES; j2++)
                    if (sg_fam_in(PAGES[j2].fam, FAM_EDIT) || PAGES[j2].fam == FAM_FX || PAGES[j2].fam == FAM_LFO)
                        j &= text_w(&FONT_S, PAGES[j2].title) <= 122 - 46;
                check(j, "GRID: every lock page's name fits the header whole (no \"LOCK VOIC\")");
            }
        }
        {   /* (the hidden lock gesture is cut: PRESETS / ALGORITHM with a step held lock nothing, change no sound) */
            uint32_t nl = 0, j, eng = t->eng_req, pr = t->preset, sel = song.sel;
            for (j = 0; j < NLOCK; j++) nl += t->lock[j].step != 0xFFu && t->lock[j].param < P_COUNT;
            encs[panel.enc[EN_PRESET]] = 2; frame(); encs[panel.enc[EN_ALGO]] = 1; frame(); encs[panel.enc[EN_PRESET]] = -1; frame();
            for (j = 0; j < NLOCK; j++) nl -= t->lock[j].step != 0xFFu && t->lock[j].param < P_COUNT;
            check(!nl && t->eng_req == eng && t->preset == pr && song.sel == sel && sg_held == 1u << 4,
                  "GRID: step held + PRESETS / ALGORITHM: nothing (no lock, no sound, no track change)");
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
        syn_go(0, 0);                                   /* (TRIG: the step's own values) */
        fm1_in.notes |= 1u << k5; frame(); tap(B_SCL); frame();
        check(sg_alt == 1u && syn_tab == 0u && syn_hcur() == 1u, "GRID: step held + SEL (PAGE): TRIG 2/2, FLAG RETRIG NUDGE on the knobs");
        encs[panel.enc[EN_K1]] = 1; frame(); encs[panel.enc[EN_K3]] = -4; frame();
        check((t->step[4].flags & SF_ACCENT) && t->micro[4] == -4, "GRID: FLAG ACC, NUDGE -4");
        ui.force = 1; frame(); ppm("grid-synth-flag");
        press(B_OCTDN); release(B_OCTDN);
        check(!step_locked(t, 4) && !step_fill(t, 4), "GRID: step held + OCT-: its locks, nudge and condition go");
        fm1_in.notes &= ~(1u << k5); frame(); ui.msg_t = 0;
        {   /* (PHEBUS wave 5, Ben: "give me the spice & dice from minifreak") TRIG 2/2 with nothing held: SPICE, DICE; the tiles
             * show the roll's variation; the notes stay as written */
            step_t keep[NSTEP];
            int16_t d0 = mxt[0].dice;
            memcpy(keep, t->step, sizeof keep);
            check(syn_tab == 0u && syn_hcur() == 1u, "GRID: TRIG 2/2, nothing held");
            encs[panel.enc[EN_K1]] = 80; frame();
            check(mxt[0].spice > 60, "GRID: TRIG 2/2, KNOB 1: SPICE up");
            encs[panel.enc[EN_K2]] = 1; frame();
            check(mxt[0].dice == (d0 + 1) % 10000 && !strncmp(ui.msg, "DICE #", 6), "GRID: TRIG 2/2, KNOB 2: a new roll (DICE #n)");
            encs[panel.enc[EN_K2]] = -1; frame();
            check(mxt[0].dice == d0, "GRID: KNOB 2 left: back to the roll before");
            encs[panel.enc[EN_K2]] = 3; frame();
            ui.msg_t = 0; ui.force = 1; frame(); ppm("grid-synth-spice");
            check(!memcmp(keep, t->step, sizeof keep), "GRID: Spice and Dice change no step");
            mxt[0].spice = 0; mxt[0].dice = d0;
        }
        tap(B_SCL); frame();                            /* (TRIG 1/2: NOTE STEP HIT VEL) */
        key(3);                                           /* (wave 10: a black key turns no page: not drawn) */
        check(ui.cursor / 16u == 0u, "GRID: black key 2 (wave 10, not drawn): no page turned");
        press(B_OCTUP); release(B_OCTUP); frames(20);     /* (wave 10) OCT+ tapped: the next page of steps */
        check(ui.cursor / 16u == 1u, "GRID: OCT+ tapped: the page of steps 17-32");
        ui.force = 1; frame(); ppm("grid-synth-p2");
        ui.msg_t = 0;
        press(B_OCTUP); release(B_OCTUP); frames(20);     /* REVIEW 4: past the last page, one added (a copy) */
        check(t->p[P_SLEN] == 48 && ui.cursor / 16u == 2u && !memcmp(&t->step[36], &t->step[20], sizeof(step_t)) &&
              t->step[36].n == 3u && !strcmp(ui.msg, "48 STEPS"),
              "GRID: OCT+ on the last page: a page added, a copy of the last, shown, \"48 STEPS\"");
        ui.force = 1; frame(); ppm("grid-synth-page-add");
        press(B_OCTUP); release(B_OCTUP); frames(20);
        check(t->p[P_SLEN] == 64 && ui.cursor / 16u == 3u && !strcmp(ui.msg, "64 STEPS"), "GRID: OCT+ again: 64 STEPS");
        press(B_OCTUP); release(B_OCTUP); frames(20);
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
        tap(B_SEQ); frames(2);                          /* (wave 9: SEQ's categories, TRIG then PATTERN) */
        check(on_drum_page() && syn_tab == 0u && cur_cat() == CT_PATT, "GRID, SEQ on TRIG: its next category, PATTERN");
        {   int16_t sw = t->p[P_SSWING]; encs[panel.enc[EN_K3]] = 5; frame();
            check(t->p[P_SSWING] == sw + 5, "GRID, PATTERN: KNOB 3 SWING"); t->p[P_SSWING] = sw; }
        tap(B_SEQ); frames(2); tap(B_SEQ); frames(2);
        check(on_drum_page() && syn_tab == 0u && syn_hcur() == 0u, "GRID, SEQ again (A-D), and again: TRIG 1/2");
        open_family(FAM_ENV); frames(2);
        tap(B_SEQ); frames(2);
        check(on_drum_page() && kb_grid && song.sel == 0u, "REVIEW 2: SEQ on a sound page (ENV): back to the GRID");
        go_home(); frames(2);
        check(!kb_grid, "HOME: the keys play notes again");
        /* the drum grid: hold a step, as on the synths; its own values on TRIG (PHEBUS wave 3: SEQ, the Rytm's TRIG page) */
        song.sel = TRK_DRUM; studio_open(SC_DRUM); drum_lane = 0; drum_cursor = 0; dpg_go(0, 0); frames(16);
        dtest_clear(1);
        fm1_in.notes |= 1u << key_of_white(1); frame();
        check(dt_has(1, 0) && sg_held == 2u, "DRUM GRID: an empty step pressed: set at once (as before)");
        for (i = 0; i < 13u; i++) { encs[panel.enc[EN_K2]] = 1; frame(); }
        encs[panel.enc[EN_K3]] = 5; frame();
        check(dtest_rat(1, 0) == 1u && dt_x(1, 0) >= 0 && dpat.x[dt_x(1, 0)].micro == 5 && !TDRUM->micro[1],
              "DRUM GRID, TRIG: step held + KNOB 2 RETRIG 1/32 (x2), KNOB 3 NUDGE 5: the trig's own (not the step's)");
        ui.force = 1; frame(); ppm("grid-drum-held");
        {   /* (PHEBUS wave 2, restyled in wave 3) TRIG 2/2: the trig's CHANCE (the Rytm's steps), its retrig's LENGTH and
             * CURVE, and its NOTE (a pitch lock); the drum voice's own pages still lock per step (FX, a trig held) */
            const dtx_t *x = &dpat.x[dt_x(1, 0)];
            tap(B_SCL); frame();
            check(drum_pg == 0u && dpg_half() == 1u, "DRUM GRID: SEL with a step held: TRIG 2/2");
            encs[panel.enc[EN_K1]] = -1; frame();
            encs[panel.enc[EN_K2]] = 1; frame();
            encs[panel.enc[EN_K3]] = -1; frame();
            encs[panel.enc[EN_K4]] = 2; frame();
            check(x->prob == 41 && x->rlen == 24 && x->rfade < 0 && x->rrate == 13 && x->micro == 5 &&
                  dt_lk(1, 0, DSP_NOTE) >= 0 && (int8_t)dpat.lk[dt_lk(1, 0, DSP_NOTE)].val == 2,
                  "DRUM GRID, TRIG 2/2: CHANCE 50 -> 41 %, the retrig's LENGTH 1 -> 1.5 steps, CURVE down, NOTE +2");
            ui.force = 1; frame(); ppm("grid-drum-held-2");
            tap(B_SCL); frame();
            tap(B_FX); frames(2);                       /* (wave 9) FX, a step held: the drum voice's FX (INS), the GRID stays */
            check(drum_pg == DPG_FX && cur_cat() == CT_DINS && sg_held,
                  "DRUM GRID: a step held + FX: the FX tab, INS (the kit's INSERT; the Rytm's page keys)");
            check(drum_pg == DPG_FX && dpg_half() == 0u && !strcmp(PAGES[dbus_page(0)].title, "INSERT"), "DRUM GRID: FX: INS (INSERT)");
            encs[panel.enc[EN_K1]] = 1; frame();        /* (U2: the INSERT empty, its knobs do nothing, lock nothing) */
            check(!step_locked(TDRUM, 1) && lock_find(TDRUM, 1, P_IA, 0) < 0,
                  "DRUM GRID: the kit INSERT empty (OFF): a knob with a step held locks nothing (as a synth's empty slot; U2)");
            TDRUM->p[P_ITYPE] = 1;                      /* (SOFT) */
            encs[panel.enc[EN_K1]] = 1; frame();
            check(step_locked(TDRUM, 1) && lock_find(TDRUM, 1, P_IA, 0) >= 0,
                  "DRUM GRID: on the FX tab, a knob locks the drum voice's own parameter on that step (as any tab; KNOB 1 INSERT's A)");
            ui.force = 1; frame(); ppm("grid-drum-lock");
            press(B_OCTDN); release(B_OCTDN);
            check(!step_locked(TDRUM, 1), "DRUM GRID: OCT- on the FX tab: that page's locks go");
            dpg_go(DPG_FX, DBH_SENDS); frame();        /* (the drum effects under FX) SENDS: the sound's, a lock on the hit */
            encs[panel.enc[EN_K1]] = 4; frame();
            check(dt_lk(1, 0, DSP_DEL) >= 0 && !step_locked(TDRUM, 1),
                  "DRUM GRID: FX > SENDS with a step held: DELAY locks the sound's delay send on that hit (as AMP 2/2 did)");
            encs[panel.enc[EN_K3]] = 1; frame();
            check(str_eq(ui.msg, "NOT LOCKABLE") && !sl_kfx_off(0), "DRUM GRID: KIT FX with a step held: not lockable (the slot's)");
            dt_lk_del(1, 0, DSP_ALL);
            lock_del(TDRUM, 1, P_COUNT); dpg_go(0, 0);
            TDRUM->p[P_ITYPE] = 0;
        }
        fm1_in.notes &= ~(1u << key_of_white(1)); frame();
        check(dt_has(1, 0), "DRUM GRID: let go after an edit: kept");
        key(key_of_white(1));
        check(!dt_has(1, 0), "DRUM GRID: tapped again: cleared");
        tap(B_ENV); frames(2);                            /* ENV / LFO on the drum GRID: the drum screen's AMP and LFO */
        check(on_drum_page() && drum_pg == 3u, "DRUM GRID: ENV opens the sound's AMP page (the drum screen's)");
        ui.msg_t = 0; tap(B_LFO); frames(2);
        check(on_drum_page() && drum_pg == 4u, "DRUM GRID: LFO opens the sound's LFO page (every drum sound has one now)");
        tap(B_FX); frames(2);
        check(on_drum_page() && drum_pg == DPG_FX, "DRUM: FX: the FX tab (INS SENDS SLICER, then MIX)");
        {   /* the kit's FILTER (EDIT's KIT FLT) and the sound's SENDS (FX), nothing held */
            int16_t f0 = TDRUM->p[P_TFLT];
            uint32_t d0 = sl_send(drum_lane, 1), o;
            dpg_go(DPG_FX, DBH_FILTER); frame();
            encs[panel.enc[EN_K1]] = -5; frame();
            dpg_go(DPG_FX, DBH_SENDS); frame(); ui.force = 1; frame(); ppm("drum-fx-sends");
            encs[panel.enc[EN_K1]] = 4; frame();
            for (o = 0, i = 0; i < 16u; i++) o += i != drum_lane && sl_send(i, 1) != 0u;
            check(TDRUM->p[P_TFLT] < f0 && sl_send(drum_lane, 1) > d0 && !o && song.g[G_DRDLY] == 0 &&
                  px_count(0, SG_KNOB_Y, 240, 58, TE_DRUM) > 40u,
                  "DRUM: KIT FLT turns the kit's filter; FX > SENDS the selected sound's delay send only (teal)");
            TDRUM->p[P_TFLT] = f0; sl_send_set(drum_lane, 1, d0);
        }
        for (i = 0; i < 6u && cur_cat() != CT_MIX; i++) tap(B_FX);
        ui.force = 1; frames(2);
        check(!on_drum_page() && cur_cat() == CT_MIX && px_count(0, 0, 240, OP_BAR_Y, TE_DRUM) == 0u,
              "DRUM: FX, four more: MIX, the one white MIX screen (U1), no teal above the strip");
        tap(B_SEQ); frames(2);
        check(on_drum_page() && drum_keys == DK_PLAY && grid_keys_on(), "DRUM: SEQ from them: back to the drum screen");
        encs[panel.enc[EN_ALGO]] = -1; frames(2);
        check(song.sel == 2u && on_drum_page() && kb_grid, "GRID: ALGORITHM to a synth track: its GRID");
        song.sel = 0; go_home(); frames(16);
        memcpy(trk, keep, sizeof keep); dpat = keep_dp; sync_reload = 1;
        song.sel = TRK_DRUM;
    }
    {   /* ---- the four sequencer bugs of the review (REVIEW.md 11) and the full lock table (ENG-9) */
        track_t *t = &trk[0];
        static track_t keep[NTRK];
        static dpat_t keep_dp;                          /* (PHEBUS wave 2: the drum pattern is not in trk[]) */
        uint32_t k3 = key_of_white(2), k5 = key_of_white(4), k6 = key_of_white(5), k7 = key_of_white(6), j, n, ok;
        memcpy(keep, trk, sizeof keep); keep_dp = dpat;
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
            check(ui.layer == LY_SEQ, "bug (d): SEQ held 180 ms: its layer shows");
            release(B_SEQ); frames(2);
            check(on_drum_page() && ui.page == pg, "bug (d): ... let go: no tap (the GRID stays)");
            press(B_FX); frames((SHOW_MS + 40u) / 16u); release(B_FX); frames(2);
            check(on_drum_page() && ui.page == pg, "bug (d): FX held until shown, let go: no FX page");
            press(B_EDIT); frames((SHOW_MS + 40u) / 16u); release(B_EDIT); frames(2);
            check(on_drum_page() && ui.page == pg, "bug (d): EDIT held until shown, let go: no EDIT page");
            tap(B_FX); frames(2);
            check(cur_cat() == CT_INS, "bug (d): FX tapped quickly: a tap (wave 9: the voice's FX first, INS)");
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
            encs[panel.enc[EN_K4]] = 1; frame();         /* (wave 9: COND on KNOB 4; OCT+ with a step held is no gesture) */
            tap(B_SCL); frame(); encs[panel.enc[EN_K3]] = 5; frame(); tap(B_SCL); frame();
            tap(B_ENV); frame(); encs[panel.enc[EN_K1]] = 4; frame(); encs[panel.enc[EN_K2]] = 4; frame();
            fm1_in.notes &= ~(1u << k5); frames(2); ui.msg_t = 0;
            st0 = t->step[4]; m0 = t->micro[4]; f0 = step_fill(t, 4);
            for (j = 0, nl0 = 0; j < NLOCK; j++) nl0 += t->lock[j].step == 4u;
            check(step_on(&st0) && note_len(t, 4) == 2u && m0 == 5 && f0 == FC_FILL && nl0 == 2u, "undo: set up: note, LEN 2, NUDGE 5, FILL, 2 locks");
            key(k5);
            check(!step_on(&t->step[4]) && !t->micro[4] && !step_fill(t, 4) && !step_locked(t, 4), "undo: the step tapped: cleared, all of it");
            press(B_SAVE); frames((SHOW_MS + 40u) / 16u); ui.force = 1; frame();
            check(pm_lm && str_eq(pm_lm, "UNDO") && pm_lp && str_eq(pm_lp, "REDO"), "undo (wave 10): SAVE held: the legend says - UNDO + REDO");
            ppm("layer-erase-undo");
            edges_btn |= BT(B_OCTDN); fm1_in.buttons |= BT(B_OCTDN); frame(); fm1_in.buttons &= ~BT(B_OCTDN); frame();
            ui.force = 1; frame();
            for (j = 0; j < NLOCK; j++) nl += t->lock[j].step == 4u;
            check(!memcmp(&t->step[4], &st0, sizeof st0) && note_len(t, 4) == 2u && t->micro[4] == m0 && step_fill(t, 4) == f0 && nl == nl0,
                  "undo: SAVE + OCT-: the whole step back (note, LEN, locks, nudge, condition)");
            edges_btn |= BT(B_OCTUP); fm1_in.buttons |= BT(B_OCTUP); frame(); fm1_in.buttons &= ~BT(B_OCTUP); frame();
            check(!step_on(&t->step[4]) && !step_locked(t, 4) && !step_fill(t, 4), "undo: SAVE + OCT+: redo, cleared again");
            release(B_SAVE); frames(20);                   /* (past LY_QUIET_MS) */
            go_home(); frames(4);
        }
        memcpy(trk, keep, sizeof keep); dpat = keep_dp; sync_reload = 1;
        song.sel = TRK_DRUM;
    }
    {   /* REVIEW 4: the GRID follows the playhead onto the next page; a page picked by hand stays until it moves on */
        track_t *t = &trk[0];
        static track_t keep[NTRK];
        static dpat_t keep_dp;                          /* (PHEBUS wave 2: the drum pattern is not in trk[]) */
        uint32_t j;
        memcpy(keep, trk, sizeof keep); keep_dp = dpat;
        song.sel = 0; go_home(); frames(16);
        steps_clear(t); t->p[P_SLEN] = 32;
        tap(B_SEQ); frames(2);
        ui.cursor = 3;
        transport_req = 1;
        for (j = 0; j < 400u && t->seq_idx % 32u < 17u; j++) frame();
        check(song.playing && t->seq_idx % 32u >= 16u && ui.cursor / 16u == 1u, "GRID, playing: the view follows the playhead onto page 2");
        ui.force = 1; frame(); ppm("grid-synth-follow");
        tap(B_OCTDN);                                     /* (wave 10: -/+, page 1 by hand) */
        frames(4);
        check(t->seq_idx % 32u >= 16u && ui.cursor / 16u == 0u, "GRID, playing: page 1 picked by hand stays while the playhead is on 2");
        for (j = 0; j < 400u && t->seq_idx % 32u >= 16u; j++) frame();
        frames(2);
        check(t->seq_idx % 32u < 16u && ui.cursor / 16u == 0u, "  ... and the playhead back on page 1: page 1");
        for (j = 0; j < 400u && t->seq_idx % 32u < 17u; j++) frame();
        check(ui.cursor / 16u == 1u, "  ... onto page 2 again: page 2");
        transport_req = 2; frames(4);
        go_home(); frames(4);
        memcpy(trk, keep, sizeof keep); dpat = keep_dp; sync_reload = 1;
        song.sel = TRK_DRUM;
    }
    {   /* PHEBUS (Ben, 21 Nov 2026, picker "Yes, add it"): STEP REC on a synth track's GRID. REC, stopped: the keys play,
         * a note or a chord lands on the cursor's step and the cursor moves on; SEQ a rest, ARP a tie (or a key held + SEQ);
         * round at the end; SELECT / KNOB 2 move back; one undo the whole pass; REC again leaves; playing: live */
        track_t *t = &trk[1];
        static track_t keep[NTRK];
        static dpat_t keep_dp;                          /* (PHEBUS wave 2: the drum pattern is not in trk[]) */
        uint32_t ka = key_of_white(0), kb = key_of_white(2), kc = key_of_white(4), na, nb, nc, j, empty = 1;
        memcpy(keep, trk, sizeof keep); keep_dp = dpat;
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
        check(t->step[5].note[0] == nb && t->step[6].time != ST_TIE && !step_on(&t->step[6]) && ui.cursor == 7u,
              "STEP REC: a key held + SEQ: written, then a rest (wave 9: SEQ always a rest; the tie is ARP's)");
        fm1_in.notes &= ~(1u << kb); frames(2);
        check(ui.cursor == 7u && !step_on(&t->step[7]), "STEP REC: ... let go: nothing more written");
        press(B_OCTUP); release(B_OCTUP); frames(20);
        check(song.octave == 0 && ui.cursor == 8u, "STEP REC (wave 10): OCT+ tapped: the step after (the cursor on 9), no octave");
        press(B_OCTDN); release(B_OCTDN); frames(20);
        check(ui.cursor == 7u, "STEP REC: OCT- tapped: the step before");
        tap(B_OCTUP); tap(B_OCTUP); frames(20);
        check(song.octave == 1 && ui.cursor == 7u, "STEP REC: OCT+ twice quickly: the octave up, the cursor where it was");
        key(ka); frames(2);
        check(t->step[7].note[0] == na + 12u && ui.cursor == 8u, "STEP REC: ... the note an octave up on step 8");
        tap(B_OCTDN); tap(B_OCTDN); frames(20);
        encs[panel.enc[EN_K2]] = 20; frame();
        check(ui.cursor == 15u, "STEP REC: KNOB 2: the cursor (to the last step, no further)");
        key(kc); frames(2);
        check(t->step[15].note[0] == nc && ui.cursor == 0u, "STEP REC: a note on the last step: the cursor round to step 1");
        t->p[P_SLEN] = 32; ui.cursor = 15; frame();
        key(kb); frames(2);
        check(t->step[15].note[0] == nb && ui.cursor == 16u && sg_page() == 1u, "STEP REC: 32 steps: on from 16 onto page 2");
        ui.force = 1; frame(); ppm("grid-step-rec-p2");
        encs[panel.enc[EN_SELECT]] = -1; frame();
        check(ui.cursor == 16u, "STEP REC: SELECT: nothing (wave 9: the type knob; the cursor is KNOB 2's)");
        encs[panel.enc[EN_K2]] = -1; frame();
        check(ui.cursor == 15u && sg_page() == 0u, "STEP REC: KNOB 2 left: the cursor back a step (page 1)");
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
        press(B_SAVE); press(B_OCTDN); release(B_OCTDN); release(B_SAVE); frames(2);
        for (j = 0; j < 32u; j++) empty &= !step_on(&t->step[j]) && t->step[j].time != ST_TIE;
        check(empty && t->p[P_SLEN] == 16 && !strcmp(ui.msg, "UNDO"), "STEP REC: SAVE + OCT-: the whole pass undone (the empty 16 steps it started from)");
        press(B_SAVE); press(B_OCTUP); release(B_OCTUP); release(B_SAVE); frames(2);
        check(t->step[0].note[0] == nc && t->step[5].note[0] == nb, "STEP REC: SAVE + OCT+: redone");
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
        check(sr_on && ui.hold_kind == 0u, "STEP REC: REC held (wave 10): STEP REC as a tap, no ring");
        release(B_REC); frames(2); tap(B_REC); frames(2);
        song.octave = 0; go_home(); frames(4);
        memcpy(trk, keep, sizeof keep); dpat = keep_dp; sync_reload = 1;
        song.sel = TRK_DRUM;
    }
    {   /* the REC screen (the free take is cut, PHEBUS wave 1: an empty project records at the tempo set, no MODE) */
        static track_t keep[NTRK];
        static dpat_t keep_dp;                          /* (PHEBUS wave 2: the drum pattern is not in trk[]) */
        uint32_t lw;
        memcpy(keep, trk, sizeof keep); keep_dp = dpat;
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
        check(trk[song.sel].p[P_SLEN] == 16, "REC screen: KNOB 2 nothing (wave 8: the length is hold SEQ's STEPS, one place)");
        trk[song.sel].p[P_SLEN] = 32;
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
        memcpy(trk, keep, sizeof keep); dpat = keep_dp;
        rec_wait = 0; frame();
    }
    for (i = 0; i < DRUM_KITS; i++) { kit_set((uint32_t)((int16_t)i)); ui.force = 1; frame(); }

    {   /* menu NOTES (PR #11 by @renebohne): sounding synth voices light their keys */
        song.sel = 0; go_home(); ui.force = 1; frame();
        trk[0].p[P_CHORD] = 0; trk[0].p[P_QUANT] = 0; trk[0].p[P_ROOT] = 0; trk[0].p[P_TRANS] = 0;
        song.octave = 0;
        lights_notes = 0;
        trk[0].v[0].active = 1; trk[0].v[0].gate = 1; trk[0].v[0].stage = 1; trk[0].v[0].note = 60;
        frame();
        check((keys_lit() & (1u << 7)) == 0, "NOTES off: a sounding synth voice lights no key");
        chain_goto(FAM_GLO, "LIGHTS"); frame();
        encs[panel.enc[EN_K4]] = 1; frame();
        check(lights_notes == 1u, "GLO > DEVICE > LIGHTS: NOTES (KNOB 4) right: ON");
        ui.force = 1; frame(); ppm("page-menu-notes");
        go_home(); frame();
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
                {LY_SEQ, 1, "SEQ roll"}, {LY_FX, 0, "FX punch"}, {LY_GLO, 1, "GLO mix (wave 13: the parts heard: the hat key C4)"}};
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
            ui.layer = LY_EDIT;
            check(keys_lit() == 0u && keys_notes_dim() == 0u, "NOTES on, EDIT held (wave 10: MASTER's pair only): no key lit or glowing");
            ui.layer = LY_ARP;
            check(keys_lit() == 0u && keys_notes_dim() == 0u, "NOTES on, ARP held (wave 10: -/+ and MASTER only): no key lit");
            lights_notes = 0;
            ui.layer = LY_SEQ;
            check((keys_lit() >> 7 & 1u) == 0, "NOTES off, SEQ: as before (no note lights)");
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
        chain_goto(FAM_GLO, "AUDIO"); ui.force = 1; frame(); ppm("menu-usb");
        usb_full = 0;
        encs[panel.enc[EN_K2]] = 1; frame();
        check(usb_full == 1u && (lights_word() >> 11 & 1u) == 1u, "DEVICE > AUDIO: USB (KNOB 2) -> FULL, saved with the settings");
        tap(B_OCTDN); frames(20);
        check(usb_full == 0u && !ui.home && cur_cat() == CT_DEVICE, "DEVICE > AUDIO: OCT- steps USB back to MASTER, the page stays (U5: never HOME)");
        check(usb_serial == 0u && !(lights_word() >> 16 & 1u), "DEVICE > AUDIO: SERIAL OFF by default (2.3 settings read so)");
        encs[panel.enc[EN_K3]] = 1; frame();
        check(usb_serial == 1u && (lights_word() >> 16 & 1u), "DEVICE > AUDIO: SERIAL (KNOB 3) -> ON, saved with the settings");
        ui.force = 1; frame(); ppm("menu-serial");
        lights_from_word(lights_word());
        check(usb_serial == 1u, "DEVICE > AUDIO: SERIAL read back");
        tap(B_OCTDN); frames(20);
        check(usb_serial == 0u && cur_cat() == CT_DEVICE, "DEVICE > AUDIO: OCT- steps SERIAL back to OFF");
        go_home(); frame();
        chain_goto(FAM_GLO, "LIGHTS"); frame();
        encs[panel.enc[EN_K4]] = -1; frame();
        check(lights_notes == 0u, "DEVICE > LIGHTS: NOTES left: off");
        go_home(); frame();
    }

    {   /* menu LIGHTS / KEYS: the backlight for playing in the dark */
        uint32_t m, nbits;
        go_home(); ui.force = 1; frame();
        check(lights_lvl == LIGHTS_OFF && fm1_led_bg_ns == 0u && lights_keys_mask() == 0u, "LIGHTS: off by default");
        chain_goto(FAM_GLO, "LIGHTS"); ui.force = 1; frame();
        encs[panel.enc[EN_K2]] = 1; frame();
        check(lights_lvl == LIGHTS_LOW && fm1_led_bg_ns == 500u, "LIGHTS: KNOB 2 right: LOW (a 0.5 us pulse a frame)");
        encs[panel.enc[EN_K2]] = 1; frame();
        check(lights_lvl == LIGHTS_MID && fm1_led_bg_ns == 1000u, "LIGHTS: MID (1 us)");
        encs[panel.enc[EN_K2]] = 1; frame(); encs[panel.enc[EN_K2]] = 1; frame();
        check(lights_lvl == LIGHTS_HIGH && fm1_led_bg_ns == 2000u, "LIGHTS: HIGH (2 us, under the 4 us glow), and it stops there");
        ui.force = 1; frame(); ppm("page-menu-lights");
        encs[panel.enc[EN_K2]] = -1; frame();
        check(lights_lvl == LIGHTS_MID, "LIGHTS: KNOB 2 left: dimmer");
        tap(B_OCTUP); frames(20);
        check(lights_lvl == LIGHTS_HIGH && cur_cat() == CT_DEVICE, "LIGHTS: OCT+ one step up, the page stays");
        encs[panel.enc[EN_K2]] = -9; frame();
        check(lights_lvl == LIGHTS_OFF && fm1_led_bg_ns == 0u, "LIGHTS: ... down to OFF");
        encs[panel.enc[EN_K3]] = 1; frame();
        m = lights_keys_mask();
        check(lights_keys == KEYS_C && lights_lvl == LIGHTS_LOW && m == (1u << 7 | 1u << 19),
              "KEYS (KNOB 3): C KEYS (C4, C5), LIGHTS raised to LOW");
        encs[panel.enc[EN_K3]] = 1; frame();
        for (m = lights_keys_mask(), nbits = 0; m; m &= m - 1u)
            nbits++;
        check(lights_keys == KEYS_WHITE && nbits == 16u && !(lights_keys_mask() & (1u << 1)),
              "KEYS: WHITE KEYS (16 of 27, F#3 dark)");
        encs[panel.enc[EN_K3]] = 1; frame(); encs[panel.enc[EN_K3]] = 1; frame();
        check(lights_keys == KEYS_ALL && lights_keys_mask() == 0x7FFFFFFu, "KEYS: ALL KEYS (every one of the 27 keys), and it stops there");
        ui.force = 1; frame(); ppm("page-menu-keys-all");
        encs[panel.enc[EN_K3]] = -9; frame();
        check(lights_keys == KEYS_OFF, "KEYS: KNOB 3 left: back to OFF");
        lights_keys = KEYS_ALL;
        check(lights_word() >> 4 & 15u, "KEYS: ALL KEYS kept in the settings word");
        lights_from_word(lights_word());
        check(lights_keys == KEYS_ALL, "KEYS: ALL KEYS read back");
        lights_lvl = LIGHTS_OFF; frame();
        check(lights_keys_mask() == 0u && fm1_led_bg_ns == 0u, "KEYS: nothing lit while LIGHTS is OFF");
        lights_keys = KEYS_OFF;
        go_home(); frame();
    }

    {   /* (review U5, Ben 26 Nov 2026: "Redraw in the GLO frame") GLO > DEVICE: four normal GLO pages, SCREEN LIGHTS AUDIO
         * TOOLS, header GLO, the tab row, the dials; SEL the next page, OCT- steps a knob (never HOME) */
        uint32_t i2, ndev = 0, ok;
        static const char *const DT[4] = {"SCREEN", "LIGHTS", "AUDIO", "TOOLS"};
        go_home(); frame();
        tap(B_GLO); check(cur_cat() == CT_GLOBAL, "GLO: GLOBAL");
        tap(B_GLO); check(cur_cat() == CT_SYSTEM, "GLO again: SYSTEM");
        tap(B_GLO); check(cur_cat() == CT_MIDI && !strcmp(PAGES[ui.page].title, "MIDI"), "GLO again: MIDI");
        tap(B_SCL); tap(B_SCL); check(!strcmp(PAGES[ui.page].title, "MIDI 3"), "MIDI, SEL, SEL: MIDI 3");
        check(PAGES[ui.page].id[1] == V_LEARN && PAGES[ui.page].id[2] == V_LCLEAR, "U5: MIDI 3: LEARN, then CLEAR beside it");
        tap(B_GLO); frames(2); ui.force = 1; frame(); ppm("menu-screen");
        check(!ui.menu && cur_cat() == CT_DEVICE && !strcmp(PAGES[ui.page].title, "SCREEN"), "GLO again: DEVICE, a GLO page: SCREEN");
        check(nav_tabs_shown == (CT_GLOBAL | CT_SYSTEM << 8 | CT_MIDI << 16 | CT_DEVICE << 24), "U5: DEVICE has GLO's tab row");
        check(px_count(2, 1, 46, 18, TE_G4) > 40u, "U5: DEVICE's header: the tag GLO, white-grey");
        for (i2 = 0; i2 < 4u; i2++) {
            ok = !strcmp(PAGES[ui.page].title, DT[i2]) && PAGES[ui.page].fam == FAM_GLO && cur_cat() == CT_DEVICE;
            ndev += ok;
            if (i2 == 1u) { ui.force = 1; frame(); ppm("menu-lights"); }
            if (i2 == 2u) { ui.force = 1; frame(); ppm("menu-audio"); }
            if (i2 == 3u) { ui.force = 1; frame(); ppm("menu-system"); }
            tap(B_SCL);
        }
        check(ndev == 4u && !strcmp(PAGES[ui.page].title, "SCREEN"), "U5: DEVICE: SEL walks SCREEN LIGHTS AUDIO TOOLS, round");
        for (i2 = 0, ok = 1; i2 < 4u; i2++) {
            const param_desc_t *dd;
            int16_t *vp;
            uint32_t c2;
            for (c2 = 0; c2 < 4u; c2++)
                if ((dd = page_desc(&PAGES[ui.page], c2, &vp)) != 0)
                    ok &= strlen(dd->label) <= 6u;
            tap(B_SCL);
        }
        check(ok, "U5: DEVICE's knob names six characters at most (V2)");
        chain_goto(FAM_GLO, "LIGHTS"); frame();
        encs[panel.enc[EN_SELECT]] = 1; frame();
        check(!strcmp(PAGES[ui.page].title, "LIGHTS"), "DEVICE: SELECT does nothing (no type here)");
        encs[panel.enc[EN_K1]] = -1; frame();
        check(disco == DISCO_QUIET, "DEVICE > LIGHTS: KNOB 1 sets DISCO: left, QUIET");
        encs[panel.enc[EN_K1]] = 1; frame();
        check(disco == DISCO_NORMAL, "DEVICE > LIGHTS: ... right, NORMAL");
        encs[panel.enc[EN_K3]] = 1; frame(); encs[panel.enc[EN_K3]] = 1; frame();
        check(lights_keys == KEYS_WHITE, "DEVICE > LIGHTS: KNOB 3 sets KEYS");
        encs[panel.enc[EN_K4]] = 1; frame();
        check(lights_notes == 1u, "DEVICE > LIGHTS: KNOB 4 sets NOTES");
        tap(B_OCTDN); frames(20); tap(B_OCTDN); frames(20);
        check(!ui.home && cur_cat() == CT_DEVICE, "U5: OCT- on DEVICE never goes HOME (it steps the knob turned)");
        chain_goto(FAM_GLO, "TOOLS"); frame();
        encs[panel.enc[EN_K2]] = -1; frame();
        check(ui.menu == 0, "DEVICE > TOOLS: ABOUT left: nothing");
        encs[panel.enc[EN_K2]] = 1; frame(); ui.force = 1; frame(); ppm("page-about");
        check(ui.menu == 2, "DEVICE > TOOLS: ABOUT (KNOB 2) right: ABOUT opens");
        tap(B_OCTDN); frames(2);
        check(ui.menu == 0 && cur_cat() == CT_DEVICE && !strcmp(PAGES[ui.page].title, "TOOLS"), "U5: OCT- from ABOUT: back to TOOLS, in GLO");
        tap(B_GLO); frames(2); check(cur_cat() == CT_GLOBAL && ui.layer == LY_PLAY, "GLO on DEVICE: round to GLOBAL");
        tap(B_GLO); check(cur_cat() == CT_SYSTEM, "... and GLO again: SYSTEM (no extra press counted)");
        lights_keys = KEYS_OFF; lights_notes = 0; lights_lvl = LIGHTS_OFF;
        go_home(); ui.force = 1; frame();
    }

    {   /* SELECT (2.4): the pages of the family shown; the tempo on HOME (TRACKS), in a layer, on REC */
        int16_t bpm;
        #define PT() (PAGES[ui.page].title)
        #define SEL(n) do { encs[panel.enc[EN_SELECT]] = (n); frame(); } while (0)
        {   /* PHEBUS, 21 Nov 2026 (Ben: "Merge and move"): each voice's chain without the shared effects, LFO 2 named
             * LFO SYNC; DLY, REV/CHO under GLO, neutral; every parameter still on a page (CLOUD, TEXTURE: cut, wave 1) */
            /* (PHEBUS wave 4, Ben: "The same layout on every voice") the same chain on every synth engine */
#define SYN_CHAIN "OSC|OSC 2|OSC 3|OSC 4|FILT|FILT 2|ENV|FILT ENV|FX 1|FX 2|FX 3|SENDS|SLICER|LFO 1|LFO SYNC|LFO 2|MATRIX"   /* (wave 8: SENDS) */
            static const char *const WANT[5] = {SYN_CHAIN, SYN_CHAIN, SYN_CHAIN, SYN_CHAIN,
                ""};   /* (wave 8: the drum voice's pages are its drum screen's, SRC FLTR AMP LFO FX) */
#undef SYN_CHAIN
            static const char *const NM[5] = {"ACID", "JUNO", "ELEMENTS", "PLAITS", "drums (their own bus)"};
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
            {   /* (wave 8) the shared effects' eight settings on the FX button's pages only, the master's DUST DUCK there
                 * too; GLO is the settings: no effect, no DJ filter, no swing, no drums' level or sends there */
                uint32_t onglo = 0, nmfx = 0, k;
                for (i = 0; i < NPAGES; i++) {
                    for (k = 0; k < 4u; k++) {
                        uint32_t id = PAGES[i].id[k];
                        if (id == 0xFFu || PAGES[i].scope != SC_GLOBAL)
                            continue;
                        if (PAGES[i].fam == FAM_MFX)
                            seen[id]++;
                        else if (id == G_DTIME || id == G_DFDBK || id == G_DCOLOR || id == G_DMIX || id == G_RSIZE || id == G_RDAMP ||
                                 id == G_CRATE || id == G_CDEPTH || id == G_DUST || id == G_DUCK)
                            onglo++;
                        if (PAGES[i].fam == FAM_GLO && (id == G_FILT || id == G_SWING || id == G_ROLL || id == G_DRLVL ||
                                                        id == G_DRREV || id == G_DRDLY))
                            onglo++;
                    }
                    nmfx += PAGES[i].fam == FAM_MFX;
                    g_glo |= PAGES[i].fam == FAM_GLO && (!strcmp(PAGES[i].title, "DLY") || !strcmp(PAGES[i].title, "REV/CHO") ||
                                                         !strcmp(PAGES[i].title, "MASTER") || !strcmp(PAGES[i].title, "DRUMS"));
                }
                check(nmfx == 3u && !onglo && !g_glo && seen[G_DTIME] == 1 && seen[G_DFDBK] == 1 && seen[G_DCOLOR] == 1 &&
                      seen[G_DMIX] == 1 && seen[G_RSIZE] == 1 && seen[G_RDAMP] == 1 && seen[G_CRATE] == 1 && seen[G_CDEPTH] == 1 &&
                      seen[G_DUST] == 1 && seen[G_DUCK] == 1 && !seen[G_FILT],
                      "FX: DELAY, REV/CHO, MASTER hold every shared effect setting, once; GLO none (no DLY REV/CHO MASTER DRUMS)");
                check(!strcmp(GP[G_DFDBK].label, "FDBK") && !strcmp(GP[G_CRATE].label, "RATE") && !strcmp(GP[G_CDEPTH].label, "DEPTH"),
                      "one spelling: FDBK (was FEEDBK), the chorus's RATE DEPTH (were CRT CDP)");
            }
            go_home(); open_family(FAM_GLO); frame();
            for (i = 0; i < 16u; i++) { encs[panel.enc[EN_SELECT]] = -1; frame(); }
            ui.force = 1; frame(); ppm("page-global");
            check(!strcmp(PAGES[ui.page].title, "GLOBAL") && PAGES[ui.page].id[0] == G_BPM, "GLO: GLOBAL first, BPM on KNOB 1");
        }
        song.playing = 0; song.sel = 0; go_home(); ui.force = 1; frame();
        bpm = song.g[G_BPM];
        SEL(-1); SEL(1);
        check(song.g[G_BPM] == bpm && !vis_on && ui.home + (cur_page()->scope == SC_TRK), "SELECT on HOME: nothing (no type there; never the tempo)");
        tap(B_HOME);
        check(vis_shown() && vis_style < VIS_N, "HOME on HOME (wave 9): its next category, the screensaver (one of the five, at random: 23 Nov)");
        { uint8_t s0 = vis_style; tap(B_SCL); check(vis_shown() && vis_style == (s0 + 1u) % VIS_N, "screensaver, SEL (PAGE): the next one"); }
        tap(B_HOME); check(!vis_on && cur_page()->scope == SC_TRK, "screensaver, HOME: round to HOME");
        vis_style = 0;
        open_family(FAM_GLO); frame();
        encs[panel.enc[EN_K1]] = 3; frame();
        check(song.g[G_BPM] > bpm, "the tempo: GLO > GLOBAL, KNOB 1 BPM");
        song.g[G_BPM] = bpm; go_home(); frame();
        tap(B_LFO);
        check(!strcmp(PT(), "LFO 1") && cur_cat() == CT_LFO1, "LFO opens its first category, LFO 1");
        {   int16_t w0 = trk[0].p[P_LWAVE];
            SEL(1);
            check(!strcmp(PT(), "LFO 1") && trk[0].p[P_LWAVE] == w0 + 1 && song.g[G_BPM] == bpm,
                  "SELECT on LFO 1: its WAVE (the type), not a page, not the tempo");
            trk[0].p[P_LWAVE] = w0; }
        tap(B_SCL);
        check(!strcmp(PT(), "LFO SYNC"), "SEL (PAGE) on LFO 1: its sync page, LFO 1 SYNC");
        ui.force = 1; frame(); ppm("page-lfo-sync");
        tap(B_SCL); check(!strcmp(PT(), "LFO 1"), "SEL again: round to LFO 1");
        tap(B_LFO); check(!strcmp(PT(), "LFO 2"), "LFO again: LFO 2 (the next category)");
        tap(B_SCL); check(!strcmp(PT(), "LFO 2"), "SEL on LFO 2 (one page): nothing");
        tap(B_LFO); check(!strcmp(PT(), "MATRIX"), "LFO again: MATRIX");
        tap(B_LFO); check(!strcmp(PT(), "LFO 1"), "LFO again: round to LFO 1");
        go_home(); tap(B_FX); tap(B_FX);
        check(!strcmp(PT(), "SENDS") && PAGES[ui.page].id[0] == P_DLY && PAGES[ui.page].id[1] == P_REV && PAGES[ui.page].id[2] == P_CHOR,
              "FX, FX: the voice's SENDS (DELAY REVERB CHORUS: its own amounts)");
        ui.force = 1; frames(2); ppm("page-sends");
        check(px_count(0, OP_KNOB_Y, 240, 58, TE_G4) == 0u && px_count(0, OP_KNOB_Y, 240, 58, TE_COL[0]) > 60u,
              "SENDS: its dials in the voice's colour (they are the voice's), no white-grey");
        go_home(); tap(B_EDIT);
        check(!strcmp(PT(), "OSC") && cur_cat() == CT_OSC, "EDIT: OSC, the first category");
        tap(B_SCL); check(!strcmp(PT(), "OSC 2"), "OSC, SEL: OSC 2");
        tap(B_SCL); check(!strcmp(PT(), "OSC 3"), "SEL: OSC 3");
        tap(B_SCL); check(!strcmp(PT(), "OSC 4"), "SEL: OSC 4");
        tap(B_SCL); check(!strcmp(PT(), "OSC"), "SEL: round to OSC");
        tap(B_SCL); tap(B_EDIT);
        check(!strcmp(PT(), "FILT"), "EDIT again (from OSC 2): the next category, FILT, its first page");
        tap(B_SCL);
        check(!strcmp(PT(), "FILT 2") && PAGES[ui.page].id[0] == P_FKEY, "FILT, SEL: FILT 2 (KEY LINK)");
        ui.force = 1; frames(2); ppm("page-filter");
        {   /* (wave 9) the tab row: EDIT's categories OSC FILT, FILT filled in the voice's colour, 2/2 at the right */
            check(px_count(40, 131 + 4, 50, 17, TE_COL[0]) > 200u && px_count(200, 131, 40, 23, TE_COL[0]) > 10u &&
                  nav_tabs_shown == (CT_OSC | CT_FILT << 8), "tabs: OSC FILT (EDIT's), FILT filled, 2/2 at the right");
            tap(B_SCL); ui.force = 1; frames(2);
            check(!strcmp(PT(), "FILT") && px_count(4, 24, 120, 32, TE_COL[0]) > 150u,
                  "FILT: its type large in the voice's colour (LP 24)");
        }
        {   /* the dials: four values, one tall face, in the voice's colour */
            uint32_t lit;
            ui.msg_t = 0; ui.force = 1; frame();
            lit = px_count(2, OP_KNOB_Y + 4, 58, 52, TE_COL[0]);
            check(lit > 30u, "FILT: the dials show CUTOFF, in the voice's colour");
        }
        tap(B_EDIT);
        check(!strcmp(PT(), "OSC"), "EDIT on FILT: round to OSC (never on to ENV: no press changes group)");
        tap(B_SEQ); frames(2);
        check(on_drum_page() && kb_grid && syn_tab == 0u && cur_cat() == CT_TRIG, "SEQ on a sound page (OSC): the GRID, on TRIG (its first category)");
        tap(B_SEQ); frames(2);
        check(on_drum_page() && cur_cat() == CT_PATT, "SEQ again: PATTERN, the pattern's settings (SEQ held: their shortcut)");
        go_home(); frames(2);
        tap(B_ENV);
        check(!strcmp(PT(), "ENV"), "the button still opens its family");
        tap(B_ENV);
        check(!strcmp(PT(), "FILT ENV"), "... and a second tap its next category (the filter's own envelope)");
        press(B_GLO); frames(3);
        SEL(1);
        check(!strcmp(PT(), "FILT ENV") && song.g[G_BPM] == bpm, "SELECT with a layer held (GLO): nothing (wave 8: no tempo there)");
        release(B_GLO); frames(2);
        rec_wait = 1; frame(); SEL(1); rec_wait = 0; frame();
        check(song.g[G_BPM] == bpm, "SELECT on the REC screen: nothing (the tempo in one place, GLO > GLOBAL)");
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
        perf.par[perf.last][1] = 0;
        edges_btn |= BT(B_PLAY); fm1_in.buttons |= BT(B_PLAY);
        encs[panel.enc[EN_K2]] = 2; frame();
        fm1_in.buttons &= ~BT(B_PLAY);
        check(trk[0].p[P_LWAVE] == w0 && perf.par[perf.last][1] > 0 && transport_req,
              "#102: PLAY pressed in PERFORM plays, KNOB 2 is the last pad's only");
        perf.par[perf.last][1] = 0;
        transport_req = 0; song.g[G_DUST] = 0;
        release(B_FX); frames(20);
        song.g[G_BPM] = 120;
    }

    {   /* DRUMS grid (2.4): the keys are the steps of the sound; the sound and the step heard */
        uint32_t i;
        song.playing = 0; song.sel = TRK_DRUM; go_home(); frame();
        dpat_clear(&dpat);
        TDRUM->p[P_SLEN] = 32;
        studio_open(SC_DRUM); drum_lane = 0; drum_cursor = 0; frames(2);
        memset(pad_lit, 0, sizeof pad_lit);
        key(key_of_white(2)); frame();
        check(dt_has(2, 0) && drum_cursor == 2 && pad_lit[0], "grid: white key 3 sets step 3 of the sound (heard), the cursor on it");
        check((keys_lit() >> key_of_white(2)) & 1u, "grid: its key lit");
        key(key_of_white(2));
        check(!dt_mask(2), "grid: the key again clears it");
        key(3);                                                     /* (G#3: wave 10, not drawn: nothing) */
        check(drum_cursor / 16u == 0u, "grid: a black key (wave 10, not drawn): no page");
        tap(B_OCTUP); frames(20);
        check(drum_cursor / 16u == 1u, "grid: OCT+ tapped: the page of steps (17-32)");
        key(key_of_white(0));
        check(dt_has(16, 0), "grid: ... and the white keys are its steps (step 17)");
        ui.msg_t = 0;
        dtest_set(17, 6, LV_HARD, 1); TDRUM->micro[17] = 3;
        tap(B_OCTUP); frames(20);                                   /* (OCT+ on the last page: a page added) */
        check(drum_cursor / 16u == 2u && TDRUM->p[P_SLEN] == 48 && dt_has(33, 6) &&
              dtest_lvl(33, 6) == LV_HARD && TDRUM->micro[33] == 3 && dt_has(32, 0),
              "grid: OCT+ on the last page: a page added, a copy of the last (hits, levels, nudges)");
        ui.force = 1; frame(); ppm("grid-drum-page-add");
        dtest_clear(17); TDRUM->micro[17] = 0; TDRUM->micro[33] = 0;
        TDRUM->p[P_SLEN] = 32; drum_cursor = 16; frame();
        memset(pad_lit, 0, sizeof pad_lit);
        drum_lane = 0; frame();                         /* (wave 9, Ben: "Drum sound pick: SELECT knob") */
        encs[panel.enc[EN_SELECT]] = 3; frame(); frame();
        check(drum_lane == 7 && pad_lit[7], "grid: SELECT a detent from KICK: the next sound in the Rytm order (RIM, key 2), heard while stopped");
        encs[panel.enc[EN_SELECT]] = 1; frame(); encs[panel.enc[EN_SELECT]] = 1; frame();
        check(drum_lane == 2, "grid: SELECT, two more: SNARE 2, then SNARE (along the bottom row)");
        frames(20);
        key(key_of_white(9)); frame();
        check(dt_has(25, 2), "grid: a white key sets the step of the sound picked");
        key(key_of_white(9)); frame();
        check(!dt_has(25, 2), "grid: again: cleared");
        {   /* PHEBUS (21 Nov 2026): the STUTTER lane, after DRUM on the GRID (ui_stut.c, punch.c stl) */
            uint32_t j, kw0 = key_of_white(0), kw12 = key_of_white(12);
            check(stl_new == 0u && !strcmp(STL_LEN[stl_new], "1/4"),
                  "STUTTER (Ben, 21 Nov: \"1/4 is preferably\"): new steps start at 1/4");
            memset(&stl, 0, sizeof stl); stl.len = 16; stl.act = 1; stl_new = 1;
            ui.force = 1; frame();
            for (j = 0; j < 8u && cur_cat() != CT_STUT; j++) tap(B_FX);   /* (wave 14: FX's last category) */
            frames(2);
            check(stl_shown() && song.sel == TRK_DRUM && grid_keys_on() && kb_grid,
                  "STUTTER: FX to its last category, STUT: the lane, the keys its steps");
            ui.force = 1; frame(); ppm("grid-stutter-empty");
            check(px_count(0, OP_BAR_Y + 24, 240, 3, TE_COL[3]) > 100u && px_count(0, OP_BAR_Y + 24, 240, 3, TE_COL[0]) == 0,
                  "STUTTER: the track strip underlines the voice still selected (DR), as on the MIX pages");
            check(px_count(2, 1, 46, 18, TE_G4) > 40u && px_count(48, 0, 4, 20, TE_G4) == 0,
                  "STUTTER: its header tag drawn in the box (MIX, the whole mix's)");
            key(kw12);
            check(stl_lit(12) && stl_code(12) == 1u, "STUTTER: tap step 13: lit, 1/8 (the length for new steps)");
            check(!dt_mask(12), "STUTTER: ... and no drum hit set");
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
            press(B_SAVE); press(B_OCTDN); release(B_OCTDN); release(B_SAVE); frames(2);
            check(stl_code(14) == 0u, "STUTTER: SAVE + OCT-: undo (step 15 back to 1/4)");
            press(B_SAVE); press(B_OCTUP); release(B_OCTUP); release(B_SAVE); frames(2);
            check(stl_code(14) == 2u, "STUTTER: SAVE + OCT+: redo");
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
            key(3);                                      /* (wave 10: a black key, not drawn: nothing) */
            check(stl_pg == 0u, "STUTTER: a black key (not drawn): no page");
            tap(B_OCTUP); frames(20);
            check(stl_pg == 1u, "STUTTER: OCT+ tapped: steps 17-32");
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
            check(stl_shown() && song.sel == 2u, "STUTTER (wave 14): ALGORITHM left: voice 3 selected, the lane stays");
            encs[panel.enc[EN_ALGO]] = 1; frames(2);
            encs[panel.enc[EN_ALGO]] = 1; frames(2);
            check(stl_shown() && song.sel == TRK_DRUM, "STUTTER (wave 14): right, right: DRUM, no fifth voice past it, the lane stays");
            tap(B_SEQ); frames(2);
            check(!stl_shown() && song.sel == TRK_DRUM && drum_keys == DK_PLAY && cur_cat() == CT_TRIG, "STUTTER: SEQ: the drum steps again");
            memset(&stl, 0, sizeof stl); stl.len = 16; stl.act = 1;
            ui.force = 1; frame();
        }
        go_home(); frame();
        memset(pad_lit, 0, sizeof pad_lit);
        press(B_SEQ); frames(3);
        encs[panel.enc[EN_K1]] = 2; frames(2);
        check(!pad_lit[2] && !pad_lit[1], "SEQ layer, drum track: KNOB 1 (the RETRIG rate) picks or plays no sound");
        release(B_SEQ); frames(2);
        drt_live = 10;
        song.sel = 1; go_home(); frame();
        trk[1].p[P_SDIV] = 5;
        tap(B_SEQ); tap(B_SEQ); frames(16);              /* (wave 10: DIV on SEQ's PATTERN page, not the hold) */
        for (i = 0; i < 5u; i++) { encs[panel.enc[EN_K2]] = 1; frame(); }
        check(trk[1].p[P_SDIV] == (int16_t)(NDIV_STEP - 1u), "SEQ > PATTERN: KNOB 2 DIV goes on to 1/2, 1BAR, 2BAR");
        go_home(); frames(2);
        trk[1].p[P_SDIV] = 2; TDRUM->p[P_SLEN] = 16; dpat_clear(&dpat);
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
            dtest_clear(j);
            if (!(j % 4u)) dtest_set(j, 0, LV_NORM, 0);
            if ((j % 8u) == 4u) dtest_set(j, 3, LV_NORM, 0);
            dtest_set(j, 4, (j & 1u) ? LV_SOFT : LV_NORM, 0);
            for (k = 0; k < 4u; k++) trk[k].seq_active = 1;
        }
        go_home(); frames(20);                      /* (the knobs quiet after the layers above) */
        transport_req = 1; frames(30);
        check(!vis_shown(), "visualiser: closed on the TRACKS screen");
        encs[panel.enc[EN_SELECT]] = 1; frames(2);
        check(!vis_shown() && !vis_on, "visualiser: SELECT on HOME: nothing (wave 9: SELECT is a type, HOME has none)");
        tap(B_HOME); frames(2);
        check(vis_shown() && vis_on && vis_style < VIS_N, "visualiser: HOME on HOME opens the screensaver (HOME's next category; which: at random, 23 Nov)");
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
            if (st + 1u < VIS_N) tap(B_SCL);           /* (wave 9: SEL, the PAGE button, the next screensaver) */
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
        check(vis_style == VIS_N - 1u && vis_shown(), "visualiser: SEL walked them, the last BEACH");
        check((lights_word() >> 17 & 15u) == VIS_N - 1u, "visualiser: kept in the settings word");
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
        check(ui.layer == LY_GLO, "visualiser: a layer held shows its screen");
        release(B_GLO); frames(4);
        check(vis_shown(), "visualiser: ... and it comes back");
        tap(B_HOME); frames(2);
        check(!vis_shown() && !vis_on, "visualiser: HOME closes it");
        encs[panel.enc[EN_SELECT]] = 1; frames(2);
        tap(B_LFO); frames(2);
        check(!vis_shown() && !vis_on && !strcmp(PAGES[ui.page].title, "LFO 1"), "visualiser: a page button closes it and opens its page");
        transport_req = 2; frames(4);
        for (j = 0; j < NTRK; j++) steps_clear(&trk[j]);
        vis_style = 0; song.sel = 0; go_home(); frames(20);
        vis_rare_checks();
        art_checks();
    }

    {   /* 2.6, after Felucca 1.4 / 1.5 (Leo Kuroshita): STEP LEN, SEQ VEL, the drum mutes (EDIT + a black key), MIDI
         * LEARN (GLO + F#3, a knob, a CC; LEARN CLEAR in the menu), the INSERT pages (the drum track too), ANALOG's FILTER */
        uint32_t j, w, ins_d = 0, flt_a = 0, flt_o = 0;
        #define K(n, v) do { encs[panel.enc[n]] = (v); frames(5); } while (0)
        #define WALK(fa_, ti_, hit_) do { if (chain_goto(fa_, ti_)) hit_ = 1; } while (0)   /* (wave 9: its button, then SEL) */
        song.playing = 0;
        song.sel = 0; steps_clear(&trk[0]); trk[0].p[P_SLEN] = 16; go_home(); frames(16);
        trk[0].step[4].n = 1; trk[0].step[4].note[0] = 60; trk[0].step[4].time = ST_NOTE; trk[0].step[4].vel = 100;   /* step 5
                                                         * (held SEQ set it before PHEBUS took the step keys off) */
        tap(B_SEQ); frames(2);                          /* (wave 4: the GRID at TRIG, as the drum screen's SEQ) */
        check(on_drum_page() && kb_grid && syn_tab == 0u, "SEQ from HOME: the GRID, on TRIG (the step's own values)");
        fm1_in.notes = 1u << key_of_white(4); frame();     /* LEN on the GRID (a step held, KNOB 2), as STEP's KNOB 3 was */
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
        press(B_SAVE); frames(10);
        edges_btn |= BT(B_OCTDN); fm1_in.buttons |= BT(B_OCTDN); frame(); fm1_in.buttons &= ~BT(B_OCTDN); frame();
        release(B_SAVE); frames(16);
        check(note_len(&trk[0], 4) == 1u && trk[0].step[5].n == 0u && trk[0].step[4].n == 1u,
              "SAVE + OCT-: the LEN turns of that hold undone at once (the steps as before the step was held)");
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
            check(!strcmp(ui.msg, "MIDI PLAYS VOICE 2"), "REC on voice 1, a MIDI note on CH 2: \"MIDI PLAYS VOICE 2\" (not recorded)");
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
            check(trk[1].eng_req == BANK[x0].e && trk[1].preset == bank_pi[x0] && !strncmp(ui.msg, "VOICE 2 PROG 0", 14),
                  "MIDI Program Change 0: track 2 takes the first sound of its PRESETS list (VOICE 2 PROG 0)");
            pc_req[3] = 3;                                /* PC 2 on the drum channel: kit 2 (an unchanged kit: at once) */
            frame();
            check(pkit.base == 2u && !ui.ask, "MIDI Program Change on the drum channel: kit 2, all 16 slots (wave 12)");
            pc_req[1] = (uint8_t)(tot + 1u);              /* past the end: nothing */
            k = trk[1].eng_req;
            frame();
            check(trk[1].eng_req == k && trk[1].preset == bank_pi[x0], "MIDI Program Change past the list: ignored (no wrap)");
            kit_set((uint32_t)(k0));
            set_engine_of(&trk[1], e0); apply_preset_to(&trk[1], p0); trk[1].engine = trk[1].eng_req;
            frames(4);
            press(B_FX); frames(10);
            fm1_in.notes = 1u << key_of_white(PF_ECHO); frame();   /* (wave 6: the ECHO pad is the echo throw) */
            ui.force = 1; frame(); ppm("layer-fx-throw");
            check(perf.thr[1] == 32767 && thr_dly > 0, "PERFORM: the ECHO pad throws into the delay while held");
            fm1_in.notes = 0; frame();
            release(B_FX); frames(4);
            check(perf.thr[1] == 0 && song.octave == 0, "  let go: no THROW, the octave untouched");
        }
        steps_clear(&trk[0]); go_home(); frames(16);
        {   /* the GRID: two steps held + KNOB 3 (VEL): both one up (the turn accelerated once, not once a step; held SEQ
             * had this test until PHEBUS took its step keys off) */
            uint32_t v0, v1;
            song.sel = 0; trk[0].p[P_SLEN] = 16; go_home(); frames(16);
            studio_open(SC_DRUM); frames(2);
            key(key_of_white(0)); key(key_of_white(4));
            fm1_in.notes = 1u << key_of_white(0) | 1u << key_of_white(4); frame();
            v0 = step_vel_shown(&trk[0].step[0]), v1 = step_vel_shown(&trk[0].step[4]);
            K(EN_K3, 1);
            check(step_on(&trk[0].step[0]) && trk[0].step[0].vel == v0 + 1u && trk[0].step[4].vel == v1 + 1u,
                  "GRID: two steps held + KNOB 3: both velocities one up");
            fm1_in.notes = 0; frame();
            steps_clear(&trk[0]); go_home(); frames(16);
        }

        song.sel = TRK_DRUM; go_home(); frames(16);
        dtest_set(0, 0, LV_NORM, 0);
        press(B_EDIT); frames(10);
        key(1);                                           /* a black key: nothing */
        key(0);                                           /* (wave 10) EDIT erases nothing */
        check(er_mute == 0u && drum_mute == 0u && dt_has(0, 0), "EDIT + a key (drum track, wave 10): nothing erased or muted");
        release(B_EDIT); frames(2);
        fm1_in.notes |= 1u << key_of_lane(0); frames(2);   /* the kick's key (the drum grid: key 4) held + OCT-: erased */
        edges_btn |= BT(B_OCTDN); fm1_in.buttons |= BT(B_OCTDN); frame(); fm1_in.buttons &= ~BT(B_OCTDN); frame();
        fm1_in.notes &= ~(1u << key_of_lane(0)); frames(2);
        check(!dt_has(0, 0) && drum_mute == 0u, "the kick's key held + OCT-: its steps erased (not muted)");
        drum_mute = 0;
        dtest_clear(0);
        drum_open(FAM_EDIT); dpg_go(DPG_FX, 0); frame();   /* (wave 8: the drum voice's own pages, its drum screen's FX tab) */
        ins_d = on_drum_page() && !strcmp(PAGES[dbus_page(dpg_half())].title, "INSERT");
        check(ins_d, "the drum track: the INSERT page on its FX tab");
        go_home(); frames(16);

        {   /* (wave 4) the voice filter's TYPE on every engine's FILT page (ANALOG's TYPE page is gone with ANALOG) */
            static const uint8_t EE[4] = {ENGI_ACID, ENGI_JUNO, ENGI_ELEM, ENGI_PRISM};
            uint32_t e;
            song.sel = 0;
            for (flt_a = 1, e = 0; e < 4u; e++) {
                set_engine_of(&trk[0], EE[e]); trk[0].engine = trk[0].eng_req;
                for (flt_o = 0, j = 0; j < NPAGES; j++)
                    flt_o |= PAGES[j].sel == P_FTYPE && PAGES[j].id[0] == P_FCUT && PAGES[j].id[3] == P_FDRV && PAGES[j].fam == FAM_FLT &&
                             page_shown(&PAGES[j]);
                flt_a &= flt_o;
            }
            check(flt_a, "FILT: CUTOFF RESO ENV DRIVE on every engine, its TYPE on SELECT (one voice filter)");
            go_home(); frame();
        }
        set_engine_of(&trk[0], TRK_DEF[0][0]); apply_preset_to(&trk[0], TRK_DEF[0][1]); trk[0].engine = trk[0].eng_req;
        go_home(); frames(16);

        for (w = 0; w < ML_N; w++) ml_tab[w] = 0;
        check(chain_goto(FAM_GLO, "MIDI 3") && PAGES[ui.page].id[1] == V_LEARN, "GLO > MIDI 3: KNOB 2 LEARN (wave 9: was GLO + F#3)");
        frames(16); K(EN_K2, 1);
        check(mlu.on == 1u && !mlu.pick, "GLO > MIDI 3, KNOB 2 right: MIDI LEARN on");
        go_home(); tap(B_FX); frame();                    /* FX: INS, FX 1; SELECT its TYPE */
        frames(16);
        ui.force = 1; frame(); ppm("header-learn");
        K(EN_SELECT, 1);
        check(mlu.pick && mlu.id == P_X1TYPE && mlu.trk == 0u && ml_arm, "LEARN: SELECT turned picks its parameter (FX 1's TYPE)");
        midi_cc(&trk[0], 74, 10); frame();
        check(ml_count() == 1u && (ml_tab[0] & 127u) == 74u && (ml_tab[0] >> 9) == ml_code(P_X1TYPE) && !mlu.pick,
              "LEARN: a CC sent: CC 74 = track 1 FX 1 TYPE");
        go_home(); frames(16);
        press(B_GLO); frames(10); key(1); key(1); release(B_GLO); frames(4);
        check(mlu.on && !trk[0].p[P_MUTE], "GLO + F#3 twice (wave 13: voice 1 muted, then not): MIDI LEARN untouched");
        chain_goto(FAM_GLO, "MIDI 3"); frames(16); K(EN_K2, -1);
        check(!mlu.on, "GLO > MIDI 3, KNOB 2 left: LEARN off");
        go_home(); frames(4);
        midi_cc(&trk[1], 74, 127);
        check(trk[0].p[P_X1TYPE] == TP[P_X1TYPE].max, "CC 74 (any channel): track 1's FX 1 TYPE to its top");
        xs_defaults(trk[0].p, 0, XT_OFF);
        chain_goto(FAM_GLO, "MIDI 3"); frames(4);
        j = ml_count();
        encs[panel.enc[EN_K3]] = 1; frames(4);
        check(j && ml_count() == j, "U5: MIDI 3 > CLEAR right: it asks first (nothing cleared yet)");
        press(B_SAVE); frames(90); release(B_SAVE); frames(2);
        check(ml_count() == 0u, "U5: MIDI 3 > CLEAR, SAVE held: nothing learned any more");
        go_home(); frames(16);
        {   /* GLO > MIDI: CH1 CH2 CH3 OTHER (2.6) */
            uint32_t hit = 0;
            WALK(FAM_GLO, "MIDI", hit);
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
                check(song.g[G_CH2] == 4 && !strcmp(ui.msg, "CH 3 = VOICE 3"), "GLO > MIDI: CH2 turned onto track 3's channel: on to 4, \"CH 3 = VOICE 3\"");
                K(EN_K2, -1);                             /* back: 3 taken, on to 2 */
                check(song.g[G_CH2] == 2, "  turned back: over 3 again, to 2");
                K(EN_K4, -7);                             /* DR CH 10 -> 3: track 3's, then 2, 1 (taken): OFF */
                check(song.g[G_DRCH] == 0, "  the drums turned down onto 3, 2, 1 (all taken): OFF");
                song.g[G_DRCH] = 10, frame();
                song.g[G_ROUTE] = 6;                      /* IN = CH 5 */
                go_home(); frame();
                chain_goto(FAM_GLO, "MIDI");
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
    {   /* PHEBUS wave 6: PERFORM (ui_perf.c, punch.c perf_*). Ben: "the MPC's feel for pad like fx play and stuttering";
         * pickers "A: pad map, trimmed set", latch, the knobs on the last pad, the STUTTER lane kept and recorded into */
        uint32_t p, k, x, over0, kn;
        go_home(); song.sel = 0; song.playing = 0; ui.force = 1; frames(3);
        perf.held = perf.lat = 0; perf_init();
        /* in: FX held a while, nothing touched, let go: PERFORM stays (as GLO's mixer); FX again: out */
        press(B_FX); frames(50); release(B_FX); frames(2);
        check(ly_lock == LY_FX && ui.layer == LY_FX, "PERFORM: FX held 0.7 s untouched and let go: it stays open (as the mixer)");
        ui.force = 1; frame(); ppm("perform-lock");
        check(px_count(170, 4, 70, 20, C_WHITE) > 40u, "PERFORM: LOCK shown while it stays open");
        key(key_of_white(3));
        check(!trk[0].v[0].active && !perf.held, "PERFORM open: a key is a pad (1/8), no note; up: off");
        tap(B_FX); frames(2);
        check(ly_lock == LY_PLAY && ui.layer == LY_PLAY && cur_page()->scope == SC_TRK, "PERFORM: FX again: out, and only that (no FX page)");
        fm1_in.notes = 1u << key_of_white(4); frame(); frame();
        check(perf.held == 0u && trk[0].v[0].active, "out of PERFORM the keys play notes again (it never keeps them)");
        fm1_in.notes = 0; frames(4);
        press(B_FX); frames(3); key(key_of_white(2)); release(B_FX); frames(2);
        check(ly_lock == LY_PLAY && ui.layer == LY_PLAY && !perf.held, "FX held + a pad, let go: momentary, nothing stays");
        /* the screen: held white, latched with a dot, the lane's grey with a bar */
        press(B_FX); frames(12);
        fm1_in.notes = 1u << key_of_white(2); frame(); frame();
        ui.force = 1; frame(); ppm("perform-held");
        check(px_count(PF_PAD_X(2), PF_Y + PF_PAD_Y(2), PF_PAD_W, PF_PAD_H, C_WHITE) > 800u, "PERFORM: the 1/4 pad held: drawn white (the whole mix: no voice colour)");
        check(px_count(0, 67, 240, 88, TE_COL[0]) + px_count(0, 67, 240, 88, TE_COL[1]) + px_count(0, 67, 240, 88, TE_COL[2]) +
              px_count(0, 67, 240, 88, TE_COL[3]) == 0u, "PERFORM: no voice colour on the effect pads (the colour rule: the whole mix)");
        /* latch: OCT+ while it is held */
        fm1_in.buttons |= BT(B_OCTUP); edges_btn |= BT(B_OCTUP); frame(); fm1_in.buttons &= ~BT(B_OCTUP); frame();
        fm1_in.notes = 0; frame();
        check(perf.lat == 1u << PF_S4 && !perf.held, "LATCH: OCT+ while the 1/4 is held: it stays with the key up");
        /* latch: a pad pressed with OCT+ held */
        fm1_in.buttons |= BT(B_OCTUP); edges_btn |= BT(B_OCTUP); frame();
        fm1_in.notes = 1u << key_of_white(8); frame(); fm1_in.notes = 0; frame();
        fm1_in.buttons &= ~BT(B_OCTUP); frame();
        check(perf.lat == (1u << PF_S4 | 1u << PF_ISOL), "LATCH: OCT+ held + the ISO LO pad: latched too (2 latched)");
        stl.ev[0] = (pev_t){PF_PHASER + 1u, 0, 16, 0};  /* (the lane plays PHASER: as a recorded pad shows) */
        song.playing = 1; transport_req = 1; frames(3);
        ui.force = 1; frame(); ppm("perform-latched");
        check(perf.lane == 1u << PF_PHASER, "PERFORM: a pad the lane plays (recorded) shows as playing");
        check(px_count(PF_PAD_X(PF_ISOL) + PF_PAD_W - 4, PF_Y + PF_PAD_Y(PF_ISOL) + 1, 3, 3, C_WHITE) == 9u,   /* (wave 7: the frame every screen has) */ "LATCH: the latched ISO LO pad shows its dot");
        /* the knobs: the last pad's */
        kn = perf.par[PF_ISOL][0];
        encs[panel.enc[EN_K1]] = -4; frame();
        check(perf.par[PF_ISOL][0] < (int8_t)kn && perf.last == PF_ISOL && perf.par[PF_ISOH][0] == perf.par[PF_ISOL][0],
              "knobs: KNOB 1 turns the last pad pressed (ISO LO: LOW X, the three ISO pads' one crossover)");
        fm1_in.notes = 1u << key_of_white(11); frame(); fm1_in.notes = 0; frame();
        kn = perf.par[PF_MOTION][1];
        encs[panel.enc[EN_K2]] = 3; frame();
        check(perf.last == PF_MOTION && perf.par[PF_MOTION][1] > (int8_t)kn && perf.par[PF_ISOL][1] == PFD[PF_ISOL].k[1].def,
              "knobs: then the MOTION pad: KNOB 2 its DEPTH, ISO's untouched");
        ui.force = 1; frame(); ppm("perform-knobs-motion");
        /* exit with latches on: they stay (hands free to play), the header says FX 2; STOP lets them go */
        release(B_FX); frames(2);
        go_home(); ui.force = 1; frame(); ppm("home-fx-latched");
        check(pf_count(perf.lat) == 2u && px_count(OP_BPM_R + 18, 2, 34, 16, C_WHITE) > 200u, "out of PERFORM, latched pads play on: the header says PF2, in white (V5: the beat lights' place)");
        press(B_FX); frames(12);
        fm1_in.buttons |= BT(B_OCTDN); edges_btn |= BT(B_OCTDN); frame(); fm1_in.buttons &= ~BT(B_OCTDN); frame();
        check(!perf.lat, "LATCH: OCT- in PERFORM: every latch off");
        fm1_in.buttons |= BT(B_OCTUP); edges_btn |= BT(B_OCTUP); frame();
        fm1_in.notes = 1u << key_of_white(9); frame(); fm1_in.notes = 0; frame(); fm1_in.buttons &= ~BT(B_OCTUP); frame();
        release(B_FX); frames(2);
        check(perf.lat == 1u << PF_ISOM, "(ISO MID latched, PERFORM left)");
        tap(B_PLAY); frames(3);
        check(!song.playing && !perf.lat, "STOP: the latches go");
        memset(stl.ev, 0, sizeof stl.ev);
        /* every pad's dials: no value past its column (each knob at its least, middle, most) */
        over0 = dial_over;
        press(B_FX); frames(12);
        for (p = 0; p < PF_NPAD; p++) {
            perf.last = (uint8_t)p;
            for (x = 0; x < 3u; x++) {
                for (k = 0; k < 4u; k++) {
                    const pk_t *d = &PFD[p].k[k];
                    if (d->kind == KK_GLO)
                        song.g[d->gid] = (int16_t)(x == 0u ? GP[d->gid].min : x == 1u ? (GP[d->gid].min + GP[d->gid].max) / 2 : GP[d->gid].max);
                    else
                        perf.par[p][k] = (int8_t)(x == 0u ? d->lo : x == 1u ? (d->lo + d->hi) / 2 : d->hi);
                }
                ui.force = 1; frame();
            }
        }
        {
            char b[96];
            snprintf(b, sizeof b, "PERFORM: no knob value past its 40 px column (%d pads, each knob at its least, middle, most)", (int)PF_NPAD);
            check(dial_over == over0, b);
        }
        release(B_FX); frames(2);
        if (ly_lock == LY_FX) { tap(B_FX); frames(2); }   /* (held that long untouched: it stayed open) */
        perf_init();
        song.g[G_DTIME] = GP[G_DTIME].def, song.g[G_RSIZE] = GP[G_RSIZE].def, song.g[G_RDAMP] = GP[G_RDAMP].def;
        {   /* PERFORM in columns (SPEC 23 Nov 2026, 22:07; Ben: "Group them by type into columns as much as possible. With the
             * exception of the black keys which make sense being a row ... keep all the on screen labelling on one row ... Then
             * the 16 white keys arranged in columns below"): the pads keep their keys; the screen draws white key n in column
             * (n-1)/4, row (n-1)%4 under a heading of its type; the black keys one row in key order */
            static const char *const PADS[27] = {"1/1", "1/2", "1/4", "1/8", "1/16", "HALF", "ECHO", "VERB", "ISO LO", "ISO MID",
                                                 "ISO HI", "MOTION", "CRUSH", "PHASER", "JET", "VOWEL", "V1", "V2", "V3", "KICKS",
                                                 "TOPS", "SH+CL", "HT+SN", "REVERSE", "STOP", "SWP DN", "SWP UP"};
            static const char *const HEADS[4] = {"REPEAT", "TIME", "FILTER", "COLOUR"};
            uint32_t n, c, ok, ok2, ok3;
            int32_t bx[11], by[11], bw[11];
            /* the keys: every white key and every black key plays the pad it always has (no pad moved) */
            for (n = 0, ok = 1; n < 16u; n++)
                ok &= punch_fx_of_key(key_of_white(n)) == (int32_t)n && punch_key_of_fx(n) == key_of_white(n) && !strcmp(PFD[n].pad, PADS[n]);
            for (n = 0; n < 11u; n++)
                ok &= punch_fx_of_key(PUNCH_BLACK[n]) == (int32_t)(16u + n) && punch_key_of_fx(16u + n) == PUNCH_BLACK[n] &&
                      !strcmp(PFD[16u + n].pad, PADS[16u + n]);
            check(ok, "PERFORM in columns: every white key (1/1 on key 1 .. VOWEL on key 16) and black key keeps its pad");
            press(B_FX); frames(12);
            for (n = 0, ok = 1, ok2 = 1; n < 27u; n++) {   /* held from the keyboard: that pad, lit, drawn in its own place */
                uint32_t k = n < 16u ? key_of_white(n) : PUNCH_BLACK[n - 16u];
                fm1_in.notes = 1u << k; frames(2);
                ok &= perf.held == PFB(n) && led_lv[14u + k] == LL_LIT;
                ui.force = 1; frame();
                if (n < 16u) {
                    uint32_t cell, in = px_count((uint32_t)PF_PAD_X(n), PF_Y + (uint32_t)PF_PAD_Y(n), PF_PAD_W, PF_PAD_H, C_WHITE);
                    ok2 &= PF_PAD_X(n) == 2 + (int32_t)(n / 4u) * 60 && PF_PAD_Y(n) == 33 + (int32_t)(n % 4u) * 19 && in > 700u;
                    for (cell = 0; cell < 16u; cell++)   /* (no other cell filled white) */
                        if (cell != n)
                            ok2 &= px_count((uint32_t)PF_PAD_X(cell), PF_Y + (uint32_t)PF_PAD_Y(cell), PF_PAD_W, PF_PAD_H, C_WHITE) < 60u;
                    if (n == 9u) ppm("perform-held-iso-mid");
                }
                fm1_in.notes = 0; frames(2);
                ok &= !perf.held;
            }
            check(ok, "PERFORM in columns: each of the 27 keys held plays its pad and lights its key; let go, off");
            check(ok2, "PERFORM in columns: white key n's pad drawn in column (n-1)/4, row (n-1)%4, held white there only");
            /* the black keys: one line, key order, apart, on the screen, their names inside */
            for (n = 0, ok = 1; n < 11u; n++) {
                pf_chip_box(n, &bx[n], &by[n], &bw[n]);
                ok &= by[n] == by[0] && bx[n] >= 0 && bx[n] + bw[n] <= 239 && text_w(&FONT_S, PF_BK_LAB[n]) - 1 <= bw[n] - 2;
                if (n)
                    ok &= bx[n] >= bx[n - 1] + bw[n - 1] + 2;
            }
            ok &= bx[3] - bx[2] - bw[2] > bx[4] - bx[3] - bw[3] && bx[7] - bx[6] - bw[6] > bx[8] - bx[7] - bw[7];
            ok &= PF_Y + PF_BK_Y >= 40 && PF_BK_Y + PF_BK_H <= PF_HEAD_Y;   /* (under the caption, over the headings) */
            check(ok, "PERFORM in columns: the 11 black keys one row in key order, apart, the three groups wider apart, inside 240 px");
            fm1_in.notes = 1u << key_of_white(2); frames(2); fm1_in.notes = 0; frames(2);   /* (1/4 the last pad) */
            ui.force = 1; frame(); ppm("perform-idle");
            {   /* each name and line drawn inside its chip, nothing between (no frame: the last pad a white key's) */
                uint32_t x, y, lit = 0, out = 0;
                for (y = PF_Y + PF_BK_Y - 1u; y < PF_Y + PF_BK_Y + PF_BK_H + 1u; y++)
                    for (x = 0; x < 240u; x++) {
                        int inside = 0;
                        if (swap16(screen[y * 240u + x]) == C_BLACK)
                            continue;
                        lit++;
                        for (n = 0; n < 11u; n++)
                            inside |= (int32_t)x >= bx[n] && (int32_t)x < bx[n] + bw[n] && (int32_t)y >= PF_Y + by[n] &&
                                      (int32_t)y < PF_Y + by[n] + PF_BK_H;
                        out += !inside;
                    }
                ok = lit > 300u && out == 0u;
            }
            check(ok, "PERFORM in columns: the black row's names and lines all inside their chips (none touching)");
            for (n = 0, ok = 1; n < 11u; n++) {   /* the voices' colours on 1 2 3 only, teal on the drum mutes only */
                for (c = 0; c < NTRK; c++) {
                    uint32_t px = px_count((uint32_t)bx[n], PF_Y + (uint32_t)by[n], (uint32_t)bw[n], PF_BK_H, TE_COL[c]);
                    uint32_t want = n < 3u ? c == n : n < 7u ? c == TRK_DRUM : 0u;
                    ok &= want ? px > 10u : px == 0u;
                }
            }
            for (c = 0; c < NTRK; c++)
                ok &= px_count(0, PF_Y + PF_HEAD_Y, 240, SG_KNOB_Y - PF_Y - PF_HEAD_Y, TE_COL[c]) == 0u;
            check(ok, "PERFORM in columns: purple, blue, coral on 1 2 3 only; teal on KIK TOP S+C H+S only; none on the pads");
            for (c = 0, ok = 1; c < 4u; c++)   /* the columns' headings: REPEAT TIME FILTER COLOUR, quiet grey, over their columns */
                ok &= !strcmp(PF_HEAD[c], HEADS[c]) && text_w(&FONT_S, PF_HEAD[c]) - 1 <= PF_PAD_W - 2 &&
                      px_count((uint32_t)PF_COL_X(c), PF_Y + PF_HEAD_Y, PF_PAD_W, 14, TE_G3) > 30u &&
                      PF_HEAD_Y + 12 <= PF_PAD_Y(0) && PF_PAD_Y(3) + PF_PAD_H <= (int32_t)(SG_KNOB_Y - PF_Y);
            check(ok, "PERFORM in columns: a heading over each column (REPEAT TIME FILTER COLOUR), between the black row and the pads");
            for (n = 0, ok = 1; n < 16u; n++)   /* every pad's name inside its cell, (review V5) 2 px spare each side */
                ok &= (text_w(&FONT_S, PFD[n].pad) - 1 <= PF_PAD_W - 4 || cv_text_tight(0, 0, PFD[n].pad, 0, 0) - 1 <= PF_PAD_W - 4);
            for (c = 0; c < 3u; c++)
                ok &= PF_COL_X(c + 1u) - PF_COL_X(c) - PF_PAD_W >= 3;   /* (the four types as four blocks: gutters wider than rows) */
            ok &= PF_COL_X(0) >= 2 && PF_COL_X(3) + PF_PAD_W <= 238 && PF_PAD_Y(1) - PF_PAD_Y(0) - PF_PAD_H < PF_COL_X(1) - PF_PAD_W;
            check(ok, "PERFORM in columns: no pad's name past its cell (ISO MID, PHASER, MOTION), 2 px clear of the screen's edges (V5)");
            /* two latched and a mute latched: each in its own cell, the dot; the last pressed framed */
            fm1_in.buttons |= BT(B_OCTUP); edges_btn |= BT(B_OCTUP); frame();
            fm1_in.notes = 1u << key_of_white(1); frame(); fm1_in.notes = 0; frame();
            fm1_in.notes = 1u << key_of_white(11); frame(); fm1_in.notes = 0; frame();
            fm1_in.notes = 1u << PUNCH_BLACK[3]; frame(); fm1_in.notes = 0; frame();
            fm1_in.buttons &= ~BT(B_OCTUP); frame();
            fm1_in.notes = 1u << key_of_white(13); frame(); fm1_in.notes = 0; frame();   /* PHASER pressed last, let go */
            ui.force = 1; frame(); ppm("perform-latched-mute");
            ok = perf.lat == (PFB(PF_S2) | PFB(PF_MOTION) | PFB(PF_MKICK)) && perf.last == PF_PHASER;
            ok3 = ok;
            for (n = 0; n < 16u; n++) {
                uint32_t dot = px_count((uint32_t)(PF_PAD_X(n) + PF_PAD_W - 4), PF_Y + (uint32_t)PF_PAD_Y(n) + 1u, 3, 3, C_WHITE);
                ok &= (n == PF_S2 || n == PF_MOTION) ? dot == 9u : dot < 9u;
            }
            ok &= px_count((uint32_t)(bx[3] + bw[3] - 4), PF_Y + (uint32_t)by[3] + PF_BK_H - 3u, 3, 2, C_WHITE) == 6u &&
                  px_count((uint32_t)bx[3], PF_Y + (uint32_t)by[3], (uint32_t)bw[3], PF_BK_H, TE_DIM[TRK_DRUM]) > 100u;
            check(ok, "PERFORM in columns: 1/2 and MOTION latched: their dots in their cells; KIK latched: its chip filled, its dot");
            ok3 &= px_count((uint32_t)PF_PAD_X(PF_PHASER), PF_Y + (uint32_t)PF_PAD_Y(PF_PHASER), PF_PAD_W, 1, TE_G3) == PF_PAD_W &&
                   px_count((uint32_t)PF_PAD_X(PF_S1), PF_Y + (uint32_t)PF_PAD_Y(PF_S1), PF_PAD_W, 1, TE_G3) < 10u;
            check(ok3, "PERFORM in columns: the last pad pressed (PHASER, column 4 row 2) framed, the others not");
            perf.last = PF_REV; ui.force = 1; frame();
            check(px_count((uint32_t)bx[7] - 1u, PF_Y + (uint32_t)by[7] - 1u, (uint32_t)bw[7] + 2u, 1, TE_G3) == (uint32_t)bw[7] + 2u,
                  "PERFORM in columns: REV the last pressed: framed round its chip");
            perf_unlatch(); perf.last = PF_S1;
            release(B_FX); frames(2);
            if (ly_lock == LY_FX) { tap(B_FX); frames(2); }
        }
        /* the STUTTER lane: the recorded pads drawn as spans, erased with a step held + OCT-, cleared with KNOB 3 twice */
        song.sel = TRK_DRUM; studio_open(SC_DRUM); frames(2);
        stl_view = 1, stl_pg = 0; ui.force = 1; frames(2);
        check(stl_shown(), "(the STUTTER lane on the GRID)");
        memset(&stl, 0, sizeof stl); stl.len = 16; stl.act = 1;
        stl.ev[0] = (pev_t){PF_S8 + 1u, 4, 2, 0};
        stl.ev[1] = (pev_t){PF_SWDN + 1u, 8, 6, 0};
        stl.ev[2] = (pev_t){PF_CRUSH + 1u, 12, 3, 0};
        stl.ev[3] = (pev_t){PF_MTOPS + 1u, 0, 16, 0};
        lane_set_ui(12, 0), lane_set_ui(13, 0);
        ui.force = 1; frame(); ppm("stut-fx");
        check(px_count(4 + 4 * 29, 22 + 82, 2 * 29 - 2, 12, TE_G2) > 300u && px_count(4, 22 + 82, 4 * 29 - 2, 12, TE_G2) == 0u,
              "V7: STUTTER lane: a recorded pad drawn under its steps' columns, the same rows of 8 (1/8 on 5-6: row 1, columns 5-6)");
        check(pev_count() == 4u && px_count(120, OP_KNOB_Y + 26, 54, 26, TE_G4) > 20u,
              "STUTTER lane: KNOB 3 says how many are recorded (FX 4)");
        fm1_in.notes |= 1u << key_of_white(9); frame();
        ui.force = 1; frame(); ppm("stut-fx-held");
        fm1_in.buttons |= BT(B_OCTDN); edges_btn |= BT(B_OCTDN); frame(); fm1_in.buttons &= ~BT(B_OCTDN); frame();
        fm1_in.notes &= ~(1u << key_of_white(9)); frame();
        check(!stl.ev[1].fx && !stl.ev[3].fx && stl.ev[0].fx && stl.ev[2].fx, "STUTTER lane: step 10 held + OCT-: the pads over it erased (SW DN, TOPS), the rest kept");
        check(!stl_lit(9), "STUTTER lane: ... and step 10, lit by that press, unlit again (the press was for the pads)");
        press(B_SAVE); press(B_OCTDN); release(B_OCTDN); release(B_SAVE); frames(2);
        check(stl.ev[1].fx && stl.ev[3].fx, "STUTTER lane: SAVE + OCT-: the erase undone");
        frames(20);                                      /* (the knobs quiet a moment after a layer, #39) */
        encs[panel.enc[EN_K3]] = 1; frame();
        check(stl.ev[0].fx && !strcmp(ui.msg, "HOLD SAVE: CLEAR PADS"), "STUTTER lane: KNOB 3 turned: it asks (HOLD SAVE: CLEAR PADS)");
        frames(20); encs[panel.enc[EN_K3]] = 1; frame();
        check(stl.ev[0].fx, "STUTTER lane: KNOB 3 again (wave 8: no second turn): nothing cleared");
        ui.force = 1; frame(); ppm("stut-ask");
        {
            uint32_t n = 0, r0 = song.rec;
            press(B_SAVE); frames(90); release(B_SAVE);
            for (k = 0; k < PEV_N; k++) n += stl.ev[k].fx != 0u;
            check(n == 0u && stl_lit(12) && song.rec == r0, "STUTTER lane: SAVE held until the ring fills: every recorded pad cleared, the lit steps kept");
        }
        memset(&stl, 0, sizeof stl); stl.len = 16; stl.act = 1;
        stl_view = 0;
        song.playing = 0; transport_req = 2; frames(2);
        go_home(); song.sel = 0; frames(2);
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
         * SEQ -> TRIG -> back */
        static const char *const GONE[9] = {"GEN", "GEN 2", "ACID GEN", "ACID GEN 2", "HARMONY", "DRUM GEN", "DRUM GEN 2", "PLAY", "SONG"};
        uint32_t k, j, none = 1;
        for (k = 0; k < NPAGES; k++)
            for (j = 0; j < 9u; j++)
                none &= strcmp(PAGES[k].title, GONE[j]) != 0;
        check(none, "no generator, PLAY or SONG page left");
        go_home(); song.playing = 0; ly_lock = LY_PLAY;
        track_select(TRK_DRUM); frames(2);
        studio_open(SC_DRUM); dpg_go(1, 0); frames(2);
        tap(B_SEQ); frames(2);
        check(on_drum_page() && drum_pg == 0u, "DRUMS: SEQ on the drum screen: TRIG (PHEBUS wave 3; the KIT page is gone)");
        tap(B_SCL); frames(2);
        check(on_drum_page() && drum_pg == 0u && dpg_half() == 1u, "DRUMS: SEL on TRIG: its next page, TRIG 2/2");
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
        uint8_t idx_ch[40];
        char nm[40];
        mlu.on = 0;                                     /* (wave 4: the shots without LEARN in the header) */
        for (v = 0; v < 5u; v++) {
            go_home(); song.playing = 0; ly_lock = LY_PLAY; frames(2);
            track_select(v < 2u ? 0u : v - 1u); frames(2);
            if (v < 2u)
                host_preset(&trk[0], v ? ENGI_JUNO : ENGI_ACID, 0);
            if (v == 2u)                                 /* (track 2 starts on JUNO since 21 Nov: ELEMENTS for its shots) */
                host_preset(&trk[1], ENGI_ELEM, 3);
            frames(2);
            if (v == 4u) {                               /* (wave 8) the drums: no chain; EDIT is their drum screen's tabs,
                                                          * SRC FLTR AMP LFO and FX (the drum voice's own pages) */
                go_home(); frames(2);
                tap(B_EDIT); frames(2);
                check(on_drum_page() && drum_pg == 1u, "chain (drums): EDIT opens the drum screen's SRC (the sounds' pages)");
                check(!chain_list(idx_ch, sizeof idx_ch), "chain (drums): no page of the synth chain (their pages: the drum screen's)");
                continue;
            } else {
                tap(B_EDIT); frames(2);
                check(on_chain() && fam_stage(cur_page()->fam) == 0 && stage_page(0, 0) == ui.page, "chain: EDIT opens OSC, its first page");
            }
            {   /* (wave 9) every page of the voice by ONE labelled button plus SEL: EDIT, ENV, LFO, FX each step their
                 * categories (round), SEL the pages of each (round); the sound chain's pages and the MIX pages, each once */
                static const uint8_t GB[4] = {B_EDIT, B_ENV, B_FX, B_LFO};
                uint32_t g, c2, p2, cnt[NPAGES], okw = 1, nshot = 0;
                memset(cnt, 0, sizeof cnt);
                for (g = 0; g < 4u; g++) {
                    uint32_t c0;
                    go_home(); frames(2); tap(GB[g]); frames(2);
                    c0 = cur_cat();
                    for (c2 = 0; c2 < 6u; c2++) {
                        uint32_t first = ui.page;
                        okw &= nav_btn_of(cur_cat()) == GB[g];
                        for (p2 = 0; p2 < 8u; p2++) {
                            if (cur_cat() == CT_STUT) {   /* (wave 14) FX's STUT: the lane, one page (SEL: nothing) */
                                okw &= stl_shown();
                                tap(B_SCL);
                                okw &= stl_shown() && cur_cat() == CT_STUT;
                                break;
                            }
                            cnt[ui.page]++;
                            ui.force = 1; frames(2);
                            snprintf(nm, sizeof nm, "chain-%s-%02u", VN[v], (unsigned)nshot++);
                            ppm(nm);
                            printf("chain %-6s %-5s %-9s %-6s %-6s %-6s %-6s ok\n", VN[v], CT_NAME[cur_cat()], cur_page()->title,
                                   opk.lab[0], opk.lab[1], opk.lab[2], opk.lab[3]);
                            tap(B_SCL);
                            if (ui.page == first) break;
                        }
                        tap(GB[g]);
                        if (cur_cat() == c0) break;
                    }
                }
                for (j = 0; j < NPAGES; j++)
                    okw &= (fam_stage(PAGES[j].fam) >= 0 || PAGES[j].fam == FAM_MFX) ? cnt[j] == 1u : cnt[j] == 0u;
                check(okw, "chain: every sound page and MIX page by one button + SEL, each once (EDIT ENV FX LFO)");
            }
        }
        track_select(TRK_DRUM); frames(2); studio_open(SC_DRUM); frames(2);
        tap(B_LFO); frames(1);
        check(on_drum_page() && drum_pg == 4u, "chain (drums): LFO opens the drum screen's LFO page (every sound has one)");
        tap(B_ENV); frames(1);
        check(on_drum_page() && drum_pg == 3u, "chain (drums): ENV opens the sound's AMP page");
        track_select(2); frames(2); go_home(); tap(B_FX); frames(2);
        check(on_chain() && fam_stage(cur_page()->fam) == 3, "chain: FX: the voice's FX (INS, FX 1; wave 9: FX = voice FX then MIX)");
        encs[panel.enc[EN_ALGO]] = 1; frames(2);
        check(song.sel == TRK_DRUM && on_drum_page() && drum_pg == DPG_FX, "chain: from a voice's FX stage to the drums: their FX tab");
        encs[panel.enc[EN_ALGO]] = -1; frames(2);
        check(song.sel == 2u && on_drum_page(), "chain: ALGORITHM back from the drum screen: the synth's GRID (the drum screen stays a GRID)");
        track_select(0); host_preset(&trk[0], ENGI_ACID, 0); go_home(); frames(2);
    }
    {   /* PHEBUS (review 8, Ben 21 Nov: "Sound big, character small"): each sound page draws what its knobs do, big, and
         * every knob moves the picture; no character (cut: heads in the strip only); each page shot at three settings */
        static const struct { uint8_t trk, eng, pre, fam; const char *title, *shot; uint8_t mask; } SP[] = {
            {0, ENGI_JUNO, 11, FAM_OSC, "OSC", "osc-juno", 15}, {0, ENGI_JUNO, 11, FAM_OSC, "OSC 2", "osc2", 15},
            {0, ENGI_JUNO, 11, FAM_OSC, "OSC 3", "osc3-juno", 0}, {0, ENGI_JUNO, 11, FAM_FLT, "FILT", "filt-juno", 15},
            {0, ENGI_JUNO, 11, FAM_FLT, "FILT 2", "filt2", 15}, {0, ENGI_JUNO, 11, FAM_ENV, "ENV", "env", 15},
            {0, ENGI_JUNO, 11, FAM_ENV, "FILT ENV", "fenv", 15}, {0, ENGI_JUNO, 11, FAM_OSC, "OSC 4", "osc4", 0},
            /* (PHEBUS wave 5) the FX slots (a DELAY, a PHASER, an EQ), LFO 2, the matrix */
            {0, ENGI_JUNO, 11, FAM_FX, "FX 1", "fx1-dist", 6}, {0, ENGI_JUNO, 11, FAM_FX, "FX 2", "fx2-phaser", 15},
            {0, ENGI_JUNO, 11, FAM_FX, "FX 3", "fx3-eq", 7}, {0, ENGI_JUNO, 11, FAM_MOD, "LFO 2", "lfo2", 3},
            {0, ENGI_JUNO, 11, FAM_MOD, "MATRIX", "matrix", 0},
            {0, ENGI_ACID, 0, FAM_OSC, "OSC", "osc-acid", 15}, {0, ENGI_ACID, 0, FAM_FLT, "FILT", "filt-acid", 15},
            {2, ENGI_PRISM, 58, FAM_OSC, "OSC", "osc-va", 7}, {2, ENGI_PRISM, 0, FAM_OSC, "OSC", "osc-fold", 7},
            {1, ENGI_ELEM, 3, FAM_OSC, "OSC", "osc-elem", 7},
            /* (wave 8) the SENDS page; the drum voice's FILTER and INSERT are its drum screen's FX tab (drum-fx shots) */
            {0, ENGI_JUNO, 11, FAM_FX, "SENDS", "sends", 7}};
        uint32_t c, k, ok_all = 1, chr = 1;
        char nm[40], what[120];
        song.playing = 0;
        mlu.on = 0;                                     /* (wave 4: the shots without LEARN in the header) */
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
            t->p[P_FLINK] = 1;                                       /* (wave 4: the filter's own envelope drawn) */
            if (SP[c].trk < 3u && (SP[c].fam == FAM_FX || SP[c].fam == FAM_MOD)) {   /* (wave 5) slots and cells at work */
                xs_type_set(t->p, 0, XT_DIST); xs_type_set(t->p, 1, XT_PHASER); xs_type_set(t->p, 2, XT_EQ);   /* (wave 8: inserts) */
                t->p[P_DLY] = 50; t->p[P_REV] = 70; t->p[P_CHOR] = 30;   /* (the SENDS page at work) */
                t->p[P_X3TIME] = 90; t->p[P_X3INT] = 40;
                mx_set(SP[c].trk, MS_LFO2, MD_M2, 40); mx_set(SP[c].trk, MS_VEL, MD_LVL, 30);
                mx_set(SP[c].trk, MS_AENV, MD_RES, -24); t->p[P_ED_FLT] = 50; t->p[P_LD_PIT] = 6;
                mx_row = MS_FENV; mx_col = MD_CUT;
            }
            if (!is_drum(t)) t->p[P_TFLT] = 0;
            frames(16);
            if (!chain_goto(SP[c].fam, SP[c].title)) {
                snprintf(what, sizeof what, "sound page %s: reached", SP[c].shot); check(0, what); continue;
            }
            pg = cur_page();
            ui.force = 1; frames(2);
            chr &= SP[c].fam == FAM_ENV || SP[c].fam == FAM_FX || SP[c].fam == FAM_MOD || px_count(196, 22, 44, 38, TE_COL[SP[c].trk & 3u]) < 8u;   /* (ENV: the AMP | FILT
                                                         * selector there, mock-synth-env-A) */
            for (k = 0; k < 4u; k++) {                  /* every knob: the picture moves */
                const param_desc_t *d;
                int16_t v0;
                uint32_t h0, h1;
                int32_t st;
                uint32_t id = pg_id(pg, k);
                if (id >= P_COUNT) continue;
                d = track_desc(t, id);                     /* (wave 7: assigned before it is read) */
                if (d->max <= d->min || !((SP[c].mask >> k) & 1u)) continue;
                v0 = t->p[id];
                st = (d->max - d->min) / 3 > 1 ? (d->max - d->min) / 3 : 1;
                ui.force = 1; frames(2); h0 = plot_hash();
                t->p[id] = (int16_t)(v0 + st <= d->max ? v0 + st : v0 - st);
                ui.force = 1; frames(2); h1 = plot_hash();
                t->p[id] = v0;
                if (h0 == h1) { printf("ui:   %s: knob %u (%s) moves nothing\n", SP[c].shot, (unsigned)k + 1, d->label); ok = 0; }
            }
            snprintf(what, sizeof what, "sound page %s: each knob moves its picture", SP[c].shot);
            check(ok, what);
            ok_all &= ok;
            for (set = 0; set < 3u; set++) {            /* three settings: as it is, a third and five sixths of each */
                for (k = 0; set && k < 4u; k++) {
                    const param_desc_t *d;
                    uint32_t id = pg_id(pg, k);
                    if (id >= P_COUNT || (id == P_E0 && (SP[c].eng == ENGI_PRISM || SP[c].eng == ENGI_ELEM))) continue;
                    d = track_desc(t, id);
                    t->p[id] = (int16_t)(d->min + (d->max - d->min) * (set == 1u ? 2 : 5) / 6);
                }
                ui.force = 1; frames(2);
                snprintf(nm, sizeof nm, "snd-%s-%c", SP[c].shot, 'a' + (int)set);
                ppm(nm);
            }
        }
        check(chr, "sound pages: no character at the top right (the characters are heads in the strip only)");
        go_home(); track_select(0); host_preset(&trk[0], ENGI_ACID, 0); frames(2);
        trk[0].p[P_VOICE] = V_LEGATO; trk[0].p[P_GLIDE] = 0; trk[0].p[P_ITYPE] = 0;
    }
    {   /* PHEBUS wave 4, the MiniFreak voices (Ben, 21 Nov 2026: "Oscillator type plus a few macro knobs ... The same layout
         * on every voice"; 22 Nov: every Type on every track). On each synth track, OSC's KNOB 1 walks every Type, one a
         * detent, and the three macros take the Type's names; the same knob layout on every track and Type */
        uint32_t tr, ty, ok = 1, lay = 1;
        char what[120];
        for (tr = 0; tr < NPART; tr++) {
            go_home(); track_select(tr); frames(2);
            chain_goto(FAM_OSC, "OSC"); frames(16);
            for (ty = 0; ty < NVTYPE + 2u; ty++) { encs[panel.enc[EN_SELECT]] = -1; frame(); }   /* (wave 9: TYPE on SELECT) */
            for (ty = 0; ty < NVTYPE; ty++) {
                uint32_t k;
                int16_t *vp;
                ok &= vtype_of(TSEL) == ty && TSEL->eng_req == VTYPES[ty].eng;
                for (k = 0; k < 3u; k++) {                /* M1 .. M3 on KNOB 1 .. 3: the Type's names (or the engine's own) */
                    const param_desc_t *d = page_desc(&PAGES[ui.page], k, &vp);
                    const char *want = VTYPES[ty].lab[k];
                    if (!d || (want && !str_eq(d->label, want))) {
                        printf("ui:   track %u %s: macro %u is %s\n", tr + 1u, VTYPES[ty].name, k, d ? d->label : "-");
                        ok = 0;
                    }
                }
                lay &= PAGES[ui.page].sel == V_TYPE && PAGES[ui.page].id[0] == V_M1 && PAGES[ui.page].id[2] == V_M3 &&
                       PAGES[ui.page].id[3] == V_M4;
                if (tr == 2u && (ty == 0u || ty == 1u || ty == 2u || ty == 18u || ty == 22u)) {
                    snprintf(what, sizeof what, "osc-type-%s", VTYPES[ty].name);
                    for (k = 0; what[k]; k++) if (what[k] == ' ') what[k] = '-';
                    ui.force = 1; frame(); ppm(what);
                }
                encs[panel.enc[EN_SELECT]] = 1; frame(); frames(2);
            }
        }
        check(ok, "OSC: SELECT (the TYPE) on every synth track: every Type in turn (31), its macros named as the Type means them");
        check(lay, "OSC: TYPE on SELECT, M1 M2 M3 M4 on KNOB 1 to 4, the same on every track and Type");
        track_select(0); host_preset(&trk[0], ENGI_ACID, 0); trk[0].engine = trk[0].eng_req; go_home(); frames(2);
    }
    {   /* "ADSR + link" (Ben, 21 Nov): FILT ENV while LINKed shows the amp ENV's values dim; the first turn makes it OWN,
         * starting where the amp ENV is */
        track_t *t = &trk[1];
        int16_t a0;
        go_home(); track_select(1); frames(2);
        t->p[P_FLINK] = 0; t->p[P_ATK] = 20; t->p[P_DEC] = 40; t->p[P_SUS] = 70; t->p[P_REL] = 30; t->p[P_FDEC] = 5;
        chain_goto(FAM_ENV, "FILT ENV"); frames(16);
        ui.force = 1; frame(); ppm("page-fenv-link");
        a0 = t->p[P_DEC];
        encs[panel.enc[EN_K2]] = 2; frame();
        check(t->p[P_FLINK] == 1 && t->p[P_FATK] == 20 && t->p[P_FDEC] == a0 + 2 && t->p[P_FSUS] == 70 && t->p[P_FREL] == 30 &&
              t->p[P_DEC] == a0, "FILT ENV, LINKed: the first turn makes it OWN from the amp ENV's values, then turns DECAY");
        ui.msg_t = 0; ui.force = 1; frame(); ppm("page-fenv-own");
        chain_goto(FAM_FLT, "FILT 2"); frames(16);
        encs[panel.enc[EN_K2]] = -1; frame();
        check(t->p[P_FLINK] == 0, "FILT 2: KNOB 2 LINK back to AMP");
        go_home(); track_select(0); frames(2);
    }
    {   /* item 9: one value size in fixed 40 px columns on every page: no value drawn past its column (the drum LFO page's
         * "PITCH" against the next ring, wave 3), on every synth page of every Type, the GRID's tabs, the drum screen */
        uint32_t tr, ty, i, n = 0, over0;
        dial_over = 0;
        for (tr = 0; tr < NPART; tr++) {
            go_home(); track_select(tr); frames(2);
            for (ty = 0; ty < NVTYPE; ty += 3u) {
                uint8_t idx[40];
                uint32_t np;
                vtype_switch(TSEL, ty); TSEL->engine = TSEL->eng_req; frames(2);
                np = chain_list(idx, sizeof idx);
                for (i = 0; i < np; i++) {
                    uint32_t k, x;
                    page_go(idx[i]);
                    for (x = 0; x < 3u; x++) {             /* each knob at its least, its middle, its most */
                        for (k = 0; k < 4u; k++) {
                            int16_t *vp;
                            const param_desc_t *d = page_desc(&PAGES[idx[i]], k, &vp);
                            if (d && vp && pg_id(&PAGES[idx[i]], k) != V_TYPE)
                                *vp = (int16_t)(x == 0u ? d->min : x == 1u ? (d->min + d->max) / 2 : d->max);
                        }
                        ui.force = 1; frame(); n++;
                    }
                }
            }
            host_preset(TSEL, tr == 0u ? ENGI_ACID : tr == 1u ? ENGI_JUNO : ENGI_PRISM, 0); TSEL->engine = TSEL->eng_req;
        }
        over0 = dial_over;
        song.sel = 0; studio_open(SC_DRUM); frames(2);
        for (i = 0; i < 6u; i++) for (ty = 0; ty < 3u; ty++) { syn_go(i, ty % syn_halves(i)); ui.force = 1; frame(); n++; }
        song.sel = TRK_DRUM; studio_open(SC_DRUM); frames(2);
        for (i = 0; i < 6u; i++) for (ty = 0; ty < 5u; ty++) { dpg_go(i, ty % dpg_halves(i)); ui.force = 1; frame(); n++; }   /* (wave 8: the FX tab, its five halves) */
        kit_set((uint32_t)(0)); frames(2);                  /* (a HEX kit: its FLTR with the empty ENV knob) */
        for (ty = 0; ty < 2u; ty++) { dpg_go(2, ty); ui.force = 1; frame(); n++; }
        kit_set((uint32_t)(DRUM_DEFAULT_KIT)); frames(2);
        dpg_go(4, 0); ui.force = 1; frame();
        {
            char b[80];
            snprintf(b, sizeof b, "no value drawn past its 40 px column (%u screens; the drum LFO page too)", n);
            check(dial_over == 0u && over0 == 0u, b);
        }
        go_home(); track_select(0); frames(2);
    }
    {   /* PHEBUS (21 Nov 2026): TOOLS > NEW: every send and effect off, the STUTTER lane empty */
        uint32_t ok = 1, l;
        for (i = 0; i < NPART; i++) trk[i].p[P_REV] = 50, trk[i].p[P_DLY] = 40, trk[i].p[P_DIST] = 20;
        sl_send_set(3, 0, 2); song.g[G_SWING] = 30; song.g[G_DRREV] = 20; stl.on[0] = 5; stl.len = 40;
        project_new(); frames(4);
        for (i = 0; i < NPART; i++)
            ok &= !trk[i].p[P_DIST] && !trk[i].p[P_CHOR] && !trk[i].p[P_DLY] && !trk[i].p[P_REV] && !trk[i].p[P_ITYPE] &&
                  !trk[i].p[P_SSWING] && trk[i].eng_req == TRK_DEF[i][0] && trk[i].preset == TRK_DEF[i][1];
        for (l = 0; l < DRUM_LANES; l++)
            ok &= !sl_send(l, 0) && !sl_send(l, 1);
        ok &= !TDRUM->p[P_DIST] && !TDRUM->p[P_DLY] && !TDRUM->p[P_REV] && !TDRUM->p[P_ITYPE] && !song.g[G_DRREV] &&
              !song.g[G_DRDLY] && !song.g[G_SWING] && !song.g[G_DUST] && !song.g[G_DUCK] && !song.g[G_FILT] &&
              pkit.base == DRUM_DEFAULT_KIT && !kit_changed();
        check(ok, "TOOLS > NEW: the default sounds dry, every send 0, no swing, master FX off");
        check(plain_defaults(), "TOOLS > NEW: the plain saws again (ACID INIT bass, JUNO chords, PLAITS VA lead)");
        check(!stl.on[0] && stl.len == 16u && stl.act, "TOOLS > NEW: the STUTTER lane empty, 16 steps, ON");
    }
    /* ---- PHEBUS wave 7: HOME as three zones (Ben's picker "HOME: B: three zones"), one value size, the frame and the
     * colour rule on every screen */
    {
        uint32_t i, ok, z0, z1, z2, over0;
        char b[96];
        static const uint32_t ZY[3] = {40, 78, 116};   /* SYNTH, DRUM, PERFORM: their rows (the deck above) */
        uint32_t vcols = 0;
        go_home(); track_select(0); frames(2);
        for (i = 0; i < NTRK; i++) trk[i].p[P_MUTE] = 0;
        song.solo = 0; drum_mute = drum_solo = 0; perf.lat = 0; stl.act = 1; live_sec = -1; live_req = -1;
        ui.force = 1; frame(); ppm("home-zones");
        /* the zones: each voice's colour in SYNTH only on its own column; teal in DRUM; PERFORM white and grey only */
        ok = 1;
        for (i = 0; i < NPART; i++) {
            uint32_t v, x0 = 4 + i * 79;
            ok &= px_count(x0, ZY[0] + 16, 72, 22, TE_COL[i]) > 20u;
            for (v = 0; v < NTRK; v++)
                if (v != i)
                    ok &= px_count(x0, ZY[0], 74, 38, TE_COL[v]) == 0u;
        }
        check(ok, "HOME SYNTH: three voices, each in its own colour in its own column, no other voice's colour there");
        check(px_count(0, ZY[1], 240, 38, TE_DRUM) > 100u && px_count(0, ZY[1], 240, 38, TE_COL[0]) + px_count(0, ZY[1], 240, 38, TE_COL[1]) +
              px_count(0, ZY[1], 240, 38, TE_COL[2]) == 0u, "HOME DRUM: the drum voice's teal, no melodic voice's colour");
        for (i = 0; i < NTRK; i++) vcols += px_count(0, ZY[2], 240, 38, TE_COL[i]);
        check(vcols == 0u, "HOME PERFORM: white and grey only (everyone's: the colour rule)");
        check(px_count(4, ZY[0] + 17, 70, 16, TE_COL[0]) > 300u && px_count(83, ZY[0] + 17, 70, 16, TE_COL[1]) < 300u,
              "HOME: the selected voice obvious (its name inverted in its colour; the others plain)");
        track_select(TRK_DRUM); frames(2); ui.force = 1; frame();
        check(px_count(2, ZY[1] + 1, 36, 16, TE_DRUM) > 300u && px_count(4, ZY[0] + 17, 70, 16, TE_COL[0]) < 300u,
              "HOME: the drums selected, DRUM inverted, no voice inverted");
        ppm("home-zones-drum");
        track_select(0); frames(2);
        /* the zones' state: mutes, solos, the drum sounds muted, the lane off, latched pads, the pattern playing */
        z0 = home_zone_sig[0], z1 = home_zone_sig[1], z2 = home_zone_sig[2];
        trk[1].p[P_MUTE] = 1; song.solo = 1u << 2; drum_mute = 1u << 2 | 1u << 5; ui.force = 1; frame();
        check(px_count(83, ZY[0] + 16, 74, 18, TE_MUTE) > 40u && px_count(162, ZY[0] + 16, 74, 18, TE_SOLO) > 40u,
              "HOME SYNTH: a muted voice's yellow M, a soloed voice's ice-blue S, in its own column");
        check(px_count(83, ZY[0] + 16, 74, 18, TE_COL[1]) == 0u, "HOME SYNTH: a muted voice's name grey, not its colour");
        check(px_count(0, ZY[1], 240, 18, TE_MUTE) > 40u && home_zone_sig[1] != z1, "HOME DRUM: sounds muted in MUTE mode: a yellow M and how many");
        trk[1].p[P_MUTE] = 0; song.solo = 0; drum_mute = 0;
        perf.lat = 1u << PF_SWDN | 1u << PF_PHASER; stl.act = 0; ui.force = 1; frame(); ppm("home-zones-perform");
        check(px_count(150, ZY[2] + 1, 88, 16, C_WHITE) > 40u && home_zone_sig[2] != z2,
              "HOME PERFORM: the latched pads named at the right, white");
        perf.lat = 0; stl.act = 1;
        live_sec = 1; ui.force = 1; frame();
        check(px_count(200, OP_BODY_Y + 1, 11, 16, C_WHITE) > 100u, "HOME deck: the pattern playing (B) filled white");
        live_sec = -1; ui.force = 1; frame();
        (void)z0;
        /* the HOME knob row: the four values one size (the pages' dials: one tall face; DECAY was a small "2.52S") */
        {
            uint32_t k, hgt[4], same = 1;
            for (k = 0; k < 4u; k++) {
                uint32_t x, y, top = 999, bot = 0, x0 = 2 + 60 * k + DIAL_VX - 2;
                for (y = OP_KNOB_Y + 20u; y < OP_KNOB_Y + 56u; y++)
                    for (x = x0; x < x0 + DIAL_VW; x++)
                        if (swap16(screen[y * 240u + x]) != C_BLACK) {
                            if (y < top) top = y;
                            if (y > bot) bot = y;
                        }
                hgt[k] = bot >= top ? bot - top : 0u;
            }
            for (k = 1; k < 4u; k++) same &= hgt[k] == hgt[0];
            snprintf(b, sizeof b, "HOME knobs: CUTOFF DECAY STUT LEVEL values one size (%u %u %u %u px)", hgt[0], hgt[1], hgt[2], hgt[3]);
            check(same && hgt[0] >= 16u, b);
            check(px_count(0, OP_KNOB_Y, 240, OP_KNOB_H, TE_COL[0]) > 100u &&
                  px_count(2 + 120, OP_KNOB_Y + 2, 58, OP_KNOB_H - 2, TE_COL[0]) == 0u, "HOME knobs: the voice's in its colour, STUT white-grey");
        }
        /* no value past its 40 px column, on every screen of the instrument */
        dial_over = 0;
        {
            uint32_t n = 0, p;
            go_home(); ui.force = 1; frame(); n++;
            track_select(TRK_DRUM); frames(2); ui.force = 1; frame(); n++; track_select(0); frames(2);
            for (p = 0; p < PF_NPAD; p++) { perf.last = (uint8_t)p; press(B_FX); frames(12); ui.force = 1; frame(); release(B_FX); frames(2); n++; }
            {
                static const uint32_t LB[6] = {B_EDIT, B_ARP, B_SEQ, B_SCL, B_GLO, B_SAVE};
                uint32_t q, tr;
                for (tr = 0; tr < NTRK; tr += 3u) {
                    track_select(tr); frames(2);
                    for (q = 0; q < 6u; q++) { press(LB[q]); frames(12); ui.force = 1; frame(); release(LB[q]); frames(2); n++; }
                }
                track_select(0); frames(2);
            }
            for (p = 0; p < NPAGES; p++)
                if (PAGES[p].fam == FAM_GLO || PAGES[p].fam == FAM_MFX) { go_home(); page_go(p); frames(2); ui.force = 1; frame(); n++; }
            go_home(); frames(2);
            song.sel = TRK_DRUM; studio_open(SC_DRUM); frames(2); stl_open(); ui.force = 1; frame(); n++;
            stl_leave(); go_home(); track_select(0); frames(2);
            over0 = dial_over;
            snprintf(b, sizeof b, "no value past its 40 px column: HOME, PERFORM's 22 pads, every layer, GLO and FX pages, the lane (%u)", n);
            check(over0 == 0u, b);
        }
        /* the colour rule, pixel by pixel: a shared screen (PERFORM, the STUTTER lane, a GLO page) has no voice colour
         * above the strip; a voice's page none of the other voices' */
        {
            uint32_t v, bad = 0, p, o2;
            press(B_FX); frames(12); ui.force = 1; frame();
            /* (wave 13; PERFORM in columns: the black keys' one row, y 41..54) the mute chips carry their voice's colour: 1 2 3
             * theirs, the drum mutes teal, each only its own; everything else on PERFORM none */
            for (v = 0; v < NTRK; v++) bad += px_count(0, 0, 240, PF_Y + PF_BK_Y, TE_COL[v]) +
                                             px_count(0, PF_Y + PF_BK_Y + PF_BK_H, 240, OP_BAR_Y - PF_Y - PF_BK_Y - PF_BK_H, TE_COL[v]);
            for (v = 0; v < 3u; v++) {
                uint32_t o;
                int32_t cx, cy, cw;
                pf_chip_box(v, &cx, &cy, &cw);
                for (o = 0; o < NTRK; o++)
                    bad += o != v ? px_count(cx, PF_Y + cy, cw, PF_BK_H, TE_COL[o]) : px_count(cx, PF_Y + cy, cw, PF_BK_H, TE_COL[o]) < 12u;
            }
            {
                int32_t dx, dy, dw, ex, ew;
                pf_chip_box(3, &dx, &dy, &dw);
                pf_chip_box(6, &ex, &dy, &ew);
                for (v = 0; v < 3u; v++) bad += px_count(dx, PF_Y + dy, 240 - dx, PF_BK_H, TE_COL[v]);
                bad += px_count(dx, PF_Y + dy, ex + ew - dx, PF_BK_H, TE_COL[TRK_DRUM]) < 48u;
                bad += px_count(ex + ew, PF_Y + dy, 240 - ex - ew, PF_BK_H, TE_COL[TRK_DRUM]);
            }
            release(B_FX); frames(2);
            check(bad == 0u, "colour rule: PERFORM: a voice colour only on that voice's mute chip (V1 V2 V3, the drum mutes teal)");
            song.sel = TRK_DRUM; studio_open(SC_DRUM); frames(2); stl_open(); ui.force = 1; frame();
            for (v = 0, bad = 0; v < NTRK; v++) bad += px_count(0, 0, 240, OP_BAR_Y, TE_COL[v]);
            stl_leave(); go_home(); track_select(0); frames(2);
            check(bad == 0u, "colour rule: the STUTTER lane has no voice colour above the strip");
            for (p = 0; p < NPAGES; p++)
                if (PAGES[p].fam == FAM_GLO && shared_page(&PAGES[p])) {
                    go_home(); page_go(p); frames(2); ui.force = 1; frame();
                    for (v = 0; v < NTRK; v++) bad += px_count(0, 0, 240, OP_BAR_Y, TE_COL[v]);
                }
            go_home(); frames(2);
            check(bad == 0u, "colour rule: every shared GLO page has no voice colour above the strip");
            for (v = 0; v < NTRK; v++) {                /* (wave 8) the FX button's pages, from every voice: white and grey */
                track_select(v); frames(2);
                for (p = 0; p < NPAGES; p++)
                    if (PAGES[p].fam == FAM_MFX) {
                        go_home(); page_go(p); frames(2); ui.force = 1; frame();
                        for (o2 = 0; o2 < NTRK; o2++)
                            bad += px_count(0, 0, 240, OP_BAR_Y, TE_COL[o2]) + px_count(0, 0, 240, OP_BAR_Y, TE_MID[o2]);
                    }
            }
            go_home(); track_select(0); frames(2);
            check(bad == 0u, "colour rule: the FX button's DELAY, REV/CHO, MASTER (the whole mix's) no voice colour, from any voice");
            {   /* the drum screen's FX tab: the drum voice's own pages, teal (and no melodic voice's colour) */
                uint32_t h;
                song.sel = TRK_DRUM; studio_open(SC_DRUM); frames(2);
                TDRUM->p[P_ITYPE] = 1;                  /* (an INSERT in: its values shown; empty, U2: none) */
                for (h = 0; h < NDBUS; h++) {
                    dpg_go(DPG_FX, h); ui.force = 1; frame();
                    for (o2 = 0; o2 < NPART; o2++) bad += px_count(0, 0, 240, OP_BAR_Y, TE_COL[o2]);
                    bad += px_count(0, SG_KNOB_Y, 240, 58, TE_DRUM) < 20u;   /* (the kit's pages and the sound's SENDS:
                                                         * teal; MIX is the PAGES' white one now, U1) */
                }
                TDRUM->p[P_ITYPE] = 0;
                dpg_go(1, 0); go_home(); track_select(0); frames(2);
                check(bad == 0u, "colour rule: the drum FX tab's own pages (kit INS, SENDS, SLICER, KIT FLT) in teal only");
            }
            for (v = 0; v < NPART; v++) {               /* each voice's every sound page: no other voice's colour, in any
                                                         * of its shades (wave 7 found voice 1's OSC drawing dim coral) */
                uint8_t idx[40];
                uint32_t np, i2, o, mine = 0;
                go_home(); track_select(v); frames(2); open_family(FAM_OSC); frames(2);
                np = chain_list(idx, sizeof idx);
                for (i2 = 0, bad = 0; i2 < np; i2++) {
                    page_go(idx[i2]); frames(1); knob_tint_all(TRK_DRUM);   /* (as a drum or shared screen leaves it) */
                    ui.force = 1; frame();
                    for (o = 0; o < NTRK; o++)
                        if (o != v)
                            bad += px_count(0, 0, 240, OP_BAR_Y, TE_COL[o]) + px_count(0, 0, 240, OP_BAR_Y, TE_MID[o]) +
                                   px_count(0, 0, 240, OP_BAR_Y, TE_DIM[o]);
                    mine += px_count(0, 0, 240, OP_BAR_Y, TE_COL[v]) > 100u;
                }
                snprintf(b, sizeof b, "colour rule: voice %u's %u sound pages in its colour only (no other voice's shade)", v + 1, np);
                check(bad == 0u && mine == np, b);
            }
            go_home(); track_select(0); frames(2);
        }
    }
    /* ---- PHEBUS wave 8 (SPEC.md, "22 Nov 2026, 23:20: the layout, in Ben's words"): one rule everywhere, same words */
    {
        uint32_t ok, k, j;
        char b[120];
        /* THE CONFIRM on every screen that asks: one turn asks (HOLD REC: ...), a second turn does nothing, REC tapped drops the
         * question (nothing recorded), REC held until the ring fills does it */
        static const struct { const char *page; uint8_t knob; const char *q; } AK[] = {
            {"USER", 1, "HOLD SAVE: LOAD 1"}, {"USER", 2, "HOLD SAVE: ERASE 1"}, {"USER", 3, "HOLD SAVE: SAVE 1"},
            {"TOOLS", 0, "HOLD SAVE: CLEAR"}, {"TOOLS", 1, "HOLD SAVE: INIT"}, {"PROJECT", 3, "HOLD SAVE: NEW, BANK KEPT"}};
        song.playing = 0; song.sel = 0; ui.uslot = 0;
        for (k = 0, ok = 1; k < NELEM(AK); k++) {
            uint32_t pg, before;
            for (pg = 0; pg < NPAGES && strcmp(PAGES[pg].title, AK[k].page); pg++)
                ;
            go_home(); page_go(pg); frames(16);
            trk[0].step[2].n = 1; trk[0].step[2].time = ST_NOTE; trk[0].step[2].note[0] = 50;
            before = up_ops[0] + up_ops[1] + up_ops[2] + (uint32_t)step_on(&trk[0].step[2]) * 1000u + trk[0].preset * 7u;
            encs[panel.enc[EN_K1 + AK[k].knob]] = 1; frame();
            ok &= ui.ask != 0 && !strcmp(ui.msg, AK[k].q);
            if (strcmp(ui.msg, AK[k].q)) printf("ui:   asked \"%s\", want \"%s\"\n", ui.msg, AK[k].q);
            frames(10); encs[panel.enc[EN_K1 + AK[k].knob]] = 1; frame();      /* a second turn: nothing */
            ok &= up_ops[0] + up_ops[1] + up_ops[2] + (uint32_t)step_on(&trk[0].step[2]) * 1000u + trk[0].preset * 7u == before;
            tap(B_SAVE); frames(2);                                           /* tapped: no (wave 10: SAVE is the confirm) */
            ok &= !ui.ask && !song.rec && !rec_wait && cur_page() == &PAGES[pg];
            encs[panel.enc[EN_K1 + AK[k].knob]] = 1; frame();
            if (k == 0u) { ui.force = 1; frame(); ppm("page-user-ask"); }
            press(B_SAVE); frames(90); release(B_SAVE); frames(2);            /* held, the ring full: done */
            ok &= !ui.ask && !song.rec && !rec_wait;
        }
        check(ok && up_ops[0] == 1u && up_ops[1] == 1u && up_ops[2] == 1u,
              "confirm: USER LOAD / ERASE / SAVE, TOOLS CLEAR / INIT and PROJECT NEW ask, a second turn nothing, SAVE tap no, SAVE held yes");
        check(!step_on(&trk[0].step[2]), "confirm: TOOLS CLEAR, confirmed: the pattern cleared");
        /* no action is reached by a second press: a GO turned twice in a row, quickly, does nothing */
        {
            uint32_t pg;
            for (pg = 0; pg < NPAGES && strcmp(PAGES[pg].title, "USER"); pg++)
                ;
            go_home(); page_go(pg); frames(16); j = up_ops[2];
            encs[panel.enc[EN_K4]] = 1; frame(); encs[panel.enc[EN_K4]] = 1; frame(); encs[panel.enc[EN_K4]] = 1; frame();
            check(up_ops[2] == j && ui.ask == ASK_UP, "no double press: SAVE turned three times quickly: nothing saved, the question shown");
            tap(B_ENV); frames(2);
            check(!ui.ask, "confirm: another button drops the question");
        }
        /* SAME WORDS. The filter page: CUTOFF RESO ENV TYPE on the first half, DRIVE first on the second, on a synth voice and
         * on both drum machines (the HEX machine has no filter envelope: that knob empty) */
        {
            uint32_t fp = 0, f2 = 0;
            const char *SYNL[4], *HEXL[4], *SYL[4];
            for (j = 0; j < NPAGES; j++) {
                if (!strcmp(PAGES[j].title, "FILT")) fp = j;
                if (!strcmp(PAGES[j].title, "FILT 2")) f2 = j;
            }
            for (k = 0; k < 4u; k++) {
                uint32_t hid = dh_id(DM_HEX, DPG_FLTR, 0, k), sid = dh_id(DM_SYN, DPG_FLTR, 0, k);
                SYNL[k] = TP[PAGES[fp].id[k]].label;
                HEXL[k] = hid == 0xFFu ? "" : dsp_desc(DM_HEX, 0, hid)->label;
                SYL[k] = sid == 0xFFu ? "" : dsp_desc(DM_SYN, 0, sid)->label;
            }
            /* (wave 9: the synth FILT's TYPE is SELECT's, DRIVE takes its knob; the drum screen's SELECT picks the sound, so the
             * drum FLTR keeps TYPE on KNOB 4 and DRIVE first on 2/2: the first three knobs the same on both, a call for Ben) */
            ok = !strcmp(SYNL[0], "CUTOFF") && !strcmp(SYNL[1], "RESO") && !strcmp(SYNL[2], "ENV") && !strcmp(SYNL[3], "DRIVE") &&
                 PAGES[fp].sel == P_FTYPE && PAGES[f2].sel == P_FTYPE && !strcmp(SYL[3], "TYPE") && !strcmp(HEXL[3], "TYPE");
            for (k = 0; k < 3u; k++)
                ok &= !strcmp(SYL[k], SYNL[k]) && (k == 2u ? !HEXL[k][0] : !strcmp(HEXL[k], SYNL[k]));
            snprintf(b, sizeof b, "same filter page: synth %s %s %s %s, 606 %s %s %s %s, HEX %s %s -- %s", SYNL[0], SYNL[1], SYNL[2],
                     SYNL[3], SYL[0], SYL[1], SYL[2], SYL[3], HEXL[0], HEXL[1], HEXL[3]);
            check(ok, b);
            check(PAGES[fp].id[3] == P_FDRV && dh_id(DM_HEX, DPG_FLTR, 1, 0) == DSP_DRIVE && dh_id(DM_SYN, DPG_FLTR, 1, 0) == DSP_S0 + DS_DRIVE,
                  "same filter page: DRIVE on the synth FILT KNOB 4 (TYPE on SELECT), first on the drum FLTR 2/2");
            ok = 1;                                     /* one spelling: no FREQ, RES, FEEDBK as a knob's name anywhere */
            for (j = 0; j < P_COUNT; j++) ok &= !TP[j].label || (strcmp(TP[j].label, "FREQ") && strcmp(TP[j].label, "RES") && strcmp(TP[j].label, "FEEDBK"));
            for (j = 0; j < G_COUNT; j++) ok &= !GP[j].label || (strcmp(GP[j].label, "FREQ") && strcmp(GP[j].label, "RES") && strcmp(GP[j].label, "FEEDBK"));
            for (j = 0; j < DS_N; j++) ok &= strcmp(DS_DESC[j].label, "FREQ") && strcmp(DS_DESC[j].label, "RES");
            for (j = 0; j < PF_NPAD; j++) for (k = 0; k < 4u; k++) ok &= strcmp(PFD[j].k[k].lab, "FREQ") && strcmp(PFD[j].k[k].lab, "FEEDBK");
            check(ok, "one spelling: CUTOFF, RESO, FDBK on every knob (no FREQ, RES, FEEDBK)");
        }
        /* one repeat word: a synth step's repeat is RETRIG, as a drum trig's */
        {
            const char *lab[4];
            char v[4][12];
            int32_t ratio[4];
            uint32_t dim;
            song.sel = 0;
            trk[0].step[0].n = 1; trk[0].step[0].time = ST_NOTE;
            syn_trig_vals(0, 1, lab, v, ratio, &dim);
            check(!strcmp(lab[1], "RETRIG"), "one repeat word: a synth step held, TRIG 2/2 KNOB 2 is RETRIG (was RATCH)");
        }
        /* the throws' knobs are their own: ECHO and VERB no longer turn the delay's TIME or the reverb's SIZE and DAMP */
        ok = 1;
        for (k = 0; k < 4u; k++) {
            ok &= PFD[PF_ECHO].k[k].kind != KK_GLO && PFD[PF_VERB].k[k].kind != KK_GLO;
        }
        check(ok && !strcmp(PFD[PF_ECHO].k[1].lab, "FDBK") && !strcmp(PFD[PF_VERB].k[3].lab, "AMOUNT"),
              "PERFORM ECHO: FDBK and AMOUNT (its own); VERB: AMOUNT; no shared TIME SIZE DAMP on a pad");
        /* the PATTERN page (a leftover since wave 4, unreachable) is gone; STEPS is hold SEQ's; EDIT's length knob is DOUBLE */
        for (j = 0, ok = 1; j < NPAGES; j++) ok &= strcmp(PAGES[j].title, "PATTERN") != 0 && PAGES[j].fam != FAM_SEQ;
        check(ok, "the leftover PATTERN page is gone (the pattern's settings: hold SEQ, GATE DIV SWING STEPS)");
        go_home(); frames(16); press(B_SEQ); frames(12); ui.force = 1; frame();
        check(px_count(2, OP_KNOB_Y, 58, 18, TE_COL[0]) > 10u && px_count(62, OP_KNOB_Y, 58, 18, TE_COL[0]) > 10u,
              "SEQ held (wave 10): KNOB 1 RATE, KNOB 2 SHIFT (it was EDIT held's)");
        release(B_SEQ); frames(16);
        /* HOME: the screensaver's way written on it: a ">" after the FLOW band */
        go_home(); ui.force = 1; frame();
        check(px_count(HB_X + HB_W, OP_BODY_Y, 12, 16, TE_G3) > 6u, "HOME: a \">\" after the FLOW band (SELECT right: the screensaver)");
        go_home(); track_select(0); frames(2);
    }
    /* ================= (wave 9) HOW TO MOVE AROUND: Ben, 23 Nov 2026, 08:25 and 08:42 (SPEC.md). A labelled button steps
     * through its group's categories (round), never pages; SEL (PAGE) through the pages of the category shown (round); SELECT
     * is the type on a synth page and the sound on the drum screen, never a page, a tab or the tempo */
    {
        static const uint8_t BTNS[9] = {B_EDIT, B_ENV, B_LFO, B_FX, B_SEQ, B_ARP, B_GLO, B_SAVE, B_HOME};
        static const char *const BN[9] = {"EDIT", "ENV", "LFO", "FX", "SEQ", "ARP", "GLO", "SAVE", "HOME"};
        uint32_t v, b, i2, ng, k2, n2, ok, okp;
        char what[160];
        mlu.on = 0; song.playing = 0; transport_req = 2; frames(2);
        for (v = 0; v < 2u; v++) {                       /* a synth voice, the drum voice */
            track_select(v ? TRK_DRUM : 0u); frames(2);
            for (b = 0; b < 9u; b++) {
                const uint8_t *g = nav_group(BTNS[b], &ng);
                go_home(); frames(2); ui.menu = 0; vis_on = 0; frames(2);
                ok = 1;
                for (i2 = 0; i2 <= ng; i2++) {          /* each press: the next category, in order, round */
                    tap(BTNS[b]); frames(1);
                    uint32_t want = g[(i2 + (BTNS[b] == B_HOME)) % ng];   /* (HOME: from HOME, its next) */
                    ok &= cur_cat() == want && nav_btn_of(cur_cat()) == BTNS[b];
                    ok &= nav_cat(&k2, &n2) == want && (k2 == 0u || want == CT_SAVER);   /* (a category always opens on its first page; the screensaver: any, at random) */
                    if (v && BTNS[b] == B_ARP) break;   /* (the drum ARP: CHROMATIC, again off: below) */
                }
                if (ui.menu) { ui.menu = 0; go_home(); frames(2); }
                snprintf(what, sizeof what, "%s on %s: its categories in order, each on its first page, round; never another group's",
                         BN[b], v ? "the drums" : "a synth voice");
                check(ok, what);
            }
        }
        track_select(TRK_DRUM); go_home(); frames(2);
        tap(B_ARP); check(drum_keys == DK_CHROM, "drums, ARP: CHROMATIC (its one category)");
        tap(B_ARP); check(drum_keys == DK_PLAY && on_drum_page(), "drums, ARP again: the keys are steps again (CHROMATIC off)");
        /* SEL: exactly the pages of the category shown, round; one page: nothing */
        for (v = 0, okp = 1; v < 2u; v++) {
            track_select(v ? TRK_DRUM : 0u); frames(2);
            for (b = 0; b < 8u; b++) {
                uint32_t c, step;
                go_home(); frames(2);
                nav_group(BTNS[b], &ng);
                for (c = 0; c < ng; c++) {
                    uint32_t c0, sig0, seen = 0;
                    tap(BTNS[b]); frames(1);
                    c0 = nav_cat(&k2, &n2);
                    sig0 = ui.page * 131u + syn_pg * 7u + drum_pg * 977u + dpg_half() * 31u + syn_half[0] * 5u + ui.menu_sel * 3u;
                    for (step = 1; step <= n2; step++) {
                        uint32_t k3, n3;
                        tap(B_SCL); frames(1);
                        okp &= nav_cat(&k3, &n3) == c0 && k3 == step % n2 && n3 == n2;
                        seen |= 1u << k3;
                    }
                    okp &= seen == (1u << n2) - 1u &&
                           sig0 == ui.page * 131u + syn_pg * 7u + drum_pg * 977u + dpg_half() * 31u + syn_half[0] * 5u + ui.menu_sel * 3u;
                    if (!okp) { printf("ui:   SEL: %s %s, %s\n", v ? "drums" : "synth", BN[b], CT_NAME[c0] ? CT_NAME[c0] : "?"); break; }
                }
                if (ui.menu) { ui.menu = 0; go_home(); frames(2); }
            }
        }
        check(okp, "SEL (PAGE): exactly the pages of the category shown, in order, round (one page: nothing), every group, both kinds of voice");
        {   /* every page of the instrument by ONE labelled button plus SEL: every PAGES page a voice has (the synth's, the drum
             * screen's FX pages), the screensavers, the menu's sections */
            uint32_t seen[NPAGES], miss = 0, c, step;
            memset(seen, 0, sizeof seen);
            for (v = 0; v < 2u; v++) {
                track_select(v ? TRK_DRUM : 0u); frames(2);
                for (b = 0; b < 9u; b++) {
                    go_home(); frames(2);
                    nav_group(BTNS[b], &ng);
                    for (c = 0; c < ng; c++) {
                        tap(BTNS[b]); frames(1);
                        nav_cat(&k2, &n2);
                        for (step = 0; step < n2; step++) {
                            if (!ui.home && !ui.menu && !on_drum_page()) seen[ui.page] = 1;
                            if (on_drum_page() && is_drum(TSEL) && drum_pg == DPG_FX) seen[dbus_page(dpg_half())] = 1;
                            tap(B_SCL); frames(1);
                        }
                    }
                    if (ui.menu) { ui.menu = 0; go_home(); frames(2); }
                }
            }
            for (i2 = 0; i2 < NPAGES; i2++)
                if (!seen[i2] && PAGES[i2].scope != SC_TRK && PAGES[i2].scope != SC_DRUM) {
                    printf("ui:   not reached by a button + SEL: %s\n", PAGES[i2].title);
                    miss++;
                }
            check(!miss, "every page of the instrument: reached by one labelled button plus SEL (GRID and HOME: SEQ, HOME)");
        }
        /* SELECT never changes the page, the tab, the half or the tempo, anywhere; on a page with a type it turns exactly that
         * (a step held: locks it), on a page without it changes nothing at all; on the drum screen it is the sound */
        {
            static track_t tk[NTRK];
            static int16_t gk[G_COUNT];
            uint32_t c, step, okn = 1, okt = 1, okd = 1, ntype = 0, nnone = 0;
            for (v = 0; v < 2u; v++) {
                track_select(v ? TRK_DRUM : 0u); frames(2);
                for (b = 0; b < 9u; b++) {
                    go_home(); frames(2);
                    nav_group(BTNS[b], &ng);
                    for (c = 0; c < ng; c++) {
                        tap(BTNS[b]); frames(1);
                        nav_cat(&k2, &n2);
                        for (step = 0; step < n2; step++) {
                            const page_t *pg;
                            uint32_t id, kind = sel_target(&pg, &id), ln = drum_lane, ty = vtype_of(TSEL);
                            uint32_t where = ui.page * 131u + ui.home * 3u + syn_tab * 7u + syn_pg * 11u + drum_pg * 977u +
                                             dpg_half() * 31u + syn_half[0] * 5u + ui.menu * 17u + ui.menu_sel * 3u + vis_on * 19u +
                                             vis_style * 23u + (uint32_t)on_drum_page() * 29u;
                            int16_t bpm = song.g[G_BPM];
                            memcpy(tk, trk, sizeof tk); memcpy(gk, song.g, sizeof gk);
                            encs[panel.enc[EN_SELECT]] = 1; frames(2);
                            okn &= where == ui.page * 131u + ui.home * 3u + syn_tab * 7u + syn_pg * 11u + drum_pg * 977u +
                                            dpg_half() * 31u + syn_half[0] * 5u + ui.menu * 17u + ui.menu_sel * 3u + vis_on * 19u +
                                            vis_style * 23u + (uint32_t)on_drum_page() * 29u && song.g[G_BPM] == bpm;
                            if (kind == SK_NONE) {
                                uint32_t q, same = !memcmp(gk, song.g, sizeof gk);
                                for (q = 0; q < NTRK; q++) same &= !memcmp(tk[q].p, trk[q].p, sizeof trk[q].p);
                                okt &= same && drum_lane == ln;
                                if (!same) printf("ui:   SELECT changed something on %s\n", ui.home ? "HOME" : PAGES[ui.page].title);
                                nnone++;
                            } else if (kind == SK_SOUND) {
                                okd &= drum_lane == lane_step(ln, 1, 1);   /* (the drum grid's key order, round) */
                                drum_lane = (uint8_t)ln;
                            } else {
                                uint32_t moved = id == V_TYPE ? vtype_of(TSEL) != ty
                                               : pg->scope == SC_GLOBAL ? song.g[id] != gk[id] : TSEL->p[id] != tk[song.sel].p[id];
                                okt &= moved;
                                if (!moved) printf("ui:   SELECT did not turn the type on %s\n", PAGES[ui.page].title);
                                if (id == V_TYPE) { encs[panel.enc[EN_SELECT]] = -1; frames(2); }
                                else memcpy(trk, tk, sizeof tk);
                                ntype++;
                            }
                            tap(B_SCL); frames(1);
                        }
                    }
                    if (ui.menu) { ui.menu = 0; go_home(); frames(2); }
                }
            }
            snprintf(what, sizeof what, "SELECT, on every page of every group (%u typed, %u without): no page, tab, half or tempo change", ntype, nnone);
            check(okn, what);
            check(okt && ntype == 16u, "SELECT: turns the type where the page has one (OSC TYPE, FILT TYPE, FX 1-3 TYPE, LFO WAVE, SLICER; "
                  "the drum kit's INSERT TYPE and SLICER mode, review 26 Nov U2, the SLICER again under ARP's CHROMATIC), nothing elsewhere");
            check(okd, "SELECT on the drum screen (every tab): the next of the 16 sounds");
            go_home(); track_select(0); host_preset(&trk[0], ENGI_ACID, 0); frames(2);
        }
        {   /* the chip: what SELECT turns, where the header's title is, in the voice's colour; none where SELECT does nothing */
            char cv[10]; int32_t cr; uint16_t cc;
            go_home(); frames(2); tap(B_EDIT); tap(B_EDIT); frames(2);  /* FILT */
            ui.force = 1; frame(); ppm("sel-chip-filt");
            check(sel_chip(cv, &cr, &cc) && cc == TE_COL[0] && px_count(50, 0, 90, 20, TE_DIM[0]) > 100u,
                  "SELECT's chip on FILT: the filter's TYPE in the header's title place, on the voice's dark shade");
            encs[panel.enc[EN_SELECT]] = 1; frame(); ui.force = 1; frame(); ppm("sel-chip-hot");
            check(px_count(50, 0, 90, 20, TE_COL[0]) > 300u, "SELECT turned: the chip filled in the voice's colour a moment (as a touched dial)");
            encs[panel.enc[EN_SELECT]] = -1; frames(50);
            tap(B_ENV); frames(2);
            check(!sel_chip(cv, &cr, &cc) && px_count(50, 0, 90, 20, TE_DIM[0]) == 0u, "ENV (no type): no chip, the sound's name");
            track_select(TRK_DRUM); go_home(); tap(B_SEQ); frames(2); ui.force = 1; frame(); ppm("sel-chip-drum");
            check(sel_chip(cv, &cr, &cc) && cc == TE_DRUM && str_eq(cv, LANE_SHORT[drum_lane & 15u]),
                  "drum screen: the chip names the selected sound (SELECT's), in teal");
            track_select(0); go_home(); frames(2);
        }
        {   /* SEQ + OCT- halves (asks when it would drop something: the one confirm), SEQ + OCT+ clones (steps, locks, nudges,
             * conditions); one undo each */
            track_t *t = &trk[0];
            uint32_t j;
            song.sel = 0; go_home(); frames(2);
            steps_clear(t); for (j = 0; j < NLOCK; j++) t->lock[j].step = LOCK_FREE;
            t->p[P_SLEN] = 16;
            t->step[2].n = 1; t->step[2].note[0] = 60; t->step[2].time = ST_NOTE; t->step[2].vel = 100;
            lock_set(t, 2, P_FCUT, 30); t->micro[2] = 4; step_fill_set(t, 2, FC_FILL);
            press(B_SEQ); frames(10);
            press(B_OCTUP); release(B_OCTUP);
            check(t->p[P_SLEN] == 32 && step_on(&t->step[18]) && t->step[18].note[0] == 60 && lock_find(t, 18, P_FCUT, 0) >= 0 &&
                  t->micro[18] == 4 && step_fill(t, 18) == FC_FILL, "SEQ + OCT+: doubled as a clone (the note, its lock, nudge, condition on 19)");
            press(B_OCTDN); release(B_OCTDN);
            check(t->p[P_SLEN] == 32 && ui.ask == ASK_HALVE && str_eq(ui.msg, "HOLD SAVE: HALVE TO 16"),
                  "SEQ + OCT-: the second half holds steps: it asks (HOLD SAVE: HALVE TO 16), nothing dropped yet");
            press(B_SAVE); frames(110); release(B_SAVE); frames(2);
            check(t->p[P_SLEN] == 16 && !step_on(&t->step[18]) && lock_find(t, 18, P_FCUT, 0) < 0 && !t->micro[18] && step_on(&t->step[2]),
                  "... SEQ kept held, SAVE held until the ring fills: halved, the second half dropped, the first kept");
            press(B_OCTDN); release(B_OCTDN);
            check(t->p[P_SLEN] == 8 && !ui.ask, "SEQ + OCT- again (steps 9-16 empty): halved at once, nothing to ask");
            release(B_SEQ); frames(2);
            press(B_SAVE); press(B_OCTDN); release(B_OCTDN); release(B_SAVE); frames(2);
            check(t->p[P_SLEN] == 16, "SAVE + OCT-: undo brings the length back");
            t->p[P_SLEN] = 64; press(B_SEQ); frames(10); press(B_OCTUP); release(B_OCTUP);
            check(t->p[P_SLEN] == 64 && str_eq(ui.msg, "64 STEPS: FULL"), "SEQ + OCT+ at 64 steps: nothing (64 STEPS: FULL)");
            release(B_SEQ); frames(2);
            t->p[P_SLEN] = 16; steps_clear(t); for (j = 0; j < NLOCK; j++) t->lock[j].step = LOCK_FREE;
        }
        {   /* the routes removed do nothing */
            track_t *t = &trk[0];
            uint32_t k5 = key_of_white(4);
            song.sel = 0; go_home(); frames(2); tap(B_SEQ); frames(2);
            t->step[4].n = 1; t->step[4].note[0] = 60; t->step[4].time = ST_NOTE;
            fm1_in.notes |= 1u << k5; frame();
            press(B_OCTUP); release(B_OCTUP);
            check(step_fill(t, 4) == FC_NORM, "removed: a step held + OCT+ (its condition: TRIG KNOB 4 COND)");
            fm1_in.notes &= ~(1u << k5); frames(2);
            track_select(TRK_DRUM); studio_open(SC_DRUM); frames(16);
            { uint32_t ln = drum_lane; press(B_SCL); frames(30); key(key_of_white(9)); release(B_SCL); frames(2);
              check(drum_lane == ln && ui.layer == LY_PLAY, "removed: SEL held + a key (the drum sound pick: SELECT)"); }
            tap(B_SCL); frames(2); check(drum_keys != DK_MUTE, "removed: SEL tapped on the drum screen: no MUTE mode (the mixer's)");
            track_select(0); go_home(); frames(2);
        }
        {   /* the new and changed pages fit their columns and keep the colour rule */
            uint32_t over0 = dial_over, bad = 0, q;
            song.sel = 0; go_home(); tap(B_SEQ); tap(B_SEQ); frames(2); ui.force = 1; frame(); ppm("grid-pattern-2");
            track_select(TRK_DRUM); go_home(); tap(B_SEQ); tap(B_SEQ); frames(2); ui.force = 1; frame(); ppm("drum-pattern");
            for (q = 0; q < NPART; q++) bad += px_count(0, 0, 240, OP_BAR_Y, TE_COL[q]);
            press(B_GLO); frames(12); ui.force = 1; frame();
            for (q = 0; q < NPART; q++) bad += px_count(0, OP_BODY_Y + 96, 240, 36, TE_COL[q]);
            bad += px_count(0, 0, 240, 240, TE_RED);
            release(B_GLO); frames(2);
            check(dial_over == over0, "PATTERN (synth GRID, drum screen): no value past its column");
            check(!bad, "colour rule: the drum PATTERN page and the drum mixer keys hold no melodic voice's colour, no red");
            track_select(0); go_home(); frames(2);
        }
        {   /* NO SOUND CHANGES: the same edits made by the new routes and written straight (what the old routes wrote) render
             * the same audio, sample for sample (each in a fork of this process: the same state to start from) */
            uint32_t r, h[2];
            for (r = 0; r < 2u; r++) {
                int fd[2];
                pid_t pid;
                if (pipe(fd)) { h[r] = r; continue; }
                fflush(stdout);
                pid = fork();
                if (!pid) {
                    uint32_t hh = 2166136261u, q, j, t0 = fm1_ms;
                    static int32_t o3[CTL * 2];
                    track_t *t = &trk[0];
                    close(fd[0]);
                    song.sel = 0; go_home(); frames(2);
                    dpat_clear(&dpat);                  /* (wave 11: the drums silent, what earlier tests left there out) */
                    for (q = 0; q < NTRK; q++) { memset(trk[q].v, 0, sizeof trk[q].v); trk[q].p[P_MUTE] = 0; trk[q].nheld = 0; trk[q].arp_phys = 0; }
                    for (q = 0; q < NROLL; q++) roll[q].on = 0;
                    memset(kb_kind, 0, sizeof kb_kind); song.solo = 0; perf_unlatch(); fill_lat = fill_arm = 0;
                    for (q = 0; q < NXSLOT; q++)        /* (the slots as a fresh sound's: what earlier tests set there out) */
                        xs_type_set(t->p, q, XT_OFF);
                    t->p[P_SLCR] = 0;
                    memset(xst, 0, sizeof xst);
                    for (q = 0; q < NTRK; q++) trk[q].ins_run = trk[q].tail = 0;
                    steps_clear(t); t->p[P_SLEN] = 16;
                    for (j = 0; j < 16u; j += 2u) { t->step[j].n = 1; t->step[j].note[0] = (uint8_t)(48 + j); t->step[j].time = ST_NOTE; t->step[j].vel = 100; }
                    if (!r) {                           /* straight */
                        t->p[P_FTYPE] = (int16_t)(t->p[P_FTYPE] + 1);
                        t->p[P_LWAVE] = (int16_t)(t->p[P_LWAVE] + 1);
                        xs_type_set(t->p, 0, XT_DIST);
                        t->p[P_SSWING] = (int16_t)(t->p[P_SSWING] + 3);
                        for (q = 0; q < NPART; q++) trk[q].p[P_ROOT] = 2;
                        { for (j = 0; j < 16u; j++) { t->step[16 + j] = t->step[j]; }
                        t->p[P_SLEN] = 32; }
                    } else {                            /* by the new routes */
                        for (q = 0; q < NPART; q++) trk[q].p[P_ROOT] = 0;
                        { tap(B_EDIT); tap(B_EDIT); frames(16); encs[panel.enc[EN_SELECT]] = 1; frames(2); }   /* FILT TYPE */
                        { tap(B_LFO); frames(2); encs[panel.enc[EN_SELECT]] = 1; frames(2); }                 /* LFO WAVE */
                        { go_home(); tap(B_FX); frames(2);
                        while (t->p[P_X1TYPE] != XT_DIST) { encs[panel.enc[EN_SELECT]] = 1; frames(2); } }  /* FX 1 TYPE */
                        { tap(B_SEQ); tap(B_SEQ); frames(16); encs[panel.enc[EN_K3]] = 3; frames(2); }       /* PATTERN SWING */
                        { go_home(); tap(B_ARP); tap(B_ARP); frames(16); encs[panel.enc[EN_K1]] = 2; frames(2); }   /* KEY ROOT */
                        { go_home(); press(B_SEQ); frames(10); press(B_OCTUP); release(B_OCTUP); release(B_SEQ); frames(2); }
                    }
                    go_home(); frames(2);
                    while (fm1_ms < t0 + 8000u) frame();     /* (the same time passed on both: free LFOs, tails) */
                    transport_req = 1;
                    for (q = 0; q < 600u; q++) {
                        mix_block(o3, CTL);
                        if (q == 0u) ui_input();               /* (the transport request taken) */
                        for (j = 0; j < CTL * 2u; j++) hh = (hh ^ (uint32_t)o3[j]) * 16777619u;
                    }
                    if (write(fd[1], &hh, sizeof hh) != (ssize_t)sizeof hh) _exit(1);
                    _exit(0);
                }
                close(fd[1]);
                if (read(fd[0], &h[r], sizeof h[r]) != (ssize_t)sizeof h[r]) h[r] = r + 7u;
                close(fd[0]);
                waitpid(pid, 0, 0);
            }
            check(h[0] == h[1], "no sound changes: FILT TYPE, LFO WAVE, FX 1 TYPE (SELECT), SWING (PATTERN), ROOT (KEY), SEQ + OCT+ = the same audio as written straight");
        }
    }
    {   /* ==== (wave 10, Ben 23 Nov 2026, 09:08 and 09:20: "every control has ONE idea") ==== */
        track_t *t = &trk[0];
        uint32_t k, n0, i;
        int8_t o0;
        #define PMT(b) do { tap(b); frames(20); } while (0)     /* a single tap of -/+, then past the double tap's window */
        #define DTAP(b) do { tap(b); tap(b); frames(20); } while (0)
        ly_lock = LY_PLAY; song.playing = 0; transport_req = 2; ui.ask = 0; ui.msg_t = 0; frames(2);
        track_select(0); go_home(); frames(2);
        /* -- 1. OCT- / OCT+: a single tap is the screen's minus / plus, never the octave; a double tap the octave -- */
        steps_clear(t); t->p[P_SLEN] = 32;
        for (k = 0; k < 32u; k += 4u) { t->step[k].n = 1; t->step[k].note[0] = 60; t->step[k].time = ST_NOTE; t->step[k].vel = 100; }
        song.octave = 0; o0 = song.octave;
        tap(B_SEQ); frames(2);
        check(on_drum_page() && !is_drum(TSEL) && sg_page() == 0u, "w10: SEQ: the GRID, steps 1-16");
        ui.force = 1; frame();
        check(pm_shown_m && str_eq(pm_shown_m, "STEPS") && pm_shown_p && str_eq(pm_shown_p, "STEPS"),
              "w10: GRID: the -/+ legend says STEPS (the pages of steps)");
        ppm("w10-grid-legend");
        PMT(B_OCTUP);
        check(sg_page() == 1u && song.octave == o0, "w10: GRID: OCT+ tapped once: the next page of steps, the octave unchanged");
        PMT(B_OCTDN);
        check(sg_page() == 0u && song.octave == o0, "w10: GRID: OCT- tapped once: the page before, the octave unchanged");
        DTAP(B_OCTUP);
        check(sg_page() == 0u && song.octave == o0 + 1 && t->p[P_SLEN] == 32, "w10: GRID: OCT+ twice quickly: the octave up, the first tap's page taken back");
        DTAP(B_OCTDN);
        check(song.octave == o0 && sg_page() == 0u, "w10: GRID: OCT- twice quickly: the octave down again, the page as it was");
        PMT(B_OCTUP); PMT(B_OCTUP);
        check(t->p[P_SLEN] == 48 && song.octave == o0, "w10: GRID: OCT+ on the last page: a page added (as before), no octave");
        t->p[P_SLEN] = 32; sg_cur_set(0); frames(2);
        /* a sound page: nothing touched, nothing done and no legend; then the touched knob, finely */
        tap(B_EDIT); frames(2); go_home(); frames(2);
        tap(B_EDIT); frames(2); tap(B_EDIT); frames(2);
        check(!on_drum_page() && cur_page()->cat == CT_FILT, "w10: EDIT twice: FILT");
        ui.force = 1; frame();
        check(!pm_shown_m && !pm_shown_p, "w10: FILT, nothing touched yet: no -/+ legend");
        n0 = (uint32_t)t->p[P_FRES];
        PMT(B_OCTUP);
        check(t->p[P_FRES] == (int16_t)n0 && song.octave == o0, "w10: FILT, nothing touched: OCT+ does nothing (not the octave)");
        encs[panel.enc[EN_K2]] = 1; frame(); frames(10);
        n0 = (uint32_t)t->p[P_FRES];
        ui.force = 1; frame();
        check(pm_shown_m && str_eq(pm_shown_m, "RESO"), "w10: FILT, RESO turned: the legend says RESO");
        ppm("w10-page-legend");
        PMT(B_OCTUP);
        check(t->p[P_FRES] == (int16_t)n0 + 1 && song.octave == o0, "w10: FILT: OCT+ RESO one fine step up");
        PMT(B_OCTDN);
        check(t->p[P_FRES] == (int16_t)n0, "w10: FILT: OCT- RESO one fine step down");
        DTAP(B_OCTDN);
        check(t->p[P_FRES] == (int16_t)n0 && song.octave == o0 - 1, "w10: FILT: OCT- twice quickly: the octave down, RESO as it was");
        DTAP(B_OCTUP);
        check(song.octave == o0, "w10: ... OCT+ twice quickly: the octave back");
        {   /* GLO > GLOBAL: BPM, the tempo a beat at a time */
            int16_t b0;
            tap(B_GLO); frames(2);
            encs[panel.enc[EN_K1]] = 1; frame(); frames(10);
            b0 = song.g[G_BPM];
            PMT(B_OCTUP);
            check(song.g[G_BPM] == b0 + 1, "w10: GLO > GLOBAL, BPM touched: OCT+ the tempo up one");
            PMT(B_OCTDN);
            check(song.g[G_BPM] == b0, "w10: ... OCT- down one");
        }
        {   /* SAVE > PRESETS: the sound before / after */
            uint32_t tot, p0;
            go_home(); tap(B_SAVE); tap(B_SAVE); frames(2);
            ui.force = 1; frame();
            check(cur_page()->graph == GR_BROWSE && pm_shown_m && str_eq(pm_shown_m, "SOUND"), "w10: SAVE > PRESETS: the legend says SOUND");
            p0 = preset_pos(&tot);
            PMT(B_OCTUP);
            check(preset_pos(&tot) == (p0 + 1u) % tot && song.octave == o0, "w10: SAVE > PRESETS: OCT+ the next sound");
            PMT(B_OCTDN);
            check(preset_pos(&tot) == p0, "w10: ... OCT- the one before");
            {   /* (Ben, 26 Nov 2026: "Wait 0.3 s on load pages") */
                int16_t r0 = t->p[P_FRES];
                t->p[P_FRES] = (int16_t)(r0 + 7);         /* (an edited sound) */
                tap(B_OCTUP); frames(2);
                check(preset_pos(&tot) == p0 && t->p[P_FRES] == r0 + 7, "E4: SAVE > PRESETS: a single tap waits (nothing loaded yet, within 0.3 s)");
                frames(20);                               /* (no further input: the UI tick loads it) */
                check(preset_pos(&tot) == (p0 + 1u) % tot && song.octave == o0, "E4: ... 0.3 s with no second tap: the next sound loads");
                PMT(B_OCTDN);
                t->p[P_FRES] = (int16_t)(r0 + 7);
                DTAP(B_OCTUP);
                check(preset_pos(&tot) == p0 && t->p[P_FRES] == r0 + 7 && song.octave == o0 + 1,
                      "E4: SAVE > PRESETS: a double tap: the octave up, nothing loaded, the edited sound kept");
                DTAP(B_OCTDN);
                check(preset_pos(&tot) == p0 && t->p[P_FRES] == r0 + 7 && song.octave == o0, "E4: ... and down again, the sound still kept");
                tap(B_OCTUP); tap(B_HOME); frames(2);
                check(preset_pos(&tot) == (p0 + 1u) % tot, "E4: a tap, then another button within 0.3 s: the tap acts first");
                PMT(B_OCTDN);
                t->p[P_FRES] = r0;
            }
        }
        go_home(); frames(2);
        {
            const char *m, *p;
            pm_words(&m, &p);
            check(!m && !p, "w10: HOME: no -/+ legend (nothing to do)");
        }
        {   /* SEQ's third category, A-D: the patterns; -/+ the one before / after */
            uint32_t l0 = sec_loads;
            tap(B_SEQ); frames(2); tap(B_SEQ); frames(2); tap(B_SEQ); frames(2);
            {
                uint32_t kk, nn;
                check(nav_cat(&kk, &nn) == CT_PATTS, "w10: SEQ three times: A-D, the patterns (a SEQ category)");
            }
            ui.force = 1; frame();
            check(pm_shown_m && str_eq(pm_shown_m, "PATT"), "w10: A-D: the legend says PATT");
            ppm("w10-seq-patterns");
            live_sec = 0;
            PMT(B_OCTUP);
            check(sec_loads == l0 + 1u && live_sec == 1, "w10: A-D, stopped: OCT+ loads B");
            tap(B_OCTDN); frames(2);
            check(sec_loads == l0 + 1u && live_sec == 1, "E4: A-D, stopped: a single tap waits (within 0.3 s, nothing loaded)");
            frames(20);
            check(sec_loads == l0 + 2u && live_sec == 0, "E4: ... 0.3 s with no second tap: A loads");
            DTAP(B_OCTUP);
            check(sec_loads == l0 + 2u && live_sec == 0 && song.octave == o0 + 1, "E4: A-D, stopped: a double tap: the octave, nothing loaded");
            DTAP(B_OCTDN);
            check(sec_loads == l0 + 2u && song.octave == o0, "E4: ... and back");
            PMT(B_OCTUP);
            encs[panel.enc[EN_K1]] = -1; frame(); frames(4);
            check(live_sec == 0, "w10: A-D: KNOB 1 back to A");
            {   /* E4 on the drum track's A-D */
                uint32_t kk, nn, l1;
                track_select(TRK_DRUM); go_home(); frames(2);
                tap(B_SEQ); frames(2); tap(B_SEQ); frames(2); tap(B_SEQ); frames(2);
                l1 = sec_loads;
                check(nav_cat(&kk, &nn) == CT_PATTS && is_drum(TSEL), "E4: (the drum track, SEQ > A-D)");
                DTAP(B_OCTUP);
                check(sec_loads == l1 && live_sec == 0 && song.octave == o0 + 1, "E4: drums' A-D, stopped: a double tap: the octave, nothing loaded");
                DTAP(B_OCTDN);
                tap(B_OCTUP); frames(2);
                check(sec_loads == l1, "E4: drums' A-D: a single tap waits");
                frames(20);
                check(sec_loads == l1 + 1u && live_sec == 1, "E4: ... then B loads");
                encs[panel.enc[EN_K1]] = -1; frame(); frames(4);
                track_select(0);
            }
            {   /* (review 26 Nov, U4, Ben's picker "Four pattern tiles") A-D's body: PATTERN A to D as tiles, STORED, PLAYING,
                 * QUEUED, EMPTY (as hold SAVE and HOME draw them), white and grey; on a synth voice and on the drums */
                uint32_t v2, ok4 = 1;
                for (v2 = 0; v2 < 2u; v2++) {
                    uint32_t ty = v2 ? 94u : SG_BODY_Y + 22u, o;   /* (the tiles' top: the drum screen's band, the GRID's body) */
                    if (v2) { track_select(TRK_DRUM); frames(2); nav_go(CT_PATTS, 0); frames(2); }
                    live_sec = 0; live_req = 1; ui.force = 1; frame();
                    ppm(v2 ? "u4-drum-ad" : "u4-synth-ad");
                    ok4 &= px_count(1, ty, 58, 3, C_WHITE) > 150u;                    /* A: PLAYING, a white bar */
                    ok4 &= px_count(61, ty, 58, 1, C_WHITE) > 50u && px_count(61, ty + 33u, 58, 1, C_WHITE) > 50u;   /* B: QUEUED */
                    ok4 &= px_count(121, ty + 4u, 58, 28, TE_G1) > 900u && px_count(181, ty + 4u, 58, 28, TE_G1) > 900u;   /* C D EMPTY */
                    ok4 &= px_count(1, ty + 4u, 58, 28, TE_G2) > 900u;                /* A: STORED's grey */
                    for (o = 0; o < NTRK; o++) ok4 &= px_count(0, ty, 240, 34, TE_COL[o]) == 0u;
                    live_req = -1; ui.force = 1; frame();
                    ok4 &= px_count(61, ty + 4u, 58, 28, TE_G2) > 900u && px_count(61, ty + 33u, 58, 1, C_WHITE) == 0u;   /* B: STORED */
                    if (!v2) ok4 &= px_count(0, SG_BODY_Y, 240, 109, TE_COL[0]) == 0u && px_count(0, SG_BODY_Y, 240, 109, TE_DIM[0]) == 0u;   /* (no steps) */
                }
                track_select(0); frames(2);
                check(ok4, "U4: SEQ > A-D: four tiles, PATTERN A to D: PLAYING (a white bar), QUEUED (framed), STORED, EMPTY; "
                           "white and grey; on a synth voice (no steps) and on the drums");
            }
        }
        go_home(); frames(2);
        /* -- 12. (Ben 10:33) the steps show only on SEQ: from the GRID with nothing held a page button opens its real page;
         *    with a step held its knobs come under the steps (the lock view) and the GRID comes back when it is let go -- */
        {
            static const uint8_t PB[4] = {B_EDIT, B_ENV, B_LFO, B_FX};
            static const uint8_t PC[4] = {CT_OSC, CT_AMP, CT_LFO1, CT_INS};
            uint32_t j, ok = 1;
            int l;
            for (j = 0; j < 4u; j++) {
                go_home(); tap(B_SEQ); frames(2);
                tap(PB[j]); frames(2);
                ok &= !on_drum_page() && !kb_grid && cur_page()->cat == PC[j];
            }
            check(ok, "w10: GRID, nothing held: EDIT, ENV, LFO, FX each open their real page (the keys play notes)");
            ui.force = 1; frame(); ppm("w10-grid-to-page");
            go_home(); tap(B_SEQ); frames(2);
            t->step[4].n = 1; t->step[4].note[0] = 60; t->step[4].time = ST_NOTE; t->step[4].vel = 100;
            fm1_in.notes |= 1u << key_of_white(4); frame();
            tap(B_ENV); frames(2);
            check(on_drum_page() && syn_tab == 3u && sg_held, "w10: GRID, a step held + ENV: the lock view (ENV's knobs under the steps)");
            encs[panel.enc[EN_K1]] = 2; frame();
            l = lock_find(t, 4, P_ATK, 0);
            ui.force = 1; frame(); ppm("w10-grid-lock-view");
            check(l >= 0, "w10: ... a turn locks (ATTACK on step 5)");
            fm1_in.notes &= ~(1u << key_of_white(4)); frames(3);
            check(on_drum_page() && syn_tab == 0u && cur_cat() == CT_TRIG, "w10: the step let go: back to the plain SEQ screen (TRIG)");
            lock_del(t, 4, P_COUNT); step_clear(&t->step[4]);
            go_home(); frames(2);
        }
        /* -- 2. MINUS REMOVES -- */
        {   /* a GRID step held + OCT- (TRIG): the step itself goes */
            tap(B_SEQ); frames(2);
            fm1_in.notes |= 1u << key_of_white(4); frame(); frames(2);
            tap(B_OCTDN);
            fm1_in.notes &= ~(1u << key_of_white(4)); frames(22);
            check(!step_on(&t->step[4]) && song.octave == o0, "w10: GRID, TRIG: a step held + OCT-: its note erased (no octave)");
        }
        go_home(); frames(2);
        {   /* a key held + OCT- (a synth voice, stopped): that note goes from every step */
            uint32_t kk = key_of_white(4), note = kb_map(t, kk), j, left = 0;
            steps_clear(t); t->p[P_SLEN] = 16;
            for (j = 0; j < 16u; j += 2u) { t->step[j].n = 1; t->step[j].note[0] = (uint8_t)note; t->step[j].time = ST_NOTE; t->step[j].vel = 100; }
            t->step[1].n = 1; t->step[1].note[0] = (uint8_t)(note + 2u); t->step[1].time = ST_NOTE;
            fm1_in.notes |= 1u << kk; frames(2);
            tap(B_OCTDN);
            fm1_in.notes &= ~(1u << kk); frames(22);
            for (j = 0; j < 16u; j += 2u) left += step_on(&t->step[j]);
            check(!left && step_on(&t->step[1]) && song.octave == o0, "w10: a key held + OCT-: its note erased from the pattern, another note kept, no octave");
            press(B_SAVE); tap(B_OCTDN); release(B_SAVE); frames(2);
            check(step_on(&t->step[0]), "w10: SAVE + OCT-: the erase undone");
        }
        {   /* the drums: a sound's key held + OCT-: its hits go (OCT- first, then the key: a ghost hit, as ever) */
            uint32_t j, left = 0;
            track_select(TRK_DRUM); go_home(); frames(2);
            steps_clear(TDRUM); TDRUM->p[P_SLEN] = 16;
            for (j = 0; j < 16u; j += 4u) { dtest_set(j, 0, LV_NORM, 0); dtest_set(j, 2, LV_NORM, 0); }
            fm1_in.notes |= 1u << key_of_lane(0); frames(2);
            tap(B_OCTDN);
            fm1_in.notes &= ~(1u << key_of_lane(0)); frames(22);
            for (j = 0; j < 16u; j++) left += dt_has(j, 0);
            check(!left && dt_has(0, 2), "w10: drums: the kick's key held + OCT-: every kick erased, the snares kept");
            track_select(0); go_home(); frames(2);
        }
        {   /* PERFORM: a latched pad held + OCT-: its latch only */
            press(B_FX); frames(12);
            press(B_OCTUP); key(key_of_white(2)); key(key_of_white(3)); release(B_OCTUP); frames(2);
            check((perf.lat >> 2) & (perf.lat >> 3) & 1u, "w10: PERFORM: OCT+ held + two pads: both latched");
            ui.force = 1; frame(); ppm("w10-perform");
            tap(B_OCTDN);
            check(!perf.lat, "w10: PERFORM: OCT- with no pad held: every latch off");
            release(B_FX); frames(4);
        }
        {   /* EDIT + OCT-: the whole voice, after the confirm (hold SAVE); EDIT held erases nothing */
            uint32_t j, on = 0;
            steps_clear(t); t->p[P_SLEN] = 16;
            for (j = 0; j < 16u; j += 2u) { t->step[j].n = 1; t->step[j].note[0] = 60; t->step[j].time = ST_NOTE; t->step[j].vel = 100; }
            press(B_EDIT); frames(12);
            ui.force = 1; frame(); ppm("w10-hold-edit");
            check(pm_lm && str_eq(pm_lm, "CLEAR") && !pm_lp, "w10: EDIT held: the legend says - CLEAR");
            key(key_of_white(7)); frames(2);
            for (j = 0; j < 16u; j += 2u) on += step_on(&t->step[j]);
            check(on == 8u, "w10: EDIT held + a key: nothing erased (EDIT erases nothing now)");
            tap(B_OCTDN); release(B_EDIT); frames(2);
            check(ui.ask == ASK_CLEAR && !strncmp(ui.msg, "HOLD SAVE: CLEAR VOICE 1", 24), "w10: EDIT + OCT-: it asks (HOLD SAVE: CLEAR VOICE 1)");
            check(step_on(&t->step[0]), "w10: ... nothing cleared yet; the question stays after EDIT is let go");
            ui.force = 1; frame(); ppm("w10-ask-clear");
            press(B_SAVE); frames(30);
            ui.force = 1; frame(); ppm("w10-confirm-ring");
            check(ui.hold_kind == 3u, "w10: SAVE held with the question: the ring");
            frames(70); release(B_SAVE); frames(2);
            check(!step_on(&t->step[0]) && !ui.ask, "w10: SAVE held until the ring fills: the voice cleared");
            press(B_SAVE); tap(B_OCTDN); release(B_SAVE); frames(2);
            check(step_on(&t->step[0]), "w10: SAVE + OCT-: undo (the voice back)");
            press(B_SAVE); tap(B_OCTUP); release(B_SAVE); frames(2);
            check(!step_on(&t->step[0]), "w10: SAVE + OCT+: redo (cleared again)");
            press(B_SAVE); tap(B_OCTDN); release(B_SAVE); frames(2);
        }
        {   /* REC only records: held, it clears nothing and confirms nothing */
            uint32_t r0 = song.rec;
            press(B_REC); frames(160); release(B_REC); frames(2);
            check(step_on(&t->step[0]), "w10: REC held 2.5 s: nothing cleared");
            if (song.rec != r0 || rec_wait) { tap(B_REC); frames(2); }
            if (sr_on) { tap(B_REC); frames(2); }
            go_home(); frames(2);
            press(B_EDIT); tap(B_OCTDN); release(B_EDIT); frames(2);
            check(ui.ask == ASK_CLEAR, "w10: (asked again)");
            press(B_REC); frames(120); release(B_REC); frames(2);
            check(!ui.ask && step_on(&t->step[0]), "w10: REC held with a question: no confirm (the question goes, nothing cleared)");
            if (song.rec || rec_wait) { tap(B_REC); frames(2); }
            if (sr_on) { tap(B_REC); frames(2); }
            go_home(); frames(2);
        }
        {   /* the confirm is SAVE's everywhere: TOOLS NEW asks HOLD SAVE */
            tap(B_SAVE); frames(2);
            check(cur_page()->cat == CT_PROJ, "w10: SAVE once: PROJECT (the bank: NEW moved here from TOOLS)");
            encs[panel.enc[EN_K4]] = 1; frame();
            check(ui.ask == ASK_BANK && !strncmp(ui.msg, "HOLD SAVE: NEW", 14), "w10: PROJECT NEW asks HOLD SAVE: NEW, BANK KEPT");
            tap(B_SAVE); frames(2);
            check(!ui.ask, "w10: SAVE tapped: no (the question goes)");
            go_home(); frames(2);
        }
        {   /* storing stays SAVE's: hold SAVE + key 2 (B, used): asks, the ring at once as SAVE is held */
            uint32_t st = sec_stores;
            press(B_SAVE); frames(12);
            ui.force = 1; frame(); ppm("w10-hold-save");
            check(pm_lm && str_eq(pm_lm, "UNDO") && pm_lp && str_eq(pm_lp, "REDO"), "w10: SAVE held: the legend says - UNDO + REDO");
            key(key_of_white(1)); frames(90);
            check(sec_stores == st + 1u, "w10: SAVE held + key 2: B (used) stored once the ring filled, SAVE kept held");
            release(B_SAVE); frames(2);
            press(B_SAVE); frames(12); key(key_of_white(2)); release(B_SAVE); frames(2);
            check(sec_stores == st + 2u, "w10: SAVE held + key 3: an empty C stored at once");
        }
        /* -- 4. SEQ is RHYTHM -- */
        {
            uint32_t l0 = sec_loads, r;
            int ro = 0;
            press(B_SEQ); frames(12);
            ui.force = 1; frame(); ppm("w10-hold-seq");
            check(bk_shown == 0x1Fu, "w10: SEQ held: the strip draws A B C D BAR on the black keys");
            check(pm_lm && str_eq(pm_lm, "HALVE") && pm_lp && str_eq(pm_lp, "DOUBLE"), "w10: SEQ held: the legend says - HALVE + DOUBLE");
            check(hdr_mchip == 0 && px_count(52, 2, 70, 16, TE_COL[0]) > 20u, "w10: SEQ held: MASTER's chip in the header (SWING, the voice's colour)");
            key(PUNCH_BLACK[1]); frames(2);
            check(sec_loads == l0 + 1u && live_sec == 1, "w10: SEQ held + black key 2 (G#3): pattern B (stopped: loaded)");
            fm1_in.notes |= 1u << key_of_white(5); frames(3);
            for (r = 0; r < NROLL; r++) ro |= roll[r].on;
            fm1_in.notes &= ~(1u << key_of_white(5)); frames(2);
            check(ro, "w10: SEQ held + a white key: it rolls (ROLL is SEQ's)");
            {
                uint32_t pl = song.playing;
                fm1_in.buttons |= BT(B_PLAY); edges_btn |= BT(B_PLAY); frames(3);
                check(fill_held && song.playing == pl, "w10: SEQ held + PLAY held: FILL, the transport untouched");
                tap(B_OCTUP);
                fm1_in.buttons &= ~BT(B_PLAY); frames(3);
                check(fill_lat && !fill_held, "w10: SEQ + PLAY + OCT+: the FILL latched");
                fm1_in.buttons |= BT(B_PLAY); edges_btn |= BT(B_PLAY); frames(2); fm1_in.buttons &= ~BT(B_PLAY); frames(2);
                check(!fill_lat && song.playing == pl, "w10: SEQ + PLAY again: the latched FILL off");
            }
            release(B_SEQ); frames(4);
            press(B_ARP); frames(12);
            fm1_in.notes |= 1u << key_of_white(5); frames(3);
            ro = 0;
            for (r = 0; r < NROLL; r++) ro |= roll[r].on;
            fm1_in.notes &= ~(1u << key_of_white(5)); frames(2);
            check(!ro, "w10: ARP held + a key: no roll (nothing rhythmic on ARP)");
            release(B_ARP); frames(4);
        }
        {   /* the drums: SEQ held + a key RETRIGs; the black keys are the patterns, not rates */
            int ro = 0;
            uint32_t r, d0;
            track_select(TRK_DRUM); go_home(); frames(2);
            d0 = drt_live;
            press(B_SEQ); frames(12);
            fm1_in.notes |= 1u << key_of_white(0); frames(3);
            for (r = 0; r < NROLL; r++) ro |= roll[r].on;
            fm1_in.notes &= ~(1u << key_of_white(0)); frames(2);
            key(PUNCH_BLACK[2]); frames(2);
            check(ro && drt_live == d0, "w10: drums: SEQ held + a key: RETRIG; a black key no rate (it is a pattern)");
            encs[panel.enc[EN_K1]] = 1; frame();
            check(drt_live == d0 + 1u, "w10: drums: SEQ held + KNOB 1: the RETRIG rate");
            release(B_SEQ); frames(4);
            track_select(0); go_home(); frames(2);
        }
        /* -- 5. ARP is NOTES: + OCT- / OCT+ the pattern a semitone -- */
        {
            uint32_t j;
            steps_clear(t); t->p[P_SLEN] = 16;
            t->step[0].n = 1; t->step[0].note[0] = 60; t->step[0].time = ST_NOTE; t->step[0].vel = 100;
            press(B_ARP); frames(12);
            ui.force = 1; frame(); ppm("w10-hold-arp");
            check(pm_lm && str_eq(pm_lm, "SEMITONE") && pm_lp && str_eq(pm_lp, "SEMITONE"), "w10: ARP held: the legend says - SEMITONE +");
            tap(B_OCTUP); tap(B_OCTUP);
            release(B_ARP); frames(2);
            check(t->step[0].note[0] == 62 && song.octave == o0, "w10: ARP + OCT+ twice (a double tap there is two semitones): the pattern +2, no octave");
            press(B_ARP); tap(B_OCTDN); release(B_ARP); frames(2);
            check(t->step[0].note[0] == 61, "w10: ARP + OCT-: a semitone down");
            for (j = 0; j < 1u; j++) (void)j;
        }
        /* -- 6. GLO: MIDI LEARN is a GLO page; the mixer: no fill; SOLO and PAN on the drawn black keys -- */
        {
            uint32_t i2, learn = 0;
            for (i2 = 0; i2 < NPAGES; i2++)
                learn |= PAGES[i2].fam == FAM_GLO && (PAGES[i2].id[0] == V_LEARN || PAGES[i2].id[1] == V_LEARN);
            check(learn, "w10: MIDI LEARN is a GLO page (MIDI 3)");
            press(B_GLO); frames(12);
            fm1_in.notes |= 1u << key_of_white(8); frames(2);
            check(!fill_held && !fill_now, "w10: the mixer: key 9 is no FILL");
            fm1_in.notes &= ~(1u << key_of_white(8)); frames(2);
            tap(B_SCL); frames(2);
            check(mix_pan && bk_shown == 0xFu, "w13: the mixer: SEL its second page (the pans); the strip draws the four voices");
            ui.force = 1; frame(); ppm("w13-mixer-pan");
            tap(B_SCL);
            release(B_GLO); frames(4);
            track_select(TRK_DRUM); frames(2);
            press(B_GLO); frames(12);
            key(PUNCH_BLACK[0]); frames(2);
            check(trk[0].p[P_MUTE] && bk_shown == 0xFu, "w13: the mixer, the drums selected: the same map (F#3 mutes voice 1)");
            ui.force = 1; frame(); ppm("w13-mixer-drums");
            key(PUNCH_BLACK[0]);
            {
                uint32_t dm0 = drum_mute, so0 = song.solo;
                key(PUNCH_BLACK[4]); frames(2);
                check(!fill_held && !fill_arm && song.solo == so0 && drum_mute == dm0, "w13: the mixer: D#4 (not drawn): nothing");
            }
            release(B_GLO); frames(4);
            drum_mute = 0;
            track_select(0); go_home(); frames(2);
        }
        /* -- 7. MASTER with a button held: its pair, relatively; the volume never jumps -- */
        {
            uint32_t mq;
            int16_t sw0, b0;
            master_in(512); master_in(600); frames(2);
            mq = song.master_q12;
            check(mq == (600u * 600u >> 8) || (mq > 1300u && mq < 1450u), "w10: MASTER, nothing held: the volume follows the pot");
            sw0 = t->p[P_SSWING];
            press(B_SEQ); frames(2);
            master_in(700); frames(2);
            check(t->p[P_SSWING] > sw0 && song.master_q12 == mq, "w10: SEQ + MASTER: SWING up, the volume unchanged");
            ui.force = 1; frame(); ppm("w10-master-swing");
            check(!strncmp(ui.msg, "SWING ", 6), "w10: SEQ + MASTER: the readout says SWING n");
            n0 = (uint32_t)t->p[P_SSWING];
            master_in(650); frames(2);
            check(t->p[P_SSWING] < (int16_t)n0 && t->p[P_SSWING] >= sw0, "w10: SEQ + MASTER turned back: SWING down again (relative)");
            master_in(900); frames(2);
            release(B_SEQ); frames(4);
            check(song.master_q12 == mq, "w10: SEQ held while the pot went to 900, then let go: the volume where it was");
            master_in(950); frames(2);
            check(song.master_q12 > mq && song.master_q12 < (950u * 950u >> 8) - 400u,
                  "w10: MASTER after a pair: the volume moves from where it was, no jump to the pot");
            master_in(1023); frames(2);
            check(song.master_q12 == (1023u * 1023u >> 8), "w10: MASTER at its end stop: the volume at its top too (they meet)");
            t->p[P_SSWING] = sw0;
            mq = song.master_q12;
            /* every pair */
            press(B_GLO); frames(2); b0 = song.g[G_BPM];
            master_in(800); frames(2);
            check(song.g[G_BPM] < b0 && song.master_q12 == mq, "w10: GLO + MASTER: TEMPO, relative (down)");
            release(B_GLO); frames(4);
            press(B_EDIT); frames(2); n0 = (uint32_t)t->p[P_LEVEL];
            master_in(600); frames(2);
            check(t->p[P_LEVEL] < (int16_t)n0 && song.master_q12 == mq, "w10: EDIT + MASTER: the voice's LEVEL");
            release(B_EDIT); frames(4);
            press(B_ARP); frames(2); n0 = (uint32_t)(t->p[P_TRANS] + 100);
            master_in(800); frames(2);
            check(t->p[P_TRANS] + 100 > (int32_t)n0 && song.master_q12 == mq, "w10: ARP + MASTER: TRANSP up");
            ui.force = 1; frame(); ppm("w10-master-transp");
            release(B_ARP); frames(4);
            press(B_ENV); frames(2);
            master_in(1000); frames(2);
            check(mac_env[0] > 0 && song.master_q12 == mq, "w10: ENV + MASTER: the amp envelope longer (a scale, nothing stored changed)");
            ui.force = 1; frame(); ppm("w10-hold-env");
            {   /* (review V6) both envelopes: ms under a second, then seconds, the sustain in % */
                char e1[12], e2[12], e3[12], e4[12];
                int okf = env_fmt(P_ATK, &TP[P_ATK], 0, e1) && env_fmt(P_FREL, &TP[P_FREL], 127, e2) &&
                          env_fmt(P_FSUS, &TP[P_FSUS], 127, e3) && env_fmt(P_DEC, &TP[P_DEC], 60, e4) && !env_fmt(P_FCUT, &TP[P_FCUT], 60, e4);
                env_fmt(P_DEC, &TP[P_DEC], 60, e4);
                check(okf && strstr(e1, "MS") && e2[strlen(e2) - 1] == 'S' && !strstr(e2, "MS") && !strcmp(e3, "100%") && strlen(e4) <= 6u,
                      "V6: envelope times in one format with units (MS, then S), sustain in %");
            }
            check(px_count(0, 22, 240, 17, C_WHITE) < 60u, "V4: ENV + MASTER: no white bar over the big MASTER readout (said once)");
            check(pm_lm == 0 && pm_lp == 0, "w10: ENV held: no -/+ (the MASTER pair only)");
            release(B_ENV); frames(4);
            press(B_LFO); frames(2);
            master_in(500); frames(2);
            check(mac_dep[0] < 0 && song.master_q12 == mq, "w10: LFO + MASTER: the modulation depth down");
            ui.force = 1; frame(); ppm("w10-hold-lfo");
            release(B_LFO); frames(4);
            press(B_FX); frames(2);
            master_in(200); frames(2);
            check(fx_wet < 100u && song.master_q12 == mq, "w10: FX + MASTER: FX MIX down");
            ui.force = 1; frame(); ppm("w10-master-fxmix");
            master_in(0); frames(2);
            check(fx_wet == 0u, "w10: FX + MASTER to its end stop: FX MIX 0");
            release(B_FX); frames(4);
            check(song.master_q12 == mq, "w10: FX let go with the pot at 0: the volume still at the top (no jump)");
            master_in(500); frames(2);
            check(song.master_q12 == mq, "w10: MASTER alone, turned up: the volume already at its top stays there");
            master_in(250); frames(2);
            check(song.master_q12 > 900u && song.master_q12 < 1150u, "w10: ... turned down half way: the volume half way (they meet again)");
            fx_wet = 100; mac_env[0] = 0; mac_dep[0] = 0; frames(2);
        }
        /* -- 8. black keys only where drawn -- */
        {
            uint32_t pg0, j, any = 0;
            tap(B_SEQ); frames(2);
            t->p[P_SLEN] = 32; pg0 = sg_page();
            key(PUNCH_BLACK[1]); frames(2);
            check(sg_page() == pg0, "w10: GRID: a black key (not drawn) turns no page");
            go_home(); track_select(TRK_DRUM); go_home(); frames(2);
            memset((void *)pad_lit, 0, sizeof pad_lit);
            fm1_in.notes |= 1u << PUNCH_BLACK[0]; frames(2);
            for (j = 0; j < 16u; j++) any |= pad_lit[j];
            fm1_in.notes &= ~(1u << PUNCH_BLACK[0]); frames(2);
            check(!any, "w10: drums on HOME: a black key plays nothing (it played the kick)");
            track_select(0); go_home(); frames(2);
            t->p[P_CHORD] = 1;
            fm1_in.notes |= 1u << PUNCH_BLACK[0]; frames(2);
            check(kb_kind[PUNCH_BLACK[0]] != KS_MOD, "w10: chord mode on HOME: F#3 is no chord modifier (not drawn there)");
            fm1_in.notes &= ~(1u << PUNCH_BLACK[0]); frames(2);
            tap(B_ARP); tap(B_ARP); frames(2);
            check(cur_page()->cat == CT_KEY, "w10: ARP twice: KEY");
            fm1_in.notes |= 1u << PUNCH_BLACK[0]; frames(2);
            check(kb_kind[PUNCH_BLACK[0]] == KS_MOD, "w10: chord mode on ARP > KEY: F#3 the 3RD modifier (drawn in the strip)");
            ui.force = 1; frame(); ppm("w10-key-chord");
            check(px_count(2, BK_Y, 37, 12, C_WHITE) > 100u, "w10: KEY page: the held modifier lit in the strip");
            fm1_in.notes &= ~(1u << PUNCH_BLACK[0]); frames(2);
            t->p[P_CHORD] = 0;
            go_home(); frames(2);
        }
        /* -- 9. every hold shows its jobs; HOME, SEL, REC held: none -- */
        {
            static const uint8_t HB[7] = {B_FX, B_SEQ, B_ARP, B_GLO, B_SAVE, B_EDIT, B_ENV};
            uint32_t j;
            for (j = 0; j < 7u; j++) {
                press(HB[j]); frames(12);
                ui.force = 1; frame();
                check(ui.layer != LY_PLAY, HB[j] == B_FX ? "w10: FX held: a layer shows (PERFORM)" : HB[j] == B_SEQ ? "w10: SEQ held: its layer shows" :
                      HB[j] == B_ARP ? "w10: ARP held: its layer shows" : HB[j] == B_GLO ? "w10: GLO held: the mixer shows" :
                      HB[j] == B_SAVE ? "w10: SAVE held: its layer shows" : HB[j] == B_EDIT ? "w10: EDIT held: its layer (MASTER LEVEL, - CLEAR)" :
                      "w10: ENV held: its layer (MASTER LENGTH)");
                if (HB[j] != B_SAVE)
                    check(px_count(52, 2, 70, 16, C_BLACK) < 70u * 16u - 40u, "w10: ... the header names MASTER's pair (its chip)");
                release(HB[j]); frames(4);
            }
            press(B_HOME); frames(60);
            check(ui.layer == LY_PLAY && !ui.menu, "w10: HOME held: nothing (the menu is GLO's DEVICE only)");
            release(B_HOME); frames(4);
            press(B_SCL); frames(30);
            check(ui.layer == LY_PLAY, "w10: SEL held: no layer");
            release(B_SCL); frames(4);
            go_home(); frames(2);
        }
        /* -- column overflow and the colour rule: the tab row's names never run into the legend; the legend and the
         *    strip white and grey only -- */
        {
            uint32_t j, bad = 0, col = 0, x, y;
            track_select(TRK_DRUM); tap(B_FX); frames(2);
            ui.force = 1; frame(); ppm("w10-drum-fx-tabs");
            for (y = 131; y < 154; y++)                 /* (the legend's zone, right of the names: white and grey only) */
                for (x = 236 - 60; x < 236; x++) {
                    uint16_t c = swap16(screen[y * 240u + x]);
                    col += c == TE_COL[0] || c == TE_COL[1] || c == TE_COL[2];
                }
            check(nav_tabs_fit, "w10: the drum FX tab row fits beside the legend (fixed labels, none run over)");
            {   /* (review V1, Ben 26 Nov 2026) one fixed short label a category, the spec's words */
                static const struct { uint8_t c; const char *n; } FL[] = {{CT_PROJ, "PROJ"}, {CT_PRESETS, "PRST"}, {CT_USER, "USER"},
                    {CT_KITS, "KITS"}, {CT_SPATT, "PATT"}, {CT_TOOLS, "TOOL"}, {CT_GLOBAL, "GLOB"}, {CT_SYSTEM, "SYS"},
                    {CT_MIDI, "MIDI"}, {CT_DEVICE, "DEV"}, {CT_INS, "INS"}, {CT_SENDS, "SEND"}, {CT_SLICER, "SLIC"},
                    {CT_MIX, "MIX"}, {CT_STUT, "STUT"}};
                uint32_t q, okl = 1;
                for (q = 0; q < sizeof FL / sizeof FL[0]; q++)
                    okl &= CT_NAME[FL[q].c] && !strcmp(CT_NAME[FL[q].c], FL[q].n);
                for (q = 0; q < CT_N; q++)
                    okl &= !CT_NAME[q] || strlen(CT_NAME[q]) <= 5u;
                check(okl, "V1: tab labels fixed: PROJ PRST USER KITS PATT TOOL; GLOB SYS MIDI DEV; INS SEND SLIC | MIX STUT");
            }
            check(!col, "w10: colour rule: the -/+ legend has no voice colour");
            for (j = 0; j < 4u; j++) {
                track_select(j); go_home(); frames(2);
                tap(B_FX); frames(2); ui.force = 1; frame(); bad += !nav_tabs_fit;
                tap(B_SEQ); frames(2); ui.force = 1; frame(); bad += !nav_tabs_fit;
                tap(B_SEQ); frames(2); ui.force = 1; frame(); bad += !nav_tabs_fit;
                tap(B_SEQ); frames(2); ui.force = 1; frame(); bad += !nav_tabs_fit;
                tap(B_LFO); frames(2); ui.force = 1; frame(); bad += !nav_tabs_fit;
                tap(B_GLO); frames(2); ui.force = 1; frame(); bad += !nav_tabs_fit;
            }
            check(!bad, "w10: every tab row (FX, SEQ, LFO, GLO on each voice, the GRID's too) fits beside its legend");
            track_select(TRK_DRUM); go_home(); frames(2);
            press(B_GLO); frames(12); ui.force = 1; frame();
            col = 0;                                    /* (wave 13: the mixer's strip holds the four voices: each cell its
                                                         * own voice's colour only, nothing else in the strip) */
            for (y = BK_Y; y < BK_Y + 32u; y++)
                for (x = 0; x < 240u; x++) {
                    uint16_t c = swap16(screen[y * 240u + x]);
                    uint32_t v;
                    for (v = 0; v < 4u; v++)
                        col += (c == TE_COL[v] || c == TE_MID[v]) && (x < 1u + 60u * v || x >= 59u + 60u * v);
                }
            check(!col && px_count(1, BK_Y, 58, 32, TE_COL[0]) > 20u && px_count(181, BK_Y, 58, 32, TE_COL[3]) > 20u,
                  "w13: colour rule: the mixer's strip: each voice's colour only in its own cell (V1 purple .. DRUM teal)");
            release(B_GLO); frames(4);
            track_select(0); go_home(); frames(2);
        }
        /* -- the sound: FX MIX at 0 is the dry mix exactly; ENV LENGTH and MOD DEPTH back at 0: as stored -- */
        {
            uint32_t h[3] = {0, 1, 2}, r;
            for (r = 0; r < 3u; r++) {
                int fd[2];
                pid_t pid;
                if (pipe(fd)) break;
                pid = fork();
                if (!pid) {
                    uint32_t hh = 2166136261u, q, j;
                    static int32_t o3[CTL * 2];
                    close(fd[0]);
                    steps_clear(t); t->p[P_SLEN] = 16;
                    for (j = 0; j < 16u; j += 2u) { t->step[j].n = 1; t->step[j].note[0] = (uint8_t)(48 + j); t->step[j].time = ST_NOTE; t->step[j].vel = 100; }
                    for (j = 0; j < NPART; j++) trk[j].p[P_DLY] = trk[j].p[P_REV] = trk[j].p[P_CHOR] = 0;
                    for (j = 0; j < DRUM_LANES; j++) pkit.s[j].send = 0;
                    song.g[G_DRDLY] = song.g[G_DRREV] = 0; TDRUM->p[P_DLY] = TDRUM->p[P_REV] = TDRUM->p[P_CHOR] = 0;
                    perf_unlatch(); dpat_clear(&dpat);
                    memset(&stl, 0, sizeof stl); stl.len = 16;
                    if (r != 1u) { t->p[P_DLY] = 110; t->p[P_REV] = 110; t->p[P_CHOR] = 90; }
                    fx_wet = r == 2u ? 100u : 0u;           /* (0: sends on, FX MIX 0; 1: no send at all; 2: sends on, 100) */
                    for (q = 0; q < 40u; q++) mix_block(o3, CTL);   /* (the return's ease settled) */
                    transport_req = 1;
                    for (q = 0; q < 900u; q++) {
                        mix_block(o3, CTL);
                        if (q == 0u) ui_input();
                        for (j = 0; j < CTL * 2u; j++) hh = (hh ^ (uint32_t)o3[j]) * 16777619u;
                    }
                    if (write(fd[1], &hh, sizeof hh) != (ssize_t)sizeof hh) _exit(1);
                    _exit(0);
                }
                close(fd[1]);
                if (read(fd[0], &h[r], sizeof h[r]) != (ssize_t)sizeof h[r]) h[r] = r + 7u;
                close(fd[0]);
                waitpid(pid, 0, 0);
            }
            check(h[0] == h[1] && h[0] != h[2], "w10: FX + MASTER at 0 (FX MIX 0 %): the output is the dry mix exactly (the sends change nothing; at 100 % they do)");
        }
        {
            uint32_t k0 = 0, k1 = 0, k2 = 0;
            mac_env[0] = 0; esync_coef(t); k0 = esync_k[0][2];
            mac_env[0] = 32; esync_coef(t); k1 = esync_k[0][2];
            mac_env[0] = 0; esync_coef(t); k2 = esync_k[0][2];
            check(k1 < k0 && k1 * 2u > k0 - k0 / 8u && k1 * 2u < k0 + k0 / 8u && k2 == k0,
                  "w10: ENV LENGTH +32: the release twice as long; back at 0: exactly as stored");
        }
        #undef PMT
        #undef DTAP
        song.octave = 0; fx_wet = 100;
        go_home(); frames(2);
    }
    {   /* ---- PHEBUS wave 11 (SPEC.md, "23 Nov 2026, 10:41: hold to map", the answers at 10:50): HOLD LFO / ENV TO MAP */
        track_t *t = &trk[0];
        uint32_t pg0, k, nmap = 0, nref = 0, bad = 0;
        #define KT(n, v) do { encs[panel.enc[n]] = (v); frame(); } while (0)
        song.sel = 0; ly_lock = LY_PLAY; go_home(); frames(8);
        fm1_irq_off(); mx_clear_sound(0); t->p[P_LD_FLT] = t->p[P_LD_PIT] = t->p[P_ED_FLT] = t->p[P_LD_AMP] = 0; fm1_irq_on();
        map_lfo2 = map_fenv = 0;
        check(chain_goto(FAM_FLT, "FILT"), "w11: the FILT page");
        pg0 = ui.page;
        press(B_LFO); frames(3);
        check(map_lyr == LY_LFO && ui.page == pg0 && ui.layer == LY_PLAY && map_src() == MS_LFO1,
              "w11: hold LFO on FILT: the page stays (no layer, no other page), mapping LFO 1 (none shown yet)");
        ui.force = 1; frame(); ppm("w11-map-empty");
        KT(EN_K1, 3);
        check(t->p[P_LD_FLT] > 0 && mx_get(0, MS_LFO1, MD_CUT) == t->p[P_LD_FLT],
              "w11: LFO held, CUTOFF turned right: LFO 1 > CUT positive (the matrix's own cell, P_LD_FLT)");
        KT(EN_K2, -4);
        check(mx_get(0, MS_LFO1, MD_RES) < 0, "w11: ... RESO turned left: LFO 1 > RES negative (bipolar)");
        KT(EN_K4, 5);
        check(mx_as_find(0, P_FDRV) >= 0 && mx_get(0, MS_LFO1, (uint32_t)mx_as_find(0, P_FDRV)) > 0,
              "w11: ... DRIVE: an assignable column for DRIVE, LFO 1 on it");
        {
            int16_t e0 = t->p[P_ED_FLT];
            KT(EN_K3, 4);
            check(t->p[P_ED_FLT] == e0 && strstr(ui.msg, "NOT MAPPABLE") != 0,
                  "w11: ... ENV (a modulation amount): refused, and it says so (ENV: NOT MAPPABLE)");
        }
        ui.msg_t = 0; ui.force = 1; frame(); ppm("w11-map-filt");
        check(pm_shown_m && !strcmp(pm_shown_m, "REMOVE") && !pm_shown_p, "w11: mapping: the legend says - REMOVE");
        release(B_LFO); frames(3);
        check(map_lyr == 0 && ui.page == pg0 && ui.layer == LY_PLAY, "w11: LFO let go: the page, as it was (a hold never navigates)");
        check(map_marks(cur_page()) == 0xBu, "w11: outside the hold: CUTOFF, RESO and DRIVE carry the mark (ENV, a fixed cell's own: no)");
        ui.force = 1; frame(); ppm("w11-filt-marks");
        check(chain_goto(FAM_MOD, "MATRIX"), "w11: the MATRIX page");
        {
            uint8_t cols[MD_N];
            uint32_t n = mx_vis(0, cols);
            check(n == MD_NFIX + 2u && cols[MD_NFIX] == (uint8_t)mx_as_find(0, P_FDRV) && cols[MD_NFIX + 1u] >= MD_AS1,
                  "w11: MATRIX: the fixed columns, DRIVE's column, then + (one empty)");
            mx_row = MS_LFO2; mx_col = (int16_t)mx_as_find(0, P_FDRV);
            frames(20);
            KT(EN_K3, 4);
            check(mx_get(0, MS_LFO2, (uint32_t)mx_col) > 0, "w11: MATRIX: LFO2 > DRIVE set on the grid");
            mx_row = MS_LFO1;
        }
        ui.force = 1; frame(); ppm("w11-matrix");
        check(chain_goto(FAM_FLT, "FILT"), "w11: back to FILT");
        map_lfo2 = 1;
        press(B_LFO); frames(3);
        {
            int32_t a, st;
            uint32_t id;
            st = (int32_t)map_knob(cur_page(), 3, MS_LFO2, &a, &id);
            check(map_src() == MS_LFO2 && st == 2 && a > 0, "w11: the grid's LFO2 > DRIVE shows on the FILT page held with LFO 2 (both ways)");
        }
        tap(B_SCL); frames(2);
        check(map_src() == MS_LFO1 && ui.page == pg0 && map_lyr, "w11: SEL while held: LFO 2 > LFO 1 (the page stays)");
        tap(B_SCL); frames(2);
        check(map_src() == MS_LFO2, "w11: SEL again: LFO 2");
        ui.force = 1; frame(); ppm("w11-map-lfo2");
        tap(B_SCL); frames(2);
        while (mx_get(0, MS_LFO1, MD_RES) < 0) KT(EN_K2, 1);
        while (mx_get(0, MS_LFO1, MD_RES) > 0) KT(EN_K2, -1);
        check(mx_get(0, MS_LFO1, MD_RES) == 0 && mx_col_cells(0, MD_RES) == 0, "w11: back to 0: the mapping gone (the cell freed)");
        release(B_LFO); frames(3);
        check(ui.page == pg0, "w11: let go after SEL: no page walked");
        /* the one last shown */
        check(chain_goto(FAM_MOD, "LFO 2"), "w11: LFO 2 shown");
        check(chain_goto(FAM_FLT, "FILT") && map_lfo2 == 1, "w11: ... then FILT: LFO 2 is the one last shown");
        check(chain_goto(FAM_MOD, "LFO 1"), "w11: LFO 1 shown");
        check(chain_goto(FAM_FLT, "FILT") && map_lfo2 == 0, "w11: ... then FILT: LFO 1");
        /* ENV: the amp envelope (last shown) cannot move its own times; the filter's can */
        check(chain_goto(FAM_ENV, "ENV"), "w11: ENV (AMP)");
        press(B_ENV); frames(3);
        check(map_lyr == LY_ENV && map_src() == MS_AENV && ui.layer == LY_PLAY, "w11: hold ENV on AMP: mapping the amp envelope");
        {
            int16_t a0 = t->p[P_ATK];
            KT(EN_K1, 3);
            check(mx_map_get(0, MS_AENV, P_ATK) == 0 && t->p[P_ATK] == a0 && strstr(ui.msg, "ITSELF") != 0,
                  "w11: ... ATTACK from the amp envelope: refused (NOT FROM ITSELF)");
        }
        ui.msg_t = 0;
        tap(B_SCL); frames(2);
        KT(EN_K1, 3);
        check(map_src() == MS_FENV && mx_map_get(0, MS_FENV, P_ATK) > 0, "w11: SEL: the filter envelope; ATTACK from it: mapped");
        ui.force = 1; frame(); ppm("w11-map-env");
        master_in(512); master_in(700); frames(2);
        check(mac_env[0] != 0, "w11: ENV held while mapping: MASTER still ENV LENGTH (wave 10's pair)");
        mac_env[0] = 0;
        release(B_ENV); frames(3);
        /* OCT- removes this source's mappings on this page; SAVE + OCT- brings them back */
        check(chain_goto(FAM_FLT, "FILT"), "w11: FILT again");
        map_lfo2 = 0;
        press(B_LFO); frames(2);
        {
            int16_t f0 = t->p[P_LD_FLT];
            int32_t dr = mx_map_get(0, MS_LFO1, P_FDRV);
            tap(B_OCTDN); frames(2);
            check(f0 && dr && !t->p[P_LD_FLT] && !mx_map_get(0, MS_LFO1, P_FDRV) && mx_map_get(0, MS_LFO2, P_FDRV) > 0,
                  "w11: LFO + OCT-: LFO 1's mappings on FILT gone (CUTOFF, DRIVE); LFO 2's stay");
            release(B_LFO); frames(3);
            press(B_SAVE); frames(2); tap(B_OCTDN); frames(2); release(B_SAVE); frames(4);
            check(t->p[P_LD_FLT] == f0 && mx_map_get(0, MS_LFO1, P_FDRV) == dr, "w11: SAVE + OCT-: they come back (undo)");
        }
        /* every synth page: each knob mappable maps onto its own column, one cell and only one changes; the rest refuse */
        {
            static const char *const TI[][2] = {{"OSC", 0}, {"OSC 2", 0}, {"OSC 3", 0}, {"OSC 4", 0}, {"FILT", 0}, {"FILT 2", 0},
                                                 {"ENV", 0}, {"FILT ENV", 0}, {"FX 1", 0}, {"FX 2", 0}, {"FX 3", 0}, {"SENDS", 0},
                                                 {"SLICER", 0}, {"LFO 1", 0}, {"LFO SYNC", 0}, {"LFO 2", 0}, {"MATRIX", 0}};
            static const uint8_t FA[] = {FAM_OSC, FAM_OSC, FAM_OSC, FAM_OSC, FAM_FLT, FAM_FLT, FAM_ENV, FAM_ENV, FAM_FX, FAM_FX,
                                         FAM_FX, FAM_FX, FAM_FX, FAM_MOD, FAM_MOD, FAM_MOD, FAM_MOD};
            uint32_t i;
            fm1_irq_off(); mx_clear_sound(0); t->p[P_LD_FLT] = t->p[P_LD_PIT] = t->p[P_ED_FLT] = 0;
            xs_type_set(t->p, 0, XT_PHASER); xs_type_set(t->p, 1, XT_DIST); xs_type_set(t->p, 2, XT_CRUSH);
            t->p[P_SLCR] = 1; fm1_irq_on();
            for (i = 0; i < NELEM(FA); i++) {
                if (!chain_goto(FA[i], TI[i][0])) { printf("  (no page %s)\n", TI[i][0]); bad++; continue; }
                press(B_LFO); frames(2);
                if (map_lyr != LY_LFO) { printf("  %s: no map\n", TI[i][0]); bad++; }
                for (k = 0; k < 4u; k++) {
                    int32_t a;
                    uint32_t id, st = map_knob(cur_page(), k, map_src(), &a, &id), j, ch = 0;
                    mxt_t m0 = mxt[0];
                    int16_t p0[P_COUNT];
                    memcpy(p0, t->p, sizeof p0);
                    if (!st)
                        continue;
                    KT(EN_K1 + k, 2);
                    for (j = 0; j < MX_NCELL; j++)
                        ch += m0.c[j].at != mxt[0].c[j].at || m0.c[j].amt != mxt[0].c[j].amt;
                    for (j = 0; j < P_COUNT; j++)
                        ch += p0[j] != t->p[j];
                    if (st == 2u) {
                        nmap++;
                        if (ch != 1u || !mx_map_get(0, map_src(), id)) { printf("  %s knob %u: %u changes\n", TI[i][0], (unsigned)k + 1, (unsigned)ch); bad++; }
                        KT(EN_K1 + k, -40); KT(EN_K1 + k, -40);
                        if (mx_map_get(0, map_src(), id) >= 0) { printf("  %s knob %u: not negative\n", TI[i][0], (unsigned)k + 1); bad++; }
                        fm1_irq_off(); mx_map_set(0, map_src(), id, 0); fm1_irq_on();
                        if (mx_map_get(0, map_src(), id) || mx_used(0)) bad++;
                    } else {
                        nref++;
                        if (ch || !strstr(ui.msg, "NOT")) { printf("  %s knob %u: refused but %u changes\n", TI[i][0], (unsigned)k + 1, (unsigned)ch); bad++; }
                    }
                    ui.msg_t = 0;
                }
                if (i == 8u) { ui.force = 1; frame(); ppm("w11-map-fx1"); }
                if (i == 0u) { ui.force = 1; frame(); ppm("w11-map-osc"); }
                release(B_LFO); frames(3);
            }
            printf("  %u knobs mapped, %u refused\n", (unsigned)nmap, (unsigned)nref);
            check(!bad && nmap >= 20u && nref >= 10u, "w11: every synth page: each mappable knob maps (one cell, its own), each other one refuses visibly");
        }
        /* the limit: 19 cells */
        {
            uint32_t j;
            static const uint8_t IDS[] = {P_ATK, P_DEC, P_SUS, P_REL, P_FATK, P_FDEC, P_FSUS, P_FREL, P_FDRV, P_GLIDE, P_DETUNE,
                                          P_DLY, P_REV, P_CHOR, P_LRATE, P_LFADE, P_L2RATE, P_SLDEPTH, P_X1TIME};
            fm1_irq_off(); mx_clear_sound(0);
            for (j = 0; j < NELEM(IDS); j++) mx_map_set(0, MS_LFO2, IDS[j], 9);
            fm1_irq_on();
            check(chain_goto(FAM_FLT, "FILT"), "w11: FILT for the limit");
            map_lfo2 = 1;
            press(B_LFO); frames(2);
            KT(EN_K2, 3);
            check(!mx_get(0, MS_LFO2, MD_RES) && strstr(ui.msg, "MATRIX FULL") != 0, "w11: the 20th cell: MATRIX FULL: 19 CELLS, said (nothing changed)");
            ui.force = 1; frame(); ppm("w11-map-full");
            release(B_LFO); frames(3);
            fm1_irq_off(); mx_clear_sound(0); fm1_irq_on();
        }
        /* where there is no map: HOME, the GRID, the drums: wave 10's pair layer as it was */
        go_home(); frames(4);
        press(B_LFO); frames(SHOW_MS / 16u + 4u);
        check(!map_lyr && ui.layer == LY_LFO, "w11: hold LFO on HOME: the MASTER pair layer (wave 10), no map");
        release(B_LFO); frames(4);
        {   /* (wave 12, Ben 13:35: "Hold LFO maps on drums too") a drum sound page: LFO held maps onto the slot's knobs */
            uint32_t kd, okc = 1;
            song.sel = TRK_DRUM; kit_set(DRUM_DEFAULT_KIT); drum_lane = 2; drum_screen_open(); dpg_go(3, 0); frames(4);   /* AMP 1/2 */
            press(B_LFO); frames(SHOW_MS / 16u + 4u);
            check(map_lyr == LY_LFO && map_vis && ui.layer != LY_LFO, "w12: hold LFO on a drum sound page (AMP): MAP, the page stays");
            encs[panel.enc[EN_K2]] = 20; frame();       /* DECAY: +20 */
            check(pkit.s[2].lfo.dst == dl_of(DM_SYN, DSP_S0 + DS_DECAY) && pkit.s[2].lfo.dep == 20,
                  "w12: drums, LFO held + KNOB 2 (DECAY): the snare's LFO DEST is DECAY, DEPTH +20");
            ui.force = 1; frame(); ppm("w12-drum-map");
            okc &= px_count(0, 131, 240, 23, TE_DRUM) > 40u && dial_over == 0u;
            for (kd = 0; kd < 240u * 23u; kd++) {           /* the banner: the drums' teal outline, black, grey only */
                uint16_t c = swap16(screen[(131u + kd / 240u) * 240u + kd % 240u]);
                okc &= c == TE_DRUM || c == C_BLACK || c == TE_G1 || c == TE_G3 || c == TE_G4 || c == C_WHITE;
            }
            check(okc, "w12: the drums' MAP view: MAP LFO outlined in teal, its columns fit, the colour rule");
            dpg_go(2, 0); frame();                      /* FLTR: CUTOFF onto */
            encs[panel.enc[EN_K1]] = -10; frame();
            check(pkit.s[2].lfo.dst == DL_CUT && pkit.s[2].lfo.dep == -10 && !strcmp(ui.msg, "LFO > CUT (WAS DECAY)"),
                  "w12: another knob replaces it, said: LFO > CUT (WAS DECAY), -10");
            encs[panel.enc[EN_K4]] = 1; frame();
            check(strstr(ui.msg, "NOT MAPPABLE") != 0 && pkit.s[2].lfo.dst == DL_CUT, "w12: TYPE (a named value): NOT MAPPABLE");
            check(combo_live(HC_LY(LY_LFO), B_OCTDN) && !combo_live(HC_LY(LY_LFO), B_SCL),
                  "w12: lights: OCT- (remove) lit, SEL dark (one LFO a sound)");
            tap(B_OCTDN); frames(2);
            check(pkit.s[2].lfo.dep == 0, "w12: OCT- while held: the mapping off");
            release(B_LFO); frames(4);
            press(B_ENV); frames(SHOW_MS / 16u + 4u);
            check(!map_lyr && ui.layer == LY_ENV, "w12: hold ENV on the drums: MASTER's pair only (no envelope to map)");
            release(B_ENV); frames(4);
            kit_set(DRUM_DEFAULT_KIT);
        }
        song.sel = 0; go_home(); frames(4);
        /* the sound: a mapping made by the hold plays exactly as the same cell set on the grid, and differs from none */
        {
            uint32_t h[3], r;
            fm1_irq_off(); mx_clear_sound(0); fm1_irq_on();
            t->p[P_FDRV] = 30;
            for (r = 0; r < 3u; r++) {
                int fd[2];
                pid_t pid;
                if (pipe(fd)) break;
                pid = fork();
                if (!pid) {
                    uint32_t hh = 2166136261u, q, j;
                    static int32_t o3[CTL * 2];
                    close(fd[0]);
                    if (r == 1u) {                       /* (the hold: FILT, LFO held, DRIVE right) */
                        chain_goto(FAM_FLT, "FILT"); map_lfo2 = 0;
                        press(B_LFO); frames(2); KT(EN_K4, 6); release(B_LFO); frames(2);
                    } else if (r == 2u) {                /* (the grid: the same amount on DRIVE's column) */
                        int32_t a = 0;
                        chain_goto(FAM_FLT, "FILT"); map_lfo2 = 0;
                        press(B_LFO); frames(2); KT(EN_K4, 6); a = mx_map_get(0, MS_LFO1, P_FDRV); release(B_LFO); frames(2);
                        fm1_irq_off(); mx_clear_sound(0); mxt[0].as[0] = P_FDRV + 1u; mx_set(0, MS_LFO1, MD_AS1, a); fm1_irq_on();
                    } else {
                        chain_goto(FAM_FLT, "FILT");
                    }
                    for (j = 0; j < NPART; j++) trk[j].p[P_DLY] = trk[j].p[P_REV] = trk[j].p[P_CHOR] = 0;
                    t->p[P_LRATE] = 100;
                    trk_note_on(t, 48, 110);
                    for (q = 0; q < 600u; q++) {
                        mix_block(o3, CTL);
                        for (j = 0; j < CTL * 2u; j++) hh = (hh ^ (uint32_t)o3[j]) * 16777619u;
                    }
                    if (write(fd[1], &hh, sizeof hh) != (ssize_t)sizeof hh) _exit(1);
                    _exit(0);
                }
                close(fd[1]);
                if (read(fd[0], &h[r], sizeof h[r]) != (ssize_t)sizeof h[r]) h[r] = r + 7u;
                close(fd[0]);
                waitpid(pid, 0, 0);
            }
            check(h[1] == h[2] && h[0] != h[1], "w11: the sound: LFO 1 > DRIVE by the hold = the same cell set on the grid; differs from none");
        }
        #undef KT
        go_home(); frames(2);
    }
    check(TE_COL[0] == RGB(140, 118, 240) && TE_COL[1] == RGB(40, 160, 245) && TE_COL[2] == RGB(255, 128, 108) &&
          TE_DRUM == RGB(20, 196, 170) && TE_DRUM == TE_COL[TRK_DRUM],
          "w11: the colour code (Ben, 11:33): voice 1 purple, 2 blue, 3 coral, the drums teal (one place: TE_COL)");
    {   /* ---- wave 11 (E): voice 3 coral beside the record red: the screens to look at (screenshots for Ben) */
        track_select(2); go_home(); frames(3);
        rec_wait = 1; ui.force = 1; frame(); ppm("w11-v3-rec-armed"); rec_wait = 0; frames(2);
        transport_req = 1; frames(6); song.rec = 1u << 2; ui.force = 1; frames(3); ppm("w11-v3-home-rec");
        chain_goto(FAM_FLT, "FILT"); frames(3); ui.force = 1; frame(); ppm("w11-v3-filt-rec");
        song.rec = 0; transport_req = 2; frames(4);
        go_home(); frames(2); press(B_EDIT); frames(2); tap(B_OCTDN); release(B_EDIT); frames(2);
        press(B_SAVE); frames(40); ui.force = 1; frame(); ppm("w11-v3-confirm-ring"); release(B_SAVE); frames(3);
        tap(B_HOME); ui.ask = 0; ui.msg_t = 0; frames(3);
        track_select(TRK_DRUM); drum_screen_open(); frames(3); ui.force = 1; frame(); ppm("w11-drum-teal");
        track_select(0); go_home(); frames(3);
    }
    {   /* ---- wave 11: the lights sheet (review/wave11/lights.png): the panel under each hold, QUIET NORMAL SHOW */
        char path[512];
        static const struct { const char *n; uint32_t b; } H[] = {{"hold FX (PERFORM)", B_FX}, {"hold SEQ", B_SEQ}, {"hold ARP", B_ARP},
            {"hold EDIT", B_EDIT}, {"hold GLO (mixer)", B_GLO}, {"hold SAVE", B_SAVE}, {"hold ENV (map)", B_ENV}, {"hold LFO (map)", B_LFO},
            {"hold OCT-", B_OCTDN}};
        uint32_t i;
        snprintf(path, sizeof path, "%s/lights.txt", outdir); remove(path);
        lt_pattern(); song.sel = 0; go_home(); frames(3); lights_dump("NORMAL, HOME, nothing held");
        chain_goto(FAM_FLT, "FILT"); frames(3);
        for (i = 0; i < NELEM(H); i++) {
            char n[64];
            press(H[i].b); frames(12);
            snprintf(n, sizeof n, "%s, FILT page", H[i].n); lights_dump(n);
            release(H[i].b); frames(4);
        }
        song.sel = TRK_DRUM; go_home(); frames(2);
        press(B_GLO); frames(12); lights_dump("hold GLO, drums (mixer)"); release(B_GLO); frames(3);
        press(B_OCTDN); frames(12); lights_dump("hold OCT-, drums (ghost keys)"); release(B_OCTDN); frames(3);
        song.sel = 0; go_home(); frames(2); tap(B_SEQ); frames(3);
        fm1_in.notes |= 1u << key_of_white(2); frames(4); lights_dump("a step held (GRID)"); fm1_in.notes = 0; frames(3);
        go_home(); frames(2); press(B_EDIT); frames(2); tap(B_OCTDN); release(B_EDIT); frames(3); lights_dump("a question waiting");
        tap(B_HOME); ui.ask = 0; ui.msg_t = 0; frames(3);
        tap(B_SEQ); frames(3); transport_req = 1; frames(10);
        lights_dump("NORMAL, GRID playing"); frame(); frame(); frame(); lights_dump("NORMAL, GRID playing (a moment later)");
        disco = DISCO_QUIET; frames(3); lights_dump("QUIET, GRID playing");
        press(B_SEQ); frames(12); lights_dump("QUIET, hold SEQ"); release(B_SEQ); frames(3);
        go_home(); disco = DISCO_SHOW; frames(30); lights_dump("SHOW, HOME playing"); frames(7); lights_dump("SHOW, HOME playing (a moment later)");
        disco = DISCO_NORMAL; transport_req = 2; frames(4); go_home(); frames(2);
    }
    {   /* ---- wave 11: the new views keep in their columns and to the colour rule */
        uint32_t over0 = dial_over, bad = 0, v, i;
        static const char *const PG[] = {"FILT", "ENV", "FX 1", "LFO 1", "OSC"};
        static const uint8_t FA[] = {FAM_FLT, FAM_ENV, FAM_FX, FAM_MOD, FAM_OSC};
        for (v = 0; v < NPART; v++) {
            track_select(v); go_home(); frames(2);
            fm1_irq_off(); mx_clear_sound(v); mx_map_set(v, MS_LFO2, P_FDRV, -64); mx_map_set(v, MS_LFO2, P_FRES, 63); fm1_irq_on();
            for (i = 0; i < NELEM(FA); i++) {
                uint32_t o;
                if (!chain_goto(FA[i], PG[i])) { bad++; continue; }
                map_lfo2 = 1;
                press(B_LFO); frames(12); ui.force = 1; frame();
                for (o = 0; o < NPART + 1u; o++)        /* (no other voice's colour, no red) */
                    if (o != v && (o < NPART ? 1 : 0))
                        bad += px_count(0, 0, 240, OP_BAR_Y, TE_COL[o]) != 0u;
                bad += px_count(0, 0, 240, OP_BAR_Y, TE_RED) != 0u;
                release(B_LFO); frames(4);
                ui.force = 1; frame();
                for (o = 0; o < NPART; o++)             /* (the marks: the voice's own colour only) */
                    if (o != v)
                        bad += px_count(0, 0, 240, OP_BAR_Y, TE_COL[o]) != 0u;
            }
            fm1_irq_off(); mx_clear_sound(v); fm1_irq_on();
        }
        check(!bad && dial_over == over0, "w11: MAP view and the marks on every synth voice: in their columns, no other voice's colour, no red");
        track_select(0); go_home(); frames(2);
        {   /* the MATRIX's heads: every column's name in its 32 px, with 19 columns in use */
            uint8_t cols[MD_N];
            uint32_t n, j, wide = 0;
            static const uint8_t IDS[] = {P_ATK, P_DEC, P_SUS, P_REL, P_FATK, P_FDEC, P_FSUS, P_FREL, P_FDRV, P_GLIDE, P_DETUNE,
                                          P_DLY, P_REV, P_CHOR, P_LRATE, P_LFADE, P_L2RATE, P_SLDEPTH, P_X1TIME};
            char nm[8];
            fm1_irq_off(); mx_clear_sound(0);
            for (j = 0; j < NELEM(IDS); j++) mx_map_set(0, MS_LFO2, IDS[j], 9);
            fm1_irq_on();
            n = mx_vis(0, cols);
            for (j = 0; j < n; j++) { mx_col_name(0, cols[j], nm); wide += cv_text_tight(0, 0, nm, 0, 0) > 28; }   /* (V2: FX 1 set tight) */
            chain_goto(FAM_MOD, "MATRIX"); mx_col = cols[n - 1u]; frames(2); ui.force = 1; frame(); ppm("w11-matrix-full");
            check(n == MD_NFIX + 19u && !wide && px_count(0, 0, 240, OP_BAR_Y, TE_COL[1]) + px_count(0, 0, 240, OP_BAR_Y, TE_COL[2]) == 0u,
                  "w11: MATRIX with 19 parameter columns: each name fits its column, the view scrolls to the last, voice 1's colours only");
            fm1_irq_off(); mx_clear_sound(0); fm1_irq_on();
        }
        go_home(); frames(2);
    }
    {   /* ---- wave 11 (D1): storing a pattern has a home under SAVE: SAVE > PATTERN (the hold stays its shortcut) */
        uint32_t s0, j;
        song.sel = 0; go_home(); frames(4);
        for (j = 0; j < 6u && cur_cat() != CT_SPATT; j++) tap(B_SAVE);
        check(cur_cat() == CT_SPATT && j == 5u, "w11: SAVE pressed five times: PROJECT, PRESETS, USER, KITS (wave 12), PATTERN");
        encs[panel.enc[EN_K1]] = 2; frames(3);
        check(spatt_sel == 2u, "w11: SAVE > PATTERN: KNOB 1 picks the pattern (C)");
        ui.force = 1; frame(); ppm("w11-save-pattern");
        s0 = sec_stores;
        encs[panel.enc[EN_K2]] = 1; frames(3);
        check(sec_stores == s0 + 1u && live_sec == 2, "w11: SAVE > PATTERN: KNOB 2 STORE into an empty pattern: stored at once (as SAVE + key 3)");
        encs[panel.enc[EN_K1]] = -5; frames(3);
        encs[panel.enc[EN_K2]] = 1; frames(3);
        check(ui.ask == ASK_STORE && sec_stores == s0 + 1u && strstr(ui.msg, "STORE A") != 0,
              "w11: SAVE > PATTERN: STORE over a stored pattern (A) asks: HOLD SAVE: STORE A");
        ui.force = 1; frame(); ppm("w11-save-pattern-ask");
        press(B_SAVE); frames(HOLD_CLEAR_MS / 16u + 6u); release(B_SAVE); frames(3);
        check(sec_stores == s0 + 2u && live_sec == 0, "w11: ... SAVE held until the ring fills: stored");
        check(pm_shown_m && !strcmp(pm_shown_m, "PATT"), "w11: SAVE > PATTERN: -/+ the pattern before / after");
        go_home(); frames(4);
    }
    {   /* ---- wave 11: the confirm, the black keys and the strip, the DISCO setting */
        uint32_t c, ok, f, i;
        static uint8_t snap[3][FM1_NCOL];
        song.sel = 0; ly_lock = LY_PLAY; go_home(); frames(4);
        lt_pattern();
        press(B_EDIT); frames(2); tap(B_OCTDN); release(B_EDIT); frames(3);
        for (c = 0, ok = ui.ask != 0; c <= 40u; c++)
            ok &= c == B_SAVE ? led_lv[c] != LL_OFF : led_lv[c] == LL_OFF;
        check(ok, "w11: a question waiting: SAVE alone lit (it pulses), every other button and key dark");
        ui.force = 1; frame(); ppm("w11-lights-ask");
        tap(B_HOME); frames(4); ui.ask = 0; ui.msg_t = 0; frames(2);
        /* the black keys' lights are the strip drawn, on every screen that draws one; elsewhere only a note's */
        {
            uint32_t bad = 0, n = 0;
            #define BKCHK(what) do { uint32_t q_, live_ = 0; frames(2); \
                for (q_ = 0; q_ < 11u; q_++) live_ |= (uint32_t)(led_lv[14u + PUNCH_BLACK[q_]] != LL_OFF) << q_; \
                if (bk_drawn_f + 2u >= ui.frame) { n++; if (live_ != bk_drawn) { printf("  %s: lights %03x strip %03x\n", what, live_, bk_drawn); bad++; } } \
                else if (live_) { printf("  %s: black keys lit, no strip\n", what); bad++; } } while (0)
            go_home(); frames(2); BKCHK("HOME, nothing held");
            press(B_SEQ); frames(12); BKCHK("SEQ held"); release(B_SEQ); frames(3);
            press(B_GLO); frames(12); BKCHK("GLO held (voice 1)"); release(B_GLO); frames(3);
            press(B_FX); frames(12); BKCHK("PERFORM"); release(B_FX); frames(3);
            song.sel = TRK_DRUM; go_home(); frames(2);
            press(B_GLO); frames(12); BKCHK("GLO held (drums)"); release(B_GLO); frames(3);
            fm1_in.notes |= 1u << PUNCH_BLACK[2]; BKCHK("the drums, a black key pressed (no job)");
            printf("  lv %u keys %u lk %u\n", led_lv[14 + PUNCH_BLACK[2]], drum_keys, lights_keys); fm1_in.notes = 0; frames(2);
            drum_screen_open(); frames(3); fm1_in.notes |= 1u << PUNCH_BLACK[2]; BKCHK("the drum screen, a black key pressed"); fm1_in.notes = 0; frames(2);
            song.sel = 0; go_home(); frames(2); tap(B_SEQ); frames(3);
            fm1_in.notes |= 1u << PUNCH_BLACK[1]; BKCHK("the GRID, a black key pressed"); fm1_in.notes = 0; frames(2);
            go_home(); frames(2); trk[0].p[P_CHORD] = 1; chain_goto(FAM_SCL, "KEY"); frames(3);
            BKCHK("ARP > KEY, chord mode (the modifiers drawn)");
            trk[0].p[P_CHORD] = 0; go_home(); frames(3);
            #undef BKCHK
            check(!bad && n >= 5u, "w11: the black keys' lights are the strip drawn (SEQ, the mixers, PERFORM, KEY), dark where none is drawn");
        }
        /* QUIET: nothing changes while a pattern plays (HOME, the GRID); SHOW: the keys move; the level persists */
        song.sel = 0; go_home(); frames(2);
        for (i = 0; i < 2u; i++) {
            disco = DISCO_QUIET;
            if (i) { tap(B_SEQ); frames(3); }
            transport_req = 1; frames(6);
            memcpy(snap[0], fm1_led, FM1_NCOL); memcpy(snap[1], fm1_led_dim, FM1_NCOL); memcpy(snap[2], fm1_led_bg, FM1_NCOL);
            for (f = 0, ok = song.playing != 0u; f < 120u; f++) {
                frame();
                ok &= !memcmp(snap[0], fm1_led, FM1_NCOL) && !memcmp(snap[1], fm1_led_dim, FM1_NCOL) && !memcmp(snap[2], fm1_led_bg, FM1_NCOL);
            }
            check(ok, i ? "w11: QUIET, the GRID playing: no light changes in 2 s (no play head, no beat, no blink)"
                        : "w11: QUIET, HOME playing: no light changes in 2 s (no hits, no notes, no beat)");
            transport_req = 2; frames(4); go_home(); frames(2);
        }
        {
            uint32_t sn = 0, sh = 0, ch = 0;
            uint8_t prev[41];
            disco = DISCO_NORMAL; transport_req = 1; frames(4);
            for (f = 0; f < 120u; f++) { frame(); for (c = 14; c <= 40u; c++) sn += led_lv[c] != LL_OFF; }
            disco = DISCO_SHOW; memcpy(prev, led_lv, 41);
            for (f = 0; f < 120u; f++) {
                frame();
                for (c = 14; c <= 40u; c++) sh += led_lv[c] != LL_OFF;
                ch += memcmp(prev, led_lv, 41) != 0;
                memcpy(prev, led_lv, 41);
                if (f == 60u) { ui.force = 1; ppm("w11-show-home"); }
            }
            check(sh > sn * 2u && ch > 10u && fm1_led_bg_ns == LIGHTS_NS[LIGHTS_HIGH],
                  "w11: SHOW, HOME playing: the keys dance (far more lit than NORMAL, changing), every button's backlight HIGH");
            press(B_SEQ); frames(12);
            for (c = 14, ok = 1; c <= 40u; c++)
                ok &= (led_lv[c] == LL_LIT) == (uint32_t)combo_live(led_ctx, c) || (led_lv[c] != LL_OFF) == 0;
            check(ok && led_ctx == HC_LY0 + LY_SEQ, "w11: SHOW: a hold wins, its hints and nothing else");
            release(B_SEQ); frames(3);
            transport_req = 2; frames(4);
        }
        {
            uint32_t w;
            disco = DISCO_SHOW; w = lights_word(); disco = DISCO_NORMAL; lights_from_word(w);
            ok = disco == DISCO_SHOW;
            disco = DISCO_QUIET; w = lights_word(); disco = DISCO_NORMAL; lights_from_word(w);
            ok &= disco == DISCO_QUIET;
            lights_from_word(w & 0x3FFFFFFFu);
            check(ok && disco == DISCO_NORMAL, "w11: DISCO kept in the settings word (SHOW, QUIET read back; a word from before: NORMAL)");
        }
        disco = DISCO_NORMAL; go_home(); frames(4);
    }
    {   /* ---- PHEBUS wave 11 (SPEC.md, "23 Nov 2026, 10:45: the lights"): every control under every hold: lit <=> it does something
         * together with what is held (what it does alone, as PLAY's transport under GLO, does not count); dark <=> nothing */
        static const struct { const char *n; void (*su)(void); uint32_t hc; } LT[] = {
            {"FILT: FX", su_filt, B_FX}, {"FILT: SEQ", su_filt, B_SEQ}, {"FILT: ARP", su_filt, B_ARP}, {"FILT: EDIT", su_filt, B_EDIT},
            {"FILT: GLO", su_filt, B_GLO}, {"FILT: SAVE", su_filt, B_SAVE}, {"FILT: ENV (map)", su_filt, B_ENV},
            {"FILT: LFO (map)", su_filt, B_LFO}, {"LFO 2: LFO (map)", su_lfo2, B_LFO}, {"FILT: OCT-", su_filt, B_OCTDN},
            {"FILT: OCT+", su_filt, B_OCTUP}, {"FILT: a key (C4) held", su_filt, 14u + 7u},
            {"HOME: LFO (pair)", su_home, B_LFO}, {"HOME: ENV (pair)", su_home, B_ENV}, {"HOME: SEQ", su_home, B_SEQ},
            {"HOME, drums: SEQ", su_home_drum, B_SEQ}, {"HOME, drums: GLO", su_home_drum, B_GLO},
            {"HOME, drums: ARP", su_home_drum, B_ARP}, {"HOME, drums: OCT-", su_home_drum, B_OCTDN},
            {"HOME, drums: OCT+", su_home_drum, B_OCTUP}, {"HOME, drums: FX", su_home_drum, B_FX},
            {"HOME, drums: a key held", su_home_drum, 14u + 2u},
            {"PERFORM, a pad latched: FX", su_perf_lat, B_FX},
            {"GRID: a step held", su_grid, 14u + 2u}, {"drum screen: a step held", su_dscreen, 14u + 2u},
            {"STUT (v1): FX", su_lane, B_FX}, {"STUT (v1): SEQ", su_lane, B_SEQ}, {"STUT (v1): ARP", su_lane, B_ARP},
            {"STUT (v1): EDIT", su_lane, B_EDIT}, {"STUT (v1): GLO", su_lane, B_GLO}, {"STUT (v1): SAVE", su_lane, B_SAVE},
            {"STUT (v1): ENV", su_lane, B_ENV}, {"STUT (v1): LFO", su_lane, B_LFO}, {"STUT (v1): OCT-", su_lane, B_OCTDN},
            {"STUT (v1): OCT+", su_lane, B_OCTUP}, {"STUT (v1): a lane step held", su_lane_s, 14u + 2u},
            {"STUT (drums): SEQ", su_lane_drum, B_SEQ}, {"STUT (drums): ARP", su_lane_drum, B_ARP},
        };
        uint32_t i, c;
        static uint32_t litset[NELEM(LT)][2];
        lt_bad = 0;
        for (i = 0; i < NELEM(LT); i++) {
            char b[200];
            uint32_t n = lt_check(LT[i].n, LT[i].su, LT[i].hc, litset[i]), o = 0;
            o += (uint32_t)snprintf(b + o, sizeof b - o, "  lit: %s:", LT[i].n);
            for (c = 0; c <= 40u && o < 180u; c++)
                if ((litset[i][c >> 5] >> (c & 31u)) & 1u) {
                    if (c < 14u) o += (uint32_t)snprintf(b + o, sizeof b - o, " %s", B_NAME[c]);
                    else o += (uint32_t)snprintf(b + o, sizeof b - o, " k%u", (unsigned)(c - 14u));
                }
            printf("%s (%u)\n", b, (unsigned)n);
        }
        check(!lt_bad, "w11, w14: the lights under every hold (38 holds x 40 controls, 13 on FX's STUT): each lit control does something with it, each dark one nothing");
        go_home(); frames(4);
    }
    {   /* ---- wave 12 (Ben, 23 Nov 2026, 11:02 and 11:10): sounds on the drum screen, kits on HOME, SAVE > KITS */
        dslot_t s606;
        uint32_t ok, px, k;
        char nm[24];
        song.sel = TRK_DRUM; kit_set(DRUM_DEFAULT_KIT); dpat_clear(&dpat); drum_lane = 2;
        transport_req = 2; frames(2);
        drum_screen_open(); dpg_go(1, 0); frames(4);
        s606 = pkit.s[2];
        encs[panel.enc[EN_PRESET]] = 1; frame();
        slot_name(&pkit.s[2], nm, sizeof nm);
        check(str_eq(nm, "80S SNARE") && str_eq(brw_l1, "SOUND 80S SNARE") && brw_t && !ui.ask,
              "w12: drum screen, PRESETS +1: the SNARE slot takes the next snare at once (SOUND 80S SNARE), nothing asked");
        check(str_eq(brw_l2, "SNARE 9/41"), "w12: the readout says the group and the place in it (SNARE 9/41)");
        ui.force = 1; frame(); ppm("w12-drum-sound-browse");
        {   /* (review 26 Nov, O2, Ben's picker "Six-row list on drums too") PRESETS turning on the drum screen: the slot's
             * sounds as six rows over the body, as a synth's PRESETS list; the one taken fourth, teal, a white dot; same kind
             * first (the snares round it); the dials stay (no readout saying it again) */
            uint32_t r, oth = 0, o;
            char nb[28];
            px = px_count(54, SG_BODY_Y + 4u + 3u * 17u, 180, 16, TE_DRUM);
            ok = px > 100u && px_count(4, SG_BODY_Y + 4u + 3u * 17u + 6u, 3, 3, C_WHITE) == 9u;
            for (r = 0; r < 6u; r++)
                if (r != 3u) oth += px_count(0, SG_BODY_Y + 4u + r * 17u, 240, 16, TE_DRUM);
            for (o = 0; o < NPART; o++) oth += px_count(0, 0, 240, OP_BAR_Y, TE_COL[o]);
            ok &= !oth && px_count(0, SG_BODY_Y + 106u, 240, 3, C_GRAY) == 0u;   /* (six rows: nothing of a seventh) */
            for (r = 0; r < 6u; r++) {                   /* (each row a snare: same kind first) */
                snd_name_at(2, (brw.pos + r + brw.n - 3u) % brw.n, nb, sizeof nb);
                ok &= strstr(nb, "SNARE") != 0;
            }
            ok &= !brw_draw() && px_count(0, SG_KNOB_Y + 4, 240, 34, TE_DRUM) > 100u;   /* (the dials, teal) */
            check(ok, "O2: drum screen, PRESETS turning: a six-row list over the body (the synth PRESETS list's), the sound taken teal, "
                      "same kind first; the dials stay");
        }
        check(str_len(brw_l1) * 8u + 4u <= 236u, "w12: the readout's name fits the screen (the longest: SOUND HEX GLITCH SNARE 2)");
        encs[panel.enc[EN_PRESET]] = -1; frame();
        check(!memcmp(&pkit.s[2], &s606, sizeof s606) && !kit_changed(), "w12: PRESETS back one: the 606 SNARE again, the kit unchanged");
        encs[panel.enc[EN_PRESET]] = -9; frame();
        check(str_eq(brw_l1, "SOUND INIT HEX") && sl_mach(2) == DM_HEX && kit_changed(),
              "w12: back to the list's start: INIT HEX, the HEX machine in the 606's snare slot");
        drum_on(LANE_NOTE[2], 100);
        for (k = 0, ok = 0; k < NDRUM; k++) ok |= drums.v[k].active && drums.lane[k] == 2u && drums.synth[k] == 2u;
        check(ok, "w12: the slot plays with its own machine (HEX) in a CLASSIC kit");
        ui.force = 1; frame(); ppm("w12-drum-init-hex");
        press(B_SAVE); tap(B_OCTDN); release(B_SAVE); frames(2);
        check(!memcmp(&pkit.s[2], &s606, sizeof s606), "w12: SAVE + OCT- undoes the whole turn: the 606 SNARE back");
        press(B_SAVE); tap(B_OCTUP); release(B_SAVE); frames(2);
        check(sl_mach(2) == DM_HEX, "w12: SAVE + OCT+ redoes it");
        press(B_SAVE); tap(B_OCTDN); release(B_SAVE); frames(2);
        /* HOME: whole kits; unchanged: at once; changed: it asks */
        go_home(); frames(4);
        encs[panel.enc[EN_PRESET]] = 1; frame();
        check(pkit.base == kit_src(8) && !ui.ask && str_eq(brw_l1, "KIT 80S") && str_eq(brw_l2, "CLASSIC KITS 4/32"),
              "w12: HOME, PRESETS +1 over an unchanged kit: the 80S at once, all 16 slots (KIT 80S, CLASSIC KITS 4/32)");
        ui.force = 1; frame(); ppm("w12-home-kit-browse");
        drum_param_set(DSP_S0 + DS_DECAY, 20); frames(2);
        home_kit_name(nm);
        check(str_eq(nm, "CUSTOM"), "w12: a slot edited: HOME's DRUM zone says CUSTOM");
        ui.force = 1; frames(100); ppm("w12-home-custom");
        encs[panel.enc[EN_PRESET]] = 1; frame();
        check(ui.ask == ASK_KIT && str_eq(ui.msg, "HOLD SAVE: LOAD VINTAGE") && pkit.base == kit_src(8) && kit_changed(),
              "w12: PRESETS over a changed kit asks (HOLD SAVE: LOAD VINTAGE), nothing loaded yet");
        ui.force = 1; frame(); ppm("w12-home-kit-ask");
        press(B_SAVE); frames(100); release(B_SAVE); frames(2);
        check(pkit.base == kit_src(9) && !kit_changed(), "w12: SAVE held until the ring fills: the VINTAGE kit loads");
        encs[panel.enc[EN_PRESET]] = 1; frame();
        check(pkit.base == kit_src(10) && !ui.ask, "w12: and on: unchanged, so no question");
        press(B_SAVE); tap(B_OCTDN); release(B_SAVE); frames(2);
        check(pkit.base == kit_src(8) && kit_changed() && pkit.s[2].snd[7] == 20u, "w12: SAVE + OCT-: the changed kit back, its edit in it");
        /* SAVE > KITS */
        frames(40);                                       /* (the knobs wait a moment after a layer: #39) */
        tap(B_SAVE); tap(B_SAVE); frames(40);
        check(cur_cat() == CT_KITS, "w12: SAVE twice: KITS (after PROJECT; U6: PRESETS is KITS on the drums, U7: no USER)");
        {   /* (review 26 Nov, U7, Ben's picker "Skip USER on drums") SAVE's walk on the drum voice leaves USER out; a synth
             * voice's keeps it */
            uint32_t j2, user = 0, ng2;
            nav_group(B_SAVE, &ng2);
            for (j2 = 0; j2 < ng2 + 1u; j2++) { tap(B_SAVE); frames(2); user += cur_cat() == CT_USER; }
            check(!user && is_drum(TSEL), "U7: SAVE round on the drum voice: never USER");
            track_select(0); frames(2);
            nav_group(B_SAVE, &ng2);
            for (j2 = 0; j2 < ng2 + 1u; j2++) { tap(B_SAVE); frames(2); user += cur_cat() == CT_USER; }
            check(user >= 1u, "U7: ... a synth voice's SAVE still has USER");
            track_select(TRK_DRUM); frames(2);
            go_home(); frames(40); tap(B_SAVE); tap(B_SAVE); frames(40);
            ui.force = 1; frame(); ppm("u7-drum-save-kits");
            {   /* (review 26 Nov, U6, Ben's picker "One home each") on the drums SAVE > PRESETS points to KITS: one place
                 * under SAVE for whole kits; a synth voice's PRESETS (or USER) with the drums then selected: KITS */
                uint32_t j3, pres = 0, ng3, okk = 1, pp = NPAGES;
                const uint8_t *g3 = nav_group(B_SAVE, &ng3);
                for (j3 = 0; j3 < ng3; j3++) pres += g3[j3] == CT_PRESETS || g3[j3] == CT_USER;
                for (j3 = 0; j3 < NPAGES; j3++) if (PAGES[j3].cat == CT_PRESETS) pp = j3;
                nav_go(CT_PRESETS, 0); frames(2); okk &= cur_cat() == CT_KITS;
                track_select(0); frames(2); nav_go(CT_PRESETS, 0); frames(2); okk &= ui.page == pp;
                track_select(TRK_DRUM); frames(2); okk &= cur_cat() == CT_KITS;
                track_select(0); frames(2); nav_go(CT_USER, 0); frames(2); okk &= cur_cat() == CT_USER;
                track_select(TRK_DRUM); frames(2); okk &= cur_cat() == CT_KITS;
                check(!pres && okk, "U6: the drums' SAVE: PROJECT KITS PATTERNS TOOLS; PRESETS points to KITS (from a synth voice's PRESETS or USER too)");
            }
        }
        ui.force = 1; frame(); ppm("w12-save-kits");
        encs[panel.enc[EN_K4]] = 1; frame();
        check(ui.ask == ASK_UKIT && str_eq(ui.msg, "HOLD SAVE: SAVE OVER MY HEX 1"), "w12: SAVE over an old MY HEX kit asks");
        ask_drop();
        encs[panel.enc[EN_K1]] = 4; frame();
        encs[panel.enc[EN_K4]] = 1; frame();
        check(ukit_state(4) == UK_STORED && !ui.ask && !kit_changed() && pkit.base == DSRC_USER + 4u,
              "w12: SAVE into an empty kit: stored at once (MY KIT 5), the kit now that one");
        kit_set(DRUM_DEFAULT_KIT); frames(2);
        encs[panel.enc[EN_K2]] = 1; frame();
        check(pkit.base == DSRC_USER + 4u && pkit.s[2].snd[7] == 20u, "w12: LOAD over an unchanged kit: MY KIT 5 at once");
        ui.force = 1; frame(); ppm("w12-save-kits-stored");
        {   /* (Ben, 26 Nov 2026: "Make it say "This will stop the audio" - like Ableton does") a user kit while playing */
            encs[panel.enc[EN_K1]] = 1; frame();           /* MY KIT 6: empty */
            transport_req = 1; frames(3);
            check(ukit_sel == 5u && !ukit_has(5) && song.playing, "E3: (MY KIT 6 empty, playing)");
            encs[panel.enc[EN_K4]] = 1; frame();
            check(ui.ask == ASK_UKIT && str_eq(ui.msg, "HOLD SAVE: STOPS THE AUDIO") && ask_keeps && song.playing &&
                  ukit_state(5) != UK_STORED, "E3: SAVE into an empty kit while playing asks (HOLD SAVE: STOPS THE AUDIO, white), nothing written");
            ui.force = 1; frame(); ppm("e3-kit-stop-ask");
            press(B_SAVE); frames(30); release(B_SAVE); frames(2);
            check(song.playing && ukit_state(5) != UK_STORED, "E3: let go before the ring fills: still playing, nothing written");
            press(B_SAVE); frames(20); ui.force = 1; frame(); ppm("e3-kit-stop-hold");
            frames(80); release(B_SAVE); frames(3);
            check(!song.playing && !transport_req && ukit_state(5) == UK_STORED && pkit.base == DSRC_USER + 5u && !kit_changed(),
                  "E3: held until the ring fills: the transport stops, then MY KIT 6 is written");
            transport_req = 1; frames(3);
            encs[panel.enc[EN_K4]] = 1; frame();
            check(ui.ask == ASK_UKIT && (ui.ask_arg & UK_ASK_STOP) && !ask_keeps && str_eq(ui.msg, "HOLD SAVE: STOPS THE AUDIO"),
                  "E3: SAVE over a stored kit while playing: the same question, red (it replaces)");
            ask_drop(); frame();
            encs[panel.enc[EN_K3]] = 1; frame();
            check(ui.ask == ASK_UKIT && (ui.ask_arg & UK_ASK_STOP) && (ui.ask_arg & 3u) == UKOP_ERASE && song.playing,
                  "E3: ERASE while playing: asks the same");
            press(B_SAVE); frames(100); release(B_SAVE); frames(3);
            check(!song.playing && ukit_state(5) == UK_ERASED, "E3: held: stopped, then erased");
            encs[panel.enc[EN_K1]] = -1; frame();
        }
        ukit_put(4, 0);
        kit_set(DRUM_DEFAULT_KIT); song.sel = 0; ui.msg_t = 0; go_home(); frames(4);
    }
    {   /* ---- wave 12 (Ben 13:35: "QUIET also dims the screen"): QUIET dims every band, NORMAL brings it back, still readable */
        uint32_t k, maxq = 0, maxn = 0;
        uint8_t d0 = disco;
        go_home(); frames(4);
        disco = DISCO_QUIET; frames(2); ui.force = 1; frame();
        for (k = 0; k < 240u * 240u; k++) { uint32_t c = swap16(screen[k]); maxq = (c >> 11) > maxq ? (c >> 11) : maxq; }
        ppm("w12-quiet-dim");
        disco = DISCO_NORMAL; frames(2); ui.force = 1; frame();
        for (k = 0; k < 240u * 240u; k++) { uint32_t c = swap16(screen[k]); maxn = (c >> 11) > maxn ? (c >> 11) : maxn; }
        check(scr_dim == 0u && maxn == 31u && maxq <= 18u && maxq >= 14u,
              "w12: DISCO QUIET draws the screen at 9/16 (white 31 -> 17 of 31: dim, readable); NORMAL: full again");
        disco = d0; frames(2);
    }
    {   /* ---- wave 14 (SPEC.md, "23 Nov 2026, 15:44" and the answers at 15:55): the stutter sequence on FX. "Both on FX ... but
         * don't walk the PERFORM page. The perform page is only in the long press for FX button"; FX opens on the voice's
         * effects first. FX steps INS SENDS SLICER (drums FILTER INS SENDS SLICER), MIX, STUT, round; ALGORITHM picks the four
         * voices only */
        static const uint8_t SYN_FX[6] = {CT_INS, CT_SENDS, CT_SLICER, CT_MIX, CT_STUT, CT_INS};
        static const uint8_t DRM_FX[6] = {CT_DINS, CT_DSENDS, CT_DSLICER, CT_MIX, CT_STUT, CT_DINS};   /* (a synth voice's order) */
        uint32_t v, j, ok, bad, perf_seen = 0, kw4 = key_of_white(4), kb0 = PUNCH_BLACK[0];
        char nm[32];
        memset(&stl, 0, sizeof stl); stl.len = 16; stl.act = 1;
        transport_req = 2; frames(2); song.rec = 0; rec_wait = 0;
        for (v = 0, ok = 1; v < NTRK; v++) {             /* every voice, from HOME: the order, round; PERFORM never by a tap */
            const uint8_t *want = v == TRK_DRUM ? DRM_FX : SYN_FX;
            uint32_t n = 6u;
            go_home(); track_select(v); frames(2);
            for (j = 0; j < n; j++) {
                tap(B_FX); frames(2);
                ok &= cur_cat() == want[j] && song.sel == v && (cur_cat() == CT_STUT) == (stl_shown() != 0);
                perf_seen |= ui.layer == LY_FX || layer_shown || ly_lock == LY_FX;
            }
        }
        check(ok, "w14: FX tapped, every voice: INS SENDS SLICER MIX STUT, round to INS (the drums the same since their effects moved under FX)");
        for (j = 0; j < 24u; j++) { tap(B_FX); frames(1); perf_seen |= ui.layer == LY_FX || layer_shown || ly_lock == LY_FX; }
        check(!perf_seen, "w14: PERFORM is never reached by tapping FX (36 taps over the four voices)");
        go_home(); track_select(0); frames(2);
        nav_go(CT_STUT, 0); frames(2);
        press(B_FX); frames(12);
        check(ui.layer == LY_FX && layer_shown && stl_view, "w14: hold FX on STUT: PERFORM (the lane under it, kept)");
        ui.force = 1; frame(); ppm("w14-stut-hold-fx");
        frames(40); release(B_FX); frames(2);
        check(ly_lock == LY_FX, "w14: hold FX 0.7 s untouched: PERFORM stays open, as before");
        tap(B_FX); frames(2);
        check(ly_lock == LY_PLAY && stl_shown() && cur_cat() == CT_STUT, "w14: FX again lets it go: back on the lane");
        /* ALGORITHM: the four voices only, from every screen, the steps (SEQ) included; never the lane */
        {
            static const uint8_t FROM[6] = {B_HOME, B_EDIT, B_SEQ, B_FX, B_ARP, B_GLO};
            uint32_t f, q, seen = 0, lane = 0;
            for (f = 0, ok = 1; f < 6u; f++) {
                go_home(); track_select(0); frames(2);
                if (FROM[f] != B_HOME) { tap(FROM[f]); frames(2); }
                for (q = 0; q < 6u; q++) {
                    encs[panel.enc[EN_ALGO]] = 1; frames(2);
                    seen |= 1u << song.sel;
                    lane |= stl_view;
                }
                ok &= song.sel == TRK_DRUM;
                for (q = 0; q < 6u; q++) { encs[panel.enc[EN_ALGO]] = -1; frames(2); seen |= 1u << song.sel; lane |= stl_view; }
                ok &= song.sel == 0u;
            }
            check(ok && seen == 15u && !lane, "w14: ALGORITHM from HOME, EDIT, SEQ (the steps), FX, ARP, GLO: 1, 2, 3, DRUM and no further; never the lane");
            go_home(); track_select(0); frames(2); nav_go(CT_STUT, 0); frames(2);
            for (q = 0, ok = 1; q < 5u; q++) { encs[panel.enc[EN_ALGO]] = 1; frames(2); ok &= stl_shown() != 0; }
            check(ok && song.sel == TRK_DRUM, "w14: ALGORITHM on STUT: the voice changes (to DRUM, no further), the lane stays");
        }
        /* the lane, a whole-mix screen: no voice colour above the strip, whichever voice; the voice stays selected */
        for (v = 0, bad = 0, ok = 1; v < NTRK; v++) {
            uint32_t o;
            go_home(); track_select(v); frames(2);
            for (j = 0; j < 8u && cur_cat() != CT_STUT; j++) tap(B_FX);
            ui.force = 1; frames(2);
            for (o = 0; o < NTRK; o++)
                bad += px_count(0, 0, 240, OP_BAR_Y, TE_COL[o]) + px_count(0, 0, 240, OP_BAR_Y, TE_MID[o]) + px_count(0, 0, 240, OP_BAR_Y, TE_DIM[o]);
            ok &= song.sel == v && px_count(0, OP_BAR_Y + 24, 240, 3, TE_COL[v]) > 100u && nav_tabs_fit;   /* (review V1: the legend may drop its word) */
            ok &= px_count(2, 1, 46, 18, TE_G4) > 40u;    /* (the tag MIX, white-grey) */
            snprintf(nm, sizeof nm, "w14-stut-v%u", (unsigned)v + 1u);
            ppm(nm);
            tap(B_FX); frames(2);
            ok &= song.sel == v && !stl_view && cur_cat() == (v == TRK_DRUM ? CT_DINS : CT_INS);
        }
        check(bad == 0u, "w14: STUT on each voice: no voice colour above the strip (tag MIX, white and grey)");
        check(ok, "w14: STUT on each voice: the strip underlines the voice still selected, the tabs fit; FX again: INS, the same voice");
        {   /* the tab rows of FX: voice group, divider, MIX STUT, beside the page and the legend, on every voice and screen */
            uint32_t ok2 = 1, wb = 0;
            for (v = 0; v < NTRK; v++) {
                go_home(); track_select(v); frames(2);
                for (j = 0; j < 6u; j++) {
                    tap(B_FX); frames(2);
                    encs[panel.enc[EN_K3]] = 1; frames(2);   /* (a knob turned: the legend has its word) */
                    ui.force = 1; frame();
                    ok2 &= nav_tabs_fit;
                    wb += nav_tabs_bare;
                    if (v == TRK_DRUM && j == 4u) ppm("w14-drum-mix-tabs");
                    if (v == 0u && j == 3u) ppm("w14-synth-mix-tabs");
                }
                encs[panel.enc[EN_K3]] = -6; frames(2);
            }
            check(ok2, "w14: every FX category's tab row on every voice fits (V1: fixed labels, the legend drops its word first)");
            printf("ui:   (the legend kept its boxes, not its word, on %u of 24 FX rows: the drums' six categories)\n", (unsigned)wb);
        }
        /* its keys, black keys, dials, lights, on voice 1 */
        go_home(); track_select(0); frames(2); nav_go(CT_STUT, 0); frames(2);
        {
            uint32_t s0 = trk[0].step[4].n | trk[0].step[4].time << 8;
            key(kw4); frames(1);
            check(stl_lit(4) && (trk[0].step[4].n | trk[0].step[4].time << 8) == s0, "w14: STUT on voice 1: a white key lights the lane's step 5 (voice 1's steps untouched)");
            frames(2);
            check(led_lv[14u + kw4] == LL_LIT, "w14: ... and its key is lit (the lane's steps on the white keys)");
            fm1_in.notes |= 1u << kb0; frame(); frame();
            ok = perf.held == PFB(PF_S1) && kb_kind[kb0] == KS_FX;
            ppm("w14-stut-black-key");
            fm1_in.notes &= ~(1u << kb0); frame(); frame();
            check(ok && !perf.held, "w14: STUT on voice 1: F#3 plays the 1/1 stutter while held (not a note of voice 1)");
            check(!keys_dead(), "w14: ... every black key live (drawn in the strip): lit by the lights");
            encs[panel.enc[EN_K2]] = -1; frame();
            ok = !stl.act;
            encs[panel.enc[EN_K2]] = 1; frame();
            check(ok && stl.act, "w14: STUT: KNOB 2 the lane OFF and ON (its dials as before)");
            for (j = 0; j < 10u && stl.len < 32u; j++) { encs[panel.enc[EN_K4]] = 1; frame(); }
            tap(B_OCTUP); frames(20);
            check(stl.len == 32u && stl_pg == 1u, "w14: STUT: KNOB 4 its length (32), OCT+ its next page of steps");
            tap(B_OCTDN); frames(20);
            stl.len = 16; stl_pg = 0; frames(2);
        }
        /* hold SEQ, ARP, EDIT on STUT: the selected voice's, as on any FX page; SEQ's halve / double refused (the lane's
         * length is KNOB 4's) */
        {
            int16_t len0 = trk[0].p[P_SLEN];
            uint8_t n0 = trk[0].step[0].note[0];
            lt_pattern(); n0 = trk[0].step[0].note[0]; len0 = trk[0].p[P_SLEN];
            nav_go(CT_STUT, 0); frames(2);
            press(B_ARP); frames(12); tap(B_OCTUP); frames(2); release(B_ARP); frames(2);
            check(trk[0].step[0].note[0] == (uint8_t)(n0 + 1u) && stl_shown(), "w14: STUT, hold ARP + OCT+: voice 1's pattern up a semitone (the voice selected); the lane stays");
            press(B_SAVE); tap(B_OCTDN); release(B_SAVE); frames(2);
            press(B_SEQ); frames(12); tap(B_OCTDN); frames(2); release(B_SEQ); frames(2);
            check(trk[0].p[P_SLEN] == len0 && stl.len == 16u, "w14: STUT, hold SEQ + OCT-: refused (dark), nothing halved");
            frames(20);
            press(B_EDIT); frames(12); tap(B_OCTDN); frames(2); release(B_EDIT); frames(2);
            check(ui.ask && !strcmp(ui.msg, "HOLD SAVE: CLEAR VOICE 1"), "w14: STUT, hold EDIT + OCT-: asks HOLD SAVE: CLEAR VOICE 1 (the voice named)");
            ask_drop(); frames(2);
            check(stl_shown(), "w14: ... the lane still on screen");
        }
        /* REC on STUT: the lane's own take; no voice armed */
        {
            uint32_t s1 = 0;
            go_home(); track_select(0); frames(2); nav_go(CT_STUT, 0); frames(2);
            memset(&stl, 0, sizeof stl); stl.len = 16; stl.act = 1;
            transport_req = 1; frames(4);
            tap(B_REC); frames(2);
            check(song.rec == REC_LANE && !((song.rec >> 0) & 1u), "w14: STUT, playing, REC: recording the lane only (no voice armed)");
            ui.force = 1; frame(); ppm("w14-stut-rec");
            check(px_count(0, OP_BAR_Y, 240, 28, TE_RED) == 0u, "w14: ... the strip shows no voice armed (no red dot)");
            for (j = 0; j < 4u; j++) { s1 |= trk[0].step[j].time; }
            fm1_in.notes |= 1u << kb0; frames(40); fm1_in.notes &= ~(1u << kb0); frames(20);
            check(pev_count() == 1u && (stl.ev[0].fx & 31u) == PF_S1 + 1u, "w14: ... F#3 played: its 1/1 stutter recorded into the lane");
            encs[panel.enc[EN_ALGO]] = 1; frames(2);
            check(song.rec == REC_LANE, "w14: ... ALGORITHM turned: the take stays the lane's (no voice armed by it)");
            tap(B_REC); frames(2);
            check(!song.rec, "w14: REC again: off");
            transport_req = 2; frames(4);
            track_select(0); frames(2);
            tap(B_REC); frames(2);
            check(rec_wait && rec_lane && !song.rec, "w14: STUT, stopped, REC: armed for the lane (the REC screen)");
            ui.force = 1; frame(); ppm("w14-stut-rec-armed");
            check(px_count(0, 78, 240, 76, TE_RED) == 0u, "w14: ... the REC screen frames no voice");
            key(kw4); frames(2);
            check(rec_wait && !song.playing, "w14: ... a white key (a lane step) does not start it");
            tap(B_PLAY); frames(4);
            check(song.playing && song.rec == REC_LANE, "w14: ... PLAY: the lane's take starts");
            tap(B_REC); transport_req = 2; frames(4);
            go_home(); frames(2);
            tap(B_REC); frames(2);
            check(rec_wait && !rec_lane, "w14: REC off the lane (HOME): armed for the selected voice, as before");
            tap(B_REC); frames(2);
            memset(&stl, 0, sizeof stl); stl.len = 16; stl.act = 1;
        }
        /* lane steps held: OCT- erases its pads; a page button does nothing (dark) */
        {
            go_home(); track_select(1); frames(2); nav_go(CT_STUT, 0); frames(2);
            stl.ev[0] = (pev_t){PF_S8 + 1u, 4, 2, 0};
            fm1_in.notes |= 1u << kw4; frame(); frame();
            ui.force = 1; frame(); ppm("w14-stut-v2-step-held");
            tap(B_EDIT); frames(2);
            ok = stl_shown() && cur_cat() == CT_STUT;
            tap(B_OCTDN); frames(2);
            fm1_in.notes &= ~(1u << kw4); frames(2);
            check(ok && !stl.ev[0].fx && song.sel == 1u, "w14: STUT on voice 2: a step held: EDIT does nothing (dark), OCT- erases its pads");
            tap(B_EDIT); frames(2);
            check(!stl_view && cur_cat() == CT_OSC && song.sel == 1u, "w14: EDIT (nothing held): leaves the lane for voice 2's OSC, as from any category");
            nav_go(CT_STUT, 0); frames(2); go_home(); frames(2);
            check(!stl_view, "w14: HOME leaves it too");
        }
        /* HOME's PERFORM zone says where the lane lives */
        {
            go_home(); track_select(0); perf_unlatch(); frames(2); ui.force = 1; frame(); ppm("w14-home-perform");
            check(px_count(170, OP_BODY_Y + HZ_DECK + 2 * HZ_H, 66, 18, TE_G3) > 30u, "w14: HOME's PERFORM zone: FX>STUT at the right while no pad is latched");
        }
        memset(&stl, 0, sizeof stl); stl.len = 16; stl.act = 1;
        go_home(); track_select(0); frames(2);
    }
    {   /* ---- (Ben 22:31 to 22:45) the hold-SAVE layer in plain words; SAVE > PROJECT, the bank's KEEP SWAP NEW */
        static const char *const HINTS[] = {"PRESS KEY 1-4: STORE PATTERN", "IT SAVES ITSELF WHEN YOU STOP",
            "KEEP A COPY: SAVE > PROJECT", "PROJECT: NOT SAVED YET", "PROJECT: STOP TO SAVE",
            "EMPTY: NOTHING KEPT YET", "NEW HITS AT VELOCITY 127", "TAP A KEY: MUTE", "SEL: NEXT SECTION  OCT-: BACK",
            "KNOBS 1-3 SET ROWS  OCT+: OK", "KNOBS 1-4 SET ROWS  SEL: NEXT", "PADS YOU RECORD SHOW HERE", "PLAY: GO, REC: CANCEL",
            "HOLD SAVE: REPLACE THE BANK", "HOLD SAVE: SWAP WITH THE BANK", "HOLD SAVE: NEW, BANK KEPT",
            "THE BANK IS EMPTY: KEEP FIRST", "SWAPPED WITH THE BANK", "NEW PROJECT, BANK KEPT", "SAVE ERROR: NOTHING CHANGED"};
        uint32_t i, ok = 1, v, b0;
        char a1[24], a2[24];
        for (i = 0; i < NELEM(HINTS); i++)
            ok &= 4 + text_w(&FONT_S, HINTS[i]) <= 236 && str_len(HINTS[i]) <= 29u;
        check(ok, "keep: every plain-words line fits its line (232 px, the message's 29 letters)");
        song.playing = 0; transport_req = 0; song.sel = 0; ly_lock = LY_PLAY; go_home(); frames(4);
        proj_unsaved = 0; live_sec = 1; live_req = -1;
        press(B_SAVE); frames(15); ui.force = 1; frame(); ppm("keep-hold-save");
        v = px_count(0, LY_TILES_Y, 240, OP_KNOB_Y - LY_TILES_Y, TE_COL[0]) + px_count(0, LY_TILES_Y, 240, OP_KNOB_Y - LY_TILES_Y, TE_COL[1]) +
            px_count(0, LY_TILES_Y, 240, OP_KNOB_Y - LY_TILES_Y, TE_COL[2]) + px_count(0, LY_TILES_Y, 240, OP_KNOB_Y - LY_TILES_Y, TE_COL[3]) +
            px_count(0, LY_TILES_Y, 240, OP_KNOB_Y - LY_TILES_Y, TE_RED);
        check(ui.layer == LY_SAVE && str_eq(sub_line(), "") && pm_lm && str_eq(pm_lm, "UNDO") && v == 0u &&
              px_count(0, LY_TILES_Y, 240, 16, TE_G4) > 100u && px_count(61, LY_TILES_Y + 18, 58, 3, C_WHITE) == 174u,
              "keep: hold SAVE says PRESS KEY 1-4: STORE PATTERN, -/+ UNDO REDO kept, B marked playing; white and grey only");
        b0 = px_count(0, LY_TILES_Y + 60, 240, 16, C_WHITE);
        proj_unsaved = 1; ui.force = 1; frame(); ppm("keep-hold-save-unsaved");
        check(b0 == 0u && px_count(0, LY_TILES_Y + 60, 240, 16, C_WHITE) > 150u,
              "keep: PROJECT: SAVED in grey; NOT SAVED YET in white (work at risk: not to be missed)");
        release(B_SAVE); frames(3); proj_unsaved = 0;
        go_home(); frames(2); tap(B_SAVE); frames(2);
        bank_has = 0; memset(bank_ops, 0, sizeof bank_ops);
        ui.force = 1; frame(); ppm("keep-project-empty");
        frames(40);                                       /* (the knobs wait a moment after a layer: #39) */
        v = px_count(0, 0, 240, OP_KNOB_Y, TE_COL[0]) + px_count(0, 0, 240, OP_KNOB_Y, TE_COL[1]) +
            px_count(0, 0, 240, OP_KNOB_Y, TE_COL[2]) + px_count(0, 0, 240, OP_KNOB_Y, TE_COL[3]) + px_count(0, 0, 240, OP_KNOB_Y, TE_RED);
        check(cur_cat() == CT_PROJ && v == 0u, "keep: SAVE > PROJECT first, white and grey only (the whole instrument's)");
        encs[panel.enc[EN_K2]] = 1; frames(3);
        check(!ui.ask && bank_ops[1] == 0u && strstr(ui.msg, "BANK IS EMPTY"), "keep: SWAP with an empty bank: THE BANK IS EMPTY: KEEP FIRST");
        ui.msg_t = 0; frames(2);
        encs[panel.enc[EN_K1]] = 1; frames(3);
        check(!ui.ask && bank_ops[0] == 1u && bank_has && strstr(ui.msg, "KEPT IN THE BANK"), "keep: KEEP into an empty bank: at once");
        ui.msg_t = 0; ui.force = 1; frame(); ppm("keep-project-kept");
        encs[panel.enc[EN_K1]] = 1; frames(3);
        check(ui.ask == ASK_BANK && !strcmp(ui.msg, "HOLD SAVE: REPLACE THE BANK") && !ask_keeps && bank_ops[0] == 1u,
              "keep: KEEP over a kept project asks: HOLD SAVE: REPLACE THE BANK (red ring)");
        ui.force = 1; frame(); ppm("keep-project-ask-keep");
        press(B_SAVE); frames(HOLD_CLEAR_MS / 16u + 6u); release(B_SAVE); frames(3);
        check(bank_ops[0] == 2u && !ui.ask, "keep: ... SAVE held until the ring fills: kept");
        encs[panel.enc[EN_K2]] = 1; frames(3);
        check(ui.ask == ASK_BANK && !strcmp(ui.msg, "HOLD SAVE: SWAP WITH THE BANK") && ask_keeps,
              "keep: SWAP asks with a white ring (nothing is lost: SWAP again goes back)");
        press(B_SAVE); frames(40); ui.force = 1; frame(); ppm("keep-project-ask-swap"); frames(HOLD_CLEAR_MS / 16u); release(B_SAVE); frames(3);
        check(bank_ops[1] == 1u && strstr(ui.msg, "SWAPPED"), "keep: ... held: swapped");
        encs[panel.enc[EN_K4]] = 1; frames(3);
        check(ui.ask == ASK_BANK && !strcmp(ui.msg, "HOLD SAVE: NEW, BANK KEPT") && !ask_keeps, "keep: NEW asks: HOLD SAVE: NEW, BANK KEPT");
        ui.force = 1; frame(); ppm("keep-project-ask-new");
        tap(B_SAVE); frames(2);
        song.playing = 1;
        encs[panel.enc[EN_K1]] = 1; frames(3);
        check(!ui.ask && bank_ops[0] == 2u && strstr(ui.msg, "STOP PLAYING FIRST"), "keep: while playing: STOP PLAYING FIRST, nothing done");
        song.playing = 0; ui.msg_t = 0;
        proj_text(a1, a2, 2, 120, "CUSTOM");
        check(str_eq(a1, "2 PATTERNS  120 BPM") && str_eq(a2, "KIT: CUSTOM"), "keep: a summary: 2 PATTERNS  120 BPM / KIT: CUSTOM");
        proj_text(a1, a2, 0, 90, "606");
        check(str_eq(a1, "NO PATTERNS  90 BPM") && str_eq(a2, "KIT: 606"), "keep: a summary: NO PATTERNS  90 BPM / KIT: 606");
        bank_np = 3; bank_bpm = 92; str_cpy(bank_kit, "808", 9); proj_unsaved = 1;
        ui.force = 1; frame(); ppm("keep-project-full");
        proj_unsaved = 0; bank_has = 0;
        go_home(); frames(2);
    }
    printf("ui: %s\n", fails ? "FAILED" : "pages, layers (punch, steps, erase, roll, key, mix), layer lock, song layer, REC hold, drums, REC, 20000-frame fuzz PASS");
    return fails;
}
#endif
