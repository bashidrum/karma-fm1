#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# PHEBUS wave 9: the one-page button map, drawn for a phone: python3 tools/button_map.py review/wave9/button-map.png (needs Pillow and the Inter font)
from PIL import Image, ImageDraw, ImageFont
import sys

W = 1080
F = "/usr/share/fonts/opentype/inter/"
def font(w, s): return ImageFont.truetype(F + {"b": "Inter-Bold.otf", "s": "Inter-SemiBold.otf", "r": "Inter-Regular.otf", "m": "Inter-Medium.otf"}[w], s)

BG = (11, 11, 14); CARD = (22, 22, 28); LINE = (44, 44, 54)
WHITE = (240, 240, 244); GREY = (150, 150, 162); DIM = (95, 95, 108)
PUR = (140, 118, 240); PUR_D = (48, 40, 82); COR = (255, 128, 108); COR_D = (87, 44, 37)
MIXC = (235, 235, 240)

f_title = font("b", 64); f_sub = font("m", 30); f_h = font("b", 34); f_lab = font("s", 22)
f_txt = font("r", 28); f_txtb = font("s", 28); f_pill = font("s", 26); f_key = font("b", 36); f_small = font("m", 22)

# each button: name, synth categories [(name, pages)], drum categories or None, sel-pages text, hold, select
MIX = "MIX"
B = [
    ("EDIT", [("OSC", "1 to 4"), ("FILT", "1 to 2")], [("SRC", "1 to 3"), ("FLTR", "1 to 2")],
     "ERASE: keys erase, K1 SHIFT, K3 TRANSP. + OCT- undo, + OCT+ redo",
     "synth: OSC the oscillator Type, FILT the filter TYPE"),
    ("ENV", [("AMP", ""), ("FILT", "")], [("AMP", "1 to 2")], "", "synth: nothing (no type)"),
    ("LFO", [("LFO 1", "+ sync"), ("LFO 2", ""), ("MATRIX", "")], [("LFO", "1 to 2")], "",
     "synth: the LFO's WAVE"),
    ("FX", [("INS", "FX 1 2 3"), ("SENDS", ""), ("SLICER", ""), (MIX, "3 pages")],
     [("FILTER", ""), ("INS", "1 to 2"), ("SENDS", ""), ("SLICER", ""), (MIX, "3 pages")],
     "PERFORM: the keys are effect pads on the whole mix", "synth: the insert TYPE, the SLICER's mode"),
    ("SEQ", [("TRIG", "1 to 2"), ("PATTERN", "")], [("TRIG", "1 to 2"), ("PATTERN", "")],
     "STEPS: GATE DIV SWING STEPS. + OCT- halve (asks), + OCT+ double as a clone", "nothing on TRIG and PATTERN"),
    ("ARP", [("ARP", "1 to 2"), ("KEY", "1 to 2")], [("CHROMATIC", ""), ("OFF", "keys: steps")], "ROLL (drums: RETRIG)",
     "nothing"),
    ("GLO", [("GLOBAL", ""), ("SYSTEM", ""), ("MIDI", "1 to 3"), ("DEVICE", "menu")], None,
     "the MIXER. Drums selected: the 16 keys mute the sounds (F#3 solo)", "nothing"),
    ("SAVE", [("PRESETS", ""), ("USER", ""), ("TOOLS", "")], None, "PATTERNS A to D (play, store)", "nothing"),
    ("HOME", [("HOME", ""), ("SAVER", "4 kinds")], None, "the MENU (GLO's DEVICE, a shortcut)", "nothing"),
]
SHARED = {"GLO", "SAVE", "HOME"}

def tw(d, s, f): return d.textlength(s, font=f)

def wrap(d, s, f, w):
    out, line = [], ""
    for word in s.split():
        t = (line + " " + word).strip()
        if tw(d, t, f) <= w: line = t
        else: out.append(line); line = word
    if line: out.append(line)
    return out

