/* SPDX-License-Identifier: GPL-3.0-only */
/* The oscillator Types (PHEBUS wave 4; Ben, 21 Nov 2026, picker "OSC page: A: Type + three macros": "knob 1 picks the
 * Type, knobs 2 to 4 are its three macros, renamed per type"; 22 Nov: "Make the perfect version. Then we'll compromise":
 * every Type on every track). As the MiniFreak's oscillator: one Type list, the same on every voice, and three macro knobs
 * whose names change with the Type. A Type is an engine and, where the engine has several, one of its models: ACID (the
 * 303), JUNO (the Juno-106's DCO), each of Plaits' models (PLAITS: Emilie Gillet's, MIT) and each of Elements' exciters
 * (ELEMENTS: hers too). Its macros are the engine's own E0 .. E7, by Type: M1 .. M3 on OSC 1/3, M4 and M5 (what else the
 * Type has) on OSC 3/3; each named as the Type means it (the Plaits names from her manual's description of each model).
 * Nothing here is stored: a track's Type is its engine (the engine byte) and that engine's MODEL (P_E0), as before. */
typedef struct {
    uint8_t eng, model;          /* engine (ENGI_*), its MODEL (P_E0; 0xFF: an engine without models) */
    const char *name;            /* on the TYPE knob (five characters) */
    const char *big;             /* on the OSC page, large (nine) */
    uint8_t m[5];                /* M1 .. M5: P_E0 + m, 0xFF: none */
    const char *lab[5];          /* their names (0: the engine's) */
} vtype_t;
#define VT_PLX(k) ((uint8_t)(PRM_PLX + (k)))
#define VT_PL(md, nm, bg, a, b, c) {ENGI_PRISM, md, nm, bg, {1, 2, 3, 4, 5}, {a, b, c, "T.ENV", "SUB"}}
#define VT_EL(md, nm, bg) {ENGI_ELEM, md, nm, bg, {1, 2, 3, 6, 4}, {0, 0, 0, 0, 0}}
static const vtype_t VTYPES[] = {
    {ENGI_ACID, 0xFF, "ACID", "ACID 303", {0, 5, 7, 0xFF, 0xFF}, {"WAVE", "ACCENT", "SLIDE", 0, 0}},
    {ENGI_JUNO, 0xFF, "JUNO", "JUNO DCO", {0, 1, 2, 3, 7}, {"WAVE", "PW", "SUB", "NOISE", "CHORUS"}},
    VT_PL(VT_PLX(0), "VA", "VA", "DETUNE", "SQUARE", "SAW"),
    VT_PL(VT_PLX(1), "VCF", "VA VCF", "F.RES", "F.CUT", "WAVE"),
    VT_PL(PRM_FOLD, "FOLD", "FOLD", "SHAPE", "FOLD", "SKEW"),
    VT_PL(VT_PLX(2), "PHDIS", "PHASE D", "FREQ", "AMOUNT", "ASYM"),
    VT_PL(VT_PLX(3), "FM", "2-OP FM", "RATIO", "INDEX", "FDBK"),
    VT_PL(VT_PLX(4), "6OP A", "6-OP A", "PATCH", "LEVEL", "ENV"),
    VT_PL(VT_PLX(5), "6OP B", "6-OP B", "PATCH", "LEVEL", "ENV"),
    VT_PL(VT_PLX(6), "6OP C", "6-OP C", "PATCH", "LEVEL", "ENV"),
    VT_PL(PRM_WAVE, "WAVE", "WAVETABLE", "BANK", "ROW", "COLUMN"),
    VT_PL(VT_PLX(8), "TERRN", "TERRAIN", "TERRN", "SIZE", "OFFSET"),
    VT_PL(PRM_HARM, "HARM", "ADDITIVE", "BUMPS", "CENTRE", "WIDTH"),
    VT_PL(PRM_CHORD, "CHORD", "CHORD", "CHORD", "INVRT", "SOUND"),
    VT_PL(VT_PLX(9), "STRMC", "STR MACH", "CHORD", "COLOUR", "WAVE"),
    VT_PL(PRM_SWARM, "SWARM", "SWARM", "SPREAD", "RATE", "SIZE"),
    VT_PL(VT_PLX(7), "FRMNT", "FORMANT", "RATIO", "FORMNT", "WIDTH"),
    VT_PL(VT_PLX(10), "CHIP", "CHIPTUNE", "CHORD", "ARP", "PW"),
    VT_PL(VT_PLX(11), "STRNG", "STRING", "INHARM", "BRIGHT", "DECAY"),
    VT_PL(VT_PLX(12), "MODAL", "MODAL", "MATERL", "BRIGHT", "DECAY"),
    VT_EL(EM_BOUNCE, "BOUNC", "BOUNCE"),
    VT_EL(EM_PICK, "PICK", "PLUCKED"),
    VT_EL(EM_BOW, "BOW", "BOWED"),
    VT_EL(EM_BLOW, "BLOW", "BLOWN"),
    {ENGI_ELEM, EM_OMIN, "OMIN", "OMINOUS", {1, 2, 3, 6, 7}, {0, 0, 0, 0, 0}},
    VT_PL(VT_PLX(13), "NOISE", "NOISE", "FILTER", "CLOCK", "RESO"),
    VT_PL(VT_PLX(14), "SPECH", "SPEECH", "MODE", "FORMNT", "WORD"),
    VT_PL(VT_PLX(15), "PARTC", "PARTICLE", "RANDOM", "DENSTY", "FILTER"),
    VT_PL(VT_PLX(16), "KICK", "KICK", "PUNCH", "TONE", "DECAY"),
    VT_PL(VT_PLX(17), "SNARE", "SNARE", "NOISE", "TONE", "DECAY"),
    VT_PL(VT_PLX(18), "HIHAT", "HI-HAT", "METAL", "TONE", "DECAY"),
};
#define NVTYPE ((uint32_t)NELEM(VTYPES))
_Static_assert(NELEM(VTYPES) == 2u + PRM_COUNT + EM_COUNT, "a Type a model: ACID, JUNO, every PLAITS and ELEMENTS model");
#undef VT_PL
#undef VT_EL
/* the Type a track plays (the engine it asked for, its MODEL), NVTYPE: none (the drum track, an engine gone) */
static uint32_t vtype_find(uint32_t e, uint32_t md)   /* (wave 5) .. of engine e, MODEL md */
{
    uint32_t i;
    for (i = 0; i < NVTYPE; i++)
        if (VTYPES[i].eng == e && (VTYPES[i].model == 0xFFu || VTYPES[i].model == md))
            return i;
    return NVTYPE;
}
static uint32_t vtype_of(const track_t *t) { return vtype_find(t->eng_req % NENGINES, (uint32_t)t->p[P_E0]); }
/* the E index (0..7) of macro k (0..4) on track t, 0xFF: none */
static uint32_t vtype_macro(const track_t *t, uint32_t k)
{
    uint32_t ty = vtype_of(t);
    return ty < NVTYPE && k < 5u ? VTYPES[ty].m[k] : 0xFFu;
}
/* the name the Type gives E index i on track t (0: the engine's own) */
static const char *vtype_label(const track_t *t, uint32_t i)
{
    uint32_t ty = vtype_of(t), k;
    if (ty >= NVTYPE)
        return 0;
    for (k = 0; k < 5u; k++)
        if (VTYPES[ty].m[k] == i)
            return VTYPES[ty].lab[k];
    return 0;
}
/* the Type's MODEL is the knob TYPE (P_E0 of a PLAITS or ELEMENTS voice): never a macro, never its own knob */
static int vtype_is_model(const track_t *t, uint32_t id)
{
    return id == P_E0 && (t->eng_req % NENGINES == ENGI_PRISM || t->eng_req % NENGINES == ENGI_ELEM);
}

