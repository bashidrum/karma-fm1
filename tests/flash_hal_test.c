/* SPDX-License-Identifier: GPL-3.0-only */
/* Host test of the RAM flash driver itself (firmware/hal/fm1_flash.h, unchanged, included as the firmware includes
 * it): fl_erase4k_ram, fl_prog_ram, fl_read_ram, fl_jedec_ram against a model of the SPI NOR chip.
 *
 * How: the driver's registers are fixed addresses (SPI0, TIMER4, PORTD, SFC, PWR_CON ...). They are mapped here as
 * real memory at those addresses; the pages with side effects are kept inaccessible, so every access faults, the
 * handler prepares the value (TIMER4 advances, SPI0 says "done"), lets that one instruction run (the trap flag) and
 * then applies the write: a byte sent to the chip, CS, XIP on or off, the core reset. The chip model: WREN, RDSR,
 * 0x20 erase, 0x02 page program, 0x0B read, 0x9F id, 0x66 / 0x99 reset; busy for a set time after an erase or a
 * page, or for ever; while busy it answers only RDSR and 66h / 99h.
 *
 * Built twice by tests/run_tests.sh: as the app (the new waits: 2 s / 50 ms, late counted, a restart if still busy)
 * and with -DFELUCCA_LOADER (the update loader's build: the same 2 s / 50 ms waits since 26 Nov 2026, still -4, no restart).
 *
 * Checks, the app: a fast chip (the P25Q80's 45 ms / 0.7 ms) and the slow chip (an erase of 1.2 s, a page of 30 ms)
 * both succeed, the data is right, the slow operations are counted late with the longest time; an erase or a page
 * never done ends in 66h, 99h, 20 ms, then the core reset, with XIP never switched back on while the chip was busy;
 * TIMER4 wrapping during the wait. The loader build: the slow chip's erase and page now succeed (2 s / 50 ms); past
 * those it still gets -4 and never restarts (DECISIONS.md, "The flash driver and the slower chip").
 *
 * Linux x86-64 only (signals, the trap flag, low fixed mappings); elsewhere it says so and passes. */
#define _GNU_SOURCE
#include <stdio.h>
#if !(defined(__linux__) && defined(__x86_64__))
int main(void)
{
    puts("flash_hal_test: skipped (needs Linux x86-64)");
    return 0;
}
#else
#include <setjmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <ucontext.h>
#include <unistd.h>

/* the AC791N's cache instructions: nothing to do on the host */
__asm__(".macro csync\n.endm\n.macro flushinv a\n.endm\n");
#define KARMA_DUAL_CORE 1
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"   /* (the hal's 32-bit addresses on a 64-bit host) */
#include "../firmware/hal/fm1_flash.h"

#define TICKS_MS 24000ull
/* ---- the chip ---- */
static struct {
    uint8_t mem[0x100000];
    uint64_t busy_until;                  /* sim ticks */
    int wel, reset_en, cs_low, cmd, n;
    uint32_t addr, id;
    uint8_t page[256];
    uint32_t page_n;
    uint64_t erase_ticks, prog_ticks;
    int erases, progs, resets, ignored;
} nor;
static uint64_t sim_now, step = 24u * 1000u;  /* TIMER4 advances 1 ms each time it is read */
static int xip_while_busy, xip_on;
static uint32_t sfc_con = 0x00000200u;        /* SFC_CON as last written (its page is trapped) */
static struct { int seen; uint32_t sfc, iomap, c1; uint64_t at; int busy_before_reset, resets; } core_reset;
static sigjmp_buf reset_jmp;

static int nor_busy(void) { return nor.busy_until > sim_now; }

static uint8_t nor_byte(uint8_t b)
{
    uint8_t r = 0xFF;
    if (nor.n == 0) {
        nor.cmd = b;
        if (nor_busy() && b != 0x05 && b != 0x35 && b != 0x66 && b != 0x99)
            nor.cmd = -1, nor.ignored++;
    } else switch (nor.cmd) {
    case 0x05: r = (uint8_t)((nor_busy() ? 1 : 0) | (nor.wel ? 2 : 0)); break;
    case 0x35: r = 0; break;
    case 0x9F: r = (uint8_t)(nor.id >> (8 * (3 - nor.n))); break;
    case 0x20: case 0x02: case 0x0B:
        if (nor.n <= 3) nor.addr = (nor.addr << 8) | b;
        else if (nor.cmd == 0x02 && nor.page_n < 256) nor.page[nor.page_n++] = b;
        else if (nor.cmd == 0x0B && nor.n >= 5) r = nor.mem[nor.addr++ & 0xFFFFFu];
        break;
    default: break;
    }
    nor.n++;
    return r;
}

