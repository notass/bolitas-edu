#ifndef SDL_INC_H
#define SDL_INC_H
/* SDL2 se incluye distinto según plataforma (PC/PSP: <SDL2/SDL.h>, Android: <SDL.h>) */
#if defined(__has_include)
#  if __has_include(<SDL2/SDL.h>)
#    include <SDL2/SDL.h>
#  else
#    include <SDL.h>
#  endif
#else
#  include <SDL2/SDL.h>
#endif
#endif
