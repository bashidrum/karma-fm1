/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Parameter descriptors, formatting and the page table. */
static const char *const N_LWAVE[] = {"SIN", "TRI", "SAW", "SQR", "S&H"};
/* SLOOP 2.6, after Felucca 1.4 / 1.5 (Leo Kuroshita) ----------------------------------------------------------------
 * LFO 2. SYNC: OFF (RATE), or one cycle per note value of the tempo, longest first (LSYNC_UNITS: in core.h div_units'
 * units); TRIG: NOTE restarts it at PHS on a fresh phrase (as before), FREE never; POL: BI -1..+1 (as before), UNI 0..+1 */
static const char *const N_LSYNC[] = {"OFF", "4BAR", "2BAR", "1BAR", "1/2", "1/4", "1/8", "8T", "1/16", "16T", "1/32"};
static const char *const N_LTRIG[] = {"NOTE", "FREE", "PATT"};   /* (PHEBUS, 21 Nov 2026: PATT = PATTERN, seq.c seq_tick; the
                                                                  * whole word does not fit the knob) */
static const char *const N_LPOL[] = {"BI", "UNI"};
/* GLO > MIDI (2.6): a synth track's MIDI channel, and the other channels */
static const char *const N_MCH[] = {"OFF", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16"};
static const char *const N_MOTH[] = {"SEL", "OFF"};
/* (PHEBUS wave 4) the voice filter's TYPE (vfilt.c VF_*; P_FTYPE was ANALOG's, an engine gone): the ladder's 24 dB and
 * 12 dB low-pass, its band-pass, its 12 dB high-pass. LP24 on ACID and JUNO is their own ladder (the 303's, the IR3109) */
static const char *const N_FTYPE[] = {W_LP24, W_LP12, "BP", "HP"};
_Static_assert(sizeof N_FTYPE / sizeof N_FTYPE[0] == VF_NTYPE, "a name a filter type");
static const char *const N_FLINK[] = {"AMP", "OWN"};   /* the filter's envelope: LINK to the amp ENV, or its OWN ADSR */
/* ENV SYNC (voice.c): ATK DEC REL as note values of the tempo, shortest first. 0..127 split evenly over these 25
 * (param_format's F_INT name list: value v is ESYNC_NAMES[v * 25 / 128]); ESYNC_UNITS: each in 1/384 of a bar of
 * 4/4 (a quarter 96), 0 = the shortest time ATK / DEC / REL 0 has. 0-terminated (param_format) */
#define ESYNC_N 25u
static const char *const ESYNC_NAMES[ESYNC_N + 1u] = {
    "0", "1/64T", "1/64", "1/32T", "1/64D", "1/32", "1/16T", "1/32D", "1/16", "1/8T", "1/16D", "1/8", "1/4T", "1/8D",
    "1/4", "1/2T", "1/4D", "1/2", "1/1T", "1/2D", "1/1", "1/1D", "2BAR", "3BAR", "4BAR", 0};
static const uint16_t ESYNC_UNITS[ESYNC_N] = {0, 4, 6, 8, 9, 12, 16, 18, 24, 32, 36, 48, 64, 72,
                                              96, 128, 144, 192, 256, 288, 384, 576, 768, 1152, 1536};
static uint32_t esync_idx(int32_t v) { return (uint32_t)(v & 127) * ESYNC_N >> 7; }   /* (param_format's split) */
/* the INSERT's types (fx.c IT_*; append-only: stored). Its three values A B C mean what the type says (ins_desc) */
static const char *const N_ITYPE[] = {"OFF", "SOFT", "HARD", "FOLD", "FUZZ", "CRUSH", W_PHASER, W_FLANGE, W_CHORUS};
/* CRUSH: BITS 1..16 and RATE (the held sample's rate) over the range in 16 even parts (0-terminated) */
static const char *const N_IBITS[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15",
                                      "16", 0};
static const char *const N_IRATE[] = {"689", "919", "1.1k", "1.4k", "1.8k", "2.2k", "2.8k", "3.7k", "4.4k", "5.5k",
                                      "7.4k", "8.8k", "11k", "15k", "22k", "44k", 0};
/* (PHEBUS wave 5) the FX slots' types (core.h XT_*; stored, append-only): on the TYPE knob (five letters) and in the slot's
 * box (fx-A, large) */
static const char *const N_XTYPE[] = {"OFF", W_CHORUS, W_PHASER, W_FLANGE, W_REVERB, "DELAY", "DIST", "SOFT", "HARD", "FOLD",
                                      "FUZZ", "CRUSH", "EQ"};
static const char *const N_XBIG[] = {"OFF", W_CHORUS, W_PHASER, W_FLANGE, W_REVERB, "DELAY", "DIST", "SOFT", "HARD", "FOLD",
                                     "FUZZ", "CRUSH", "EQ"};
_Static_assert(sizeof N_XTYPE / sizeof N_XTYPE[0] == XT_N && sizeof N_XBIG / sizeof N_XBIG[0] == XT_N, "a name an FX type");
/* the mod matrix's rows and columns (core.h MS_*, MD_*): short in the grid, long on the knobs */
static const char *const N_MSRC[] = {W_LFO1, W_LFO2, "AENV", "FENV", "PENV", "VEL", "KEY"};
static const char *const N_MDST[] = {"PIT", "M1", "M2", "M3", "CUT", "RES", "LVL", "PAN", W_FX1, W_FX2, W_FX3};
_Static_assert(sizeof N_MSRC / sizeof N_MSRC[0] == MS_N && sizeof N_MDST / sizeof N_MDST[0] == MD_NFIX, "the grid's names");
static const char *const N_AMODE[] = {"OFF", "UP", "DN", "UPDN", "RND", "ORD"};
static const char *const N_MOUT[] = {"KEYS", "SEQ"};
static const char *const N_MIN[] = {"NOTES", "CLOCK", "CH 1", "CH 2", "CH 3", "CH 4", "CH 5", "CH 6", "CH 7", "CH 8", "CH 9",
                                     "CH 10", "CH 11", "CH 12", "CH 13", "CH 14", "CH 15", "CH 16"};  /* G_ROUTE: MIDI in, notes and clock, the clock only, or (2.6) one channel only, on the selected track (seq.c) */    /* G_MIDI: what goes to MIDI OUT (seq.c seq_out_on) */
static const char *const N_DIV[] = {"1/4", "1/8", "1/16", "1/32", "8T", "16T"};
static const char *const N_SDIV[] = {"1/4", "1/8", "1/16", "1/32", "8T", "16T", "1/2", "1BAR", "2BAR"};   /* core.h div_units */
static const char *const N_DLY[] = {"1/4", "1/8", "1/16", "1/32", "8T", "16T", "1/8D", "1/16D", "1/4D"};   /* core.h dly_units
                                                                          * (2.6: 1/4D, after Felucca 1.5.1) */
static const char *const N_SCALE[] = {"CHROM", "MAJ", "MIN", "DOR", "MIX", "PEN", "MPEN", "HARM",
                                    "PHRY", "LYD", "LOC", "MEL", "BLUES", "WHOLE", "DIMHW", "DIMWH"};
static const char *const N_ONOFF[] = {"OFF", "ON"};
static const char *const N_QUANT[] = {"OFF", "SNAP", "WHITE"};   /* seq.c kb_map; 1 = SNAP as the old ON */
static const char *const N_VOICE[] = {"POLY", "MONO", "LEG", "UNI"};   /* V_POLY .. V_UNISON */
static const char *const N_GLMODE[] = {"RATE", "TIME"};
static const char *const N_PRIO[] = {"LAST", "LOW", "HIGH"};
static const char *const N_ALLOC[] = {"ROT", "REUSE"};
static const char *const N_ORDER[] = {"NOTE", "PLAY"};
static const char *const N_CLICK[] = {"OFF", "REC", "ON"};   /* G_CLOCK is the metronome (seq.c click_tick) */
static const char *const N_NOTE[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
static const char *const N_DASH[] = {"--"};
static const char *const N_SYNC[] = {"INT", "USB", "TRS"};   /* G_SYNC: the tempo and PLAY / STOP from (seq.c mclk) */
static const char *const N_GO[] = {"--", "GO"};
static const char *const N_KNOBS[] = {"GM", "70-77", "1-8", "AUTO", "PANEL"};
/* HEX (hex_drums.c: the LXR's drum voices, lxr_dsp.c): HM_*, HW_*, HC_* (the transients), HF_* */
static const char *const N_HXMODEL[] = {"DRUM", "SNARE", "CLAP", "CYM", "HAT"};
static const char *const N_HXWAVE[] = {"SIN", "TRI", "SAW", "SQR", "NOIS", "P12", "P25", "P37"};
static const char *const N_HXCLICK[] = {"SNAP", "OFFS", "CLK", "CK2", "TICK", "KICK", "RIM", "DRIP*", "HAT", "CLAP",
                                        "KK2*", "SNR", "TOM", "SP2*"};   /* *: not in the flash, TICK KICK SNR play */
static const char *const N_HXFILT[] = {"LP", "HP", "BP", "UBP", "NTCH", "PEAK", "LP2", "OFF"};
static const char *const N_HXMODW[] = {"FMSIN", "FMTRI", "FMSAW", "FMSQR", "FMNOI", "FMPWM",   /* DRUM: the mod osc */
                                       "MXSIN", "MXTRI", "MXSAW", "MXSQR", "MXNOI", "MXPWM"};
static const char *const N_HXWAV2[] = {"SI/SI", "SI/TR", "SI/SA", "SI/SQ", "SI/NO", "SI/PW", "TR/SI", "TR/TR", "TR/SA",
                                       "TR/SQ", "TR/NO", "TR/PW", "SA/SI", "SA/TR", "SA/SA", "SA/SQ", "SA/NO", "SA/PW",
                                       "SQ/SI", "SQ/TR", "SQ/SA", "SQ/SQ", "SQ/NO", "SQ/PW", "NO/SI", "NO/TR", "NO/SA",
                                       "NO/SQ", "NO/NO", "NO/PW", "PW/SI", "PW/TR", "PW/SA", "PW/SQ", "PW/NO", "PW/PW"};
static const char *const N_SLCR[] = {"OFF", "GATE", "STUT"};             /* SL_OFF .. SL_STUT (slicer.c) */
static const char *const N_SLDIV[] = {"1/8", "1/16", "1/32", "8T", "16T", "32T"};   /* SL_DEN */
static const char *const N_CHORD[] = {"OFF", "TRIAD", "7TH", "9TH", "SUS4", "POWER"};   /* seq.c CHORD_DEG */
static const char *const N_ROLL[] = {"1/8", "1/16", "1/32", "32T", "1/64"};   /* seq.c ROLL_DEN */
static const char *const N_ENGNAME[] = {"ANALOG", "DIGITAL", "PHASE", "LOFI", "SAMPLE", "VOICE", "TRIO", "WHEEL", "GRAIN",
                                        "FM6", "PHYS", "NOISE",
                                        "PLAITS", "ELEMENTS", "ACID", "JUNO",
};
_Static_assert(sizeof N_ENGNAME / sizeof N_ENGNAME[0] == NENGINES, "a name an engine");

#include "vtype.c"            /* (PHEBUS wave 4) the oscillator Types and their macros */
#define PD(l, f, mn, mx, df) {l, f, mn, mx, df, 0, 0}
#define PE(l, n, df) {l, F_ENUM, 0, (int16_t)(sizeof(n) / sizeof(n[0]) - 1), df, n, 0}

static const param_desc_t TP[P_COUNT] = {
    [P_LEVEL] = PD("LVL", F_DB, 0, 127, 104),
    [P_ATK] = PD("ATTACK", F_TIME, 0, 127, 10),   /* (wave 4: the ENV page's names, mock-synth-env-A) */
    [P_DEC] = PD("DECAY", F_TIME, 0, 127, 70),
    [P_SUS] = PD(W_SUS, F_PCT, 0, 127, 90),
    [P_REL] = PD(W_REL, F_TIME, 0, 127, 60),
    [P_ED_FLT] = PD("ENV", F_BIPCT, -64, 63, 0),  /* (wave 4) the voice filter's ENV amount: +-64 a full sweep (voice.c) */
    [P_ED_PIT] = PD("PIT", F_BIPCT, -64, 63, 0),
    [P_ED_SHP] = PD("SHP", F_BIPCT, -64, 63, 0),
    [P_ED_FX] = PD("TRIM", F_INT, -64, 63, 0),   /* the sound's level trim, 1/2 dB (presets set it) */
    [P_LRATE] = PD("RATE", F_LFOHZ, 0, 127, 60),
    [P_LWAVE] = PE("WAVE", N_LWAVE, 0),
    [P_LPHASE] = PD("PHASE", F_INT, 0, 127, 0),
    [P_LFADE] = PD("FADE", F_TIME, 0, 127, 0),
    [P_LD_PIT] = PD("PIT", F_BIPCT, -64, 63, 0),
    [P_LD_FLT] = PD("FLT", F_BIPCT, -64, 63, 0),
    [P_LD_SHP] = PD("SHP", F_BIPCT, -64, 63, 0),
    [P_LD_AMP] = PD("AMP", F_PCT, 0, 127, 0),
    [P_AMODE] = PE("MODE", N_AMODE, 0),
    [P_ARATE] = PE("RATE", N_DIV, 2),
    [P_AOCT] = PD("OCT", F_INT, 1, 4, 1),
    [P_AGATE] = PD("GATE", F_PCT, 1, 127, 64),
    [P_ASWING] = PD("SWING", F_SWING, 0, 100, 0),
    [P_APROB] = PD("PROB", F_PCT, 0, 127, 127),
    [P_AHOLD] = PE("HOLD", N_ONOFF, 0),
    [P_AORDER] = PE("ORD", N_ORDER, 0),
    [P_ROOT] = PD("ROOT", F_NOTE, 0, 11, 0),
    [P_SCALE] = PE(W_SCALE, N_SCALE, 0),
    [P_QUANT] = PE(W_QUANT, N_QUANT, 0),
    [P_TRANS] = PD("TRN", F_SEMI, -24, 24, 0),
    [P_SLEN] = PD("STEPS", F_STEPS, 1, NSTEP, 16),
    [P_SDIV] = PE("DIV", N_SDIV, 2),
    [P_SSWING] = PD("SWING", F_SWING, 0, 100, 0),
    [P_SGATE] = PD("GATE", F_PCT, 1, 127, 64),
    [P_DIST] = PD("DIST", F_PCT, 0, 127, 0),
    [P_CHOR] = PD("CHORUS", F_PCT, 0, 127, 0),
    [P_DLY] = PD("DELAY", F_PCT, 0, 127, 0),
    [P_REV] = PD("REVERB", F_PCT, 0, 127, 0),
    [P_VOICE] = PE("VOICE", N_VOICE, 0),
    [P_GLIDE] = PD("GLIDE", F_TIME, 0, 127, 0),
    [P_GLMODE] = PE("GLMODE", N_GLMODE, 0),
    [P_PRIO] = PE("PRIOR", N_PRIO, 0),
    [P_ALLOC] = PE("ALLOC", N_ALLOC, 0),
    [P_DETUNE] = PD(W_DETUNE, F_INT, 0, 127, 40),
    [P_PAN] = PD("PAN", F_BIPCT, -64, 63, 0),
    [P_MUTE] = PE("MUTE", N_ONOFF, 0),
    [P_SLCR] = PE(W_SLICE, N_SLCR, 0),
    [P_SLPAT] = PD("PAT", F_INT, 1, 16, 1),        /* SL_PAT[] */
    [P_SLRATE] = PE("RATE", N_SLDIV, 1),
    [P_SLDEPTH] = PD("DEPTH", F_PCT, 0, 127, 127),
    [P_CHORD] = PE("CHORD", N_CHORD, 0),
    [P_TFLT] = PD("FILTER", F_FILT, -64, 63, 0),   /* (wave 8: the drum voice's own, its FX tab) */
    [P_STRUM] = PD("STRUM", F_INT, -60, 60, 0),   /* ms a note: > 0 low to high (down), < 0 high to low */
    [P_VLEAD] = PE("VLEAD", N_ONOFF, 0),
    /* 2.6, after Felucca 1.5 / 1.4. The INSERT (fx.c track_insert): TYPE OFF = as before. A B C as the editor
     * protocol names them; the device shows the type's own labels (ins_desc) */
    [P_ITYPE] = PE("INSERT", N_ITYPE, 0),
    [P_IA] = PD("INS A", F_PCT, 0, 127, 64),
    [P_IB] = PD("INS B", F_PCT, 0, 127, 96),
    [P_IC] = PD("INS C", F_PCT, 0, 127, 96),
    [P_IMIX] = PD("MIX", F_PCT, 0, 127, 127),
    [P_FTYPE] = PE("TYPE", N_FTYPE, 0),           /* (wave 4) the voice filter's type (vfilt.c) */
    [P_ESYNC] = PE(W_ESYNC, N_ONOFF, 0),          /* OFF = ATK DEC REL in ms, as before (voice.c) */
    [P_LSYNC] = PE("SYNC", N_LSYNC, 0),           /* LFO 2: OFF = RATE, as before */
    [P_LTRIG] = PE("TRIG", N_LTRIG, 0),
    [P_LPOL] = PE("POL", N_LPOL, 0),
    /* (PHEBUS wave 4) the voice filter, its envelope, the oscillator's OCT and TUNE (core.h). The filter's ADSR starts as the
     * amp ENV's defaults: LINK is the start, and turning one of them first copies the amp ENV (ui_input.c) */
    [P_FCUT] = PD("CUTOFF", F_PCT, 0, 127, 127),
    [P_FRES] = PD("RESO", F_PCT, 0, 127, 0),
    [P_FKEY] = PD("KEY", F_PCT, 0, 127, 0),
    [P_FDRV] = PD("DRIVE", F_PCT, 0, 127, 0),
    [P_FLINK] = PE("LINK", N_FLINK, 0),
    [P_FATK] = PD("ATTACK", F_TIME, 0, 127, 10),
    [P_FDEC] = PD("DECAY", F_TIME, 0, 127, 70),
    [P_FSUS] = PD(W_SUS, F_PCT, 0, 127, 90),
    [P_FREL] = PD(W_REL, F_TIME, 0, 127, 60),
    [P_OCT] = {"OCT", F_SIGN, -3, 3, 0, 0, 0},
    [P_TUNE] = {"TUNE", F_SIGN, -99, 99, 0, 0, "ct"},
    /* (PHEBUS wave 5) the FX slots: OFF to start (Ben, 21 Nov 2026: "I don't like having fx on by default"); a TYPE picked
     * sets its three knobs to that type's start (xs_defaults). What TIME INTENS AMOUNT mean: xs_desc */
    [P_X1TYPE] = PE("TYPE", N_XTYPE, 0), [P_X1TIME] = PD("TIME", F_PCT, 0, 127, 0),
    [P_X1INT] = PD(W_DEPTH, F_PCT, 0, 127, 0), [P_X1AMT] = PD("AMOUNT", F_PCT, 0, 127, 0),
    [P_X2TYPE] = PE("TYPE", N_XTYPE, 0), [P_X2TIME] = PD("TIME", F_PCT, 0, 127, 0),
    [P_X2INT] = PD(W_DEPTH, F_PCT, 0, 127, 0), [P_X2AMT] = PD("AMOUNT", F_PCT, 0, 127, 0),
    [P_X3TYPE] = PE("TYPE", N_XTYPE, 0), [P_X3TIME] = PD("TIME", F_PCT, 0, 127, 0),
    [P_X3INT] = PD(W_DEPTH, F_PCT, 0, 127, 0), [P_X3AMT] = PD("AMOUNT", F_PCT, 0, 127, 0),
    /* the second LFO (modmx.c): free running from the start */
    [P_L2RATE] = PD("RATE", F_LFOHZ, 0, 127, 50),
    [P_L2WAVE] = PE("WAVE", N_LWAVE, 0),
};
/* an FX slot's TIME INTENS AMOUNT (k 0..2) as its TYPE names them, the MiniFreak's way (each effect three knobs). The defaults:
 * what a TYPE starts at when it is picked. (Wave 8, Ben 22 Nov 2026: "FX = whole mix only": CHORUS, REVERB and DELAY are no
 * slot types any more, the voice's sends are its SENDS page; their rows stay for the stored numbers, never shown) */
#define PU(l, f, df, n, u) {l, f, 0, 127, df, n, u}
static const param_desc_t XS_DESC[XT_N][3] = {
    [XT_OFF] = {PU("-", F_PCT, 0, 0, 0), PU("-", F_PCT, 0, 0, 0), PU("-", F_PCT, 0, 0, 0)},
    [XT_CHORUS] = {PU("RATE", F_LFOHZ, 40, 0, 0), PU("DEPTH", F_PCT, 60, 0, 0), PU("SEND", F_PCT, 24, 0, 0)},
    [XT_PHASER] = {PU("RATE", F_LFOHZ, 40, 0, 0), PU("FDBK", F_PCT, 64, 0, 0), PU("MIX", F_PCT, 127, 0, 0)},
    [XT_FLANGER] = {PU("RATE", F_LFOHZ, 30, 0, 0), PU("FDBK", F_PCT, 80, 0, 0), PU("MIX", F_PCT, 127, 0, 0)},
    [XT_REVERB] = {PU("SIZE", F_PCT, 90, 0, 0), PU("DAMP", F_PCT, 60, 0, 0), PU("SEND", F_PCT, 36, 0, 0)},
    [XT_DELAY] = {PU("TIME", F_PCT, 1, 0, 0), PU("FDBK", F_PCT, 60, 0, 0), PU("SEND", F_PCT, 28, 0, 0)},
    [XT_DIST] = {PU("TONE", F_PCT, 64, 0, 0), PU("DRIVE", F_PCT, 40, 0, 0), PU("MIX", F_PCT, 127, 0, 0)},
    [XT_SOFT] = {PU("TONE", F_CUTOFF, 96, 0, 0), PU("DRIVE", F_PCT, 64, 0, 0), PU("MIX", F_PCT, 127, 0, 0)},
    [XT_HARD] = {PU("TONE", F_CUTOFF, 96, 0, 0), PU("DRIVE", F_PCT, 64, 0, 0), PU("MIX", F_PCT, 127, 0, 0)},
    [XT_FOLD] = {PU("TONE", F_CUTOFF, 96, 0, 0), PU("DRIVE", F_PCT, 64, 0, 0), PU("MIX", F_PCT, 127, 0, 0)},
    [XT_FUZZ] = {PU("TONE", F_CUTOFF, 96, 0, 0), PU("DRIVE", F_PCT, 64, 0, 0), PU("MIX", F_PCT, 127, 0, 0)},
    [XT_CRUSH] = {PU("RATE", F_INT, 96, N_IRATE, "Hz"), PU("BITS", F_INT, 64, N_IBITS, 0), PU("MIX", F_PCT, 127, 0, 0)},
    [XT_EQ] = {PU("LOW", F_EQDB, 64, 0, 0), PU("MID", F_EQDB, 64, 0, 0), PU("HIGH", F_EQDB, 64, 0, 0)},
};
#undef PU
/* the voice's send to a shared effect of an old slot type (CHORUS, REVERB, DELAY: the stores' numbers): P_CHOR, P_REV, P_DLY */
static uint32_t xs_send_id(int32_t ty) { return ty == XT_CHORUS ? P_CHOR : ty == XT_REVERB ? P_REV : P_DLY; }
/* a preset's extra parameters (preset_t.x) into p, each clamped to its range */
static void preset_extras(int16_t *p, const preset_t *pr)
{
    uint32_t i;
    for (i = 0; i + 1u < 8u; i += 2u) {
        int32_t id = pr->x[i] - 1;
        if (id >= 0 && id < P_E0)
            p[id] = (int16_t)clamp(pr->x[i + 1u], TP[id].min, TP[id].max);
    }
}


static const char *const PART_N[5] = {"", "A", "B", "C", "D"};   /* GLO G_SLOT (1..4): SAVE > PARTS */
static const param_desc_t GP[G_COUNT] = {
    [G_BPM] = PD("BPM", F_BPM, 40, 240, 90),
    [G_SWING] = PD("SWING", F_SWING, 0, 100, 0),
    [G_CLOCK] = PE("CLICK", N_CLICK, 0),            /* (the old CLK slot: projects keep their format) */
    [G_TUNE] = PD("TUNE", F_INT, -50, 50, 0),
    [G_DTIME] = PE("TIME", N_DLY, 1),
    [G_DFDBK] = PD("FDBK", F_PCT, 0, 120, 60),   /* (wave 8: one spelling, FDBK) */
    [G_DCOLOR] = PD(W_COLOUR, F_PCT, 0, 127, 70),
    [G_DMIX] = PD("MIX", F_PCT, 0, 127, 90),
    [G_RSIZE] = PD("SIZE", F_PCT, 0, 127, 90),
    [G_RDAMP] = PD("DAMP", F_PCT, 0, 127, 60),
    [G_CRATE] = PD("RATE", F_LFOHZ, 0, 127, 40),    /* (wave 8: the chorus's, on FX > REV/CHO) */
    [G_CDEPTH] = PD("DEPTH", F_PCT, 0, 127, 60),
    [G_MIDI] = PE("OUT", N_MOUT, 0),            /* MIDI OUT: the keys, or the sequencer too (a setting of the FM-1) */
    [G_SYNC] = PE("SYNC", N_SYNC, 0),           /* a setting of the FM-1, not of a project (panel.c lights_sync) */
    [G_ROUTE] = PE("IN", N_MIN, 0),               /* MIDI IN: NOTES (and the clock), CLOCK only, or CH n: that channel plays the
                                                   * selected track (a setting of the FM-1; it starts at CH 1: panel.c lights_min) */
    [G_INFO] = PD("CPU", F_INT, 0, 0, 0),
    [G_SLOT] = {"PART", F_ENUM, 1, 4, 1, PART_N, 0},   /* (3.0.5) the section A..D */
    [G_NAME] = PE("NAME", N_DASH, 0),
    [G_LOAD] = PE("LOAD", N_GO, 0),
    [G_SAVE] = PE("SAVE", N_GO, 0),
    [G_ENGSEL] = PE("ENG", N_ENGNAME, 0),
    [G_ENGGO] = PE("SET", N_GO, 0),
    [G_CLRSEQ] = PE(W_CLEAR, N_GO, 0),
    [G_INITSND] = PE("INIT", N_GO, 0),
    [G_DRCH] = PE("CH4", N_MCH, 10),                  /* GM drum part (track 4) MIDI channel, 0 = OFF (2.6: as GLO > MIDI's CH1..3) */
    [G_DRLVL] = PD("LVL", F_INT, 0, 127, 100),
    [G_DRREV] = PD("REVERB", F_INT, 0, 127, 0),          /* (dry until asked for; SLOOP: 16) */
    [G_DUST] = PD("DUST", F_PCT, 0, 127, 0),
    [G_DUCK] = PD("DUCK", F_PCT, 0, 127, 0),
    [G_FILT] = PD("FILT", F_FILT, -64, 63, 0),
    [G_ROLL] = PE("ROLL", N_ROLL, 1),
    [G_NEWPRJ] = PE("NEW", N_GO, 0),
    [G_DRDLY] = PD("DELAY", F_INT, 0, 127, 0),
    [G_CH1] = PE("CH1", N_MCH, 1),                /* 2.6, GLO > MIDI: synth track 1's channel, in and out (OFF: */
    [G_CH2] = PE("CH2", N_MCH, 2),                /* no MIDI in; out on its old one) */
    [G_CH3] = PE("CH3", N_MCH, 3),
    [G_CHOTH] = PE("OTHER", N_MOTH, 0),           /* the channels no track has: SEL the selected track, OFF none */
    [G_HXMODEL] = PE("MODEL", N_HXMODEL, 0),     /* EDIT on the drum track: the HEX sound (ui_studio.c hex_ui_sync); */
    [G_HXWAVE] = PE("WAVE", N_HXWAVE, 0),        /* the LXR's 0..127 values. These are the DRUM model's: the pages */
    [G_HXTUNE] = PD("TUNE", F_INT, 0, 127, 36),  /* show each model's own (hex_desc) */
    [G_HXFINE] = PD("PSLOPE", F_INT, -64, 62, 0),
    [G_HXATK] = PD("ATK", F_INT, 0, 127, 0),
    [G_HXDEC] = PD("DEC", F_INT, 0, 127, 30),
    [G_HXSLOPE] = PD("SLOPE", F_INT, 0, 127, 40),
    [G_HXLVL] = PD("LEVEL", F_INT, 0, 127, 110),
    [G_HXPAMT] = PD(W_PAMT, F_INT, 0, 127, 0),
    [G_HXPDEC] = PD(W_PDEC, F_INT, 0, 127, 30),
    [G_HXRATIO] = PD("M.PIT", F_INT, 0, 127, 48),
    [G_HXFM] = PD("FM", F_INT, 0, 127, 0),
    [G_HXCLICK] = PE("TRANS", N_HXCLICK, 0),
    [G_HXCLVL] = PD("T.LVL", F_INT, 0, 127, 60),
    [G_HXCTONE] = PD("T.PIT", F_INT, 0, 127, 25),
    [G_HXFTYPE] = PE("TYPE", N_HXFILT, 0),       /* (wave 8: the filter page's words, CUTOFF RESO TYPE, */
    [G_HXCUT] = PD("CUTOFF", F_INT, 0, 127, 110),  /* as the synth voice's FILT) */
    [G_HXRES] = PD("RESO", F_INT, 0, 127, 0),
    [G_HXDRIVE] = PD("DRIVE", F_INT, 0, 127, 40),
    [G_HXAUX1] = PD("MOD", F_INT, 0, 127, 0),
    [G_HXAUX2] = PD("DIST", F_INT, 0, 127, 0),
    [G_HXCRUSH] = PD("CRUSH", F_INT, 0, 126, 0),
    [G_KNOBS] = PE("KNOBS", N_KNOBS, 4),            /* a controller's 8 knobs (CC 1-8 or 70-77: the Akai MPK minis):
                                                     * PANEL the FM-1's eight rotaries (PRESETS SELECT ALGORITHM MASTER,
                                                     * KNOB 1..4); 70-77, 1-8, AUTO: EDIT's two pages; GM: the GM map */
    [G_BEND] = PD("BEND", F_SEMI, 1, 12, 2),      /* 2.6, GLO > MIDI 2: MIDI pitch bend's range (a setting of the FM-1) */
};

static const param_desc_t DRUM_KIT_DESC = PE("KIT", DRUM_KIT_NAMES, 0);
/* the INSERT's A B C as its TYPE means them (TP's ranges and defaults): the drives DRIVE TONE LEVEL, CRUSH BITS RATE
 * LPF, the swept ones RATE DEPTH FDBK; OFF TP's own */
#define PU(l, f, df, n, u) {l, f, 0, 127, df, n, u}
static const param_desc_t INS_DESC[3][3] = {
    {PU("DRIVE", F_PCT, 64, 0, 0), PU("TONE", F_CUTOFF, 96, 0, 0), PU("LEVEL", F_DB, 96, 0, 0)},
    {PU("BITS", F_INT, 64, N_IBITS, 0), PU("RATE", F_INT, 96, N_IRATE, "Hz"), PU("LPF", F_CUTOFF, 96, 0, 0)},
    {PU("RATE", F_LFOHZ, 64, 0, 0), PU("DEPTH", F_PCT, 96, 0, 0), PU("FDBK", F_PCT, 96, 0, 0)},
};
#undef PU
static const param_desc_t *ins_desc(int32_t type, uint32_t k)
{
    static const param_desc_t NONE = {"-", F_PCT, 0, 127, 64, 0, 0};   /* INSERT OFF: A B C do nothing, not shown
                                                         * (they were INS A, INS B, INS C; review 21) */
    if (type <= 0 || type >= (int32_t)NELEM(N_ITYPE))
        return k < 3u ? &NONE : &TP[P_IA + k];
    return &INS_DESC[type < 5 ? 0 : type == 5 ? 1 : 2][k % 3u];   /* (SOFT .. FUZZ, CRUSH, the swept ones) */
}
static const param_desc_t ESYNC_DESC[3] = {   /* ATK DEC REL with ENV SYNC on: the range and defaults as ever */
    {"ATTACK", F_INT, 0, 127, 10, ESYNC_NAMES, 0},
    {"DECAY", F_INT, 0, 127, 70, ESYNC_NAMES, 0},
    {W_REL, F_INT, 0, 127, 60, ESYNC_NAMES, 0},
};

static const param_desc_t *track_desc(const track_t *t, uint32_t id)
{
    if(is_drum(t) && id==P_E0) return &DRUM_KIT_DESC;
    if (id >= P_X1TYPE && id <= P_X3AMT && (id - P_X1TYPE) % 4u) {   /* (wave 5) a slot's knob as its TYPE names it */
        int32_t ty = t->p[P_X1TYPE + (id - P_X1TYPE) / 4u * 4u];
        return &XS_DESC[ty > 0 && ty < XT_N ? ty : 0][(id - P_X1TYPE) % 4u - 1u];
    }
    if (id >= P_IA && id <= P_IC)                     /* the INSERT's values as its TYPE names them */
        return ins_desc(t->p[P_ITYPE], id - P_IA);
    if (t->p[P_ESYNC] && (id == P_ATK || id == P_DEC || id == P_REL || id == P_FATK || id == P_FDEC || id == P_FREL))
        return &ESYNC_DESC[id == P_ATK || id == P_FATK ? 0 : id == P_DEC || id == P_FDEC ? 1 : 2];   /* ENV SYNC: note
                                                       * values (wave 4: the filter's envelope too) */
    if (id >= P_E0 && id <= P_E7) {                   /* the engine asked for (t->engine follows after a fade) */
        const engine_t *e = ENGINES[t->eng_req % NENGINES];
        const param_desc_t *d = e->desc ? e->desc(t, id - P_E0) : 0;   /* a mode-dependent label / names */
        const char *l = is_drum(t) ? 0 : vtype_label(t, id - P_E0);
        if (!d)
            d = &e->edit[id - P_E0];
        if (l) {                                      /* (wave 4) the macro as the Type names it (vtype.c) */
            static param_desc_t vt_d[8];
            vt_d[id - P_E0] = *d;
            vt_d[id - P_E0].label = l;
            return &vt_d[id - P_E0];
        }
        return d;
    }
    return &TP[id];
}

/* a knob turned `steps` on an F_INT with a 0-terminated name list (ENV SYNC's note values, the INSERT's BITS and
 * RATE): a name a detent, landing on the first value that shows it (2.6, as Felucca 1.5's ENV SYNC) */
static int32_t names_turn(const param_desc_t *d, int32_t v, int32_t steps)
{
    int32_t n = 0, range = d->max - d->min + 1, k;
    while (d->names[n])
        n++;
    k = clamp((clamp(v, d->min, d->max) - d->min) * n / range + steps, 0, n - 1);
    return d->min + (k * range + n - 1) / n;
}

/* value string (<= 5 chars) and unit for a parameter value */
static void param_format(const param_desc_t *d, int32_t v, char *val, const char **unit)
{
    *unit = "";
    switch (d->fmt) {
    case F_PCT:                                       /* of the range: 0 .. 100 % */
        fmt_int(val, d->max > 0 ? (v * 100 + d->max / 2) / d->max : v);
        *unit = "%";
        break;
    case F_SWING:                                     /* 0 straight .. 100 the most (2.5: it showed MPC's 50..75 %,
                                                       * the share of a step pair the first one gets: confusing) */
        fmt_int(val, clamp(v, 0, 100));
        break;
    case F_FILT:                                      /* LP 1..100 % closed, HP 1..100 % */
        if (!v) {
            str_cpy(val, "OFF", 6);
        } else {
            str_cpy(val, v < 0 ? "LP" : "HP", 6);
            fmt_int(val + 2, v < 0 ? (-v * 100 + 32) / 64 : (v * 100 + 31) / 63);   /* (a mode name: no unit, review 18) */
        }
        break;
    case F_BIPCT:
        fmt_int(val, v * 100 / 64);
        if (v > 0) {
            char t[8];
            fmt_int(t, v * 100 / 64);
            val[0] = '+';
            str_cpy(val + 1, t, 6);
        }
        *unit = "%";
        break;
    case F_TIME: {
        uint32_t ms10 = TIME_MS_X10[v & 127];
        if (ms10 < 100u) {
            fmt_fix(val, (int32_t)ms10, 1);
            *unit = "ms";
        } else if (ms10 < 10000u) {
            fmt_int(val, (int32_t)((ms10 + 5u) / 10u));
            *unit = "ms";
        } else {
            fmt_fix(val, (int32_t)(ms10 / 100u), 2);
            if (ms10 >= 100000u)
                fmt_fix(val, (int32_t)(ms10 / 1000u), 1);
            *unit = "s";
        }
        break;
    }
    case F_LFOHZ: {
        uint32_t h = LFO_HZ_X100[v & 127];
        if (h < 1000u)
            fmt_fix(val, (int32_t)h, 2);
        else
            fmt_fix(val, (int32_t)(h / 10u), 1);
        *unit = "Hz";
        break;
    }
    case F_CUTOFF: {
        uint32_t h = CUTOFF_HZ[v & 127];
        if (h < 1000u) {
            fmt_int(val, (int32_t)h);
            *unit = "Hz";
        } else if (h < 10000u) {
            fmt_fix(val, (int32_t)(h / 100u), 1);
            *unit = "kHz";
        } else {
            fmt_int(val, (int32_t)((h + 500u) / 1000u));   /* "12 kHz": "12.5" would leave no room for the unit */
            *unit = "kHz";
        }
        break;
    }
    case F_DB:
        if (v <= 0) {
            str_cpy(val, "OFF", 6);
        } else if (LEVEL_DB_X10[v] > -100) {
            fmt_fix(val, LEVEL_DB_X10[v], 1);
            *unit = "dB";
        } else {
            fmt_int(val, (LEVEL_DB_X10[v] - 5) / 10);   /* "-12 dB": five characters leave no room for the unit */
            *unit = "dB";
        }
        break;
    case F_SIGN:                                      /* (wave 4) a signed whole number, + shown: OCT, TUNE */
        fmt_int(val, v);
        if (v > 0) {
            char t[8];
            fmt_int(t, v);
            val[0] = '+';
            str_cpy(val + 1, t, 6);
        }
        if (d->unit)
            *unit = d->unit;
        break;
    case F_EQDB: {                                    /* (wave 5) an EQ band: 64 flat, +-12 dB */
        int32_t db10 = (v - 64) * 120 / 64;
        if (!db10)
            str_cpy(val, "0", 6);
        else if (db10 > -100 && db10 < 100)
            fmt_fix(val, db10, 1);
        else
            fmt_int(val, db10 / 10);
        if (db10 > 0) {
            char t[8];
            str_cpy(t, val, 8);
            val[0] = '+';
            str_cpy(val + 1, t, 6);
        }
        *unit = "dB";
        break;
    }
    case F_SEMI:
        fmt_int(val, v);
        if (v > 0) {
            char t[8];
            fmt_int(t, v);
            val[0] = '+';
            str_cpy(val + 1, t, 6);
        }
        *unit = "st";
        break;
    case F_ENUM:
        str_cpy(val, d->names[v < d->min ? d->min : v > d->max ? d->max : v], 7);   /* (review V2: six letters) */
        if (d->unit)
            *unit = d->unit;
        break;
    case F_BPM:
        fmt_int(val, v);
        *unit = "BPM";
        break;
    case F_NOTE:
        str_cpy(val, N_NOTE[v % 12], 6);
        break;
    case F_ONOFF:
        str_cpy(val, N_ONOFF[v ? 1 : 0], 6);
        break;
    case F_STEPS:                                     /* (no unit: the label says STEPS, review 21) */
        fmt_int(val, v);
        break;
    default:
        if (d->names) {                               /* F_INT with a 0-terminated name list: the range */
            uint32_t k = 0;                           /* split evenly over the names (engine desc hooks) */
            while (d->names[k])
                k++;
            str_cpy(val, d->names[(uint32_t)(clamp(v, d->min, d->max) - d->min) * k / (uint32_t)(d->max - d->min + 1)], 6);
        } else {
            fmt_int(val, v);
        }
        if (d->unit)
            *unit = d->unit;
        break;
    }
}

/* ------------------------------------------------------------ pages --- */
/* (Ben, 20 Nov 2026: "Oscillator engine > Filter > ADSR > FX + MOD matrix", after the MiniFreak) a voice's sound
 * pages are one chain of five stages, in signal order, the same on every engine: OSC (FAM_EDIT) > FILTER (FAM_FLT)
 * > ENV (FAM_ENV) > FX (FAM_FX) > MOD (FAM_LFO). SELECT walks the whole chain; EDIT steps stage to stage; ENV, FX
 * and LFO open their own stage (ui.c, chain_open). A stage's first page holds its four most used controls. The
 * family numbers are the UI's only (no project, preset, editor or MIDI number holds them) */
/* (wave 8, Ben 22 Nov 2026: "FX = whole mix only") FAM_MFX: the FX button's pages, the whole mix's (DELAY, REV/CHO, MASTER);
 * FAM_FX stays the voice's FX stage (its three inserts, SENDS, SLICER), reached by EDIT and SELECT on the chain. FAM_DRM: the
 * drum voice's own pages (FILTER, INSERT, SENDS), shown only as the drum screen's FX tab (ui_drum.c) */
enum { FAM_HOME, FAM_ENV, FAM_LFO, FAM_FX, FAM_SCL, FAM_EDIT, FAM_GLO, FAM_SAVE, FAM_ARP, FAM_SEQ, FAM_TRK, FAM_FLT,
       FAM_MFX, FAM_DRM, FAM_COUNT };
#define FAM_OSC FAM_EDIT
#define FAM_MOD FAM_LFO
#define NSTAGE 5u
static const uint8_t STAGE_FAM[NSTAGE] = {FAM_OSC, FAM_FLT, FAM_ENV, FAM_FX, FAM_MOD};
static const char *const STAGE_NAME[NSTAGE] = {"OSC", "FILT", "ENV", "FX", "MOD"};
static int32_t fam_stage(uint32_t fam)             /* the chain's stage of a family, -1 = not a sound stage */
{
    uint32_t s;
    for (s = 0; s < NSTAGE; s++)
        if (STAGE_FAM[s] == fam)
            return (int32_t)s;
    return -1;
}
enum { SC_TRACK, SC_GLOBAL, SC_ENGINE, SC_STEP, SC_TRK, SC_DRUM };
enum { GR_NONE, GR_ADSR, GR_LFO, GR_STEPS, GR_ARP, GR_SCALE, GR_FX, GR_BROWSE, GR_SLOTS, GR_USER, GR_TRK,
       GR_SLCR, GR_INS, GR_HEX, GR_XSLOT, GR_LFO2, GR_MATRIX, GR_SENDS, GR_SPATT, GR_UKIT, GR_PROJ };

typedef struct {
    const char *title;
    uint8_t fam, scope, graph;
    uint8_t id[4];               /* param ids; 0xFF = empty slot */
    uint8_t sel;                 /* (wave 9, Ben 23 Nov 2026: "the select knob ... acting as a knob 5 effectively changing
                                  * parameters within the category that have a type") the id SELECT turns on this page: its
                                  * TYPE (V_TYPE, a slot's TYPE, the filter's, the LFO's WAVE, the SLICER's mode, the INSERT's);
                                  * SEL_NONE: a page with no type, SELECT does nothing there */
    uint8_t cat;                 /* (wave 9, Ben 23 Nov 2026, 08:42: "Hit the edit key, which brings Osc and Filter categories.
                                  * And if the Osc category has 4 pages, you cycle through those using the Sel ... if you press
                                  * edit again it moves to the next category") its category (CT_*): a labelled button steps
                                  * through its categories (ui_nav.c), SEL (PAGE) through the pages of one, in PAGES order */
    uint8_t eng;                 /* 0: every synth engine; PG_ENG(e): engine e's own page; PG_OTHER: an engine without
                                  * its own pages (none a track can choose today: ENG_GONE, the host tests' engine 0) */
} page_t;
#define SEL_NONE 0xFFu
/* (wave 9) the categories: a labelled button's group is a list of them (ui_nav.c NAV_GROUP); those after CT_GRID live on the
 * GRID and the drum screen only (their pages are tabs and halves, not PAGES) or are screens of their own */
enum { CT_NONE, CT_OSC, CT_FILT, CT_AMP, CT_FENV, CT_LFO1, CT_LFO2, CT_MATRIX, CT_INS, CT_SENDS, CT_SLICER, CT_MIX,
       CT_DFILT, CT_DINS, CT_DSENDS, CT_DSLICER, CT_ARP, CT_KEY, CT_GLOBAL, CT_SYSTEM, CT_MIDI, CT_PRESETS, CT_USER,
       CT_TOOLS, CT_HOME, CT_GRID,
       CT_TRIG, CT_PATT, CT_PATTS, CT_SRC, CT_FLTR, CT_DAMP, CT_DLFO, CT_CHROM, CT_DEVICE, CT_SAVER, CT_SPATT, CT_KITS,
       CT_STUT,                 /* (wave 14, Ben 23 Nov 2026, 15:55: "Both on FX") the STUTTER lane, FX's last category: a screen of its own */
       CT_PROJ,                 /* (the bank, Ben 22:45) SAVE > PROJECT: KEEP, SWAP, NEW */
       CT_N };
#define PG_ENG(e) ((uint8_t)((e) + 1u))
#define PG_OTHER 0xFFu

/* (PHEBUS wave 4) the OSC page's knobs that are not a parameter of their own: TYPE (the engine and its MODEL, vtype.c) and
 * the Type's macros M1 .. M5 (an engine's E0 .. E7 by Type: pg_id resolves them on the selected track). Above every P_ id */
enum { V_TYPE = 0xF0, V_M1, V_M2, V_M3, V_M4, V_M5,
       /* (wave 5) an FX slot's TIME INTENS AMOUNT (as its TYPE means them: xs_knob_id); LFO 2's SYNC and TRIG (mxt_t); the
        * matrix page's SOURCE DEST AMOUNT and its fourth knob (CLEAR, or on an assignable column TARGET) */
       V_XA, V_XB, V_XC, V_L2SYNC, V_L2TRIG, V_MSRC, V_MDST, V_MAMT, V_MK4 };
#define V_LEARN 0xEEu          /* (wave 9) GLO > MIDI 3: MIDI LEARN on / off (it was GLO + F#3); 0xFF is an empty knob */
_Static_assert(P_COUNT < V_LEARN && G_COUNT < V_LEARN, "V_LEARN above every parameter");
/* (review U5, Ben 26 Nov 2026: "Redraw in the GLO frame") GLO > DEVICE's settings as knobs of normal GLO pages: the FM-1's own
 * settings (kept with the settings, not a project: ui_menu.c dev_desc, dev_edit); and (MIDI 3) LEARN's CLEAR beside LEARN */
enum { V_COLOR = 0xD0, V_ZOOM, V_DISCO, V_LIGHTS, V_KEYS, V_NOTES, V_LOWCUT, V_USB, V_SERIAL, V_CALIB, V_ABOUT, V_LCLEAR };
#define V_DEV0 V_COLOR
#define V_DEVN V_LCLEAR
_Static_assert(P_COUNT < V_DEV0 && G_COUNT < V_DEV0 && V_DEVN < 0xE8u, "the DEVICE ids above every parameter, below 0xE8");
_Static_assert(P_COUNT <= V_TYPE, "the virtual knob ids above every parameter");

static const page_t PAGES[] = {
    /* ---- the sound chain (fam_stage): OSC > FILT > ENV > FX > MOD. (PHEBUS wave 4; Ben, 21 Nov 2026: "Melodic voices: the
     * MiniFreak's swift sound design ... The same layout on every voice") every synth track has these pages, the same
     * knobs in the same places, whatever its Type; the drum track's are its own FILTER, INSERT and SLICER (its sounds'
     * pages are the drum screen's, SRC FLTR AMP LFO: PHEBUS wave 3, ui_drum.c) */
    /* OSC: the Type and its three macros (mock-synth-osc-A); the pitch and the voice; the Type's other controls */
    {"OSC", FAM_OSC, SC_TRACK, GR_NONE, {V_M1, V_M2, V_M3, V_M4}, V_TYPE, CT_OSC},
    {"OSC 2", FAM_OSC, SC_TRACK, GR_NONE, {P_OCT, P_TUNE, P_GLIDE, P_GLMODE}, V_TYPE, CT_OSC},
    {"OSC 3", FAM_OSC, SC_TRACK, GR_NONE, {V_M5, P_VOICE, P_DETUNE, P_PRIO}, V_TYPE, CT_OSC},
    {"OSC 4", FAM_OSC, SC_TRACK, GR_NONE, {P_ALLOC, 0xFF, 0xFF, 0xFF}, V_TYPE, CT_OSC},   /* (wave 5) the voicing settings: GLIDE's mode,
                                                         * the poly allocation (the MiniFreak keeps them in its Sound settings) */
    /* FILT: the voice filter (vfilt.c; mock-synth-filt-A): CUTOFF RESO ENV TYPE, then KEY DRIVE and the envelope's LINK */
    {"FILT", FAM_FLT, SC_TRACK, GR_NONE, {P_FCUT, P_FRES, P_ED_FLT, P_FDRV}, P_FTYPE, CT_FILT},
    {"FILT 2", FAM_FLT, SC_TRACK, GR_NONE, {P_FKEY, P_FLINK, 0xFF, 0xFF}, P_FTYPE, CT_FILT},   /* (wave 8: DRIVE first, as the drum FLTR's 2/2) */
    /* ENV: the amp envelope, then the filter's own (mock-synth-env-A: AMP | FILT) */
    {"ENV", FAM_ENV, SC_TRACK, GR_ADSR, {P_ATK, P_DEC, P_SUS, P_REL}, SEL_NONE, CT_AMP},
    {"FILT ENV", FAM_ENV, SC_TRACK, GR_ADSR, {P_FATK, P_FDEC, P_FSUS, P_FREL}, SEL_NONE, CT_FENV},
    /* FX: (PHEBUS wave 5, mock-synth-fx-A) three slots in series, each a TYPE and its three knobs (fx.c xs_run). (Wave 8, Ben
     * 22 Nov 2026: "FX = whole mix only"; "each voice's fx sends should be on their own edit page") the slots are this voice's
     * inserts only; SENDS: how much of this voice goes to the shared delay, reverb and chorus (whose settings are the FX
     * button's pages, never a voice's); then the SLICER, the voice's own */
    {"FX 1", FAM_FX, SC_TRACK, GR_XSLOT, {V_XA, V_XB, V_XC, 0xFF}, P_X1TYPE, CT_INS},
    {"FX 2", FAM_FX, SC_TRACK, GR_XSLOT, {V_XA, V_XB, V_XC, 0xFF}, P_X2TYPE, CT_INS},
    {"FX 3", FAM_FX, SC_TRACK, GR_XSLOT, {V_XA, V_XB, V_XC, 0xFF}, P_X3TYPE, CT_INS},
    {"SENDS", FAM_FX, SC_TRACK, GR_SENDS, {P_DLY, P_REV, P_CHOR, 0xFF}, SEL_NONE, CT_SENDS},
    {"SLICER", FAM_FX, SC_TRACK, GR_SLCR, {P_SLPAT, P_SLRATE, P_SLDEPTH, 0xFF}, P_SLCR, CT_SLICER},   /* drum track too (its FX tab) */
    /* MOD: the LFO (RATE WAVE PHASE FADE; SYNC TRIG POL, and ENV SYNC), the second LFO, then the matrix (mock-synth-mod-A:
     * the grid, Ben's pick). Where the LFO and the envelopes go is the matrix's (the old LFO DEST and ENV DEST are cells) */
    {"LFO 1", FAM_MOD, SC_TRACK, GR_LFO, {P_LRATE, P_LPHASE, P_LFADE, P_LPOL}, P_LWAVE, CT_LFO1},
    {"LFO SYNC", FAM_MOD, SC_TRACK, GR_LFO, {P_LSYNC, P_LTRIG, P_ESYNC, 0xFF}, P_LWAVE, CT_LFO1},   /* SYNC TRIG POL, ENV SYNC */
    {"LFO 2", FAM_MOD, SC_TRACK, GR_LFO2, {P_L2RATE, V_L2SYNC, V_L2TRIG, 0xFF}, P_L2WAVE, CT_LFO2},
    {"MATRIX", FAM_MOD, SC_TRACK, GR_MATRIX, {V_MSRC, V_MDST, V_MAMT, V_MK4}, SEL_NONE, CT_MATRIX},
    /* ---- the drum voice's own (its bus: the Rytm's FX track), the drum screen's FX tab (ui_drum.c DBUS), with the SLICER
     * above: its FILTER (LP / HP), its INSERT (2.6, Felucca 1.5) and dry / wet. (The drum effects under FX, Ben 21:45: the kit-wide
     * SENDS page is gone, "Remove them": each sound's own sends are FX > SENDS, a drum screen sound page, ui_drum.c) */
    {"FILTER", FAM_DRM, SC_TRACK, GR_NONE, {P_TFLT, 0xFF, 0xFF, 0xFF}, SEL_NONE, CT_DFILT},
    {"INSERT", FAM_DRM, SC_TRACK, GR_INS, {P_IA, P_IB, P_IC, 0xFF}, P_ITYPE, CT_DINS},   /* (review 26 Nov, Ben's picker "Type on
                                                         * SELECT, as synths": the type on SELECT, its values on KNOB 1 to 3, as
                                                         * a synth voice's FX slot; it was KNOB 1 since wave 9) */
    {"INSERT 2", FAM_DRM, SC_TRACK, GR_INS, {P_IMIX, 0xFF, 0xFF, 0xFF}, SEL_NONE, CT_DINS},
    {"SLICER", FAM_DRM, SC_TRACK, GR_SLCR, {P_SLPAT, P_SLRATE, P_SLDEPTH, 0xFF}, P_SLCR, CT_DSLICER},   /* (review 26 Nov) the drum
                                                         * voice's, as the synth SLICER: its mode on SELECT, PAT RATE DEPTH on
                                                         * KNOB 1 to 3 */
    /* ---- the other pages */
    {"KEY", FAM_SCL, SC_TRACK, GR_SCALE, {P_ROOT, P_SCALE, P_QUANT, P_CHORD}, SEL_NONE, CT_KEY},
    {"KEY 2", FAM_SCL, SC_TRACK, GR_SCALE, {P_TRANS, P_STRUM, P_VLEAD, 0xFF}, SEL_NONE, CT_KEY},   /* (2.4: the chords played) */
    /* (wave 8) GLO is the settings (and, held, the mixer): the tempo lives here and only here (Ben: "tempo in one place");
     * the swing is the pattern's (hold SEQ), the effects the FX button's, the drums' level the mixer's */
    {"GLOBAL", FAM_GLO, SC_GLOBAL, GR_NONE, {G_BPM, G_CLOCK, G_TUNE, 0xFF}, SEL_NONE, CT_GLOBAL},
    {"SYSTEM", FAM_GLO, SC_GLOBAL, GR_NONE, {G_SYNC, G_INFO, 0xFF, 0xFF}, SEL_NONE, CT_SYSTEM},   /* (review U6, Ben: "One home
                                                         * each": OUT and IN are MIDI 2's only) */
    {"MIDI", FAM_GLO, SC_GLOBAL, GR_NONE, {G_CH1, G_CH2, G_CH3, G_DRCH}, SEL_NONE, CT_MIDI},   /* 2.6: each track's channel, the drums' too */
    {"MIDI 2", FAM_GLO, SC_GLOBAL, GR_NONE, {G_ROUTE, G_CHOTH, G_MIDI, G_BEND}, SEL_NONE, CT_MIDI},   /* 2.6: IN, OTHER, OUT, BEND: every MIDI
                                                                           * setting in these pages */
    {"MIDI 3", FAM_GLO, SC_GLOBAL, GR_NONE, {G_KNOBS, V_LEARN, V_LCLEAR, 0xFF}, SEL_NONE, CT_MIDI},   /* a controller's knobs (2026-10-20);
                                                         * (review U5) LEARN's CLEAR beside it */
    /* (wave 8) the FX button: the whole mix's effects, white and grey. The delay, the reverb and chorus every voice sends to
     * (each voice's SENDS page; PERFORM's ECHO and VERB throw into them), the master bus (DUST, DUCK) */
    {"DELAY", FAM_MFX, SC_GLOBAL, GR_NONE, {G_DTIME, G_DFDBK, G_DCOLOR, G_DMIX}, SEL_NONE, CT_MIX},
    {"REV/CHO", FAM_MFX, SC_GLOBAL, GR_NONE, {G_RSIZE, G_RDAMP, G_CRATE, G_CDEPTH}, SEL_NONE, CT_MIX},
    {"MASTER", FAM_MFX, SC_GLOBAL, GR_NONE, {G_DUST, G_DUCK, 0xFF, 0xFF}, SEL_NONE, CT_MIX},
    {"PRESETS", FAM_SAVE, SC_GLOBAL, GR_BROWSE, {0xFF, 0xFF, 0xFF, 0xFF}, SEL_NONE, CT_PRESETS},   /* browser: PRESETS knob / KNOB 1 */
    {"USER", FAM_SAVE, SC_GLOBAL, GR_USER, {0xFF, 0xFF, 0xFF, 0xFF}, SEL_NONE, CT_USER},       /* user presets: SLOT LOAD ERASE SAVE */
    /* (wave 12) the user kits, whole drum kits of 16 sounds: KIT LOAD ERASE SAVE (ui_kit.c) */
    {"KITS", FAM_SAVE, SC_GLOBAL, GR_UKIT, {0xFF, 0xFF, 0xFF, 0xFF}, SEL_NONE, CT_KITS},
    /* (wave 11) storing a pattern a home of its own (it was hold SAVE + key 1..4 only, the shortcut it stays): KNOB 1 the
     * pattern A..D, KNOB 2 STORE (a GO: right stores, over a stored one it asks: hold SAVE) */
    {"PATTERNS", FAM_SAVE, SC_GLOBAL, GR_SPATT, {0xFF, 0xFF, 0xFF, 0xFF}, SEL_NONE, CT_SPATT},
    /* (3.0.5) PROJECT (PARTS) is gone from SAVE: the sections are SAVE held + keys 1..8. SAVE here is the sound's */
    {"TOOLS", FAM_SAVE, SC_GLOBAL, GR_NONE, {G_CLRSEQ, G_INITSND, 0xFF, 0xFF}, SEL_NONE, CT_TOOLS},   /* (the bank: NEW moved to
                                                         * PROJECT, with KEEP and SWAP; G_NEWPRJ keeps its id) */
    {"ARP", FAM_ARP, SC_TRACK, GR_ARP, {P_AMODE, P_ARATE, P_AOCT, P_AGATE}, SEL_NONE, CT_ARP},
    {"ARP 2", FAM_ARP, SC_TRACK, GR_NONE, {P_ASWING, P_APROB, P_AHOLD, P_AORDER}, SEL_NONE, CT_ARP},
    {"TRACKS", FAM_TRK, SC_TRK, GR_TRK, {0, 1, 2, 3}, SEL_NONE, CT_HOME},   /* REC button; TRACK LEVEL LEN PAN */
    {"DRUMS", FAM_TRK, SC_DRUM, GR_NONE, {0xFF,0xFF,0xFF,0xFF}, SEL_NONE, CT_GRID},
    /* (the bank, Ben 22:45: "one live one and one dead one") SAVE's first category: the live project and the one kept in the
     * bank, KEEP (KNOB 1), SWAP (KNOB 2), NEW (KNOB 4): ui_draw.c graph_proj, project.c bank_op. (Last in the list so no page
     * moves its index; SAVE's order is ui_nav.c's) */
    {"PROJECT", FAM_SAVE, SC_GLOBAL, GR_PROJ, {0xFF, 0xFF, 0xFF, 0xFF}, SEL_NONE, CT_PROJ},
    /* (review U5) GLO > DEVICE: the FM-1's settings, four GLO pages (they were the old menu's sections; last, so no
     * page before moves its index) */
    {"SCREEN", FAM_GLO, SC_GLOBAL, GR_NONE, {V_COLOR, V_ZOOM, 0xFF, 0xFF}, SEL_NONE, CT_DEVICE},
    {"LIGHTS", FAM_GLO, SC_GLOBAL, GR_NONE, {V_DISCO, V_LIGHTS, V_KEYS, V_NOTES}, SEL_NONE, CT_DEVICE},
    {"AUDIO", FAM_GLO, SC_GLOBAL, GR_NONE, {V_LOWCUT, V_USB, V_SERIAL, 0xFF}, SEL_NONE, CT_DEVICE},
    {"TOOLS", FAM_GLO, SC_GLOBAL, GR_NONE, {V_CALIB, V_ABOUT, 0xFF, 0xFF}, SEL_NONE, CT_DEVICE},
};
#define NPAGES (sizeof(PAGES) / sizeof(PAGES[0]))

/* the drum track has no sound of its own: it uses the global pages (not the preset
 * pages, nor TOOLS > INIT: page_desc), STEP, PATTERN, SLICER and TRACKS; every other page
 * shows "DRUM TRACK" */
static int page_for_drum(const page_t *pg)
{
    if (pg->scope == SC_GLOBAL)
        return pg->graph != GR_USER;                /* (wave 11: SAVE > PATTERN: the drums' too; wave 12: PRESETS, their kits) */
    if (pg->eng)                                    /* (a synth engine's own page) */
        return 0;
    return pg->scope != SC_ENGINE && (pg->scope != SC_TRACK || pg->fam == FAM_DRM || pg->graph == GR_SLCR);   /* (wave 8: its
                                                         * FILTER, INSERT, SENDS, SLICER: the drum screen's FX tab) */
}
/* a page the buttons and SELECT reach on the selected track. On the sound chain (fam_stage) a page the voice does
 * not have is skipped, never shown empty: (wave 4) every synth track has the same chain, the drum track its own (FILTER,
 * INSERT, SLICER, the global FX) */
static int page_shown(const page_t *pg)
{
    if (pg->graph == GR_HEX)                         /* (the HEX sound pages: cut, PHEBUS wave 3: the drum screen's pages) */
        return 0;
    if (fam_stage(pg->fam) >= 0)                     /* (wave 8) the chain is the synth voices'; the drum voice's pages are
                                                         * the drum screen's (SRC FLTR AMP LFO FX: ui_drum.c) */
        return !is_drum(TSEL);
    if (is_drum(TSEL) && (pg->graph == GR_BROWSE || pg->graph == GR_USER))
        return 0;                                    /* (review 26 Nov, "One home each", "Skip USER on drums": the drums'
                                                         * SAVE > PRESETS is KITS, no USER; ui.c page_fix lands on KITS) */
    return pg->fam != FAM_DRM;                       /* (the drum screen's FX tab only: no button opens them) */
}
/* (wave 4) knob k's parameter id on page pg for the selected track: the OSC page's macros M1 .. M5 (vtype.c) as the
 * engine's E0 .. E7 they are on its Type (0xFF: the Type has none); every other id as it is (V_TYPE stays V_TYPE) */
#include "modmx.c"             /* (PHEBUS wave 5) the mod matrix */
static int16_t mx_row, mx_col;   /* (wave 5) the MATRIX page's cursor: the cell SOURCE and DEST point at (the UI's) */
/* (wave 11) the MATRIX page's columns, as shown: the eleven fixed ones, the assignable columns that have a parameter (in their
 * order), then one empty assignable column, "+", to make another (none when every column has one). Returns how many */
static uint32_t mx_vis(uint32_t ti, uint8_t *cols)
{
    uint32_t c, n = 0, fr = MX_NAS;
    for (c = 0; c < MD_NFIX; c++)
        cols[n++] = (uint8_t)c;
    for (c = 0; c < MX_NAS; c++) {
        if (mxt[ti % NPART].as[c])
            cols[n++] = (uint8_t)(MD_AS1 + c);
        else if (fr == MX_NAS)
            fr = c;
    }
    if (fr < MX_NAS)
        cols[n++] = (uint8_t)(MD_AS1 + fr);
    return n;
}
static uint32_t mx_vis_pos(uint32_t ti, uint32_t col)  /* column col's place among those shown (not shown: the last) */
{
    uint8_t cols[MD_N];
    uint32_t n = mx_vis(ti, cols), i;
    for (i = 0; i < n; i++)
        if (cols[i] == col)
            return i;
    return n - 1u;
}
/* a column's name in the grid's head: three letters, one name a parameter (wave 11: up to 19 parameter columns side by side, so
 * the LFOs' RATEs, the two envelopes' stages and the slots' knobs need names of their own); "+" an empty column */
static void mx_col_name(uint32_t ti, uint32_t col, char *b)
{
    static const struct { uint8_t id; char n[4]; } SN[] = {
        {P_ATK, "ATK"}, {P_DEC, "DEC"}, {P_SUS, "SUS"}, {P_REL, "REL"}, {P_FATK, "FAT"}, {P_FDEC, "FDC"}, {P_FSUS, "FSU"},
        {P_FREL, "FRL"}, {P_FDRV, "DRV"}, {P_GLIDE, "GLD"}, {P_DETUNE, "DTN"}, {P_DLY, "DLY"}, {P_REV, "REV"}, {P_CHOR, "CHO"},
        {P_LRATE, "L1R"}, {P_LPHASE, "L1P"}, {P_LFADE, "L1F"}, {P_L2RATE, "L2R"}, {P_SLDEPTH, "SLD"}, {P_FCUT, "CUT"},
        {P_FRES, "RES"}, {P_TUNE, "TUN"}, {P_PAN, "PAN"}, {P_LEVEL, "LVL"}};
    uint32_t a = col >= MD_AS1 && col < MD_N ? mxt[ti % NPART].as[col - MD_AS1] : 0u, i, id;
    if (col < MD_NFIX) {
        str_cpy(b, N_MDST[col], 6);
        return;
    }
    if (!a) {
        str_cpy(b, "+", 6);
        return;
    }
    id = a - 1u;
    for (i = 0; i < NELEM(SN); i++)
        if (SN[i].id == id) {
            str_cpy(b, SN[i].n, 4);
            return;
        }
    if (id >= P_X1TYPE && id < P_X1TYPE + 4u * NXSLOT) {   /* a slot's knob: its number, T I A */
        b[0] = (char)('1' + (id - P_X1TYPE) / 4u), b[1] = "-TIA"[(id - P_X1TYPE) % 4u], b[2] = 0;
        return;
    }
    if (id >= P_E0 && id <= P_E7) {                     /* the Type's controls: E1 .. E8 */
        b[0] = 'E', b[1] = (char)('1' + id - P_E0), b[2] = 0;
        return;
    }
    str_cpy(b, track_desc(&trk[ti % NPART], id)->label, 4);
}
/* (wave 5) an FX page's slot (0..2) */
static uint32_t pg_slot(const page_t *pg) { return ((uint32_t)pg->sel - P_X1TYPE) / 4u % NXSLOT; }   /* (wave 9: its TYPE is SELECT's) */
static uint32_t pg_id(const page_t *pg, uint32_t k)
{
    uint32_t id = pg->id[k & 3u];
    if (id >= V_M1 && id <= V_M5) {
        uint32_t m = is_drum(TSEL) ? 0xFFu : vtype_macro(TSEL, id - V_M1);
        return m < 8u ? P_E0 + m : 0xFFu;
    }
    if (id >= V_XA && id <= V_XC) {                   /* (wave 5) an FX slot's knob, its own (wave 8: every type an insert,
                                                       * no shared setting on a voice's page); OFF: none */
        int32_t ty = TSEL->p[pg->sel];
        if (is_drum(TSEL) || ty <= XT_OFF || ty >= XT_N || xt_send(ty))
            return 0xFFu;
        return P_XS(pg_slot(pg), id - V_XA + 1u);
    }
    if (id == V_MAMT) {                               /* (wave 5) a fixed cell's AMOUNT is its parameter (it locks) */
        int32_t a = mx_alias((uint32_t)mx_row % MS_N, (uint32_t)mx_col % MD_N);
        return a >= 0 ? (uint32_t)a : id;
    }
    return id;
}
static uint8_t learn_shown;                             /* (wave 9: MIDI LEARN on, for GLO > MIDI 3: ui.c ml_toggle sets it) */
static const param_desc_t VTYPE_DESC = {"TYPE", F_INT, 0, NVTYPE - 1u, 0, 0, 0};   /* (formatted by vtype_fmt) */
static int16_t vtype_shown;                            /* (TYPE's value, as page_desc hands it out: vtype_of) */
static const param_desc_t *dev_desc(uint32_t id, int16_t **valp);   /* (review U5: ui_menu.c) */
static const param_desc_t *page_desc(const page_t *pg, uint32_t slot, int16_t **valp)
{
    uint32_t id = pg_id(pg, slot);
    if (id >= V_DEV0 && id <= V_DEVN)
        return dev_desc(id, valp);
    if (id == V_TYPE && !is_drum(TSEL)) {
        vtype_shown = (int16_t)vtype_of(TSEL);
        *valp = &vtype_shown;
        return &VTYPE_DESC;
    }
    if (id == V_LEARN) {                                  /* (wave 9) MIDI LEARN on / off (ui.c mlu, not saved) */
        static const char *const OO[] = {"OFF", "ON"};
        static const param_desc_t L = PE("LEARN", OO, 0);
        vtype_shown = (int16_t)(learn_shown != 0);
        *valp = &vtype_shown;
        return &L;
    }
    if (id == 0xFFu || (is_drum(TSEL) && (!page_for_drum(pg) || (pg->scope == SC_GLOBAL && id == G_INITSND)))) {
        *valp = 0;
        return 0;
    }
    if (pg->scope == SC_TRK) {
        *valp = 0;
        return 0;
    }
    if (pg->scope == SC_TRACK && !is_drum(TSEL) && id >= V_XA && id <= V_MK4) {   /* (wave 5) the slots, LFO 2, the matrix */
        static param_desc_t vd[4];
        static int16_t vv[4];
        uint32_t ti = song.sel % NPART;
        param_desc_t *d = &vd[slot & 3u];
        if (id <= V_XC) {                             /* (an OFF slot's knobs: none) */
            *valp = 0;
            return 0;
        }
        if (id == V_L2SYNC || id == V_L2TRIG) {
            static const param_desc_t L2[2] = {PE("SYNC", N_LSYNC, 0), PE("TRIG", N_LTRIG, 1)};
            *valp = id == V_L2SYNC ? &mxt[ti].l2sync : &mxt[ti].l2trig;
            return &L2[id - V_L2SYNC];
        }
        if (id == V_MSRC) {
            static const param_desc_t S = PE("SOURCE", N_MSRC, 0);
            *valp = &mx_row;
            return &S;
        }
        if (id == V_MDST) {                           /* (wave 11) the columns shown: the fixed, the assignable in use, "+" */
            static param_desc_t D = {"DEST", F_INT, 0, 0, 0, 0, 0};
            uint8_t cols[MD_N];
            uint32_t n = mx_vis(ti, cols);
            D.max = (int16_t)(n - 1u);
            vv[1] = (int16_t)mx_vis_pos(ti, (uint32_t)mx_col);
            *valp = &vv[1];
            return &D;
        }
        if (id == V_MAMT) {                           /* a grid cell (a fixed one is its parameter: pg_id) */
            static const param_desc_t A = {"AMOUNT", F_BIPCT, -64, 63, 0, 0, 0};
            vv[2] = (int16_t)mx_get(ti, (uint32_t)mx_row % MS_N, (uint32_t)mx_col % MD_N);
            *valp = &vv[2];
            return &A;
        }
        /* V_MK4: on an assignable column its TARGET (the parameter it moves), elsewhere CLEAR (a turn: the cell to 0) */
        if (mx_col >= MD_AS1 && mx_col < (int16_t)MD_N) {
            static const param_desc_t T = {"TARGET", F_INT, 0, P_COUNT, 0, 0, 0};
            vv[3] = (int16_t)mxt[ti].as[mx_col - MD_AS1];
            *valp = &vv[3];
            return &T;
        } else {
            static const param_desc_t C = {"CLEAR", F_INT, 0, 1, 0, 0, 0};
            vv[3] = 0;
            *valp = &vv[3];
            return &C;
        }
    }
    if (pg->scope == SC_GLOBAL) {
        *valp = &song.g[id];
        return &GP[id];
    }
    if (id == V_MAMT || id > P_COUNT) {               /* (a fixed cell's AMOUNT: its parameter, below; else none) */
        *valp = 0;
        return 0;
    }
    *valp = &TSEL->p[id];
    return track_desc(TSEL, id);
}

#include "drum_par.c"           /* (PHEBUS wave 3) the drum sounds' pages: their parameters, ranges, names */
