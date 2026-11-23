/* SPDX-License-Identifier: GPL-3.0-only */
/* PHEBUS wave 12: the kit is 16 slots, each any sound of any kit, the machine per slot (Ben, 23 Nov 2026, 11:02 and 11:10).
 *   other machine  a 606 slot takes a HEX sound: it plays exactly as in its own kit, the other 15 exactly as before
 *   mixed kit      three HEX and three CLASSIC slots hit at once: the drum voice's output is the sum of each alone, each
 *                  voice on its slot's machine
 *   the 808 snare  in the 606 kit (Ben's example): it sounds as the 808's snare
 *   chokes         the HAT and PEDAL slots cut the OPEN HAT slot, whatever machines sit in them
 *   lists          the sound list of several slots (INIT HEX, INIT CLASSIC, then the slot's kind from every kit, then all),
 *                  its wrap; the kit list; the INIT sounds plain, audible and as loud as a kit's
 *   locks          a machine change carries the locks with an equivalent (CUTOFF, DECAY ..), drops the others (counted),
 *                  keeps the shared ones (sends, LFO, NOTE); back to the first machine: every lock as it was
 *   user kits      stored and loaded whole (both machines in one kit); an old MY HEX kit loads as it played
 * Exit status: the number of failed checks. */
#define main hostsim_main
#include <stddef.h>
#include "hostsim.c"
#undef main
#define PROJ_HOST 1
static uint32_t trk_def_engine(uint32_t i) { return i < NPART ? ENGI_JUNO : 0u; }
#include "../firmware/src/project.c"

static int fails;
static void check(int ok, const char *what)
{
    printf("kitmix: %-104s %s\n", what, ok ? "ok" : "FAIL");
    fails += !ok;
}
static int32_t ml[CTL], mr[CTL], rv[CTL], dl[CTL];
static void quiet(void)
{
    memset(&drums, 0, sizeof drums);
    memset(&dlr, 0, sizeof dlr);
    drum_cap = NDRUM;
    rng_state = 0x2468ACEu;
    lx_rng_state = 0x2545F491u;                   /* (the HEX voices' noise: its own, kept between hits) */
    song.g[G_DRLVL] = 100;
    song.g[G_DRREV] = song.g[G_DRDLY] = 0;
    TDRUM->p[P_PAN] = 0;
}
/* hits of the lanes in mask, at once, rendered n blocks into buf (dry, the drum voice's own mix); a hash */
static uint32_t hits(uint32_t mask, uint32_t n, int32_t *buf)
{
    uint32_t b, i, l, h = 2166136261u;
    quiet();
    for (l = 0; l < DRUM_LANES; l++)
        if ((mask >> l) & 1u)
            drum_on(LANE_NOTE[l], 100);
    for (b = 0; b < n; b++) {
        memset(ml, 0, sizeof ml), memset(mr, 0, sizeof mr), memset(rv, 0, sizeof rv), memset(dl, 0, sizeof dl);
        drums_render(ml, mr, rv, dl, CTL);
        for (i = 0; i < CTL; i++) {
            if (buf)
                buf[b * CTL + i] = ml[i];
            h = (h ^ (uint32_t)ml[i]) * 16777619u;
        }
    }
    return h;
}
static double rms(const int32_t *b, uint32_t n)
{
    double e = 0;
    uint32_t i;
    for (i = 0; i < n; i++)
        e += (double)b[i] * b[i];
    return sqrt(e / n);
}
#define NB 2000u
static int32_t A[NB * CTL], B[NB * CTL], C[NB * CTL];

