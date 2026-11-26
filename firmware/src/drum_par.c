/* SPDX-License-Identifier: GPL-3.0-only */
/* The drum sounds' parameters as the Rytm-style pages show them (PHEBUS wave 3; the model: drum_snd.c). Included at the
 * end of params.c (it needs its names and param_format). */
/* ---- the parameters: what each id is on each machine, its range and how it reads */
static const char *const N_DSWAVE[] = {"OFF", "SINE", "TRI", "SQR", "FM", "BELL"};
static const char *const N_DSNOISE[] = {"OFF", "WHITE", "METAL", "CYM", "CHIP", "CLAP"};
static const char *const N_DSFT[] = {"OFF", "LP", "BP", "HP", "LP+", "BP+", "HP+"};   /* + : the tone through it too */
static const char *const N_DLDST[] = {"PITCH", "CUT", "RESO", "LEVEL"};   /* (wave 8: RESO, one spelling) */
/* (wave 12) DEST on each machine: the four, then every knob that can be wiggled (drum_snd.c DLD_HEX, DLD_SYN, in order) */
static const char *const N_DLDST_HEX[] = {"PITCH", "CUT", "RESO", "LEVEL", "FINE", "ATK", "DEC", "SLOPE", W_PAMT, W_PDEC,
                                          "M PIT", "FM", "T LVL", "T PIT", "DRIVE", "DIST", "CRUSH", "REV", "DEL"};
static const char *const N_DLDST_SYN[] = {"PITCH", "CUT", "RESO", "LEVEL", "FINE", "SWEEP", "SWTIM", "HOLD", "DECAY", "TONE",
                                          "PART2", "P2LVL", "CLICK", W_NSLVL, W_NSHLD, W_NSDEC, "ENV", "HPF", "CHIP", "DRIVE",
                                          "REV", "DEL"};
_Static_assert(sizeof N_DLDST_HEX / sizeof N_DLDST_HEX[0] == DL_N + NDLD_HEX &&
               sizeof N_DLDST_SYN / sizeof N_DLDST_SYN[0] == DL_N + NDLD_SYN, "a DEST name a knob");
static const char *const N_DLMOD[] = {"TRIG", "FREE", "ONE"};
#define DD(l, f, mn, mx, df) {l, f, mn, mx, df, 0, 0}
#define DE(l, n, df) {l, F_ENUM, 0, (int16_t)(sizeof(n) / sizeof(n[0]) - 1), df, n, 0}
static const param_desc_t DS_DESC[DS_N] = {
    [DS_WAVE] = DE("WAVE", N_DSWAVE, 1), [DS_NOISE] = DE("NOISE", N_DSNOISE, 0),
    [DS_TUNE] = DD("TUNE", F_INT, 0, 127, 60), [DS_FINE] = DD("FINE", F_INT, 0, 15, 0),
    [DS_SWEEP] = DD("SWEEP", F_INT, 0, 96, 0), [DS_SWTIME] = DD("SWTIME", F_INT, 0, 127, 40),
    [DS_HOLD] = DD("HOLD", F_INT, 0, 127, 0), [DS_DECAY] = DD("DECAY", F_INT, 0, 127, 60),
    [DS_TONE] = DD("TONE", F_INT, 0, 127, 100), [DS_PART2] = DD("PART2", F_INT, 0, 127, 0),
    [DS_P2LVL] = DD("P2 LVL", F_INT, 0, 127, 0), [DS_CLICK] = DD("CLICK", F_INT, 0, 127, 0),
    [DS_NLVL] = DD(W_NSLVL, F_INT, 0, 127, 0), [DS_NHOLD] = DD(W_NSHLD, F_INT, 0, 127, 0),
    [DS_NDEC] = DD(W_NSDEC, F_INT, 0, 127, 40), [DS_FTYPE] = DE("TYPE", N_DSFT, 0),
    [DS_RES] = DD("RESO", F_INT, 0, 31, 0), [DS_FREQ] = DD("CUTOFF", F_INT, 0, 127, 127),   /* (wave 8: the synth FILT's words) */
    [DS_FENV] = DD("ENV", F_INT, -64, 63, 0), [DS_HPF] = DD("HPF", F_INT, 0, 127, 0),
    [DS_CHIP] = DD("CHIP", F_INT, 0, 127, 0), [DS_DRIVE] = DD("DRIVE", F_INT, 0, 127, 0),
    [DS_LEVEL] = {"LEVEL", F_INT, -32, 31, 0, 0, "dB"},
};
static const param_desc_t DL_DESC[8] = {     /* DSP_LSPD .. DSP_LMOD */
    DD("SPEED", F_LFOHZ, 0, 127, 64), DD("DEPTH", F_INT, -64, 63, 0), DE("DEST", N_DLDST, 0), DE("WAVE", N_LWAVE, 0),
    DE("SYNC", N_LSYNC, 0), DD("FADE", F_INT, -64, 63, 0), DD("PHASE", F_INT, 0, 127, 0), DE("MODE", N_DLMOD, 0),
};
/* (the drum effects under FX) the sound's sends, named and ranged as a synth voice's SENDS (params.c P_DLY, P_REV: 0..127);
 * its KIT FX switch (Ben 21:45: "a KIT FX switch per sound"): ON through the kit's FILTER, INSERT and SLICER, OFF straight to
 * the mix. DSP_KFX is a knob of the SENDS page only, no lock id (not lockable: it lives in the slot's send byte) */