def pill(d, x, y, name, sub, col, dark, mix=False, first=False):
    """a category: name in a rounded box; its pages (SEL's) small under it. Returns the x after it"""
    w = max(tw(d, name, f_pill) + 32, (tw(d, sub, f_small) + 16) if sub else 0)
    h = 46
    if mix:
        d.rounded_rectangle((x, y, x + w, y + h), 10, fill=(40, 40, 46), outline=MIXC, width=2)
        tc = MIXC
    elif first:
        d.rounded_rectangle((x, y, x + w, y + h), 10, fill=col)
        tc = BG
    else:
        d.rounded_rectangle((x, y, x + w, y + h), 10, fill=dark, outline=col, width=2)
        tc = col
    d.text((x + w / 2, y + h / 2 + 1), name, font=f_pill, fill=tc, anchor="mm")
    if sub:
        d.text((x + w / 2, y + h + 18), sub, font=f_small, fill=GREY if not mix else MIXC, anchor="mm")
    return x + w

def chain(d, x, y, cats, col, dark, maxx):
    """the categories in press order, an arrow between, a return arrow at the end; MIX after a divider"""
    x0 = x
    for i, (n, sub) in enumerate(cats):
        mix = n == MIX
        if i:
            if mix:
                d.line((x + 14, y - 6, x + 14, y + 52), fill=GREY, width=3)
                x += 28
            else:
                d.text((x + 16, y + 23), ">", font=f_txtb, fill=GREY, anchor="mm")
                x += 32
        x = pill(d, x, y, n, sub, col, dark, mix, first=(i == 0))
    # back round
    if x + 12 + tw(d, "back to " + cats[0][0], f_small) <= maxx:
        d.text((x + 12, y + 23), "back to", font=f_small, fill=DIM, anchor="lm")
        d.text((x + 12 + tw(d, "back to ", f_small), y + 23), cats[0][0], font=f_small, fill=col, anchor="lm")
    else:
        d.text((x + 12, y + 23), "\u21ba", font=f_txtb, fill=DIM, anchor="lm")
    return x

