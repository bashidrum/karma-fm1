/* SPDX-License-Identifier: GPL-3.0-only */
/* The drum effects under FX (Ben, 23 Nov 2026: 21:30 "FX should be for FX", 21:40 "FILTER should live under Edit. Filter
 * envelope should live under ENV. Sends should live under FX", 21:41 "Just mirror the synths as closely as possible", 21:45
 * "Remove them" and "a KIT FX switch per sound"), through the real UI (tests/ui_pages_test.c's harness, the audio between):
 *   pages    EDIT and ENV hold no send; EDIT steps SRC FLTR | KIT FLT; FX steps INS SENDS SLICER, MIX, STUT (a synth
 *            voice's order); the kit's pages (INSERT, INSERT 2, SLICER, KIT FLT) say KIT, show no chip and SELECT does
 *            nothing there; SENDS shows the sound's chip; no page reaches the kit-wide sends; HOME's knobs neither
 *   sends    a sound's DELAY and REVERB on FX > SENDS change that sound only (heard: the kick's audio identical)
 *   KIT FX   OFF: sample for sample the sound as in a kit whose FILTER, INSERT and SLICER are neutral, with all three on;
 *            a sound left ON is changed by them; the mixer's mute silences an OFF sound; the switch survives save / load
 *            and a user kit; not lockable
 *   LFO      hold LFO on SENDS: REVERB mappable, KIT FX not
 *   screens  the tab rows fit; colour rule on the new pages; pictures for Ben
 * Exit status: the number of failed checks. */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
#include <sys/mman.h>
#define DX_NS (FS * 2u)
static int32_t *r_a, *r_b;
static uint32_t lanes_of(uint32_t m) { return m; }
static void dx_pattern(uint32_t lanes)             /* a bar: the kick on the beats, the hat on the eighths, the snare on 2, 4 */
{
    uint32_t i;
    for (i = 0; i < NTRK; i++) {
        steps_clear(&trk[i]);
        trk[i].p[P_MUTE] = 0;
        trk[i].p[P_DLY] = trk[i].p[P_REV] = 0;
    }
    dpat_clear(&dpat);
    song.solo = 0, drum_mute = drum_solo = 0;
    TDRUM->p[P_SLEN] = 16;
    for (i = 0; i < 16u; i++) {
        if ((lanes & 1u) && i % 4u == 0u) dt_set(i, 0, 110);
        if ((lanes & 4u) && i % 8u == 4u) dt_set(i, 2, 100);
        if ((lanes & 16u) && i % 2u == 0u) dt_set(i, 4, 90);
    }
}
/* render DX_NS samples from PLAY into o, in a fork (every render from the same state); setup runs in the fork first */
static void dx_render(int32_t *o, void (*setup)(int), int arg)
{
    pid_t p = fork();
    if (!p) {
        uint32_t b;
        int32_t blk[CTL * 2u];
        setup(arg);
        transport_req = 1;
        for (b = 0; b < DX_NS / CTL; b++) {
            mix_block(blk, CTL);
            memcpy(o + b * CTL * 2u, blk, sizeof blk);
        }
        _exit(0);
    }
    waitpid(p, 0, 0);
}
static uint32_t dx_diff(void) { uint32_t i, d = 0; for (i = 0; i < DX_NS * 2u; i++) d += r_a[i] != r_b[i]; return d; }
static double dx_energy(const int32_t *o) { double e = 0; uint32_t i; for (i = 0; i < DX_NS * 2u; i++) e += (double)o[i] * o[i]; return e; }

