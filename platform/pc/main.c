/* Bolitas Edu - prototipo SDL2 (PC / Android / PSP).
 * Pantalla lógica 480x272 (la de la PSP). Sin fuentes ni imágenes externas:
 * todo se dibuja con rectángulos y una fuente bitmap 5x7 incluida.
 *
 *   PC:      1 2 3 4 (o Z X C V) = tambores, flechas + Enter, Esc = atrás, L = idioma
 *   PSP:     Cuadrado/Triángulo/Círculo/Cruz = tambores, Start = menú, L/R = idioma
 *   Android: tocar tambores; botón Atrás = menú
 *
 *   ./bolitas_pc --smoke   -> prueba automática sin jugador (guarda capturas .bmp)
 */
#if defined(__has_include)
#  if __has_include(<SDL2/SDL.h>)
#    include <SDL2/SDL.h>
#  else
#    include <SDL.h>
#  endif
#else
#  include <SDL2/SDL.h>
#endif
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "edu.h"

#define W 480
#define H 272
#define PI_F 3.14159265f

/* ------------------------------------------------------------------ fuente */
/* Fuente clásica 5x7, ASCII 32..126, columnas (bit0 = arriba). */
static const unsigned char FONT[95][5] = {
    {0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x5F,0x00,0x00},{0x00,0x07,0x00,0x07,0x00},
    {0x14,0x7F,0x14,0x7F,0x14},{0x24,0x2A,0x7F,0x2A,0x12},{0x23,0x13,0x08,0x64,0x62},
    {0x36,0x49,0x56,0x20,0x50},{0x00,0x08,0x07,0x03,0x00},{0x00,0x1C,0x22,0x41,0x00},
    {0x00,0x41,0x22,0x1C,0x00},{0x2A,0x1C,0x7F,0x1C,0x2A},{0x08,0x08,0x3E,0x08,0x08},
    {0x00,0x80,0x70,0x30,0x00},{0x08,0x08,0x08,0x08,0x08},{0x00,0x00,0x60,0x60,0x00},
    {0x20,0x10,0x08,0x04,0x02},{0x3E,0x51,0x49,0x45,0x3E},{0x00,0x42,0x7F,0x40,0x00},
    {0x72,0x49,0x49,0x49,0x46},{0x21,0x41,0x49,0x4D,0x33},{0x18,0x14,0x12,0x7F,0x10},
    {0x27,0x45,0x45,0x45,0x39},{0x3C,0x4A,0x49,0x49,0x31},{0x41,0x21,0x11,0x09,0x07},
    {0x36,0x49,0x49,0x49,0x36},{0x46,0x49,0x49,0x29,0x1E},{0x00,0x00,0x14,0x00,0x00},
    {0x00,0x40,0x34,0x00,0x00},{0x00,0x08,0x14,0x22,0x41},{0x14,0x14,0x14,0x14,0x14},
    {0x00,0x41,0x22,0x14,0x08},{0x02,0x01,0x59,0x09,0x06},{0x3E,0x41,0x5D,0x59,0x4E},
    {0x7C,0x12,0x11,0x12,0x7C},{0x7F,0x49,0x49,0x49,0x36},{0x3E,0x41,0x41,0x41,0x22},
    {0x7F,0x41,0x41,0x41,0x3E},{0x7F,0x49,0x49,0x49,0x41},{0x7F,0x09,0x09,0x09,0x01},
    {0x3E,0x41,0x41,0x51,0x73},{0x7F,0x08,0x08,0x08,0x7F},{0x00,0x41,0x7F,0x41,0x00},
    {0x20,0x40,0x41,0x3F,0x01},{0x7F,0x08,0x14,0x22,0x41},{0x7F,0x40,0x40,0x40,0x40},
    {0x7F,0x02,0x1C,0x02,0x7F},{0x7F,0x04,0x08,0x10,0x7F},{0x3E,0x41,0x41,0x41,0x3E},
    {0x7F,0x09,0x09,0x09,0x06},{0x3E,0x41,0x51,0x21,0x5E},{0x7F,0x09,0x19,0x29,0x46},
    {0x26,0x49,0x49,0x49,0x32},{0x03,0x01,0x7F,0x01,0x03},{0x3F,0x40,0x40,0x40,0x3F},
    {0x1F,0x20,0x40,0x20,0x1F},{0x3F,0x40,0x38,0x40,0x3F},{0x63,0x14,0x08,0x14,0x63},
    {0x03,0x04,0x78,0x04,0x03},{0x61,0x59,0x49,0x4D,0x43},{0x00,0x7F,0x41,0x41,0x41},
    {0x02,0x04,0x08,0x10,0x20},{0x00,0x41,0x41,0x41,0x7F},{0x04,0x02,0x01,0x02,0x04},
    {0x40,0x40,0x40,0x40,0x40},{0x00,0x03,0x07,0x08,0x00},{0x20,0x54,0x54,0x78,0x40},
    {0x7F,0x28,0x44,0x44,0x38},{0x38,0x44,0x44,0x44,0x28},{0x38,0x44,0x44,0x28,0x7F},
    {0x38,0x54,0x54,0x54,0x18},{0x00,0x08,0x7E,0x09,0x02},{0x18,0xA4,0xA4,0x9C,0x78},
    {0x7F,0x08,0x04,0x04,0x78},{0x00,0x44,0x7D,0x40,0x00},{0x20,0x40,0x40,0x3D,0x00},
    {0x7F,0x10,0x28,0x44,0x00},{0x00,0x41,0x7F,0x40,0x00},{0x7C,0x04,0x78,0x04,0x78},
    {0x7C,0x08,0x04,0x04,0x78},{0x38,0x44,0x44,0x44,0x38},{0xFC,0x18,0x24,0x24,0x18},
    {0x18,0x24,0x24,0x18,0xFC},{0x7C,0x08,0x04,0x04,0x08},{0x48,0x54,0x54,0x54,0x24},
    {0x04,0x04,0x3F,0x44,0x24},{0x3C,0x40,0x40,0x20,0x7C},{0x1C,0x20,0x40,0x20,0x1C},
    {0x3C,0x40,0x30,0x40,0x3C},{0x44,0x28,0x10,0x28,0x44},{0x4C,0x90,0x90,0x90,0x7C},
    {0x44,0x64,0x54,0x4C,0x44},{0x00,0x08,0x36,0x41,0x00},{0x00,0x00,0x77,0x00,0x00},
    {0x00,0x41,0x36,0x08,0x00},{0x02,0x01,0x02,0x04,0x02},
};

