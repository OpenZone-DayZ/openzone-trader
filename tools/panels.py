# Draws the trade screen's panels: a dark plate for each big panel, framed in
# worn steel whose outline is not a true rectangle (chamfered corners, dents,
# bites, flakes of metal lifting off its edge), unevenly battered, with a
# plate for the title at the top. Our own procedural drawing (the owner,
# 2026-11-11: panels very much like the inventory of STALKER 2; nothing of that
# game's art is used).
#
#   python -I tools/panels.py ui/OpenZone_Trader/ozt_trade.json ui/textures \
#       --paa <ImageToPAA.exe> OpenZone_Trader/gui/textures
#
# For every panel of the description named in BIG it draws ozt_panel_<name>_ca.png into the second folder, PAD units bigger than
# the panel on every side, and converts it into the folder after the
# converter's path. The art is drawn at SCALE pixels a unit and resized to a
# texture with sides of powers of two; the widget stretches it back.
import io
import json
import math
import os
import random
import subprocess
import sys

from PIL import Image, ImageDraw, ImageFilter

SCALE = 3          # pixels a unit while drawing
PAD = 3            # units of art beyond the panel's edge, for what sticks out
BAND = 10          # the frame's width across, units

# The panels that wear art, by name in the description.
BIG = ["BuyPanel", "ShelfPanel", "CharPanel", "DealPanel", "SellPanel", "OwnPanel"]



def clamp(v):
    return max(0, min(255, int(v)))


def pow2(n):
    p = 1
    while p < n:
        p *= 2
    return p


def wobble(rnd, a, b, step_lo, step_hi, deep):
    """Points of an edge from a to b (unit space, the outward normal on the
    left of travel), bitten inwards here and there."""
    (x0, y0), (x1, y1) = a, b
    length = math.hypot(x1 - x0, y1 - y0)
    ux, uy = (x1 - x0) / length, (y1 - y0) / length
    nx, ny = uy, -ux          # left of travel: outward for a clockwise path
    pts = []
    t = 0.0
    while t < length:
        d = 0.0
        r = rnd.random()
        if r < 0.10:
            d = rnd.uniform(1.0, deep)          # a bite
        elif r < 0.45:
            d = rnd.uniform(0.15, 0.8)          # a dent
        pts.append((x0 + ux * t - nx * d, y0 + uy * t - ny * d))
        t += rnd.uniform(step_lo, step_hi)
    return pts


def outline(rnd, x, y, w, h):
    """A clockwise outline of a plate at (x, y, w, h): chamfered corners of
    their own size, wobbling edges."""
    c = [rnd.uniform(2.5, 7) for _ in range(4)]
    corners = [
        ((x + c[0], y), (x + w - c[1], y)),
        ((x + w, y + c[1]), (x + w, y + h - c[2])),
        ((x + w - c[2], y + h), (x + c[3], y + h)),
        ((x, y + h - c[3]), (x, y + c[0])),
    ]
    pts = []
    for a, b in corners:
        pts += wobble(rnd, a, b, 6, 30, 2.2)
        pts.append(b)
    return pts


def to_px(pts):
    return [(px * SCALE, py * SCALE) for px, py in pts]


def bevel(d, pts, outer_edge, light, dark, width=2):
    """Light where an edge faces up or left, shadow where it faces down or
    right; an inner edge the other way round (a raised plate)."""
    n = len(pts)
    for i in range(n):
        (x0, y0), (x1, y1) = pts[i], pts[(i + 1) % n]
        dx, dy = x1 - x0, y1 - y0
        ln = math.hypot(dx, dy) or 1
        nx, ny = dy / ln, -dx / ln            # outward for a clockwise path
        lit = (-nx - ny) / 1.4142             # facing up-left
        if not outer_edge:
            lit = -lit
        if lit > 0.15:
            col = light[:3] + (int(light[3] * lit),)
        elif lit < -0.15:
            col = dark[:3] + (int(dark[3] * -lit),)
        else:
            continue
        d.line([(x0 * SCALE, y0 * SCALE), (x1 * SCALE, y1 * SCALE)], fill=col, width=width)


