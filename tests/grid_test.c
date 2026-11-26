/* SPDX-License-Identifier: GPL-3.0-only */
/* The drum grid, Rytm style (Ben, 23 Nov 2026, 21:07 and 21:14, as he typed it; SPEC.md). Through the real UI (tests/
 * ui_pages_test.c's harness, the audio between frames, as tests/w13_test.c):
 *   the table   the grid's 16 places hold exactly his roles, row by row; two key orders over it: the keys played by hand
 *               count the Rytm's way, the bottom row first (Ben, 22:45: keys 1 to 4 KICK RIM SNARE 2 SNARE ... 13 to 16 HAT
 *               OP HAT RIDE CRASH); the mixer and SELECT run down the columns (Ben, 21:10: keys 1 to 4 HAT PEDAL KICK 2 KICK
 *               ... 13 to 16 CRASH SHAKER CLAP SNARE), his 21:10 order kept (22:46); SELECT follows the Rytm order (22:48)
 *   the mixer   each white key mutes, and with OCT+ solos, exactly its sound; the tile that changes is the one in its grid
 *               place; every white key lit (each does something); heard: key 4's mute silences the KICK and nothing else,
 *               key 16's the SNARE, key 1's the HAT, key 15's the CLAP (sample for sample against the same pattern with
 *               that sound's trigs taken out); a solo by key 16 leaves the SNARE alone
 *   the drum screen's map   the selected slot framed in its grid place; slots with hits marked there
 *   SELECT      walks the Rytm order and goes round (CRASH -> KICK); the chip names the sound
 *   the keys    on HOME a white key plays its grid sound; hold SEQ + a key retrigs that sound; hold SEQ's tiles are the grid;
 *               a hit lights its sound's key
 *   names       all 16 short names fit their tiles (with an M or S tab too); the colour rule on the changed screens
 * Exit status: the number of failed checks. */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
#include <sys/mman.h>
#define GR_NS (FS * 2u)
static int32_t *r_a, *r_b;
static uint32_t *hits_a;
/* Ben's grid, row by row from the top left (LANE_SHORT's spelling: "snare2"), and his key order */
static const char *const WANT_GRID[16] = {"hat", "op hat", "ride", "crash", "pedal", "conga", "c-bell", "shaker",
                                          "kick 2", "lo tom", "hi tom", "clap", "kick", "rim", "snare2", "snare"};
static const char *const WANT_PLAY[16] = {"kick", "rim", "snare2", "snare", "kick 2", "lo tom", "hi tom", "clap",
                                          "pedal", "conga", "c-bell", "shaker", "hat", "op hat", "ride", "crash"};
static const char *const WANT_KEYS[16] = {"hat", "pedal", "kick 2", "kick", "op hat", "conga", "lo tom", "rim",
                                          "ride", "c-bell", "hi tom", "snare2", "crash", "shaker", "clap", "snare"};
static uint32_t lane_named(const char *s)
{
    uint32_t l;
    for (l = 0; l < DRUM_LANES; l++)
        if (str_eq(LANE_SHORT[l], s))
            return l;
    return 99u;
}

static void table(void)
{
    uint32_t i, ok = 1, ok2 = 1, seen = 0;
    for (i = 0; i < 16u; i++) {
        ok &= str_eq(LANE_SHORT[DRUM_GRID[i]], WANT_GRID[i]);
        seen |= 1u << DRUM_GRID[i];
        ok2 &= str_eq(LANE_SHORT[lane_of_mixkey(i)], WANT_KEYS[i]) && str_eq(LANE_SHORT[lane_of_white(i)], WANT_PLAY[i]) &&
               white_of_lane(lane_of_white(i)) == i && white_of_lane_o(lane_of_mixkey(i), DG_MIX) == i &&
               lane_of_key(key_of_white(i)) == lane_of_white(i) && key_of_lane(lane_of_white(i)) == key_of_white(i);
    }
    check(ok && seen == 0xFFFFu, "the grid's 16 places hold exactly Ben's roles (HAT OP HAT RIDE CRASH / ... / KICK RIM SNARE 2 SNARE)");
    check(ok2, "played by hand: 1-4 KICK RIM SNARE 2 SNARE .. 13-16 HAT OP HAT RIDE CRASH; the mixer down the columns: 1-4 HAT PEDAL KICK 2 KICK ..");
    for (i = 0, ok = 1; i < 27u; i++)                  /* a black key: the white key's left of it (it plays nothing: wave 10) */
        if (punch_key(i) < 0)
            ok &= lane_of_key(i) == lane_of_key(i - 1u);
    check(ok, "a black key maps as the white key left of it (as before)");
    check(lane_of_mixkey(3) == lane_named("kick") && lane_of_mixkey(15) == lane_named("snare") && lane_of_mixkey(0) == lane_named("hat") &&
          lane_of_white(0) == lane_named("kick") && lane_of_white(3) == lane_named("snare") && lane_of_white(12) == lane_named("hat") &&
          lane_of_white(15) == lane_named("crash"),
          "the two orders pinned: mixer key 4 KICK, 16 SNARE, 1 HAT; by hand key 1 KICK, 4 SNARE, 13 HAT, 16 CRASH");
}

