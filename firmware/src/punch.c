/* SPDX-License-Identifier: GPL-3.0-only */
/* PERFORM (PHEBUS wave 6). Built on KARMA's PUNCH-IN FX (pocket-operator style: hold FX, press a key and the whole mix
 * goes through that effect while the key is held; release it and the mix comes back), now the MPC's way (Ben, 21 Nov
 * 2026: "Then I want the MPC's feel for pad like fx play and stuttering"; picker "A: pad map, trimmed set": "about 18
 * effects: straight stutters 1/1 to 1/32, reverse, half, tape stop, sweeps, gate, crush, echo, kills, throws; latch;
 * knobs tweak the last effect pressed"). The keys are pads (PF_*): momentary while held, latched with OCT+, several at
 * once (one time effect, every colour effect, the throws and the kills together), four knobs on the last pad pressed.
 * Played pads record into the STUTTER lane while the sequencer records (Ben: "Keep, and record played FX into it"):
 * stl.ev below, saved with each pattern (project.c, format 8's 124 bytes).
 * The engine: a mono ring of the mix (PUNCH_N samples, 0.74 s, 16 bits linear: PHEBUS wave 1, the CLOUD that shared its
 * words and made it 8 bits mu-law is cut) feeds the TIME effects (one at a time, the last pressed: the loops, reverse,
 * half speed, tape stop, the pitch drop, the flanger); the COLOUR effects (low-pass and high-pass sweeps, the low cut,
 * crush, gate) run after them in stereo, each with its own state; the THROWS raise every voice's send to the shared
 * delay and reverb (fx.c, KARMA 2.6's REV / ECHO THROW) and ring out after; the KILLS fade voices out as a mute does.
 * Every change crossfades over 64 samples. Runs in the audio ISR (mix_block, fx.c). */
#define PUNCH_N 32768u                    /* power of two */
/* the transport clock (core.h clk_pos) places the loops and the gate on the grid */
static uint32_t clk_samples(void) { return clk_pos / (uint32_t)song.g[G_BPM]; }   /* samples into the beat */
/* the TIME effects on the ring (codes 0..3: the STUTTER lane's lengths, stl_code) */
enum { PX_LOOP4, PX_LOOP8, PX_LOOP16, PX_LOOP32, PX_LOOP8T, PX_LOOP16T, PX_REV, PX_HALF, PX_STOP, PX_DROP, PX_FLANGE };
/* the pads, white keys 1..16 then the black keys that have one (punch_fx_of_key) */
enum { PF_S1, PF_S2, PF_S4, PF_S8, PF_S16, PF_S32, PF_REV, PF_HALF,      /* white keys 1..8: the stutters, REVRS, HALF */
       PF_LP, PF_HP, PF_CRUSH, PF_GATE, PF_STOP, PF_DROP, PF_ECHO, PF_VERB,   /* 9..16 */
       PF_KDRUM, PF_KBASS, PF_LCUT, PF_FLNG, PF_S8T, PF_S16T, PF_NPAD };   /* black keys F#3 G#3 A#3 C#4, C#5 D#5 */
#define PUNCH_NFX PF_NPAD
#define PFB(p) (1u << (p))
#define PF_SEQ (PFB(PF_S1) | PFB(PF_S2))                       /* the sequencer replays (punch.c rep_*) */
#define PF_TIME (PF_SEQ | PFB(PF_S4) | PFB(PF_S8) | PFB(PF_S16) | PFB(PF_S32) | PFB(PF_S8T) | PFB(PF_S16T) | PFB(PF_REV) | \
                 PFB(PF_HALF) | PFB(PF_STOP) | PFB(PF_DROP) | PFB(PF_FLNG))   /* one at a time: the last pressed */
static const int8_t PF_RING[PF_NPAD] = {-1, -1, PX_LOOP4, PX_LOOP8, PX_LOOP16, PX_LOOP32, PX_REV, PX_HALF,
                                     -1, -1, -1, -1, PX_STOP, PX_DROP, -1, -1, -1, -1, -1, PX_FLANGE, PX_LOOP8T, PX_LOOP16T};
