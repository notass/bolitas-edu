/* Sonidos sintetizados (sin archivos): tambores, aciertos, fallos, fanfarria.
 * Si no hay dispositivo de audio, todo queda en silencio sin dar errores. */
#ifndef AUDIO_H
#define AUDIO_H

void audio_init(void);
void audio_close(void);
int  audio_ok(void);

void sfx_drum(int i);   /* tambor 0..3 (cuadrado, triángulo, círculo, cruz) */
void sfx_good(void);    /* acierto */
void sfx_bad(void);     /* fallo suave */
void sfx_win(void);     /* fanfarria */
void sfx_pop(void);     /* toque corto (menú, pintar) */
void sfx_animal(int i); /* sonido de animal: 0 gato 1 perro 2 vaca 3 pato 4 oveja 5 rana 6 pollito 7 caballo 8 cerdo 9 gallo */
void audio_set_muted(int muted);
int  audio_muted(void);

#endif
