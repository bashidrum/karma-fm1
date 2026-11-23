/* SPDX-License-Identifier: GPL-3.0-only */
/* PHEBUS wave 12 (Ben, 23 Nov 2026, 11:02 and 11:10: the kit is 16 slots in the project, each any sound of any kit): old
 * projects sound as wave 11 played them.
 *   render   projects saved by wave 11 (tests/kit12_ref/ *.fun8, made by wave 11's own code: tests/kit12_ref.sh): the
 *            default 606 pattern; a HEX kit with locks; an edited 606* with locks; the 808 with locks; MY HEX 2; a 606*
 *            copy kept while the 808 plays. Each loaded as the FM-1 loads its working project (core and ext), its drum
 *            pattern played for two bars, a hash of the audio a line. Built with -DW11 against wave 11 (tests/kit12_ref.sh)
 *            it writes the projects and prints wave 11's hashes (tests/kit12_ref.txt); built against this tree it loads
 *            the same projects and compares: the sound must be identical.
 * Usage: kit12_test DIR [REF]   (DIR: where the .fun8 files are; REF: the reference hashes; W11: writes DIR, prints) */
#include <stdint.h>
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
    printf("kit12: %-100s %s\n", what, ok ? "ok" : "FAIL");
    fails += !ok;
}
static const char *const NAMES[] = {"606-default", "hex-metal-locks", "606-edited-locks", "808-locks", "myhex2-locks",
                                    "606-copy-808-plays"};
#define NSC 6u
static void reset_audio(void)                     /* every state the audio keeps, as at power-on */
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
static void base(void)                            /* an empty project, the drums only, 1/16 steps, 120 BPM */
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
static uint32_t render_hash(void)                 /* two bars of the pattern, the audio hashed (FNV-1a) */
{
    static int32_t o[2u * CTL];
    uint32_t h = 2166136261u, b, j, n = (uint32_t)FS * 4u / CTL;
    reset_audio();
    transport_req = 1;
    for (b = 0; b < n; b++) {
        mix_block(o, CTL);
        for (j = 0; j < 2u * CTL; j++)
            h = (h ^ (uint32_t)o[j]) * 16777619u;
    }
    transport_req = 2;
    for (b = 0; b < 8u; b++)
        mix_block(o, CTL);
    return h;
}

/* the same, in a fork of this process as it is (every render from the same state the process had before any render: what a
 * render leaves behind, the sequencer's clock and the effects' running state, never reaches the next one) */
static uint32_t render_iso(void)
{
    int fd[2];
    uint32_t h = 0;
    pid_t pid;
    if (pipe(fd))
        return 0;
    if (!(pid = fork())) {
        h = render_hash();
        if (write(fd[1], &h, sizeof h) != (ssize_t)sizeof h)
            _exit(1);
        _exit(0);
    }
    close(fd[1]);
    if (read(fd[0], &h, sizeof h) != (ssize_t)sizeof h)
        h = 0;
    close(fd[0]);
    waitpid(pid, 0, 0);
    return h;
}

