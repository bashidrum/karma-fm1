/* SPDX-License-Identifier: GPL-3.0-only */
/* MASTER (PHEBUS wave 10). Ben, 23 Nov 2026, 09:08: "I feel the master knob is underused ... Would make sense if it was bound to
 * the other controls as a button press and twist master and it changes a broad / master like parameter that's related to the
 * button you're pressing"; picker "Yes, those pairs". Included by ui_layers.c.
 *
 * MASTER is a pot with end stops (the hal's ADC channel 4, 0..1023), not an encoder, so nothing here ever jumps to where the pot
 * is. Every use is RELATIVE, by "value scaling" (the takeover the DAWs call scaling): the pot moved from a to b moves a value
 * at p (0..1 of its range) the same share of the way to the end it is turning toward:
 *     turning up    p += (1 - p) (b - a) / (1023 - a)       turning down    p -= p (a - b) / a
 * so a value and the pot that are apart never jump, both reach their ends together (the pot's end stop is the value's end),
 * and once at an end they move together again. Picked over a pickup (the value waits until the pot passes it: a dead knob,
 * maybe for a whole turn) and over plain relative (the ends could never be reached again once apart).
 *
 *   nothing held    the volume (master_q12, the pot squared as before). After a pair it stays where it was and follows the
 *                   pot again by the same scaling as soon as the pot is turned with nothing held: no jump
 *   a button held   its pair (held or kept open: its layer). The volume does not move
 *       FX   FX MIX  the whole mix's effect return, 0..100 % (voice.c fx_wet)
 *       SEQ  SWING   the selected voice's swing (P_SSWING, its PATTERN page's)
 *       ARP  TRANSP  the selected synth voice's TRANS (ARP > KEY 2), -24..+24
 *       EDIT LEVEL   the selected voice's level (P_LEVEL; the drums G_DRLVL)
 *       ENV  LENGTH  the selected synth voice's amp envelope, every stage together (voice.c mac_env: a scale, nothing
 *                    stored changes); on the drums the selected sound's decay
 *       LFO  DEPTH   the selected synth voice's modulation depth (voice.c mac_dep: a scale of the matrix's modulator amounts,
 *                    nothing stored changes); on the drums the selected sound's LFO DEPTH
 *       GLO  TEMPO   the tempo (G_BPM, as GLO > GLOBAL's KNOB 1)
 * While one turns, the message bar says what and where ("FX MIX 64 %"); the held layer's header shows the pair (its chip). */
#define MP_NOISE 4                                      /* pot steps under this: the ADC's noise, not a turn */
static int16_t mp_pot = -1;                             /* the pot as last taken, -1 before the first reading */
static uint16_t mp_vol;                                 /* the volume's place, 0..65535 (the pot's 0..1023 as Q16 / 64) */
static uint8_t mp_id;                                   /* the pair being turned (its layer), 0 none */
static uint16_t mp_pos;                                 /* its value's place, 0..65535, and the value written last */
static int16_t mp_last;
static uint8_t mp_used;                                 /* a pair turned: the held button is no tap (layers_input) */

