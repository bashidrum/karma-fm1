/* SPDX-License-Identifier: GPL-3.0-only */
/* PERFORM (PHEBUS wave 6, rebuilt in wave 13). Built on KARMA's PUNCH-IN FX (pocket-operator style: hold FX, press a key and
 * the whole mix goes through that effect while the key is held; release it and the mix comes back), now the MPC's way (Ben,
 * 21 Nov 2026: "Then I want the MPC's feel for pad like fx play and stuttering"). The keys are pads (PF_*): momentary while
 * held, latched with OCT+, several at once (one time effect, every colour effect, the throws and the mutes together), four
 * knobs on the last pad pressed. Played pads record into the STUTTER lane while the sequencer records (Ben: "Keep, and record
 * played FX into it"): stl.ev below, saved with each pattern (project.c, format 8's 124 bytes).
 * (wave 13, Ben 23 Nov 2026, 14:13: "If you could play all those mutes and fx in one place that would be a power house for
 * keeping beats varied in daft punk style drops") THE PERFORM SET: effects on the white keys, four rows of four,
 *     1/1   1/2   1/4    1/8          the stutters ("Most of the fx I'll use are broad stutters")
 *     1/16  HALF  ECHO   VERB         half time ("I like 1/2 time"), the throws
 *     ISO LO ISO MID ISO HI MOTION    isolate ("I want to hear this band": 220 Hz and 1.1 kHz), a tempo-synced band-pass sweep
 *     CRUSH PHASER JET   VOWEL        a subtle crusher ("thin and papery / gritty"), phaser, jet, a formant filter
 * and the mutes and four extras on the black keys, F#3 .. F#5:
 *     V1 V2 V3 | KICKS TOPS SH+CL HT+SN | REV STOP SW DN SW UP
 * The engine: a mono ring of the mix (PUNCH_N samples, 0.74 s, 16 bits linear) feeds the TIME effects (one at a time, the
 * last pressed: the loops, reverse, half speed, tape stop, the jet); the COLOUR effects (crush, the isolator, motion, vowel,
 * phaser, the two sweeps) run after them in stereo, each with its own state; the THROWS raise every voice's send to the
 * shared delay and reverb (fx.c) and ring out after; the MUTES fade voices (perf_kill) and drum slots (perf_dmute) out as a
 * mute does, their notes still playing unheard. Every change crossfades. Runs in the audio ISR (mix_block, fx.c). */
#define PUNCH_N 32768u                    /* power of two */
/* the transport clock (core.h clk_pos) places the loops on the grid */
static uint32_t clk_samples(void) { return clk_pos / (uint32_t)song.g[G_BPM]; }   /* samples into the beat */
/* the TIME effects on the ring (codes 0..3: the STUTTER lane's lengths, stl_code) */
enum { PX_LOOP4, PX_LOOP8, PX_LOOP16, PX_LOOP32, PX_LOOP8T, PX_LOOP16T, PX_LOOP4T, PX_LOOP8D, PX_LOOP16D, PX_REV, PX_HALF,
       PX_STOP, PX_FLANGE };
/* the pads: the 16 white keys (rows of four), the 11 black keys in key order, then the lane's own repeats (its black keys'
 * 1/32, triplets and dotted lengths: one pad, its length a knob) */
enum { PF_S1, PF_S2, PF_S4, PF_S8,
       PF_S16, PF_HALF, PF_ECHO, PF_VERB,
       PF_ISOL, PF_ISOM, PF_ISOH, PF_MOTION,
       PF_CRUSH, PF_PHASER, PF_JET, PF_VOWEL,
       PF_MV1, PF_MV2, PF_MV3,                            /* F#3 G#3 A#3: mute voice 1, 2, 3 */
       PF_MKICK, PF_MTOPS, PF_MSHCL, PF_MHTSN,           /* C#4 D#4 F#4 G#4: the drum mutes */
       PF_REV, PF_STOP, PF_SWDN, PF_SWUP,                /* A#4 C#5 D#5 F#5 */
       PF_LSTUT, PF_NPAD };
#define PF_NKEY 27u                       /* the pads on keys (PF_LSTUT is the lane's) */
#define PUNCH_NFX PF_NPAD
#define PFB(p) (1u << (p))
#define PF_SEQ (PFB(PF_S1) | PFB(PF_S2))                       /* the sequencer replays (punch.c rep_*) */
#define PF_TIME (PF_SEQ | PFB(PF_S4) | PFB(PF_S8) | PFB(PF_S16) | PFB(PF_HALF) | PFB(PF_REV) | PFB(PF_STOP) | PFB(PF_JET) | \
                 PFB(PF_LSTUT))                                  /* one at a time: the last pressed */
#define PF_ISO (PFB(PF_ISOL) | PFB(PF_ISOM) | PFB(PF_ISOH))
#define PF_MUTES (PFB(PF_MV1) | PFB(PF_MV2) | PFB(PF_MV3) | PFB(PF_MKICK) | PFB(PF_MTOPS) | PFB(PF_MSHCL) | PFB(PF_MHTSN))
static const int8_t PF_RING[PF_NPAD] = {-1, -1, PX_LOOP4, PX_LOOP8, PX_LOOP16, PX_HALF, -1, -1, -1, -1, -1, -1,
                                        -1, -1, PX_FLANGE, -1, -1, -1, -1, -1, -1, -1, -1, PX_REV, PX_STOP, -1, -1, -1};
/* (wave 13, Ben: "The black keys on the stutter sequencer should just do stutters / beat repeat fx") the lane's repeats past
 * 1/16: its length code (KNOB 1 of PF_LSTUT, recorded with its span) -> the loop, the label; in the lane's key order */
#define LST_N 6u
static const uint8_t LST_PX[LST_N] = {PX_LOOP4T, PX_LOOP8T, PX_LOOP16T, PX_LOOP8D, PX_LOOP16D, PX_LOOP32};
static const char *const LST_NAME[LST_N] = {"1/4T", "1/8T", "1/16T", "1/8D", "1/16D", "1/32"};
/* the drum mutes (Ben's words, 14:13) by the 16 slots (drums.c: KICK KICK2 SNARE CLAP HAT OPHAT PEDAL RIM SNARE2 LOTOM HITOM
 * CRASH RIDE SHAKER CONGA COWBELL). Snare is SNARE and SNARE 2 (RIM is not); hats HAT, OPEN HAT, PEDAL. Held together: what
 * each mutes adds up, so what is heard is what all of them allow */
#define DS_(l) (1u << (l))
#define DM_KICKS (DS_(0) | DS_(1) | DS_(9))                                           /* "Mute kicks and low tom" */
#define DM_TOPS (0xFFFFu & ~(DS_(2) | DS_(8) | DS_(3) | DS_(4) | DS_(5) | DS_(6) | DS_(13)))   /* "all drums but tops - snare,
                                                                                     * clap and hats, shaker" */
#define DM_SHCL (0xFFFFu & ~(DS_(13) | DS_(3)))                                       /* "all drums but shaker and clap" */
#define DM_HTSN (0xFFFFu & ~(DS_(4) | DS_(5) | DS_(6) | DS_(2) | DS_(8)))             /* "all drums but hats and snare" */
static const uint16_t PF_DMASK[4] = {DM_KICKS, DM_TOPS, DM_SHCL, DM_HTSN};
enum { PP_CRUSH, PP_ISO, PP_MOTION, PP_VOWEL, PP_PHASER, PP_SWDN, PP_SWUP, PP_N };   /* the COLOUR effects, in the order they run */
static const uint8_t PP_PAD[PP_N] = {PF_CRUSH, PF_ISOL, PF_MOTION, PF_VOWEL, PF_PHASER, PF_SWDN, PF_SWUP};
/* the knobs (KNOB 1..4 on the last pad pressed; perf.par): a value lo..hi, shown by its kind */
enum { KK_NONE, KK_PCT, KK_SEMI, KK_LIST, KK_CUT, KK_INT, KK_X, KK_GLO, KK_VOW };
typedef struct {
    const char *lab;
    const char *const *names;             /* KK_LIST: the names, lo..hi */
    uint8_t kind, gid;                    /* KK_GLO: the global it turns (shared with GLO's page: G_*) */
    int8_t lo, hi, def;
} pk_t;
static const char *const PL_LEN[4] = {"1/16", "1/8", "1/4", "1/2"};       /* 16ths: 1 2 4 8 */
static const char *const PL_TIME[4] = {"1/8", "1/4", "1/2", "1/1"};       /* 16ths: 2 4 8 16 */
static const char *const PL_FRATE[4] = {"1/2", "1/1", "2/1", "4/1"};      /* a sweep: 8 16 32 64 16ths */
static const char *const PL_MRATE[4] = {"1/4", "1/2", "1/1", "2/1"};      /* 16ths: 4 8 16 32 */
static const char *const PL_SWLEN[5] = {"1/4", "1/2", "1/1", "2/1", "4/1"};   /* a beat, two, a bar, two bars, four */
static const char *const PL_PRATE[5] = {"1/4", "1/2", "1/1", "2/1", "4/1"};
static const char *const PL_VRATE[5] = {"OFF", "1/4", "1/2", "1/1", "2/1"};
static const char *const PL_SLOPE[2] = {"12DB", "24DB"};
/* the isolator's crossovers (Ben: "Daft punk values of 220hz bass 1.1khz mids, high 1.1khz +": the starts) */
static const char *const PL_X1[12] = {"80", "100", "120", "150", "180", "200", "220", "250", "300", "350", "400", "500"};
static const char *const PL_X2[12] = {"600", "700", "800", "900", "1K", "1.1K", "1.2K", "1.5K", "2K", "2.5K", "3K", "4K"};
static const uint16_t PL_X1HZ[12] = {80, 100, 120, 150, 180, 200, 220, 250, 300, 350, 400, 500};
static const uint16_t PL_X2HZ[12] = {600, 700, 800, 900, 1000, 1100, 1200, 1500, 2000, 2500, 3000, 4000};
static const uint8_t PL_LEN16[4] = {1, 2, 4, 8}, PL_TIME16[4] = {2, 4, 8, 16}, PL_FRATE16[4] = {8, 16, 32, 64},
                     PL_MRATE16[4] = {4, 8, 16, 32}, PL_SW16[5] = {4, 8, 16, 32, 64}, PL_VRATE16[5] = {0, 4, 8, 16, 32};
