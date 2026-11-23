/* SPDX-License-Identifier: GPL-3.0-only */
/* PHEBUS wave 2 (Ben, 21 Nov 2026, picker "Per-sound trigs, one track": "each sound's hits get their own velocity, condition,
 * nudge, retrig and locks; all sounds share one pattern length"): the drum trigs as the sequencer plays them.
 *   locks      a lock of the snare's parameter on step 5 changes the snare on step 5 only: not the kick on the same step,
 *              not the snare on another step, not the kit
 *   velocity   each trig's velocity is its hit's (and scales its level)
 *   chance     CHNC per trig at its own percentage, drawn per trig (seeded: the same counts every run)
 *   conditions FILL / NOT FILL per trig, two sounds on one step each by its own
 *   retrig     a trig's retrig: its rate, its length (INF: up to the sound's next trig), its velocity curve
 *   nudge      two sounds on one step, each at its own nudge (late, early: a step ahead, across the loop)
 * Exit status: the number of failed checks. */
#define main hostsim_main
#include "hostsim.c"
#undef main

static int fails;
static void check(int ok, const char *what) { printf("drumtrig: %-96s %s\n", what, ok ? "ok" : "FAIL"); fails += !ok; }

typedef struct { uint64_t blk; uint8_t note, vel, own; uint8_t cut, lvl; } hit_t;
static hit_t hits[20000];
static uint32_t nhits;
static uint64_t blk;
static void run_block(void)                        /* a block, and every drum hit that started in it */
{
    int32_t out[CTL * 2];
    uint32_t a = drums.age, k;
    mix_block(out, CTL);
    if (drums.age != a)
        for (k = 0; k < NDRUM; k++)
            if (drums.v[k].age > a && nhits < 20000u) {
                hit_t *h = &hits[nhits++];
                h->blk = blk;
                h->note = drums.v[k].note;
                h->vel = drums.v[k].vel;
                h->own = drums.synth[k] == 2 && drums.vx[k].hx.d == &drum_own[k];
                h->cut = drums.synth[k] == 2 ? drums.vx[k].hx.d->cut : 0;
                h->lvl = drums.synth[k] == 2 ? drums.vx[k].hx.d->level : 0;
            }
    blk++;
}
static void run(uint32_t n) { while (n--) run_block(); }
static void reset(uint32_t kit)
{
    uint32_t i;
    host_tracks_init();
    for (i = 0; i < NTRK; i++)
        steps_clear(&trk[i]);
    memset(&drums, 0, sizeof drums);
    nhits = 0;
    blk = 0;
    song.g[G_BPM] = 120;
    song.g[G_CLOCK] = 0;
    song.rec = 0;
    kit_set((uint32_t)((int16_t)kit));
    TDRUM->p[P_SLEN] = 16;
    TDRUM->p[P_SDIV] = 2;                          /* 1/16 */
    drum_mute = 0;
    fill_held = 0;
    fill_arm = 0;
}
static uint32_t step_blocks(void) { return div_samples(2) / CTL; }   /* (about: a 1/16 at 120 BPM, 172 blocks) */
static uint32_t count(uint32_t note, uint64_t b0, uint64_t b1)
{
    uint32_t i, n = 0;
    for (i = 0; i < nhits; i++)
        n += hits[i].note == note && hits[i].blk >= b0 && hits[i].blk < b1;
    return n;
}
static const hit_t *nth(uint32_t note, uint32_t k)  /* the k-th hit of note */
{
    uint32_t i;
    for (i = 0; i < nhits; i++)
        if (hits[i].note == note && !k--)
            return &hits[i];
    return 0;
}

