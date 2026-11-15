#include "gfx.h"
#include "font_data.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ vista */
static float s_scale = 1.0f;
static int s_ox = 0, s_oy = 0;

static int X(int x) { return s_ox + (int)floorf((float)x * s_scale + 0.5f); }
static int Y(int y) { return s_oy + (int)floorf((float)y * s_scale + 0.5f); }
static int L(int v) { return (int)floorf((float)v * s_scale + 0.5f); }
static int L1(int v) { int p = L(v); return p < 1 ? 1 : p; } /* longitud mínima 1 px */

void gfx_begin(SDL_Renderer *r) {
    int ow = W, oh = H;
    SDL_GetRendererOutputSize(r, &ow, &oh);
    float sx = (float)ow / (float)W, sy = (float)oh / (float)H;
    s_scale = sx < sy ? sx : sy;
    if (s_scale >= 1.0f) { /* con escala >= 1 usamos factor entero cuando no pierde mucho (píxeles nítidos) */
        float fl = floorf(s_scale);
        if (s_scale - fl < 0.15f) s_scale = fl;
    }
    s_ox = (int)((float)ow - (float)W * s_scale) / 2;
    s_oy = (int)((float)oh - (float)H * s_scale) / 2;
    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    SDL_RenderClear(r);
    SDL_Rect clip = { s_ox, s_oy, (int)((float)W * s_scale + 0.5f), (int)((float)H * s_scale + 0.5f) };
    SDL_RenderSetClipRect(r, &clip);
}

float gfx_scale(void) { return s_scale; }

void gfx_to_logical(int px, int py, int *lx, int *ly) {
    *lx = (int)((float)(px - s_ox) / s_scale);
    *ly = (int)((float)(py - s_oy) / s_scale);
}

void gfx_setc(SDL_Renderer *r, unsigned c) {
    SDL_SetRenderDrawColor(r, (Uint8)((c >> 16) & 255), (Uint8)((c >> 8) & 255), (Uint8)(c & 255), 255);
}

/* ------------------------------------------------------------------ primitivas */
unsigned gfx_lighten(unsigned c, int d) {
    int rr = (int)((c >> 16) & 255) + d, gg = (int)((c >> 8) & 255) + d, bb = (int)(c & 255) + d;
    if (rr > 255) rr = 255;
    if (gg > 255) gg = 255;
    if (bb > 255) bb = 255;
    if (rr < 0) rr = 0;
    if (gg < 0) gg = 0;
    if (bb < 0) bb = 0;
    return ((unsigned)rr << 16) | ((unsigned)gg << 8) | (unsigned)bb;
}

static void prect(SDL_Renderer *r, int x0, int y0, int x1, int y1, unsigned col) { /* en píxeles reales */
    SDL_Rect q = { x0, y0, x1 - x0, y1 - y0 };
    if (q.w <= 0 || q.h <= 0) return;
    gfx_setc(r, col);
    SDL_RenderFillRect(r, &q);
}

void gfx_rect(SDL_Renderer *r, int x, int y, int w, int h, unsigned col) {
    prect(r, X(x), Y(y), X(x + w), Y(y + h), col);
}

void gfx_frame(SDL_Renderer *r, int x, int y, int w, int h, unsigned col, int t) {
    int x0 = X(x), y0 = Y(y), x1 = X(x + w), y1 = Y(y + h), pt = L1(t);
    prect(r, x0, y0, x1, y0 + pt, col);
    prect(r, x0, y1 - pt, x1, y1, col);
    prect(r, x0, y0, x0 + pt, y1, col);
    prect(r, x1 - pt, y0, x1, y1, col);
}