def rivet(d, cx, cy, r=1.3):
    d.ellipse([(cx - r - 0.4) * SCALE, (cy - r - 0.4) * SCALE, (cx + r + 0.4) * SCALE, (cy + r + 0.4) * SCALE], fill=(4, 3, 2, 230))
    d.ellipse([(cx - r) * SCALE, (cy - r) * SCALE, (cx + r) * SCALE, (cy + r) * SCALE], fill=(34, 31, 28, 255))
    d.ellipse([(cx - r * 0.8) * SCALE, (cy - r * 0.8) * SCALE, (cx + r * 0.05) * SCALE, (cy + r * 0.05) * SCALE], fill=(78, 74, 68, 170))


def steel(rnd, size, base, spread, streak_alpha):
    """A layer of dark steel the size of the art: grain and brushed streaks."""
    img = Image.new("RGBA", size, base + (255,))
    noise = Image.effect_noise(size, spread * 4).convert("L")
    tint = Image.merge("RGBA", (noise, noise, noise, Image.new("L", size, 18)))
    img.alpha_composite(tint)
    if streak_alpha > 0:
        streaks = Image.new("RGBA", size, (0, 0, 0, 0))
        sd = ImageDraw.Draw(streaks)
        w, h = size
        for _ in range(int(w * h / 1400)):
            x = rnd.randint(0, w)
            y = rnd.randint(0, h)
            ln = rnd.randint(20, 160)
            tone = rnd.choice((150, 0))
            sd.line([(x, y), (x + ln, y + rnd.randint(-1, 1))], fill=(tone, tone, tone, rnd.randint(3, streak_alpha)), width=1)
        img.alpha_composite(streaks)
    return img


def patch_poly(rnd, x0, y0, x1, y1, torn):
    """A small plate laid over the frame, one corner maybe torn off."""
    pts = [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]
    if not torn:
        return pts
    k = rnd.randrange(4)
    prev = pts[k - 1]
    cur = pts[k]
    nxt = pts[(k + 1) % 4]
    a = rnd.uniform(0.2, 0.45)
    b = rnd.uniform(0.2, 0.45)
    pa = (cur[0] + (prev[0] - cur[0]) * a, cur[1] + (prev[1] - cur[1]) * a)
    pb = (cur[0] + (nxt[0] - cur[0]) * b, cur[1] + (nxt[1] - cur[1]) * b)
    jag = ((pa[0] + pb[0]) / 2 + rnd.uniform(-0.8, 0.8), (pa[1] + pb[1]) / 2 + rnd.uniform(-0.8, 0.8))
    return pts[:k] + [pa, jag, pb] + pts[k + 1:]


# Colours, sampled from a screenshot of the reference's trade screen at
# 1920x1080: the panels are nearly black, their frames barely lighter, the
# light only where the metal is worn.
FRAME = (21, 19, 17)
HEADER = (19, 17, 15)
INSIDE = (4, 3, 2)
PATCH = (27, 25, 22)
EDGE_LIGHT = (92, 88, 82, 150)
EDGE_DARK = (0, 0, 0, 200)
WORN = (74, 70, 64)
HEAD_H = 36        # the title band at the top, units
HEAD_GAP = 4       # the dark gap between the band and the frame below


