/* SPDX-License-Identifier: GPL-3.0-only */
/* PHEBUS wave 13: the mixer and the STUTTER lane's black keys, through the real UI (tests/ui_pages_test.c's harness: the
 * keys and buttons as the FM-1 and the browser simulator give them, the audio between frames).
 *   C1  (Ben, 14:19: "It didn't solo anything either way") a pattern of kick, snare and hat with voice 1 playing a bass line:
 *       hold GLO, OCT+ held + the snare's key: the render is exactly the snare alone (sample for sample against a render of
 *       the snare's trigs alone, nothing else playing); tap the kick, the hat and F#3 (voice 1): the same; the drum hits
 *       counted are the snare's only. The same with GLO kept open (held 0.7 s and let go, as the simulator's double-click)
 *   the mixer's screen: the line says TAP A KEY: MUTE and [-] CLEAR [+] SOLO; a muted sound's tile yellow, a soloed one ice
 *       blue, the sounds a solo silences dark; the four voices on F#3 .. C#4 drawn in their colours, the other black keys not
 *       drawn and dark; SEL the pans; OCT- clears
 *   the lane: its black keys lit (live, drawn), each held plays its repeat and lights; the keys' lights agree with what they do
 * Exit status: the number of failed checks. */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
#include <sys/mman.h>
#define W13_NS (FS * 3u)
static int32_t *r_a, *r_b;
static uint32_t *hits_a;
static void w13_setup(void)
{
    uint32_t i;
    static const uint8_t BASS[16] = {36, 0, 36, 0, 39, 0, 36, 0, 43, 0, 36, 0, 41, 0, 36, 0};
    for (i = 0; i < NTRK; i++) {
        steps_clear(&trk[i]);
        trk[i].p[P_MUTE] = 0;
    }
    song.solo = 0, drum_mute = drum_solo = 0;
    TDRUM->p[P_SLEN] = 16, trk[0].p[P_SLEN] = 16;
    for (i = 0; i < 16u; i++) {
        if (i % 4u == 0u) dt_set(i, 0, 100);
        if (i % 8u == 4u) dt_set(i, 2, 100);
        if (i % 2u == 0u) dt_set(i, 4, 90);
        if (BASS[i]) put_step(&trk[0], i, 1, &BASS[i], ST_NOTE, 0);
    }
    for (i = 0; i < NTRK; i++) trk[i].p[P_DLY] = trk[i].p[P_REV] = 0;
}
static void w13_render(int32_t *o, uint32_t *hm)      /* from PLAY: W13_NS samples, and the drum lanes that hit */
{
    uint32_t b, k, a;
    int32_t blk[CTL * 2u];
    transport_req = 1;
    a = drums.age;
    for (b = 0; b < W13_NS / CTL; b++) {
        mix_block(blk, CTL);
        memcpy(o + b * CTL * 2u, blk, sizeof blk);
        if (drums.age != a)
            for (k = 0; k < NDRUM; k++)
                if (drums.v[k].age > a)
                    *hm |= 1u << lane_of_note(drums.v[k].note);
        a = drums.age;
    }
}
static int w13_same(void)
{
    uint32_t i, d = 0, f = 0;
    for (i = 0; i < W13_NS * 2u; i++) { if (r_a[i] != r_b[i] && !d) f = i; d += r_a[i] != r_b[i]; }
    if (d) { int32_t m = 0; for (i = 0; i < W13_NS * 2u; i++) { int32_t q = r_a[i] - r_b[i]; q = q < 0 ? -q : q; m = q > m ? q : m; }
             printf("  (%u samples differ, the first at %u (%d vs %d); largest %d; hits %x)\n", d, f / 2u, r_a[f], r_b[f], m, hits_a[0]); }
    return d == 0;
}
static double w13_energy(const int32_t *o) { double e = 0; uint32_t i; for (i = 0; i < W13_NS * 2u; i++) e += (double)o[i] * o[i]; return e; }
/* the reference: the snare's trigs alone (no other drum, no voice heard), after the same frames as the case (keys that do
 * nothing: D#4, not drawn) so every running state is the same */
