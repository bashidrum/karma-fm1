/* SPDX-License-Identifier: GPL-3.0-only */
/* PHEBUS wave 3, the Rytm drum screens (Ben, 21 Nov 2026: "Drums: the Analog Rytm MKII's easy drum programming. Pick a sound,
 * 16 trig keys, hold a trig and turn to lock, sound pages in the Rytm's order (SRC, FLTR, AMP, LFO), mute, fill, retrig,
 * chromatic"; picker "Make every kit editable" (keep the 606 start; turning a knob on any kit makes your own editable copy)):
 *   copy-on-write  the 606 edited: the built-in kit untouched, the project's copy plays, is saved with the project and loads
 *                  back; on another kit the copy does not play; a project without one: none
 *   606 lock       a lock of the 606 snare's decay on one step: that hit alone (the kit and the other steps as they are)
 *   LFO, sends     a sound's LFO moves its hit (pitch), a locked send is the hit's own
 *   mute, solo     MUTE mode's keys mute a sound in the sequencer; a solo leaves only that one
 *   fill           FILL trigs play with the fill held or latched, and not after; NOT FILL the other way round
 *   live retrig    ARP + a key on the drum track: the sound at the rate picked (a black key), recorded as a retrig
 *   chromatic      CHROMATIC mode: the selected sound at the key's pitch, recorded live as the trig's pitch lock
 * Exit status: the number of failed checks. */
#define main hostsim_main
#include <stddef.h>
#include "hostsim.c"
#undef main
#define PROJ_HOST 1
static uint32_t trk_def_engine(uint32_t i) { static const uint8_t E[NPART] = {0, 1, 3}; return E[i % NPART]; }
#include "../firmware/src/project.c"

static int fails;
static void check(int ok, const char *what) { printf("drumrytm: %-100s %s\n", what, ok ? "ok" : "FAIL"); fails += !ok; }

