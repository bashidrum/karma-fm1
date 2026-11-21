/* SPDX-License-Identifier: GPL-3.0-only */
/* The STUTTER lane (PHEBUS, 21 Nov 2026; punch.c stl_*): a lit step makes the whole mix repeat the span played just
 * before its run of lit steps, at the step's length, for as long as the run lasts, then the mix comes back exactly.
 * A 4-on-the-floor beat with a bassline at 120 BPM, the lane 32 steps long: 1/8 on steps 13..16, 1/4 on 29..32.
 * Checked on the output (after the master and its limiter), against the same render with the lane off:
 *   - nothing differs before step 13, and the render with no lit step is the render with the lane off, sample for sample
 *   - during the run the output repeats with the step's period (consecutive periods the same; the autocorrelation's
 *     peak at that lag) and is the mix of the span before the run (mono, the punch ring's 8-bit mu-law)
 *   - after the run the output is the dry render again (after the 64-sample fade), and no step at the seams is bigger
 *     than the steps of the music around it
 *   - live punch-in (FX held + a key) takes over the lane while held
 * argv[1]: a WAV of the render (optional). Prints the numbers. */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
#include <math.h>
#define BPM 120u
#define STEP_S (FS * 60u / BPM / 4u)                  /* a 1/16, samples (5512.5 at 120: rounded down) */
#define NSAMP (FS * 8u)                               /* two passes of the 32-step lane: 8 s */
#include <sys/mman.h>
static int32_t *out_on, *out_off, *out_none, *out_re, *mix_on, *mix_off;   /* (shared: each render in a child, from power-on state) */
static int fails;
static void check(int ok, const char *what) { printf("stutter: %-78s %s\n", what, ok ? "ok" : "FAIL"); fails += !ok; }