#define K_NONE {"", 0, KK_NONE, 0, 0, 0, 0}
#define K_AMT {"AMOUNT", 0, KK_PCT, 0, 0, 100, 100}
#define K_STUT {"PITCH", 0, KK_SEMI, 0, -12, 12, 0}, {"DECAY", 0, KK_PCT, 0, 0, 100, 0}, {"GATE", 0, KK_PCT, 0, 10, 100, 100}, K_AMT
#define K_ISO {"LOW X", PL_X1, KK_LIST, 0, 0, 11, 6}, {"HIGH X", PL_X2, KK_LIST, 0, 0, 11, 5}, K_NONE, K_AMT
#define K_MUTE {K_NONE, K_NONE, K_NONE, K_NONE}
static const struct {
    const char *pad, *name;               /* the pad's label (seven letters at most; the mutes' five), the caption */
    pk_t k[4];
} PFD[PF_NPAD] = {
    {"1/1", "1/1 REPLAY", {K_NONE, K_NONE, K_NONE, K_NONE}},
    {"1/2", "1/2 REPLAY", {K_NONE, K_NONE, K_NONE, K_NONE}},
    {"1/4", "1/4 REPEAT", {K_STUT}},
    {"1/8", "1/8 REPEAT", {K_STUT}},
    {"1/16", "1/16 REPEAT", {K_STUT}},
    {"HALF", "HALF TIME", {{"LENGTH", PL_LEN, KK_LIST, 0, 0, 3, 2}, K_NONE, K_NONE, K_AMT}},
    /* (wave 8) the throws' knobs are their own: the delay's TIME and the reverb's SIZE and DAMP are the FX button's pages */
    {"ECHO", "ECHO THROW", {K_NONE, {"FDBK", 0, KK_PCT, 0, 0, 95, 70}, K_NONE, K_AMT}},
    {"VERB", "REVERB THROW", {K_NONE, K_NONE, K_NONE, K_AMT}},
    {"ISO LO", "ISOLATE LOW", {K_ISO}},
    {"ISO MID", "ISOLATE MID", {K_ISO}},
    {"ISO HI", "ISOLATE HIGH", {K_ISO}},
    {"MOTION", "MOTION FILTER", {{"RATE", PL_MRATE, KK_LIST, 0, 0, 3, 2}, {"DEPTH", 0, KK_PCT, 0, 0, 100, 70},
                                 {"RESO", 0, KK_PCT, 0, 0, 100, 50}, K_AMT}},
    {"CRUSH", "BIT CRUSH", {{"BITS", 0, KK_INT, 0, 2, 12, 9}, {"RATE", 0, KK_X, 0, 1, 16, 2}, {"THIN", 0, KK_PCT, 0, 0, 100, 20},
                            K_AMT}},
    {"PHASER", "PHASER", {{"RATE", PL_PRATE, KK_LIST, 0, 0, 4, 2}, {"DEPTH", 0, KK_PCT, 0, 0, 100, 70},
                          {"FDBK", 0, KK_PCT, 0, 0, 90, 50}, K_AMT}},
    {"JET", "JET FLANGER", {{"RATE", PL_FRATE, KK_LIST, 0, 0, 3, 2}, {"FDBK", 0, KK_PCT, 0, 0, 90, 80}, K_NONE, K_AMT}},
    {"VOWEL", "VOWEL FILTER", {{"VOWEL", 0, KK_VOW, 0, 0, 40, 20}, {"RATE", PL_VRATE, KK_LIST, 0, 0, 4, 0},
                               {"RESO", 0, KK_PCT, 0, 0, 100, 60}, K_AMT}},
    {"V1", "MUTE VOICE 1", K_MUTE},
    {"V2", "MUTE VOICE 2", K_MUTE},
    {"V3", "MUTE VOICE 3", K_MUTE},
    {"KICKS", "MUTE KICKS + LO TOM", K_MUTE},
    {"TOPS", "MUTE ALL BUT TOPS", K_MUTE},
    {"SH+CL", "ALL BUT SHAKER+CLAP", K_MUTE},
    {"HT+SN", "ALL BUT HATS+SNARE", K_MUTE},
    {"REVERSE", "REVERSE", {{"LENGTH", PL_LEN, KK_LIST, 0, 0, 3, 2}, K_NONE, K_NONE, K_AMT}},
    {"STOP", "TAPE STOP", {{"TIME", PL_TIME, KK_LIST, 0, 0, 3, 1}, K_NONE, K_NONE, K_AMT}},
    {"SWP DN", "SWEEP DOWN", {{"LENGTH", PL_SWLEN, KK_LIST, 0, 0, 4, 2}, {"RESO", 0, KK_PCT, 0, 0, 100, 30},
                             {"RANGE", 0, KK_CUT, 0, 0, 100, 30}, {"SLOPE", PL_SLOPE, KK_LIST, 0, 0, 1, 1}}},
    {"SWP UP", "SWEEP UP", {{"LENGTH", PL_SWLEN, KK_LIST, 0, 0, 4, 2}, {"RESO", 0, KK_PCT, 0, 0, 100, 30},
                           {"RANGE", 0, KK_CUT, 0, 40, 127, 95}, {"SLOPE", PL_SLOPE, KK_LIST, 0, 0, 1, 1}}},
    {"1/32", "LANE REPEAT", {{"LENGTH", LST_NAME, KK_LIST, 0, 0, 5, 5}, K_NONE, K_NONE, K_NONE}},
};
#undef K_STUT
#undef K_ISO
#undef K_MUTE
/* the ring keeps the mono mix >> PUNCH_SH, 16 bits linear (0 = silence): PUNCH_SH bits of headroom over the 16-bit
 * scale for a hot mix (clamped beyond; Ben, after wave 1: "Keep 12 dB"); reads give it back at the mix's scale */
#define PUNCH_SH 2
static int16_t punch_ring[PUNCH_N] __attribute__((section(".pool")));
#define PUNCH_PUT(i, v) (punch_ring[(i) & (PUNCH_N - 1u)] = (int16_t)(v))
/* (wave 13) the COLOUR effects' states, float (the AC79's FPU, as the voice filter): two channels each */
typedef struct {
    float iso[2][7][2];           /* the isolator: seven SVFs a side (Linkwitz-Riley 24 dB/oct at both crossovers) */
    float ig[3];                  /* its bands' gains (0..1) */
    float mo[2][2], vo[2][2][2], ph[2][4], pfb[2];   /* motion's SVF, vowel's two formants, the phaser's four stages and
                                                      * its feedback */
    float sw[2][2][2][2];         /* the sweeps (down, up): two SVFs a side */
    float swp[2];                 /* .. where each is in its LENGTH, 0..1 */
} pxs_t;
static pxs_t pxs;
static struct {
    int8_t cur;                   /* TIME effect playing (fading out when the one asked for differs), -1 none */
    uint8_t pad;                  /* .. the pad it is (0xFF: the STUTTER lane's step: the knobs' defaults) */
    int32_t g;                    /* wet gain Q15 */
    uint32_t w;                   /* ring write index */
    uint32_t start, t, len;       /* loop / reverse: start index, samples since, length */
    uint32_t pre;                 /* loops: samples of dry still to go to the next division */
    uint32_t k, sub;              /* position in the loop; HALF: odd / even sample */
    uint32_t dspd;                /* tape stop: speed step per sample */
    uint32_t rp;                  /* tape stop: read position Q16 */
    uint32_t spd;                 /* tape stop speed Q16 */
    uint32_t lfo;                 /* jet phase */
    uint32_t pq, pspd;            /* loops: read position in the loop Q16 and its speed (PITCH; 65536: as recorded) */
    int32_t rg, dec;              /* loops: this repeat's gain and the gain from one repeat to the next (DECAY), Q15 */
    uint32_t gl;                  /* loops: the part of each repeat that sounds (GATE; len: all) */
    int32_t held_l, held_r, hn;   /* crush: the sample held, how long more */
    int32_t thl, thr;             /* crush: THIN's low-pass a side */
    int32_t pg[PP_N];             /* the colour effects' wet gains, Q15 */
    uint8_t iso_t;                /* the isolator's bands asked for (kept as they were while it fades out) */
    uint32_t pfree;               /* the colour LFOs' own clock when stopped (samples) */
} punch = {.cur = -1, .pad = 0xFF, .pspd = 65536, .rg = 32767, .dec = 32767};

