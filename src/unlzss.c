#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unlzss.h"

static int input_size, output_size;
static unsigned char *input_ptr, *output_ptr;

#define N_BITS      12
#define F_BITS      4
#define THRESHOLD   2
#define N           (1 << N_BITS)
#define F           ((1 << F_BITS) + THRESHOLD)

static void LZSS_decode() {
    int input_idx = 0;
    uint8_t al, ch;
    uint16_t dx;
    uint32_t ebx, edi;

    unsigned char *work_ptr = calloc(1, 65536);

    output_size = 0;
    ebx = 0;
    dx = 0;
    edi = N - F;

    for (;;) {
        dx = dx >> 1;

        if (!(dx >> 8)) {
            if (input_idx >= input_size)
                break;

            al = input_ptr[input_idx++];

            dx = 0xff00 | al;
        }

        if (dx & 1) {
            if (input_idx >= input_size)
                break;

            al = input_ptr[input_idx++];

            work_ptr[edi] = al;

            edi = (edi + 1) & (N - 1);

            output_ptr[output_size++] = al;
            continue;
        }

        if (input_idx >= input_size)
            break;

        al = input_ptr[input_idx++];
        ch = al;

        if (input_idx >= input_size)
            break;

        al = input_ptr[input_idx++];

        ebx = (al << 4) & 0xff00;
        ebx = ebx | ch;

        int length = (al & 0x0f) + THRESHOLD + 1;

        do {
            al = work_ptr[ebx];
            work_ptr[edi] = al;

            ebx = (ebx + 1) & (N - 1);
            edi = (edi + 1) & (N - 1);

            output_ptr[output_size++] = al;
        } while (--length > 0);
    }

    free(work_ptr);
}

int LZSS_decompress(char *source, char *dest, int size)
{
    input_ptr = (unsigned char *)source;
    input_size = size;
    output_ptr = (unsigned char *)dest;
    
    LZSS_decode();

    return output_size;
}
