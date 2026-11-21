#include "paint.h"
#include <string.h>

#define R(x, y, w, h)               { SH_RECT, { x, y, w, h, 0, 0 } }
#define E(cx, cy, rx, ry)           { SH_ELLIPSE, { cx, cy, rx, ry, 0, 0 } }
#define T(x1, y1, x2, y2, x3, y3)   { SH_TRI, { x1, y1, x2, y2, x3, y3 } }

static const Drawing DRAWINGS[PAINT_DRAWINGS] = {
    { { "La casa", "The house", "Etxea" }, 9, {
        R(0, 0, 300, 200),            /* 0 cielo */
        E(250, 40, 22, 22),           /* 1 sol */
        E(70, 38, 32, 14),            /* 2 nube */
        R(0, 150, 300, 50),           /* 3 césped */
        R(172, 48, 16, 30),           /* 4 chimenea */
        R(70, 90, 130, 70),           /* 5 pared */
        T(55, 92, 135, 40, 215, 92),  /* 6 tejado */
        R(120, 115, 30, 45),          /* 7 puerta */
        R(165, 105, 25, 25),          /* 8 ventana */
    } },
    { { "La flor", "The flower", "Lorea" }, 12, {
        R(0, 0, 300, 200),            /* 0 cielo */
        E(45, 40, 22, 22),            /* 1 sol */
        R(0, 160, 300, 40),           /* 2 suelo */
        R(144, 95, 12, 70),           /* 3 tallo */
        E(122, 138, 24, 10),          /* 4 hoja izquierda */
        E(180, 126, 24, 10),          /* 5 hoja derecha */
        E(150, 38, 22, 22),           /* 6 pétalo */
        E(181, 61, 22, 22),           /* 7 pétalo */
        E(169, 98, 22, 22),           /* 8 pétalo */
        E(131, 98, 22, 22),           /* 9 pétalo */
        E(119, 61, 22, 22),           /* 10 pétalo */
        E(150, 70, 16, 16),           /* 11 centro */
    } },
    { { "El cohete", "The rocket", "Suziria" }, 11, {
        R(0, 0, 300, 200),            /* 0 espacio */
        E(240, 50, 28, 28),           /* 1 luna */
        E(55, 45, 7, 7),              /* 2 estrella */
        E(85, 140, 6, 6),             /* 3 estrella */
        T(135, 160, 165, 160, 150, 195), /* 4 llama */
        T(125, 110, 125, 160, 98, 168),  /* 5 aleta izquierda */
        T(175, 110, 175, 160, 202, 168), /* 6 aleta derecha */
        R(125, 52, 50, 108),          /* 7 cuerpo */
        T(125, 52, 175, 52, 150, 10), /* 8 nariz */
        E(150, 92, 14, 14),           /* 9 ventana */
        E(260, 150, 12, 12),          /* 10 planeta */
    } },
};

const Drawing *paint_drawing(int i) { return &DRAWINGS[(i % PAINT_DRAWINGS + PAINT_DRAWINGS) % PAINT_DRAWINGS]; }

void paint_init(Paint *p) { memset(p, 0, sizeof *p); }

static long cross(int ax, int ay, int bx, int by, int px, int py) {
    return (long)(bx - ax) * (py - ay) - (long)(by - ay) * (px - ax);
}

static int region_hit(const Region *r, int x, int y) {
    const short *v = r->v;
    switch (r->kind) {
    case SH_RECT: return x >= v[0] && x < v[0] + v[2] && y >= v[1] && y < v[1] + v[3];
    case SH_ELLIPSE: {
        float dx = (float)(x - v[0]) / (float)v[2], dy = (float)(y - v[1]) / (float)v[3];
        return dx * dx + dy * dy <= 1.0f;
    }
    case SH_TRI: {
        long a = cross(v[0], v[1], v[2], v[3], x, y);
        long b = cross(v[2], v[3], v[4], v[5], x, y);
        long c = cross(v[4], v[5], v[0], v[1], x, y);
        return (a >= 0 && b >= 0 && c >= 0) || (a <= 0 && b <= 0 && c <= 0);
    }
    }
    return 0;
}

int paint_hit(const Drawing *d, int x, int y) {
    for (int i = d->n - 1; i >= 0; i--)
        if (region_hit(&d->reg[i], x, y)) return i;
    return -1;
}

int paint_fill(Paint *p, int region) {
    const Drawing *d = paint_drawing(p->drawing);
    if (region < 0 || region >= d->n) return 0;
    unsigned char c = (unsigned char)(p->color + 1);
    if (p->fill[p->drawing][region] == c) return 0;
    p->fill[p->drawing][region] = c;
    return 1;
}

void paint_clear(Paint *p) { memset(p->fill[p->drawing], 0, PAINT_MAX_REGIONS); }

void paint_next_drawing(Paint *p) { p->drawing = (p->drawing + 1) % PAINT_DRAWINGS; p->cursor = 0; }

void paint_move_cursor(Paint *p, int dir) {
    int n = paint_drawing(p->drawing)->n;
    p->cursor = (p->cursor + dir + n) % n;
}

int paint_count_filled(const Paint *p) {
    const Drawing *d = paint_drawing(p->drawing);
    int c = 0;
    for (int i = 0; i < d->n; i++) if (p->fill[p->drawing][i]) c++;
    return c;
}

int paint_complete(const Paint *p) { return paint_count_filled(p) == paint_drawing(p->drawing)->n; }