/* (PHEBUS, 21 Nov 2026, Ben: "a stutter sequencer ... where you could set the length of the stutter pattern and then
 * turn stutter on by step"; picker: length "Per step", repeats "The whole mix") The STUTTER lane (ui_stut.c draws and
 * edits it): steps of 1/16 on the transport's grid, its own length, each step off or on with a length 1/4 1/8 1/16 1/32
 * (code 0..3 = PX_LOOP4 .. PX_LOOP32). A lit step under the playhead asks for that loop (stl_want, set a block at a
 * time by seq.c events_block: the block its drum hits start in); punch_process plays it as the punch key would, but
 * on the span played just before the run of lit steps began (punch_seq_align). Steps in a row with the same length are
 * one loop. A pad takes over while it plays. Saved with the project (project.c stl_put / stl_get).
 * (wave 6, Ben: "Keep, and record played FX into it") ev: the pads played while the sequencer recorded, each a span of
 * the lane's steps that plays the pad again (format 8's 124 bytes beside the lane: 31 of them) */
#define PEV_N 31u
typedef struct {
    uint8_t fx;                   /* the pad + 1 (0: empty) | the knob turned while it played << 5 | 0x80: kv holds it */
    uint8_t st, len;              /* its first step (0..63) and how many (1..64) */
    int8_t kv;                    /* that knob's value */
} pev_t;
static struct {
    uint8_t on[8];                /* bit per step */
    uint8_t lc[16];               /* 2 bits a step: the length code */
    uint8_t len;                  /* 1..64 steps */
    uint8_t act;                  /* the lane on (else it keeps its steps and plays nothing, the recorded pads neither) */
    uint8_t lx[8];                /* (PHEBUS, 21 Nov 2026) bit per step: + 4 to its code: 4 = 1/2, 5 = 1/1 (lc 0 / 1) */
    pev_t ev[PEV_N];              /* (wave 6) the pads recorded */
} stl = {.len = 16, .act = 1};
_Static_assert(sizeof(pev_t) * PEV_N == 124u, "the recorded pads fill format 8's room beside the lane");
_Static_assert(PF_NPAD < 31, "a pad + 1 fits pev_t.fx's five bits");
/* (wave 13) the pads' numbering in a saved lane (project.c perf.rsv[0]): 0 = waves 6 to 12's 22 pads, PEV_V13 = these */
#define PEV_V13 13u
static volatile uint8_t pev_drop_n;     /* old spans a load dropped (no pad now): the UI says how many (ui_input.c) */
/* (PHEBUS, Ben 21 Nov 2026: "1/2 would even be nice to play with ... we may as well go the full 1/1"; picker "Replay the
 * steps": "1/2 and 1/1 loop the sequencer's last steps; shorter ones stay as audio stutter") A run of 1/2 or 1/1 steps is
 * too long for the 0.74 s ring: the SEQUENCER replays instead. From the run's first 1/16 (rep_g0, counted from PLAY) every
 * track and the drums play the steps of the half bar / bar just before it, in a loop, notes and locks as they were, for
 * as long as the run lasts (seq.c seq_src); the song position counts on underneath, so afterwards each track plays the
 * step it would have played. rep_l16: the span in 1/16s (8, 16), 0 = none. Set a block at a time (stl_seq). The 1/1 and
 * 1/2 pads do the same (wave 6) */
static uint32_t rep_g0;
static uint8_t rep_l16;
static volatile int8_t stl_want = -1;   /* the loop the lane asks for now, -1 none (audio ISR) */
static int8_t stl_was = -1;             /* .. in the block before (punch_process: a run of lit steps starts) */
static uint32_t stl_t;                  /* samples since that run began */
static uint8_t stl_pos;                 /* the lane's step under the playhead (the screen) */
static uint8_t stl_view;                /* the GRID shows the lane (ui_stut.c; ui.c track_select: off) */
static int stl_lit(uint32_t s) { return (stl.on[(s >> 3) & 7u] >> (s & 7u)) & 1u; }
static uint32_t stl_code(uint32_t s)    /* 0..3: 1/4 1/8 1/16 1/32 (the ring's loops); 4: 1/2, 5: 1/1 (the sequencer's) */
{
    return ((uint32_t)stl.lc[(s >> 2) & 15u] >> (2u * (s & 3u)) & 3u) | ((uint32_t)stl.lx[(s >> 3) & 7u] >> (s & 7u) & 1u) << 2;
}

/* ---- the pads (wave 6) ---- */
static struct {
    volatile uint32_t held, lat;  /* the pads held on the keys (keyboard ISR), latched (OCT+) */
    uint32_t lane, act;           /* the recorded pads playing now, everything playing (perf_block) */
    uint32_t rprev, ropen;        /* recording: the pads played in the block before, those with a span open */
    volatile uint32_t rsplit;     /* (wave 13) a held pad that became another (the lane's repeat, another length): its span
                                   * ends and a new one starts */
    uint16_t stamp[PF_NPAD], ctr;    /* the order they started in (the last TIME pad wins) */
    uint16_t rg0[PF_NPAD];           /* recording: the 16th its span started (low bits) .. */
    uint8_t rst[PF_NPAD];            /* .. and its step on the lane */
    int8_t par[PF_NPAD][4];          /* the knobs' values */
    uint8_t kn[PF_NPAD];             /* the knob turned while it played (k + 1, 0 none), for the recording .. */
    int8_t knv[PF_NPAD];             /* .. and its value */
    uint8_t ovk[PF_NPAD];            /* the lane plays it: its recorded knob (k + 1, 0 none) .. */
    int8_t ovv[PF_NPAD];             /* .. and value */
    uint8_t last;                 /* the pad the knobs turn (the last pressed that has knobs: not a mute) */
    uint8_t cap;                  /* (wave 13) the last pad pressed (the caption names it) */
    int8_t ring;                  /* the TIME effect on the ring now (PX_*), -1 none, -2 a replay (the ring stays dry) */
    uint8_t seq16;                /* a 1/2 or 1/1 pad: the replay's span in 1/16s */
    volatile uint8_t full;        /* a recorded pad found the lane full (the UI says so once) */
    uint8_t rmark;                /* a recording pass began: its first span marks the lane's undo */
    volatile uint8_t lkeys;       /* (wave 13) the lane's black keys held that play PF_LSTUT (a bit a length code) */
    uint8_t lso;                  /* .. recording: the length its open span plays */
    int32_t thr[2];               /* the throws: the reverb's and the delay's send asked for, Q15 */
    int32_t fb;                   /* ECHO: the delay's feedback at least (fx.c), G_DFDBK * 230 scale */
} perf = {.last = PF_S4, .cap = PF_S4, .ring = -1};
static void perf_undo_mark(void);       /* seq.c: the lane before a recording pass writes into it */
static void perf_init(void)             /* the knobs at their start (power-on) */
{
    uint32_t p, k;
    for (p = 0; p < PF_NPAD; p++)
        for (k = 0; k < 4u; k++)
            perf.par[p][k] = PFD[p].k[k].def;
}
static int32_t pf_val(uint32_t p, uint32_t k)   /* a knob's value now: the recorded one while the lane plays the pad */
{
    if ((perf.lane >> p) & 1u && !(((perf.held | perf.lat) >> p) & 1u) && perf.ovk[p] == k + 1u)
        return perf.ovv[p];
    return perf.par[p][k];
}
static int32_t pf_amt(uint32_t p)       /* AMOUNT, Q15 (100 %: 32767, as the old punch-in's full wet) */
{
    int32_t a = pf_val(p, 3);
    return a >= 100 ? 32767 : a * 32767 / 100;
}
/* a pad pressed (seq.c key_down, keyboard ISR). OCT+ held: latch it (a latched one: off). A latched pad pressed: off
 * (the MPC's) */
static void perf_press(uint32_t p, uint32_t latch)
{
    uint32_t b = PFB(p);
    if (p >= PF_NPAD)
        return;
    if (p != PF_LSTUT && !((PF_MUTES >> p) & 1u))   /* (a mute and the lane's repeats have no knobs here: the knobs stay) */
        perf.last = (uint8_t)p;
    if (p != PF_LSTUT)
        perf.cap = (uint8_t)p;
    perf.kn[p] = 0;
    if (latch || (perf.lat & b)) {
        perf.lat ^= b;
        perf.held &= ~b;
        return;
    }
    perf.held |= b;
}
static void perf_release(uint32_t p)
{
    if (p < PF_NPAD)
        perf.held &= ~PFB(p);
}
/* (wave 13) the lane's black keys (seq.c key_down with the STUTTER lane shown): 0..4 the 1/1 .. 1/16 pads, 5..10 PF_LSTUT at
 * its lengths (LST_*); the last pressed plays, and let go it falls back to another still held */
