# Draws the metal frame of the trade screen: worn steel plates with seams,
# rivets, scratches and chipped edges. Our own procedural drawing (the owner,
# 2026-11-11: frames of slightly battered metal plates, as in the trade screen
# of STALKER 2; nothing of that game's art is used).
#
#   python -I tools/plates.py ui/textures --paa <ImageToPAA.exe> OpenZone_Trader/gui/textures
#
# Writes the pictures into the first folder (the sources, not packed) and,
# with --paa, each converted into the folder after the converter's path:
#   ozt_plate_h_ca.png  1024x16  a long horizontal edge
#   ozt_plate_hs_ca.png  256x16  a short horizontal edge
#   ozt_plate_v_ca.png  16x1024  a long vertical edge
#   ozt_plate_vs_ca.png  16x256  a short vertical edge
#   ozt_plate_bolt_ca.png 16x16  a corner plate with a bolt
# The suffix _ca keeps the alpha channel in the conversion.
import os
import random
import subprocess
import sys

from PIL import Image, ImageDraw

BASE = (64, 62, 58)


def clamp(v):
    return max(0, min(255, int(v)))


def shade(c, d):
    return (clamp(c[0] + d), clamp(c[1] + d), clamp(c[2] + d))


def strip(length, seed):
    """A horizontal strip `length` x 16, the outer edge at the top."""
    rnd = random.Random(seed)
    h = 16
    img = Image.new("RGBA", (length, h), BASE + (255,))
    px = img.load()

    # Plates of their own shade, seams between them.
    seams = []
    x = 0
    while x < length:
        span = rnd.randint(150, 270)
        tone = rnd.randint(-7, 7)
        for xx in range(x, min(length, x + span)):
            for y in range(h):
                px[xx, y] = shade(BASE, tone) + (255,)
        x += span
        if x < length - 20:
            seams.append(x)

    # Brushed grain along the strip, and noise.
    for y in range(h):
        x = 0
        while x < length:
            run = rnd.randint(12, 90)
            d = rnd.randint(-5, 5)
            for xx in range(x, min(length, x + run)):
                r, g, b, a = px[xx, y]
                n = rnd.randint(-4, 4)
                px[xx, y] = shade((r, g, b), d + n) + (a,)
            x += run

    # The bevel across: light at the outer edge, shadow at the inner.
    bevel = {0: 58, 1: 32, 2: 12, 13: -12, 14: -28, 15: -44}
    for y, d in bevel.items():
        for xx in range(length):
            r, g, b, a = px[xx, y]
            px[xx, y] = shade((r, g, b), d) + (a,)

    draw = ImageDraw.Draw(img)
    # Seams with rivets beside them.
    for s in seams:
        for y in range(1, h - 1):
            r, g, b, a = px[s, y]
            px[s, y] = shade((r, g, b), -40) + (a,)
            if s + 1 < length:
                r, g, b, a = px[s + 1, y]
                px[s + 1, y] = shade((r, g, b), 18) + (a,)
        for rx in (s - 7, s + 8):
            if 3 <= rx < length - 3:
                rivet(px, rx, 8)

    # Scratches: thin light lines at shallow angles.
    for _ in range(length // 30):
        x0 = rnd.randint(0, length - 1)
        y0 = rnd.randint(3, h - 4)
        ln = rnd.randint(8, 55)
        dy = rnd.choice((-1, 0, 0, 1))
        x1 = min(length - 1, x0 + ln)
        y1 = max(2, min(h - 3, y0 + dy))
        lift = rnd.randint(14, 34)
        for t in range(ln + 1):
            xx = x0 + t
            if xx >= length:
                break
            yy = round(y0 + (y1 - y0) * t / max(1, ln))
            r, g, b, a = px[xx, yy]
            px[xx, yy] = shade((r, g, b), lift) + (a,)

    # Chips at the edges: bright bare metal, a dark rim beneath.
    for _ in range(length // 45):
        cx = rnd.randint(2, length - 6)
        w = rnd.randint(2, 6)
        top = rnd.random() < 0.6
        for xx in range(cx, min(length, cx + w)):
            depth = rnd.randint(1, 3)
            for k in range(depth):
                y = k if top else h - 1 - k
                r, g, b, a = px[xx, y]
                px[xx, y] = shade((r, g, b), 52 - k * 10) + (a,)
            y = depth if top else h - 1 - depth
            r, g, b, a = px[xx, y]
            px[xx, y] = shade((r, g, b), -22) + (a,)

    # A little grime and rust.
    for _ in range(length // 60):
        cx = rnd.randint(0, length - 1)
        cy = rnd.randint(2, h - 3)
        rust = rnd.random() < 0.4
        for xx in range(cx, min(length, cx + rnd.randint(1, 4))):
            r, g, b, a = px[xx, cy]
            if rust:
                px[xx, cy] = (clamp(r * 0.7 + 92 * 0.3), clamp(g * 0.7 + 58 * 0.3), clamp(b * 0.7 + 36 * 0.3), a)
            else:
                px[xx, cy] = shade((r, g, b), -18) + (a,)
    del draw
    return img


def rivet(px, cx, cy):
    """A rivet of radius 2.5 at (cx, cy): dark rim, light top left."""
    for dy in range(-3, 4):
        for dx in range(-3, 4):
            d2 = dx * dx + dy * dy
            x = cx + dx
            y = cy + dy
            if d2 > 9:
                continue
            r, g, b, a = px[x, y]
            if d2 >= 6:
                px[x, y] = shade((r, g, b), -30) + (a,)
            elif dx + dy < 0:
                px[x, y] = shade((r, g, b), 40) + (a,)
            else:
                px[x, y] = shade((r, g, b), 6) + (a,)


def bolt(seed):
    """A small corner plate with a bolt, on a clear ground."""
    rnd = random.Random(seed)
    img = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
    px = img.load()
    for y in range(1, 15):
        for x in range(1, 15):
            d = rnd.randint(-5, 5)
            if x == 1 or y == 1:
                d += 34
            if x == 14 or y == 14:
                d -= 30
            px[x, y] = shade(BASE, 6 + d) + (255,)
    rivet(px, 8, 8)
    for x in range(6, 11):
        r, g, b, a = px[x, 8]
        px[x, 8] = shade((r, g, b), -34) + (a,)
    return img


def main():
    out = sys.argv[1]
    paa = None
    paa_out = out
    if "--paa" in sys.argv:
        at = sys.argv.index("--paa")
        paa = sys.argv[at + 1]
        if len(sys.argv) > at + 2:
            paa_out = sys.argv[at + 2]
    os.makedirs(out, exist_ok=True)
    os.makedirs(paa_out, exist_ok=True)
    made = {
        "ozt_plate_h_ca": strip(1024, 11),
        "ozt_plate_hs_ca": strip(256, 12),
        "ozt_plate_v_ca": strip(1024, 13).transpose(Image.Transpose.ROTATE_90),
        "ozt_plate_vs_ca": strip(256, 14).transpose(Image.Transpose.ROTATE_90),
        "ozt_plate_bolt_ca": bolt(15),
    }
    for name, img in made.items():
        png = os.path.join(out, name + ".png")
        img.save(png)
        if paa:
            target = os.path.join(paa_out, name + ".paa")
            if os.path.exists(target):
                os.remove(target)
            done = subprocess.run([paa, png, target], capture_output=True, text=True, stdin=subprocess.DEVNULL)
            if done.returncode != 0 or not os.path.isfile(target):
                raise SystemExit("ImageToPAA failed for " + name + ": " + done.stdout + done.stderr)
        print(name, img.size)


if __name__ == "__main__":
    main()
