#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# PHEBUS wave 11: the lights sheet. Draws the FM-1's buttons and keys as the UI lit them in each scenario tests/ui_pages_test.c
# writes to build/host/lights.txt (a line each: "name:41 levels:disco", level 0 dark, 1 backlight, 2 glow, 3 lit).
#   python3 tools/lights_sheet.py build/host/lights.txt review/wave11/lights.png      (needs Pillow and the Inter font)
import sys
from PIL import Image, ImageDraw, ImageFont

F = "/usr/share/fonts/opentype/inter/"
def font(w, s): return ImageFont.truetype(F + {"b": "Inter-Bold.otf", "s": "Inter-SemiBold.otf", "r": "Inter-Regular.otf"}[w], s)

BTN = ["FX", "SEL", "ENV", "LFO", "EDIT", "GLO", "HOME", "SAVE", "ARP", "SEQ", "PLAY", "REC", "OCT-", "OCT+"]
ORDER = ["OCT-", "OCT+", "FX", "SEL", "ENV", "LFO", "EDIT", "GLO", "HOME", "SAVE", "ARP", "SEQ", "PLAY", "REC"]
DISCO = ["QUIET", "NORMAL", "SHOW"]
BG = (14, 14, 17); CARD = (24, 24, 30); TXT = (236, 236, 240); GREY = (140, 140, 150)
LV_BTN = [(40, 39, 36), (70, 68, 60), (128, 122, 104), (255, 236, 170)]     # dark, backlight, glow, lit
LV_KEY = [(46, 46, 50), (80, 80, 86), (150, 150, 158), (255, 255, 255)]

def main(src, out):
    rows = []
    for l in open(src):
        name, lv, dz = l.rstrip("\n").rsplit(":", 2)
        rows.append((name, [int(c) for c in lv], int(dz)))
    W, RH = 1500, 196
    im = Image.new("RGB", (W, 150 + RH * len(rows) + 40), BG)
    d = ImageDraw.Draw(im)
    fh, fn, fs, fb = font("b", 40), font("s", 24), font("r", 18), font("s", 15)
    d.text((40, 30), "PHEBUS wave 11: the lights", font=fh, fill=TXT)
    lx = 40
    for i, lab in enumerate(["dark", "backlight", "glow", "lit"]):
        d.rounded_rectangle((lx, 96, lx + 30, 118), 5, fill=LV_BTN[i], outline=(90, 90, 96))
        d.text((lx + 40, 107), lab, font=fs, fill=GREY, anchor="lm")
        lx += 40 + d.textlength(lab, font=fs) + 30
    d.text((lx + 20, 107), "a button or key that works with what is held is lit; the rest dark", font=fs, fill=GREY, anchor="lm")
    y = 150
    for name, lv, dz in rows:
        d.rounded_rectangle((24, y, W - 24, y + RH - 14), 14, fill=CARD)
        d.text((44, y + 16), name, font=fn, fill=TXT)
        d.text((W - 44, y + 16), "DISCO " + DISCO[dz], font=fs, fill=GREY, anchor="ra")
        bx = 44
        for b in ORDER:
            i = BTN.index(b)
            d.rounded_rectangle((bx, y + 56, bx + 74, y + 92), 7, fill=LV_BTN[lv[i]])
            d.text((bx + 37, y + 74), b, font=fb, fill=(20, 20, 22) if lv[i] >= 2 else (150, 150, 158), anchor="mm")
            bx += 82
        # the keys: F3 .. G5 (27), white below, black above between
        kx0, ky = 44, y + 104
        wi = 0
        for k in range(27):
            pc = (53 + k) % 12
            if pc in (1, 3, 6, 8, 10):
                continue
            x = kx0 + wi * 44
            d.rectangle((x, ky + 24, x + 40, ky + 62), fill=LV_KEY[lv[14 + k]], outline=(70, 70, 76))
            wi += 1
        wi = 0
        for k in range(27):
            pc = (53 + k) % 12
            if pc in (1, 3, 6, 8, 10):
                x = kx0 + wi * 44 - 14
                d.rectangle((x, ky, x + 26, ky + 22), fill=LV_KEY[lv[14 + k]], outline=(90, 90, 96))
            else:
                wi += 1
        y += RH
    im.save(out)
    print(out, im.size)

main(sys.argv[1], sys.argv[2])
