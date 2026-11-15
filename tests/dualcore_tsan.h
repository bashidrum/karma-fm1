/* SPDX-License-Identifier: GPL-3.0-only */
/* (3.1.4) the two-core mix on the host with CPU1's job on a REAL second thread, for ThreadSanitizer: what CPU1's job
 * and CPU0's parts touch at the same time. -include this with -DKARMA_DUAL_CORE=1 -fsanitize=thread (tests/tsan_dual.sh).
 * The submit / join handshake is a mutex and a condition, as the FM-1's is a mailbox with barriers: TSan sees it as the
 * synchronisation it is, so only real overlaps are reported. */
#pragma once
#include <stdint.h>
#include <pthread.h>
typedef void (*audio_worker_fn)(void *);
static int audio_worker_online = 1;
static audio_worker_fn aw_fn;
static void *aw_ctx;
static int aw_state;                       /* 0 idle, 1 submitted, 2 done */
static uint32_t aw_jobs;
static pthread_mutex_t aw_mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t aw_cv = PTHREAD_COND_INITIALIZER;
static pthread_t aw_th;
static int aw_started;
static void *aw_loop(void *x)
{
    (void)x;
    for (;;) {
        pthread_mutex_lock(&aw_mu);
        while (aw_state != 1)
            pthread_cond_wait(&aw_cv, &aw_mu);
        audio_worker_fn f = aw_fn;
        void *c = aw_ctx;
        pthread_mutex_unlock(&aw_mu);
        f(c);                              /* (outside the lock: in parallel with CPU0's parts) */
        pthread_mutex_lock(&aw_mu);
        aw_state = 2;
        aw_jobs++;
        pthread_cond_broadcast(&aw_cv);
        pthread_mutex_unlock(&aw_mu);
    }
    return 0;
}
static int audio_worker_submit(audio_worker_fn fn, void *context)
{
    if (!aw_started) {
        aw_started = 1;
        pthread_create(&aw_th, 0, aw_loop, 0);
    }
    pthread_mutex_lock(&aw_mu);
    aw_fn = fn;
    aw_ctx = context;
    aw_state = 1;
    pthread_cond_broadcast(&aw_cv);
    pthread_mutex_unlock(&aw_mu);
    return 1;
}
static int audio_worker_join(void)
{
    pthread_mutex_lock(&aw_mu);
    while (aw_state == 1)
        pthread_cond_wait(&aw_cv, &aw_mu);
    aw_state = 0;
    pthread_mutex_unlock(&aw_mu);
    return 1;
}
static void audio_worker_check(void) {}
static inline uint32_t fm1_ticks(void) { return 0; }