enum { PP_LP, PP_HP, PP_LCUT, PP_CRUSH, PP_GATE, PP_N };   /* the COLOUR effects, in the order they run */
static const uint8_t PP_PAD[PP_N] = {PF_LP, PF_HP, PF_LCUT, PF_CRUSH, PF_GATE};
/* the knobs (KNOB 1..4 on the last pad pressed; perf.par): a value lo..hi, shown by its kind */
enum { KK_NONE, KK_PCT, KK_SEMI, KK_LIST, KK_CUT, KK_INT, KK_X, KK_GLO };
typedef struct {
    const char *lab;
    const char *const *names;             /* KK_LIST: the names, lo..hi */
    uint8_t kind, gid;                    /* KK_GLO: the global it turns (shared with GLO's page: G_*) */
    int8_t lo, hi, def;
} pk_t;
static const char *const PL_LEN[4] = {"1/16", "1/8", "1/4", "1/2"};       /* 16ths: 1 2 4 8 */
static const char *const PL_TIME[4] = {"1/8", "1/4", "1/2", "1/1"};       /* 16ths: 2 4 8 16 */
static const char *const PL_SPEED[7] = {"0", "1/16", "1/8", "1/4", "1/2", "1/1", "2/1"};   /* 16ths: 0 1 2 4 8 16 32 */
static const char *const PL_GRATE[3] = {"1/8", "1/16", "1/32"};
static const char *const PL_FRATE[4] = {"1/2", "1/1", "2/1", "4/1"};      /* a sweep: 8 16 32 64 16ths */
static const uint8_t PL_LEN16[4] = {1, 2, 4, 8}, PL_TIME16[4] = {2, 4, 8, 16}, PL_SPEED16[7] = {0, 1, 2, 4, 8, 16, 32},
                     PL_FRATE16[4] = {8, 16, 32, 64};
#define K_NONE {"", 0, KK_NONE, 0, 0, 0, 0}
#define K_AMT {"AMOUNT", 0, KK_PCT, 0, 0, 100, 100}
#define K_STUT {"PITCH", 0, KK_SEMI, 0, -12, 12, 0}, {"DECAY", 0, KK_PCT, 0, 0, 100, 0}, {"GATE", 0, KK_PCT, 0, 10, 100, 100}, K_AMT
static const struct {
    const char *pad, *name;               /* the pad's label (5 letters; the black keys' 4), the caption */
    pk_t k[4];
} PFD[PF_NPAD] = {
    {"1/1", "1/1 REPLAY", {K_NONE, K_NONE, K_NONE, K_NONE}},
    {"1/2", "1/2 REPLAY", {K_NONE, K_NONE, K_NONE, K_NONE}},
    {"1/4", "1/4 REPEAT", {K_STUT}},
    {"1/8", "1/8 REPEAT", {K_STUT}},
    {"1/16", "1/16 REPEAT", {K_STUT}},
    {"1/32", "1/32 REPEAT", {K_STUT}},
    {"REVRS", "REVERSE", {{"LENGTH", PL_LEN, KK_LIST, 0, 0, 3, 2}, K_NONE, K_NONE, K_AMT}},
    {"HALF", "HALF SPEED", {{"LENGTH", PL_LEN, KK_LIST, 0, 0, 3, 2}, K_NONE, K_NONE, K_AMT}},
    {"LP", "LOW-PASS SWEEP", {{"CUTOFF", 0, KK_CUT, 0, 0, 127, 34}, {"RESO", 0, KK_PCT, 0, 0, 100, 71},
                              {"SPEED", PL_SPEED, KK_LIST, 0, 0, 6, 4}, K_AMT}},
    {"HP", "HIGH-PASS SWEEP", {{"CUTOFF", 0, KK_CUT, 0, 0, 127, 79}, {"RESO", 0, KK_PCT, 0, 0, 100, 71},
                               {"SPEED", PL_SPEED, KK_LIST, 0, 0, 6, 4}, K_AMT}},
    {"CRUSH", "BIT CRUSH", {{"BITS", 0, KK_INT, 0, 2, 12, 7}, {"RATE", 0, KK_X, 0, 1, 16, 1}, K_NONE, K_AMT}},
    {"GATE", "TRANCE GATE", {{"RATE", PL_GRATE, KK_LIST, 0, 0, 2, 1}, {"WIDTH", 0, KK_PCT, 0, 10, 90, 50}, K_NONE, K_AMT}},
    {"STOP", "TAPE STOP", {{"TIME", PL_TIME, KK_LIST, 0, 0, 3, 1}, K_NONE, K_NONE, K_AMT}},
    {"DROP", "PITCH DROP", {{"DEPTH", 0, KK_SEMI, 0, 1, 24, 12}, {"TIME", PL_TIME, KK_LIST, 0, 0, 3, 2}, K_NONE, K_AMT}},
    {"ECHO", "ECHO THROW", {{"TIME", 0, KK_GLO, G_DTIME, 0, 0, 0}, {"FDBK", 0, KK_PCT, 0, 0, 95, 70}, K_NONE, K_AMT}},
    {"VERB", "REVERB THROW", {{"SIZE", 0, KK_GLO, G_RSIZE, 0, 0, 0}, {"DAMP", 0, KK_GLO, G_RDAMP, 0, 0, 0}, K_NONE, K_AMT}},
    {"-DRM", "KILL DRUMS", {K_NONE, K_NONE, K_NONE, K_NONE}},
    {"-BAS", "KILL BASS", {{"VOICE", 0, KK_INT, 0, 1, 3, 1}, K_NONE, K_NONE, K_NONE}},
    {"LCUT", "LOW CUT", {{"FREQ", 0, KK_CUT, 0, 20, 80, 43}, K_NONE, K_NONE, K_AMT}},
    {"FLNG", "FLANGER SWEEP", {{"RATE", PL_FRATE, KK_LIST, 0, 0, 3, 1}, {"FDBK", 0, KK_PCT, 0, 0, 90, 55}, K_NONE, K_AMT}},
    {"8T", "1/8T REPEAT", {K_STUT}},
    {"16T", "1/16T REPEAT", {K_STUT}},
};
#undef K_STUT
#define PX_DROP_W 2048u                   /* PITCH DROP: the window of its two read taps (46 ms) */
/* the ring keeps the mono mix >> PUNCH_SH, 16 bits linear (0 = silence): PUNCH_SH bits of headroom over the 16-bit
 * scale for a hot mix (clamped beyond; Ben, after wave 1: "Keep 12 dB"); reads give it back at the mix's scale */