typedef struct { uint64_t blk; uint8_t note, vel, own, k; int32_t base16, decay; } hit_t;
static hit_t hits[20000];
static uint32_t nhits;
static uint64_t blk;
static uint32_t seen_age;                          /* (the hits seen so far: a key's hit starts before the block) */
static void run_block(void)                        /* a block, and every drum hit that started in it (or before it) */
{
    int32_t out[CTL * 2];
    uint32_t a = seen_age, k;
    mix_block(out, CTL);
    seen_age = drums.age;
    if (drums.age != a)
        for (k = 0; k < NDRUM; k++)
            if (drums.v[k].age > a && nhits < 20000u) {
                hit_t *h = &hits[nhits++];
                h->blk = blk;
                h->k = (uint8_t)k;
                h->note = drums.v[k].note;
                h->vel = drums.v[k].vel;
                h->own = drums.synth[k] == 1 ? (const void *)drums.vx[k].ds.d == (const void *)&drum_own[k]
                                             : drums.vx[k].hx.d == &drum_own[k];
                h->base16 = drums.synth[k] == 1 ? drums.vx[k].ds.base16 : drums.vx[k].hx.note * 16;
                h->decay = drums.synth[k] == 1 ? drums.vx[k].ds.d->decay : drums.vx[k].hx.d->dec;
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
    memset(&pkit, 0, sizeof pkit);
    nhits = 0;
    seen_age = 0;
    blk = 0;
    song.g[G_BPM] = 120;
    song.g[G_CLOCK] = 0;
    song.rec = 0;
    TDRUM->p[P_E0] = (int16_t)kit;
    TDRUM->p[P_SLEN] = 16;
    TDRUM->p[P_SDIV] = 2;                          /* 1/16 */
    drum_mute = drum_solo = 0;
    drum_keys = DK_PLAY;
    fill_held = fill_arm = fill_lat = 0;
    kb_grid = 0;
    song.sel = TRK_DRUM;
    ly_bit[LY_ROLL] = 1u << 8;                     /* (the buttons: any bits, as seq2_test) */
    ly_bit[LY_ERASE] = 1u << 4;
}
static uint32_t step_blocks(void) { return div_samples(2) / CTL; }
static uint32_t count(uint32_t note, uint64_t b0, uint64_t b1)
{
    uint32_t i, n = 0;
    for (i = 0; i < nhits; i++)
        n += hits[i].note == note && hits[i].blk >= b0 && hits[i].blk < b1;
    return n;
}
static const hit_t *nth(uint32_t note, uint32_t k)
{
    uint32_t i;
    for (i = 0; i < nhits; i++)
        if (hits[i].note == note && !k--)
            return &hits[i];
    return 0;
}
static uint32_t k606(void) { return DRUM_DEFAULT_KIT - DRUM_SAMPLED; }
static int32_t snd_val_test(uint32_t l, uint32_t id) { return dsp_get(drum_model(), lane_snd(l), lane_lfo(l), l, id); }
static void drum_octave_test(void)              /* chromatic: OCT+ (the UI: drum_chr_oct) one octave up */
{
    const hit_t *a, *b;
    reset(DRUM_DEFAULT_KIT);
    drum_lane = 0;
    drum_keys = DK_CHROM;
    key_down(CHR_KEY0), key_up(CHR_KEY0), run(2);
    drum_chr_oct = 1;
    key_down(CHR_KEY0), key_up(CHR_KEY0), run(2);
    drum_chr_oct = 0;
    drum_keys = DK_PLAY;
    a = nth(36, 0), b = nth(36, 1);
    check(a && b && b->base16 - a->base16 == 12 * 16, "chromatic: the octave up (OCT+): the same key 12 semitones higher");
}

static void t_copy_on_write(void)
{
    static dkit_t keep;
    static project_t p;
    uint32_t l, ok = 1;
    const dsnd_t *sn606;
    int32_t semi;
    reset(DRUM_DEFAULT_KIT);
    keep = DS_KITS[k606()];
    sn606 = &DS_KITS[k606()].s[ds_lane(LANE_NOTE[2], &semi)];
    check(!pkit_on() && drum_model() == DM_SYN && dsp_desc(DM_SYN, lane_snd(2), DSP_S0 + DS_DECAY) != 0,
          "copy-on-write: the 606 at power-on is the built-in kit, a synthesised one with the same pages (DECAY on AMP)");
    pkit_make();                                   /* (the first knob turned on a sound: ui_drum.c drum_param_turn) */
    dsp_put(pkit.model, pkit.snd[2], &pkit.lfo[2], DSP_S0 + DS_DECAY, 9);
    dsp_put(pkit.model, pkit.snd[6], &pkit.lfo[6], DSP_S0 + DS_TUNE, 90);   /* (PEDAL: its own sound in the copy) */
    for (l = 0; l < DRUM_LANES; l++)
        ok &= !memcmp(&DS_KITS[k606()].s[l], &keep.s[l], sizeof keep.s[l]);
    check(ok && pkit_on() && pkit.src == DRUM_DEFAULT_KIT && sn606->decay != 9,
          "copy-on-write: the edit is in the project's copy; the built-in 606 is untouched");
    check(snd_val_test(2, DSP_S0 + DS_DECAY) == 9 && snd_val_test(4, DSP_S0 + DS_TUNE) != 90 && snd_val_test(6, DSP_S0 + DS_TUNE) == 90,
          "copy-on-write: each of the 16 sounds its own (PEDAL edited, the closed HAT it shared in the 606 is not)");
    dt_set(0, 2, 100);
    transport_req = 1;
    run(step_blocks() * 2u);
    transport_req = 2;
    run_block();
    check(nth(38, 0) && nth(38, 0)->decay == 9, "copy-on-write: the snare plays the project's copy (its DECAY 9)");
    proj_capture(&p);                              /* saved with the project: its ext */
    memset(&pkit, 0, sizeof pkit);
    proj_kit_take((const uint8_t *)&p, (int)sizeof p);
    check(pkit_on() && snd_val_test(2, DSP_S0 + DS_DECAY) == 9 && snd_val_test(6, DSP_S0 + DS_TUNE) == 90,
          "copy-on-write: saved in the project's ext and loaded back (the copy, its edits)");
    proj_kit_take((const uint8_t *)&p, (int)sizeof(proj_core_t));
    check(!pkit_on() && pkit.magic == 0u, "copy-on-write: a project stored without one (a section's core, an older one): none");
    proj_kit_take((const uint8_t *)&p, (int)sizeof p);
    TDRUM->p[P_E0] = DRUM_SAMPLED + 1u;            /* the 909: the copy (of the 606) does not play there */
    check(!pkit_on() && pkit_other() && drum_model() == DM_SYN, "copy-on-write: on another kit the 606's copy stays, unplayed");
    TDRUM->p[P_E0] = 0;                            /* a HEX kit: the same four pages, the HEX machine's parameters */
    check(drum_model() == DM_HEX && dsp_desc(DM_HEX, lane_snd(2), DSP_DEC) && !dsp_desc(DM_HEX, lane_snd(2), DSP_S0 + DS_DECAY),
          "copy-on-write: a HEX kit's sounds have the HEX machine's parameters on the same pages");
    {   /* a damaged kit from outside: refused whole */
        pkit_t bad = pkit;
        bad.model = 7;
        memcpy((uint8_t *)&p + sizeof(proj_core_t), &bad, sizeof bad);
        proj_kit_take((const uint8_t *)&p, (int)sizeof p);
        check(pkit.magic == 0u, "copy-on-write: a kit that does not check out is dropped (the built-in kit plays)");
    }
}

static void t_lock_606(void)
{
    const dsnd_t *sn;
    int32_t semi, dec;
    reset(DRUM_DEFAULT_KIT);
    sn = &DS_KITS[k606()].s[ds_lane(LANE_NOTE[2], &semi)];
    dec = sn->decay;
    dt_set(0, 2, 100), dt_set(4, 2, 100), dt_set(8, 2, 100), dt_set(4, 0, 100);
    dt_lk_set(4, 2, DSP_S0 + DS_DECAY, 20);        /* the 606 snare's DECAY on step 5: 20 */
    transport_req = 1;
    run(step_blocks() * 10u);
    transport_req = 2;
    run_block();
    check(nth(38, 1) && nth(38, 1)->own && nth(38, 1)->decay == 20,
          "606 lock: the snare on step 5 plays its locked DECAY (20) from its voice's own copy");
    check(nth(38, 0) && nth(38, 2) && !nth(38, 0)->own && !nth(38, 2)->own && nth(38, 0)->decay == dec,
          "606 lock: the snare on steps 1 and 9 plays the kit's sound");
    check(nth(36, 0) && !nth(36, 0)->own, "606 lock: the kick on the same step is not locked");
    check(sn->decay == dec && !pkit_on(), "606 lock: the kit is not changed, no copy made");
}

static void t_lfo_send(void)
{
    const hit_t *a, *b;
    reset(DRUM_DEFAULT_KIT);
    dt_set(0, 2, 100), dt_set(4, 2, 100);
    dt_lk_set(4, 2, DSP_NOTE, 7);                  /* a pitch lock: +7 semitones */
    dt_lk_set(4, 2, DSP_REV, 15);                  /* .. and the snare's reverb send full on that hit */
    transport_req = 1;
    run(step_blocks() * 6u);
    a = nth(38, 0), b = nth(38, 1);
    check(a && b && b->base16 - a->base16 == 7 * 16, "pitch lock: the hit 7 semitones up");
    check(b && dlr.sov[b->k][0] == 16u && dlr.sov[b->k][1] == 0u && drum_send[2][0] == 0u,
          "send lock: that hit's own reverb send full (the sound's send as it was)");
    run(step_blocks() * 14u);
    transport_req = 2;
    run_block();
    {   /* the LFO: the project's copy, the snare's LFO on its pitch, deep and fast: the pitch moves within the hit */
        uint32_t k, b0, moved = 0;
        int32_t last = 0;
        reset(DRUM_DEFAULT_KIT);
        pkit_make();
        dsp_put(pkit.model, pkit.snd[2], &pkit.lfo[2], DSP_LDEP, 63);
        dsp_put(pkit.model, pkit.snd[2], &pkit.lfo[2], DSP_LSPD, 110);
        dsp_put(pkit.model, pkit.snd[2], &pkit.lfo[2], DSP_LDST, DL_PIT);
        trk_note_on(TDRUM, LANE_NOTE[2], 100);
        for (k = 0; k < NDRUM && !(drums.v[k].active && drums.v[k].note == 38); k++)
            ;
        for (b0 = 0; b0 < 200u && k < NDRUM; b0++) {
            run_block();
            if (b0 && drums.vx[k].ds.lp16 != last)
                moved++;
            last = drums.vx[k].ds.lp16;
        }
        check(k < NDRUM && moved > 20u, "LFO: the sound's LFO on PITCH moves its hit as it rings");
    }
}

static void t_mute_solo(void)
{
    uint64_t b0;
    reset(DRUM_DEFAULT_KIT);
    dt_set(0, 0, 100), dt_set(0, 2, 100), dt_set(0, 4, 100);
    TDRUM->p[P_SLEN] = 4;
    drum_keys = DK_MUTE;                           /* MUTE mode: the snare's white key mutes it */
    key_down(key_of_lane(2));
    key_up(key_of_lane(2));
    check(drum_mute == 1u << 2 && dt_has(0, 2), "mute: MUTE mode's key mutes the snare (its trigs kept)");
    transport_req = 1;
    run(step_blocks() * 8u);
    b0 = blk;
    check(count(36, 0, b0) == 2u && count(38, 0, b0) == 0u && count(42, 0, b0) == 2u, "mute: the sequencer skips the snare, plays the rest");
    key_down(key_of_lane(2));
    key_up(key_of_lane(2));
    drum_solo = 1u << 0;                           /* (SEL + key in MUTE mode: the kick soloed, ui_layers.c) */
    run(step_blocks() * 8u);
    check(count(36, b0, blk) == 2u && count(38, b0, blk) == 0u && count(42, b0, blk) == 0u, "solo: only the soloed kick plays");
    transport_req = 2;
    run_block();
    drum_solo = 0;
    drum_keys = DK_PLAY;
}

static void t_fill(void)
{
    uint64_t b0, b1, b2;
    reset(DRUM_DEFAULT_KIT);
    TDRUM->p[P_SLEN] = 4;
    dt_set(0, 2, 100), dt_x_make(0, 2)->cond = FC_FILL;   /* the snare FILL ONLY, the hat NOT FILL */
    dt_set(2, 4, 100), dt_x_make(2, 4)->cond = FC_NOFILL;
    transport_req = 1;
    run(step_blocks() * 8u);
    b0 = blk;
    fill_lat = 1;                                  /* latched (F#5 twice on the drum GRID, ui_drum.c) */
    run(step_blocks() * 8u);
    b1 = blk;
    fill_lat = 0;
    run(step_blocks() * 8u);
    b2 = blk;
    fill_held = 1;
    run(step_blocks() * 4u);
    fill_held = 0;
    transport_req = 2;
    run_block();
    check(count(38, 0, b0) == 0u && count(42, 0, b0) == 2u, "fill: no fill: the FILL trig silent, the NOT FILL one plays");
    check(count(38, b0, b1) == 2u && count(42, b0, b1) == 0u, "fill: latched: the FILL trigs play, the NOT FILL ones do not");
    check(count(38, b1, b2) == 0u && count(42, b1, b2) == 2u, "fill: unlatched: back as before");
    check(count(38, b2, blk) == 1u, "fill: held: the FILL trig plays");
    fill_lat = 1;
    transport_req = 1;
    run(2);
    transport_req = 2;
    run_block();
    check(!fill_lat, "fill: STOP ends a latched fill");
}

static void t_live_retrig(void)
{
    uint32_t sb = step_blocks(), kb = PUNCH_BLACK[6];   /* the black key G#4: 1/32 */
    int x;
    reset(DRUM_DEFAULT_KIT);
    TDRUM->p[P_SLEN] = 16;
    fm1_in.buttons |= ly_bit[LY_ROLL];             /* ARP held: the RETRIG layer */
    key_down(kb);                                   /* a rate */
    key_up(kb);
    check(drt_live == 13u, "live retrig: ARP + the black key G#4 picks 1/32");
    song.rec = 1u << TRK_DRUM;
    transport_req = 1;
    run(sb + 2u);                                  /* (step 2 begins) */
    key_down(key_of_lane(2));                      /* ARP + the snare's key, held a step and a half */
    run(sb + sb / 2u);
    key_up(key_of_lane(2));
    fm1_in.buttons &= ~ly_bit[LY_ROLL];
    run(4);
    song.rec = 0;
    transport_req = 2;
    run_block();
    printf("drumrytm:   live retrig: %u snare hits\n", count(38, 0, blk));
    check(count(38, 0, blk) >= 3u && count(38, 0, blk) <= 4u, "live retrig: the snare retrigged at 1/32 while held (3 hits in a step and a half)");
    x = dt_x(1, 2) >= 0 ? dt_x(1, 2) : dt_x(2, 2);
    check((dt_has(1, 2) || dt_has(2, 2)) && x >= 0 && dpat.x[x].rrate == 13u && dpat.x[x].rlen == DT_RLEN_DEF,
          "live retrig: recorded as a trig with a retrig of its own rate (1/32, one step)");
}

static void t_chromatic(void)
{
    uint32_t sb = step_blocks();
    int lk;
    int32_t base = 0;
    const hit_t *a, *b;
    reset(DRUM_DEFAULT_KIT);
    drum_lane = 2;                                 /* the snare selected */
    drum_keys = DK_CHROM;
    key_down(CHR_KEY0);                            /* C4: the sound's own pitch */
    key_up(CHR_KEY0);
    run(2);
    key_down(CHR_KEY0 + 5u);                       /* F4: 5 semitones up */
    key_up(CHR_KEY0 + 5u);
    run(2);
    a = nth(38, 0), b = nth(38, 1);
    check(a && b && b->base16 - a->base16 == 5 * 16, "chromatic: the selected sound at the key's pitch (F4: +5)");
    base = a ? a->base16 : 0;
    song.rec = 1u << TRK_DRUM;
    transport_req = 1;
    run(sb * 3u + sb / 4u);                        /* (into step 4) */
    key_down(CHR_KEY0 + 3u);
    key_up(CHR_KEY0 + 3u);
    run(sb);
    key_down(CHR_KEY0);                            /* the own pitch: no lock */
    key_up(CHR_KEY0);
    run(sb);
    song.rec = 0;
    transport_req = 2;
    run_block();
    lk = dt_lk(3, 2, DSP_NOTE);
    check(dt_has(3, 2) && lk >= 0 && (int8_t)dpat.lk[lk].val == 3, "chromatic: recorded live: the trig keeps its pitch (a NOTE lock, +3)");
    check(dt_has(4, 2) && dt_lk(4, 2, DSP_NOTE) < 0, "chromatic: a trig at the sound's own pitch has no NOTE lock");
    nhits = 0;
    drum_keys = DK_PLAY;
    TDRUM->p[P_SLEN] = 4;
    transport_req = 1;
    run(sb * 4u + 2u);
    transport_req = 2;
    run_block();
    check(nth(38, 0) && nth(38, 0)->base16 == base + 3 * 16, "chromatic: the recorded trig plays back at its pitch");
    drum_octave_test();
}

int main(void)
{
    t_copy_on_write();
    t_lock_606();
    t_lfo_send();
    t_mute_solo();
    t_fill();
    t_live_retrig();
    t_chromatic();
    printf("drumrytm: %s\n", fails ? "FAILED" : "PASS");
    return fails;
}
