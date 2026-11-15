/* Núcleo educativo portable (C99, sin dependencias, sin malloc).
 * Pensado para PSP original (32 MB RAM) y Android (SDL2). */
#ifndef EDU_H
#define EDU_H

#include <stddef.h>
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
    MG_MAYOR,    /* número mayor/menor */
    MG_LETRA,    /* letra inicial/final */
    MG_COLORES,  /* nombre del color  */
    MG_FORMAS,   /* nombre de la figura */
    MG_DIAS,     /* días y meses      */
    MG_COUNT
} MiniGame;

/* Convierte texto UTF-8 a mayúsculas (upper=1) o minúsculas (upper=0), incluidas
 * á é í ó ú ü ñ. Para alternar cómo se muestra el contenido de aprendizaje. */
void edu_set_case(char *dst, size_t cap, const char *src, int upper);

/* Colores (8) y figuras (5): tablas bilingües compartidas con el dibujo. */
int         edu_color_count(void);
unsigned    edu_color_rgb(int i);                 /* 0xRRGGBB */
const char *edu_color_name(int i, Lang lang);
int         edu_shape_count(void);
const char *edu_shape_name(int i, Lang lang);     /* 0 círculo 1 cuadrado 2 triángulo 3 rectángulo 4 rombo */

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
