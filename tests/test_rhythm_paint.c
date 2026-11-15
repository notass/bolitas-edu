#include "../core/rhythm.h"
#include "../core/paint.h"
#include <stdio.h>
#include <string.h>

static int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { fails++; if (fails < 12) { printf("FALLO: " __VA_ARGS__); printf("\n"); } } } while (0)

/* Avanza el reloj hasta la fase pedida, en pasos de 10 ms */
static void run_until(Rhythm *h, RhPhase ph) {
    for (int i = 0; i < 100000 && h->phase != ph; i++) rh_update(h, 0.01f);
}

/* Juega una ronda: offset = desfase de cada pulsación respecto al compás (s) */
static void play_round(Rhythm *h, float offset, int wrong_at, int skip_all) {
    run_until(h, RH_PLAY);
    int seq[RH_MAX], len = h->len;
    memcpy(seq, h->seq, sizeof seq);
    float tt = 0;
    int i = 0;
    while (h->phase == RH_PLAY && tt < 20) {
        rh_update(h, 0.01f);
        tt += 0.01f;
        if (!skip_all && i < len && tt >= rh_slot_time(i) + offset) {
            rh_press(h, i == wrong_at ? (seq[i] + 1) % 4 : seq[i]);
            i++;
        }
    }
}

int main(void) {
    Rng g; rng_seed(&g, 99);
    Rhythm h; rh_init(&h, &g);

    /* secuencias: longitud por nivel y tambores válidos */
    for (int lv = 1; lv <= 3; lv++) {
        for (int k = 0; k < 500; k++) {
            h.level = lv; rh_init(&h, &g); h.level = lv;
            rh_update(&h, 0.0f);
            Rhythm t; rh_init(&t, &g);
            t.level = lv;
            /* regenerar secuencia con el nivel ya fijado: jugar una ronda perfecta y mirar la siguiente */
            play_round(&t, 0.0f, -1, 0);
            if (t.level == lv && t.success) {
                run_until(&t, RH_LISTEN);
                CHECK(t.len == lv + 2, "longitud nivel %d = %d", lv, t.len);
                for (int i = 0; i < t.len; i++) CHECK(t.seq[i] >= 0 && t.seq[i] <= 3, "tambor invalido");
            }
        }
    }

    /* pulsar a tiempo y bien: éxito */
    rh_init(&h, &g);
    play_round(&h, 0.0f, -1, 0);
    CHECK(h.phase == RH_RESULT && h.success == 1, "ronda perfecta debe ganar");
    for (int i = 0; i < h.len; i++) CHECK(h.res[i] == RH_PERFECT, "nota %d deberia ser perfecta (%d)", i, h.res[i]);

    /* un poco tarde (dentro de la ventana buena): éxito pero no perfecto */
    rh_init(&h, &g);
    play_round(&h, 0.2f, -1, 0);
    CHECK(h.success == 1, "pulsar algo tarde debe valer");
    CHECK(h.res[0] == RH_GOOD, "nota algo tarde = buena (%d)", h.res[0]);

    /* demasiado tarde: fallo */
    rh_init(&h, &g);
    play_round(&h, 0.45f, -1, 0);
    CHECK(h.success == 0, "muy tarde debe fallar");

    /* tambor equivocado: fallo */
    rh_init(&h, &g);
    play_round(&h, 0.0f, 1, 0);
    CHECK(h.success == 0 && h.res[1] == RH_MISS, "tambor equivocado debe fallar");

    /* no pulsar nada: fallo, y se repite la misma secuencia */
    rh_init(&h, &g);
    int seq0[RH_MAX], len0;
    run_until(&h, RH_PLAY);
    len0 = h.len; memcpy(seq0, h.seq, sizeof seq0);
    play_round(&h, 0.0f, -1, 1);
    CHECK(h.success == 0, "sin pulsar debe fallar");
    run_until(&h, RH_LISTEN);
    CHECK(h.len == len0 && memcmp(seq0, h.seq, sizeof(int) * (size_t)len0) == 0, "tras fallar se repite la secuencia");

    /* pulsar muy pronto no cuenta ni rompe nada */
    rh_init(&h, &g);
    run_until(&h, RH_PLAY);
    CHECK(rh_press(&h, h.seq[0]) == 0, "pulsacion demasiado pronta ignorada");
    CHECK(h.next == 0, "no avanza con pulsacion temprana");
    CHECK(rh_press(&h, 7) == 0 && rh_press(&h, -1) == 0, "tambor fuera de rango");

    /* fuera de la fase de juego se ignora */
    rh_init(&h, &g);
    CHECK(rh_press(&h, 0) == 0, "pulsar al escuchar se ignora");

    /* sube de nivel tras 2 rondas ganadas, baja tras 2 falladas */
    rh_init(&h, &g);
    for (int i = 0; i < 2; i++) { play_round(&h, 0.0f, -1, 0); run_until(&h, RH_LISTEN); }
    CHECK(h.level == 2, "sube a nivel 2 (%d)", h.level);
    CHECK(h.len == 4, "secuencia de nivel 2 tiene 4 notas (%d)", h.len);
    for (int i = 0; i < 2; i++) { play_round(&h, 0.0f, -1, 1); run_until(&h, RH_LISTEN); }
    CHECK(h.level == 1 && h.len == 3, "baja a nivel 1 con secuencia nueva (%d, %d)", h.level, h.len);

    /* eventos del tamborilero: todas las notas suenan una sola vez */
    rh_init(&h, &g);
    unsigned seen = 0; int times = 0;
    while (h.phase == RH_LISTEN) { rh_update(&h, 0.01f); seen |= h.ev_notes; for (int i = 0; i < RH_MAX; i++) if (h.ev_notes >> i & 1) times++; }
    CHECK(times == h.len && seen == (1u << h.len) - 1, "notas del tamborilero (%d/%d)", times, h.len);

    /* ---- colorear ---- */
    Paint p; paint_init(&p);
    for (int d = 0; d < PAINT_DRAWINGS; d++) {
        const Drawing *dr = paint_drawing(d);
        CHECK(dr->n > 0 && dr->n <= PAINT_MAX_REGIONS, "regiones dibujo %d", d);
        /* el fondo (región 0) cubre todo el dibujo */
        CHECK(paint_hit(dr, 1, 1) == 0 && paint_hit(dr, 298, 198) >= 0, "fondo dibujo %d", d);
        /* todas las regiones deben poder tocarse en algún punto (no quedar tapadas del todo) */
        for (int i = 0; i < dr->n; i++) {
            int found = 0;
            for (int y = 0; y < PAINT_H_ && !found; y += 2)
                for (int x = 0; x < PAINT_W && !found; x += 2)
                    if (paint_hit(dr, x, y) == i) found = 1;
            CHECK(found, "dibujo %d region %d inalcanzable", d, i);
        }
    }
    /* casa: tejado, pared, puerta */
    const Drawing *casa = paint_drawing(0);
    CHECK(paint_hit(casa, 135, 60) == 6, "tejado");
    CHECK(paint_hit(casa, 100, 130) == 5, "pared");
    CHECK(paint_hit(casa, 135, 140) == 7, "puerta sobre la pared");
    CHECK(paint_hit(casa, 250, 40) == 1, "sol");
    CHECK(paint_hit(casa, 20, 180) == 3, "cesped");
    CHECK(paint_hit(casa, 20, 20) == 0, "cielo");
    /* pintar */
    p.color = 3;
    CHECK(paint_fill(&p, 5) == 1 && p.fill[0][5] == 4, "pintar pared");
    CHECK(paint_fill(&p, 5) == 0, "repetir mismo color no cambia");
    CHECK(paint_fill(&p, 99) == 0 && paint_fill(&p, -1) == 0, "region invalida");
    CHECK(paint_count_filled(&p) == 1 && !paint_complete(&p), "contador");
    for (int i = 0; i < casa->n; i++) paint_fill(&p, i);
    CHECK(paint_complete(&p), "completo");
    paint_clear(&p);
    CHECK(paint_count_filled(&p) == 0, "borrar");
    paint_next_drawing(&p); CHECK(p.drawing == 1 && p.cursor == 0, "siguiente dibujo");
    p.cursor = 0; paint_move_cursor(&p, -1);
    CHECK(p.cursor == paint_drawing(1)->n - 1, "cursor da la vuelta");

    /* mayúsculas / minúsculas con acentos y ñ */
    char cb[64];
    edu_set_case(cb, sizeof cb, "Mamá, ñandú y pingüino ¿qué?", 1);
    CHECK(strcmp(cb, "MAMÁ, ÑANDÚ Y PINGÜINO ¿QUÉ?") == 0, "a mayusculas: %s", cb);
    edu_set_case(cb, sizeof cb, "MAMÁ, ÑANDÚ Y PINGÜINO ¿QUÉ?", 0);
    CHECK(strcmp(cb, "mamá, ñandú y pingüino ¿qué?") == 0, "a minusculas: %s", cb);
    edu_set_case(cb, sizeof cb, "Monday 12:30", 1);
    CHECK(strcmp(cb, "MONDAY 12:30") == 0, "ingles: %s", cb);
    edu_set_case(cb, 6, "abcdefghij", 1);
    CHECK(strcmp(cb, "ABCDE") == 0, "recorta al tamano: %s", cb);
    edu_set_case(cb, 4, "aáb", 1); /* no parte un carácter de 2 bytes */
    CHECK(strcmp(cb, "AÁ") == 0 || strcmp(cb, "AÁB") == 0 || strcmp(cb, "A") == 0, "limite con acento: %s", cb);
    edu_set_case(cb, 1, "x", 1);
    CHECK(cb[0] == 0, "capacidad 1");

    printf("ritmo y colorear: %d fallos\n", fails);
    return fails ? 1 : 0;
}
