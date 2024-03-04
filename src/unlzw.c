#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unlzw.h"

#define CIMPL 0

#if CIMPL
static char *input, *input0;    // Active and initial pointer
static int input_size;          // Size of input buffer
static char *output;            // Output buffer
static int output_size;

// Hijack standard fputc/fgetc
#define fputc   output_byte
#define fgetc   input_byte

static void output_byte(int code, FILE *outputf)
{
    printf("%c", code);
    // TODO: Add check for output_size
    *output++ = code;
}

static int input_byte(FILE *inputf)
{
    if (input - input0 >= input_size)
        return EOF;

    return *input++;
}

/* file.c */

static void writeBinary(FILE * output, int code);
static int readBinary(FILE * input);

static int leftover = 0;
static int leftoverBits;

static void writeBinary(FILE * output, int code) {
    if (leftover > 0) {
        int previousCode = (leftoverBits << 4) + (code >> 8);
        
        fputc(previousCode, output);
        fputc(code, output);
        
        leftover = 0; // no leftover now
    } else {
        leftoverBits = code & 0xF; // save leftover, the last 00001111
        leftover = 1;
        
        fputc(code >> 4, output);
    }
}

static int readBinary(FILE * input) {
    int code = fgetc(input);    
    if (code == EOF) return 0;

    if (leftover > 0) {
        code = (leftoverBits << 8) + code;
        
        leftover = 0;
    } else {
        int nextCode = fgetc(input);
        
        leftoverBits = nextCode & 0xF; // save leftover, the last 00001111
        leftover = 1;
        
        code = (code << 4) + (nextCode >> 4);
    }
    return code;
}

/* array.c */

typedef struct{
    int prefix; // prefix for byte > 255
    int character; // the last byte of the string
} DictElement;

static void dictionaryArrayAdd(int prefix, int character, int value);
static int dictionaryArrayPrefix(int value);
static int dictionaryArrayCharacter(int value);

static DictElement dictionaryArray[4095];

// add prefix + character to the dictionary
static void dictionaryArrayAdd(int prefix, int character, int value) {
    dictionaryArray[value].prefix = prefix;
    dictionaryArray[value].character = character;
}

static int dictionaryArrayPrefix(int value) {
    return dictionaryArray[value].prefix;
}

static int dictionaryArrayCharacter(int value) {
    return dictionaryArray[value].character;
}

/* dictionary.c */

enum {
    emptyPrefix = -1 // empty prefix for ASCII characters
};

// the "string" in the dictionary consists of the last byte of the string and an index to a prefix for that string
struct DictNode {
    int value; // the position in the list
    int prefix; // prefix for byte > 255
    int character; // the last byte of the string
    struct DictNode *next;
};

static void dictionaryInit();
static void appendNode(struct DictNode *node);
static void dictionaryDestroy();
static int dictionaryLookup(int prefix, int character);
static int dictionaryPrefix(int value);
static int dictionaryCharacter(int value);
static void dictionaryAdd(int prefix, int character, int value);

// the dictionary
static struct DictNode *dictionary, *tail;

// initialize the dictionary of ASCII characters @12bits
static void dictionaryInit() {
    int i;
    struct DictNode *node;
    for (i = 0; i < 256; i++) { // ASCII
        node = (struct DictNode *)malloc(sizeof(struct DictNode));
        node->prefix = emptyPrefix;
        node->character = i;
        appendNode(node);
    }       
}

// add node to the list
static void appendNode(struct DictNode *node) {
    if (dictionary != NULL) tail->next = node;
    else dictionary = node;
    tail = node;
    node->next = NULL;
}

// destroy the whole dictionary down to NULL
static void dictionaryDestroy() {
    while (dictionary != NULL) {
        dictionary = dictionary->next; /* the head now links to the next element */
    }
}

// is prefix + character in the dictionary?
static int dictionaryLookup(int prefix, int character) {
    struct DictNode *node;
    for (node = dictionary; node != NULL; node = node->next) { // ...traverse forward
        if (node->prefix == prefix && node->character == character) return node->value;
    }
    return emptyPrefix;
}