static uint32_t mp_scale(uint32_t p, int32_t a, int32_t b)   /* p (0..65535) as the pot goes a -> b (see above) */
{
    if (b > a)
        return a >= 1023 ? p : p + (65535u - p) * (uint32_t)(b - a) / (uint32_t)(1023 - a);
    return a <= 0 ? p : p - p * (uint32_t)(a - b) / (uint32_t)a;
}
/* the pair of layer l on the selected voice: its value's place (lo..hi) and where it is; 0: none there */
typedef struct {
    const char *chip, *word;                            /* the header chip's word (six letters), the readout's */
    int16_t *vp;                                        /* a stored value, or 0: a drum sound's (did) */
    int32_t lo, hi, v;
    uint32_t did;                                       /* the drum sound's parameter id */
    uint16_t col;
} mpair_t;
static int16_t mp_tmp;                                  /* (the macros and the drum sound's value, as an int16 to scale) */
static int master_pair(uint32_t l, mpair_t *m)
{
    track_t *t = TSEL;
    int drum = is_drum(t);
    uint32_t v = song.sel & 3u;
    memset(m, 0, sizeof *m);
    m->col = TE_COL[v];
    switch (l) {
    case LY_FX:
        m->chip = "FX MIX", m->word = "FX MIX ", m->col = C_WHITE;
        mp_tmp = (int16_t)fx_wet, m->vp = &mp_tmp, m->lo = 0, m->hi = 100;
        break;
    case LY_SEQ:
        m->chip = "SWING", m->word = "SWING ", m->vp = &t->p[P_SSWING], m->lo = 0, m->hi = 100;
        break;
    case LY_ARP:
        if (drum)
            return 0;
        m->chip = "TRANSP", m->word = "TRANSPOSE ", m->vp = &t->p[P_TRANS], m->lo = -24, m->hi = 24;
        break;
    case LY_EDIT:
        m->chip = "LEVEL", m->word = "LEVEL ", m->vp = drum ? &song.g[G_DRLVL] : &t->p[P_LEVEL], m->lo = 0, m->hi = 127;
        break;
    case LY_ENV:
    case LY_LFO:
        m->chip = l == LY_ENV ? "LENGTH" : "DEPTH";
        m->word = l == LY_ENV ? "ENV LENGTH " : "MOD DEPTH ";
        if (drum) {                                     /* the selected sound's decay / LFO depth */
            const param_desc_t *d;
            m->did = l == LY_ENV ? drum_dec_id() : DSP_LDEP;
            if (!(d = dsp_desc(drum_model(), lane_snd(drum_lane & 15u), m->did)))
                return 0;
            m->word = l == LY_ENV ? "DECAY " : "LFO DEPTH ";
            mp_tmp = (int16_t)snd_val(drum_lane & 15u, m->did), m->vp = &mp_tmp, m->lo = d->min, m->hi = d->max;
        } else {
            mp_tmp = l == LY_ENV ? mac_env[v % NPART] : mac_dep[v % NPART];
            m->vp = &mp_tmp, m->lo = -64, m->hi = 63;
        }
        break;
    case LY_GLO:
        m->chip = "TEMPO", m->word = "TEMPO ", m->col = C_WHITE, m->vp = &song.g[G_BPM], m->lo = 40, m->hi = 240;
        break;
    default:
        return 0;
    }
    m->v = clamp(*m->vp, m->lo, m->hi);
    return 1;
}
/* the value of pair l as shown ("64 %", "+2", "x1.5" ...) */
static void master_val_str(uint32_t l, const mpair_t *m, char *b)
{
    int32_t v = m->v;
    if (l == LY_FX) {
        fmt_int(b, v);
        str_cpy(b + str_len(b), " %", 3);
    } else if (l == LY_ARP) {
        if (v > 0)
            b[0] = '+', fmt_int(b + 1, v);
        else
            fmt_int(b, v);
    } else if ((l == LY_ENV || l == LY_LFO) && !m->did) {   /* the scale, in % of the stored */
        fmt_int(b, l == LY_ENV ? (int32_t)((mac_env_q12(-v) * 100u + 2048u) >> 12) : (64 + v) * 100 / 64);
        str_cpy(b + str_len(b), " %", 3);
    } else {
        fmt_int(b, v);
    }
}
static void master_write(uint32_t l, const mpair_t *m, int32_t v)
{
    uint32_t sv = song.sel % NPART;
    if (m->did) {
        drum_param_set(m->did, v);
        return;
    }
    if (l == LY_FX)
        fx_wet = (uint8_t)v;
    else if ((l == LY_ENV || l == LY_LFO) && !is_drum(TSEL))
        (l == LY_ENV ? mac_env : mac_dep)[sv] = (int8_t)v;
    else
        *m->vp = (int16_t)v;
}
/* the layer whose pair MASTER turns now (held or kept open), LY_PLAY: none */
static uint32_t master_layer(void)
{
    uint32_t l, b = fm1_in.buttons;
    if (ui.menu)
        return LY_PLAY;
    for (l = LY_FX; l < LY_COUNT; l++)
        if (l != LY_SEL && l != LY_SAVE && ly_bit[l] && (b & ly_bit[l]))
            return l;
    return ly_lock != LY_PLAY && ly_lock != LY_SAVE ? ly_lock : LY_PLAY;
}
/* the pot read (main.c every pass, filtered; the simulator's web_master; the host tests), 0..1023 */
static void master_in(int32_t pot)
{
    uint32_t l;
    int32_t a = mp_pot;
    pot = clamp(pot, 0, 1023);
    if (a < 0) {                                        /* the first reading: the volume is where the pot is (as before) */
        mp_pot = (int16_t)pot;
        mp_vol = (uint16_t)(pot * 64);
        song.master_q12 = (uint32_t)(pot * pot) >> 8;
        return;
    }
    if (pot - a < MP_NOISE && a - pot < MP_NOISE)
        return;
    mp_pot = (int16_t)pot;
    l = master_layer();
    if (l == LY_PLAY) {                                 /* the volume, from where it is (no jump after a pair) */
        uint32_t k10;
        mp_vol = (uint16_t)mp_scale(mp_vol, a, pot);
        k10 = (uint32_t)mp_vol >> 6;
        song.master_q12 = (k10 * k10) >> 8;
        mp_id = 0;
        return;
    }
    {
        mpair_t m;
        int32_t v;
        char b[24], vs[10];
        mp_used = 1;                                    /* (a turn: the held button is no tap, its layer shows) */
        if (!master_pair(l, &m)) {
            ui_message("NOTHING ON MASTER HERE");
            return;
        }
        if (mp_id != l || mp_last != m.v)               /* (another pair, or the value moved by a knob meanwhile) */
            mp_pos = (uint16_t)((uint32_t)(m.v - m.lo) * 65535u / (uint32_t)(m.hi - m.lo));
        mp_id = (uint8_t)l;
        mp_pos = (uint16_t)mp_scale(mp_pos, a, pot);
        v = m.lo + (int32_t)(((uint32_t)mp_pos * (uint32_t)(m.hi - m.lo) + 32767u) / 65535u);
        if (v != m.v)
            master_write(l, &m, v);
        mp_last = (int16_t)v;
        m.v = v;
        master_val_str(l, &m, vs);
        str_cpy(b, m.word, sizeof b);
        ui_say(b, vs);
        master_t = 40;
        ui.force = 1;
    }
}
