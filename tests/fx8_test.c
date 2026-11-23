/* SPDX-License-Identifier: GPL-3.0-only */
/* PHEBUS wave 8, the layout logic (SPEC.md, "22 Nov 2026, 23:20: the layout, in Ben's words"): the sound side of his answers.
 *   sends     "FX = whole mix only": a voice's sends to the delay, reverb and chorus are its own (the SENDS page), heard with
 *             no slot; its three slots are inserts, independent of them; no shared setting is any voice's parameter
 *   convert   a project of waves 5 to 7 (format 8, rsv 2): each CHORUS / REVERB / DELAY slot becomes OFF in its place with its
 *             send kept, a send it had no slot for 0 (it was silent), the inserts and their locks as they were, a matrix
 *             cell on such a slot's FX column on an assignable column aimed at the send; a project from before wave 5: every
 *             send kept; "cut the spare filters": the synth tracks' old FILT onto the voice filter, the master FILT dropped;
 *             "tempo in one place" (and one swing): GLOBAL SWING added to each pattern's own
 *   cc        MIDI CC 74 moves the voice filter's CUTOFF (the drum channel: its FILTER); CC 91 / 93 / 94 the sends, no slot
 * Exit status: the number of failed checks. */
#include <stdint.h>
#define main hostsim_main
#include "hostsim.c"
#undef main
#define PROJ_HOST 1
static uint32_t trk_def_engine(uint32_t i) { return i < NPART ? ENGI_JUNO : 0u; }
#include "../firmware/src/project.c"
static int fails;
static void check(int ok, const char *what)
{
    printf("fx8: %-104s %s\n", what, ok ? "ok" : "FAIL");
    fails += !ok;
}
#define NB 400u
static int32_t out_l[NB * CTL], ref_l[NB * CTL];
static void render(uint32_t nb)
{
    static int32_t o[2u * CTL];
    uint32_t b, i;
    for (b = 0; b < nb; b++) {
        mix_block(o, CTL);
        for (i = 0; i < CTL; i++)
            out_l[b * CTL + i] = o[2u * i];
    }
}
static double diff(void)
{
    double d = 0;
    uint32_t i;
    for (i = 0; i < NB * CTL; i++)
        d += fabs((double)out_l[i] - ref_l[i]);
    return d / (NB * CTL);
}
#include <sys/mman.h>
static int32_t *sh;
static void run(void (*cfg)(int), int a)                /* a fresh state each time: a child renders, the parent compares */
{
    pid_t pid;
    if (!sh)
        sh = mmap(0, sizeof out_l, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    fflush(stdout);
    if (!(pid = fork())) {
        cfg(a);
        render(NB);
        memcpy(sh, out_l, sizeof out_l);
        _exit(0);
    }
    waitpid(pid, 0, 0);
    memcpy(out_l, sh, sizeof out_l);
}
static void base(void)
{
    host_tracks_init();
    song.master_q12 = 2048;
    trk[0].p[P_LEVEL] = 100;
    trk[0].p[P_VOICE] = V_POLY;
}
/* sends and inserts: what each knob does alone */
static void cfg_send(int which)
{
    base();
    if (which & 1)
        trk[0].p[P_REV] = 100;
    if (which & 2)
        trk[0].p[P_DLY] = 100;
    if (which & 4)
        trk[0].p[P_CHOR] = 100;
    if (which & 8) {
        xs_type_set(trk[0].p, 0, XT_PHASER);
        xs_type_set(trk[0].p, 1, XT_DIST);
        xs_type_set(trk[0].p, 2, XT_CRUSH);
    }
    trk_note_on(&trk[0], 45, 100);
}
static void t_sends(void)
{
    int w, ok = 1;
    run(cfg_send, 0);
    memcpy(ref_l, out_l, sizeof ref_l);
    for (w = 1; w <= 4; w <<= 1) {
        run(cfg_send, w);
        ok &= diff() > 10.0;
    }
    check(ok, "sends: REVERB, DELAY, CHORUS each heard on a voice with every slot OFF (the SENDS page, no slot)");
    run(cfg_send, 8);
    memcpy(ref_l, out_l, sizeof ref_l);
    run(cfg_send, 8 | 7);
    check(diff() > 10.0, "sends and inserts independent: three inserts and three sends at once, the sends still heard");
    {   /* no shared setting is a voice's: every voice page's knob is a P_ id of that voice, never a G_ one */
        uint32_t i, k, bad = 0;
        song.sel = 0;
        for (i = 0; i < NPAGES; i++) {
            if (fam_stage(PAGES[i].fam) < 0)
                continue;
            for (k = 0; k < 4u; k++) {
                uint32_t id = pg_id(&PAGES[i], k);
                int16_t *vp;
                const param_desc_t *d = page_desc(&PAGES[i], k, &vp);
                if (d && vp && (vp < trk[0].p || vp >= trk[0].p + P_COUNT) && id != V_TYPE && (id < V_L2SYNC || id > V_MK4))
                    bad++, printf("  %s knob %u is not the voice's\n", PAGES[i].title, (unsigned)k + 1u);
                if (PAGES[i].scope == SC_GLOBAL)
                    bad++;
            }
        }
        for (i = 0; i < NXSLOT; i++) {                  /* (an old send type in a slot: no knob at all) */
            trk[0].p[P_XS(i, 0)] = XT_DELAY;
            for (k = 1; k < 4u; k++) {
                int16_t *vp;
                uint32_t pi;
                for (pi = 0; pi < NPAGES && !(PAGES[pi].graph == GR_XSLOT && pg_slot(&PAGES[pi]) == i); pi++)
                    ;
                bad += page_desc(&PAGES[pi], k, &vp) != 0;
            }
            trk[0].p[P_XS(i, 0)] = XT_OFF;
        }
        check(!bad, "no shared FX setting (delay TIME FDBK, reverb SIZE DAMP, chorus RATE DEPTH) on any voice page");
    }
}

/* a project of waves 5 to 7 (rsv[0] 2): send slots, a send without one, matrix cells on their columns, the old FILT, the
 * master FILT, GLOBAL SWING */
static void t_convert(void)
{
    static project_t w;
    int l, ok;
    base();
    proj_capture(&w);
    check(w.c.rsv[0] == PROJ_SEND_V, "a project saved now is marked as wave 8's (rsv 3: the sends their own)");
    w.c.rsv[0] = PROJ_FX_V;
    {
        int16_t *p = w.c.t[0].p;
        uint32_t k;
        for (k = 0; k < NLOCK; k++)
            w.c.t[0].lock[k].step = LOCK_FREE;
        p[P_X1TYPE] = XT_DELAY, p[P_DLY] = 50;
        p[P_X2TYPE] = XT_PHASER, p[P_X2TIME] = 30, p[P_X2INT] = 70, p[P_X2AMT] = 100;
        p[P_X3TYPE] = XT_REVERB, p[P_REV] = 60;
        p[P_CHOR] = 40;                                 /* (no CHORUS slot: it was silent) */
        p[P_TFLT] = -40, p[P_FCUT] = 120, p[P_FTYPE] = VF_LP24, p[P_SSWING] = 10;
        w.c.t[0].lock[0].step = 4, w.c.t[0].lock[0].param = P_X2TIME, w.c.t[0].lock[0].val = 99;
        w.c.t[0].lock[1].step = 5, w.c.t[0].lock[1].param = P_DLY, w.c.t[0].lock[1].val = 10;
        w.c.t[0].lock[2].step = 6, w.c.t[0].lock[2].param = P_CHOR, w.c.t[0].lock[2].val = 90;
        memset(w.c.t[0].mod, 0, sizeof w.c.t[0].mod);
        w.c.t[0].mod[3] = 1;                            /* (LFO 2 FREE) */
        w.c.t[0].mod[8] = (uint8_t)mx_at(MS_LFO2, MD_FX1), w.c.t[0].mod[9] = 30;   /* LFO2 > FX1 (the DELAY slot) */
        w.c.t[0].mod[10] = (uint8_t)mx_at(MS_LFO1, MD_FX2), w.c.t[0].mod[11] = 20; /* LFO1 > FX2 (the PHASER) */
        w.c.g[G_FILT] = 30;
        w.c.g[G_SWING] = 25;
    }
    w.c.sum = proj_sum(&w.c);
    base();
    fx8_lost = 0;
    proj_apply(&w.c, 1);
    l = lock_find(&trk[0], 4, P_X2TIME, 0);
    ok = trk[0].p[P_X1TYPE] == XT_OFF && trk[0].p[P_X2TYPE] == XT_PHASER && trk[0].p[P_X3TYPE] == XT_OFF &&
         trk[0].p[P_X2TIME] == 30 && trk[0].p[P_X2INT] == 70 && trk[0].p[P_X2AMT] == 100 && l >= 0 && trk[0].lock[l].val == 99;
    check(ok, "a wave 5 to 7 project: its DELAY and REVERB slots OFF in their places, the PHASER insert where it was, its lock kept");
    ok = trk[0].p[P_DLY] == 50 && trk[0].p[P_REV] == 60 && !trk[0].p[P_CHOR] && lock_find(&trk[0], 5, P_DLY, 0) >= 0 &&
         lock_find(&trk[0], 6, P_CHOR, 0) < 0;
    check(ok, "... its sends as they sounded: DELAY 50 and REVERB 60 kept (a lock too), CHORUS (no slot, silent) 0, its lock freed");
    ok = mx_get(0, MS_LFO1, MD_FX2) == 20 && !mx_get(0, MS_LFO2, MD_FX1) && mxt[0].as[0] == P_DLY + 1u &&
         mx_get(0, MS_LFO2, MD_AS1) == 30 && !fx8_lost;
    check(ok, "... its matrix: LFO2 > the DELAY slot now LFO2 > AS1 aimed at DELAY (same amount), LFO1 > FX2 (an insert) kept");
    ok = !trk[0].p[P_TFLT] && trk[0].p[P_FCUT] < 120 && trk[0].p[P_FTYPE] == VF_LP12 && !song.g[G_FILT];   /* (JUNO: its own
                                                         * ladder is fixed, the 12 dB filter after it as the DJ filter was) */
    check(ok, "... its hidden FILT low-pass onto the voice filter's CUTOFF; the master FILT (gone) dropped");
    check(trk[0].p[P_SSWING] == 35 && trk[1].p[P_SSWING] == 25 && TDRUM->p[P_SSWING] == 25 && !song.g[G_SWING],
          "... GLOBAL SWING 25 added to every pattern's own swing (one swing, hold SEQ), GLOBAL SWING 0");
    {   /* both assignable columns taken: the cell has nowhere to go (counted, the one lossy case) */
        w.c.t[0].mod[0] = P_FDRV + 1u, w.c.t[0].mod[1] = P_E1 + 1u;
        w.c.sum = proj_sum(&w.c);
        base();
        fx8_lost = 0;
        proj_apply(&w.c, 1);
        check(fx8_lost == 1u && !mx_get(0, MS_LFO2, MD_FX1) && mxt[0].as[0] == P_FDRV + 1u,
              "... both assignable columns already used: that one cell dropped and counted (fx8_lost)");
    }
    {   /* saved again: as it is now, nothing converted twice */
        base();
        proj_apply(&w.c, 1);
        proj_capture(&w);
        trk[0].p[P_REV] = 0;
        proj_apply(&w.c, 1);
        check(trk[0].p[P_REV] == 60 && trk[0].p[P_SSWING] == 35 && trk[0].p[P_X2TYPE] == XT_PHASER, "... saved again (rsv 3): loads as it is");
    }
}

static void t_cc(void)
{
    base();
    song.g[G_KNOBS] = 0;                                /* (GLO > MIDI 3 KNOBS GM: the standard CC map; PANEL takes 70-77) */
    trk[0].p[P_FCUT] = 127;
    midi_cc(&trk[0], 74, 20);
    check(trk[0].p[P_FCUT] < 30 && !trk[0].p[P_TFLT], "MIDI CC 74 on a synth voice: its voice filter's CUTOFF (the hidden DJ filter gone)");
    midi_cc(TDRUM, 74, 20);
    check(TDRUM->p[P_TFLT] < 0, "MIDI CC 74 on the drum channel: the drum voice's FILTER");
    trk[1].p[P_REV] = trk[1].p[P_CHOR] = trk[1].p[P_DLY] = 0;
    midi_cc(&trk[1], 91, 100), midi_cc(&trk[1], 93, 90), midi_cc(&trk[1], 94, 80);
    check(trk[1].p[P_REV] > 90 && trk[1].p[P_CHOR] > 80 && trk[1].p[P_DLY] > 70 && trk[1].p[P_X1TYPE] == XT_OFF &&
          trk[1].p[P_X2TYPE] == XT_OFF && trk[1].p[P_X3TYPE] == XT_OFF, "MIDI CC 91 / 93 / 94: the sends, no slot taken");
}

int main(void)
{
    setvbuf(stdout, 0, _IONBF, 0);
    t_sends();
    t_convert();
    t_cc();
    printf("fx8: %s\n", fails ? "FAILED" : "all checks ok");
    return fails;
}
