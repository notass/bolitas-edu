#include "hang.h"
#include <string.h>

void hang_set_word(Hang *h, const char *word) {
    memset(h, 0, sizeof *h);
    strncpy(h->word, word, sizeof h->word - 1);
}

void hang_init(Hang *h, Lang lang, int level, Rng *g) {
    char w[16];
    edu_pick_word(w, sizeof w, lang, level, g);
    hang_set_word(h, w);
}

int hang_len(const Hang *h) { return (int)strlen(h->word); }

int hang_revealed(const Hang *h, int pos) {
    if (h->state != HANG_PLAYING) return 1;      /* al terminar se enseña la palabra */
    int c = h->word[pos] - 'a';
    return c >= 0 && c < 26 && (h->guessed & (1u << c));
}

char hang_letter_char(int letter) { return letter == 26 ? 'N' : (char)('A' + letter); }

int hang_guess(Hang *h, int letter) {
    if (h->state != HANG_PLAYING || letter < 0 || letter >= HANG_LETTERS) return HANG_REPEAT;
    if (h->guessed & (1u << letter)) return HANG_REPEAT;
    h->guessed |= 1u << letter;
    int hit = 0;
    if (letter < 26) for (const char *p = h->word; *p; p++) if (*p - 'a' == letter) hit = 1;
    if (!hit) {
        h->wrong++;
        if (h->wrong >= HANG_LIVES) h->state = HANG_LOST;
        return HANG_BAD;
    }
    int all = 1;
    for (const char *p = h->word; *p; p++) if (!(h->guessed & (1u << (*p - 'a')))) all = 0;
    if (all) h->state = HANG_WON;
    return HANG_HIT;
}