static void pages(void)
{
    uint32_t m, pg, h, k, ok = 1, n, f;
    for (m = 0; m < 2u; m++)                           /* EDIT (SRC FLTR) and ENV (AMP), LFO: no send, no KIT FX */
        for (pg = DPG_SRC; pg < DPG_N; pg++)
            for (h = 0, n = dh_count(m, pg, &f); h < n; h++)
                for (k = 0; k < 4u; k++) {
                    uint32_t id = dh_id(m, pg, h, k);
                    ok &= id != DSP_REV && id != DSP_DEL && id != DSP_KFX;
                }
    check(ok, "D1: SRC, FLTR, AMP, LFO on both machines hold no send and no KIT FX (EDIT and ENV: dry sound design)");
    for (m = 0, ok = 1; m < 2u; m++) {                 /* no lonely knob: every AMP half has two knobs at least */
        for (h = 0, n = dh_count(m, DPG_AMP, &f); h < n; h++) {
            uint32_t c = 0;
            for (k = 0; k < 4u; k++) c += dh_id(m, DPG_AMP, h, k) != 0xFFu;
            if (c < 2u) printf("  (machine %u AMP %u/%u: %u knob)\n", (unsigned)m, (unsigned)h + 1u, (unsigned)n, (unsigned)c);
            ok &= m == DM_HEX ? c >= 2u : 1;
        }
    }
    check(ok, "D1: the HEX machine's AMP 2/2: CRUSH and LEVEL (the CLASSIC machine's LEVEL alone: reported)");
    for (k = 0, ok = 1; k < NPAGES; k++)
        for (h = 0; h < 4u; h++)
            ok &= !((PAGES[k].scope == SC_GLOBAL) && (PAGES[k].id[h] == G_DRREV || PAGES[k].id[h] == G_DRDLY));
    check(ok, "D3: no page holds the kit-wide drum sends (G_DRREV, G_DRDLY)");
    /* EDIT and FX on the drums */
    song.sel = TRK_DRUM; kit_set(DRUM_DEFAULT_KIT); go_home(); frames(2);
    tap(B_EDIT); frames(2); ok = cur_cat() == CT_SRC;
    tap(B_EDIT); frames(2); ok &= cur_cat() == CT_FLTR;
    tap(B_EDIT); frames(2); ok &= cur_cat() == CT_DFILT && dpg_kit() && nav_tabs_fit;
    ui.force = 1; frame(); ppm("dfx-kit-filter");
    tap(B_EDIT); frames(2); ok &= cur_cat() == CT_SRC;
    check(ok, "D2: EDIT on the drums: SRC, FLTR, then KIT FLT (the kit's FILTER, past a divider), round; the tabs fit");
    tap(B_FX); frames(2); ok = cur_cat() == CT_DINS && dpg_kit() && nav_tabs_fit;
    ui.force = 1; frame(); ppm("dfx-kit-insert");
    tap(B_SCL); frames(2); ok &= cur_cat() == CT_DINS && dpg_kit();
    tap(B_FX); frames(2); ok &= cur_cat() == CT_DSENDS && dpg_sends() && nav_tabs_fit;
    tap(B_FX); frames(2); ok &= cur_cat() == CT_DSLICER && dpg_kit();
    ui.force = 1; frame(); ppm("dfx-kit-slicer");
    tap(B_FX); frames(2); ok &= cur_cat() == CT_MIX && !on_drum_page();   /* (U1: the one white MIX screen) */
    tap(B_FX); frames(2); ok &= cur_cat() == CT_STUT;
    tap(B_FX); frames(2); ok &= cur_cat() == CT_DINS;
    check(ok, "D2: FX on the drums: INS (SEL: INSERT 2), SENDS, SLICER, MIX, STUT, round: a synth voice's order; no FILTER");
    /* KIT on the kit's pages: no chip, SELECT inert; the sound's chip on SENDS */
    {
        static const uint8_t KH[4] = {0, 1, 3, DBH_FILTER};
        uint32_t i, okk = 1, ln;
        char cv[8];
        int32_t cr;
        uint16_t cc;
        drum_lane = 2;
        for (i = 0; i < 4u; i++) {
            dpg_go(DPG_FX, KH[i]); ui.force = 1; frame();
            ln = drum_lane;
            encs[panel.enc[EN_SELECT]] = 1; frames(2);
            okk &= dpg_kit() && !sel_chip(cv, &cr, &cc) && drum_lane == ln;
            okk &= px_count(0, 0, 240, OP_BAR_Y, TE_COL[0]) + px_count(0, 0, 240, OP_BAR_Y, TE_COL[1]) +
                   px_count(0, 0, 240, OP_BAR_Y, TE_COL[2]) + px_count(0, 0, 240, OP_BAR_Y, TE_RED) == 0u;
        }
        check(okk, "D2: INSERT, INSERT 2, SLICER, KIT FLT: header KIT (no sound chip), SELECT does nothing; teal, no other voice's colour");
        dpg_go(DPG_FX, DBH_SENDS); ui.force = 1; frame(); ppm("dfx-sends");
        ln = drum_lane;
        check(sel_chip(cv, &cr, &cc) && str_eq(cv, LANE_SHORT[drum_lane]) && cc == TE_DRUM,
              "D2: SENDS: the header names the selected sound (SNARE), in teal");
        encs[panel.enc[EN_SELECT]] = 1; frames(2);
        check(drum_lane == lane_step(ln, 1, 1), "D2: SELECT on SENDS picks the sound (the next in SELECT's order)");
        drum_lane = 2;
        frames(300);                                   /* (the sound SELECT auditioned rings out: since SELECT follows the Rytm
                                                        * order it is KICK 2, a long one, which the KIT FX renders below would
                                                        * otherwise hear through the kit's effects) */
        for (i = 0, okk = 1; i < NTRK; i++)
            okk &= px_count(0, 0, 240, OP_BAR_Y, i == TRK_DRUM ? TE_RED : TE_COL[i]) == 0u;
        check(okk && px_count(0, SG_KNOB_Y, 240, 58, TE_DRUM) > 40u, "D6: SENDS in teal only (no red, no melodic voice's colour)");
    }
    /* HOME's knobs on the drums never reach a kit-wide send */
    go_home(); frames(2);
    for (k = 0; k < 4u; k++) { encs[panel.enc[EN_K1 + k]] = 3; frame(); }
    check(song.g[G_DRREV] == 0 && song.g[G_DRDLY] == 0, "D3: HOME's knobs on the drums: no kit-wide send moves");
    TDRUM->p[P_TFLT] = 0;
}

