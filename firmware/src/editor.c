/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Editor protocol: SysEx for the backup page (web/editor.html, web/EDITOR_PROTOCOL.md). It keeps only the
 * device's identity (INFO), PING, the backup / restore of its data (BK_LIST / GET / PUT, v6) and the read-only load
 * figures (STAT, PROF, CRASH; PCOST / PVAR in KARMA_PCOST builds) for tools/karma_*.py. KARMA's editing commands
 * (sounds, steps, tracks, presets, projects, samples, kits, live sync: 2..33, 37..42, 68..76) are cut: their numbers
 * are kept so the protocol does not move, and the device does not answer them.
 *   F0 7D 46 4C cmd args.. F7     (7D = non-commercial ID, "FL")
 * Every request it knows gets a reply with the same cmd. Frames arrive through sx_frame (usb.c), replies leave through
 * ota_wire_send(). */
#define ED_HDR0 0x7D
#define ED_HDR1 0x46
#define ED_HDR2 0x4C
enum { ED_INFO = 1, ED_PING = 25, ED_BK_LIST = 34, ED_BK_GET = 35, ED_BK_PUT = 36,
       ED_STAT = 90,                                                            /* (3.0.4) the load and the USB, read only */
       ED_PCOST = 91, ED_PVAR = 92, ED_CRASH = 93,                              /* (3.0.5) KARMA_PCOST builds: the stages' time */
       ED_PROF = 94 };                                                          /* (3.1.3) where mix_block's time goes, read only */
#define ED_PROTO 11u                                   /* the protocol version INFO ends with (backup: 6 and later) */

static uint8_t ed_out[600];
static uint32_t ed_n;

static void ed_begin(uint32_t cmd)
{
    ed_out[0] = 0xF0;
    ed_out[1] = ED_HDR0;
    ed_out[2] = ED_HDR1;
    ed_out[3] = ED_HDR2;
    ed_out[4] = (uint8_t)cmd;
    ed_n = 5;
}
static void ed_b(uint32_t v)
{
    if (ed_n < sizeof ed_out - 1u)
        ed_out[ed_n++] = (uint8_t)(v & 0x7Fu);
}
static void ed_str(const char *s, uint32_t max)   /* ASCII, 0-terminated */
{
    uint32_t i;
    for (i = 0; s && s[i] && i < max; i++)
        ed_b((uint8_t)s[i] & 0x7Fu);
    ed_b(0);
}
static void ed_send(void)
{
    ed_out[ed_n++] = 0xF7;
    ota_wire_send(ed_out, ed_n);
}

/* ---- pack7: 7-bit SysEx bytes <-> 8-bit data ---- */
static uint32_t ed_unpack7(const uint8_t *a, uint32_t na, uint8_t *out, uint32_t max)
{
    uint32_t n = 0;                                 /* groups: msb byte, then up to 7 bytes */
    while (na && n < max) {
        uint32_t m = *a++, j;
        na--;
        for (j = 0; j < 7u && na && n < max; j++, na--)
            out[n++] = (uint8_t)(*a++ | ((m >> j) & 1u) << 7);
    }
    return n;
}
static void ed_pack7(const uint8_t *p, uint32_t n)      /* pack7: a top-bits byte, then up to 7 bytes */
{
    while (n) {
        uint32_t k = n > 7u ? 7u : n, m = 0, i;
        for (i = 0; i < k; i++)
            m |= (uint32_t)(p[i] >> 7) << i;
        ed_b(m);
        for (i = 0; i < k; i++)
            ed_b(p[i] & 127u);
        p += k;
        n -= k;
    }
}
static uint8_t ed_unpack_buf[256] __attribute__((aligned(4)));   /* (a backup PUT's data, unpacked) */

/* ---- v6: backup / restore (web/EDITOR_PROTOCOL.md). Objects: 0 the working project, 1 the settings
 * (colours, calibration, the lights, SYNC), 2..5 the projects A..D, 6..7 the user preset
 * banks, 8 the FM6 patch bank (v9), 9 the SYN drum kits (v10), 10 the MIDI part (2.6). (32..35 were the user sample
 * slots: cut, PHEBUS wave 1; no longer listed, and a restore of them is refused with rc 1.)
 * LIST takes a snapshot of the working project and the settings; GET reads 1..256 bytes of an object.
 * PUT stages one object in RAM (begin: id, length, CRC-32; data; commit), checks it as a load would,
 * then writes it through the usual A/B commit: a cut-off restore never leaves half an object. */
