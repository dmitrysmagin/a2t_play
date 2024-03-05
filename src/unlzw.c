#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unlzw.h"

#define CIMPL 0

#include <stdint.h>

static uint8_t le76, le77;
static uint16_t le6a, le6c, le6e, le70, stringlength, le74, bitshift, prevshift, prevcode;
static uint32_t prevbitstring;

static uint16_t bitmask[5] = { 0x1ff, 0x3ff, 0x7ff, 0xfff, 0x1fff };

static unsigned char *input_ptr;
static int input_size;
static unsigned char *output_ptr;
static int output_size;

static int nextcode()
{
    uint16_t code, shift, cx;
    uint32_t bitstring;

    shift = prevshift;
    //code = prevcode;
    bitstring = prevbitstring/*(code << 16) + shift*/ + bitshift;


    prevshift = bitstring & 0xffff;
    prevcode = bitstring >> 16;

    cx = shift & 7;

    shift = prevbitstring/*((code << 16) + shift)*/ >> 3;
    bitstring = input_ptr[shift] + (input_ptr[shift+1] << 8) + (input_ptr[shift+2] << 16);

    bitstring >>= cx;

    prevbitstring += bitshift;

    return bitstring & bitmask[bitshift - 9];
}

static void LZW_decode()
{
    uint8_t *stack = calloc(1, 65636);
    uint8_t *work_ptr = calloc(1, 65636);

    uint16_t code;
    uint32_t output_idx;

    int sp = 65536 - 1;

    stringlength = 0;
    bitshift = 9;
    le70 = 0x102;
    le74 = 0x200;
    output_idx = 0;
    code = 0;
    le6a = 0;
    le6c = 0;
    le6e = 0;
    le76 = 0;
    le77 = 0;
    prevbitstring = 0;
    prevshift = 0;
    prevcode = 0;

    for (;;) {
        code = nextcode();
        if (code == 0x101)
            break;

        if (code == 0x100) {
            bitshift = 9;
            le74 = 0x200;
            le70 = 0x102;
            code = nextcode();
            le6a = code;
            le6c = code;
            le77 = code;
            le76 = code;

            output_ptr[output_idx++] = code;
            continue;
        }

        le6a = code;
        le6e = code;

        if (code >= le70) {
            code = le6c;
            le6a = code;
            code = (code & 0xff00) + le76;
            sp--;
            stack[sp] = code;
            stringlength++;
        }

        while (le6a > 0xff) {
            code = (code & 0xff00) + work_ptr[(le6a * 3) + 2];
            sp--;
            stack[sp] = code;
            stringlength++;
            code = work_ptr[le6a * 3] + (work_ptr[(le6a * 3) + 1] << 8);
            le6a = code;
        }

        code = le6a;
        le76 = code;
        le77 = code;
        sp--;
        stack[sp] = code;
        stringlength++;

        while (stringlength--) {
            output_ptr[output_idx++] = stack[sp++];
        }

        stringlength = 0;

        work_ptr[(le70 * 3) + 2] = le77;
        work_ptr[(le70 * 3) + 1] = le6c >> 8;
        work_ptr[(le70 * 3) + 0] = le6c;
        le70++;

        code = le6e;
        le6c = code;

        if (le70 >= le74 && bitshift < 14) {
            bitshift++;
            le74 <<= 1;
        }
    }

    output_size = output_idx;
    free(stack);
    free(work_ptr);
}

int LZW_decompress(char *source, char *dest, int size)
{
    input_ptr = (unsigned char *)source;
    input_size = size;
    output_ptr = (unsigned char *)dest;

    LZW_decode();

    return output_size;
}
