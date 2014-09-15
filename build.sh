#!/bin/sh

gcc src/a2t.c src/depack.c src/sixpack.c -o a2t -std=c99 -g -Wall -Wextra