def draw(out):
    H = 6000
    im = Image.new("RGB", (W, H), BG)
    d = ImageDraw.Draw(im)
    y = 56
    d.text((60, y), "PHEBUS", font=f_lab, fill=GREY)
    d.text((60, y + 34), "How to move around", font=f_title, fill=WHITE)
    y += 130
    rules = [("1", "BUTTON", "opens its first category; again: the next one"),
             ("2", "SEL", "the PAGE button: the next page inside"),
             ("3", "SELECT", "knob 5: the TYPE. On the drums: the sound"),
             ("4", "HOLD", "a shortcut or a performance layer, never the only way")]
    cw = (W - 120 - 20) // 2
    for i, (n, k, t) in enumerate(rules):
        cx = 60 + (i % 2) * (cw + 20); cy = y + (i // 2) * 128
        d.rounded_rectangle((cx, cy, cx + cw, cy + 112), 16, fill=CARD)
        d.ellipse((cx + 20, cy + 22, cx + 66, cy + 68), fill=WHITE)
        d.text((cx + 43, cy + 45), n, font=f_h, fill=BG, anchor="mm")
        d.text((cx + 84, cy + 20), k, font=f_txtb, fill=WHITE)
        for j, l in enumerate(wrap(d, t, f_small, cw - 104)):
            d.text((cx + 84, cy + 56 + j * 26), l, font=f_small, fill=GREY)
    y += 128 * 2 + 18
    # legend
    lx = 60
    for name, c, dark, mix in (("this voice", PUR, PUR_D, False), ("drum voice", COR, COR_D, False), ("whole mix", MIXC, None, True)):
        if mix: d.rounded_rectangle((lx, y, lx + 30, y + 30), 7, fill=(40, 40, 46), outline=MIXC, width=2)
        else: d.rounded_rectangle((lx, y, lx + 30, y + 30), 7, fill=dark, outline=c, width=2)
        d.text((lx + 42, y + 15), name, font=f_small, fill=GREY, anchor="lm")
        lx += 42 + tw(d, name, f_small) + 36
    d.text((60, y + 50), "Small grey text under a category: its pages, walked by SEL.", font=f_small, fill=DIM)
    y += 100

    LX = 60; RX = 244; RW = W - RX - 60
    for name, syn, drum, hold, sel in B:
        # measure the card
        lines = []
        hh = 22
        hh += 96  # synth chain
        if drum: hh += 112
        hl = wrap(d, hold, f_txt, RW - 120) if hold else []
        sl_syn = wrap(d, sel, f_txt, RW - 120)
        sel_d = "the sound (1 of 16)" if drum else None
        hh += 8 + 36 * max(1, len(hl)) + 36 * len(sl_syn) + (36 if sel_d else 0) + 18
        d.rounded_rectangle((LX - 16, y, W - 44, y + hh), 18, fill=CARD)
        # the button, as a key cap
        d.rounded_rectangle((LX, y + 22, LX + 150, y + 22 + 74), 14, fill=(205, 205, 212))
        d.rounded_rectangle((LX, y + 22 + 66, LX + 150, y + 22 + 74), 0, fill=(160, 160, 168))
        d.text((LX + 75, y + 22 + 34), name, font=f_key, fill=(20, 20, 24), anchor="mm")
        d.text((LX + 75, y + 120), "again: next", font=f_small, fill=DIM, anchor="mm")
        cy = y + 26
        shared = name in SHARED
        col, dark = (MIXC, (40, 40, 46)) if shared else (PUR, PUR_D)
        if not shared and drum:
            d.text((RX, cy - 2), "SYNTH", font=f_lab, fill=PUR)
        chain(d, RX + (86 if (drum and not shared) else 0), cy - 10 if not (drum and not shared) else cy - 10, syn, col, dark, W - 60)
        cy += 96
        if drum:
            d.text((RX, cy - 2), "DRUMS", font=f_lab, fill=COR)
            chain(d, RX + 86, cy - 10, drum, COR, COR_D, W - 60)
            cy += 112
        d.line((RX, cy - 6, W - 76, cy - 6), fill=LINE, width=2)
        cy += 6
        d.text((RX, cy + 2), "HOLD", font=f_lab, fill=GREY)
        if hl:
            for j, l in enumerate(hl): d.text((RX + 120, cy + j * 36), l, font=f_txt, fill=WHITE)
        else:
            d.text((RX + 120, cy), "nothing", font=f_txt, fill=DIM)
        cy += 36 * max(1, len(hl))
        d.text((RX, cy + 2), "SELECT", font=f_lab, fill=GREY)
        for j, l in enumerate(sl_syn):
            d.text((RX + 120, cy + j * 36), l, font=f_txt, fill=PUR if l != "nothing" and not shared and "nothing" not in l else DIM)
        cy += 36 * len(sl_syn)
        if sel_d:
            d.text((RX + 120, cy), "drums: " + sel_d, font=f_txt, fill=COR)
            cy += 36
        y += hh + 18

    # SEL and the lock
    for title, body, c in (
        ("SEL  (PAGE)", "Tap: the next page of the category shown, round to the first. A category of one page: nothing. Held: nothing. The tab row names the button's categories; \"2/4\" at its right is the page SEL is on.", WHITE),
        ("A LOCK ON ANY PAGE", "Hold the step, press the page's button (and SEL) until the page shows, turn a knob. SELECT locks a type where one can be locked.", WHITE),
        ("COMBOS, THE FEW", "SEQ + OCT- halve the pattern (asks: keep SEQ, hold REC). SEQ + OCT+ double it as a clone. EDIT + OCT- / OCT+ undo, redo. OCT+ with a pad or F#5 held: latch.", WHITE)):
        lines = wrap(d, body, f_txt, W - 160)
        hh = 70 + 38 * len(lines) + 10
        d.rounded_rectangle((LX - 16, y, W - 44, y + hh), 18, fill=CARD)
        d.text((LX + 4, y + 20), title, font=f_h, fill=c)
        for j, l in enumerate(lines): d.text((LX + 4, y + 70 + j * 38), l, font=f_txt, fill=GREY)
        y += hh + 18
    d.text((W / 2, y + 30), "PHEBUS wave 9  ·  23 Nov 2026  ·  the guide: GUIDE.md section 2", font=f_small, fill=DIM, anchor="mm")
    im = im.crop((0, 0, W, int(y + 70)))
    im.save(out)
    print(out, im.size)

draw(sys.argv[1])
