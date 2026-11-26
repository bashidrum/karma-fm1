/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* GLO > DEVICE: the FM-1's own settings (SLOOP's menu, 2.4; KARMA kept the sections). (Review U5, Ben 26 Nov 2026: "Redraw in
 * the GLO frame") its sections are four normal GLO pages now (params.c PAGES: SCREEN, LIGHTS, AUDIO, TOOLS), with the header
 * GLO, the tab row and the dials every GLO page has; SEL the next page, GLO the next category, OCT- / OCT+ step the knob last
 * turned as on any page (never back to HOME). The knobs are virtual ids (V_COLOR ..): dev_desc says what each is, dev_edit
 * turns it. MIDI LEARN's CLEAR moved beside LEARN (GLO > MIDI 3). TOOLS: CALIB (the hardware calibration) and ABOUT, opened
 * by a turn right; ABOUT is a screen over the page (ui.menu = 2), OCT- back to TOOLS. The settings are kept with the
 * settings (settings_later: saved once quiet, as GLO > SYSTEM's) */
static const char *const DISCO_NAME[DISCO_N] = {"QUIET", "NORM", "SHOW"};   /* (wave 11) how much the lights move (a dial
                                                         * shows five characters: NORM) */
static const char *const LIGHTS_NAME[LIGHTS_N] = {"OFF", "LOW", "MID", "HIGH"};   /* every button lit, the labels readable */
static const char *const KEYS_NAME[KEYS_N] = {"OFF", "C", "WHITE", "ALL"};   /* keys lit too, at the LIGHTS level (C: the C
                                                         * keys; review V2: six characters at most) */
static const char *const PAL_NAME[] = {"GREEN", "AMBER", "CYAN", "RED", "MONO"};   /* (gfx.c PALETTES, in order) */
static const char *const USB_NAME[] = {"MIX", "FULL", "STEMS"};   /* (USB AUDIO: the whole mix (was MASTER), every voice, STEMS) */
_Static_assert(sizeof PAL_NAME / sizeof PAL_NAME[0] == NPALETTES, "a name for each palette");

/* knob id's description and its value now (in vtype_shown, params.c) */
static const param_desc_t *dev_desc(uint32_t id, int16_t **valp)
{
    static const param_desc_t D[] = {
        [V_COLOR - V_DEV0] = PE("COLOUR", PAL_NAME, 0), [V_ZOOM - V_DEV0] = PE("ZOOM", N_ONOFF, 0),
        [V_DISCO - V_DEV0] = PE("DISCO", DISCO_NAME, 1), [V_LIGHTS - V_DEV0] = PE("LIGHTS", LIGHTS_NAME, 0),
        [V_KEYS - V_DEV0] = PE("KEYS", KEYS_NAME, 0), [V_NOTES - V_DEV0] = PE("NOTES", N_ONOFF, 0),
        [V_LOWCUT - V_DEV0] = PE("LOWCUT", N_ONOFF, 0), [V_USB - V_DEV0] = PE("USB", USB_NAME, 0),
        [V_SERIAL - V_DEV0] = PE("SERIAL", N_ONOFF, 0), [V_CALIB - V_DEV0] = PE("CALIB", N_GO, 0),
        [V_ABOUT - V_DEV0] = PE("ABOUT", N_GO, 0), [V_LCLEAR - V_DEV0] = PE("CLEAR", N_GO, 0),
    };
    int32_t v = 0;
    switch (id) {
    case V_COLOR: v = (int32_t)settings.palette; break;
    case V_ZOOM: v = settings.zoom != 0; break;
    case V_DISCO: v = disco % DISCO_N; break;
    case V_LIGHTS: v = lights_lvl % LIGHTS_N; break;
    case V_KEYS: v = lights_keys % KEYS_N; break;
    case V_NOTES: v = lights_notes != 0; break;
    case V_LOWCUT: v = settings.lowcut != 0; break;
    case V_USB: v = usb_stems_set ? 2 : usb_full ? 1 : 0; break;
    case V_SERIAL: v = usb_serial != 0; break;
    default: break;                                    /* (CALIB, ABOUT, CLEAR: actions, "--") */
    }
    vtype_shown = (int16_t)v;
    *valp = &vtype_shown;
    return &D[(id - V_DEV0) % (sizeof D / sizeof D[0])];
}
/* a value that does nothing now (dim): KEYS without LIGHTS, NOTES when QUIET, CLEAR with nothing learned */
static int dev_dim(uint32_t id)
{
    return (id == V_KEYS && !lights_lvl) || (id == V_NOTES && disco == DISCO_QUIET) || (id == V_LCLEAR && !ml_count());
}
/* the change waiting for the next start (USB AUDIO, USB SERIAL: usb.c) */
static int dev_restart(uint32_t id)
{
    return (id == V_SERIAL && usb_serial != usb_cdc_now()) || (id == V_USB && usb_stems_set != USB_STEMS_NOW);
}

static void ask_set(uint32_t kind, uint32_t arg, const char *what);   /* (ui_input.c) */
static int32_t dev_step(int32_t v, int32_t s, int32_t n) { return clamp(v + s, 0, n - 1); }
/* knob id turned s (> 0 right, < 0 left): the setting, stopping at the ends; the actions on a turn right */
static void dev_edit(uint32_t id, int32_t s)
{
    if (!s)
        return;
    switch (id) {
    case V_COLOR:
        settings.palette = (uint32_t)dev_step((int32_t)settings.palette, s, (int32_t)NPALETTES);
        palette_set(settings.palette);
        ui.force = 1;
        break;
    case V_ZOOM:
        settings.zoom = s > 0;
        break;
    case V_LOWCUT:
        settings.lowcut = s > 0;
        fx_lowcut = (uint8_t)(settings.lowcut != 0);
        break;
    case V_DISCO:
        disco = (uint8_t)dev_step(disco, s, DISCO_N);
        break;
    case V_LIGHTS:
        lights_lvl = (uint8_t)dev_step(lights_lvl, s, LIGHTS_N);
        break;
    case V_KEYS:
        lights_keys = (uint8_t)dev_step(lights_keys, s, KEYS_N);
        if (lights_keys && !lights_lvl)
            lights_lvl = LIGHTS_LOW;                   /* keys lit need a level: the lowest */
        break;
    case V_NOTES:
        lights_notes = (uint8_t)(s > 0);
        break;
    case V_USB: {                                      /* MASTER, FULL, STEMS (KARMA 3.0: at the next start) */
        int32_t v = dev_step(usb_stems_set ? 2 : usb_full ? 1 : 0, s, 3);
        usb_full = (uint8_t)(v == 1);
        usb_stems_set = (uint8_t)(v == 2);
        if (dev_restart(V_USB))
            ui_message("USB AUDIO: AT THE NEXT START");
        break;
    }
    case V_SERIAL:
        usb_serial = (uint8_t)(s > 0);
        if (dev_restart(V_SERIAL))
            ui_message("USB SERIAL: AT THE NEXT START");
        break;
    case V_CALIB:
        if (s > 0) {
            panel_setup();
            ui.force = 1;
        }
        return;
    case V_ABOUT:
        if (s > 0) {
            ui.menu = 2;
            ui.force = 1;
        }
        return;
    case V_LCLEAR:                                     /* (2.6) every learned CC: asks, as every action that loses
                                                         * something (hold SAVE) */
        if (s > 0) {
            if (ml_count())
                ask_set(ASK_GO, V_LCLEAR, "CLEAR LEARN");
            else
                ui_message("NOTHING LEARNED");
        }
        return;
    default:
        return;
    }
    settings_later = 1;                                /* (kept with the settings, saved once quiet) */
}

/* ABOUT (GLO > DEVICE > TOOLS, a turn right): who made it, every upstream credit kept (a licence requirement), the build
 * date. In the GLO frame: the header GLO ABOUT; lines 15 px apart (the dim credits 14), clear of the bottom edge (review V5) */
static void draw_about(void)
{
    static const struct { uint8_t y; uint8_t c; const char *s; } L[] = {
        {0, 1, "KARMA: ARIEL MUCHNIK"}, {15, 0, "FELUCCA: LEO KUROSHITA"}, {30, 0, "H\xDCGELTON INSTRUMENTS"},
        {45, 1, "@KUROGEDELIC"}, {60, 0, "GPL-3.0, NO WARRANTY"}, {75, 1, "GITHUB: KARMA-FM1"},
        {94, 2, "BASED ON SLOOP (ISOD89)"}, {108, 2, "303: X0X  JUNO: FM-106"}, {122, 2, "CODE: EMILIE GILLET"},
        {136, 2, "DRUMS: LXR-02, ERICA SYNTHS"}, {150, 2, "FONT: TERMINUS (OFL)"},
    };
    uint32_t i, pass;
    if (!ui.force && ui.menu_sig == 2u)
        return;
    ui.menu_sig = 2u;
    lcd_fill(0, 0, 240, 240, C_BLACK);
    op_header("GLO", TE_G4, "ABOUT", &ui.head_sig);
    for (pass = 0; pass < 2u; pass++) {                /* the canvas holds 124 rows: two bands, 22 .. 237 */
        cv_begin(240, pass ? 92u : 124u, C_BLACK);
        cv_oy = pass ? -124 : 0;
        cv_text(4, 2, &FONT_L, "PHEBUS", C_WHITE);     /* (Ben's build: PHEBUS, based on KARMA; review 21) */
        {                                               /* the four voices' colours after the name (the splash's bars) */
            int32_t x = 4 + (int32_t)text_w(&FONT_L, "PHEBUS") + 8, k;
            for (k = 0; k < 4; k++)
                cv_rect(x + k * 14, 12, 10, 10, TE_COL[k]);
        }
        cv_text(cv_text(4, 32, &FONT_S, "BASED ON ", C_HI), 32, &FONT_S, FELUCCA_VERSION, C_HI);
        for (i = 0; i < sizeof L / sizeof L[0]; i++)
            cv_text(4, 50 + L[i].y, &FONT_S, L[i].s, L[i].c == 1u ? C_AMB : L[i].c ? C_DIM : C_HI);
        cv_text(236 - text_w(&FONT_S, "HUGELTON.COM"), 50 + 45, &FONT_S, "HUGELTON.COM", C_AMB);
        cv_text(236 - text_w(&FONT_S, __DATE__), 50 + 75, &FONT_S, __DATE__, C_GRAY);   /* build date (by GITHUB's line) */
        cv_oy = 0;
        cv_blit(0, 22 + pass * 124u);
    }
}

static void enc_drop(void)                             /* knob turns nobody takes */
{
    uint32_t k;
    for (k = 0; k < NE; k++)
        panel_enc(k);
}