#ifdef W11
/* ---- wave 11: the projects, as its UI and sequencer make them */
static void pattern(void)                         /* every sound: hats, chokes (PEDAL after the open hat), velocities */
{
    static const uint8_t P[DRUM_LANES][16] = {
        /* KICK */ {110, 0, 0, 0, 100, 0, 0, 0, 120, 0, 0, 0, 100, 0, 0, 60},
        /* KICK2 */ {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 90},
        /* SNARE */ {0, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 0, 110, 0, 0, 0},
        /* CLAP */ {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 90, 0, 0, 0},
        /* HAT */ {80, 0, 70, 0, 80, 0, 0, 0, 80, 0, 70, 0, 80, 0, 70, 0},
        /* OHAT */ {0, 0, 0, 0, 0, 0, 90, 0, 0, 0, 0, 0, 0, 0, 100, 0},
        /* PEDAL */ {0, 0, 0, 0, 0, 0, 0, 80, 0, 0, 0, 0, 0, 0, 0, 0},
        /* RIM */ {0, 0, 0, 70, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        /* SNARE2 */ {0, 0, 0, 0, 0, 60, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        /* LOTOM */ {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 100, 0, 0},
        /* HITOM */ {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        /* CRASH */ {100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        /* RIDE */ {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 90, 0, 0, 0, 0, 0},
        /* SHAKER */ {0, 50, 0, 50, 0, 50, 0, 50, 0, 50, 0, 50, 0, 50, 0, 50},
        /* CONGA */ {0, 0, 0, 0, 0, 0, 0, 0, 0, 90, 0, 0, 0, 0, 0, 0},
        /* CBELL */ {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 80, 0, 0, 0, 0}};
    uint32_t l, i;
    dpat_clear(&dpat);
    for (l = 0; l < DRUM_LANES; l++)
        for (i = 0; i < 16u; i++)
            if (P[l][i])
                dt_set(i, l, P[l][i]);
    dpat.vel[14][10] = 95;                        /* HI TOM */
}
static void make(uint32_t sc)
{
    dtx_t *x;
    base();
    memset(&pkit, 0, sizeof pkit);
    memset(drum_send, 0, sizeof drum_send);
    pattern();
    switch (sc) {
    case 0:                                       /* the default 606 pattern */
        TDRUM->p[P_E0] = DRUM_DEFAULT_KIT;
        break;
    case 1:                                       /* HEX METAL, HEX locks, a CLASSIC lock that does nothing, sends */
        TDRUM->p[P_E0] = 2;
        dt_lk_set(4, 2, DSP_DEC, 60);
        dt_lk_set(12, 2, DSP_CUT, 40);
        dt_lk_set(0, 0, DSP_COARSE, 36);
        dt_lk_set(8, 0, DSP_LDEP, (uint8_t)(int8_t)40);
        dt_lk_set(8, 0, DSP_LSPD, 100);
        dt_lk_set(6, 5, DSP_REV, 10);
        dt_lk_set(2, 4, DSP_NOTE, (uint8_t)(int8_t)-5);
        dt_lk_set(4, 0, DSP_S0 + DS_DECAY, 100);
        dt_lk_set(13, 9, DSP_FTYPE, HF_BP);
        drum_send[2][0] = 8, drum_send[2][1] = 3;
        drum_send[5][1] = 5;
        if ((x = dt_x_make(10, 4)) != 0)
            x->rrate = 6, x->rlen = 32;
        break;
    case 2:                                       /* the 606 edited (its copy), CLASSIC locks, a HEX lock that does nothing */
        TDRUM->p[P_E0] = DRUM_DEFAULT_KIT;
        pkit_make();
        dsp_put(pkit.model, pkit.snd[2], &pkit.lfo[2], DSP_S0 + DS_DECAY, 90);
        dsp_put(pkit.model, pkit.snd[6], &pkit.lfo[6], DSP_S0 + DS_TUNE, 70);   /* (PEDAL: its own) */
        dsp_put(pkit.model, pkit.snd[4], &pkit.lfo[4], DSP_LDEP, 30);
        dsp_put(pkit.model, pkit.snd[4], &pkit.lfo[4], DSP_LSPD, 90);
        dsp_put(pkit.model, pkit.snd[0], &pkit.lfo[0], DSP_S0 + DS_FREQ, 80);
        dt_lk_set(4, 2, DSP_S0 + DS_FREQ, 50);
        dt_lk_set(12, 2, DSP_S0 + DS_TUNE, 64);
        dt_lk_set(7, 6, DSP_S0 + DS_TUNE, 72);
        dt_lk_set(0, 0, DSP_LDEP, (uint8_t)(int8_t)20);
        dt_lk_set(0, 0, DSP_DEC, 10);
        dt_lk_set(9, 14, DSP_NOTE, (uint8_t)(int8_t)7);
        dt_lk_set(12, 3, DSP_DEL, 6);
        drum_send[0][0] = 5, drum_send[0][1] = 2;
        drum_send[6][0] = 12;
        break;
    case 3:                                       /* the 808, CLASSIC locks, sends */
        TDRUM->p[P_E0] = DRUM_SAMPLED;
        dt_lk_set(4, 2, DSP_S0 + DS_NLVL, 120);
        dt_lk_set(8, 0, DSP_S0 + DS_DECAY, 30);
        dt_lk_set(6, 5, DSP_S0 + DS_HPF, 60);
        drum_send[3][0] = 9;
        drum_send[12][1] = 4;
        break;
    case 4:                                       /* MY HEX 2 (its defaults: HEX ACID) */
        TDRUM->p[P_E0] = DRUM_SYN + 1u;
        dt_lk_set(4, 2, DSP_DRIVE, 90);
        dt_lk_set(0, 0, DSP_DEC, 50);
        drum_send[4][1] = 3;
        break;
    default:                                      /* a 606 copy, but the 808 plays */
        TDRUM->p[P_E0] = DRUM_DEFAULT_KIT;
        pkit_make();
        dsp_put(pkit.model, pkit.snd[2], &pkit.lfo[2], DSP_S0 + DS_DECAY, 100);
        TDRUM->p[P_E0] = DRUM_SAMPLED;
        dt_lk_set(4, 2, DSP_S0 + DS_DECAY, 20);
        break;
    }
}
#endif

static project_t blob;
static int load_mem(void)                         /* blob as the working project, as at power-on */
{
    proj_core_t *q = (proj_core_t *)proj_tmp.raw;
    base();
    dpat_clear(&dpat);
    memcpy(proj_tmp.raw, &blob, sizeof blob);
    if (!proj_import(q, proj_tmp.raw, (int)sizeof blob, PROJ_V7_TMP))
        return 0;
#ifdef W11
    proj_kit_take(proj_tmp.raw, (int)sizeof blob);
#else
    proj_kit_take(proj_tmp.raw, (int)sizeof blob, q);
#endif
    proj_apply(q, 1);
    return 1;
}
static int load(const char *dir, uint32_t sc)     /* the working project from its file */
{
    char path[512];
    FILE *f;
    size_t n;
    snprintf(path, sizeof path, "%s/%s.fun8", dir, NAMES[sc]);
    if (!(f = fopen(path, "rb")))
        return 0;
    n = fread(&blob, 1, sizeof blob, f);
    fclose(f);
    return n == sizeof blob && load_mem();
}

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : "tests/kit12_ref";
    uint32_t sc, h[NSC];
    char want[NSC][64];
    int nref = 0;
#ifdef W11
    for (sc = 0; sc < NSC; sc++) {
        char path[512];
        FILE *f;
        make(sc);
        proj_capture(&blob);
        snprintf(path, sizeof path, "%s/%s.fun8", dir, NAMES[sc]);
        if (!(f = fopen(path, "wb")) || fwrite(&blob, 1, sizeof blob, f) != sizeof blob) {
            printf("kit12: cannot write %s\n", path);
            return 1;
        }
        fclose(f);
    }
#endif
    if (argc > 2) {
        FILE *f = fopen(argv[2], "r");
        char line[160];
        while (f && fgets(line, sizeof line, f) && nref < (int)NSC)
            if (!strncmp(line, "render ", 7))
                sscanf(line + 7, "%63s", want[nref++]);
        if (f)
            fclose(f);
    }
    for (sc = 0; sc < NSC; sc++) {
        char got[64];
        if (!load(dir, sc)) {
            check(0, NAMES[sc]);
            continue;
        }
        h[sc] = render_iso();
        snprintf(got, sizeof got, "%s=%08x", NAMES[sc], h[sc]);
        printf("render %s\n", got);
        if (nref) {
            char what[128];
            snprintf(what, sizeof what, "a wave 11 project sounds as wave 11 played it: %s", NAMES[sc]);
            check(sc < (uint32_t)nref && !strcmp(want[sc], got), what);
        }
#ifndef W11
        {   /* saved again (wave 12's format: the 16 slots in the ext) and loaded: the same sound, the same locks */
            static dpat_t keep;
            char what[128];
            uint32_t h2;
            keep = dpat;
            proj_capture(&blob);
            check(((const pkit_t *)blob.x.kit)->magic == PK_MAGIC, "saved: the ext holds the 16 slots (PKT2)");
            load_mem();
            h2 = render_iso();
            snprintf(what, sizeof what, "saved as wave 12 and loaded again: the same sound and locks: %s", NAMES[sc]);
            check(h2 == h[sc] && !memcmp(&keep, &dpat, sizeof keep), what);
            if (sc == 5u)
                check(pkit_old_kept == 5u && ukit_state(4) == UK_STORED && uk_ram[4].s[2].snd[7] == 100u,
                      "a copy of a kit that did not play (606*, the 808 playing): kept as MY KIT 5, its edit in it");
        }
#endif
    }
    return fails != 0;
}
