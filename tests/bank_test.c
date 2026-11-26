/* SPDX-License-Identifier: GPL-3.0-only */
/* The bank (Ben, 23 Nov 2026, 22:45: "Just enough so that I can keep one in the bank. So one live one and one dead one. Or
 * even just the option of turning the device off and on again without having lost what I was working on"): project.c's
 * real flash code (bank_op, autosave_tick, sections_write, autosave_resume, persist_boot's reading) on a simulated NOR, under
 * the real UI (tests/ui_pages_test.c's harness), as storage_test and ukit_test do for theirs.
 * Exit status: the number of failed checks. */
#define UI_PAGES_HARNESS_ONLY 1
#define REAL_PROJECT 1
#define UKIT_FLASH_TEST 1
#include "ui_pages_test.c"
#include <sys/wait.h>
#include <sys/mman.h>
static uint8_t nor[0x100000];
static int32_t prog_left = -1;                     /* erases + programs allowed before the power goes (-1: no limit) */
static uint32_t nor_ops;                           /* erases and programs done */
static uint8_t flash_ok = 1;
static uint16_t out_quiet = 0xFFFFu;               /* (audio.c: the output's silent samples) */
static int st_read(uint32_t off, void *dst, uint32_t n) { memcpy(dst, nor + off, n); return 0; }
static int nor_cut(void) { if (prog_left == 0) return 1; if (prog_left > 0) prog_left--; nor_ops++; return 0; }
static int st_erase(uint32_t off) { if (nor_cut()) return -5; memset(nor + off, 0xFF, 4096); return 0; }
static int st_prog(uint32_t off, const void *src, uint32_t n)
{
    const uint8_t *s = src;
    uint32_t i;
    if (nor_cut())
        return -5;
    for (i = 0; i < n; i++)
        nor[off + i] &= s[i];
    return 0;
}
#include "../firmware/src/storage.c"
static void up_boot(void) {}
#define FL_JEDEC_OK(x) 1
#define FL_FAR(f) f
static uint32_t fl_jedec_ram(void) { return 0; }
static void fl_plain_window_init(void) {}
static uint32_t irq_save(void) { return 0; }
static void irq_restore(uint32_t f) { (void)f; }
#define FELUCCA_FLASH 1
#include "../firmware/src/project.c"

