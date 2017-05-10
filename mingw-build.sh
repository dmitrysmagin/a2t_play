#!/bin/sh

CFLAGS="-std=c99 -g -Wall -Wextra -Wno-unused-parameter -Wno-unused-function `sdl-config --cflags`"
CFLAGS+=" -mno-ms-bitfields"
LDFLAGS="-lSDL -mconsole"

gcc	src/a2t.c src/depack.c src/sixpack.c \
	src/ymf262.c \
	-o a2t $CFLAGS $LDFLAGS