/* Lee el siguiente carácter de una cadena UTF-8 y lo reduce a ASCII
 * (á->a, ñ->n, ¿->?, ¡->!) porque la fuente solo tiene ASCII. */
static int next_char(const unsigned char **pp) {
    const unsigned char *p = *pp;
    int c = *p;
    if (c == 0) return 0;
    if (c < 0x80) { *pp = p + 1; return c; }
    if (p[1] == 0) { *pp = p + 1; return '?'; }
    if (c == 0xC3) {
        static const char lo_b[] = "\xA1\xA9\xAD\xB3\xBA\xB1\xBC", lo_a[] = "aeiounu";
        static const char up_b[] = "\x81\x89\x8D\x93\x9A\x91\x9C", up_a[] = "AEIOUNU";
        const char *q;
        *pp = p + 2;
        if ((q = strchr(lo_b, p[1])) != NULL) return lo_a[q - lo_b];
        if ((q = strchr(up_b, p[1])) != NULL) return up_a[q - up_b];
        return '?';
    }
    if (c == 0xC2) {
        *pp = p + 2;
        return p[1] == 0xBF ? '?' : (p[1] == 0xA1 ? '!' : ' ');
    }
    *pp = p + 1;
    while ((**pp & 0xC0) == 0x80) (*pp)++;
    return '?';
}