static void song_setup(void)
{
    static const uint8_t BASS[16] = {36, 0, 36, 48, 0, 36, 39, 0, 36, 0, 43, 36, 0, 46, 36, 0};
    uint32_t j;
    host_tracks_init();
    song.g[G_BPM] = BPM;
    song.master_q12 = 2048;
    memset(&drums, 0, sizeof drums);
    drums.set = -2;
    TDRUM->p[P_E0] = DRUM_DEFAULT_KIT;
    for (j = 0; j < 16u; j++) {
        static const uint8_t KICK[1] = {36}, KS[2] = {36, 38}, HAT[1] = {42};
        if (j % 4u == 0u)
            put_step(TDRUM, j, j % 8u == 4u ? 2u : 1u, j % 8u == 4u ? KS : KICK, ST_NOTE, 0);
        else if (j % 2u == 0u)
            put_step(TDRUM, j, 1, HAT, ST_NOTE, 0);
        else
            put_step(TDRUM, j, 0, 0, ST_REST, 0);
        if (BASS[j])
            put_step(&trk[0], j, 1, &BASS[j], ST_NOTE, 0);
        else
            put_step(&trk[0], j, 0, 0, ST_REST, 0);
    }
    TDRUM->p[P_SLEN] = 16;
    trk[0].p[P_SLEN] = 16;
    trk[0].p[P_VOICE] = V_MONO;
    trk[0].seq_active = 1;
    TDRUM->seq_active = 1;
    memset(&stl, 0, sizeof stl);
    stl.len = 32;
    stl.act = 1;
    stl_want = stl_was = -1;
    punch.req = -1, punch.cur = -1, punch.g = 0, punch.w = 0;
    memset(punch_ring, 0, sizeof punch_ring);
    clk_beat = 0, clk_pos = 0;
    transport_req = 1;
}
static void lane_set(uint32_t s, uint32_t code)
{
    stl.on[s >> 3] |= (uint8_t)(1u << (s & 7u));
    stl.lc[s >> 2] = (uint8_t)((stl.lc[s >> 2] & ~(3u << 2u * (s & 3u))) | code << 2u * (s & 3u));
}
/* retrig: every step of a run starts the loop again (the alternative tried) */
static int32_t *mix_to;                               /* the mix after the punch-in, before the master (its DC blocker) */
static void render_in(int32_t *o, int retrig)
{
    uint32_t b;
    int32_t blk[CTL * 2u];
    uint8_t pos_was = 0xFF;
    for (b = 0; b < NSAMP / CTL; b++) {
        mix_block(blk, CTL);
        if (retrig && stl_pos != pos_was && stl_want >= 0 && punch.req < 0) {
            punch.cur = -1, punch.g = 0, stl_was = -1;   /* (after this block: the next one starts it again) */
        }
        pos_was = stl_pos;
        memcpy(o + b * CTL * 2u, blk, sizeof blk);
        if (mix_to) {
            uint32_t i;
            for (i = 0; i < CTL; i++)
                mix_to[2u * (b * CTL + i)] = mix_l[i], mix_to[2u * (b * CTL + i) + 1u] = mix_r[i];
        }
    }
}
/* the setup done, the render in a child process: every render from the same state (no tail of the last one) */
static void render(int32_t *o, int retrig)
{
    pid_t p = fork();
    int st = 0;
    if (p == 0) {
        render_in(o, retrig);
        _exit(0);
    }
    waitpid(p, &st, 0);
    assert(WIFEXITED(st) && WEXITSTATUS(st) == 0);
}
static double corr(const int32_t *a, const int32_t *b, uint32_t n, int ch_a, int ch_b, int mono_b)
{
    double sab = 0, saa = 0, sbb = 0;
    uint32_t i;
    for (i = 0; i < n; i++) {
        double x = a[2u * i + (uint32_t)ch_a], y = mono_b ? (b[2u * i] + b[2u * i + 1u]) * 0.5 : b[2u * i + (uint32_t)ch_b];
        sab += x * y, saa += x * x, sbb += y * y;
    }
    return saa > 0 && sbb > 0 ? sab / sqrt(saa * sbb) : 0;
}
static uint32_t first_diff(const int32_t *a, const int32_t *b, uint32_t from, uint32_t to)
{
    uint32_t i;
    for (i = from; i < to; i++)
        if (a[2u * i] != b[2u * i] || a[2u * i + 1u] != b[2u * i + 1u])
            return i;
    return to;
}
static int32_t max_step(const int32_t *o, uint32_t a, uint32_t b)   /* the biggest sample-to-sample step, both channels */
{
    int32_t m = 0;
    uint32_t i, c;
    for (i = a + 1u; i < b; i++)
        for (c = 0; c < 2u; c++) {
            int32_t d = o[2u * i + c] - o[2u * (i - 1u) + c];
            d = d < 0 ? -d : d;
            m = d > m ? d : m;
        }
    return m;
}
static int32_t max_diff(const int32_t *a, const int32_t *b, uint32_t from, uint32_t to)
{
    int32_t m = 0;
    uint32_t i;
    for (i = 2u * from; i < 2u * to; i++) {
        int32_t d = a[i] - b[i];
        d = d < 0 ? -d : d;
        m = d > m ? d : m;
    }
    return m;
}
/* a run of the lane: lit from step s0 for nst steps, loop length len samples; o the render, ref the lane off */
static void run_check(const char *name, const int32_t *o, const int32_t *ref, uint32_t s0, uint32_t nst, uint32_t len,
                      uint32_t pass)
{
    uint32_t base = pass * 32u * STEP_S, at = first_diff(o, ref, base + s0 * STEP_S - 2048u, base + (s0 + nst) * STEP_S + 4096u);
    uint32_t end = base + (uint32_t)((s0 + nst) * (double)FS * 60.0 / BPM / 4.0), span = (uint32_t)(nst * (double)FS * 60.0 / BPM / 4.0 + 0.5);
    uint32_t back, lag, best = 0, j, reps = span / len;
    double c_rep, c_src, c_best = -2, c_after;
    char b[160];
    int32_t d_after, st_seam, st_music;
    /* the onset: where the output first differs from the dry render (within a block of the step's start) */
    snprintf(b, sizeof b, "%s: the output changes at step %u's start (sample %u, the grid %u)", name, s0 + 1u, at,
             base + (uint32_t)(s0 * (double)FS * 60.0 / BPM / 4.0));
    check(at >= base + s0 * STEP_S - 64u && at <= base + s0 * STEP_S + 2u * CTL + 64u, b);
    /* consecutive periods the same (away from the seams' 64-sample declick) */
    c_rep = reps >= 2u ? corr(o + 2u * (at + 96u), o + 2u * (at + len + 96u), len - 192u, 0, 0, 0) : 1.0;
    {
        const int32_t *mo = o == out_on ? mix_on : o;   /* (the mix before the master: the output's DC blocker moves) */
        int32_t dmax = reps >= 2u ? max_diff(mo, mo + 2u * len, at + 96u, at + len - 96u) : 0;
        snprintf(b, sizeof b, "%s: period %u samples: one repeat is the one before (output corr %.6f, mix max diff %d)", name,
                 len, c_rep, dmax);
        check(c_rep > 0.9999 && dmax <= 64, b);         /* (the punch's mix at full: the dry under it at 1/4096, -72 dB) */
    }
    /* the autocorrelation of the run: its peak at the loop's length (searched from len / 2 to 3 len / 2) */
    if (reps >= 2u) {
        for (lag = len / 2u; lag <= len * 3u / 2u && at + lag + len < end; lag += 8u) {
            double c = corr(o + 2u * (at + 96u), o + 2u * (at + 96u + lag), len - 192u, 0, 0, 0);
            if (c > c_best)
                c_best = c, best = lag;
        }
        for (j = best > 8u ? best - 8u : 0u; j <= best + 8u; j++) {   /* (to the sample) */
            double c = corr(o + 2u * (at + 96u), o + 2u * (at + 96u + j), len - 192u, 0, 0, 0);
            if (c >= c_best)
                c_best = c, best = j;
        }
        snprintf(b, sizeof b, "%s: autocorrelation peak at lag %u (expected %u), %.6f", name, best, len, c_best);
        check(best >= len - 2u && best <= len + 2u, b);
    }
    /* what repeats: the span played just before the run (dry render), as the mono ring keeps it */
    back = at - len;
    c_src = corr(o + 2u * (at + 96u), ref + 2u * (back + 96u), len - 192u, 0, 0, 1);
    snprintf(b, sizeof b, "%s: it repeats the %u samples before the run (corr with the dry mix there %.4f)", name, len, c_src);
    check(c_src > 0.97, b);
    snprintf(b, sizeof b, "%s: the whole mix, mono, while it stutters (L = R: corr %.6f)", name,
             corr(o + 2u * (at + 96u), o + 2u * (at + 96u), len - 192u, 0, 1, 0));
    check(corr(o + 2u * (at + 96u), o + 2u * (at + 96u), len - 192u, 0, 1, 0) > 0.99999, b);
    /* after the run: the dry mix again, exactly, once the 64-sample fade is done (before the master); the output after
     * the master's DC blocker settles to it (its state saw the stutter): printed */
    {
        const int32_t *mo = o == out_on ? mix_on : 0, *mr = mix_off;
        uint32_t last = end, i2;
        for (i2 = end; mo && i2 < end + 3u * STEP_S; i2++)
            if (mo[2u * i2] != mr[2u * i2] || mo[2u * i2 + 1u] != mr[2u * i2 + 1u])
                last = i2;
        d_after = max_diff(o, ref, end + 2u * CTL + 64u, end + 3u * STEP_S);
        c_after = corr(o + 2u * (end + 2u * CTL + 64u), ref + 2u * (end + 2u * CTL + 64u), 3u * STEP_S - 2u * CTL - 64u, 0, 0, 0);
        snprintf(b, sizeof b, "%s: the mix is the dry one, sample for sample, from %u samples after the grid", name, last + 1u - end);
        check(mo && last + 1u <= end + 2u * CTL + 64u, b);
        snprintf(b, sizeof b, "%s: the output then (DC blocker settling): max diff %d, corr %.6f, %d a step later", name,
                 d_after, c_after, max_diff(o, ref, end + STEP_S * 4u, end + STEP_S * 5u));
        check(c_after > 0.99, b);
    }
    /* no click: the steps at the seams (in and out) no bigger than the music's around them */
    st_seam = max_step(o, at - 64u, at + 192u);
    if (max_step(o, end - 64u, end + 2u * CTL + 192u) > st_seam)
        st_seam = max_step(o, end - 64u, end + 2u * CTL + 192u);
    st_music = max_step(ref, at - 2u * STEP_S, end + 2u * STEP_S);
    snprintf(b, sizeof b, "%s: the seams' largest sample step %d, the dry music's around them %d", name, st_seam, st_music);
    check(st_seam <= st_music, b);
}

