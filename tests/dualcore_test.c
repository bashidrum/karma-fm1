/* SPDX-License-Identifier: GPL-3.0-only */
/* (3.1.3) the HEX voices on CPU1 (drums.c drums_hex_pre, fx.c core1_parts): with KARMA_DUAL_CORE=1 and
 * tests/dualcore_host.h, a HEX beat through the whole mix: CPU1's job ran, rendered the HEX voices, and the output is
 * the one-core output, bit for bit (the same beat with the job's HEX block refused: hx_pre_mask cleared at the join) */
#define main hostsim_main
#include <stddef.h>
#include "hostsim.c"
#undef main
static uint32_t pre_blocks;
static uint64_t render(int use_cpu1)
{
    uint32_t i, b;
    int32_t o[2 * CTL];
    uint64_t h = 1469598103934665603ull;
    for (i = 0; i < G_COUNT; i++) song.g[i] = GP[i].def;
    host_tracks_init();
    memset(&drums, 0, sizeof drums);
    TDRUM->p[P_E0] = 0;                                   /* HEX BOOM */
    song.g[G_DRREV] = 64;
    song.g[G_DRDLY] = 64;
    for (b = 0; b < FS * 4 / CTL; b++) {
        uint32_t st = (b * CTL) / (FS / 8);
        if ((b * CTL) % (FS / 8) < CTL) {
            if (st % 4 == 0) drum_on(36, 110);
            if (st % 8 == 4) drum_on(38, 110);
            drum_on(42, 90);
            if (st % 16 == 14) drum_on(46, 110);
        }
        if (!use_cpu1)
            w_mask = 0;                                   /* (no job: one core) */
        mix_block(o, CTL);
        if (use_cpu1 && w_hex)
            pre_blocks++;
        for (i = 0; i < 2 * CTL; i++)
            h = (h ^ (uint32_t)o[i]) * 1099511628211ull;
    }
    return h;
}
int main(int argc, char **argv)
{
    /* one mode a run (a fresh process each: the effects' lines start empty); tests/run_tests.sh compares the two */
    int cpu1 = argc > 1 && argv[1][0] == '2';
    uint64_t h;
    w_mask = cpu1 ? 6u : 0u;
    h = render(cpu1);
    printf("%016llx %s: CPU1 jobs %u, blocks with the HEX voices on CPU1 %u\n", (unsigned long long)h,
           cpu1 ? "two cores" : "one core", aw_jobs, pre_blocks);
    return cpu1 && pre_blocks < 1000u;
}
