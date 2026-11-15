/* Bolitas Edu - SDL2 (PC / Android / PSP / Vita).
 * Todo se dibuja en coordenadas lógicas 480x272 (ver gfx.h).
 *
 * Mandos (PSP / Vita):  cruceta o botones del rombo = opciones en los juegos,
 *   cruz = elegir, círculo = atrás (menús), Start = menú, Select = MAYÚSCULAS/minúsculas,
 *   L/R = idioma (menús) o color (colorear).
 * PC: flechas, Enter, Esc, Tab = Aa, L = idioma, 1-4 / Z X C V = opciones, N / Backspace = colorear.
 * Táctil / ratón: tocar.
 *
 *   ./bolitas_pc --smoke   -> prueba automática sin jugador (guarda capturas .bmp)
 */
#include "sdl_inc.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "edu.h"
#include "rhythm.h"
#include "paint.h"
#include "gfx.h"
#include "audio.h"

#if defined(__PSP__) || defined(__vita__)
#  define PSP_LIKE_PAD 1
#endif

/* ------------------------------------------------------------------ datos de menú */
typedef enum { K_QUIZ, K_RHYTHM, K_PAINT } Kind;
typedef struct { Kind kind; MiniGame mg; } Entry;

static const Entry SEC_NUM[] = { {K_QUIZ, MG_SUMAS}, {K_QUIZ, MG_RESTAS}, {K_QUIZ, MG_CONTAR},
                                 {K_QUIZ, MG_SERIES}, {K_QUIZ, MG_MAYOR} };
static const Entry SEC_LET[] = { {K_QUIZ, MG_SILABAS}, {K_QUIZ, MG_LETRA}, {K_QUIZ, MG_VOCAB} };
static const Entry SEC_DIS[] = { {K_QUIZ, MG_COLORES}, {K_QUIZ, MG_FORMAS}, {K_QUIZ, MG_HORA}, {K_QUIZ, MG_DIAS} };
static const Entry SEC_ART[] = { {K_RHYTHM, MG_SUMAS}, {K_PAINT, MG_SUMAS} };

typedef struct {
    const char *name[2];
    const char *soon[2];     /* texto si aún no hay juegos */
    unsigned col;
    const Entry *e;
    int n;
} Section;

#define N_SECTIONS 5
static const Section SECS[N_SECTIONS] = {
    { {"Números", "Numbers"}, {"", ""}, 0xFB8C00, SEC_NUM, 5 },
    { {"Letras", "Letters"}, {"", ""}, 0x42A5F5, SEC_LET, 3 },
    { {"Descubre", "Discover"}, {"", ""}, 0x66BB6A, SEC_DIS, 4 },
    { {"Música y arte", "Music & art"}, {"", ""}, 0xAB47BC, SEC_ART, 2 },
    { {"Agilidad mental", "Brain games"},
      {"Pronto: comecocos, memoria, Simon y puzzles", "Coming soon: pac-man, memory, Simon and puzzles"},
      0xEF5350, NULL, 0 },
};

static const char *entry_name(const Entry *e, Lang l) {
    if (e->kind == K_RHYTHM) return l == LANG_EN ? "Rhythm march" : "Marcha rítmica";
    if (e->kind == K_PAINT) return l == LANG_EN ? "Coloring" : "Colorear";
    return edu_game_name(e->mg, l);
}

/* ------------------------------------------------------------------ estado */
enum { ST_HOME, ST_LIST, ST_QUIZ, ST_RHYTHM, ST_PAINT };

typedef enum {
    A_LEFT, A_RIGHT, A_UP, A_DOWN, A_OK, A_BACK, A_CASE, A_LANG, A_L, A_R,
    A_SQR, A_TRI, A_CIR, A_CRO, A_D0, A_D1, A_D2, A_D3
} Act;

typedef struct {
    int state, quit, smoke;
    int home_sel, sec, list_sel;
    Lang lang;
    int upper;                  /* 1 = MAYÚSCULAS, 0 = minúsculas (contenido de aprendizaje) */
    Rng rng;
    float time;
    /* preguntas */
    MiniGame mg;
    Progress prog;
    Round r;
    unsigned wrong_mask;
    int missed, score, rounds;
    float pet_x[3], pet_target, good_t, shake_t, press_t[4];
    /* ritmo */
    Rhythm rh;
    float army_x, army_target, pad_t[4], hit_t, banner_t;
    int last_wins;
    /* colorear */
    Paint pt;
    float done_t, fill_t;
} Game;

static const unsigned PET_COL[6] = { 0xF06292, 0xFFD54F, 0x4FC3F7, 0x81C784, 0xBA68C8, 0xFF8A65 };

/* ------------------------------------------------------------------ utilidades de texto */
static char s_tc[8][96];
static int s_tcn;

/* texto de contenido en MAYÚSCULAS o minúsculas según el ajuste */
static const char *tc(const Game *g, const char *s) {
    char *b = s_tc[s_tcn++ & 7];
    edu_set_case(b, sizeof s_tc[0], s, g->upper);
    return b;
}

static int fit_size(const char *s, int maxw, int start) {
    static const int L[] = { 40, 28, 24, 20, 17, 14, 12, 10 };
    for (int i = 0; i < 8; i++) {
        if (L[i] > start) continue;
        if (gfx_text_width(s, L[i]) <= maxw) return L[i];
    }
    return 10;
}

/* ------------------------------------------------------------------ partículas (confeti) */
typedef struct { float x, y, vx, vy, life; unsigned col; int size; } Part;
#define N_PARTS 120
static Part s_parts[N_PARTS];
static unsigned s_vr = 777u;

static float vrand(void) { /* 0..1 solo para efectos visuales */
    s_vr = s_vr * 1664525u + 1013904223u;
    return (float)(s_vr >> 8) / (float)(1u << 24);
}

static void burst(float x, float y, int n, int big) {
    for (int i = 0, k = 0; i < N_PARTS && k < n; i++) {
        if (s_parts[i].life > 0) continue;
        float a = vrand() * 6.2831853f, sp = (big ? 60.0f : 40.0f) + vrand() * (big ? 120.0f : 70.0f);
        s_parts[i].x = x; s_parts[i].y = y;
        s_parts[i].vx = cosf(a) * sp; s_parts[i].vy = sinf(a) * sp - 70.0f;
        s_parts[i].life = 0.7f + vrand() * 0.6f;
        s_parts[i].col = PET_COL[(int)(vrand() * 5.99f)];
        s_parts[i].size = 2 + (int)(vrand() * 2.5f);
        k++;
    }
}

static void parts_update(float dt) {
    for (int i = 0; i < N_PARTS; i++) {
        if (s_parts[i].life <= 0) continue;
        s_parts[i].life -= dt;
        s_parts[i].vy += 260.0f * dt;
        s_parts[i].x += s_parts[i].vx * dt;
        s_parts[i].y += s_parts[i].vy * dt;
    }
}

static void parts_draw(SDL_Renderer *r) {
    for (int i = 0; i < N_PARTS; i++)
        if (s_parts[i].life > 0)
            gfx_rect(r, (int)s_parts[i].x, (int)s_parts[i].y, s_parts[i].size, s_parts[i].size, s_parts[i].col);
}

/* ------------------------------------------------------------------ escenario */
static void draw_sky(SDL_Renderer *r) {
    for (int y = 0; y < H; y += 4) { /* azul que aclara hacia el horizonte */
        unsigned rr = 0x90 + (unsigned)(y * 0x50 / H), gg = 0xD0 + (unsigned)(y * 0x28 / H), bb = 0xF5 + (unsigned)(y * 0x0A / H);
        gfx_rect(r, 0, y, W, 4, (rr << 16) | (gg << 8) | bb);
    }
}

