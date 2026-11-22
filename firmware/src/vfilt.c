/* SPDX-License-Identifier: GPL-3.0-only */
/* The voice filter (PHEBUS wave 4; Ben, 21 Nov 2026: "The filter isn't working as I'd expect a 101 or 303 to work, it
 * doesn't growl or move in the same way"; SPEC "New direction": "Oscillator type plus a few macro knobs, filter,
 * envelopes ... The same layout on every voice"). One resonant filter after the oscillator of every voice, whatever its
 * Type, before the amp (voice.c track_render renders the oscillator at full level, filters it, then applies the
 * envelope: a self-oscillating filter dies with the note, never cut off).
 *
 * A four-pole transistor ladder, as the SH-101's and the Juno's IR3109 (and the 303's diode ladder in spirit), solved
 * without a unit delay in its feedback (the topology-preserving transform: V. Zavalishin, "The Art of VA Filter Design",
 * ch. 4 and 5): each stage a one-pole low-pass, the four in a loop with the feedback k. The input after the feedback
 * goes through a soft saturator, the transistors' tanh in a cheap cubic: it bounds the loop, so at the top of RESO
 * (k past 4) the filter sings a sine of its own at the cutoff, as an analogue one does, and DRIVE pushes the input into it
 * for the growl. TYPE takes the stages' outputs (the Oberheim Xpander's mixes): LP24 the fourth, LP12 the second, BP a
 * 12 dB band-pass, HP a 12 dB high-pass. Float: the AC79's FPU, as ACID (tools/build.py FPU).
 *
 * CUTOFF 0..127 (with every modulation added, 1/256 of a step) spans 20 Hz to 20 kHz, 12.7 steps an octave; the
 * coefficient is worked out once a block and slid across it (no steps in a fast envelope). RESO 0..127: k 0 .. 4.3; the
 * level the resonance takes from the bass is given back in part (1 + 0.6 k), as on a Minimoog's output, not all of it
 * (the thinning is part of the sound). */

#define VF_IN (1.0f / 98304.0f)                /* an oscillator at full level (2 VOICE_FS) into the saturator at ~0.5 */
#define VF_OUT 98304.0f
static inline float vf_sat(float x)            /* tanh's shape: x - 4/27 x^3, flat at +-1.5 (1.0) */
{
    x = x > 1.5f ? 1.5f : x < -1.5f ? -1.5f : x;
    return x - 0.148148148f * x * x * x;
}
/* the stage gain G = g / (1 + g), g = tan(pi fc / fs), from the cutoff in 1/256 steps */
static float vf_gain(int32_t cut8)
{
    float c = (float)cut8 * (1.0f / 256.0f), x, g;
    if (c < -20.0f)
        c = -20.0f;
    x = 20.0f * fm_exp2f(c * (1.0f / 12.7f)) * (3.14159265f / (float)FS);   /* pi fc / fs */
    if (x > 1.28f)                             /* (~18 kHz) */
        x = 1.28f;
    g = x * (15.0f - x * x) / (15.0f - 6.0f * x * x);                     /* tan x (Pade, 0.3 % at 1.28) */
    return g / (1.0f + g);
}
/* n samples (n <= CTL) of b through voice v's filter, in place. cut8: the cutoff with its modulation (1/256 steps),
 * res, drive 0..127, type VF_* */
static void vf_run(voice_t *v, int32_t *b, uint32_t n, int32_t cut8, int32_t res, int32_t drive, uint32_t type)
{
    float s1 = v->vf[0], s2 = v->vf[1], s3 = v->vf[2], s4 = v->vf[3], G = v->vf[4], Gn = vf_gain(cut8), dG;
    float k = (float)clamp(res, 0, 127) * (4.3f / 127.0f), din = VF_IN * (1.0f + (float)clamp(drive, 0, 127) * (7.0f / 127.0f));
    float dout = VF_OUT * (1.0f + 0.6f * k) / (1.0f + (float)clamp(drive, 0, 127) * (2.0f / 127.0f));
    float inv, dinv;
    uint32_t i;
    if (!(G > 0.0f))                           /* (a fresh voice: no slide from nothing) */
        G = Gn;
    dG = (Gn - G) * (1.0f / (float)CTL);
    inv = 1.0f / (1.0f + k * G * G * G * G);   /* the loop's 1 / (1 + k G^4), slid as G (no divide a sample) */
    dinv = (1.0f / (1.0f + k * Gn * Gn * Gn * Gn) - inv) * (1.0f / (float)CTL);
    for (i = 0; i < n; i++) {
        float G2, S, u, y1, y2, y3, y4, w, o;
        G += dG;
        inv += dinv;
        G2 = G * G;
        /* the stages' states as they stand: y4 = G^4 u + S, S = G^3 b1 + G^2 b2 + G b3 + b4, b = (1 - G) s */
        S = (1.0f - G) * (G2 * G * s1 + G2 * s2 + G * s3 + s4);
        u = vf_sat(((float)b[i] * din - k * S) * inv);
        w = (u - s1) * G; y1 = w + s1; s1 = y1 + w;
        w = (y1 - s2) * G; y2 = w + s2; s2 = y2 + w;
        w = (y2 - s3) * G; y3 = w + s3; s3 = y3 + w;
        w = (y3 - s4) * G; y4 = w + s4; s4 = y4 + w;
        o = type == VF_LP12 ? y2 : type == VF_BP ? 4.0f * (y2 - 2.0f * y3 + y4) : type == VF_HP ? u - 2.0f * y1 + y2 : y4;
        b[i] = (int32_t)(o * dout);
    }
    v->vf[0] = fm_flush(s1);
    v->vf[1] = fm_flush(s2);
    v->vf[2] = fm_flush(s3);
    v->vf[3] = fm_flush(s4);
    v->vf[4] = G;
}