static void t_other_machine(void)
{
    uint32_t l, ok = 1, h606[DRUM_LANES], hx, hhex;
    dslot_t d;
    kit_set(DRUM_DEFAULT_KIT);
    for (l = 0; l < DRUM_LANES; l++)
        h606[l] = hits(1u << l, 400, 0);
    kit_set(0);                                   /* HEX BOOM: its snare as its kit plays it */
    hhex = hits(1u << 2, 400, 0);
    kit_set(DRUM_DEFAULT_KIT);
    slot_from_kit(&d, 0, 2);
    pkit.s[2] = d;
    hx = hits(1u << 2, 400, 0);
    for (l = 0; l < DRUM_LANES; l++)
        if (l != 2u)
            ok &= hits(1u << l, 400, 0) == h606[l];
    check(sl_mach(2) == DM_HEX && sl_mach(0) == DM_SYN && hx == hhex,
          "the 606's SNARE slot takes HEX BOOM's snare (the other machine): it plays as in HEX BOOM");
    check(ok, "the other 15 slots play exactly as the 606's");
    {   /* Ben's example: the 808's snare in the 606 */
        uint32_t h808;
        kit_set(DRUM_SAMPLED);
        h808 = hits(1u << 2, 400, 0);
        kit_set(DRUM_DEFAULT_KIT);
        slot_from_kit(&d, DRUM_SAMPLED, 2);
        pkit.s[2] = d;
        check(hits(1u << 2, 400, 0) == h808 && hits(1u, 400, 0) == h606[0],
              "the 808 SNARE in the 606 kit sounds as the 808's snare; the 606's kick as the 606's");
    }
}
static void t_mixed(void)
{
    static const uint8_t L[6] = {0, 2, 4, 7, 9, 13};
    uint32_t i, k, same = 1, mach_ok = 1;
    kit_set(DRUM_DEFAULT_KIT);
    for (i = 0; i < 6u; i += 2u)                  /* KICK, HAT, LOW TOM from HEX METAL; SNARE, RIM, SHAKER the 606's */
        slot_from_kit(&pkit.s[L[i]], 2, L[i]);
    memset(C, 0, sizeof C);
    for (i = 0; i < 6u; i++) {                   /* (each alone, its noise seeded as it is among the six: the hits before it
                                                  * started and let go unheard, so the random numbers they take are taken) */
        uint32_t b, j, q;
        quiet();
        for (j = 0; j < i; j++)
            drum_on(LANE_NOTE[L[j]], 100);
        for (q = 0; q < NDRUM; q++)
            drums.v[q].active = 0;
        drums.tail = drums.trev = drums.tdly = 0;
        drum_on(LANE_NOTE[L[i]], 100);
        for (b = 0; b < NB; b++) {
            memset(ml, 0, sizeof ml), memset(mr, 0, sizeof mr), memset(rv, 0, sizeof rv), memset(dl, 0, sizeof dl);
            drums_render(ml, mr, rv, dl, CTL);
            for (q = 0; q < CTL; q++)
                A[b * CTL + q] = ml[q];
        }
        for (k = 0; k < NB * CTL; k++)
            C[k] += A[k];
    }
    quiet();
    for (i = 0; i < 6u; i++)
        drum_on(LANE_NOTE[L[i]], 100);
    for (i = 0; i < NB; i++) {
        memset(ml, 0, sizeof ml), memset(mr, 0, sizeof mr), memset(rv, 0, sizeof rv), memset(dl, 0, sizeof dl);
        drums_render(ml, mr, rv, dl, CTL);
        for (k = 0; k < CTL; k++)
            B[i * CTL + k] = ml[k];
    }
    for (k = 0; k < NB * CTL; k++)
        same &= B[k] == C[k];
    quiet();
    for (i = 0; i < 6u; i++)
        drum_on(LANE_NOTE[L[i]], 100);
    for (k = 0; k < NDRUM; k++)
        mach_ok &= drums.v[k].active && drums.synth[k] == (sl_mach(drums.lane[k]) == DM_HEX ? 2u : 1u) &&
                   drums.trim[k] == SL_TRIM(pkit.s[drums.lane[k]].mach);
    check(mach_ok, "a mixed kit: each voice plays its slot's machine (HEX METAL's 3 slots HEX, the 606's 3 CLASSIC) and its trim");
    check(same && rms(B, NB * CTL) > 100.0, "a mixed kit, six slots hit at once: the output is exactly the sum of each alone");
}
static void t_choke(void)
{
    uint32_t k, open = 0;
    kit_set(DRUM_DEFAULT_KIT);
    slot_from_kit(&pkit.s[5], 0, 5);              /* OPEN HAT: HEX BOOM's (HEX); HAT stays the 606's (CLASSIC) */
    quiet();
    drum_on(LANE_NOTE[5], 100);
    hits(0, 0, 0);
    drum_on(LANE_NOTE[5], 100);
    for (k = 0; k < 20u; k++)
        drums_render(ml, mr, rv, dl, CTL);
    drum_on(LANE_NOTE[4], 100);
    for (k = 0; k < NDRUM; k++)
        open += drums.v[k].active && drums.v[k].note == LANE_NOTE[5];
    check(!open, "chokes: the HAT slot (a CLASSIC sound) cuts the OPEN HAT slot (a HEX sound)");
    drum_on(LANE_NOTE[5], 100);
    drum_on(LANE_NOTE[6], 100);
    for (k = 0, open = 0; k < NDRUM; k++)
        open += drums.v[k].active && drums.v[k].note == LANE_NOTE[5];
    check(!open, "chokes: the PEDAL slot cuts it too");
}
static void t_lists(void)
{
    uint32_t src, kind, s = kit_src_n(), n = snd_n(), e, ok = 1, L;
    char b[32], g[16];
    uint32_t i, sz;
    check(s == 37u + 4u, "the kits: 5 HEX, 32 CLASSIC, then MY HEX 1 to 4 (the user kits not stored over)");
    check(n == 2u + 16u * s, "the sounds: 2 INIT, then 16 kinds of every kit");
    for (L = 0; L < DRUM_LANES; L += 5u) {         /* KICK, OPEN HAT, LOW TOM, CONGA */
        snd_at(L, 0, &src, &kind), ok &= src == DSRC_INIT && kind == DM_HEX;
        snd_at(L, 1, &src, &kind), ok &= src == DSRC_INIT && kind == DM_SYN;
        for (e = 2; e < 2u + s; e++)
            snd_at(L, e, &src, &kind), ok &= kind == L && src == kit_src(e - 2u);
        snd_at(L, 2u + s, &src, &kind), ok &= kind == (L ? 0u : 1u);   /* then the other kinds from KICK */
        snd_at(L, n - 1u, &src, &kind), ok &= kind == (L == 15u ? 14u : 15u) && src == kit_src(s - 1u);
    }
    check(ok, "the sound list for KICK, OPEN HAT, LOW TOM, CONGA: INIT HEX, INIT CLASSIC, its kind from every kit in kit "
              "order, then every other kind from KICK on");
    snd_name_at(2, 0, b, sizeof b), ok = str_eq(b, "INIT HEX");
    snd_name_at(2, 1, b, sizeof b), ok &= str_eq(b, "INIT CLASSIC");
    snd_name_at(2, 2, b, sizeof b), ok &= str_eq(b, "HEX BOOM SNARE");
    snd_name_at(2, 2u + 5u, b, sizeof b), ok &= str_eq(b, "808 SNARE");
    snd_name_at(2, 2u + s, b, sizeof b), ok &= str_eq(b, "HEX BOOM KICK");
    snd_group(2, 2u + 5u, g, sizeof g, &i, &sz), ok &= str_eq(g, "SNARE") && i == 6u && sz == s;
    snd_group(2, 1, g, sizeof g, &i, &sz), ok &= str_eq(g, "INIT") && i == 2u && sz == 2u;
    check(ok, "the names: INIT HEX, INIT CLASSIC, HEX BOOM SNARE, 808 SNARE (6/41 of SNARE), HEX BOOM KICK");
    kit_set(DRUM_DEFAULT_KIT);
    check(snd_pos(2) == (int32_t)(2u + 7u) && snd_pos(0) == (int32_t)(2u + 7u), "the 606's snare and kick: where they stand in their slots' lists");
    {   /* the INITs: plain, audible, bounded, near a kit's level */
        static const uint8_t Ls[3] = {0, 2, 4};
        uint32_t m, j, okl = 1;
        for (m = 0; m < 2u; m++)
            for (j = 0; j < 3u; j++) {
                double r, rk;
                uint32_t k, peak = 0;
                kit_set(m ? DRUM_DEFAULT_KIT : 0u);
                hits(1u << Ls[j], 600, A);
                rk = rms(A, 600 * CTL);
                slot_init(&pkit.s[Ls[j]], m, Ls[j]);
                hits(1u << Ls[j], 600, A);
                r = rms(A, 600 * CTL);
                for (k = 0; k < 600u * CTL; k++)
                    peak = (uint32_t)(A[k] < 0 ? -A[k] : A[k]) > peak ? (uint32_t)(A[k] < 0 ? -A[k] : A[k]) : peak;
                printf("  INIT %s on %s: rms %.0f (the kit's own %.0f), peak %u\n", DM_NAME[m], KIND_NAME[Ls[j]], r, rk, peak);
                okl &= r > rk / 4.0 && r < rk * 4.0 && peak < 40000u && sl_mach(Ls[j]) == m;
            }
        check(okl, "INIT HEX and INIT CLASSIC on a kick, a snare, a hat: sounding, bounded, within 12 dB of the kit's own");
    }
}
static void t_locks(void)
{
    static dlk_t org[DT_NLK];
    pkit_t o;
    dslot_t d;
    uint32_t kept, dropped, ok;
    int k;
    kit_set(DRUM_DEFAULT_KIT);
    dpat_clear(&dpat);
    dt_set(4, 2, 100);
    dt_lk_set(4, 2, DSP_S0 + DS_FREQ, 127);       /* CUTOFF (top), DECAY, NLVL (no HEX knob), the LFO's DEPTH, a send, NOTE */
    dt_lk_set(4, 2, DSP_S0 + DS_DECAY, 0);
    dt_lk_set(4, 2, DSP_S0 + DS_NLVL, 99);
    dt_lk_set(4, 2, DSP_LDEP, (uint8_t)(int8_t)-20);
    dt_lk_set(4, 2, DSP_REV, 9);
    dt_lk_set(4, 2, DSP_NOTE, 5);
    dt_lk_set(4, 0, DSP_S0 + DS_DECAY, 33);       /* (another slot's: untouched) */
    memcpy(org, dpat.lk, sizeof org);
    o = pkit;
    slot_from_kit(&d, 0, 2);                      /* HEX BOOM's snare */
    slot_take(2, &d, org, o.s[2].mach & 1u, o.s[2].snd, &kept, &dropped);
    ok = kept == 2u && dropped == 1u && dt_lk(4, 2, DSP_CUT) >= 0 && dpat.lk[dt_lk(4, 2, DSP_CUT)].val == 127 &&
         dt_lk(4, 2, DSP_DEC) >= 0 && dpat.lk[dt_lk(4, 2, DSP_DEC)].val == 0 && dt_lk(4, 2, DSP_S0 + DS_NLVL) < 0 &&
         dt_lk(4, 2, DSP_LDEP) >= 0 && dt_lk(4, 2, DSP_REV) >= 0 && dt_lk(4, 2, DSP_NOTE) >= 0 &&
         dt_lk(4, 0, DSP_S0 + DS_DECAY) >= 0;
    check(ok, "locks, CLASSIC to HEX: CUTOFF and DECAY go over (2 kept), N.LVL has none (1 dropped), LFO send NOTE stay");
    {
        uint32_t nid, nv;
        ok = lk_carry(DSP_S0 + DS_RES, 31, DM_SYN, o.s[2].snd, DM_HEX, d.snd, &nid, &nv) && nid == DSP_RES && nv == 127u;
        ok &= lk_carry(DSP_S0 + DS_FTYPE, 3, DM_SYN, o.s[2].snd, DM_HEX, d.snd, &nid, &nv) && nid == DSP_FTYPE && nv == HF_HP;
        ok &= !lk_carry(DSP_S0 + DS_FTYPE, 4, DM_SYN, o.s[2].snd, DM_HEX, d.snd, &nid, &nv);   /* LP+: no such TYPE on HEX */
        ok &= lk_carry(DSP_S0 + DS_LEVEL, (uint8_t)(int8_t)-32, DM_SYN, o.s[2].snd, DM_HEX, d.snd, &nid, &nv) && nv == 0u;
    }
    check(ok, "locks: the same share of the range (RESO 31 of 31 -> 127), a TYPE by its name (HP -> HP; LP+ none), LEVEL's bottom");
    slot_take(2, &o.s[2], org, o.s[2].mach & 1u, o.s[2].snd, &kept, &dropped);
    check(!memcmp(org, dpat.lk, sizeof org) || (dt_lk(4, 2, DSP_S0 + DS_NLVL) >= 0 && kept == 0u && dropped == 0u),
          "locks: the 606 snare back: every lock as it was (N.LVL too)");
    k = dt_lk(4, 2, DSP_S0 + DS_FREQ);
    check(k >= 0 && dpat.lk[k].val == 127, "locks: and their values");
}
static void t_user_kits(void)
{
    pkit_t k, m;
    uint32_t ha, hb;
    kit_set(DRUM_DEFAULT_KIT);
    slot_from_kit(&pkit.s[0], 0, 0);              /* a HEX kick in the 606: both machines in one kit */
    pkit.s[3].send = 5;
    ha = hits(0x0Du, 600, 0);
    check(ukit_state(5) == UK_NONE && ukit_put(5, &pkit) == 0 && ukit_state(5) == UK_STORED, "user kits: MY KIT 6 stored");
    kit_set(DRUM_SAMPLED);
    check(kit_make(DSRC_USER + 5u, &k), "user kits: MY KIT 6 read back");
    pkit = k;
    hb = hits(0x0Du, 600, 0);
    check(ha == hb && sl_mach(0) == DM_HEX && sl_mach(2) == DM_SYN && pkit.s[3].send == 5u && !kit_changed(),
          "user kits: loaded, it plays as it did (a HEX kick beside the 606's sounds, a send), unchanged");
    check(kit_pos(DSRC_USER + 5u) == (int32_t)(37u + 4u), "user kits: listed after the built-in kits and MY HEX 1 to 4");
    {   /* an old MY HEX kit (MY HEX 2: its defaults, HEX ACID's sounds) loads as it played: HEX ACID's trim */
        kit_set(1);
        ha = hits(0x7u, 600, 0);
        check(kit_make(DRUM_SYN + 1u, &m), "MY HEX 2 made as a whole kit");
        pkit = m;
        hb = hits(0x7u, 600, 0);
        check(ha == hb && pkit.base == DRUM_SYN + 1u, "MY HEX 2 (as at power-on: HEX ACID) sounds as HEX ACID, as it did");
    }
    check(ukit_put(5, 0) == 0 && ukit_state(5) == UK_ERASED && kit_pos(DSRC_USER + 5u) < 0, "user kits: erased: gone from the list");
}

int main(void)
{
    host_tracks_init();
    dsu_defaults();
    t_other_machine();
    t_mixed();
    t_choke();
    t_lists();
    t_locks();
    t_user_kits();
    printf("kitmix: %s\n", fails ? "FAILED" : "all ok");
    return fails;
}