static int text_width(const char *s, int sc) {
    const unsigned char *p = (const unsigned char *)s;
    int n = 0;
    while (next_char(&p)) n++;
    return n ? n * 6 * sc - sc : 0;
}

static void setc(SDL_Renderer *r, unsigned c) {
    SDL_SetRenderDrawColor(r, (c >> 16) & 255, (c >> 8) & 255, c & 255, 255);
}

static void draw_text(SDL_Renderer *r, const char *s, int x, int y, int sc, unsigned col) {
    const unsigned char *p = (const unsigned char *)s;
    int c;
    setc(r, col);
    while ((c = next_char(&p)) != 0) {
        if (c < 32 || c > 126) c = '?';
        const unsigned char *g = FONT[c - 32];
        for (int cx = 0; cx < 5; cx++)
            for (int cy = 0; cy < 8; cy++)
                if ((g[cx] >> cy) & 1) {
                    SDL_Rect q = { x + cx * sc, y + cy * sc, sc, sc };
                    SDL_RenderFillRect(r, &q);
                }
        x += 6 * sc;
    }
}

static void draw_text_c(SDL_Renderer *r, const char *s, int cx, int y, int sc, unsigned col) {
    draw_text(r, s, cx - text_width(s, sc) / 2, y, sc, col);
}

