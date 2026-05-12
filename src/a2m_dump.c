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
static char trace_buf[131072];
static size_t trace_len = 0;

static void init_trace(uint16_t reg, uint8_t val)
{
    if (trace_len < sizeof(trace_buf) - 8)
        trace_len += (size_t)sprintf(trace_buf + trace_len, "%03x %02x\n", reg & 0x1ff, val & 0xff);
}

static void dump_frame(void)
{
    int i;
    if (frames_dumped >= max_frames) {
        play_status = isStopped;
        return;
    }
    printf("1:");
    for (i = 0; i < 256; i++) printf("%02x", shadow_regs[0][i]);
    printf(" 2:");
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

    write_trace_hook = init_trace;

    if (!a2t_play(data)) {
        fprintf(stderr, "Failed to play %s\n", argv[1]);
        return 1;
    }

    set_overall_volume(63);

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

    frame_hook = dump_frame;

    unsigned char buf[4096];
    memset(buf, 0, sizeof(buf));

    while (play_status == isPlaying && !songend)
        a2t_update(buf, (int)sizeof(buf));

    a2t_stop();
    a2t_shut();

    return 0;
}
