/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Engine table: order = PRESETS browsing order (and the engine numbers of the editor protocol). */
#include "dsp.c"
/* SLOOP 2.5: the big per-part state of GRAIN, FM6 and PHYS shares one arena per synth part (a part plays one
 * engine at a time: on a switch the old engine fades out, its voices end, then the new one starts, voice.c
 * engine_block). The first engine to ask for it after another one gets it cleared, as at power-on. */
static void *eng_arena_of(const track_t *t, uint32_t eng);
static void *eng_arena_need(const track_t *t, uint32_t eng, uint32_t need);
static uint32_t eng_arena_size(const track_t *t);
static uint32_t eng_arena_gen[NPART];           /* (counts the claims: an engine sees its state was cleared) */
static inline int32_t voice_amp(int32_t s, const vmod_t *m, uint32_t i) { return mulq15(mulq15(s, amp_at(m, i)), VOICE_FS); }
static inline int32_t soft_knee(int32_t y, int32_t k)   /* linear up to k, then only the peaks saturate */
{
    int32_t a = y < 0 ? -y : y;
    if (a <= k)
        return y;
    a = k + (softclip((a - k) * 2) >> 1);
    return y < 0 ? -a : a;
}
/* (pitch_inc: felucca_tables.h, SLOOP 2.5: from the top octave) */
/* SLOOP for the FM-1 OS (2026-10-19, Ariel's call): the bass track plays ACID or JUNO, track 2 ELEMENTS, track 3
 * PLAITS; the other engines are gone. Their numbers stay (projects, user presets and the editor keep them) and
 * hold ENG_GONE, which no track can choose (eng_allowed) */
#include "eng_phys.c"           /* PHYS's bank and models, for ELEMENTS (from Felucca 1.0: DaisySP, Rings: MIT) */
#include "eng_elem.c"           /* ELEMENTS: Elements' exciters (Emilie Gillet, MIT) on PHYS's bank */
#include "eng_acid.c"           /* ACID: X0X's TB-303 (Charles Vestal, GPL-3.0; Open303, MIT) */
#include "eng_juno.c"           /* JUNO: FM-106's Juno-106 voice (PatttF, GPL-3.0; after ultramaster KR-106) */
#include "eng_prism.c"          /* PRISM: models of Plaits (Emilie Gillet, MIT), fixed point: prism_dsp.c */

static void gone_note_on(track_t *t, voice_t *v) { (void)t; (void)v; }
static void gone_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{ (void)t; (void)v; (void)out; (void)n; (void)m; }
static const preset_t GONE_PRESETS[] = {{"-", {0}, {0, 0, 127, 10}, 0, 0}};
static const engine_t ENG_GONE = {
    .name = "-",
    .page_title = {"-", "-"},
    .edit = {{"-", F_INT, 0, 0, 0, 0, 0}, {"-", F_INT, 0, 0, 0, 0, 0}, {"-", F_INT, 0, 0, 0, 0, 0}, {"-", F_INT, 0, 0, 0, 0, 0},
             {"-", F_INT, 0, 0, 0, 0, 0}, {"-", F_INT, 0, 0, 0, 0, 0}, {"-", F_INT, 0, 0, 0, 0, 0}, {"-", F_INT, 0, 0, 0, 0, 0}},
    .presets = GONE_PRESETS,
    .npresets = 1,
    .note_on = gone_note_on,
    .render = gone_render,
    .fil_page = -1,
    .color = 0x8410,
    .macro = {P_E0, P_E1, P_E2, P_E3},
    .poly = 1,
};

/* each part's arena. (PHEBUS wave 4; Ben, 22 Nov 2026: "Make the perfect version. Then we'll compromise": every oscillator
 * Type on every track) every part plays any engine, one at a time, so each part's arena is as big as the biggest engine's
 * state (Plaits' PARTICLE, STRING and SPEECH models, ~16 KB each, one voice; the rest fit several voices in it, prism_poly).
 * A part's engines take turns in its arena (voice.c engine_block: the old one fades out first), as before. PHEBUS_ARENA_T12:
 * the bytes of tracks 1 and 2's arenas, the full size unless a build asks for less (the device build, when the pool cannot
 * hold three full ones: tools/build.py; a Plaits model that then does not fit one voice in a part's arena is silent there,
 * and the OSC page says so) */
