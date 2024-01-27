// sdl backend for a2t/a2m player

#ifdef _WIN32
#include <windows.h>
#endif

#include "a2t.h"
#include <SDL.h>

#define FREQHZ 44100
#define BUFFSMPL 2048

SDL_AudioSpec audio;

static void playcallback(void *unused, Uint8 *stream, int len)
{
    a2t_update(stream, len);
}

/* MINGW has built-in kbhit() in conio.h */
#ifdef _WIN32
#include <conio.h>
#else
#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

static int kbhit(void)
{
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}
#endif

void rewind_console(int lines)
{
    #ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO cbsi;

    if (GetConsoleScreenBufferInfo(hConsole, &cbsi)) {
        COORD coord = {
            cbsi.dwCursorPosition.X,
            coord.Y = cbsi.dwCursorPosition.Y - lines
        };

        SetConsoleCursorPosition(hConsole, coord);
    }
    #else
    // linux ansi terminal
    printf("\033[%dA", lines);
    #endif
}

void show_event(tADTRACK2_EVENT table[20])
{
    static char effects[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ&%!@=#$~^`><";

    printf("EVT0: ");
    for (int i = 0; i < 20; i++) {
        uint8_t note = table[i].note;
        uint8_t inst = table[i].instr_def;

        printf("%02x%02x%s", note, inst, i < 19 ? "|" : "\n");
    }
    printf("EVT1: ");
    for (int i = 0; i < 20; i++) {
        uint8_t def0 = table[i].eff[0].def;
        uint8_t val0 = table[i].eff[0].val;

        printf(" %c%02x%s", effects[def0], val0, i < 19 ? "|" : "\n");
    }
    printf("EVT2: ");
    for (int i = 0; i < 20; i++) {
        uint8_t def1 = table[i].eff[1].def;
        uint8_t val1 = table[i].eff[1].val;

        printf(" %c%02x%s", effects[def1], val1, i < 19 ? "|" : "\n");
    }
}

void show_eff(char *name, tEFFECT_TABLE table[2][20])
{
    static char effects[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ&%!@=#$~^`><";

    for (int j = 0; j < 2; j++) {
        printf("%s%d: ", name, j + 1);
        for (int i = 0; i < 20; i++) {
            uint8_t def = table[j][i].def;
            uint8_t val = table[j][i].val;

            /*if (def & ef_fix2) {
                val |= ((def - ef_fix2 - 36) << 4); // def - ef_Extended2 - ef_fix2
                def = 36;
            } else {
                def &= 0x3f;
            }*/

            printf(" %c%02x%s", def < 48 ? effects[def] : '?', val, i < 19 ? "|" : "\n");
        }
    }
}

void show_info()
{
    printf("Order %03d, Pattern %03d, Row %03d\n", current_order, current_pattern, current_line);
    printf("VOIC: ");
    for (int i = 0; i < 20; i++) {
        printf("  %02x%s", ch->voice_table[i], i < 19 ? "|" : "\n");
    }
    printf("FREQ: ");
    for (int i = 0; i < 20; i++) {
        printf("%04x%s", ch->freq_table[i], i < 19 ? "|" : "\n");
    }
    show_eff("EFF", ch->effect_table);
    show_eff("LEF", ch->last_effect);
    //show_eff("GLF", ch->glfsld_table);
    show_event(ch->event_table);
    printf("FMRG: ");
    for (int i = 0; i < 20; i++) {
        printf("%02x%02x%s", ch->macro_table[i].fmreg_pos & 0xff, ch->macro_table[i].fmreg_duration, i < 19 ? "|" : "\n");
    }
    printf("VIBM: ");
    for (int i = 0; i < 20; i++) {
        printf("%02x%02x%s", ch->macro_table[i].vib_pos & 0xff, ch->macro_table[i].vib_count, i < 19 ? "|" : "\n");
    }
}

#undef main
int main(int argc, char *argv[])
{
    char *a2t;

    // if no arguments
    if (argc == 1) {
        printf("Usage: a2t_play.exe *.a2t\n");
        return 1;
    }

    a2t_init(FREQHZ);

    /* HINT: Perhaps, SDL_INIT_EVERYTHING doesn't let audio work without
             setting video mode with MINGW */
    SDL_Init(SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE);

    audio.freq = FREQHZ;
    audio.format = AUDIO_S16;
    audio.channels = 2;
    audio.samples = BUFFSMPL;
    audio.callback = playcallback;
    audio.userdata = 0; // use later

    if (SDL_OpenAudio(&audio, 0) < 0) {
        printf("Error initializing SDL_OpenAudio %s\n", SDL_GetError());
        return 1;
    }

    a2t = a2t_load(argv[1]);
    if (a2t == NULL) {
        printf("Error reading %s\n", argv[1]);
        return 1;
    }

    a2t_play(a2t);
    SDL_PauseAudio(0);

    printf("Playing - press anything to exit\n");

    while (!kbhit()) {
        show_info();
        rewind_console(12);
        SDL_Delay(10);
    }

    show_info();

    a2t_stop();
    SDL_PauseAudio(1);
    SDL_CloseAudio();

    SDL_Quit();

    a2t_shut();

    return 0;
}
