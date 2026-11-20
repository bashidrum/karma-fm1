#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""KARMA's boot splash (2026-10-26): the KARMA wordmark in pixels, a letter a track colour (1 blue, 2 green,
3 silver, DRUM orange, and the characters' lavender), the four tracks' bars under it; the same letters as the
website's (web/landing). The SLOOP logo below stays for --assets (its history):
The SLOOP logo: a four-colour sail (the four tracks: blue, green, yellow, orange) on the
mast of a 270-degree dial (the knobs of the UI), a hull, and a geometric monoline wordmark.

  tools/gen_logo.py OUT.h          firmware boot splash (RLE, 16-colour palette, RGB565)
  tools/gen_logo.py --assets DIR   sloop-logo.svg, sloop-logo.png, sloop-icon.svg/png, sloop-splash.png
"""
import math
import sys
from pathlib import Path

from PIL import Image, ImageDraw

BLUE, GREEN, YELLOW, ORANGE = (40, 124, 255), (30, 204, 112), (255, 198, 24), (255, 98, 26)
WHITE, BLACK = (242, 242, 242), (0, 0, 0)
HEX = lambda c: "#%02X%02X%02X" % c
SAIL = [BLUE, GREEN, YELLOW, ORANGE]

# ---- geometry (units: the icon is 240 x 240, the wordmark x-height is 72) -------------------
ICON = dict(r=106, w=12, mast_dx=-30, top=-80, bot=38, sail_w=96, gap=6, hull=64)
XH, SW = 72, 12                                  # wordmark x-height, stroke


def sail_bands(cx, cy, k):
    g = ICON
    mx, top, bot = cx + g["mast_dx"] * k, cy + g["top"] * k, cy + g["bot"] * k
    sx, h, wmax, gap = mx + 11 * k, bot - top, g["sail_w"] * k, g["gap"] * k
    bh = (h - 3 * gap) / 4
    out = []
    for i, c in enumerate(SAIL):
        y0 = top + i * (bh + gap)
        y1 = y0 + bh
        out.append((c, [(sx, y0), (sx + wmax * (y0 - top) / h, y0), (sx + wmax * (y1 - top) / h, y1), (sx, y1)]))
    return mx, top, bot, out


def rrect(d, box, r, fill):
    """rounded rectangle (ImageDraw.rounded_rectangle needs Pillow 8.2)"""
    x0, y0, x1, y1 = box
    r = min(r, (x1 - x0) / 2, (y1 - y0) / 2)
    d.rectangle([x0 + r, y0, x1 - r, y1], fill=fill)
    d.rectangle([x0, y0 + r, x1, y1 - r], fill=fill)
    for cx, cy in ((x0 + r, y0 + r), (x1 - r, y0 + r), (x0 + r, y1 - r), (x1 - r, y1 - r)):
        d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=fill)


def draw_icon(d, ox, oy, k):
    g = ICON
    cx, cy, r, w = ox + 120 * k, oy + 120 * k, g["r"] * k, g["w"] * k
    d.arc([cx - r, cy - r, cx + r, cy + r], start=135, end=405, fill=WHITE, width=max(1, round(w)))
    for a in (135, 405):
        x = cx + (r - w / 2) * math.cos(math.radians(a))
        y = cy + (r - w / 2) * math.sin(math.radians(a))
        d.ellipse([x - w / 2, y - w / 2, x + w / 2, y + w / 2], fill=WHITE)
    mx, top, bot, bands = sail_bands(cx, cy, k)
    rrect(d, [mx - 5 * k, top - 4 * k, mx + 5 * k, bot + 14 * k], 5 * k, WHITE)
    for c, poly in bands:
        d.polygon(poly, fill=c)
    rrect(d, [cx - g["hull"] * k, bot + 24 * k, cx + g["hull"] * k, bot + 36 * k], 6 * k, WHITE)


def word_width(k):
    rx = 0.29 * XH
    return (2 * rx + 20 + SW + 20 + 3 * XH + 2 * 18) * k


def draw_word(d, ox, oy, k, col=WHITE):
    w, xh = SW * k, XH * k

    def cap(x, y):
        d.ellipse([x - w / 2, y - w / 2, x + w / 2, y + w / 2], fill=col)

    def line(x0, y0, x1, y1):
        d.line([(x0, y0), (x1, y1)], fill=col, width=max(1, round(w)))
        cap(x0, y0)
        cap(x1, y1)

    base, x = oy + xh, ox
    ry, rx = (xh + w) / 4, 0.29 * xh
    b1, b2 = [x, oy, x + 2 * rx, oy + 2 * ry], [x, base - 2 * ry, x + 2 * rx, base]
    d.arc(b1, start=90, end=335, fill=col, width=max(1, round(w)))
    d.arc(b2, start=270, end=515, fill=col, width=max(1, round(w)))
    for b, a in ((b1, 335), (b2, 155)):
        cx, cy, a = (b[0] + b[2]) / 2, (b[1] + b[3]) / 2, math.radians(a)
        cap(cx + (rx - w / 2) * math.cos(a), cy + (ry - w / 2) * math.sin(a))
    x += 2 * rx + 20 * k
    line(x + w / 2, oy - 32 * k, x + w / 2, base - w / 2)
    x += w + 20 * k
    R = xh / 2
    for _ in range(3):
        d.ellipse([x, oy, x + 2 * R, oy + 2 * R], outline=col, width=max(1, round(w)))
        if _ < 2:
            x += 2 * R + 18 * k
    line(x + w / 2, oy + R, x + w / 2, base + 36 * k)


def render(kind, W, H, k, bg=BLACK, ss=4):
    im = Image.new("RGB", (W * ss, H * ss), bg)
    d = ImageDraw.Draw(im)
    K = k * ss
    if kind == "h":                                   # icon + wordmark side by side
        draw_icon(d, 10 * K, (H * ss - 240 * K) / 2, K)
        draw_word(d, 270 * K, H * ss / 2 - 36 * K, K)
    elif kind == "v":                                 # icon over the wordmark (the boot splash)
        draw_icon(d, (W * ss - 240 * K) / 2, 0, K)
        draw_word(d, (W * ss - word_width(K)) / 2, 262 * K, K)
    else:
        draw_icon(d, (W * ss - 240 * K) / 2, (H * ss - 240 * K) / 2, K)
    return im.resize((W, H), Image.LANCZOS)



# ---- KARMA (the boot splash) ------------------------------------------------------------------
KARMA_CELLS = {'w': [(0, 0), (1, 0), (2, 0), (3, 0), (4, 0), (0, 1), (1, 1), (4, 1), (5, 1), (0, 2), (1, 2), (4, 2), (5, 2), (0, 3), (1, 3), (2, 3), (3, 3), (4, 3), (0, 4), (1, 4), (0, 5), (1, 5), (0, 6), (1, 6), (7, 0), (8, 0), (11, 0), (12, 0), (7, 1), (8, 1), (11, 1), (12, 1), (7, 2), (8, 2), (11, 2), (12, 2), (7, 3), (8, 3), (9, 3), (10, 3), (11, 3), (12, 3), (7, 4), (8, 4), (11, 4), (12, 4), (7, 5), (8, 5), (11, 5), (12, 5), (7, 6), (8, 6), (11, 6), (12, 6), (14, 0), (15, 0), (16, 0), (17, 0), (18, 0), (19, 0), (14, 1), (15, 1), (14, 2), (15, 2), (14, 3), (15, 3), (16, 3), (17, 3), (18, 3), (14, 4), (15, 4), (14, 5), (15, 5), (14, 6), (15, 6), (16, 6), (17, 6), (18, 6), (19, 6), (21, 0), (22, 0), (23, 0), (24, 0), (25, 0), (21, 1), (22, 1), (25, 1), (26, 1), (21, 2), (22, 2), (25, 2), (26, 2), (21, 3), (22, 3), (23, 3), (24, 3), (25, 3), (21, 4), (22, 4), (25, 4), (26, 4), (21, 5), (22, 5), (25, 5), (26, 5), (21, 6), (22, 6), (23, 6), (24, 6), (25, 6), (28, 0), (29, 0), (32, 0), (33, 0), (28, 1), (29, 1), (32, 1), (33, 1), (28, 2), (29, 2), (32, 2), (33, 2), (28, 3), (29, 3), (32, 3), (33, 3), (28, 4), (29, 4), (32, 4), (33, 4), (28, 5), (29, 5), (32, 5), (33, 5), (29, 6), (30, 6), (31, 6), (32, 6), (36, 0), (37, 0), (38, 0), (39, 0), (40, 0), (35, 1), (36, 1), (35, 2), (36, 2), (36, 3), (37, 3), (38, 3), (39, 3), (39, 4), (40, 4), (39, 5), (40, 5), (35, 6), (36, 6), (37, 6), (38, 6), (39, 6)]}   # PHEBUS, bold 6x7 letters (Ben, 20 Nov 2026)
KARMA_COL = {"k1": (140, 118, 240), "k2": (40, 160, 245), "k3": (228, 228, 236), "k4": (20, 196, 170),
             "k5": (255, 128, 108)}                   # Ben's four track colours, the R white (ui_studio.c TE_COL)
KARMA_COL["w"] = (255, 255, 255)
KARMA_BARS = ("k1", "k2", "k4", "k5")                 # the four tracks, in order


def render_karma(W, H):
    im = Image.new("RGB", (W, H), BLACK)
    d = ImageDraw.Draw(im)
    cw, ch = 5, 8                                     # a cell 5 x 8 px, solid: an arcade wordmark, plain white
    x0, y0 = (W - 41 * cw) // 2, 58
    for g, pts in KARMA_CELLS.items():
        for x, y in pts:
            d.rectangle([x0 + x * cw, y0 + y * ch, x0 + x * cw + cw - 1, y0 + y * ch + ch - 1], fill=KARMA_COL[g])
    bw, gap, y = 44, 6, y0 + 7 * ch + 18               # the four voices: a bar each
    bx = (W - (4 * bw + 3 * gap)) // 2
    for i, g in enumerate(KARMA_BARS):
        d.rectangle([bx + i * (bw + gap), y, bx + i * (bw + gap) + bw - 1, y + 5], fill=KARMA_COL[g])
    return im

# ---- SVG (stroke centred on the path) ---------------------------------------------------------
def svg_icon(ox, oy):
    g, k = ICON, 1.0
    cx, cy, w = ox + 120, oy + 120, g["w"]
    r = g["r"] - w / 2
    a0, a1 = math.radians(135), math.radians(405)
    p0 = (cx + r * math.cos(a0), cy + r * math.sin(a0))
    p1 = (cx + r * math.cos(a1), cy + r * math.sin(a1))
    s = [f'<path d="M{p0[0]:.2f} {p0[1]:.2f} A{r:.2f} {r:.2f} 0 1 1 {p1[0]:.2f} {p1[1]:.2f}" fill="none" '
         f'stroke="{HEX(WHITE)}" stroke-width="{w}" stroke-linecap="round"/>']
    mx, top, bot, bands = sail_bands(cx, cy, k)
    s.append(f'<rect x="{mx - 5:.2f}" y="{top - 4:.2f}" width="10" height="{bot + 14 - top + 4:.2f}" rx="5" fill="{HEX(WHITE)}"/>')
    for c, poly in bands:
        s.append('<polygon points="' + " ".join(f"{x:.2f},{y:.2f}" for x, y in poly) + f'" fill="{HEX(c)}"/>')
    s.append(f'<rect x="{cx - g["hull"]:.2f}" y="{bot + 24:.2f}" width="{2 * g["hull"]}" height="12" rx="6" fill="{HEX(WHITE)}"/>')
    return s


def svg_word(ox, oy):
    w, xh, col = SW, XH, HEX(WHITE)
    base, x = oy + xh, ox
    ry, rx = (xh + w) / 4 - w / 2, 0.29 * xh - w / 2
    c1 = (x + 0.29 * xh, oy + (xh + w) / 4)
    c2 = (x + 0.29 * xh, base - (xh + w) / 4)

    def pt(c, a):
        a = math.radians(a)
        return c[0] + rx * math.cos(a), c[1] + ry * math.sin(a)
    s = []
    # top bowl 90 -> 335 (clockwise on screen), bottom bowl 270 -> 515
    for c, a, b in ((c1, 90, 335), (c2, 270, 515)):
        p, q = pt(c, a), pt(c, b)
        large = 1 if (b - a) > 180 else 0
        s.append(f'<path d="M{p[0]:.2f} {p[1]:.2f} A{rx:.2f} {ry:.2f} 0 {large} 1 {q[0]:.2f} {q[1]:.2f}" fill="none" '
                 f'stroke="{col}" stroke-width="{w}" stroke-linecap="round"/>')
    x += 2 * 0.29 * xh + 20
    s.append(f'<line x1="{x + w / 2:.2f}" y1="{oy - 32 + w / 2:.2f}" x2="{x + w / 2:.2f}" y2="{base - w / 2:.2f}" '
             f'stroke="{col}" stroke-width="{w}" stroke-linecap="round"/>')
    x += w + 20
    R = xh / 2
    for i in range(3):
        s.append(f'<circle cx="{x + R:.2f}" cy="{oy + R:.2f}" r="{R - w / 2:.2f}" fill="none" stroke="{col}" stroke-width="{w}"/>')
        if i < 2:
            x += 2 * R + 18
    s.append(f'<line x1="{x + w / 2:.2f}" y1="{oy + R:.2f}" x2="{x + w / 2:.2f}" y2="{base + 36 - w / 2:.2f}" '
             f'stroke="{col}" stroke-width="{w}" stroke-linecap="round"/>')
    return s


def svg(kind):
    if kind == "h":
        W, H = 760, 260
        body = svg_icon(10, 10) + svg_word(270, H / 2 - 36)
    else:
        W, H = 240, 240
        body = svg_icon(0, 0)
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" width="{W}" height="{H}" role="img" '
            f'aria-label="SLOOP">\n<rect width="{W}" height="{H}" fill="#000"/>\n' + "\n".join(body) + "\n</svg>\n")


# ---- firmware splash ---------------------------------------------------------------------------
SPLASH_W, SPLASH_H = 240, 188
# the scale: the icon (240), the gap (22), the wordmark's x-height (72), the p's descender (36) and its round
# end (6): 376 units in SPLASH_H rows. At 0.56 (before 2.4) the p's tail ran past row 188 and was cut
SPLASH_K = 0.5
assert (262 + 72 + 36 + 6) * SPLASH_K <= SPLASH_H, "the wordmark's p must fit the splash"


def rgb565(c):
    r, g, b = c
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def splash_header(path):
    im = render_karma(SPLASH_W, SPLASH_H)
    # a fixed palette: black, and each colour at 1/3, 2/3 and full (pixel art: only the full ones used)
    cols = [BLACK]
    for c in KARMA_COL.values():
        cols += [tuple(round(v * f) for v in c) for f in (1 / 3, 2 / 3, 1.0)]
    src = im.tobytes()
    px = []
    cache = {}
    for i in range(0, len(src), 3):
        p = (src[i], src[i + 1], src[i + 2])
        if p not in cache:
            cache[p] = min(range(16), key=lambda k: sum((p[j] - cols[k][j]) ** 2 for j in range(3)))
        px.append(cache[p])
    rle = bytearray()                                 # (run - 1) << 4 | index, runs of 1..16
    i = 0
    while i < len(px):
        j = i
        while j < len(px) and px[j] == px[i] and j - i < 16:
            j += 1
        rle.append(((j - i - 1) << 4) | px[i])
        i = j
    L = ["/* generated by tools/gen_logo.py: the KARMA boot splash */", "#pragma once", "#include <stdint.h>",
         f"#define SLOOP_SPLASH_W {SPLASH_W}", f"#define SLOOP_SPLASH_H {SPLASH_H}",
         "static const uint16_t SLOOP_SPLASH_PAL[16] = {" + ", ".join(f"0x{rgb565(c):04X}" for c in cols) + "};",
         f"static const uint8_t SLOOP_SPLASH_RLE[{len(rle)}] = {{"]
    for k in range(0, len(rle), 24):
        L.append("    " + ", ".join(str(b) for b in rle[k:k + 24]) + ",")
    L.append("};")
    Path(path).write_text("\n".join(L) + "\n")
    print(f"logo: splash {SPLASH_W}x{SPLASH_H}, {len(rle)} B RLE -> {path}")


def assets(out):
    out = Path(out)
    out.mkdir(parents=True, exist_ok=True)
    (out / "sloop-logo.svg").write_text(svg("h"))
    (out / "sloop-icon.svg").write_text(svg("i"))
    render("h", 1520, 520, 2.0).save(out / "sloop-logo.png")
    render("i", 512, 512, 512 / 240).save(out / "sloop-icon.png")
    render("v", SPLASH_W, SPLASH_H, SPLASH_K).resize((SPLASH_W * 2, SPLASH_H * 2), Image.NEAREST).save(out / "sloop-splash.png")
    print(f"logo: assets in {out}")


if __name__ == "__main__":
    if len(sys.argv) == 3 and sys.argv[1] == "--assets":
        assets(sys.argv[2])
    elif len(sys.argv) == 2:
        splash_header(sys.argv[1])
    else:
        sys.exit(__doc__)