static int dictionaryPrefix(int value) {
    struct DictNode *node;
    for (node = dictionary; node != NULL; node = node->next) { // ...traverse forward
        if (node->value == value) return node->prefix;
    }
    return -1;
}

static int dictionaryCharacter(int value) {
    struct DictNode *node;
    for (node = dictionary; node != NULL; node = node->next) { // ...traverse forward
        if (node->value == value) {
            //printf("\nNODE %i %i %i\n", node->value, node->prefix, node->character);
            return node->character;
        }
    }
    return -1;
}

// add prefix + character to the dictionary
static void dictionaryAdd(int prefix, int character, int value) {
    struct DictNode *node;
    node = (struct DictNode *)malloc(sizeof(struct DictNode));
    node->value = value;
    node->prefix = prefix;
    node->character = character;
    //printf("\n(%i) = (%i) + (%i)\n", node->value, node->prefix, node->character);
    appendNode(node);
}

/* lzw.c */

enum {
    dictionarySize = 4095, // maximum number of entries defined for the dictionary (2^12 = 4096)
    codeLength = 12, // the codes which are taking place of the substrings
    maxValue = dictionarySize - 1
};

// function declarations
static void compress(FILE *inputFile, FILE *outputFile);
static void decompress(FILE *inputFile, FILE *outputFile);
static int decode(int code, FILE * outputFile);

// compression
static void compress(FILE *inputFile, FILE *outputFile) {    
    int prefix = getc(inputFile);
    if (prefix == EOF) {
        return;
    }
    int character;

    int nextCode;
    int index;
    
    // LZW starts out with a dictionary of 256 characters (in the case of 8 codeLength) and uses those as the "standard"
    //  character set.
    nextCode = 256; // next code is the next available string code
    dictionaryInit();
    
    // while (there is still data to be read)
    while ((character = getc(inputFile)) != EOF) { // ch = read a character;
        
        // if (dictionary contains prefix+character)
        if ((index = dictionaryLookup(prefix, character)) != -1) prefix = index; // prefix = prefix+character
        else { // ...no, try to add it
            // encode s to output file
            writeBinary(outputFile, prefix);
            
            // add prefix+character to dictionary
            if (nextCode < dictionarySize) dictionaryAdd(prefix, character, nextCode++);
            
            // prefix = character
            prefix = character; //... output the last string after adding the new one
        }
    }
    // encode s to output file
    writeBinary(outputFile, prefix); // output the last code
    
    if (leftover > 0) fputc(leftoverBits << 4, outputFile);
    
    // free the dictionary here
    dictionaryDestroy();
}

// decompression
// to reconstruct a string from an index we need to traverse the dictionary strings backwards, following each
//   successive prefix index until this prefix index is the empty index
static void decompress(FILE * inputFile, FILE * outputFile) {
    // int prevcode, currcode
    int previousCode; int currentCode;
    int nextCode = 256; // start with the same dictionary of 256 characters

    int firstChar;
    
    // prevcode = read in a code
    previousCode = readBinary(inputFile);
    if (previousCode == 0) {
        return;
    }
    fputc(previousCode, outputFile);
    
    // while (there is still data to read)
    while ((currentCode = readBinary(inputFile)) > 0) { // currcode = read in a code
    
        if (currentCode >= nextCode) {
            fputc(firstChar = decode(previousCode, outputFile), outputFile); // S+C+S+C+S exception [2.]
            //printf("%c", firstChar);
            //appendCharacter(firstChar = decode(previousCode, outputFile));
        } else firstChar = decode(currentCode, outputFile); // first character returned! [1.]
        
        // add a new code to the string table
        if (nextCode < dictionarySize) dictionaryArrayAdd(previousCode, firstChar, nextCode++);
        
        // prevcode = currcode
        previousCode = currentCode;
    }
    //printf("\n");
}

