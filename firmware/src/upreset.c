/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* User presets (editor protocol v2, cmds 16-21; the SAVE > USER page): 32
 * slots in two storage.c objects (OBJ_UPRESET0/1, 0xDC000..0xDFFFF), 16
 * records each, mirrored in RAM so browsing never reads flash. A record:
 * engine, name, the instrument parameters, a 16-step pattern.
 *
 * A bank whose magic, record size or slot count differ reads as empty; so
 * does a record with another UP_VER. A record keeps np = the P_COUNT it was
 * stored with; when that differs it is mapped by count: its last 8 values
 * are P_E0..P_E7, the first np - 8 are P_LEVEL.. in order, missing ones take
 * their defaults. So common parameters may only be added just before P_E0
 * (else bump UP_VER).
 * (PHEBUS wave 4) record 2: the same 192 bytes; np is P_COUNT_V8 (the parameters before wave 4, mapped as above) and
 * the 32 bytes of the 16-step pattern (never loaded: a sound never changes the sequence) hold the parameters from
 * P_COUNT_V8 on, a signed byte each (px). A record 1 still loads, its filter values mapped onto the voice filter
 * (ui.c voice_from_engine); a record 2 is shown empty by a firmware before wave 4 (it reads record 1 only).
 *
 * With -DUP_HOST (host test) only the part above #ifndef UP_HOST is built;
 * it needs nothing but core.h. */
#define UP_PER_BANK 16u
#define UP_PMAX 72u                              /* room for P_COUNT to grow */
#define UP_USED 0xA5u
#define UP_VER 3u                                /* (wave 5: the FX slots, LFO 2 and the matrix; records 1 and 2 still
                                                  * read, their DIST, INSERT and sends made slots: modmx.c fx_from_legacy) */
#ifndef PHEBUS_UP_MX
#define PHEBUS_UP_MX 1                           /* (wave 5) each record's mod matrix in the bank, after the records */
#endif
#define UP_BANK_MAGIC 0x31425055u                /* "UPB1" */
typedef struct {
    uint8_t used, ver, engine, np;               /* UP_USED, UP_VER, engine, P_COUNT when stored */
    char name[12];                               /* ASCII 32..126, 0-padded (no 0 when 12 long) */
    int16_t p[UP_PMAX];
    union {
        struct { uint8_t note[16], flags[16]; };   /* record 1: note 0 = rest; flags 1 accent, 2 slide, 4 tie */
        int8_t px[32];                           /* record 2: p[P_COUNT_V8 ..], a byte each */
    };
} up_rec_t;
typedef struct {
    uint32_t magic;
    uint16_t rsize, nslot;
    up_rec_t r[UP_PER_BANK];
#if PHEBUS_UP_MX
    uint8_t mx[UP_PER_BANK][46];                 /* (wave 5) each record's mod matrix, LFO 2's SYNC and TRIG (project.c
                                                  * mx_pack's bytes; Spice and Dice there are the pattern's: not loaded). A
                                                  * bank stored before (without them) loads with none */
#endif
} up_bank_t;
#define UP_BANK_V2 (8u + UP_PER_BANK * sizeof(up_rec_t))   /* a bank as waves 1 to 4 stored it */
#ifdef ST_PAYLOAD_MAX
_Static_assert(sizeof(up_bank_t) <= ST_PAYLOAD_MAX, "a bank fits its flash object");
#endif
_Static_assert(sizeof(up_rec_t) == 192, "user preset record layout");
_Static_assert(P_COUNT_V8 <= UP_PMAX && P_COUNT - P_COUNT_V8 <= 32 && P_COUNT < 128, "user preset record: P_COUNT");
static void mx_pack(uint8_t *b, uint32_t ti);           /* (project.c, wave 5) */
static void mx_unpack(const uint8_t *b, uint32_t ti);
static up_bank_t up_bank[UP_SLOTS / UP_PER_BANK];