def framed_box(rnd, art, x, y, w, h, band):
    """A frame of dark worn steel around a near-black inside, its outline not
    quite a rectangle. Returns the outline, for the wear drawn after."""
    W, H = art.size
    outer = outline(rnd, x, y, w, h)
    inner = outline(rnd, x + band, y + band, w - 2 * band, h - 2 * band)
    frame_mask = Image.new("L", (W, H), 0)
    ImageDraw.Draw(frame_mask).polygon(to_px(outer), fill=255)
    inside_mask = Image.new("L", (W, H), 0)
    ImageDraw.Draw(inside_mask).polygon(to_px(inner), fill=255)
    art.paste(steel(rnd, (W, H), FRAME, 4, 10), (0, 0), frame_mask)
    inside = steel(rnd, (W, H), INSIDE, 1, 0)
    inside.putalpha(242)
    art.paste(inside, (0, 0), inside_mask)
    d = ImageDraw.Draw(art, "RGBA")
    bevel(d, outer, True, EDGE_LIGHT, EDGE_DARK, 2)
    bevel(d, inner, False, (60, 57, 53, 140), (0, 0, 0, 220), 2)
    d.line(to_px(inner + inner[:1]), fill=(0, 0, 0, 170), width=SCALE)
    return outer, frame_mask