static void draw_clouds(SDL_Renderer *r, float t) {
    static const float Y[3] = { 56.0f, 96.0f, 40.0f }, SP[3] = { 7.0f, 4.0f, 10.0f }, OFF[3] = { 40.0f, 230.0f, 380.0f };
    for (int i = 0; i < 3; i++) {
        int x = (int)fmodf(t * SP[i] + OFF[i], (float)(W + 120)) - 60, y = (int)Y[i];
        gfx_ellipse(r, x, y, 26, 10, 0xFFFFFF);
        gfx_ellipse(r, x - 17, y + 3, 16, 8, 0xFFFFFF);
        gfx_ellipse(r, x + 17, y + 3, 18, 8, 0xFFFFFF);
    }
}

static void draw_ground(SDL_Renderer *r, float t) {
    gfx_ellipse(r, 90, 250, 190, 40, 0x8BC34A);
    gfx_ellipse(r, 380, 258, 220, 36, 0x8BC34A);
    gfx_rect(r, 0, 238, W, H - 238, 0x7CB342);
    for (int i = 0; i < 12; i++) { /* florecitas que se mecen */
        int fx = 18 + i * 40 + (i % 3) * 7, fy = 248 + (i % 2) * 10;
        int sw = (int)(sinf(t * 2.0f + (float)i) * 2.0f);
        gfx_line(r, fx, fy, fx + sw, fy - 8, 1, 0x388E3C);
        gfx_circle(r, fx + sw, fy - 9, 3, PET_COL[i % 6]);
        gfx_circle(r, fx + sw, fy - 9, 1, 0xFFFFFF);
    }
}

static void draw_scene(SDL_Renderer *r, float t) {
    draw_sky(r);
    draw_clouds(r, t);
    draw_ground(r, t);
}

/* Bolita animada que salta al compás, parpadea y respira. jump en px. */
static void draw_pet(SDL_Renderer *r, float x, float ground, int rad, unsigned col, float t, float phase,
                     int mood, float jump, float look) {
    float beat = fmodf(t * 100.0f / 60.0f + phase, 1.0f);
    float hop = fabsf(sinf(beat * PI_F)) * 4.0f;
    float sy = 1.0f + 0.07f * sinf(beat * PI_F * 2.0f);
    int blink = fmodf(t + phase * 5.0f, 3.4f) < 0.13f;
    float lift = hop + jump;
    gfx_ellipse(r, (int)x, (int)ground + 1, (int)((float)rad * (0.85f - lift / 80.0f)), 3, 0x6A9A33);
    gfx_blob(r, (int)x, (int)(ground - (float)rad * sy - lift), rad, col, mood, 1.0f / sy, sy, look, blink);
}

static void draw_chip(SDL_Renderer *r, int x, int y, int w, int h, const char *text, unsigned fill, unsigned fg) {
    gfx_round_rect(r, x, y, w, h, h / 2, 0x37474F);
    gfx_round_rect(r, x + 1, y + 1, w - 2, h - 2, h / 2 - 1, fill);
    gfx_text_c(r, text, x + w / 2, y + h / 2 - 7, 14, fg);
}

static void draw_back_button(SDL_Renderer *r) {
    gfx_round_rect(r, 4, 4, 28, 24, 8, 0x37474F);
    gfx_tri(r, 21, 9, 21, 23, 11, 16, 0xFFFFFF);
}

typedef struct { int sym; const char *text; } Hint;

/* Pistas de botones en la parte baja, centradas */
static void draw_hints(SDL_Renderer *r, int y, const Hint *h, int n) {
    int total = 0, w[6];
    for (int i = 0; i < n; i++) {
        w[i] = gfx_text_width(h[i].text, 12) + (h[i].sym >= 0 ? 16 : 0);
        total += w[i] + (i ? 16 : 0);
    }
    int x = W / 2 - total / 2;
    for (int i = 0; i < n; i++) {
        if (h[i].sym >= 0) {
            gfx_circle(r, x + 6, y + 5, 7, 0x37474F);
            gfx_symbol(r, h[i].sym, x + 6, y + 5, 3, 1, 0xFFFFFF);
            x += 16;
        }
        gfx_text_ol(r, h[i].text, x, y, 12, 0xFFFFFF, 0x1B3A1B);
        x += gfx_text_width(h[i].text, 12) + 16;
    }
}

static void draw_top_chips(Game *g, SDL_Renderer *r, int with_lang) {
    draw_chip(r, W - 52, 4, 48, 24, g->upper ? "ABC" : "abc", 0xFFFFFF, 0x263238);
    if (with_lang) draw_chip(r, W - 100, 4, 44, 24, g->lang == LANG_ES ? "ES" : "EN", 0xFFF59D, 0x263238);
}

/* ------------------------------------------------------------------ preguntas */
static const int BX[4] = { 10, 166, 322, 166 }, BY[4] = { 148, 118, 148, 178 };
#define BW 148
#define BH 32

static void quiz_new_round(Game *g) {
    edu_make_round(&g->r, g->mg, g->prog.level, g->lang, &g->rng);
    g->wrong_mask = 0;
    g->missed = 0;
}

static void quiz_start(Game *g, MiniGame mg) {
    g->mg = mg;
    progress_init(&g->prog);
    g->score = g->rounds = 0;
    for (int i = 0; i < 3; i++) g->pet_x[i] = 40.0f - 26.0f * (float)i;
    g->pet_target = 70;
    g->good_t = g->shake_t = 0;
    memset(g->press_t, 0, sizeof g->press_t);
    g->state = ST_QUIZ;
    quiz_new_round(g);
}

static void quiz_answer(Game *g, int i) {
    if (g->state != ST_QUIZ || g->good_t > 0 || i < 0 || i > 3) return;
    if (g->wrong_mask & (1u << i)) return;
    g->press_t[i] = 0.25f;
    if (i == g->r.correct) {
        if (!g->missed) { g->score++; progress_answer(&g->prog, 1); }
        g->rounds++;
        g->good_t = 0.9f;
        g->pet_target += 45;
        if (g->pet_target > 430) { g->pet_target = 70; for (int k = 0; k < 3; k++) g->pet_x[k] = -20.0f - 26.0f * (float)k; }
        sfx_good();
        burst(g->pet_x[0], 210, 22, 0);
    } else {
        if (!g->missed) progress_answer(&g->prog, 0);
        g->missed = 1;
        g->wrong_mask |= 1u << i;
        g->shake_t = 0.4f;
        sfx_bad();
    }
}

static void quiz_update(Game *g, float dt) {
    for (int i = 0; i < 4; i++) if (g->press_t[i] > 0) g->press_t[i] -= dt;
    if (g->shake_t > 0) g->shake_t -= dt;
    if (g->good_t > 0) {
        g->good_t -= dt;
        if (g->good_t <= 0) quiz_new_round(g);
    }
    float k = dt * 5.0f > 1.0f ? 1.0f : dt * 5.0f;
    g->pet_x[0] += (g->pet_target - g->pet_x[0]) * k;
    for (int i = 1; i < 3; i++) g->pet_x[i] += (g->pet_x[i - 1] - 26.0f - g->pet_x[i]) * (k * 0.9f);
}

