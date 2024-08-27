#!/bin/sh

CFLAGS="-std=c99 -g -Wall -Wextra -Wno-unused-parameter -Wno-unused-function `sdl-config --cflags`"
LDFLAGS="-lSDL -mconsole"

gcc	src/sdl.c src/a2t.c src/depack.c src/sixpack.c src/unlzh.c src/unlzw.c src/unlzss.c src/opl3.c \
	src/debug.c -o a2t $CFLAGS $LDFLAGS