static void t_locks(void)
{
    const hex_snd_t *kit = HEX_KITS[0].s;          /* HEX BOOM */
    int32_t semi;
    const hex_snd_t *sn = &kit[ds_lane(LANE_NOTE[2], &semi)], *kk = &kit[ds_lane(LANE_NOTE[0], &semi)];
    const hit_t *s0, *s1, *s2, *k1;
    uint8_t cut_before = sn->cut;
    reset(0);
    dt_set(0, 2, 100);                             /* the snare on steps 1, 5 and 9; the kick on 5 */
    dt_set(4, 2, 100);
    dt_set(8, 2, 100);
    dt_set(4, 0, 100);
    dt_lk_set(4, 2, DSP_CUT, 20);                  /* the snare's cutoff on step 5: 20 (its kit has 100), its level 60 */
    dt_lk_set(4, 2, DSP_LEVEL, 60);
    transport_req = 1;
    run(step_blocks() * 10u);
    transport_req = 2;
    run_block();
    s0 = nth(38, 0), s1 = nth(38, 1), s2 = nth(38, 2), k1 = nth(36, 0);
    check(s0 && s1 && s2 && k1 && k1->blk == s1->blk, "locks: the snare on 1, 5, 9 and the kick on 5 played");
    check(s1 && s1->own && s1->cut == 20 && s1->lvl == 60, "locks: the snare on step 5 plays its locked cutoff (20) and level (60)");
    check(k1 && !k1->own && k1->cut == kk->cut && k1->lvl == kk->level,
          "locks: the kick on the same step plays as its kit has it (the lock is the snare's alone)");
    check(s0 && s2 && !s0->own && !s2->own && s0->cut == cut_before && s2->cut == cut_before,
          "locks: the snare on steps 1 and 9 plays as the kit has it (the lock is step 5's alone)");
    check(sn->cut == cut_before && sn->level == HEX_KITS[0].s[ds_lane(LANE_NOTE[2], &semi)].level,
          "locks: the kit itself is not changed");
    {   /* a synthesised kit (the 606): a HEX byte's lock is not one of its machine's parameters, so it is not applied; the
         * hit plays (its own parameters lock since PHEBUS wave 3: tests/drumrytm_test.c) */
        reset(DRUM_DEFAULT_KIT);
        dt_set(4, 2, 100);
        dt_lk_set(4, 2, DSP_CUT, 20);
        transport_req = 1;
        run(step_blocks() * 6u);
        transport_req = 2;
        run_block();
        check(count(38, 0, blk) == 1u && !nth(38, 0)->own, "locks: a HEX lock on a synthesised kit: not its machine's, the snare plays unlocked");
    }
}

static void t_velocity(void)
{
    static const uint8_t V[4] = {20, 64, 100, 127};
    uint32_t i, ok = 1;
    double e[4];
    reset(DRUM_DEFAULT_KIT);
    for (i = 0; i < 4u; i++)
        dt_set(i * 4u, 0, V[i]);                   /* the kick on 1, 5, 9, 13 at 20, 64, 100, 127 */
    transport_req = 1;
    run(step_blocks() * 16u - 4u);
    transport_req = 2;
    run_block();
    for (i = 0; i < 4u; i++)
        ok &= nth(36, i) && nth(36, i)->vel == V[i];
    check(ok && count(36, 0, blk) == 4u, "velocity: each trig's hit at its own velocity (20, 64, 100, 127)");
    for (i = 0; i < 4u; i++) {                     /* the level: each hit alone, its energy */
        uint32_t b, k;
        int32_t o[CTL * 2];
        memset(&drums, 0, sizeof drums);
        e[i] = 0;
        dt_set(0, 0, V[i]);
        trk_note_on(TDRUM, LANE_NOTE[0], dt_vel(0, 0));
        for (b = 0; b < (uint32_t)(FS / 4 / CTL); b++) {
            mix_block(o, CTL);
            for (k = 0; k < CTL * 2u; k++) e[i] += (double)o[k] * o[k];
        }
    }
    check(e[0] < e[1] && e[1] < e[2] && e[2] < e[3] && e[0] > 0, "velocity: a higher velocity, a louder hit");
}