int main(int argc, char **argv)
{
    uint32_t i, l8 = FS * 60u / BPM / 2u, l4 = FS * 60u / BPM;
    int32_t *m = mmap(0, 6u * NSAMP * 2u * sizeof(int32_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    assert(m != MAP_FAILED);
    out_on = m, out_off = m + NSAMP * 2u, out_none = m + NSAMP * 4u, out_re = m + NSAMP * 6u;
    mix_on = m + NSAMP * 8u, mix_off = m + NSAMP * 10u;
    /* the lane off, and the lane on with no lit step: the same render */
    song_setup();
    for (i = 12; i < 16; i++) lane_set(i, 1);
    for (i = 28; i < 32; i++) lane_set(i, 0);
    stl.act = 0;
    mix_to = mix_off;
    render(out_off, 0);
    mix_to = 0;
    song_setup();
    render(out_none, 0);
    check(!memcmp(out_off, out_none, NSAMP * 2u * sizeof(int32_t)), "no lit step: the render is the lane off's, sample for sample");
    {
        double e = 0;
        for (i = 0; i < 2u * NSAMP; i++) e += (double)out_off[i] * out_off[i];
        printf("stutter: (the beat's level: rms %.0f)\n", sqrt(e / (2.0 * NSAMP)));
    }
    /* the lane on: 1/8 on 13..16, 1/4 on 29..32 */
    song_setup();
    for (i = 12; i < 16; i++) lane_set(i, 1);
    for (i = 28; i < 32; i++) lane_set(i, 0);
    mix_to = mix_on;
    render(out_on, 0);
    mix_to = 0;
    check(first_diff(out_on, out_off, 0, NSAMP) >= 12u * STEP_S - 64u, "nothing changes before step 13");

    for (i = 0; i < 2u; i++) {
        char nm[32];
        snprintf(nm, sizeof nm, "1/8 on 13-16, pass %u", i + 1u);
        run_check(nm, out_on, out_off, 12, 4, l8, i);
        snprintf(nm, sizeof nm, "1/4 on 29-32, pass %u", i + 1u);
        if (i == 0u)
            run_check(nm, out_on, out_off, 28, 4, l4, i);
    }
    /* the alternative: each step starts its loop again (not chosen: printed for the record) */
    song_setup();
    for (i = 12; i < 16; i++) lane_set(i, 1);
    render(out_re, 1);
    {
        uint32_t at = first_diff(out_re, out_off, 0, NSAMP);
        printf("stutter: (retrigger each step, 1/8 on 13-16: consecutive 1/8s corr %.4f; one loop: %.4f)\n",
               corr(out_re + 2u * (at + 96u), out_re + 2u * (at + l8 + 96u), l8 - 192u, 0, 0, 0),
               corr(out_on + 2u * (at + 96u), out_on + 2u * (at + l8 + 96u), l8 - 192u, 0, 0, 0));
    }
    /* live punch-in takes over: FX + LOOP 16 held through the 1/8 run */
    song_setup();
    for (i = 12; i < 16; i++) lane_set(i, 1);
    {
        uint32_t b;
        int32_t blk[CTL * 2u];
        uint32_t held = 0;
        for (b = 0; b < NSAMP / CTL; b++) {
            uint32_t t = b * CTL;
            punch.req = (int8_t)(t >= 11u * STEP_S && t < 17u * STEP_S ? PX_LOOP16 : -1);
            mix_block(blk, CTL);
            if (t > 13u * STEP_S && t < 16u * STEP_S)
                held += punch.cur == PX_LOOP16;
        }
        check(held == (3u * STEP_S - CTL) / CTL + 1u || held >= (3u * STEP_S) / CTL - 2u, "FX held + a key: its effect plays over the lane's steps");
        punch.req = -1;
    }
    if (argc > 1) {
        FILE *f = fopen(argv[1], "wb");
        if (f) {
            wav_hdr(f, NSAMP);
            for (i = 0; i < NSAMP; i++)
                wav_put(f, out_on[2u * i], out_on[2u * i + 1u]);
            fclose(f);
        }
    }
    printf(fails ? "stutter: %d FAILED\n" : "stutter: all ok\n", fails);
    return fails != 0;
}