static void draw_option_button(Game *g, SDL_Renderer *r, int i) {
    int x = BX[i], y = BY[i];
    unsigned sc = gfx_symbol_color(i), base = gfx_lighten(sc, 85);
    if (g->wrong_mask & (1u << i)) base = 0xCFD8DC;
    if (g->good_t > 0 && i == g->r.correct) base = 0xA5D6A7;
    if (g->press_t[i] > 0) { base = gfx_lighten(base, 25); y += 2; }
    gfx_round_rect(r, x, y + 2, BW, BH, 9, 0x2F3B45);
    gfx_round_rect(r, x, y, BW, BH, 9, 0x37474F);
    gfx_round_rect(r, x + 2, y + 2, BW - 4, BH - 4, 7, base);
    unsigned bc = (g->wrong_mask & (1u << i)) ? 0x90A4AE : sc;
    gfx_circle(r, x + 18, y + BH / 2, 12, bc);
    gfx_symbol(r, i, x + 18, y + BH / 2, 5, 2, 0xFFFFFF);
    const char *t = tc(g, g->r.option[i]);
    int sz = fit_size(t, BW - 46, 24);
    gfx_text_c(r, t, x + 32 + (BW - 38) / 2, y + BH / 2 - (int)(0.35f * (float)sz), sz, 0x1B1B2F);
}

static void draw_quiz(Game *g, SDL_Renderer *r) {
    char buf[128];
    draw_scene(r, g->time);

    draw_back_button(r);
    gfx_text(r, edu_game_name(g->mg, g->lang), 40, 12, 14, 0x263238);
    draw_top_chips(g, r, 0);
    snprintf(buf, sizeof buf, g->lang == LANG_ES ? "Nivel %d   Aciertos %d" : "Level %d   Score %d", g->prog.level, g->score);
    gfx_text(r, buf, W - 58 - gfx_text_width(buf, 12), 13, 12, 0x37474F);

    /* pregunta (el hueco de las sílabas "_" se ve más grande) */
    const char *p = tc(g, g->r.prompt);
    char pb[128];
    size_t o = 0;
    for (; *p && o + 3 < sizeof pb; p++) {
        if (*p == '_') { pb[o++] = '_'; pb[o++] = '_'; } else pb[o++] = *p;
    }
    pb[o] = 0;
    int sz = fit_size(pb, W - 24, 28);
    gfx_text_ol_c(r, pb, W / 2, 40, sz, 0x263238, 0xFFFFFF);

    /* dibujo según el minijuego */
    float t = g->time;
    if (g->mg == MG_CONTAR) {
        int n = g->r.visual_a, rows = n > 10 ? 2 : 1;
        for (int row = 0; row < rows; row++) {
            int cnt = row == 0 ? (n > 10 ? 10 : n) : n - 10;
            int x0 = W / 2 - (cnt * 22) / 2 + 11, y = rows == 1 ? 90 : 78 + row * 22;
            for (int i = 0; i < cnt; i++) {
                int dy = (int)(sinf(t * 4.0f + (float)(row * 10 + i) * 0.7f) * 2.0f);
                gfx_blob(r, x0 + i * 22, y + dy, 8, PET_COL[(row * 10 + i) % 6], 1, 1.0f, 1.0f, 0, 0);
            }
        }
    } else if (g->mg == MG_HORA) {
        gfx_clock(r, W / 2, 88, 24, g->r.visual_a, g->r.visual_b);
    } else if (g->mg == MG_COLORES) {
        draw_pet(r, (float)W / 2, 108, 22, edu_color_rgb(g->r.visual_a), t, 0.3f, 1, 0, 0);
    } else if (g->mg == MG_FORMAS) {
        gfx_shape(r, g->r.visual_a, W / 2, 88, 20, 0x1E88E5);
    } else if ((g->mg == MG_SUMAS || g->mg == MG_RESTAS) && g->r.level == 1) {
        int a = g->r.visual_a, b = g->r.visual_b, total = a + b, x0;
        if (g->mg == MG_SUMAS) {
            x0 = W / 2 - ((total + 1) * 20) / 2 + 10;
            for (int i = 0; i < a; i++) gfx_circle(r, x0 + i * 20, 90, 7, PET_COL[0]);
            for (int i = 0; i < b; i++) gfx_circle(r, x0 + (a + 1 + i) * 20, 90, 7, PET_COL[1]);
        } else {
            x0 = W / 2 - (a * 20) / 2 + 10;
            for (int i = 0; i < a; i++) gfx_circle(r, x0 + i * 20, 90, 7, i >= a - b ? 0xB0BEC5 : PET_COL[0]);
        }
    }

    for (int i = 0; i < 4; i++) draw_option_button(g, r, i);

    /* la bolita y sus amiguitas */
    float p1 = 1.0f - g->good_t / 0.9f;
    int mood = g->good_t > 0 ? 1 : (g->shake_t > 0 ? -1 : 0);
    float shake = g->shake_t > 0 ? sinf(g->time * 60.0f) * 3.0f : 0;
    for (int i = 2; i >= 0; i--) {
        float pk = p1 - 0.1f * (float)i, jump = (g->good_t > 0 && pk > 0 && pk < 1) ? sinf(pk * PI_F) * 36.0f : 0;
        draw_pet(r, g->pet_x[i] + shake, 236, i == 0 ? 15 : 12, PET_COL[(g->mg + (int)i * 2) % 6], g->time,
                 (float)i * 0.23f, mood, jump, g->pet_target - g->pet_x[0] > 4 ? 0.8f : 0);
    }
    parts_draw(r);

    if (g->good_t > 0) {
        static const char *OK[2][4] = { {"¡Muy bien!", "¡Genial!", "¡Fantástico!", "¡Eso es!"},
                                        {"Great!", "Awesome!", "Fantastic!", "You got it!"} };
        gfx_text_ol_c(r, OK[g->lang == LANG_EN][g->rounds % 4], W / 2, 216, 20, 0xFFFFFF, 0x1B5E20);
    } else if (g->shake_t > 0) {
        gfx_text_ol_c(r, g->lang == LANG_ES ? "¡Casi! Prueba otra" : "Almost! Try another", W / 2, 216, 20, 0xFFFFFF, 0xBF360C);
    }
    static const Hint HQ[2] = { {-1, "Start: menú"}, {-1, "Select: Aa"} };
    static const Hint HQE[2] = { {-1, "Start: menu"}, {-1, "Select: Aa"} };
    draw_hints(r, 258, g->lang == LANG_ES ? HQ : HQE, 2);
}

/* ------------------------------------------------------------------ marcha rítmica */
static const int PADX[4] = { 196, 240, 284, 240 }, PADY[4] = { 176, 146, 176, 206 };
#define TL_X0 80
#define TL_X1 400
#define TL_Y  66

static void rhythm_start(Game *g) {
    rh_init(&g->rh, &g->rng);
    g->army_x = 40; g->army_target = 40;
    memset(g->pad_t, 0, sizeof g->pad_t);
    g->hit_t = g->banner_t = 0;
    g->last_wins = 0;
    g->state = ST_RHYTHM;
}

static void rhythm_press(Game *g, int d) {
    if (g->state != ST_RHYTHM) return;
    g->pad_t[d] = 0.22f;
    sfx_drum(d);
    int res = rh_press(&g->rh, d);
    if (res == RH_PERFECT) burst((float)(TL_X0 + (g->rh.next - 1) * (TL_X1 - TL_X0) / (g->rh.len - 1)), TL_Y, 10, 0);
    if (res != 0) g->hit_t = 0.25f;
}

