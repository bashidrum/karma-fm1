/* SPDX-License-Identifier: GPL-3.0-only */
/* PERFORM's pads on the punch-in engine (punch.c; PHEBUS wave 6): each pad that acts in punch_process (the TIME and
 * COLOUR effects) changes the mix while held, stays bounded, and the mix is exactly the dry one again once its key is up
 * (after the 64-sample fade). The FX-held keyboard picks pads (white 1-16; the black keys that have one) and never plays
 * or records a note. Each does what its name says, and its knobs do what they are called (punch_pads_say). The throws
 * and kills act outside punch_process: tests/perf_test.c, with the whole mix.
 * argv[1]: a WAV demo (a beat, then every pad for one beat). */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
#include <math.h>
#define BLOCKS (FS * 3u / CTL)                        /* 3 s at 120 BPM */
static int32_t got[CTL * 2u];

static void beat_setup(void)
{
    static const uint8_t B[16][2] = {{36, 42}, {42}, {42}, {42}, {38, 42}, {42}, {42}, {36, 46},
                                     {36, 42}, {42}, {36}, {42}, {38, 42}, {42}, {42}, {46}};
    uint32_t j, q;
    host_tracks_init();
    memset(&drums, 0, sizeof drums);
    TDRUM->p[P_E0] = 0;                               /* the sampled kit: no random noise */
    song.g[G_BPM] = 120;
    for (j = 0; j < NSTEP; j++) {
        step_t *s = &TDRUM->step[j];
        memset(s, 0, sizeof *s);
        s->time = ST_REST;
        if (j < 16u)
            for (q = 0; q < 2u && B[j][q]; q++) {
                s->note[s->n++] = B[j][q];
                s->time = ST_NOTE;
                s->vel = 100;
            }
    }
    TDRUM->p[P_SLEN] = 16;
    perf.held = perf.lat = 0;
    punch.cur = -1;
    punch.g = 0;
    clk_beat = 0;
    clk_pos = 0;
    transport_req = 1;
}

/* a deterministic "mix": a decaying 110 Hz tone every quarter + noise bursts every 1/16 */
static void test_mix(uint32_t b, int32_t *l, int32_t *r)
{
    static uint32_t seed = 1;
    uint32_t i;
    if (b == 0) seed = 1;
    for (i = 0; i < CTL; i++) {
        uint32_t t = b * CTL + i, q = t % (FS / 2u), s16 = t % (FS / 8u);
        int32_t tone = (sine_i((uint32_t)t * 10712093u) * (int32_t)(FS / 2u - q) / (int32_t)(FS / 2u)) >> 1;
        int32_t nz = s16 < 2000u ? (int32_t)((seed = seed * 1664525u + 1013904223u) >> 18) - 8192 : 0;
        l[i] = (tone + nz) * 3;
        r[i] = (tone - nz) * 3;
    }
}

