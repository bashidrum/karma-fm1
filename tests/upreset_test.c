/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Host test of the user preset record (firmware/src/upreset.c, -DUP_HOST part):
 * a bank round trip through storage.c on a simulated NOR,
 * bank / record version checks, map-by-count, pattern <-> steps. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define __attribute__(x)
#define UP_HOST 1
#include "../firmware/src/core.h"

static uint8_t nor[0x100000];
static int st_read(uint32_t off, void *dst, uint32_t n) { memcpy(dst, nor + off, n); return 0; }
static int st_erase(uint32_t off) { memset(nor + off, 0xFF, 4096); return 0; }
static int st_prog(uint32_t off, const void *src, uint32_t n)
{
    const uint8_t *s = src;
    uint32_t i;
    for (i = 0; i < n; i++)
        nor[off + i] &= s[i];
    return 0;
}
#include "../firmware/src/storage.c"
#include "../firmware/src/upreset.c"

/* (KARMA's editor sent records with UP_PUT; that command is cut. Its parser stays here only to build test
 * records.) */
static int up_name_ok(const uint8_t *s, uint32_t n)   /* 1..12 printable ASCII */
{
    uint32_t i;
    if (!n || n > 12u)
        return 0;
    for (i = 0; i < n; i++)
        if (s[i] < 32u || s[i] > 126u)
            return 0;
    return 1;
}

/* the arguments: slot, engine, name, P_COUNT x v14, 16 x (note, flags) -> *r (values not yet
 * clamped); 0 ok, 1 bad arguments. *slot gets the slot byte when there is one. */
static int up_parse(const uint8_t *a, uint32_t na, up_rec_t *r, uint32_t *slot)
{
    uint32_t i, n, k;
    if (na < 3u)
        return 1;
    *slot = a[0];
    for (n = 0; 2u + n < na && a[2 + n]; n++)
        ;
    k = 3u + n;                                  /* after the name's 0 */
    if (a[0] >= UP_SLOTS || a[1] >= NENGINES || 2u + n >= na || !up_name_ok(a + 2, n) ||
        na < k + 2u * P_COUNT + 32u)
        return 1;
    memset(r, 0, sizeof *r);
    r->used = UP_USED;
    r->ver = UP_VER;
    r->engine = a[1];
    r->np = P_COUNT;
    for (i = 0; i < n; i++)
        r->name[i] = (char)a[2 + i];
    for (i = 0; i < P_COUNT; i++, k += 2u)
        r->p[i] = (int16_t)((int32_t)((a[k] & 127u) | (a[k + 1] & 127u) << 7) - 8192);
    for (i = 0; i < 16u; i++, k += 2u) {
        r->note[i] = a[k];
        r->flags[i] = a[k + 1];
        up_pat_norm(&r->note[i], &r->flags[i]);
    }
    return 0;
}

static int check(const char *what, int ok)
{
    printf("%-46s %s\n", what, ok ? "ok" : "FAIL");
    return ok ? 0 : 1;
}

static uint32_t put_frame(uint8_t *a, uint32_t slot, uint32_t eng, const char *name, int32_t base)
{
    uint32_t n = 0, i;
    a[n++] = (uint8_t)slot;
    a[n++] = (uint8_t)eng;
    for (i = 0; name[i]; i++)
        a[n++] = (uint8_t)name[i];
    a[n++] = 0;
    for (i = 0; i < P_COUNT; i++) {
        uint32_t u = (uint32_t)(base + (int32_t)i + 8192);
        a[n++] = u & 127u;
        a[n++] = (u >> 7) & 127u;
    }
    for (i = 0; i < 16u; i++) {
        a[n++] = (uint8_t)(i % 3u ? 40u + i : 0u);  /* rests on 0, 3, 6 .. */
        a[n++] = (uint8_t)(i == 3u ? 4u : i % 3u ? 1u : 2u);   /* step 3: tie; slide on a rest drops */
    }
    return n;
}