#define PUNCH_SH 2
static int16_t punch_ring[PUNCH_N] __attribute__((section(".pool")));
#define PUNCH_PUT(i, v) (punch_ring[(i) & (PUNCH_N - 1u)] = (int16_t)(v))
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
    uint32_t lfo;                 /* flanger phase */
    uint32_t pq, pspd;            /* loops: read position in the loop Q16 and its speed (PITCH; 65536: as recorded) */
    int32_t rg, dec;              /* loops: this repeat's gain and the gain from one repeat to the next (DECAY), Q15 */
    uint32_t gl;                  /* loops: the part of each repeat that sounds (GATE; len: all) */
    int32_t ds;                   /* PITCH DROP: semitones down now, Q8 */
    /* the COLOUR effects' states (PP_*): LP f1/f2, HP f3/f4, the low cut's two SVFs a side, crush's held sample */
    int32_t f1l, f2l, f1r, f2r, f3l, f4l, f3r, f4r;
    int32_t z[4][2];
    int32_t held_l, held_r, hn;
    int32_t pg[PP_N];             /* their wet gains, Q15 */
    int32_t lcut, hcut;           /* the sweeps' cutoffs now, 0..127 << 8 */
    uint32_t gt;                  /* the gate's samples since it started (stopped: its own clock) */
} punch = {.cur = -1, .pad = 0xFF, .pspd = 65536, .rg = 32767, .dec = 32767, .lcut = 127 << 8, .hcut = 20 << 8};

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
    uint16_t stamp[PF_NPAD], ctr;    /* the order they started in (the last TIME pad wins) */
    uint16_t rg0[PF_NPAD];           /* recording: the 16th its span started (low bits) .. */
    uint8_t rst[PF_NPAD];            /* .. and its step on the lane */
    int8_t par[PF_NPAD][4];          /* the knobs' values */
    uint8_t kn[PF_NPAD];             /* the knob turned while it played (k + 1, 0 none), for the recording .. */
    int8_t knv[PF_NPAD];             /* .. and its value */
    uint8_t ovk[PF_NPAD];            /* the lane plays it: its recorded knob (k + 1, 0 none) .. */
    int8_t ovv[PF_NPAD];             /* .. and value */
    uint8_t last;                 /* the pad the knobs turn (the last pressed) */
    int8_t ring;                  /* the TIME effect on the ring now (PX_*), -1 none, -2 a replay (the ring stays dry) */
    uint8_t seq16;                /* a 1/2 or 1/1 pad: the replay's span in 1/16s */
    volatile uint8_t full;        /* a recorded pad found the lane full (the UI says so once) */
    uint8_t rmark;                /* a recording pass began: its first span marks the lane's undo */
    int32_t thr[2];               /* the throws: the reverb's and the delay's send asked for, Q15 */
    int32_t fb;                   /* ECHO: the delay's feedback at least (fx.c), G_DFDBK * 230 scale */
} perf = {.last = PF_S4, .ring = -1};
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
    perf.last = (uint8_t)p;
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
    uint32_t i, fr = PEV_N;
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
        if ((e->fx & 31u) == p + 1u && e->st < L &&
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
    stl.ev[fr].kv = perf.knv[p];
    stl.ev[fr].fx = (uint8_t)((p + 1u) | (perf.kn[p] ? ((perf.kn[p] - 1u) << 5 | 0x80u) : 0u));
}
static void perf_rec_drop(void) { perf.rprev = perf.ropen = 0; }   /* a pattern or a project loaded: the spans open go */
/* once a block, before the tracks (seq.c events_block, stl_seq): what plays, recorded, chosen */
static void perf_block(void)
{
    uint32_t L = stl.len ? stl.len : 16u, q = BEAT_U / 4u, g16 = clk_beat * 4u + clk_pos / q, s = g16 % L;
    uint32_t live = perf.held | perf.lat, lane = 0, act, rise, i, best = PF_NPAD;
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
        uint32_t rec = song.rec && song.playing, cur = rec ? live : 0u, up = cur & ~perf.rprev, dn = perf.ropen & ~cur;
        if (rec && !rec_was)
            perf.rmark = 1;
        rec_was = (uint8_t)rec;
        for (i = 0; dn >> i; i++)
            if ((dn >> i) & 1u) {
                uint32_t len = (uint16_t)((uint16_t)qn - perf.rg0[i]);
                perf_rec_put(i, perf.rst[i] % L, len < 1u ? 1u : len > L ? L : len, L);
            }
        perf.ropen &= ~dn;
        for (i = 0; up >> i; i++)
            if ((up >> i) & 1u) {
                perf.rg0[i] = (uint16_t)qn;
                perf.rst[i] = (uint8_t)(qn % L);
            }
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
        perf.ring = PF_RING[best] >= 0 ? PF_RING[best] : -2;
        if (best == PF_S1 || best == PF_S2)
            perf.seq16 = best == PF_S1 ? 16u : 8u;
        punch.pad = (uint8_t)best;
    }
    perf_kill = (uint8_t)(((act >> PF_KDRUM) & 1u) << TRK_DRUM |
                          ((act >> PF_KBASS) & 1u) << ((uint32_t)clamp(pf_val(PF_KBASS, 0), 1, 3) - 1u));
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

