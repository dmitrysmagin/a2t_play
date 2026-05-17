/*
 * a2m_dump - dump OPL3 register writes from an A2M/A2T file
 *
 * Build with -Dclocks to enable register dump output.
 * Automatically creates <input_basename>.reg with the register dump.
 *
 * This #include's a2t.c so all static functions/variables are accessible.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#ifdef clocks
/* nothing needed, a2t.c's opl_out() has the #ifdef clocks dump */
#endif

int frames_dumped = 0;
static volatile int sigint_received = 0;

#include "../src/a2t.c"

static void handle_sigint(int sig)
{
    (void)sig;
    sigint_received = 1;
    songend = true;
    play_status = isStopped;
}

/*
 * Instrumentation: build a2m_dump with -DA2M_DUMP_CONTEXT to emit dump_context_f(stderr)
 * on selected IRQ frame numbers (see static list in a2m_dump_context_at_irq_frames).
 * Example: ... -DA2M_DUMP_CONTEXT -Dclocks -o a2m_dump ...
 * Run: ./a2m_dump tune.a2m /dev/null 40000 2>ctx.log
 * top-2act IRQ 10893–10902: compact ch9 trace only (no dump_context_f).
 */

#ifdef A2M_DUMP_CONTEXT
#include "debug.h"

static bool a2m_dump_context_irq_requested(void)
{
    static const int want[] = {
        /* badapple: primary shadow_regs[0][0xa5] vs Pascal @ IRQ 14878 (logical chan 4, regoffs_n=5) */
        14874, 14875, 14876, 14877, 14878, 14879, 14880,
        /* fm-troni: just before / at / after first Pascal-C divergence (0xA2), plus later bursts */
        15259, 15260, 15261, 15262, 15263,
        15620, 15621, 15622,
        34556, 34557, 34558, 34559,
        /* fank5: secondary shadow_regs[1][0xb0] key-on vs Pascal ~IRQ 47232 (chan index 10, regoffs_n=0x100) */
        47228, 47229, 47230, 47231, 47232, 47233, 47234, 47235, 47236,
        /* fm63b_rv: ~IRQ 5305 secondary shadow_regs[1][0xb1] key-on vs Pascal (logical chan ~12 / regoffs 0x101) */
        5302, 5303, 5304, 5305, 5306, 5307, 5308, 5309, 5310, 5311, 5312, 5313, 5314, 5315,
        /* top-2act: bank1 shadow_regs[1][0xa3] vs Pascal ~10895 (logical chan 9 / track 10, regoffs_n=0x103) */
        10893, 10894, 10895, 10896, 10897, 10898, 10899, 10900, 10901, 10902,
        /* samsara: ch10 freq_table divergence starting at frame 20854 */
        20853, 20854, 20855, 20856, 20857, 20858, 20859, 20860,
        20915, 20916, 20917, 20918, 20919, 20920, 20921,
        /* 4thcoast: vib_freq divergence around frame 29100 */
        29095, 29096, 29097, 29098, 29099, 29100, 29101, 29102, 29103, 29104, 29105,
        -1
    };
    int i;

    for (i = 0; want[i] >= 0; i++) {
        if (frames_dumped == want[i])
            return true;
    }
    return false;
}

/* Compact trace for top-2act — avoids multi-screen dump_context_f at each IRQ.
 *
 * Semantics (see a2t.c poll_proc / play_line / update_song_position):
 * - When poll_proc runs with ticks crossing speed, play_line() reads pattern row current_line,
 *   then update_song_position() increments current_line. stderr "row" is that post-advance index.
 * - a2t_update() may call frame_hook multiple times per buffer; only ticklooper==0 iterations run
 *   poll_proc(), so consecutive irq_frame lines often share the same ticks/row/event_table.
 */