static up_rec_t *up_rec(uint32_t k) { return &up_bank[k / UP_PER_BANK].r[k % UP_PER_BANK]; }

static int up_valid(const up_rec_t *r)
{
    return r->used == UP_USED && r->ver >= 1u && r->ver <= UP_VER && r->engine < NENGINES && r->np >= 8u &&
           r->np <= UP_PMAX && r->name[0];
}

static int up_used(uint32_t k) { return k < UP_SLOTS && up_valid(up_rec(k)); }

static void up_bank_check(uint32_t b, int len)  /* after loading bank b (len bytes, -1 = none): wrong shape -> empty */
{
    up_bank_t *bk = &up_bank[b];
    if ((len != (int)sizeof *bk && len != (int)UP_BANK_V2) || bk->magic != UP_BANK_MAGIC || bk->rsize != sizeof(up_rec_t) ||
        bk->nslot != UP_PER_BANK)
        memset(bk, 0, sizeof *bk);
#if PHEBUS_UP_MX
    else if (len == (int)UP_BANK_V2)
        memset(bk->mx, 0, sizeof bk->mx);
#endif
}

/* the record's values in today's P_* order (mapped by count, see above); def = the defaults */
static void up_params(const up_rec_t *r, int16_t *out, const int16_t *def)
{
    uint32_t i, nc = r->np - 8u;
    for (i = 0; i < P_E0; i++)
        out[i] = i < nc ? r->p[i] : def[i];
    for (i = 0; i < 8u; i++)
        out[P_E0 + i] = r->p[nc + i];
    for (i = P_COUNT_V8; i < P_COUNT; i++)              /* (record 2: wave 4's; record 1: mapped by up_values; wave 5's
                                                         * from record 3) */
        out[i] = r->ver >= (i < P_COUNT_V9 ? 2u : 3u) ? r->px[i - P_COUNT_V8] : def[i];
}

static void up_name(uint32_t k, char *b)       /* upper case, 0-terminated: b holds 13 */
{
    const up_rec_t *r = up_rec(k);
    uint32_t i;
    for (i = 0; i < 12u && r->name[i]; i++)
        b[i] = r->name[i] >= 'a' && r->name[i] <= 'z' ? (char)(r->name[i] - 32) : r->name[i];
    b[i] = 0;
}

static void up_pat_norm(uint8_t *note, uint8_t *flags)   /* tie: no note; rest: no flags */
{
    *note &= 127u;
    if (*flags & 4u) {
        *note = 0;
        *flags = 4;
    } else {
        *flags = *note ? (uint8_t)(*flags & (SF_ACCENT | SF_SLIDE)) : 0u;
    }
}

static void up_pat_from(up_rec_t *r, const step_t *st)   /* the first 16 steps -> the pattern */
{
    uint32_t i;
    for (i = 0; i < 16u; i++) {
        r->note[i] = st[i].time == ST_NOTE && st[i].n ? st[i].note[0] : 0u;
        r->flags[i] = st[i].time == ST_TIE ? 4u : st[i].flags;
        up_pat_norm(&r->note[i], &r->flags[i]);
    }
}

static int up_pat_empty(const up_rec_t *r)
{
    uint32_t i;
    for (i = 0; i < 16u; i++)
        if (r->note[i])
            return 0;
    return 1;
}

#ifndef UP_HOST
static const param_desc_t *up_desc(uint32_t e, uint32_t i)
{
    return i >= P_E0 ? &ENGINES[e]->edit[i - P_E0] : &TP[i];
}

static void up_values(const up_rec_t *r, int16_t *v)   /* mapped and clamped for its engine */
{
    int16_t def[P_COUNT];
    uint32_t i;
    for (i = 0; i < P_COUNT; i++)
        def[i] = up_desc(r->engine, i)->def;
    up_params(r, v, def);
    if (r->ver < 2u)                                   /* (wave 4) a record 1: its filter onto the voice filter */
        voice_from_engine(v, r->engine, 1);
    for (i = 0; i < P_COUNT; i++)
        v[i] = (int16_t)clamp(v[i], up_desc(r->engine, i)->min, up_desc(r->engine, i)->max);
}

