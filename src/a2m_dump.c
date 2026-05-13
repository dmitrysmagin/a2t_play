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

#ifdef clocks
/* nothing needed, a2t.c's opl_out() has the #ifdef clocks dump */
#endif

int frames_dumped = 0;

#include "../src/a2t.c"

/*
 * Instrumentation: build a2m_dump with -DA2M_DUMP_CONTEXT to emit dump_context_f(stderr)
 * on selected IRQ frame numbers (see static list in a2m_dump_context_at_irq_frames).
 * Example: ... -DA2M_DUMP_CONTEXT -Dclocks -o a2m_dump ...
 * Run: ./a2m_dump tune.a2m /dev/null 40000 2>ctx.log
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
        -1
    };
    int i;

    for (i = 0; want[i] >= 0; i++) {
        if (frames_dumped == want[i])
            return true;
    }
    return false;
}

static void a2m_dump_context_at_irq_frames(void)
{
    if (!a2m_dump_context_irq_requested())
        return;

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
/* INIT trace disabled
static char trace_buf[131072];
static size_t trace_len = 0;

static void init_trace(uint16_t reg, uint8_t val)
{
    if (trace_len < sizeof(trace_buf) - 8)
        trace_len += (size_t)sprintf(trace_buf + trace_len, "%03x %02x\n", reg & 0x1ff, val & 0xff);
}
*/
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
    frames_dumped++;
}

int main(int argc, char *argv[])
{
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

    /* INIT trace disabled
    write_trace_hook = init_trace;
    */

    if (!a2t_play(data)) {
        fprintf(stderr, "Failed to play %s\n", argv[1]);
        return 1;
    }

    set_overall_volume(63);

    /* INIT trace disabled
    if (trace_len > 0)
        fwrite(trace_buf, 1, trace_len, stdout);

    {
        int i;
        printf("INIT:");
        for (i = 0; i < 256; i++) printf("%02x", shadow_regs[0][i]);
        printf(" ");
        for (i = 0; i < 256; i++) printf("%02x", shadow_regs[1][i]);
        printf("\n");
    }

    write_trace_hook = NULL;
    */

    frame_hook = dump_frame;

    unsigned char buf[4096];
    memset(buf, 0, sizeof(buf));

    while (play_status == isPlaying && !songend)
        a2t_update(buf, (int)sizeof(buf));

    a2t_stop();
    a2t_shut();

    return 0;
}
