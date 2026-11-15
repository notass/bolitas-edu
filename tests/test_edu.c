#include "../core/edu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { fails++; if (fails < 10) { printf("FALLO: " __VA_ARGS__); printf("\n"); } } } while (0)

int main(void) {
    Rng g; rng_seed(&g, 12345);
    long rounds = 0;
    for (int game = 0; game < MG_COUNT; game++)
    for (int lang = 0; lang < 2; lang++)
    for (int level = 1; level <= EDU_LEVELS; level++)
    for (int it = 0; it < 3000; it++, rounds++) {
        Round r; edu_make_round(&r, (MiniGame)game, level, (Lang)lang, &g);
        CHECK(r.correct >= 0 && r.correct < EDU_OPTIONS, "indice correcta %d", r.correct);
        CHECK(r.prompt[0], "prompt vacio (juego %d)", game);
        for (int i = 0; i < EDU_OPTIONS; i++) {
            CHECK(r.option[i][0], "opcion vacia (juego %d)", game);
            for (int j = i + 1; j < EDU_OPTIONS; j++)
                CHECK(strcmp(r.option[i], r.option[j]) != 0, "opciones repetidas '%s' (juego %d)", r.option[i], game);
        }
        int a, b, c;
        switch (game) {
        case MG_SUMAS:
            CHECK(sscanf(r.prompt, "%d + %d", &a, &b) == 2 && atoi(r.option[r.correct]) == a + b, "suma: %s", r.prompt);
            break;
        case MG_RESTAS:
            CHECK(sscanf(r.prompt, "%d - %d", &a, &b) == 2 && b <= a && atoi(r.option[r.correct]) == a - b, "resta: %s", r.prompt);
            break;
        case MG_CONTAR:
            CHECK(atoi(r.option[r.correct]) == r.visual_a, "contar");
            break;
        case MG_SERIES:
            CHECK(sscanf(r.prompt, "%d, %d, %d", &a, &b, &c) == 3 && atoi(r.option[r.correct]) == c + (c - b), "serie: %s", r.prompt);
            break;
        case MG_SILABAS: {
            int us = 0; for (const char *p = r.prompt; *p; p++) us += (*p == '_');
            CHECK(us == 1, "silaba: %s", r.prompt);
            break; }
        case MG_HORA: {
            char buf[24]; snprintf(buf, sizeof buf, "%d:%02d", r.visual_a, r.visual_b);
            CHECK(strcmp(buf, r.option[r.correct]) == 0, "hora");
            break; }
        case MG_MAYOR: {
            int best = 0, v[4];
            for (int i = 0; i < 4; i++) v[i] = atoi(r.option[i]);
            for (int i = 1; i < 4; i++) if (r.visual_a ? v[i] > v[best] : v[i] < v[best]) best = i;
            CHECK(r.correct == best, "mayor/menor: %s", r.prompt);
            break; }
        case MG_COLORES:
            CHECK(strcmp(r.option[r.correct], edu_color_name(r.visual_a, (Lang)lang)) == 0, "color");
            CHECK(r.visual_a < (level == 1 ? 4 : (level == 2 ? 6 : 8)), "color fuera del nivel");
            break;
        case MG_FORMAS:
            CHECK(strcmp(r.option[r.correct], edu_shape_name(r.visual_a, (Lang)lang)) == 0, "figura");
            CHECK(r.visual_a < level + 2, "figura fuera del nivel");
            break;
        case MG_LETRA: {
            char w[32]; int len;
            CHECK(sscanf(strchr(r.prompt, '"'), "\"%31[^\"]\"", w) == 1, "letra: %s", r.prompt);
            len = (int)strlen(w);
            char c = (char)((r.visual_a ? w[len - 1] : w[0]) - 'a' + 'A');
            CHECK(r.option[r.correct][0] == c && r.option[r.correct][1] == 0, "letra %c: %s", c, r.prompt);
            break; }
        case MG_DIAS: {
            static const char *D[2][7] = {
                {"lunes","martes","miércoles","jueves","viernes","sábado","domingo"},
                {"Monday","Tuesday","Wednesday","Thursday","Friday","Saturday","Sunday"} };
            static const char *M[2][12] = {
                {"enero","febrero","marzo","abril","mayo","junio","julio","agosto","septiembre","octubre","noviembre","diciembre"},
                {"January","February","March","April","May","June","July","August","September","October","November","December"} };
            int months = r.visual_b >> 1, after = r.visual_b & 1, n = months ? 12 : 7;
            const char *base = months ? M[lang][r.visual_a] : D[lang][r.visual_a];
            const char *want = months ? M[lang][(r.visual_a + (after ? 1 : n - 1)) % n]
                                      : D[lang][(r.visual_a + (after ? 1 : n - 1)) % n];
            CHECK(strstr(r.prompt, base) != NULL, "dias: falta '%s' en %s", base, r.prompt);
            CHECK(strcmp(r.option[r.correct], want) == 0, "dias: %s -> %s", r.prompt, r.option[r.correct]);
            CHECK(months == (level == 3), "dias/meses por nivel");
            break; }
        default: break;
        }
    }
    /* dificultad adaptable */
    Progress p; progress_init(&p);
    for (int i = 0; i < 3; i++) progress_answer(&p, 1);
    CHECK(p.level == 2, "sube de nivel");
    progress_answer(&p, 0); progress_answer(&p, 0);
    CHECK(p.level == 1, "baja de nivel");

    /* muestra */
    Rng s; rng_seed(&s, 7);
    for (int game = 0; game < MG_COUNT; game++) {
        Round r; edu_make_round(&r, (MiniGame)game, 2, LANG_ES, &s);
        printf("[%s] %s  ->  %s | %s | %s | %s  (correcta: %s)\n", edu_game_name((MiniGame)game, LANG_ES),
               r.prompt, r.option[0], r.option[1], r.option[2], r.option[3], r.option[r.correct]);
    }
    printf("%ld rondas probadas, %d fallos\n", rounds, fails);
    return fails ? 1 : 0;
}