/* ---- the mixer: keys, tiles, lights */
static void tile_xy(uint32_t place, uint32_t *x, uint32_t *y) { *x = 1u + (place % 4u) * 60u, *y = BK_Y + 34u + (place / 4u) * 20u; }
static void mixer_keys(void)
{
    uint32_t w, okm = 1, oks = 1, okt = 1, okl = 1, okn = 1, okf = 1;
    dpat_clear(&dpat);
    song.playing = 0, song.solo = 0, drum_mute = drum_solo = 0;
    song.sel = 0; go_home(); frames(3);
    press(B_GLO); frames(12);
    for (w = 0; w < 16u; w++) {
        uint32_t want = lane_named(WANT_KEYS[w]), p, x, y, c;
        key(key_of_white(w)); ui.force = 1; frame();
        okm &= drum_mute == 1u << want && drum_lane == want;
        for (p = 0; p < 16u; p++) {                     /* the yellow tile: only the one in its grid place */
            tile_xy(p, &x, &y);
            c = px_count(x, y, 58, 18, TE_MUTE);
            okt &= DRUM_GRID[p] == want ? c > 400u : c == 0u;
            if (DRUM_GRID[p] == want)                   /* its name clear of the tile's left edge and of the M tab (x + 50) */
                okf &= px_count(x, y + 1u, 1, 16, C_BLACK) == 0u && px_count(x + 49u, y + 1u, 1, 16, C_BLACK) == 0u &&
                       px_count(x + 1u, y + 1u, 46, 16, C_BLACK) > 30u;
        }
        for (c = 0; c < 16u; c++)                       /* every white key lit: each does something here */
            okl &= led_lv[14u + key_of_white(c)] == LL_LIT;
        key(key_of_white(w)); frame();
        okm &= drum_mute == 0u;
        press(B_OCTUP); key(key_of_white(w)); release(B_OCTUP); ui.force = 1; frame();
        oks &= drum_solo == 1u << want;
        for (p = 0; p < 16u; p++) {
            tile_xy(p, &x, &y);
            c = px_count(x, y, 58, 18, TE_SOLO);
            okt &= DRUM_GRID[p] == want ? c > 400u : c == 0u;
            if (DRUM_GRID[p] == want)
                okf &= px_count(x, y + 1u, 1, 16, C_BLACK) == 0u && px_count(x + 49u, y + 1u, 1, 16, C_BLACK) == 0u;
            okt &= px_count(x, y, 58, 18, TE_COL[0]) + px_count(x, y, 58, 18, TE_COL[1]) + px_count(x, y, 58, 18, TE_COL[2]) +
                   px_count(x, y, 58, 18, TE_RED) == 0u;
        }
        if (w == 3u) ppm("grid-mixer-solo-kick");
        press(B_OCTUP); key(key_of_white(w)); release(B_OCTUP); frame();
        oks &= drum_solo == 0u;
    }
    check(okm, "mixer: white key n mutes exactly its sound in key order (key 1 HAT .. key 4 KICK .. key 16 SNARE); again: unmuted");
    check(oks, "mixer: OCT+ held + white key n solos exactly its sound in key order; again: off");
    check(okt, "mixer: the tile that turns yellow / ice blue is the one in that sound's grid place, and no other; no voice colour or red");
    check(okf, "mixer: each name fits its tile muted or soloed: a pixel clear of the tile's edge and of the M / S tab (OP HAT, SHAKER, C-BELL ...)");
    check(okl, "mixer: every white key lit while GLO is held (each mutes / solos its sound)");
    /* a picture for Ben: a few mutes and a solo */
    key(key_of_white(0)); key(key_of_white(4)); key(key_of_white(13));    /* HAT, OP HAT, SHAKER muted */
    press(B_OCTUP); key(key_of_white(15)); release(B_OCTUP);              /* SNARE soloed */
    key(PUNCH_BLACK[1]);                                                  /* voice 2 muted */
    ui.force = 1; frame(); ppm("grid-mixer");
    okn = drum_mute == (1u << 4 | 1u << 5 | 1u << 13) && drum_solo == 1u << 2;
    check(okn, "mixer: HAT, OP HAT, SHAKER muted by keys 1, 5, 14; SNARE soloed by key 16");
    {
        uint32_t x, y;
        tile_xy(15, &x, &y);
        check(px_count(x, y, 58, 18, TE_SOLO) > 400u && px_count(1, BK_Y + 34, 58, 18, TE_MUTE) > 400u &&
              px_count(61, BK_Y + 34, 58, 18, TE_MUTE) > 400u && px_count(181, BK_Y + 34 + 20, 58, 18, TE_MUTE) > 400u,
              "mixer: SNARE's tile (bottom right) ice blue; HAT, OP HAT (top left) and SHAKER (second row, right) yellow");
    }
    tap(B_OCTDN); frames(2);
    trk[1].p[P_MUTE] = 0;
    release(B_GLO); frames(3);
}