#if FELUCCA_FLASH
#define ED_BK_RAW ((uint8_t *)&proj_tmp)                  /* the staging RAM (main loop, as the project loads) */
_Static_assert(sizeof proj_tmp >= sizeof(project_t) && sizeof proj_tmp >= sizeof(up_bank_t) &&
               sizeof proj_tmp >= sizeof(persist_t), "backup staging");
static persist_t ed_bk_set;                             /* LIST's snapshot of the settings */
static uint8_t ed_bk_ml[MIDI_PART];                     /* .. and of the MIDI part (2.6, object 10: project.c) */
static uint8_t ed_bk_valid, ed_bk_put, ed_bk_id;
static uint32_t ed_bk_len, ed_bk_crc, ed_bk_pos, ed_bk_ms;
static void ed_bk_u32(uint32_t v) { uint32_t i; for (i = 0; i < 5u; i++) ed_b((v >> (7u * i)) & 127u); }
static uint32_t ed_bk_r32(const uint8_t *a)
{
    return (uint32_t)a[0] | (uint32_t)a[1] << 7 | (uint32_t)a[2] << 14 | (uint32_t)a[3] << 21 | (uint32_t)a[4] << 28;
}

static const uint8_t *ed_bk_obj(uint32_t id, uint32_t *len)   /* 0 = no such object; *len 0 = empty */
{
    *len = 0;
    if (id == 0u) {
        *len = sizeof(project_t);
        return ED_BK_RAW;
    }
    if (id == 1u) {
        *len = sizeof ed_bk_set;
        return (const uint8_t *)&ed_bk_set;
    }
    if (id >= 2u && id <= 5u) {
        if (project_used(id - 2u))
            *len = sizeof proj_slot[0];
        return (const uint8_t *)&proj_slot[id - 2u];
    }
    if (id == 6u || id == 7u) {
        if (up_bank[id - 6u].magic == UP_BANK_MAGIC)
            *len = sizeof up_bank[0];
        return (const uint8_t *)&up_bank[id - 6u];
    }
    if (id == 8u)                                         /* (the FM6 patch bank: FM6 is gone, empty) */
        return (const uint8_t *)&up_bank[0];
    if (id == 9u) {                                       /* SLOOP 2.5: the SYN drum kits (drum_synth.c dsu) */
        (void)dsu_kit(0);
        *len = sizeof dsu;
        return (const uint8_t *)&dsu;
    }
    if (id == 10u) {                                      /* SLOOP 2.6: the MIDI part (the MIDI LEARN map, GLO > MIDI's
                                                         * channels): listed only while it is not the default (a backup
                                                         * without it stays one an older editor reads; its restore leaves
                                                         * them as they are) */
        if (!midi_part_default(ed_bk_ml))
            *len = sizeof ed_bk_ml;
        return ed_bk_ml;
    }
    return 0;
}
static const uint8_t ED_BK_IDS[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};   /* 9: SYN kits (2.5), 10: MIDI LEARN (2.6) */

/* a flash erase silences the audio for ~50 ms and stalls USB: only while stopped (as the panel) */
static uint32_t ed_flash_busy(void) { return song.playing || transport_req; }

static uint32_t ed_bk_commit(void)
{
    uint8_t *raw = ED_BK_RAW;
    uint32_t id = ed_bk_id, n = ed_bk_len;
    if (ed_bk_pos != n || st_crc32(raw, n) != ed_bk_crc)
        return 2;
    if (id == 1u)
        return settings_restore(raw, n);
    if (id <= 5u)
        return project_restore(id == 0u ? 4u : id - 2u, raw, n);
    if (id == 6u || id == 7u) {                           /* a user preset bank (n 0: empty) */
        const up_bank_t *b = (const up_bank_t *)raw;
        if (ed_flash_busy())
            return 3;
        if (n && (n != sizeof *b || b->magic != UP_BANK_MAGIC || b->rsize != sizeof(up_rec_t) || b->nslot != UP_PER_BANK))
            return 2;
        if (!flash_ok || st_save(OBJ_UPRESET0 + id - 6u, raw, n))
            return 4;
        memset(&up_bank[id - 6u], 0, sizeof up_bank[0]);
        if (n)
            memcpy(&up_bank[id - 6u], raw, n);
        up_bank_check(id - 6u, (int)n);
        up_gen++;
        return 0;
    }
    if (id == 8u)                                         /* (the FM6 patch bank: FM6 is gone, ignored) */
        return 0;
    if (id == 9u) {                                       /* SLOOP 2.5: the SYN drum kits, written with the settings */
        persist_t p;
        uint32_t k;
        if (ed_flash_busy())
            return 3;
        if (n != sizeof(dsu_bank_t) || !dsu_valid((const dsu_bank_t *)raw))
            return 2;
        memcpy(&dsu, raw, sizeof dsu);
        for (k = 0; k < DSU_N; k++)
            dsu_fix_kit(&dsu.k[k]);
        persist_fill(&p);
        if (!flash_ok || settings_write(&p))
            return 4;
        persist_saved = p;
        return 0;
    }
    if (id == 10u) {                                      /* SLOOP 2.6: the MIDI part, written with the settings */
        persist_t p;
        if (ed_flash_busy())
            return 3;
        if (n != MIDI_PART && n != 2u * ML_N && n != 2u * ML_N + 4u)
            return 2;
        midi_part_read(raw, n);                           /* (every entry and channel checked) */
        persist_fill(&p);
        if (!flash_ok || settings_write(&p))
            return 4;
        persist_saved = p;
        return 0;
    }
    return 1;
}