static const param_desc_t DSEND_DESC[2] = {DD("REVERB", F_INT, 0, DRUM_SEND_REV, 0), DD("DELAY", F_INT, 0, DRUM_SEND_DLY, 0)};
#define DSP_KFX 0xF0u
static const param_desc_t DKFX_DESC = DE("KIT FX", N_ONOFF, 1);
static const param_desc_t DNOTE_DESC = DD("NOTE", F_SEMI, -48, 48, 0);   /* (chromatic: 27 keys, OCT -2 .. +2) */
/* a HEX byte's descriptor as the sound's model reads it (the DRUM model's are params.c GP's, G_HXMODEL ..) */
static const param_desc_t *hex_desc_m(uint32_t m, uint32_t id)
{
    static const param_desc_t SN[] = {DD("PSLOPE", F_INT, -64, 62, 0), DD(W_PAMT, F_INT, 0, 127, 0),
                                      DD(W_PDEC, F_INT, 0, 127, 30), DD(W_NSFRQ, F_INT, 0, 127, 120),
                                      DD("MIX", F_INT, 0, 127, 90), DD("REPT", F_INT, 0, 127, 0)};
    static const param_desc_t CY[] = {DD("FINE", F_INT, -64, 62, 0), DD("MOD2", F_INT, 0, 127, 60),
                                      DD("PIT2", F_INT, 0, 127, 120), DD("PIT1", F_INT, 0, 127, 90),
                                      DD("MOD1", F_INT, 0, 127, 60), DE("WAVES", N_HXWAV2, 22)};
    static const param_desc_t AUX_DRUM = DE("MOD", N_HXMODW, 0);
    static const uint8_t AT[22] = {9, 9, 9, 0, 9, 9, 9, 9, 1, 2, 3, 4, 9, 9, 9, 9, 9, 9, 9, 5, 9, 9};
    uint32_t a = id < 22u ? AT[id] : 9u;
    if (a == 9u)
        return &GP[G_HXMODEL + id % 22u];
    if (m == HM_DRUM)
        return a == 5u ? &AUX_DRUM : &GP[G_HXMODEL + id];
    return m <= HM_CLAP ? &SN[a] : &CY[a];
}
/* id's descriptor on machine model (snd: the sound, for a HEX model's own names); 0: not one of that machine's */
static const param_desc_t *dsp_desc(uint32_t model, const uint8_t *snd, uint32_t id)
{
    if (id < DSP_COUNT)
        return model == DM_HEX ? hex_desc_m(snd ? snd[DSP_MODEL] % HM_COUNT : HM_DRUM, id) : 0;
    if (id == DSP_REV || id == DSP_DEL)
        return &DSEND_DESC[id - DSP_REV];
    if (id == DSP_KFX)
        return &DKFX_DESC;
    if (id == DSP_LDST) {                        /* (wave 12) the machine's own list */
        static const param_desc_t DH = DE("DEST", N_DLDST_HEX, 0), DS = DE("DEST", N_DLDST_SYN, 0);
        return model == DM_SYN ? &DS : &DH;
    }
    if (id >= DSP_LSPD && id <= DSP_LMOD)
        return &DL_DESC[id - DSP_LSPD];
    if (id >= DSP_S0 && id < DSP_S0 + DS_N)
        return model == DM_SYN ? &DS_DESC[id - DSP_S0] : 0;
    if (id == DSP_NOTE)
        return &DNOTE_DESC;
    return 0;
}
/* the value of id in sound snd / LFO lf (lane l's sends) */
static int32_t dsp_get(uint32_t model, const uint8_t *snd, const dlfo_t *lf, uint32_t l, uint32_t id)
{
    const dsnd_t *d = (const dsnd_t *)snd;
    if (id < DSP_COUNT)
        return model != DM_HEX ? 0 : id == DSP_FINE ? (int32_t)(int8_t)snd[id] : (int32_t)snd[id];
    if (id == DSP_REV || id == DSP_DEL)
        return (int32_t)sl_send(l, id - DSP_REV);
    if (id == DSP_KFX)
        return (int32_t)!sl_kfx_off(l);
    switch (id) {
    case DSP_LSPD: return lf->spd;
    case DSP_LDEP: return lf->dep;
    case DSP_LDST: return lf->dst;
    case DSP_LWAV: return lf->wav;
    case DSP_LSYN: return lf->syn;
    case DSP_LFAD: return lf->fad;
    case DSP_LSPH: return lf->sph;
    case DSP_LMOD: return lf->mode;
    default: break;
    }
    if (id < DSP_S0 || id >= DSP_S0 + DS_N || model != DM_SYN)
        return 0;
    switch (id - DSP_S0) {
    case DS_WAVE: return d->wave;
    case DS_NOISE: return (d->src & DN_CLAP) ? 5 : d->src & 15;
    case DS_TUNE: return d->pitch;
    case DS_FINE: return d->fine;
    case DS_SWEEP: return d->bend;
    case DS_SWTIME: return d->btime;
    case DS_HOLD: return d->hold;
    case DS_DECAY: return d->decay;
    case DS_TONE: return d->tlev;
    case DS_PART2: return d->t2;
    case DS_P2LVL: return d->t2lev;
    case DS_CLICK: return d->click;
    case DS_NLVL: return d->nlev;
    case DS_NHOLD: return d->nhold;
    case DS_NDEC: return d->ndec;
    case DS_FTYPE: { uint32_t m = d->flt & 7u; return m > 4u ? (int32_t)m - 1 : m == 4u ? 0 : (int32_t)m; }
    case DS_RES: return d->flt >> 3;
    case DS_FREQ: return d->fcut;
    case DS_FENV: return d->fenv;
    case DS_HPF: return d->hpf;
    case DS_CHIP: return d->chip;
    case DS_DRIVE: return d->drive;
    case DS_LEVEL: return ((int32_t)d->level - 128 + (d->level >= 128u ? 2 : -1)) / 4;
    default: return 0;
    }
}
/* id set to v (clamped to its range) in sound snd / LFO lf; not the sends (the caller's: drum_send, or a voice's own) */
static void dsp_put(uint32_t model, uint8_t *snd, dlfo_t *lf, uint32_t id, int32_t v)
{
    const param_desc_t *ds = dsp_desc(model, snd, id);
    dsnd_t *d = (dsnd_t *)snd;
    if (!ds)
        return;
    v = clamp(v, ds->min, ds->max);
    if (id < DSP_COUNT) {
        snd[id] = (uint8_t)v;
        dsu_fix_sound((hex_snd_t *)snd);         /* (a value the model has no use for: back in its range) */
        return;
    }
    switch (id) {
    case DSP_LSPD: lf->spd = (uint8_t)v; return;
    case DSP_LDEP: lf->dep = (int8_t)v; return;
    case DSP_LDST: lf->dst = (uint8_t)v; return;
    case DSP_LWAV: lf->wav = (uint8_t)v; return;
    case DSP_LSYN: lf->syn = (uint8_t)v; return;
    case DSP_LFAD: lf->fad = (int8_t)v; return;
    case DSP_LSPH: lf->sph = (uint8_t)v; return;
    case DSP_LMOD: lf->mode = (uint8_t)v; return;
    default: break;
    }
    if (id < DSP_S0 || id >= DSP_S0 + DS_N)
        return;
    switch (id - DSP_S0) {
    case DS_WAVE: d->wave = (uint8_t)v; break;
    case DS_NOISE: d->src = (uint8_t)(v == 5 ? DN_WHITE | DN_CLAP : v); break;
    case DS_TUNE: d->pitch = (uint8_t)v; break;
    case DS_FINE: d->fine = (uint8_t)v; break;
    case DS_SWEEP: d->bend = (uint8_t)v; break;
    case DS_SWTIME: d->btime = (uint8_t)v; break;
    case DS_HOLD: d->hold = (uint8_t)v; break;
    case DS_DECAY: d->decay = (uint8_t)v; break;
    case DS_TONE: d->tlev = (uint8_t)v; break;
    case DS_PART2: d->t2 = (uint8_t)v; break;
    case DS_P2LVL: d->t2lev = (uint8_t)v; break;
    case DS_CLICK: d->click = (uint8_t)v; break;
    case DS_NLVL: d->nlev = (uint8_t)v; break;
    case DS_NHOLD: d->nhold = (uint8_t)v; break;
    case DS_NDEC: d->ndec = (uint8_t)v; break;
    case DS_FTYPE: d->flt = (uint8_t)((d->flt & ~7u) | (uint32_t)(v >= 4 ? (v - 3) | DF_ALL : v)); break;
    case DS_RES: d->flt = (uint8_t)((d->flt & 7u) | (uint32_t)v << 3); break;
    case DS_FREQ: d->fcut = (uint8_t)v; break;
    case DS_FENV: d->fenv = (int8_t)v; break;
    case DS_HPF: d->hpf = (uint8_t)v; break;
    case DS_CHIP: d->chip = (uint8_t)v; break;
    case DS_DRIVE: d->drive = (uint8_t)v; break;
    case DS_LEVEL: d->level = (uint8_t)clamp(128 + v * 4, 0, 255); break;
    default: break;
    }
}
/* the value as the dials show it: v of id (desc d) into val, unit (the sends as a share of their range, as the KIT page
 * showed them) */
