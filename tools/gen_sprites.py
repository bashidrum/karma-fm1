#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Draws the four characters, one a track (1 the robot, 2 the alien, 3 the crocodile, DRUM the snake), and writes
firmware/src/op_sprites.h. They dance: each has three poses at full size (IDLE, LEAN: leaning to the right of
the picture, mirrored for the left; UP: arms, tail or head up), two for the mixer and a head for HOME's lanes.
  tools/gen_sprites.py [PREVIEW.png]     (the preview: every pose, 4x, in the tracks' colours)
Pixel art, 4 bits a pixel, kept in flash: the full size pictures are 56 x 52 and drawn twice as large (each
pixel 2 x 2: chunky, the same stroke as the mixer's); the colours are shades of the track's own colour (a
character is its voice's colour) and white and black for the eyes (op_pal in ui_op.c).
The pictures are made here, by code (ellipses, rounded boxes, thick lines; each part outlined): original
drawings, no artwork copied. Needs Pillow and numpy."""
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
        self.a[max(0, y0):y1 + 1, max(0, x0):x1 + 1] = c

    def line(self, pts, c, w=1):
        m = Image.new("L", (self.w, self.h), 0)
        ImageDraw.Draw(m).line(pts, fill=255, width=w)
        self.a[np.array(m) > 0] = c

    def image(self):
        im = Image.new("P", (self.w, self.h))
        im.putdata(self.a.flatten().tolist())
        return im


def eye(p, x, y, look=0, big=False):
    """a cute eye: white, a black pupil (looking left -1, ahead 0, right 1), its white glint"""
    if big:                                         # 4 x 5
        p.rect(x, y, x + 3, y + 4, WHITE)
        px = x + 1 + look
        p.rect(px, y + 1, px + 1, y + 4, BLACK)
        p.px([(px, y + 1)], WHITE)
    else:                                           # 3 x 3
        p.rect(x, y, x + 2, y + 2, WHITE)
        p.rect(x + 1 + look, y + 1, x + 1 + look, y + 2, BLACK)


# --------------------------------------------------------------------------------------------- the robot ---
def robot(pose, s=None):
    """track 1: the robot, boxy, an aerial with a light, a visor with two eyes and a smile, a little keyboard on
    its chest (a synth of its own). IDLE: arms down; LEAN: to the right, a hand pointing up and out, the other on
    its hip, a knee up; UP: both arms up, a hop."""
    p = Pic()
    hop = -2 if pose == "UP" else 0
    hx = {"IDLE": 0, "LEAN": 5, "UP": 0}[pose]      # the head's shift; the body's is half of it
    bx = hx // 2
    y = hop
    # legs and feet (behind the body)
    if pose == "LEAN":
        p.tube([(22 + bx, 40), (17, 44), (19, 47)], 5)              # the left knee up, out to the side
        p.box(14, 45, 23, 49, 2)
        p.tube([(33 + bx, 40), (35, 47)], 5)
        p.box(31, 46, 41, 51, 2)
    else:
        for lx in (22, 33):
            p.tube([(lx, 40 + y), (lx, 46 + y)], 5)
        p.box(16, 46 + y, 25, 51 + y, 2)
        p.box(30, 46 + y, 39, 51 + y, 2)
    # arms (behind the body at the shoulders)
    L, R = (17 + bx, 27 + y), (38 + bx, 27 + y)
    if pose == "IDLE":
        arms = [[L, (12, 31), (11, 37)], [R, (43, 31), (44, 37)]]
        hands = [(11, 38), (44, 38)]
    elif pose == "LEAN":
        arms = [[L, (10, 30), (6, 24)], [R, (45, 22), (50, 15)]]        # both arms up and out to the right
        hands = [(5, 22), (51, 13)]
    else:
        arms = [[L, (10, 22), (7, 14)], [R, (45, 22), (48, 14)]]
        hands = [(7, 12), (48, 12)]
    for a in arms:
        p.tube(a, 4)
    for x, yy in hands:
        p.ell(x, yy, 2.5, 2.5, LIGHT)
    # the body: a rounded box, its keyboard
    p.box(16 + bx, 22 + y, 39 + bx, 41 + y, 3)
    p.rect(19 + bx, 31 + y, 36 + bx, 37 + y, WHITE)                 # the keys
    for k in range(6):
        p.rect(19 + bx + 3 * k - 1 if k else 18 + bx, 31 + y, 19 + bx + 3 * k - 1 if k else 18 + bx, 37 + y, DARK)
    p.rect(37 + bx, 31 + y, 37 + bx, 37 + y, DARK)
    p.rect(18 + bx, 30 + y, 37 + bx, 30 + y, DARK)
    p.rect(18 + bx, 38 + y, 37 + bx, 38 + y, DARK)
    for k in (1, 2, 4, 5):                                           # the black keys
        p.rect(19 + bx + 3 * k - 2, 31 + y, 19 + bx + 3 * k - 1, 34 + y, BLACK)
    for k, c in enumerate((LIGHT, WHITE, LIGHT)):                    # three lights over them
        p.rect(21 + bx + 5 * k, 25 + y, 22 + bx + 5 * k, 26 + y, c)
    p.rect(35 + bx, 25 + y, 36 + bx, 26 + y, DARK)
    # the head: aerial, ears, box, visor
    ax = 28 + hx
    p.tube([(ax, 7 + y), (ax, 6 + y)], 2)
    p.ell(ax, 5 + y, 2, 2, WHITE)
    p.box(ax - 15, 11 + y, ax - 13, 16 + y, 1)
    p.box(ax + 12, 11 + y, ax + 14, 16 + y, 1)
    p.box(ax - 12, 6 + y, ax + 11, 21 + y, 3)
    p.box(ax - 9, 9 + y, ax + 8, 18 + y, 2, BLACK, None)
    p.rect(ax - 7, 11 + y, ax - 5, 14 + y, WHITE)                    # eyes: tall, rounded by the visor's black
    p.rect(ax + 4, 11 + y, ax + 6, 14 + y, WHITE)
    if pose == "UP":                                                 # happy: eyes shut ^ ^
        p.rect(ax - 7, 11 + y, ax - 5, 14 + y, BLACK)
        p.rect(ax + 4, 11 + y, ax + 6, 14 + y, BLACK)
        p.px([(ax - 7, 13 + y), (ax - 6, 12 + y), (ax - 5, 13 + y), (ax + 4, 13 + y), (ax + 5, 12 + y), (ax + 6, 13 + y)], WHITE)
    p.px([(ax - 3, 16 + y), (ax - 2, 17 + y), (ax - 1, 17 + y), (ax, 17 + y), (ax + 1, 17 + y), (ax + 2, 16 + y)], LIGHT)
    p.marks += [("visor", ax, y), ("keys", 28 + bx, y)]
    return p


# --------------------------------------------------------------------------------------------- the alien ---
def alien(pose):
    """track 2: the alien, a big round head, two feelers with lights, big dark eyes with a glint, a small smile, a
    little body. IDLE: arms down; LEAN: swaying right, arms out to the side, one up; UP: both arms up, a hop."""
    p = Pic()
    hop = -2 if pose == "UP" else 0
    hx = {"IDLE": 0, "LEAN": 4, "UP": 0}[pose]
    bx = hx // 2
    y = hop
    # legs and feet
    if pose == "LEAN":
        p.tube([(24 + bx, 42), (20, 46), (18, 48)], 4)
        p.ell(16, 48, 4, 2.5)
        p.tube([(32 + bx, 42), (34, 49)], 4)
        p.ell(36, 50, 4.5, 2.5)
    else:
        for lx, fx in ((24, 21), (32, 35)):
            p.tube([(lx, 42 + y), (lx, 48 + y)], 4)
            p.ell(fx, 49 + y, 4.5, 2.5)
    # arms
    L, R = (21 + bx, 33 + y), (35 + bx, 33 + y)
    if pose == "IDLE":
        arms, hands = [[L, (16, 37), (14, 42)], [R, (40, 37), (42, 42)]], [(14, 43), (42, 43)]
    elif pose == "LEAN":
        arms, hands = [[L, (13, 34), (6, 31)], [R, (43, 32), (50, 27)]], [(5, 30), (51, 26)]
    else:
        arms, hands = [[L, (12, 30), (6, 22)], [R, (44, 30), (50, 22)]], [(5, 20), (51, 20)]
    for a in arms:
        p.tube(a, 3)
    for x, yy in hands:
        p.ell(x, yy, 2, 2, LIGHT)
    # the body: a small pear, a pale belly
    p.ell(28 + bx, 37 + y, 8, 8)
    p.ell(28 + bx, 39 + y, 4, 4, LIGHT, None)
    # the head
    hc = 28 + hx
    p.tube([(hc - 7, 8 + y), (hc - 10, 5 + y)], 2)
    p.tube([(hc + 7, 8 + y), (hc + 10, 5 + y)], 2)
    p.ell(hc - 10, 5 + y, 2, 2, WHITE)
    p.ell(hc + 10, 5 + y, 2, 2, WHITE)
    p.ell(hc, 17 + y, 15, 12)
    p.ell(hc - 5, 11 + y, 4, 2, LIGHT, None)                         # a shine on the dome
    for sx in (-1, 1):                                               # the eyes: big, dark, slanted a little
        ex = hc + sx * 7
        p.part(lambda d, ex=ex, sx=sx: d.ellipse((ex - 4, 15 + y, ex + 4, 23 + y), fill=255), BLACK, None)
        p.px([(ex - 2, 17 + y), (ex - 1, 17 + y), (ex - 2, 18 + y)], WHITE)
        p.px([(ex + 2, 21 + y)], WHITE)
        p.marks.append(("aeye", ex, 19 + y))
    if pose == "UP":                                                 # an open, happy mouth
        p.rect(hc - 2, 25 + y, hc + 2, 26 + y, BLACK)
        p.px([(hc - 3, 25 + y), (hc + 3, 25 + y)], DARK)
    else:
        p.px([(hc - 2, 25 + y), (hc - 1, 26 + y), (hc, 26 + y), (hc + 1, 26 + y), (hc + 2, 25 + y)], DARK)
    p.px([(hc - 11, 24 + y), (hc - 10, 24 + y), (hc + 10, 24 + y), (hc + 11, 24 + y)], LIGHT)   # cheeks
    return p


# ----------------------------------------------------------------------------------------- the crocodile ---
def croc(pose):
    """track 3: the crocodile, standing, turned to the right: a long snout with a grin and two teeth, eyes on
    top of its head, a pale belly, bumps down its back, a thick tail. IDLE: arms down, tail on the ground;
    LEAN: to the right, arms up and out, the tail swung up; UP: both arms up, tail up, a hop."""
    p = Pic()
    hop = -2 if pose == "UP" else 0
    hx = {"IDLE": 0, "LEAN": 2, "UP": 0}[pose]
    bx = hx // 2
    y = hop
    # the tail, behind it all
    if pose == "IDLE":
        p.poly([(19, 37), (22, 47), (12, 50), (1, 50), (1, 48), (11, 44)])
    else:
        p.poly([(18 + bx, 35 + y), (22 + bx, 45 + y), (12, 43 + y), (4, 35 + y), (2, 30 + y), (7, 33 + y), (14, 37 + y)])
    # legs and feet: turned to the right
    if pose == "LEAN":
        p.tube([(21 + bx, 44), (17, 46)], 5)
        p.box(12, 44, 21, 48, 2)
        p.tube([(30 + bx, 44), (31, 48)], 5)
        p.box(28, 47, 38, 51, 2)
    else:
        for lx in (21, 30):
            p.tube([(lx, 44 + y), (lx, 48 + y)], 5)
            p.box(lx - 3, 47 + y, lx + 6, 51 + y, 2)
    # the far arm (behind the body)
    if pose == "IDLE":
        p.tube([(18, 31), (14, 36), (14, 39)], 4)
        hand = (14, 40)
    elif pose == "LEAN":
        p.tube([(18 + bx, 31), (11, 30), (7, 25)], 4)
        hand = (6, 23)
    else:
        p.tube([(18, 29 + y), (13, 23 + y), (11, 16 + y)], 4)
        hand = (11, 14 + y)
    p.ell(*hand, 2.5, 2.5, LIGHT)
    # the bumps down its back, the body, its belly
    for k in range(4):
        p.ell(14 + bx, 25 + y + k * 5, 2, 2)
    p.ell(25 + bx, 36 + y, 11, 11)
    p.ell(27 + bx, 38 + y, 6, 8, LIGHT, None)
    for k in range(3):
        p.rect(24 + bx, 35 + y + k * 3, 30 + bx, 35 + y + k * 3, COL)
    # the near arm (raised: behind the snout, its hand above it)
    def near_arm():
        if pose == "UP":
            p.tube([(33, 29 + y), (41, 16 + y), (44, 8 + y)], 4)
            hand = (44, 6 + y)
        elif pose == "LEAN":
            p.tube([(33 + bx, 31), (42, 33), (49, 30)], 4)
            hand = (50, 29)
        else:
            p.tube([(33, 31), (37, 36), (37, 39)], 4)
            hand = (37, 40)
        p.ell(*hand, 2.5, 2.5, LIGHT)
    near_arm()
    # the head: eyes on top, then the skull, the jaw and the snout as one
    hc = 23 + hx
    for sx in (-4, 4):
        p.ell(hc + sx, 8 + y, 4, 4)

    def head(d):
        d.ellipse((hc - 10, 8 + y, hc + 10, 26 + y), fill=255)
        d.polygon([(hc + 2, 11 + y), (hc + 26, 13 + y), (hc + 29, 15 + y), (hc + 29, 22 + y), (hc + 25, 25 + y),
                   (hc + 2, 26 + y)], fill=255)
    p.part(head)
    p.rect(hc + 4, 21 + y, hc + 26, 24 + y, LIGHT)                   # the pale jaw
    p.px([(hc + 27, 21 + y), (hc + 27, 22 + y)], LIGHT)
    for x in range(hc + 4, hc + 29):                                 # the mouth, up at the cheek
        p.px([(x, 20 + y)], DARK)
    p.px([(hc + 3, 19 + y), (hc + 2, 18 + y)], DARK)
    p.px([(hc + 24, 14 + y), (hc + 26, 14 + y)], DARK)               # nostrils
    for x in (hc + 9, hc + 16, hc + 23):                             # teeth
        p.px([(x, 21 + y), (x - 1, 21 + y)], WHITE)
        p.px([(x - 1, 22 + y)], WHITE)
    for sx in (-4, 4):
        eye(p, hc + sx - 1, 6 + y, 1)
    if pose == "UP":                                                 # happy: eyes shut
        for sx in (-4, 4):
            p.rect(hc + sx - 1, 6 + y, hc + sx + 1, 8 + y, COL)
            p.px([(hc + sx - 1, 7 + y), (hc + sx, 6 + y), (hc + sx + 1, 7 + y)], DARK)
    return p


# --------------------------------------------------------------------------------------------- the snake ---
def snake(pose):
    """the drums: the snake, coiled, its head up, big eyes, a pale front. (Kept under the picture's top 6 rows:
    the drum screen is lower.) IDLE: upright, the tail's tip down; LEAN: swaying right in an S, the tongue out,
    the tip up; UP: stretched tall, eyes shut with joy, the tip up."""
    p = Pic()
    hx = {"IDLE": 0, "LEAN": 9, "UP": 0}[pose]
    top = 8 if pose == "UP" else 11
    # the coils, the bottom one first; the tail's tip
    tip = [(42, 47), (49, 44), (51, 39)] if pose != "IDLE" else [(42, 48), (48, 48), (52, 46)]
    p.tube(tip, 4)
    p.ell(28, 46, 17, 5)
    p.ell(27, 39, 13, 4.5)
    for (x0, x1, yy) in ((16, 40, 43), (19, 35, 36)):              # a shine along each
        p.rect(x0, yy, x1, yy, LIGHT)
    # the neck: up from the coils to the head, its pale front
    if pose == "LEAN":
        neck = [(26, 37), (21, 31), (24, 24), (hx + 27, top + 11)]
    elif pose == "UP":
        neck = [(27, 37), (30, 30), (27, 22), (28, top + 11)]
    else:
        neck = [(27, 37), (30, 31), (27, 26), (28, top + 11)]
    p.tube(neck, 9)
    for (x0, y0), (x1, y1) in zip(neck, neck[1:]):
        p.line([(x0 + 2, y0), (x1 + 2, y1)], LIGHT, 2)
    # the head
    hc = 28 + hx
    p.ell(hc, top + 6, 11, 8)
    look = 1 if pose == "LEAN" else 0
    eye(p, hc - 7, top + 3, look, True)
    eye(p, hc + 3, top + 3, look, True)
    if pose == "UP":                                                 # happy: eyes shut
        for ex in (hc - 7, hc + 3):
            p.rect(ex, top + 3, ex + 3, top + 7, COL)
            p.px([(ex, top + 5), (ex + 1, top + 4), (ex + 2, top + 4), (ex + 3, top + 5)], DARK)
    p.px([(hc - 3, top + 10), (hc - 2, top + 11), (hc - 1, top + 11), (hc, top + 11), (hc + 1, top + 11), (hc + 2, top + 10)], DARK)
    p.px([(hc - 9, top + 9), (hc - 8, top + 9), (hc + 7, top + 9), (hc + 8, top + 9)], LIGHT)
    if pose == "LEAN":                                               # the tongue, forked, out to the side
        p.px([(hc + 11, top + 9), (hc + 12, top + 9), (hc + 13, top + 8), (hc + 13, top + 10)], LIGHT)
    return p


CHARS = [("ROBOT", robot), ("ALIEN", alien), ("CROC", croc), ("SNAKE", snake)]
POSES = ["IDLE", "LEAN", "UP"]


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
    for what, x, y in p.marks:
        if what == "visor":                                      # the robot's: black, two eyes, a smile
            x0, y0 = m(x - 9, 9 + y)
            x1, y1 = m(x + 8, 18 + y)
            o[y0:y1 + 1, x0:x1 + 1] = BLACK
            for ex in (x0 + 2, x1 - 3):
                o[y0 + 1:y0 + 3, ex:ex + 2] = WHITE
            o[y1 - 1, x0 + 4:x1 - 3] = LIGHT
        elif what == "keys":                                     # its keyboard: white, black keys, framed
            x0, y0 = m(x - 9, 30 + y)
            x1, y1 = m(x + 9, 38 + y)
            o[y0:y1 + 1, x0:x1 + 1] = DARK
            o[y0 + 1:y1, x0 + 1:x1] = WHITE
            for k in range(x0 + 2, x1 - 1, 2):
                if (k - x0) % 7 != 6:
                    o[y0 + 1:y0 + 3, k] = BLACK
        elif what == "aeye":                                     # the alien's: big, black, a glint
            cx, cy = m(x, y)
            o[cy - 3:cy + 3, cx - 3:cx + 3] = COL
            o[cy - 2:cy + 2, cx - 2:cx + 2] = BLACK
            for dx, dy in ((-2, -2), (1, -2), (-2, 1), (1, 1)):
                put(cx + dx, cy + dy, COL)
            put(cx - 1, cy - 1, WHITE)
    return o


# ------------------------------------------------------------------------------------- HOME's lanes: heads ---
HEADS = {   # HW x HH, drawn by hand: . see-through, o dark, c the colour, l light, w white, k black
"ROBOT": [
"........oo........",
".......owwo.......",
"........oo........",
"........oo........",
"..oooooooooooooo..",
"..occcccccccccco..",
"ooockkkkkkkkkkcooo",
"ocockwwkkkkwwkcoco",
"ocockwwkkkkwwkcoco",
"ocockkkkkkkkkkcoco",
"ocockklkkkklkkcoco",
"ooockkkllllkkkcooo",
"..occcccccccccco..",
"..oooooooooooooo..",
"......occcco......",
"......oooooo......",
],
"ALIEN": [
".ooo..........ooo.",
".owo..........owo.",
".ooo..........ooo.",
"...oo........oo...",
"....oooooooooo....",
"...occllcccccco...",
"..occllcccccccco..",
".occcccccccccccco.",
".ockkkcccccckkkco.",
".ockwkkcccckwkkco.",
".ockkkkcccckkkkco.",
"..ockkcccccckkco..",
"..olcccckkcccclo..",
"...occcccccccco...",
"....oooooooooo....",
"..................",
],
"CROC": [
"..................",
"..................",
".oooo.oooo........",
".owko.owko........",
".owko.owko........",
"occccocccco.......",
"occcccccccoooooo..",
"occccccccccccckco.",
"occcccccccccccccco",
"occoooooooooooooo.",
"occclwlllwllllllo.",
"occclllllllllllo..",
".occoooooooooooo..",
".occcco...........",
".occcco...........",
"..oooo............",
],
"SNAKE": [
"..................",
".....oooooooo.....",
"...ooccccccccoo...",
"..occcccccccccco..",
".occwwwccccwwwcco.",
".occwwkccccwwkcco.",
".occwwkccccwwkcco.",
".occcccccccccccco.",
".olcccccccccccclo.",
".occcocccccocccco.",
"..occcooooooccco..",
"...occcccccccco...",
"....oolcccccoo....",
".....olcccco......",
".....olcccco......",
".....olcccco......",
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
         " * original pictures. OP_SPR_BIG[track][pose]: IDLE, LEAN (to the right; mirrored for the left), UP, 56 x 52",
         " * drawn twice as large. */", "#pragma once",
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
    L.append("static const uint8_t *const OP_SPR_BIG[4][3] = {%s};" % ", ".join(
        "{%s}" % ", ".join("OP_SPR_" + n + ("" if pz == "IDLE" else "_" + pz) for pz in POSES) for n, _ in CHARS))
    L += [f"#define OP_MX_W {MW}", f"#define OP_MX_H {MH}", f"#define OP_HM_W {HW}", f"#define OP_HM_H {HH}"]
    small = []
    for name, f in CHARS:
        mx = [mixer(f(pz)) for pz in POSES[:2]]
        hm = head(name)
        small.append(mx + [hm])
        emit(name + "_MX", mx[0])
        emit(name + "_MX_LEAN", mx[1])
        emit(name + "_HM", hm)
    L.append("static const uint8_t *const OP_SPR_MX[4][2] = {%s};" % ", ".join(
        "{OP_SPR_%s_MX, OP_SPR_%s_MX_LEAN}" % (n, n) for n, _ in CHARS))
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
        sm = Image.new("RGB", (3 * (MW * s + 16) + 16, 4 * (MH * s + 16) + 16), (30, 30, 34))
        for r, ims in enumerate(small):
            pal = shades(TRACK_RGB[r])
            for c, a in enumerate(ims):
                rgb = Image.new("RGB", (a.shape[1], a.shape[0]))
                rgb.putdata([pal[v] for v in a.flatten().tolist()])
                k = s if c < 2 else 2 * s
                sm.paste(rgb.resize((a.shape[1] * k, a.shape[0] * k), Image.NEAREST), (16 + c * (MW * s + 16), 16 + r * (MH * s + 16)))
        sm.save(sys.argv[1].replace(".png", "-small.png"))


if __name__ == "__main__":
    main()
