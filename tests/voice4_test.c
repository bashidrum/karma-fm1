/* SPDX-License-Identifier: GPL-3.0-only */
/* PHEBUS wave 4, the MiniFreak voices: the sound side (SPEC.md, "New direction for PHEBUS" and the 22 Nov entries).
 *   types     every oscillator Type (vtype.c) selectable and sounding on every synth track (1, 2, 3): a held note, its
 *             level above silence, bounded, finite
 *   filter    for each engine family (ACID, JUNO, PLAITS, ELEMENTS): CUTOFF down darkens the sound (its spectral
 *             centroid falls), RESO up makes a peak at the cutoff, the ENV amount opens the filter on the attack and it
 *             closes as the envelope decays (the centroid moves over time), the LFO to the filter moves it
 *             (the centroid swings), TYPE HP thins the lows; a self-oscillating RESO rings bounded
 *   acid bug  the verified bug: ACID ignored ENV > FLT and LFO > FLT (eng_acid.c never read m->cutoff): both move it now
 *   link/own  the filter envelope LINK (follows the amp ENV) vs OWN (its own ADSR): with a slow amp attack and a fast own
 *             decay the two give different filter movement; OWN follows its own decay, LINK the amp's
 *   oct/tune  OCT moves the pitch an octave, TUNE by cents
 * Seeded and deterministic. Exit status: the number of failed checks. */
#include <stdint.h>
#define main hostsim_main
#include "hostsim.c"
#undef main
static int fails;
static void check(int ok, const char *what)
{
    printf("voice4: %-90s %s\n", what, ok ? "ok" : "FAIL");
    fails += !ok;
}
#define NB 1400u
static int32_t out_l[NB * CTL];
static void render(uint32_t nb)
{
    static int32_t o[2u * CTL];
    uint32_t b, i;
    for (b = 0; b < nb && b < NB; b++) {
        mix_block(o, CTL);
        for (i = 0; i < CTL; i++)
            out_l[b * CTL + i] = o[2u * i];
    }
}
static double rms(uint32_t a, uint32_t b)
{
    double e = 0;
    uint32_t i;
    for (i = a; i < b; i++)
        e += (double)out_l[i] * out_l[i];
    return sqrt(e / (double)(b - a));
}
/* the spectral centroid (Hz) of samples a .. a + 1024, a Hann window, by a plain DFT over 256 bins (to 11 kHz) */
static double centroid(uint32_t a)
{
    double num = 0, den = 0;
    uint32_t k, i;
    for (k = 1; k < 256u; k++) {
        double re = 0, im = 0, w = 2 * M_PI * k / 1024.0, m;
        for (i = 0; i < 1024u; i++) {
            double h = 0.5 - 0.5 * cos(2 * M_PI * i / 1024.0), x = h * out_l[a + i];
            re += x * cos(w * i);
            im -= x * sin(w * i);
        }
        m = sqrt(re * re + im * im);
        num += m * k * FS / 1024.0;
        den += m;
    }
    return den > 0 ? num / den : 0;
}
static double band(uint32_t a, double hz)                 /* level of the bin nearest hz in a .. a + 2048 (Hann) */
{
    double re = 0, im = 0, w = 2 * M_PI * hz / FS;
    uint32_t i;
    for (i = 0; i < 2048u; i++) {
        double h = 0.5 - 0.5 * cos(2 * M_PI * i / 2048.0), x = h * out_l[a + i];
        re += x * cos(w * i);
        im -= x * sin(w * i);
    }
    return sqrt(re * re + im * im);
}
static int32_t peak(uint32_t a, uint32_t b)
{
    int32_t m = 0;
    uint32_t i;
    for (i = a; i < b; i++)
        m = abs(out_l[i]) > m ? abs(out_l[i]) : m;
    return m;
}
/* a clean start: the defaults, every send off, one synth track at Type ty, the filter open, no modulation, the amp ENV
 * a held organ (ATK 0, SUS full), mono */