static void nor_cs(int low)
{
    if (low && !nor.cs_low) {
        nor.n = 0; nor.addr = 0; nor.page_n = 0; nor.cmd = -1;
    } else if (!low && nor.cs_low && nor.n) {
        int was_en = nor.reset_en;
        nor.reset_en = 0;
        switch (nor.cmd) {
        case 0x06: nor.wel = 1; break;
        case 0x20:
            if (nor.wel && nor.n == 4) {
                memset(nor.mem + (nor.addr & 0xFF000u), 0xFF, 0x1000);
                nor.busy_until = sim_now + nor.erase_ticks;
                nor.erases++;
            }
            nor.wel = 0;
            break;
        case 0x02:
            if (nor.wel && nor.n >= 4) {
                uint32_t i;
                for (i = 0; i < nor.page_n; i++)       /* wraps inside the page, as a NOR does */
                    nor.mem[(nor.addr & 0xFFF00u) | ((nor.addr + i) & 0xFFu)] &= nor.page[i];
                nor.busy_until = sim_now + nor.prog_ticks;
                nor.progs++;
            }
            nor.wel = 0;
            break;
        case 0x66: nor.reset_en = 1; break;
        case 0x99:
            if (was_en) { nor.busy_until = 0; nor.wel = 0; nor.resets++; }
            break;
        default: break;
        }
    }
    nor.cs_low = low;
}

/* ---- the registers ---- */
#define PG_SYS   0x10000u                  /* PWR_CON 0x10000, TIMER4 0x10804 */
#define PG_SPI   0x11000u                  /* SPI0 0x11C00 */
#define PG_SFC   0x40000u                  /* SFC_CON 0x40200 */
#define PG_PORT  0x50000u                  /* PORTD 0x500C0 */
static const uint32_t TRAPPED[] = {PG_SYS, PG_SPI, PG_SFC, PG_PORT};
static struct { uint32_t addr, old; int write; } pend;
#define W32(a) (*(volatile uint32_t *)(uintptr_t)(a))

static void prot(uint32_t pg, int on) { mprotect((void *)(uintptr_t)pg, 0x1000, on ? PROT_NONE : PROT_READ | PROT_WRITE); }

static void on_segv(int sig, siginfo_t *si, void *ctx)
{
    ucontext_t *uc = ctx;
    uint32_t a = (uint32_t)(uintptr_t)si->si_addr & ~3u, pg = a & ~0xFFFu;
    size_t i;
    (void)sig;
    for (i = 0; i < sizeof TRAPPED / sizeof TRAPPED[0]; i++)
        if (TRAPPED[i] == pg)
            break;
    if (i == sizeof TRAPPED / sizeof TRAPPED[0] || (uintptr_t)si->si_addr >= 0x100000000ull) {
        fprintf(stderr, "flash_hal_test: stray access at %p\n", si->si_addr);
        _exit(2);
    }
    prot(pg, 0);
    if (a == 0x10804u) {                   /* TIMER4: time moves on */
        sim_now += step;
        W32(a) = (uint32_t)sim_now;
    } else if (a == 0x11C00u) {
        W32(a) |= 0x8000u;                 /* SPI0 PND: the byte is done */
    }
    pend.addr = a;
    pend.old = W32(a);
    pend.write = (uc->uc_mcontext.gregs[REG_ERR] & 2) != 0;
    uc->uc_mcontext.gregs[REG_EFL] |= 0x100;      /* run that one instruction, then trap */
}

static void on_trap(int sig, siginfo_t *si, void *ctx)
{
    ucontext_t *uc = ctx;
    uint32_t a = pend.addr, v = W32(a);
    (void)sig; (void)si;
    uc->uc_mcontext.gregs[REG_EFL] &= ~0x100;
    if (pend.write || v != pend.old) {
        if (a == 0x11C08u) {               /* SPI0_BUF: a byte out, the answer in */
            W32(a) = nor_byte((uint8_t)v);
        } else if (a == 0x500C0u) {        /* PORTD_OUT: PD0 is CS# */
            nor_cs(!(v & 1u));
        } else if (a == 0x40200u) {        /* SFC_CON: XIP on (nonzero) or off */
            sfc_con = v;
            xip_on = v != 0;
            if (xip_on && nor_busy())
                xip_while_busy++;
        } else if (a == 0x10000u && (v & 0x10u)) {   /* PWR_CON bit 4: the core reset */
            core_reset.seen++;
            core_reset.sfc = sfc_con;
            core_reset.iomap = W32(0x5101Cu);
            core_reset.c1 = W32(0x1EEE004u);
            core_reset.at = sim_now;
            core_reset.busy_before_reset = nor_busy();
            core_reset.resets = nor.resets;
            W32(a) = 0;
            prot(a & ~0xFFFu, 1);
            siglongjmp(reset_jmp, 1);
        }
    }
    prot(a & ~0xFFFu, 1);
}

