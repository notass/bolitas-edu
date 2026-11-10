#include "edu.h"
#include <stdio.h>
#include <string.h>

/* ---------- RNG (xorshift32) ---------- */
void rng_seed(Rng *g, uint32_t seed) { g->state = seed ? seed : 0x9E3779B9u; }

uint32_t rng_next(Rng *g) {
    uint32_t x = g->state;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return g->state = x;
}

int rng_range(Rng *g, int lo, int hi) {
    return lo + (int)(rng_next(g) % (uint32_t)(hi - lo + 1));
}

/* ---------- utilidades ---------- */
static void shuffle_correct(Round *r, Rng *g) {
    int j = rng_range(g, 0, EDU_OPTIONS - 1);
    char tmp[24];
    memcpy(tmp, r->option[0], sizeof tmp);
    memcpy(r->option[0], r->option[j], sizeof tmp);
    memcpy(r->option[j], tmp, sizeof tmp);
    r->correct = j;
}

static int has_option(const Round *r, int n, const char *s) {
    for (int i = 0; i < n; i++)
        if (strcmp(r->option[i], s) == 0) return 1;
    return 0;
}

/* option[0] = correcta; el resto, números cercanos y distintos. */
static void set_num_options(Round *r, int correct, int maxv, Rng *g) {
    int vals[EDU_OPTIONS], n = 1, guard = 0;
    vals[0] = correct;
    while (n < EDU_OPTIONS && guard++ < 200) {
        int d = rng_range(g, 1, 4);
        if (rng_range(g, 0, 1)) d = -d;
        int v = correct + d, dup = 0;
        if (v < 0 || v > maxv) continue;
        for (int i = 0; i < n; i++) if (vals[i] == v) dup = 1;
        if (!dup) vals[n++] = v;
    }
    for (int v = 0; n < EDU_OPTIONS; v++) {
        int dup = 0;
        for (int i = 0; i < n; i++) if (vals[i] == v) dup = 1;
        if (!dup) vals[n++] = v;
    }
    for (int i = 0; i < EDU_OPTIONS; i++)
        snprintf(r->option[i], sizeof r->option[i], "%d", vals[i]);
    shuffle_correct(r, g);
}

static int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

/* ---------- datos bilingües ---------- */
typedef struct { int n; const char *s[5]; } Word;

static const Word WORDS_ES[] = {
    {2, {"ca", "sa"}}, {2, {"ga", "to"}}, {2, {"lu", "na"}}, {2, {"ma", "no"}},
    {3, {"pe", "lo", "ta"}}, {3, {"to", "ma", "te"}}, {3, {"za", "pa", "to"}},
    {3, {"tor", "tu", "ga"}}, {3, {"man", "za", "na"}}, {3, {"ven", "ta", "na"}},
    {3, {"co", "ne", "jo"}}, {3, {"ca", "ba", "llo"}}, {3, {"ju", "gue", "te"}},
    {4, {"ma", "ri", "po", "sa"}}, {4, {"e", "le", "fan", "te"}},
    {4, {"te", "le", "fo", "no"}}, {4, {"bi", "ci", "cle", "ta"}},
};
static const Word WORDS_EN[] = {
    {2, {"ap", "ple"}}, {2, {"wa", "ter"}}, {2, {"ta", "ble"}}, {2, {"ba", "by"}},
    {3, {"ba", "na", "na"}}, {3, {"to", "ma", "to"}}, {3, {"po", "ta", "to"}},
    {3, {"kan", "ga", "roo"}}, {3, {"el", "e", "phant"}}, {3, {"di", "no", "saur"}},
    {3, {"but", "ter", "fly"}},
    {4, {"hel", "i", "cop", "ter"}}, {4, {"wa", "ter", "mel", "on"}},
    {4, {"cat", "er", "pil", "lar"}}, {4, {"al", "li", "ga", "tor"}},
};
#define COUNT_OF(a) ((int)(sizeof(a) / sizeof((a)[0])))

