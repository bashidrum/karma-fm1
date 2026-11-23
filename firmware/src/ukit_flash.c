/* SPDX-License-Identifier: GPL-3.0-only */
/* (PHEBUS wave 12) The user kits in flash (ukit.c's four functions, the FM-1's way): one storage.c object, OBJ_UKIT (A/B
 * sectors at 0xB4000 / 0xB5000, in the room free since wave 2): a head, then the six kits whole (pkit_t, 592 bytes each):
 * 3,564 bytes of the sector's 3,840. Nothing is mirrored in RAM: the head's six states are kept (6 bytes) and a kit or one
 * slot is read from the current copy when it is wanted (a load, a detent of the sound list on a user kit's sound). A store
 * reads the current copy into storage.c's st_buf, changes the one kit there and writes the other copy (the A/B commit:
 * a store cut off leaves the kits as they were). Included by project.c after storage.c. */
#define UK_MAGIC 0x31544B55u                      /* "UKT1" */
typedef struct {
    uint32_t magic;
    uint8_t st[UKIT_N];                           /* UK_NONE, UK_STORED, UK_ERASED */
    uint8_t rsv[2];
} uk_head_t;
#define UK_LEN (sizeof(uk_head_t) + UKIT_N * sizeof(pkit_t))
_Static_assert(sizeof(uk_head_t) == 12u && UK_LEN <= ST_PAYLOAD_MAX, "the user kits fit one flash object");
static uint8_t uk_st[UKIT_N];                     /* the states as flash has them */
static int8_t uk_cur = -2;                        /* the current copy (0, 1), -1 none, -2 not looked at yet */
static uint32_t uk_seq;
static void ukit_boot(void)                       /* (project.c persist_boot, and after every store) */
{
    st_hdr_t h;
    uint32_t n;
    memset(uk_st, 0, sizeof uk_st);
    uk_cur = (int8_t)st_current(OBJ_UKIT, &h);    /* (its body checked, into st_buf) */
    uk_seq = uk_cur >= 0 ? h.seq : 0u;
    if (uk_cur >= 0 && (h.len != UK_LEN || ((const uk_head_t *)(const void *)st_buf)->magic != UK_MAGIC))
        uk_cur = -1;
    for (n = 0; uk_cur >= 0 && n < UKIT_N; n++) {
        uint8_t s = ((const uk_head_t *)(const void *)st_buf)->st[n];
        uk_st[n] = (uint8_t)(s <= UK_ERASED ? s : UK_NONE);
    }
}
static uint32_t ukit_state(uint32_t n)
{
    if (!flash_ok)
        return n < DSU_N ? UK_NONE : UK_ERASED;
    if (uk_cur == -2)
        ukit_boot();
    return n < UKIT_N ? uk_st[n] : UK_ERASED;
}
static int uk_read(uint32_t off, void *d, uint32_t len)   /* from the current copy's payload */
{
    if (uk_cur < 0)
        return 0;
    return st_read(st_sector(OBJ_UKIT, (uint32_t)uk_cur) + ST_PAYLOAD_OFF + off, d, len) == 0;
}
static int ukit_get(uint32_t n, pkit_t *k)
{
    if (ukit_state(n) != UK_STORED)
        return 0;
    return uk_read(sizeof(uk_head_t) + n * sizeof(pkit_t), k, sizeof *k);
}
static int ukit_slot(uint32_t n, uint32_t l, dslot_t *d)
{
    if (ukit_state(n) != UK_STORED)
        return 0;
    return uk_read(sizeof(uk_head_t) + n * sizeof(pkit_t) + __builtin_offsetof(pkit_t, s) + (l & 15u) * sizeof(dslot_t), d,
                   sizeof *d);
}
/* kit n stored (k 0: erased): the object as it is into st_buf, the kit changed there, the other copy written */
static int ukit_put(uint32_t n, const pkit_t *k)
{
    uk_head_t *hd = (uk_head_t *)(void *)st_buf;
    uint32_t base, off, c;
    st_hdr_t h;
    int rc;
    if (!flash_ok || n >= UKIT_N)
        return -1;
    ukit_boot();
    if (uk_cur < 0) {                             /* (none yet: an empty object) */
        memset(st_buf, 0, UK_LEN);
        hd->magic = UK_MAGIC;
    }
    hd->st[n] = (uint8_t)(k ? UK_STORED : UK_ERASED);
    if (k)
        memcpy(st_buf + sizeof(uk_head_t) + n * sizeof(pkit_t), k, sizeof *k);
    else
        memset(st_buf + sizeof(uk_head_t) + n * sizeof(pkit_t), 0, sizeof(pkit_t));
    c = uk_cur == 0 ? 1u : 0u;                    /* write the other copy */
    base = st_sector(OBJ_UKIT, c);
    if ((rc = st_erase(base)) != 0)
        return rc;
    for (off = 0; off < UK_LEN; off += 256u)
        if ((rc = st_prog(base + ST_PAYLOAD_OFF + off, st_buf + off, UK_LEN - off > 256u ? 256u : UK_LEN - off)) != 0)
            return rc;
    h.magic = ST_MAGIC;
    h.type = (uint16_t)OBJ_UKIT;
    h.slot = (uint16_t)c;
    h.seq = uk_seq + 1u;
    h.len = UK_LEN;
    h.crc = st_crc32(st_buf, UK_LEN);
    h.rsv[0] = h.rsv[1] = 0xFFFFFFFFu;
    h.hcrc = st_crc32(&h, sizeof h - 4u);
    if ((rc = st_prog(base, &h, sizeof h)) != 0)   /* the commit record, last */
        return rc;
    ukit_boot();
    return uk_cur == (int8_t)c && uk_st[n] == (k ? UK_STORED : UK_ERASED) ? 0 : -7;
}