static void dsp_format(uint32_t id, const param_desc_t *d, int32_t v, char *val, const char **unit)
{
    if (id == DSP_REV || id == DSP_DEL) {
        fmt_int(val, v * 100 / d->max);
        *unit = "%";
        return;
    }
    param_format(d, v, val, unit);
}

/* ---- the pages: each machine's parameters in the Rytm's order, four a half (0xFF: an empty knob). (Wave 8, Ben 22 Nov 2026:
 * "same filter page on drums and synths") FLTR is the synth voice's FILT on both machines: CUTOFF RESO ENV TYPE on its first
 * half (the HEX machine has no filter envelope: that knob empty), DRIVE first on its second half */
enum { DPG_SRC, DPG_FLTR, DPG_AMP, DPG_LFO, DPG_N };
typedef struct { uint8_t pg, id[4]; } dhalf_t;
#define S_(k) (DSP_S0 + DS_##k)
static const dhalf_t DH_HEX[] = {
    {DPG_SRC, {DSP_COARSE, DSP_PAMT, DSP_PDEC, DSP_WAVE}},          /* TUNE, the pitch sweep, the oscillator */
    {DPG_SRC, {DSP_MODEL, DSP_AUX1, DSP_RATIO, DSP_FM}},            /* the machine, its modulator */
    {DPG_SRC, {DSP_CLICK, DSP_CLVL, DSP_CTONE, DSP_FINE}},          /* the transient; PSLOPE (DRUM, SNARE) / FINE */
    {DPG_FLTR, {DSP_CUT, DSP_RES, 0xFF, DSP_FTYPE}},
    {DPG_FLTR, {DSP_DRIVE, 0xFF, 0xFF, 0xFF}},
    {DPG_AMP, {DSP_ATK, DSP_DEC, DSP_SLOPE, DSP_AUX2}},             /* the envelope, DIST (the Rytm's OVR) */
    {DPG_AMP, {DSP_CRUSH, 0xFF, 0xFF, DSP_LEVEL}},                 /* (the drum effects under FX: DEL REV left for FX > SENDS) */
    {DPG_LFO, {DSP_LSPD, DSP_LDEP, DSP_LDST, DSP_LWAV}},
    {DPG_LFO, {DSP_LSYN, DSP_LFAD, DSP_LSPH, DSP_LMOD}},
};
static const dhalf_t DH_SYN[] = {
    {DPG_SRC, {S_(TUNE), S_(FINE), S_(WAVE), S_(TONE)}},
    {DPG_SRC, {S_(SWEEP), S_(SWTIME), S_(PART2), S_(P2LVL)}},
    {DPG_SRC, {S_(NOISE), S_(NLVL), S_(CLICK), S_(CHIP)}},
    {DPG_FLTR, {S_(FREQ), S_(RES), S_(FENV), S_(FTYPE)}},
    {DPG_FLTR, {S_(DRIVE), S_(HPF), 0xFF, 0xFF}},                   /* (DRIVE, then the noise's own high-pass) */
    {DPG_AMP, {S_(HOLD), S_(DECAY), S_(NHOLD), S_(NDEC)}},
    {DPG_AMP, {0xFF, 0xFF, 0xFF, S_(LEVEL)}},                        /* (LEVEL where the HEX machine has it; DEL REV: FX > SENDS) */
    {DPG_LFO, {DSP_LSPD, DSP_LDEP, DSP_LDST, DSP_LWAV}},
    {DPG_LFO, {DSP_LSYN, DSP_LFAD, DSP_LSPH, DSP_LMOD}},
};
#undef S_
#define NDH_HEX (sizeof DH_HEX / sizeof DH_HEX[0])
#define NDH_SYN (sizeof DH_SYN / sizeof DH_SYN[0])
static const dhalf_t *dh_tab(uint32_t model, uint32_t *n)
{
    *n = model == DM_SYN ? NDH_SYN : NDH_HEX;
    return model == DM_SYN ? DH_SYN : DH_HEX;
}
/* the halves page pg has on machine model, and the first one's index */
static uint32_t dh_count(uint32_t model, uint32_t pg, uint32_t *first)
{
    uint32_t n, i, c = 0;
    const dhalf_t *t = dh_tab(model, &n);
    *first = 0;
    for (i = 0; i < n; i++)
        if (t[i].pg == pg && !c++)
            *first = i;
    return c;
}
/* knob k of half h of page pg: its id, 0xFF none (h past the page's last: its last) */
static uint32_t dh_id(uint32_t model, uint32_t pg, uint32_t h, uint32_t k)
{
    uint32_t f, c = dh_count(model, pg, &f), n;
    const dhalf_t *t = dh_tab(model, &n);
    if (!c)
        return 0xFFu;
    return t[f + (h < c ? h : c - 1u)].id[k & 3u];
}

