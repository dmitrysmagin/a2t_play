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
    uint8_t code, prevcode;
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

            code = input_ptr[input_idx++];
            dx = 0xff00 | code;
        }

        if (dx & 1) {
            if (input_idx >= input_size)
                break;

            code = input_ptr[input_idx++];
            work_ptr[edi] = code;
            edi = (edi + 1) & (N - 1);
            output_ptr[output_size++] = code;
            continue;
        }

        if (input_idx >= input_size)
            break;

        prevcode = code = input_ptr[input_idx++];

        if (input_idx >= input_size)
            break;

        code = input_ptr[input_idx++];
        ebx = ((code << 4) & 0xff00) | prevcode;

        int length = (code & 0x0f) + THRESHOLD + 1;

        do {
            output_ptr[output_size++] = work_ptr[edi] = work_ptr[ebx];

            ebx = (ebx + 1) & (N - 1);
            edi = (edi + 1) & (N - 1);
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