static const char *VOCAB[][2] = { /* {es, en} */
    {"gato", "cat"},  {"perro", "dog"},  {"casa", "house"}, {"sol", "sun"},
    {"luna", "moon"}, {"agua", "water"}, {"libro", "book"}, {"flor", "flower"},
    {"pez", "fish"},  {"mesa", "table"}, {"leche", "milk"}, {"pan", "bread"},
};
#define VOCAB_N COUNT_OF(VOCAB)

/* ---------- minijuegos ---------- */
static void make_sum(Round *r, int level, Rng *g, int minus) {
    int top = level == 1 ? 10 : (level == 2 ? 20 : 100);
    int a, b;
    if (minus) {
        a = rng_range(g, 1, top);
        b = rng_range(g, 0, a);
        snprintf(r->prompt, sizeof r->prompt, "%d - %d = ?", a, b);
        set_num_options(r, a - b, top, g);
    } else {
        a = rng_range(g, 0, top);
        b = rng_range(g, 0, top - a);
        snprintf(r->prompt, sizeof r->prompt, "%d + %d = ?", a, b);
        set_num_options(r, a + b, top, g);
    }
    r->visual_a = a; r->visual_b = b;
}

static void make_count(Round *r, int level, Lang lang, Rng *g) {
    int top = level == 1 ? 5 : (level == 2 ? 10 : 20);
    int n = rng_range(g, 1, top);
    snprintf(r->prompt, sizeof r->prompt, lang == LANG_ES ? "¿Cuántas bolitas hay?" : "How many blobs?");
    r->visual_a = n;
    set_num_options(r, n, top + 5, g);
}

static void make_series(Round *r, int level, Rng *g) {
    static const int steps[3][4] = {{1, 2, 1, 2}, {2, 5, 10, 5}, {3, 4, 6, 7}};
    int step = steps[level - 1][rng_range(g, 0, 3)];
    int a = rng_range(g, 0, level == 3 ? 30 : 10);
    snprintf(r->prompt, sizeof r->prompt, "%d, %d, %d, ?", a, a + step, a + 2 * step);
    r->visual_a = a; r->visual_b = step;
    set_num_options(r, a + 3 * step, 200, g);
}

static void make_syllables(Round *r, int level, Lang lang, Rng *g) {
    const Word *tab = lang == LANG_ES ? WORDS_ES : WORDS_EN;
    int total = lang == LANG_ES ? COUNT_OF(WORDS_ES) : COUNT_OF(WORDS_EN);
    int want = level + 1, cand[32], nc = 0;
    for (int i = 0; i < total; i++) if (tab[i].n == want) cand[nc++] = i;
    const Word *w = &tab[cand[rng_range(g, 0, nc - 1)]];
    int miss = rng_range(g, 0, w->n - 1);

    r->prompt[0] = 0;
    for (int i = 0; i < w->n; i++) {
        strncat(r->prompt, i == miss ? "_" : w->s[i], sizeof r->prompt - strlen(r->prompt) - 1);
        if (i < w->n - 1) strncat(r->prompt, "-", sizeof r->prompt - strlen(r->prompt) - 1);
    }
    snprintf(r->option[0], sizeof r->option[0], "%s", w->s[miss]);
    int n = 1, guard = 0;
    while (n < EDU_OPTIONS && guard++ < 500) {
        const Word *o = &tab[rng_range(g, 0, total - 1)];
        const char *s = o->s[rng_range(g, 0, o->n - 1)];
        if (!has_option(r, n, s)) snprintf(r->option[n++], sizeof r->option[0], "%s", s);
    }
    shuffle_correct(r, g);
    r->visual_a = w->n; r->visual_b = miss;
}

