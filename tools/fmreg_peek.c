/* One-off: unpack A2M v9-14 songdata (ffver 12-14 = LZH) and print FMREG blob header + inference for instrument N. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/unlzh.h"

#define UINT32LE(P) ((uint32_t)(unsigned char)(P)[0] | ((uint32_t)(unsigned char)(P)[1] << 8) | \
                     ((uint32_t)(unsigned char)(P)[2] << 16) | ((uint32_t)(unsigned char)(P)[3] << 24))

#define tREGISTER_TABLE_DEF_V9_14_SIZE (15)
#define tFMREG_TABLE_V9_14_SIZE (3831)
#define A2M_SONGDATA_V9_14_FMREG_TABLE_P(P, I) ((uint8_t *)((P) + 14621 + (I) * tFMREG_TABLE_V9_14_SIZE))

static uint8_t fmreg_infer_length_from_cells(const uint8_t *src)
{
    uint8_t inferred = 0;

    for (unsigned int e = 0; e < 255; e++) {
        const uint8_t *cell = &src[6 + e * tREGISTER_TABLE_DEF_V9_14_SIZE];
        uint8_t dur = cell[14];
        int16_t fs = (int16_t)(cell[11] | (cell[12] << 8));
        uint8_t mf = (uint8_t)(cell[10] & 0xf0);

        if (dur != 0 || fs != 0 || mf != 0)
            inferred = (uint8_t)(e + 1);
    }

    return inferred;
}

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : "modules/top-2act.a2m";
    int ins_arg = argc > 2 ? atoi(argv[2]) : 19;

    if (ins_arg < 1 || ins_arg > 255) {
        fprintf(stderr, "instrument must be 1..255\n");
        return 1;
    }

    FILE *fp = fopen(path, "rb");
    if (!fp) {
        perror(path);
        return 1;
    }

    uint8_t hdr[16];
    if (fread(hdr, 1, 16, fp) != 16) {
        fprintf(stderr, "short read header\n");
        return 1;
    }

    if (memcmp(hdr, "_A2module_", 10) != 0) {
        fprintf(stderr, "not _A2module_\n");
        return 1;
    }

    uint8_t ffver = hdr[14];
    if (ffver < 9 || ffver > 14) {
        fprintf(stderr, "ffver %u: only v9-14 supported here\n", ffver);
        return 1;
    }

    uint32_t lens[17];
    uint8_t lb[17 * 4];

    if (fread(lb, 1, sizeof lb, fp) != sizeof lb) {
        fprintf(stderr, "short lens\n");
        return 1;
    }
    for (int i = 0; i < 17; i++)
        lens[i] = UINT32LE(lb + i * 4);

    long packed_off = ftell(fp);
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, packed_off, SEEK_SET);

    unsigned long packed_sz = (unsigned long)(fsize - packed_off);
    uint8_t *packed = malloc(packed_sz);
    if (!packed || fread(packed, 1, packed_sz, fp) != packed_sz) {
        fprintf(stderr, "read packed songdata fail\n");
        return 1;
    }
    fclose(fp);

    if (lens[0] > packed_sz) {
        fprintf(stderr, "len[0] %u > file remainder\n", lens[0]);
        return 1;
    }

#define A2M_SONGDATA_V9_14_SIZE (1138338)
    uint8_t *unpacked = calloc(1, A2M_SONGDATA_V9_14_SIZE);
    if (!unpacked)
        return 1;

    int dec = 0;
    if (ffver == 12 || ffver == 13 || ffver == 14)
        dec = LZH_decompress((char *)packed, (char *)unpacked, (int)lens[0], A2M_SONGDATA_V9_14_SIZE);
    else {
        fprintf(stderr, "ffver %u: add apack branch if needed\n", ffver);
        return 1;
    }

    if ((unsigned)dec != A2M_SONGDATA_V9_14_SIZE)
        fprintf(stderr, "note: LZH_decompress reported unpacked size %u (fixed layout %d)\n",
                (unsigned)dec, A2M_SONGDATA_V9_14_SIZE);

    int idx = ins_arg - 1;
    uint8_t *fm = A2M_SONGDATA_V9_14_FMREG_TABLE_P(unpacked, idx);

    uint8_t file_len = fm[0];
    uint8_t inferred = fmreg_infer_length_from_cells(fm);

    printf("instrument %d (1-based): file length byte=%u header[1..5]=%02x %02x %02x %02x %02x inferred_cells=%u\n",
           ins_arg, file_len, fm[1], fm[2], fm[3], fm[4], fm[5], inferred);

    /* Print first few non-empty cells */
    int shown = 0;
    for (unsigned e = 0; e < 255 && shown < 8; e++) {
        const uint8_t *cell = &fm[6 + e * tREGISTER_TABLE_DEF_V9_14_SIZE];
        uint8_t dur = cell[14];
        int16_t fs = (int16_t)(cell[11] | (cell[12] << 8));
        uint8_t mf = (uint8_t)(cell[10] & 0xf0);
        if (dur == 0 && fs == 0 && mf == 0)
            continue;
        printf("  cell[%u]: macro_flags=%02x freq_slide=%d dur=%u raw[10..14]=%02x %02x %02x %02x %02x\n",
               e, mf, (int)fs, dur, cell[10], cell[11], cell[12], cell[13], cell[14]);
        shown++;
    }

    free(unpacked);
    free(packed);
    return 0;
}