static void up_boot(void)                      /* persist_boot: the banks from flash */
{
#if FELUCCA_FLASH
    uint32_t b;
    for (b = 0; b < UP_SLOTS / UP_PER_BANK; b++)
        up_bank_check(b, flash_ok ? st_load(OBJ_UPRESET0 + b, &up_bank[b], sizeof up_bank[b]) : -1);
#endif
}

/* record k = *r (0: erase), then the bank to flash: 0 ok, 2 flash error, 3 no flash (kept in RAM) */
static int up_put(uint32_t k, const up_rec_t *r)
{
    up_bank_t *bk = &up_bank[k / UP_PER_BANK];
    bk->magic = UP_BANK_MAGIC;
    bk->rsize = sizeof(up_rec_t);
    bk->nslot = UP_PER_BANK;
    if (r)
        *up_rec(k) = *r;
    else
        memset(up_rec(k), 0, sizeof(up_rec_t));
    if (!r) {
        uint32_t i;
        for (i = 0; i < NTRK; i++)
            if (trk[i].user == k + 1u)
                trk[i].user = 0;
    }
    up_gen++;
#if FELUCCA_FLASH
    if (flash_ok)
        return st_save(OBJ_UPRESET0 + k / UP_PER_BANK, bk, sizeof *bk) ? 2 : 0;
#endif
    return 3;
}

static void up_slot_label(char *b, uint32_t k)  /* "U07" */
{
    b[0] = 'U';
    b[1] = (char)('0' + (k + 1u) / 10u);
    b[2] = (char)('0' + (k + 1u) % 10u);
    b[3] = 0;
}

/* the selected part's sound -> slot k; name 0 or "": engine name + slot number ("ANALOG 07").
 * 1 = the drum track is selected (it has no sound to store) */
static int up_store(uint32_t k, const char *name)
{
    up_rec_t r;
    uint32_t i;
    if (is_drum(TSEL))
        return 1;
    memset(&r, 0, sizeof r);
    r.used = UP_USED;
    r.ver = UP_VER;
    r.engine = TSEL->eng_req;
    r.np = P_COUNT_V8;
    if (name && name[0]) {
        for (i = 0; i < 12u && name[i]; i++)
            r.name[i] = name[i];
    } else {
        char b[16], l[4];
        str_cpy(b, ENGINES[TSEL->eng_req]->name, 9);
        up_slot_label(l, k);
        str_cpy(b + str_len(b), " ", 2);
        str_cpy(b + str_len(b), l + 1, 3);
        for (i = 0; i < 12u && b[i]; i++)
            r.name[i] = b[i];
    }
    for (i = 0; i < P_COUNT_V8; i++)
        r.p[i] = TSEL->p[i];
    for (i = P_COUNT_V8; i < P_COUNT; i++)              /* (record 2: wave 4's parameters, a byte each; 3: wave 5's) */
        r.px[i - P_COUNT_V8] = (int8_t)clamp(TSEL->p[i], -128, 127);
#if PHEBUS_UP_MX
    mx_pack(up_bank[k / UP_PER_BANK].mx[k % UP_PER_BANK], song.sel % NPART);   /* (wave 5) its matrix */
#endif
    return up_put(k, &r);
}

/* slot k -> the selected part's sound: engine and every parameter except its mix (LEVEL,
 * PAN, MUTE: the TRACKS faders) and its pattern parameters (param_kept). LIVE: the pattern
 * stored in the record is not loaded: changing the sound never changes the sequence.
 * 0 ok, 1 empty (or the drum track is selected) */
