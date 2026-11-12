CC ?= gcc
CFLAGS = -std=c99 -Wall -Wextra -O2

test: tests/test_edu.c core/edu.c core/edu.h
	$(CC) $(CFLAGS) -o test_edu tests/test_edu.c core/edu.c
	./test_edu

# Prototipo para PC (necesita libsdl2-dev)
pc: platform/pc/main.c core/edu.c core/edu.h
	$(CC) -std=gnu99 -Wall -Wextra -O2 -Icore -o bolitas_pc platform/pc/main.c core/edu.c \
		$(shell sdl2-config --cflags --libs) -lm

clean:
	rm -f test_edu bolitas_pc *.bmp

.PHONY: test pc clean