/* ---- the sends: the snare's REVERB on SENDS, the kick untouched */
static void su_sends(int a)
{
    dx_pattern(a & 1 ? 1u : 1u | 4u);                  /* (a & 1: the kick alone) */
    song.sel = TRK_DRUM; go_home(); frames(2);
    drum_lane = 2;                                     /* the snare's REVERB and DELAY up on FX > SENDS, by the knobs (or not:
                                                        * the same frames either way, so the rest of the state is the same) */
    drum_screen_open(); dpg_go(DPG_FX, DBH_SENDS); frames(2);
    encs[panel.enc[EN_K2]] = a & 2 ? 30 : 0; frame();
    encs[panel.enc[EN_K1]] = a & 2 ? 20 : 0; frame();
    go_home(); frames(2);
}
static void sends_audio(void)
{
    dx_render(r_a, su_sends, 0);
    dx_render(r_b, su_sends, 2);
    check(dx_diff() > 1000u, "D2: FX > SENDS: the snare's REVERB and DELAY turned: heard (the render changes)");
    dx_render(r_a, su_sends, 1);
    dx_render(r_b, su_sends, 3);
    check(dx_diff() == 0u, "D2: ... and the kick alone renders identically: a sound's sends change only that sound");
    {   /* the knobs set the snare's own, nothing else */
        uint32_t l, o = 0;
        su_sends(2);
        for (l = 0; l < 16u; l++) o += l != 2u && (sl_send(l, 0) | sl_send(l, 1));
        check(sl_send(2, 0) > 0u && sl_send(2, 1) > 0u && !o && !song.g[G_DRREV] && !song.g[G_DRDLY],
              "D2: SENDS: REVERB (KNOB 2) and DELAY (KNOB 1) are the snare's own, no other sound's, no kit-wide send");
        for (l = 0; l < 16u; l++) sl_send_set(l, 0, 0), sl_send_set(l, 1, 0);
    }
}

