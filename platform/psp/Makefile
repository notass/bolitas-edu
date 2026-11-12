# PSP original (pspdev). Compilar: make -C platform/psp
TARGET = bolitas_edu
OBJS = ../pc/main.o ../../core/edu.o

CFLAGS = -O2 -G0 -Wall -std=gnu99 -I../../core
CXXFLAGS = $(CFLAGS)
ASFLAGS = $(CFLAGS)

# SDL2 para PSP: pspdev instala su configuración junto al toolchain
SDL_CFLAGS := $(shell psp-pkg-config --cflags sdl2 2>/dev/null || sdl2-config --cflags)
SDL_LIBS   := $(shell psp-pkg-config --libs sdl2 2>/dev/null || sdl2-config --libs)
CFLAGS += $(SDL_CFLAGS)
LIBS = $(SDL_LIBS) -lm

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Bolitas Edu

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