static uint32_t lane_pad_of(uint32_t bi) { static const uint8_t P[5] = {PF_S1, PF_S2, PF_S4, PF_S8, PF_S16}; return bi < 5u ? P[bi] : PF_LSTUT; }
static void perf_lane_press(uint32_t bi, uint32_t latch)
{
    uint32_t c = bi - 5u;
    if (bi < 5u) {
        perf_press(lane_pad_of(bi), latch);
        return;
    }
    if (c >= LST_N)
        return;
    if (((perf.held | perf.lat) >> PF_LSTUT) & 1u && perf.par[PF_LSTUT][0] != (int8_t)c && !latch) {
        perf.par[PF_LSTUT][0] = (int8_t)c;  /* (another length while one plays: it takes over, a new span) */
        perf.knv[PF_LSTUT] = (int8_t)c;
        perf.kn[PF_LSTUT] = 1;
        perf.lkeys |= (uint8_t)(1u << c);
        perf.held |= PFB(PF_LSTUT);
        perf.rsplit |= PFB(PF_LSTUT);
        return;
    }
    perf.par[PF_LSTUT][0] = (int8_t)c;
    perf_press(PF_LSTUT, latch);
    perf.knv[PF_LSTUT] = (int8_t)c;
    perf.kn[PF_LSTUT] = 1;
    if (perf.held & PFB(PF_LSTUT))
        perf.lkeys |= (uint8_t)(1u << c);
}
static void perf_lane_release(uint32_t bi)
{
    uint32_t c = bi - 5u, i;
    if (bi < 5u) {
        perf_release(lane_pad_of(bi));
        return;
    }
    if (c >= LST_N)
        return;
    perf.lkeys &= (uint8_t)~(1u << c);
    if (!perf.lkeys) {
        perf_release(PF_LSTUT);
        return;
    }
    if (perf.par[PF_LSTUT][0] == (int8_t)c) {         /* (the one playing let go, another still held: that one) */
        for (i = 0; i < LST_N && !((perf.lkeys >> i) & 1u); i++)
            ;
        perf.par[PF_LSTUT][0] = (int8_t)i;
        perf.knv[PF_LSTUT] = (int8_t)i;
        perf.rsplit |= PFB(PF_LSTUT);
    }
}
static void perf_latch_held(void) { perf.lat |= perf.held; }   /* OCT+ with pads held (UI) */
static void perf_unlatch(void) { perf.lat = 0; }                /* OCT-: every latch off (UI); STOP */
static uint32_t pf_count(uint32_t m)
{
    uint32_t n = 0;
    for (; m; m &= m - 1u)
        n++;
    return n;
}
/* a span recorded: the pad's spans it overlaps go, then it takes a free place (none: perf.full) */
static void perf_rec_put(uint32_t p, uint32_t st, uint32_t len, uint32_t L)
{
    uint32_t i, fr = PEV_N, kn = perf.kn[p];
    int8_t kv = perf.knv[p];
    if (p == PF_LSTUT)                              /* (wave 13: the lane's repeat keeps the length it played) */
        kn = 1, kv = (int8_t)perf.lso;
    if (perf.rmark) {                               /* (the first span of this take: the lane before it, for EDIT + OCT-) */
        perf.rmark = 0;
        perf_undo_mark();
    }
    for (i = 0; i < PEV_N; i++) {
        pev_t *e = &stl.ev[i];
        if (!e->fx) {
            if (fr == PEV_N)
                fr = i;
            continue;
        }
        if ((e->fx & 31u) == p + 1u && e->st < L && (p != PF_LSTUT || e->kv == kv) &&
            ((e->st + L - st) % L < len || (st + L - e->st) % L < (e->len < L ? e->len : L))) {
            e->fx = 0;                              /* (bytes: the ISR's own; the UI edits with it off) */
            if (fr == PEV_N || i < fr)
                fr = i;
        }
    }
    if (fr == PEV_N) {
        perf.full = 1;
        return;
    }
    stl.ev[fr].st = (uint8_t)st;
    stl.ev[fr].len = (uint8_t)len;
    stl.ev[fr].kv = kv;
    stl.ev[fr].fx = (uint8_t)((p + 1u) | (kn ? ((kn - 1u) << 5 | 0x80u) : 0u));
}
static void perf_rec_drop(void) { perf.rprev = perf.ropen = 0; }   /* a pattern or a project loaded: the spans open go */
/* once a block, before the tracks (seq.c events_block, stl_seq): what plays, recorded, chosen */
static void perf_block(void)
{
    uint32_t L = stl.len ? stl.len : 16u, q = BEAT_U / 4u, g16 = clk_beat * 4u + clk_pos / q, s = g16 % L;
    uint32_t live = perf.held | perf.lat, lane = 0, act, rise, i, best = PF_NPAD, dm = 0;
    uint32_t qn = g16 + (clk_pos % q >= q / 2u);   /* (the nearest 16th: a span's ends are quantised) */
    uint16_t age = 0xFFFF;
    if (song.playing && stl.act)                    /* the lane's recorded pads at this step */
        for (i = 0; i < PEV_N; i++) {
            const pev_t *e = &stl.ev[i];
            uint32_t p = (e->fx & 31u) - 1u, ln = e->len < L ? e->len : L;
            if (!e->fx || p >= PF_NPAD || e->st >= L || (s + L - e->st) % L >= ln)
                continue;
            lane |= PFB(p);
            perf.ovk[p] = e->fx & 0x80u ? (uint8_t)(((e->fx >> 5) & 3u) + 1u) : 0u;
            perf.ovv[p] = e->kv;
        }
    {   /* recording (the sequencer recording, playing): a pad's span from its press (or latch) to its release */
        static uint8_t rec_was;
        uint32_t rec = song.rec && song.playing, cur = rec ? live : 0u, split = perf.rsplit & cur & perf.ropen;
        uint32_t up = (cur & ~perf.rprev) | split, dn = (perf.ropen & ~cur) | split;
        perf.rsplit = 0;
        if (rec && !rec_was)
            perf.rmark = 1;
        rec_was = (uint8_t)rec;
        for (i = 0; dn >> i; i++)
            if ((dn >> i) & 1u) {
                uint32_t len = (uint16_t)((uint16_t)qn - perf.rg0[i]);
                if (!len && ((split >> i) & 1u))
                    continue;                       /* (another length within the same 16th: the new one only) */
                perf_rec_put(i, perf.rst[i] % L, len < 1u ? 1u : len > L ? L : len, L);
            }
        perf.ropen &= ~dn;
        for (i = 0; up >> i; i++)
            if ((up >> i) & 1u) {
                perf.rg0[i] = (uint16_t)qn;
                perf.rst[i] = (uint8_t)(qn % L);
            }
        if ((up >> PF_LSTUT) & 1u)
            perf.lso = (uint8_t)perf.par[PF_LSTUT][0];
        perf.ropen |= up;
        perf.rprev = cur;
    }
    act = live | lane;
    rise = act & ~perf.act;
    for (i = 0; rise >> i; i++)
        if ((rise >> i) & 1u)
            perf.stamp[i] = ++perf.ctr;
    perf.act = act;
    perf.lane = lane;
    for (i = 0; i < PF_NPAD; i++)                      /* the TIME pad: the last started */
        if ((act & PF_TIME) >> i & 1u && (uint16_t)(perf.ctr - perf.stamp[i]) < age) {
            age = (uint16_t)(perf.ctr - perf.stamp[i]);
            best = i;
        }
    perf.seq16 = 0;
    perf.ring = -1;
    if (best < PF_NPAD) {
        perf.ring = best == PF_LSTUT ? (int8_t)LST_PX[clamp(pf_val(PF_LSTUT, 0), 0, (int32_t)LST_N - 1)]
                    : PF_RING[best] >= 0 ? PF_RING[best] : -2;
        if (best == PF_S1 || best == PF_S2)
            perf.seq16 = best == PF_S1 ? 16u : 8u;
        punch.pad = (uint8_t)best;
    }
    /* (wave 13) the mutes: the voices' (as wave 6's kills: faded, playing on unheard) and the drum slots' */
    perf_kill = (uint8_t)((act >> PF_MV1) & 7u);
    for (i = 0; i < 4u; i++)
        if ((act >> (PF_MKICK + i)) & 1u)
            dm |= PF_DMASK[i];
    perf_dmute = (uint16_t)dm;
    perf.thr[0] = (act >> PF_VERB) & 1u ? pf_amt(PF_VERB) : 0;
    perf.thr[1] = (act >> PF_ECHO) & 1u ? pf_amt(PF_ECHO) : 0;
    perf.fb = pf_val(PF_ECHO, 1) * 127 / 100 * 230;
}
static void stl_tick(void)              /* seq.c events_block, before the clock moves on: the step this block is in */
{
    int32_t w = -1;
    uint32_t s = (clk_beat * 4u + clk_pos / (BEAT_U / 4u)) % (stl.len ? stl.len : 16u);
    stl_pos = (uint8_t)s;
    if (song.playing && stl.act && stl_lit(s) && stl_code(s) < 4u)
        w = (int32_t)stl_code(s);       /* (1/2, 1/1: the sequencer's, stl_seq) */
    stl_want = (int8_t)w;
}
static void stl_seq(void)               /* seq.c events_block, before the tracks' steps: a 1/2 or 1/1 run (rep_*) */
{
    uint32_t g, s, c, l16;
    perf_block();                       /* (wave 6: the pads first; a 1/1 or 1/2 pad comes before the lane) */
    g = clk_beat * 4u + clk_pos / (BEAT_U / 4u), s = g % (stl.len ? stl.len : 16u), c = stl_code(s);
    l16 = song.playing && stl.act && stl_lit(s) && c >= 4u ? (c == 4u ? 8u : 16u) : 0u;
    if (perf.ring != -1)                /* a pad's TIME effect plays: the lane's steps wait */
        l16 = 0;
    if (perf.seq16 && song.playing)
        l16 = perf.seq16;
    if (l16 != rep_l16)                 /* a run begins here (lit steps in a row with the same length: one run) */
        rep_g0 = g;
    rep_l16 = (uint8_t)l16;
}