static int fails;
static void ok(int c, const char *w) { printf("bank: %-100s %s\n", w, c ? "ok" : "FAIL"); fails += !c; }
/* the FM-1 switched off and on: RAM as at power-on (the .noinit patterns A, B garbage: cleared), then what main.c does */
static void power_cycle(void)
{
    memset(proj_sec_a, 0, sizeof proj_sec_a);
    memset(&proj_sec_c, 0, sizeof proj_sec_c);
    memset(&proj_sec_d, 0, sizeof proj_sec_d);
    st_flip = 0, st_gen[0] = st_gen[1] = 0xFFFFFFFFu;
    autosave_hash = 0, sec_dirty = song_dirty = 0, stop_save = was_running = 0;
    memset(&bank_s, 0, sizeof bank_s);
    uk_cur = -2;
    live_sec = -1, live_req = -1;
    song.playing = 0, transport_req = 0;
    host_tracks_init();
    kit_set(DRUM_DEFAULT_KIT);
    song.g[G_BPM] = 90;
    persist_boot();
    autosave_resume();
}
static void settle(void) { uint32_t i; for (i = 0; i < 8u; i++) frame(); transport_req = 0; song.playing = 0; }
/* a project of its own: tempo, a synth line, drum hits, a sound changed in the kit, patterns stored (mask) */
static void make_project(uint32_t seed, uint32_t pats)
{
    uint32_t i;
    song.g[G_BPM] = (int16_t)(80 + seed * 7u);
    for (i = 0; i < 16u; i += 2u + seed % 3u) {
        trk[0].step[i].n = 1, trk[0].step[i].time = ST_NOTE, trk[0].step[i].note[0] = (uint8_t)(40 + (i + seed) % 12u);
        dt_set(i, (seed + i) % 4u, 100);
    }
    slot_from_kit(&pkit.s[2 + seed % 3u], seed & 1u ? DRUM_SAMPLED : 2u, 2);   /* (a sound from another kit: CUSTOM) */
    pkit.s[2 + seed % 3u].mach |= SL_CHG;
    pkit_gen++;
    for (i = 0; i < 4u; i++)
        if ((pats >> i) & 1u) {
            trk[1].step[i].n = 1, trk[1].step[i].time = ST_NOTE, trk[1].step[i].note[0] = (uint8_t)(50 + i + seed);
            section_store(i);
        }
    settle();
}
static project_t live_now(void) { project_t p; proj_capture(&p); return p; }
static void diffp(const project_t *a, const project_t *b)
{
    const uint8_t *x = (const uint8_t *)a, *y = (const uint8_t *)b;
    uint32_t i, n = 0;
    for (i = 0; i < sizeof *a; i++)
        if (x[i] != y[i] && n++ < 6)
            printf("  (byte %u: %u, was %u; core %u, g at %u, t at %u, d at %u)\n", i, x[i], y[i], (unsigned)sizeof(proj_core_t),
                   (unsigned)__builtin_offsetof(proj_core_t, g), (unsigned)__builtin_offsetof(proj_core_t, t), (unsigned)__builtin_offsetof(proj_core_t, d));
    if (n) printf("  (%u bytes differ)\n", n);
}
/* the five objects of the live (b 0) or the bank (b 1) project as flash has them */
typedef struct { int n[5]; uint8_t b[5][7936]; } five_t;
static five_t *fa, *fb, *fz;
static void five(int b, five_t *f)
{
    uint32_t i;
    for (i = 0; i < 5u; i++)
        f->n[i] = st_load((b ? OBJ_BPRJ0 : OBJ_PRJ0) + i, f->b[i], sizeof f->b[i]);
}
static int five_eq(const five_t *a, const five_t *b)
{
    uint32_t i;
    for (i = 0; i < 5u; i++)
        if ((a->n[i] > 0 ? a->n[i] : 0) != (b->n[i] > 0 ? b->n[i] : 0) ||   /* (no object and an empty one: both empty) */
            (a->n[i] > 0 && memcmp(a->b[i], b->b[i], (uint32_t)a->n[i])))
            return 0;
    return 1;
}
static uint32_t render_hash(void)                  /* 2 s of the live project from the start, a hash of every sample */
{
    int32_t o[CTL * 2u];
    uint32_t b, i, h = 2166136261u;
    transport_req = 1;
    for (b = 0; b < FS * 2u / CTL; b++) {
        mix_block(o, CTL);
        for (i = 0; i < CTL * 2u; i++)
            h = (h ^ (uint32_t)o[i]) * 16777619u;
    }
    transport_req = 2;
    settle();
    return h;
}
static uint32_t render_in_child(void)              /* (a fork: the render leaves this process as it was) */
{
    static uint32_t *sh;
    pid_t p;
    int st;
    if (!sh)
        sh = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    p = fork();
    if (!p) { sh[0] = render_hash(); _exit(0); }
    waitpid(p, &st, 0);
    return sh[0];
}
/* in a child: the operation cut after k flash writes, then the power cycle; 1: the live and the bank each whole, as before
 * the operation or as after it (SWAP: exchanged), never a mix and never lost */
static int cut_case(uint32_t op, int32_t k, const five_t *live0, const five_t *bank0, const five_t *live1, const five_t *bank1)
{
    pid_t p = fork();
    int st;
    if (!p) {
        int r;
        prog_left = k;
        (void)bank_op(op);
        prog_left = -1;
        power_cycle();
        five(0, fz);
        r = five_eq(fz, live0) || five_eq(fz, live1);
        five(1, fz);
        r &= five_eq(fz, bank0) || five_eq(fz, bank1);
        if (op == 1u) {                                /* (SWAP: both or neither exchanged) */
            five_t *l = fa;
            five(0, l);
            r &= (five_eq(l, live0) && five_eq(fz, bank0)) || (five_eq(l, live1) && five_eq(fz, bank1));
        }
        _exit(r ? 0 : 1);
    }
    waitpid(p, &st, 0);
    return WIFEXITED(st) && WEXITSTATUS(st) == 0;
}
/* the sound of a project brought back by SWAP against the same project loaded straight from its bytes, both from this same
 * moment (two children: the audio's history the same) */
static uint32_t render_via(int swap, const project_t *ref)
{
    static uint32_t *sh;
    pid_t p;
    int st;
    if (!sh)
        sh = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    p = fork();
    if (!p) {
        if (swap) {
            if (bank_op(1)) _exit(1);
        } else {
            proj_core_t *q = (proj_core_t *)(void *)proj_tmp.raw;
            memcpy(proj_tmp.raw, ref, sizeof *ref);
            proj_kit_take(proj_tmp.raw, (int)sizeof *ref, q);
            project_apply(q);
        }
        settle();
        sh[0] = render_hash();
        _exit(0);
    }
    waitpid(p, &st, 0);
    return sh[0];
}
static uint32_t ops_of(uint32_t op)                /* how many flash writes an operation takes (in a child) */
{
    static uint32_t *sh;
    pid_t p;
    int st;
    if (!sh)
        sh = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    p = fork();
    if (!p) { nor_ops = 0; sh[0] = bank_op(op) == 0 ? nor_ops : 0u; _exit(0); }
    waitpid(p, &st, 0);
    return sh[0];
}

