/*
 * dump_fmreg - dump FMREG table entries for a given instrument from an A2T file
 *
 * Build with:
 *   gcc -std=c99 -g -Wall -Wextra -I src -o dump_fmreg dump_fmreg.c \
 *       src/depack.c src/sixpack.c src/unlzh.c src/unlzw.c src/unlzss.c \
 *       src/opl3.c src/debug.c -lm
 *
 * Usage: dump_fmreg <file.a2t> [instrument_number]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Include the main player source so we get all static data */
#include "src/a2t.c"

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: dump_fmreg <file.a2t> [instrument_number]\n");
        return 1;
    }

    int target_ins = 5; /* default: instrument 5 */
    if (argc >= 3) {
        target_ins = atoi(argv[2]);
        if (target_ins < 1 || target_ins > 255) {
            fprintf(stderr, "Instrument number must be 1..255\n");
            return 1;
        }
    }

    a2t_init(44100);

    char *data = a2t_load(argv[1]);
    if (!data) {
        fprintf(stderr, "Failed to load %s\n", argv[1]);
        return 1;
    }

    if (!a2t_play(data)) {
        fprintf(stderr, "Failed to play/parse %s\n", argv[1]);
        return 1;
    }

    /* After a2t_play, instrinfo is populated */
    unsigned int ninsts = instrinfo->count;
    printf("File: %s\n", argv[1]);
    printf("Number of instruments: %u\n", ninsts);

    if (target_ins > ninsts) {
        printf("Instrument %d not found (max is %u)\n", target_ins, ninsts);
        a2t_stop();
        a2t_shut();
        return 1;
    }

    tINSTR_DATA_EXT *inst = get_instr(target_ins);
    if (!inst) {
        printf("Instrument %d not found\n", target_ins);
        a2t_stop();
        a2t_shut();
        return 1;
    }

    printf("\n=== Instrument %d ===\n", target_ins);
    printf("Vibrato table: %d\n", inst->vibrato);
    printf("Arpeggio table: %d\n", inst->arpeggio);

    tFMREG_TABLE *fmreg = inst->fmreg;
    if (!fmreg) {
        printf("No FMREG table for this instrument.\n");
        a2t_stop();
        a2t_shut();
        return 1;
    }

    printf("FMREG Table:\n");
    printf("  length:       %u\n", fmreg->length);
    printf("  loop_begin:   %u\n", fmreg->loop_begin);
    printf("  loop_length:  %u\n", fmreg->loop_length);
    printf("  keyoff_pos:   %u\n", fmreg->keyoff_pos);
    printf("  arpeggio_tbl: %u\n", fmreg->arpeggio_table);
    printf("  vibrato_tbl:  %u\n", fmreg->vibrato_table);
    printf("\n  FMREG entries (%u total):\n", fmreg->length);

    unsigned int max_entries = fmreg->length;
    if (max_entries > 255) max_entries = 255;

    for (unsigned int e = 0; e < max_entries; e++) {
        tREGISTER_TABLE_DEF *d = &fmreg->data[e];
        printf("\n  --- Entry %u ---\n", e);
        printf("    freq_slide:  %d (0x%04x)\n", d->freq_slide, (uint16_t)d->freq_slide);
        printf("    panning:     %u\n", d->panning);
        printf("    duration:    %u\n", d->duration);
        printf("    macro_flags: 0x%02x\n", d->macro_flags);
        printf("    FM data:\n");
        printf("      multipM=%u ksrM=%u sustM=%u vibrM=%u tremM=%u\n",
               d->fm.multipM, d->fm.ksrM, d->fm.sustM, d->fm.vibrM, d->fm.tremM);
        printf("      multipC=%u ksrC=%u sustC=%u vibrC=%u tremC=%u\n",
               d->fm.multipC, d->fm.ksrC, d->fm.sustC, d->fm.vibrC, d->fm.tremC);
        printf("      volM=%u kslM=%u  volC=%u kslC=%u\n",
               d->fm.volM, d->fm.kslM, d->fm.volC, d->fm.kslC);
        printf("      attckM=%u decM=%u  attckC=%u decC=%u\n",
               d->fm.attckM, d->fm.decM, d->fm.attckC, d->fm.decC);
        printf("      sustnM=%u relM=%u  sustnC=%u relC=%u\n",
               d->fm.sustnM, d->fm.relM, d->fm.sustnC, d->fm.relC);
        printf("      wformM=%u wformC=%u  connect=%u feedb=%u\n",
               d->fm.wformM, d->fm.wformC, d->fm.connect, d->fm.feedb);
    }

    a2t_stop();
    a2t_shut();
    return 0;
}