static int px_loop(int32_t fx) { return fx <= PX_LOOP16T; }   /* on the grid */
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
    static const uint8_t DIV[6] = {1, 2, 4, 8, 3, 6};  /* the loops: beat / DIV (1/4 .. 1/32, 1/8T, 1/16T) */
    uint32_t p = punch.pad, len;
    if (fx <= PX_LOOP16T)
        len = beat / DIV[fx];
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
/* the loops' PITCH, DECAY and GATE (a pad's; the STUTTER lane's steps: none), a block at a time */
static void punch_loop_knobs(void)
{
    uint32_t p = punch.pad;
    if (p >= PF_NPAD || PF_RING[p] < 0 || PF_RING[p] > PX_LOOP16T) {
        punch.pspd = 65536u, punch.dec = 32767, punch.gl = punch.len;
        return;
    }
    punch.pspd = (uint32_t)pr_ratio(pf_val(p, 0) << 8);
    punch.dec = (100 - pf_val(p, 1)) * 32767 / 100;
    punch.gl = pf_val(p, 2) >= 100 ? punch.len : punch.len * (uint32_t)pf_val(p, 2) / 100u;
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
        uint32_t back = ph % len;
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
    punch.ds = 0;
    punch.rg = 32767;
    punch_loop_knobs();
    punch.pq = (uint32_t)(((uint64_t)punch.k * punch.pspd) % ((uint64_t)len << 16));
    if (fx == PX_FLANGE)                                /* FLANGER: the sweep starts at its shortest delay */
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

/* the SVF of dsp.c tsvf_lp with its high-pass out too (Butterworth: tsvf_coef reso 40, damping 5799 Q12), two in a
 * row (Linkwitz-Riley, 24 dB/oct): LOW CUT (the old KILL LOW's, its frequency on a knob) */
#define PX_SVF(A1, A2, A3, Z0, Z1, IN, HP) do {                                                \
        int32_t v3_ = (IN) - Z1, v1_ = ((A1) * Z0 + (A2) * v3_) >> 13;                         \
        int32_t v2_ = Z1 + (((A2) * Z0 + (A3) * v3_) >> 13);                                   \
        HP = (IN) - ((v1_ * 5799) >> 12) - v2_;                                                 \
        Z0 = clamp(2 * v1_ - Z0, -150000, 150000);                                              \
        Z1 = clamp(2 * v2_ - Z1, -150000, 150000);                                              \
    } while (0)
