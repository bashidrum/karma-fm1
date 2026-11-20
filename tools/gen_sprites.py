#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Draws the four characters, one a track (1 the robot, 2 the alien, 3 the crocodile, DRUM the snake), and writes
firmware/src/op_sprites.h. Fierce, not cute (Ben, 20 Nov 2026): a grey alien, a croc-man, a chrome robot with a
blank visor, a reared cobra. They dance and strike: each has four poses at full size (IDLE, LEAN: leaning to the
right of the picture, mirrored for the left; UP: arms, tail or head up; STRIKE: shown for a moment when its voice
plays an accent), three for the mixer (IDLE, LEAN, STRIKE) and a head for HOME's lanes.
  tools/gen_sprites.py [PREVIEW.png]     (the preview: every pose, 4x, in the tracks' colours)
Pixel art, 4 bits a pixel, kept in flash: the full size pictures are 56 x 52 and drawn twice as large (each
pixel 2 x 2: chunky, the same stroke as the mixer's); the colours are shades of the track's own colour (a
character is its voice's colour, no other hue) and white and black (op_pal in ui_op.c).
The pictures are made here, by code (ellipses, rounded boxes, thick lines; each part outlined): original
drawings, no artwork copied. Needs Pillow and numpy."""
import math
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

T, DARK, COL, LIGHT, WHITE, BLACK = range(6)       # 0 = see-through; the shades of the track's colour, then eyes
BW, BH = 56, 52                                     # full size (drawn 2x: 112 x 104)
MW, MH = 36, 34                                     # the mixer's
HW, HH = 18, 16                                     # HOME's lanes: the head
OUT = Path(__file__).resolve().parent.parent / "firmware/src/op_sprites.h"
TRACK_RGB = [(140, 118, 240), (40, 160, 245), (20, 196, 170), (255, 128, 108)]   # TE_COL in ui_studio.c


def shades(c):
    """as op_pal in ui_op.c: DARK the colour 3/8 of the way to black ... (kept the same, for the preview)"""
    dk = tuple(v * 3 // 8 for v in c)
    lt = tuple(v + (255 - v) * 4 // 8 for v in c)
    return {T: (12, 12, 14), DARK: dk, COL: c, LIGHT: lt, WHITE: (236, 236, 240), BLACK: (0, 0, 0)}


class Pic:
    """A picture made of parts, each a shape filled with a colour and outlined (DARK, 1 px, 4-connected) as it is
    laid over what is there; then the details (pixels, lines) straight on."""

    def __init__(self, w=BW, h=BH):
        self.w, self.h = w, h
        self.a = np.zeros((h, w), np.uint8)
        self.marks = []                             # (what, x, y): the faces' details, for the mixer's size

    def part(self, draw, fill=COL, out=DARK):
        m = Image.new("L", (self.w, self.h), 0)
        draw(ImageDraw.Draw(m))
        k = np.array(m) > 0
        if out is not None:
            g = k.copy()
            g[1:, :] |= k[:-1, :]
            g[:-1, :] |= k[1:, :]
            g[:, 1:] |= k[:, :-1]
            g[:, :-1] |= k[:, 1:]
            self.a[g & ~k] = out
        self.a[k] = fill

    # shapes as parts
    def ell(self, cx, cy, rx, ry, fill=COL, out=DARK):
        self.part(lambda d: d.ellipse((cx - rx, cy - ry, cx + rx, cy + ry), fill=255), fill, out)

    def box(self, x0, y0, x1, y1, r=0, fill=COL, out=DARK):
        self.part(lambda d: d.rounded_rectangle((x0, y0, x1, y1), r, fill=255), fill, out)

    def tube(self, pts, w, fill=COL, out=DARK):     # a limb: a thick line, round at its joints and ends
        def f(d):
            d.line(pts, fill=255, width=w)
            for x, y in pts:
                d.ellipse((x - (w - 1) / 2, y - (w - 1) / 2, x + (w - 1) / 2, y + (w - 1) / 2), fill=255)
        self.part(f, fill, out)

    def poly(self, pts, fill=COL, out=DARK):
        self.part(lambda d: d.polygon(pts, fill=255), fill, out)

    # details
    def px(self, pts, c):
        for x, y in pts:
            if 0 <= x < self.w and 0 <= y < self.h:
                self.a[int(y), int(x)] = c

    def rect(self, x0, y0, x1, y1, c):              # filled, inclusive
        if y1 >= 0 and x1 >= 0:
            self.a[max(0, y0):y1 + 1, max(0, x0):x1 + 1] = c

    def line(self, pts, c, w=1):
        m = Image.new("L", (self.w, self.h), 0)
        ImageDraw.Draw(m).line(pts, fill=255, width=w)
        self.a[np.array(m) > 0] = c

    def image(self):
        im = Image.new("P", (self.w, self.h))
        im.putdata(self.a.flatten().tolist())
        return im


def rot(pts, ang, ox, oy):
    """points (x forward, y down) turned by ang degrees (negative: up) about (0, 0), then moved to (ox, oy)"""
    c, s = math.cos(math.radians(ang)), math.sin(math.radians(ang))
    return [(ox + x * c - y * s, oy + x * s + y * c) for x, y in pts]


def joint(p, x, y, r=1.5):
    """a mechanical joint: a small round knuckle, a dark pin"""
    p.ell(x, y, r, r)
    p.px([(round(x), round(y))], DARK)


def claw(p, x, y, dx, dy, n=3, spread=2):
    """n white claws from (x, y), two pixels each, pointing (dx, dy), fanned across"""
    ox, oy = -dy, dx
    for k in range(n):
        o = (k - (n - 1) / 2) * spread
        bx, by = x + ox * o, y + oy * o
        p.px([(round(bx), round(by)), (round(bx + dx + ox * o * 0.3), round(by + dy + oy * o * 0.3))], WHITE)


def fingers(p, x, y, tips):
    """long thin fingers (outlined) from the palm (x, y) to each tip"""
    for tx, ty in tips:
        p.tube([(x, y), (tx, ty)], 1)
    p.ell(x, y, 1.5, 1.5)


# --------------------------------------------------------------------------------------------- the robot ---
def seg(p, a, b, w, fill=COL):
    """a limb segment as a hard rectangle from a to b, w wide"""
    dx, dy = b[0] - a[0], b[1] - a[1]
    n = math.hypot(dx, dy) or 1
    ox, oy = -dy / n * w / 2, dx / n * w / 2
    p.poly([(a[0] + ox, a[1] + oy), (b[0] + ox, b[1] + oy), (b[0] - ox, b[1] - oy), (a[0] - ox, a[1] - oy)], fill)


def clamp(p, e, d, open_=1):
    """a clamp hand at e, its two prongs pointing d (a unit direction)"""
    ox, oy = -d[1], d[0]
    seg(p, (e[0] - ox * 3, e[1] - oy * 3), (e[0] + ox * 3, e[1] + oy * 3), 3, LIGHT)
    for sgn in (-1, 1):
        k = 2.5 + (0.8 if open_ else 0)
        a = (e[0] + ox * k * sgn + d[0], e[1] + oy * k * sgn + d[1])
        seg(p, a, (a[0] + d[0] * 4, a[1] + d[1] * 4), 2, LIGHT)


def robot(pose):
    """track 1: the robot (Ben, 20 Nov 2026: "more classic ... blockier"): a classic box robot, all right angles:
    a square head with an antenna, a black visor slit and a grille of a mouth, a box of a chest with a panel and
    rivets, square shoulders, limbs of rectangles, clamps for hands, slabs for feet. IDLE: planted wide, clamps
    down; LEAN: to the right, a clamp raised, the other arm bent; UP: both arms straight up, a hop; STRIKE: a
    lunge to the right, an arm thrust out, clamp open, the visor flaring white."""
    p = Pic()
    y = -2 if pose == "UP" else 0
    bx = {"IDLE": 0, "LEAN": 2, "UP": 0, "STRIKE": 4}[pose]
    # legs: thigh and shin a rectangle each, slab feet
    if pose == "LEAN":
        legs = [((23 + bx, 37), (20, 48)), ((33 + bx, 37), (37, 48))]
    elif pose == "STRIKE":
        legs = [((23 + bx, 37), (14, 48)), ((33 + bx, 37), (41, 48))]
    else:
        legs = [((23, 37 + y), (21, 48 + y)), ((33, 37 + y), (35, 48 + y))]
    for a, b in legs:
        seg(p, a, b, 6)
        mx, my = (a[0] + b[0]) / 2, (a[1] + b[1]) / 2
        p.line([(mx - 2, my), (mx + 2, my)], DARK)                     # the knee's seam
        p.box(b[0] - 5, b[1], b[0] + 5, min(51, b[1] + 3), 0)
        p.line([(b[0] - 4, b[1] + 1), (b[0] + 4, b[1] + 1)], LIGHT)
    # hips
    p.box(19 + bx, 33 + y, 37 + bx, 37 + y, 0)
    p.line([(28 + bx, 34 + y), (28 + bx, 36 + y)], DARK)
    # arms: upper arm, forearm (heavier), a clamp
    S = [(14 + bx, 20 + y), (42 + bx, 20 + y)]
    if pose == "IDLE":
        arms = [(S[0], (12, 27), (11, 35), (0, 1)), (S[1], (44, 27), (45, 35), (0, 1))]
    elif pose == "LEAN":
        arms = [(S[0], (11, 27), (11, 34), (0, 1)), (S[1], (49, 15), (50, 7), (0, -1))]
    elif pose == "UP":
        arms = [(S[0], (11, 12 + y), (11, 5 + y), (0, -1)), (S[1], (45, 12 + y), (45, 5 + y), (0, -1))]
    else:
        arms = [(S[0], (8, 24), (6, 17), (0, -1)), (S[1], (48, 21), (50, 21), (1, 0))]
    for sh, el, ha, d in arms:
        seg(p, sh, el, 4)
        seg(p, el, ha, 6)
        clamp(p, ha, d, pose in ("STRIKE", "UP"))
    if pose == "STRIKE":                                               # the thrust: speed lines behind the arm
        for ly, l in ((15, 5), (27, 4)):
            p.line([(40, ly), (40 + l, ly)], LIGHT)
    # the chest: a box, a light panel with a dial, a grille, rivets
    p.box(17 + bx, 16 + y, 39 + bx, 33 + y, 0)
    p.box(20 + bx, 19 + y, 27 + bx, 25 + y, 0, LIGHT)
    p.px([(22 + bx, 21 + y), (23 + bx, 22 + y), (24 + bx, 23 + y)], DARK)
    for k in range(3):
        p.line([(30 + bx, 20 + y + k * 2), (36 + bx, 20 + y + k * 2)], DARK)
    p.rect(21 + bx, 28 + y, 23 + bx, 30 + y, BLACK)
    p.px([(22 + bx, 29 + y)], WHITE)
    p.rect(26 + bx, 28 + y, 28 + bx, 30 + y, DARK)
    p.rect(31 + bx, 28 + y, 35 + bx, 30 + y, DARK)
    p.px([(18 + bx, 17 + y), (38 + bx, 17 + y), (18 + bx, 32 + y), (38 + bx, 32 + y)], LIGHT)
    # shoulders: square blocks
    for sx, sy in S:
        p.box(sx - 3, sy - 4, sx + 3, sy + 2, 0)
        p.line([(sx - 2, sy - 3), (sx + 2, sy - 3)], LIGHT)
    # neck
    hx = bx + {"IDLE": 0, "LEAN": 2, "UP": 0, "STRIKE": 3}[pose]
    p.box(25 + hx, 13 + y, 31 + hx, 16 + y, 0, DARK)
    # the head: a box, ear bolts, an antenna, a black visor slit, a grille
    cx, cy = 28 + hx, 7 + y
    p.box(cx - 9, cy - 1, cx - 7, cy + 2, 0)
    p.box(cx + 7, cy - 1, cx + 9, cy + 2, 0)
    p.box(cx - 7, cy - 5, cx + 7, cy + 6, 0)
    p.line([(cx - 6, cy - 4), (cx + 2, cy - 4)], LIGHT)
    p.rect(cx, cy - 8, cx, cy - 6, COL)
    p.rect(cx - 1, cy - 9, cx + 1, cy - 8, LIGHT)
    p.rect(cx - 5, cy - 2, cx + 5, cy, BLACK)
    if pose == "STRIKE":                                               # the visor flares
        p.rect(cx - 5, cy - 1, cx + 5, cy - 1, WHITE)
        p.px([(cx + 10, cy - 1), (cx + 11, cy - 1), (cx + 12, cy - 1)], LIGHT)
    else:                                                              # two hard eyes in the slit
        p.rect(cx - 4, cy - 1, cx - 2, cy - 1, WHITE)
        p.rect(cx + 2, cy - 1, cx + 4, cy - 1, WHITE)
    for k in range(-4, 5, 2):                                          # the mouth: a grille
        p.rect(cx + k, cy + 3, cx + k, cy + 4, DARK)
    p.marks += [("visor", cx, cy - 1, pose == "STRIKE")]
    return p


# --------------------------------------------------------------------------------------------- the alien ---
def alien(pose):
    """track 2: the alien, a grey: a huge bulbous head tapering to a small chin, enormous black almond eyes slanted
    up and out (a glint in each, a frown of a brow over them), a thin neck, a thin body, long thin limbs, long
    three fingered hands. IDLE: arms hanging; LEAN: swaying right, a long hand raised; UP: both hands raised,
    fingers spread, a hop; STRIKE: it leans in and down, eyes wider, mouth open, both hands reaching, spread."""
    p = Pic()
    y = -2 if pose == "UP" else 0
    bx = {"IDLE": 0, "LEAN": 2, "UP": 0, "STRIKE": 2}[pose]
    hx = {"IDLE": 0, "LEAN": 4, "UP": 0, "STRIKE": 5}[pose]
    hy = 3 if pose == "STRIKE" else 0
    c = 27.5
    # legs: long, thin, knobbly knees, small feet
    if pose == "LEAN":
        legs = [[(25 + bx, 40), (22, 45), (20, 50)], [(31 + bx, 40), (34, 45), (36, 50)]]
    elif pose == "STRIKE":
        legs = [[(25 + bx, 40), (20, 45), (17, 50)], [(31 + bx, 40), (36, 44), (37, 50)]]
    else:
        legs = [[(25, 40 + y), (23, 45 + y), (22, 50 + y)], [(30, 40 + y), (32, 45 + y), (33, 50 + y)]]
    for lg in legs:
        p.tube(lg, 2)
        fx, fy = lg[2]
        p.poly([(fx - 2, fy + 1), (fx + 3, fy + 1), (fx + 1, fy - 1)])
    # arms: long and thin, with long fingers
    S = [(23 + bx, 30 + y + hy // 2), (32 + bx, 30 + y + hy // 2)]
    if pose == "IDLE":
        arms = [[S[0], (19, 36), (16, 43)], [S[1], (36, 36), (39, 43)]]
        tips = [[(14, 48), (16, 49), (18, 48)], [(37, 48), (39, 49), (41, 48)]]
    elif pose == "LEAN":
        arms = [[S[0], (17, 34), (11, 38)], [S[1], (40, 25), (46, 17)]]
        tips = [[(7, 42), (8, 43), (11, 43)], [(45, 11), (48, 11), (51, 13)]]
    elif pose == "UP":
        arms = [[S[0], (16, 24 + y), (10, 16 + y)], [S[1], (39, 24 + y), (45, 16 + y)]]
        tips = [[(5, 13 + y), (7, 10 + y), (10, 10 + y)], [(45, 10 + y), (48, 10 + y), (50, 13 + y)]]
    else:
        arms = [[S[0], (16, 33), (9, 29)], [S[1], (41, 30), (48, 27)]]
        tips = [[(3, 25), (4, 29), (4, 32), (7, 34)], [(52, 21), (54, 25), (54, 28), (51, 32)]]
    for a, tp in zip(arms, tips):
        p.tube(a, 2)
        fingers(p, *a[2], tp)
    # neck, body: thin, a pot belly hint, ribs
    p.tube([(c + bx + (hx - bx) * 0.6, 25 + y + hy), (c + bx, 30 + y)], 2)
    p.ell(c + bx, 34 + y, 5, 6)
    p.px([(int(c + bx) - 2, 31 + y), (int(c + bx) - 1, 31 + y), (int(c + bx) - 2, 33 + y), (int(c + bx) - 1, 33 + y),
          (int(c + bx) + 2, 31 + y), (int(c + bx) + 3, 31 + y), (int(c + bx) + 2, 33 + y), (int(c + bx) + 3, 33 + y)], DARK)
    # the head: a dome, tapering to the chin
    hc, ty = c + hx, y + hy
    big = pose == "STRIKE"

    def head(d):
        d.ellipse((hc - 13, ty, hc + 13, ty + 20), fill=255)
        d.polygon([(hc - 12, ty + 12), (hc + 12, ty + 12), (hc + 3, ty + 25), (hc - 3, ty + 25)], fill=255)
    p.part(head)
    p.part(lambda d: d.ellipse((hc - 9, ty + 2, hc - 3, ty + 6), fill=255), LIGHT, None)   # the dome's shine
    p.px([(int(hc) - 8, ty + 7), (int(hc) - 9, ty + 8)], LIGHT)
    for sx in (-1, 1):                                                  # the eyes
        def eyep(d, sx=sx):
            e = [(1, 0), (5, -3), (11, -4), (12, -1), (9, 3), (4, 4)] if not big else \
                [(0, 0), (5, -4), (12, -5), (13, -1), (10, 4), (4, 5)]
            d.polygon([(hc + sx * (x + 0.5) - (0.5 if sx < 0 else 0), ty + 14 + yy) for x, yy in e], fill=255)
        p.part(eyep, BLACK, DARK)
        gx = int(hc + sx * 4) - (1 if sx < 0 else 0)
        p.px([(gx + sx, ty + 12), (gx + 2 * sx, ty + 12), (gx + sx, ty + 13)], WHITE)   # the glint
        p.px([(gx + 6 * sx, ty + 16)], LIGHT)
        p.px([(int(hc) + sx * 2 - (1 if sx < 0 else 0), ty + 10), (int(hc) + sx * 3 - (1 if sx < 0 else 0), ty + 11)], DARK)   # the frown
        p.marks.append(("aeye", hc + sx * 7, ty + 14, big))
    p.px([(int(hc) - 1, ty + 19), (int(hc) + 1, ty + 19)], DARK)      # nostrils
    if big:                                                            # mouth open
        p.rect(int(hc) - 2, ty + 21, int(hc) + 2, ty + 23, BLACK)
        p.px([(int(hc) - 3, ty + 22), (int(hc) + 3, ty + 22)], DARK)
    else:
        p.rect(int(hc) - 2, ty + 22, int(hc) + 2, ty + 22, DARK)
    return p


# ----------------------------------------------------------------------------------------- the crocodile ---
def croc(pose):
    """track 3: the crocodile, a muscular croc-man standing upright, hunched, turned to the right: a heavy tail on
    the ground, ridged scales down its back and tail, a pale plated belly, thick arms with white claws, clawed
    feet, a long snout full of teeth, an eye under a hard brow. IDLE: hunched, claws forward, jaws shut and
    toothy; LEAN: to the right, a claw raised, the tail swung up; UP: head back, jaws parted, both claws up, a
    hop; STRIKE: the jaws snap wide open, claws out."""
    p = Pic()
    y = -2 if pose == "UP" else 0
    bx = {"IDLE": 0, "LEAN": 2, "UP": 0, "STRIKE": 3}[pose]
    au, al = {"IDLE": (0, 0), "LEAN": (-4, 4), "UP": (-28, -2), "STRIKE": (-38, 26)}[pose]
    # the tail, behind it all: heavy, on the ground (swung up when it moves)
    if pose in ("IDLE", "STRIKE"):
        tail = [(19, 31), (23, 44), (14, 49), (5, 51), (0, 51), (0, 49), (8, 46), (14, 40)]
        ridge = [(15, 40), (11, 44), (6, 47)]
    else:
        tail = [(19, 30 + y), (23, 43 + y), (14, 44 + y), (7, 40 + y), (2, 33 + y), (5, 33 + y), (10, 37 + y), (15, 36 + y)]
        ridge = [(13, 37 + y), (8, 36 + y), (5, 33 + y)]
    p.poly(tail)
    for tx, ty in ridge:                                               # scutes along the tail
        p.poly([(tx - 1, ty), (tx + 1, ty - 1), (tx + 1, ty + 1)], COL)
        p.px([(tx, ty)], DARK)
    # legs: thick, bent, clawed feet
    if pose == "LEAN":
        legs = [[(21 + bx, 41), (17, 45), (17, 49)], [(29 + bx, 41), (33, 45), (32, 49)]]
    elif pose == "STRIKE":
        legs = [[(21 + bx, 41), (16, 45), (14, 49)], [(29 + bx, 41), (35, 44), (36, 49)]]
    else:
        legs = [[(21, 41 + y), (19, 45 + y), (19, 49 + y)], [(29, 41 + y), (32, 45 + y), (31, 49 + y)]]
    for lg in legs:
        p.tube(lg[:2], 6)
        p.tube(lg[1:], 4)
        fx, fy = lg[2]
        p.box(fx - 3, fy, fx + 4, fy + 2, 1)
        p.px([(fx + 5, fy + 2), (fx + 5, fy + 1), (fx + 3, fy + 3 if fy + 3 < 52 else fy + 2)], WHITE)
    # the far arm, behind the body
    if pose == "IDLE":
        far = [(23, 24), (21, 31), (29, 33)]
    elif pose == "LEAN":
        far = [(23 + bx, 24), (13, 22), (9, 12)]                       # (thrown out behind and up: clear of the body)
    elif pose == "UP":
        far = [(23, 22 + y), (13, 15 + y), (10, 5 + y)]
    else:
        far = [(24 + bx, 24), (20, 30), (31, 31)]
    p.tube(far[:2], 5)
    p.tube(far[1:], 4)
    # the body: a hunched barrel, a plated belly
    p.part(lambda d: (d.ellipse((15 + bx, 21 + y, 35 + bx, 45 + y), fill=255),
                      d.ellipse((19 + bx, 16 + y, 35 + bx, 30 + y), fill=255)))
    p.part(lambda d: d.ellipse((26 + bx, 25 + y, 35 + bx, 44 + y), fill=255), LIGHT, None)
    for k in range(5):
        yy = 28 + y + k * 3
        p.line([(28 + bx, yy), (34 + bx, yy)], DARK)
    # scale rows on the back, ridges down the spine
    for k, (sx, sy) in enumerate([(19, 19), (16, 24), (15, 29), (15, 34), (16, 39)]):
        p.poly([(sx + bx - 2, sy + y), (sx + bx, sy + y - 2), (sx + bx + 1, sy + y + 1)], COL)
        p.px([(sx + bx + 3, sy + y + 1), (sx + bx + 5, sy + y + 1), (sx + bx + 4, sy + y + 3)], DARK)
    # the head: the skull, then the jaws on a hinge
    H = (32 + bx, 17 + y)
    up = rot([(-4, -6), (3, -6), (16, -4), (21, -3), (22, -1), (21, 0), (0, 1)], au, *H)
    lo = rot([(0, 0), (20, 0), (21, 1), (19, 3), (6, 4), (-3, 4)], al, *H)
    p.poly(lo)
    p.ell(H[0] - 1, H[1] - 2, 5, 5)
    p.poly(up)
    if au != al:                                                       # open: the mouth, teeth top and bottom
        mouth = rot([(1, 1), (20, 1)], au, *H)[::-1] + rot([(1, -1), (19, -1)], al, *H)
        mouth = [H] + rot([(19, 1)], au, *H) + rot([(18, -1)], al, *H)
        p.part(lambda d: d.polygon(mouth, fill=255), BLACK, None)
        for t in range(4, 20, 3):
            (ax, ay), = rot([(t, 1.5)], au, *H)
            (bx2, by2), = rot([(t + 1, -1.5)], al, *H)
            p.px([(round(ax), round(ay))], WHITE)
            p.px([(round(bx2), round(by2))], WHITE)
        p.marks.append(("cjaw", H[0], H[1], 1))
    else:                                                              # shut: interlocking teeth along the jaw
        for t in range(5, 21, 2):
            (ax, ay), = rot([(t, 1 if t % 4 == 1 else 0)], au, *H)
            p.px([(round(ax), round(ay))], WHITE)
        p.line([rot([(1, 0.5)], au, *H)[0], rot([(20, 0.5)], au, *H)[0]], DARK)
        for t in range(6, 20, 4):
            (ax, ay), = rot([(t, 1)], au, *H)
            p.px([(round(ax), round(ay))], WHITE)
        p.marks.append(("cjaw", H[0], H[1], 0))
    (nx, ny), = rot([(19, -5)], au, *H)                                # the nostril bump
    p.ell(nx, ny, 1.5, 1)
    # the eye: on top of the skull, under a heavy brow
    (ex, ey), = rot([(1, -6)], au * 0.5, *H)
    ex, ey = round(ex), round(ey)
    p.ell(ex, ey, 3, 2.5)
    p.rect(ex - 1, ey, ex + 1, ey + 1, BLACK)
    p.px([(ex + 1, ey)], WHITE)
    p.px([(ex - 2, ey - 2), (ex - 1, ey - 2), (ex, ey - 1), (ex + 1, ey - 1), (ex + 2, ey - 1)], DARK)
    p.marks.append(("ceye", ex, ey, 0))
    # the near arm: a big shoulder, a thick forearm, claws
    if pose == "IDLE":
        near, cl = [(30, 25), (35, 32), (41, 30)], (1, 0)
    elif pose == "LEAN":
        near, cl = [(30 + bx, 26), (40, 32), (48, 29)], (1, 0)         # (reaching out under the snout)
    elif pose == "UP":
        near, cl = [(29, 24 + y), (39, 29 + y), (47, 24 + y)], (1, -1)
    else:
        near, cl = [(31 + bx, 25), (39, 31), (46, 33)], (1, 0)
    p.ell(near[0][0], near[0][1], 4, 4)
    p.tube(near[:2], 5)
    p.tube(near[1:], 5)
    p.px([(near[0][0] - 1, near[0][1] - 2), (near[0][0], near[0][1] - 3)], LIGHT)
    hx, hy = near[2]
    p.ell(hx, hy, 2.5, 2.5, COL)
    claw(p, hx + cl[0] * 3, hy + cl[1] * 3, *cl, n=3, spread=1.5 if pose == "STRIKE" else 1)
    if pose in ("UP", "STRIKE", "LEAN"):
        fx, fy = far[2]
        claw(p, fx + (0 if pose == "UP" else 2 if pose == "STRIKE" else -1), fy - (3 if pose != "STRIKE" else 0),
             *((0, -1) if pose != "STRIKE" else (1, 0)), n=3)
    return p


# --------------------------------------------------------------------------------------------- the snake ---
def snake(pose):
    """the drums: the snake (Ben, 20 Nov 2026: "lose the hood"): a viper reared out of its coils on a slim neck in
    an S, belly plates down the neck's front, a flat wedge of a head, a slit eye under a hard brow, the mouth open
    on two white fangs. (Kept under the picture's top 6 rows: the drum screen is lower.) IDLE: reared, fangs
    bared; LEAN: swaying right, the forked tongue out; UP: stretched tall, hissing; STRIKE: the neck straightens
    and lunges to the right, jaws wide, fangs and tongue out."""
    p = Pic()
    gape = {"IDLE": 2, "LEAN": 2, "UP": 4, "STRIKE": 5}[pose]
    # the coils, the bottom one first; the tail's tip
    tip = [(42, 48), (49, 47), (52, 43), (51, 40)] if pose != "IDLE" else [(42, 49), (49, 49), (53, 47)]
    p.tube(tip, 3)
    p.ell(27, 47, 18, 4.5)
    p.ell(25, 41, 12, 3.5)
    p.rect(13, 45, 40, 45, LIGHT)
    p.rect(16, 39, 33, 39, LIGHT)
    for k in range(11, 44, 4):                                         # scales across them
        p.px([(k, 48), (k + 2, 50)], DARK)
    for k in range(16, 36, 4):
        p.px([(k + 1, 42), (k + 3, 43)], DARK)
    # the head's hinge, then the neck up to it from the coils: slim, an S
    H = {"IDLE": (27, 15), "LEAN": (33, 16), "UP": (28, 12), "STRIKE": (37, 21)}[pose]
    end = (H[0] - 4, H[1] + 1)
    if pose == "IDLE":
        ctrl = [(26, 40), (32, 34), (24, 27), (23, 20), end]
    elif pose == "LEAN":
        ctrl = [(25, 40), (19, 33), (28, 27), (30, 21), end]
    elif pose == "UP":
        ctrl = [(26, 40), (31, 33), (24, 25), (23, 17), end]
    else:
        ctrl = [(25, 40), (22, 34), (29, 28), end]
    p.tube(ctrl, 6)
    def neck_x(yy):                                                    # the neck's centre at a row
        for (x0, y0), (x1, y1) in zip(ctrl, ctrl[1:]):
            if y1 <= yy <= y0:
                return x0 + (x1 - x0) * (y0 - yy) / max(1, y0 - y1)
        return ctrl[-1][0]
    for yy in range(end[1] + 2, 39):                                   # the belly plates: a pale band, a line a plate
        x = round(neck_x(yy)) + 1
        p.rect(x - 1, yy, x + 1, yy, LIGHT if yy % 2 else DARK)
    # the head: flat, a wedge turned to the right, the jaws on a hinge, open
    au, al = {"IDLE": (-6, 16), "LEAN": (-6, 14), "UP": (-22, 26), "STRIKE": (-24, 36)}[pose]
    def J(pts, a):
        return rot([(x * 1.3, yy * 1.3) for x, yy in pts], a, *H)
    lo = J([(-2, 0), (10, 0), (11, 0.8), (9, 2), (-3, 2.5)], al)
    up = J([(-6, -1), (-5, -3.2), (-1, -4.4), (5, -3.8), (10, -2.2), (12, -1), (11.5, 0), (0, 1), (-6, 1)], au)
    p.poly(lo, LIGHT)                                                  # (its pale underside)
    p.poly(up)
    mouth = [H] + J([(11, 0.5)], au) + J([(10, -0.3)], al)
    p.part(lambda d: d.polygon(mouth, fill=255), BLACK, None)
    for f0, f1 in (((9, 0.6), (9, 3.4)), ((6, 0.6), (6, 2.4))):       # the fangs, down from the upper jaw
        (x0, y0), (x1, y1) = J([f0, f1], au)
        p.line([(round(x0), round(y0)), (round(x1), round(y1))], WHITE)
    (ex, ey), = J([(2, -2.4)], au)                                     # the eye: a slit under a brow
    ex, ey = round(ex), round(ey)
    p.px([(ex - 1, ey), (ex, ey), (ex + 1, ey)], BLACK)
    p.px([(ex + 1, ey)], WHITE)
    p.px([(ex - 2, ey - 2), (ex - 1, ey - 2), (ex, ey - 1), (ex + 1, ey - 1), (ex + 2, ey - 1)], DARK)
    (nx, ny), = J([(10.5, -1.4)], au)
    p.px([(round(nx), round(ny))], DARK)                               # the nostril
    p.marks.append(("seye", ex, ey, 0))
    if pose != "IDLE":                                                 # the forked tongue, out of the mouth
        (tx, ty), = J([(8, 0)], (au + al) / 2)
        (ux, uy), = J([(15, 0)], (au + al) / 2 * 0.5)
        p.tube([(tx, ty), (ux, uy)], 1, LIGHT)
        ux, uy = round(ux), round(uy)
        p.px([(ux + 1, uy - 1), (ux + 2, uy - 2), (ux + 1, uy + 1), (ux + 2, uy + 2)], LIGHT)
    p.marks.append(("smouth", H[0] - 3, H[1], gape))
    if pose == "STRIKE":                                               # the lunge: speed lines behind the head
        for ly, l in ((H[1] - 6, 5), (H[1] - 2, 4)):
            p.line([(H[0] - 16 - l, ly), (H[0] - 16, ly)], LIGHT)
    return p


CHARS = [("ROBOT", robot), ("ALIEN", alien), ("CROC", croc), ("SNAKE", snake)]
POSES = ["IDLE", "LEAN", "UP", "STRIKE"]



# ------------------------------------------------------------------------------------------ the mixer's size ---
def _ring(k):                                       # the pixels next to a shape (4-connected)
    g = k.copy()
    g[1:, :] |= k[:-1, :]
    g[:-1, :] |= k[1:, :]
    g[:, 1:] |= k[:, :-1]
    g[:, :-1] |= k[:, 1:]
    return g & ~k


def mixer(p):
    """the full size picture made small, MW x MH: the outline taken off, the rest shrunk (each pixel the colour that
    weighs most in its patch: the eyes' white and black most), outlined again; then the faces' details drawn
    again at this size (they would be lost)"""
    WEIGHT = {DARK: 1.3, COL: 1.0, LIGHT: 1.5, WHITE: 3.2, BLACK: 2.6}
    a = p.a.copy()
    a[(a == DARK) & _ring(a == T)] = T                           # (DARK next to the outside: the outline)
    w, h = MW - 2, MH - 2
    sx, sy = BW / w, BH / h
    o = np.zeros((MH, MW), np.uint8)
    for y in range(h):
        for x in range(w):
            sc, n, solid = {}, 0.0, 0.0
            for yy in range(int(y * sy), int(np.ceil((y + 1) * sy))):
                for xx in range(int(x * sx), int(np.ceil((x + 1) * sx))):
                    f = (min((x + 1) * sx, xx + 1) - max(x * sx, xx)) * (min((y + 1) * sy, yy + 1) - max(y * sy, yy))
                    v = a[yy, xx]
                    n += f
                    if v:
                        solid += f
                        sc[v] = sc.get(v, 0) + WEIGHT[v] * f
            if solid * 2 >= n:
                o[y + 1, x + 1] = max(sc, key=sc.get)
    o[_ring(o != T)] = DARK

    def m(x, y):
        return int(x / sx) + 1, int(y / sy) + 1

    def put(x, y, c):
        if 0 <= x < MW and 0 <= y < MH:
            o[y, x] = c
    for what, x, y, k in p.marks:
        if what == "visor":                                      # the robot's: black glass, a glint (or the flare)
            x0, y0 = m(x - 5, y - 1)
            x1, y1 = m(x + 5, y + 1)
            o[y0:y1 + 1, x0:x1 + 1] = BLACK
            if k:
                o[(y0 + y1) // 2, x0:x1 + 1] = WHITE
            else:
                put(x0 + 1, (y0 + y1) // 2, WHITE)
                put(x1 - 1, (y0 + y1) // 2, WHITE)
        elif what == "aeye":                                     # the alien's: a slanted black almond, a glint
            cx, cy = m(x, y)
            d = 1 if x > p.w / 2 else -1
            for dx, dy in ((-1, 0), (0, 0), (1, 0), (2, -1), (-1, 1), (0, 1), (1, 1)) + (((2, 0), (3, -1)) if k else ()):
                put(cx + d * dx - (1 if d < 0 else 0), cy + dy, BLACK)
            put(cx + d * 1 - (1 if d < 0 else 0), cy, WHITE)
        elif what in ("ceye", "seye"):                           # the croc's, the snake's: a slit, a glint
            cx, cy = m(x, y)
            put(cx - 1, cy, BLACK)
            put(cx, cy, WHITE)
    return o


# ------------------------------------------------------------------------------------- HOME's lanes: heads ---
HEADS = {   # HW x HH, drawn by hand: . see-through, o dark, c the colour, l light, w white, k black
"ROBOT": [
"........ll........",
"........cc........",
"..oooooooooooooo..",
"..olllllllccccco..",
"..occcccccccccco..",
"oookkkkkkkkkkkkooo",
"ocokwwwkkkkwwwkoco",
"oookkkkkkkkkkkkooo",
"..occcccccccccco..",
"..occcccccccccco..",
"..occocococococo..",
"..occocococococo..",
"..occcccccccccco..",
"..oooooooooooooo..",
"......occcco......",
"....oooooooooo....",
],
"ALIEN": [
".....oooooooo.....",
"...oocllccccco....".replace("co....", "coo..."),
"..occllcccccccco..",
".occlcccccccccccо.".replace("о", "o"),
".occcccccccccccco.",
"occcccccccccccccco",
"ockkccccccccccckko".replace("ockk", "ocKK").replace("K", "k"),
"okkwkkcccccckkwkko",
"okkkkkkcccckkkkkko",
".occkkkkcckkkkcco.",
"..occckkcckkccco..",
"...occcccccccco...",
"....occcoocccо....".replace("о", "o"),
".....occoocco.....",
"......occcco......",
".......oooo.......",
],
"CROC": [
"..................",
"..oooo............",
".ocllco...........",
".ockwcooooooooo...",
"occccccccccccccoo.",
"occcccccccccccccco",
"occcccccccccccccco",
"occowowowowowowoo.",
"occokkkkkkkkkkko..",
"occowkwkwkwkwko...",
"occollllllllllo...",
".occoooooooooo....",
".occcco...........",
".occcco...........",
"..occco...........",
"..ooooo...........",
],
"SNAKE": [
"..................",
"..................",
"....oooooo........",
"..ooccccccooo.....",
".occccccccccccooo.",
"occccckwccccccccco",
"occcccccccccccccco",
"occccccoowkkwkkoo.",
"occcccokkwkkwkko..",
".occccollllllloo..",
"..occcooooooo.....",
"..occlco..........",
"..ocooco..........",
"..occlco..........",
"..ocooco..........",
"..occlco..........",
],
}


def head(name):
    k = {".": T, "o": DARK, "c": COL, "l": LIGHT, "w": WHITE, "k": BLACK}
    rows = HEADS[name]
    assert len(rows) == HH and all(len(r) == HW for r in rows), name
    return np.array([[k[ch] for ch in r] for r in rows], np.uint8)


def main():
    big = [[f(pz).image() for pz in POSES] for _, f in CHARS]
    L = ["/* generated by tools/gen_sprites.py: the four characters, 4 bits a pixel (op_pal in ui_op.c: 1 dark, 2 the",
         " * track's colour, 3 light, 4 white, 5 black), the first pixel of a byte in its low half. Drawn by that script:",
         " * original pictures. OP_SPR_BIG[track][pose]: IDLE, LEAN (to the right; mirrored for the left), UP, STRIKE (on",
         " * an accent), 56 x 52 drawn twice as large. OP_SPR_MX[track][pose]: the mixer's, IDLE, LEAN, STRIKE. */", "#pragma once",
         f"#define OP_BIG_W {BW}", f"#define OP_BIG_H {BH}"]
    total = 0

    def emit(name, im):
        nonlocal total
        px = np.array(im).flatten().tolist()
        by = [px[i] | (px[i + 1] << 4) for i in range(0, len(px), 2)]
        total += len(by)
        L.append(f"static const uint8_t OP_SPR_{name}[{len(by)}] = {{")
        for i in range(0, len(by), 28):
            L.append("    " + ",".join(str(b) for b in by[i:i + 28]) + ",")
        L.append("};")

    for (name, _), ims in zip(CHARS, big):
        for pz, im in zip(POSES, ims):
            emit(name + ("" if pz == "IDLE" else "_" + pz), im)
    L.append("static const uint8_t *const OP_SPR_BIG[4][4] = {%s};" % ", ".join(
        "{%s}" % ", ".join("OP_SPR_" + n + ("" if pz == "IDLE" else "_" + pz) for pz in POSES) for n, _ in CHARS))
    L += [f"#define OP_MX_W {MW}", f"#define OP_MX_H {MH}", f"#define OP_HM_W {HW}", f"#define OP_HM_H {HH}"]
    small = []
    for name, f in CHARS:
        mx = [mixer(f(pz)) for pz in ("IDLE", "LEAN", "STRIKE")]
        hm = head(name)
        small.append(mx + [hm])
        emit(name + "_MX", mx[0])
        emit(name + "_MX_LEAN", mx[1])
        emit(name + "_MX_STRIKE", mx[2])
        emit(name + "_HM", hm)
    L.append("static const uint8_t *const OP_SPR_MX[4][3] = {%s};" % ", ".join(
        "{OP_SPR_%s_MX, OP_SPR_%s_MX_LEAN, OP_SPR_%s_MX_STRIKE}" % (n, n, n) for n, _ in CHARS))
    L.append("static const uint8_t *const OP_SPR_HM[4] = {%s};" % ", ".join("OP_SPR_%s_HM" % n for n, _ in CHARS))
    OUT.write_text("\n".join(L) + "\n")
    print(OUT, total, "B")
    if len(sys.argv) > 1:
        s = 4
        sheet = Image.new("RGB", (len(POSES) * (BW * s + 16) + 16, 4 * (BH * s + 16) + 16), (30, 30, 34))
        for r, ims in enumerate(big):
            pal = shades(TRACK_RGB[r])
            for c, im in enumerate(ims):
                rgb = Image.new("RGB", im.size)
                rgb.putdata([pal[v] for v in np.array(im).flatten().tolist()])
                sheet.paste(rgb.resize((BW * s, BH * s), Image.NEAREST), (16 + c * (BW * s + 16), 16 + r * (BH * s + 16)))
        sheet.save(sys.argv[1])
        s = 6
        sm = Image.new("RGB", (4 * (MW * s + 16) + 16, 4 * (MH * s + 16) + 16), (30, 30, 34))
        for r, ims in enumerate(small):
            pal = shades(TRACK_RGB[r])
            for c, a in enumerate(ims):
                rgb = Image.new("RGB", (a.shape[1], a.shape[0]))
                rgb.putdata([pal[v] for v in a.flatten().tolist()])
                k = s if c < 3 else 2 * s
                sm.paste(rgb.resize((a.shape[1] * k, a.shape[0] * k), Image.NEAREST), (16 + c * (MW * s + 16), 16 + r * (MH * s + 16)))
        sm.save(sys.argv[1].replace(".png", "-small.png"))


if __name__ == "__main__":
    main()