/* ---- the mixer heard: a pattern of KICK, SNARE, HAT and CLAP; a key's mute takes out its sound and nothing else */
static void gr_setup(void)
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
        if (i % 4u == 0u) dt_set(i, 0, 100);           /* KICK */
        if (i % 8u == 4u) dt_set(i, 2, 100);           /* SNARE */
        if (i % 2u == 1u) dt_set(i, 4, 90);            /* HAT */
        if (i % 8u == 6u) dt_set(i, 3, 100);           /* CLAP */
    }
}
static void gr_render(int32_t *o, uint32_t *hm)
{
    uint32_t b, k, a;
    int32_t blk[CTL * 2u];
    transport_req = 1;
    a = drums.age;
    for (b = 0; b < GR_NS / CTL; b++) {
        mix_block(blk, CTL);
        memcpy(o + b * CTL * 2u, blk, sizeof blk);
        if (drums.age != a)
            for (k = 0; k < NDRUM; k++)
                if (drums.v[k].age > a)
                    *hm |= 1u << lane_of_note(drums.v[k].note);
        a = drums.age;
    }
}
static int gr_same(void)
{
    uint32_t i, d = 0;
    for (i = 0; i < GR_NS * 2u; i++)
        d += r_a[i] != r_b[i];
    if (d)
        printf("  (%u samples differ)\n", d);
    return d == 0;
}
/* the case: GLO held, white key w (with OCT+: solo); the reference: the lanes in keep only, a key that does nothing (D#4) */
static uint32_t gr_case(uint32_t w, int solo, uint32_t keep, int ref)
{
    pid_t p = fork();
    int st;
    if (!p) {
        uint32_t i, l, hm = 0;
        gr_setup();
        if (ref)
            for (i = 0; i < 16u; i++)
                for (l = 0; l < DRUM_LANES; l++)
                    if (!((keep >> l) & 1u)) dt_clr(i, l);
        song.sel = TRK_DRUM; go_home(); frames(3);
        press(B_GLO); frames(12);
        if (solo) press(B_OCTUP);
        key(ref ? PUNCH_BLACK[4] : key_of_white(w));
        if (solo) release(B_OCTUP);
        release(B_GLO); frames(3);
        if (ref && solo)                              /* (the voices silenced the same way as the solo does: the noise runs the same) */
            song.solo = 1u << TRK_DRUM;
        gr_render(ref ? r_b : r_a, &hm);
        hits_a[ref] = hm;
        _exit(0);
    }
    waitpid(p, &st, 0);
    return hits_a[ref];
}
static void mixer_audio(void)
{
    static const struct { uint8_t w; const char *what; } C[4] = {
        {3, "key 4's mute: the KICK silenced, SNARE HAT CLAP heard, sample for sample"},
        {15, "key 16's mute: the SNARE silenced, nothing else, sample for sample"},
        {0, "key 1's mute: the HAT silenced, nothing else, sample for sample"},
        {14, "key 15's mute: the CLAP silenced, nothing else, sample for sample"}};
    const uint32_t all = 1u << 0 | 1u << 2 | 1u << 3 | 1u << 4;
    uint32_t i, h, hr;
    for (i = 0; i < 4u; i++) {
        uint32_t l = lane_of_mixkey(C[i].w);
        h = gr_case(C[i].w, 0, 0, 0);
        hr = gr_case(0, 0, all & ~(1u << l), 1);
        check(h == (all & ~(1u << l)) && hr == h && gr_same(), C[i].what);
    }
    h = gr_case(15, 1, 0, 0);
    hr = gr_case(0, 1, 1u << 2, 1);
    check(h == 1u << 2 && hr == h && gr_same(), "OCT+ + key 16: the SNARE soloed, heard alone, sample for sample");
}

