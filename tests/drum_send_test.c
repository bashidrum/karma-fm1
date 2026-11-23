/* SPDX-License-Identifier: GPL-3.0-only */
/* each drum sound's own sends (drums.c drum_send): the project round trip, and the audio (a sound's send wets that
 * sound alone).  cc -O1 -w -Ibuild/gen -Ifirmware/src -o build/host/drum_send_test tests/drum_send_test.c -lm */
#define main hostsim_main
#include <stddef.h>
#include "hostsim.c"
#undef main
#define PROJ_HOST 1
static uint32_t trk_def_engine(uint32_t i) { static const uint8_t E[NPART] = {0, 1, 3}; return E[i % NPART]; }
#include "../firmware/src/project.c"
static int fails;
static void check(const char *w, int ok) { printf("%-70s %s\n", w, ok ? "ok" : "FAIL"); fails += !ok; }
/* the energy of the output from 0.6 s to 1.4 s after a hit of lane l (its dry sound is over: the tail is the sends') */
static double tail(uint32_t l)
{
    static int32_t o[CTL * 2];
    double e = 0; uint32_t b, i;
    for (b = 0; b < (uint32_t)(FS * 3 / CTL); b++) mix_block(o, CTL);       /* silence first */
    drum_on(LANE_NOTE[l], 110);
    for (b = 0; b < (uint32_t)(FS * 14 / 10 / CTL); b++) {
        mix_block(o, CTL);
        if (b > (uint32_t)(FS * 6 / 10 / CTL)) for (i = 0; i < CTL * 2u; i++) e += (double)o[i] * o[i];
    }
    return e;
}
int main(void)
{
    static project_t p;
    uint32_t l; int same = 1;
    double dry_snare, dry_kick, wet_snare, kick_after, dly_hat, hat_dry;
    host_tracks_init();
    kit_set((uint32_t)(DRUM_DEFAULT_KIT));
    song.g[G_DRREV] = 0; song.g[G_DRDLY] = 0;
    for (l = 0; l < DRUM_LANES; l++) { sl_send_set(l, 0, (l * 5u + 3u) % 16u); sl_send_set(l, 1, (l * 3u + 1u) % 8u); }
    proj_capture(&p);
    check("capture: the checksum holds", p.c.sum == proj_sum(&p.c) && proj_ok(&p.c));
    for (l = 0; l < DRUM_LANES; l++) sl_send_set(l, 0, 0), sl_send_set(l, 1, 0);
    proj_kit_take((const uint8_t *)&p, (int)sizeof p, &p.c);
    proj_apply(&p.c, 1);
    for (l = 0; l < DRUM_LANES; l++) same &= sl_send(l, 0) == (l * 5u + 3u) % 16u && sl_send(l, 1) == (l * 3u + 1u) % 8u;
    check("load (wave 12: the slots in the ext): the 16 sounds' reverb and delay sends come back", same);
    for (l = 0, same = 1; l < DRUM_LANES; l++) same &= p.c.d.dsend[l] == pkit.s[l].send;
    check("the core keeps each sound's sends too (dsend, for a firmware before wave 12)", same);
    {   /* the kit itself: a load puts it back; a section never changes it (wave 12: the kit is the project's) */
        static const int16_t K[3] = {5, 15, 45};       /* 808, HOUSE, MY HEX 4 */
        int ok = 1, ok2 = 1; uint32_t j;
        for (j = 0; j < 3u; j++) {
            kit_set((uint32_t)(K[j])); proj_capture(&p); kit_set(0); proj_kit_take((const uint8_t *)&p, (int)sizeof p, &p.c);
            proj_apply(&p.c, 1); ok &= pkit.base == K[j] && !memcmp(pkit.s[2].snd, kit_snd((uint32_t)K[j], 2), 22);
            kit_set((uint32_t)(K[j])); proj_capture(&p); kit_set(0); proj_apply(&p.c, 0); ok2 &= pkit.base == 0u;
        }
        check("the drum kit comes back with a load (808, HOUSE, MY HEX 4)", ok);
        check("a section (pattern A to D) leaves the kit as it is (wave 12: the project's 16 slots)", ok2);
        kit_set((uint32_t)(DRUM_DEFAULT_KIT));
    }
    {   /* a format 7 project without the sends' flag (rsv[0] 0x40, a project from before 2.5): every sound dry; with it,
         * the bits in the drum track's P_E1 .. P_E7 (7 a sound) */
        static project_v7_t v;
        static project_t keep;
        proj_capture(&keep);                           /* (the project as it is: back after, its globals too) */
        memset(&v, 0, sizeof v);
        v.magic = PROJ_MAGIC_V7, v.size = sizeof v;
        v.t[TRK_DRUM].e[1] = 0x7F;                     /* (sound 0: every bit) */
        v.sum = proj_sum_v7(&v);
        same = proj_import(&p.c, &v, (int)sizeof v, PROJ_V7_TMP);
        proj_kit_take((const uint8_t *)&p, 0, &p.c);
        proj_apply(&p.c, 1);
        for (l = 0; l < DRUM_LANES; l++) same &= !sl_send(l, 0) && !sl_send(l, 1);
        check("a format 7 project from before (no flag): every sound dry", same);
        v.rsv[0] = 0x40;
        v.sum = proj_sum_v7(&v);
        same = proj_import(&p.c, &v, (int)sizeof v, PROJ_V7_TMP);
        proj_kit_take((const uint8_t *)&p, 0, &p.c);
        proj_apply(&p.c, 1);
        same &= sl_send(0, 0) == 127u && sl_send(0, 1) == 127u   /* (0..15, 0..7 then; 0..127 since the drum sends moved under FX) */ && !sl_send(1, 0) && !sl_send(1, 1);
        check("a format 7 project with the flag: the sounds' sends converted", same);
        proj_kit_take((const uint8_t *)&keep, (int)sizeof keep, &keep.c);
        proj_apply(&keep.c, 1);
        for (l = 0; l < DRUM_LANES; l++) sl_send_set(l, 0, 0), sl_send_set(l, 1, 0);
    }
    song.g[G_DRREV] = 0; song.g[G_DRDLY] = 0;
    dry_snare = tail(2); dry_kick = tail(0); hat_dry = tail(4);
    sl_send_set(2, 0, DRUM_SEND_REV);
    wet_snare = tail(2); kick_after = tail(0);
    sl_send_set(2, 0, 0); sl_send_set(4, 1, DRUM_SEND_DLY);
    dly_hat = tail(4);
    printf("tail energy: snare dry %.3g wet %.3g | kick dry %.3g, with the snare's send %.3g | hat dry %.3g delayed %.3g\n",
           dry_snare, wet_snare, dry_kick, kick_after, hat_dry, dly_hat);
    check("the snare's reverb send: a tail after the snare", wet_snare > dry_snare * 50 + 1e6);
    check("the kick stays dry with it", kick_after <= dry_kick * 1.05 + 1e3);
    check("the hat's delay send: echoes after the hat", dly_hat > hat_dry * 50 + 1e6);
    return fails != 0;
}
