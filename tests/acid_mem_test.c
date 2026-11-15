/* SPDX-License-Identifier: GPL-3.0-only */
/* (3.1.5) ACID GEN's settings, each synth track its own (acid_gen.c ag_trk; Ariel: "it should remember each parameter
 * for each track always"): a knob turned on track 1 stays track 1's; track 2 shows its own (the defaults); back on track
 * 1 its values come back; changing track writes no line; HARMONY's key goes to every track. */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"

static uint8_t steps_sig(const track_t *t) { uint32_t i, h = 7; for (i = 0; i < sizeof t->step; i++) h = h * 31u + ((const uint8_t *)t->step)[i]; return (uint8_t)h; }

int main(void)
{
    uint8_t s1;
    fails = 0;
    panel = PANEL_DEFAULT;
    layers_init();
    host_tracks_init();
    go_home(); frames(4);
    song.sel = 0; acid_gen_sync();
    check(song.g[G_AGDENS] == GP[G_AGDENS].def, "ACID GEN: track 1 starts at the defaults (DENS 70 %)");
    song.g[G_AGDENS] = 20; song.g[G_AGSEED] = 345; acid_gen_sync();
    check(ag_trk[0].v[0] == 20 && ag_trk[0].seed == 345 && ag_dirty, "track 1's DENS and SEED turned: kept as track 1's (to be saved)");
    s1 = steps_sig(&trk[1]);
    song.sel = 1; acid_gen_sync();
    check(song.g[G_AGDENS] == GP[G_AGDENS].def && song.g[G_AGSEED] == GP[G_AGSEED].def, "track 2: its own settings on the pages (the defaults)");
    check(steps_sig(&trk[1]) == s1, "changing track writes no line");
    song.g[G_AGACC] = 100; acid_gen_sync();
    song.sel = 0; acid_gen_sync();
    check(song.g[G_AGDENS] == 20 && song.g[G_AGSEED] == 345 && song.g[G_AGACC] == GP[G_AGACC].def, "back on track 1: its DENS and SEED, not track 2's ACC");
    check(ag_trk[1].v[1] == 100, "track 2 kept its ACC");
    ag_key_all(7, 2);
    check(ag_trk[0].v[5] == 7 && ag_trk[2].v[5] == 7 && ag_trk[1].v[3] == 2, "HARMONY's key: every track's ROOT and SCALE");
    /* (the settings object's part, project.c ag_part: flash code the host harness does not build: checked on an FM-1,
     * a value turned, stopped, switched off and on) */
    printf("acid gen memory: %s\n", fails ? "FAIL" : "all passed");
    return fails != 0;
}