int main(void)
{
    uint8_t a[640];
    up_rec_t r, got;
    uint32_t n, slot = 99, i;
    int bad = 0, len, ok;
    int16_t v[P_COUNT], def[P_COUNT];
    memset(nor, 0xFF, sizeof nor);

    n = put_frame(a, 5, 2, "Bass One", -40);
    bad += check("a record", up_parse(a, n, &r, &slot) == 0 && up_valid(&r));
    {
        char nm[13];
        *up_rec(31) = r;
        memcpy(up_rec(31)->name, "Low case", 9);
        up_name(31, nm);
        bad += check("name shown upper case", !strcmp(nm, "LOW CASE"));
    }

    /* bank round trip through storage.c */
    n = put_frame(a, 17, 3, "Keys", 7);
    up_parse(a, n, &r, &slot);
    up_bank[1].magic = UP_BANK_MAGIC;
    up_bank[1].rsize = sizeof(up_rec_t);
    up_bank[1].nslot = UP_PER_BANK;
    *up_rec(17) = r;
    bad += check("bank fits one object", sizeof(up_bank_t) <= ST_PAYLOAD_MAX);
    bad += check("bank save", st_save(OBJ_UPRESET0 + 1, &up_bank[1], sizeof up_bank[1]) == 0);
    memset(up_bank, 0, sizeof up_bank);
    len = st_load(OBJ_UPRESET0 + 1, &up_bank[1], sizeof up_bank[1]);
    up_bank_check(1, len);
    got = *up_rec(17);
    bad += check("bank load: the record is back", up_used(17) && !memcmp(&got, &r, sizeof r));
    bad += check("other slots empty", !up_used(16) && !up_used(18) && !up_used(0));
    len = st_load(OBJ_UPRESET0, &up_bank[0], sizeof up_bank[0]);
    up_bank_check(0, len);
    bad += check("bank 0 never written -> empty", len < 0 && !up_used(0) && up_bank[0].magic == 0);
    bad += check("banks in 0xDC000..0xDFFFF", st_sector(OBJ_UPRESET0, 0) == 0xDC000u &&
                                                   st_sector(OBJ_UPRESET0 + 1, 1) == 0xDF000u &&
                                                   st_sector(OBJ_PROJECT0 + 3, 1) + 4096u <= 0xA0000u);
    up_bank[1].rsize = 190;                                 /* another record layout */
    up_bank_check(1, (int)sizeof up_bank[1]);
    bad += check("bank with another record size -> empty", !up_used(17));
    up_bank[1].magic = UP_BANK_MAGIC;
    up_bank[1].rsize = sizeof(up_rec_t);
    up_bank[1].nslot = UP_PER_BANK;
    *up_rec(17) = r;
    up_rec(17)->ver = UP_VER + 1u;
    bad += check("record with another version -> empty", !up_used(17));

    /* map by count: a record from a build with 2 parameters fewer */
    for (i = 0; i < P_COUNT; i++)
        def[i] = (int16_t)(1000 + i);
    r.np = P_COUNT - 2u;
    for (i = 0; i < P_COUNT; i++)
        r.p[i] = (int16_t)i;
    up_params(&r, v, def);
    ok = 1;
    for (i = 0; i < P_E0; i++)
        ok &= v[i] == (i < P_E0 - 2u ? (int16_t)i : def[i]);
    for (i = 0; i < 8u; i++)
        ok &= v[P_E0 + i] == (int16_t)(P_E0 - 2u + i);
    bad += check("np < P_COUNT: mapped by count", ok);
    /* a record saved before the SLICER (P_COUNT 53, P_E0 45): the four SLICER parameters (just
     * before P_E0) take their defaults, everything else keeps its id */
    r.np = 53;
    for (i = 0; i < 53u; i++)
        r.p[i] = (int16_t)(2000 + i);
    up_params(&r, v, def);
    ok = P_SLCR == 45 && P_SLDEPTH + 1 == P_CHORD && P_CHORD == 49 && P_TFLT == 50 && P_VLEAD == 52 && P_ITYPE == 53 &&
         P_E0 == 63;
    for (i = 0; i < 45u; i++)
        ok &= v[i] == (int16_t)(2000 + i);
    for (i = P_SLCR; i < P_E0; i++)
        ok &= v[i] == def[i];
    for (i = 0; i < 8u; i++)
        ok &= v[P_E0 + i] == (int16_t)(2000 + 45 + i);
    bad += check("old record (np 53): SLICER, CHORD and FILTER defaults, E0..E7 kept", ok);
    /* a record of SLOOP 2.0 .. 2.3 (P_COUNT 58, P_E0 50): only the FILTER (2.4) takes its default */
    r.np = 58;
    for (i = 0; i < 58u; i++)
        r.p[i] = (int16_t)(3000 + i);
    up_params(&r, v, def);
    ok = 1;
    for (i = 0; i < P_TFLT; i++)
        ok &= v[i] == (int16_t)(3000 + i);
    for (i = P_TFLT; i < P_E0; i++)
        ok &= v[i] == def[i];
    for (i = 0; i < 8u; i++)
        ok &= v[P_E0 + i] == (int16_t)(3000 + 50 + i);
    bad += check("2.3 record (np 58): FILTER, STRUM, VLEAD and the 2.6 ten defaults, CHORD and E0..E7 kept", ok);
    /* a record of SLOOP 2.4 .. 2.5 (P_COUNT 61, P_E0 53): the ten of 2.6 (INSERT, FTYPE, ESYNC, LFO 2) default */
    r.np = 61;
    for (i = 0; i < 61u; i++)
        r.p[i] = (int16_t)(4000 + i);
    up_params(&r, v, def);
    ok = 1;
    for (i = 0; i < P_ITYPE; i++)
        ok &= v[i] == (int16_t)(4000 + i);
    for (i = P_ITYPE; i < P_E0; i++)
        ok &= v[i] == def[i];
    for (i = 0; i < 8u; i++)
        ok &= v[P_E0 + i] == (int16_t)(4000 + 53 + i);
    bad += check("2.5 record (np 61): the 2.6 parameters (INSERT, FTYPE, ESYNC, LFO 2) default, the rest and E0..E7 kept", ok);
    /* a record of SLOOP 1.0 (P_COUNT 57, P_E0 49): CHORD (SLOOP 2.0) takes its default */
    r.np = 57;
    for (i = 0; i < 57u; i++)
        r.p[i] = (int16_t)(3000 + i);
    up_params(&r, v, def);
    ok = 1;
    for (i = 0; i < P_CHORD; i++)
        ok &= v[i] == (int16_t)(3000 + i);
    ok &= v[P_CHORD] == def[P_CHORD];
    for (i = 0; i < 8u; i++)
        ok &= v[P_E0 + i] == (int16_t)(3000 + 49 + i);
    bad += check("SLOOP 1.0 record (np 57): CHORD default, the rest kept", ok);
    r.np = P_COUNT;
    for (i = 0; i < P_COUNT; i++)
        r.p[i] = (int16_t)i;
    up_params(&r, v, def);
    ok = 1;
    for (i = 0; i < P_COUNT; i++)
        ok &= v[i] == (int16_t)i;
    bad += check("np == P_COUNT: as stored", ok);

    {   /* steps -> pattern (UP_STORE) */
        step_t st[NSTEP];
        memset(st, 0, sizeof st);
        for (i = 0; i < 16u; i++)
            st[i].time = ST_REST;
        st[0] = (step_t){{60, 64, 67, 0}, 3, ST_NOTE, SF_ACCENT | SF_SLIDE, 100};
        st[1] = (step_t){{0}, 0, ST_TIE, 0, 0};
        st[2] = (step_t){{50}, 0, ST_NOTE, SF_ACCENT, 0};   /* n = 0: empty */
        st[20] = (step_t){{70}, 1, ST_NOTE, 0, 90};         /* beyond 16: not stored */
        up_pat_from(&r, st);
        bad += check("steps -> pattern", r.note[0] == 60 && r.flags[0] == 3 && r.note[1] == 0 && r.flags[1] == 4 &&
                                             r.note[2] == 0 && r.flags[2] == 0 && !up_pat_empty(&r));
        memset(st, 0, sizeof st);
        up_pat_from(&r, st);
        bad += check("empty sequencer -> empty pattern", up_pat_empty(&r));
    }
    printf("%s\n", bad ? "USER PRESET TEST FAILED" : "user preset test passed");
    return bad != 0;
}
