/* SPDX-License-Identifier: GPL-3.0-only */
/* (3.1.3) the two-core mix on the host: -include this with -DKARMA_DUAL_CORE=1 (tests/run_tests.sh: regress in two-core
 * mode). CPU1's job runs when CPU0 joins it, so CPU0's work and CPU1's happen in that order: the output must be the
 * one-core output, bit for bit (integer sums), whatever the split; the golden renders check it (as SLOOP's
 * tests/cpu1_modes.sh). fm1_ticks: 0, so the balance keeps the split it starts with (tracks 2 and 3, and the HEX
 * voices, on CPU1) */
#pragma once
#include <stdint.h>
typedef void (*audio_worker_fn)(void *);
static int audio_worker_online = 1;
static audio_worker_fn aw_fn;
static void *aw_ctx;
static int aw_pending;
static uint32_t aw_jobs;
static int audio_worker_submit(audio_worker_fn fn, void *context)
{
    aw_fn = fn;
    aw_ctx = context;
    aw_pending = 1;
    return 1;
}
static int audio_worker_join(void)
{
    if (aw_pending) {
        aw_pending = 0;
        aw_fn(aw_ctx);
        aw_jobs++;
    }
    return 1;
}
static void audio_worker_check(void) {}
static inline uint32_t fm1_ticks(void) { return 0; }
