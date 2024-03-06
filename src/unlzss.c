#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unlzss.h"

static int ibufCount, ibufSize;
static int input_size, output_size;
static unsigned char *input_ptr, *output_ptr, *work_ptr;

static uint8_t al, cl, ch, cf;
static uint16_t dx;
static uint32_t ebx, edi;

#define N_BITS      12
#define F_BITS      4
#define THRESHOLD   2
#define N           (1 << N_BITS)
#define F           ((1 << F_BITS) + THRESHOLD)

static char GetChar() {
    if (ibufCount < ibufSize) {
        al = input_ptr[ibufCount];
        ibufCount++;
        cf = 0;
    } else {
        cf = 1;
    }

    return al;
}

static void PutChar()
{
    output_ptr[output_size] = al;
    output_size++;
}

static void LZSS_decode() {
    ibufCount = 0;
    ibufSize = input_size;

    output_size = 0;
    ebx = 0;
    dx = 0;
    edi = N - F;
j1:
    dx = dx >> 1;
    if ((dx >> 8) != 0)
        goto j2;
    GetChar();
    if (cf == 1) goto j5;
    dx = 0xff00 | al;

j2:
    if ((dx & 1) == 0)
        goto j3;
    GetChar();
    if (cf == 1) goto j5;

    work_ptr[edi] = al;

    edi = (edi + 1) & (N - 1);

    PutChar();
    goto j1;
j3:
    GetChar();
    if (cf == 1) goto j5;
    ch = al;
    GetChar();
    if (cf == 1) goto j5;

    ebx = (al << 4) & 0xff00;
    ebx = ebx | ch;

    cl = (al & 0x0f) + THRESHOLD;
    cl++;
j4:
    ebx = ebx & (N - 1);

    al = work_ptr[ebx];

    work_ptr[edi] = al;

    edi++;
    edi = edi & (N - 1);
    PutChar();
    ebx++;
    cl--;
    if (cl > 0) goto j4;
    goto j1;
j5:
    ;
}

int LZSS_decompress(char *source, char *dest, int size)
{
    input_ptr = (unsigned char *)source;
    input_size = size;
    output_ptr = (unsigned char *)dest;
    work_ptr = calloc(1, 65536);

    LZSS_decode();

    free(work_ptr);

    return output_size;
}