static int px_loop(int32_t fx) { return fx <= PX_LOOP16D; }   /* on the grid */
static int px_replay(int32_t fx) { return fx <= PX_HALF; }    /* replays a span of the ring */

static uint32_t beat_samples(void) { return (uint32_t)FS * 60u / (uint32_t)song.g[G_BPM]; }

static int32_t ring_raw(uint32_t i) { return punch_ring[i & (PUNCH_N - 1u)]; }   /* the sample kept (mix >> PUNCH_SH) */
static int32_t ring_at(uint32_t i) { return ring_raw(i) << PUNCH_SH; }
static int32_t ring_q16(uint32_t p)                 /* read at Q16 position, linear (on the stored values: no overflow) */
{
    int32_t a = ring_raw(p >> 16), b = ring_raw((p >> 16) + 1u);
    return (a + (((b - a) * (int32_t)((p >> 1) & 0x7FFFu)) >> 15)) << PUNCH_SH;
}
static int32_t ring_raw_q16(uint32_t p)             /* ring_q16 at the stored scale */
{
    int32_t a = ring_raw(p >> 16), b = ring_raw((p >> 16) + 1u);
    return a + (((b - a) * (int32_t)((p >> 1) & 0x7FFFu)) >> 15);
}

/* the length of a TIME effect's span, samples */
static uint32_t punch_len(int32_t fx, uint32_t beat)
{
    /* the loops: beat x NUM / DEN (1/4 .. 1/32, 1/8T, 1/16T; wave 13: 1/4T, 1/8D, 1/16D) */
    static const uint8_t NUM[9] = {1, 1, 1, 1, 1, 1, 2, 3, 3}, DEN[9] = {1, 2, 4, 8, 3, 6, 3, 4, 8};
    uint32_t p = punch.pad, len;
    if (fx <= PX_LOOP16D)
        len = beat * NUM[fx] / DEN[fx];
    else if (fx == PX_REV)                              /* REVERSE: LENGTH (1/4: a beat, as the old one) */
        len = p < PF_NPAD ? beat * PL_LEN16[clamp(pf_val(p, 0), 0, 3)] / 4u : beat;
    else if (fx == PX_HALF)                             /* HALF: LENGTH heard, half of it played (1/4: beat / 2) */
        len = p < PF_NPAD ? beat * PL_LEN16[clamp(pf_val(p, 0), 0, 3)] / 8u : beat / 2u;
    else
        len = beat;
    if (len > PUNCH_N - 1024u)
        len = PUNCH_N - 1024u;
    if (len < 64u)
        len = 64u;
    return len;
}
/* the loops' PITCH, DECAY and GATE (a pad's; the STUTTER lane's steps and its repeats past 1/16: none), a block at a time */
static void punch_loop_knobs(void)
{
    uint32_t p = punch.pad;
    if (p >= PF_NPAD || PF_RING[p] < 0 || PF_RING[p] > PX_LOOP16D) {
        punch.pspd = 65536u, punch.dec = 32767, punch.gl = punch.len;
        return;
    }
    punch.pspd = (uint32_t)pr_ratio(pf_val(p, 0) << 8);
    punch.dec = (100 - pf_val(p, 1)) * 32767 / 100;
    punch.gl = pf_val(p, 2) >= 100 ? punch.len : punch.len * (uint32_t)pf_val(p, 2) / 100u;
}

/* (wave 7) where a loop's read starts: (k * pspd) mod (len << 16), in 32 bits (the device has no 64-bit divide or modulo:
 * __umoddi3). len <= PUNCH_N - 1024 (< 2^15), k < len, pspd < 2^18 (PITCH +12: 2^17). pspd = ph 2^16 + pl: the ph part
 * is ((k ph) mod len) 2^16, exact; k pl < 2^31; their sum < 2^32. Equal to the 64-bit form over every k, len, pspd
 * (tests/punch_test.c punch_pq0 against it) */
static uint32_t punch_pq0(uint32_t k, uint32_t pspd, uint32_t len)
{
    uint32_t m = len << 16, hi = ((k * (pspd >> 16)) % len) << 16, lo = k * (pspd & 0xFFFFu);
    return (hi + lo) % m;
}
static void punch_start(int32_t fx)
{
    uint32_t beat = beat_samples(), len, ph = clk_samples();
    punch.cur = (int8_t)fx;
    punch.t = 0;
    len = punch_len(fx, beat);
    punch.len = len;
    punch.pre = 0;
    punch.start = punch.w;
    if (px_loop(fx) && song.playing) {                /* on the grid: */
        uint32_t back;
        if (fx >= PX_LOOP4T)                            /* (wave 13: a length that is not a beat's share: on the bar's grid) */
            ph += (clk_beat & 3u) * beat;
        back = ph % len;
        if (len - back <= 1536u) {                      /* just before a division: from that one */
            punch.pre = len - back;
        } else {                                        /* else from the last division start */
            punch.start = punch.w - back;
            punch.t = back;
        }
    }
    punch.rp = punch.w << 16;
    punch.spd = 65536u;
    punch.dspd = 65536u / len + 1u;
    if (fx == PX_STOP && punch.pad < PF_NPAD)             /* TAPE STOP: TIME (1/4: to a stop in a beat, as the old one) */
        punch.dspd = 65536u / (beat * PL_TIME16[clamp(pf_val(punch.pad, 0), 0, 3)] / 4u) + 1u;
    punch.sub = 0;
    punch.k = punch.t % len;
    punch.lfo = 0;
    punch.rg = 32767;
    punch_loop_knobs();
    punch.pq = punch_pq0(punch.k, punch.pspd, len);
    if (fx == PX_FLANGE)                                /* JET: the sweep starts at its shortest delay */
        punch.lfo = 0xC0000000u;
}

/* the wet sample for the mono ring effects (after the ring got this sample); no divide per
 * sample: k is the position in the loop, counted up and wrapped */
static int32_t punch_ring_fx(int32_t fx)
{
    uint32_t len = punch.len, k = punch.k;
    int32_t y;
    switch (fx) {
    case PX_REV:
        y = ring_at(punch.start - k);
        break;
    case PX_STOP:
        y = ring_q16(punch.rp);
        punch.rp += punch.spd;
        punch.spd = punch.spd > punch.dspd ? punch.spd - punch.dspd : 0u;   /* to a stop in its TIME */
        return ((y >> PUNCH_SH) * (int32_t)(punch.spd >> 4)) >> (12 - PUNCH_SH);
    default:                                        /* loops, HALF */
        if (punch.pspd != 65536u) {                 /* (PITCH: the repeat read faster or slower, from its start) */
            y = ring_q16((punch.start << 16) + punch.pq);
            punch.pq += punch.pspd;
            if ((punch.pq >> 16) >= len)
                punch.pq -= len << 16;
        } else {
            y = ring_at(punch.start + k);
        }
        break;
    }
    if (fx != PX_HALF || (punch.sub ^= 1u) == 0u) {     /* HALF: every other sample */
        if (k + 1u >= len) {
            punch.k = 0;
            punch.pq = 0;
            if (punch.dec < 32767)
                punch.rg = (punch.rg * punch.dec) >> 15;   /* DECAY: each repeat quieter */
        } else {
            punch.k = k + 1u;
        }
    }
    if (punch.rg != 32767)
        y = ((y >> 3) * (punch.rg >> 3)) >> 9;
    if (k >= punch.gl)                                  /* GATE: the rest of the repeat silent (64 samples down) */
        y = k - punch.gl >= 64u ? 0 : (y * (int32_t)(64u - (k - punch.gl))) >> 6;
    /* declick at the seams */
    return k < 64u ? (y * (int32_t)k) >> 6 : len - k < 64u ? (y * (int32_t)(len - k)) >> 6 : y;
}

/* the loops, HALF and REVERSE replay ring samples [start, start + len) (REVERSE: up to start,
 * backwards). The ring keeps recording the mix, and after PUNCH_N samples (0.74 s) the writer comes
 * round into that span: it skips those slots once the loop is captured, so a held loop plays for as
 * long as the key is held. The rest of the ring keeps recording (the next effect has its past). */
