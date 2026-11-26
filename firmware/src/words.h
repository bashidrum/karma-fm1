/* SPDX-License-Identifier: GPL-3.0-only */
/* (review V2, Ben 26 Nov 2026: "One dictionary everywhere. Prefer UK english where space allows for it") THE WORDS the screens
 * use, one spelling each, six characters or fewer, and always VOICE, never TRACK. The labels in the parameter tables
 * (params.c, drum_par.c, the engines) and the messages use these; GUIDE.md's DICTIONARY lists them for the reader. A dial
 * shows five letters comfortably and six at most (ui_drum.c dial_vx) */
#ifndef PHEBUS_WORDS_H
#define PHEBUS_WORDS_H
#define W_DETUNE "DETUNE"
#define W_BRIGHT "BRIGHT"
#define W_PHASER "PHASER"
#define W_FLANGE "FLANGE"
#define W_CHORUS "CHORUS"
#define W_REVERB "REVERB"
#define W_LP24 "LP 24"
#define W_LP12 "LP 12"
#define W_FX1 "FX 1"
#define W_FX2 "FX 2"
#define W_FX3 "FX 3"
#define W_LFO1 "LFO 1"
#define W_LFO2 "LFO 2"
#define W_COLOUR "COLOUR"
#define W_SUS "SUS"
#define W_REL "REL"
#define W_NSLVL "NS LVL"
#define W_NSHLD "NS HLD"
#define W_NSDEC "NS DEC"
#define W_NSFRQ "NS FRQ"
#define W_PAMT "P AMT"
#define W_PDEC "P DEC"
#define W_SCALE "SCALE"
#define W_QUANT "QUANT"
#define W_DEPTH "DEPTH"
#define W_STRUCT "STRUCT"
#define W_ELEMNT "ELEMNT"
#define W_CLEAR "CLEAR"
#define W_SLICE "SLICE"
#define W_ESYNC "E SYNC"
#define W_VOICE "VOICE"
/* a name longer than six letters (an engine's) as the dials and chips show it */
static inline const char *word_short(const char *s)
{
    return s[0] == 'E' && s[1] == 'L' && s[2] == 'E' && s[3] == 'M' ? W_ELEMNT : s;
}
#endif
