/* SPDX-License-Identifier: GPL-3.0-only */
/* PERFORM (PHEBUS wave 6; punch.c perf_*, ui_perf.c): the MPC's pads on the keys. What Ben asked for, checked:
 *   1 every pad changes the whole mix audibly while it is held, and the mix comes back after release: sample for sample
 *     for the TIME and COLOUR effects and the KILLS (after their fade), within a stated tolerance for the THROWS (their
 *     echoes and reverb ring out: that is a throw) and the 1/1 and 1/2 replays (notes ring on naturally, as the STUTTER
 *     lane's replay, tests/stutter_test.c); no effect leaves more samples at full scale than the dry render has
 *   2 LATCH: a pad pressed with OCT+ held, or OCT+ while it is held, plays on without fingers; pressed again: off; OCT-
 *     (perf_unlatch): every latch off
 *   3 the knobs act on the pad they belong to (a knob of one pad changes that pad's sound, not another's)
 *   4 played pads record into the STUTTER lane while the sequencer records (the pad, its span, the knob turned), play
 *     back with the pattern exactly as played (a synthetic mix, a bar a whole number of blocks: sample for sample),
 *     and keep playing loop after loop
 *   5 saved and loaded with the pattern (proj_capture_core / proj_apply), identical playback after
 *   6 no stuck state after STOP, after a pattern change, after every pad is let go
 *   7 the STUTTER lane's own steps play exactly as before wave 6 (a render hashed on the wave 5 merge)
 * cc -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -Itests -o build/host/perf_test tests/perf_test.c build/host/libplx.a -lstdc++ -lm */
#define main hostsim_main
#include <stddef.h>
#include "hostsim.c"
#undef main
#define PROJ_HOST 1
static uint32_t trk_def_engine(uint32_t i) { static const uint8_t E[NPART] = {0, 1, 3}; return E[i % NPART]; }
#include "../firmware/src/project.c"
#include <assert.h>
#include <math.h>
#include <sys/mman.h>
static int fails;
static void check(int ok, const char *what) { printf("perform: %-96s %s\n", what, ok ? "ok" : "FAIL"); fails += !ok; }