static int32_t px_lcut(const tsvf_t *c, int32_t x, int32_t (*z)[2])
{
    int32_t h, b;
    x = clamp(x, -280000, 280000) >> 1;
    PX_SVF(c->a1, c->a2, c->a3, z[0][0], z[0][1], x, h);
    PX_SVF(c->a1, c->a2, c->a3, z[1][0], z[1][1], h, b);
    return b << 1;
}
#undef PX_SVF
static int32_t px_ramp(int32_t g, int32_t target)   /* a wet gain a sample nearer its target (64 samples end to end) */
{
    return g < target ? (g + 512 > target ? target : g + 512) : g > target ? (g - 512 < target ? target : g - 512) : g;
}
#define PX_WET(X, W, G) ((X) + (((((W) - (X)) >> 3) * ((G) >> 3)) >> 9))   /* (no 32-bit overflow) */

/* the COLOUR effects (after the TIME effect), in place */
static void punch_colour(int32_t *l, int32_t *r, uint32_t n, uint32_t beat)
{
    uint32_t i, j, on = 0, run = 0, step = 1, wid = 1, bits = 0, hold = 1;
    int32_t tg[PP_N];
    tsvf_t clp, chp, clc;
    for (j = 0; j < PP_N; j++) {
        uint32_t p = PP_PAD[j];
        tg[j] = (perf.act >> p) & 1u ? pf_amt(p) : 0;
        on |= (uint32_t)(tg[j] != 0) << j;
        if (tg[j] || punch.pg[j])
            run |= 1u << j;
    }
    if (!run)
        return;
    if (run & 1u << PP_LP) {                            /* LP: from open down to CUTOFF in SPEED */
        int32_t to = pf_val(PF_LP, 0) << 8, sp = PL_SPEED16[clamp(pf_val(PF_LP, 2), 0, 6)];
        int32_t d = sp ? (int32_t)((uint32_t)(127 << 8) * n * 4u / (beat * (uint32_t)sp) + 1u) : 127 << 8;
        if (on & 1u << PP_LP)
            punch.lcut = punch.lcut > to ? (punch.lcut - d > to ? punch.lcut - d : to) : (punch.lcut + d < to ? punch.lcut + d : to);
        tsvf_coef(&clp, punch.lcut, pf_val(PF_LP, 1) * 127 / 100);
    }
    if (run & 1u << PP_HP) {                            /* HP: from the bottom up to CUTOFF in SPEED */
        int32_t to = pf_val(PF_HP, 0) << 8, sp = PL_SPEED16[clamp(pf_val(PF_HP, 2), 0, 6)];
        int32_t d = sp ? (int32_t)((uint32_t)(127 << 8) * n * 4u / (beat * (uint32_t)sp) + 1u) : 127 << 8;
        if (on & 1u << PP_HP)
            punch.hcut = punch.hcut < to ? (punch.hcut + d < to ? punch.hcut + d : to) : (punch.hcut - d > to ? punch.hcut - d : to);
        tsvf_coef(&chp, punch.hcut, pf_val(PF_HP, 1) * 127 / 100);
    }
    if (run & 1u << PP_LCUT)
        tsvf_coef(&clc, pf_val(PF_LCUT, 0) << 8, 40);
    if (run & 1u << PP_CRUSH) {
        bits = 18u - (uint32_t)clamp(pf_val(PF_CRUSH, 0), 2, 12);   /* (BITS 7: the old mask 0x7FF) */
        hold = (uint32_t)clamp(pf_val(PF_CRUSH, 1), 1, 16);
    }
    if (run & 1u << PP_GATE) {                          /* GATE: RATE and WIDTH, on the grid while playing */
        static const uint8_t DIV[3] = {2, 4, 8};
        step = beat / DIV[clamp(pf_val(PF_GATE, 0), 0, 2)];
        step = step ? step : 1u;
        wid = step * (uint32_t)clamp(pf_val(PF_GATE, 1), 10, 90) / 100u;
    }
    for (i = 0; i < n; i++) {
        int32_t x = l[i], y = r[i], w0, w1;
        if (run & 1u << PP_LP) {
            w0 = tsvf_lp(&clp, x >> 1, &punch.f1l, &punch.f2l) << 1;
            w1 = tsvf_lp(&clp, y >> 1, &punch.f1r, &punch.f2r) << 1;
            punch.pg[PP_LP] = px_ramp(punch.pg[PP_LP], tg[PP_LP]);
            x = PX_WET(x, w0, punch.pg[PP_LP]), y = PX_WET(y, w1, punch.pg[PP_LP]);
        }
        if (run & 1u << PP_HP) {
            w0 = x - (tsvf_lp(&chp, x >> 1, &punch.f3l, &punch.f4l) << 1);
            w1 = y - (tsvf_lp(&chp, y >> 1, &punch.f3r, &punch.f4r) << 1);
            punch.pg[PP_HP] = px_ramp(punch.pg[PP_HP], tg[PP_HP]);
            x = PX_WET(x, w0, punch.pg[PP_HP]), y = PX_WET(y, w1, punch.pg[PP_HP]);
        }
        if (run & 1u << PP_LCUT) {
            w0 = px_lcut(&clc, x, &punch.z[0]);
            w1 = px_lcut(&clc, y, &punch.z[2]);
            punch.pg[PP_LCUT] = px_ramp(punch.pg[PP_LCUT], tg[PP_LCUT]);
            x = PX_WET(x, w0, punch.pg[PP_LCUT]), y = PX_WET(y, w1, punch.pg[PP_LCUT]);
        }
        if (run & 1u << PP_CRUSH) {
            if (--punch.hn <= 0) {                      /* RATE: one sample in RATE held */
                punch.hn = (int32_t)hold;
                punch.held_l = x >= 0 ? x & ~((1 << bits) - 1) : -((-x) & ~((1 << bits) - 1));   /* towards zero */
                punch.held_r = y >= 0 ? y & ~((1 << bits) - 1) : -((-y) & ~((1 << bits) - 1));
            }
            punch.pg[PP_CRUSH] = px_ramp(punch.pg[PP_CRUSH], tg[PP_CRUSH]);
            x = PX_WET(x, punch.held_l, punch.pg[PP_CRUSH]), y = PX_WET(y, punch.held_r, punch.pg[PP_CRUSH]);
        }
        if (run & 1u << PP_GATE) {
            uint32_t ph = (song.playing ? clk_samples() + i : punch.gt) % step;
            int32_t gg = ph < wid ? 32767 : 0;
            if (ph < 64u)
                gg = (int32_t)ph * 512;
            else if (ph >= wid && ph < wid + 64u)
                gg = 32767 - (int32_t)(ph - wid) * 512;
            punch.gt++;
            punch.pg[PP_GATE] = px_ramp(punch.pg[PP_GATE], tg[PP_GATE]);
            w0 = ((x >> 3) * gg) >> 12;
            w1 = ((y >> 3) * gg) >> 12;
            x = PX_WET(x, w0, punch.pg[PP_GATE]), y = PX_WET(y, w1, punch.pg[PP_GATE]);
        }
        l[i] = x, r[i] = y;
    }
    for (j = 0; j < PP_N; j++)                          /* faded out: its state cleared, the next press starts clean */
        if ((run >> j) & 1u && !tg[j] && !punch.pg[j]) {
            if (j == PP_LP)
                punch.f1l = punch.f2l = punch.f1r = punch.f2r = 0, punch.lcut = 127 << 8;
            else if (j == PP_HP)
                punch.f3l = punch.f4l = punch.f3r = punch.f4r = 0, punch.hcut = 20 << 8;
            else if (j == PP_LCUT)
                memset(punch.z, 0, sizeof punch.z);
            else if (j == PP_CRUSH)
                punch.hn = 0;
            else
                punch.gt = 0;
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
        if (fx == PX_FLANGE && punch.pad < PF_NPAD) {      /* FLANGER: RATE (a bar a sweep: the old one), FDBK */
            binc = binc * 4u / PL_FRATE16[clamp(pf_val(punch.pad, 0), 0, 3)];
            ftap = (uint32_t)pf_val(punch.pad, 1) * 9000u / 55u;
        } else if (fx == PX_FLANGE) {
            binc >>= 2;
        }
        if (fx == PX_DROP && punch.pad < PF_NPAD && want == fx) {   /* PITCH DROP: DEPTH semitones down in its TIME */
            int32_t to = pf_val(punch.pad, 0) << 8;
            uint32_t span = beat * PL_TIME16[clamp(pf_val(punch.pad, 1), 0, 3)] / 4u;
            int32_t d = (int32_t)((uint32_t)to * n / (span ? span : 1u)) + 1;
            punch.ds = punch.ds + d < to ? punch.ds + d : to;
        }
    for (i = 0; i < n; i++) {
        int32_t x = l[i], y = r[i], wl = x, wr = y, m = (x + y) >> 1;
        uint32_t target = want == fx ? (uint32_t)amt : 0u;
        if (!punch_owns(fx, punch.w))                   /* (a held loop keeps its material) */
            PUNCH_PUT(punch.w, clamp(m >> PUNCH_SH, -32768, 32767));
        switch (fx) {
        case PX_DROP: {                                 /* two taps read slower, half a window apart (the old OCT UP's,
                                                         * any ratio): the mix falls in pitch and stays there */
            uint32_t rt = pr_ratio(-punch.ds), d1, d2;
            int32_t g1;
            punch.pq += 65536u - rt;                    /* (slower than written: the taps fall behind) */
            if (punch.pq >= PX_DROP_W << 16)
                punch.pq -= PX_DROP_W << 16;
            d1 = punch.pq + (2u << 16);
            d2 = d1 + (PX_DROP_W / 2u << 16);
            if (d2 >= (PX_DROP_W + 2u) << 16)
                d2 -= PX_DROP_W << 16;
            g1 = (int32_t)((punch.pq >> 16) <= PX_DROP_W / 2u ? punch.pq >> 16 : PX_DROP_W - (punch.pq >> 16)) << 5;   /* g1 + g2 = 1 */
            wl = wr = (ring_raw_q16((punch.w << 16) - d1) * g1 + ring_raw_q16((punch.w << 16) - d2) * (32768 - g1)) >> (15 - PUNCH_SH);
            break;
        }
        case PX_FLANGE: {                               /* 0.6..6 ms, swept over RATE, left and right a quarter apart */
            uint32_t dl = (26u << 16) + (uint32_t)((sine_i(punch.lfo) + 32768) * 240),
                     dr = (26u << 16) + (uint32_t)((sine_i(punch.lfo + 0x40000000u) + 32768) * 240);
            int32_t el = ring_raw_q16((punch.w << 16) - dl), er = ring_raw_q16((punch.w << 16) - dr);
            punch.lfo += binc;
            PUNCH_PUT(punch.w, clamp((m >> PUNCH_SH) + (((el + er) * (int32_t)ftap) >> 15), -32768, 32767));   /* feedback (the ring is the delay line) */
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
/* the black keys, F#3 .. F#5 (the drum screen's RETRIG rates use them too), and the pads on them (-1 none) */
static const uint8_t PUNCH_BLACK[11] = {1, 3, 5, 8, 10, 13, 15, 17, 20, 22, 25};
static const int8_t PF_BLACK[11] = {PF_KDRUM, PF_KBASS, PF_LCUT, PF_FLNG, -1, -1, -1, -1, PF_S8T, PF_S16T, -1};
/* the keyboard (seq.c keyboard_block) while FX is held: a key -> its pad (-1 none) */
static int32_t punch_fx_of_key(uint32_t k)
{
    int32_t w = punch_key(k);
    uint32_t i;
    if (w >= 0)
        return w;
    for (i = 0; i < 11u; i++)
        if (PUNCH_BLACK[i] == k)
            return PF_BLACK[i];
    return -1;
}
static uint32_t punch_key_of_fx(uint32_t fx)          /* a pad -> its key (the lit key) */
{
    static const uint8_t K[7] = {0, 2, 4, 6, 7, 9, 11};   /* the white keys of an octave, from F */
    uint32_t i;
    if (fx < 16u)
        return fx / 7u * 12u + K[fx % 7u];
    for (i = 0; i < 11u; i++)
        if (PF_BLACK[i] == (int8_t)fx)
            return PUNCH_BLACK[i];
    return 0;
}
