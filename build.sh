#!/bin/sh

CFLAGS="-std=c99 -g -Wall -Wextra -Wno-unused-parameter -Wno-unused-function `sdl-config --cflags`"
LDFLAGS="`sdl-config --libs` -lm"

gcc	src/a2t.c src/depack.c src/sixpack.c \
	src/ymf262.c \
	-o a2t $CFLAGS $LDFLAGS
