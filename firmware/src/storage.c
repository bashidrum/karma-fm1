/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Persistent storage on the SPI NOR.
 *
 * Every object has an A/B sector pair. A save goes to the copy that is not
 * the current one: erase the sector, program the payload (from offset 256),
 * then the 32-byte header at offset 0 last. The header is the commit record;
 * on load the valid copy with the highest seq wins, so a torn write leaves
 * the previous copy in charge.
 *
 * Flash access goes through three hooks (also used by the host test):
 *   st_read(off, dst, n)   st_erase(off)   st_prog(off, src, n)
 */
#define ST_MAGIC 0x554C4546u                   /* "FELU" */
#define ST_SECTOR 4096u
#define ST_PAYLOAD_OFF 256u
#define ST_PAYLOAD_MAX (ST_SECTOR - ST_PAYLOAD_OFF)

/* flash map (FL_DATA 0x97000..0xDFFFF, FL_GLOB 0xFC000..): settings 0xFC000 / 0xFD000; user preset banks 0xDC000..0xDFFFF
 * (upreset.c); the FM6 patch bank's object number (unused since FM6 went). PHEBUS wave 2 (the new project format, project.c
 * "FUN8"): the projects are "big" objects (below), each copy two neighbouring sectors, all in 0xA0000..0xB3FFF (where KARMA's
 * user sample slots were, cut in PHEBUS wave 1): the four project slots (sections A..D) 0xA0000..0xAFFFF, the working
 * project (autosave) 0xB0000..0xB3FFF. (Wave 12) the user drum kits 0xB4000 / 0xB5000 (OBJ_UKIT). 0xB6000..0xDBFFF is free.
 * The objects of the old format (FUN7 and older) stay where they were, read once to convert and never written again: the
 * slots 0x97000..0x9EFFF, the working project's copy A 0x9F000, copy B 0xFE000. (KARMA 3.0.5's saved songs, one sector
 * each at 0xE5000, 0xE6000, 0xFB000, are released: PHEBUS wave 1 cut the songs, wave 2 drops their objects) */
/* object numbers are stored in each header (h.type): never renumber one. 9..11 were the songs (released) */
enum { OBJ_SETTINGS, OBJ_PROJECT0, OBJ_UPRESET0 = OBJ_PROJECT0 + 4, OBJ_AUTOSAVE = OBJ_UPRESET0 + 2, OBJ_FM6BANK,
       OBJ_PRJ0 = OBJ_FM6BANK + 4, OBJ_WORK = OBJ_PRJ0 + 4, OBJ_UKIT, OBJ_COUNT };   /* (wave 12: OBJ_UKIT, the user drum
                                                * kits, ukit_flash.c: a plain object at 0xB4000 / 0xB5000) */
#define ST_BIG_BASE 0xA0000u                   /* the big objects, 4 sectors each (2 copies of 2) */
#define ST_BIG_MAX (2u * ST_SECTOR - ST_PAYLOAD_OFF)   /* a big object's payload: offset 256 of its first sector to the end
                                                        * of its second, 7936 bytes */
static int st_big(uint32_t obj) { return obj >= OBJ_PRJ0 && obj <= OBJ_WORK; }
static int st_gone(uint32_t obj) { return obj > OBJ_FM6BANK && obj < OBJ_PRJ0; }   /* (the songs' numbers: no object) */
static uint32_t st_max(uint32_t obj) { return st_big(obj) ? ST_BIG_MAX : ST_PAYLOAD_MAX; }

typedef struct {
    uint32_t magic;
    uint16_t type, slot;
    uint32_t seq, len, crc, rsv[2];
    uint32_t hcrc;
} st_hdr_t;

static int st_read(uint32_t off, void *dst, uint32_t n);
static int st_erase(uint32_t off);
static int st_prog(uint32_t off, const void *src, uint32_t n);

static uint32_t st_crc32_up(uint32_t c, const void *p, uint32_t n)   /* zlib CRC-32, 4 bits per step; c: ~ the CRC so far */
{
    static const uint32_t T[16] = {
        0x00000000u, 0x1DB71064u, 0x3B6E20C8u, 0x26D930ACu, 0x76DC4190u, 0x6B6B51F4u, 0x4DB26158u, 0x5005713Cu,
        0xEDB88320u, 0xF00F9344u, 0xD6D6A3E8u, 0xCB61B38Cu, 0x9B64C2B0u, 0x86D3D2D4u, 0xA00AE278u, 0xBDBDF21Cu};
    const uint8_t *b = p;
    while (n--) {
        c ^= *b++;
        c = (c >> 4) ^ T[c & 15u];
        c = (c >> 4) ^ T[c & 15u];
    }
    return c;
}
static uint32_t st_crc32(const void *p, uint32_t n) { return ~st_crc32_up(0xFFFFFFFFu, p, n); }

