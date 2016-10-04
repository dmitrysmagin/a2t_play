#!/bin/sh

CFLAGS="-std=c99 -g -Wall -Wextra -Wno-unused-parameter -Wno-unused-function `sdl-config --cflags`"

gcc	src/a2t.c src/depack.c src/sixpack.c \
	src/ymf262.c \
	-o a2t $CFLAGS `sdl-config --libs`
