#!/bin/sh

CFLAGS="-std=c99 -g -Wall -Wextra -Wno-unused-parameter -Wno-unused-function `sdl-config --cflags`"
LDFLAGS="`sdl-config --libs` -lm"

gcc	src/sdl.c src/a2t.c src/depack.c src/sixpack.c src/unlzh.c src/unlzw.c src/opl3.c \
	-o a2t $CFLAGS $LDFLAGS
