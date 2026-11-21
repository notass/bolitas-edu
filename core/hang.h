/* Ahorcado amable: en vez de una horca, la bolita tiene globos y pierde uno por fallo. */
#ifndef HANG_H
#define HANG_H
#include "edu.h"

#define HANG_LIVES 6
#define HANG_LETTERS 27           /* A-Z y Ñ (índice 26) */

typedef struct {
    char word[16];                /* minúsculas a-z */
    unsigned guessed;             /* bit i = letra i probada */
    int wrong;                    /* fallos */
    int state;                    /* 0 jugando, 1 ganada, -1 perdida */
} Hang;

enum { HANG_PLAYING = 0, HANG_WON = 1, HANG_LOST = -1 };
enum { HANG_BAD = -1, HANG_REPEAT = 0, HANG_HIT = 1 };

void hang_init(Hang *h, Lang lang, int level, Rng *g);
void hang_set_word(Hang *h, const char *word);            /* para pruebas */
int  hang_guess(Hang *h, int letter);                     /* letra 0..25 (A-Z), 26 = Ñ */
int  hang_revealed(const Hang *h, int pos);               /* 1 si la letra de esa posición ya se ve */
int  hang_len(const Hang *h);
char hang_letter_char(int letter);                        /* 'A'..'Z', 'N' para Ñ en la tabla ASCII */
#endif
