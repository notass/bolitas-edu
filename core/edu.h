/* Núcleo educativo portable (C99, sin dependencias, sin malloc).
 * Pensado para PSP original (32 MB RAM) y Android (SDL2). */
#ifndef EDU_H
#define EDU_H

#include <stdint.h>

#define EDU_OPTIONS 4
#define EDU_LEVELS  3

typedef enum { LANG_ES = 0, LANG_EN = 1 } Lang;

typedef enum {
    MG_SUMAS,    /* sumas             */
    MG_RESTAS,   /* restas            */
    MG_CONTAR,   /* contar bolitas    */
    MG_SERIES,   /* series numéricas  */
    MG_SILABAS,  /* sílabas -> lectura */
    MG_VOCAB,    /* vocabulario ES/EN */
    MG_HORA,     /* leer el reloj     */
    MG_COUNT
} MiniGame;

typedef struct { uint32_t state; } Rng;
void     rng_seed(Rng *g, uint32_t seed);
uint32_t rng_next(Rng *g);
int      rng_range(Rng *g, int lo, int hi); /* inclusivo */

/* Una pregunta: el render decide cómo dibujarla (tambores, bolitas...). */
typedef struct {
    MiniGame game;
    int  level;                       /* 1..EDU_LEVELS */
    char prompt[64];                  /* texto de la pregunta */
    char option[EDU_OPTIONS][24];     /* una opción por tambor/botón */
    int  correct;                     /* índice 0..3 de la correcta */
    int  visual_a, visual_b;          /* datos para dibujar: nº de bolitas, hora/minutos... */
} Round;

void        edu_make_round(Round *r, MiniGame game, int level, Lang lang, Rng *g);
const char *edu_game_name(MiniGame game, Lang lang);

/* Dificultad adaptable: sube tras 3 aciertos seguidos, baja tras 2 fallos. */
typedef struct { int level, streak_ok, streak_bad; } Progress;
void progress_init(Progress *p);
void progress_answer(Progress *p, int ok);

#endif
