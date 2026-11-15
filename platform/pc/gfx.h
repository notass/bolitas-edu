/* Dibujo 2D sobre SDL_Renderer. Todo se dibuja en coordenadas "lógicas" de 480x272
 * (la pantalla de la PSP) y gfx lo escala a la resolución real: así las formas y el
 * texto salen nítidos en PSP (x1), Vita (x2, 960x544) y cualquier móvil. */
#ifndef GFX_H
#define GFX_H

#include "sdl_inc.h"

#define W 480
#define H 272
#define PI_F 3.14159265f

/* Al empezar cada fotograma: calcula escala y bandas negras, limpia y recorta. */
void  gfx_begin(SDL_Renderer *r);
float gfx_scale(void);
/* pixel real de la ventana -> coordenada lógica (para ratón / táctil) */
void  gfx_to_logical(int px, int py, int *lx, int *ly);

void     gfx_setc(SDL_Renderer *r, unsigned rgb);
unsigned gfx_lighten(unsigned rgb, int d);          /* d < 0 oscurece */
void     gfx_rect(SDL_Renderer *r, int x, int y, int w, int h, unsigned col);
void     gfx_frame(SDL_Renderer *r, int x, int y, int w, int h, unsigned col, int thick);
void     gfx_ellipse(SDL_Renderer *r, int cx, int cy, int rx, int ry, unsigned col);
void     gfx_circle(SDL_Renderer *r, int cx, int cy, int rad, unsigned col);
void     gfx_ring(SDL_Renderer *r, int cx, int cy, int rad, int thick, unsigned col);
void     gfx_tri(SDL_Renderer *r, int x1, int y1, int x2, int y2, int x3, int y3, unsigned col);
void     gfx_line(SDL_Renderer *r, int x1, int y1, int x2, int y2, int thick, unsigned col);
/* Solo el contorno de una elipse (sin relleno) */
void     gfx_ellipse_outline(SDL_Renderer *r, int cx, int cy, int rx, int ry, int thick, unsigned col);
/* Rectángulo con esquinas redondeadas (radio en px lógicos) */
void     gfx_round_rect(SDL_Renderer *r, int x, int y, int w, int h, int rad, unsigned col);

/* Texto (fuente Outfit Bold incrustada). 'size' = alto de la "em" en px lógicos; las
 * letras mayúsculas miden 0,7*size. 'y' es el borde superior de las mayúsculas, así
 * que un texto de una línea ocupa de y a y + 0,7*size (más los rabillos). */
int  gfx_text_width(const char *s, int size);
void gfx_text(SDL_Renderer *r, const char *s, int x, int y, int size, unsigned col);
void gfx_text_c(SDL_Renderer *r, const char *s, int cx, int y, int size, unsigned col);
/* Con borde oscuro para que se lea sobre cualquier fondo */
void gfx_text_ol(SDL_Renderer *r, const char *s, int x, int y, int size, unsigned col, unsigned outline);
void gfx_text_ol_c(SDL_Renderer *r, const char *s, int cx, int y, int size, unsigned col, unsigned outline);

/* Bolita. mood: 1 contenta, 0 normal, -1 triste. sx/sy: aplastado/estirado.
 * look: -1..1 hacia dónde mira. blink: 1 = ojos cerrados. */
void gfx_blob(SDL_Renderer *r, int cx, int cy, int rad, unsigned col, int mood,
              float sx, float sy, float look, int blink);

void gfx_clock(SDL_Renderer *r, int cx, int cy, int rad, int h, int m);

/* Símbolo de botón PSP centrado: 0 cuadrado, 1 triángulo, 2 círculo, 3 cruz */
void gfx_symbol(SDL_Renderer *r, int i, int cx, int cy, int size, int thick, unsigned col);
unsigned gfx_symbol_color(int i);

/* Figura para el juego de figuras: 0 círculo 1 cuadrado 2 triángulo 3 rectángulo 4 rombo */
void gfx_shape(SDL_Renderer *r, int kind, int cx, int cy, int size, unsigned col);

#endif