static void ref_snare(int kind)
{
    pid_t p = fork();
    int st;
    if (!p) {
        uint32_t i, l, hm = 0;
        w13_setup();
        steps_clear(&trk[0]);
        for (i = 0; i < 16u; i++)
            for (l = 0; l < DRUM_LANES; l++)
                if (l != 2u) dt_clr(i, l);
        if (kind == 0) {
            song.sel = TRK_DRUM; studio_open(SC_DRUM); frames(3);
            press(B_GLO); frames(12);
            press(B_OCTUP); key(PUNCH_BLACK[4]); release(B_OCTUP);
            release(B_GLO); frames(3);
        } else if (kind == 1) {
            song.sel = 0; go_home(); frames(3);
            press(B_GLO); frames(12);
            key(PUNCH_BLACK[4]); key(PUNCH_BLACK[4]); key(PUNCH_BLACK[4]);
            release(B_GLO); frames(3);
        } else {
            song.sel = TRK_DRUM; go_home(); frames(3);
            press(B_GLO); frames(45); release(B_GLO); frames(2);
            press(B_OCTUP); key(PUNCH_BLACK[4]); release(B_OCTUP);
            tap(B_GLO); frames(2);
        }
        if (kind == 1)                                /* (the voices silenced the same way, so every engine's noise runs the same:
                                                       * the drums' noise shares the generator with the voices that render) */
            trk[0].p[P_MUTE] = 1;
        else
            song.solo = 1u << TRK_DRUM;
        w13_render(r_b, &hm);
        _exit(0);
    }
    waitpid(p, &st, 0);
}
static void mixer_audio(void)
{
    pid_t p;
    int st;
    double e;
    ref_snare(0);
    e = w13_energy(r_b);
    check(e > 1e9, "C1: (the reference: the snare alone sounds)");
    /* solo: GLO held, OCT+ held + the snare's key (the white key 3) */
    p = fork();
    if (!p) {
        hits_a[0] = 0;
        w13_setup();
        song.sel = TRK_DRUM; studio_open(SC_DRUM); frames(3);
        press(B_GLO); frames(12);
        press(B_OCTUP); key(key_of_mixlane(2)); release(B_OCTUP);
        release(B_GLO); frames(3);
        w13_render(r_a, &hits_a[0]);
        _exit(drum_solo == 1u << 2 ? 0 : 1);
    }
    waitpid(p, &st, 0);
    check(WEXITSTATUS(st) == 0 && hits_a[0] == 1u << 2 && w13_same(),
          "C1: GLO + OCT+ held + the snare's key: only the snare is heard (sample for sample the snare alone: no kick, no hat, no voice 1)");
    /* mute: GLO held, the kick's key, the hat's key, F#3 (voice 1) */
    p = fork();
    if (!p) {
        hits_a[0] = 0;
        w13_setup();
        song.sel = 0; go_home(); frames(3);
        press(B_GLO); frames(12);
        key(key_of_mixlane(0)); key(key_of_mixlane(4)); key(PUNCH_BLACK[0]);
        release(B_GLO); frames(3);
        w13_render(r_a, &hits_a[0]);
        _exit(drum_mute == (1u << 0 | 1u << 4) && trk[0].p[P_MUTE] ? 0 : 1);
    }
    waitpid(p, &st, 0);
    ref_snare(1);
    check(WEXITSTATUS(st) == 0 && hits_a[0] == 1u << 2 && w13_same(),
          "C1: GLO + the kick's key, the hat's, F#3 (voice 1): the snare alone, sample for sample (voice 1 selected: the same map)");
    /* the simulator's way: GLO kept open (held 0.7 s untouched, let go; the page's double-click holds it down: the same) */
    p = fork();
    if (!p) {
        hits_a[0] = 0;
        w13_setup();
        song.sel = TRK_DRUM; go_home(); frames(3);
        press(B_GLO); frames(45); release(B_GLO); frames(2);
        if (ly_lock != LY_GLO) _exit(2);
        press(B_OCTUP); key(key_of_mixlane(2)); release(B_OCTUP);
        tap(B_GLO); frames(2);
        w13_render(r_a, &hits_a[0]);
        _exit(drum_solo == 1u << 2 ? 0 : 1);
    }
    waitpid(p, &st, 0);
    ref_snare(2);
    check(WEXITSTATUS(st) == 0 && hits_a[0] == 1u << 2 && w13_same(),
          "C1: the mixer kept open (GLO 0.7 s and let go), OCT+ + the snare's key: the snare alone");
    /* before wave 13 (the bug, for the record): a drum solo left the voices playing; now a voice solo silences the drums too */
    p = fork();
    if (!p) {
        hits_a[0] = 0;
        w13_setup();
        song.sel = 0; go_home(); frames(3);
        press(B_GLO); frames(12);
        press(B_OCTUP); key(PUNCH_BLACK[0]); release(B_OCTUP);
        release(B_GLO); frames(3);
        w13_render(r_a, &hits_a[0]);
        _exit(song.solo == 1u ? 0 : 1);
    }
    waitpid(p, &st, 0);
    check(WEXITSTATUS(st) == 0 && hits_a[0] == 0u && w13_energy(r_a) > 1e9, "C1: OCT+ + F#3: voice 1 soloed: no drum hit at all, voice 1 heard");
}
static void mixer_screen(void)
{
    uint32_t c, ok;
    w13_setup();
    song.sel = 0; go_home(); frames(3);
    press(B_GLO); frames(12);
    key(key_of_mixlane(5));                              /* OP HAT muted */
    press(B_OCTUP); key(key_of_mixlane(0)); release(B_OCTUP);   /* KICK soloed */
    key(PUNCH_BLACK[2]);                              /* voice 3 muted */
    ui.force = 1; frame(); ppm("w13-mixer");
    /* (the drum grid: OP HAT top row, second column; KICK bottom left; SNARE bottom right) */
    check(px_count(1 + 60, BK_Y + 34, 58, 18, TE_MUTE) > 400u, "mixer screen: OP HAT's tile yellow (muted), its M in a black tab");
    check(px_count(1, BK_Y + 34 + 60, 58, 18, TE_SOLO) > 400u, "mixer screen: KICK's tile ice blue (soloed)");
    check(px_count(1 + 180, BK_Y + 34 + 60, 58, 18, TE_COL[TRK_DRUM]) == 0u && px_count(1 + 180, BK_Y + 34 + 60, 58, 18, TE_G2) > 20u,
          "mixer screen: SNARE silenced by the solo: dark grey, no teal");
    check(px_count(121, BK_Y, 58, 31, TE_MUTE) > 600u && px_count(1, BK_Y, 58, 31, TE_COL[0]) == 0u,
          "mixer screen: voice 3's cell yellow (muted); voice 1 dark (silenced by the kick's solo)");
    for (c = 0, ok = 1; c < 11u; c++)                  /* F#3 .. C#4 lit (live), the rest dark */
        ok &= (led_lv[14u + PUNCH_BLACK[c]] != LL_OFF) == (c < 4u);
    check(ok && bk_drawn == 0xFu, "mixer: the four voice keys drawn and lit, the other seven black keys neither");
    check(led_lv[B_OCTDN] == LL_LIT && led_lv[B_SCL] == LL_LIT, "mixer: OCT- (CLEAR) and SEL (the pans) lit");
    tap(B_SCL); frames(2);
    ui.force = 1; frame(); ppm("w13-mixer-pans");
    check(mix_pan, "mixer: SEL: the pans' page");
    tap(B_SCL); frames(2);
    tap(B_OCTDN); frames(2);
    check(!drum_mute && !drum_solo && !song.solo && !trk[2].p[P_MUTE] && led_lv[B_OCTDN] == LL_OFF,
          "mixer: OCT-: every mute and solo off (and OCT- dark then: nothing to clear)");
    release(B_GLO); frames(3);
}
static void lane_lights(void)
{
    uint32_t i, ok = 1;
    w13_setup();
    song.sel = TRK_DRUM; studio_open(SC_DRUM); frames(2);
    stl_view = 1, stl_pg = 0; ui.force = 1; frames(3);
    for (i = 0; i < 11u; i++)
        ok &= led_lv[14u + PUNCH_BLACK[i]] != LL_OFF;
    check(ok && bk_drawn == 0x7FFu, "the lane: its 11 black keys drawn (the repeats) and lit");
    for (i = 0; i < 11u; i++) {
        fm1_in.notes = 1u << PUNCH_BLACK[i]; frames(2);
        ok &= led_lv[14u + PUNCH_BLACK[i]] == LL_LIT && (perf.held & (1u << lane_pad_of(i)));
        if (i == 6u) { ui.force = 1; frame(); ppm("w13-lane-key"); }
        fm1_in.notes = 0; frames(2);
        ok &= !perf.held;
    }
    check(ok, "the lane: each black key held plays its repeat and lights; let go, off");
    stl_view = 0; go_home(); frames(2);
}
static void perform_shot(void)
{
    go_home(); song.sel = 0; frames(2);
    press(B_FX); frames(12);
    fm1_in.notes = 1u << key_of_white(PF_ISOL) | 1u << PUNCH_BLACK[3] | 1u << PUNCH_BLACK[0]; frames(3);
    ui.force = 1; frame(); ppm("w13-perform-held");
    fm1_in.notes = 0; frames(2);
    release(B_FX); frames(3);
}
int main(int argc, char **argv)
{
    uint32_t i;
    outdir = argc > 1 ? argv[1] : "build/host";
    r_a = mmap(0, W13_NS * 2u * sizeof(int32_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    r_b = mmap(0, W13_NS * 2u * sizeof(int32_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    hits_a = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    bank_resolve();
    panel = PANEL_DEFAULT;
    layers_init();
    settings.palette = 4;
    palette_set(4);
    host_tracks_init();
    for (i = 0; i < NPART; i++) { trk_def_sound(&trk[i], i); trk[i].engine = trk[i].eng_req; }
    kit_set((uint32_t)(DRUM_DEFAULT_KIT));
    song.g[G_BPM] = 120;
    go_home(); ui.force = 1; frames(4);
    mixer_audio();
    mixer_screen();
    lane_lights();
    perform_shot();
    printf("w13: %s (%d failed)\n", fails ? "FAILED" : "all ok", fails);
    return fails;
}