static track_t *setup(uint32_t tr, uint32_t ty)
{
    uint32_t i;
    track_t *t = &trk[tr];
    host_tracks_init();
    memset(ins, 0, sizeof ins);
    for (i = 0; i < NTRK; i++) {
        trk[i].ins_run = 0;
        trk[i].tail = 0;
        trk[i].p[P_REV] = trk[i].p[P_CHOR] = trk[i].p[P_DLY] = trk[i].p[P_DIST] = 0;
        trk[i].p[P_LEVEL] = 100;
        trk[i].p[P_MUTE] = i != tr;
    }
    vtype_switch(t, ty);
    t->engine = t->eng_req;
    t->pold = 0, t->xf_on = 0;
    t->p[P_ATK] = 0, t->p[P_DEC] = 60, t->p[P_SUS] = 127, t->p[P_REL] = 20;
    t->p[P_FCUT] = 127, t->p[P_FRES] = 0, t->p[P_ED_FLT] = 0, t->p[P_FTYPE] = VF_LP24, t->p[P_FKEY] = 0, t->p[P_FDRV] = 0;
    t->p[P_FLINK] = 0;
    t->p[P_LD_FLT] = t->p[P_LD_PIT] = t->p[P_LD_SHP] = t->p[P_LD_AMP] = 0;
    t->p[P_ED_PIT] = t->p[P_ED_SHP] = 0;
    t->p[P_VOICE] = V_MONO, t->p[P_ED_FX] = 0, t->p[P_TRANS] = 0;
    if (t->eng_req == ENGI_PRISM)
        t->p[P_E4] = 0;
    for (i = 0; i < NVOICE; i++)
        t->v[i].active = 0, t->v[i].gate = 0;
    t->nmono = 0, t->mono_note = 0, t->xp_n = 0;
    song.playing = 0;
    song.master_q12 = 2048;
    memset(&lim_dac, 0, sizeof lim_dac);
    lim_dac.env = LIM_T, lim_dac.g = 32768, lim_dac.hold = 0;
    return t;
}
static const char *const FAM[4] = {"ACID", "JUNO", "PLAITS", "ELEMENTS"};
static uint32_t fam_type(uint32_t f)                     /* a Type of each family: ACID, JUNO, PLAITS VA, ELEMENTS BOW */
{
    uint32_t i;
    for (i = 0; i < NVTYPE; i++)
        if ((f == 0 && VTYPES[i].eng == ENGI_ACID) || (f == 1 && VTYPES[i].eng == ENGI_JUNO) ||
            (f == 2 && VTYPES[i].eng == ENGI_PRISM && VTYPES[i].model == PRM_PLX) ||
            (f == 3 && VTYPES[i].eng == ENGI_ELEM && VTYPES[i].model == EM_BOW))
            return i;
    return 0;
}

static void t_types(void)
{
    uint32_t tr, ty, bad = 0, n = 0;
    char b[96];
    for (tr = 0; tr < NPART; tr++)
        for (ty = 0; ty < NVTYPE; ty++) {
            track_t *t = setup(tr, ty);
            double r;
            int32_t pk;
            int ok;
            ok = vtype_of(t) == ty;
            trk_note_on(t, 48, 100);
            render(300);
            r = rms(100u * CTL, 300u * CTL);
            pk = peak(0, 300u * CTL);
            ok &= (r > 30.0 || pk > 3000) && pk < 32767 * 2 && r == r;   /* (PARTICLE: sparse grains, by their peaks) */
            if (!ok) {
                snprintf(b, sizeof b, "  track %u %s (%s): rms %.0f peak %d", tr + 1u, VTYPES[ty].big, VTYPES[ty].name, r, pk);
                check(0, b);
                bad++;
            }
            n++;
        }
    snprintf(b, sizeof b, "every Type selectable and sounding on every track (%u of %u)", n - bad, n);
    check(!bad, b);
}