static int punch_owns(int32_t fx, uint32_t w)
{
    uint32_t d = w - punch.start, len = punch.len;
    if (fx < 0 || !px_replay(fx) || punch.pre)
        return 0;
    if (fx == PX_REV)
        return d >= 1u && ((d + len - 1u) & (PUNCH_N - 1u)) < len;
    return d >= len && (d & (PUNCH_N - 1u)) < len;
}

static int32_t px_ramp(int32_t g, int32_t target)   /* a wet gain a sample nearer its target (64 samples end to end) */
{
    return g < target ? (g + 512 > target ? target : g + 512) : g > target ? (g - 512 < target ? target : g - 512) : g;
}
#define PX_WET(X, W, G) ((X) + (((((W) - (X)) >> 3) * ((G) >> 3)) >> 9))   /* (no 32-bit overflow) */

/* ---- (wave 13) the colour effects' filters, float: the trapezoidal SVF (A. Simper; dsp.c tsvf_lp's, at float precision so
 * the isolator's bands add back up to the mix) */
#define PX_PI 3.14159265f
#define PX_IN (1.0f / 131072.0f)                          /* the mix (int, about +-2^17 at full scale) <-> +-1 */
typedef struct { float k, a1, a2, a3; } fsvf_t;
static float px_tan(float x)                              /* tan x (Pade; 0.3 % at 1.28, about 18 kHz) */
{
    x = x > 1.28f ? 1.28f : x;
    return x * (15.0f - x * x) / (15.0f - 6.0f * x * x);
}
static void fsvf_set(fsvf_t *c, float hz, float k)        /* k: the damping (1.414: Butterworth; small: resonant) */
{
    float g = px_tan(PX_PI * hz * (1.0f / (float)FS));
    c->k = k;
    c->a1 = 1.0f / (1.0f + g * (g + k));
    c->a2 = g * c->a1;
    c->a3 = g * c->a2;
}
static inline float fsvf(const fsvf_t *c, float *s, float x, float *bp)   /* -> the low-pass; *bp the band-pass */
{
    float v3 = x - s[1], v1 = c->a1 * s[0] + c->a2 * v3, v2 = s[1] + c->a2 * s[0] + c->a3 * v3;
    s[0] = 2.0f * v1 - s[0];
    s[1] = 2.0f * v2 - s[1];
    *bp = v1;
    return v2;
}
#define FSVF_HP(c, x, lp, bp) ((x) - (c)->k * (bp) - (lp))
static float px_hz(float idx) { return 30.0f * fm_exp2f(idx * (9.0589f / 127.0f)); }   /* CUTOFF_HZ's scale: 0 30 Hz .. 127 16 kHz */
#define PX_BW 1.41421356f
static int32_t px_out(float y)                            /* back to the mix's scale (bounded: a resonance never runs away) */
{
    y *= 131072.0f;
    return (int32_t)(y > 1048576.0f ? 1048576.0f : y < -1048576.0f ? -1048576.0f : y);
}
/* the colour effects' LFO phase, 0..1, over span16 16ths: on the song's grid while playing, else their own clock */
static float px_phase(uint32_t span16, uint32_t beat)
{
    uint32_t q, u;
    if (!span16)
        return 0.0f;
    if (song.playing) {
        q = BEAT_U / 4u;
        u = (clk_beat * 4u + clk_pos / q) % span16;
        return ((float)u + (float)(clk_pos % q) * (1.0f / (float)q)) / (float)span16;
    }
    u = beat / 4u * span16;
    return (float)(punch.pfree % (u ? u : 1u)) / (float)(u ? u : 1u);
}
static float px_cos01(float ph)                           /* (1 - cos 2 pi ph) / 2: 0 at the start, 1 half way */
{
    return 0.5f - 0.5f * (float)sine_i((uint32_t)(ph * 4294967040.0f) + 0x40000000u) * (1.0f / 32768.0f);
}
/* the vowels, U O A E I (the knob's order: the second formant climbing), F1 and F2 in CUTOFF_HZ's scale */
static const float VOW_F1[5] = {46.6f, 59.6f, 64.6f, 58.1f, 44.4f},    /* 300 570 730 530 270 Hz */
                   VOW_F2[5] = {68.1f, 67.4f, 72.7f, 83.3f, 87.7f};    /* 870 840 1090 1840 2290 Hz */