static void rhythm_update(Game *g, float dt) {
    rh_update(&g->rh, dt);
    for (int i = 0; i < 4; i++) {
        if (g->pad_t[i] > 0) g->pad_t[i] -= dt;
        if (g->rh.ev_notes && g->rh.phase == RH_LISTEN) { /* el tamborilero toca */ }
    }
    for (int i = 0; i < g->rh.len; i++)
        if (g->rh.ev_notes & (1u << i)) { g->pad_t[g->rh.seq[i]] = 0.25f; sfx_drum(g->rh.seq[i]); }
    if (g->hit_t > 0) g->hit_t -= dt;
    if (g->banner_t > 0) g->banner_t -= dt;
    if (g->rh.ev_result) {
        g->banner_t = 1.5f;
        if (g->rh.ev_result > 0) {
            sfx_good();
            burst(g->army_x + 40, 200, 24, 0);
            g->army_target = 40 + (float)(g->rh.wins % 7) * 45.0f;
            if (g->rh.wins % 7 == 0) { /* llegaron a la bandera */
                sfx_win();
                burst(430, 190, 60, 1);
                g->army_x = -60;
            }
        } else {
            sfx_bad();
        }
    }
    float k = dt * 3.0f > 1.0f ? 1.0f : dt * 3.0f;
    g->army_x += (g->army_target - g->army_x) * k;
}

static void draw_pad(SDL_Renderer *r, int i, float flash) {
    int rad = 22 + (int)(flash * 5.0f);
    unsigned c = gfx_symbol_color(i);
    gfx_circle(r, PADX[i], PADY[i] + 2, rad, 0x263238);
    gfx_circle(r, PADX[i], PADY[i], rad, flash > 0 ? gfx_lighten(c, 60) : c);
    gfx_circle(r, PADX[i], PADY[i] - 4, rad - 6, gfx_lighten(c, flash > 0 ? 80 : 35));
    gfx_circle(r, PADX[i], PADY[i], rad - 4, flash > 0 ? gfx_lighten(c, 50) : c);
    gfx_symbol(r, i, PADX[i], PADY[i], 8, 3, 0xFFFFFF);
}

static void draw_rhythm(Game *g, SDL_Renderer *r) {
    const Rhythm *h = &g->rh;
    char buf[96];
    draw_scene(r, g->time);
    draw_back_button(r);
    gfx_text(r, g->lang == LANG_ES ? "Marcha rítmica" : "Rhythm march", 40, 12, 14, 0x263238);
    draw_top_chips(g, r, 0);
    snprintf(buf, sizeof buf, g->lang == LANG_ES ? "Nivel %d   Marchas %d" : "Level %d   Marches %d", h->level, h->wins);
    gfx_text(r, buf, W - 58 - gfx_text_width(buf, 12), 13, 12, 0x37474F);

    /* línea de tiempo con los tambores de la secuencia */
    gfx_round_rect(r, 40, 36, 400, 62, 14, 0x263A55);
    float sp = (float)(TL_X1 - TL_X0) / (float)(h->len - 1);
    for (int i = 0; i < h->len; i++) {
        int x = TL_X0 + (int)(sp * (float)i);
        float d = h->t - rh_slot_time(i);
        int active = (h->phase == RH_LISTEN && d > -0.05f && d < 0.22f);
        int rad = active ? 20 : 16;
        unsigned ring = 0x90A4AE;
        if (h->phase == RH_PLAY || h->phase == RH_RESULT) {
            if (h->res[i] == RH_PERFECT) ring = 0xFFD54F;
            else if (h->res[i] == RH_GOOD) ring = 0x81C784;
            else if (h->res[i] == RH_MISS) ring = 0xEF5350;
            else if (h->phase == RH_PLAY && i == h->next) ring = 0xFFFFFF;
        }
        gfx_circle(r, x, TL_Y, rad + 3, ring);
        gfx_circle(r, x, TL_Y, rad, active ? 0xFFFFFF : 0xECEFF1);
        gfx_symbol(r, h->seq[i], x, TL_Y, rad / 3 + 2, 2, gfx_symbol_color(h->seq[i]));
    }
    if (h->phase != RH_RESULT) { /* cursor del compás: cuando cruza un círculo, toca golpear */
        float x = (float)TL_X0 + (h->t / RH_BEAT - 1.0f) * sp;
        if (x > (float)(TL_X0 - 30) && x < (float)(TL_X1 + 30)) {
            gfx_rect(r, (int)x - 1, 40, 3, 52, 0xFFFFFF);
            gfx_tri(r, (int)x - 5, 38, (int)x + 5, 38, (int)x, 46, 0xFFFFFF);
        }
    }

    /* mensaje de fase */
    const char *msg;
    unsigned mc = 0xFFFFFF;
    if (h->phase == RH_LISTEN) msg = g->lang == LANG_ES ? "Escucha..." : "Listen...";
    else if (h->phase == RH_PLAY) msg = g->lang == LANG_ES ? "¡Ahora tú!" : "Your turn!";
    else {
        int perfect = 1;
        for (int i = 0; i < h->len; i++) if (h->res[i] != RH_PERFECT) perfect = 0;
        if (!h->success) { msg = g->lang == LANG_ES ? "¡Otra vez!" : "Try again!"; mc = 0xFFCCBC; }
        else msg = perfect ? (g->lang == LANG_ES ? "¡Perfecto!" : "Perfect!") : (g->lang == LANG_ES ? "¡Muy bien!" : "Great!");
    }
    gfx_text_ol_c(r, msg, W / 2, 104, 24, mc, 0x263A55);

    /* los cuatro tambores en rombo, como los botones de la consola */
    for (int i = 0; i < 4; i++) draw_pad(r, i, g->pad_t[i] / 0.22f > 1 ? 1 : (g->pad_t[i] > 0 ? g->pad_t[i] / 0.22f : 0));

    /* bandera y ejército */
    gfx_line(r, 440, 238, 440, 196, 2, 0x5D4037);
    gfx_tri(r, 440, 196, 440, 210, 462, 203 + (int)(sinf(g->time * 5.0f) * 2.0f), 0xE53935);
    for (int i = 3; i >= 0; i--) {
        float jump = 0;
        if (g->hit_t > 0) jump = sinf(g->hit_t / 0.25f * PI_F) * 7.0f;
        if (h->phase == RH_RESULT && h->success && g->banner_t > 0) jump += fabsf(sinf(g->time * 9.0f + (float)i)) * 14.0f;
        int mood = h->phase == RH_RESULT ? (h->success ? 1 : -1) : 0;
        draw_pet(r, g->army_x + (float)i * 26.0f, 236, 12, PET_COL[(i * 2 + 1) % 6], g->time, (float)i * 0.2f, mood, jump, 0.6f);
    }
    parts_draw(r);
    static const Hint HR[2] = { {-1, "Start: menú"}, {-1, "Select: Aa"} };
    draw_hints(r, 258, HR, 2);
}

/* ------------------------------------------------------------------ colorear */
#define PX0 14
#define PY0 30
#define SW_X0 328
#define SW_Y0 34

static void swatch_rect(int i, int *x, int *y, int *w, int *h) {
    *x = SW_X0 + (i % 2) * 72; *y = SW_Y0 + (i / 2) * 40; *w = 66; *h = 34;
}