/* ---- each pad does what its name says ---- */
#define NS (FS * 8u)                                  /* up to 8 s */
static int32_t ol[NS], or_[NS];
typedef int32_t (*gen_f)(uint32_t t, int ch);
static double sweep_hz;                               /* the generators' tone */
static int32_t g_sine(uint32_t t, int ch) { (void)ch; return (int32_t)(100000.0 * sin(2 * M_PI * sweep_hz * t / FS)); }
static int32_t g_burst(uint32_t t, int ch)            /* a chord for 0.5 s, then nothing */
{
    double x = 0;
    if (t >= FS / 2u) return 0;
    x += sin(2 * M_PI * 220 * t / FS) + sin(2 * M_PI * 277.2 * t / FS + ch) + sin(2 * M_PI * 329.6 * t / FS) + 0.5 * sin(2 * M_PI * 880 * t / FS);
    return (int32_t)(10000.0 * x);
}
/* pad p held from sample `on` to `off` over the generator, punch_process as mix_block runs it (the pads first) */
static void run_fx(uint32_t p, gen_f g, uint32_t ns, uint32_t on, uint32_t off, int playing)
{
    uint32_t b, i;
    int8_t keep[PF_NPAD][4];
    memcpy(keep, perf.par, sizeof keep);              /* (beat_setup powers the knobs on: the test's set kept) */
    beat_setup();
    memcpy(perf.par, keep, sizeof keep);
    song.playing = (uint8_t)playing;
    clk_pos = 0, clk_beat = 0;
    for (b = 0; b < ns / CTL; b++) {
        int32_t l[CTL], r[CTL];
        uint32_t t0 = b * CTL;
        for (i = 0; i < CTL; i++) l[i] = g(t0 + i, 0), r[i] = g(t0 + i, 1);
        perf.held = t0 >= on && t0 < off ? 1u << p : 0u;
        stl_seq();
        punch_process(l, r, CTL);
        clk_pos += CTL * (uint32_t)song.g[G_BPM];
        while (clk_pos >= BEAT_U) clk_pos -= BEAT_U, clk_beat++;
        for (i = 0; i < CTL; i++) {
            assert(l[i] > -(1 << 22) && l[i] < (1 << 22) && r[i] > -(1 << 22) && r[i] < (1 << 22));
            ol[t0 + i] = l[i], or_[t0 + i] = r[i];
        }
    }
    perf.held = 0;
}
static double rms(const int32_t *x, uint32_t a, uint32_t b)
{
    double e = 0;
    uint32_t i;
    for (i = a; i < b; i++) e += (double)x[i] * x[i];
    return sqrt(e / (double)(b - a));
}
static double tone_pow(const int32_t *x, uint32_t a, uint32_t b, double hz)   /* Goertzel */
{
    double w = 2 * M_PI * hz / FS, c = 2 * cos(w), s1 = 0, s2 = 0, s0;
    uint32_t i;
    for (i = a; i < b; i++) s0 = x[i] + c * s1 - s2, s2 = s1, s1 = s0;
    return s1 * s1 + s2 * s2 - c * s1 * s2;
}
static double held_ratio(uint32_t p, double hz)       /* the held pad's level on a tone, from 0.6 s into the hold */
{
    sweep_hz = hz;
    run_fx(p, g_sine, FS * 2u, FS / 2u, FS * 3u / 2u, 0);
    return rms(ol, FS * 11u / 10u, FS * 7u / 5u) / rms(ol, FS / 10u, FS * 2u / 5u);
}
static void punch_pads_say(void)
{
    uint32_t beat = FS / 2u, t, i;
    double a, b2;
    /* LOW CUT: 24 dB/oct at its FREQ (251 Hz to start) */
    a = held_ratio(PF_LCUT, 60), b2 = held_ratio(PF_LCUT, 1000);
    printf("low cut: 60 Hz %.3f, 1 kHz %.3f\n", a, b2);
    assert(a < 0.06 && fabs(b2 - 1) < 0.12);
    perf.par[PF_LCUT][0] = 70;                        /* FREQ up to ~900 Hz: 400 Hz goes too */
    sweep_hz = 400;
    run_fx(PF_LCUT, g_sine, FS * 2u, FS / 2u, FS * 3u / 2u, 0);
    perf.par[PF_LCUT][0] = PFD[PF_LCUT].k[0].def;
    assert(rms(ol, FS * 11u / 10u, FS * 7u / 5u) < 0.15 * rms(ol, FS / 10u, FS * 2u / 5u));
    /* LP: down to its CUTOFF (380 Hz) in SPEED (1/2): 5 kHz gone, 100 Hz through; HP up to its CUTOFF: 60 Hz gone */
    a = held_ratio(PF_LP, 5000), b2 = held_ratio(PF_LP, 100);
    printf("low-pass sweep: 5 kHz %.3f, 100 Hz %.3f\n", a, b2);
    assert(a < 0.05 && b2 > 0.7);
    a = held_ratio(PF_HP, 60), b2 = held_ratio(PF_HP, 8000);
    printf("high-pass sweep: 60 Hz %.3f, 8 kHz %.3f\n", a, b2);
    assert(a < 0.1 && b2 > 0.7);
    /* PITCH DROP: an octave down (DEPTH 12) in its TIME: 440 Hz in, 220 Hz out */
    sweep_hz = 440;
    run_fx(PF_DROP, g_sine, FS * 3u, FS / 2u, FS * 5u / 2u, 0);
    a = tone_pow(ol, FS * 2u, FS * 5u / 2u, 220), b2 = tone_pow(ol, FS * 2u, FS * 5u / 2u, 440);
    printf("pitch drop: 220 Hz / 440 Hz power %.1f, level %.2f\n", a / b2, rms(ol, FS * 2u, FS * 5u / 2u) / rms(ol, 0, FS / 2u));
    assert(a > 30 * b2 && fabs(rms(ol, FS * 2u, FS * 5u / 2u) / rms(ol, 0, FS / 2u) - 1) < 0.3);
    /* GATE: on the grid, 1/16, half open: the second half of each 1/16 silent */
    sweep_hz = 1000;
    run_fx(PF_GATE, g_sine, FS * 2u, FS / 2u, FS * 3u / 2u, 1);
    {
        uint32_t step = beat / 4u, k = 0, ok = 0;
        for (t = FS * 3u / 4u; t + step < FS * 5u / 4u; t += step, k++) {
            uint32_t s0 = t - t % step;
            ok += rms(ol, s0 + 200u, s0 + step / 2u - 200u) > 0.6 * 70711 && rms(ol, s0 + step / 2u + 200u, s0 + step - 200u) < 0.01 * 70711;
        }
        printf("gate: %u of %u sixteenths open then shut\n", ok, k);
        assert(ok == k);
    }
    /* CRUSH: BITS 7 (steps of 2^11), RATE 1 */
    run_fx(PF_CRUSH, g_sine, FS, FS / 4u, FS * 3u / 4u, 0);
    for (t = FS / 2u; t < FS * 3u / 4u; t++) {        /* (within the wet gain's 1/4096 of a step of 2048) */
        int32_t q = ol[t] % 2048;
        q = q < 0 ? -q : q;
        assert(q <= 40 || q >= 2048 - 40);
    }
    /* TAPE STOP: to a stop in its TIME (a beat): silent after, while held */
    sweep_hz = 300;
    run_fx(PF_STOP, g_sine, FS * 2u, FS / 2u, FS * 3u / 2u, 0);
    printf("tape stop: half a beat in %.2f, after a beat %.4f\n", rms(ol, FS / 2u + beat / 2u - 500u, FS / 2u + beat / 2u + 500u) / rms(ol, 0, FS / 2u),
           rms(ol, FS / 2u + beat + 200u, FS * 3u / 2u) / rms(ol, 0, FS / 2u));
    assert(rms(ol, FS / 2u + beat + 200u, FS * 3u / 2u) < 0.01 * rms(ol, 0, FS / 2u));
    /* 1/8T: a third of a beat, over and over (on the grid, the chord stopped) */
    run_fx(PF_S8T, g_burst, FS * 2u, FS / 4u, FS * 3u / 2u, 0);
    for (t = FS * 3u / 5u; t < FS * 3u / 5u + beat; t++) assert(ol[t] == ol[t + beat / 3u]);
    assert(rms(ol, FS * 3u / 5u, FS * 3u / 5u + beat) > 0.3 * rms(ol, FS / 10u, FS / 5u));
    /* 1/4 with PITCH +12: the repeat read twice as fast (the 220 Hz chord note comes back at 440) */
    sweep_hz = 220;
    perf.par[PF_S4][0] = 12;
    run_fx(PF_S4, g_sine, FS * 2u, FS / 4u, FS * 3u / 2u, 0);
    a = tone_pow(ol, FS * 3u / 4u, FS * 5u / 4u, 440), b2 = tone_pow(ol, FS * 3u / 4u, FS * 5u / 4u, 220);
    printf("1/4 PITCH +12: 440 / 220 Hz power %.1f\n", a / b2);
    assert(a > 10 * b2);
    perf.par[PF_S4][0] = 0;
    /* 1/8 with DECAY 50 %: each repeat half the one before */
    perf.par[PF_S8][1] = 50;
    sweep_hz = 220;
    run_fx(PF_S8, g_sine, FS * 2u, FS / 4u, FS * 3u / 2u, 0);
    {
        uint32_t len = beat / 2u, s0 = FS / 4u + len, k;
        double r[3];
        for (k = 0; k < 3u; k++) r[k] = rms(ol, s0 + k * len + 200u, s0 + (k + 1u) * len - 200u);
        printf("1/8 DECAY 50: repeats %.0f %.0f %.0f\n", r[0], r[1], r[2]);
        assert(fabs(r[1] / r[0] - 0.5) < 0.05 && fabs(r[2] / r[1] - 0.5) < 0.05);
    }
    perf.par[PF_S8][1] = 0;
    /* 1/4 with GATE 50 %: the second half of each repeat silent */
    perf.par[PF_S4][2] = 50;
    run_fx(PF_S4, g_sine, FS * 2u, FS / 4u, FS * 3u / 2u, 0);
    {
        uint32_t s0 = FS / 4u + beat;
        assert(rms(ol, s0 + 200u, s0 + beat / 2u - 200u) > 0.5 * 70711 && rms(ol, s0 + beat / 2u + 200u, s0 + beat - 200u) < 0.01 * 70711);
    }
    perf.par[PF_S4][2] = 100;
    /* AMOUNT 50 %: half wet */
    perf.par[PF_STOP][3] = 50;
    run_fx(PF_STOP, g_sine, FS * 2u, FS / 2u, FS * 3u / 2u, 0);
    a = rms(ol, FS / 2u + beat + 200u, FS * 3u / 2u) / rms(ol, 0, FS / 2u);
    printf("tape stop at AMOUNT 50: %.2f of the level left after the stop\n", a);
    assert(a > 0.4 && a < 0.6);
    perf.par[PF_STOP][3] = 100;
    /* FLANGER: stereo, bounded, different */
    run_fx(PF_FLNG, g_burst, FS, FS / 10u, FS * 9u / 10u, 0);
    for (t = FS / 5u, i = 0; t < FS * 2u / 5u; t++) i += ol[t] != or_[t];
    assert(i > FS / 10u);
    song.playing = 0;
    printf("pads: LOW CUT, LP, HP, PITCH DROP, GATE, CRUSH, TAPE STOP, 1/8T, the stutters' PITCH, DECAY, GATE, AMOUNT do what they say PASS\n");
}