static void t_chance(void)
{
    uint32_t a, b, c, i, k, same = 0, n;
    dtx_t *x;
    reset(DRUM_DEFAULT_KIT);
    TDRUM->p[P_SLEN] = 1;                          /* one step: 16 trials a bar */
    dt_set(0, 0, 100), dt_set(0, 2, 100), dt_set(0, 4, 100);
    x = dt_x_make(0, 0), x->cond = FC_CHANCE, x->prob = 25;   /* the kick 25 %, the snare 75 %, the hat always */
    x = dt_x_make(0, 2), x->cond = FC_CHANCE, x->prob = 75;
    rng_state = 0x1234567u;                        /* (seeded) */
    transport_req = 1;
    run(step_blocks() * 16u * 32u - 4u);           /* 32 bars: 512 trials */
    transport_req = 2;
    run_block();
    a = count(36, 0, blk), b = count(38, 0, blk), c = count(42, 0, blk);
    printf("drumtrig:   chance: kick %u, snare %u, hat %u of 512\n", a, b, c);
    check(c == 512u && a >= 89u && a <= 167u && b >= 345u && b <= 423u,   /* (4 standard deviations either side) */
          "chance: CHNC 25 % plays about a quarter, CHNC 75 % about three quarters, no condition every time");
    for (i = 0, n = 0; i < nhits; i++)              /* drawn per trig: the kick and the snare not always together */
        if (hits[i].note == 36) {
            for (k = 0; k < nhits && !(hits[k].note == 38 && hits[k].blk == hits[i].blk); k++)
                ;
            same += k < nhits;
            n++;
        }
    check(n && same < n, "chance: each trig drawn on its own (the kick sometimes plays without the snare)");
    for (i = 0, n = 0; i < 10000u; i++)
        n += dt_plays(dt_x(0, 0));
    check(n > 2300u && n < 2700u, "chance: dt_plays at 25 %: 2500 of 10000, give or take");
    dpat.x[dt_x(0, 0)].prob = 100;
    for (i = 0, n = 0; i < 1000u; i++)
        n += dt_plays(dt_x(0, 0));
    dpat.x[dt_x(0, 0)].prob = 1;
    for (i = 0, k = 0; i < 1000u; i++)
        k += dt_plays(dt_x(0, 0));
    check(n == 1000u && k < 40u, "chance: 100 % always, 1 % almost never");
}

static void t_cond(void)
{
    uint64_t b0, b1;
    reset(DRUM_DEFAULT_KIT);
    dt_set(4, 2, 100), dt_set(4, 4, 100);          /* step 5: the snare FILL ONLY, the hat NOT FILL */
    dt_x_make(4, 2)->cond = FC_FILL;
    dt_x_make(4, 4)->cond = FC_NOFILL;
    transport_req = 1;
    run(step_blocks() * 16u);
    b0 = blk;
    fill_held = 1;
    run(step_blocks() * 16u);
    b1 = blk;
    fill_held = 0;
    transport_req = 2;
    run_block();
    check(count(38, 0, b0) == 0u && count(42, 0, b0) == 1u && count(38, b0, b1) == 1u && count(42, b0, b1) == 0u,
          "conditions: two sounds on one step, each by its own (no fill: the hat; fill: the snare)");
}

