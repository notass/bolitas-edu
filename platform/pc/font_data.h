/* Datos de la fuente (generados por tools/make_font.py) */
#ifndef FONT_DATA_H
#define FONT_DATA_H

#define FONT_SIZE_COUNT 7

typedef struct {
    unsigned short code;        /* punto de código Unicode */
    unsigned short x, y, w, h;  /* rectángulo en el atlas */
    short xoff, yoff;           /* desplazamiento desde el origen de la línea base */
    unsigned short adv16;       /* avance horizontal en 1/16 de píxel */
} FontGlyph;

typedef struct {
    int em;                     /* tamaño de la fuente en píxeles */
    int atlas_w, atlas_h;
    int ascent;                 /* de la línea base al borde superior */
    const FontGlyph *glyphs;
    int nglyphs;
    const unsigned char *rle;   /* atlas de 8 bits (alfa), comprimido: 0 n = n ceros */
    int rle_len;
} FontSize;

extern const FontSize FONT_SIZES[FONT_SIZE_COUNT];

#endif