/* (wave 7) the loops' start in 32 bits (the device has no __umoddi3) equals the 64-bit form over every real value:
 * every len 64 .. PUNCH_N - 1024, every k < len (sampled densely, all at the ends), every PITCH -12 .. +12 */
static void t_pq0(void)
{
    uint32_t len, n = 0;
    int32_t semi;
    for (semi = -12; semi <= 12; semi++) {
        uint32_t sp = (uint32_t)pr_ratio(semi << 8);
        for (len = 64u; len <= PUNCH_N - 1024u; len += len < 2048u ? 1u : 37u) {
            uint32_t k, st = len < 4096u ? 1u : 13u;
            for (k = 0; k < len; k += (k < 64u || k + 64u > len) ? 1u : st) {
                uint32_t want = (uint32_t)(((uint64_t)k * sp) % ((uint64_t)len << 16));
                assert(punch_pq0(k, sp, len) == want);
                n++;
            }
        }
        {   /* the largest len, every k */
            uint32_t k, L = PUNCH_N - 1024u;
            for (k = 0; k < L; k++)
                assert(punch_pq0(k, sp, L) == (uint32_t)(((uint64_t)k * sp) % ((uint64_t)L << 16)));
        }
    }
    for (len = 64u; len <= PUNCH_N - 1024u; len += 97u)   /* and any speed up to 2^18 */
        for (n = 0; n < 4096u; n++) {
            uint32_t k = (n * 2654435761u) % len, sp = (n * 40503u) & 0x3FFFFu;
            assert(punch_pq0(k, sp, len) == (uint32_t)(((uint64_t)k * sp) % ((uint64_t)len << 16)));
        }
    printf("loops' read start in 32 bits = the 64-bit modulo, every len, k, PITCH PASS\n");
}