static uint32_t st_sector(uint32_t obj, uint32_t copy)  /* flash offset of copy A (0) / B (1) (a big one: its first sector) */
{
    if (obj == OBJ_SETTINGS)
        return 0xFC000u + copy * ST_SECTOR;
    if (obj == OBJ_AUTOSAVE)
        return copy ? 0xFE000u : 0x9F000u;
    if (obj == OBJ_FM6BANK)                           /* (unused since FM6 went) */
        return 0xE5000u + copy * ST_SECTOR;
    if (st_big(obj))
        return ST_BIG_BASE + (obj - OBJ_PRJ0) * 4u * ST_SECTOR + copy * 2u * ST_SECTOR;
    if (obj == OBJ_UKIT)
        return 0xB4000u + copy * ST_SECTOR;
    if (obj >= OBJ_UPRESET0 && obj < OBJ_AUTOSAVE)
        return 0xDC000u + (obj - OBJ_UPRESET0) * 2u * ST_SECTOR + copy * ST_SECTOR;
    return 0x97000u + (obj - OBJ_PROJECT0) * 2u * ST_SECTOR + copy * ST_SECTOR;
}

static uint8_t st_buf[ST_PAYLOAD_MAX] __attribute__((aligned(4)));

static int st_head(uint32_t obj, uint32_t copy, st_hdr_t *h)   /* commit record valid: 0 */
{
    if (obj >= OBJ_COUNT || st_gone(obj) || copy > 1u)
        return -1;
    if (st_read(st_sector(obj, copy), h, sizeof *h))
        return -1;
    if (h->magic != ST_MAGIC || h->type != obj || h->slot != copy || h->len > st_max(obj) ||   /* (slot: the copy
                                                     * it was written to; after Felucca 1.0) */
        h->hcrc != st_crc32(h, sizeof *h - 4u))
        return -1;
    return 0;
}

static int st_body(uint32_t obj, uint32_t copy, const st_hdr_t *h)   /* payload -> st_buf, CRC ok: 0 (a big object: its CRC
                                                                       * only, read 256 bytes at a time; st_load reads it) */
{
    if (st_big(obj)) {
        uint8_t b[256];
        uint32_t off, c = 0xFFFFFFFFu, base = st_sector(obj, copy) + ST_PAYLOAD_OFF;
        for (off = 0; off < h->len; off += 256u) {
            uint32_t n = h->len - off > 256u ? 256u : h->len - off;
            if (st_read(base + off, b, n))
                return -1;
            c = st_crc32_up(c, b, n);
        }
        return ~c == h->crc ? 0 : -1;
    }
    if (st_read(st_sector(obj, copy) + ST_PAYLOAD_OFF, st_buf, h->len) || st_crc32(st_buf, h->len) != h->crc)
        return -1;
    return 0;
}

/* the current copy: the valid one with the highest seq (A on a tie), -1 when
 * neither is valid. Headers first, so only the winner's payload is read (it
 * is left in st_buf, but a big object's); *h gets its header. */
static int st_current(uint32_t obj, st_hdr_t *h)
{
    st_hdr_t a, b;
    int va = st_head(obj, 0, &a) == 0, vb = st_head(obj, 1, &b) == 0;
    if (vb && (!va || b.seq > a.seq)) {
        if (st_body(obj, 1, &b) == 0) {
            *h = b;
            return 1;
        }
        vb = 0;
    }
    if (va && st_body(obj, 0, &a) == 0) {
        *h = a;
        return 0;
    }
    if (vb && st_body(obj, 1, &b) == 0) {
        *h = b;
        return 1;
    }
    return -1;
}

/* load object into dst (up to max bytes); returns the length, or -1 */
static int st_load(uint32_t obj, void *dst, uint32_t max)
{
    uint32_t i;
    st_hdr_t h;
    int cur;
    if ((cur = st_current(obj, &h)) < 0)
        return -1;
    if (h.len > max)
        h.len = max;
    if (st_big(obj))                                  /* (its CRC checked by st_current: the payload straight into dst) */
        return st_read(st_sector(obj, (uint32_t)cur) + ST_PAYLOAD_OFF, dst, h.len) ? -1 : (int)h.len;
    for (i = 0; i < h.len; i++)
        ((uint8_t *)dst)[i] = st_buf[i];
    return (int)h.len;
}

/* an object in up to three parts, src then src2 then src3 (SLOOP 2.5: the settings and the SYN kits after them;
 * 2.6: then the MIDI LEARN map) */