/* ---------------------------------------------------------------- the whole mix, through mix_block ---- */
#define BPM 120u
#define STEP_S (FS * 60u / BPM / 4u)
#define NS (FS * 10u)                                 /* 10 s a render */
#define NB (NS / CTL)
static int32_t *ref, *out, *refm, *outm, *mixto, *pos;       /* (shared with the forked renders) the output; the mix before the master */
static int32_t *refx, *refxm;                                  /* (wave 13) a mute's reference: those parts never played */
static uint32_t strip_lanes, strip_trks;                       /* .. the drum slots, the voices it leaves out (song_setup) */
static void song_setup(void)
{
    static const uint8_t BASS[32] = {36, 0, 36, 48, 0, 36, 39, 0, 36, 0, 43, 36, 0, 46, 36, 0,
                                     41, 0, 41, 53, 0, 41, 44, 0, 41, 0, 48, 41, 0, 51, 41, 0};
    static const uint8_t CH[3] = {60, 63, 67};
    uint32_t j;
    host_tracks_init();
    song.g[G_BPM] = BPM;
    song.master_q12 = 2048;
    memset(&drums, 0, sizeof drums);
    kit_set((uint32_t)(0));                               /* the sampled kit: no random noise */
    for (j = 0; j < 16u; j++) {
        static const uint8_t KICK[1] = {36}, KS[2] = {36, 38}, HAT[1] = {42};
        if (j % 4u == 0u) put_step(TDRUM, j, j % 8u == 4u ? 2u : 1u, j % 8u == 4u ? KS : KICK, ST_NOTE, 0);
        else if (j % 2u == 0u) put_step(TDRUM, j, 1, HAT, ST_NOTE, 0);
        else put_step(TDRUM, j, 0, 0, ST_REST, 0);
    }
    for (j = 0; j < 32u; j++)
        put_step(&trk[0], j, BASS[j] ? 1u : 0u, &BASS[j], BASS[j] ? ST_NOTE : ST_REST, 0);
    for (j = 0; j < 16u; j++)                         /* track 2: a chord on 1 and 9 */
        put_step(&trk[1], j, j % 8u == 0u ? 3u : 0u, CH, j % 8u == 0u ? ST_NOTE : ST_REST, 0);
    for (j = 0; j < 16u; j++) {                       /* (wave 13) voice 3: a note on 3 and 11 (its mute has something) */
        static const uint8_t V3N[1] = {72};
        put_step(&trk[2], j, j % 8u == 2u ? 1u : 0u, V3N, j % 8u == 2u ? ST_NOTE : ST_REST, 0);
    }
    TDRUM->p[P_SLEN] = 16, trk[0].p[P_SLEN] = 32, trk[1].p[P_SLEN] = 16, trk[2].p[P_SLEN] = 16;
    trk[0].p[P_VOICE] = V_MONO;
    trk[0].seq_active = trk[1].seq_active = trk[2].seq_active = 1, TDRUM->seq_active = 1;
    if (strip_lanes || strip_trks) {                  /* (wave 13: the reference for a mute: those parts never played) */
        for (j = 0; j < NSTEP; j++) {
            uint32_t l;
            for (l = 0; l < DRUM_LANES; l++)
                if ((strip_lanes >> l) & 1u)
                    dt_clr(j, l);
        }
        for (j = 0; j < 3u; j++)
            if ((strip_trks >> j) & 1u)
                steps_clear(&trk[j]);
    }
    memset(&stl, 0, sizeof stl);
    stl.len = 16, stl.act = 1;
    perf.held = perf.lat = 0;
    clk_beat = 0, clk_pos = 0;
    transport_req = 1;
}
typedef struct { int32_t pad; uint32_t on, off; int latch; int32_t pad2; } act_t;   /* a pad (and a second) from block on to off */
static void render_in(int32_t *o, const act_t *a)
{
    uint32_t b;
    int32_t blk[CTL * 2u];
    for (b = 0; b < NB; b++) {
        if (a && a->pad >= 0) {
            if (b == a->on)
                perf_press((uint32_t)a->pad, (uint32_t)a->latch);
            if (b == a->on && a->pad2 > 0)
                perf_press((uint32_t)a->pad2, 0);
            if (b == a->off)
                a->latch ? perf_unlatch() : perf_release((uint32_t)a->pad);
            if (b == a->off && a->pad2 > 0)
                perf_release((uint32_t)a->pad2);
        }
        mix_block(blk, CTL);
        memcpy(o + b * CTL * 2u, blk, sizeof blk);
        {
            uint32_t i;
            for (i = 0; i < CTL; i++)
                mixto[2u * (b * CTL + i)] = mix_l[i], mixto[2u * (b * CTL + i) + 1u] = mix_r[i];
        }
    }
    pos[o == ref ? 0 : 1] = (int32_t)(trk[0].seq_abs * 1000u + TDRUM->seq_abs);   /* (where the tracks are at the end) */
    if (a && a->pad >= 0) {                           /* (the stuck-state check, in the child: after its render) */
        int idle = punch.cur == -1 && !perf.act && !perf.thr[0] && !perf.thr[1] && !perf_kill && !perf_dmute && !rep_l16 &&
                   !thr_rev && !thr_dly;
        uint32_t j;
        for (j = 0; j < PP_N; j++)
            idle &= punch.pg[j] == 0;
        _exit(idle ? 0 : 3);
    }
}
static int render(int32_t *o, const act_t *a)          /* from power-on state each time: 1 = left idle (no stuck state) */
{
    pid_t p = fork();
    int st = 0;
    mixto = o == ref ? refm : o == refx ? refxm : outm;
    if (p == 0) {
        song_setup();
        render_in(o, a);
        _exit(0);
    }
    waitpid(p, &st, 0);
    assert(WIFEXITED(st));
    return WEXITSTATUS(st) == 0;
}
static double energy(const int32_t *o, uint32_t a, uint32_t b)
{
    double e = 0;
    uint32_t i;
    for (i = 2u * a; i < 2u * b; i++)
        e += (double)o[i] * o[i];
    return e;
}
static double diff_energy(const int32_t *x, const int32_t *y, uint32_t a, uint32_t b)
{
    double e = 0;
    uint32_t i;
    for (i = 2u * a; i < 2u * b; i++)
        e += (double)(x[i] - y[i]) * (x[i] - y[i]);
    return e;
}
static uint32_t last_diff(const int32_t *x, const int32_t *y, uint32_t a, uint32_t b)   /* the last sample that differs + 1 */
{
    uint32_t i, l = a;
    for (i = a; i < b; i++)
        if (x[2u * i] != y[2u * i] || x[2u * i + 1u] != y[2u * i + 1u])
            l = i + 1u;
    return l;
}
static int32_t max_abs_diff(const int32_t *x, const int32_t *y, uint32_t a, uint32_t b)
{
    int32_t m = 0;
    uint32_t i;
    for (i = 2u * a; i < 2u * b; i++) {
        int32_t d = x[i] - y[i];
        d = d < 0 ? -d : d;
        m = d > m ? d : m;
    }
    return m;
}
static uint32_t full_scale(const int32_t *o, uint32_t a, uint32_t b)   /* samples at the output's limit */
{
    uint32_t i, n = 0;
    for (i = 2u * a; i < 2u * b; i++)
        n += o[i] >= 32767 || o[i] <= -32768;
    return n;
}
static double env_corr(const int32_t *a, const int32_t *b, uint32_t n)   /* the 256-sample RMS envelopes' correlation */
{
    double sab = 0, saa = 0, sbb = 0, ma = 0, mb = 0;
    static double ea[4096], eb[4096];
    uint32_t w, i, nw = n / 256u > 4096u ? 4096u : n / 256u;
    for (w = 0; w < nw; w++) {
        double x = 0, y = 0;
        for (i = w * 256u; i < w * 256u + 256u; i++)
            x += (double)a[2u * i] * a[2u * i] + (double)a[2u * i + 1u] * a[2u * i + 1u],
            y += (double)b[2u * i] * b[2u * i] + (double)b[2u * i + 1u] * b[2u * i + 1u];
        ea[w] = sqrt(x), eb[w] = sqrt(y), ma += ea[w], mb += eb[w];
    }
    ma /= nw, mb /= nw;
    for (w = 0; w < nw; w++)
        sab += (ea[w] - ma) * (eb[w] - mb), saa += (ea[w] - ma) * (ea[w] - ma), sbb += (eb[w] - mb) * (eb[w] - mb);
    return saa > 0 && sbb > 0 ? sab / sqrt(saa * sbb) : 0;
}
static void pads_audible(void)
{
    uint32_t p, on = 2u * FS / CTL + 7u, off = on + (uint32_t)(1.5 * FS / CTL);   /* held 1.5 s from bar 2 (+ a little) */
    char b[200];
    int idle;
    render(ref, 0);
    for (p = 0; p < PF_NPAD; p++) {
        act_t a = {(int32_t)p, on, off, 0, 0};
        uint32_t s0 = on * CTL + 4096u, s1 = off * CTL, after = off * CTL, settle = 2u * CTL + 64u, tail = 0, last;
        double de, e, rel;
        int32_t md;
        int seq = p == PF_S1 || p == PF_S2, thr = p == PF_ECHO || p == PF_VERB, kill = (PF_MUTES >> p) & 1u;
        idle = render(out, &a);
        de = diff_energy(out, ref, s0, s1);
        e = energy(ref, s0, s1);
        rel = e > 0 ? 10.0 * log10(de / e + 1e-30) : -999;
        if (kill)
            settle = 10u * CTL;                       /* (a mute's fade: 8 blocks; the notes played on unheard) */
        if (p == PF_SWDN || p == PF_SWUP)
            settle = 70u * CTL;                       /* (wave 13: a sweep goes back smoothly, 46 ms) */
        if (thr)
            tail = 5u * FS;                           /* (the echoes and the reverb ring out) */
        if (seq)
            tail = 2u * FS;                           /* (the bass's notes, replayed, ring on and its envelopes catch up) */
        last = last_diff(outm, refm, after, NS);
        md = max_abs_diff(outm, refm, after + settle + tail, NS);
        snprintf(b, sizeof b, "%-14s held: the mix changes (difference %.1f dB of the dry mix)", PFD[p].name, rel);
        check(rel > -20.0, b);
        if (!thr && !seq) {
            snprintf(b, sizeof b, "%-14s let go: the dry mix again, sample for sample, %u samples after release (%s)",
                     PFD[p].name, last > after ? last - after : 0u, kill ? "a mute's fade: 320" :
                     p == PF_SWDN || p == PF_SWUP ? "the sweep's return: 2,304" : "the fade: 128");
            check(last <= after + settle, b);
        } else if (thr) {
            snprintf(b, sizeof b, "%-14s let go: %.0f s on, within %d of the dry mix (tolerance 64: the echoes' and reverb's last bits)",
                     PFD[p].name, (double)tail / FS, md);
            check(md <= 64, b);
        } else {   /* a replay: the tracks play on from where they would have been (the same steps as the dry run), the music the
                    * same (the level envelope's correlation, the level within 1 dB); not sample for sample: the oscillators run
                    * on freely, so a note replayed leaves them at another phase (the STUTTER lane's 1/1 and 1/2 the same) */
            uint32_t w0 = after + tail, n = NS - w0 < 4u * FS ? NS - w0 : 4u * FS;
            double ec = env_corr(outm + 2u * w0, refm + 2u * w0, n), lv = 10.0 * log10(energy(outm, w0, w0 + n) / energy(refm, w0, w0 + n));
            double base = env_corr(refm + 2u * w0, refm + 2u * (w0 - 4u * FS), n);   /* (the dry mix against itself two bars
                                                         * before: the same notes, the oscillators elsewhere) */
            snprintf(b, sizeof b, "%-14s let go: every track on the step it would be on (%d, %d); %.0f s on, the level envelope corr %.3f (dry, 2 bars apart: %.3f), level %+.2f dB",
                     PFD[p].name, pos[1], pos[0], (double)tail / FS, ec, base, lv);
            check(pos[0] == pos[1] && ec >= 0.90 && fabs(lv) < 1.0, b);   /* (0.90: the STUTTER lane's own 1/1 replay, as Ben
                                                         * has it since 21 Nov, measures 0.80 .. 0.91 on this song a second at
                                                         * a time after its run; the pad 0.91 .. 0.95) */
        }
        snprintf(b, sizeof b, "%-14s no sample at full scale that the dry mix has not (%u, dry %u); no stuck state after",
                 PFD[p].name, full_scale(out, s0, NS), full_scale(ref, s0, NS));
        check(full_scale(out, s0, NS) <= full_scale(ref, s0, NS) && idle, b);
    }
    {   /* latch: OCT+ held as the pad is pressed; it plays on after the key is up, until unlatched */
        act_t a = {PF_PHASER, on, off, 1, 0};
        uint32_t last;
        idle = render(out, &a);
        last = last_diff(outm, refm, off * CTL, NS);
        check(diff_energy(out, ref, on * CTL + 4096u, off * CTL) > 0.01 * energy(ref, on * CTL + 4096u, off * CTL) &&
              last <= off * CTL + 2u * CTL + 64u && idle,
              "LATCH: the phaser pressed with OCT+ plays on with no key held; unlatched, the dry mix again");
    }
}