static void a2m_dump_top2act_ch9_compact(void)
{
    const int c = 9;
    tCH_MACRO_TABLE *mt = &ch->macro_table[c];
    tFMREG_TABLE *rt = get_fmreg_table(mt->fmreg_ins);
    tVIBRATO_TABLE *vt = get_vibrato_table(mt->vib_table);
    uint16_t freq = ch->freq_table[c];
    uint16_t vlow = mt->vib_freq & 0x1fff;
    int8_t vcell = 0;

    fprintf(stderr,
            "\n######## top-2act irq_frame=%d ticks=%d row=%u pattern=%u order=%u ########\n",
            frames_dumped, ticks, (unsigned)current_line, (unsigned)current_pattern,
            (unsigned)current_order);
    fprintf(stderr,
            "engine: speed=%u tick0=%d ticks=%d (row=current_line after poll_proc; pattern row last "
            "played when boundary crossed this poll_proc was one less, modulo pattern)\n",
            (unsigned)speed, tick0, ticks);
    fprintf(stderr,
            "bank1 pitch slot ch9: shadow[1][a3/b3]=0x%02x/0x%02x  freq_table[9]=0x%04x key_on_hi=%d\n",
            shadow_regs[1][0xa3], shadow_regs[1][0xb3], (unsigned)freq,
            (freq & 0x2000) ? 1 : 0);
    fprintf(stderr,
            "ch9 regoffs_n=0x%03x 4op_hi=%d 4op_lo=%d voice_ins=%u event_note=0x%02x instr_def=%u\n",
            (unsigned)regoffs_n(c), (int)is_4op_chan_hi(c), (int)is_4op_chan_lo(c),
            (unsigned)ch->voice_table[c], ch->event_table[c].note,
            (unsigned)ch->event_table[c].instr_def);
    fprintf(stderr,
            "ch9 macro: fmreg_ins=%u fmreg_pos=%u fmreg_duration=%u "
            "arpg_table=%u arpg_pos=%u arpg_count=%u\n",
            (unsigned)mt->fmreg_ins, (unsigned)mt->fmreg_pos, (unsigned)mt->fmreg_duration,
            (unsigned)mt->arpg_table, (unsigned)mt->arpg_pos, (unsigned)mt->arpg_count);
    fprintf(stderr,
            "ch9 macro vib: vib_table=%u vib_pos=%u vib_count=%u vib_freq=0x%04x vib_delay=%u vib_paused=%d\n",
            (unsigned)mt->vib_table, (unsigned)mt->vib_pos, (unsigned)mt->vib_count,
            (unsigned)mt->vib_freq, (unsigned)mt->vib_delay, (int)mt->vib_paused);
    if (vt && vt->length) {
        fprintf(stderr,
                "  vibrato_tbl: len=%u speed=%u delay_hdr=%u loop_b=%u loop_len=%u keyoff_pos=%u\n",
                (unsigned)vt->length, (unsigned)vt->speed, (unsigned)vt->delay,
                (unsigned)vt->loop_begin, (unsigned)vt->loop_length,
                (unsigned)vt->keyoff_pos);
        if (mt->vib_pos >= 1 && mt->vib_pos <= vt->length) {
            vcell = vt->data[mt->vib_pos - 1];
            fprintf(stderr, "  data[vib_pos=%u]=%d\n", (unsigned)mt->vib_pos, (int)vcell);
        } else {
            fprintf(stderr, "  data[vib_pos=%u]=(out of range for len=%u)\n",
                    (unsigned)mt->vib_pos, (unsigned)vt->length);
        }
    } else {
        fprintf(stderr, "  vibrato_tbl: (null or len 0)\n");
    }

    if (rt && rt->length) {
        fprintf(stderr,
                "ch9 fmreg_tbl: len=%u keyoff_pos=%u global_speed=%u\n",
                (unsigned)rt->length, (unsigned)rt->keyoff_pos, (unsigned)speed);
        if (mt->fmreg_pos >= 1 && mt->fmreg_pos <= rt->length) {
            const tREGISTER_TABLE_DEF *d = &rt->data[mt->fmreg_pos - 1];
            uint32_t dis = 0;

            if (mt->fmreg_ins >= 1 && mt->fmreg_ins <= instrinfo->count)
                dis = instrinfo->instruments[mt->fmreg_ins - 1].dis_fmreg_cols;
            fprintf(stderr,
                    "  active_cell[%u]: dur=%u freq_slide=%d macro_flags=0x%02x "
                    "dis_fmreg_cols=0x%08x col26_freq_slide_active=%d\n",
                    (unsigned)(mt->fmreg_pos - 1), (unsigned)d->duration,
                    (int)d->freq_slide, (unsigned)d->macro_flags,
                    (unsigned)dis, (int)((dis & (1u << 26)) == 0));
        } else {
            fprintf(stderr, "  active_cell: fmreg_pos=%u (idle/end)\n",
                    (unsigned)mt->fmreg_pos);
        }
    } else {
        fprintf(stderr, "ch9 fmreg_tbl: (null or len 0)\n");
    }

    fprintf(stderr,
            "ch9 partner ch10: freq_table[10]=0x%04x shadow[1][a0/b0]=0x%02x/0x%02x\n",
            (unsigned)ch->freq_table[10],
            shadow_regs[1][0xa0], shadow_regs[1][0xb0]);
    fprintf(stderr,
            "ch9 effects: eff_s0=%02x/%02x eff_s1=%02x/%02x last_s0=%02x/%02x last_s1=%02x/%02x "
            "fslide=%u/%u porta_s0=%u@%u porta_s1=%u@%u ftune=%d\n",
            ch->effect_table[0][c].def, ch->effect_table[0][c].val,
            ch->effect_table[1][c].def, ch->effect_table[1][c].val,
            ch->last_effect[0][c].def, ch->last_effect[0][c].val,
            ch->last_effect[1][c].def, ch->last_effect[1][c].val,
            (unsigned)ch->fslide_table[0][c], (unsigned)ch->fslide_table[1][c],
            (unsigned)ch->porta_table[0][c].freq, (unsigned)ch->porta_table[0][c].speed,
            (unsigned)ch->porta_table[1][c].freq, (unsigned)ch->porta_table[1][c].speed,
            (int)ch->ftune_table[c]);

    fprintf(stderr,
            "ch9/ch10 glfsld: s0=%02x/%02x s1=%02x/%02x | ch10 eff: s0=%02x/%02x s1=%02x/%02x\n",
            ch->glfsld_table[0][9].def, ch->glfsld_table[0][9].val,
            ch->glfsld_table[1][9].def, ch->glfsld_table[1][9].val,
            ch->effect_table[0][10].def, ch->effect_table[0][10].val,
            ch->effect_table[1][10].def, ch->effect_table[1][10].val);
    fprintf(stderr, "ch9 pattern vibrato vibr_table[0]: pos=%u dir=%u speed=%u depth=%u fine=%d\n",
            (unsigned)ch->vibr_table[0][9].pos, (unsigned)ch->vibr_table[0][9].dir,
            (unsigned)ch->vibr_table[0][9].speed, (unsigned)ch->vibr_table[0][9].depth,
            (int)ch->vibr_table[0][9].fine);
    fprintf(stderr, "ch9 pattern vibrato vibr_table[1]: pos=%u dir=%u speed=%u depth=%u fine=%d\n",
            (unsigned)ch->vibr_table[1][9].pos, (unsigned)ch->vibr_table[1][9].dir,
            (unsigned)ch->vibr_table[1][9].speed, (unsigned)ch->vibr_table[1][9].depth,
            (int)ch->vibr_table[1][9].fine);
    fprintf(stderr, "ch10 pattern vibrato vibr_table[0]: pos=%u dir=%u speed=%u depth=%u fine=%d\n",
            (unsigned)ch->vibr_table[0][10].pos, (unsigned)ch->vibr_table[0][10].dir,
            (unsigned)ch->vibr_table[0][10].speed, (unsigned)ch->vibr_table[0][10].depth,
            (int)ch->vibr_table[0][10].fine);
    fprintf(stderr, "ch10 pattern vibrato vibr_table[1]: pos=%u dir=%u speed=%u depth=%u fine=%d\n",
            (unsigned)ch->vibr_table[1][10].pos, (unsigned)ch->vibr_table[1][10].dir,
            (unsigned)ch->vibr_table[1][10].speed, (unsigned)ch->vibr_table[1][10].depth,
            (int)ch->vibr_table[1][10].fine);

    /* Logical pattern index at divergence is 5 (see stderr header pattern=%u).
     * Prefer correlating cells with event_table on irq_frame where row just advanced (e.g. 10895);
     * same-pattern snapshot kept at 10893 for backwards-compatible logs. */
    if (frames_dumped == 10893 || frames_dumped == 10895) {
        const unsigned pat = 5;

        fprintf(stderr,
                "pattern %u rows 0-2: global slide/vibrato/portamento OR ch8-11 OR row>=1 non-empty:\n",
                pat);
        for (unsigned row = 0; row <= 2; row++) {
            for (int cc = 0; cc < songinfo->nm_tracks; cc++) {
                tADTRACK2_EVENT *ev = get_event_p((int)pat, cc, (int)row);
                uint8_t d0 = ev->eff[0].def;
                uint8_t d1 = ev->eff[1].def;
                bool nz_eff = (ev->eff[0].def | ev->eff[0].val | ev->eff[1].def | ev->eff[1].val) != 0;
                bool nz_note_ins = (ev->note != 0 || ev->instr_def != 0);
                bool interesting =
                    (d0 == ef_GlobalFSlideUp || d0 == ef_GlobalFSlideDown ||
                     d1 == ef_GlobalFSlideUp || d1 == ef_GlobalFSlideDown ||
                     d0 == ef_GlobalFreqSlideUpXF || d0 == ef_GlobalFreqSlideDnXF ||
                     d1 == ef_GlobalFreqSlideUpXF || d1 == ef_GlobalFreqSlideDnXF ||
                     d0 == ef_Vibrato || d1 == ef_Vibrato ||
                     d0 == ef_ExtraFineVibrato || d1 == ef_ExtraFineVibrato ||
                     d0 == ef_VibratoVolSlide || d1 == ef_VibratoVolSlide ||
                     d0 == ef_VibratoVSlideFine || d1 == ef_VibratoVSlideFine ||
                     d0 == ef_FSlideDown || d1 == ef_FSlideDown ||
                     d0 == ef_FSlideUp || d1 == ef_FSlideUp ||
                     d0 == ef_FSlideDownFine || d1 == ef_FSlideDownFine ||
                     d0 == ef_FSlideUpFine || d1 == ef_FSlideUpFine ||
                     d0 == ef_TonePortamento || d1 == ef_TonePortamento ||
                     (cc >= 8 && cc <= 11) || (row >= 1 && (nz_eff || nz_note_ins)));

                if (!interesting)
                    continue;

                fprintf(stderr,
                        "  p%u r%u ch%02d note=%02x ins=%02x eff[0]=%02x/%02x eff[1]=%02x/%02x\n",
                        pat, row, cc, ev->note, ev->instr_def,
                        ev->eff[0].def, ev->eff[0].val, ev->eff[1].def, ev->eff[1].val);
            }
        }
    }

    {
        uint8_t vb = ch->voice_table[c];
        uint8_t eb = ch->event_table[c].instr_def;
        uint8_t nb = ch->event_table[c].note & (uint8_t)~keyoff_flag;
        int ft_v = (int)get_instr_fine_tune(vb);
        int ft_e = (int)get_instr_fine_tune(eb);
        uint16_t nf = 0;

        if (nb >= 1 && nb <= 12 * 8 + 1)
            nf = nFreq((uint8_t)(nb - 1));
        fprintf(stderr,
                "ch9 pitch_decode: note_cleared=0x%02x nFreq(nb-1)=0x%04x fine_tune(voice_ins%u)=%d "
                "fine_tune(event_instr%u)=%d ftune_row=%d\n",
                (unsigned)nb, (unsigned)nf,
                (unsigned)vb, ft_v, (unsigned)eb, ft_e, (int)ch->ftune_table[c]);
        fprintf(stderr,
                "  expect Pascal output_note base: 0x%04x + ftune_row => 0x%04x ; freq_table&1fff=0x%04x (delta=%d)\n",
                (unsigned)(uint16_t)(nf + ft_v),
                (unsigned)(uint16_t)((uint16_t)(nf + ft_v) + (uint16_t)ch->ftune_table[c]),
                (unsigned)(ch->freq_table[c] & 0x1fff),
                (int)(ch->freq_table[c] & 0x1fff) -
                    (int)((uint16_t)(nf + ft_v) + (uint16_t)ch->ftune_table[c]));
    }

    /* Same math as macro_vibrato__porta_* / portamento (preview only). */
    if (vcell > 0)
        fprintf(stderr,
                "preview macro_vibrato: calc_freq_shift_up(vib&1fff=0x%04x,depth=%u)=0x%04x\n",
                (unsigned)vlow, (unsigned)(uint8_t)vcell,
                (unsigned)calc_freq_shift_up(vlow, (uint16_t)(uint8_t)vcell));
    else if (vcell < 0)
        fprintf(stderr,
                "preview macro_vibrato: calc_freq_shift_down(vib&1fff=0x%04x,depth=%u)=0x%04x\n",
                (unsigned)vlow, (unsigned)(uint8_t)(int)-vcell,
                (unsigned)calc_freq_shift_down(vlow, (uint16_t)(uint8_t)(-(int)vcell)));
    else
        fprintf(stderr,
                "preview macro_vibrato: depth 0 -> change_freq center vib_freq low=0x%04x\n",
                (unsigned)vlow);
}

