# Bolitas Edu (nombre provisional)

Juego educativo rítmico para niñas desde 6 años, estilo LocoRoco/Patapon.
Español principal, inglés secundario. PC (pruebas), PSP original, y después PS Vita y Android.

## Secciones del menú
- **Números:** sumas, restas, contar bolitas, series, mayor o menor.
- **Letras:** sílabas, letras mágicas (letra inicial/final), palabras del mundo, letra que falta (deletrear).
- **Descubre:** colores, figuras, reloj (con **chuleta**), días y meses, sonidos de animales.
- **Música y arte:** marcha rítmica (estilo Patapon, con secuencias de tambores) y colorear.
- **Agilidad mental:** ahorcado (la bolita pierde un globo por fallo). Pendiente: comecocos, memoria, Simon, puzzles.
- **Japonés:** palabras en hiragana (con romaji) e hiragana suelto.

Idiomas: castellano, inglés y euskera (L/R o el botón ES/EN/EU; también en Opciones).

## Controles
| | PSP / Vita | PC |
|---|---|---|
| Elegir opción (juegos) | cruceta o botones del rombo (la opción de la izquierda = ◻ / ←, arriba = △ / ↑...) | flechas, o 1-4 / Z X C V |
| Elegir (menús) | ✕ | Enter |
| Atrás | ○ (menús), Start (en partida) | Esc |
| **Opciones** (idioma, Aa, sonido) | **Start** en el menú principal / lista | O |
| MAYÚSCULAS / minúsculas | Select | Tab |
| Idioma | L / R (en los menús) | L |
| Chuleta del reloj / repetir sonido del animal | L / R | H |
| Colorear | ✕ pintar, △ otro dibujo, ◻ borrar, L/R color, ↑↓ zona | N otro, Retroceso borrar |
| Ahorcado | cruceta mueve, ✕ o ○ elige letra | teclas A-Z |

Táctil / ratón: tocar. Mayúsculas por defecto (letra de palo para empezar a leer);
idioma, Aa y sonido se guardan. En el menú principal Start nunca cierra el juego.

## Estructura
- `core/` lógica en C99 sin dependencias ni malloc (PSP-friendly). Probada (`make test`).
  - `edu.c` preguntas de los minijuegos, dificultad adaptable, mayúsculas/minúsculas.
  - `rhythm.c` marcha rítmica (compás, ventanas de acierto, secuencias por patrones).
  - `paint.c` dibujos para colorear (regiones y detección de toque).
  - `hang.c` ahorcado (palabra, letras probadas, fallos).
- `platform/pc/` capa SDL2: `main.c` (pantallas), `gfx.c` (dibujo y texto), `audio.c` (sonidos sintetizados).
  Sirve también para Android y PSP/Vita. Todo se dibuja en 480x272 lógicos y se escala a la pantalla real.
- `platform/psp/` Makefile de pspdev (genera EBOOT.PBP).
- `platform/vita/` CMake de VitaSDK (genera `bolitas_edu.vpk`, Title ID BOLI00001) con iconos de LiveArea (`tools/make_vita_assets.py`).
- `tools/make_font.py` genera `platform/pc/font_data.c` desde `tools/Outfit-Bold.ttf`.

## Tipografía
Outfit Bold (SIL Open Font License 1.1, ver `platform/pc/fonts/Outfit-OFL.txt`); hiragana de Noto Sans CJK JP (OFL, `NotoSansCJK-OFL.txt`): "a" y "g" de una
sola planta, clara para aprender a leer. Va incrustada en el código (no hace falta ninguna librería).
Para regenerarla: `python3 tools/make_font.py` (necesita Pillow).

## Compilar
- Pruebas: `make test`
- PC: `make pc && ./bolitas_pc` (necesita libsdl2-dev)
- Prueba automática con capturas: `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./bolitas_pc --smoke`
- GitHub Actions: `test`, `pc` (capturas), `psp` (EBOOT.PBP) y `vita` (`.vpk`).
- Vita: `cmake -S platform/vita -B build-vita && cmake --build build-vita` (con VitaSDK). Se instala en una Vita con homebrew copiando el `.vpk` e instalándolo con VitaShell.

## Pendiente
Proyecto Android, módulos por juego, y más juegos de Agilidad mental (comecocos, memoria, Simon, puzzles). La versión Vita está sin probar en consola real.