int main(int argc, char **argv)
{
    project_t L, B, x;
    uint32_t hl, hb, n, k, okc;
    five_t *l0, *b0, *l1, *b1;
    outdir = argc > 1 ? argv[1] : "build/host";
    fa = malloc(sizeof *fa), fb = malloc(sizeof *fb), fz = malloc(sizeof *fz);
    l0 = malloc(sizeof *l0), b0 = malloc(sizeof *b0), l1 = malloc(sizeof *l1), b1 = malloc(sizeof *b1);
    memset(nor, 0xFF, sizeof nor);
    bank_resolve();
    panel = PANEL_DEFAULT;
    layers_init();
    settings.palette = 4;
    palette_set(4);
    power_cycle();
    go_home(); frames(3);
    ok(!bank_summary(&n, (int32_t *)&k, (char[9]){0}) && st_flip == 0u, "a fresh FM-1: the bank is empty, the live project in the first area");

    /* ---- what survives power off and on (Part 3a) */
    make_project(1, 0x5u);                             /* patterns A and C stored, stopped: in RAM, not yet in flash */
    ok(sec_dirty == 0x5u, "patterns stored: in RAM first (written once stopped, quiet and untouched 1.5 s)");
    persist_flush_now();
    L = live_now();
    hl = render_in_child();
    power_cycle();
    x = live_now();
    diffp(&x, &L);
    ok(!memcmp(&x, &L, sizeof x), "once written: power off and on brings the working project back bit for bit");
    ok(project_used(0) && !project_used(1) && project_used(2) && !project_used(3), "... and patterns A and C");
    song.playing = 1;                                  /* stored while playing, then the power goes before a stop */
    trk[1].step[9].n = 1, trk[1].step[9].time = ST_NOTE, trk[1].step[9].note[0] = 61;
    section_store(3);
    song.g[G_BPM] = 133;
    {
        uint32_t t;
        for (t = 0; t < 200u; t++) { fm1_ms += 50u; autosave_tick(); sections_flush(); }   /* 10 s of play: no write */
    }
    ok(sec_dirty & 0x8u, "playing: nothing is written to flash (a write stalls the audio): pattern D waits in RAM");
    song.playing = 0;
    power_cycle();
    ok(!project_used(3) && song.g[G_BPM] == L.c.g[G_BPM], "power cut while playing: pattern D and the tempo change are lost, the project as last saved comes back");
    song.g[G_BPM] = 133;                               /* the same, then STOP and wait */
    section_store(3);
    song.playing = 1; autosave_tick(); song.playing = 0;
    {
        uint32_t t, at = 0;
        for (t = 0; t < 60u && !at; t++) {             /* 50 ms a tick, the panel untouched */
            fm1_ms += 50u; ui_input_ms = fm1_ms - 2000u * (t > 30u);
            autosave_tick(); sections_flush();
            if (autosave_hash == work_sum((proj_capture_core(&autosave_buf), autosave_buf.sum)) && !sec_dirty) at = t;
        }
        printf("  (saved %u ms after the stop)\n", at * 50u);
        ok(at && at * 50u <= 2000u, "stopped: the working project and pattern D are written within 2 s (0.3 s for the project once silent)");
    }
    L = live_now();
    power_cycle();
    x = live_now();
    ok(!memcmp(&x, &L, sizeof x) && project_used(3), "... and after power off and on they are back");
    {   /* an edit while stopped, soon after a save: written 2.5 s after the last touch, 5 s after the save at most (was 20 s) */
        uint32_t t, at = 0, h0;
        autosave_ms = fm1_ms;
        song.g[G_BPM]++;
        trk[2].step[3].n = 1, trk[2].step[3].time = ST_NOTE, trk[2].step[3].note[0] = 70;
        h0 = autosave_hash;
        ui_input_ms = fm1_ms;
        for (t = 1; t <= 200u && !at; t++) { fm1_ms += 50u; autosave_tick(); if (autosave_hash != h0) at = t; }
        printf("  (an edit while stopped: saved after %u ms)\n", at * 50u);
        ok(at && at * 50u <= 5100u, "an edit while stopped is written within about 5 s (2.5 s after the last touch)");
    }

    /* ---- the bank: KEEP, NEW, SWAP */
    persist_flush_now();
    L = live_now();
    hl = render_in_child();
    five(0, l0);
    ok(bank_op(0) == 0 && bank_summary(&n, (int32_t *)&k, (char[9]){0}) && n == 3u && k == (uint32_t)L.c.g[G_BPM],
       "KEEP into the empty bank: done; the bank's summary: 3 patterns, the live tempo");
    five(1, fb);
    ok(five_eq(fb, l0), "KEEP: the bank holds the live project's five objects byte for byte");
    song.playing = 1;
    ok(bank_op(0) == 2 && bank_op(1) == 2 && bank_op(2) == 2, "refused while playing (all three), nothing written");
    song.playing = 0;
    ok(bank_op(2) == 0 && !project_used(0) && !project_used(2) && !project_used(3), "NEW: a fresh live project, patterns A to D empty");
    five(1, fz);
    ok(five_eq(fz, l0), "NEW: the bank untouched");
    make_project(2, 0x2u);
    persist_flush_now();
    B = live_now();
    hb = render_in_child();
    ok(hb != hl, "(a second, different project in the live slot)");
    {
        uint32_t ra = render_via(1, 0), rb = render_via(0, &L), rc = render_via(0, &B);
        ok(ra == rb && ra != rc, "SWAP: the kept project sounds as itself, sample for sample (against it loaded from its bytes)");
    }
    ok(bank_op(1) == 0, "SWAP: done");
    settle();
    x = live_now();
    diffp(&x, &L);
    ok(!memcmp(&x, &L, sizeof x) && project_used(0) && project_used(2) && project_used(3) && !project_used(1),
       "KEEP, NEW, SWAP: the kept project is live again, bit for bit, patterns A C D");
    ok(bank_summary(&n, (int32_t *)&k, (char[9]){0}) && n == 1u && k == (uint32_t)B.c.g[G_BPM], "... the other one is in the bank (1 pattern, its tempo)");
    power_cycle();
    x = live_now();
    ok(!memcmp(&x, &L, sizeof x) && st_flip == 1u && bank_summary(&n, (int32_t *)&k, (char[9]){0}) && n == 1u,
       "power off and on after a SWAP: the same live project, the bank still holds the other");
    ok(bank_op(1) == 0 && (settle(), x = live_now(), !memcmp(&x, &B, sizeof x)), "SWAP again: the other project live, bit for bit");
    ok(bank_op(1) == 0 && (settle(), x = live_now(), !memcmp(&x, &L, sizeof x)), "SWAP twice is where you started");

    /* ---- power cuts at every write */
    persist_flush_now();
    five(0, l0); five(1, b0);
    n = ops_of(1);
    okc = n > 0u;
    for (k = 0; k < n; k++)
        okc &= cut_case(1, (int32_t)k, l0, b0, b0, l0);
    printf("  (SWAP: %u flash writes)\n", n);
    ok(okc, "SWAP cut after each of its writes, then power on: both projects whole, swapped or not, never mixed or lost");
    /* KEEP over a kept one: the bank becomes a copy of the live; cut anywhere, the old bank or the new, whole */
    make_project(3, 0x9u);                             /* (the live one changed: KEEP has something to replace) */
    persist_flush_now();
    five(0, l0); five(1, b0);
    n = ops_of(0);
    okc = n > 0u;
    for (k = 0; k < n; k++)
        okc &= cut_case(0, (int32_t)k, l0, b0, l0, l0);
    printf("  (KEEP: %u flash writes)\n", n);
    ok(okc, "KEEP cut after each of its writes, then power on: the live project whole, the bank the old one or the new one, whole");
    {   /* a cut KEEP, then a KEEP to the end: the new bank (the half-written copies of the first never read) */
        pid_t p = fork();
        int st;
        if (!p) {
            prog_left = (int32_t)(n / 2u);
            bank_op(0);
            prog_left = -1;
            power_cycle();
            five(1, fz);
            if (!five_eq(fz, b0)) _exit(1);
            if (bank_op(0)) _exit(2);
            power_cycle();
            five(1, fz);
            _exit(five_eq(fz, l0) ? 0 : 3);
        }
        waitpid(p, &st, 0);
        ok(WIFEXITED(st) && !WEXITSTATUS(st), "a KEEP cut half way leaves the old bank; KEEP again: the new one");
    }
    {   /* the objects before the bank's area are untouched: user kits, settings */
        uint32_t i, untouched = 1;
        for (i = 0xCC000u; i < 0xDC000u; i++) untouched &= nor[i] == 0xFFu;
        ok(untouched, "the free room after the bank (0xCC000..0xDBFFF) untouched");
    }
    printf("bank: %s (%d failed)\n", fails ? "FAILED" : "all ok", fails);
    return fails;
}