#define PRISM_ARENA (sizeof(void *) == 8u ? 16704u : 16640u)   /* (a 64-bit host: the pointers in Plaits' objects) */
typedef union {
    acid_slot_t acid;
    juno_slot_t juno;
    uint32_t phys[PHYS_ARENA / 4u];
    prism_slot_t prism[NVOICE];
    uint64_t plx[PRISM_ARENA / 8u];
} eng_arena_full_t;
#define ARENA_FULL ((uint32_t)sizeof(eng_arena_full_t))
#ifndef PHEBUS_ARENA_T12
#define PHEBUS_ARENA_T12 ARENA_FULL
#endif
#define ARENA_T12 ((((uint32_t)(PHEBUS_ARENA_T12) < ARENA_FULL ? (uint32_t)(PHEBUS_ARENA_T12) : ARENA_FULL) + 7u) & ~7u)
_Static_assert(ARENA_T12 >= sizeof(juno_slot_t) && ARENA_T12 >= sizeof(acid_slot_t) && ARENA_T12 >= PHYS_ARENA,
               "every part's arena holds ACID, JUNO and ELEMENTS whole (PHEBUS_ARENA_T12 trims Plaits' biggest models only)");
static uint64_t eng_arena0[ARENA_T12 / 8u] __attribute__((section(".pool")));
static uint64_t eng_arena1[ARENA_T12 / 8u] __attribute__((section(".pool")));
static eng_arena_full_t eng_arena2 __attribute__((section(".pool")));
static void *const ENG_ARENA[NPART] = {eng_arena0, eng_arena1, &eng_arena2};
static const uint32_t ENG_ARENA_N[NPART] = {sizeof eng_arena0, sizeof eng_arena1, sizeof eng_arena2};
static uint8_t eng_arena_own[NPART];            /* the engine + 1 whose state is in it, 0 = none */
#ifdef ARENA_STATS
static uint32_t eng_arena_claims, eng_arena_bad;   /* host tests: claims, accesses by an engine the part does not play */
#endif
static uint32_t eng_arena_size(const track_t *t) { uint32_t p = (uint32_t)(t - trk); return p < NPART ? ENG_ARENA_N[p] : 0u; }
/* part t's arena for engine eng, which needs `need` bytes (0: none, the part's is smaller) */
static void *eng_arena_need(const track_t *t, uint32_t eng, uint32_t need)
{
    uint32_t p = (uint32_t)(t - trk);
    if (p >= NPART || need > ENG_ARENA_N[p])
        return 0;
#ifdef ARENA_STATS
    if (t->engine != eng)
        eng_arena_bad++;
#endif
    if (eng_arena_own[p] != eng + 1u) {
        memset(ENG_ARENA[p], 0, ENG_ARENA_N[p]);
        eng_arena_own[p] = (uint8_t)(eng + 1u);
        eng_arena_gen[p]++;
#ifdef ARENA_STATS
        eng_arena_claims++;
#endif
    }
    return ENG_ARENA[p];
}
static void *eng_arena_of(const track_t *t, uint32_t eng)
{
    uint32_t need = eng == ENGI_ACID ? sizeof(acid_slot_t) : eng == ENGI_JUNO ? sizeof(juno_slot_t)
                  : eng == ENGI_PRISM ? sizeof(prism_slot_t) : PHYS_ARENA;
    return eng_arena_need(t, eng, need);
}

static const engine_t *const ENGINES[NENGINES] = {&ENG_GONE, &ENG_GONE, &ENG_GONE, &ENG_GONE, &ENG_GONE,
                                                    &ENG_GONE, &ENG_GONE, &ENG_GONE, &ENG_GONE,
                                                    &ENG_GONE,   /* 9: FM6 was here */
                                                    &ENG_GONE, &ENG_GONE,   /* PHYS, NOISE were here */
                                                    &ENG_PRISM, &ENG_ELEM, &ENG_ACID, &ENG_JUNO,
};
/* the engines each synth track may play (the drum track: none). PRESETS lists only theirs. (PHEBUS wave 4; Ben, 21 Nov
 * 2026: "Oscillator type ... The same layout on every voice"; 22 Nov: "Make the perfect version": every Type on every
 * track) every synth track plays every engine */
static int eng_allowed(const track_t *t, uint32_t e)
{
    uint32_t p = (uint32_t)(t - trk);
    return p < NPART && (e == ENGI_ACID || e == ENGI_JUNO || e == ENGI_ELEM || e == ENGI_PRISM);
}
static uint32_t eng_default(const track_t *t)        /* the engine a track falls back to */
{
    uint32_t p = (uint32_t)(t - trk);
    return p == 0u ? ENGI_ACID : p == 1u ? ENGI_ELEM : ENGI_PRISM;
}

/* every factory sound as loud as the others: a level trim per preset, 1/2 dB, measured on a phrase
 * that fits the sound (tools/level_presets.py writes preset_trim.h); a track keeps it in P_ED_FX */
#include "preset_trim.h"
static int16_t preset_trim(uint32_t e, uint32_t pi)
{
    return e < PT_ENGINES && pi < PT_MAX ? PRESET_TRIM[e][pi] : 0;
}