static void t_filter(void)
{
    uint32_t f;
    char b[96];
    for (f = 0; f < 4u; f++) {
        uint32_t ty = fam_type(f);
        track_t *t;
        double open, shut, flat, res, c0, c1, lo_lp, lo_hp, cl[8], mn = 1e9, mx = 0;
        int32_t pk;
        uint32_t i;
        /* CUTOFF */
        t = setup(0, ty);
        trk_note_on(t, 36, 100);
        render(400);
        open = centroid(300u * CTL);
        lo_lp = band(300u * CTL, 65.4 * 2);
        t = setup(0, ty);
        t->p[P_FCUT] = 40;
        trk_note_on(t, 36, 100);
        render(400);
        shut = centroid(300u * CTL);
        snprintf(b, sizeof b, "%s: CUTOFF down darkens it (centroid %.0f -> %.0f Hz)", FAM[f], open, shut);
        check(shut < open * 0.7, b);
        /* RESO: a peak at the cutoff */
        t = setup(0, ty);
        t->p[P_FCUT] = 70;
        trk_note_on(t, 36, 100);
        render(400);
        flat = centroid(300u * CTL);
        t = setup(0, ty);
        t->p[P_FCUT] = 70, t->p[P_FRES] = 115;
        trk_note_on(t, 36, 100);
        render(400);
        res = centroid(300u * CTL);
        pk = peak(0, 400u * CTL);
        snprintf(b, sizeof b, "%s: RESO up, a peak at the cutoff (centroid %.0f -> %.0f Hz), bounded (peak %d)", FAM[f], flat,
                 res, pk);
        check(fabs(res - flat) > flat * 0.08 && pk < 65535, b);
        /* ENV amount: open on the attack, closing as the filter envelope decays (OWN: a fast decay to 0) */
        t = setup(0, ty);
        t->p[P_FCUT] = 30, t->p[P_ED_FLT] = 40, t->p[P_FLINK] = 1;
        t->p[P_FATK] = 0, t->p[P_FDEC] = 75, t->p[P_FSUS] = 0, t->p[P_FREL] = 20;
        trk_note_on(t, 36, 100);
        render(1200);
        c0 = centroid(10u * CTL);
        c1 = centroid(1100u * CTL);
        snprintf(b, sizeof b, "%s: ENV amount opens it on the attack, it closes as it decays (%.0f -> %.0f Hz)", FAM[f], c0, c1);
        check(c0 > c1 * 1.3, b);
        /* LFO > FLT */
        t = setup(0, ty);
        t->p[P_FCUT] = 60, t->p[P_LD_FLT] = 50, t->p[P_LRATE] = 60, t->p[P_LWAVE] = 0;
        trk_note_on(t, 36, 100);
        render(1400);
        for (i = 0; i < 8u; i++) {
            cl[i] = centroid((200u + i * 140u) * CTL);
            mn = cl[i] < mn ? cl[i] : mn;
            mx = cl[i] > mx ? cl[i] : mx;
        }
        snprintf(b, sizeof b, "%s: LFO > FLT moves it (centroid %.0f .. %.0f Hz)", FAM[f], mn, mx);
        check(mx > mn * 1.25, b);
        /* TYPE HP thins the lows */
        t = setup(0, ty);
        t->p[P_FCUT] = 75, t->p[P_FTYPE] = VF_HP;
        trk_note_on(t, 36, 100);
        render(400);
        lo_hp = band(300u * CTL, 65.4 * 2);
        snprintf(b, sizeof b, "%s: TYPE HP takes the lows away (%.0f -> %.0f at 131 Hz)", FAM[f], lo_lp, lo_hp);
        check(lo_hp < lo_lp * 0.5, b);
    }
}

/* a self-oscillating RESO rings at the cutoff with no input left to it, bounded, and dies with the note */
static void t_selfosc(void)
{
    uint32_t ty = fam_type(2), i;
    track_t *t = setup(0, ty);
    double r, pkhz = 0, best = 0;
    char b[96];
    t->p[P_FCUT] = 70, t->p[P_FRES] = 127;
    trk_note_on(t, 24, 100);
    render(600);
    r = rms(300u * CTL, 600u * CTL);
    for (i = 0; i < 60u; i++) {                          /* where its peak is: 20 Hz .. 20 kHz, 12.7 steps an octave */
        double hz = 100.0 * pow(2.0, i / 8.0), m = band(400u * CTL, hz);
        if (m > best)
            best = m, pkhz = hz;
    }
    snprintf(b, sizeof b, "RESO at the top rings near the cutoff (%.0f Hz, the cutoff %.0f Hz), bounded (peak %d)", pkhz,
             20.0 * pow(2.0, 70 / 12.7), peak(0, 600u * CTL));
    check(r > 100.0 && peak(0, 600u * CTL) < 65535 && pkhz > 20.0 * pow(2.0, 70 / 12.7) * 0.6 &&
          pkhz < 20.0 * pow(2.0, 70 / 12.7) * 1.7, b);
    trk_note_off(t, 24);
    render(800);
    check(rms(700u * CTL, 800u * CTL) < 5.0, "  and it dies with the note (the amp after the filter)");
}

/* the verified bug: ACID's ENV > FLT and LFO > FLT did nothing */
static void t_acid_bug(void)
{
    uint32_t ty = fam_type(0);
    track_t *t;
    double a, b2, c0, c1;
    char b[96];
    t = setup(0, ty);
    t->p[P_FCUT] = 40, t->p[P_ED_FLT] = 0, t->p[P_FLINK] = 1, t->p[P_FATK] = 0, t->p[P_FDEC] = 75, t->p[P_FSUS] = 0;
    trk_note_on(t, 36, 100);
    render(600);
    a = centroid(10u * CTL);
    t = setup(0, ty);
    t->p[P_FCUT] = 40, t->p[P_ED_FLT] = 50, t->p[P_FLINK] = 1, t->p[P_FATK] = 0, t->p[P_FDEC] = 75, t->p[P_FSUS] = 0;
    trk_note_on(t, 36, 100);
    render(600);
    b2 = centroid(10u * CTL);
    snprintf(b, sizeof b, "ACID: ENV > FLT opens its 303 filter (attack centroid %.0f -> %.0f Hz)", a, b2);
    check(b2 > a * 1.3, b);
    t = setup(0, ty);
    t->p[P_FCUT] = 50, t->p[P_LD_FLT] = 60, t->p[P_LRATE] = 50;
    trk_note_on(t, 36, 100);
    render(1400);
    c0 = centroid(200u * CTL), c1 = centroid(500u * CTL);
    {
        double mn = 1e9, mx = 0;
        uint32_t i;
        for (i = 0; i < 8u; i++) {
            double c = centroid((200u + i * 140u) * CTL);
            mn = c < mn ? c : mn, mx = c > mx ? c : mx;
        }
        snprintf(b, sizeof b, "ACID: LFO > FLT moves its 303 filter (centroid %.0f .. %.0f Hz)", mn, mx);
        check(mx > mn * 1.25, b);
    }
    (void)c0, (void)c1;
}