/* the COLOUR effects (after the TIME effect), in place */
static void punch_colour(int32_t *l, int32_t *r, uint32_t n, uint32_t beat)
{
    uint32_t i, j, on = 0, run = 0, bits = 0, hold = 1, c;
    int32_t tg[PP_N], thin = 0;
    fsvf_t ci1, ci2, cmo, cv1, cv2, csw[2][2];
    float ib[3] = {0, 0, 0}, pa[2] = {0, 0}, vk = 1.0f, swg[2] = {0, 0};
    for (j = 0; j < PP_N; j++) {
        uint32_t p = PP_PAD[j];
        tg[j] = (j == PP_ISO ? (perf.act & PF_ISO) != 0u : (perf.act >> p) & 1u) ? pf_amt(p) : 0;
        if (j >= PP_SWDN)                               /* (the sweeps: no AMOUNT, all of it) */
            tg[j] = (perf.act >> p) & 1u ? 32767 : 0;
        on |= (uint32_t)(tg[j] != 0) << j;
        if (tg[j] || punch.pg[j])
            run |= 1u << j;
    }
    if (!run)
        return;
    if (!song.playing)
        punch.pfree += n;
    if (run & 1u << PP_CRUSH) {                         /* CRUSH: BITS, RATE, THIN (a low shelf down, at about 300 Hz) */
        bits = 18u - (uint32_t)clamp(pf_val(PF_CRUSH, 0), 2, 12);
        hold = (uint32_t)clamp(pf_val(PF_CRUSH, 1), 1, 16);
        thin = clamp(pf_val(PF_CRUSH, 2), 0, 100) * 32767 / 100;
    }
    if (run & 1u << PP_ISO) {                           /* the isolator: LOW X, HIGH X; the bands held */
        if (on & 1u << PP_ISO) {
            uint32_t bt = (perf.act >> PF_ISOL) & 7u;
            if (!punch.pg[PP_ISO])                      /* (from dry: the bands at once, the wet gain fades) */
                for (c = 0; c < 3u; c++)
                    pxs.ig[c] = (float)((bt >> c) & 1u);
            punch.iso_t = (uint8_t)bt;
        }
        fsvf_set(&ci1, (float)PL_X1HZ[clamp(pf_val(PF_ISOL, 0), 0, 11)], PX_BW);
        fsvf_set(&ci2, (float)PL_X2HZ[clamp(pf_val(PF_ISOL, 1), 0, 11)], PX_BW);
        for (c = 0; c < 3u; c++)
            ib[c] = (float)((punch.iso_t >> c) & 1u);
    }
    if (run & 1u << PP_MOTION) {                        /* MOTION: a band-pass up and down once in RATE, DEPTH octaves */
        float ph = px_phase(PL_MRATE16[clamp(pf_val(PF_MOTION, 0), 0, 3)], beat);
        float oct = (float)clamp(pf_val(PF_MOTION, 1), 0, 100) * 0.06f * px_cos01(ph);
        fsvf_set(&cmo, 150.0f * fm_exp2f(oct), 2.0f - (float)clamp(pf_val(PF_MOTION, 2), 0, 100) * 0.018f);
    }
    if (run & 1u << PP_VOWEL) {                         /* VOWEL: two formants; RATE sweeps them U to I and back */
        uint32_t sp = PL_VRATE16[clamp(pf_val(PF_VOWEL, 1), 0, 4)];
        float pos = (float)clamp(pf_val(PF_VOWEL, 0), 0, 40) * 0.1f, f1, f2, fr;
        int32_t a;
        if (sp) {
            float t = px_phase(sp, beat) + pos * 0.125f;
            t -= (float)(int32_t)t;
            pos = (t < 0.5f ? t : 1.0f - t) * 8.0f;
        }
        a = (int32_t)pos;
        a = a > 3 ? 3 : a;
        fr = pos - (float)a;
        f1 = VOW_F1[a] + (VOW_F1[a + 1] - VOW_F1[a]) * fr;
        f2 = VOW_F2[a] + (VOW_F2[a + 1] - VOW_F2[a]) * fr;
        vk = 0.5f - (float)clamp(pf_val(PF_VOWEL, 2), 0, 100) * 0.0042f;
        fsvf_set(&cv1, px_hz(f1), vk);
        fsvf_set(&cv2, px_hz(f2), vk);
    }
    if (run & 1u << PP_PHASER) {                        /* PHASER: four all-passes swept over RATE, DEPTH, right a quarter on */
        uint32_t sp = PL_SW16[clamp(pf_val(PF_PHASER, 0), 0, 4)];
        float ph = px_phase(sp, beat), dep = (float)clamp(pf_val(PF_PHASER, 1), 0, 100) * 0.05f;
        for (c = 0; c < 2u; c++) {
            float t = px_tan(PX_PI * 200.0f * fm_exp2f(dep * px_cos01(ph + 0.25f * (float)c)) * (1.0f / (float)FS));
            pa[c] = (t - 1.0f) / (t + 1.0f);            /* (the all-passes' coefficient a side) */
        }
    }
    for (j = PP_SWDN; j <= PP_SWUP; j++)                /* the SWEEPS: from open (down) / the bottom (up) to RANGE in LENGTH,
                                                         * then held */
        if (run & 1u << j) {
            uint32_t p = PP_PAD[j], d = j - PP_SWDN, len = beat * PL_SW16[clamp(pf_val(p, 0), 0, 4)] / 4u;
            float to = (float)clamp(pf_val(p, 2), 0, 127), idx, k2 = 1.6f - (float)clamp(pf_val(p, 1), 0, 100) * 0.0145f;
            if ((on >> j) & 1u && pxs.swp[d] < 1.0f)
                pxs.swp[d] += (float)n / (float)(len ? len : 1u);
            if (pxs.swp[d] > 1.0f)
                pxs.swp[d] = 1.0f;
            idx = d ? to * pxs.swp[d] : 127.0f + (to - 127.0f) * pxs.swp[d];
            fsvf_set(&csw[d][0], px_hz(idx), PX_BW);
            fsvf_set(&csw[d][1], px_hz(idx), k2);       /* (the resonance on the last stage) */
            swg[d] = pf_val(p, 3) ? 1.0f : 0.0f;        /* (24 dB: both stages; 12 dB: the last only) */
        }
    for (i = 0; i < n; i++) {
        int32_t x = l[i], y = r[i];
        if (run & 1u << PP_CRUSH) {
            int32_t tx = x, ty = y;
            if (thin) {                                 /* THIN: the lows a step down (a one-pole low-pass taken away) */
                punch.thl += ((x - punch.thl) >> 5) - ((x - punch.thl) >> 8);
                punch.thr += ((y - punch.thr) >> 5) - ((y - punch.thr) >> 8);
                tx = x - (((punch.thl >> 3) * (thin >> 3)) >> 9);
                ty = y - (((punch.thr >> 3) * (thin >> 3)) >> 9);
            }
            if (--punch.hn <= 0) {                      /* RATE: one sample in RATE held */
                punch.hn = (int32_t)hold;
                punch.held_l = tx >= 0 ? tx & ~((1 << bits) - 1) : -((-tx) & ~((1 << bits) - 1));   /* towards zero */
                punch.held_r = ty >= 0 ? ty & ~((1 << bits) - 1) : -((-ty) & ~((1 << bits) - 1));
            }
            punch.pg[PP_CRUSH] = px_ramp(punch.pg[PP_CRUSH], tg[PP_CRUSH]);
            x = PX_WET(x, punch.held_l, punch.pg[PP_CRUSH]), y = PX_WET(y, punch.held_r, punch.pg[PP_CRUSH]);
        }
        if (run & ~1u) {                                /* the float ones */
            float o[2] = {(float)x * PX_IN, (float)y * PX_IN}, bp, lp, hp, w;
            if (run & 1u << PP_ISO) {                   /* LOW = LP4 at X1 (then the all-pass of X2, so the bands add up);
                                                         * MID = LP4 at X2 of HP4 at X1; HIGH = HP4 at X2 of HP4 at X1 */
                for (c = 0; c < 3u; c++) {              /* (a band in or out: 64 samples end to end) */
                    float dl = ib[c] - pxs.ig[c];
                    pxs.ig[c] += dl > 0.0156f ? 0.0156f : dl < -0.0156f ? -0.0156f : dl;
                }
                punch.pg[PP_ISO] = px_ramp(punch.pg[PP_ISO], tg[PP_ISO]);
                w = (float)punch.pg[PP_ISO] * (1.0f / 32767.0f);
                for (c = 0; c < 2u; c++) {
                    float (*s)[2] = pxs.iso[c], x0 = o[c], lpa, hpa, low, hi1, lpb, hpb, mid, high, b2;
                    lpa = fsvf(&ci1, s[0], x0, &bp);
                    hpa = FSVF_HP(&ci1, x0, lpa, bp);
                    low = fsvf(&ci1, s[1], lpa, &bp);
                    lp = fsvf(&ci1, s[2], hpa, &bp);
                    hi1 = FSVF_HP(&ci1, hpa, lp, bp);
                    lpb = fsvf(&ci2, s[3], hi1, &bp);
                    hpb = FSVF_HP(&ci2, hi1, lpb, bp);
                    mid = fsvf(&ci2, s[4], lpb, &bp);
                    lp = fsvf(&ci2, s[5], hpb, &bp);
                    high = FSVF_HP(&ci2, hpb, lp, bp);
                    (void)fsvf(&ci2, s[6], low, &b2);
                    low -= 2.0f * ci2.k * b2;           /* (the all-pass of X2: LOW in step with MID and HIGH) */
                    o[c] = x0 + (pxs.ig[0] * low + pxs.ig[1] * mid + pxs.ig[2] * high - x0) * w;
                }
            }
            if (run & 1u << PP_MOTION) {
                punch.pg[PP_MOTION] = px_ramp(punch.pg[PP_MOTION], tg[PP_MOTION]);
                w = (float)punch.pg[PP_MOTION] * (1.0f / 32767.0f);
                for (c = 0; c < 2u; c++) {
                    (void)fsvf(&cmo, pxs.mo[c], o[c], &bp);
                    o[c] += (bp * cmo.k * 1.4f - o[c]) * w;
                }
            }
            if (run & 1u << PP_VOWEL) {
                float b2;
                punch.pg[PP_VOWEL] = px_ramp(punch.pg[PP_VOWEL], tg[PP_VOWEL]);
                w = (float)punch.pg[PP_VOWEL] * (1.0f / 32767.0f);
                for (c = 0; c < 2u; c++) {
                    (void)fsvf(&cv1, pxs.vo[c][0], o[c], &bp);
                    (void)fsvf(&cv2, pxs.vo[c][1], o[c], &b2);
                    o[c] += ((bp + 0.6f * b2) * vk * 1.2f - o[c]) * w;
                }
            }
            if (run & 1u << PP_PHASER) {
                float fb = (float)clamp(pf_val(PF_PHASER, 2), 0, 90) * 0.0085f;
                punch.pg[PP_PHASER] = px_ramp(punch.pg[PP_PHASER], tg[PP_PHASER]);
                w = (float)punch.pg[PP_PHASER] * (1.0f / 32767.0f);
                for (c = 0; c < 2u; c++) {
                    float a = pa[c], v = o[c] + fb * pxs.pfb[c], q;
                    uint32_t st;
                    for (st = 0; st < 4u; st++) {       /* (a first-order all-pass, one state: y = a x + s; s = x - a y) */
                        q = a * v + pxs.ph[c][st];
                        pxs.ph[c][st] = v - a * q;
                        v = q;
                    }
                    pxs.pfb[c] = v;
                    o[c] += ((o[c] + v) * 0.5f - o[c]) * w;
                }
            }
            for (j = PP_SWDN; j <= PP_SWUP; j++)
                if (run & 1u << j) {
                    uint32_t d = j - PP_SWDN;
                    punch.pg[j] += punch.pg[j] < tg[j] ? (punch.pg[j] + 16 > tg[j] ? tg[j] - punch.pg[j] : 16)
                                                      : (punch.pg[j] - 16 < tg[j] ? tg[j] - punch.pg[j] : -16);   /* (let go: back
                                                         * in about 46 ms, a smooth return) */
                    w = (float)punch.pg[j] * (1.0f / 32767.0f);
                    for (c = 0; c < 2u; c++) {
                        float v = o[c];
                        if (swg[d] != 0.0f) {
                            lp = fsvf(&csw[d][0], pxs.sw[d][c][0], v, &bp);
                            v = d ? FSVF_HP(&csw[d][0], v, lp, bp) : lp;
                        }
                        lp = fsvf(&csw[d][1], pxs.sw[d][c][1], v, &bp);
                        hp = d ? FSVF_HP(&csw[d][1], v, lp, bp) : lp;
                        o[c] += (hp - o[c]) * w;
                    }
                }
            x = px_out(o[0]), y = px_out(o[1]);
        }
        l[i] = x, r[i] = y;
    }
    for (j = 0; j < PP_N; j++)                          /* faded out: its state cleared, the next press starts clean */
        if ((run >> j) & 1u && !tg[j] && !punch.pg[j]) {
            if (j == PP_CRUSH)
                punch.hn = 0, punch.thl = punch.thr = 0;
            else if (j == PP_ISO)
                memset(pxs.iso, 0, sizeof pxs.iso), memset(pxs.ig, 0, sizeof pxs.ig), punch.iso_t = 0;
            else if (j == PP_MOTION)
                memset(pxs.mo, 0, sizeof pxs.mo);
            else if (j == PP_VOWEL)
                memset(pxs.vo, 0, sizeof pxs.vo);
            else if (j == PP_PHASER)
                memset(pxs.ph, 0, sizeof pxs.ph), memset(pxs.pfb, 0, sizeof pxs.pfb);
            else
                memset(pxs.sw[j - PP_SWDN], 0, sizeof pxs.sw[0]), pxs.swp[j - PP_SWDN] = 0.0f;
        }
}