static int decode(int code, FILE * outputFile) {
    int character; int temp;

    if (code > 255) { // decode
        character = dictionaryArrayCharacter(code);
        temp = decode(dictionaryArrayPrefix(code), outputFile); // recursion
    } else {
        character = code; // ASCII
        temp = code;
    }
    fputc(character, outputFile);
    //printf("%c", character);
    //appendCharacter(character);
    return temp;
}

#else
#include <stdint.h>

static uint8_t le76, le77;
static uint16_t le6a, le6c, le6e, le70, le72, le74, bitshift, le82a, prevcode;

static uint16_t bitmask[5] = { 0x1ff, 0x3ff, 0x7ff, 0xfff, 0x1fff };

static unsigned char *input_ptr;
static int input_size;
static unsigned char *output_ptr;
static int output_size;

static int nextcode()
{
    uint16_t code, bx = 0, cx = 0;
    uint32_t bitstring;

    bx = le82a;
    code = prevcode;
    bitstring = ((code << 16) + bx) + bitshift;

    le82a = bitstring & 0xffff;
    prevcode = bitstring >> 16;
    cx = bx & 7;

    bx = bitstring = ((code << 16) + bx) >> 3;
    bitstring = input_ptr[bx] + (input_ptr[bx+1] << 8) + (input_ptr[bx+2] << 16);

    bitstring >>= cx;

    return bitstring & bitmask[bitshift - 9];
}

static void LZW_decode()
{
    uint8_t *stack = calloc(1, 65636);
    uint8_t *work_ptr = calloc(1, 65636);

    uint16_t ax = 0, bx = 0, cx = 0;
    uint32_t edi;

    int sp = 65536 - 1;

    le72 = 0;
    bitshift = 9;
    le70 = 0x102;
    le74 = 0x200;
    edi = 0;
    ax = 0;
    le6a = 0;
    le6c = 0;
    le6e = 0;
    le76 = 0;
    le77 = 0;
    le82a = 0;
    prevcode = 0;

    for (;;) {
        ax = nextcode();
        if (ax == 0x101)
            break;

        if (ax == 0x100) {
            bitshift = 9;
            le74 = 0x200;
            le70 = 0x102;
            ax = nextcode();
            le6a = ax;
            le6c = ax;
            le77 = ax;
            le76 = ax;

            output_ptr[edi] = ax;
            edi++;
            continue;
        }

        le6a = ax;
        le6e = ax;

        if (ax >= le70) {
            ax = le6c;
            le6a = ax;
            ax = (ax & 0xff00) + le76;
            sp--;
            stack[sp] = (uint8_t)ax;
            le72++;
        }

        while (le6a > 0xff) {
            bx = le6a * 3;
            ax = (ax & 0xff00) + work_ptr[bx+2];
            sp--;
            stack[sp] = ax;
            le72++;
            ax = work_ptr[bx] + (work_ptr[bx+1] << 8);
            le6a = ax;
        }

        ax = le6a;
        le76 = ax;
        le77 = ax;
        sp--;
        stack[sp] = ax;
        le72++;

        cx = le72;

        while (cx != 0) {
            ax = stack[sp];
            sp++;
            output_ptr[edi] = ax;
            edi++;
            cx--;
        }

        le72 = 0;
        bx = le70 * 3;

        work_ptr[bx+2] = le77;
        work_ptr[bx+1] = le6c >> 8;
        work_ptr[bx+0] = le6c;
        le70++;

        ax = le6e;
        le6c = ax;
        bx = le70;

        if (bx >= le74 && bitshift < 14) {
            bitshift++;
            le74 = le74 << 1;
        }
    }

    output_size = edi;
    free(stack);
    free(work_ptr);
}
#endif

int LZW_decompress(char *source, char *dest, int size)
{
#if CIMPL
    input = input0 = source;
    input_size = size;
    output = dest;

    decompress(0, 0);
#else
  input_ptr = (unsigned char *)source;
  input_size = size;
  output_ptr = (unsigned char *)dest;

  LZW_decode();
#endif

    return output_size;
}