static void draw_region(SDL_Renderer *r, const Region *g, unsigned fill, unsigned line, int thick) {
    const short *v = g->v;
    switch (g->kind) {
    case SH_RECT:
        gfx_rect(r, PX0 + v[0], PY0 + v[1], v[2], v[3], fill);
        gfx_frame(r, PX0 + v[0], PY0 + v[1], v[2], v[3], line, thick);
        break;
    case SH_ELLIPSE:
        gfx_ellipse(r, PX0 + v[0], PY0 + v[1], v[2] + thick, v[3] + thick, line);
        gfx_ellipse(r, PX0 + v[0], PY0 + v[1], v[2], v[3], fill);
        break;
    default:
        gfx_tri(r, PX0 + v[0], PY0 + v[1], PX0 + v[2], PY0 + v[3], PX0 + v[4], PY0 + v[5], fill);
        gfx_line(r, PX0 + v[0], PY0 + v[1], PX0 + v[2], PY0 + v[3], thick, line);
        gfx_line(r, PX0 + v[2], PY0 + v[3], PX0 + v[4], PY0 + v[5], thick, line);
        gfx_line(r, PX0 + v[4], PY0 + v[5], PX0 + v[0], PY0 + v[1], thick, line);
        break;
    }
}

/* Solo el contorno (para marcar la región seleccionada sin tapar las demás) */
static void outline_region(SDL_Renderer *r, const Region *g, unsigned line, int thick) {
    const short *v = g->v;
    switch (g->kind) {
    case SH_RECT: gfx_frame(r, PX0 + v[0], PY0 + v[1], v[2], v[3], line, thick); break;
    case SH_ELLIPSE: gfx_ellipse_outline(r, PX0 + v[0], PY0 + v[1], v[2], v[3], thick, line); break;
    default:
        gfx_line(r, PX0 + v[0], PY0 + v[1], PX0 + v[2], PY0 + v[3], thick, line);
        gfx_line(r, PX0 + v[2], PY0 + v[3], PX0 + v[4], PY0 + v[5], thick, line);
        gfx_line(r, PX0 + v[4], PY0 + v[5], PX0 + v[0], PY0 + v[1], thick, line);
        break;
    }
}

static void paint_start(Game *g) {
    paint_init(&g->pt);
    g->done_t = g->fill_t = 0;
    g->state = ST_PAINT;
}

static void paint_do_fill(Game *g, int region) {
    if (paint_fill(&g->pt, region)) {
        const Region *rg = &paint_drawing(g->pt.drawing)->reg[region];
        float cx = (float)(PX0 + rg->v[0]), cy = (float)(PY0 + rg->v[1]);
        if (rg->kind == SH_RECT) { cx += (float)rg->v[2] / 2; cy += (float)rg->v[3] / 2; }
        if (rg->kind == SH_TRI) { cx = (float)(PX0 + (rg->v[0] + rg->v[2] + rg->v[4]) / 3); cy = (float)(PY0 + (rg->v[1] + rg->v[3] + rg->v[5]) / 3); }
        burst(cx, cy, 8, 0);
        sfx_pop();
        if (paint_complete(&g->pt)) { sfx_win(); burst(PX0 + 150, PY0 + 90, 70, 1); g->done_t = 2.5f; }
    }
}

static void draw_pill(SDL_Renderer *r, int x, int y, int w, int sym, const char *text) {
    gfx_round_rect(r, x, y, w, 26, 13, 0x37474F);
    gfx_round_rect(r, x + 2, y + 2, w - 4, 22, 11, 0xFFFFFF);
    gfx_circle(r, x + 15, y + 13, 9, gfx_symbol_color(sym));
    gfx_symbol(r, sym, x + 15, y + 13, 4, 2, 0xFFFFFF);
    gfx_text(r, text, x + 30, y + 7, 14, 0x263238);
}

static void draw_paint(Game *g, SDL_Renderer *r) {
    draw_sky(r);
    draw_clouds(r, g->time);
    draw_back_button(r);
    const Drawing *d = paint_drawing(g->pt.drawing);
    gfx_text(r, d->name[g->lang == LANG_EN], 40, 12, 14, 0x263238);
    draw_top_chips(g, r, 0);

    gfx_rect(r, PX0 - 4, PY0 - 4, PAINT_W + 8, PAINT_H_ + 8, 0x37474F);
    for (int i = 0; i < d->n; i++) {
        unsigned f = g->pt.fill[g->pt.drawing][i];
        draw_region(r, &d->reg[i], f ? edu_color_rgb((int)f - 1) : 0xFAFAFA, 0x546E7A, 1);
    }
    /* región seleccionada con teclado/mando: contorno que parpadea */
    if (fmodf(g->time, 0.8f) < 0.55f) outline_region(r, &d->reg[g->pt.cursor], 0xFF4081, 3);

    for (int i = 0; i < edu_color_count(); i++) {
        int x, y, w, h;
        swatch_rect(i, &x, &y, &w, &h);
        int sel = i == g->pt.color;
        gfx_round_rect(r, x - (sel ? 3 : 0), y - (sel ? 3 : 0), w + (sel ? 6 : 0), h + (sel ? 6 : 0), 9, sel ? 0xFFFFFF : 0x37474F);
        gfx_round_rect(r, x + 1, y + 1, w - 2, h - 2, 8, edu_color_rgb(i));
        if (sel) gfx_frame(r, x - 3, y - 3, w + 6, h + 6, 0x263238, 2);
    }
    gfx_text_ol_c(r, tc(g, edu_color_name(g->pt.color, g->lang)), SW_X0 + 69, 202, 24, 0xFFFFFF, 0x263238);

    draw_pill(r, 14, 238, 124, 1, g->lang == LANG_ES ? "Otro dibujo" : "Next");
    draw_pill(r, 146, 238, 104, 0, g->lang == LANG_ES ? "Borrar" : "Clear");
    draw_pill(r, 258, 238, 100, 3, g->lang == LANG_ES ? "Pintar" : "Paint");
    char buf[32];
    snprintf(buf, sizeof buf, "%d / %d", paint_count_filled(&g->pt), d->n);
    gfx_text_ol(r, buf, 380, 244, 14, 0xFFFFFF, 0x263238);
    if (g->done_t > 0) gfx_text_ol_c(r, g->lang == LANG_ES ? "¡Qué bonito!" : "How beautiful!", PX0 + 150, PY0 + 80, 28, 0xFFFFFF, 0xAD1457);
    parts_draw(r);
}

/* ------------------------------------------------------------------ menús */
static const int TILE_X[5] = { 20, 170, 320, 95, 245 }, TILE_Y[5] = { 52, 52, 52, 152, 152 };
#define TILE_W 140
#define TILE_H 90