static int ed_backup(uint32_t cmd, const uint8_t *a, uint32_t na)   /* 1: a backup command (reply built) */
{
    uint32_t i, len, rc;
    const uint8_t *p;
    if (cmd == ED_BK_LIST) {
        rc = !flash_ok ? 4u : 0u;
        if (!rc) {
            proj_capture((project_t *)ED_BK_RAW);           /* the working project, as it is now */
            persist_fill(&ed_bk_set);
            midi_part_fill(ed_bk_ml);
            ed_bk_valid = 1;
            ed_bk_put = 0;
        }
        ed_b(rc);
        ed_b(rc ? 0u : (uint32_t)sizeof ED_BK_IDS - (ed_bk_obj(10u, &len) && !len ? 1u : 0u));
        for (i = 0; !rc && i < sizeof ED_BK_IDS; i++) {
            p = ed_bk_obj(ED_BK_IDS[i], &len);
            if (ED_BK_IDS[i] == 10u && !len)
                continue;                                 /* (nothing learned: not in the list) */
            ed_b(ED_BK_IDS[i]);
            ed_bk_u32(len);
            ed_bk_u32(st_crc32(p, len));
            fm1_wdt_feed();
        }
        return 1;
    }
    if (cmd == ED_BK_GET) {                               /* id, off (5), count (2) -> id, rc, off, count, data */
        uint32_t off = na >= 6u ? ed_bk_r32(a + 1) : 0u, count = na >= 8u ? (uint32_t)a[6] | (uint32_t)a[7] << 7 : 0u;
        p = na >= 1u ? ed_bk_obj(a[0], &len) : 0;
        rc = na != 8u || !p || !count || count > 256u || off > len || count > len - off ? 1u
             : !ed_bk_valid || (a[0] == 0u && ed_bk_put) ? 5u : 0u;   /* 5: LIST first (the snapshot is gone) */
        ed_b(na ? a[0] : 127u);
        ed_b(rc);
        ed_bk_u32(off);
        ed_b(rc ? 0u : count & 127u);
        ed_b(rc ? 0u : count >> 7);
        if (!rc)
            ed_pack7(p + off, count);
        return 1;
    }
    if (cmd == ED_BK_PUT) {                               /* op, id, ... -> op, id, rc */
        uint32_t op = na >= 2u ? a[0] : 99u, id = na >= 2u ? a[1] : 127u;
        rc = 1;
        if (!flash_ok) {
            rc = 4;
        } else if (op == 0u && na == 12u && (id <= 10u)) {  /* begin: id, length (5), CRC-32 (5) */
            len = ed_bk_r32(a + 2);
            if (id >= 2u || len) {                        /* (the working project and the settings are never empty) */
                if (len <= sizeof proj_tmp) {
                    ed_bk_put = 1;
                    ed_bk_valid = 0;                      /* (the staging RAM is the snapshot's) */
                    ed_bk_id = (uint8_t)id;
                    ed_bk_len = len;
                    ed_bk_crc = ed_bk_r32(a + 7);
                    ed_bk_pos = 0;
                    ed_bk_ms = fm1_ms;
                    rc = 0;
                }
            }
        } else if (!ed_bk_put || ed_bk_id != id || fm1_ms - ed_bk_ms > 15000u) {
            rc = 5;                                       /* no begin for this object (or too long ago) */
        } else if (op == 1u && na >= 9u && ed_bk_r32(a + 2) == ed_bk_pos) {   /* data: off (5), pack7 */
            uint32_t k = ed_unpack7(a + 7, na - 7u, ed_unpack_buf, sizeof ed_unpack_buf);
            if (k && k <= ed_bk_len - ed_bk_pos) {
                memcpy(ED_BK_RAW + ed_bk_pos, ed_unpack_buf, k);
                ed_bk_pos += k;
                ed_bk_ms = fm1_ms;
                rc = 0;
            }
        } else if (op == 2u && na == 2u) {                /* commit */
            rc = ed_bk_commit();
            ed_bk_put = 0;
            if (!rc) {
                sync_reload = 1;
                ui.force = 1;
            }
        } else if (op == 3u && na == 2u) {                /* abort */
            ed_bk_put = 0;
            rc = 0;
        }
        ed_b(op & 127u);
        ed_b(id & 127u);
        ed_b(rc);
        return 1;
    }
    return 0;
}
#else
static int ed_backup(uint32_t cmd, const uint8_t *a, uint32_t na)   /* no flash: nothing to back up */
{
    (void)a;
    (void)na;
    if (cmd < ED_BK_LIST || cmd > ED_BK_PUT)
        return 0;
    ed_b(4);
    return 1;
}
#endif

