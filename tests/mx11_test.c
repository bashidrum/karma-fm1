/* SPDX-License-Identifier: GPL-3.0-only */
/* PHEBUS wave 11: the mod matrix's storage, layout 2 (project.c mx_pack), against waves 5 to 10 (layout 1).
 *   render   matrices stored by waves 5 to 10 (layout 1: cells on the fixed columns, on AS1 and AS2 aimed at a filter, a
 *            macro, a send, an envelope time; a full matrix of 19 cells): each read (mx_unpack, as a project and a user
 *            preset read it) and played, a hash of the audio printed a line each. Built against wave 10's own firmware
 *            (tests/mx11_ref.sh), the same file gives wave 10's hashes: tests/mx11_ref.txt. Here: they must be the same
 *            (old projects and presets sound identical)
 *   pack     layout 1 read, written as layout 2, read again: every cell, column, LFO 2, Spice and Dice as they were
 *   many     19 cells on 19 different parameters (hold to map's limit): stored and read back exactly
 * Usage: mx11_test [REF]   (REF: the reference hashes to compare; none: print them) */
#include <stdint.h>
#define main hostsim_main
#include "hostsim.c"
#undef main
#define PROJ_HOST 1
static uint32_t trk_def_engine(uint32_t i) { return i < NPART ? ENGI_JUNO : 0u; }
#include "../firmware/src/project.c"
#ifndef MD_NFIX
#define MD_NFIX 11u
#endif
static int fails;
static void check(int ok, const char *what)
{
    printf("mx11: %-100s %s\n", what, ok ? "ok" : "FAIL");
    fails += !ok;
}
static void reset(void)
{
    uint32_t i, k;
    host_tracks_init();
    for (i = 0; i < 4u; i++) {
        int16_t *ln = ins[i].line;
        memset(&ins[i], 0, sizeof ins[i]);
        ins[i].line = ln;
    }
    memset(xst, 0, sizeof xst);
    memset(ins_lines, 0, sizeof ins_lines);
    memset(dly_buf, 0, sizeof dly_buf);
    memset(cho_buf, 0, sizeof cho_buf);
    memset(rev_line, 0, sizeof rev_line);
    memset(rev_ap, 0, sizeof rev_ap);
    memset(&fx, 0, sizeof fx);
    for (k = 0; k < NTRK; k++) {
        trk[k].ins_run = trk[k].tail = 0;
        trk[k].lfo_ph = 0, trk[k].lfo_val = 0, trk[k].lfo_fade = 32767, trk[k].lvl = 0;
        for (i = 0; i < NVOICE; i++)
            memset(&trk[k].v[i], 0, sizeof trk[k].v[i]);
        trk[k].nmono = trk[k].mono_note = 0;
    }
    rng_state = 0x1234567u;
    song.playing = 0;
    song.master_q12 = 2048;
    memset(&lim_dac, 0, sizeof lim_dac);
    lim_dac.env = LIM_T, lim_dac.g = 32768, lim_dac.hold = 0;
    trk[0].p[P_LEVEL] = 100;
    trk[0].p[P_ATK] = 0, trk[0].p[P_DEC] = 40, trk[0].p[P_SUS] = 90, trk[0].p[P_REL] = 30;
    trk[0].p[P_VOICE] = V_POLY;
    trk[0].p[P_FCUT] = 70, trk[0].p[P_FRES] = 40, trk[0].p[P_FDRV] = 30, trk[0].p[P_DLY] = 40, trk[0].p[P_LRATE] = 90;
    trk[0].p[P_L2RATE] = 70;
}
/* a layout 1 blob (waves 5 to 10): as[0], as[1], then cells (s, d on the 13 columns, amount) */
typedef struct { uint8_t as0, as1, n, c[19][3]; } blob1_t;
static void blob1(uint8_t *b, const blob1_t *x)
{
    uint32_t i;
    memset(b, 0, 46);
    b[0] = x->as0, b[1] = x->as1, b[3] = 1;            /* (LFO 2 FREE) */
    for (i = 0; i < x->n; i++) {
        b[8u + 2u * i] = (uint8_t)(x->c[i][0] * 13u + x->c[i][1] + 1u);
        b[9u + 2u * i] = x->c[i][2];
    }
}
static uint32_t render_hash(const uint8_t *b)
{
    static int32_t o[2u * CTL];
    uint32_t h = 2166136261u, q, j;
    reset();
    mx_unpack(b, 0);
    trk_note_on(&trk[0], 60, 100);
    trk_note_on(&trk[0], 67, 90);
    for (q = 0; q < 700u; q++) {
        if (q == 400u)
            trk_note_off(&trk[0], 60), trk_note_off(&trk[0], 67);
        mix_block(o, CTL);
        for (j = 0; j < 2u * CTL; j++)
            h = (h ^ (uint32_t)o[j]) * 16777619u;
    }
    return h;
}
#define C(s, d, a) {s, d, (uint8_t)(int8_t)(a)}
static const blob1_t B1[] = {
    {0, 0, 2, {C(MS_LFO2, MD_CUT, 40), C(MS_AENV, MD_RES, -30)}},
    {P_FDRV + 1, 0, 2, {C(MS_LFO1, 11, 50), C(MS_VEL, 11, -20)}},                        /* AS1: DRIVE */
    {P_DLY + 1, P_E1 + 1, 3, {C(MS_LFO2, 11, 60), C(MS_LFO1, 12, 40), C(MS_FENV, MD_PAN, 30)}},   /* AS1 DELAY, AS2 a macro */
    {P_REL + 1, P_GLIDE + 1, 3, {C(MS_KEY, 11, 30), C(MS_LFO2, 12, -50), C(MS_LFO1, MD_FX1, 20)}},
    {P_FDRV + 1, P_DLY + 1, 19, {C(0, 0, 10), C(1, 1, 11), C(2, 2, 12), C(3, 3, -13), C(4, 5, 14), C(5, 6, 15), C(6, 7, 16),
                                 C(0, 8, 17), C(1, 9, 18), C(2, 10, -19), C(3, 11, 20), C(4, 12, 21), C(5, 11, 22),
                                 C(6, 12, -23), C(1, 4, 24), C(2, 6, 25), C(3, 7, 26), C(1, 11, 27), C(2, 12, -28)}},   /* full */
};
#define NB1 (sizeof B1 / sizeof B1[0])
int main(int argc, char **argv)
{
    uint32_t i, h[NB1];
    uint8_t b[46];
    for (i = 0; i < NB1; i++) {
        blob1(b, &B1[i]);
        h[i] = render_hash(b);
        printf("render %u %08x\n", (unsigned)i, (unsigned)h[i]);
    }
#ifdef MXL2
    if (argc > 1) {                                     /* against wave 10's own hashes */
        FILE *f = fopen(argv[1], "r");
        uint32_t ok = f != 0, k, x;
        for (i = 0; f && i < NB1; i++)
            ok &= fscanf(f, "render %u %x\n", &k, &x) == 2 && k == i && x == h[i];
        if (f)
            fclose(f);
        check(ok, "old projects and presets (layout 1, waves 5 to 10) sound identical to wave 10 (render hashes, 5 matrices)");
    }
    {   /* layout 1 -> 2 -> back: every cell as it was */
        uint32_t ok = 1, s, d;
        for (i = 0; i < NB1; i++) {
            int32_t g[MS_N][MD_NFIX + 2];
            uint8_t b2[46];
            blob1(b, &B1[i]);
            reset();
            mx_unpack(b, 0);
            for (s = 0; s < MS_N; s++)
                for (d = 0; d < MD_NFIX + 2u; d++)
                    g[s][d] = mx_get(0, s, d);
            mxt[0].spice = 77, mxt[0].dice = 4321, mxt[0].l2sync = 7, mxt[0].l2trig = 2;
            mx_pack(b2, 0);
            ok &= b2[7] == MXL2;
            mx_clear_sound(0);
            mxt[0].spice = mxt[0].dice = 0;
            mx_unpack(b2, 0);
            for (s = 0; s < MS_N; s++)
                for (d = 0; d < MD_NFIX + 2u; d++)
                    ok &= g[s][d] == mx_get(0, s, d);
            ok &= mxt[0].spice == 77 && mxt[0].dice == 4321 && mxt[0].l2sync == 7 && mxt[0].l2trig == 2 && !mx_unpack_lost;
            ok &= mxt[0].as[0] == B1[i].as0 && (!B1[i].as1 || mxt[0].as[1] == B1[i].as1);
        }
        check(ok, "a layout 1 matrix written as layout 2 and read back: every cell, both columns, LFO 2, Spice, Dice as they were");
    }
    {   /* 19 cells on 19 different parameters (hold to map's most): stored exactly */
        static const uint8_t IDS[19] = {P_ATK, P_DEC, P_SUS, P_REL, P_FATK, P_FDEC, P_FSUS, P_FREL, P_FDRV, P_GLIDE, P_DETUNE,
                                        P_DLY, P_REV, P_CHOR, P_LRATE, P_LFADE, P_L2RATE, P_SLDEPTH, P_X1TIME};
        uint32_t ok = 1, k, n = 0;
        uint8_t b2[46];
        reset();
        mx_clear_sound(0);
        for (k = 0; k < 19u; k++)
            if (mx_map_set(0, MS_LFO2, IDS[k], (int32_t)k * 3 - 27 ? (int32_t)k * 3 - 27 : 5) == 0)
                n++;
            else
                printf("  parameter %u refused\n", (unsigned)IDS[k]);
        ok &= n == 19u && mx_used(0) == 19u;
        ok &= mx_map_set(0, MS_LFO1, P_PAN, 10) != 0;   /* (P_PAN: no knob, and the matrix is full anyway) */
        ok &= mx_map_set(0, MS_VEL, P_FCUT, 10) == -1;  /* (the 20th: MATRIX FULL; LFO 1 > CUTOFF would be the fixed cell) */
        mx_pack(b2, 0);
        mx_clear_sound(0);
        mx_unpack(b2, 0);
        for (k = 0; k < 19u; k++)
            ok &= mx_map_get(0, MS_LFO2, IDS[k]) == ((int32_t)k * 3 - 27 ? (int32_t)k * 3 - 27 : 5);
        check(ok && !mx_unpack_lost, "19 cells on 19 parameters: all stored and read back; the 20th refused (MATRIX FULL: 19 CELLS)");
    }
    printf("mx11: %s\n", fails ? "FAILED" : "all ok");
#else
    (void)argc, (void)argv;
#endif
    return fails;
}