static void map_fixed(uint32_t at, uint32_t len)
{
    void *p = mmap((void *)(uintptr_t)at, len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE,
                   -1, 0);
    if (p != (void *)(uintptr_t)at) {
        printf("flash_hal_test: skipped (cannot map the registers at %#x)\n", at);
        exit(0);
    }
}

static void hw_init(void)
{
    struct sigaction sa;
    size_t i;
    map_fixed(0x10000u, 0x42000u);         /* 0x10000 .. 0x52000: SYS, SPI0, SFC, PORTD, IOMAP */
    map_fixed(0x1EEE000u, 0x1000u);        /* CACHE_CON, CPU1's C1_CON */
    W32(0x1EEE008u) = 0x4000u;             /* cache idle */
    W32(0x40200u) = 0x00000200u;           /* XIP on, read mode 2 (not continuous) */
    W32(0x5101Cu) = 0x20u;                 /* the pins with the SFC */
    W32(0x500C0u) = 1u;                    /* CS high */
    W32(0x1EEE004u) = 8u;                  /* CPU1 released (running) */
    xip_on = 1;
    memset(&sa, 0, sizeof sa);
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = on_segv;
    sigaction(SIGSEGV, &sa, 0);
    sa.sa_sigaction = on_trap;
    sigaction(SIGTRAP, &sa, 0);
    for (i = 0; i < sizeof TRAPPED / sizeof TRAPPED[0]; i++)
        prot(TRAPPED[i], 1);
}

static int fails;
static void check(const char *what, int ok)
{
    printf("%-78s %s\n", what, ok ? "ok" : "FAIL");
    if (!ok)
        fails++;
}

static void chip(double erase_ms, double prog_ms)
{
    nor.erase_ticks = (uint64_t)(erase_ms * TICKS_MS);
    nor.prog_ticks = (uint64_t)(prog_ms * TICKS_MS);
    nor.busy_until = 0;
}

/* one erase of the sector at off and the 16 pages after it; 0 or the first error */
static int write_sector(uint32_t off, const uint8_t *src, uint32_t *took_erase, uint32_t *took_prog_max)
{
    uint32_t k, t;
    int rc = fl_erase4k_ram(off, took_erase);
    if (rc)
        return rc;
    *took_prog_max = 0;
    for (k = 0; k < 0x1000u; k += 256u) {
        if ((rc = fl_prog_ram(off + k, src + k, 256u, &t)) != 0)
            return rc;
        if (t > *took_prog_max)
            *took_prog_max = t;
    }
    return 0;
}