/* ---- KIT FX: a = bits: 1 the kick's KIT FX OFF, 2 the kit's FILTER, INSERT and SLICER on, 4 the hat too, 8 the mixer mutes the kick */
static void su_kfx(int a)
{
    uint32_t l;
    dx_pattern(a & 4 ? 1u | 16u : 1u);
    for (l = 0; l < 16u; l++) sl_kfx_set(l, 0);
    if (a & 1) sl_kfx_set(0, 1);
    if (a & 2) {
        TDRUM->p[P_TFLT] = -40;                        /* low-pass */
        TDRUM->p[P_ITYPE] = 2, TDRUM->p[P_IMIX] = 100;
        TDRUM->p[P_SLCR] = SL_GATE;
    } else {
        TDRUM->p[P_TFLT] = 0, TDRUM->p[P_ITYPE] = 0, TDRUM->p[P_SLCR] = SL_OFF;
    }
    if (a & 8) drum_mute = 1u;
    song.sel = 0; go_home(); frames(2);
}
static void kitfx_audio(void)
{
    dx_render(r_a, su_kfx, 0);                         /* the kick, KIT FX ON, the kit neutral */
    dx_render(r_b, su_kfx, 1);                         /* the kick, KIT FX OFF, the kit neutral */
    check(dx_energy(r_a) > 1e9 && dx_diff() == 0u, "D4: KIT FX OFF with the kit neutral: sample for sample the same sound");
    dx_render(r_b, su_kfx, 1 | 2);                     /* KIT FX OFF, the kit's FILTER, INSERT, SLICER all on */
    check(dx_diff() == 0u, "D4: KIT FX OFF: the kit's FILTER (low-pass), INSERT and SLICER (GATE) on: the kick untouched, sample for sample");
    dx_render(r_b, su_kfx, 2);                         /* KIT FX ON, the kit on */
    check(dx_diff() > 1000u, "D4: KIT FX ON: the kit's FILTER, INSERT and SLICER change the kick (as before)");
    dx_render(r_a, su_kfx, 4 | 1);                     /* kick OFF + hat ON, kit neutral */
    dx_render(r_b, su_kfx, 4 | 1 | 2);                 /* kick OFF + hat ON, kit on: the hat changes, so the mix does */
    check(dx_diff() > 1000u, "D4: \"stutter the tops, leave the kick alone\": with the kick OFF, the kit's effects still act on the hat");
    dx_render(r_b, su_kfx, 1 | 2 | 8);
    check(dx_energy(r_b) < dx_energy(r_a) / 1e4, "D4: the mixer's mute silences a KIT FX OFF sound too");
}
static void kitfx_ui(void)
{
    pkit_t k;
    song.sel = TRK_DRUM; kit_set(DRUM_DEFAULT_KIT); go_home(); frames(2);
    drum_lane = 4;
    drum_screen_open(); dpg_go(DPG_FX, DBH_SENDS); frames(2);
    check(sl_kfx_off(4) == 0u, "D4: KIT FX ON to start (every sound through the kit's effects, as before)");
    encs[panel.enc[EN_K3]] = -1; frames(2);
    ui.force = 1; frame(); ppm("dfx-sends-kitfx-off");
    check(sl_kfx_off(4) == 1u && sl_kfx_off(0) == 0u, "D4: SENDS KNOB 3 KIT FX: OFF for the selected sound (HAT) only");
    dpg_go(DPG_FX, 0); ui.force = 1; frame(); ppm("dfx-kit-insert-15");
    check(kfx_count() == 15u, "D4: the kit's pages count the sounds through them: 15 OF 16 SOUNDS");
    check(ukit_put(5, &pkit) == 0 && kit_make(DSRC_USER + 5, &k) && (k.s[4].send & SL_KFX_OFF) && !(k.s[0].send & SL_KFX_OFF),
          "D4: and a user kit (SAVE > KITS) keeps it");
    ukit_put(5, 0);
    sl_kfx_set(4, 0);
    /* hold LFO on SENDS: REVERB mappable, KIT FX not */
    dpg_go(DPG_FX, DBH_SENDS); frames(2);
    press(B_LFO); frames(12);
    encs[panel.enc[EN_K2]] = 10; frames(2);
    {
        uint32_t d = pkit.s[4].lfo.dst;
        int32_t dep = pkit.s[4].lfo.dep;
        check(dl_id(sl_mach(4), d) == DSP_REV && dep != 0, "D5: hold LFO on SENDS, KNOB 2: the sound's LFO onto its REVERB");
        ui.force = 1; frame(); ppm("dfx-sends-lfo");
        encs[panel.enc[EN_K3]] = 5; frames(2);
        check(pkit.s[4].lfo.dst == d && strstr(ui.msg, "NOT MAPPABLE") != 0, "D5: KIT FX: not mappable (a switch)");
    }
    release(B_LFO); frames(3);
    go_home(); frames(2);
}

int main(int argc, char **argv)
{
    uint32_t i;
    outdir = argc > 1 ? argv[1] : "build/host";
    r_a = mmap(0, DX_NS * 2u * sizeof(int32_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    r_b = mmap(0, DX_NS * 2u * sizeof(int32_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    (void)lanes_of;
    bank_resolve();
    panel = PANEL_DEFAULT;
    layers_init();
    settings.palette = 4;
    palette_set(4);
    host_tracks_init();
    for (i = 0; i < NPART; i++) { trk_def_sound(&trk[i], i); trk[i].engine = trk[i].eng_req; }
    kit_set((uint32_t)(DRUM_DEFAULT_KIT));
    song.g[G_BPM] = 120;
    go_home(); ui.force = 1; frames(4);
    pages();
    sends_audio();
    kitfx_audio();
    kitfx_ui();
    printf("dfx ui: %s (%d failed)\n", fails ? "FAILED" : "all ok", fails);
    return fails;
}