static void a2m_dump_context_at_irq_frames(void)
{
    if (!a2m_dump_context_irq_requested())
        return;

    if (frames_dumped >= 10893 && frames_dumped <= 10902) {
        a2m_dump_top2act_ch9_compact();
        fflush(stderr);
        return;
    }

    fprintf(stderr,
            "\n######## A2M_DUMP_CONTEXT irq_frame=%d ticks=%d row=%u pattern=%u order=%u ########\n",
            frames_dumped, ticks, (unsigned)current_line, (unsigned)current_pattern,
            (unsigned)current_order);
    fprintf(stderr,
            "peek shadow primary A/B regs: [0xa0..a8]=%02x %02x %02x %02x %02x %02x %02x %02x %02x  [0xb0..b8]=%02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
            shadow_regs[0][0xa0], shadow_regs[0][0xa1], shadow_regs[0][0xa2],
            shadow_regs[0][0xa3], shadow_regs[0][0xa4], shadow_regs[0][0xa5],
            shadow_regs[0][0xa6], shadow_regs[0][0xa7], shadow_regs[0][0xa8],
            shadow_regs[0][0xb0], shadow_regs[0][0xb1], shadow_regs[0][0xb2],
            shadow_regs[0][0xb3], shadow_regs[0][0xb4], shadow_regs[0][0xb5],
            shadow_regs[0][0xb6], shadow_regs[0][0xb7], shadow_regs[0][0xb8]);
    fprintf(stderr,
            "peek shadow SECONDARY (bank 1) A/B: [0xa0..a8]=%02x %02x %02x %02x %02x %02x %02x %02x %02x  [0xb0..b8]=%02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
            shadow_regs[1][0xa0], shadow_regs[1][0xa1], shadow_regs[1][0xa2],
            shadow_regs[1][0xa3], shadow_regs[1][0xa4], shadow_regs[1][0xa5],
            shadow_regs[1][0xa6], shadow_regs[1][0xa7], shadow_regs[1][0xa8],
            shadow_regs[1][0xb0], shadow_regs[1][0xb1], shadow_regs[1][0xb2],
            shadow_regs[1][0xb3], shadow_regs[1][0xb4], shadow_regs[1][0xb5],
            shadow_regs[1][0xb6], shadow_regs[1][0xb7], shadow_regs[1][0xb8]);
    fprintf(stderr,
            "peek ch9/ch10 freq_table & shadow slot (regoffs_n 10=0x100 -> sec A0/B0): "
            "freq9=0x%04x freq10=0x%04x  shadow[1][a0/b0]=%02x/%02x\n",
            (unsigned)ch->freq_table[9], (unsigned)ch->freq_table[10],
            shadow_regs[1][0xa0], shadow_regs[1][0xb0]);
    fprintf(stderr,
            "peek ch4 (regoffs_n(4)=0x05 -> primary 0xA5): freq_table[4]=0x%04x zero_fq[4]=0x%04x "
            "macro vib_freq=0x%04x vib_paused=%d shadow[0][a5/b5]=0x%02x/0x%02x\n",
            (unsigned)ch->freq_table[4], (unsigned)ch->zero_fq_table[4],
            (unsigned)ch->macro_table[4].vib_freq, (int)ch->macro_table[4].vib_paused,
            shadow_regs[0][0xa5], shadow_regs[0][0xb5]);
        /* Decode pitch like output_note / Pascal — compare to Pascal SHORTINT(ins_parameter(ins,12)) */
        if (frames_dumped >= 14874 && frames_dumped <= 14880) {
            const int c = 4;
            uint8_t raw_note = ch->event_table[c].note;
            uint8_t nb = raw_note & (uint8_t)~keyoff_flag;
            uint8_t vins = ch->voice_table[c];
            uint8_t idef = ch->event_table[c].instr_def;
            int ft_slot = (int)ch->ftune_table[c];
            int8_t fi_v = get_instr_fine_tune(vins);
            int8_t fi_e = get_instr_fine_tune(idef);
            uint16_t nf = 0;
            if (nb >= 1 && nb <= 12 * 8 + 1)
                nf = nFreq((uint8_t)(nb - 1));
            int sum = (int)nf + (int)fi_v + ft_slot;
            uint16_t got_low = ch->freq_table[c] & (uint16_t)0x1fff;
            tINSTR_DATA *id_v = get_instr_data(vins);
            tINSTR_DATA *id_e = get_instr_data(idef);
            fprintf(stderr,
                    "badapple ch4 freq_decode: nb=%u (from note 0x%02x) nFreq(nb-1)=0x%04x "
                    "fine_tune(voice_ins%u)=%d fine_tune(event_instr%u)=%d ftune_table=%d "
                    "=> sum(nFreq+fine_voice+ftune)=%d freq_table&1fff=0x%04x (delta=%d)\n",
                    (unsigned)nb, (unsigned)raw_note, (unsigned)nf,
                    (unsigned)vins, (int)fi_v, (unsigned)idef, (int)fi_e, ft_slot,
                    sum, (unsigned)got_low, (int)got_low - sum);
            fprintf(stderr,
                    "badapple ch4 instr_data: voice instr_record fine_tune=%d event_def instr_record fine_tune=%d\n",
                    id_v ? (int)id_v->fine_tune : -999,
                    id_e ? (int)id_e->fine_tune : -999);
            fprintf(stderr,
                    "badapple ch4 4op/partner: hi=%d lo5=%d freq[5]=0x%04x regoffs_n(4)=0x%03x\n",
                    (int)is_4op_chan_hi(c), (int)is_4op_chan_lo(5),
                    (unsigned)ch->freq_table[5], (unsigned)regoffs_n(c));
            fprintf(stderr,
                    "badapple ch4 porta/fslide/glfsld: "
                    "s0 porta(freq=%u speed=%u) s1 porta(freq=%u speed=%u) "
                    "fslide s0=%u s1=%u glfsld s0=%02x/%02x s1=%02x/%02x "
                    "eff_table s0=%02x/%02x s1=%02x/%02x "
                    "last_eff s0=%02x/%02x s1=%02x/%02x\n",
                    (unsigned)ch->porta_table[0][c].freq,
                    (unsigned)ch->porta_table[0][c].speed,
                    (unsigned)ch->porta_table[1][c].freq,
                    (unsigned)ch->porta_table[1][c].speed,
                    (unsigned)ch->fslide_table[0][c],
                    (unsigned)ch->fslide_table[1][c],
                    ch->glfsld_table[0][c].def, ch->glfsld_table[0][c].val,
                    ch->glfsld_table[1][c].def, ch->glfsld_table[1][c].val,
                    ch->effect_table[0][c].def, ch->effect_table[0][c].val,
                    ch->effect_table[1][c].def, ch->effect_table[1][c].val,
                    ch->last_effect[0][c].def, ch->last_effect[0][c].val,
                    ch->last_effect[1][c].def, ch->last_effect[1][c].val);
            {
                uint8_t mt_ins = ch->macro_table[c].fmreg_ins;
                unsigned pos = ch->macro_table[c].fmreg_pos;
                tFMREG_TABLE *rt = get_fmreg_table(mt_ins);
                uint32_t dis = 0;

                if (mt_ins >= 1 && mt_ins <= instrinfo->count)
                    dis = instrinfo->instruments[mt_ins - 1].dis_fmreg_cols;
                fprintf(stderr,
                        "badapple ch4 fmreg: macro_fmreg_ins=%u fmreg_pos=%u fmreg_duration=%u "
                        "dis_fmreg_cols=0x%08x freq_slide_col26_active=%d\n",
                        (unsigned)mt_ins, pos,
                        (unsigned)ch->macro_table[c].fmreg_duration,
                        (unsigned)dis,
                        (int)((dis & (1u << 26)) == 0));
                if (rt && rt->length) {
                    fprintf(stderr,
                            "badapple ch4 fmreg rt: length=%u keyoff_pos=%u ",
                            (unsigned)rt->length, (unsigned)rt->keyoff_pos);
                    if (pos >= 1 && pos <= rt->length) {
                        const tREGISTER_TABLE_DEF *d = &rt->data[pos - 1];

                        fprintf(stderr,
                                "active_cell[%u] dur=%u freq_slide=%d macro_flags=0x%02x\n",
                                (unsigned)(pos - 1), (unsigned)d->duration, (int)d->freq_slide,
                                (unsigned)d->macro_flags);
                    } else {
                        fprintf(stderr, "(no active_cell pos=%u)\n", pos);
                    }
                } else {
                    fprintf(stderr, "badapple ch4 fmreg rt: (null or length 0)\n");
                }
            }
        }

    /* fm63b_rv: ch11–ch13 + fmreg summary (must stay inside explicit want[] IRQ range above). */
    if (frames_dumped >= 5302 && frames_dumped <= 5315) {
        fprintf(stderr,
                "fm63b_ctx irq=%d: flag_4op=0x%02x percussion=%u nm_tracks=%u shadow[1][b1/a1]=0x%02x/0x%02x\n",
                frames_dumped,
                (unsigned)songinfo->flag_4op, (unsigned)percussion_mode,
                (unsigned)songinfo->nm_tracks,
                shadow_regs[1][0xb1], shadow_regs[1][0xa1]);
        for (int cc = 9; cc <= 13; cc++) {
            uint16_t fq = ch->freq_table[cc];
            uint8_t mt_ins = ch->macro_table[cc].fmreg_ins;
            tFMREG_TABLE *rt = get_fmreg_table(mt_ins);
            uint16_t pos = ch->macro_table[cc].fmreg_pos;
            uint16_t dur = ch->macro_table[cc].fmreg_duration;
            uint8_t mflags = 0;

            if (rt && pos >= 1 && pos <= rt->length)
                mflags = rt->data[pos - 1].macro_flags;

            fprintf(stderr,
                    "  ch%02d: freq=0x%04x key_on_hi=%d note=0x%02x regoffs_n=0x%03x "
                    "4op_hi=%d 4op_lo=%d fmreg_ins=%u pos=%u dur=%u macro_flags=0x%02x "
                    "(nr_bit=%d env_bit=%d zfq_bit=%d)\n",
                    cc, (unsigned)fq, (fq & 0x2000) ? 1 : 0,
                    ch->event_table[cc].note,
                    (unsigned)regoffs_n(cc),
                    (int)is_4op_chan_hi(cc), (int)is_4op_chan_lo(cc),
                    (unsigned)mt_ins, (unsigned)pos, (unsigned)dur, (unsigned)mflags,
                    (mflags & 0x80) ? 1 : 0, (mflags & 0x40) ? 1 : 0, (mflags & 0x20) ? 1 : 0);
        }
    }

    dump_context_f(stderr, ch);

    /* 4thcoast: trace vib_freq divergence around frame 29100 */
    if (frames_dumped >= 29095 && frames_dumped <= 29105) {
        for (int c = 8; c <= 10; c++) {
            fprintf(stderr, "  [dump_frame] ch%d: vib_freq=0x%04x vib_pos=%u vib_count=%u vib_paused=%d arpgg_s0_state=%u arpgg_s0_note=%u arpgg_s1_state=%u arpgg_s1_note=%u\n",
                    c,
                    (unsigned)ch->macro_table[c].vib_freq,
                    (unsigned)ch->macro_table[c].vib_pos,
                    (unsigned)ch->macro_table[c].vib_count,
                    ch->macro_table[c].vib_paused ? 1 : 0,
                    (unsigned)ch->arpgg_table[0][c].state,
                    (unsigned)ch->arpgg_table[0][c].note,
                    (unsigned)ch->arpgg_table[1][c].state,
                    (unsigned)ch->arpgg_table[1][c].note);
        }
    }

    if (frames_dumped >= 20853 && frames_dumped <= 20860) {
        int c = 10;
        fprintf(stderr,
                "\n#### SAM dump frame=%d ch10: freq_table=0x%04x (keyon=%d block=%d fnum=0x%03x) "
                "ftune=%d porta_s0={freq=0x%04x speed=%u} porta_s1={freq=0x%04x speed=%u}\n",
                frames_dumped,
                (unsigned)ch->freq_table[c],
                (ch->freq_table[c] >> 13) & 1,
                ((ch->freq_table[c] >> 10) & 7),
                (unsigned)(ch->freq_table[c] & 0x3ff),
                (int)ch->ftune_table[c],
                (unsigned)ch->porta_table[0][c].freq,
                (unsigned)ch->porta_table[0][c].speed,
                (unsigned)ch->porta_table[1][c].freq,
                (unsigned)ch->porta_table[1][c].speed);
    }

    fflush(stderr);
}
#endif

