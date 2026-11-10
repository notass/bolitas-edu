CC ?= gcc
CFLAGS = -std=c99 -Wall -Wextra -O2

test: tests/test_edu.c core/edu.c core/edu.h
	$(CC) $(CFLAGS) -o test_edu tests/test_edu.c core/edu.c
	./test_edu

clean:
	rm -f test_edu

.PHONY: test clean