static void draw_home(Game *g, SDL_Renderer *r) {
    draw_scene(r, g->time);
    gfx_text_ol_c(r, "BOLITAS EDU", W / 2 - 20, 12, 28, 0x263238, 0xFFFFFF);
    draw_top_chips(g, r, 1);
    for (int pass = 0; pass < 2; pass++) /* la tarjeta elegida se dibuja la última para que su borde quede encima */
    for (int k = 0; k < N_SECTIONS; k++) {
        int i = k, x, y, sel;
        if (pass == 0 && i == g->home_sel) continue;
        if (pass == 1) { if (k > 0) break; i = g->home_sel; }
        x = TILE_X[i]; y = TILE_Y[i]; sel = i == g->home_sel;
        float bob = sel ? fabsf(sinf(g->time * 6.0f)) * 3.0f : 0;
        unsigned c = SECS[i].col;
        if (SECS[i].n == 0) c = gfx_lighten(c, 70);
        gfx_round_rect(r, x - (sel ? 4 : 0), y - (sel ? 4 : 0), TILE_W + (sel ? 8 : 0), TILE_H + (sel ? 8 : 0), 14, sel ? 0xFFFFFF : 0x37474F);
        gfx_round_rect(r, x + 2, y + 2, TILE_W - 4, TILE_H - 4, 12, c);
        gfx_round_rect(r, x + 6, y + 6, TILE_W - 12, 20, 9, gfx_lighten(c, 40));
        char b[24];
        if (SECS[i].n == 0) snprintf(b, sizeof b, "%s", g->lang == LANG_ES ? "pronto" : "soon");
        else snprintf(b, sizeof b, g->lang == LANG_ES ? "%d juegos" : "%d games", SECS[i].n);
        gfx_text_c(r, b, x + TILE_W / 2, y + 12, 12, 0x263238);
        gfx_blob(r, x + TILE_W / 2, y + 44 - (int)bob, 14, gfx_lighten(c, 90), sel ? 1 : 0, 1, 1, 0, fmodf(g->time + (float)i, 3.3f) < 0.12f);
        const char *nm = SECS[i].name[g->lang == LANG_EN];
        int sz = fit_size(nm, TILE_W - 14, 20);
        gfx_text_ol_c(r, nm, x + TILE_W / 2, y + 64, sz, 0xFFFFFF, gfx_lighten(c, -90));
    }
    static const Hint HH[3] = { {3, "Elegir"}, {-1, "L/R: ES/EN"}, {-1, "Select: Aa"} };
    static const Hint HHE[3] = { {3, "Choose"}, {-1, "L/R: ES/EN"}, {-1, "Select: Aa"} };
    draw_hints(r, 258, g->lang == LANG_ES ? HH : HHE, 3);
}

static void draw_list(Game *g, SDL_Renderer *r) {
    const Section *s = &SECS[g->sec];
    draw_scene(r, g->time);
    draw_back_button(r);
    draw_top_chips(g, r, 1);
    gfx_text_ol(r, s->name[g->lang == LANG_EN], 42, 8, 24, 0xFFFFFF, gfx_lighten(s->col, -90));
    if (s->n == 0) {
        gfx_round_rect(r, 40, 80, 400, 110, 16, 0x37474F);
        gfx_round_rect(r, 43, 83, 394, 104, 14, 0xFFFFFF);
        const char *m = s->soon[g->lang == LANG_EN];
        int sz = fit_size(m, 360, 20);
        gfx_text_c(r, m, W / 2, 125, sz, 0x263238);
    }
    for (int i = 0; i < s->n; i++) {
        int y = 46 + i * 36, sel = i == g->list_sel;
        float bob = sel ? fabsf(sinf(g->time * 6.0f)) * 2.0f : 0;
        gfx_round_rect(r, 50 - (sel ? 3 : 0), y - (sel ? 3 : 0), 380 + (sel ? 6 : 0), 32 + (sel ? 6 : 0), 14, sel ? 0xFFFFFF : 0x37474F);
        gfx_round_rect(r, 52, y + 2, 376, 28, 12, sel ? gfx_lighten(s->col, 95) : gfx_lighten(s->col, 60));
        gfx_blob(r, 76, y + 16 - (int)bob, 10, PET_COL[(g->sec + i) % 6], sel ? 1 : 0, 1, 1, 0, 0);
        gfx_text(r, entry_name(&s->e[i], g->lang), 100, y + 9, 20, 0x1B1B2F);
    }
    static const Hint HL[2] = { {3, "Elegir"}, {2, "Atrás"} };
    static const Hint HLE[2] = { {3, "Choose"}, {2, "Back"} };
    draw_hints(r, 258, g->lang == LANG_ES ? HL : HLE, 2);
}

/* ------------------------------------------------------------------ ajustes guardados */
static void settings_path(char *out, size_t n) {
    char *base = SDL_GetPrefPath("notappss", "bolitas-edu");
    out[0] = 0;
    if (!base) return;
    snprintf(out, n, "%ssave.dat", base);
    SDL_free(base);
}

static void settings_load(Game *g) {
    char path[600];
    settings_path(path, sizeof path);
    if (!path[0]) return;
    FILE *f = fopen(path, "rb");
    if (!f) return;
    unsigned char d[4];
    if (fread(d, 1, 4, f) == 4 && d[0] == 'B' && d[1] == 'E') {
        g->lang = d[2] ? LANG_EN : LANG_ES;
        g->upper = d[3] ? 1 : 0;
    }
    fclose(f);
}

static void settings_save(const Game *g) {
    if (g->smoke) return;
    char path[600];
    settings_path(path, sizeof path);
    if (!path[0]) return;
    FILE *f = fopen(path, "wb");
    if (!f) return;
    unsigned char d[4] = { 'B', 'E', (unsigned char)(g->lang == LANG_EN), (unsigned char)g->upper };
    fwrite(d, 1, 4, f);
    fclose(f);
}

/* ------------------------------------------------------------------ acciones */
static void open_entry(Game *g, const Entry *e) {
    sfx_pop();
    if (e->kind == K_QUIZ) quiz_start(g, e->mg);
    else if (e->kind == K_RHYTHM) rhythm_start(g);
    else paint_start(g);
}

static void act(Game *g, Act a) {
    if (a == A_CASE) { g->upper = !g->upper; sfx_pop(); settings_save(g); return; }
    switch (g->state) {
    case ST_HOME:
        switch (a) {
        case A_LEFT:  g->home_sel = (g->home_sel + N_SECTIONS - 1) % N_SECTIONS; sfx_pop(); break;
        case A_RIGHT: g->home_sel = (g->home_sel + 1) % N_SECTIONS; sfx_pop(); break;
        case A_UP:    g->home_sel = g->home_sel >= 3 ? g->home_sel - 3 : g->home_sel; sfx_pop(); break;
        case A_DOWN:  g->home_sel = g->home_sel < 3 ? (g->home_sel + 3 > 4 ? 4 : g->home_sel + 3) : g->home_sel; sfx_pop(); break;
        case A_OK: case A_CRO:
            g->sec = g->home_sel; g->list_sel = 0; g->state = ST_LIST; sfx_pop(); break;
        case A_LANG: case A_L: case A_R:
            g->lang = g->lang == LANG_ES ? LANG_EN : LANG_ES; sfx_pop(); settings_save(g); break;
        case A_BACK: g->quit = 1; break;
        default: break;
        }
        break;
    case ST_LIST: {
        const Section *s = &SECS[g->sec];
        switch (a) {
        case A_UP:   if (s->n) g->list_sel = (g->list_sel + s->n - 1) % s->n; sfx_pop(); break;
        case A_DOWN: if (s->n) g->list_sel = (g->list_sel + 1) % s->n; sfx_pop(); break;
        case A_OK: case A_CRO: if (s->n) open_entry(g, &s->e[g->list_sel]); else sfx_bad(); break;
        case A_LANG: case A_L: case A_R:
            g->lang = g->lang == LANG_ES ? LANG_EN : LANG_ES; sfx_pop(); settings_save(g); break;
        case A_BACK: case A_CIR: g->state = ST_HOME; sfx_pop(); break;
        default: break;
        }
        break; }
    case ST_QUIZ: case ST_RHYTHM: {
        int d = -1;
        switch (a) { /* cruceta y botones del rombo apuntan a la misma opción: izq/cuadrado, arriba/triángulo... */
        case A_LEFT: case A_SQR: case A_D0: d = 0; break;
        case A_UP: case A_TRI: case A_D1: d = 1; break;
        case A_RIGHT: case A_CIR: case A_D2: d = 2; break;
        case A_DOWN: case A_CRO: case A_D3: d = 3; break;
        case A_BACK: g->state = ST_HOME + 1; sfx_pop(); return;
        default: break;
        }
        if (d >= 0) { if (g->state == ST_QUIZ) quiz_answer(g, d); else rhythm_press(g, d); }
        break; }
    case ST_PAINT:
        switch (a) {
        case A_LEFT: case A_L: g->pt.color = (g->pt.color + edu_color_count() - 1) % edu_color_count(); sfx_pop(); break;
        case A_RIGHT: case A_R: g->pt.color = (g->pt.color + 1) % edu_color_count(); sfx_pop(); break;
        case A_UP: paint_move_cursor(&g->pt, -1); break;
        case A_DOWN: paint_move_cursor(&g->pt, 1); break;
        case A_OK: case A_CRO: paint_do_fill(g, g->pt.cursor); paint_move_cursor(&g->pt, 1); break;
        case A_TRI: paint_next_drawing(&g->pt); sfx_pop(); break;
        case A_SQR: paint_clear(&g->pt); sfx_pop(); break;
        case A_BACK: g->state = ST_LIST; sfx_pop(); break;
        default: break;
        }
        break;
    }
}

