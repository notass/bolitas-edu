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

/* texto según idioma: castellano, inglés, euskera */
static const char *L3(Lang l, const char *es, const char *en, const char *eu) {
    return l == LANG_EN ? en : (l == LANG_EU ? eu : es);
}

static int li(Lang l) { return l == LANG_EN ? 1 : (l == LANG_EU ? 2 : 0); }

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
static const Word WORDS_EU[] = {
    {2, {"e", "txe"}}, {2, {"ka", "tu"}}, {2, {"ar", "di"}}, {2, {"o", "gi"}},
    {3, {"li", "bu", "ru"}}, {3, {"i", "lar", "gi"}}, {3, {"e", "guz", "ki"}},
    {3, {"txa", "kur", "ra"}}, {3, {"ma", "ha", "i"}}, {3, {"ar", "ra", "in"}},
    {3, {"ze", "ru", "a"}}, {3, {"a", "ha", "te"}}, {3, {"zal", "di", "a"}},
    {4, {"txi", "me", "le", "ta"}}, {4, {"bi", "zi", "kle", "ta"}},
    {4, {"dor", "to", "ka", "a"}}, {4, {"sa", "ga", "rra", "k"}},
};
#define COUNT_OF(a) ((int)(sizeof(a) / sizeof((a)[0])))

