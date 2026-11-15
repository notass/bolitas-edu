CC ?= gcc
CFLAGS = -std=c99 -Wall -Wextra -O2
CORE = core/edu.c core/rhythm.c core/paint.c
PCSRC = platform/pc/main.c platform/pc/gfx.c platform/pc/audio.c platform/pc/font_data.c

test: tests/test_edu.c tests/test_rhythm_paint.c $(CORE) core/edu.h core/rhythm.h core/paint.h
	$(CC) $(CFLAGS) -o test_edu tests/test_edu.c core/edu.c
	./test_edu
	$(CC) $(CFLAGS) -o test_rp tests/test_rhythm_paint.c $(CORE)
	./test_rp

# Prototipo para PC (necesita libsdl2-dev)
pc: $(PCSRC) $(CORE) platform/pc/gfx.h platform/pc/audio.h platform/pc/sdl_inc.h platform/pc/font_data.h
	$(CC) -std=gnu99 -Wall -Wextra -O2 -Icore -o bolitas_pc $(PCSRC) $(CORE) \
		$(shell sdl2-config --cflags --libs) -lm

clean:
	rm -f test_edu test_rp bolitas_pc *.bmp

.PHONY: test pc clean