static void basename_no_ext(char *dst, size_t dstsize, const char *path)
{
    const char *p = strrchr(path, '/');
    const char *q = strrchr(path, '\\');
    if (q > p) p = q;
    if (!p) p = path; else p++;

    size_t len = strcspn(p, ".");
    if (len >= dstsize) len = dstsize - 1;
    memcpy(dst, p, len);
    dst[len] = '\0';
}

static int max_frames = 500;
#define WR_TRACE_SIZE 64
static uint16_t wr_trace_reg[WR_TRACE_SIZE];
static uint8_t wr_trace_val[WR_TRACE_SIZE];
static int wr_trace_idx = 0;
static int wr_trace_count = 0;

static void wr_trace(uint16_t full_reg, uint8_t val)
{
    wr_trace_reg[wr_trace_idx] = full_reg;
    wr_trace_val[wr_trace_idx] = val;
    wr_trace_idx = (wr_trace_idx + 1) % WR_TRACE_SIZE;
    if (wr_trace_count < WR_TRACE_SIZE) wr_trace_count++;
}
static void dump_frame(void)
{
    int i;
#ifdef A2M_DUMP_CONTEXT
    a2m_dump_context_at_irq_frames();
#endif
    if (frames_dumped >= max_frames) {
        play_status = isStopped;
        return;
    }
    printf("%d 0 ", frames_dumped);
    for (i = 0; i < 256; i++) printf("%02x", shadow_regs[0][i]);
    printf("\n");
    printf("%d 1 ", frames_dumped);
    for (i = 0; i < 256; i++) printf("%02x", shadow_regs[1][i]);
    printf("\n");
    /* Event table dump commented out
    printf("%d E ", frames_dumped);
    for (i = 0; i < 20; i++) {
        uint8_t d0 = ch->event_table[i].eff[0].def;
        uint8_t d1 = ch->event_table[i].eff[1].def;
        if (d0 == ef_Arpeggio) d0 = 0;
        if (d1 == ef_Arpeggio) d1 = 0;
        printf("%02x%02x%02x%02x%02x%02x",
               ch->event_table[i].note,
               ch->event_table[i].instr_def,
               d0,
               ch->event_table[i].eff[0].val,
               d1,
               ch->event_table[i].eff[1].val);
    }
    printf("\n");
    */
    printf("%d F ", frames_dumped);
    for (i = 0; i < 20; i++)
      printf("%04x", ch->freq_table[i]);
    printf("\n");
    printf("%d MB ", frames_dumped);
    for (i = 0; i < 20; i++) {
        tCH_MACRO_TABLE *mt = &ch->macro_table[i];
        printf("%04x%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x%02x%04x",
               mt->fmreg_pos, mt->arpg_pos, mt->vib_pos,
               mt->fmreg_duration, mt->arpg_count, mt->vib_count, mt->vib_delay,
               mt->fmreg_ins, mt->arpg_table, mt->vib_table, mt->arpg_note,
               (unsigned)mt->vib_paused,
               mt->vib_freq);
    }
    printf("\n");
    printf("%d PT ", frames_dumped);
    for (i = 0; i < 20; i++)
      printf("%04x%02x%04x%02x",
             ch->porta_table[0][i].freq,
             ch->porta_table[0][i].speed,
             ch->porta_table[1][i].freq,
             ch->porta_table[1][i].speed);
    printf("\n");
    printf("%d FT ", frames_dumped);
    for (i = 0; i < 20; i++)
      printf("%02x", (unsigned)(uint8_t)ch->ftune_table[i]);
    printf("\n");
    printf("%d AT ", frames_dumped);
    for (i = 0; i < 20; i++)
      printf("%02x%02x%02x%02x",
             ch->arpgg_table[0][i].state,
             ch->arpgg_table[0][i].note,
             ch->arpgg_table[0][i].add1,
             ch->arpgg_table[0][i].add2);
    printf("\n");
    printf("%d VT ", frames_dumped);
    for (i = 0; i < 20; i++)
      printf("%02x%02x%02x%02x%01x",
             ch->vibr_table[0][i].pos,
             ch->vibr_table[0][i].dir,
             ch->vibr_table[0][i].speed,
             ch->vibr_table[0][i].depth,
             ch->vibr_table[0][i].fine ? 1 : 0);
    printf("\n");
    printf("%d TT ", frames_dumped);
    for (i = 0; i < 20; i++)
      printf("%02x%02x%02x%02x%01x",
             ch->trem_table[0][i].pos,
             ch->trem_table[0][i].dir,
             ch->trem_table[0][i].speed,
             ch->trem_table[0][i].depth,
             ch->trem_table[0][i].fine ? 1 : 0);
    printf("\n");
    printf("%d RT ", frames_dumped);
    for (i = 0; i < 20; i++)
      printf("%02x", ch->retrig_table[0][i]);
    printf("\n");
    printf("%d MV ", frames_dumped);
    for (i = 0; i < 20; i++)
      printf("%02x", ch->modulator_vol[i]);
    printf("\n");
    printf("%d CV ", frames_dumped);
    for (i = 0; i < 20; i++)
      printf("%02x", ch->carrier_vol[i]);
    printf("\n");

    printf("%d VS ", frames_dumped);
    for (i = 0; i < 20; i++)
      printf("%02x", ch->voice_table[i]);
    printf("\n");

    printf("%d FP ", frames_dumped);
    for (i = 0; i < 20; i++) {
        tFM_INST_DATA *fp = &ch->fmpar_table[i];
        printf("%02x%02x%01x%01x%01x",
               (unsigned)fp->volM, (unsigned)fp->volC,
               (unsigned)fp->kslM, (unsigned)fp->kslC,
               (unsigned)fp->connect);
    }
    printf("\n");

    printf("%d GV ", frames_dumped);
    printf("%02x%02x%02x%01x%01x",
           (unsigned)global_volume, (unsigned)fade_out_volume,
           (unsigned)overall_volume,
           (unsigned)(volume_scaling ? 1 : 0),
           (unsigned)(percussion_mode ? 1 : 0));
    printf("\n");

    // printf("%d WR ", frames_dumped);
    // for (i = 0; i < WR_TRACE_SIZE; i++) {
    //     int idx;
    //     if (i < wr_trace_count) {
    //         idx = (wr_trace_idx - wr_trace_count + i + WR_TRACE_SIZE) % WR_TRACE_SIZE;
    //         printf("%03x%02x", (unsigned)(wr_trace_reg[idx] & 0x1ff),
    //                (unsigned)wr_trace_val[idx]);
    //     } else {
    //         printf("00000");
    //     }
    // }
    // printf("\n");

    frames_dumped++;
}

