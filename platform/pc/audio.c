#include "audio.h"
#include "sdl_inc.h"
#include <math.h>
#include <string.h>

#define RATE 22050
#define NV 12

typedef struct {
    float f0, f1;      /* frecuencia inicial y final (Hz) */
    float phase, vol;
    float t, dur;      /* segundos */
    int kind;          /* 0 tono, 1 tambor (tono + ruido), 2 zumbido suave */
    int delay;         /* muestras que faltan para empezar */
    int active;
} Voice;

static Voice s_v[NV];
static SDL_AudioDeviceID s_dev;
static unsigned s_noise = 12345u;
static int s_muted;

static float noise(void) {
    s_noise = s_noise * 1664525u + 1013904223u;
    return (float)((int)(s_noise >> 9) - (1 << 22)) / (float)(1 << 22);
}

static void callback(void *ud, Uint8 *stream, int len) {
    (void)ud;
    Sint16 *out = (Sint16 *)stream;
    int n = len / 2;
    for (int i = 0; i < n; i++) {
        float sum = 0;
        for (int k = 0; k < NV; k++) {
            Voice *v = &s_v[k];
            if (!v->active) continue;
            if (v->delay > 0) { v->delay--; continue; }
            if (v->t >= v->dur) { v->active = 0; continue; }
            float p = v->t / v->dur, env = (1.0f - p) * (1.0f - p);
            float f = v->f0 + (v->f1 - v->f0) * p;
            v->phase += 2.0f * 3.14159265f * f / (float)RATE;
            if (v->phase > 6.2831853f) v->phase -= 6.2831853f;
            float s = sinf(v->phase);
            if (v->kind == 1) s = s * 0.9f + noise() * 0.35f * (1.0f - p) * (1.0f - p);
            else if (v->kind == 2) s = (s > 0 ? 1.0f : -1.0f) * 0.35f + s * 0.3f;
            sum += s * env * v->vol;
            v->t += 1.0f / (float)RATE;
        }
        sum *= 0.6f;
        if (sum > 1.0f) sum = 1.0f;
        if (sum < -1.0f) sum = -1.0f;
        out[i] = (Sint16)(sum * 30000.0f);
    }
}

void audio_init(void) {
    SDL_AudioSpec want, have;
    memset(&want, 0, sizeof want);
    memset(&have, 0, sizeof have);
    want.freq = RATE;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 512;
    want.callback = callback;
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) return;
    s_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (s_dev) SDL_PauseAudioDevice(s_dev, 0);
}

void audio_close(void) {
    if (s_dev) { SDL_CloseAudioDevice(s_dev); s_dev = 0; }
}

int audio_ok(void) { return s_dev != 0; }

static void voice(float f0, float f1, float vol, float dur, int kind, float delay_s) {
    if (!s_dev || s_muted) return;
    SDL_LockAudioDevice(s_dev);
    int slot = 0;
    float oldest = -1.0f;
    for (int k = 0; k < NV; k++) { /* hueco libre, o la voz más antigua */
        if (!s_v[k].active) { slot = k; break; }
        if (s_v[k].t > oldest) { oldest = s_v[k].t; slot = k; }
    }
    Voice *v = &s_v[slot];
    v->f0 = f0; v->f1 = f1; v->vol = vol; v->dur = dur; v->kind = kind;
    v->phase = 0; v->t = 0; v->delay = (int)(delay_s * (float)RATE); v->active = 1;
    SDL_UnlockAudioDevice(s_dev);
}

void sfx_drum(int i) {
    static const float F[4] = { 170.0f, 230.0f, 300.0f, 130.0f };
    float f = F[i & 3];
    voice(f * 1.8f, f * 0.7f, 0.9f, 0.22f, 1, 0);
    voice(f * 2.0f, f * 2.0f, 0.18f, 0.12f, 0, 0); /* un toque de tono para que se distinga cada tambor */
}

void sfx_good(void) {
    voice(784.0f, 784.0f, 0.5f, 0.16f, 0, 0);       /* sol */
    voice(1175.0f, 1175.0f, 0.5f, 0.28f, 0, 0.09f); /* re */
}

void sfx_bad(void) { voice(190.0f, 120.0f, 0.5f, 0.28f, 2, 0); }

void sfx_win(void) {
    static const float N[5] = { 523.0f, 659.0f, 784.0f, 1047.0f, 1319.0f };
    for (int i = 0; i < 5; i++) voice(N[i], N[i], 0.45f, 0.3f, 0, 0.1f * (float)i);
}

void sfx_pop(void) { voice(900.0f, 1300.0f, 0.4f, 0.06f, 0, 0); }

void audio_set_muted(int muted) { s_muted = muted ? 1 : 0; }
int audio_muted(void) { return s_muted; }

/* Sonidos de animales hechos con tonos: no son grabaciones, pero se distinguen bien. */
void sfx_animal(int i) {
    switch (i) {
    case 0: /* gato: miau */
        voice(450, 800, 0.55f, 0.16f, 0, 0); voice(800, 900, 0.55f, 0.18f, 0, 0.16f); voice(900, 420, 0.5f, 0.3f, 0, 0.34f);
        break;
    case 1: /* perro: guau guau */
        voice(420, 180, 0.9f, 0.14f, 1, 0); voice(380, 160, 0.9f, 0.16f, 1, 0.3f); break;
    case 2: /* vaca: muuu */
        voice(150, 135, 0.7f, 0.9f, 2, 0); voice(300, 270, 0.25f, 0.9f, 0, 0); break;
    case 3: /* pato: cuac cuac */
        voice(520, 300, 0.6f, 0.11f, 2, 0); voice(520, 300, 0.6f, 0.11f, 2, 0.17f); voice(500, 280, 0.6f, 0.11f, 2, 0.4f); break;
    case 4: /* oveja: beee (dos tonos casi iguales hacen temblar) */
        voice(340, 320, 0.45f, 0.7f, 2, 0); voice(352, 332, 0.45f, 0.7f, 2, 0); break;
    case 5: /* rana: crooac */
        for (int k = 0; k < 4; k++) voice(140.0f + 20.0f * (float)k, 220.0f, 0.6f, 0.07f, 2, 0.09f * (float)k);
        break;
    case 6: /* pollito: pío pío */
        for (int k = 0; k < 4; k++) voice(1900, 2700, 0.35f, 0.07f, 0, 0.14f * (float)k);
        break;
    case 7: /* caballo: iiijijiii */
        voice(700, 1500, 0.5f, 0.25f, 0, 0); voice(1500, 600, 0.5f, 0.45f, 2, 0.25f); voice(1100, 1500, 0.3f, 0.2f, 0, 0.7f); break;
    case 8: /* cerdo: oinc oinc */
        voice(230, 110, 0.8f, 0.14f, 1, 0); voice(210, 100, 0.8f, 0.16f, 1, 0.25f); break;
    default: /* gallo: quiquiriquí */
        voice(550, 850, 0.5f, 0.15f, 0, 0); voice(850, 650, 0.5f, 0.12f, 0, 0.17f);
        voice(650, 1100, 0.5f, 0.18f, 0, 0.32f); voice(1100, 500, 0.5f, 0.45f, 0, 0.52f); break;
    }
}