def wear(rnd, art, outer, frame_mask, x, y, w, h, band):
    """Uneven wear: dirt and rubbed stretches, plates laid over the frame, a
    torn corner here and there, chips and flakes at the edge, scratches."""
    W, H = art.size
    per = 2 * (w + h)
    dirt = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    dd = ImageDraw.Draw(dirt)
    for _ in range(int(per / 16)):
        side = rnd.randrange(4)
        t = rnd.random()
        if side == 0:
            cx, cy = x + t * w, y + rnd.uniform(0, band)
        elif side == 1:
            cx, cy = x + w - rnd.uniform(0, band), y + t * h
        elif side == 2:
            cx, cy = x + t * w, y + h - rnd.uniform(0, band)
        else:
            cx, cy = x + rnd.uniform(0, band), y + t * h
        rx = rnd.uniform(4, 26)
        ry = rnd.uniform(2, 6)
        if side in (1, 3):
            rx, ry = ry, rx
        if rnd.random() < 0.55:
            col = (0, 0, 0, rnd.randint(50, 130))
        else:
            col = (70, 66, 60, rnd.randint(14, 34))
        dd.ellipse([(cx - rx) * SCALE, (cy - ry) * SCALE, (cx + rx) * SCALE, (cy + ry) * SCALE], fill=col)
    dirt = dirt.filter(ImageFilter.GaussianBlur(radius=3 * SCALE))
    clipped = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    clipped.paste(dirt, (0, 0), frame_mask)
    art.alpha_composite(clipped)
    d = ImageDraw.Draw(art, "RGBA")

    def lay_patch(x0, y0, x1, y1):
        pts = patch_poly(rnd, x0, y0, x1, y1, rnd.random() < 0.5)
        d.polygon(to_px([(px_ + 0.6, py_ + 0.8) for px_, py_ in pts]), fill=(0, 0, 0, 160))
        tone = rnd.randint(-3, 4)
        d.polygon(to_px(pts), fill=(PATCH[0] + tone, PATCH[1] + tone, PATCH[2] + tone, 255))
        bevel(d, pts, True, (84, 80, 74, 120), (0, 0, 0, 200), 2)
        if (x1 - x0) >= (y1 - y0):
            cy = (y0 + y1) / 2
            rivet(d, x0 + 3, cy)
            rivet(d, x1 - 3, cy)
        else:
            cx = (x0 + x1) / 2
            rivet(d, cx, y0 + 3)
            rivet(d, cx, y1 - 3)

    px0 = x + rnd.uniform(30, 90)
    while px0 < x + w - 60:
        ln = rnd.uniform(24, 64)
        if rnd.random() < 0.45:
            lay_patch(px0, y - 0.5, px0 + ln, y + band + 1)
        if rnd.random() < 0.45:
            lay_patch(px0 + 12, y + h - band - 1, px0 + 12 + ln, y + h + 0.5)
        px0 += ln + rnd.uniform(60, 170)
    py0 = y + rnd.uniform(40, 90)
    while py0 < y + h - 60:
        ln = rnd.uniform(24, 64)
        if rnd.random() < 0.4:
            lay_patch(x - 0.5, py0, x + band + 1, py0 + ln)
        if rnd.random() < 0.4:
            lay_patch(x + w - band - 1, py0 + 10, x + w + 0.5, py0 + 10 + ln)
        py0 += ln + rnd.uniform(60, 180)

    # Bare steel where the edge took knocks, more on some stretches.
    hot = [rnd.random() for _ in range(9)]
    for i in range(len(outer)):
        (x0, y0), (x1, y1) = outer[i], outer[(i + 1) % len(outer)]
        if rnd.random() > 0.08 + hot[i % 9] * 0.55:
            continue
        t = rnd.random()
        cx, cy = x0 + (x1 - x0) * t, y0 + (y1 - y0) * t
        size = rnd.uniform(0.4, 1.8)
        poly = [(cx + math.cos(k * 1.05) * size * rnd.uniform(0.4, 1.1), cy + math.sin(k * 1.05) * size * rnd.uniform(0.4, 1.1)) for k in range(6)]
        tone = rnd.randint(-12, 10)
        d.polygon(to_px(poly), fill=(clamp(WORN[0] + tone), clamp(WORN[1] + tone), clamp(WORN[2] + tone), 210))

    # Flakes of metal lifting off the edge, a shadow under each.
    for _ in range(int(per / 55)):
        i = rnd.randrange(len(outer))
        (x0, y0), (x1, y1) = outer[i], outer[(i + 1) % len(outer)]
        t = rnd.random()
        fx, fy = x0 + (x1 - x0) * t, y0 + (y1 - y0) * t
        dx, dy = x1 - x0, y1 - y0
        ln = math.hypot(dx, dy) or 1
        ux, uy = dx / ln, dy / ln
        nx, ny = uy, -ux
        long_ = rnd.uniform(2, 7)
        lift = rnd.uniform(0.6, 1.6)
        flake = [(fx - ux * long_ / 2, fy - uy * long_ / 2), (fx + ux * long_ / 2, fy + uy * long_ / 2),
                 (fx + ux * long_ * 0.3 + nx * lift, fy + uy * long_ * 0.3 + ny * lift),
                 (fx - ux * long_ * 0.2 + nx * lift * 0.7, fy - uy * long_ * 0.2 + ny * lift * 0.7)]
        d.polygon(to_px([(qx + 0.5, qy + 0.6) for qx, qy in flake]), fill=(0, 0, 0, 150))
        tone = rnd.randint(-8, 10)
        d.polygon(to_px(flake), fill=(clamp(44 + tone), clamp(41 + tone), clamp(37 + tone), 240))
        d.line(to_px(flake[:2]), fill=(84, 80, 74, 160), width=1)

    # Scratches: thin light specks along the band.
    for _ in range(int(per / 12)):
        side = rnd.randrange(4)
        t = rnd.random()
        if side == 0:
            sx, sy, ang = x + t * w, y + rnd.uniform(1, band - 1), 0
        elif side == 1:
            sx, sy, ang = x + w - rnd.uniform(1, band - 1), y + t * h, math.pi / 2
        elif side == 2:
            sx, sy, ang = x + t * w, y + h - rnd.uniform(1, band - 1), 0
        else:
            sx, sy, ang = x + rnd.uniform(1, band - 1), y + t * h, math.pi / 2
        ang += rnd.uniform(-0.3, 0.3)
        ln = rnd.uniform(1.5, 12)
        d.line([(sx * SCALE, sy * SCALE), ((sx + math.cos(ang) * ln) * SCALE, (sy + math.sin(ang) * ln) * SCALE)],
               fill=(110, 106, 98, rnd.randint(40, 120)), width=1)

    mid = band / 2
    for cx, cy in ((x + 11, y + mid), (x + w - 11, y + mid), (x + 11, y + h - mid), (x + w - 11, y + h - mid)):
        rivet(d, cx, cy)


