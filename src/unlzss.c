#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unlzss.h"

static int input_size, output_size;
static unsigned char *input_ptr, *output_ptr, *work_ptr;

#define N_BITS      12
#define F_BITS      4
#define THRESHOLD   2
#define N           (1 << N_BITS)
#define F           ((1 << F_BITS) + THRESHOLD)

static void LZSS_decode() {
    int ibufCount, ibufSize;
    uint8_t al, cl, ch;
    uint16_t dx;
    uint32_t ebx, edi;

    work_ptr = calloc(1, 65536);

    ibufCount = 0;
    ibufSize = input_size;

    output_size = 0;
    ebx = 0;
    dx = 0;
    edi = N - F;

    for (;;) {
        dx = dx >> 1;

        if ((dx >> 8) == 0) {
            if (ibufCount >= ibufSize)
                break;

            al = input_ptr[ibufCount++];

            dx = 0xff00 | al;
        }

        if ((dx & 1) != 0) {
            if (ibufCount >= ibufSize)
                break;

            al = input_ptr[ibufCount++];

            work_ptr[edi] = al;

            edi = (edi + 1) & (N - 1);

            output_ptr[output_size++] = al;
            continue;
        }

        if (ibufCount >= ibufSize)
            break;

        al = input_ptr[ibufCount++];
        ch = al;

        if (ibufCount >= ibufSize)
            break;

        al = input_ptr[ibufCount++];

        ebx = (al << 4) & 0xff00;
        ebx = ebx | ch;

        cl = (al & 0x0f) + THRESHOLD;
        cl++;

        do {
            ebx = ebx & (N - 1);

            al = work_ptr[ebx];

            work_ptr[edi] = al;

            edi++;
            edi = edi & (N - 1);

            output_ptr[output_size++] = al;
            ebx++;
            cl--;
        } while (cl > 0);
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