int main(int argc, char **argv)
{
    setvbuf(stdout, 0, _IONBF, 0);
    perf_init();
    t_pq0();
    uint32_t fx, i, b, on = FS / CTL, off = on + FS / 2u / CTL;   /* held from 1 s to 1.5 s */
    FILE *f = argc > 1 ? fopen(argv[1], "wb") : 0;
    for (fx = 0; fx < PF_NPAD; fx++) {
        uint64_t diff = 0;
        if (fx == PF_S1 || fx == PF_S2 || fx == PF_ECHO || fx == PF_VERB || fx == PF_KDRUM || fx == PF_KBASS)
            continue;                                 /* (the sequencer, the sends, the mutes: tests/perf_test.c) */
        beat_setup();
        song.playing = 1;
        clk_pos = 0;
        for (b = 0; b < BLOCKS; b++) {
            int32_t l[CTL], r[CTL], l0[CTL], r0[CTL];
            test_mix(b, l, r);
            memcpy(l0, l, sizeof l), memcpy(r0, r, sizeof r);
            if (b == on) perf.held = 1u << fx;
            if (b == off) perf.held = 0;
            stl_seq();
            punch_process(l, r, CTL);
            clk_pos += CTL * (uint32_t)song.g[G_BPM];   /* (the transport clock) */
            while (clk_pos >= BEAT_U) clk_pos -= BEAT_U, clk_beat++;
            for (i = 0; i < CTL; i++) {
                assert(l[i] > -(1 << 22) && l[i] < (1 << 22) && r[i] > -(1 << 22) && r[i] < (1 << 22));
                if (b < on || b >= off + 3u)
                    assert(l[i] == l0[i] && r[i] == r0[i]);   /* dry before, dry again after */
                else
                    diff += (uint64_t)(l[i] > l0[i] ? l[i] - l0[i] : l0[i] - l[i]);
            }
        }
        if (diff <= 2000u * (off - on))
            printf("fx %u (%s): diff %llu\n", fx, PFD[fx].name, (unsigned long long)diff);
        assert(diff > 2000u * (off - on));            /* it did something */
        assert(punch.cur == -1);
        song.playing = 0;
    }
    {   /* the keyboard while FX is held: the keys pick pads; nothing sounds, nothing records */
        uint32_t k;
        beat_setup();
        song.sel = 0;
        song.rec = 1u;
        mix_block(got, CTL);
        ly_bit[LY_FX] = 1u << 2;                      /* FX held (seq.c layer_now) */
        fm1_in.buttons = ly_bit[LY_FX];
        uint32_t seen = 0;
        for (k = 0; k < 27u; k++) {
            int32_t w = punch_fx_of_key(k);
            fm1_in.notes = 1u << k;
            events_block(CTL);
            assert(w < (int32_t)PF_NPAD && (w < 0 ? !perf.held : perf.held == 1u << w));
            if (w >= 0) {
                assert(punch_key_of_fx((uint32_t)w) == k);   /* (the key that lights) */
                assert(punch_key(k) < 0 ? w >= 16 : w == punch_key(k));
                seen |= 1u << w;
            }
            assert(!trk[0].step[0].n && !trk[0].v[0].active);
            fm1_in.notes = 0;
            events_block(CTL);
            assert(!perf.held);
        }
        assert(seen == (1u << PF_NPAD) - 1u);           /* every pad on a key of its own */
        assert(punch_key(0) == 0 && punch_key(26) == 15);
        assert(punch_fx_of_key(1) == PF_KDRUM && punch_fx_of_key(8) == PF_FLNG && punch_fx_of_key(20) == PF_S8T &&
               punch_fx_of_key(25) == -1);
        fm1_in.buttons = 0;
        song.rec = 0;
        transport_req = 2;
        events_block(CTL);
    }
    punch_pads_say();
    {   /* (PHEBUS wave 1) the ring is 16 bits linear (the CLOUD that shared its words is cut): a mix written idle reads
         * back within one step of its scale (mix >> PUNCH_SH), loud and quiet alike (8-bit mu-law was ~35 dB on a loud mix) */
        static int32_t l[CTL], r[CTL];
        uint32_t b, i, w0;
        int32_t worst = 0;
        perf.held = 0; perf.ring = -1; punch.cur = -1; punch.g = 0;
        w0 = punch.w;
        for (b = 0; b < 64u; b++) {
            for (i = 0; i < CTL; i++) l[i] = r[i] = (int32_t)(((b * CTL + i) * 2654435761u) >> 17) - 32768 >> (b & 7u);
            punch_process(l, r, CTL);
        }
        for (b = 0; b < 64u; b++)
            for (i = 0; i < CTL; i++) {
                int32_t x = (int32_t)(((b * CTL + i) * 2654435761u) >> 17) - 32768 >> (b & 7u), d = x - ring_at(w0 + b * CTL + i);
                if (d < 0) d = -d;
                if (d > worst) worst = d;
            }
        assert(worst < (1 << PUNCH_SH));
        printf("punch-in ring: 16 bits linear, a sample read back within %d of the mix (its step: %d)\n", (int)worst, 1 << PUNCH_SH);
    }
    if (f) {   /* demo: one beat dry, then each effect for one beat with a beat dry between */
        uint32_t beat = FS / 2u / CTL, b, total = beat * (1u + 2u * PF_NPAD);
        static int32_t blk[CTL * 2u];
        wav_hdr(f, total * CTL);
        beat_setup();
        TDRUM->p[P_E0] = DRUM_SAMPLED + 1u;           /* 909 */
        for (b = 0; b < total; b++) {
            uint32_t q;
            int32_t e = b < beat ? -1 : (int32_t)((b - beat) / beat);
            perf.held = e >= 0 && e % 2 == 0 ? 1u << (e / 2) : 0u;
            mix_block(blk, CTL);
            for (q = 0; q < CTL; q++) wav_put(f, blk[2 * q], blk[2 * q + 1]);
        }
        fclose(f);
    }
    printf("PERFORM pads on the punch ring: each changes the mix while held, bounded, exact dry mix after release; "
           "FX-held keys pick pads, play and record nothing PASS\n");
    return 0;
}