def head_band(rnd, art, x, y, w):
    """The title band across the top: dark steel, a lit top edge, worn ends,
    and a short dotted line under the middle, where the title stands."""
    W, H = art.size
    pts = outline(rnd, x + 1, y, w - 2, HEAD_H)
    mask = Image.new("L", (W, H), 0)
    ImageDraw.Draw(mask).polygon(to_px(pts), fill=255)
    art.paste(steel(rnd, (W, H), HEADER, 3, 8), (0, 0), mask)
    d = ImageDraw.Draw(art, "RGBA")
    bevel(d, pts, True, (100, 96, 90, 160), (0, 0, 0, 210), 2)
    for _ in range(int(w / 10)):
        sx = rnd.uniform(x + 2, x + w - 2)
        sy = rnd.uniform(y + 1, y + HEAD_H - 1)
        ln = rnd.uniform(1.5, 9)
        d.line([(sx * SCALE, sy * SCALE), ((sx + ln) * SCALE, (sy + rnd.uniform(-0.6, 0.6)) * SCALE)],
               fill=(100, 96, 88, rnd.randint(30, 100)), width=1)
    # The dotted line under the title.
    cx = x + w / 2
    dot_y = y + HEAD_H - 6
    k = cx - 38
    while k < cx + 38:
        d.rectangle([k * SCALE, dot_y * SCALE, (k + 1.1) * SCALE, (dot_y + 0.7) * SCALE], fill=(150, 146, 138, 200))
        k += 2.2
    for rx in (x + 8, x + w - 8):
        rivet(d, rx, y + HEAD_H / 2)


def draw_panel(name, w, h, tab, seed):
    rnd = random.Random(seed)
    W = int((w + 2 * PAD) * SCALE)
    H = int((h + 2 * PAD) * SCALE)
    art = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    top = PAD
    if tab:
        head_band(rnd, art, PAD, PAD, w)
        top = PAD + HEAD_H + HEAD_GAP
    box_h = PAD + h - top
    outer, frame_mask = framed_box(rnd, art, PAD, top, w, box_h, BAND)
    wear(rnd, art, outer, frame_mask, PAD, top, w, box_h, BAND)
    art = art.filter(ImageFilter.GaussianBlur(radius=0.35))
    tw = min(pow2(int((w + 2 * PAD) * 1.7)), 2048)
    th = min(pow2(int((h + 2 * PAD) * 1.7)), 1024)
    return art.resize((tw, th), Image.LANCZOS)


def main():
    desc, out = sys.argv[1], sys.argv[2]
    paa = None
    paa_out = out
    if "--paa" in sys.argv:
        at = sys.argv.index("--paa")
        paa = sys.argv[at + 1]
        if len(sys.argv) > at + 2:
            paa_out = sys.argv[at + 2]
    os.makedirs(out, exist_ok=True)
    os.makedirs(paa_out, exist_ok=True)
    d = json.load(io.open(desc, encoding="utf-8"))
    body = [a for c in d["root"]["frame"]["children"] for k, a in c.items() if a.get("name") == "TradeBody"][0]
    seed = 101
    for c in body["children"]:
        kind, a = next(iter(c.items()))
        if kind != "panel" or a.get("name") not in BIG or not isinstance(a.get("size"), list):
            continue
        name = a["name"].replace("Panel", "").lower()
        has_head = any(next(iter(k.values())).get("name", "").endswith("Head") for k in a.get("children", []))
        img = draw_panel(name, a["size"][0], a["size"][1], has_head, seed)
        seed += 7
        stem = "ozt_panel_" + name + "_ca"
        png = os.path.join(out, stem + ".png")
        img.save(png)
        if paa:
            target = os.path.join(paa_out, stem + ".paa")
            if os.path.exists(target):
                os.remove(target)
            done = subprocess.run([paa, png, target], capture_output=True, text=True, stdin=subprocess.DEVNULL)
            if done.returncode != 0 or not os.path.isfile(target):
                raise SystemExit("ImageToPAA failed for " + stem + ": " + done.stdout + done.stderr)
        print(stem, img.size, "tab" if has_head else "")


if __name__ == "__main__":
    main()
