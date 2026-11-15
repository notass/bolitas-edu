#include "rhythm.h"
#include <string.h>

#define RESULT_TIME 1.6f
#define START_TIME  0.5f

float rh_slot_time(int i) { return (float)(i + 1) * RH_BEAT; }
float rh_listen_len(const Rhythm *h) { return (float)(h->len + 1) * RH_BEAT; }
float rh_play_len(const Rhythm *h) { return rh_slot_time(h->len - 1) + RH_WIN_GOOD + 0.05f; }

/* Patrones por nivel: 0/1/2 = tambor A/B/C. Enseñan a reconocer y repetir patrones. */
static const char *PATTERNS[3][5] = {
    { "001", "011", "010", "001", "011" },
    { "0001", "0101", "0110", "0011", "0111" },
    { "00101", "01012", "00112", "01201", "01102" },
};

static void new_sequence(Rhythm *h) {
    const char *pat = PATTERNS[h->level - 1][rng_range(h->rng, 0, 4)];
    int sym[3], n = 0;
    while (n < 3) { /* tres tambores distintos */
        int d = rng_range(h->rng, 0, 3), dup = 0;
        for (int i = 0; i < n; i++) if (sym[i] == d) dup = 1;
        if (!dup) sym[n++] = d;
    }
    h->len = (int)strlen(pat);
    for (int i = 0; i < h->len; i++) h->seq[i] = sym[pat[i] - '0'];
}

static void start_listen(Rhythm *h) {
    h->phase = RH_LISTEN;
    h->t = -START_TIME; /* pequeña pausa antes de empezar */
    h->listen_fired = 0;
    h->next = 0;
    memset(h->res, 0, sizeof h->res);
}

void rh_init(Rhythm *h, Rng *rng) {
    memset(h, 0, sizeof *h);
    h->rng = rng;
    h->level = 1;
    new_sequence(h);
    start_listen(h);
}

static void finish_round(Rhythm *h) {
    int ok = 1;
    for (int i = 0; i < h->len; i++) if (h->res[i] <= 0) ok = 0;
    h->success = ok;
    h->ev_result = ok ? 1 : -1;
    h->phase = RH_RESULT;
    h->t = 0;
    if (ok) {
        h->wins++;
        h->streak_bad = 0;
        for (int i = 0; i < h->len; i++) h->score += h->res[i];
        if (++h->streak_ok >= 2 && h->level < 3) { h->level++; h->streak_ok = 0; }
    } else {
        h->streak_ok = 0;
        if (++h->streak_bad >= 2 && h->level > 1) { h->level--; h->streak_bad = 0; }
    }
}

void rh_update(Rhythm *h, float dt) {
    h->ev_notes = 0;
    h->ev_result = 0;
    h->ev_miss = 0;
    h->t += dt;
    switch (h->phase) {
    case RH_LISTEN:
        while (h->listen_fired < h->len && h->t >= rh_slot_time(h->listen_fired)) {
            h->ev_notes |= 1u << h->listen_fired;
            h->listen_fired++;
        }
        /* la fase de juego empieza justo un pulso después de la última nota del tamborilero,
         * así toda la secuencia queda en la misma cuadrícula de compás */
        if (h->t >= rh_listen_len(h)) { h->phase = RH_PLAY; h->t = 0; h->next = 0; }
        break;
    case RH_PLAY:
        while (h->next < h->len && h->t > rh_slot_time(h->next) + RH_WIN_GOOD) {
            h->res[h->next++] = RH_MISS;
            h->ev_miss = 1;
        }
        if (h->next >= h->len) finish_round(h);
        break;
    case RH_RESULT:
        if (h->t >= RESULT_TIME) {
            if (h->success) new_sequence(h);  /* si falló, se repite la misma secuencia */
            else if (h->len != (int)strlen(PATTERNS[h->level - 1][0])) new_sequence(h); /* bajó de nivel */
            start_listen(h);
        }
        break;
    }
}

int rh_press(Rhythm *h, int drum) {
    h->ev_result = 0;
    if (h->phase != RH_PLAY || h->next >= h->len || drum < 0 || drum > 3) return 0;
    int i = h->next;
    float d = h->t - rh_slot_time(i);
    if (d < -RH_WIN_GOOD) return 0;            /* demasiado pronto: no cuenta */
    int r;
    if (drum != h->seq[i]) r = RH_MISS;
    else r = (d <= RH_WIN_PERFECT && d >= -RH_WIN_PERFECT) ? RH_PERFECT : RH_GOOD;
    h->res[i] = r;
    h->next++;
    if (h->next >= h->len) finish_round(h);
    return r;
}