/* ------------------------------------------------------------------ dibujo */
static void fill_ellipse(SDL_Renderer *r, int cx, int cy, int rx, int ry) {
    if (rx < 1) rx = 1;
    if (ry < 1) ry = 1;
    for (int dy = -ry; dy <= ry; dy++) {
        float t = 1.0f - (float)(dy * dy) / (float)(ry * ry);
        int dx = (int)((float)rx * sqrtf(t > 0 ? t : 0));
        SDL_RenderDrawLine(r, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

static void fill_circle(SDL_Renderer *r, int cx, int cy, int rad) { fill_ellipse(r, cx, cy, rad, rad); }

static void fill_rect(SDL_Renderer *r, int x, int y, int w, int h, unsigned col) {
    SDL_Rect q = { x, y, w, h };
    setc(r, col);
    SDL_RenderFillRect(r, &q);
}

static unsigned lighten(unsigned c, int d) {
    int rr = ((c >> 16) & 255) + d, gg = ((c >> 8) & 255) + d, bb = (c & 255) + d;
    if (rr > 255) rr = 255;
    if (gg > 255) gg = 255;
    if (bb > 255) bb = 255;
    return ((unsigned)rr << 16) | ((unsigned)gg << 8) | (unsigned)bb;
}

static const unsigned BLOB_COL[6] = { 0xF06292, 0xFFD54F, 0x4FC3F7, 0x81C784, 0xBA68C8, 0xFF8A65 };
static const unsigned DRUM_COL[4] = { 0xE8574B, 0xF2B632, 0x4BB8E8, 0x7BC96F };

/* mood: 1 contenta, 0 normal, -1 "ups". sx/sy: aplastado. */
static void draw_blob(SDL_Renderer *r, int cx, int cy, int rad, unsigned col, int mood, float sx, float sy) {
    int rx = (int)(rad * sx), ry = (int)(rad * sy);
    int er = rad / 4 > 2 ? rad / 4 : 2, pr = rad / 8 > 1 ? rad / 8 : 1;
    setc(r, col);
    fill_ellipse(r, cx, cy, rx, ry);
    setc(r, 0xFFFFFF);
    fill_circle(r, cx - rad / 3, cy - ry / 4, er);
    fill_circle(r, cx + rad / 3, cy - ry / 4, er);
    setc(r, 0x222222);
    fill_circle(r, cx - rad / 3, cy - ry / 4, pr);
    fill_circle(r, cx + rad / 3, cy - ry / 4, pr);
    int my = cy + ry / 3, dm = mood > 0 ? rad / 6 + 1 : (mood < 0 ? -(rad / 6 + 1) : 0);
    SDL_RenderDrawLine(r, cx - rad / 3, my, cx, my + dm);
    SDL_RenderDrawLine(r, cx, my + dm, cx + rad / 3, my);
}

static void draw_clock(SDL_Renderer *r, int cx, int cy, int rad, int h, int m) {
    setc(r, 0x37474F);
    fill_circle(r, cx, cy, rad + 3);
    setc(r, 0xFFFFFF);
    fill_circle(r, cx, cy, rad);
    setc(r, 0x37474F);
    for (int k = 0; k < 12; k++) {
        float a = k * 30.0f * PI_F / 180.0f;
        int in = (k % 3 == 0) ? rad - 9 : rad - 5;
        SDL_RenderDrawLine(r, cx + (int)(sinf(a) * in), cy - (int)(cosf(a) * in),
                           cx + (int)(sinf(a) * (rad - 1)), cy - (int)(cosf(a) * (rad - 1)));
    }
    float am = m * 6.0f * PI_F / 180.0f;
    float ah = ((h % 12) * 30.0f + m * 0.5f) * PI_F / 180.0f;
    int lm = rad - 6, lh = (int)(rad * 0.55f);
    for (int o = -1; o <= 1; o++) {
        setc(r, 0x37474F);
        SDL_RenderDrawLine(r, cx + o, cy, cx + o + (int)(sinf(ah) * lh), cy - (int)(cosf(ah) * lh));
        setc(r, 0xE53935);
        SDL_RenderDrawLine(r, cx + o, cy, cx + o + (int)(sinf(am) * lm), cy - (int)(cosf(am) * lm));
    }
    setc(r, 0x37474F);
    fill_circle(r, cx, cy, 3);
}

/* botón PSP dibujado pequeño: 0 cuadrado, 1 triángulo, 2 círculo, 3 cruz */
static void draw_symbol(SDL_Renderer *r, int i, int x, int y) {
    setc(r, 0x222222);
    switch (i) {
    case 0: { SDL_Rect q = { x, y, 9, 9 }; SDL_RenderDrawRect(r, &q); break; }
    case 1:
        SDL_RenderDrawLine(r, x + 4, y, x, y + 9);
        SDL_RenderDrawLine(r, x + 4, y, x + 8, y + 9);
        SDL_RenderDrawLine(r, x, y + 9, x + 8, y + 9);
        break;
    case 2: fill_circle(r, x + 4, y + 4, 4); setc(r, 0xFFFFFF); fill_circle(r, x + 4, y + 4, 2); break;
    default:
        SDL_RenderDrawLine(r, x, y, x + 8, y + 8);
        SDL_RenderDrawLine(r, x + 8, y, x, y + 8);
        break;
    }
}

/* ------------------------------------------------------------------ juego */
enum { ST_MENU = 0, ST_PLAY = 1 };

typedef struct {
    int state, sel, quit;
    MiniGame mg;
    Lang lang;
    Rng rng;
    Progress prog;
    Round r;
    unsigned wrong_mask;
    int missed, score, rounds;
    float blob_x, blob_target, good_t, shake_t, press_t[4], time;
} Game;

#define DRUM_X(i) (24 + (i) * 112)
#define DRUM_Y 136
#define DRUM_W 96
#define DRUM_H 52
#define MENU_Y 66
#define MENU_H 24

static void new_round(Game *g) {
    edu_make_round(&g->r, g->mg, g->prog.level, g->lang, &g->rng);
    g->wrong_mask = 0;
    g->missed = 0;
}

static void game_start(Game *g, MiniGame mg) {
    g->mg = mg;
    progress_init(&g->prog);
    g->score = g->rounds = 0;
    g->blob_x = g->blob_target = 40;
    g->good_t = g->shake_t = 0;
    memset(g->press_t, 0, sizeof g->press_t);
    g->state = ST_PLAY;
    new_round(g);
}

static void game_answer(Game *g, int i) {
    if (g->state != ST_PLAY || g->good_t > 0 || i < 0 || i > 3) return;
    if (g->wrong_mask & (1u << i)) return;
    g->press_t[i] = 0.25f;
    if (i == g->r.correct) {
        if (!g->missed) { g->score++; progress_answer(&g->prog, 1); }
        g->rounds++;
        g->good_t = 0.8f;
        g->blob_target += 45;
        if (g->blob_target > 440) { g->blob_target = 40; g->blob_x = -20; }
    } else {
        if (!g->missed) progress_answer(&g->prog, 0);
        g->missed = 1;
        g->wrong_mask |= 1u << i;
        g->shake_t = 0.4f;
    }
}

static void game_update(Game *g, float dt) {
    g->time += dt;
    for (int i = 0; i < 4; i++) if (g->press_t[i] > 0) g->press_t[i] -= dt;
    if (g->shake_t > 0) g->shake_t -= dt;
    if (g->good_t > 0) {
        g->good_t -= dt;
        if (g->good_t <= 0) new_round(g);
    }
    float k = dt * 5.0f > 1.0f ? 1.0f : dt * 5.0f;
    g->blob_x += (g->blob_target - g->blob_x) * k;
}

static void act_drum(Game *g, int i) { if (g->state == ST_PLAY) game_answer(g, i); }
static void act_nav(Game *g, int d) { if (g->state == ST_MENU) g->sel = (g->sel + d + MG_COUNT) % MG_COUNT; }
static void act_ok(Game *g) { if (g->state == ST_MENU) game_start(g, (MiniGame)g->sel); }
static void act_back(Game *g) { if (g->state == ST_PLAY) g->state = ST_MENU; else g->quit = 1; }
static void act_lang(Game *g) {
    g->lang = g->lang == LANG_ES ? LANG_EN : LANG_ES;
    if (g->state == ST_PLAY) new_round(g);
}

/* ------------------------------------------------------------------ pintado */
static void draw_background(SDL_Renderer *r) {
    for (int y = 0; y < H; y += 4) {
        int t = y * 60 / H;
        fill_rect(r, 0, y, W, 4, 0xA8DCF5 - ((unsigned)t << 8) - ((unsigned)t << 16) / 2);
    }
    setc(r, 0x8BC34A);
    fill_ellipse(r, 90, 250, 190, 40);
    fill_ellipse(r, 380, 258, 220, 36);
    fill_rect(r, 0, 238, W, H - 238, 0x7CB342);
}

static void draw_row_blobs(SDL_Renderer *r, const Round *rd) {
    int a = rd->visual_a, b = rd->visual_b, total = a + b, x0;
    if (rd->game == MG_SUMAS) {
        x0 = W / 2 - ((total + 1) * 20) / 2 + 10;
        for (int i = 0; i < a; i++) { setc(r, BLOB_COL[0]); fill_circle(r, x0 + i * 20, 97, 7); }
        for (int i = 0; i < b; i++) { setc(r, BLOB_COL[1]); fill_circle(r, x0 + (a + 1 + i) * 20, 97, 7); }
    } else { /* restas: las b últimas "se van" (grises) */
        x0 = W / 2 - (a * 20) / 2 + 10;
        for (int i = 0; i < a; i++) {
            setc(r, i >= a - b ? 0xB0BEC5 : BLOB_COL[0]);
            fill_circle(r, x0 + i * 20, 97, 7);
        }
    }
}

static void game_draw(Game *g, SDL_Renderer *r) {
    char buf[64];
    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    SDL_RenderClear(r);
    draw_background(r);

    if (g->state == ST_MENU) {
        draw_text_c(r, "BOLITAS EDU", W / 2, 14, 5, 0x37474F);
        for (int i = 0; i < MG_COUNT; i++) {
            int y = MENU_Y + i * MENU_H;
            const char *nm = edu_game_name((MiniGame)i, g->lang);
            if (i == g->sel) {
                fill_rect(r, 90, y - 2, 300, MENU_H - 2, 0xFFFFFF);
                setc(r, BLOB_COL[i % 6]);
                fill_circle(r, 104, y + 9, 7);
            }
            draw_text(r, nm, 124, y + 1, 2, i == g->sel ? 0x263238 : 0x455A64);
        }
        draw_text_c(r, g->lang == LANG_ES ? "ES" : "EN", W - 28, 248, 3, 0x1B5E20);
        draw_text(r, g->lang == LANG_ES ? "Arriba/Abajo + OK   L: idioma" : "Up/Down + OK   L: language",
                  8, 260, 1, 0x1B5E20);
        return;
    }

    /* HUD */
    draw_text(r, edu_game_name(g->mg, g->lang), 8, 6, 2, 0x37474F);
    snprintf(buf, sizeof buf, g->lang == LANG_ES ? "Nivel %d  Aciertos %d" : "Level %d  Score %d",
             g->prog.level, g->score);
    draw_text(r, buf, W - 8 - text_width(buf, 1), 6, 1, 0x37474F);

    /* pregunta */
    int sc = 4;
    while (sc > 1 && text_width(g->r.prompt, sc) > W - 40) sc--;
    draw_text_c(r, g->r.prompt, W / 2, 30, sc, 0x263238);

    /* dibujo según el minijuego */
    if (g->mg == MG_CONTAR) {
        int n = g->r.visual_a, rows = n > 10 ? 2 : 1;
        for (int row = 0; row < rows; row++) {
            int cnt = row == 0 ? (n > 10 ? 10 : n) : n - 10;
            int x0 = W / 2 - (cnt * 22) / 2 + 11, y = rows == 1 ? 97 : 80 + row * 24;
            for (int i = 0; i < cnt; i++) { setc(r, BLOB_COL[(row * 10 + i) % 6]); fill_circle(r, x0 + i * 22, y, 8); }
        }
    } else if (g->mg == MG_HORA) {
        draw_clock(r, W / 2, 98, 30, g->r.visual_a, g->r.visual_b);
    } else if ((g->mg == MG_SUMAS || g->mg == MG_RESTAS) && g->r.level == 1) {
        draw_row_blobs(r, &g->r);
    }

    /* tambores */
    for (int i = 0; i < 4; i++) {
        int x = DRUM_X(i), y = DRUM_Y;
        unsigned c = DRUM_COL[i];
        if (g->wrong_mask & (1u << i)) c = 0x9E9E9E;
        if (g->good_t > 0 && i == g->r.correct) c = 0x66BB6A;
        if (g->press_t[i] > 0) { c = lighten(c, 50); y += 3; }
        fill_rect(r, x - 2, y - 2, DRUM_W + 4, DRUM_H + 4, 0x37474F);
        fill_rect(r, x, y, DRUM_W, DRUM_H, c);
        fill_rect(r, x, y, DRUM_W, 6, lighten(c, 40));
        draw_symbol(r, i, x + 6, y + 9);
        const char *t = g->r.option[i];
        int s2 = 3;
        while (s2 > 1 && text_width(t, s2) > DRUM_W - 12) s2--;
        draw_text_c(r, t, x + DRUM_W / 2, y + (DRUM_H - 8 * s2) / 2 + 3, s2, 0x1B1B2F);
    }

    /* bolita que rueda (salta al ritmo, 100 pulsos por minuto) */
    float beat = fmodf(g->time * 100.0f / 60.0f, 1.0f);
    float bounce = fabsf(sinf(beat * PI_F)) * 5.0f;
    float sy = 1.0f + 0.06f * sinf(beat * PI_F);
    if (g->good_t > 0) bounce = sinf((1.0f - g->good_t / 0.8f) * PI_F) * 40.0f;
    float shake = g->shake_t > 0 ? sinf(g->time * 60.0f) * 3.0f : 0;
    int mood = g->good_t > 0 ? 1 : (g->shake_t > 0 ? -1 : 0);
    draw_blob(r, (int)(g->blob_x + shake), 222 - (int)bounce, 14, BLOB_COL[g->mg % 6], mood, 1.0f / sy, sy);

    /* mensajes (sin castigo: solo ánimo) */
    if (g->good_t > 0)
        draw_text_c(r, g->lang == LANG_ES ? "¡Muy bien!" : "Great!", W / 2, 196, 2, 0x1B5E20);
    else if (g->shake_t > 0)
        draw_text_c(r, g->lang == LANG_ES ? "¡Casi! Prueba otra" : "Almost! Try another", W / 2, 196, 2, 0xBF360C);

    draw_text(r, g->lang == LANG_ES ? "Esc/Start: menu" : "Esc/Start: menu", 8, 260, 1, 0x1B5E20);
}

/* ------------------------------------------------------------------ entrada */
static void on_key(Game *g, SDL_Keycode k) {
    switch (k) {
    case SDLK_1: case SDLK_z: act_drum(g, 0); break;
    case SDLK_2: case SDLK_x: act_drum(g, 1); break;
    case SDLK_3: case SDLK_c: act_drum(g, 2); break;
    case SDLK_4: case SDLK_v: act_drum(g, 3); break;
    case SDLK_UP: act_nav(g, -1); break;
    case SDLK_DOWN: act_nav(g, 1); break;
    case SDLK_RETURN: case SDLK_SPACE: act_ok(g); break;
    case SDLK_ESCAPE: case SDLK_AC_BACK: act_back(g); break;
    case SDLK_l: act_lang(g); break;
    default: break;
    }
}

static void on_joy(Game *g, int b) {
#ifdef __PSP__
    /* 0 triangulo, 1 circulo, 2 cruz, 3 cuadrado, 4 L, 5 R, 6 abajo, 7 izq, 8 arriba, 9 der, 10 select, 11 start */
    switch (b) {
    case 3: act_drum(g, 0); act_ok(g); break;
    case 0: act_drum(g, 1); act_ok(g); break;
    case 1: act_drum(g, 2); act_ok(g); break;
    case 2: act_drum(g, 3); act_ok(g); break;
    case 6: act_nav(g, 1); break;
    case 8: act_nav(g, -1); break;
    case 4: case 5: case 10: act_lang(g); break;
    case 11: act_back(g); break;
    default: break;
    }
#else
    if (b >= 0 && b <= 3) { act_drum(g, b); act_ok(g); }
    else if (b == 6 || b == 7) act_back(g);
#endif
}

static void on_click(Game *g, int x, int y) {
    if (g->state == ST_MENU) {
        if (x >= W - 70 && y >= 244) { act_lang(g); return; }
        for (int i = 0; i < MG_COUNT; i++) {
            int yy = MENU_Y + i * MENU_H;
            if (x >= 90 && x < 390 && y >= yy - 2 && y < yy + MENU_H - 2) { g->sel = i; act_ok(g); return; }
        }
    } else {
        for (int i = 0; i < 4; i++)
            if (x >= DRUM_X(i) && x < DRUM_X(i) + DRUM_W && y >= DRUM_Y && y < DRUM_Y + DRUM_H) { act_drum(g, i); return; }
        if (y >= 250 && x < 140) act_back(g);
    }
}

/* ------------------------------------------------------------------ prueba automática */
static void save_shot(SDL_Renderer *ren, const char *name) {
    SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, W, H, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!s) return;
    if (SDL_RenderReadPixels(ren, NULL, SDL_PIXELFORMAT_ARGB8888, s->pixels, s->pitch) == 0)
        SDL_SaveBMP(s, name);
    SDL_FreeSurface(s);
}

static int run_smoke(SDL_Renderer *ren, Game *g) {
    char name[64];
    game_draw(g, ren);
    save_shot(ren, "shot_menu.bmp");
    for (int mg = 0; mg < MG_COUNT; mg++) {
        game_start(g, (MiniGame)mg);
        for (int k = 0; k < 30; k++) game_update(g, 0.016f);
        for (int round = 0; round < 8; round++) {
            if (round == 1) { /* un fallo primero: debe quedar marcado y no avanzar */
                int w = (g->r.correct + 1) % 4;
                game_answer(g, w);
                if (!g->missed || !(g->wrong_mask & (1u << w)) || g->good_t > 0) { printf("FALLO: fallo no registrado\n"); return 1; }
                game_draw(g, ren);
                snprintf(name, sizeof name, "shot_%d_oops.bmp", mg);
                save_shot(ren, name);
            }
            game_draw(g, ren);
            if (round == 0) { snprintf(name, sizeof name, "shot_%d.bmp", mg); save_shot(ren, name); }
            game_answer(g, g->r.correct);
            if (g->good_t <= 0) { printf("FALLO: acierto no registrado\n"); return 1; }
            for (int k = 0; k < 60; k++) game_update(g, 0.016f);
            if (g->good_t > 0) { printf("FALLO: la ronda no avanza\n"); return 1; }
        }
        if (g->rounds != 8) { printf("FALLO: rondas=%d\n", g->rounds); return 1; }
        printf("ok  %s  nivel=%d aciertos=%d\n", edu_game_name((MiniGame)mg, LANG_ES), g->prog.level, g->score);
    }
    g->lang = LANG_EN;
    game_start(g, MG_VOCAB);
    game_draw(g, ren);
    save_shot(ren, "shot_en.bmp");
    return 0;
}

int main(int argc, char **argv) {
    int smoke = argc > 1 && strcmp(argv[1], "--smoke") == 0;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    Uint32 wflags = SDL_WINDOW_SHOWN;
    int ww = W * 2, wh = H * 2;
#if defined(__ANDROID__)
    wflags |= SDL_WINDOW_FULLSCREEN;
#elif defined(__PSP__)
    ww = W; wh = H;
#else
    wflags |= SDL_WINDOW_RESIZABLE;
#endif
    SDL_Window *win = SDL_CreateWindow("Bolitas Edu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, ww, wh, wflags);
    if (!win) { fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError()); return 1; }
    SDL_Renderer *ren = NULL;
    if (!smoke) ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    if (!ren) { fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError()); return 1; }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_RenderSetLogicalSize(ren, W, H);
    if (SDL_NumJoysticks() > 0) SDL_JoystickOpen(0);

    Game g;
    memset(&g, 0, sizeof g);
    g.lang = LANG_ES;
    rng_seed(&g.rng, smoke ? 1234u : (SDL_GetTicks() | 1u));
    progress_init(&g.prog);

    if (smoke) {
        int rc = run_smoke(ren, &g);
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
        printf(rc ? "SMOKE: FALLO\n" : "SMOKE: OK\n");
        return rc;
    }

    Uint32 last = SDL_GetTicks();
    while (!g.quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT: g.quit = 1; break;
            case SDL_KEYDOWN: if (!e.key.repeat) on_key(&g, e.key.keysym.sym); break;
            case SDL_JOYBUTTONDOWN: on_joy(&g, e.jbutton.button); break;
            case SDL_MOUSEBUTTONDOWN: on_click(&g, e.button.x, e.button.y); break;
            default: break;
            }
        }
        Uint32 now = SDL_GetTicks();
        float dt = (now - last) / 1000.0f;
        last = now;
        if (dt > 0.05f) dt = 0.05f;
        game_update(&g, dt);
        game_draw(&g, ren);
        SDL_RenderPresent(ren);
        if (dt < 0.012f) SDL_Delay(8);
    }
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