/* LINK: the filter follows the amp ENV; OWN: its own ADSR. A slow amp attack (1 s) and an own envelope that is up at
 * once and gone in ~0.2 s: LINK opens slowly, OWN opens at once then closes */
static void t_link(void)
{
    uint32_t ty = fam_type(1);
    track_t *t;
    double l0, l1, o0, o1;
    char b[96];
    t = setup(0, ty);
    t->p[P_ATK] = 100, t->p[P_FCUT] = 25, t->p[P_ED_FLT] = 50, t->p[P_FLINK] = 0;
    t->p[P_FATK] = 0, t->p[P_FDEC] = 60, t->p[P_FSUS] = 0;
    trk_note_on(t, 36, 100);
    render(1300);
    l0 = centroid(150u * CTL), l1 = centroid(1200u * CTL);
    t = setup(0, ty);
    t->p[P_ATK] = 100, t->p[P_FCUT] = 25, t->p[P_ED_FLT] = 50, t->p[P_FLINK] = 1;
    t->p[P_FATK] = 0, t->p[P_FDEC] = 60, t->p[P_FSUS] = 0;
    trk_note_on(t, 36, 100);
    render(1300);
    o0 = centroid(150u * CTL), o1 = centroid(1200u * CTL);
    snprintf(b, sizeof b, "LINK follows the amp ENV: opening slowly with it (%.0f -> %.0f Hz)", l0, l1);
    check(l1 > l0 * 1.2, b);
    snprintf(b, sizeof b, "OWN follows its own ADSR: open at once, then closed (%.0f -> %.0f Hz)", o0, o1);
    check(o0 > o1 * 1.2, b);
    check(o0 > l0 * 1.2 && l1 > o1 * 1.2, "  LINK and OWN move the same filter differently");
}

static double pitch_of(uint32_t a)                      /* the period by autocorrelation (40 .. 1000 Hz), parabolic */
{
    uint32_t L, best = 0, i;
    double bv = -1e30, r[1200];
    for (L = 44u; L < 1103u; L++) {
        double s = 0;
        for (i = 0; i < 8192u; i++)
            s += (double)out_l[a + i] * out_l[a + i + L];
        r[L] = s;
    }
    for (L = 45u; L < 1102u; L++)                      /* the first strong peak (an octave below scores as high) */
        if (r[L] > r[L - 1] && r[L] >= r[L + 1] && r[L] > bv * 1.02) {
            if (bv > 0 && r[L] < r[best] * 1.05)
                continue;
            bv = r[L], best = L;
            if (r[L] > 0.9 * r[44])
                break;
        }
    if (!best)
        return 0;
    {
        double y0 = r[best - 1], y1 = r[best], y2 = r[best + 1], d = (y0 - y2) / (2 * (y0 - 2 * y1 + y2));
        return FS / ((double)best + d);
    }
}
static void t_octtune(void)
{
    uint32_t ty = fam_type(1);
    track_t *t;
    double f0, f1, f2;
    char b[96];
    t = setup(0, ty);
    t->p[P_FCUT] = 50;
    trk_note_on(t, 45, 100);
    render(1000);
    f0 = pitch_of(200u * CTL);
    t = setup(0, ty);
    t->p[P_FCUT] = 50, t->p[P_OCT] = 1;
    trk_note_on(t, 45, 100);
    render(1000);
    f1 = pitch_of(200u * CTL);
    t = setup(0, ty);
    t->p[P_FCUT] = 50, t->p[P_TUNE] = 50;
    trk_note_on(t, 45, 100);
    render(1000);
    f2 = pitch_of(200u * CTL);
    snprintf(b, sizeof b, "OCT +1 an octave up (%.1f -> %.1f Hz), TUNE +50 a quarter tone (%.1f Hz)", f0, f1, f2);
    check(fabs(f1 / f0 - 2.0) < 0.03 && fabs(f2 / f0 - pow(2.0, 50.0 / 1200.0)) < 0.006, b);
}

int main(int argc, char **argv)
{
    (void)argc, (void)argv;
    srand(1);
    t_types();
    t_filter();
    t_selfosc();
    t_acid_bug();
    t_link();
    t_octtune();
    printf("voice4: %s (%d failed)\n", fails ? "FAILED" : "all ok", fails);
    return fails;
}
