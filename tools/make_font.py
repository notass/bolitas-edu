#!/usr/bin/env python3
"""Genera platform/pc/font_data.c a partir de Outfit-Bold.ttf (SIL OFL 1.1).

Rasteriza los glifos a varios tamaños, los empaqueta en un atlas de 8 bits por
tamaño y los guarda comprimidos (RLE de ceros) dentro de un .c, para que el juego
no dependa de ninguna librería de fuentes (válido en PSP, Vita, Android y PC).

Uso:  python3 tools/make_font.py          (necesita Pillow)
"""
import os
import sys
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
TTF = os.path.join(HERE, "Outfit-Bold.ttf")
# Hiragana: Noto Sans CJK JP Bold (SIL OFL 1.1). Solo se usa para generar; el resultado va dentro de font_data.c
KANA_TTC = os.environ.get("KANA_TTC", "/usr/share/fonts/opentype/noto/NotoSansCJK-Bold.ttc")
KANA_SIZES = [32, 64]
KANA_CHARS = list("あいうえおかきくけこさしすせそたちつてとなにぬねのはひふへほまみむめもやゆよらりるれろわをん") + ["ぱ", "ご"]
OUT = os.path.join(HERE, "..", "platform", "pc", "font_data.c")

SIZES = [10, 14, 20, 28, 40, 56, 80]                       # alto de la "em" en píxeles
CHARS = [chr(c) for c in range(32, 127)] + list("áéíóúüñÁÉÍÓÚÜÑ¿¡")
PAD = 1


def rle(data: bytes) -> bytes:
    """0 va seguido de la longitud de la tirada de ceros (1..255); el resto, literal."""
    out = bytearray()
    i, n = 0, len(data)
    while i < n:
        b = data[i]
        if b == 0:
            j = i
            while j < n and data[j] == 0 and j - i < 255:
                j += 1
            out += bytes((0, j - i))
            i = j
        else:
            out.append(b)
            i += 1
    return bytes(out)


def build_size(em, ttf=None, chars=None, index=0):
    ttf = ttf or TTF
    chars = chars or CHARS
    font = ImageFont.truetype(ttf, em, index=index)
    ascent, descent = font.getmetrics()
    glyphs = []
    for ch in chars:
        l, t, r, b = font.getbbox(ch, anchor="ls")
        adv = font.getlength(ch)
        if r <= l or b <= t:
            glyphs.append(dict(code=ord(ch), img=None, w=0, h=0, xoff=0, yoff=0, adv=adv))
            continue
        w, h = r - l + 2 * PAD, b - t + 2 * PAD
        im = Image.new("L", (w, h), 0)
        ImageDraw.Draw(im).text((PAD - l, PAD - t), ch, font=font, fill=255, anchor="ls")
        glyphs.append(dict(code=ord(ch), img=im, w=w, h=h, xoff=l - PAD, yoff=t - PAD, adv=adv))

    aw = 256 if em <= 20 else 512
    x = y = rowh = 0
    for g in glyphs:
        if g["img"] is None:
            g["x"] = g["y"] = 0
            continue
        if x + g["w"] + 1 > aw:
            x, y, rowh = 0, y + rowh + 1, 0
        g["x"], g["y"] = x, y
        x += g["w"] + 1
        rowh = max(rowh, g["h"])
    ah = y + rowh + 1
    atlas = Image.new("L", (aw, ah), 0)
    for g in glyphs:
        if g["img"] is not None:
            atlas.paste(g["img"], (g["x"], g["y"]))
    return dict(em=em, aw=aw, ah=ah, ascent=ascent, glyphs=glyphs, data=rle(atlas.tobytes()),
                raw=aw * ah)


def main():
    sizes = [build_size(s) for s in SIZES]
    kana = [build_size(s, KANA_TTC, KANA_CHARS) for s in KANA_SIZES]
    lines = []
    w = lines.append
    w("/* GENERADO por tools/make_font.py a partir de Outfit-Bold.ttf (SIL Open Font License 1.1,")
    w(" * ver platform/pc/fonts/Outfit-OFL.txt). No editar a mano. */")
    w('#include "font_data.h"')
    w("")
    for pre, group in (("", sizes), ("K", kana)):
      for si, s in enumerate(group):
        w(f"static const unsigned char {pre}RLE_{si}[] = {{")
        d = s["data"]
        for i in range(0, len(d), 24):
            w("  " + ",".join(str(b) for b in d[i:i + 24]) + ",")
        w("};")
        w(f"static const FontGlyph {pre}GLYPHS_{si}[] = {{")
        for g in s["glyphs"]:
            w("  {%d,%d,%d,%d,%d,%d,%d,%d}," % (g["code"], g["x"], g["y"], g["w"], g["h"],
                                              g["xoff"], g["yoff"], round(g["adv"] * 16)))
        w("};")
        w("")
    w("const FontSize FONT_SIZES[FONT_SIZE_COUNT] = {")
    for si, s in enumerate(sizes):
        w("  {%d,%d,%d,%d,GLYPHS_%d,%d,RLE_%d,%d}," % (
            s["em"], s["aw"], s["ah"], s["ascent"], si, len(s["glyphs"]), si, len(s["data"])))
    w("};")
    w("")
    w("const FontSize KANA_SIZES[KANA_SIZE_COUNT] = {")
    for si, s in enumerate(kana):
        w("  {%d,%d,%d,%d,KGLYPHS_%d,%d,KRLE_%d,%d}," % (
            s["em"], s["aw"], s["ah"], s["ascent"], si, len(s["glyphs"]), si, len(s["data"])))
    w("};")
    text = "\n".join(lines) + "\n"
    with open(OUT, "w", encoding="utf-8") as f:
        f.write(text)
    tot = sum(len(s["data"]) for s in sizes)
    print("sizes:", [(s["em"], s["aw"], s["ah"], len(s["data"])) for s in sizes])
    print("datos comprimidos: %d bytes; fichero .c: %d bytes" % (tot, len(text)))


if __name__ == "__main__":
    sys.exit(main())