/* (PHEBUS wave 4) the TIME index (params.c F_TIME, 0..127) nearest ms10 tenths of a millisecond */
static int16_t time_idx(uint32_t ms10)
{
    uint32_t i, b = 0, bd = 0xFFFFFFFFu;
    for (i = 0; i < 128u; i++) {
        uint32_t d = TIME_MS_X10[i] > ms10 ? TIME_MS_X10[i] - ms10 : ms10 - TIME_MS_X10[i];
        if (d < bd)
            bd = d, b = i;
    }
    return (int16_t)b;
}
/* (wave 4) a sound made before the voice filter (a factory preset, a user preset of record 1, a project saved before wave 4:
 * p, its engine e) onto the voice filter, as close as it sounded: each engine's own filter values become the FILT page's
 * (ACID: CUT RES, ENVMOD as the ENV amount the 303's envelope swept, DECAY as an OWN envelope that decays as the 303's,
 * DRIVE; JUNO: CUT RES, ENVMOD, its half key tracking, LINK as its one envelope); PLAITS and ELEMENTS start open (they had
 * no filter: ENV > FLT moved PLAITS' TIMB, now T.ENV); the old track FILT (the DJ LP / HP) becomes the voice filter's
 * cutoff and type. old: p holds a project's or a record's values (its ENV > FLT and FILT are mapped); else a preset's */
