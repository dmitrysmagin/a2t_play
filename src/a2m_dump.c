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

static void a2m_dump_context_at_irq_frames(void)
{
    static const int want[] = {
        /* fm-troni: just before / at / after first Pascal-C divergence (0xA2), plus later bursts */
        15259, 15260, 15261, 15262, 15263,
        15620, 15621, 15622,
        34556, 34557, 34558, 34559,
        /* fank5: secondary shadow_regs[1][0xb0] key-on vs Pascal ~IRQ 47232 (chan index 10, regoffs_n=0x100) */
        47228, 47229, 47230, 47231, 47232, 47233, 47234, 47235, 47236,
        -1
    };
    int i;

    for (i = 0; want[i] >= 0; i++) {
        if (frames_dumped != want[i])
            continue;

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
        dump_context_f(stderr, ch);
        fflush(stderr);
        return;
    }
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
