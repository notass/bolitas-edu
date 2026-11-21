#!/usr/bin/env python3
"""Genera las imágenes de LiveArea de la Vita (icono, fondo, pantalla de arranque).
Uso: python3 tools/make_vita_assets.py   (necesita Pillow)"""
import os
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "platform", "vita", "sce_sys")
FONT = os.path.join(HERE, "Outfit-Bold.ttf")
COLORS = [(0xF0, 0x62, 0x92), (0xFF, 0xD5, 0x4F), (0x4F, 0xC3, 0xF7), (0x81, 0xC7, 0x84), (0xBA, 0x68, 0xC8)]
S = 4  # supersampling para bordes suaves


def sky(w, h):
    im = Image.new("RGB", (w, h))
    px = im.load()
    for y in range(h):
        t = y / h
        c = (int(0x90 + 0x50 * t), int(0xD0 + 0x28 * t), int(0xF5 + 0x0A * t))
        for x in range(w):
            px[x, y] = c
    return im


def blob(draw, cx, cy, r, col):
    dark = tuple(max(0, v - 85) for v in col)
    shade = tuple(max(0, v - 28) for v in col)
    light = tuple(min(255, v + 70) for v in col)
    draw.ellipse((cx - r - S, cy - r, cx + r + S, cy + r + 2 * S), fill=dark)
    draw.ellipse((cx - r, cy - r, cx + r, cy + r), fill=shade)
    draw.ellipse((cx - r + 2 * S, cy - r + 2 * S, cx + r - 2 * S, cy + r - 3 * S), fill=col)
    draw.ellipse((cx - r * 0.55, cy - r * 0.62, cx - r * 0.1, cy - r * 0.32), fill=light)
    er = r * 0.2
    for dx in (-0.33, 0.33):
        ex, ey = cx + dx * r, cy - r * 0.15
        draw.ellipse((ex - er - S, ey - er - S, ex + er + S, ey + er + S), fill=(0x2B, 0x2B, 0x2B))
        draw.ellipse((ex - er, ey - er, ex + er, ey + er), fill=(255, 255, 255))
        draw.ellipse((ex - er * .45, ey - er * .3, ex + er * .45, ey + er * .6), fill=(0x22, 0x22, 0x22))
    draw.arc((cx - r * .35, cy + r * .05, cx + r * .35, cy + r * .5), 20, 160, fill=(0x22, 0x22, 0x22), width=2 * S)


def scene(w, h, title=True):
    big = sky(w * S, h * S)
    d = ImageDraw.Draw(big)
    d.ellipse((-w * S * .1, h * S * .82, w * S * .6, h * S * 1.3), fill=(0x8B, 0xC3, 0x4A))
    d.ellipse((w * S * .4, h * S * .86, w * S * 1.1, h * S * 1.35), fill=(0x8B, 0xC3, 0x4A))
    n = 5
    for i, col in enumerate(COLORS[:n]):
        r = int(h * S * (0.17 if i == 1 else 0.12))
        cx = int(w * S * (0.18 + 0.16 * i))
        blob(d, cx, int(h * S * (0.78 - (0.06 if i == 1 else 0))), r, col)
    if title:
        f = ImageFont.truetype(FONT, int(h * S * 0.2))
        txt = "BOLITAS EDU"
        tw = d.textlength(txt, font=f)
        x, y = (w * S - tw) / 2, h * S * 0.08
        for ox, oy in ((-3, 0), (3, 0), (0, -3), (0, 3), (-3, -3), (3, 3), (-3, 3), (3, -3)):
            d.text((x + ox * S / 2, y + oy * S / 2), txt, font=f, fill=(255, 255, 255))
        d.text((x, y), txt, font=f, fill=(0x26, 0x32, 0x38))
    return big.resize((w, h), Image.LANCZOS)


def save_indexed(im, path):
    """La Vita exige PNG de 8 bits con paleta."""
    im.convert("RGB").quantize(colors=128, method=Image.MEDIANCUT, dither=Image.FLOYDSTEINBERG).save(path, optimize=True)


def main():
    os.makedirs(os.path.join(OUT, "livearea", "contents"), exist_ok=True)
    # icono 128x128
    big = sky(128 * S, 128 * S)
    d = ImageDraw.Draw(big)
    d.ellipse((-20 * S, 98 * S, 150 * S, 170 * S), fill=(0x8B, 0xC3, 0x4A))
    blob(d, 64 * S, 70 * S, 40 * S, COLORS[1])
    icon = big.resize((128, 128), Image.LANCZOS)
    save_indexed(icon, os.path.join(OUT, "icon0.png"))
    save_indexed(scene(840, 500), os.path.join(OUT, "livearea", "contents", "bg0.png"))
    save_indexed(scene(280, 158, title=False), os.path.join(OUT, "livearea", "contents", "startup.png"))
    print("ok")

if __name__ == "__main__":
    main()
