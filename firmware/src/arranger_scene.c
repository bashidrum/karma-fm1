/* SPDX-License-Identifier: GPL-3.0-only */
/* The sections A..D (SAVE held + keys 1..8): which are stored, applying one, edits that stick. */
static uint32_t arrangement_ready(void)
{
    uint32_t i, ready = 0;
    for (i = 0; i < 4u; i++)
        if (proj_ok(&proj_slot[i])) ready |= 1u << i;
    return ready;
}

/* edits stick (2026-10-28, Ariel: erased hits came back each time the chain came round): the section playing (live_sec)
 * takes what was changed while it played, as a SAVE + 5..8 would: in the main loop a moment after the panel was last
 * touched, mid-bar (project.c section_keep_ui; at the section change it cost 0.6 ms on the downbeat, measured on the
 * FM-1). sec_kept tells the main loop to write it to flash when quiet.
 * (3.0.5, GitHub issue #3: the next pattern built after SAVE + 5..8 went into the part just stored, so storing it
 * made the two equal) Not the part just stored (sec_fresh) until a part is picked again (SAVE + 1..4) */
static volatile uint8_t sec_kept;
static uint32_t prof_keep_max, prof_apply_max;
/* main loop: while a section plays, what was changed in it goes into it 0.3 s after the panel was last touched
 * (input_ms), in a bar's 2nd or 3rd beat (a section changes on a bar: never mid-copy) */
static void section_keep_ui(uint32_t now, uint32_t input_ms)
{
    static uint32_t kept_ms;
    int32_t s = live_sec;
    uint32_t b = clk_beat & 3u, pt0 = PROF_NOW(), was;
    if (!song.playing || s < 0 || s > 3 || s == sec_fresh || input_ms == kept_ms || now - input_ms <= 300u ||
        (b != 1u && b != 2u) || !proj_ok(&proj_slot[s]))
        return;
    kept_ms = input_ms;
    was = proj_slot[s].sum;
    proj_capture(&proj_slot[s]);
    if (proj_slot[s].sum != was)
        sec_kept |= (uint8_t)(1u << s);
    if (PROF_NOW() - pt0 > prof_keep_max)
        prof_keep_max = PROF_NOW() - pt0;
}  /* (console `prof`: a section change's two halves, the worst, ticks) */
/* a live section (seq.c live_block), at an audio-block boundary on the bar. The tempo and the global FX stay
 * (only the drum level and reverb come from it). */
static void arrangement_apply(uint32_t scene)
{
    uint32_t k, pt1 = PROF_NOW();
    for (k = 0; k < NTRK; k++) {
        track_t *t = &trk[k];
        seq_release(t);
        trk_all_off(t);
        t->nheld = t->arp_phys = t->arp_note = t->rh_n = t->rskip_n = 0;
        t->rskip_lanes = 0;
    }
    proj_apply(&proj_slot[scene], 0);
    if (PROF_NOW() - pt1 > prof_apply_max)
        prof_apply_max = PROF_NOW() - pt1;
    live_sec = (int8_t)(scene & 3u);                /* (what it keeps, what HOME shows) */
    sync_reload = 1;
    ui.force = 1;
}
