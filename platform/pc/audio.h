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

#endif