int main(void)
{
    static uint8_t src[0x1000], back[0x1000];
    uint32_t i, te = 0, tp = 0, id;
    int rc;
    setvbuf(stdout, 0, _IONBF, 0);
    hw_init();
    for (i = 0; i < sizeof src; i++)
        src[i] = (uint8_t)(i * 7u + 3u);
    memset(nor.mem, 0x5A, sizeof nor.mem);

    nor.id = 0xB36014u;
    id = fl_jedec_ram();
    check("the JEDEC id read through the driver (UC25HQ80)", id == 0xB36014u);

    /* the P25Q80 as measured on generic NOR: an erase 45 ms, a page 0.7 ms */
    chip(45, 0.7);
    rc = write_sector(0x97000u, src, &te, &tp);
    fl_read_ram(0x97000u, back, sizeof back);
    check("a fast chip (45 ms erase, 0.7 ms page): the sector written and read back", rc == 0 && !memcmp(src, back, sizeof src));
    check("  XIP back on afterwards, never while the chip was busy", xip_on && !xip_while_busy);
#ifndef FELUCCA_LOADER
    check("  nothing counted late", fl_late_count() == 0 && fl_late_max_ms() == 0);
    check("  the chip's name for the console", !strcmp(fl_chip_name(), "UC25HQ80"));

    /* the slow chip: an erase 1.2 s, a page 30 ms (past the old 500 ms / 10 ms) */
    chip(1200, 30);
    for (i = 0; i < sizeof src; i++)
        src[i] = (uint8_t)(i * 13u + 1u);
    rc = write_sector(0x98000u, src, &te, &tp);
    fl_read_ram(0x98000u, back, sizeof back);
    check("the slow chip (1.2 s erase, 30 ms page): the save succeeds (the old limits said -4)",
          rc == 0 && !memcmp(src, back, sizeof src));
    check("  the erase took about 1.2 s, the pages about 30 ms",
          te >= 1199000u && te <= 1202000u && tp >= 29000u && tp <= 32000u);
    check("  XIP never on while the chip was busy", !xip_while_busy && nor.ignored == 0);
    check("  late: 1 erase + 16 pages, the longest the erase (1200 ms)",
          fl_late_count() == 17u && fl_late_max_ms() >= 1198u && fl_late_max_ms() <= 1202u);
    check("  no restart", core_reset.seen == 0 && nor.resets == 0);

    /* just inside the new limits: an erase of 1.99 s, a page of 49 ms */
    chip(1990, 49);
    rc = write_sector(0x99000u, src, &te, &tp);
    check("an erase of 1.99 s and pages of 49 ms: still a success", rc == 0 && core_reset.seen == 0);

    /* TIMER4 wraps during a slow erase */
    sim_now = 0xFFFFFFFFull - 24u * 300000u;
    chip(1200, 0.7);
    rc = fl_erase4k_ram(0x9A000u, &te);
    check("TIMER4 wrapping during a 1.2 s erase: done, timed right", rc == 0 && te >= 1199000u && te <= 1202000u);

    /* an erase that never finishes: 66h, 99h, 20 ms, the core reset; never back to XIP */
    chip(0, 0);
    nor.erase_ticks = 1ull << 50;          /* (busy for ever, from the command on) */
    {
        uint64_t t0 = sim_now;
        int resets0 = nor.resets;
        if (sigsetjmp(reset_jmp, 1) == 0) {
            rc = fl_erase4k_ram(0x9B000u, &te);
            check("an erase never done: the driver must not return", 0);
        } else {
            uint64_t ms = (core_reset.at - t0) / TICKS_MS;
            check("an erase never done: the unit restarts (PWR_CON core reset)", core_reset.seen == 1);
            check("  after 2 s of waiting and the 20 ms reset recovery", ms >= 2020u && ms <= 2025u);
            check("  the chip reset first (66h then 99h) and idle at the restart",
                  core_reset.resets == resets0 + 1 && !core_reset.busy_before_reset);
            check("  XIP still off at the restart, never on while busy", core_reset.sfc == 0 && !xip_while_busy);
            check("  the pins handed back to the SFC, CPU1 held", (core_reset.iomap & 0x20u) && (core_reset.c1 & 2u) && !(core_reset.c1 & 8u));
        }
    }
    /* after the "restart": XIP on again for the next case, as the boot would leave it */
    prot(PG_SFC, 0);
    W32(0x40200u) = sfc_con = 0x00000200u;
    prot(PG_SFC, 1);
    xip_on = 1;

    /* a page that never finishes: the same, after 50 ms */
    chip(45, 0);
    nor.prog_ticks = 1ull << 50;
    {
        uint64_t t0;
        int resets0 = nor.resets;
        rc = fl_erase4k_ram(0x9C000u, &te);
        t0 = sim_now;
        if (sigsetjmp(reset_jmp, 1) == 0) {
            rc = fl_prog_ram(0x9C000u, src, 256u, &tp);
            check("a page never done: the driver must not return", 0);
        } else {
            uint64_t ms = (core_reset.at - t0) / TICKS_MS;
            check("a page never done: the unit restarts after 50 ms + 20 ms, the chip reset, XIP off",
                  core_reset.seen == 2 && ms >= 70u && ms <= 75u && core_reset.resets == resets0 + 1 &&
                  core_reset.sfc == 0 && !xip_while_busy);
        }
    }
#else
    /* the update loader's build (Ben, 26 Nov 2026: its waits only): 2 s / 50 ms, still -4 and never a restart */
    chip(1200, 30);
    rc = fl_erase4k_ram(0x98000u, &te);
    check("loader build, the slow chip's 1.2 s erase now succeeds", rc == 0 && te >= 1199000u && te <= 1202000u);
    check("  and XIP is not back on while the chip is still erasing", xip_while_busy == 0);
    rc = fl_prog_ram(0x98000u, src, 256u, &tp);
    check("loader build, a 30 ms page now succeeds", rc == 0 && tp >= 29000u && tp <= 31000u);
    chip(2500, 30);                        /* a chip slower than even the new limit */
    rc = fl_erase4k_ram(0x99000u, &te);
    check("loader build, an erase past 2 s: -4 at 2 s (the install can be pressed again)", rc == -4 && te > 2000000u && te <= 2002000u);
    sim_now += 2000u * TICKS_MS;           /* let it finish */
    chip(45, 80);
    rc = fl_erase4k_ram(0x9A000u, &te);
    rc = rc ? rc : fl_prog_ram(0x9A000u, src, 256u, &tp);
    check("loader build, an 80 ms page: -4 at 50 ms", rc == -4 && tp > 50000u && tp <= 52000u);
    check("  never a restart in the loader's build", core_reset.seen == 0);
#endif
    printf("flash_hal_test%s: %s\n",
#ifdef FELUCCA_LOADER
           " (loader build)",
#else
           "",
#endif
           fails ? "FAILED" : "passed");
    return fails ? 1 : 0;
}
#endif