int main(int argc, char *argv[])
{
    signal(SIGINT, handle_sigint);

    if (argc < 2) {
        fprintf(stderr, "Usage: a2m_dump <file.a2m> [output.reg] [max_frames]\n");
        return 1;
    }

    char outname[1032];
    if (argc >= 3) {
        snprintf(outname, sizeof(outname), "%s", argv[2]);
    } else {
        char base[1024];
        basename_no_ext(base, sizeof(base), argv[1]);
        snprintf(outname, sizeof(outname), "%s.reg", base);
    }

    if (argc >= 4) {
        max_frames = atoi(argv[3]);
        if (max_frames <= 0) max_frames = 500;
    }

    if (!freopen(outname, "w", stdout)) {
        fprintf(stderr, "Failed to open %s for writing\n", outname);
        return 1;
    }

    a2t_init(44100);

    char *data = a2t_load(argv[1]);
    if (!data) {
        fprintf(stderr, "Failed to load %s\n", argv[1]);
        return 1;
    }

    write_trace_hook = wr_trace;

    if (!a2t_play(data)) {
        fprintf(stderr, "Failed to play %s\n", argv[1]);
        return 1;
    }

    set_overall_volume(63);



    frame_hook = dump_frame;

    unsigned char buf[4096];
    memset(buf, 0, sizeof(buf));

    while (play_status == isPlaying && !songend)
        a2t_update(buf, (int)sizeof(buf));

    a2t_stop();
    a2t_shut();

    return 0;
}
