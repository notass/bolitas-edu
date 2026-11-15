/* Colorear: dibujos hechos de regiones (rectángulo, elipse, triángulo) que se
 * rellenan con un color de la paleta. Sin dependencias de SDL. Coordenadas del
 * dibujo: 300 x 200. */
#ifndef PAINT_H
#define PAINT_H

#include "edu.h"

#define PAINT_W 300
#define PAINT_H_ 200
#define PAINT_MAX_REGIONS 16
#define PAINT_DRAWINGS 3

typedef enum { SH_RECT = 0, SH_ELLIPSE = 1, SH_TRI = 2 } ShapeKind;

/* RECT: x,y,w,h  ELLIPSE: cx,cy,rx,ry  TRI: x1,y1,x2,y2,x3,y3 */
typedef struct { ShapeKind kind; short v[6]; } Region;

typedef struct {
    const char *name[2];             /* es, en */
    int n;
    Region reg[PAINT_MAX_REGIONS];   /* orden de dibujo: la última queda encima */
} Drawing;

typedef struct {
    int drawing;                                         /* dibujo actual */
    unsigned char fill[PAINT_DRAWINGS][PAINT_MAX_REGIONS]; /* 0 sin pintar, c+1 = color c */
    int cursor;                                          /* región seleccionada (teclado/mando) */
    int color;                                           /* color elegido 0..7 */
} Paint;

const Drawing *paint_drawing(int i);
void paint_init(Paint *p);
/* Región más alta bajo el punto (x,y) del dibujo, o -1 */
int  paint_hit(const Drawing *d, int x, int y);
/* Pinta la región con el color actual. Devuelve 1 si cambió algo. */
int  paint_fill(Paint *p, int region);
void paint_clear(Paint *p);
void paint_next_drawing(Paint *p);
void paint_move_cursor(Paint *p, int dir);
int  paint_count_filled(const Paint *p);   /* regiones pintadas del dibujo actual */
int  paint_complete(const Paint *p);

#endif