static void pellipse(SDL_Renderer *r, int cx, int cy, int rx, int ry, unsigned col) { /* píxeles reales */
    if (rx < 1) rx = 1;
    if (ry < 1) ry = 1;
    gfx_setc(r, col);
    for (int dy = -ry; dy <= ry; dy++) {
        float t = 1.0f - (float)(dy * dy) / (float)(ry * ry);
        int dx = (int)((float)rx * sqrtf(t > 0 ? t : 0) + 0.5f);
        SDL_RenderDrawLine(r, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

void gfx_ellipse(SDL_Renderer *r, int cx, int cy, int rx, int ry, unsigned col) {
    pellipse(r, X(cx), Y(cy), L1(rx), L1(ry), col);
}

void gfx_circle(SDL_Renderer *r, int cx, int cy, int rad, unsigned col) { gfx_ellipse(r, cx, cy, rad, rad, col); }

void gfx_ring(SDL_Renderer *r, int cx, int cy, int rad, int thick, unsigned col) {
    int pc = X(cx), pcy = Y(cy), pr = L1(rad), rin = pr - L1(thick);
    gfx_setc(r, col);
    for (int dy = -pr; dy <= pr; dy++) {
        int dxo = (int)(sqrtf((float)(pr * pr - dy * dy)) + 0.5f);
        if (rin > 0 && dy > -rin && dy < rin) {
            int dxi = (int)(sqrtf((float)(rin * rin - dy * dy)) + 0.5f);
            SDL_RenderDrawLine(r, pc - dxo, pcy + dy, pc - dxi, pcy + dy);
            SDL_RenderDrawLine(r, pc + dxi, pcy + dy, pc + dxo, pcy + dy);
        } else {
            SDL_RenderDrawLine(r, pc - dxo, pcy + dy, pc + dxo, pcy + dy);
        }
    }
}

void gfx_tri(SDL_Renderer *r, int x1, int y1, int x2, int y2, int x3, int y3, unsigned col) {
    int px[3] = { X(x1), X(x2), X(x3) }, py[3] = { Y(y1), Y(y2), Y(y3) };
    int miny = py[0] < py[1] ? (py[0] < py[2] ? py[0] : py[2]) : (py[1] < py[2] ? py[1] : py[2]);
    int maxy = py[0] > py[1] ? (py[0] > py[2] ? py[0] : py[2]) : (py[1] > py[2] ? py[1] : py[2]);
    gfx_setc(r, col);
    for (int y = miny; y <= maxy; y++) {
        float xs[3];
        int n = 0;
        for (int i = 0; i < 3; i++) {
            int j = (i + 1) % 3, ya = py[i], yb = py[j];
            if (ya == yb) continue;
            if ((y >= ya && y <= yb) || (y >= yb && y <= ya))
                xs[n++] = (float)px[i] + (float)(y - ya) / (float)(yb - ya) * (float)(px[j] - px[i]);
        }
        if (n < 2) continue;
        float lo = xs[0], hi = xs[0];
        for (int k = 1; k < n; k++) { if (xs[k] < lo) lo = xs[k]; if (xs[k] > hi) hi = xs[k]; }
        SDL_RenderDrawLine(r, (int)lo, y, (int)(hi + 0.5f), y);
    }
}

void gfx_line(SDL_Renderer *r, int x1, int y1, int x2, int y2, int thick, unsigned col) {
    int ax = X(x1), ay = Y(y1), bx = X(x2), by = Y(y2), pt = L1(thick);
    int dx = bx - ax < 0 ? ax - bx : bx - ax, dy = by - ay < 0 ? ay - by : by - ay;
    gfx_setc(r, col);
    for (int o = -(pt / 2); o <= (pt - 1) / 2; o++) {
        if (dx >= dy) SDL_RenderDrawLine(r, ax, ay + o, bx, by + o);
        else SDL_RenderDrawLine(r, ax + o, ay, bx + o, by);
    }
}

static void pline(SDL_Renderer *r, int ax, int ay, int bx, int by, int pt, unsigned col) { /* píxeles reales */
    int dx = bx - ax < 0 ? ax - bx : bx - ax, dy = by - ay < 0 ? ay - by : by - ay;
    gfx_setc(r, col);
    for (int o = -(pt / 2); o <= (pt - 1) / 2; o++) {
        if (dx >= dy) SDL_RenderDrawLine(r, ax, ay + o, bx, by + o);
        else SDL_RenderDrawLine(r, ax + o, ay, bx + o, by);
    }
}

void gfx_ellipse_outline(SDL_Renderer *r, int cx, int cy, int rx, int ry, int thick, unsigned col) {
    int px = X(cx), py = Y(cy), prx = L1(rx), pry = L1(ry), pt = L1(thick);
    int steps = 40 + (prx + pry) / 3;
    int lx = px + prx, ly = py;
    for (int i = 1; i <= steps; i++) {
        float a = 2.0f * PI_F * (float)i / (float)steps;
        int nx = px + (int)(cosf(a) * (float)prx), ny = py + (int)(sinf(a) * (float)pry);
        pline(r, lx, ly, nx, ny, pt, col);
        lx = nx; ly = ny;
    }
}

void gfx_round_rect(SDL_Renderer *r, int x, int y, int w, int h, int rad, unsigned col) {
    int x0 = X(x), y0 = Y(y), x1 = X(x + w), y1 = Y(y + h), pr = L1(rad);
    if (pr * 2 > x1 - x0) pr = (x1 - x0) / 2;
    if (pr * 2 > y1 - y0) pr = (y1 - y0) / 2;
    prect(r, x0 + pr, y0, x1 - pr, y1, col);
    prect(r, x0, y0 + pr, x0 + pr, y1 - pr, col);
    prect(r, x1 - pr, y0 + pr, x1, y1 - pr, col);
    pellipse(r, x0 + pr, y0 + pr, pr, pr, col);
    pellipse(r, x1 - pr - 1, y0 + pr, pr, pr, col);
    pellipse(r, x0 + pr, y1 - pr - 1, pr, pr, col);
    pellipse(r, x1 - pr - 1, y1 - pr - 1, pr, pr, col);
}

/* ------------------------------------------------------------------ texto */
#define CAP_H 0.70f /* altura de las mayúsculas de Outfit, en "em" */

static SDL_Texture *s_tex[FONT_SIZE_COUNT];

static SDL_Texture *font_texture(SDL_Renderer *r, int si) {
    if (s_tex[si]) return s_tex[si];
    const FontSize *fs = &FONT_SIZES[si];
    size_t n = (size_t)fs->atlas_w * (size_t)fs->atlas_h;
    Uint8 *px = (Uint8 *)malloc(n * 4);
    if (!px) return NULL;
    size_t o = 0;
    for (int i = 0; i < fs->rle_len && o < n;) { /* descomprimir: 0 n = n ceros */
        Uint8 b = fs->rle[i++], cnt = 1, a = b;
        if (b == 0) { cnt = fs->rle[i++]; a = 0; }
        for (int k = 0; k < cnt && o < n; k++, o++) {
            px[o * 4 + 0] = 255; px[o * 4 + 1] = 255; px[o * 4 + 2] = 255; px[o * 4 + 3] = a;
        }
    }
    SDL_Texture *t = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, fs->atlas_w, fs->atlas_h);
    if (t) {
        SDL_UpdateTexture(t, NULL, px, fs->atlas_w * 4);
        SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
    }
    free(px);
    s_tex[si] = t;
    return t;
}

/* tamaño embebido más cercano (en proporción) al tamaño físico pedido */
static int pick_size(float em_phys) {
    int best = 0;
    float bd = 1e9f;
    for (int i = 0; i < FONT_SIZE_COUNT; i++) {
        float d = fabsf(logf(em_phys / (float)FONT_SIZES[i].em));
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}

static unsigned utf8_next(const unsigned char **pp) {
    const unsigned char *p = *pp;
    unsigned c = *p;
    if (c == 0) return 0;
    if (c < 0x80) { *pp = p + 1; return c; }
    if ((c & 0xE0) == 0xC0 && (p[1] & 0xC0) == 0x80) { *pp = p + 2; return ((c & 0x1F) << 6) | (p[1] & 0x3F); }
    if ((c & 0xF0) == 0xE0 && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80) {
        *pp = p + 3;
        return ((c & 0x0F) << 12) | ((unsigned)(p[1] & 0x3F) << 6) | (p[2] & 0x3F);
    }
    *pp = p + 1;
    while ((**pp & 0xC0) == 0x80) (*pp)++;
    return '?';
}

static const FontGlyph *find_glyph(const FontSize *fs, unsigned code) {
    if (code >= 32 && code <= 126) return &fs->glyphs[code - 32];
    for (int i = 95; i < fs->nglyphs; i++) if (fs->glyphs[i].code == code) return &fs->glyphs[i];
    return &fs->glyphs['?' - 32];
}

int gfx_text_width(const char *s, int size) {
    int si = pick_size((float)size * (s_scale > 0 ? s_scale : 1.0f));
    const FontSize *fs = &FONT_SIZES[si];
    const unsigned char *p = (const unsigned char *)s;
    unsigned c;
    float w = 0;
    while ((c = utf8_next(&p)) != 0) w += (float)find_glyph(fs, c)->adv16 / 16.0f;
    return (int)(w * (float)size / (float)fs->em + 0.5f);
}

void gfx_text(SDL_Renderer *r, const char *s, int x, int y, int size, unsigned col) {
    float em_phys = (float)size * s_scale;
    int si = pick_size(em_phys);
    const FontSize *fs = &FONT_SIZES[si];
    SDL_Texture *tex = font_texture(r, si);
    if (!tex) return;
    float f = em_phys / (float)fs->em;
    SDL_SetTextureColorMod(tex, (Uint8)((col >> 16) & 255), (Uint8)((col >> 8) & 255), (Uint8)(col & 255));
    float pen = (float)X(x);
    int base = Y(y) + (int)(CAP_H * em_phys + 0.5f);
    const unsigned char *p = (const unsigned char *)s;
    unsigned c;
    while ((c = utf8_next(&p)) != 0) {
        const FontGlyph *g = find_glyph(fs, c);
        if (g->w > 0 && g->h > 0) {
            SDL_Rect src = { g->x, g->y, g->w, g->h };
            int dw = (int)((float)g->w * f + 0.5f), dh = (int)((float)g->h * f + 0.5f);
            SDL_Rect dst = { (int)(pen + (float)g->xoff * f + 0.5f), base + (int)((float)g->yoff * f + (g->yoff < 0 ? -0.5f : 0.5f)),
                             dw < 1 ? 1 : dw, dh < 1 ? 1 : dh };
            SDL_RenderCopy(r, tex, &src, &dst);
        }
        pen += (float)g->adv16 / 16.0f * f;
    }
}

void gfx_text_c(SDL_Renderer *r, const char *s, int cx, int y, int size, unsigned col) {
    gfx_text(r, s, cx - gfx_text_width(s, size) / 2, y, size, col);
}

void gfx_text_ol(SDL_Renderer *r, const char *s, int x, int y, int size, unsigned col, unsigned outline) {
    static const int OFF[8][2] = { {-1,0},{1,0},{0,-1},{0,1},{-1,-1},{1,-1},{-1,1},{1,1} };
    for (int i = 0; i < 8; i++) gfx_text(r, s, x + OFF[i][0], y + OFF[i][1], size, outline);
    gfx_text(r, s, x, y, size, col);
}

void gfx_text_ol_c(SDL_Renderer *r, const char *s, int cx, int y, int size, unsigned col, unsigned outline) {
    gfx_text_ol(r, s, cx - gfx_text_width(s, size) / 2, y, size, col, outline);
}

/* ------------------------------------------------------------------ bolita */
void gfx_blob(SDL_Renderer *r, int cx, int cy, int rad, unsigned col, int mood,
              float sx, float sy, float look, int blink) {
    int rx = (int)((float)rad * sx), ry = (int)((float)rad * sy);
    int er = rad / 4 > 2 ? rad / 4 : 2, pr = rad / 8 > 1 ? rad / 8 : 1;
    int ex = rad / 3, ey = cy - ry / 4, po = (int)(look * (float)er * 0.5f);
    gfx_ellipse(r, cx, cy + 1, rx + 1, ry + 1, gfx_lighten(col, -50)); /* contorno suave */
    gfx_ellipse(r, cx, cy, rx, ry, col);
    gfx_ellipse(r, cx - rx / 3, cy - ry / 2, rx / 4 > 1 ? rx / 4 : 1, ry / 6 > 1 ? ry / 6 : 1, gfx_lighten(col, 45)); /* brillo */
    if (blink) {
        gfx_line(r, cx - ex - er, ey, cx - ex + er, ey, 1, 0x222222);
        gfx_line(r, cx + ex - er, ey, cx + ex + er, ey, 1, 0x222222);
    } else {
        gfx_circle(r, cx - ex, ey, er, 0xFFFFFF);
        gfx_circle(r, cx + ex, ey, er, 0xFFFFFF);
        gfx_circle(r, cx - ex + po, ey, pr, 0x222222);
        gfx_circle(r, cx + ex + po, ey, pr, 0x222222);
    }
    if (rad >= 10 && mood > 0) { /* mejillas */
        gfx_circle(r, cx - rx * 2 / 3, cy + ry / 5, rad / 7, gfx_lighten(col, -25));
        gfx_circle(r, cx + rx * 2 / 3, cy + ry / 5, rad / 7, gfx_lighten(col, -25));
    }
    int my = cy + ry / 3, dm = mood > 0 ? rad / 5 + 1 : (mood < 0 ? -(rad / 6 + 1) : 0);
    gfx_line(r, cx - rad / 3, my, cx, my + dm, 1, 0x222222);
    gfx_line(r, cx, my + dm, cx + rad / 3, my, 1, 0x222222);
}

void gfx_clock(SDL_Renderer *r, int cx, int cy, int rad, int h, int m) {
    gfx_circle(r, cx, cy, rad + 3, 0x37474F);
    gfx_circle(r, cx, cy, rad, 0xFFFFFF);
    for (int k = 0; k < 12; k++) {
        float a = (float)k * 30.0f * PI_F / 180.0f;
        int in = (k % 3 == 0) ? rad - 7 : rad - 4;
        gfx_line(r, cx + (int)(sinf(a) * (float)in), cy - (int)(cosf(a) * (float)in),
                 cx + (int)(sinf(a) * (float)(rad - 1)), cy - (int)(cosf(a) * (float)(rad - 1)),
                 (k % 3 == 0) ? 2 : 1, 0x37474F);
    }
    float am = (float)m * 6.0f * PI_F / 180.0f;
    float ah = ((float)(h % 12) * 30.0f + (float)m * 0.5f) * PI_F / 180.0f;
    int lm = rad - 5, lh = (int)((float)rad * 0.55f);
    gfx_line(r, cx, cy, cx + (int)(sinf(ah) * (float)lh), cy - (int)(cosf(ah) * (float)lh), 3, 0x37474F);
    gfx_line(r, cx, cy, cx + (int)(sinf(am) * (float)lm), cy - (int)(cosf(am) * (float)lm), 2, 0xE53935);
    gfx_circle(r, cx, cy, 3, 0x37474F);
}

/* ------------------------------------------------------------------ símbolos y figuras */
unsigned gfx_symbol_color(int i) {
    static const unsigned C[4] = { 0xE0559A, 0x3DAE62, 0xE0463E, 0x3D6FD6 }; /* cuadrado, triángulo, círculo, cruz */
    return C[i & 3];
}

void gfx_symbol(SDL_Renderer *r, int i, int cx, int cy, int s, int t, unsigned col) {
    switch (i & 3) {
    case 0: gfx_frame(r, cx - s, cy - s, 2 * s + 1, 2 * s + 1, col, t); break;
    case 1: {
        int bx = s, by = (int)((float)s * 0.85f);
        gfx_line(r, cx, cy - s, cx - bx, cy + by, t, col);
        gfx_line(r, cx, cy - s, cx + bx, cy + by, t, col);
        gfx_line(r, cx - bx, cy + by, cx + bx, cy + by, t, col);
        break; }
    case 2: gfx_ring(r, cx, cy, s + 1, t, col); break;
    default:
        gfx_line(r, cx - s, cy - s, cx + s, cy + s, t, col);
        gfx_line(r, cx + s, cy - s, cx - s, cy + s, t, col);
        break;
    }
}

static void shape_body(SDL_Renderer *r, int kind, int cx, int cy, int s, unsigned col) {
    switch (kind) {
    case 0: gfx_circle(r, cx, cy, s, col); break;
    case 1: gfx_rect(r, cx - s * 9 / 10, cy - s * 9 / 10, s * 18 / 10, s * 18 / 10, col); break;
    case 2: gfx_tri(r, cx, cy - s, cx - s, cy + s * 8 / 10, cx + s, cy + s * 8 / 10, col); break;
    case 3: gfx_rect(r, cx - s * 14 / 10, cy - s * 7 / 10, s * 28 / 10, s * 14 / 10, col); break;
    default:
        gfx_tri(r, cx, cy - s, cx - s * 12 / 10, cy, cx + s * 12 / 10, cy, col);
        gfx_tri(r, cx, cy + s, cx - s * 12 / 10, cy, cx + s * 12 / 10, cy, col);
        break;
    }
}

void gfx_shape(SDL_Renderer *r, int kind, int cx, int cy, int s, unsigned col) {
    shape_body(r, kind, cx + 2, cy + 2, s, gfx_lighten(col, -70)); /* sombra */
    shape_body(r, kind, cx, cy, s, col);
}
