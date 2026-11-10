# Bolitas Edu (nombre provisional)

Juego educativo rítmico para niñas desde 6 años, estilo LocoRoco/Patapon.
Español principal, inglés secundario. Android (SDL2) y PSP original (PSPSDK).

## Estructura
- `core/`  lógica de minijuegos en C99, sin dependencias ni malloc (PSP-friendly). Probada.
- `tests/` pruebas del núcleo: `make test`.
- `platform/` (pendiente) capa fina: ventana, entrada, audio, ficheros.
  - `android/` SDL2 + zonas táctiles que imitan los botones.
  - `psp/` PSPSDK/pspdev, pantalla 480x272, sin táctil.

## Minijuegos actuales (núcleo)
Sumas, restas, contar bolitas, series numéricas, sílabas (lectura),
vocabulario ES/EN, leer el reloj. Cada ronda trae 4 opciones = 4 tambores/botones.
La dificultad se adapta sola (`Progress`).

## Ideas siguientes
Formas y colores, memoria musical, animales y naturaleza, emociones,
mapa del mundo, abecedario, mayor/menor, dinero, meses y días.
