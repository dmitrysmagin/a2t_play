/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2002 Simon Peter <dn.tlp@gmx.net>, et al.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * debug.h - AdPlug Debug Logger
 * Copyright (c) 2002 Riven the Mage <riven@ok.ru>
 * Copyright (c) 2002 Simon Peter <dn.tlp@gmx.net>
 */

#include <stdio.h>
#include <stdarg.h>

#include "a2t.h"

#define DEBUG
#ifdef DEBUG

static FILE *log = NULL;

void AdPlug_LogFile(const char *filename)
{
    if (log)
        fclose(log);
    log = fopen(filename, "wt");
}

void AdPlug_LogWrite(const char *fmt, ...)
{
    va_list argptr;

    va_start(argptr, fmt);

    if (log) {
        vfprintf(log, fmt, argptr);
        fflush(log);
    } else
        vfprintf(stderr, fmt, argptr);

    va_end(argptr);
}

#else

void AdPlug_LogFile(char *filename)
{
    (void)filename;
}
void AdPlug_LogWrite(char *fmt, ...)
{
    (void)fmt;
}

#endif

#define DUMP_CHANS 20
#define DUMP_SLOTS 2

static void print_hex_block(FILE *out, const char *indent, const uint8_t *p, size_t len, size_t row_w)
{
    size_t i;
    for (i = 0; i < len; i++) {
        if (i % row_w == 0)
            fprintf(out, "%s%04zu:", indent, i);
        fprintf(out, " %02x", p[i]);
        if (i % row_w == row_w - 1 || i == len - 1)
            fprintf(out, "\n");
    }
}

static void dump_fm_inst_data(FILE *out, int chan, const tFM_INST_DATA *f)
{
    fprintf(out, "  fmpar_table[%d]:\n", chan);
    fprintf(out, "    multipM=%u ksrM=%u sustM=%u vibrM=%u tremM=%u\n",
            (unsigned)f->multipM, (unsigned)f->ksrM, (unsigned)f->sustM,
            (unsigned)f->vibrM, (unsigned)f->tremM);
    fprintf(out, "    multipC=%u ksrC=%u sustC=%u vibrC=%u tremC=%u\n",
            (unsigned)f->multipC, (unsigned)f->ksrC, (unsigned)f->sustC,
            (unsigned)f->vibrC, (unsigned)f->tremC);
    fprintf(out, "    volM=%u kslM=%u volC=%u kslC=%u\n",
            (unsigned)f->volM, (unsigned)f->kslM, (unsigned)f->volC, (unsigned)f->kslC);
    fprintf(out, "    decM=%u attckM=%u decC=%u attckC=%u\n",
            (unsigned)f->decM, (unsigned)f->attckM, (unsigned)f->decC, (unsigned)f->attckC);
    fprintf(out, "    relM=%u sustnM=%u relC=%u sustnC=%u\n",
            (unsigned)f->relM, (unsigned)f->sustnM, (unsigned)f->relC, (unsigned)f->sustnC);
    fprintf(out, "    wformM=%u wformC=%u connect=%u feedb=%u\n",
            (unsigned)f->wformM, (unsigned)f->wformC, (unsigned)f->connect, (unsigned)f->feedb);
}

static void dump_adtrack2_event(FILE *out, int chan, const tADTRACK2_EVENT *e)
{
    fprintf(out, "  event_table[%d]: note=0x%02x instr_def=%u\n",
            chan, e->note, (unsigned)e->instr_def);
    fprintf(out, "    eff[0]: def=0x%02x val=0x%02x\n", e->eff[0].def, e->eff[0].val);
    fprintf(out, "    eff[1]: def=0x%02x val=0x%02x\n", e->eff[1].def, e->eff[1].val);
}

static void dump_effect_pair(FILE *out, int chan, const char *name, const tEFFECT_TABLE t[DUMP_SLOTS][DUMP_CHANS])
{
    int s;
    for (s = 0; s < DUMP_SLOTS; s++) {
        fprintf(out, "  %s[%d][%d]: def=0x%02x val=0x%02x\n",
                name, s, chan, t[s][chan].def, t[s][chan].val);
    }
}

static void dump_macro(FILE *out, int chan, const tCH_MACRO_TABLE *m)
{
    fprintf(out, "  macro_table[%d]:\n", chan);
    fprintf(out, "    fmreg_pos=%u arpg_pos=%u vib_pos=%u\n",
            (unsigned)m->fmreg_pos, (unsigned)m->arpg_pos, (unsigned)m->vib_pos);
    fprintf(out, "    fmreg_duration=%u arpg_count=%u vib_count=%u vib_delay=%u\n",
            (unsigned)m->fmreg_duration, (unsigned)m->arpg_count,
            (unsigned)m->vib_count, (unsigned)m->vib_delay);
    fprintf(out, "    fmreg_ins=%u arpg_table=%u vib_table=%u arpg_note=%u\n",
            (unsigned)m->fmreg_ins, (unsigned)m->arpg_table,
            (unsigned)m->vib_table, (unsigned)m->arpg_note);
    fprintf(out, "    vib_paused=%d vib_freq=0x%04x\n", (int)m->vib_paused, (unsigned)m->vib_freq);
}