static void t_retrig(void)
{
    uint32_t i, ok = 1, sb = step_blocks();
    dtx_t *x;
    int32_t g;
    reset(DRUM_DEFAULT_KIT);
    dt_set(0, 4, 100);                             /* the hat on 1: retrig 1/32, one step: 2 hits half a step apart */
    x = dt_x_make(0, 4), x->rrate = 13, x->rlen = 16;
    dt_set(4, 2, 120);                             /* the snare on 5: 1/64 over two steps, fading out: 8 hits, softer and softer */
    x = dt_x_make(4, 2), x->rrate = 16, x->rlen = 32, x->rfade = -128;
    dt_set(8, 3, 100);                             /* the clap on 9: 1/16, INF: until its next trig (13) */
    x = dt_x_make(8, 3), x->rrate = 10, x->rlen = 0;
    dt_set(12, 3, 100);
    dt_set(14, 0, 60);                             /* the kick on 15: 1/32 fading in */
    x = dt_x_make(14, 0), x->rrate = 13, x->rlen = 16, x->rfade = 127;
    transport_req = 1;
    run(sb * 16u - 4u);
    transport_req = 2;
    run_block();
    g = nth(42, 1) ? (int32_t)(nth(42, 1)->blk - nth(42, 0)->blk) : 0;
    check(count(42, 0, blk) == 2u && g >= (int32_t)sb / 2 - 1 && g <= (int32_t)sb / 2 + 1,
          "retrig: 1/32 over one 1/16 step: 2 hits, half a step apart");
    for (i = 0; i < 8u; i++)
        ok &= nth(38, i) && (!i || nth(38, i)->vel < nth(38, i - 1u)->vel) &&
              (!i || (nth(38, i)->blk - nth(38, i - 1u)->blk) + 1u >= sb / 4u && nth(38, i)->blk - nth(38, i - 1u)->blk <= sb / 4u + 1u);
    check(ok && count(38, 0, blk) == 8u && nth(38, 0)->vel == 120,
          "retrig: 1/64 over two steps: 8 hits a quarter step apart, from 120 fading out");
    check(count(39, 0, blk) == 5u, "retrig: 1/16 INF from step 9: hits on 9, 10, 11, 12, then the clap's next trig on 13 ends it");
    check(count(36, 0, blk) == 2u && nth(36, 0)->vel < nth(36, 1)->vel, "retrig: a curve > 0 fades in (the second hit louder)");
}

static void t_nudge(void)
{
    uint32_t sb = step_blocks();
    const hit_t *k, *s, *c, *h, *r;
    double q = (double)FS * 60.0 / 120.0 / 4.0 / CTL / 4.0, bar = q * 64.0;   /* a quarter step, a bar, in blocks */
    reset(DRUM_DEFAULT_KIT);
    dt_set(4, 0, 100), dt_set(4, 2, 100);          /* step 5: the kick on the grid, the snare +16 (a quarter step late) */
    dt_x_make(4, 2)->micro = 16;
    dt_set(8, 3, 100), dt_set(8, 4, 100);          /* step 9: the clap on the grid, the hat -16 (a quarter step early) */
    dt_x_make(8, 4)->micro = -16;
    dt_set(0, 7, 100);                             /* step 1: the rim -32 (half a step early: across the loop, from 16) */
    dt_x_make(0, 7)->micro = -32;
    transport_req = 1;
    run(sb * 16u + sb / 2u);                       /* a bar and half a step: the rim's second hit, early */
    transport_req = 2;
    run_block();
    k = nth(36, 0), s = nth(38, 0), c = nth(39, 0), h = nth(42, 0), r = nth(37, 1);
    check(k && s && c && h && (double)(s->blk - k->blk) >= q - 1.5 && (double)(s->blk - k->blk) <= q + 1.5,
          "nudge: on one step the kick on the grid, the snare +16 a quarter step after it");
    check(c && h && (double)(c->blk - h->blk) >= q - 1.5 && (double)(c->blk - h->blk) <= q + 1.5,
          "nudge: on one step the clap on the grid, the hat -16 a quarter step before it");
    check(count(37, 0, blk) == 2u && r && nth(37, 0)->blk == 0u && (double)r->blk >= bar - 2.0 * q - 1.5 &&
          (double)r->blk <= bar - 2.0 * q + 1.5,
          "nudge: the first step's trig -32: at PLAY at once, then half a step before the loop comes round");
    check(count(36, 0, blk) == 1u && count(38, 0, blk) == 1u && count(39, 0, blk) == 1u && count(42, 0, blk) == 1u,
          "nudge: every nudged trig once, none doubled");
}

int main(void)
{
    t_locks();
    t_velocity();
    t_chance();
    t_cond();
    t_retrig();
    t_nudge();
    printf("drumtrig: %s\n", fails ? "FAILED" : "all passed");
    return fails;
}
