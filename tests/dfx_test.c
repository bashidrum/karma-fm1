/* SPDX-License-Identifier: GPL-3.0-only */
/* The drum effects under FX (Ben, 23 Nov 2026, 21:30 to 21:45: "Remove them" (the kit-wide drum sends), "a KIT FX switch per
 * sound"): old projects that used the kit-wide sends sound as they did.
 *   render   projects saved by wave 14 (made by wave 14's own code: tests/dfx_ref.sh, -DW14 against the tree before this
 *            change): the kit's reverb and delay sends with the sounds' own and send locks; one that clamps at 127; the
 *            same with the drum FILTER and INSERT on; the same with the drum SLICER on. Built with -DW14 it writes the
 *            projects, wave 14's hash of two bars of each and the SLICER case's audio (tests/dfx_ref/ *.fun8, hashes.txt,
 *            kit-sends-slicer.raw); built against this tree it loads them, plays the same two bars and compares: the hash
 *            (sample for sample), the SLICER case sample by sample (how far, and only how far, it moves)
 * Usage: dfx_test DIR */
#include <stdint.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
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
    printf("dfx: %-104s %s\n", what, ok ? "ok" : "FAIL");
    fails += !ok;
}
static const char *const NAMES[] = {"kit-sends", "kit-sends-clamp", "kit-sends-filter-insert", "kit-sends-slicer"};
#define NSC 4u
#define NBLK ((uint32_t)FS * 4u / CTL)
static int32_t aud[2u * CTL * ((uint32_t)FS * 4u / CTL + 1u)], ref[2u * CTL * ((uint32_t)FS * 4u / CTL + 1u)];
static void reset_audio(void)                     /* every state the audio keeps, as at power-on (as tests/kit12_test.c) */
{
    uint32_t i, k;
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
    memset(&drums, 0, sizeof drums);
    memset(&dlr, 0, sizeof dlr);
    drum_cap = NDRUM;
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
}
static void base(void)
{
    uint32_t i;
    host_tracks_init();
    for (i = 0; i < NTRK; i++)
        steps_clear(&trk[i]);
    dsu_defaults();
    song.g[G_BPM] = 120;
    song.g[G_CLOCK] = 0;
    song.rec = 0;
    TDRUM->p[P_SLEN] = 16;
    TDRUM->p[P_SDIV] = 2;
    drum_mute = drum_solo = 0;
    for (i = 0; i < NPART; i++)
        trk[i].p[P_LEVEL] = 0;
}
static uint32_t fnv(const int32_t *o, uint32_t n)
{
    uint32_t h = 2166136261u, i;
    for (i = 0; i < n; i++)
        h = (h ^ (uint32_t)o[i]) * 16777619u;
    return h;
}
static void render(int32_t *o)                    /* two bars, in a fork (every render from the same state) */
{
    pid_t pid;
    static int32_t *sh;
    size_t sz = sizeof aud;
    if (!sh)
        sh = mmap(0, sz, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (!(pid = fork())) {
        uint32_t b;
        reset_audio();
        transport_req = 1;
        for (b = 0; b < NBLK; b++)
            mix_block(sh + b * 2u * CTL, CTL);
        _exit(0);
    }
    waitpid(pid, 0, 0);
    memcpy(o, sh, 2u * CTL * NBLK * sizeof(int32_t));
}

#ifdef W14
/* ---- wave 14: the projects, as its pages made them (FX > SENDS: G_DRREV, G_DRDLY; AMP 2/2: a sound's REV 0..15, DEL 0..7) */
static void make(uint32_t sc)
{
    static const uint8_t P[5][16] = {
        /* KICK */ {110, 0, 0, 0, 100, 0, 0, 0, 120, 0, 0, 0, 100, 0, 0, 60},
        /* SNARE */ {0, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 0, 110, 0, 0, 0},
        /* CLAP */ {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 90, 0, 0, 0},
        /* HAT */ {80, 0, 70, 0, 80, 0, 0, 0, 80, 0, 70, 0, 80, 0, 70, 0},
        /* OHAT */ {0, 0, 0, 0, 0, 0, 90, 0, 0, 0, 0, 0, 0, 0, 100, 0}};
    static const uint8_t L[5] = {0, 2, 3, 4, 5};
    uint32_t l, i;
    base();
    pkit_load_kit(DRUM_DEFAULT_KIT);
    dpat_clear(&dpat);
    for (l = 0; l < 5u; l++)
        for (i = 0; i < 16u; i++)
            if (P[l][i])
                dt_set(i, L[l], P[l][i]);
    song.g[G_DRREV] = 40, song.g[G_DRDLY] = 30;
    sl_send_set(2, 0, 8), sl_send_set(2, 1, 2);   /* the snare's own */
    sl_send_set(4, 1, 5);                         /* the hat's own delay */
    dt_lk_set(12, 2, DSP_REV, 15);                /* the snare on step 13: its reverb send locked */
    dt_lk_set(2, 4, DSP_DEL, 1);
    if (sc == 1u) {                               /* a kit send that clamps: 120 + the snare's own (8 of 15: 67) */
        song.g[G_DRREV] = 120, song.g[G_DRDLY] = 110;
    } else if (sc == 2u) {
        TDRUM->p[P_TFLT] = -30;                   /* the drum FILTER low-passing, an INSERT */
        TDRUM->p[P_ITYPE] = 2, TDRUM->p[P_IMIX] = 80;
    } else if (sc == 3u) {
        TDRUM->p[P_SLCR] = SL_GATE;
    }
}
#endif

static project_t blob;
static int load_mem(void)
{
    proj_core_t *q = (proj_core_t *)proj_tmp.raw;
    base();
    dpat_clear(&dpat);
    memcpy(proj_tmp.raw, &blob, sizeof blob);
    if (!proj_import(q, proj_tmp.raw, (int)sizeof blob, PROJ_V7_TMP))
        return 0;
    proj_kit_take(proj_tmp.raw, (int)sizeof blob, q);
    proj_apply(q, 1);
    return 1;
}
static int rd(const char *dir, const char *nm, const char *ext, void *b, size_t n)
{
    char path[512];
    FILE *f;
    size_t g;
    snprintf(path, sizeof path, "%s/%s.%s", dir, nm, ext);
    if (!(f = fopen(path, "rb")))
        return 0;
    g = fread(b, 1, n, f);
    fclose(f);
    return g == n;
}

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : "tests/dfx_ref";
    uint32_t sc;
    const size_t na = 2u * CTL * NBLK * sizeof(int32_t);
#ifdef W14
    for (sc = 0; sc < NSC; sc++) {
        char path[512];
        FILE *f;
        make(sc);
        proj_capture(&blob);
        if (!load_mem())
            return 1;
        render(aud);
        snprintf(path, sizeof path, "%s/%s.fun8", dir, NAMES[sc]);
        f = fopen(path, "wb");
        if (!f || fwrite(&blob, 1, sizeof blob, f) != sizeof blob)
            return 1;
        fclose(f);
        if (sc == 3u) {
            snprintf(path, sizeof path, "%s/%s.raw", dir, NAMES[sc]);
            f = fopen(path, "wb");
            if (!f || fwrite(aud, 1, na, f) != na)
                return 1;
            fclose(f);
        }
        snprintf(path, sizeof path, "%s/hashes.txt", dir);
        f = fopen(path, sc ? "a" : "w");
        if (!f)
            return 1;
        fprintf(f, "%s %08x\n", NAMES[sc], fnv(aud, 2u * CTL * NBLK));
        fclose(f);
        printf("wrote %s %08x\n", NAMES[sc], fnv(aud, 2u * CTL * NBLK));
    }
    return 0;
#else
    for (sc = 0; sc < NSC; sc++) {
        char what[160];
        uint32_t i, d = 0, n = 2u * CTL * NBLK;
        double e = 0, er = 0, mx = 0;
        uint32_t want = 0;
        {
            char path[512], nm[64];
            unsigned hv;
            FILE *f;
            snprintf(path, sizeof path, "%s/hashes.txt", dir);
            if ((f = fopen(path, "r")) != 0) {
                while (fscanf(f, "%63s %x", nm, &hv) == 2)
                    if (!strcmp(nm, NAMES[sc]))
                        want = hv;
                fclose(f);
            }
        }
        if (!rd(dir, NAMES[sc], "fun8", &blob, sizeof blob) || (sc == 3u && !rd(dir, NAMES[sc], "raw", ref, na)) || !load_mem()) {
            check(0, NAMES[sc]);
            continue;
        }
        if (sc == 0u)
            check(song.g[G_DRREV] == 0 && song.g[G_DRDLY] == 0 && sl_send(2, 0) == 67u + 40u && sl_send(2, 1) == 36u + 30u &&
                  sl_send(4, 1) == 90u + 30u && sl_send(0, 0) == 40u && proj_kfold_n == 16u &&
                  dpat.lk[dt_lk(12, 2, DSP_REV)].val == 127u && dpat.lk[dt_lk(2, 4, DSP_DEL)].val == 18u + 30u,
                  "kit-sends: loaded: the kit's sends folded into every sound's own (the snare 67 + 40, the hat's delay 90 + 30), "
                  "its locks too (127, 48), the kit-wide sends 0");
        if (sc == 1u)
            check(sl_send(2, 0) == 127u && sl_send(0, 0) == 120u && sl_send(0, 1) == 110u,
                  "kit-sends-clamp: 120 + the snare's 67 clamps at 127, as the audio clamped it");
        render(aud);
        for (i = 0; sc == 3u && i < n; i++) {
            double q = (double)aud[i] - ref[i];
            d += aud[i] != ref[i];
            e += q * q;
            er += (double)ref[i] * ref[i];
            mx = fabs(q) > mx ? fabs(q) : mx;
        }
        if (sc < 3u) {
            snprintf(what, sizeof what, "%s: a wave 14 project sounds as wave 14 played it, sample for sample (hash %08x)",
                     NAMES[sc], fnv(aud, n));
            check(want && fnv(aud, n) == want, what);
        } else {
            double db = er > 0 && e > 0 ? 10.0 * log10(e / er) : -999.0;
            snprintf(what, sizeof what, "%s: NOT exact with the drum SLICER on: %.1f dB under the signal (the kit's send share, "
                     "sliced before, now goes in before the SLICER); bounded under -15 dB", NAMES[sc], db);
            check(db < -15.0, what);
            printf("dfx:   slicer case: %u of %u samples differ, %.1f dB under, largest %.0f\n", (unsigned)d, (unsigned)n, db, mx);
        }
        {   /* saved again (no kit-wide sends, sends 0..127) and loaded: the same */
            static int32_t a2[sizeof aud / sizeof aud[0]];
            proj_capture(&blob);
            load_mem();
            render(a2);
            snprintf(what, sizeof what, "%s: saved in the new format and loaded again: the same sound", NAMES[sc]);
            check(!memcmp(a2, aud, na) && ((const proj_core_t *)&blob)->rsv[0] == PROJ_DSEND_V &&
                  ((const pkit_t *)blob.x.kit)->ver == PK_VER, what);
        }
    }
    {   /* the KIT FX switch: saved and loaded with the project */
        pkit_load_kit(DRUM_DEFAULT_KIT);
        sl_kfx_set(4, 1);
        proj_capture(&blob);
        pkit_load_kit(DRUM_DEFAULT_KIT);
        load_mem();
        check(sl_kfx_off(4) == 1u && sl_kfx_off(0) == 0u && sl_send(4, 0) == 0u, "KIT FX OFF on the HAT: saved with the project and loaded");
    }
    printf("dfx: %s (%d failed)\n", fails ? "FAILED" : "all ok", fails);
    return fails != 0;
#endif
}