static const char *VOCAB[][3] = { /* {es, en, eu} */
    {"gato", "cat", "katu"},  {"perro", "dog", "txakur"},  {"casa", "house", "etxe"}, {"sol", "sun", "eguzki"},
    {"luna", "moon", "ilargi"}, {"agua", "water", "ur"}, {"libro", "book", "liburu"}, {"flor", "flower", "lore"},
    {"pez", "fish", "arrain"},  {"mesa", "table", "mahai"}, {"leche", "milk", "esne"}, {"pan", "bread", "ogi"},
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
    snprintf(r->prompt, sizeof r->prompt, "%s", L3(lang, "¿Cuántas bolitas hay?", "How many blobs?", "Zenbat bolatxo daude?"));
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
    const Word *tab = lang == LANG_ES ? WORDS_ES : (lang == LANG_EN ? WORDS_EN : WORDS_EU);
    int total = lang == LANG_ES ? COUNT_OF(WORDS_ES) : (lang == LANG_EN ? COUNT_OF(WORDS_EN) : COUNT_OF(WORDS_EU));
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
    /* se pregunta en el idioma elegido y se responde en inglés (en inglés: en castellano) */
    int dst = lang == LANG_EN ? 0 : 1;
    int k = rng_range(g, 0, (int)VOCAB_N - 1);
    snprintf(r->prompt, sizeof r->prompt, L3(lang, "¿Cómo se dice \"%s\" en inglés?", "What is \"%s\" in Spanish?",
                                           "Nola esaten da \"%s\" ingelesez?"), VOCAB[k][lang]);
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
    snprintf(r->prompt, sizeof r->prompt, "%s", L3(lang, "¿Qué hora es?", "What time is it?", "Zer ordu da?"));
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

/* ---------- tablas: colores, figuras, días, meses ---------- */
static const struct { unsigned rgb; const char *n[3]; } COLORS[] = {
    {0xE53935, {"rojo", "red", "gorria"}},        {0xFB8C00, {"naranja", "orange", "laranja"}},
    {0xFDD835, {"amarillo", "yellow", "horia"}}, {0x43A047, {"verde", "green", "berdea"}},
    {0x1E88E5, {"azul", "blue", "urdina"}},       {0x8E24AA, {"morado", "purple", "morea"}},
    {0xEC407A, {"rosa", "pink", "arrosa"}},       {0x795548, {"marrón", "brown", "marroia"}},
};
#define COLOR_N COUNT_OF(COLORS)

static const char *SHAPES[][3] = {
    {"círculo", "circle", "zirkulua"},     {"cuadrado", "square", "karratua"},  {"triángulo", "triangle", "triangelua"},
    {"rectángulo", "rectangle", "laukizuzena"}, {"rombo", "diamond", "erronboa"},
};
#define SHAPE_N COUNT_OF(SHAPES)

static const char *const DAYS[3][7] = {
    {"lunes", "martes", "miércoles", "jueves", "viernes", "sábado", "domingo"},
    {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"},
    {"astelehena", "asteartea", "asteazkena", "osteguna", "ostirala", "larunbata", "igandea"},
};
static const char *const MONTHS[3][12] = {
    {"enero", "febrero", "marzo", "abril", "mayo", "junio", "julio", "agosto",
     "septiembre", "octubre", "noviembre", "diciembre"},
    {"January", "February", "March", "April", "May", "June", "July", "August",
     "September", "October", "November", "December"},
    {"urtarrila", "otsaila", "martxoa", "apirila", "maiatza", "ekaina", "uztaila", "abuztua",
     "iraila", "urria", "azaroa", "abendua"},
};

int edu_color_count(void) { return (int)COLOR_N; }
unsigned edu_color_rgb(int i) { return COLORS[clampi(i, 0, (int)COLOR_N - 1)].rgb; }
const char *edu_color_name(int i, Lang lang) { return COLORS[clampi(i, 0, (int)COLOR_N - 1)].n[li(lang)]; }
int edu_shape_count(void) { return (int)SHAPE_N; }
const char *edu_shape_name(int i, Lang lang) { return SHAPES[clampi(i, 0, (int)SHAPE_N - 1)][li(lang)]; }

/* option[0] = names[c]; el resto, otros nombres distintos; luego se baraja. */
static void set_name_options(Round *r, const char *const *names, int n, int c, Rng *g) {
    snprintf(r->option[0], sizeof r->option[0], "%s", names[c]);
    int m = 1, guard = 0;
    while (m < EDU_OPTIONS && guard++ < 500) {
        const char *s = names[rng_range(g, 0, n - 1)];
        if (!has_option(r, m, s)) snprintf(r->option[m++], sizeof r->option[0], "%s", s);
    }
    shuffle_correct(r, g);
}

static void make_bigger(Round *r, int level, Lang lang, Rng *g) {
    int top = level == 1 ? 20 : (level == 2 ? 100 : 1000);
    int v[EDU_OPTIONS], n = 0, guard = 0;
    while (n < EDU_OPTIONS && guard++ < 500) {
        int x = rng_range(g, 0, top), dup = 0;
        for (int i = 0; i < n; i++) if (v[i] == x) dup = 1;
        if (!dup) v[n++] = x;
    }
    int big = rng_range(g, 0, 1), best = 0;
    for (int i = 1; i < EDU_OPTIONS; i++)
        if (big ? v[i] > v[best] : v[i] < v[best]) best = i;
    for (int i = 0; i < EDU_OPTIONS; i++) snprintf(r->option[i], sizeof r->option[i], "%d", v[i]);
    r->correct = best;
    r->visual_a = big;
    snprintf(r->prompt, sizeof r->prompt, "%s",
             big ? L3(lang, "¿Cuál es el número mayor?", "Which number is bigger?", "Zein da zenbaki handiena?")
                 : L3(lang, "¿Cuál es el número menor?", "Which number is smaller?", "Zein da zenbaki txikiena?"));
}

static void make_letter(Round *r, int level, Lang lang, Rng *g) {
    int k = rng_range(g, 0, (int)VOCAB_N - 1);
    const char *w = VOCAB[k][li(lang)];
    int ends = level == 1 ? 0 : (level == 2 ? rng_range(g, 0, 1) : 1);
    char c = (char)((ends ? w[strlen(w) - 1] : w[0]) - 'a' + 'A');
    if (lang == LANG_ES)
        snprintf(r->prompt, sizeof r->prompt, "¿Con qué letra %s \"%s\"?", ends ? "termina" : "empieza", w);
    else if (lang == LANG_EN)
        snprintf(r->prompt, sizeof r->prompt, "What letter does \"%s\" %s with?", w, ends ? "end" : "start");
    else
        snprintf(r->prompt, sizeof r->prompt, "Zein letrarekin %s \"%s\"?", ends ? "amaitzen da" : "hasten da", w);
    snprintf(r->option[0], sizeof r->option[0], "%c", c);
    int n = 1, guard = 0;
    while (n < EDU_OPTIONS && guard++ < 500) {
        char b[2] = { (char)('A' + rng_range(g, 0, 25)), 0 };
        if (!has_option(r, n, b)) snprintf(r->option[n++], sizeof r->option[0], "%s", b);
    }
    shuffle_correct(r, g);
    r->visual_a = ends; r->visual_b = k;
}

static void make_color(Round *r, int level, Lang lang, Rng *g) {
    int pool = level == 1 ? 4 : (level == 2 ? 6 : (int)COLOR_N);
    int k = rng_range(g, 0, pool - 1);
    const char *nm[COLOR_N];
    for (int i = 0; i < (int)COLOR_N; i++) nm[i] = edu_color_name(i, lang);
    snprintf(r->prompt, sizeof r->prompt, "%s", L3(lang, "¿De qué color es la bolita?", "What color is the blob?", "Zer kolore du bolatxoak?"));
    r->visual_a = k;
    set_name_options(r, nm, (int)COLOR_N, k, g);
}

static void make_shape(Round *r, int level, Lang lang, Rng *g) {
    int k = rng_range(g, 0, level + 1); /* nivel 1: 3 figuras, 2: 4, 3: 5 */
    const char *nm[SHAPE_N];
    for (int i = 0; i < (int)SHAPE_N; i++) nm[i] = edu_shape_name(i, lang);
    snprintf(r->prompt, sizeof r->prompt, "%s", L3(lang, "¿Qué figura es?", "What shape is it?", "Zer irudi da?"));
    r->visual_a = k;
    set_name_options(r, nm, (int)SHAPE_N, k, g);
}

static void make_days(Round *r, int level, Lang lang, Rng *g) {
    int months = level == 3, n = months ? 12 : 7;
    const char *const *list = months ? MONTHS[li(lang)] : DAYS[li(lang)];
    int base = rng_range(g, 0, n - 1);
    int after = level == 1 ? 1 : rng_range(g, 0, 1);
    int c = (base + (after ? 1 : n - 1)) % n;
    if (lang == LANG_ES)
        snprintf(r->prompt, sizeof r->prompt, months ? "¿Qué mes va %s de %s?" : "¿Qué día va %s del %s?",
                 after ? "después" : "antes", list[base]);
    else if (lang == LANG_EN)
        snprintf(r->prompt, sizeof r->prompt, months ? "Which month comes %s %s?" : "Which day comes %s %s?",
                 after ? "after" : "before", list[base]);
    else
        snprintf(r->prompt, sizeof r->prompt, months ? "%s: zein hilabete dator %s?" : "%s: zein egun dator %s?",
                 list[base], after ? "ondoren" : "aurretik");
    r->visual_a = base; r->visual_b = (months << 1) | after;
    set_name_options(r, list, n, c, g);
}

/* ---------- animales, palabras, japonés, hiragana ---------- */
static const char *ANIMALS[][3] = { /* orden = número de sonido en audio.c */
    {"gato", "cat", "katu"},       {"perro", "dog", "txakur"},   {"vaca", "cow", "behi"},
    {"pato", "duck", "ahate"},     {"oveja", "sheep", "ardi"},   {"rana", "frog", "igel"},
    {"pollito", "chick", "txita"}, {"caballo", "horse", "zaldi"}, {"cerdo", "pig", "txerri"},
    {"gallo", "rooster", "oilar"},
};
#define ANIMAL_N COUNT_OF(ANIMALS)

static const char *EXTRA[][3] = { /* más palabras para deletrear y el ahorcado */
    {"elefante", "elephant", "elefante"}, {"mariposa", "butterfly", "tximeleta"},
    {"ventana", "window", "leiho"},       {"tortuga", "turtle", "dortoka"},
    {"conejo", "rabbit", "untxi"},        {"manzana", "apple", "sagar"},
    {"bicicleta", "bicycle", "bizikleta"}, {"estrella", "star", "izar"},
};
#define EXTRA_N COUNT_OF(EXTRA)

int edu_animal_count(void) { return (int)ANIMAL_N; }
const char *edu_animal_name(int i, Lang lang) { return ANIMALS[clampi(i, 0, (int)ANIMAL_N - 1)][li(lang)]; }

int edu_word_count(void) { return (int)(VOCAB_N + ANIMAL_N + EXTRA_N); }
const char *edu_word(int i, Lang lang) {
    i = clampi(i, 0, edu_word_count() - 1);
    if (i < (int)VOCAB_N) return VOCAB[i][li(lang)];
    i -= (int)VOCAB_N;
    if (i < (int)ANIMAL_N) return ANIMALS[i][li(lang)];
    return EXTRA[i - (int)ANIMAL_N][li(lang)];
}

static int word_level_ok(const char *w, int level) {
    int n = (int)strlen(w);
    return level == 1 ? (n >= 3 && n <= 4) : (level == 2 ? (n >= 5 && n <= 6) : n >= 7);
}

void edu_pick_word(char *dst, size_t cap, Lang lang, int level, Rng *g) {
    int cand[64], nc = 0, total = edu_word_count();
    level = clampi(level, 1, EDU_LEVELS);
    for (int i = 0; i < total && nc < 64; i++)
        if (word_level_ok(edu_word(i, lang), level)) cand[nc++] = i;
    if (nc == 0) for (int i = 0; i < total && nc < 64; i++) cand[nc++] = i;
    snprintf(dst, cap, "%s", edu_word(cand[rng_range(g, 0, nc - 1)], lang));
}

static const struct { const char *kana, *romaji; } KANA[] = {
    {"あ","a"},{"い","i"},{"う","u"},{"え","e"},{"お","o"},
    {"か","ka"},{"き","ki"},{"く","ku"},{"け","ke"},{"こ","ko"},
    {"さ","sa"},{"し","shi"},{"す","su"},{"せ","se"},{"そ","so"},
    {"た","ta"},{"ち","chi"},{"つ","tsu"},{"て","te"},{"と","to"},
    {"な","na"},{"に","ni"},{"ぬ","nu"},{"ね","ne"},{"の","no"},
    {"は","ha"},{"ひ","hi"},{"ふ","fu"},{"へ","he"},{"ほ","ho"},
    {"ま","ma"},{"み","mi"},{"む","mu"},{"め","me"},{"も","mo"},
    {"や","ya"},{"ゆ","yu"},{"よ","yo"},
    {"ら","ra"},{"り","ri"},{"る","ru"},{"れ","re"},{"ろ","ro"},
    {"わ","wa"},{"を","wo"},{"ん","n"},
    {"が","ga"},{"ぎ","gi"},{"ぐ","gu"},{"げ","ge"},{"ご","go"},
    {"ざ","za"},{"じ","ji"},{"ず","zu"},{"ぜ","ze"},{"ぞ","zo"},
    {"だ","da"},{"ぢ","ji"},{"づ","zu"},{"で","de"},{"ど","do"},
    {"ば","ba"},{"び","bi"},{"ぶ","bu"},{"べ","be"},{"ぼ","bo"},
    {"ぱ","pa"},{"ぴ","pi"},{"ぷ","pu"},{"ぺ","pe"},{"ぽ","po"},
};
#define KANA_N COUNT_OF(KANA)
int edu_kana_count(void) { return (int)KANA_N; }
const char *edu_kana(int i) { return KANA[clampi(i, 0, (int)KANA_N - 1)].kana; }
const char *edu_kana_romaji(int i) { return KANA[clampi(i, 0, (int)KANA_N - 1)].romaji; }

static const struct { const char *kana, *romaji, *m[3]; } JP[] = {
    {"ねこ", "neko", {"gato", "cat", "katu"}},        {"いぬ", "inu", {"perro", "dog", "txakur"}},
    {"さかな", "sakana", {"pez", "fish", "arrain"}},  {"みず", "mizu", {"agua", "water", "ur"}},
    {"ほん", "hon", {"libro", "book", "liburu"}},     {"はな", "hana", {"flor", "flower", "lore"}},
    {"つき", "tsuki", {"luna", "moon", "ilargi"}},    {"いえ", "ie", {"casa", "house", "etxe"}},
    {"ぱん", "pan", {"pan", "bread", "ogi"}},         {"つくえ", "tsukue", {"mesa", "table", "mahai"}},
    {"りんご", "ringo", {"manzana", "apple", "sagar"}}, {"そら", "sora", {"cielo", "sky", "zeru"}},
    {"くるま", "kuruma", {"coche", "car", "auto"}},   {"たまご", "tamago", {"huevo", "egg", "arrautza"}},
};
#define JP_N COUNT_OF(JP)

static void make_animal(Round *r, int level, Lang lang, Rng *g) {
    int pool = level == 1 ? 5 : (level == 2 ? 8 : (int)ANIMAL_N);
    int k = rng_range(g, 0, pool - 1);
    const char *nm[ANIMAL_N];
    for (int i = 0; i < (int)ANIMAL_N; i++) nm[i] = ANIMALS[i][li(lang)];
    snprintf(r->prompt, sizeof r->prompt, "%s",
             L3(lang, "¿Qué animal hace este sonido?", "Which animal makes this sound?", "Zein animaliak egiten du soinu hau?"));
    r->visual_a = k;
    set_name_options(r, nm, level == 1 ? 5 : (int)ANIMAL_N, k, g);
}

static void make_spell(Round *r, int level, Lang lang, Rng *g) {
    char w[16];
    edu_pick_word(w, sizeof w, lang, level, g);
    int len = (int)strlen(w), miss = rng_range(g, 0, len - 1);
    char c = w[miss];
    snprintf(r->visual_s, sizeof r->visual_s, "%s", w);
    r->visual_s[miss] = '_';
    snprintf(r->prompt, sizeof r->prompt, "%s", L3(lang, "¿Qué letra falta?", "Which letter is missing?", "Zein letra falta da?"));
    snprintf(r->option[0], sizeof r->option[0], "%c", c - 'a' + 'A');
    int n = 1, guard = 0;
    while (n < EDU_OPTIONS && guard++ < 500) {
        char b[2] = { (char)('A' + rng_range(g, 0, 25)), 0 };
        if (!has_option(r, n, b)) snprintf(r->option[n++], sizeof r->option[0], "%s", b);
    }
    shuffle_correct(r, g);
    r->visual_a = miss; r->visual_b = len;
}

static void make_japanese(Round *r, int level, Lang lang, Rng *g) {
    int pool = level == 1 ? 6 : (level == 2 ? 10 : (int)JP_N);
    int k = rng_range(g, 0, pool - 1);
    snprintf(r->prompt, sizeof r->prompt, "%s", L3(lang, "¿Qué significa?", "What does it mean?", "Zer esan nahi du?"));
    snprintf(r->visual_s, sizeof r->visual_s, "%s", JP[k].kana);
    snprintf(r->visual_r, sizeof r->visual_r, "%s", JP[k].romaji);
    snprintf(r->option[0], sizeof r->option[0], "%s", JP[k].m[li(lang)]);
    int n = 1, guard = 0;
    while (n < EDU_OPTIONS && guard++ < 500) {
        const char *s2 = JP[rng_range(g, 0, (int)JP_N - 1)].m[li(lang)];
        if (!has_option(r, n, s2)) snprintf(r->option[n++], sizeof r->option[0], "%s", s2);
    }
    shuffle_correct(r, g);
    r->visual_a = k;
}

static void make_kana(Round *r, int level, Lang lang, Rng *g) {
    int pool = level == 1 ? 10 : (level == 2 ? 25 : 46);
    int k = rng_range(g, 0, pool - 1);
    snprintf(r->prompt, sizeof r->prompt, "%s", L3(lang, "¿Cómo se lee?", "How do you read it?", "Nola irakurtzen da?"));
    snprintf(r->visual_s, sizeof r->visual_s, "%s", KANA[k].kana);
    snprintf(r->visual_r, sizeof r->visual_r, "%s", KANA[k].romaji);
    snprintf(r->option[0], sizeof r->option[0], "%s", KANA[k].romaji);
    int n = 1, guard = 0;
    while (n < EDU_OPTIONS && guard++ < 500) {
        const char *s2 = KANA[rng_range(g, 0, pool - 1)].romaji;
        if (!has_option(r, n, s2)) snprintf(r->option[n++], sizeof r->option[0], "%s", s2);
    }
    shuffle_correct(r, g);
    r->visual_a = k;
}

void edu_make_round(Round *r, MiniGame game, int level, Lang lang, Rng *g) {
    memset(r, 0, sizeof *r);
    level = clampi(level, 1, EDU_LEVELS);
    r->game = game; r->level = level;
    switch (game) {
        case MG_MAYOR:   make_bigger(r, level, lang, g); break;
        case MG_LETRA:   make_letter(r, level, lang, g); break;
        case MG_COLORES: make_color(r, level, lang, g); break;
        case MG_FORMAS:  make_shape(r, level, lang, g); break;
        case MG_DIAS:    make_days(r, level, lang, g); break;
        case MG_SUMAS:   make_sum(r, level, g, 0); break;
        case MG_RESTAS:  make_sum(r, level, g, 1); break;
        case MG_CONTAR:  make_count(r, level, lang, g); break;
        case MG_SERIES:  make_series(r, level, g); break;
        case MG_SILABAS: make_syllables(r, level, lang, g); break;
        case MG_VOCAB:   make_vocab(r, lang, g); break;
        case MG_HORA:    make_clock(r, level, lang, g); break;
        case MG_ANIMAL:  make_animal(r, level, lang, g); break;
        case MG_DELETREAR: make_spell(r, level, lang, g); break;
        case MG_JAPONES: make_japanese(r, level, lang, g); break;
        case MG_KANA:    make_kana(r, level, lang, g); break;
        default: break;
    }
}

const char *edu_game_name(MiniGame game, Lang lang) {
    static const char *N[MG_COUNT][3] = {
        {"Sumas rodantes", "Rolling sums", "Batuketa errodariak"},
        {"Restas saltarinas", "Jumping subtraction", "Kenketa saltariak"},
        {"Cuenta bolitas", "Count the blobs", "Zenbatu bolatxoak"},
        {"Series musicales", "Musical sequences", "Serie musikalak"},
        {"Tambor de sílabas", "Syllable drums", "Silaba danborrak"},
        {"Palabras del mundo", "World words", "Munduko hitzak"},
        {"Reloj cucú", "Cuckoo clock", "Kukuaren erlojua"},
        {"Mayor o menor", "Bigger or smaller", "Handiagoa ala txikiagoa"},
        {"Letras mágicas", "Magic letters", "Letra magikoak"},
        {"Colores", "Colors", "Koloreak"},
        {"Figuras", "Shapes", "Irudiak"},
        {"Días y meses", "Days & months", "Egunak eta hilabeteak"},
        {"Animales", "Animal sounds", "Animaliak"},
        {"Letra que falta", "Missing letter", "Falta den letra"},
        {"Japonés", "Japanese", "Japoniera"},
        {"Hiragana", "Hiragana", "Hiragana"},
    };
    return (game >= 0 && game < MG_COUNT) ? N[game][li(lang)] : "";
}

/* ---------- mayúsculas / minúsculas ---------- */
void edu_set_case(char *dst, size_t cap, const char *src, int upper) {
    size_t n = 0;
    if (cap == 0) return;
    while (*src && n + 1 < cap) {
        unsigned char c = (unsigned char)*src;
        if (c == 0xC3 && src[1] && n + 2 < cap) { /* á é í ó ú ñ ü y sus mayúsculas (U+00C0..U+00FF) */
            unsigned char d = (unsigned char)src[1];
            if (upper && d >= 0xA0 && d <= 0xBE && d != 0xB7) d = (unsigned char)(d - 0x20);
            else if (!upper && d >= 0x80 && d <= 0x9E && d != 0x97) d = (unsigned char)(d + 0x20);
            dst[n++] = (char)c; dst[n++] = (char)d;
            src += 2;
            continue;
        }
        if (upper && c >= 'a' && c <= 'z') c = (unsigned char)(c - 'a' + 'A');
        else if (!upper && c >= 'A' && c <= 'Z') c = (unsigned char)(c - 'A' + 'a');
        dst[n++] = (char)c;
        src++;
    }
    dst[n] = 0;
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