void dump_context_f(FILE *out, const tCHDATA *d)
{
    int chan, s;

    if (!out)
        out = stderr;

    if (!d) {
        fprintf(out, "dump_context_f: NULL tCHDATA\n");
        return;
    }

    fprintf(out, "=== tCHDATA %p ===\n", (const void *)d);

    for (chan = 0; chan < DUMP_CHANS; chan++) {
        fprintf(out, "--- channel %d ---\n", chan);
        dump_fm_inst_data(out, chan, &d->fmpar_table[chan]);
        fprintf(out, "  volume_lock=%d vol4op_lock=%d peak_lock=%d pan_lock=%d\n",
                (int)d->volume_lock[chan], (int)d->vol4op_lock[chan],
                (int)d->peak_lock[chan], (int)d->pan_lock[chan]);
        fprintf(out, "  modulator_vol=%u carrier_vol=%u\n",
                (unsigned)d->modulator_vol[chan], (unsigned)d->carrier_vol[chan]);
        dump_adtrack2_event(out, chan, &d->event_table[chan]);
        fprintf(out, "  voice_table[%d]=%u\n", chan, (unsigned)d->voice_table[chan]);
        fprintf(out, "  freq_table[%d]=0x%04x zero_fq_table[%d]=0x%04x\n",
                chan, (unsigned)d->freq_table[chan], chan, (unsigned)d->zero_fq_table[chan]);
        dump_effect_pair(out, chan, "effect_table", d->effect_table);
        for (s = 0; s < DUMP_SLOTS; s++) {
            fprintf(out, "  fslide_table[%d][%d]=%u\n", s, chan, (unsigned)d->fslide_table[s][chan]);
        }
        dump_effect_pair(out, chan, "glfsld_table", d->glfsld_table);
        for (s = 0; s < DUMP_SLOTS; s++) {
            fprintf(out, "  porta_table[%d][%d]: freq=0x%04x speed=%u\n",
                    s, chan, (unsigned)d->porta_table[s][chan].freq,
                    (unsigned)d->porta_table[s][chan].speed);
        }
        fprintf(out, "  portaFK_table[%d]=%d\n", chan, (int)d->portaFK_table[chan]);
        for (s = 0; s < DUMP_SLOTS; s++) {
            fprintf(out, "  arpgg_table[%d][%d]: state=%u note=%u add1=%u add2=%u\n",
                    s, chan,
                    (unsigned)d->arpgg_table[s][chan].state,
                    (unsigned)d->arpgg_table[s][chan].note,
                    (unsigned)d->arpgg_table[s][chan].add1,
                    (unsigned)d->arpgg_table[s][chan].add2);
        }
        for (s = 0; s < DUMP_SLOTS; s++) {
            fprintf(out,
                    "  vibr_table[%d][%d]: pos=%u dir=%u speed=%u depth=%u fine=%d\n",
                    s, chan,
                    (unsigned)d->vibr_table[s][chan].pos,
                    (unsigned)d->vibr_table[s][chan].dir,
                    (unsigned)d->vibr_table[s][chan].speed,
                    (unsigned)d->vibr_table[s][chan].depth,
                    (int)d->vibr_table[s][chan].fine);
        }
        for (s = 0; s < DUMP_SLOTS; s++) {
            fprintf(out,
                    "  trem_table[%d][%d]: pos=%u dir=%u speed=%u depth=%u fine=%d\n",
                    s, chan,
                    (unsigned)d->trem_table[s][chan].pos,
                    (unsigned)d->trem_table[s][chan].dir,
                    (unsigned)d->trem_table[s][chan].speed,
                    (unsigned)d->trem_table[s][chan].depth,
                    (int)d->trem_table[s][chan].fine);
        }
        for (s = 0; s < DUMP_SLOTS; s++) {
            fprintf(out, "  retrig_table[%d][%d]=%u\n", s, chan, (unsigned)d->retrig_table[s][chan]);
        }
        for (s = 0; s < DUMP_SLOTS; s++) {
            fprintf(out, "  tremor_table[%d][%d]: pos=%d volM=%u volC=%u\n",
                    s, chan, (int)d->tremor_table[s][chan].pos,
                    (unsigned)d->tremor_table[s][chan].volM,
                    (unsigned)d->tremor_table[s][chan].volC);
        }
        fprintf(out, "  panning_table[%d]=%u\n", chan, (unsigned)d->panning_table[chan]);
        dump_effect_pair(out, chan, "last_effect", d->last_effect);
        fprintf(out, "  volslide_type[%d]=%u\n", chan, (unsigned)d->volslide_type[chan]);
        fprintf(out, "  notedel_table[%d]=%u notecut_table[%d]=%u\n",
                chan, (unsigned)d->notedel_table[chan],
                chan, (unsigned)d->notecut_table[chan]);
        fprintf(out, "  ftune_table[%d]=%d\n", chan, (int)d->ftune_table[chan]);
        fprintf(out, "  keyoff_loop[%d]=%d\n", chan, (int)d->keyoff_loop[chan]);
        fprintf(out, "  loopbck_table[%d]=%u\n", chan, (unsigned)d->loopbck_table[chan]);
        fprintf(out, "  loop_table[%d][0..255]:\n", chan);
        print_hex_block(out, "    ", d->loop_table[chan], 256, 16);
        fprintf(out, "  reset_chan[%d]=%d\n", chan, (int)d->reset_chan[chan]);
        dump_macro(out, chan, &d->macro_table[chan]);
    }

    fprintf(out, "=== end tCHDATA ===\n");
}

void dump_context(const tCHDATA *d)
{
    dump_context_f(stdout, d);
}