/* ---------------------------------------------------------------- the engine on a synthetic mix ---- */
/* 125 BPM: a bar is 84672 samples, 2646 blocks: the blocks fall in the same places every bar, so a lane that plays the
 * same pads every bar plays them sample for sample the same */
#define SB 125u
#define BAR_B 2646u
#define BAR_S (BAR_B * CTL)
static int32_t syn_l[CTL], syn_r[CTL];
static void syn_mix(uint32_t b)                       /* a bar-periodic stereo mix: a kick-like thump, a tone, a noise hat */
{
    uint32_t i;
    for (i = 0; i < CTL; i++) {
        uint32_t t = (b % BAR_B) * CTL + i, q = t % (BAR_S / 4u), h = t % (BAR_S / 8u), z = t * 2654435761u;
        int32_t thump = q < 6000u ? (int32_t)(sine_i(q * 1200000u) * (int32_t)(6000u - q) / 6000) : 0;
        int32_t tone = sine_i(t * 13421773u) >> 2;
        int32_t hat = h < 1500u ? (int32_t)((z >> 16) & 0x3FFFu) - 8192 : 0;
        syn_l[i] = (thump + tone + hat) * 2;
        syn_r[i] = (thump + tone - hat) * 2;
    }
}
static void eng_reset(void)
{
    memset(punch_ring, 0, sizeof punch_ring);
    memset(&stl, 0, sizeof stl);
    stl.len = 16, stl.act = 1;
    punch.cur = -1, punch.g = 0, punch.w = 0;
    memset(punch.pg, 0, sizeof punch.pg);
    perf.held = perf.lat = perf.act = perf.lane = perf.rprev = perf.ropen = 0;
    perf_init();
    stl_want = stl_was = -1;
    rep_l16 = 0;
    song.g[G_BPM] = SB;
    clk_beat = 0, clk_pos = 0;
    song.playing = 1;
    song.rec = 0;
}
static void eng_block(uint32_t b, int32_t *o)         /* one block as mix_block runs it: the pads, the lane, the effect */
{
    stl_seq();
    stl_tick();
    syn_mix(b);
    punch_process(syn_l, syn_r, CTL);
    memcpy(o, syn_l, sizeof syn_l);
    memcpy(o + CTL, syn_r, sizeof syn_r);
    clk_pos += CTL * SB;
    while (clk_pos >= BEAT_U)
        clk_pos -= BEAT_U, clk_beat++;
}
static uint32_t step_block(uint32_t bar, uint32_t step)   /* the first block whose 16th is this step */
{
    uint32_t s = bar * BAR_S + step * (BAR_S / 16u), b = s / CTL;
    return b * CTL < s ? b + 1u : b;                  /* (a block whose start is at or past the step) */
}
static int32_t *bars;                                 /* 5 bars of the engine's output (L block, R block) */
static void play_bars(uint32_t n, void (*act)(uint32_t b))
{
    uint32_t b;
    for (b = 0; b < n * BAR_B; b++) {
        if (act)
            act(b);
        eng_block(b, bars + b * CTL * 2u);
    }
}
static int bars_same(uint32_t x, uint32_t y)
{
    return !memcmp(bars + x * BAR_B * CTL * 2u, bars + y * BAR_B * CTL * 2u, BAR_B * CTL * 2u * sizeof(int32_t));
}
/* bar 1, recording: the 1/4 pad from step 4 for 2 steps (its PITCH turned to -5 while held), ISO LO latched from step 8
 * to 14, CRUSH from step 12 to 15 */