static void ed_handle(const uint8_t *f, uint32_t n)   /* f: the bytes between F0 and F7 */
{
    uint32_t cmd = f[3], i;
    const uint8_t *a = f + 4;
    uint32_t na = n - 4u;
    (void)a;
    (void)na;
    ed_begin(cmd);
    if (ed_backup(cmd, a, na)) {                           /* v6: backup / restore */
        ed_send();
        return;
    }
    switch (cmd) {
    case ED_INFO:
        ed_str("FELUCCA " FELUCCA_VERSION, 24);
        ed_b(NENGINES);
        ed_b(P_COUNT);
        ed_b(G_COUNT);
        ed_b(NSTEP);
        ed_b(P_E0);
        for (i = 0; i < NENGINES; i++)
            ed_str(ENGINES[i]->name, 8);
        ed_b(NTRK);                                       /* v3 */
        ed_b(ED_PROTO);                                   /* v5..: the protocol version */
        break;
    case ED_PING:
        ed_b(0);
        break;
    case ED_STAT:                                          /* (3.0.4) the FM-1's load and its USB, readable in USB STEMS (no
                                                            * console there): 5-byte numbers, in this order (tools/karma_stat.py) */
        ed_bk_u32(fm1_ms);
        ed_bk_u32(felucca_dbg.halves);
        ed_bk_u32(felucca_dbg.late);
        ed_bk_u32(felucca_dbg.max_us);
        ed_bk_u32(felucca_dbg.last_us);
        ed_bk_u32(song.cpu_q8);
        ed_bk_u32(shed_count);
        ed_bk_u32(voice_kills);
        ed_bk_u32(song.playing);
        ed_bk_u32((uint32_t)song.g[G_BPM]);
#if KARMA_DUAL_CORE
        ed_bk_u32((uint32_t)audio_worker_online | (uint32_t)fm1_core1_barred() << 1);
        ed_bk_u32(fm1_core1_fault.count);
        ed_bk_u32(audio_worker.timeouts);
        ed_bk_u32(w_mask);
        for (i = 0; i <= NPART; i++)
            ed_bk_u32(core_peak[i] / 16u / FM1_TICKS_PER_US);
#else
        for (i = 0; i < 4u + NPART + 1u; i++)
            ed_bk_u32(0);
#endif
#if FELUCCA_UAC
        ed_bk_u32(usb_stems);
        ed_bk_u32(uac.underruns);
        ed_bk_u32(uac.overruns);
        ed_bk_u32(uac.missed);
        ed_bk_u32(uac.stalls);
#else
        for (i = 0; i < 5u; i++)
            ed_bk_u32(0);
#endif
        ed_bk_u32(felucca_dbg.nested);
        ed_bk_u32(t5_nested_ticks / FM1_TICKS_PER_US);
        ed_bk_u32(prof_blk_max / FM1_TICKS_PER_US);
        ed_bk_u32(usb.rx_pkts);
        ed_bk_u32(felucca_dbg.timer_irqs);                  /* (3.1.5) TIMER5: its ticks, the TIMER4 ticks spent in it */
        ed_bk_u32(t5_all_ticks);
#if KARMA_DUAL_CORE && KARMA_FLIPS
        ed_bk_u32(flip_bad[0]);                             /* (3.1.5 test 4) wrong reads of zeros: CPU0, CPU1, CPU1's last */
        ed_bk_u32(flip_bad[1]);
        ed_bk_u32(flip_last[1]);
        ed_bk_u32(flip_reads[1]);
        ed_bk_u32(audio_worker.rejected);
#else
        for (i = 0; i < 5u; i++)
            ed_bk_u32(0);
#endif
#if FELUCCA_UAC
        ed_bk_u32(uac.win_rs_lo);                           /* (3.1.5 test 6) the USB ring since the last read: lowest fill */
        ed_bk_u32(uac.win_pk_lo);                           /* at a render start, at a packet (then reset) */
        uac.win_rs_lo = uac.win_pk_lo = 0xFFFFFFFFu;
        ed_bk_u32(uac.adj_up);
        ed_bk_u32(uac.adj_down);
        ed_bk_u32(uac.repaid);
        ed_bk_u32(uac.owe);
#else
        for (i = 0; i < 6u; i++)
            ed_bk_u32(0);
#endif
        ed_bk_u32(audio_lost);
#if KARMA_DUAL_CORE && KARMA_BAL_MEAS
        ed_bk_u32((uint32_t)(bal_last / (int32_t)FM1_TICKS_PER_US) + 100000u);   /* (test 10) CPU1 later than CPU0, us (+ 100000) */
#else
        ed_bk_u32(100000u);
#endif
        break;
    case ED_PROF:                                          /* (3.1.3) mix_block's parts (fx.c PF_*): the blocks, then each part's
                                                            * us summed and its worst block's, since the last read (then 0),
                                                            * any build; console `prof` without the console (tools/karma_prof.py) */
        ed_bk_u32(prof_blocks);
        for (i = 0; i < PF_N; i++) {
            ed_bk_u32(prof_ticks[i] / FM1_TICKS_PER_US);
            ed_bk_u32(prof_max[i] / FM1_TICKS_PER_US);
            prof_ticks[i] = prof_max[i] = 0;
        }
        prof_blocks = 0;
#if KARMA_DUAL_CORE
        for (i = 0; i < NPART + 2u; i++) {                  /* each part's, the drums', CPU1's own longest block */
            ed_bk_u32(core_max[i] / FM1_TICKS_PER_US);
            core_max[i] = 0;
        }
#else
        for (i = 0; i < NPART + 2u; i++)
            ed_bk_u32(0);
#endif
        ed_bk_u32(prof_blk_max / FM1_TICKS_PER_US);         /* the longest block, and its parts */
        for (i = 0; i < PF_N; i++)
            ed_bk_u32(prof_worst[i] / FM1_TICKS_PER_US);
        prof_blk_max = 0;
        break;
    case ED_CRASH:                                         /* (3.0.5) the last crash's record (hal fm1_crash, kept over resets and
                                                            * updates until power-off), any USB mode: every word of it */
        {
            const uint32_t *w = (const uint32_t *)&fm1_crash;
            for (i = 0; i < sizeof fm1_crash / 4u; i++)
                ed_bk_u32(w[i]);
            w = (const uint32_t *)&fm1_core1_last;          /* then the last CPU1 fault (hal fm1_irq.h) */
            for (i = 0; i < sizeof fm1_core1_last / 4u; i++)
                ed_bk_u32(w[i]);
        }
        break;
#if KARMA_PCOST
    case ED_PCOST:                                         /* (3.0.5) per stage (dsp.c PC_*): us, then calls; cleared */
        for (i = 0; i < PC_N; i++) {
            ed_bk_u32(pc_ticks[i] / FM1_TICKS_PER_US);
            ed_bk_u32(pc_calls[i]);
            pc_ticks[i] = pc_calls[i] = 0;
        }
        ed_bk_u32(felucca_dbg.late);
        ed_bk_u32(song.cpu_q8);
        for (i = 0; i < PC_N; i++) {                       /* (3.1.3) each stage's longest call, us */
            ed_bk_u32(pc_max[i] / FM1_TICKS_PER_US);
            pc_max[i] = 0;
        }
        break;
    case ED_PVAR:                                          /* [v]: which variant of a stage runs (dsp.c pc_var); -> v */
        if (na >= 1u)
            pc_var = a[0];
        ed_bk_u32(pc_var);
        break;
#endif
    default:
        return;                                            /* (KARMA's editing commands, cut: no reply) */
    }
    ed_send();
}

/* main loop: editor frames first; anything else stays for ota_service() */
static void ed_service(void)
{
    const uint8_t *p;
    uint32_t n;
    if (!ota_frame_get(&p, &n) || n < 4u || p[0] != ED_HDR0 || p[1] != ED_HDR1 || p[2] != ED_HDR2)
        return;
    if (p[3] == ED_BK_PUT) {                               /* large frames: handled in place, then freed */
        ed_handle(p, n);
        ota_frame_done();
        return;
    }
    {
        static uint8_t f[64];
        uint32_t k = n > sizeof f ? sizeof f : n, i;
        for (i = 0; i < k; i++)
            f[i] = p[i];
        ota_frame_done();                                  /* free the frame buffer before replying */
        ed_handle(f, k);
    }
}