/* l, r: the mix before the master (fx.c mix_block), n samples, in place */
static void punch_process(int32_t *l, int32_t *r, uint32_t n)
{
    uint32_t i;
    int32_t want = perf.ring, sq = stl_want;
    uint32_t beat, binc, ftap = 9000;
    if (sq != stl_was)                                  /* (PHEBUS) the STUTTER lane: a run of lit steps begins */
        stl_t = 0;
    stl_was = (int8_t)sq;
    if (want == -1)                                     /* (a pad comes first) */
        want = sq;
    else
        sq = -1;
    if (want < -1)                                      /* (a 1/1 or 1/2 pad: the sequencer replays, the ring stays dry) */
        want = -1;
    if (sq >= 0)
        punch.pad = 0xFF;                               /* (the lane's step: its loop as the lane always played it) */
    beat = beat_samples();                              /* per block: no divide per sample */
    if (punch.cur < 0 && want < 0) {                   /* idle: only the ring */
        for (i = 0; i < n; i++)
            PUNCH_PUT(punch.w++, clamp((l[i] + r[i]) >> (1 + PUNCH_SH), -32768, 32767));
        punch_colour(l, r, n, beat);
        return;
    }
    if (punch.cur < 0 && want >= 0) {
        punch_start(want);
        if (sq >= 0) {                                  /* the lane's: the span just before its run, from the run's
                                                         * start (a block late after another loop's fade: its phase) */
            punch.pre = 0;
            punch.t = stl_t;
            punch.k = stl_t % punch.len;
            punch.start = punch.w - stl_t - punch.len;
        }
    } else if (punch.cur >= 0 && px_loop(punch.cur)) {
        punch_loop_knobs();                             /* (the knobs turned while it plays) */
    }
    stl_t += n;
    binc = 0xFFFFFFFFu / beat;                          /* a turn a beat */
    {
        int32_t amt = punch.pad < PF_NPAD ? pf_amt(punch.pad) : 32767, fx = punch.cur;
        if (punch.pad == PF_LSTUT)                      /* (the lane's repeats: no AMOUNT, all of it) */
            amt = 32767;
        if (fx == PX_FLANGE && punch.pad < PF_NPAD) {      /* JET: RATE (two bars a sweep to start), FDBK */
            binc = binc * 4u / PL_FRATE16[clamp(pf_val(punch.pad, 0), 0, 3)];
            ftap = (uint32_t)pf_val(punch.pad, 1) * 9000u / 55u;
        } else if (fx == PX_FLANGE) {
            binc >>= 2;
        }
    for (i = 0; i < n; i++) {
        int32_t x = l[i], y = r[i], wl = x, wr = y, m = (x + y) >> 1;
        uint32_t target = want == fx ? (uint32_t)amt : 0u;
        if (!punch_owns(fx, punch.w))                   /* (a held loop keeps its material) */
            PUNCH_PUT(punch.w, clamp(m >> PUNCH_SH, -32768, 32767));
        switch (fx) {
        case PX_FLANGE: {                               /* JET: 0.6..6 ms, swept over RATE, left and right a quarter apart,
                                                         * high feedback (the ring is the delay line) */
            uint32_t dl = (26u << 16) + (uint32_t)((sine_i(punch.lfo) + 32768) * 240),
                     dr = (26u << 16) + (uint32_t)((sine_i(punch.lfo + 0x40000000u) + 32768) * 240);
            int32_t el = ring_raw_q16((punch.w << 16) - dl), er = ring_raw_q16((punch.w << 16) - dr);
            punch.lfo += binc;
            PUNCH_PUT(punch.w, clamp((m >> PUNCH_SH) + (((el + er) * (int32_t)ftap) >> 15), -32768, 32767));   /* feedback */
            wl = ((x + (el << PUNCH_SH)) * 5) >> 3;
            wr = ((y + (er << PUNCH_SH)) * 5) >> 3;
            break;
        }
        default:
            if (fx < 0)
                break;
            if (punch.pre) {                            /* waiting for the division */
                if (!--punch.pre) {
                    punch.start = punch.w + 1u;
                    punch.k = 0;
                    punch.pq = 0;
                }
                break;
            }
            wl = wr = punch_ring_fx(fx);
            break;
        }
        punch.g = px_ramp(punch.g, (int32_t)target);
        l[i] = PX_WET(x, wl, punch.g);
        r[i] = PX_WET(y, wr, punch.g);
        punch.w++;
        punch.t++;
        if (punch.g == 0 && want != fx) {              /* faded out: none until the next block, which starts the
                                                         * next one with its own set-up; at most 31 samples dry */
            punch.cur = -1;
            fx = -1;
        }
    }
    }
    punch_colour(l, r, n, beat);
}

/* a key (0..26, from F3) -> its white key index 0..15, -1 = a black key (the layers' steps use it too) */
static int32_t punch_key(uint32_t k)
{
    static const int8_t W[12] = {0, -1, 1, -1, 2, -1, 3, 4, -1, 5, -1, 6};   /* from F */
    int32_t i = W[k % 12u];
    return i < 0 ? -1 : (int32_t)(k / 12u) * 7 + i;
}
/* the black keys, F#3 .. F#5 (the drum screen's RETRIG rates use them too); the pads on them are 16 + their place */
static const uint8_t PUNCH_BLACK[11] = {1, 3, 5, 8, 10, 13, 15, 17, 20, 22, 25};
/* the keyboard (seq.c keyboard_block) while FX is held: a key -> its pad (-1 none) */
static int32_t punch_fx_of_key(uint32_t k)
{
    int32_t w = punch_key(k);
    uint32_t i;
    if (w >= 0)
        return w;
    for (i = 0; i < 11u; i++)
        if (PUNCH_BLACK[i] == k)
            return 16 + (int32_t)i;
    return -1;
}
static uint32_t punch_key_of_fx(uint32_t fx)          /* a pad -> its key (the lit key) */
{
    static const uint8_t K[7] = {0, 2, 4, 6, 7, 9, 11};   /* the white keys of an octave, from F */
    if (fx < 16u)
        return fx / 7u * 12u + K[fx % 7u];
    if (fx < PF_NKEY)
        return PUNCH_BLACK[fx - 16u];
    return PUNCH_BLACK[5u + (uint32_t)clamp(perf.par[PF_LSTUT][0], 0, (int32_t)LST_N - 1)];   /* (the lane's: its key there) */
}
/* (wave 13) the lane's black key i (0..10) -> the pad it plays, and the lane's keys lit by the pads playing */
static uint32_t lane_keys_lit(void)
{
    uint32_t on = perf.held | perf.lat, m = 0, i;
    for (i = 0; i < 5u; i++)
        if ((on >> lane_pad_of(i)) & 1u)
            m |= 1u << PUNCH_BLACK[i];
    if ((on >> PF_LSTUT) & 1u)
        m |= 1u << PUNCH_BLACK[5u + (uint32_t)clamp(perf.par[PF_LSTUT][0], 0, (int32_t)LST_N - 1)];
    return m;
}
/* (wave 13) a saved lane of waves 6 to 12 (22 pads, another numbering): each span onto the pad it is now, where one is honest
 * (LP -> SWEEP DOWN, HP -> SWEEP UP, KILL DRUMS -> SH+CL (the drum mute that leaves least), KILL BASS -> the voice its knob
 * picked (V1 to start), FLNG -> JET, 1/32 8T 16T -> the lane's repeats at that length, the rest as they were); GATE, DROP and
 * LCUT have none: dropped. Returns how many were dropped */
static uint32_t pev_convert_old(pev_t *ev)
{
    static const int8_t MAP[22] = {PF_S1, PF_S2, PF_S4, PF_S8, PF_S16, PF_LSTUT, PF_REV, PF_HALF, PF_SWDN, PF_SWUP, PF_CRUSH, -1,
                                   PF_STOP, -1, PF_ECHO, PF_VERB, PF_MSHCL, PF_MV1, -1, PF_JET, PF_LSTUT, PF_LSTUT};
    uint32_t i, dropped = 0;
    for (i = 0; i < PEV_N; i++) {
        pev_t *e = &ev[i];
        uint32_t o = (e->fx & 31u) - 1u, kn = e->fx & 0x80u ? ((e->fx >> 5) & 3u) + 1u : 0u;
        int32_t p;
        if (!e->fx)
            continue;
        p = o < 22u ? MAP[o] : -1;
        if (p < 0) {
            e->fx = 0;
            dropped++;
            continue;
        }
        if (p == PF_LSTUT) {                            /* (its length: the knob) */
            e->kv = (int8_t)(o == 5u ? 5 : o == 20u ? 1 : 2);
            kn = 1;
        } else if (o == 17u) {                          /* KILL BASS: the voice its VOICE knob picked */
            if (kn == 1u)
                p = PF_MV1 + clamp(e->kv, 1, 3) - 1;
            kn = 0;
        } else if (o == 8u || o == 9u || o == 16u) {    /* (a filter's knob is not a sweep's: it goes) */
            kn = 0;
        }
        e->fx = (uint8_t)(((uint32_t)p + 1u) | (kn ? ((kn - 1u) << 5 | 0x80u) : 0u));
    }
    return dropped;
}
