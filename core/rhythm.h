/* Marcha rítmica (estilo Patapon, simplificado y educativo).
 * Un "tamborilero" toca una secuencia de tambores (un patrón: A A B, A B A B...)
 * y después la niña la repite al compás. Si lo hace bien, el ejército avanza.
 * Sin dependencias de SDL: todo el tiempo entra por rh_update(dt). */
#ifndef RHYTHM_H
#define RHYTHM_H

#include "edu.h"

#define RH_MAX 6

typedef enum { RH_LISTEN = 0, RH_PLAY = 1, RH_RESULT = 2 } RhPhase;

/* Resultado de cada nota */
enum { RH_MISS = -1, RH_PENDING = 0, RH_GOOD = 1, RH_PERFECT = 2 };

/* Ventanas de tiempo (segundos) y compás */
#define RH_BEAT         0.70f   /* ~86 pulsos por minuto: tranquilo para 6 años */
#define RH_WIN_GOOD     0.28f
#define RH_WIN_PERFECT  0.12f

typedef struct {
    Rng *rng;
    int level;                 /* 1..3 */
    int len, seq[RH_MAX];      /* secuencia de tambores 0..3 */
    RhPhase phase;
    float t;                   /* segundos dentro de la fase */
    int next;                  /* PLAY: siguiente nota por resolver */
    int res[RH_MAX];
    int listen_fired;          /* notas del tamborilero ya sonadas */
    int success;               /* resultado de la última ronda: 1 bien, 0 mal */
    int streak_ok, streak_bad;
    int wins, score;

    /* eventos de la última llamada a rh_update / rh_press (para sonido y animación) */
    unsigned ev_notes;         /* bit i: sonó la nota i del tamborilero */
    int ev_result;             /* +1 ronda bien, -1 ronda mal, 0 nada */
    int ev_miss;               /* 1 si se perdió una nota por no pulsar */
} Rhythm;

void rh_init(Rhythm *h, Rng *rng);
void rh_update(Rhythm *h, float dt);
/* Pulsar tambor 0..3. Devuelve RH_PERFECT / RH_GOOD / RH_MISS, o 0 si se ignora. */
int  rh_press(Rhythm *h, int drum);

/* Instante (en s, desde el inicio de la fase) en que toca la nota i */
float rh_slot_time(int i);
/* Duración de las fases de la secuencia actual */
float rh_listen_len(const Rhythm *h);
float rh_play_len(const Rhythm *h);

#endif
