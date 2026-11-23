/* SPDX-License-Identifier: GPL-3.0-only */
/* PHEBUS wave 12: the user drum kits in flash (firmware/src/ukit_flash.c, the FM-1's way) on a simulated NOR: none at first
 * (MY KIT 1 .. 4 are the old MY HEX), stored, read whole and a slot at a time, a second store keeps the first, erased, a
 * store cut off before its commit record leaves the kits as they were, the other objects untouched. */
#define UKIT_FLASH_TEST 1
#define main hostsim_main
#include <stddef.h>
#include "hostsim.c"
#undef main
static uint8_t nor[0x100000];
static uint32_t prog_left = 0xFFFFFFFFu;          /* (a store cut off: the programs allowed before the power goes) */
static uint8_t flash_ok = 1;
static int st_read(uint32_t off, void *dst, uint32_t n) { memcpy(dst, nor + off, n); return 0; }
static int st_erase(uint32_t off) { memset(nor + off, 0xFF, 4096); return 0; }
static int st_prog(uint32_t off, const void *src, uint32_t n)
{
    const uint8_t *s = src;
    uint32_t i;
    if (!prog_left)
        return -5;
    prog_left--;
    for (i = 0; i < n; i++)
        nor[off + i] &= s[i];
    return 0;
}
#include "../firmware/src/storage.c"
#include "../firmware/src/ukit_flash.c"
static int fails;
static void check(int ok, const char *w) { printf("ukit: %-90s %s\n", w, ok ? "ok" : "FAIL"); fails += !ok; }
int main(void)
{
    pkit_t a, b, r;
    dslot_t d;
    uint32_t i;
    memset(nor, 0xFF, sizeof nor);
    host_tracks_init();
    check(ukit_state(0) == UK_NONE && ukit_state(5) == UK_NONE && kit_src_n() == 41u && kit_src(37) == DRUM_SYN,
          "a fresh FM-1: no user kit; MY KIT 1 .. 4 are the old MY HEX 1 .. 4");
    kit_set(DRUM_DEFAULT_KIT);
    slot_from_kit(&pkit.s[2], DRUM_SAMPLED, 2);
    a = pkit;
    kit_set(2);
    b = pkit;
    check(ukit_put(4, &a) == 0 && ukit_state(4) == UK_STORED, "MY KIT 5 stored");
    check(ukit_get(4, &r) && !memcmp(&r, &a, sizeof r), "read back whole, byte for byte");
    check(ukit_slot(4, 2, &d) && !memcmp(&d, &a.s[2], sizeof d), "one slot read alone (the 808 snare in it)");
    check(ukit_put(0, &b) == 0 && ukit_get(4, &r) && !memcmp(&r, &a, sizeof r) && ukit_get(0, &r) && !memcmp(&r, &b, sizeof r),
          "MY KIT 1 stored over the old MY HEX 1: MY KIT 5 kept");
    uk_cur = -2;
    check(ukit_state(0) == UK_STORED && ukit_state(4) == UK_STORED && ukit_state(1) == UK_NONE, "after a power cycle: the same");
    prog_left = 3;                                /* the power goes during a store: the head not written */
    check(ukit_put(4, 0) != 0, "a store cut off reports it");
    prog_left = 0xFFFFFFFFu;
    uk_cur = -2;
    check(ukit_state(4) == UK_STORED && ukit_get(4, &r) && !memcmp(&r, &a, sizeof r), "the kits as they were before it");
    check(ukit_put(4, 0) == 0 && ukit_state(4) == UK_ERASED && !ukit_get(4, &r), "erased");
    for (i = 0; i < 0x1000u; i++)
        if (nor[0xA0000u + i] != 0xFFu || nor[0xB6000u + i] != 0xFFu)
            break;
    check(i == 0x1000u, "the projects' room and the free room after it untouched");
    printf("ukit: %s\n", fails ? "FAILED" : "all ok");
    return fails;
}