static int in_rect(int x, int y, int rx, int ry, int rw, int rh) { return x >= rx && x < rx + rw && y >= ry && y < ry + rh; }

static void on_pointer(Game *g, int x, int y) {
    if (in_rect(x, y, W - 52, 4, 48, 24)) { act(g, A_CASE); return; }
    if ((g->state == ST_HOME || g->state == ST_LIST) && in_rect(x, y, W - 100, 4, 44, 24)) { act(g, A_LANG); return; }
    if (g->state != ST_HOME && in_rect(x, y, 4, 4, 28, 24)) { act(g, g->state == ST_LIST ? A_CIR : A_BACK); return; }
    switch (g->state) {
    case ST_HOME:
        for (int i = 0; i < N_SECTIONS; i++)
            if (in_rect(x, y, TILE_X[i], TILE_Y[i], TILE_W, TILE_H)) { g->home_sel = i; act(g, A_OK); return; }
        break;
    case ST_LIST:
        for (int i = 0; i < SECS[g->sec].n; i++)
            if (in_rect(x, y, 50, 46 + i * 36, 380, 32)) { g->list_sel = i; act(g, A_OK); return; }
        break;
    case ST_QUIZ: case ST_RHYTHM:
        for (int i = 0; i < 4; i++) {
            int hit = g->state == ST_QUIZ ? in_rect(x, y, BX[i], BY[i], BW, BH)
                                          : (x - PADX[i]) * (x - PADX[i]) + (y - PADY[i]) * (y - PADY[i]) <= 28 * 28;
            if (hit) { act(g, (Act)(A_D0 + i)); return; }
        }
        break;
    case ST_PAINT: {
        for (int i = 0; i < edu_color_count(); i++) {
            int sx, sy, sw, sh;
            swatch_rect(i, &sx, &sy, &sw, &sh);
            if (in_rect(x, y, sx, sy, sw, sh)) { g->pt.color = i; sfx_pop(); return; }
        }
        if (in_rect(x, y, 14, 238, 124, 26)) { act(g, A_TRI); return; }
        if (in_rect(x, y, 146, 238, 104, 26)) { act(g, A_SQR); return; }
        if (in_rect(x, y, 258, 238, 100, 26)) { paint_do_fill(g, g->pt.cursor); return; }
        if (in_rect(x, y, PX0, PY0, PAINT_W, PAINT_H_)) {
            int hit = paint_hit(paint_drawing(g->pt.drawing), x - PX0, y - PY0);
            if (hit >= 0) { g->pt.cursor = hit; paint_do_fill(g, hit); }
        }
        break; }
    }
}

static void game_update(Game *g, float dt) {
    g->time += dt;
    parts_update(dt);
    if (g->state == ST_QUIZ) quiz_update(g, dt);
    else if (g->state == ST_RHYTHM) rhythm_update(g, dt);
    else if (g->state == ST_PAINT && g->done_t > 0) g->done_t -= dt;
}

static void game_draw(Game *g, SDL_Renderer *r) {
    gfx_begin(r);
    switch (g->state) {
    case ST_HOME: draw_home(g, r); break;
    case ST_LIST: draw_list(g, r); break;
    case ST_QUIZ: draw_quiz(g, r); break;
    case ST_RHYTHM: draw_rhythm(g, r); break;
    default: draw_paint(g, r); break;
    }
}

/* ------------------------------------------------------------------ entrada */
static void on_key(Game *g, SDL_Keycode k) {
    switch (k) {
    case SDLK_LEFT: act(g, A_LEFT); break;
    case SDLK_RIGHT: act(g, A_RIGHT); break;
    case SDLK_UP: act(g, A_UP); break;
    case SDLK_DOWN: act(g, A_DOWN); break;
    case SDLK_1: case SDLK_z: act(g, A_D0); break;
    case SDLK_2: case SDLK_x: act(g, A_D1); break;
    case SDLK_3: case SDLK_c: act(g, A_D2); break;
    case SDLK_4: case SDLK_v: act(g, A_D3); break;
    case SDLK_RETURN: case SDLK_SPACE: act(g, A_OK); break;
    case SDLK_ESCAPE: case SDLK_AC_BACK: act(g, A_BACK); break;
    case SDLK_TAB: act(g, A_CASE); break;
    case SDLK_l: act(g, A_LANG); break;
    case SDLK_n: act(g, A_TRI); break;
    case SDLK_BACKSPACE: case SDLK_DELETE: act(g, A_SQR); break;
    default: break;
    }
}

static void on_joy(Game *g, int b) {
#ifdef PSP_LIKE_PAD
    /* 0 triangulo, 1 circulo, 2 cruz, 3 cuadrado, 4 L, 5 R, 6 abajo, 7 izq, 8 arriba, 9 der, 10 select, 11 start */
    switch (b) {
    case 0: act(g, A_TRI); break;
    case 1: act(g, A_CIR); break;
    case 2: act(g, A_CRO); break;
    case 3: act(g, A_SQR); break;
    case 4: act(g, A_L); break;
    case 5: act(g, A_R); break;
    case 6: act(g, A_DOWN); break;
    case 7: act(g, A_LEFT); break;
    case 8: act(g, A_UP); break;
    case 9: act(g, A_RIGHT); break;
    case 10: act(g, A_CASE); break;
    case 11: act(g, A_BACK); break;
    default: break;
    }
#else
    if (b >= 0 && b <= 3) act(g, (Act)(A_D0 + b));
    else if (b == 6 || b == 7) act(g, A_BACK);
#endif
}

/* ------------------------------------------------------------------ prueba automática */
static void save_shot(SDL_Renderer *ren, const char *name) {
    int ow = W, oh = H;
    SDL_GetRendererOutputSize(ren, &ow, &oh);
    SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, ow, oh, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!s) return;
    if (SDL_RenderReadPixels(ren, NULL, SDL_PIXELFORMAT_ARGB8888, s->pixels, s->pitch) == 0)
        SDL_SaveBMP(s, name);
    SDL_FreeSurface(s);
}

#define SMOKE_FAIL(...) do { printf("FALLO: " __VA_ARGS__); printf("\n"); return 1; } while (0)