/* ---- the drum screen's map: the selected slot framed in its grid place, slots with hits marked */
static void drum_map_place(void)
{
    static const char *const SEL[4] = {"kick", "snare", "hat", "c-bell"};
    uint32_t i, ok = 1, okh = 1;
    gr_setup();
    song.playing = 0; memset(pad_lit, 0, sizeof pad_lit);
    song.sel = TRK_DRUM; go_home(); frames(2); drum_screen_open(); frames(3);
    for (i = 0; i < 4u; i++) {
        uint32_t l = lane_named(SEL[i]), p, best = 99u, bn = 0;
        drum_lane = (uint8_t)l; ui.force = 1; frame();
        for (p = 0; p < 16u; p++) {                     /* which tile has the white frame */
            int32_t x = 155 + (int32_t)(p % 4u) * 21 - 1, y = (int32_t)SG_BODY_Y + 4 + (int32_t)(p / 4u) * 12 - 1;
            uint32_t n = px_count((uint32_t)x, (uint32_t)y, 20, 1, C_WHITE) + px_count((uint32_t)x, (uint32_t)y + 10u, 20, 1, C_WHITE);
            if (n > bn) bn = n, best = p;
        }
        ok &= best == dgrid_of_lane(l) && bn >= 30u;
        if (i == 0u) ppm("grid-drum-kick");
        if (i == 3u) ppm("grid-drum-cbell");
    }
    check(ok, "drum screen: the map frames the selected slot in its grid place (KICK bottom left, SNARE bottom right, HAT top left, C-BELL)");
    for (i = 0; i < 16u; i++) {                        /* the tiles with hits (KICK SNARE HAT CLAP): dim teal in their places */
        uint32_t l = DRUM_GRID[i], has = l == 0u || l == 2u || l == 3u || l == 4u;
        int32_t x = 155 + (int32_t)(i % 4u) * 21, y = (int32_t)SG_BODY_Y + 4 + (int32_t)(i / 4u) * 12;
        uint32_t n = px_count((uint32_t)x + 2u, (uint32_t)y + 2u, 10, 5, TE_MID[3]);
        okh &= has ? n == 50u : n == 0u;
    }
    check(okh, "drum screen: the map marks the slots with hits (KICK, SNARE, HAT, CLAP) in their grid places, no other");
    drum_lane = (uint8_t)lane_named("snare"); dt_set(2, 9, 100); ui.force = 1; frame(); ppm("grid-drum-snare");
    check(px_count(0, 0, 240, 240, TE_RED) == 0u, "drum screen with the map: no red");
}

/* ---- SELECT: the key order, round */
static void select_order(void)
{
    uint32_t i, ok = 1, l;
    char cv[8];
    int32_t cr;
    uint16_t cc;
    song.sel = TRK_DRUM; go_home(); frames(2); drum_screen_open(); frames(3);
    drum_lane = (uint8_t)lane_named("kick"); frame();
    for (i = 1; i <= 16u; i++) {
        encs[panel.enc[EN_SELECT]] = 1; frame(); frame();
        ok &= str_eq(LANE_SHORT[drum_lane], WANT_PLAY[i & 15u]);
        if (i == 4u) {                                  /* the chip names it, upper case as drawn */
            sel_chip(cv, &cr, &cc);
            ok &= str_eq(cv, "kick 2") && cc == TE_DRUM;
        }
    }
    check(ok, "SELECT: KICK, RIM, SNARE 2, SNARE, KICK 2 ... RIDE, CRASH, then round to KICK (the Rytm order); the chip names it");
    l = drum_lane;                                      /* (KICK) */
    encs[panel.enc[EN_SELECT]] = -1; frame(); frame();
    check(str_eq(LANE_SHORT[drum_lane], "crash") && l == lane_named("kick"), "SELECT left from KICK: CRASH (round)");
    encs[panel.enc[EN_SELECT]] = -1; frame(); frame();
    check(str_eq(LANE_SHORT[drum_lane], "ride"), "SELECT left again: RIDE");
    check(lane_step(lane_named("kick"), -1, 0) == lane_named("kick") && lane_step(lane_named("crash"), 1, 0) == lane_named("crash") &&
          lane_step(lane_named("snare"), 1, 0) == lane_named("kick 2"),
          "CHROMATIC's -/+ follows SELECT's order, stopping at KICK and CRASH");
}