static void rec_act(uint32_t b)
{
    if (b == BAR_B)
        song.rec = 1;
    if (b == step_block(1, 4)) {
        perf_press(PF_S4, 0);
        perf.par[PF_S4][0] = -5;                      /* (as ui_perf.c perf_knob: the knob turned while the pad plays) */
        perf.kn[PF_S4] = 1, perf.knv[PF_S4] = -5;
    }
    if (b == step_block(1, 6))
        perf_release(PF_S4), perf.par[PF_S4][0] = 0;  /* (the knob back: the lane keeps the recorded -5) */
    if (b == step_block(1, 8))
        perf_press(PF_ISOL, 1);
    if (b == step_block(1, 12))
        perf_press(PF_CRUSH, 0);
    if (b == step_block(1, 14))
        perf_unlatch();
    if (b == step_block(1, 15))
        perf_release(PF_CRUSH);
    if (b == 2u * BAR_B)
        song.rec = 0;
}
static void live_act(uint32_t b)                      /* the same, played live, nothing recorded (the reference) */
{
    if (b == step_block(1, 4)) perf_press(PF_S4, 0), perf.par[PF_S4][0] = -5;
    if (b == step_block(1, 6)) perf_release(PF_S4), perf.par[PF_S4][0] = 0;
    if (b == step_block(1, 8)) perf_press(PF_ISOL, 1);
    if (b == step_block(1, 12)) perf_press(PF_CRUSH, 0);
    if (b == step_block(1, 14)) perf_unlatch();
    if (b == step_block(1, 15)) perf_release(PF_CRUSH);
}
static const pev_t *ev_of(uint32_t p)
{
    uint32_t i;
    for (i = 0; i < PEV_N; i++)
        if ((stl.ev[i].fx & 31u) == p + 1u)
            return &stl.ev[i];
    return 0;
}
static void engine_tests(void)
{
    static int32_t *live;
    const pev_t *e;
    char b[200];
    uint32_t i;
    bars = mmap(0, 6u * BAR_B * CTL * 2u * sizeof(int32_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    live = mmap(0, 3u * BAR_B * CTL * 2u * sizeof(int32_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    /* the live take, nothing recorded: its bar 1 */
    eng_reset();
    play_bars(2, live_act);
    memcpy(live, bars, 2u * BAR_B * CTL * 2u * sizeof(int32_t));
    /* recorded: bar 0 dry, bar 1 the take (recording), bars 2 .. 4 the lane plays it back */
    eng_reset();
    play_bars(5, rec_act);
    e = ev_of(PF_S4);
    snprintf(b, sizeof b, "recorded: 1/4 from step 5 for 2 steps, with its PITCH -5 (st %d len %d knob %d value %d)",
             e ? e->st : -1, e ? e->len : -1, e ? (e->fx >> 5) & 3 : -1, e ? e->kv : 0);
    check(e && e->st == 4u && e->len == 2u && (e->fx & 0x80u) && ((e->fx >> 5) & 3u) == 0u && e->kv == -5, b);
    e = ev_of(PF_ISOL);
    check(e && e->st == 8u && e->len == 6u && !(e->fx & 0x80u), "recorded: ISO LO latched from step 9 to 14 (6 steps)");
    e = ev_of(PF_CRUSH);
    check(e && e->st == 12u && e->len == 3u, "recorded: CRUSH from step 13 to 15 (3 steps)");
    check(!memcmp(bars + BAR_B * CTL * 2u, live + BAR_B * CTL * 2u, BAR_B * CTL * 2u * sizeof(int32_t)),
          "recording changes nothing heard: the take sounds as the same pads played live");
    check(bars_same(1, 2), "played back by the lane (bar 3): sample for sample the take as played (bar 2), the knob's -5 too");
    check(bars_same(2, 3) && bars_same(3, 4), "and every loop after (bars 4, 5) the same");
    {   /* the dry bar is not the take (it did something) */
        int differ = memcmp(bars, bars + BAR_B * CTL * 2u, BAR_B * CTL * 2u * sizeof(int32_t)) != 0;
        check(differ, "the take differs from the dry bar");
    }
    /* the lane OFF: the recorded pads are silent (HOME's STUT knob) */
    stl.act = 0;
    for (i = 0; i < 2u * BAR_B; i++) eng_block(5u * BAR_B + i, bars + (i % BAR_B) * CTL * 2u);
    check(!perf.lane && punch.cur == -1, "the lane OFF (HOME KNOB 3): the recorded pads do not play");
    stl.act = 1;
    /* saved and loaded with the pattern: proj_capture_core, the lane emptied, proj_apply */
    {
        static proj_core_t c;
        pev_t keep[PEV_N];
        memcpy(keep, stl.ev, sizeof keep);
        proj_capture_core(&c);
        memset(stl.ev, 0, sizeof stl.ev);
        proj_apply(&c, 1);
        check(!memcmp(keep, stl.ev, sizeof keep), "saved and loaded (project format 8, beside the STUTTER lane): the same spans");
        song.g[G_BPM] = SB;
        /* identical playback after the load: one bar again, from bar 0 of a fresh engine with the loaded lane */
        memcpy(keep, stl.ev, sizeof keep);
        eng_reset();
        memcpy(stl.ev, keep, sizeof keep);
        play_bars(3, 0);
        check(bars_same(1, 2), "after the load, the lane plays the same every bar");
        {
            static int32_t *rec2;
            rec2 = live;                              /* (reuse: the recorded run's bar 2 again) */
            eng_reset();
            play_bars(5, rec_act);
            memcpy(rec2, bars + 2u * BAR_B * CTL * 2u, BAR_B * CTL * 2u * sizeof(int32_t));
            eng_reset();
            memcpy(stl.ev, keep, sizeof keep);
            play_bars(3, 0);
            /* the loaded lane's bar 3 against the recorded run's bar 3: the ring's history before bar 1 differs (dry
             * against the take), so from bar 2 on, the same history: bar 3 */
            check(!memcmp(bars + 2u * BAR_B * CTL * 2u, rec2, BAR_B * CTL * 2u * sizeof(int32_t)),
                  "after the load: the lane's playback sample for sample as before the save");
        }
    }
    /* an older project (format 8 before wave 6: the room zero): no pads; and junk in the room: dropped */
    {
        static proj_core_t c;
        proj_capture_core(&c);
        memset(c.perf.fx, 0, sizeof c.perf.fx);
        c.perf.fx[0] = 31, c.perf.fx[1] = 3, c.perf.fx[2] = 4;   /* a pad that does not exist */
        c.perf.fx[4] = PF_S8 + 1u, c.perf.fx[5] = 70, c.perf.fx[6] = 2;   /* a step past 64 */
        proj_apply(&c, 1);
        check(!stl.ev[0].fx && !stl.ev[1].fx, "an older project or junk in the room: no recorded pads, nothing invalid kept");
    }
    /* erase (as ui_stut.c: a span's byte zeroed): the rest plays on */
    eng_reset();
    play_bars(3, rec_act);
    for (i = 0; i < PEV_N; i++)
        if ((stl.ev[i].fx & 31u) == PF_CRUSH + 1u)
            stl.ev[i].fx = 0;
    play_bars(1, 0);
    check(ev_of(PF_S4) && ev_of(PF_ISOL) && !ev_of(PF_CRUSH), "a span erased: gone, the others kept");
    /* the lane full: 31 spans, the 32nd refused with a message (perf.full) */
    eng_reset();
    stl.len = 64;
    song.rec = 1;
    for (i = 0; i < 2u * 33u; i++) {                  /* a pad a step, every other step: 33 presses */
        uint32_t bb, pad = i % 2u == 0u ? PF_S8 : PF_S8;
        for (bb = 0; bb < BAR_B / 16u; bb++) {
            if (bb == 0u && i % 2u == 0u) perf_press(pad, 0);
            if (bb == 0u && i % 2u == 1u) perf_release(pad);
            eng_block(i * (BAR_B / 16u) + bb, bars);
        }
    }
    song.rec = 0;
    eng_block(0, bars);
    {
        uint32_t n = 0;
        for (i = 0; i < PEV_N; i++) n += stl.ev[i].fx != 0u;
        snprintf(b, sizeof b, "the lane holds 31 spans; the next one is refused, said once (LANE FULL: 31 PADS): %u kept, full %u", n, perf.full);
        check(n == 31u && perf.full, b);
        perf.full = 0;
    }
    /* knobs: each knob acts on its own pad (a 1/4 pad's PITCH changes the 1/4, not the 1/8) */
    {
        static int32_t *a0, *a1;
        a0 = live, a1 = live + BAR_B * CTL * 2u;
        eng_reset();
        play_bars(1, 0);
        perf_press(PF_S8, 0);
        play_bars(1, 0);
        memcpy(a0, bars + BAR_B * 0u, BAR_B * CTL * 2u * sizeof(int32_t));
        eng_reset();
        perf.par[PF_S4][0] = 7;                       /* another pad's knob */
        play_bars(1, 0);
        perf_press(PF_S8, 0);
        play_bars(1, 0);
        check(!memcmp(a0, bars, BAR_B * CTL * 2u * sizeof(int32_t)), "knobs: the 1/4 pad's PITCH does not touch the 1/8 pad");
        eng_reset();
        perf.par[PF_S8][0] = 7;                       /* its own */
        play_bars(1, 0);
        perf_press(PF_S8, 0);
        play_bars(1, 0);
        check(memcmp(a0, bars, BAR_B * CTL * 2u * sizeof(int32_t)) != 0, "knobs: the 1/8 pad's own PITCH changes it");
        (void)a1;
    }
    /* latch by OCT+ while a pad is held (perf_latch_held), a latched pad pressed again: off; OCT- all off */
    eng_reset();
    perf_press(PF_VOWEL, 0);
    perf_latch_held();
    perf_release(PF_VOWEL);
    check(perf.lat == PFB(PF_VOWEL) && !perf.held, "LATCH: OCT+ while VOWEL is held latches it; the key up, it stays");
    perf_press(PF_VOWEL, 0);
    check(!perf.lat && !perf.held, "LATCH: the latched pad pressed again: off");
    perf_press(PF_ISOL, 1), perf_press(PF_MKICK, 1), perf_release(PF_ISOL), perf_release(PF_MKICK);
    check(pf_count(perf.lat) == 2u, "LATCH: two latched with OCT+, an effect and a mute (the header says PERF 2)");
    perf_unlatch();
    check(!perf.lat, "LATCH: OCT- lets every latch go");
    /* the last TIME pad wins, and the one under it comes back when it is let go (the MPC's beat repeat) */
    eng_reset();
    perf_press(PF_S4, 0);
    play_bars(1, 0);
    perf_press(PF_S16, 0);
    for (i = 0; i < 40u; i++) eng_block(i, bars);
    check(punch.cur == PX_LOOP16, "two TIME pads held: the last pressed plays (1/16 over 1/4)");
    perf_release(PF_S16);
    for (i = 0; i < 40u; i++) eng_block(i, bars);
    check(punch.cur == PX_LOOP4, "the 1/16 let go: the 1/4 still held plays again");
    perf_release(PF_S4);
    perf_press(PF_MOTION, 0), perf_press(PF_PHASER, 0), perf_press(PF_S8, 0), perf_press(PF_MKICK, 0);
    for (i = 0; i < 40u; i++) eng_block(i, bars);
    check(punch.cur == PX_LOOP8 && punch.pg[PP_MOTION] == 32767 && punch.pg[PP_PHASER] == 32767 && perf_dmute == DM_KICKS,
          "a TIME pad, COLOUR pads and a mute together: all of them play (1/8 + MOTION + PHASER + KICKS)");
    perf_release(PF_MOTION), perf_release(PF_PHASER), perf_release(PF_S8), perf_release(PF_MKICK);
    for (i = 0; i < 40u; i++) eng_block(i, bars);
    check(punch.cur == -1 && !punch.pg[PP_MOTION] && !punch.pg[PP_PHASER] && !perf_dmute, "all let go: nothing left playing");
}


/* ---------------------------------------------------------------- (wave 13) the seven mutes ---- */
/* Ben, 14:13: "Mute kicks and low tom / Mute all drums but tops - snare, clap and hats, shaker / Mute all drums but shaker
 * and clap / Mute all drums but hats and snare / Mute voice 1 / 2 / 3"; 14:25: "Hold to mute, latch with OCT+". Each held:
 * after its fade (8 blocks) the mix is sample for sample the song with exactly its parts never played (the rest exact);
 * let go: the song again, sample for sample (the parts played on unheard, so they come back in place: pads_audible). */
static void mute_case(const char *what, int32_t p1, int32_t p2, uint32_t lanes, uint32_t trks)
{
    uint32_t on = 2u * FS / CTL + 7u, off = on + (uint32_t)(1.5 * FS / CTL), w0 = on * CTL + 10u * CTL, w1 = off * CTL;
    act_t a = {p1, on, off, 0, p2};
    char b[220];
    int32_t md;
    strip_lanes = lanes, strip_trks = trks;
    render(refx, 0);
    strip_lanes = strip_trks = 0;
    render(out, &a);
    md = max_abs_diff(outm, refxm, w0, w1);
    snprintf(b, sizeof b, "%s held: the song without exactly those parts, sample for sample (largest difference %d); its parts were "
             "heard before (%.0f dB)", what, md, 10.0 * log10(diff_energy(refm, refxm, w0, w1) / energy(refm, w0, w1) + 1e-30));
    check(md == 0 && diff_energy(refm, refxm, w0, w1) > 0, b);
}
static void mutes_tests(void)
{
    render(ref, 0);
    mute_case("KICKS (KICK, KICK 2, LO TOM)", PF_MKICK, 0, DM_KICKS, 0);
    mute_case("TOPS (all but SNARE, SNARE 2, CLAP, HATS, SHAKER)", PF_MTOPS, 0, DM_TOPS, 0);
    mute_case("SH+CL (all but SHAKER and CLAP)", PF_MSHCL, 0, DM_SHCL, 0);
    mute_case("HT+SN (all but HAT, OPEN HAT, PEDAL, SNARE, SNARE 2)", PF_MHTSN, 0, DM_HTSN, 0);
    mute_case("V1", PF_MV1, 0, 0, 1u);
    mute_case("V2", PF_MV2, 0, 0, 2u);
    mute_case("V3", PF_MV3, 0, 0, 4u);
    /* held together: what each mutes adds up (what is heard is what all of them allow) */
    mute_case("KICKS + HT+SN together (heard: what both allow)", PF_MKICK, PF_MHTSN, DM_KICKS | DM_HTSN, 0);
    mute_case("V2 + SH+CL together", PF_MV2, PF_MSHCL, DM_SHCL, 2u);
    check((DM_KICKS & 0xFFFFu) == 0x0203u && (~DM_TOPS & 0xFFFFu) == 0x217Cu && (~DM_SHCL & 0xFFFFu) == 0x2008u &&
          (~DM_HTSN & 0xFFFFu) == 0x0174u, "the drum groups by slot: KICKS 1 2 10; TOPS keeps 3 4 5 6 7 9 14; SH+CL keeps 4 14; HT+SN keeps 3 5 6 7 9");
    {   /* separate from the mixer's mutes: a PERFORM mute never changes them; both: silent if either mutes it */
        int32_t o[CTL * 2u];
        uint32_t bb;
        song_setup();
        drum_mute = 1u << 2;                          /* (the mixer: SNARE muted) */
        perf_press(PF_MKICK, 0);
        for (bb = 0; bb < FS / CTL; bb++) mix_block(o, CTL);
        check(drum_mute == 1u << 2 && !trk[0].p[P_MUTE] && perf_dmute == DM_KICKS, "a PERFORM mute leaves the mixer's mutes as they are");
        perf_release(PF_MKICK);
        perf_press(PF_MV1, 0);
        for (bb = 0; bb < 20u; bb++) mix_block(o, CTL);
        check(!trk[0].p[P_MUTE] && perf_kill == 1u, "V1 fades voice 1 (perf_kill), its MUTE untouched");
        perf_release(PF_MV1);
        for (bb = 0; bb < 20u; bb++) mix_block(o, CTL);
        check(drum_mute == 1u << 2 && !perf_kill && !perf_dmute, "let go: the mixer's SNARE mute still there, PERFORM's gone");
        drum_mute = 0;
        transport_req = 2;
        mix_block(o, CTL);
        {   /* both: the mixer's SNARE and PERFORM's KICKS held: the song without kicks, low tom and snare */
            uint32_t on = 2u * FS / CTL + 7u, off = on + (uint32_t)(1.5 * FS / CTL);
            act_t a = {PF_MKICK, on, off, 0, 0};
            pid_t pp;
            int st = 0;
            strip_lanes = DM_KICKS | 1u << 2;
            render(refx, 0);
            strip_lanes = 0;
            mixto = outm;
            pp = fork();
            if (!pp) {
                song_setup();
                drum_mute = 1u << 2;
                render_in(out, &a);
                _exit(0);
            }
            waitpid(pp, &st, 0);
            check(max_abs_diff(outm, refxm, on * CTL + 10u * CTL, off * CTL) == 0,
                  "the mixer's SNARE mute and PERFORM's KICKS at once: silent if either mutes it (the song without both)");
        }
    }
}

/* ---------------------------------------------------------------- (wave 13) a lane of waves 6 to 12 ---- */
static void convert_tests(void)
{
    static proj_core_t c;
    static const uint8_t OLD[12][4] = {               /* old pad + 1, st, len, (knob << 5 | 0x80 | kv) */
        {9, 0, 4, 0}, {10, 4, 2, 0}, {17, 6, 2, 0}, {18, 8, 1, 0x80}, {12, 9, 1, 0}, {14, 10, 1, 0},
        {19, 11, 1, 0}, {6, 12, 1, 0}, {21, 13, 1, 0}, {22, 14, 1, 0}, {20, 15, 1, 0}, {15, 1, 1, 0}};
    static const int8_t KV[12] = {0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0};
    uint32_t i;
    song_setup();
    proj_capture_core(&c);
    memset(c.perf.fx, 0, sizeof c.perf.fx);
    c.perf.rsv[0] = 0;                                /* (waves 6 to 12) */
    for (i = 0; i < 12u; i++) {
        pev_t *e = (pev_t *)c.perf.fx + i;
        e->fx = (uint8_t)(OLD[i][0] | OLD[i][3]);
        e->st = OLD[i][1], e->len = OLD[i][2], e->kv = KV[i];
    }
    pev_drop_n = 0;
    proj_apply(&c, 1);
    {
        static const int8_t WANT[12] = {PF_SWDN, PF_SWUP, PF_MSHCL, PF_MV2, -1, -1, -1, PF_LSTUT, PF_LSTUT, PF_LSTUT, PF_JET, PF_ECHO};
        int ok = pev_drop_n == 3u;
        for (i = 0; i < 12u; i++)
            ok &= WANT[i] < 0 ? !stl.ev[i].fx : (stl.ev[i].fx & 31u) == (uint32_t)WANT[i] + 1u && stl.ev[i].st == OLD[i][1];
        ok &= stl.ev[7].kv == 5 && stl.ev[8].kv == 1 && stl.ev[9].kv == 2 && (stl.ev[7].fx & 0xE0u) == 0x80u;
        check(ok, "an old lane: LP -> SWEEP DOWN, HP -> SWEEP UP, KILL DRUMS -> SH+CL, KILL BASS (VOICE 2) -> V2, FLNG -> JET, 1/32 8T 16T -> "
                  "the lane's 1/32 1/8T 1/16T, ECHO kept; GATE, DROP, LCUT dropped, 3 counted (LANE: 3 OLD PADS DROPPED)");
    }
    proj_capture_core(&c);
    pev_drop_n = 0;
    proj_apply(&c, 1);
    check(c.perf.rsv[0] == PEV_V13 && pev_drop_n == 0u && (stl.ev[0].fx & 31u) == PF_SWDN + 1u,
          "saved again: marked as the PERFORM set's (loaded again: nothing converted twice)");
    pev_drop_n = 0;
}

/* ---------------------------------------------------------------- (wave 13) the STUTTER lane's black keys ---- */
static void lane_keys_tests(void)
{
    static const int8_t WANT[11] = {PF_S1, PF_S2, PF_S4, PF_S8, PF_S16, PF_LSTUT, PF_LSTUT, PF_LSTUT, PF_LSTUT, PF_LSTUT, PF_LSTUT};
    uint32_t i, ok = 1, b;
    int32_t o[CTL * 2u];
    song_setup();
    song.sel = TRK_DRUM, stl_view = 1, kb_grid = 1;
    for (i = 0; i < 11u; i++) {                       /* each black key: a stutter, and nothing else */
        uint32_t k = PUNCH_BLACK[i];
        uint16_t s0 = dpat.vel[0][0];
        key_down(k);
        ok &= perf.held == PFB((uint32_t)WANT[i]) && (WANT[i] != PF_LSTUT || perf.par[PF_LSTUT][0] == (int8_t)(i - 5u));

        ok &= kb_kind[k] == KS_FX && dpat.vel[0][0] == s0 && !drum_mute && !perf_dmute;   /* (a pad: no note, no step, no mute) */
        perf.act = 0, perf_block();
        ok &= WANT[i] < 2 ? perf.ring == -2 : perf.ring == (WANT[i] == PF_LSTUT ? (int8_t)LST_PX[i - 5u] : PF_RING[WANT[i]]);

        key_up(k);
        ok &= !perf.held && !perf.lkeys;
    }
    check(ok, "the lane's black keys: F#3 1/1, G#3 1/2, A#3 1/4, C#4 1/8, D#4 1/16, F#4 1/4T, G#4 1/8T, A#4 1/16T, C#5 1/8D, "
              "D#5 1/16D, F#5 1/32: each a stutter of the whole mix, nothing else");
    /* two held: the last pressed plays; it let go, the one still held again */
    key_down(PUNCH_BLACK[6]), key_down(PUNCH_BLACK[8]);
    perf_block();
    ok = perf.ring == PX_LOOP8D;
    key_up(PUNCH_BLACK[8]);
    perf_block();
    ok &= perf.ring == PX_LOOP8T;
    key_up(PUNCH_BLACK[6]);
    perf_block();
    check(ok && perf.ring == -1 && !perf.held, "the lane's keys: two held, the last plays; let go, the other again; none: dry");
    /* OCT+ held: latch */
    fm1_in.buttons = dyn_bit[1] = 1u << 3;
    key_down(PUNCH_BLACK[7]), key_up(PUNCH_BLACK[7]);
    fm1_in.buttons = 0;
    check(perf.lat == PFB(PF_LSTUT) && perf.par[PF_LSTUT][0] == 2, "the lane's keys: OCT+ held latches (1/16T)");
    perf_unlatch();
    /* the played key wins over the lane's own lit step (as a PERFORM pad) */
    memset(&stl, 0, sizeof stl), stl.len = 16, stl.act = 1;
    for (i = 0; i < 16u; i++) stl.on[i >> 3] |= (uint8_t)(1u << (i & 7u));   /* every step lit, 1/4 */
    for (b = 0; b < 200u; b++) mix_block(o, CTL);
    ok = punch.cur == PX_LOOP4;
    key_down(PUNCH_BLACK[10]);
    for (b = 0; b < 40u; b++) mix_block(o, CTL);
    ok &= punch.cur == PX_LOOP32;
    key_up(PUNCH_BLACK[10]);
    for (b = 0; b < 40u; b++) mix_block(o, CTL);
    check(ok && punch.cur == PX_LOOP4, "the lane's own lit 1/4 steps, the F#5 key (1/32) held: the key wins; let go, the steps again");
    stl_view = 0, kb_grid = 0, song.sel = 0;
    transport_req = 2;
    mix_block(o, CTL);
}

/* ---------------------------------------------------------------- stuck states: STOP, a pattern change ---- */
static void stuck_tests(void)
{
    int32_t o[CTL * 2u];
    uint32_t b, j;
    int idle;
    song_setup();
    for (b = 0; b < FS / CTL; b++) mix_block(o, CTL);
    stl.ev[0] = (pev_t){PF_S2 + 1u, 0, 16, 0};       /* the lane replays the half bar all bar */
    perf_press(PF_PHASER, 1), perf_release(PF_PHASER);   /* PHASER latched */
    perf_press(PF_VERB, 1), perf_release(PF_VERB);    /* a reverb throw latched */
    perf_press(PF_MTOPS, 1), perf_release(PF_MTOPS);  /* TOPS (all but the tops muted), latched */
    perf_press(PF_MV1, 1), perf_release(PF_MV1);      /* voice 1 muted, latched */
    for (b = 0; b < FS / CTL; b++) mix_block(o, CTL);
    check(rep_l16 == 8u && perf_kill && perf_dmute && thr_rev && punch.pg[PP_PHASER],
          "(before STOP: the lane's 1/2 replays, PHASER, VERB, TOPS, V1 latched)");
    transport_req = 2;
    for (b = 0; b < 40u; b++) mix_block(o, CTL);
    idle = punch.cur == -1 && !perf.act && !perf.lat && !perf_kill && !perf_dmute && !rep_l16 && !thr_rev && !thr_dly;
    for (j = 0; j < DRUM_LANES; j++) idle &= dm_att[j] == 0;
    for (j = 0; j < PP_N; j++) idle &= punch.pg[j] == 0;
    check(idle, "STOP: every latch off, the lane's pads stop, the replay ends, nothing left playing");
    /* a pattern change: a pattern with recorded pads playing, then one without */
    {
        static proj_core_t empty;
        song_setup();
        proj_capture_core(&empty);                    /* (no recorded pads) */
        stl.ev[0] = (pev_t){PF_S8 + 1u, 0, 16, 0};
        stl.ev[1] = (pev_t){PF_SWUP + 1u, 0, 16, 0};
        stl.ev[2] = (pev_t){PF_S1 + 1u, 8, 8, 0};
        for (b = 0; b < 2u * FS / CTL; b++) mix_block(o, CTL);
        check(perf.lane != 0u && (punch.cur >= 0 || rep_l16), "(the pattern's recorded pads play)");
        perf_press(PF_S4, 0);                         /* a pad held over the change */
        song.rec = 1;
        for (b = 0; b < 2000u; b++) mix_block(o, CTL);
        proj_apply(&empty, 0);                        /* (the section switch: its core applied, every track from step 0) */
        seq_reset_tracks(clk_pos);
        song.rec = 0;
        perf_release(PF_S4);
        for (b = 0; b < 80u; b++) mix_block(o, CTL);   /* (wave 13: SWEEP UP's return, 46 ms) */
        idle = punch.cur == -1 && !perf.act && !perf.lane && !rep_l16 && !perf.ropen;
        for (j = 0; j < PP_N; j++) idle &= punch.pg[j] == 0;
        check(idle, "a pattern change to one with no recorded pads: they stop at once, a span open is dropped, nothing stuck");
        for (j = 0; j < PEV_N; j++) idle &= !stl.ev[j].fx;
        check(idle, "  and the new pattern got nothing from the old one's take");
    }
}

/* ---------------------------------------------------------------- the STUTTER lane as before ---- */
static void lane_set(uint32_t s, uint32_t code)
{
    stl.on[s >> 3] |= (uint8_t)(1u << (s & 7u));
    stl.lc[s >> 2] = (uint8_t)((stl.lc[s >> 2] & ~(3u << 2u * (s & 3u))) | (code & 3u) << 2u * (s & 3u));
    stl.lx[s >> 3] = (uint8_t)((stl.lx[s >> 3] & ~(1u << (s & 7u))) | ((code >> 2) & 1u) << (s & 7u));
}
static void lane_golden(void)
{
    static const uint8_t BASS[16] = {36, 0, 36, 48, 0, 36, 39, 0, 36, 0, 43, 36, 0, 46, 36, 0};
    uint32_t j, b, h = 2166136261u;
    int32_t blk[CTL * 2u];
    pid_t p = fork();
    int st = 0;
    if (p == 0) {
        host_tracks_init();
        song.g[G_BPM] = 120;
        song.master_q12 = 2048;
        kit_set((uint32_t)(DRUM_DEFAULT_KIT));
        for (j = 0; j < 16u; j++) {
            static const uint8_t KICK[1] = {36}, KS[2] = {36, 38}, HAT[1] = {42};
            if (j % 4u == 0u) put_step(TDRUM, j, j % 8u == 4u ? 2u : 1u, j % 8u == 4u ? KS : KICK, ST_NOTE, 0);
            else if (j % 2u == 0u) put_step(TDRUM, j, 1, HAT, ST_NOTE, 0);
            else put_step(TDRUM, j, 0, 0, ST_REST, 0);
            if (BASS[j]) put_step(&trk[0], j, 1, &BASS[j], ST_NOTE, 0); else put_step(&trk[0], j, 0, 0, ST_REST, 0);
        }
        TDRUM->p[P_SLEN] = 16; trk[0].p[P_SLEN] = 16; trk[0].p[P_VOICE] = V_MONO;
        trk[0].seq_active = 1; TDRUM->seq_active = 1;
        memset(&stl, 0, sizeof stl); stl.len = 64; stl.act = 1;
        for (j = 4; j < 8; j++) lane_set(j, 1);       /* 1/8 */
        for (j = 12; j < 16; j++) lane_set(j, 0);     /* 1/4 */
        for (j = 20; j < 22; j++) lane_set(j, 2);     /* 1/16 */
        for (j = 26; j < 28; j++) lane_set(j, 3);     /* 1/32 */
        for (j = 32; j < 40; j++) lane_set(j, 4);     /* 1/2 */
        for (j = 48; j < 64; j++) lane_set(j, 5);     /* 1/1 */
        clk_beat = 0, clk_pos = 0; transport_req = 1;
        for (b = 0; b < FS * 20u / CTL; b++) {
            mix_block(blk, CTL);
            for (j = 0; j < CTL * 2u; j++) h = (h ^ (uint32_t)blk[j]) * 16777619u;
        }
        _exit(h == 0x8e1f541fu ? 0 : 1);
    }
    waitpid(p, &st, 0);
    check(WIFEXITED(st) && WEXITSTATUS(st) == 0,
          "the STUTTER lane's own steps (1/8 1/4 1/16 1/32 1/2 1/1, 20 s): the same render, hashed, as on the wave 5 merge");
}

int main(void)
{
    ref = mmap(0, NS * 2u * sizeof(int32_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    refm = mmap(0, NS * 2u * sizeof(int32_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    outm = mmap(0, NS * 2u * sizeof(int32_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    pos = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    out = mmap(0, NS * 2u * sizeof(int32_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    refx = mmap(0, NS * 2u * sizeof(int32_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    refxm = mmap(0, NS * 2u * sizeof(int32_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    lane_golden();
    pads_audible();
    mutes_tests();
    convert_tests();
    lane_keys_tests();
    engine_tests();
    stuck_tests();
    printf("perform: %s\n", fails ? "FAILED" : "all ok");
    return fails != 0;
}