static void make_vocab(Round *r, Lang lang, Rng *g) {
    int src = lang == LANG_ES ? 0 : 1, dst = 1 - src;
    int k = rng_range(g, 0, (int)VOCAB_N - 1);
    if (lang == LANG_ES)
        snprintf(r->prompt, sizeof r->prompt, "¿Cómo se dice \"%s\" en inglés?", VOCAB[k][src]);
    else
        snprintf(r->prompt, sizeof r->prompt, "What is \"%s\" in Spanish?", VOCAB[k][src]);
    snprintf(r->option[0], sizeof r->option[0], "%s", VOCAB[k][dst]);
    int n = 1, guard = 0;
    while (n < EDU_OPTIONS && guard++ < 500) {
        const char *s = VOCAB[rng_range(g, 0, (int)VOCAB_N - 1)][dst];
        if (!has_option(r, n, s)) snprintf(r->option[n++], sizeof r->option[0], "%s", s);
    }
    shuffle_correct(r, g);
    r->visual_a = k;
}

static void make_clock(Round *r, int level, Lang lang, Rng *g) {
    static const int mins1[] = {0}, mins2[] = {0, 30}, mins3[] = {0, 15, 30, 45};
    const int *mins = level == 1 ? mins1 : (level == 2 ? mins2 : mins3);
    int nm = level == 1 ? 1 : (level == 2 ? 2 : 4);
    int h = rng_range(g, 1, 12), m = mins[rng_range(g, 0, nm - 1)];
    snprintf(r->prompt, sizeof r->prompt, lang == LANG_ES ? "¿Qué hora es?" : "What time is it?");
    r->visual_a = h; r->visual_b = m;
    snprintf(r->option[0], sizeof r->option[0], "%d:%02d", h, m);
    int n = 1, guard = 0;
    while (n < EDU_OPTIONS && guard++ < 500) {
        char buf[24];
        snprintf(buf, sizeof buf, "%d:%02d", rng_range(g, 1, 12), mins[rng_range(g, 0, nm - 1)]);
        if (!has_option(r, n, buf)) snprintf(r->option[n++], sizeof r->option[0], "%s", buf);
    }
    for (int hh = 1; n < EDU_OPTIONS; hh++) { /* respaldo si hay pocas combinaciones */
        char buf[24];
        snprintf(buf, sizeof buf, "%d:%02d", hh, 0);
        if (!has_option(r, n, buf)) snprintf(r->option[n++], sizeof r->option[0], "%s", buf);
    }
    shuffle_correct(r, g);
}

void edu_make_round(Round *r, MiniGame game, int level, Lang lang, Rng *g) {
    memset(r, 0, sizeof *r);
    level = clampi(level, 1, EDU_LEVELS);
    r->game = game; r->level = level;
    switch (game) {
        case MG_SUMAS:   make_sum(r, level, g, 0); break;
        case MG_RESTAS:  make_sum(r, level, g, 1); break;
        case MG_CONTAR:  make_count(r, level, lang, g); break;
        case MG_SERIES:  make_series(r, level, g); break;
        case MG_SILABAS: make_syllables(r, level, lang, g); break;
        case MG_VOCAB:   make_vocab(r, lang, g); break;
        case MG_HORA:    make_clock(r, level, lang, g); break;
        default: break;
    }
}

const char *edu_game_name(MiniGame game, Lang lang) {
    static const char *N[MG_COUNT][2] = {
        {"Sumas rodantes", "Rolling sums"},
        {"Restas saltarinas", "Jumping subtraction"},
        {"Cuenta bolitas", "Count the blobs"},
        {"Series musicales", "Musical sequences"},
        {"Tambor de sílabas", "Syllable drums"},
        {"Palabras del mundo", "World words"},
        {"Reloj cucú", "Cuckoo clock"},
    };
    return (game >= 0 && game < MG_COUNT) ? N[game][lang == LANG_EN] : "";
}

/* ---------- dificultad adaptable ---------- */
void progress_init(Progress *p) { p->level = 1; p->streak_ok = p->streak_bad = 0; }

void progress_answer(Progress *p, int ok) {
    if (ok) {
        p->streak_bad = 0;
        if (++p->streak_ok >= 3 && p->level < EDU_LEVELS) { p->level++; p->streak_ok = 0; }
    } else {
        p->streak_ok = 0;
        if (++p->streak_bad >= 2 && p->level > 1) { p->level--; p->streak_bad = 0; }
    }
}