static int st_save3(uint32_t obj, const void *src, uint32_t len, const void *src2, uint32_t len2, const void *src3,
                    uint32_t len3)
{
    uint32_t seq, base, off;
    int cur, rc;
    st_hdr_t h;
    if (obj >= OBJ_COUNT || st_gone(obj) || st_big(obj) || len + len2 + len3 > ST_PAYLOAD_MAX)
        return -1;
    cur = st_current(obj, &h);
    seq = cur < 0 ? 0u : h.seq;
    base = st_sector(obj, cur == 0 ? 1u : 0u);       /* write the other copy */
    for (off = 0; off < len; off++)
        st_buf[off] = ((const uint8_t *)src)[off];    /* the driver wants RAM sources */
    for (off = 0; off < len2; off++)
        st_buf[len + off] = ((const uint8_t *)src2)[off];
    len += len2;
    for (off = 0; off < len3; off++)
        st_buf[len + off] = ((const uint8_t *)src3)[off];
    len += len3;
    if ((rc = st_erase(base)) != 0)
        return rc;
    for (off = 0; off < len; off += 256u) {
        uint32_t n = len - off > 256u ? 256u : len - off;
        if ((rc = st_prog(base + ST_PAYLOAD_OFF + off, st_buf + off, n)) != 0)
            return rc;
    }
    h.magic = ST_MAGIC;
    h.type = (uint16_t)obj;
    h.slot = (uint16_t)(cur == 0 ? 1 : 0);
    h.seq = seq + 1u;
    h.len = len;
    h.crc = st_crc32(st_buf, len);
    h.rsv[0] = h.rsv[1] = 0xFFFFFFFFu;
    h.hcrc = st_crc32(&h, sizeof h - 4u);
    if ((rc = st_prog(base, &h, sizeof h)) != 0)       /* the commit record, last */
        return rc;
    {   /* read back: a write-protected or failing part must not report SAVED */
        st_hdr_t chk;
        uint32_t c = cur == 0 ? 1u : 0u;
        if (st_head(obj, c, &chk) || memcmp(&chk, &h, sizeof h) || st_body(obj, c, &chk))   /* the whole record */
            return -7;
    }
    return 0;
}
static int st_save2(uint32_t obj, const void *src, uint32_t len, const void *src2, uint32_t len2)
{
    return st_save3(obj, src, len, src2, len2, 0, 0);
}
static int st_save(uint32_t obj, const void *src, uint32_t len) { return st_save2(obj, src, len, 0, 0); }

/* (PHEBUS wave 2) a big object (a project, project.c): len bytes from src, which must be RAM (the flash driver reads it
 * as it programs; nothing is copied), then zt zero bytes (a reserved part, kept in no RAM), up to ST_BIG_MAX in all. The
 * same A/B commit as the others: both sectors of the copy that is not the current one erased, the payload programmed,
 * the header last, then everything read back */
/* (PHEBUS wave 3) len bytes of src, then tlen of tail (0: none), then zt zeros: one object */
static int st_save_big2(uint32_t obj, const void *src, uint32_t len, const void *tail, uint32_t tlen, uint32_t zt)
{
    uint32_t seq, base, off, c, crc, all, head = len;
    uint8_t z[256];
    int cur, rc;
    st_hdr_t h;
    if (!tail)
        tlen = 0;
    len += tlen;                                      /* (len: the bytes that are not zeros, from src and tail) */
    all = len + zt;
    if (!st_big(obj) || len > ST_BIG_MAX || zt > ST_BIG_MAX - len)
        return -1;
    cur = st_current(obj, &h);
    seq = cur < 0 ? 0u : h.seq;
    c = cur == 0 ? 1u : 0u;                           /* write the other copy */
    base = st_sector(obj, c);
    if ((rc = st_erase(base)) != 0 || (rc = st_erase(base + ST_SECTOR)) != 0)
        return rc;
    crc = 0xFFFFFFFFu;
    for (off = 0; off < all; off += 256u) {
        uint32_t n = all - off > 256u ? 256u : all - off, k;
        const uint8_t *pg = (const uint8_t *)src + off;
        if (off + n > head) {                         /* (a page with the tail or the zeros in it: made up here) */
            for (k = 0; k < n; k++) {
                uint32_t o = off + k;
                z[k] = o < head ? ((const uint8_t *)src)[o] : o < len ? ((const uint8_t *)tail)[o - head] : 0u;
            }
            pg = z;
        }
        crc = st_crc32_up(crc, pg, n);
        if ((rc = st_prog(base + ST_PAYLOAD_OFF + off, pg, n)) != 0)
            return rc;
    }
    h.magic = ST_MAGIC;
    h.type = (uint16_t)obj;
    h.slot = (uint16_t)c;
    h.seq = seq + 1u;
    h.len = all;
    h.crc = ~crc;
    h.rsv[0] = h.rsv[1] = 0xFFFFFFFFu;
    h.hcrc = st_crc32(&h, sizeof h - 4u);
    if ((rc = st_prog(base, &h, sizeof h)) != 0)       /* the commit record, last */
        return rc;
    {   /* read back: a write-protected or failing part must not report SAVED */
        st_hdr_t chk;
        if (st_head(obj, c, &chk) || memcmp(&chk, &h, sizeof h) || st_body(obj, c, &chk))
            return -7;
    }
    return 0;
}
static int st_save_big(uint32_t obj, const void *src, uint32_t len, uint32_t zt) { return st_save_big2(obj, src, len, 0, 0, zt); }
