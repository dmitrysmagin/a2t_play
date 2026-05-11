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

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: a2m_dump <file.a2m> [output.reg]\n");
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

    if (!a2t_play(data)) {
        fprintf(stderr, "Failed to play %s\n", argv[1]);
        return 1;
    }

    unsigned char buf[4096];
    memset(buf, 0, sizeof(buf));

    while (play_status == isPlaying && !songend)
        a2t_update(buf, (int)sizeof(buf));

    a2t_stop();
    a2t_shut();

    return 0;
}