static void step(Game *g, int frames) { for (int k = 0; k < frames; k++) game_update(g, 0.016f); }

static int smoke_quiz(Game *g, SDL_Renderer *ren, MiniGame mg) {
    char name[64];
    quiz_start(g, mg);
    step(g, 30);
    for (int round = 0; round < 8; round++) {
        if (round == 1) { /* un fallo primero: debe marcarse y no avanzar */
            int w = (g->r.correct + 1) % 4;
            quiz_answer(g, w);
            if (!g->missed || !(g->wrong_mask & (1u << w)) || g->good_t > 0) SMOKE_FAIL("fallo no registrado (%d)", (int)mg);
            game_draw(g, ren);
            snprintf(name, sizeof name, "shot_q%d_oops.bmp", (int)mg);
            save_shot(ren, name);
        }
        if (round == 2) g->upper = 0; /* probar también minúsculas */
        game_draw(g, ren);
        if (round == 0 || round == 2) {
            snprintf(name, sizeof name, round == 0 ? "shot_q%d.bmp" : "shot_q%d_min.bmp", (int)mg);
            save_shot(ren, name);
        }
        quiz_answer(g, g->r.correct);
        if (g->good_t <= 0) SMOKE_FAIL("acierto no registrado (%d)", (int)mg);
        step(g, 70);
        if (g->good_t > 0) SMOKE_FAIL("la ronda no avanza (%d)", (int)mg);
    }
    g->upper = 1;
    if (g->rounds != 8) SMOKE_FAIL("rondas=%d", g->rounds);
    printf("ok  %-20s nivel=%d aciertos=%d\n", edu_game_name(mg, LANG_ES), g->prog.level, g->score);
    return 0;
}

static int smoke_rhythm(Game *g, SDL_Renderer *ren) {
    rhythm_start(g);
    int shots = 0, played = 0;
    for (int f = 0; f < 4000 && g->rh.wins < 3; f++) {
        game_update(g, 0.016f);
        Rhythm *h = &g->rh;
        if (h->phase == RH_PLAY && h->next < h->len && h->t >= rh_slot_time(h->next)) {
            rhythm_press(g, h->seq[h->next]);
            played++;
        }
        if (!(shots & 1) && h->phase == RH_LISTEN && h->t > 1.5f) { game_draw(g, ren); save_shot(ren, "shot_rhythm_listen.bmp"); shots |= 1; }
        if (!(shots & 2) && h->phase == RH_PLAY && h->next >= 1) { game_draw(g, ren); save_shot(ren, "shot_rhythm_play.bmp"); shots |= 2; }
        if (!(shots & 4) && h->phase == RH_RESULT) { game_draw(g, ren); save_shot(ren, "shot_rhythm_result.bmp"); shots |= 4; }
    }
    if (g->rh.wins < 3) SMOKE_FAIL("el ritmo no avanza (wins=%d, jugadas=%d)", g->rh.wins, played);
    printf("ok  ritmo                wins=%d nivel=%d\n", g->rh.wins, g->rh.level);
    return 0;
}

static int smoke_paint(Game *g, SDL_Renderer *ren) {
    char name[64];
    paint_start(g);
    for (int d = 0; d < PAINT_DRAWINGS; d++) {
        const Drawing *dr = paint_drawing(g->pt.drawing);
        for (int i = 0; i < dr->n; i++) {
            g->pt.color = (i + d) % edu_color_count();
            paint_do_fill(g, i);
            if (i == dr->n / 2) { game_draw(g, ren); snprintf(name, sizeof name, "shot_paint%d_half.bmp", d); save_shot(ren, name); }
        }
        step(g, 20);
        if (!paint_complete(&g->pt)) SMOKE_FAIL("dibujo %d incompleto", d);
        game_draw(g, ren);
        snprintf(name, sizeof name, "shot_paint%d_done.bmp", d);
        save_shot(ren, name);
        paint_next_drawing(&g->pt);
    }
    printf("ok  colorear\n");
    return 0;
}

static int run_smoke(SDL_Renderer *ren, Game *g) {
    g->state = ST_HOME;
    game_draw(g, ren);
    save_shot(ren, "shot_home.bmp");
    g->sec = 0; g->state = ST_LIST;
    game_draw(g, ren);
    save_shot(ren, "shot_list.bmp");
    g->sec = 4; g->state = ST_LIST;
    game_draw(g, ren);
    save_shot(ren, "shot_list_soon.bmp");
    /* clics sobre el menú */
    g->state = ST_HOME;
    on_pointer(g, TILE_X[1] + 10, TILE_Y[1] + 10);
    if (g->state != ST_LIST || g->sec != 1) SMOKE_FAIL("clic en seccion");
    on_pointer(g, 100, 46 + 36 + 5);
    if (g->state != ST_QUIZ || g->mg != MG_LETRA) SMOKE_FAIL("clic en juego (%d)", (int)g->mg);
    on_pointer(g, 10, 10);
    if (g->state != ST_LIST) SMOKE_FAIL("boton atras");

    for (int mg = 0; mg < MG_COUNT; mg++)
        if (smoke_quiz(g, ren, (MiniGame)mg)) return 1;
    if (smoke_rhythm(g, ren)) return 1;
    if (smoke_paint(g, ren)) return 1;

    g->lang = LANG_EN;
    quiz_start(g, MG_VOCAB);
    game_draw(g, ren);
    save_shot(ren, "shot_en.bmp");
    return 0;
}

/* ------------------------------------------------------------------ main */
int main(int argc, char **argv) {
    int smoke = argc > 1 && strcmp(argv[1], "--smoke") == 0;
    if (smoke) setvbuf(stdout, NULL, _IONBF, 0); /* que el log de CI no se pierda si algo falla */
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
#elif defined(__vita__)
    ww = 960; wh = 544;
#else
    wflags |= SDL_WINDOW_RESIZABLE;
#endif
    SDL_Window *win = SDL_CreateWindow("Bolitas Edu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, ww, wh, wflags);
    if (!win) { fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError()); return 1; }
    SDL_Renderer *ren = NULL;
    if (!smoke) ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    if (!ren) { fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError()); return 1; }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"); /* las letras escaladas salen suaves, no con dientes */
    if (SDL_NumJoysticks() > 0) SDL_JoystickOpen(0);
    audio_init();

    static Game g; /* estático: cero al empezar */
    memset(&g, 0, sizeof g);
    g.smoke = smoke;
    g.lang = LANG_ES;
    g.upper = 1; /* mayúsculas por defecto: letra de palo para empezar a leer */
    rng_seed(&g.rng, smoke ? 1234u : (SDL_GetTicks() | 1u));
    progress_init(&g.prog);
    if (!smoke) settings_load(&g);

    if (smoke) {
        int rc = run_smoke(ren, &g);
        audio_close();
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
            case SDL_MOUSEBUTTONDOWN: {
                int lx, ly;
                gfx_to_logical(e.button.x, e.button.y, &lx, &ly);
                on_pointer(&g, lx, ly);
                break; }
            default: break;
            }
        }
        Uint32 now = SDL_GetTicks();
        float dt = (float)(now - last) / 1000.0f;
        last = now;
        if (dt > 0.05f) dt = 0.05f;
        game_update(&g, dt);
        game_draw(&g, ren);
        SDL_RenderPresent(ren);
        if (dt < 0.012f) SDL_Delay(8);
    }
    settings_save(&g);
    audio_close();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
