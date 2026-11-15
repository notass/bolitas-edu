# Bolitas Edu (nombre provisional)

Juego educativo rítmico para niñas desde 6 años, estilo LocoRoco/Patapon.
Español principal, inglés secundario. PC (pruebas), PSP original, y después PS Vita y Android.

## Secciones del menú
- **Números:** sumas, restas, contar bolitas, series, mayor o menor.
- **Letras:** sílabas, letras mágicas (letra inicial/final), palabras del mundo (ES/EN).
- **Descubre:** colores, figuras, reloj, días y meses.
- **Música y arte:** marcha rítmica (estilo Patapon, con secuencias de tambores) y colorear.
- **Agilidad mental:** pendiente (comecocos, memoria, Simon, puzzles...).

## Controles
| | PSP / Vita | PC |
|---|---|---|
| Elegir opción (juegos) | cruceta o botones del rombo (la opción de la izquierda = ◻ / ←, arriba = △ / ↑...) | flechas, o 1-4 / Z X C V |
| Elegir (menús) | ✕ | Enter |
| Atrás / menú | ○ (menús), Start (juegos) | Esc |
| MAYÚSCULAS / minúsculas | Select | Tab |
| Idioma ES/EN | L / R (en los menús) | L |
| Colorear | ✕ pintar, △ otro dibujo, ◻ borrar, L/R color, ↑↓ zona | N otro, Retroceso borrar |

Táctil / ratón: tocar. Mayúsculas por defecto (letra de palo para empezar a leer);
la elección y el idioma se guardan.

## Estructura
- `core/` lógica en C99 sin dependencias ni malloc (PSP-friendly). Probada (`make test`).
  - `edu.c` preguntas de los minijuegos, dificultad adaptable, mayúsculas/minúsculas.
  - `rhythm.c` marcha rítmica (compás, ventanas de acierto, secuencias por patrones).
  - `paint.c` dibujos para colorear (regiones y detección de toque).
- `platform/pc/` capa SDL2: `main.c` (pantallas), `gfx.c` (dibujo y texto), `audio.c` (sonidos sintetizados).
  Sirve también para Android y PSP/Vita. Todo se dibuja en 480x272 lógicos y se escala a la pantalla real.
- `platform/psp/` Makefile de pspdev (genera EBOOT.PBP).
- `tools/make_font.py` genera `platform/pc/font_data.c` desde `tools/Outfit-Bold.ttf`.

## Tipografía
Outfit Bold (SIL Open Font License 1.1, ver `platform/pc/fonts/Outfit-OFL.txt`): "a" y "g" de una
sola planta, clara para aprender a leer. Va incrustada en el código (no hace falta ninguna librería).
Para regenerarla: `python3 tools/make_font.py` (necesita Pillow).

## Compilar
- Pruebas: `make test`
- PC: `make pc && ./bolitas_pc` (necesita libsdl2-dev)
- Prueba automática con capturas: `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./bolitas_pc --smoke`
- GitHub Actions: `test`, `pc` (capturas) y `psp` (EBOOT.PBP).

## Pendiente
Versión PS Vita (VitaSDK), proyecto Android, módulos por juego, y los juegos de la
sección Agilidad mental (ahorcado, comecocos, memoria, Simon, puzzles).