static void voice_from_engine(int16_t *p, uint32_t e, int old)
{
    int32_t ed = old ? p[P_ED_FLT] : 0, tf = old ? p[P_TFLT] : 0;
    p[P_FCUT] = 127;
    p[P_FRES] = 0;
    p[P_FKEY] = 0;
    p[P_FDRV] = 0;
    p[P_FLINK] = 0;
    p[P_FATK] = p[P_ATK];
    p[P_FDEC] = p[P_DEC];
    p[P_FSUS] = p[P_SUS];
    p[P_FREL] = p[P_REL];
    p[P_FTYPE] = VF_LP24;
    p[P_OCT] = 0;
    p[P_TUNE] = 0;
    p[P_ED_FLT] = 0;
    if (e == ENGI_ACID) {
        /* the 303's envelope: tau = 200 ms x 10^(DECAY / 127), 99 % gone after 4.6 tau; its sweep, ENVMOD's scaler over
         * what the 303 sweeps with ENVMOD 0 (that sweep it still makes itself), x 0.62 of the envelope's peak, in steps */
        int32_t tau = (int32_t)(200.0f * fm_exp2f((float)clamp(p[P_E4], 0, 127) * (3.321928f / 127.0f)));
        float sw = (4.0f * (float)clamp(p[P_E3], 0, 127) / 127.0f + 0.06f) * 0.62f * 12.7f;
        p[P_FCUT] = (int16_t)clamp(p[P_E1], 0, 127);
        p[P_FRES] = (int16_t)clamp(p[P_E2], 0, 127);
        p[P_ED_FLT] = (int16_t)clamp((int32_t)(sw * 0.5f + 0.5f) + ed / 2, -64, 63);
        p[P_FLINK] = 1;
        p[P_FATK] = 0;
        p[P_FDEC] = time_idx((uint32_t)tau * 46u);
        p[P_FSUS] = 0;
        p[P_FREL] = p[P_FDEC];
        p[P_FDRV] = (int16_t)clamp(p[P_E6], 0, 127);
    } else if (e == ENGI_JUNO) {
        p[P_FCUT] = (int16_t)clamp(p[P_E4], 0, 127);
        p[P_FRES] = (int16_t)clamp(p[P_E5], 0, 127);
        p[P_ED_FLT] = (int16_t)clamp((p[P_E6] + ed) / 2, -64, 63);
        p[P_FKEY] = 45;                               /* (the Juno's KYBD half way, as the engine had it: 0.35 of 100 %) */
    } else if (e == ENGI_PRISM && ed > 0) {
        p[P_E4] = (int16_t)clamp(p[P_E4] + ed, 0, 127);   /* (ENV > FLT opened TIMB: T.ENV does) */
    }
    if (tf < 0) {                                     /* the old DJ low-pass: the cutoff down as far */
        p[P_FCUT] = (int16_t)clamp(p[P_FCUT] - (-tf) * 110 / 64, 0, 127);
        if (e == ENGI_ACID || e == ENGI_JUNO)
            p[P_FTYPE] = VF_LP12;                     /* (theirs is fixed: the ladder after them, 12 dB as the DJ filter) */
    } else if (tf > 0) {                              /* the old DJ high-pass: the voice filter's HP */
        p[P_FTYPE] = VF_HP;
        p[P_FCUT] = (int16_t)clamp(tf * 110 / 63, 0, 127);
    }
    if (old)
        p[P_TFLT] = 0;
}
/* (wave 8) a synth track's old FILT (the DJ LP / HP MIDI CC 74 set, with no page since wave 4) onto its voice filter, as
 * voice_from_engine maps it: a low-pass the cutoff down as far, a high-pass the voice filter's HP at that cutoff */
static void tflt_onto_voice(int16_t *p, uint32_t e)
{
    int32_t tf = p[P_TFLT];
    if (tf < 0) {
        p[P_FCUT] = (int16_t)clamp(p[P_FCUT] - (-tf) * 110 / 64, 0, 127);
        if (e == ENGI_ACID || e == ENGI_JUNO)
            p[P_FTYPE] = p[P_FTYPE] == VF_LP24 ? VF_LP12 : p[P_FTYPE];
    } else if (tf > 0) {
        p[P_FTYPE] = VF_HP;
        p[P_FCUT] = (int16_t)clamp(tf * 110 / 63, 0, 127);
    }
    p[P_TFLT] = 0;
}
/* (wave 4) the locks of a sound made before the voice filter (track t, engine e), as voice_from_engine moved its values:
 * an ACID or JUNO lock of its filter becomes the voice filter's */
static void voice_locks_from_engine(track_t *t, uint32_t e)
{
    uint32_t i;
    for (i = 0; i < NLOCK; i++) {
        plock_t *l = &t->lock[i];
        uint32_t k = (uint32_t)l->param - P_E0;
        if (l->step == LOCK_FREE || l->param < P_E0 || l->param > P_E7)
            continue;
        if (e == ENGI_ACID && (k == 1u || k == 2u || k == 6u))
            l->param = (uint8_t)(k == 1u ? P_FCUT : k == 2u ? P_FRES : P_FDRV);
        else if (e == ENGI_ACID && k == 3u)
            l->param = P_ED_FLT, l->val = (int16_t)clamp(l->val / 4, -64, 63);
        else if (e == ENGI_JUNO && (k == 4u || k == 5u))
            l->param = (uint8_t)(k == 4u ? P_FCUT : P_FRES);
        else if (e == ENGI_JUNO && k == 6u)
            l->param = P_ED_FLT, l->val = (int16_t)clamp(l->val / 2, -64, 63);
    }
}