static int up_load_to(track_t *t, uint32_t k)       /* user preset k into track t (2.6: any track, MIDI PC) */
{
    const up_rec_t *r;
    int16_t v[P_COUNT];
    uint32_t i;
    if (!up_used(k) || is_drum(t))
        return 1;
    r = up_rec(k);
    if (!eng_allowed(t, r->engine))                     /* (a sound of an engine gone, or of another track) */
        return 1;
    up_values(r, v);
    for (i = 0; i < P_COUNT; i++)                       /* (LEN etc. of a kept pattern changed too, and */
        if (param_kept(i))                              /* a preset pattern then counted as edited) */
            v[i] = t->p[i];
    sound_change(t);                                    /* (2.6: what sounds fades out with the old sound, no click) */
    panic_req |= (uint8_t)(1u << trk_index(t));
    fm1_irq_off();                                      /* the audio ISR must not see half a sound */
    t->eng_req = r->engine;
    for (i = 0; i < P_COUNT; i++)
        t->p[i] = v[i];
    t->preset = 0;
    {                                                   /* (wave 5) its matrix and LFO 2 (Spice and Dice: the pattern's) */
        uint32_t ti = trk_index(t) % NPART;
        int16_t sp = mxt[ti].spice, dc = mxt[ti].dice;
        mx_clear_sound(ti);
#if PHEBUS_UP_MX
        if (r->ver >= 3u)
            mx_unpack(up_bank[k / UP_PER_BANK].mx[k % UP_PER_BANK], ti);
#endif
        mxt[ti].spice = sp, mxt[ti].dice = dc;
        if (r->ver < 3u) {                              /* stored before: its DIST, INSERT, sends, SHP routings */
            fx_from_legacy(t->p, 0);
            mx_from_legacy(ti, t->p, r->engine);
        }
    }
    fm1_irq_on();
    t->user = (uint8_t)(k + 1u);
    if (t == TSEL)
        sync_reload = 1;
    ui.force = 1;
    return 0;
}
static int up_load(uint32_t k) { return up_load_to(TSEL, k); }   /* into the selected track */

static uint32_t up_engine(uint32_t k) { return up_used(k) ? up_rec(k)->engine : NENGINES; }   /* its engine */
static uint32_t up_count(void)                 /* used slots */
{
    uint32_t k, n = 0;
    for (k = 0; k < UP_SLOTS; k++)
        n += (uint32_t)up_used(k);
    return n;
}

static uint32_t up_nth(uint32_t n)             /* slot of the n-th used one (n < up_count()) */
{
    uint32_t k;
    for (k = 0; k < UP_SLOTS; k++)
        if (up_used(k) && !n--)
            return k;
    return 0;
}

static uint32_t up_rank(uint32_t slot)         /* used slots before it */
{
    uint32_t k, n = 0;
    for (k = 0; k < slot && k < UP_SLOTS; k++)
        n += (uint32_t)up_used(k);
    return n;
}

/* SAVE > USER page actions, with the message in the top bar */
static void up_ui(uint32_t op, uint32_t k)     /* 0 load, 1 erase, 2 save */
{
    char l[4];
    int rc;
    up_slot_label(l, k);
    if (op != 1u && is_drum(TSEL)) {
        ui_message("DRUM TRACK: NO SOUND");
        return;
    }
    if (op < 2u && !up_used(k)) {
        ui_message("EMPTY SLOT");
        return;
    }
    if (op == 0u) {
        up_load(k);
        ui_say("LOADED ", l);
        return;
    }
    if (song.playing || transport_req) {               /* a flash erase stops the audio ~50 ms */
        ui_message("STOP BEFORE SAVE");
        return;
    }
    rc = op == 1u ? up_put(k, 0) : up_store(k, 0);
    if (rc == 3)
        ui_message(op == 1u ? "ERASED (RAM)" : "SAVED (RAM)");
    else if (rc)
        ui_message(op == 1u ? "ERASE ERROR" : "SAVE ERROR");
    else
        ui_say(op == 1u ? "ERASED " : "SAVED ", l);
    ui.force = 1;
}
#endif