/* ---- the keys elsewhere: HOME plays, hold SEQ retrigs, hold SEQ's tiles, the hit's light */
static void keys_elsewhere(void)
{
    uint32_t w, ok = 1, okl = 1, i;
    dpat_clear(&dpat);
    song.playing = 0; drum_mute = drum_solo = 0; song.solo = 0;
    song.sel = TRK_DRUM; go_home(); frames(4);
    for (w = 0; w < 16u; w++) {
        uint32_t l = lane_of_white(w), others = 0;
        memset(pad_lit, 0, sizeof pad_lit);
        fm1_in.notes = 1u << key_of_white(w); frames(2);
        for (i = 0; i < 16u; i++)
            if (i != l) others |= pad_lit[i];
        ok &= pad_lit[l] && !others;
        okl &= led_lv[14u + key_of_lane(l)] == LL_LIT;
        fm1_in.notes = 0; frames(2);
        okl &= led_lv[14u + key_of_lane(l)] == LL_LIT;   /* (the hit still lights its sound's key, a few frames) */
    }
    check(ok, "HOME, the drums: white key n plays exactly its grid sound, bottom row first (key 1 KICK, key 4 SNARE, key 16 CRASH ...)");
    check(okl, "HOME, the drums: a hit lights its sound's key (the key that plays it)");
    /* hold SEQ: RETRIG on key 1 is the KICK (the Rytm order); the tiles are the grid */
    transport_req = 1; frames(3);                     /* (RETRIG plays with the transport running, as drumrytm_test's) */
    press(B_SEQ); frames(12);
    memset(pad_lit, 0, sizeof pad_lit);
    fm1_in.notes = 1u << key_of_white(0);
    {
        uint32_t f, seen = 0;
        for (f = 0; f < 40u; f++) {                     /* (the hits a frame: a pad stays lit 6 frames, the retrig 1/16) */
            frame();
            for (i = 0; i < 16u; i++)
                seen |= (uint32_t)(pad_lit[i] != 0u) << i;
        }
        ui.force = 1; frame(); ppm("grid-seq-retrig");
        check(seen == 1u, "hold SEQ + key 1: RETRIG plays the KICK, nothing else");
        /* key 1 held: its tile white: the bottom-left tile of the grid (place 12) */
        check(px_count(2, LY_STILES_Y + 3u * LY_STILE_H + 1u, 56, LY_STILE_H - 2, C_WHITE) > 500u &&
              px_count(2, LY_STILES_Y + 1u, 56, LY_STILE_H - 2, C_WHITE) < 100u,
              "hold SEQ: the held key's tile is the KICK's place (bottom left of the grid), not key 1's top-left place");
    }
    fm1_in.notes = 0; frames(3);
    release(B_SEQ); frames(3);
    transport_req = 2; frames(3);
}

/* ---- names: every short name fits its tile (with an M or S tab), and the hold SEQ tile */
static void names_fit(void)
{
    uint32_t l, ok = 1;
    for (l = 0; l < DRUM_LANES; l++) {
        char nm[8];
        uint32_t w;
        te_upper(nm, LANE_SHORT[l], sizeof nm);
        w = text_w(&FONT_S, nm);
        ok &= 29u + (w + 1u) / 2u <= 57u && w <= 54u;   /* (58 / 56 px tiles, unmuted; the tabbed ones: drawn, below) */
    }
    check(ok, "names: all 16 short names (C-BELL, SNARE2, KICK 2, OP HAT, LO TOM, HI TOM ...) fit their tiles, with the M / S tab too");
}

int main(int argc, char **argv)
{
    uint32_t i;
    outdir = argc > 1 ? argv[1] : "build/host";
    r_a = mmap(0, GR_NS * 2u * sizeof(int32_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    r_b = mmap(0, GR_NS * 2u * sizeof(int32_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    hits_a = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
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
    check(drum_lane == 0u, "a new project: the drum screen starts on KICK (slot 1, key 1 played by hand)");
    table();
    names_fit();
    mixer_keys();
    mixer_audio();
    drum_map_place();
    select_order();
    keys_elsewhere();
    printf("grid: %s (%d failed)\n", fails ? "FAILED" : "all ok", fails);
    return fails;
}
