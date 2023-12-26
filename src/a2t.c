/*
    TODO:
    - Bug in the original player: need to reset global_volume after order restart
    - Implement fade_out_volume in set_ins_volume() and set_volume

    In order to get into Adplug:
    - Reduce the memory used for a tune
    - Rework tFIXED_SONGDATA:
        * Make instr_data an array of pointers
        * Rework direct access to songdata->instr_data with get_instr(ins) // 1 - based
        * Make instr_macros/macro_table an array of pointers
        * But first read data correctly to instr_data/instr_macros/macro_table
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "depack.h"
#include "sixpack.h"
#include "lzh.h"
#include "opl3.h"
#include "a2t.h"

#ifndef C_ASSERT
#define C_ASSERT(e) typedef char __C_ASSERT__[(e) ? 1 : -1]
#endif

#ifndef bool
typedef signed char bool;
#endif

#ifndef FALSE
#define FALSE 0
#define TRUE !FALSE
#endif

#define INT16LE(A) (int16_t)((A[0]) | (A[1] << 8))
#define UINT16LE(A) (uint16_t)((A[0]) | (A[1] << 8))
#define INT32LE(A) (int32_t)((A[0]) | (A[1] << 8) | (A[2] << 16) | (A[3] << 24))
#define UINT32LE(A) (uint32_t)((A[0]) | (A[1] << 8) | (A[2] << 16) | (A[3] << 24))

#define keyoff_flag			0x80
#define fixed_note_flag		0x90
//#define pattern_loop_flag	0xe0
//#define pattern_break_flag	0xf0

typedef struct {
    union {
        struct {
            uint8_t multipM: 4, ksrM: 1, sustM: 1, vibrM: 1, tremM : 1;
            uint8_t multipC: 4, ksrC: 1, sustC: 1, vibrC: 1, tremC : 1;
            uint8_t volM: 6, kslM: 2;
            uint8_t volC: 6, kslC: 2;
            uint8_t decM: 4, attckM: 4;
            uint8_t decC: 4, attckC: 4;
            uint8_t relM: 4, sustnM: 4;
            uint8_t relC: 4, sustnC: 4;
            uint8_t wformM: 3, : 5;
            uint8_t wformC: 3, : 5;
            uint8_t connect: 1, feedb: 3, : 4; // panning is not used here
        };
        uint8_t data[11];
    };
} tFM_INST_DATA;

C_ASSERT(sizeof(tFM_INST_DATA) == 11);

typedef struct {
    tFM_INST_DATA fm;
    uint8_t panning;
    int8_t  fine_tune;
    uint8_t perc_voice;
} tINSTR_DATA;

C_ASSERT(sizeof(tINSTR_DATA) == 14);

typedef struct {
    uint8_t length;
    uint8_t speed;
    uint8_t loop_begin;
    uint8_t loop_length;
    uint8_t keyoff_pos;
    uint8_t data[255]; // array[1..255] of Byte;
} tARPEGGIO_TABLE;

typedef struct {
    uint8_t length;
    uint8_t speed;
    uint8_t delay;
    uint8_t loop_begin;
    uint8_t loop_length;
    uint8_t keyoff_pos;
    int8_t data[255]; // array[1..255] of Shortint;
} tVIBRATO_TABLE;

typedef struct {
    tFM_INST_DATA fm;
    uint8_t freq_slide[2]; // int16_t
    uint8_t panning;
    uint8_t duration;
} tREGISTER_TABLE_DEF;

typedef struct {
    uint8_t length;
    uint8_t loop_begin;
    uint8_t loop_length;
    uint8_t keyoff_pos;
    uint8_t arpeggio_table;
    uint8_t vibrato_table;
    tREGISTER_TABLE_DEF data[255];
} tINSTR_MACRO;

typedef struct {
    tARPEGGIO_TABLE arpeggio;
    tVIBRATO_TABLE vibrato;
} tMACRO_TABLE;

C_ASSERT(sizeof(tINSTR_MACRO) == 3831);
C_ASSERT(sizeof(tMACRO_TABLE) == 521);

typedef bool tDIS_FMREG_COL[28]; // array[0..27] of Boolean;

typedef struct {
    char            songname[43];        // pascal String[42];
    char            composer[43];        // pascal String[42];
    char            instr_names[255][43];// array[1..255] of String[42];
    tINSTR_DATA     instr_data[255];     // array[1..255] of tADTRACK2_INS;
    tINSTR_MACRO    instr_macros[255];   // array[1..255] of tREGISTER_TABLE;
    tMACRO_TABLE    macro_table[255];    // array[1..255] of tMACRO_TABLE;
    uint8_t         pattern_order[0x80]; // array[0..0x7f] of Byte;
    uint8_t         tempo;
    uint8_t         speed;
    uint8_t         common_flag;
    uint16_t        patt_len;
    uint8_t         nm_tracks;
    uint16_t        macro_speedup;
    uint8_t         flag_4op;
    uint8_t         lock_flags[20];
    char            pattern_names[128][43];  // array[0..$7f] of String[42];
    //tDIS_FMREG_COL  dis_fmreg_col[255];  // array[1..255] of tDIS_FMREG_COL;
    int8_t          dis_fmreg_col[255][28]; // disabled fm macro columns in the editor
    struct {
        uint8_t		num_4op;
        uint8_t		idx_4op[128];
    } ins_4op_flags;
    uint8_t			reserved_data[1024];
    struct {
        uint8_t		rows_per_beat;
        int16_t		tempo_finetune;
    } bpm_data;
} tFIXED_SONGDATA;

typedef enum {
    isPlaying = 0, isPaused, isStopped
} tPLAY_STATUS;

typedef struct {
    uint8_t note;
    uint8_t instr_def;
    struct {
        uint8_t def;
        uint8_t val;
    } eff[2];
} tADTRACK2_EVENT;

C_ASSERT(sizeof(tADTRACK2_EVENT) == 6);

typedef struct {
    struct {
        struct {
            tADTRACK2_EVENT ev;
        } row[256];
    } ch[20];
} tPATTERN_DATA;

C_ASSERT(sizeof(tPATTERN_DATA) == 20 * 256 * 6);

const uint8_t _panning[3] = {0x30, 0x10, 0x20};

const uint8_t _instr[12] = {
    0x20, 0x20, 0x40, 0x40, 0x60, 0x60, 0x80, 0x80, 0xe0, 0xe0, 0xc0, 0xbd
};

#define BYTE_NULL (uint8_t)(0xFFFFFFFF)

const uint16_t _chmm_n[20] = {
    0x003,0x000,0x004,0x001,0x005,0x002,0x006,0x007,0x008,0x103,
    0x100,0x104,0x101,0x105,0x102,0x106,0x107,0x108,BYTE_NULL,BYTE_NULL
};
const uint16_t _chmm_m[20] = {
    0x008,0x000,0x009,0x001,0x00a,0x002,0x010,0x011,0x012,0x108,
    0x100,0x109,0x101,0x10a,0x102,0x110,0x111,0x112,BYTE_NULL,BYTE_NULL
};
const uint16_t _chmm_c[20] = {
    0x00b,0x003,0x00c,0x004,0x00d,0x005,0x013,0x014,0x015,0x10b,
    0x103,0x10c,0x104,0x10d,0x105,0x113,0x114,0x115,BYTE_NULL,BYTE_NULL
};

const uint16_t _chpm_n[20] = {
    0x003,0x000,0x004,0x001,0x005,0x002,0x106,0x107,0x108,0x103,
    0x100,0x104,0x101,0x105,0x102,0x006,0x007,0x008,0x008,0x007
};
const uint16_t _chpm_m[20] = {
    0x008,0x000,0x009,0x001,0x00a,0x002,0x110,0x111,0x112,0x108,
    0x100,0x109,0x101,0x10a,0x102,0x010,0x014,0x012,0x015,0x011
};
const uint16_t _chpm_c[20] = {
    0x00b,0x003,0x00c,0x004,0x00d,0x005,0x113,0x114,0x115,0x10b,
    0x103,0x10c,0x104,0x10d,0x105,0x013,BYTE_NULL,BYTE_NULL,BYTE_NULL,BYTE_NULL
};

uint16_t _chan_n[20], _chan_m[20], _chan_c[20];

#define ef_Arpeggio            0
#define ef_FSlideUp            1
#define ef_FSlideDown          2
#define ef_TonePortamento      3
#define ef_Vibrato             4
#define ef_TPortamVolSlide     5
#define ef_VibratoVolSlide     6
#define ef_FSlideUpFine        7
#define ef_FSlideDownFine      8
#define ef_SetModulatorVol     9
#define ef_VolSlide            10
#define ef_PositionJump        11
#define ef_SetInsVolume        12
#define ef_PatternBreak        13
#define ef_SetTempo            14
#define ef_SetSpeed            15
#define ef_TPortamVSlideFine   16
#define ef_VibratoVSlideFine   17
#define ef_SetCarrierVol       18
#define ef_SetWaveform         19
#define ef_VolSlideFine        20
#define ef_RetrigNote          21
#define ef_Tremolo             22
#define ef_Tremor              23
#define ef_ArpggVSlide         24
#define ef_ArpggVSlideFine     25
#define ef_MultiRetrigNote     26
#define ef_FSlideUpVSlide      27
#define ef_FSlideDownVSlide    28
#define ef_FSlUpFineVSlide     29
#define ef_FSlDownFineVSlide   30
#define ef_FSlUpVSlF           31
#define ef_FSlDownVSlF         32
#define ef_FSlUpFineVSlF       33
#define ef_FSlDownFineVSlF     34
#define ef_Extended            35
#define ef_Extended2           36
#define ef_SetGlobalVolume     37
#define ef_SwapArpeggio        38
#define ef_SwapVibrato         39
#define ef_ForceInsVolume      40
#define ef_Extended3           41
#define ef_ExtraFineArpeggio   42
#define ef_ExtraFineVibrato    43
#define ef_ExtraFineTremolo    44
#define ef_SetCustomSpeedTab   45
#define ef_GlobalFSlideUp      46
#define ef_GlobalFSlideDown    47
#define ef_ex_SetTremDepth     0
#define ef_ex_SetVibDepth      1
#define ef_ex_SetAttckRateM    2
#define ef_ex_SetDecayRateM    3
#define ef_ex_SetSustnLevelM   4
#define ef_ex_SetRelRateM      5
#define ef_ex_SetAttckRateC    6
#define ef_ex_SetDecayRateC    7
#define ef_ex_SetSustnLevelC   8
#define ef_ex_SetRelRateC      9
#define ef_ex_SetFeedback      10
#define ef_ex_SetPanningPos    11
#define ef_ex_PatternLoop      12
#define ef_ex_PatternLoopRec   13
#define ef_ex_ExtendedCmd      14
#define ef_ex_cmd_MKOffLoopDi  0
#define ef_ex_cmd_MKOffLoopEn  1
#define ef_ex_cmd_TPortaFKdis  2
#define ef_ex_cmd_TPortaFKenb  3
#define ef_ex_cmd_RestartEnv   4
#define ef_ex_cmd_4opVlockOff  5
#define ef_ex_cmd_4opVlockOn   6
#define ef_ex_cmd_ForceBpmSld  7
#define ef_ex_ExtendedCmd2     15
#define ef_ex_cmd2_RSS         0
#define ef_ex_cmd2_ResetVol    1
#define ef_ex_cmd2_LockVol     2
#define ef_ex_cmd2_UnlockVol   3
#define ef_ex_cmd2_LockVP      4
#define ef_ex_cmd2_UnlockVP    5
#define ef_ex_cmd2_VSlide_mod  6
#define ef_ex_cmd2_VSlide_car  7
#define ef_ex_cmd2_VSlide_def  8
#define ef_ex_cmd2_LockPan     9
#define ef_ex_cmd2_UnlockPan   10
#define ef_ex_cmd2_VibrOff     11
#define ef_ex_cmd2_TremOff     12
#define ef_ex_cmd2_FVib_FGFS   13
#define ef_ex_cmd2_FTrm_XFGFS  14
#define ef_ex_cmd2_NoRestart   15
#define ef_ex2_PatDelayFrame   0
#define ef_ex2_PatDelayRow     1
#define ef_ex2_NoteDelay       2
#define ef_ex2_NoteCut         3
#define ef_ex2_FineTuneUp      4
#define ef_ex2_FineTuneDown    5
#define ef_ex2_GlVolSlideUp    6
#define ef_ex2_GlVolSlideDn    7
#define ef_ex2_GlVolSlideUpF   8
#define ef_ex2_GlVolSlideDnF   9
#define ef_ex2_GlVolSldUpXF    10
#define ef_ex2_GlVolSldDnXF    11
#define ef_ex2_VolSlideUpXF    12
#define ef_ex2_VolSlideDnXF    13
#define ef_ex2_FreqSlideUpXF   14
#define ef_ex2_FreqSlideDnXF   15
#define ef_ex3_SetConnection   0
#define ef_ex3_SetMultipM      1
#define ef_ex3_SetKslM         2
#define ef_ex3_SetTremoloM     3
#define ef_ex3_SetVibratoM     4
#define ef_ex3_SetKsrM         5
#define ef_ex3_SetSustainM     6
#define ef_ex3_SetMultipC      7
#define ef_ex3_SetKslC         8
#define ef_ex3_SetTremoloC     9
#define ef_ex3_SetVibratoC     10
#define ef_ex3_SetKsrC         11
#define ef_ex3_SetSustainC     12

#define ef_fix1 0x80
#define ef_fix2 0x90

#define EFGR_ARPVOLSLIDE 1
#define EFGR_FSLIDEVOLSLIDE 2
#define EFGR_TONEPORTAMENTO 3
#define EFGR_VIBRATO 4
#define EFGR_TREMOLO 5
#define EFGR_VIBRATOVOLSLIDE 6
#define EFGR_PORTAVOLSLIDE 7
#define EFGR_RETRIGNOTE 8

// Effect can inherit previous effect value only within the group
// x00 <-- use previous val
const int effect_group[] = {
    [ef_ArpggVSlide] = EFGR_ARPVOLSLIDE,
    [ef_ArpggVSlideFine] = EFGR_ARPVOLSLIDE,

    [ef_FSlideUpVSlide] = EFGR_FSLIDEVOLSLIDE,
    [ef_FSlUpVSlF] = EFGR_FSLIDEVOLSLIDE,
    [ef_FSlideDownVSlide] = EFGR_FSLIDEVOLSLIDE,
    [ef_FSlDownVSlF] = EFGR_FSLIDEVOLSLIDE,
    [ef_FSlUpFineVSlide] = EFGR_FSLIDEVOLSLIDE,
    [ef_FSlUpFineVSlF] = EFGR_FSLIDEVOLSLIDE,
    [ef_FSlDownFineVSlide] = EFGR_FSLIDEVOLSLIDE,
    [ef_FSlDownFineVSlF] = EFGR_FSLIDEVOLSLIDE,

    [ef_TonePortamento] = EFGR_TONEPORTAMENTO,

    [ef_Vibrato] = EFGR_VIBRATO,
    [ef_ExtraFineVibrato] = EFGR_VIBRATO,

    [ef_Tremolo] = EFGR_TREMOLO,
    [ef_ExtraFineTremolo] = EFGR_TREMOLO,

    [ef_VibratoVolSlide] = EFGR_VIBRATOVOLSLIDE,
    [ef_VibratoVSlideFine] = EFGR_VIBRATOVOLSLIDE,

    [ef_TPortamVolSlide] = EFGR_PORTAVOLSLIDE,
    [ef_TPortamVSlideFine] = EFGR_PORTAVOLSLIDE,

    [ef_RetrigNote] = EFGR_RETRIGNOTE,
    [ef_MultiRetrigNote] = EFGR_RETRIGNOTE,

    [ef_GlobalFSlideDown] = 0,
};

const int MIN_IRQ_FREQ = 50;
const int MAX_IRQ_FREQ = 1000;

uint8_t current_order = 0;
uint8_t current_pattern = 0;
uint8_t current_line = 0; // TODO: rename to current_row

uint8_t tempo = 50;
uint8_t speed = 6;

uint16_t macro_speedup = 1;
bool irq_mode = FALSE;

int16_t IRQ_freq = 50;
int IRQ_freq_shift = 0;
bool irq_initialized = FALSE;
const bool timer_fix = TRUE;

bool pattern_break = FALSE;
bool pattern_delay = FALSE;
uint8_t next_line = 0;

int playback_speed_shift = 0;
tPLAY_STATUS play_status = isStopped;
uint8_t overall_volume = 63;
uint8_t global_volume = 63;

const uint8_t pattern_loop_flag  = 0xe0;
const uint8_t pattern_break_flag = 0xf0;

const uint8_t def_vibtrem_speed_factor = 1;
const uint8_t def_vibtrem_table_size = 32;
const uint8_t def_vibtrem_table[256] = {
    0,24,49,74,97,120,141,161,180,197,212,224,235,244,250,253,255,
    253,250,244,235,224,212,197,180,161,141,120,97,74,49,24,
    0,24,49,74,97,120,141,161,180,197,212,224,235,244,250,253,255,
    253,250,244,235,224,212,197,180,161,141,120,97,74,49,24,
    0,24,49,74,97,120,141,161,180,197,212,224,235,244,250,253,255,
    253,250,244,235,224,212,197,180,161,141,120,97,74,49,24,
    0,24,49,74,97,120,141,161,180,197,212,224,235,244,250,253,255,
    253,250,244,235,224,212,197,180,161,141,120,97,74,49,24,
    0,24,49,74,97,120,141,161,180,197,212,224,235,244,250,253,255,
    253,250,244,235,224,212,197,180,161,141,120,97,74,49,24,
    0,24,49,74,97,120,141,161,180,197,212,224,235,244,250,253,255,
    253,250,244,235,224,212,197,180,161,141,120,97,74,49,24,
    0,24,49,74,97,120,141,161,180,197,212,224,235,244,250,253,255,
    253,250,244,235,224,212,197,180,161,141,120,97,74,49,24,
    0,24,49,74,97,120,141,161,180,197,212,224,235,244,250,253,255,
    253,250,244,235,224,212,197,180,161,141,120,97,74,49,24
};

uint8_t vibtrem_speed_factor;
uint8_t vibtrem_table_size;
uint8_t vibtrem_table[256];

tFM_INST_DATA fmpar_table[20];	// array[1..20] of tFM_PARAMETER_TABLE;
bool volume_lock[20];			// array[1..20] of Boolean;
bool vol4op_lock[20] ;			// array[1..20] of Boolean;
bool peak_lock[20];			// array[1..20] of Boolean;
bool pan_lock[20];			// array[1..20] of Boolean;
uint8_t modulator_vol[20];		// array[1..20] of Byte;
uint8_t carrier_vol[20];		// array[1..20] of Byte;
tADTRACK2_EVENT event_table[20];	// array[1..20] of tADTRACK2_EVENT;
uint8_t voice_table[20];		// array[1..20] of Byte;
uint16_t freq_table[20];		// array[1..20] of Word;
uint16_t zero_fq_table[20];		// array[1..20] of Word;
struct {
    uint8_t def, val;
} effect_table[2][20];	// array[1..20] of Word;
uint8_t fslide_table[2][20];		// array[1..20] of Byte;
struct {
    uint8_t def, val;
} glfsld_table[2][20];	// array[1..20] of Word;
struct {
    uint16_t freq;
    uint8_t speed;
} porta_table[2][20];	// array[1..20] of Record freq: Word; speed: Byte; end;
bool portaFK_table[20]; // array[1..20] of Boolean;;
struct {
    uint8_t state, note, add1, add2;
} arpgg_table[2][20];		// array[1..20] of Record state,note,add1,add2: Byte; end;
struct {
    uint8_t pos, dir, speed, depth;
    bool fine;
} vibr_table[2][20];		// array[1..20] of Record pos,speed,depth: Byte; fine: Boolean; end;
struct {
    uint8_t pos, dir, speed, depth;
    bool fine;
} trem_table[2][20];		// array[1..20] of Record pos,speed,depth: Byte; fine: Boolean; end;
uint8_t retrig_table[2][20];	// array[1..20] of Byte;
struct {
    int8_t pos;
    uint8_t volM, volC;
} tremor_table[2][20];		// array[1..20] of Record pos: Integer; volume: Word; end;
uint8_t panning_table[20];	// array[1..20] of Byte;
struct {
    uint8_t def, val;
} last_effect[2][20];	// array[1..20] of Byte;
uint8_t volslide_type[20];	// array[1..20] of Byte;
uint8_t notedel_table[20];	// array[1..20] of Byte;
uint8_t notecut_table[20];	// array[1..20] of Byte;
int8_t ftune_table[20];		// array[1..20] of Shortint;
bool keyoff_loop[20];		// array[1..20] of Boolean;
typedef struct {
    uint16_t fmreg_pos,
         arpg_pos,
         vib_pos;
    uint8_t  fmreg_count,
         fmreg_duration,
         arpg_count,
         vib_count,
         vib_delay,
         fmreg_ins, // fmreg_table
         arpg_table,
         vib_table,
         arpg_note;
    bool vib_paused;
    uint16_t vib_freq;
} tCH_MACRO_TABLE;

tCH_MACRO_TABLE macro_table[20];// array[1..20] of tCH_MACRO_TABLE;

uint8_t loopbck_table[20];	// array[1..20] of Byte;
uint8_t loop_table[20][256];	// array[1..20,0..255] of Byte;
uint8_t misc_register;

uint8_t current_tremolo_depth = 0;
uint8_t current_vibrato_depth = 0;

bool speed_update, lockvol, panlock, lockVP;
uint8_t tremolo_depth, vibrato_depth;
bool volume_scaling, percussion_mode;
uint8_t last_order;
bool reset_chan[20];	// array[1..20] of Boolean;

// This would be later moved to class or struct
tFIXED_SONGDATA _songdata, *songdata = &_songdata;

// Helpers for patterns ===========================================================================
#define MAX_PATTERNS    128
#define ASSERT_PATTERN(PATTERN) (PATTERN < 0 || PATTERN >= (_patterns_allocated < MAX_PATTERNS ? _patterns_allocated : MAX_PATTERNS))
int _patterns_allocated = 0;
tPATTERN_DATA *_patterns[MAX_PATTERNS] = { 0 };

static void patterns_free()
{
    for (int i = 0; i < MAX_PATTERNS; i++) {
        if (_patterns[i]) {
            free(_patterns[i]);
            _patterns[i] = NULL;
        }
    }
}

static bool patterns_alloc(int number)
{
    patterns_free();

    printf("Allocating %d patterns == %d bytes\n", number, number * sizeof(tPATTERN_DATA));
    _patterns_allocated = number;

    for (int i = 0; i < _patterns_allocated; i++) {
        _patterns[i] = calloc(1, sizeof(tPATTERN_DATA));
    }

    return TRUE;
}

static void copy_event(tADTRACK2_EVENT *event, int pattern, int chan, int row)
{
    if (ASSERT_PATTERN(pattern)) {
        printf("ERROR: copy_event(%08x, %d, %d, %02x)\n", event, pattern, chan, row);
        return;
    }

    memcpy(event, &_patterns[pattern]->ch[chan].row[row].ev, sizeof(tADTRACK2_EVENT));
}

// Copy from buffer to an allocated pattern
static void copy_to_pattern(int pattern, tPATTERN_DATA *old)
{
        if (ASSERT_PATTERN(pattern)) {
            printf("ERROR: copy_to_pattern(%d, %08x)\n", pattern, old);
            return;
        }

        tPATTERN_DATA *dst = _patterns[pattern];

        memcpy(dst, old, sizeof(*old));
}

// Copy from old event v1-8 to v9-14 to an allocated pattern
static void copy_to_event(int pattern, int chan, int row, /*tADTRACK2_EVENT_V1234*/ void *old)
{
    if (ASSERT_PATTERN(pattern)) {
        printf("ERROR: copy_to_event(%d, %d, %02x, %08x)\n", pattern, chan, row, old);
        return;
    }

    tADTRACK2_EVENT *ev = &_patterns[pattern]->ch[chan].row[row].ev;

    memcpy(ev, old, 4);
}
// End of patterns helpers ========================================================================

double time_playing;

int ticks, tickD, tickXF;

#define FreqStart 0x156
#define FreqEnd   0x2ae
#define FreqRange  (FreqEnd - FreqStart)

/* PLAYER */
static void opl_out(uint8_t port, uint8_t val); // forward def
static inline bool is_4op_chan(int chan); // forward def
static inline bool is_4op_chan_hi(int chan); // forward def
static inline bool is_4op_chan_lo(int chan); // forward def


static void opl2out(uint16_t reg, uint16_t data)
{
    opl_out(0, reg);
    opl_out(1, data);
}

static void opl3out(uint16_t reg, uint8_t data)
{
    if (reg < 0x100) {
        opl_out(0, reg);
        opl_out(1, data);
    } else {
        opl_out(2, reg & 0xff);
        opl_out(3, data);
    }
}

static void opl3exp(uint16_t data)
{
    opl_out(2, data & 0xff);
    opl_out(3, data >> 8);
}

static uint16_t nFreq(uint8_t note)
{
    static uint16_t Fnum[13] = {0x156,0x16b,0x181,0x198,0x1b0,0x1ca,0x1e5,
                0x202,0x220,0x241,0x263,0x287,0x2ae};

    if (note >= 12 * 8)
        return (7 << 10) | FreqEnd;

    return (note / 12 << 10) | Fnum[note % 12];
}

static uint16_t calc_freq_shift_up(uint16_t freq, uint16_t shift)
{
    uint16_t oc = (freq >> 10) & 7;
    int16_t fr = (freq & 0x3ff) + shift;

    if (fr > FreqEnd) {
        if (oc == 7) {
            fr = FreqEnd;
        } else {
            oc++;
            fr -= FreqRange;
        }
    }

    return (uint16_t)((oc << 10) | fr);
}

static uint16_t calc_freq_shift_down(uint16_t freq, uint16_t shift)
{
    uint16_t oc = (freq >> 10) & 7;
    int16_t fr = (freq & 0x3ff) - shift;

    if (fr < FreqStart) {
        if (oc == 0) {
            fr = FreqStart;
        } else {
            oc--;
            fr += FreqRange;
        }
    }

    return (uint16_t)((oc << 10) | fr);
}

/* == calc_vibtrem_shift() in AT2 */
static uint16_t calc_vibrato_shift(uint8_t depth, uint8_t position)
{
    uint8_t vibr[32] = {
        0,24,49,74,97,120,141,161,180,197,212,224,235,244,250,253,255,
        253,250,244,235,224,212,197,180,161,141,120,97,74,49,24
    };

    /* ATTENTION: wtf this calculation should be ? */
    return (vibr[position & 0x1f] * depth) >> 6;
}

static void change_freq(int chan, uint16_t freq)
{
    if (is_4op_chan(chan) && is_4op_chan_hi(chan)) {
        freq_table[chan + 1] = freq_table[chan];
        chan++;
    }

    freq_table[chan] &= ~0x1fff;
    freq_table[chan] |= (freq & 0x1fff);
    opl3out(0xa0 + _chan_n[chan], freq_table[chan] & 0xFF);
    opl3out(0xb0 + _chan_n[chan], (freq_table[chan] >> 8) & 0xFF);

    if (is_4op_chan(chan) && is_4op_chan_lo(chan)) {
        freq_table[chan - 1] = freq_table[chan];
    }
}

static inline tINSTR_DATA *instrch(int chan)
{
    return &songdata->instr_data[voice_table[chan] - 1];
}

static inline tINSTR_DATA *instrn(uint8_t ins)
{
    return &songdata->instr_data[ins - 1];
}

static bool is_chan_adsr_data_empty(int chan)
{
    tFM_INST_DATA *fmpar = &fmpar_table[chan];

    return (
        !fmpar->data[4] &&
        !fmpar->data[5] &&
        !fmpar->data[6] &&
        !fmpar->data[7]
    );
}

static bool is_ins_adsr_data_empty(int ins)
{
    tINSTR_DATA *i = instrn(ins);

    return (
        !i->fm.data[4] &&
        !i->fm.data[5] &&
        !i->fm.data[6] &&
        !i->fm.data[7]
    );
}

static bool is_data_empty(void *data, unsigned int size)
{
    while (size--) {
        if (*(char *)data++)
            return FALSE;
    }

    return TRUE;
}

static inline uint16_t max(uint16_t value, uint16_t maximum)
{
    return (value > maximum ? maximum : value);
}

static void change_frequency(int chan, uint16_t freq)
{
    macro_table[chan].vib_paused = TRUE;
    change_freq(chan, freq);

    if (is_4op_chan(chan)) {
        if (is_4op_chan_hi(chan)) {
            macro_table[chan + 1].vib_count = 1;
            macro_table[chan + 1].vib_pos = 0;
            macro_table[chan + 1].vib_freq = freq;
            macro_table[chan + 1].vib_paused = FALSE;
        } else {
            macro_table[chan - 1].vib_count = 1;
            macro_table[chan - 1].vib_pos = 0;
            macro_table[chan - 1].vib_freq = freq;
            macro_table[chan - 1].vib_paused = FALSE;
        }
    }

    macro_table[chan].vib_count = 1;
    macro_table[chan].vib_pos = 0;
    macro_table[chan].vib_freq = freq;
    macro_table[chan].vib_paused = FALSE;
}

static inline uint16_t _macro_speedup()
{
    if (!macro_speedup) return 1;

    return macro_speedup;
}

static void set_clock_rate(uint8_t clock_rate)
{
}

static void update_timer(int Hz)
{
    if (Hz == 0) {
        set_clock_rate(0);
        return;
    } else {
        tempo = Hz;
    }

    if (tempo == 18 && timer_fix) {
        IRQ_freq = ((float)tempo + 0.2) * 20.0;
    } else {
        IRQ_freq = 250;
    }

    while (IRQ_freq % (tempo * _macro_speedup()) != 0) {
        IRQ_freq++;
    }

    if (IRQ_freq > MAX_IRQ_FREQ)
        IRQ_freq = MAX_IRQ_FREQ;

    while ((IRQ_freq + IRQ_freq_shift + playback_speed_shift > MAX_IRQ_FREQ) && (playback_speed_shift > 0))
        playback_speed_shift--;

    while ((IRQ_freq + IRQ_freq_shift + playback_speed_shift > MAX_IRQ_FREQ) && (IRQ_freq_shift > 0))
        IRQ_freq_shift--;

    set_clock_rate(1193180 / max(IRQ_freq + IRQ_freq_shift + playback_speed_shift, MAX_IRQ_FREQ));
}

void update_playback_speed(int speed_shift)
{
    if (!speed_shift)
        return;

    if ((speed_shift > 0) && (IRQ_freq + playback_speed_shift + speed_shift > MAX_IRQ_FREQ)) {
        while (IRQ_freq + IRQ_freq_shift + playback_speed_shift + speed_shift > MAX_IRQ_FREQ)
            speed_shift--;
    } else if ((speed_shift < 0) && (IRQ_freq + IRQ_freq_shift + playback_speed_shift + speed_shift < MIN_IRQ_FREQ)) {
        while (IRQ_freq + IRQ_freq_shift + playback_speed_shift + speed_shift < MIN_IRQ_FREQ)
            speed_shift++;
    }

    playback_speed_shift += speed_shift;
    update_timer(tempo);
}

static void key_on(int chan)
{
    if (is_4op_chan(chan) && is_4op_chan_hi(chan)) {
        opl3out(0xb0 + _chan_n[chan + 1], 0);
    } else {
        opl3out(0xb0 + _chan_n[chan], 0);
    }
}

static void key_off(int chan)
{
    freq_table[chan] &= ~0x2000;
    change_frequency(chan, freq_table[chan]);
    event_table[chan].note |= keyoff_flag;
}

static void release_sustaining_sound(int chan)
{
    opl3out(_instr[2] + _chan_m[chan], 63);
    opl3out(_instr[3] + _chan_c[chan], 63);

    // clear adsrw_mod and adsrw_car
    for (int i = 4; i <= 9; i++) {
        fmpar_table[chan].data[i] = 0;
    }

    key_on(chan);
    opl3out(_instr[4] + _chan_m[chan], BYTE_NULL);
    opl3out(_instr[5] + _chan_c[chan], BYTE_NULL);
    opl3out(_instr[6] + _chan_m[chan], BYTE_NULL);
    opl3out(_instr[7] + _chan_c[chan], BYTE_NULL);

    key_off(chan);
    event_table[chan].instr_def = 0;
    reset_chan[chan] = TRUE;
}

// inverted volume here
static uint8_t scale_volume(uint8_t volume, uint8_t scale_factor)
{
    return 63 - ((63 - volume) * (63 - scale_factor) / 63);
}

static uint32_t _4op_data_flag(uint8_t chan)
{
    uint8_t _4op_conn = 0;
    bool _4op_mode;
    uint8_t _4op_ch1, _4op_ch2;
    uint8_t _4op_ins1, _4op_ins2;

    _4op_mode = FALSE;

    if (is_4op_chan(chan)) {
        _4op_mode = TRUE;
        if (is_4op_chan_hi(chan)) {
            _4op_ch1 = chan;
            _4op_ch2 = chan + 1;
        } else {
            _4op_ch1 = chan - 1;
            _4op_ch2 = chan;
        }
        _4op_ins1 = event_table[_4op_ch1].instr_def;
        if (_4op_ins1 == 0) _4op_ins1 = voice_table[_4op_ch1];

        _4op_ins2 = event_table[_4op_ch2].instr_def;
        if (_4op_ins2 == 0) _4op_ins2 = voice_table[_4op_ch2];

        if (_4op_ins1 && _4op_ins2) {
            _4op_mode = TRUE;
            _4op_conn = (instrn(_4op_ins1)->fm.connect << 1) | instrn(_4op_ins2)->fm.connect;
        }
/*
  {------+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+---}
  { BIT  |31|30|29|28|27|26|25|24|23|22|21|20|19|18|17|16|15|14|13|12|11|10| 9| 8| 7| 6| 5| 4| 3| 2| 1| 0 }
  {------+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+---}
  { DATA |..|..|..|..|..|F7|F6|F5|F4|F3|F2|F1|F0|E7|E6|E5|E4|E3|E2|E1|E0|D3|D2|D1|D0|C3|C2|C1|C0|B1|B0|A0 }
  {------+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+---}
*/

        return
            (_4op_mode ? 1 : 0) |		// {1-bit: A0}
            ((_4op_conn & 3) << 1) |	// {2-bit: B1-B0}
            ((_4op_ch1 & 15) << 3) |	// {4-bit: C3-C0}
            ((_4op_ch2 & 15) << 7) |	// {4-bit: D3-D0}
            (_4op_ins1 << 11) |		    // {8-bit: E7-E0}
            (_4op_ins2 << 19);			// {8-bit: F7-F0}
    }

    return 0;
}

static bool _4op_vol_valid_chan(int chan)
{
    uint32_t _4op_flag = _4op_data_flag(chan);
    return ((_4op_flag & 1) && vol4op_lock[chan] &&
            ((_4op_flag >> 11) & 1) &&
            ((_4op_flag >> 19) & 1));
}

// TODO here: fade_out_volume
// inverted volume here
static void set_ins_volume(uint8_t modulator, uint8_t carrier, int chan)
{
    tINSTR_DATA *instr = instrch(chan);

    // ** OPL3 emulation workaround **
    // force muted instrument volume with missing channel ADSR data
    // when there is additionally no FM-reg macro defined for this instrument
    if (is_chan_adsr_data_empty(chan) &&
        !(songdata->instr_macros[voice_table[chan] - 1].length)) {
            modulator = 63;
            carrier = 63;
    }

    // Note: fmpar[].volM/volC has pure unscaled volume,
    // modulator_vol/carrier_vol have scaled but without overall_volume
    if (modulator != BYTE_NULL) {
        bool is_perc_chan = instr->fm.connect ||
                            (percussion_mode && (chan >= 16 && chan <= 19)); // in [17..20]

        fmpar_table[chan].volM = modulator;

        if (is_perc_chan) { // in [17..20]
            if (volume_scaling)
                modulator = scale_volume(instr->fm.volM, modulator);

            modulator = scale_volume(modulator, 63 - global_volume);

            opl3out(_instr[2] + _chan_m[chan],
                scale_volume(modulator, 63 - overall_volume) + (fmpar_table[chan].kslM << 6));
        } else {
            opl3out(_instr[2] + _chan_m[chan], modulator + (fmpar_table[chan].kslM << 6));
        }

        modulator_vol[chan] = 63 - modulator;
    }

    if (carrier != BYTE_NULL) {
        fmpar_table[chan].volC = carrier;

        if (volume_scaling)
            carrier = scale_volume(instr->fm.volC, carrier);

        carrier = scale_volume(carrier, 63 - global_volume);

        opl3out(_instr[3] + _chan_c[chan],
            scale_volume(carrier, 63 - overall_volume) + (fmpar_table[chan].kslC << 6));

        carrier_vol[chan] = 63 - carrier;
    }
}

static void set_volume(uint8_t modulator, uint8_t carrier, uint8_t chan)
{
    tINSTR_DATA *instr = instrch(chan);

    // ** OPL3 emulation workaround **
    // force muted instrument volume with missing channel ADSR data
    // when there is additionally no FM-reg macro defined for this instrument
    if (is_chan_adsr_data_empty(chan) &&
        !(songdata->instr_macros[voice_table[chan] - 1].length)) {
            modulator = 63;
            carrier = 63;
    }

    if (modulator != BYTE_NULL) {
        fmpar_table[chan].volM = modulator;

        modulator = scale_volume(instr->fm.volM, modulator);
        modulator = scale_volume(modulator, /*scale_volume(*/63 - global_volume/*, 63 - fade_out_volume)*/);

        opl3out(_instr[02] + _chan_m[chan],
            scale_volume(modulator, 63 - overall_volume) + (fmpar_table[chan].kslM << 6));

        modulator_vol[chan] = 63 - modulator;
    }

    if (carrier != BYTE_NULL) {
        fmpar_table[chan].volC = carrier;

        carrier = scale_volume(instr->fm.volC, carrier);
        carrier = scale_volume(carrier, /*scale_volume(*/63 - global_volume/*, 63 - fade_out_volume)*/);

        opl3out(_instr[03] + _chan_c[chan],
                scale_volume(carrier, 63 - overall_volume) + (fmpar_table[chan].kslC << 6));

        carrier_vol[chan] = 63 - carrier;
    }
}

static void set_ins_volume_4op(uint8_t volume, uint8_t chan)
{
    uint32_t _4op_flag;
    uint8_t _4op_conn, _4op_ch1, _4op_ch2;

    _4op_flag = _4op_data_flag(chan);
    _4op_conn = (_4op_flag >> 1) & 3;
    _4op_ch1 = (_4op_flag >> 3) & 15;
    _4op_ch2 = (_4op_flag >> 7) & 15;

    if (_4op_vol_valid_chan(chan)) {
        switch (_4op_conn) {
            case 0: // FM/FM
                if (volume == BYTE_NULL) {
                    set_volume(BYTE_NULL, fmpar_table[_4op_ch1].volC, _4op_ch1);
                } else {
                    set_volume(BYTE_NULL, volume, _4op_ch1);
                }
                break;
            case 1: // FM/AM
                if (volume == BYTE_NULL) {
                    set_volume(BYTE_NULL, fmpar_table[_4op_ch1].volC, _4op_ch1);
                    set_volume(fmpar_table[_4op_ch2].volM, BYTE_NULL, _4op_ch2);
                } else {
                    set_volume(BYTE_NULL, volume, _4op_ch1);
                    set_volume(volume, BYTE_NULL, _4op_ch2);
                }
                break;
            case 2: // AM/FM
                if (volume == BYTE_NULL) {
                    set_volume(BYTE_NULL, fmpar_table[_4op_ch1].volC, _4op_ch1);
                    set_volume(BYTE_NULL, fmpar_table[_4op_ch2].volC, _4op_ch2);
                } else {
                    set_volume(BYTE_NULL, volume, _4op_ch1);
                    set_volume(BYTE_NULL, volume, _4op_ch2);
                }
                break;
            case 3:// AM/AM
                if (volume == BYTE_NULL) {
                    set_volume(fmpar_table[_4op_ch1].volM, fmpar_table[_4op_ch1].volC, _4op_ch1);
                    set_volume(fmpar_table[_4op_ch2].volM, BYTE_NULL, _4op_ch2);
                } else {
                    set_volume(volume, volume, _4op_ch1);
                    set_volume(volume, BYTE_NULL, _4op_ch2);
                }
                break;
        }
    }
}

static void reset_ins_volume(int chan)
{
    tINSTR_DATA *instr = instrch(chan);
    uint8_t vol_mod = instr->fm.volM;
    uint8_t vol_car = instr->fm.volC;
    uint8_t conn = instr->fm.connect;

    if (volume_scaling) {
        vol_mod = (!conn ? vol_mod : 0);
        vol_car = 0;
    }

    set_ins_volume(vol_mod, vol_car, chan);
}

static void set_global_volume()
{
    for (int chan = 0; chan < songdata->nm_tracks; chan++) {
        if (_4op_vol_valid_chan(chan)) {
            set_ins_volume_4op(BYTE_NULL, chan);
        } else if (!((carrier_vol[chan] == 0) && (modulator_vol[chan] == 0))) {
            tINSTR_DATA *instr = instrch(chan);

            set_ins_volume(instr->fm.connect ? fmpar_table[chan].volM : BYTE_NULL, fmpar_table[chan].volC, chan);
        }
    }
}

void set_overall_volume(unsigned char level)
{
    overall_volume = max(level, 63);
    set_global_volume();
}

// FIXME: check ins
static void init_macro_table(int chan, uint8_t note, uint8_t ins, uint16_t freq)
{
    macro_table[chan].fmreg_count = 1;
    macro_table[chan].fmreg_pos = 0;
    macro_table[chan].fmreg_duration = 0;
    macro_table[chan].fmreg_ins = ins;
    macro_table[chan].arpg_count = 1;
    macro_table[chan].arpg_pos = 0;
    macro_table[chan].arpg_table = songdata->instr_macros[ins - 1].arpeggio_table;
    macro_table[chan].arpg_note = note;
    macro_table[chan].vib_count = 1;
    macro_table[chan].vib_paused = FALSE;
    macro_table[chan].vib_pos = 0;
    macro_table[chan].vib_table = songdata->instr_macros[ins - 1].vibrato_table;
    macro_table[chan].vib_freq = freq;
    macro_table[chan].vib_delay = songdata->macro_table[macro_table[chan].vib_table - 1].vibrato.delay;
    zero_fq_table[chan] = 0;
}

static void set_ins_data(uint8_t ins, int chan)
{
    tINSTR_DATA *i = instrn(ins);
    uint8_t old_ins;

    if ((ins != event_table[chan].instr_def) || reset_chan[chan]) {
        panning_table[chan] = !pan_lock[chan]
                                  ? i->panning
                                  : songdata->lock_flags[chan] & 3;

        opl3out(_instr[0] + _chan_m[chan], i->fm.data[0]);
        opl3out(_instr[1] + _chan_c[chan], i->fm.data[1]);
        opl3out(_instr[2] + _chan_m[chan], (i->fm.data[2] & 0xc0) + 63);
        opl3out(_instr[3] + _chan_c[chan], (i->fm.data[3] & 0xc0) + 63);
        opl3out(_instr[4] + _chan_m[chan], i->fm.data[4]);
        opl3out(_instr[5] + _chan_c[chan], i->fm.data[5]);
        opl3out(_instr[6] + _chan_m[chan], i->fm.data[6]);
        opl3out(_instr[7] + _chan_c[chan], i->fm.data[7]);
        opl3out(_instr[8] + _chan_m[chan], i->fm.data[8]);
        opl3out(_instr[9] + _chan_c[chan], i->fm.data[9]);
        opl3out(_instr[10] + _chan_n[chan], i->fm.data[10] | _panning[panning_table[chan]]);

        for (int r = 0; r < 11; r++) {
            fmpar_table[chan].data[r] = i->fm.data[r];
        }

        // Stop instr macro if resetting voice
        if (!reset_chan[chan])
            keyoff_loop[chan] = FALSE;

        if (reset_chan[chan]) {
            voice_table[chan] = ins;
            reset_ins_volume(chan);
            reset_chan[chan] = FALSE;
        }

        uint8_t note = event_table[chan].note & 0x7f;
        note = (note > 0 && note <= 12 * 8 + 1) ? note : 0;

        init_macro_table(chan, note, ins, freq_table[chan]);
    }

    voice_table[chan] = ins;
    old_ins = event_table[chan].instr_def;
    event_table[chan].instr_def = ins;

    if (!volume_lock[chan] || (ins != old_ins))
        reset_ins_volume(chan);
}

static void update_modulator_adsrw(int chan)
{
    tFM_INST_DATA *fmpar = &fmpar_table[chan];

    opl3out(_instr[4] + _chan_m[chan], fmpar->data[4]);
    opl3out(_instr[6] + _chan_m[chan], fmpar->data[6]);
    opl3out(_instr[8] + _chan_m[chan], fmpar->data[8]);
}

static void update_carrier_adsrw(int chan)
{
    tFM_INST_DATA *fmpar = &fmpar_table[chan];

    opl3out(_instr[5] + _chan_c[chan], fmpar->data[5]);
    opl3out(_instr[7] + _chan_c[chan], fmpar->data[7]);
    opl3out(_instr[9] + _chan_c[chan], fmpar->data[9]);
}

static void update_fmpar(int chan)
{
    tFM_INST_DATA *fmpar = &fmpar_table[chan];

    opl3out(_instr[0] + _chan_m[chan], fmpar->data[0]);
    opl3out(_instr[1] + _chan_c[chan], fmpar->data[1]);
    opl3out(_instr[10] + _chan_n[chan], fmpar->data[10] | _panning[panning_table[chan]]);

    set_ins_volume(fmpar->volM, fmpar->volC, chan);
}

static inline bool is_4op_chan(int chan) // 0..19
{
    char mask[20] = {
        (1<<0), (1<<0), (1<<1), (1<<1), (1<<2), (1<<2), 0, 0, 0,
        (1<<3), (1<<3), (1<<4), (1<<4), (1<<5), (1<<5), 0, 0, 0, 0, 0
    };
/*
    4-op track extension flags byte, channels 1-18
    0  - tracks 1,2
    1  - tracks 3,4
    2  - tracks 5,6
    3  - tracks 10,11
    4  - tracks 12,13
    5  - tracks 14,15
    6  - %unused%
    7  - %unused%
*/
    return (chan > 14 ? FALSE : !!(songdata->flag_4op & mask[chan]));
}

static inline bool is_4op_chan_hi(int chan)
{
    bool _4op_hi[20] = {
        TRUE, FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE,					// 0, 2, 4
        TRUE, FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE	// 9, 10, 13
    };

    return _4op_hi[chan];
}

static inline bool is_4op_chan_lo(int chan)
{
    bool _4op_lo[20] = {
        FALSE, TRUE, FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, FALSE,					// 1, 3, 5
        FALSE, TRUE, FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE	// 10, 12, 14
    };

    return _4op_lo[chan];
}

static void output_note(uint8_t note, uint8_t ins, int chan, bool restart_macro, bool restart_adsr)
{
    uint16_t freq;

    if ((note == 0) && (ftune_table[chan] == 0)) return;

    if ((note == 0) || (note > 12*8+1)) { // If NOT (note in [1..12*8+1])
        freq = freq_table[chan];
    } else {
        freq = nFreq(note - 1) + songdata->instr_data[ins - 1].fine_tune;

        if (restart_adsr) {
            key_on(chan);
        } else {
            //printf("restart_adsr=false\n");
        }

        freq_table[chan] |= 0x2000;
    }

    if (ftune_table[chan] == -127)
        ftune_table[chan] = 0;

    freq = freq + ftune_table[chan];
    change_frequency(chan, freq);

    if (note) {
        event_table[chan].note = note;

        if (is_4op_chan(chan) && is_4op_chan_lo(chan)) {
            event_table[chan - 1].note = note;
        }

        if (restart_macro) {
            if (!(((event_table[chan].eff[0].def == ef_Extended) &&
                  (event_table[chan].eff[0].val == ef_ex_ExtendedCmd2 * 16 + ef_ex_cmd2_NoRestart)) ||
                  ((event_table[chan].eff[1].def == ef_Extended) &&
                  (event_table[chan].eff[1].val == ef_ex_ExtendedCmd2 * 16 + ef_ex_cmd2_NoRestart)))) {
                init_macro_table(chan, note, ins, freq);
            } else {
                macro_table[chan].arpg_note = note;
            }
        }
    }
}

static void update_fine_effects(int slot, int chan); // forward

static bool no_loop(uint8_t current_chan, uint8_t current_line)
{
    for (int chan = 0; chan < current_chan; chan++) {
        if ((loop_table[chan][current_line] != 0) &&
            (loop_table[chan][current_line] != BYTE_NULL))
            return FALSE;
    }

    return TRUE;
}

static void check_swap_arp_vibr(tADTRACK2_EVENT *event, int slot, int chan); // forward

static void update_effect_table(int slot, int chan, int eff_group, uint8_t def, uint8_t val)
{
    uint8_t eLo = last_effect[slot][chan].def;
    uint8_t eHi = last_effect[slot][chan].val;

    effect_table[slot][chan].def = def;

    // effect_table[slot][chan].val = val ? val : (effect_group[eLo] == eff_group && eHi ? eHi) : 0;
    if (val) {
        effect_table[slot][chan].val = val;
    } else if (effect_group[eLo] == eff_group && eHi) {
        effect_table[slot][chan].val = eHi;
    } else {
        printf("\nCAAATCH\n"); // x00 without any previous compatible command, should never happen
        effect_table[slot][chan].def = 0;
        effect_table[slot][chan].val = 0;
    }
}

static void process_effects(tADTRACK2_EVENT *event, int slot, int chan)
{
    tINSTR_DATA *instr = instrch(chan);
    uint8_t def = event->eff[slot].def;
    uint8_t val = event->eff[slot].val;

    // Use previous effect value if needed
    if (!val && def && def == event_table[chan].eff[slot].def)
        val = event_table[chan].eff[slot].val;

    event_table[chan].eff[slot].def = def;
    event_table[chan].eff[slot].val = val;

    if ((def != ef_Vibrato) &&
        (def != ef_ExtraFineVibrato) &&
        (def != ef_VibratoVolSlide) &&
        (def != ef_VibratoVSlideFine))
        memset(&vibr_table[slot][chan], 0, sizeof(vibr_table[slot][chan]));

    if ((def != ef_RetrigNote) &&
        (def != ef_MultiRetrigNote))
        memset(&retrig_table[slot][chan], 0, sizeof(retrig_table[slot][chan]));

    if ((def != ef_Tremolo) &&
        (def != ef_ExtraFineTremolo))
        memset(&trem_table[slot][chan], 0, sizeof(trem_table[slot][chan]));

    if (!(((def == ef_Arpeggio) && (val != 0)) || (def == ef_ExtraFineArpeggio)) &&
        (arpgg_table[slot][chan].note != 0) && (arpgg_table[slot][chan].state != 1)) {
        arpgg_table[slot][chan].state = 1;
        change_frequency(chan, nFreq(arpgg_table[slot][chan].note - 1) +
            songdata->instr_data[event_table[chan].instr_def - 1].fine_tune);
    }

    if ((def == ef_GlobalFSlideUp) || (def == ef_GlobalFSlideDown)) {
        if ((event->eff[slot ^ 1].def == ef_Extended) &&
            (event->eff[slot ^ 1].val == ef_ex_ExtendedCmd * 16 + ef_ex_cmd_ForceBpmSld)) {

            printf("\n ef_GlobalFSlideUp or ef_GlobalFSlideDown with ef_ex_cmd_ForceBpmSld\n");

            if (def == ef_GlobalFSlideUp) {
                update_playback_speed(val);
            } else {
                update_playback_speed(-val);
            }
        } else {
            uint8_t eff;

            switch (def) {
            case ef_GlobalFSlideUp:
                eff = ef_FSlideUp;

                if ((event->eff[slot ^ 1].def == ef_Extended) &&
                    (event->eff[slot ^ 1].val == ef_ex_ExtendedCmd2 * 16 + ef_ex_cmd2_FTrm_XFGFS)) {
                    eff = ef_Extended2 + ef_fix2 + ef_ex2_FreqSlideUpXF;
                }

                if ((event->eff[slot ^ 1].def == ef_Extended) &&
                    (event->eff[slot ^ 1].val == ef_ex_ExtendedCmd2 * 16 + ef_ex_cmd2_FVib_FGFS)) {
                    eff = ef_FSlideUpFine;
                }

                effect_table[slot][chan].def = eff;
                effect_table[slot][chan].val = val;
                break;
            case ef_GlobalFSlideDown:
                eff = ef_FSlideDown;

                if ((event->eff[slot ^ 1].def == ef_Extended) &&
                    (event->eff[slot ^ 1].val == ef_ex_ExtendedCmd2 * 16 + ef_ex_cmd2_FTrm_XFGFS)) {
                    eff = ef_Extended2 + ef_fix2 + ef_ex2_FreqSlideDnXF;
                }

                if ((event->eff[slot ^ 1].def == ef_Extended) &&
                    (event->eff[slot ^ 1].val == ef_ex_ExtendedCmd2 * 16 + ef_ex_cmd2_FVib_FGFS)) {
                    eff = ef_FSlideDownFine;
                }

                effect_table[slot][chan].def = eff;
                effect_table[slot][chan].val = val;
                break;
            }

            // shouldn't it be int c = 0 ??
            for (int c = chan; c < songdata->nm_tracks; c++) {
                fslide_table[slot][c] = val;
                glfsld_table[slot][c].def = effect_table[slot][chan].def;
                glfsld_table[slot][c].val = effect_table[slot][chan].val;
            }
        }
    }

    if (tremor_table[slot][chan].pos && (def != ef_Tremor)) {
        tremor_table[slot][chan].pos = 0;
        set_ins_volume(tremor_table[slot][chan].volM, tremor_table[slot][chan].volC, chan);
    }

    uint8_t eLo = last_effect[slot][chan].def;

    switch (def) {
    case ef_Arpeggio:
        if (!val)
            break;

    case ef_ExtraFineArpeggio:
    case ef_ArpggVSlide:
    case ef_ArpggVSlideFine:
        switch (def) {
        case ef_Arpeggio:
            effect_table[slot][chan].def = ef_Arpeggio + ef_fix1;
            effect_table[slot][chan].val = val;
            break;
        case ef_ExtraFineArpeggio:
            effect_table[slot][chan].def = ef_ExtraFineArpeggio;
            effect_table[slot][chan].val = val;
            break;
        case ef_ArpggVSlide:
        case ef_ArpggVSlideFine:
            update_effect_table(slot, chan, EFGR_ARPVOLSLIDE, def, val);

            break;
        }

        if (((event->note & 0x7f) >= 1) && ((event->note & 0x7f) <= 12 * 8 + 1)) {
            arpgg_table[slot][chan].state = 0;
            arpgg_table[slot][chan].note = event->note & 0x7f;
            if ((def == ef_Arpeggio) || (def == ef_ExtraFineArpeggio)) {
                arpgg_table[slot][chan].add1 = val >> 4;
                arpgg_table[slot][chan].add2 = val & 0x0f;
            }
        } else {
            if ((event->note == 0) &&
                (((event_table[chan].note & 0x7f) >= 1) &&
                (event_table[chan].note & 0x7f) <= 12 * 8 + 1)) {

                // This never occurs most probably
                if ((def != ef_Arpeggio) &&
                    (def != ef_ExtraFineArpeggio) &&
                    (def != ef_ArpggVSlide) &&
                    (def != ef_ArpggVSlideFine))
                    arpgg_table[slot][chan].state = 0;

                arpgg_table[slot][chan].note = event_table[chan].note & 0x7f;
                if ((def == ef_Arpeggio) ||
                    (def == ef_ExtraFineArpeggio)) {
                    arpgg_table[slot][chan].add1 = val / 16;
                    arpgg_table[slot][chan].add2 = val % 16;
                }
            } else {
                effect_table[slot][chan].def = 0;
                effect_table[slot][chan].val = 0;
                // Check if this really needed
                //event_table[chan].eff[slot].def = 0;
                //event_table[chan].eff[slot].val = 0;
            }
        }
        break;

    case ef_FSlideUp:
    case ef_FSlideDown:
    case ef_FSlideUpFine:
    case ef_FSlideDownFine:
        effect_table[slot][chan].def = def;
        effect_table[slot][chan].val = val;
        fslide_table[slot][chan] = val;
        break;

    case ef_FSlideUpVSlide:
    case ef_FSlUpVSlF:
    case ef_FSlideDownVSlide:
    case ef_FSlDownVSlF:
    case ef_FSlUpFineVSlide:
    case ef_FSlUpFineVSlF:
    case ef_FSlDownFineVSlide:
    case ef_FSlDownFineVSlF:
        update_effect_table(slot, chan, EFGR_FSLIDEVOLSLIDE, def, val);
        break;

    case ef_TonePortamento:
        update_effect_table(slot, chan, EFGR_TONEPORTAMENTO, def, val);

        if ((event->note >= 1) && (event->note <= 12 * 8 + 1)) {
            porta_table[slot][chan].speed = val;
            porta_table[slot][chan].freq = nFreq(event->note - 1) +
                songdata->instr_data[event_table[chan].instr_def - 1].fine_tune;
        } else {
            porta_table[slot][chan].speed = effect_table[slot][chan].val;
        }
        break;

    case ef_TPortamVolSlide:
    case ef_TPortamVSlideFine:
        update_effect_table(slot, chan, EFGR_PORTAVOLSLIDE, def, val);

        break;

    case ef_Vibrato:
    case ef_ExtraFineVibrato:
        update_effect_table(slot, chan, EFGR_VIBRATO, def, val);

        if ((event->eff[slot ^ 1].def == ef_Extended) &&
            (event->eff[slot ^ 1].val == ef_ex_ExtendedCmd2 * 16 + ef_ex_cmd2_FVib_FGFS)) {
            vibr_table[slot][chan].fine = TRUE;
        }

        vibr_table[slot][chan].speed = val / 16;
        vibr_table[slot][chan].depth = val % 16;
        break;

    case ef_Tremolo:
    case ef_ExtraFineTremolo:
        update_effect_table(slot, chan, EFGR_TREMOLO, def, val);

        if ((event->eff[slot ^ 1].def == ef_Extended) &&
            (event->eff[slot ^ 1].val == ef_ex_ExtendedCmd2 * 16 + ef_ex_cmd2_FTrm_XFGFS)) {
            trem_table[slot][chan].fine = TRUE;
        }

        trem_table[slot][chan].speed = val / 16;
        trem_table[slot][chan].depth = val % 16;
        break;

    case ef_VibratoVolSlide:
    case ef_VibratoVSlideFine:
        update_effect_table(slot, chan, EFGR_VIBRATOVOLSLIDE, def, val);

        if ((event->eff[slot ^ 1].def == ef_Extended) &&
            (event->eff[slot ^ 1].val == ef_ex_ExtendedCmd2 * 16 + ef_ex_cmd2_FVib_FGFS))
            vibr_table[slot][chan].fine = TRUE;
        break;

    case ef_SetCarrierVol:
        set_ins_volume(BYTE_NULL, 63 - val, chan);
        break;

    case ef_SetModulatorVol:
        set_ins_volume(63 - val, BYTE_NULL, chan);
        break;

    case ef_SetInsVolume:
        if (_4op_vol_valid_chan(chan)) {
            set_ins_volume_4op(63 - val, chan);
        } else if (percussion_mode && ((chan >= 16) && (chan <= 19))) { //  in [17..20]
            set_ins_volume(63 - val, BYTE_NULL, chan);
        } else if (instr->fm.connect == 0) {
            set_ins_volume(BYTE_NULL, 63 - val, chan);
        } else {
            set_ins_volume(63 - val, 63 - val, chan);
        }
        break;

    case ef_ForceInsVolume:
        if (percussion_mode && ((chan >= 16) && (chan <= 19))) { //  in [17..20]
            set_ins_volume(63 - val, BYTE_NULL, chan);
        } else if (instr->fm.connect == 0) {
            set_ins_volume(scale_volume(instr->fm.volM, 63 - val), 63 - val, chan);
        } else {
            set_ins_volume(63 - val, 63 - val, chan);
        }
        break;

    case ef_PositionJump:
        if (no_loop(chan, current_line)) {
            pattern_break = TRUE;
            next_line = pattern_break_flag + chan;
        }
        break;

    case ef_PatternBreak:
        if (no_loop(chan, current_line)) {
            pattern_break = TRUE;
            // seek_pattern_break = TRUE; // TODO
            next_line = max(val, songdata->patt_len - 1);
        }
        break;

    case ef_SetSpeed:
        speed = val;
        break;

    case ef_SetTempo:
        update_timer(val);
        break;

    case ef_SetWaveform:
        if (val / 16 <= 7) { // in [0..7]
            fmpar_table[chan].wformC = val / 16;
            update_carrier_adsrw(chan);
        }

        if (val % 16 <= 7) { // in [0..7]
            fmpar_table[chan].wformM = val % 16;
            update_modulator_adsrw(chan);
        }
        break;

    case ef_VolSlide:
        effect_table[slot][chan].def = def;
        effect_table[slot][chan].val = val;
        break;

    case ef_VolSlideFine:
        effect_table[slot][chan].def = def;
        effect_table[slot][chan].val = val;
        break;

    case ef_RetrigNote:
    case ef_MultiRetrigNote:
        if (val) {
            if (effect_group[eLo] != EFGR_RETRIGNOTE) {
                retrig_table[slot][chan] = 1;
            }

            effect_table[slot][chan].def = def;
            effect_table[slot][chan].val = val;
        }
        break;

    case ef_SetGlobalVolume:
        global_volume = val;
        set_global_volume();
        break;

    case ef_Tremor:
        if (val) { // if hi and lo part != 0
            if (eLo != ef_Tremor) {
                tremor_table[slot][chan].pos = 0;
                tremor_table[slot][chan].volM = fmpar_table[chan].volM;
                tremor_table[slot][chan].volC = fmpar_table[chan].volC;
            }

            effect_table[slot][chan].def = def;
            effect_table[slot][chan].val = val;
        }
        break;

    case ef_Extended:
        switch (val / 16) {
        case ef_ex_SetTremDepth:
            switch (val % 16) {
            case 0:
                opl3out(_instr[11], misc_register & 0x7f);
                current_tremolo_depth = 0;
                break;

            case 1:
                opl3out(_instr[11], misc_register | 0x80);
                current_tremolo_depth = 1;
                break;
            }

        case ef_ex_SetVibDepth:
            switch (val % 16) {
            case 0:
                opl3out(_instr[11], misc_register & 0xbf);
                current_vibrato_depth = 0;
                break;

            case 1:
                opl3out(_instr[11], misc_register | 0x40);
                current_vibrato_depth = 1;
                break;
            }

        case ef_ex_SetAttckRateM:
            fmpar_table[chan].attckM = val % 16;
            update_modulator_adsrw(chan);
            break;

        case ef_ex_SetDecayRateM:
            fmpar_table[chan].decM = val % 16;
            update_modulator_adsrw(chan);
            break;

        case ef_ex_SetSustnLevelM:
            fmpar_table[chan].sustnM = val % 16;
            update_modulator_adsrw(chan);
            break;

        case ef_ex_SetRelRateM:
            fmpar_table[chan].relM = val % 16;
            update_modulator_adsrw(chan);
            break;

        case ef_ex_SetAttckRateC:
            fmpar_table[chan].attckC = val % 16;
            update_carrier_adsrw(chan);
            break;

        case ef_ex_SetDecayRateC:
            fmpar_table[chan].decC = val % 16;
            update_carrier_adsrw(chan);
            break;

        case ef_ex_SetSustnLevelC:
            fmpar_table[chan].sustnC = val % 16;
            update_carrier_adsrw(chan);
            break;

        case ef_ex_SetRelRateC:
            fmpar_table[chan].relC = val % 16;
            update_carrier_adsrw(chan);
            break;

        case ef_ex_SetFeedback:
            fmpar_table[chan].feedb = val % 16;
            update_fmpar(chan);
            break;

        case ef_ex_SetPanningPos:
            panning_table[chan] = val % 16;
            update_fmpar(chan);
            break;

        case ef_ex_PatternLoop:
        case ef_ex_PatternLoopRec:
            if (val % 16 == 0) {
                loopbck_table[chan] = current_line;
            } else {
                if (loopbck_table[chan] != BYTE_NULL) {
                    if (loop_table[chan][current_line] == BYTE_NULL)
                        loop_table[chan][current_line] = val % 16;

                    if (loop_table[chan][current_line] != 0) {
                        pattern_break = TRUE;
                        next_line = pattern_loop_flag + chan;
                    } else {
                        if (val / 16 == ef_ex_PatternLoopRec)
                            loop_table[chan][current_line] = BYTE_NULL;
                    }
                }
            }
            break;
        case ef_ex_ExtendedCmd:
            switch (val & 0x0f) {
            case ef_ex_cmd_MKOffLoopDi: keyoff_loop[chan] = FALSE;		break;
            case ef_ex_cmd_MKOffLoopEn: keyoff_loop[chan] = TRUE;		break;
            case ef_ex_cmd_TPortaFKdis: portaFK_table[chan] = FALSE;	break;
            case ef_ex_cmd_TPortaFKenb: portaFK_table[chan] = TRUE;		break;
            case ef_ex_cmd_RestartEnv:
                key_on(chan);
                change_freq(chan, freq_table[chan]);
                break;
            case ef_ex_cmd_4opVlockOff:
                if (is_4op_chan(chan)) {
                    vol4op_lock[chan] = FALSE;
                    if (is_4op_chan_hi(chan)) {
                        vol4op_lock[chan + 1] = FALSE;
                    } else {
                        vol4op_lock[chan - 1] = FALSE;
                    }
                }
                break;
            case ef_ex_cmd_4opVlockOn:
                if (is_4op_chan(chan)) {
                    vol4op_lock[chan] = TRUE;
                    if (is_4op_chan_hi(chan)) {
                        vol4op_lock[chan + 1] = TRUE;
                    } else {
                        vol4op_lock[chan - 1] = TRUE;
                    }
                }
                break;
            }
            break;
        case ef_ex_ExtendedCmd2:
            switch (val % 16) {
            case ef_ex_cmd2_RSS:        release_sustaining_sound(chan); break;
            case ef_ex_cmd2_ResetVol:   reset_ins_volume(chan); break;
            case ef_ex_cmd2_LockVol:    volume_lock  [chan] = TRUE; break;
            case ef_ex_cmd2_UnlockVol:  volume_lock  [chan] = FALSE; break;
            case ef_ex_cmd2_LockVP:     peak_lock    [chan] = TRUE; break;
            case ef_ex_cmd2_UnlockVP:   peak_lock    [chan] = FALSE; break;
            case ef_ex_cmd2_VSlide_def: volslide_type[chan] = 0; break;
            case ef_ex_cmd2_LockPan:    pan_lock     [chan] = TRUE; break;
            case ef_ex_cmd2_UnlockPan:  pan_lock     [chan] = FALSE; break;
            case ef_ex_cmd2_VibrOff:    change_frequency(chan, freq_table[chan]); break;
            case ef_ex_cmd2_TremOff:
                if (is_4op_chan(chan)) {
                    set_ins_volume_4op(BYTE_NULL, chan);
                } else {
                    set_ins_volume(fmpar_table[chan].volM, fmpar_table[chan].volC, chan);
                }
                break;
            case ef_ex_cmd2_VSlide_car:
                if ((event->eff[slot ^ 1].def == ef_Extended) &&
                    (event->eff[slot ^ 1].val == ef_ex_ExtendedCmd2 * 16 +
                              ef_ex_cmd2_VSlide_mod)) {
                    volslide_type[chan] = 3;
                } else {
                    volslide_type[chan] = 1;
                }
                break;

            case ef_ex_cmd2_VSlide_mod:
                if ((event->eff[slot ^ 1].def == ef_Extended) &&
                    (event->eff[slot ^ 1].val == ef_ex_ExtendedCmd2 * 16 +
                              ef_ex_cmd2_VSlide_car)) {
                    volslide_type[chan] = 3;
                } else {
                    volslide_type[chan] = 2;
                }
                break;
            }
            break;
        }
        break;

    case ef_Extended2:
        switch (val / 16) {
        case ef_ex2_PatDelayFrame:
            pattern_delay = TRUE;
            tickD = val % 16;
            break;

        case ef_ex2_PatDelayRow:
            pattern_delay = TRUE;
            tickD = speed * (val % 16);
            break;

        case ef_ex2_NoteDelay:
            effect_table[slot][chan].def = ef_Extended2 + ef_fix2 + ef_ex2_NoteDelay;
            effect_table[slot][chan].val = 0;
            notedel_table[chan] = val % 16;
            break;

        case ef_ex2_NoteCut:
            effect_table[slot][chan].def = ef_Extended2 + ef_fix2 + ef_ex2_NoteCut;
            effect_table[slot][chan].val = 0;
            notecut_table[chan] = val % 16;
            break;

        case ef_ex2_FineTuneUp:
            ftune_table[chan] += val % 16;
            break;

        case ef_ex2_FineTuneDown:
            ftune_table[chan] -= val % 16;
            break;

        case ef_ex2_GlVolSlideUp:
        case ef_ex2_GlVolSlideDn:
        case ef_ex2_GlVolSlideUpF:
        case ef_ex2_GlVolSlideDnF:
        case ef_ex2_GlVolSldUpXF:
        case ef_ex2_GlVolSldDnXF:
        case ef_ex2_VolSlideUpXF:
        case ef_ex2_VolSlideDnXF:
        case ef_ex2_FreqSlideUpXF:
        case ef_ex2_FreqSlideDnXF:
            effect_table[slot][chan].def = ef_Extended2 + ef_fix2 + (val >> 4);
            effect_table[slot][chan].val = val & 0x0f;
            break;
        }
        break;

    case ef_Extended3:
        switch (val / 16) {
        case ef_ex3_SetConnection:
            fmpar_table[chan].connect = val % 16;
            update_fmpar(chan);
            break;

        case ef_ex3_SetMultipM:
            fmpar_table[chan].multipM = val % 16;
            update_fmpar(chan);
            break;

        case ef_ex3_SetKslM:
            fmpar_table[chan].kslM = val % 16;
            update_fmpar(chan);
            break;

        case ef_ex3_SetTremoloM:
            fmpar_table[chan].tremM = val % 16;
            update_fmpar(chan);
            break;

        case ef_ex3_SetVibratoM:
            fmpar_table[chan].vibrM = val % 16;
            update_fmpar(chan);
            break;

        case ef_ex3_SetKsrM:
            fmpar_table[chan].ksrM = val % 16;
            update_fmpar(chan);
            break;

        case ef_ex3_SetSustainM:
            fmpar_table[chan].sustM = val % 16;
            update_fmpar(chan);
            break;

        case ef_ex3_SetMultipC:
            fmpar_table[chan].multipC = val % 16;
            update_fmpar(chan);
            break;

        case ef_ex3_SetKslC:
            fmpar_table[chan].kslC = val % 16;
            update_fmpar(chan);
            break;

        case ef_ex3_SetTremoloC:
            fmpar_table[chan].tremC = val % 16;
            update_fmpar(chan);
            break;

        case ef_ex3_SetVibratoC:
            fmpar_table[chan].vibrC = val % 16;
            update_fmpar(chan);
            break;

        case ef_ex3_SetKsrC:
            fmpar_table[chan].ksrC = val % 16;
            update_fmpar(chan);
            break;

        case ef_ex3_SetSustainC:
            fmpar_table[chan].sustC = val % 16;
            update_fmpar(chan);
            break;
        }
        break;
    }
}

static void before_process_note(tADTRACK2_EVENT *event, int chan)
{
    if (event->note == BYTE_NULL) { // Key off
        event->note = event_table[chan].note | keyoff_flag;
    } else if ((event->note >= fixed_note_flag + 1) /*&& (event->note <= fixed_note_flag + 12*8+1)*/) {
        event->note -= fixed_note_flag;
    }

    if (event->note || event->instr_def ||
        (event->eff[0].def | event->eff[0].val) ||
        (event->eff[1].def | event->eff[1].val))
    {
        event_table[chan].eff[0].def = event->eff[0].def;
        event_table[chan].eff[0].val = event->eff[0].val;
        event_table[chan].eff[1].def = event->eff[1].def;
        event_table[chan].eff[1].val = event->eff[1].val;
    }
}

static bool no_swap_and_restart(tADTRACK2_EVENT *event)
{
    // [!xx/@xx] swap arp/swap vib + [zff] no force restart
    return
        !(((event->eff[1].def == ef_SwapArpeggio) ||
            (event->eff[1].def == ef_SwapVibrato)) &&
            (event->eff[0].def == ef_Extended) &&
            (event->eff[0].val == ef_ex_ExtendedCmd2 * 16 + ef_ex_cmd2_NoRestart)) &&
        !(((event->eff[0].def == ef_SwapArpeggio) ||
            (event->eff[0].def == ef_SwapVibrato)) &&
            (event->eff[1].def == ef_Extended) &&
            (event->eff[1].val == ef_ex_ExtendedCmd2 * 16 + ef_ex_cmd2_NoRestart));
}

static bool is_eff_porta(tADTRACK2_EVENT *event)
{
    int eff0 = event->eff[0].def;
    bool is_p0 = (eff0 == ef_TonePortamento) ||
                (eff0 == ef_TPortamVolSlide) ||
                (eff0 == ef_TPortamVSlideFine);
    int eff1 = event->eff[1].def;
    bool is_p1 = (eff1 == ef_TonePortamento) ||
                (eff1 == ef_TPortamVolSlide) ||
                (eff1 == ef_TPortamVSlideFine);
    return is_p0 || is_p1;
}

static bool no_porta_or_delay(int chan)
{
    int eff0 = effect_table[0][chan].def;
    bool is_p0 = (eff0 == ef_TonePortamento) ||
                (eff0 == ef_TPortamVolSlide) ||
                (eff0 == ef_TPortamVSlideFine) ||
                (eff0 == ef_Extended2 + ef_fix2 + ef_ex2_NoteDelay);
    int eff1 = effect_table[1][chan].def;
    bool is_p1 = (eff1 == ef_TonePortamento) ||
                (eff1 == ef_TPortamVolSlide) ||
                (eff1 == ef_TPortamVSlideFine) ||
                (eff1 == ef_Extended2 + ef_fix2 + ef_ex2_NoteDelay);
    return !is_p0 && !is_p1;
}

static void new_process_note(tADTRACK2_EVENT *event, int chan)
{
    bool tporta_flag = is_eff_porta(event);

    for (int slot = 0; slot < 2; slot++) {
        if (event->eff[slot].def | event->eff[slot].val) {
            event_table[chan].eff[slot].def = event->eff[slot].def;
            event_table[chan].eff[slot].val = event->eff[slot].val;
        } else if (glfsld_table[slot][chan].def == 0 && glfsld_table[slot][chan].val == 0) {
            effect_table[slot][chan].def = 0;
            effect_table[slot][chan].val = 0;
        }
    }

    // Treat Tone Portamento based effects vs. note Key-Off's
    if (event->note & keyoff_flag) {
        key_off(chan);
    } else {
        bool no_previous_porta_or_delay = no_porta_or_delay(chan);

        if (no_previous_porta_or_delay) {
            // Usually we end up here
            output_note(event->note, voice_table[chan], chan, TRUE, no_swap_and_restart(event));
        } else if (event->note && tporta_flag) {
            // if previous note was off'ed or restart_adsr enabled for channel
            // and we are doing portamento to a new note
            if (event_table[chan].note & keyoff_flag || portaFK_table[chan]) {
                output_note(event_table[chan].note & ~keyoff_flag, voice_table[chan], chan, FALSE, TRUE);
            } else {
                event_table[chan].note = event->note;
            }
        }
    }
}

static int calc_following_order(uint8_t order);

static void play_line()
{
    tADTRACK2_EVENT _event, *event = &_event;

    if ((current_line == 0) && current_order == calc_following_order(0))
        time_playing = 0;

    if (!(pattern_break && ((next_line & 0xf0) == pattern_loop_flag)) && current_order != last_order) {
        memset(loopbck_table, BYTE_NULL, sizeof(loopbck_table));
        memset(loop_table, BYTE_NULL, sizeof(loop_table));
        last_order = current_order;
    }

    for (int chan = 0; chan < songdata->nm_tracks; chan++) {
        // Do a full copy of the event, because we modify event->note in before_process_note()
        copy_event(event, current_pattern, chan, current_line);

        // save effect_table into last_effect
        for (int slot = 0; slot < 2; slot++) {
            if (effect_table[slot][chan].def && effect_table[slot][chan].val) {
                last_effect[slot][chan].def = effect_table[slot][chan].def;
                last_effect[slot][chan].val = effect_table[slot][chan].val;
            }
            if (glfsld_table[slot][chan].def && glfsld_table[slot][chan].val) {
                effect_table[slot][chan].def = glfsld_table[slot][chan].def;
                effect_table[slot][chan].val = glfsld_table[slot][chan].val;
            } else {
                effect_table[slot][chan].def = 0;
            }
        }
        ftune_table[chan] = 0;

        before_process_note(event, chan);

        if (event->instr_def) {
            // NOTE: adjust ins
            if (is_data_empty(&songdata->instr_data[event->instr_def - 1], sizeof(tINSTR_DATA))) {
                release_sustaining_sound(chan);
            }
            set_ins_data(event->instr_def, chan);
        }

        process_effects(event, 0, chan);
        process_effects(event, 1, chan);

        new_process_note(event, chan);

        check_swap_arp_vibr(event, 0, chan);
        check_swap_arp_vibr(event, 1, chan);

        update_fine_effects(0, chan);
        update_fine_effects(1, chan);
    }
}

static void generate_custom_vibrato(uint8_t value)
{
    const uint8_t vibtab_size[16] = { 16,16,16,16,32,32,32,32,64,64,64,64,128,128,128,128 };
    int idx, idx2;

    #define min0(VALUE) ((int)VALUE >= 0 ? (int)VALUE : 0)

    switch (value) {
    case 0: // set default speed table
        vibtrem_table_size = def_vibtrem_table_size;
        memcpy(&vibtrem_table, &def_vibtrem_table, sizeof(vibtrem_table));
        break;

    case 1 ... 239: // set custom speed table (fixed size = 32)
         vibtrem_table_size = def_vibtrem_table_size;
         double mul_r = (double)value / 16.0;

         for (idx2 = 0; idx2 <= 7; idx2++) {
            vibtrem_table[idx2 * 32] = 0;

            for (idx = 1; idx <= 16; idx++) {
                vibtrem_table[idx2 * 32 + idx] = (uint8_t)round(idx * mul_r);
            }

            for (idx = 17; idx <= 31; idx++) {
                vibtrem_table[idx2 * 32 + idx] = (uint8_t)round((32 - idx) * mul_r);
            }
        }
        break;

    case 240 ... 255: // set custom speed table (speed factor = 1-4)
        vibtrem_speed_factor = (value - 240) % 4 + 1;
        vibtrem_table_size = 2 * vibtab_size[value - 240];
        int mul_b = 256 / (vibtab_size[value - 240]);

        for (idx2 = 0; idx <= 128 / vibtab_size[value - 240] - 1; idx++) {
            vibtrem_table[2 * vibtab_size[value - 240] * idx2] = 0;

            for (idx = 1; idx <= vibtab_size[value - 240]; idx++) {
                vibtrem_table[2 * vibtab_size[value - 240] * idx2 + idx] =
                    min0(idx * mul_b - 1);
            }

            for (idx = vibtab_size[value - 240] + 1; idx <= 2 * vibtab_size[value - 240] - 1; idx++) {
                vibtrem_table[2 * vibtab_size[value - 240] * idx2 + idx] =
                    min0((2 * vibtab_size[value - 240] - idx) * mul_b - 1);
            }
        }
        break;
    }
}

static void check_swap_arp_vibr(tADTRACK2_EVENT *event, int slot, int chan)
{
    // Check if second effect is ZFF - force no restart
    //bool is_norestart = ((((event->eff[slot ^ 1].def << 8) | event->eff[slot ^ 1].val)) ==
    //		    ((ef_Extended << 8) | (ef_ex_ExtendedCmd2 << 4) | ef_ex_cmd2_NoRestart));
    bool is_norestart = (event->eff[slot ^ 1].def == ef_Extended) &&
                        (event->eff[slot ^ 1].val == (ef_ex_ExtendedCmd2 * 16 + ef_ex_cmd2_NoRestart));

    switch (event->eff[slot].def) {
    case ef_SwapArpeggio:
        if (is_norestart) {
            if (macro_table[chan].arpg_pos >
                songdata->macro_table[event->eff[slot].val - 1].arpeggio.length)
                macro_table[chan].arpg_pos =
                    songdata->macro_table[event->eff[slot].val - 1].arpeggio.length;
            macro_table[chan].arpg_table = event->eff[slot].val;
        } else {
            macro_table[chan].arpg_count = 1;
            macro_table[chan].arpg_pos = 0;
            macro_table[chan].arpg_table = event->eff[slot].val;
            macro_table[chan].arpg_note = event_table[chan].note;
        }
        break;

    case ef_SwapVibrato:
        if (is_norestart) {
            if (macro_table[chan].vib_table >
                songdata->macro_table[event->eff[slot].val - 1].vibrato.length)
                macro_table[chan].vib_pos =
                    songdata->macro_table[event->eff[slot].val - 1].vibrato.length;
            macro_table[chan].vib_table = event->eff[slot].val;
        } else {
            macro_table[chan].vib_count = 1;
            macro_table[chan].vib_pos = 0;
            macro_table[chan].vib_table = event->eff[slot].val;
            macro_table[chan].vib_delay =
                songdata->macro_table[macro_table[chan].vib_table - 1].vibrato.delay;
        }
        break;
    case ef_SetCustomSpeedTab:
        generate_custom_vibrato(event->eff[slot].val);
        break;
    }
}

static void portamento_up(int chan, uint16_t slide, uint16_t limit)
{
    uint16_t freq;

    if ((freq_table[chan] & 0x1fff) == 0) return;

    freq = calc_freq_shift_up(freq_table[chan] & 0x1fff, slide);
    if (freq <= limit) {
        change_frequency(chan, freq);
    } else {
        change_frequency(chan, limit);
    }
}

static void portamento_down(int chan, uint16_t slide, uint16_t limit)
{
    uint16_t freq;

    if ((freq_table[chan] & 0x1fff) == 0) return;

    freq = calc_freq_shift_down(freq_table[chan] & 0x1fff, slide);
    if (freq >= limit) {
        change_frequency(chan, freq);
    } else {
        change_frequency(chan, limit);
    }
}

static void macro_vibrato__porta_up(int chan, uint8_t depth)
{
    uint16_t freq = calc_freq_shift_up(macro_table[chan].vib_freq & 0x1fff, depth);

    if (freq <= nFreq(12*8+1)) {
        change_freq(chan, freq);
    } else {
        change_freq(chan, nFreq(12*8+1));
    }
}

static void macro_vibrato__porta_down(int chan, uint8_t depth)
{
    uint16_t freq = calc_freq_shift_down(macro_table[chan].vib_freq & 0x1fff, depth);

    if (freq >= nFreq(0)) {
        change_freq(chan, freq);
    } else {
        change_freq(chan, nFreq(0));
    }
}

static void tone_portamento(int slot, int chan)
{
    uint16_t freq = freq_table[chan] & 0x1fff;

    if (freq > porta_table[slot][chan].freq) {
        portamento_down(chan, porta_table[slot][chan].speed, porta_table[slot][chan].freq);
    } else if (freq < porta_table[slot][chan].freq) {
        portamento_up(chan, porta_table[slot][chan].speed, porta_table[slot][chan].freq);
    }
}

static void slide_carrier_volume_up(uint8_t chan, uint8_t slide, uint8_t limit)
{
    uint8_t volC = fmpar_table[chan].volC;
    uint8_t newvolC = (volC - slide >= limit) ? volC - slide : limit;

    set_ins_volume(BYTE_NULL, newvolC, chan);

}

static void slide_modulator_volume_up(uint8_t chan, uint8_t slide, uint8_t limit)
{
    uint8_t volM = fmpar_table[chan].volM;
    uint8_t newvolM = (volM - slide >= limit) ? volM - slide : limit;

    set_ins_volume(newvolM, BYTE_NULL, chan);
}

static void slide_volume_up(int chan, uint8_t slide)
{
    tINSTR_DATA *i = instrch(chan);
    uint8_t limit1 = 0, limit2 = 0;
    uint32_t _4op_flag;
    uint8_t _4op_conn;
    uint8_t _4op_ch1, _4op_ch2;
    uint8_t _4op_ins1, _4op_ins2;

    _4op_flag = _4op_data_flag(chan);
    _4op_conn = (_4op_flag >> 1) & 3;
    _4op_ch1 = (_4op_flag >> 3) & 15;
    _4op_ch2 = (_4op_flag > 7) & 15;
    _4op_ins1 = (uint8_t)(_4op_flag >> 11) & 0xff;
    _4op_ins2 = (uint8_t)(_4op_flag >> 19) & 0xff;

    if (!_4op_vol_valid_chan(chan)) {
        tINSTR_DATA *ins = &songdata->instr_data[event_table[chan].instr_def - 1];

        limit1 = peak_lock[chan] ? ins->fm.volC : 0;
        limit2 = peak_lock[chan] ? ins->fm.volM : 0;
    }

    switch (volslide_type[chan]) {
    case 0:
        if (!_4op_vol_valid_chan(chan)) {
            slide_carrier_volume_up(chan, slide, limit1);

            if (i->fm.connect || (percussion_mode && (chan >= 16)))  // in [17..20]
               slide_modulator_volume_up(chan, slide, limit2);
        } else {
            tINSTR_DATA *ins1 = instrn(_4op_ins1);
            tINSTR_DATA *ins2 = instrn(_4op_ins2);

            uint8_t limit1_volC = peak_lock[_4op_ch1] ? ins1->fm.volC : 0;
            uint8_t limit1_volM = peak_lock[_4op_ch1] ? ins1->fm.volM : 0;
            uint8_t limit2_volC = peak_lock[_4op_ch2] ? ins2->fm.volC : 0;
            uint8_t limit2_volM = peak_lock[_4op_ch2] ? ins2->fm.volM : 0;

            switch (_4op_conn) {
            // FM/FM
            case 0:
                slide_carrier_volume_up(_4op_ch1, slide, limit1_volC);
                break;
            // FM/AM
            case 1:
                slide_carrier_volume_up(_4op_ch1, slide, limit1_volC);
                slide_modulator_volume_up(_4op_ch2, slide, limit2_volM);
                break;
            // AM/FM
            case 2:
                slide_carrier_volume_up(_4op_ch1, slide, limit1_volC);
                slide_carrier_volume_up(_4op_ch2, slide, limit2_volC);
                break;
            // AM/AM
            case 3:
                slide_carrier_volume_up(_4op_ch1, slide, limit1_volC);
                slide_modulator_volume_up(_4op_ch1, slide, limit1_volM);
                slide_modulator_volume_up(_4op_ch2, slide, limit2_volM);
                break;
           }
        }
        break;

    case 1:
        slide_carrier_volume_up(chan, slide, limit1);
        break;

    case 2:
        slide_modulator_volume_up(chan, slide, limit2);
        break;

    case 3:
        slide_carrier_volume_up(chan, slide, limit1);
        slide_modulator_volume_up(chan, slide, limit2);
        break;
    }
}

static void slide_carrier_volume_down(uint8_t chan, uint8_t slide)
{
    uint8_t volC = fmpar_table[chan].volC;
    uint8_t newvolC = volC + slide <= 63 ? volC + slide : 63;

    set_ins_volume(BYTE_NULL, newvolC, chan);
}

static void slide_modulator_volume_down(uint8_t chan, uint8_t slide)
{
    uint8_t volM = fmpar_table[chan].volM;
    uint8_t newvolM = volM + slide <= 63 ? volM + slide : 63;

    set_ins_volume(newvolM, BYTE_NULL, chan);
}

static void slide_volume_down(int chan, uint8_t slide)
{
    tINSTR_DATA *instr = instrch(chan);
    uint32_t _4op_flag;
    uint8_t _4op_conn;
    uint8_t _4op_ch1, _4op_ch2;

    _4op_flag = _4op_data_flag(chan);
    _4op_conn = (_4op_flag >> 1) & 3;
    _4op_ch1 = (_4op_flag >> 3) & 15;
    _4op_ch2 = (_4op_flag >> 7) & 15;

    switch (volslide_type[chan]) {
    case 0:
        if (!_4op_vol_valid_chan(chan)) {
            slide_carrier_volume_down(chan, slide);

            if (instr->fm.connect || (percussion_mode && (chan >= 16))) { //in [17..20]
               slide_modulator_volume_down(chan, slide);
            }
        } else {
            switch (_4op_conn) {
            // FM/FM
            case 0:
                slide_carrier_volume_down(_4op_ch1, slide);
                break;
            // FM/AM
            case 1:
                slide_carrier_volume_down(_4op_ch1, slide);
                slide_modulator_volume_down(_4op_ch2, slide);
                break;
            // AM/FM
            case 2:
                slide_carrier_volume_down(_4op_ch1, slide);
                slide_carrier_volume_down(_4op_ch2, slide);
                break;
            // AM/AM
            case 3:
                slide_carrier_volume_down(_4op_ch1, slide);
                slide_modulator_volume_down(_4op_ch1, slide);
                slide_modulator_volume_down(_4op_ch2, slide);
                break;
            }
        }
        break;

    case 1:
        slide_carrier_volume_down(chan, slide);
        break;

    case 2:
        slide_modulator_volume_down(chan, slide);
        break;

    case 3:
        slide_carrier_volume_down(chan, slide);
        slide_modulator_volume_down(chan, slide);
        break;
    }
}

static void volume_slide(int chan, uint8_t up_speed, uint8_t down_speed)
{
    if (up_speed)
        slide_volume_up(chan, up_speed);
    else if (down_speed) {
        slide_volume_down(chan, down_speed);
    }
}

static void global_volume_slide(uint8_t up_speed, uint8_t down_speed)
{
    if (up_speed != BYTE_NULL)
        global_volume = max(global_volume + up_speed, 63);

    if (down_speed != BYTE_NULL) {
        if (global_volume >= down_speed)
            global_volume -= down_speed;
        else
            global_volume = 0;
    }

    set_global_volume();
}

static void arpeggio(int slot, int chan)
{
    uint8_t arpgg_state[3] = {1, 2, 0};

    uint16_t freq;

    switch (arpgg_table[slot][chan].state) {
    case 0: freq = nFreq(arpgg_table[slot][chan].note - 1); break;
    case 1: freq = nFreq(arpgg_table[slot][chan].note - 1 + arpgg_table[slot][chan].add1); break;
    case 2: freq = nFreq(arpgg_table[slot][chan].note - 1 + arpgg_table[slot][chan].add2); break;
    default: freq = 0;
    }

    arpgg_table[slot][chan].state = arpgg_state[arpgg_table[slot][chan].state];
    change_frequency(chan, freq +
            songdata->instr_data[event_table[chan].instr_def - 1].fine_tune);
}

static void vibrato(int slot, int chan)
{
    uint16_t freq, slide;
    uint8_t direction;

    freq = freq_table[chan];

    vibr_table[slot][chan].pos += vibr_table[slot][chan].speed;
    slide = calc_vibrato_shift(vibr_table[slot][chan].depth, vibr_table[slot][chan].pos);
    direction = vibr_table[slot][chan].pos & 0x20;

    if (direction == 0)
        portamento_down(chan, slide, nFreq(0));
    else
        portamento_up(chan, slide, nFreq(12*8+1));

    freq_table[chan] = freq;
}

static void tremolo(int slot, int chan)
{
    uint16_t slide;
    uint8_t direction;

    uint8_t volM = fmpar_table[chan].volM;
    uint8_t volC = fmpar_table[chan].volC;

    trem_table[slot][chan].pos += trem_table[slot][chan].speed;
    slide = calc_vibrato_shift(trem_table[slot][chan].depth, trem_table[slot][chan].pos);
    direction = trem_table[slot][chan].pos & 0x20;

    if (direction == 0)
        slide_volume_down(chan, slide);
    else
        slide_volume_up(chan, slide);

    // is this needed?
    fmpar_table[chan].volM = volM;
    fmpar_table[chan].volC = volC;
}

static inline int chanvol(int chan)
{
    tINSTR_DATA *instr = instrch(chan);

    if (instr->fm.connect == 0)
        return 63 - fmpar_table[chan].volC;
    else
        return 63 - (fmpar_table[chan].volM + fmpar_table[chan].volC) / 2;
}

static void update_effects_slot(int slot, int chan)
{
    uint8_t eLo, eHi;

    eLo  = effect_table[slot][chan].def;
    eHi  = effect_table[slot][chan].val;

    switch (eLo) {
    case ef_Arpeggio + ef_fix1:
        arpeggio(slot, chan);
        break;

    case ef_ArpggVSlide:
        volume_slide(chan, eHi / 16, eHi % 16);
        arpeggio(slot, chan);
        break;

    case ef_ArpggVSlideFine:
        arpeggio(slot, chan);
        break;

    case ef_FSlideUp:
        portamento_up(chan, eHi, nFreq(12*8+1));
        break;

    case ef_FSlideDown:
        portamento_down(chan, eHi, nFreq(0));
        break;

    case ef_FSlideUpVSlide:
        portamento_up(chan, fslide_table[slot][chan], nFreq(12*8+1));
        volume_slide(chan, eHi / 16, eHi % 16);
        break;

    case ef_FSlUpVSlF:
        portamento_up(chan, fslide_table[slot][chan], nFreq(12*8+1));
        break;

    case ef_FSlideDownVSlide:
        portamento_down(chan, fslide_table[slot][chan], nFreq(0));
        volume_slide(chan, eHi / 16, eHi % 16);
        break;

    case ef_FSlDownVSlF:
        portamento_down(chan, fslide_table[slot][chan], nFreq(0));
        break;

    case ef_FSlUpFineVSlide:
        volume_slide(chan, eHi / 16, eHi % 16);
        break;

    case ef_FSlDownFineVSlide:
        volume_slide(chan, eHi / 16, eHi % 16);
        break;

    case ef_TonePortamento:
        tone_portamento(slot, chan);
        break;

    case ef_TPortamVolSlide:
        volume_slide(chan, eHi / 16, eHi % 16);
        tone_portamento(slot, chan);
        break;

    case ef_TPortamVSlideFine:
        tone_portamento(slot, chan);
        break;

    case ef_Vibrato:
        if (!vibr_table[slot][chan].fine)
            vibrato(slot, chan);
        break;

    case ef_Tremolo:
        if (!trem_table[slot][chan].fine)
            tremolo(slot, chan);
        break;

    case ef_VibratoVolSlide:
        volume_slide(chan, eHi / 16, eHi % 16);
        if (!vibr_table[slot][chan].fine)
            vibrato(slot, chan);
        break;

    case ef_VibratoVSlideFine:
        if (!vibr_table[slot][chan].fine)
            vibrato(slot, chan);
        break;

    case ef_VolSlide:
        volume_slide(chan, eHi / 16, eHi % 16);
        break;

    case ef_RetrigNote:
        if (retrig_table[slot][chan] >= eHi) {
            retrig_table[slot][chan] = 0;
            output_note(event_table[chan].note, event_table[chan].instr_def, chan, TRUE, TRUE);
        } else {
            retrig_table[slot][chan]++;
        }
        break;

    case ef_MultiRetrigNote:
        if (retrig_table[slot][chan] >= eHi / 16) {
            switch (eHi % 16) {
            case 0: break;
            case 8: break;

            case 1: slide_volume_down(chan, 1); break;
            case 2: slide_volume_down(chan, 2); break;
            case 3: slide_volume_down(chan, 4); break;
            case 4: slide_volume_down(chan, 8); break;
            case 5: slide_volume_down(chan, 16); break;

            case 9: slide_volume_up(chan, 1); break;
            case 10: slide_volume_up(chan, 2); break;
            case 11: slide_volume_up(chan, 4); break;
            case 12: slide_volume_up(chan, 8); break;
            case 13: slide_volume_up(chan, 16); break;

            case 6: slide_volume_down(chan, chanvol(chan) - chanvol(chan) * 2 / 3);
                break;

            case 7: slide_volume_down(chan, chanvol(chan) - chanvol(chan) * 1 / 2);
                break;

            case 14: slide_volume_up(chan, max(chanvol(chan) * 3 / 2 - chanvol(chan), 63));
                break;

            case 15: slide_volume_up(chan,max(chanvol(chan) * 2 - chanvol(chan), 63));
                break;
            }

            retrig_table[slot][chan] = 0;
            output_note(event_table[chan].note, event_table[chan].instr_def, chan, TRUE, TRUE);
        } else {
            retrig_table[slot][chan]++;
        }
        break;

    case ef_Tremor:
        if (tremor_table[slot][chan].pos >= 0) {
            if ((tremor_table[slot][chan].pos + 1) <= eHi / 16) {
                tremor_table[slot][chan].pos++;
            } else {
                slide_volume_down(chan, 63);
                tremor_table[slot][chan].pos = -1;
            }
        } else {
            if ((tremor_table[slot][chan].pos - 1) >= -(eHi % 16)) {
                tremor_table[slot][chan].pos--;
            } else {
                set_ins_volume(tremor_table[slot][chan].volM, tremor_table[slot][chan].volC, chan);
                tremor_table[slot][chan].pos = 1;
            }
        }
        break;

    case ef_Extended2 + ef_fix2 + ef_ex2_NoteDelay:
        if (notedel_table[chan] == 0) {
            notedel_table[chan] = BYTE_NULL;
            output_note(event_table[chan].note,	event_table[chan].instr_def, chan, TRUE, TRUE);
        } else if (notedel_table[chan] != BYTE_NULL) {
            notedel_table[chan]--;
        }
        break;

    case ef_Extended2 + ef_fix2 + ef_ex2_NoteCut:
        if (notecut_table[chan] == 0) {
            notecut_table[chan] = BYTE_NULL;
            key_off(chan);
        } else if (notecut_table[chan] != BYTE_NULL) {
            notecut_table[chan]--;
        }
        break;

    case ef_Extended2 + ef_fix2 + ef_ex2_GlVolSlideUp:
        global_volume_slide(eHi % 16, BYTE_NULL);
        break;

    case ef_Extended2 + ef_fix2 + ef_ex2_GlVolSlideDn:
        global_volume_slide(BYTE_NULL, eHi % 16);
        break;
    }
}

static void update_effects()
{
    for (int chan = 0; chan < songdata->nm_tracks; chan++) {
        update_effects_slot(0, chan);
        update_effects_slot(1, chan);
    }
}

static void update_fine_effects(int slot, int chan)
{
    uint8_t def, val;

    //def = event_table[chan].eff[slot].def;
    //val = event_table[chan].eff[slot].val;
    def = effect_table[slot][chan].def;
    val = effect_table[slot][chan].val;

    switch (def) {
    case ef_ArpggVSlideFine:
        volume_slide(chan, val / 16, val % 16);
        break;

    case ef_FSlideUpFine:
        portamento_up(chan, val, nFreq(12*8+1));
        break;

    case ef_FSlideDownFine:
        portamento_down(chan, val, nFreq(0));
        break;

    case ef_FSlUpVSlF:
        volume_slide(chan, val / 16, val % 16);
        break;

    case ef_FSlDownVSlF:
        volume_slide(chan, val / 16, val % 16);
        break;

    case ef_FSlUpFineVSlide:
        portamento_up(chan, fslide_table[slot][chan], nFreq(12*8+1));
        break;

    case ef_FSlUpFineVSlF:
        portamento_up(chan, fslide_table[slot][chan], nFreq(12*8+1));
        volume_slide(chan, val / 16, val % 16);
        break;

    case ef_FSlDownFineVSlide:
        portamento_down(chan, fslide_table[slot][chan], nFreq(0));
        break;

    case ef_FSlDownFineVSlF:
        portamento_down(chan, fslide_table[slot][chan], nFreq(0));
        volume_slide(chan, val / 16, val % 16);
        break;

    case ef_TPortamVSlideFine:
        volume_slide(chan, val / 16, val % 16);
        break;

    case ef_Vibrato:
        if (vibr_table[slot][chan].fine)
            vibrato(slot, chan);
        break;

    case ef_Tremolo:
        if (trem_table[slot][chan].fine)
            tremolo(slot, chan);
        break;

    case ef_VibratoVolSlide:
        if (vibr_table[slot][chan].fine)
            vibrato(slot, chan);
        break;

    case ef_VibratoVSlideFine:
        volume_slide(chan, val / 16, val % 16);
        if (vibr_table[slot][chan].fine)
            vibrato(slot, chan);
        break;

    case ef_VolSlideFine:
        volume_slide(chan, val / 16, val % 16);
        break;

    case ef_Extended2 + ef_fix2 + ef_ex2_GlVolSlideUpF:
        global_volume_slide(val, BYTE_NULL);
        break;
    case ef_Extended2 + ef_fix2 + ef_ex2_GlVolSlideDnF:
        global_volume_slide(BYTE_NULL, val);
        break;
    }
}

static void update_extra_fine_effects_slot(int slot, int chan)
{
    //int def, val;
    uint8_t eLo, eHi;

    //def = event_table[chan].eff[slot].def;
    //val = event_table[chan].eff[slot].val;
    eLo = effect_table[slot][chan].def;
    eHi = effect_table[slot][chan].val;

    switch (eLo) {
    case ef_Extended2 + ef_fix2 + ef_ex2_GlVolSldUpXF:  global_volume_slide(eHi, BYTE_NULL); break;
    case ef_Extended2 + ef_fix2 + ef_ex2_GlVolSldDnXF:  global_volume_slide(BYTE_NULL, eHi); break;
    case ef_Extended2 + ef_fix2 + ef_ex2_VolSlideUpXF:  volume_slide(chan, eHi, 0); break;
    case ef_Extended2 + ef_fix2 + ef_ex2_VolSlideDnXF:  volume_slide(chan, 0, eHi); break;
    case ef_Extended2 + ef_fix2 + ef_ex2_FreqSlideUpXF: portamento_up(chan, eHi, nFreq(12*8+1)); break;
    case ef_Extended2 + ef_fix2 + ef_ex2_FreqSlideDnXF: portamento_down(chan, eHi, nFreq(0)); break;

    case ef_ExtraFineArpeggio:
        arpeggio(slot, chan);
        break;

    case ef_ExtraFineVibrato:
        if (!vibr_table[slot][chan].fine)
            vibrato(slot, chan);
        break;

    case ef_ExtraFineTremolo:
        if (!trem_table[slot][chan].fine)
            tremolo(slot, chan);
        break;
    }
}

static void update_extra_fine_effects()
{
    for (int chan = 0; chan < songdata->nm_tracks; chan++) {
        update_extra_fine_effects_slot(0, chan);
        update_extra_fine_effects_slot(1, chan);
    }
}

static int calc_following_order(uint8_t order)
{
    int result;
    uint8_t index, jump_count;

    result = -1;
    index = order;
    jump_count = 0;

    do {
        if (songdata->pattern_order[index] < 0x80) {
            result = index;
        } else {
            index = songdata->pattern_order[index] - 0x80;
            jump_count++;
        }
    } while (!((jump_count > 0x7f) || (result != -1)));

    return result;
}

static int calc_order_jump()
{
    uint8_t temp;
    int result;

    result = 0;
    temp = 0;

    do {
        if (songdata->pattern_order[current_order] > 0x7f)
            current_order = songdata->pattern_order[current_order] - 0x80;
        temp++;
    } while (!((temp > 0x7f) || (songdata->pattern_order[current_order] < 0x80)));

    if (temp > 0x7f) {
        a2t_stop();
        result = -1;
    }

    return result;
}

static void update_song_position()
{
    if ((current_line < songdata->patt_len - 1) && !pattern_break) {
        current_line++;
    } else {
        if (!(pattern_break && ((next_line & 0xf0) == pattern_loop_flag)) &&
             (current_order < 0x7f)) {
            memset(loopbck_table, BYTE_NULL, sizeof(loopbck_table));
            memset(loop_table, BYTE_NULL, sizeof(loop_table));
            current_order++;
        }

        if (pattern_break && ((next_line & 0xf0) == pattern_loop_flag)) {
            uint8_t temp;

            temp = next_line - pattern_loop_flag;
            next_line = loopbck_table[temp];

            if (loop_table[temp][current_line] != 0)
                loop_table[temp][current_line]--;
        } else {
            if (pattern_break && ((next_line & 0xf0) == pattern_break_flag)) {
                if (event_table[next_line - pattern_break_flag].eff[1].def == ef_PositionJump) {
                    current_order = event_table[next_line - pattern_break_flag].eff[1].val;
                } else {
                    current_order = event_table[next_line - pattern_break_flag].eff[0].val;
                }
                pattern_break = FALSE;
            } else {
                if (current_order >= 0x7f)
                    current_order = 0;
            }
        }

        if ((songdata->pattern_order[current_order] > 0x7f) &&
            (calc_order_jump() == -1))
            return;

        current_pattern = songdata->pattern_order[current_order];
        if (!pattern_break) {
            current_line = 0;
        } else {
            pattern_break = FALSE;
            current_line = next_line;
        }
    }

    for (int chan = 0; chan < songdata->nm_tracks; chan++) {
        glfsld_table[0][chan].def = 0;
        glfsld_table[0][chan].val = 0;
        glfsld_table[1][chan].def = 0;
        glfsld_table[1][chan].val = 0;
    }

    if ((current_line == 0) &&
        (current_order == calc_following_order(0)) && speed_update) {
        tempo = songdata->tempo;
        speed = songdata->speed;
        update_timer(tempo);
    }
}

static void poll_proc()
{
    if (pattern_delay) {
        update_effects();
        if (tickD > 1) {
            tickD--;
        } else {
            pattern_delay = FALSE;
        }
    } else {
        if (ticks == 0) {
            play_line();
            ticks = speed;
            update_song_position();
        }
        update_effects();
        ticks--;
    }

    tickXF++;
    if (tickXF % 4 == 0) {
        update_extra_fine_effects();
        tickXF -= 4;
    }
}

static void macro_poll_proc()
{
#define  IDLE		0xfff
#define  FINISHED	0xffff
    uint16_t chan;
    uint16_t finished_flag;

    for (chan = 0; chan < 20; chan++) {
        finished_flag = keyoff_loop[chan] ? IDLE : FINISHED;
        /*if (!keyoff_loop[chan]) {
            finished_flag = FINISHED;
        } else {
            finished_flag = IDLE;
        }*/

        tCH_MACRO_TABLE *mt = &macro_table[chan];
        uint8_t fmreg_ins = mt->fmreg_ins - 1;
        tINSTR_MACRO *rt = &songdata->instr_macros[fmreg_ins];
        bool force_macro_keyon = FALSE;

        if (mt->fmreg_ins /*&& (speed != 0)*/) { // FIXME: what speed?
            if (mt->fmreg_duration > 1) {
                mt->fmreg_duration--;
            } else {
                mt->fmreg_count = 1;
                if (mt->fmreg_pos <= rt->length) {
                    if ((rt->loop_begin != 0) && (rt->loop_length != 0)) {
                        if (mt->fmreg_pos == rt->loop_begin + (rt->loop_length-1)) {
                            mt->fmreg_pos = rt->loop_begin;
                        } else {
                            if (mt->fmreg_pos < rt->length) {
                                mt->fmreg_pos++;
                            } else {
                                mt->fmreg_pos = finished_flag;
                            }
                        }
                    } else {
                        if (mt->fmreg_pos < rt->length) {
                            mt->fmreg_pos++;
                        } else {
                            mt->fmreg_pos = finished_flag;
                        }
                    }
                } else {
                    mt->fmreg_pos = finished_flag;
                }

                if (((freq_table[chan] | 0x2000) == freq_table[chan]) &&
                     (rt->keyoff_pos != 0) &&
                     (mt->fmreg_pos >= rt->keyoff_pos)) {
                    mt->fmreg_pos = IDLE;
                } else {
                    if (((freq_table[chan] | 0x2000) != freq_table[chan]) &&
                         (mt->fmreg_pos != 0) && (rt->keyoff_pos != 0) &&
                        ((mt->fmreg_pos < rt->keyoff_pos) || (mt->fmreg_pos == IDLE)))
                        mt->fmreg_pos = rt->keyoff_pos;
                }

                if ((mt->fmreg_pos != 0) &&
                    (mt->fmreg_pos != IDLE) && (mt->fmreg_pos != finished_flag)) {
                    mt->fmreg_duration = rt->data[mt->fmreg_pos - 1].duration;
                    if (mt->fmreg_duration != 0) {
                        tREGISTER_TABLE_DEF *d = &rt->data[mt->fmreg_pos - 1];

                        // force KEY-ON with missing ADSR instrument data
                        force_macro_keyon = FALSE;
                        if (mt->fmreg_pos == 1) {
                            if (is_ins_adsr_data_empty(voice_table[chan]) &&
                                !(songdata->dis_fmreg_col[fmreg_ins][0] &&
                                  songdata->dis_fmreg_col[fmreg_ins][1] &&
                                  songdata->dis_fmreg_col[fmreg_ins][2] &&
                                  songdata->dis_fmreg_col[fmreg_ins][3] &&
                                  songdata->dis_fmreg_col[fmreg_ins][12] &&
                                  songdata->dis_fmreg_col[fmreg_ins][13] &&
                                  songdata->dis_fmreg_col[fmreg_ins][14] &&
                                  songdata->dis_fmreg_col[fmreg_ins][15])) {
                                force_macro_keyon = TRUE;
                            }
                        }

                        // use translation_table[column] = { offset, mask, shift}
                        if (!songdata->dis_fmreg_col[fmreg_ins][0])
                            fmpar_table[chan].attckM = d->fm.attckM;

                        if (!songdata->dis_fmreg_col[fmreg_ins][1])
                            fmpar_table[chan].decM = d->fm.decM;

                        if (!songdata->dis_fmreg_col[fmreg_ins][2])
                            fmpar_table[chan].sustnM = d->fm.sustnM;

                        if (!songdata->dis_fmreg_col[fmreg_ins][3])
                            fmpar_table[chan].relM = d->fm.relM;

                        if (!songdata->dis_fmreg_col[fmreg_ins][4])
                            fmpar_table[chan].wformM = d->fm.wformM;

                        if (!songdata->dis_fmreg_col[fmreg_ins][6])
                            fmpar_table[chan].kslM = d->fm.kslM;

                        if (!songdata->dis_fmreg_col[fmreg_ins][7])
                            fmpar_table[chan].multipM = d->fm.multipM;

                        if (!songdata->dis_fmreg_col[fmreg_ins][8])
                            fmpar_table[chan].tremM = d->fm.tremM;

                        if (!songdata->dis_fmreg_col[fmreg_ins][9])
                            fmpar_table[chan].vibrM = d->fm.vibrM;

                        if (!songdata->dis_fmreg_col[fmreg_ins][10])
                            fmpar_table[chan].ksrM = d->fm.ksrM;

                        if (!songdata->dis_fmreg_col[fmreg_ins][11])
                            fmpar_table[chan].sustM = d->fm.sustM;

                        if (!songdata->dis_fmreg_col[fmreg_ins][12])
                            fmpar_table[chan].attckC = d->fm.attckC;

                        if (!songdata->dis_fmreg_col[fmreg_ins][13])
                            fmpar_table[chan].decC = d->fm.decC;

                        if (!songdata->dis_fmreg_col[fmreg_ins][14])
                            fmpar_table[chan].sustnC = d->fm.sustnC;

                        if (!songdata->dis_fmreg_col[fmreg_ins][15])
                            fmpar_table[chan].relC = d->fm.relC;

                        if (!songdata->dis_fmreg_col[fmreg_ins][16])
                            fmpar_table[chan].wformC = d->fm.wformC;

                        if (!songdata->dis_fmreg_col[fmreg_ins][18])
                            fmpar_table[chan].kslC = d->fm.kslC;

                        if (!songdata->dis_fmreg_col[fmreg_ins][19])
                            fmpar_table[chan].multipC = d->fm.multipC;

                        if (!songdata->dis_fmreg_col[fmreg_ins][20])
                            fmpar_table[chan].tremC = d->fm.tremC;

                        if (!songdata->dis_fmreg_col[fmreg_ins][21])
                            fmpar_table[chan].vibrC = d->fm.vibrC;

                        if (!songdata->dis_fmreg_col[fmreg_ins][22])
                            fmpar_table[chan].ksrC = d->fm.ksrC;

                        if (!songdata->dis_fmreg_col[fmreg_ins][23])
                            fmpar_table[chan].sustC = d->fm.sustC;

                        if (!songdata->dis_fmreg_col[fmreg_ins][24])
                            fmpar_table[chan].connect = d->fm.connect;

                        if (!songdata->dis_fmreg_col[fmreg_ins][25])
                            fmpar_table[chan].feedb = d->fm.feedb;

                        if (!songdata->dis_fmreg_col[fmreg_ins][27] && !pan_lock[chan])
                            panning_table[chan] = d->panning;

                        if (!songdata->dis_fmreg_col[fmreg_ins][5])
                            set_ins_volume(63 - d->fm.volM, BYTE_NULL, chan);

                        if (!songdata->dis_fmreg_col[fmreg_ins][17])
                            set_ins_volume(BYTE_NULL, 63 - d->fm.volC, chan);

                        update_modulator_adsrw(chan);
                        update_carrier_adsrw(chan);
                        update_fmpar(chan);

                        // TODO: check is those flags are really set by the editor
                        uint8_t macro_flags = d->fm.data[10];

                        if (force_macro_keyon || (macro_flags & 0x80)) { // MACRO_NOTE_RETRIG_FLAG
                            if (!((is_4op_chan(chan) && is_4op_chan_hi(chan)))) {
                                output_note(event_table[chan].note,
                                            event_table[chan].instr_def, chan, FALSE, TRUE);
                                if (is_4op_chan(chan) && is_4op_chan_lo(chan))
                                    init_macro_table(chan - 1, 0, voice_table[chan - 1], 0);
                            }
                        } else if (macro_flags & 0x40) { // MACRO_ENVELOPE_RESTART_FLAG
                            key_on(chan);
                            change_freq(chan, freq_table[chan]);
                        } else if (macro_flags & 0x20) { // MACRO_ZERO_FREQ_FLAG
                            if (freq_table[chan]) {
                                zero_fq_table[chan] = freq_table[chan];
                                freq_table[chan] = freq_table[chan] & ~0x1fff;
                                change_freq(chan, freq_table[chan]);
                            } else if (zero_fq_table[chan]) {
                                freq_table[chan] = zero_fq_table[chan];
                                zero_fq_table[chan] = 0;
                                change_freq(chan, freq_table[chan]);
                            }
                        }

                        int16_t freq_slide = INT16LE(d->freq_slide);

                        if (!songdata->dis_fmreg_col[fmreg_ins][26]) {
                            if (freq_slide > 0) {
                                portamento_up(chan, freq_slide, nFreq(12*8+1));
                            } else if (freq_slide < 0) {
                                portamento_down(chan, abs(freq_slide), nFreq(0));
                            }
                        }
                    }
                }
            }
        }

        tARPEGGIO_TABLE *at = &songdata->macro_table[mt->arpg_table - 1].arpeggio;

        if ((mt->arpg_table != 0) && (at->speed != 0)) {
            if (mt->arpg_count == at->speed) {
                mt->arpg_count = 1;

                if (mt->arpg_pos <= at->length) {
                    if ((at->loop_begin != 0) && (at->loop_length != 0)) {
                        if (mt->arpg_pos == at->loop_begin + (at->loop_length - 1)) {
                            mt->arpg_pos = at->loop_begin;
                        } else {
                            if (mt->arpg_pos < at->length)
                                mt->arpg_pos++;
                            else
                                mt->arpg_pos = finished_flag;
                        }
                    } else {
                        if (mt->arpg_pos < at->length)
                            mt->arpg_pos++;
                        else
                            mt->arpg_pos = finished_flag;
                    }
                } else {
                    mt->arpg_pos = finished_flag;
                }

                if (((freq_table[chan] | 0x2000) == freq_table[chan]) &&
                     (at->keyoff_pos != 0) &&
                     (mt->arpg_pos >= at->keyoff_pos)) {
                    mt->arpg_pos = IDLE;
                } else {
                    if (((freq_table[chan] | 0x2000) != freq_table[chan]) &&
                         (at->keyoff_pos != 0) && (at->keyoff_pos != 0) &&
                        ((mt->arpg_pos < at->keyoff_pos) || (mt->arpg_pos == IDLE)))
                        mt->arpg_pos = at->keyoff_pos;
                }

                if ((mt->arpg_pos != 0) &&
                    (mt->arpg_pos != IDLE) && (mt->arpg_pos != finished_flag)) {
                    switch (at->data[mt->arpg_pos - 1]) {
                    case 0:
                        change_frequency(chan, nFreq(mt->arpg_note - 1) +
                            songdata->instr_data[event_table[chan].instr_def - 1].fine_tune);
                        break;

                    case 1 ... 96:
                        change_frequency(chan, nFreq(max(mt->arpg_note + at->data[mt->arpg_pos], 97) - 1) +
                            songdata->instr_data[event_table[chan].instr_def - 1].fine_tune);
                        break;

                    case 0x80 ... 0x80+12*8+1:
                        change_frequency(chan, nFreq(at->data[mt->arpg_pos - 1] - 0x80 - 1) +
                            songdata->instr_data[event_table[chan].instr_def - 1].fine_tune);
                        break;
                    }
                }
            } else {
                mt->arpg_count++;
            }
        }

        tVIBRATO_TABLE *vt = &songdata->macro_table[mt->vib_table - 1].vibrato;

        if (!mt->vib_paused && (mt->vib_table != 0) && (vt->speed != 0)) {
            if (mt->vib_count == vt->speed) {
                if (mt->vib_delay != 0) {
                    mt->vib_delay--;
                } else {
                    mt->vib_count = 1;

                    if (mt->vib_pos <= vt->length) {
                        if ((vt->loop_begin != 0) && (vt->loop_length != 0)) {
                            if (mt->vib_pos == vt->loop_begin + (vt->loop_length-1)) {
                                mt->vib_pos = vt->loop_begin;
                            } else {
                                if (mt->vib_pos < vt->length)
                                    mt->vib_pos++;
                                else
                                    mt->vib_pos = finished_flag;
                            }
                        } else {
                            if (mt->vib_pos < vt->length)
                                mt->vib_pos++;
                            else
                                mt->vib_pos = finished_flag;
                        }
                    } else {
                        mt->vib_pos = finished_flag;
                    }

                    if (((freq_table[chan] | 0x2000) == freq_table[chan]) &&
                         (vt->keyoff_pos != 0) &
                         (mt->vib_pos >= vt->keyoff_pos)) {
                        mt->vib_pos = IDLE;
                    } else {
                        if (((freq_table[chan] | 0x2000) != freq_table[chan]) &&
                             (mt->vib_pos != 0) && (vt->keyoff_pos != 0) &&
                            ((mt->vib_pos < vt->keyoff_pos) || (mt->vib_pos == IDLE)))
                            mt->vib_pos = vt->keyoff_pos;
                    }

                    if ((mt->vib_pos != 0) &&
                        (mt->vib_pos != IDLE) && (mt->vib_pos != finished_flag)) {
                        if (vt->data[mt->vib_pos - 1] > 0)
                            macro_vibrato__porta_up(chan, vt->data[mt->vib_pos]);
                    } else {
                        if (vt->data[mt->vib_pos - 1] < 0)
                            macro_vibrato__porta_down(chan, abs(vt->data[mt->vib_pos]));
                        else
                            change_freq(chan, mt->vib_freq);
                    }
                }
            } else {
                mt->vib_count++;
            }
        }
    }
}

static int ticklooper, macro_ticklooper;

static void newtimer()
{
    if ((ticklooper == 0) && (irq_mode))
        poll_proc();

    if ((macro_ticklooper == 0) && (irq_mode))
        macro_poll_proc();

    ticklooper++;
    if (ticklooper >= IRQ_freq / tempo)
        ticklooper = 0;

    macro_ticklooper++;
    if (macro_ticklooper >= IRQ_freq / (tempo * _macro_speedup()))
        macro_ticklooper = 0;
}

static void init_irq()
{
    if (irq_initialized)
        return;

    irq_initialized = TRUE;
    update_timer(50);
}

static void done_irq()
{
    if(!irq_initialized)
        return;

    irq_initialized = FALSE;
    irq_mode = TRUE;
    update_timer(0);
    irq_mode = FALSE;
}

static void init_buffers()
{
    memset(fmpar_table, 0, sizeof(fmpar_table));
    memset(pan_lock, panlock, sizeof(pan_lock));
    memset(modulator_vol, 0, sizeof(modulator_vol));
    memset(carrier_vol, 0, sizeof(carrier_vol));
    memset(event_table, 0, sizeof(event_table));
    memset(freq_table, 0, sizeof(freq_table));
    memset(zero_fq_table, 0, sizeof(zero_fq_table));
    memset(effect_table, 0, sizeof(effect_table));
    memset(fslide_table, 0, sizeof(fslide_table));
    memset(glfsld_table, 0, sizeof(glfsld_table));
    memset(porta_table, 0, sizeof(porta_table));
    memset(portaFK_table, FALSE, sizeof(portaFK_table));
    memset(arpgg_table, 0, sizeof(arpgg_table));
    memset(vibr_table, 0, sizeof(vibr_table));
    memset(trem_table, 0, sizeof(trem_table));
    memset(retrig_table, 0, sizeof(retrig_table));
    memset(tremor_table, 0, sizeof(tremor_table));
    memset(last_effect, 0, sizeof(last_effect));
    memset(voice_table, 0, sizeof(voice_table));
    memset(notedel_table, BYTE_NULL, sizeof(notedel_table));
    memset(notecut_table, BYTE_NULL, sizeof(notecut_table));
    memset(ftune_table, 0, sizeof(ftune_table));
    memset(loopbck_table, BYTE_NULL, sizeof(loopbck_table));
    memset(loop_table, BYTE_NULL, sizeof(loop_table));
    memset(reset_chan, FALSE, sizeof(reset_chan));
    memset(keyoff_loop, FALSE, sizeof(keyoff_loop));
    memset(macro_table, 0, sizeof(macro_table));

    if (!lockvol) {
        memset(volume_lock, 0, sizeof(volume_lock));
    } else {
        for (int i = 0; i < 20; i++)
              volume_lock[i] = (bool)((songdata->lock_flags[i] >> 4) & 1);
    }

    if (!panlock) {
        memset(panning_table, 0, sizeof(panning_table));
    } else {
        for (int i = 0; i < 20; i++)
              panning_table[i] = songdata->lock_flags[i] & 3;
    }

    if (!lockVP) {
        memset(peak_lock, 0, sizeof(peak_lock));
    } else {
        for (int i = 0; i < 20; i++)
              peak_lock[i] = (bool)((songdata->lock_flags[i] >> 5) & 1);
    }

    static uint8_t _4op_main_chan[6] = { 1, 3, 5, 10, 12, 14 }; // 0-based

    memset(vol4op_lock, FALSE, sizeof(vol4op_lock));
    for (int i = 0; i < 6; i++) {
        vol4op_lock[_4op_main_chan[i]] =
            ((songdata->lock_flags[_4op_main_chan[i]] | 0x40) == songdata->lock_flags[_4op_main_chan[i]]);
        vol4op_lock[_4op_main_chan[i] - 1] =
            ((songdata->lock_flags[_4op_main_chan[i] - 1] | 0x40) == songdata->lock_flags[_4op_main_chan[i] - 1]);
    }

    for (int i = 0; i < 20; i++)
        volslide_type[i] = (songdata->lock_flags[i] >> 2) & 3;
}

static void init_player()
{
    opl2out(0x01, 0);

    for (int i = 0; i < 18; i++)
        opl2out(0xb0 + _chan_n[i], 0);

    for (int i = 0x80; i <= 0x8d; i++)
        opl2out(i, BYTE_NULL);

    for (int i = 0x90; i <= 0x95; i++)
        opl2out(i, BYTE_NULL);

    misc_register = (tremolo_depth << 7) +
            (vibrato_depth << 6) +
            (percussion_mode << 5);

    opl2out(0x01, 0x20);
    opl2out(0x08, 0x40);
    opl3exp(0x0105);
    opl3exp(0x04 + (songdata->flag_4op << 8));
    printf("flag_4op: %04x\n", songdata->flag_4op);

    key_off(16);
    key_off(17);
    opl2out(_instr[11], misc_register);

    init_buffers();

    if (!percussion_mode) {
        memcpy(_chan_n, _chmm_n, sizeof(_chan_n));
        memcpy(_chan_m, _chmm_m, sizeof(_chan_m));
        memcpy(_chan_c, _chmm_c, sizeof(_chan_c));
    } else {
        memcpy(_chan_n, _chpm_n, sizeof(_chan_n));
        memcpy(_chan_m, _chpm_m, sizeof(_chan_m));
        memcpy(_chan_c, _chpm_c, sizeof(_chan_c));
    }

    current_tremolo_depth = tremolo_depth;
    current_vibrato_depth = vibrato_depth;
    global_volume = 63;

    vibtrem_speed_factor = def_vibtrem_speed_factor;
    vibtrem_table_size = def_vibtrem_table_size;
    memcpy(&vibtrem_table, &def_vibtrem_table, sizeof(vibtrem_table));

    for (int i = 0; i < 20; i++) {
        arpgg_table[0][i].state = 1;
        arpgg_table[1][i].state = 1;
        voice_table[i] = i + 1;
    }
}

void a2t_stop()
{
    if (play_status == isStopped)
        return;

    irq_mode = FALSE;
    play_status = isStopped;
    global_volume = 63;
    current_tremolo_depth = tremolo_depth;
    current_vibrato_depth = vibrato_depth;
    pattern_break = FALSE;
    current_order = 0;
    current_pattern = 0;
    current_line = 0;
    playback_speed_shift = 0;

    for (int i = 0; i < 20; i++)
        release_sustaining_sound(i);

    opl2out(_instr[11], 0);
    opl3exp(0x0004);
    opl3exp(0x0005);
    lockvol = FALSE;
    panlock = FALSE;
    lockVP = FALSE;
    init_buffers();

    speed = 4;
    update_timer(50);
}

/* Clean songdata before importing a2t tune */
static void init_songdata()
{
    if (play_status != isStopped)
        a2t_stop();

    memset(songdata, 0, sizeof(_songdata));
    memset(songdata->pattern_order, 0x80, sizeof(songdata->pattern_order));

    IRQ_freq_shift = 0;
    playback_speed_shift = 0;
    songdata->patt_len = 64;
    songdata->nm_tracks = 18;
    songdata->tempo = tempo;
    songdata->speed = speed;
    songdata->macro_speedup = 1;
    speed_update = FALSE;
    lockvol = FALSE;
    panlock = FALSE;
    lockVP  = FALSE;
    tremolo_depth = 0;
    vibrato_depth = 0;
    volume_scaling = FALSE;
    percussion_mode = FALSE;
}

static int a2_import(char *tune); // forward def

void a2t_play(char *tune) // start_playing()
{
    a2t_stop();
    int err = a2_import(tune);

    if (err)
        return;

    init_player();

    if ((songdata->pattern_order[current_order] > 0x7f) &&
        (calc_order_jump() == -1))
        return;

    current_pattern = songdata->pattern_order[current_order];
    current_line = 0;
    pattern_break = FALSE;
    pattern_delay = FALSE;
    tickXF = 0;
    ticks = 0;
    next_line = 0;
    irq_mode = TRUE;
    play_status = isPlaying;

    time_playing = 0;
    ticklooper = 0;
    macro_ticklooper = 0;
    speed = songdata->speed;
    macro_speedup = songdata->macro_speedup;
    update_timer(songdata->tempo);
}

/* LOADER FOR A2M/A2T */

int ffver = 1;
int len[21];

typedef struct {
    char id[15];	// '_a2tiny_module_'
    uint8_t crc[4]; // uint32_t
    uint8_t ffver;
    uint8_t npatt;
    uint8_t tempo;
    uint8_t speed;
} A2T_HEADER;

C_ASSERT(sizeof(A2T_HEADER) == 23);

typedef struct {
    char id[10];	// '_a2module_'
    uint8_t crc[4]; // uint32_t
    uint8_t ffver;
    uint8_t npatt;
} A2M_HEADER;

C_ASSERT(sizeof(A2M_HEADER) == 16);

char *a2t_load(char *name)
{
    FILE *fh;
    int fsize;
    void *p;

    fh = fopen(name, "rb");
    if(!fh) return NULL;

    fseek(fh, 0, SEEK_END);
    fsize = ftell(fh) ;
    fseek(fh, 0, SEEK_SET);
    p = (void *)malloc(fsize);
    fread(p, 1, fsize, fh);
    fclose(fh);

    return p;
}

static inline void a2t_depack(void *src, int srcsize, void *dst)
{
    switch (ffver) {
    case 1:
    case 5:		// sixpack
        sixdepak(src, dst, srcsize);
        break;
    case 2:
    case 6:		// FIXME: lzw
        break;
    case 3:
    case 7:		// FIXME: lzss
        break;
    case 4:
    case 8:		// unpacked
        memcpy(dst, src, srcsize);
        break;
    case 9 ... 11:	// apack (aPlib)
        aP_depack(src, dst);
        break;
    case 12 ... 14: // lzh
        LZH_decompress(src, dst, srcsize);
        break;
    }
}

/* Data for importing A2T format */
typedef struct {
    uint8_t len[6][2]; // uint16_t
} A2T_VARHEADER_V1234;

typedef struct {
    uint8_t common_flag;
    uint8_t len[10][2]; // uint16_t
} A2T_VARHEADER_V5678;

typedef struct {
    uint8_t common_flag;
    uint8_t patt_len[2]; // uint16_t
    uint8_t nm_tracks;
    uint8_t macro_speedup[2]; // uint16_t
    uint8_t len[20][4]; // uint32_t
} A2T_VARHEADER_V9;

typedef struct {
    uint8_t common_flag;
    uint8_t patt_len[2]; // uint16_t
    uint8_t nm_tracks;
    uint8_t macro_speedup[2]; // uint16_t
    uint8_t flag_4op;
    uint8_t lock_flags[20];
    uint8_t len[20][4]; // uint32_t
} A2T_VARHEADER_V10;

typedef struct {
    uint8_t common_flag;
    uint8_t patt_len[2]; // uint16_t
    uint8_t nm_tracks;
    uint8_t macro_speedup[2]; // uint16_t
    uint8_t flag_4op;
    uint8_t lock_flags[20];
    uint8_t len[21][4]; // uint32_t
} A2T_VARHEADER_V11;

typedef union {
    A2T_VARHEADER_V1234 v1234;
    A2T_VARHEADER_V5678 v5678;
    A2T_VARHEADER_V9    v9;
    A2T_VARHEADER_V10   v10;
    A2T_VARHEADER_V11   v11;
} A2T_VARHEADER;

C_ASSERT(sizeof(A2T_VARHEADER_V1234) == 12);
C_ASSERT(sizeof(A2T_VARHEADER_V5678) == 21);
C_ASSERT(sizeof(A2T_VARHEADER_V9) == 86);
C_ASSERT(sizeof(A2T_VARHEADER_V10) == 107);
C_ASSERT(sizeof(A2T_VARHEADER_V11) == 111);
C_ASSERT(sizeof(A2T_VARHEADER) == 111);

// read the variable part of the header
static int a2t_read_varheader(char *blockptr)
{
    A2T_VARHEADER *varheader = (A2T_VARHEADER *)blockptr;

    switch (ffver) {
    case 1 ... 4:
        for (int i = 0; i < 6; i++)
            len[i] = UINT16LE(varheader->v1234.len[i]);
        return sizeof(A2T_VARHEADER_V1234);
    case 5 ... 8:
        songdata->common_flag = varheader->v5678.common_flag;
        for (int i = 0; i < 10; i++)
            len[i] = UINT16LE(varheader->v5678.len[i]);
        return sizeof(A2T_VARHEADER_V5678);
    case 9:
        songdata->common_flag = varheader->v9.common_flag;
        songdata->patt_len = UINT16LE(varheader->v9.patt_len);
        songdata->nm_tracks = varheader->v9.nm_tracks;
        songdata->macro_speedup = UINT16LE(varheader->v9.macro_speedup);
        for (int i = 0; i < 20; i++)
            len[i] = UINT32LE(varheader->v9.len[i]);
        return sizeof(A2T_VARHEADER_V9);
    case 10:
        songdata->common_flag = varheader->v10.common_flag;
        songdata->patt_len = UINT16LE(varheader->v10.patt_len);
        songdata->nm_tracks = varheader->v10.nm_tracks;
        songdata->macro_speedup = UINT16LE(varheader->v10.macro_speedup);
        songdata->flag_4op = varheader->v10.flag_4op;
        for (int i = 0; i < 20; i++)
            songdata->lock_flags[i] = varheader->v10.lock_flags[i];
        for (int i = 0; i < 20; i++)
            len[i] = UINT32LE(varheader->v10.len[i]);
        return sizeof(A2T_VARHEADER_V10);
    case 11 ... 14:
        songdata->common_flag = varheader->v11.common_flag;
        songdata->patt_len = UINT16LE(varheader->v11.patt_len);
        songdata->nm_tracks = varheader->v11.nm_tracks;
        songdata->macro_speedup = UINT16LE(varheader->v11.macro_speedup);
        songdata->flag_4op = varheader->v11.flag_4op;
        for (int i = 0; i < 20; i++)
            songdata->lock_flags[i] = varheader->v10.lock_flags[i];
        for (int i = 0; i < 21; i++)
            len[i] = UINT32LE(varheader->v11.len[i]);
        return sizeof(A2T_VARHEADER_V11);
    }

    return 0;
}

static int a2t_read_instruments(char *src)
{
    int instsize = (ffver < 9 ? 13 : 14);
    int dstsize = (ffver < 9 ? 250 * 13 : 255 * 14) +
                  (ffver > 11 ? 129 + 1024 + 3: 0);
    char *dst = (char *)malloc(dstsize);
    memset(dst, 0, dstsize);

    a2t_depack(src, len[0], dst);

    if (ffver == 14) {
        memcpy(&songdata->bpm_data, dst, sizeof(songdata->bpm_data));
        dst += sizeof(songdata->bpm_data);
    }

    if (ffver >= 12 && ffver <= 14) {
        memcpy(&songdata->ins_4op_flags, dst, sizeof(songdata->ins_4op_flags));
        dst += sizeof(songdata->ins_4op_flags);
        memcpy(&songdata->reserved_data, dst, sizeof(songdata->reserved_data));
        dst += sizeof(songdata->reserved_data);
    }

    for (int i = 0; i < (ffver < 9 ? 250 : 255); i++) {
        memcpy(&songdata->instr_data[i], dst + i * instsize, instsize);
    }

#if 0
    FILE *f = fopen("0_inst.dmp", "wb");
    fwrite(songdata->instr_data, 1, sizeof(songdata->instr_data), f);
    fclose(f);
#endif

    free(dst);

    return len[0];
}

static int a2t_read_instmacros(char *src)
{
    if (ffver < 9) return 0;

    a2t_depack(src, len[1], songdata->instr_macros);

#if 0
    FILE *f = fopen("1_inst_macro.dmp", "wb");
    fwrite(songdata->instr_macros, 1, sizeof(songdata->instr_macros), f);
    fclose(f);
#endif

    return len[1];
}

static int a2t_read_macrotable(char *src)
{
    if (ffver < 9) return 0;

    a2t_depack(src, len[2], songdata->macro_table);

#if 0
    FILE *f = fopen("2_macrotable.dmp", "wb");
    fwrite(songdata->macro_table, 1, sizeof(songdata->macro_table), f);
    fclose(f);
#endif

    return len[2];
}

static int a2t_read_disabled_fmregs(char *src)
{
    if (ffver < 11) return 0;

    a2t_depack(src, len[3], songdata->dis_fmreg_col);

#if 0
    FILE *f = fopen("3_fm_disregs.dmp", "wb");
    fwrite(songdata->dis_fmreg_col, 1, sizeof(songdata->dis_fmreg_col), f);
    fclose(f);
#endif

    return len[3];
}

static int a2t_read_order(char *src)
{
    int blocknum[14] = {1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 4, 4, 4, 4};
    int i = blocknum[ffver - 1];

    a2t_depack(src, len[i], songdata->pattern_order);

#if 0
    FILE *f = fopen("4_order.dmp", "wb");
    fwrite(songdata->pattern_order, 1, sizeof(songdata->pattern_order), f);
    fclose(f);
#endif

    return len[i];
}

// only for importing v 1,2,3,4,5,6,7,8
typedef struct {
    uint8_t note;
    uint8_t instr_def;
    uint8_t effect_def;
    uint8_t effect;
} tADTRACK2_EVENT_V1234;

// for importing v 1,2,3,4 patterns
typedef struct {
    struct {
        struct {
            tADTRACK2_EVENT_V1234 ev;
        } ch[9];
    } row[64];
} tPATTERN_DATA_V1234;

// for importing v 5,6,7,8 patterns
typedef struct {
    struct {
        struct {
            tADTRACK2_EVENT_V1234 ev;
        } row[64];
    } ch[18];
} tPATTERN_DATA_V5678;

C_ASSERT(sizeof(tADTRACK2_EVENT_V1234) == 4);
C_ASSERT(sizeof(tPATTERN_DATA_V1234) == 2304);
C_ASSERT(sizeof(tPATTERN_DATA_V5678) == 4608);

// Old v1234 effects
enum {
    fx_Arpeggio          = 0x00,
    fx_FSlideUp          = 0x01,
    fx_FSlideDown        = 0x02,
    fx_FSlideUpFine      = 0x03,
    fx_FSlideDownFine    = 0x04,
    fx_TonePortamento    = 0x05,
    fx_TPortamVolSlide   = 0x06,
    fx_Vibrato           = 0x07,
    fx_VibratoVolSlide   = 0x08,
    fx_SetOpIntensity    = 0x09,
    fx_SetInsVolume      = 0x0a,
    fx_PatternBreak      = 0x0b,
    fx_PatternJump       = 0x0c,
    fx_SetTempo          = 0x0d,
    fx_SetTimer          = 0x0e,
    fx_Extended          = 0x0f,
    fx_ex_DefAMdepth     = 0x00,
    fx_ex_DefVibDepth    = 0x01,
    fx_ex_DefWaveform    = 0x02,
    fx_ex_ManSlideUp     = 0x03,
    fx_ex_ManSlideDown   = 0x04,
    fx_ex_VSlideUp       = 0x05,
    fx_ex_VSlideDown     = 0x06,
    fx_ex_VSlideUpFine   = 0x07,
    fx_ex_VSlideDownFine = 0x08,
    fx_ex_RetrigNote     = 0x09,
    fx_ex_SetAttckRate   = 0x0a,
    fx_ex_SetDecayRate   = 0x0b,
    fx_ex_SetSustnLevel  = 0x0c,
    fx_ex_SetReleaseRate = 0x0d,
    fx_ex_SetFeedback    = 0x0e,
    fx_ex_ExtendedCmd    = 0x0f
};

// For importing from a2m v1234
bool adsr_carrier[9];

void convert_v1234_event(tADTRACK2_EVENT_V1234 *ev, int chan)
{
    switch (ev->effect_def) {
    case fx_Arpeggio:           ev->effect_def = ef_Arpeggio;        break;
    case fx_FSlideUp:           ev->effect_def = ef_FSlideUp;        break;
    case fx_FSlideDown:         ev->effect_def = ef_FSlideDown;      break;
    case fx_FSlideUpFine:       ev->effect_def = ef_FSlideUpFine;    break;
    case fx_FSlideDownFine:     ev->effect_def = ef_FSlideDownFine;  break;
    case fx_TonePortamento:     ev->effect_def = ef_TonePortamento;  break;
    case fx_TPortamVolSlide:    ev->effect_def = ef_TPortamVolSlide; break;
    case fx_Vibrato:            ev->effect_def = ef_Vibrato;         break;
    case fx_VibratoVolSlide:    ev->effect_def = ef_VibratoVolSlide; break;
    case fx_SetInsVolume:       ev->effect_def = ef_SetInsVolume;    break;
    case fx_PatternJump:        ev->effect_def = ef_PositionJump;    break;
    case fx_PatternBreak:       ev->effect_def = ef_PatternBreak;    break;
    case fx_SetTempo:           ev->effect_def = ef_SetSpeed;        break;
    case fx_SetTimer:           ev->effect_def = ef_SetTempo;        break;
    case fx_SetOpIntensity: {
        if (ev->effect & 0xf0) {
            ev->effect_def = ef_SetCarrierVol;
            ev->effect = (ev->effect >> 4) * 4 + 3;
        } else if (ev->effect & 0x0f) {
            ev->effect_def = ef_SetModulatorVol;
            ev->effect = (ev->effect & 0x0f) * 4 + 3;
        } else ev->effect_def = 0;
        break;
    }
    case fx_Extended: {
        switch (ev->effect >> 4) {
        case fx_ex_DefAMdepth:
            ev->effect_def = ef_Extended;
            ev->effect = ef_ex_SetTremDepth << 4 | (ev->effect & 0x0f);
            break;
        case fx_ex_DefVibDepth:
            ev->effect_def = ef_Extended;
            ev->effect = ef_ex_SetVibDepth << 4 | (ev->effect & 0x0f);
            break;
        case fx_ex_DefWaveform:
            ev->effect_def = ef_SetWaveform;
            if ((ev->effect & 0x0f) < 4) {
                ev->effect = ((ev->effect & 0x0f) << 4) | 0x0f; // 0..3
            } else {
                ev->effect = ((ev->effect & 0x0f) - 4) | 0xf0; // 4..7
            }
            break;
        case fx_ex_VSlideUp:
            ev->effect_def = ef_VolSlide;
            ev->effect = (ev->effect & 0x0f) << 4;
            break;
        case fx_ex_VSlideDown:
            ev->effect_def = ef_VolSlide;
            ev->effect = ev->effect & 0x0f;
            break;
        case fx_ex_VSlideUpFine:
            ev->effect_def = ef_VolSlideFine;
            ev->effect = (ev->effect & 0x0f) << 4;
            break;
        case fx_ex_VSlideDownFine:
            ev->effect_def = ef_VolSlideFine;
            ev->effect = ev->effect & 0x0f;
            break;
        case fx_ex_ManSlideUp:
            ev->effect_def = ef_Extended2;
            ev->effect = (ef_ex2_FineTuneUp << 4) | (ev->effect & 0x0f);
            break;
        case fx_ex_ManSlideDown:
            ev->effect_def = ef_Extended2;
            ev->effect = (ef_ex2_FineTuneDown << 4) | (ev->effect & 0x0f);
            break;
        case fx_ex_RetrigNote:
            ev->effect_def = ef_RetrigNote;
            ev->effect = (ev->effect & 0x0f) + 1;
            break;
        case fx_ex_SetAttckRate:
            ev->effect_def = ef_Extended;
            ev->effect = ev->effect & 0x0f;
            if (!adsr_carrier[chan]) {
                ev->effect |= ef_ex_SetAttckRateM << 4;
            } else {
                ev->effect |= ef_ex_SetAttckRateC << 4;
            }
            break;
        case fx_ex_SetDecayRate:
            ev->effect_def = ef_Extended;
            ev->effect = ev->effect & 0x0f;
            if (!adsr_carrier[chan]) {
                ev->effect |= ef_ex_SetDecayRateM << 4;
            } else {
                ev->effect |= ef_ex_SetDecayRateC << 4;
            }
            break;
        case fx_ex_SetSustnLevel:
            ev->effect_def = ef_Extended;
            ev->effect = ev->effect & 0x0f;
            if (!adsr_carrier[chan]) {
                ev->effect |= ef_ex_SetSustnLevelM << 4;
            } else {
                ev->effect |= ef_ex_SetSustnLevelC << 4;
            }
            break;
        case fx_ex_SetReleaseRate:
            ev->effect_def = ef_Extended;
            ev->effect = ev->effect & 0x0f;
            if (!adsr_carrier[chan]) {
                ev->effect |= ef_ex_SetRelRateM << 4;
            } else {
                ev->effect |= ef_ex_SetRelRateC << 4;
            }
            break;
        case fx_ex_SetFeedback:
            ev->effect_def = ef_Extended;
            ev->effect = (ef_ex_SetFeedback << 4) | (ev->effect & 0x0f);
            break;
        case fx_ex_ExtendedCmd:
            ev->effect_def = ef_Extended;
            ev->effect = ef_ex_ExtendedCmd2 << 4;
            if ((ev->effect & 0x0f) < 10) {
                // FIXME: Should be a parameter
                bool whole_song = FALSE;

                switch (ev->effect & 0x0f) {
                case 0: ev->effect |= ef_ex_cmd2_RSS;       break;
                case 1: ev->effect |= ef_ex_cmd2_LockVol;   break;
                case 2: ev->effect |= ef_ex_cmd2_UnlockVol; break;
                case 3: ev->effect |= ef_ex_cmd2_LockVP;    break;
                case 4: ev->effect |= ef_ex_cmd2_UnlockVP;  break;
                case 5:
                    ev->effect_def = (whole_song ? 255 : 0);
                    ev->effect = 0;
                    adsr_carrier[chan] = TRUE;
                    break;
                case 6:
                    ev->effect_def = (whole_song ? 255 : 0);
                    ev->effect = (whole_song ? 1 : 0);
                    adsr_carrier[chan] = FALSE;
                    break;
                case 7: ev->effect |= ef_ex_cmd2_VSlide_car; break;
                case 8: ev->effect |= ef_ex_cmd2_VSlide_mod; break;
                case 9: ev->effect |= ef_ex_cmd2_VSlide_def; break;
                }
            } else {
                ev->effect_def = 0;
                ev->effect = 0;
            }
            break;
        }
        break;
    }
    default:
        ev->effect_def = 0;
        ev->effect = 0;
    }
}

// common for both a2t/a2m
static int a2_read_patterns(char *src, int s)
{
    switch (ffver) {
    case 1 ... 4:	// [4][16][64][9][4]
        {
        tPATTERN_DATA_V1234 *old =
            (tPATTERN_DATA_V1234 *)malloc(sizeof(*old) * 16);

        memset(adsr_carrier, FALSE, sizeof(adsr_carrier));

        for (int i = 0; i < 4; i++) {
            if (!len[i+s]) continue;

            a2t_depack(src, len[i+s], old);

            for (int p = 0; p < 16; p++) { // pattern
                if (i * 8 + p >= _patterns_allocated)
                        break;
                for (int r = 0; r < 64; r++) // row
                for (int c = 0; c < 9; c++) { // channel
                    convert_v1234_event(&old[p].row[r].ch[c].ev, c);
                    copy_to_event(i * 16 + p, c, r, &old[p].row[r].ch[c].ev);
                }
            }

            src += len[i+s];
        }

        free(old);
        break;
        }
    case 5 ... 8:	// [8][8][18][64][4]
        {
        tPATTERN_DATA_V5678 *old =
            (tPATTERN_DATA_V5678 *)malloc(sizeof(*old) * 8);

        for (int i = 0; i < 8; i++) {
            if (!len[i+s]) continue;

            a2t_depack(src, len[i+s], old);

            for (int p = 0; p < 8; p++) { // pattern
                if (i * 8 + p >= _patterns_allocated)
                    break;
                for (int c = 0; c < 18; c++) // channel
                for (int r = 0; r < 64; r++) { // row
                    // pattern_event_copy?
                    copy_to_event(i * 8 + p, c, r, &old[p].ch[c].row[r].ev);
                }
            }

            src += len[i+s];
        }

        free(old);
        break;
        }
    case 9 ... 14:	// [16][8][20][256][6]
        {
        tPATTERN_DATA *old = (tPATTERN_DATA *)calloc(sizeof(*old) * 8, 1);

        // 16 groups of 8 patterns
        for (int i = 0; i < 16; i++) {
            if (!len[i+s]) continue;
            a2t_depack(src, len[i+s], old);
            src += len[i+s];

            for (int p = 0; p < 8; p++) { // pattern
                if (i * 8 + p >= _patterns_allocated)
                        break;

                copy_to_pattern(i * 8 + p, old + p);
            }
        }

        free(old);
        break;
        }
    }

    return 0;
}

static int a2t_read_patterns(char *src)
{
    int blockstart[14] = {2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 5, 5, 5, 5};
    int s = blockstart[ffver - 1];

    a2_read_patterns(src, s);

    return 0;
}

static void a2t_import(char *tune)
{
    A2T_HEADER *header = (A2T_HEADER *)tune;
    char *blockptr = tune + sizeof(A2T_HEADER);

    if(strncmp(header->id, "_A2tiny_module_", 15))
        return;

    patterns_alloc(header->npatt);

    init_songdata();

    memset(len, 0, sizeof(len));

    ffver = header->ffver;
    songdata->tempo = header->tempo;
    songdata->speed = header->speed;
    songdata->patt_len = 64;
    songdata->nm_tracks = 18;
    songdata->macro_speedup = 1;

    // Read variable part after header, fill len[] with values
    blockptr += a2t_read_varheader(blockptr);

    speed_update    = (songdata->common_flag >> 0) & 1;
    lockvol         = (songdata->common_flag >> 1) & 1;
    lockVP          = (songdata->common_flag >> 2) & 1;
    tremolo_depth   = (songdata->common_flag >> 3) & 1;
    vibrato_depth   = (songdata->common_flag >> 4) & 1;
    panlock         = (songdata->common_flag >> 5) & 1;
    percussion_mode = (songdata->common_flag >> 6) & 1;
    volume_scaling  = (songdata->common_flag >> 7) & 1;

    // Read instruments; all versions
    blockptr += a2t_read_instruments(blockptr);

    // Read instrument macro (v >= 9,10,11)
    blockptr += a2t_read_instmacros(blockptr);

    // Read arpeggio/vibrato macro table (v >= 9,10,11)
    blockptr += a2t_read_macrotable(blockptr);

    // Read disabled fm regs (v == 11)
    blockptr += a2t_read_disabled_fmregs(blockptr);

    // Read pattern_order
    blockptr += a2t_read_order(blockptr);

    // Read patterns
    a2t_read_patterns(blockptr);

    printf("A2T version: %d\n", header->ffver);
    printf("Number of patterns: %d\n", header->npatt);
    printf("Rows per pattern: %d\n", songdata->patt_len);
    printf("Tempo: %d\n", header->tempo);
    printf("Speed: %d\n", header->speed);
    printf("Volume scaling: %d\n", volume_scaling);
    printf("Percussion mode: %d\n", percussion_mode);
}

typedef uint8_t (tUINT16)[2];
typedef uint8_t (tUINT32)[4];

static int a2m_read_varheader(char *blockptr, int npatt)
{
    int lensize;
    int maxblock = (ffver < 5 ? npatt / 16 : npatt / 8) + 1;

    tUINT16 *src16 = (tUINT16 *)blockptr;
    tUINT32 *src32 = (tUINT32 *)blockptr;

    if (ffver < 5) lensize = 5;         // 1,2,3,4 - uint16_t len[5];
    else if (ffver < 9) lensize = 9;    // 5,6,7,8 - uint16_t len[9];
    else lensize = 17;                  // 9,10,11 - uint32_t len[17];

    switch (ffver) {
    case 1 ... 8:
        // skip possible rubbish (MARIO.A2M)
        for (int i = 0; (i < lensize) && (i <= maxblock); i++)
            len[i] = UINT16LE(src16[i]);

        return lensize * sizeof(tUINT16);
    case 9 ... 14:
        for (int i = 0; i < lensize; i++)
            len[i] = UINT32LE(src32[i]);

        return lensize * sizeof(tUINT32);
    }

    return 0;
}

/* Data for importing A2M format */
typedef struct {
    char songname[43];
    char composer[43];
    char instr_names[250][33];
    uint8_t instr_data[250][13];
    uint8_t pattern_order[128];
    uint8_t tempo;
    uint8_t speed;
    uint8_t common_flag; // A2M_SONGDATA_V5678
} A2M_SONGDATA_V1234;

typedef struct {
    char songname[43];
    char composer[43];
    char instr_names[255][43];
    // TODO: type 3 arrays below for correct loading
    uint8_t instr_data[255][14];
    uint8_t instr_macros[255][3831];
    uint8_t macro_table[255][521];
    uint8_t pattern_order[128];
    uint8_t tempo;
    uint8_t speed;
    uint8_t common_flag;
    uint8_t patt_len[2];           // uint16_t
    uint8_t nm_tracks;
    uint8_t macro_speedup[2];      // uint16_t
    uint8_t flag_4op;              // A2M_SONGDATA_V10
    uint8_t lock_flags[20];        // A2M_SONGDATA_V10
    char pattern_names[128][43];   // A2M_SONGDATA_V11
    int8_t dis_fmreg_col[255][28]; // A2M_SONGDATA_V11
    struct {
        uint8_t num_4op;
        uint8_t idx_4op[128];
    } ins_4op_flags;             // A2M_SONGDATA_V12_13
    uint8_t reserved_data[1024]; // A2M_SONGDATA_V12_13
    struct {
        uint8_t rows_per_beat;
        int8_t tempo_finetune[2]; // int16_t
    } bpm_data;                   // A2M_SONGDATA_V14
} A2M_SONGDATA_V9_14;

C_ASSERT(sizeof(A2M_SONGDATA_V1234) == 11717);
C_ASSERT(sizeof(A2M_SONGDATA_V9_14) == 1138338);

static int a2m_read_songdata(char *src)
{
    if (ffver < 9) {		// 1,2,3,4,5,6,7,8
        A2M_SONGDATA_V1234 *data = malloc(sizeof(*data));
        a2t_depack(src, len[0], data);

        memcpy(songdata->songname, data->songname, 43);
        memcpy(songdata->composer, data->composer, 43);

        for (int i = 0; i < 250; i++) {
            memcpy(songdata->instr_names[i], data->instr_names[i], 33);
            memcpy(&songdata->instr_data[i], data->instr_data[i], 13);
        }

        memcpy(songdata->pattern_order, data->pattern_order, 128);

        songdata->tempo = data->tempo;
        songdata->speed = data->speed;

        if (ffver > 4) { // 5,6,7,8
            songdata->common_flag = data->common_flag;
        }

        free(data);
    } else {			// 9 - 14
        A2M_SONGDATA_V9_14 *data = malloc(sizeof(*data));
        a2t_depack(src, len[0], data);

        memcpy(songdata->songname, data->songname, 43);
        memcpy(songdata->composer, data->composer, 43);

        for (int i = 0; i < 255; i++) {
            memcpy(songdata->instr_names[i], data->instr_names[i], 43);
            memcpy(&songdata->instr_data[i], data->instr_data[i], 14);
        }

        memcpy(songdata->instr_macros, data->instr_macros, 255 * 3831);
        memcpy(songdata->macro_table, data->macro_table, 255 * 521);
        memcpy(songdata->pattern_order, data->pattern_order, 128);

        songdata->tempo = data->tempo;
        songdata->speed = data->speed;
        songdata->common_flag = data->common_flag;
        songdata->patt_len = UINT16LE(data->patt_len);
        songdata->nm_tracks = data->nm_tracks;
        songdata->macro_speedup = UINT16LE(data->macro_speedup);

        // v10
        songdata->flag_4op = data->flag_4op;
        memcpy(songdata->lock_flags, data->lock_flags, sizeof(data->lock_flags));

        // v11
        memcpy(songdata->pattern_names, data->pattern_names, 128 * 43);
        memcpy(songdata->dis_fmreg_col, data->dis_fmreg_col, 255 * 28);

        // v12-13
        songdata->ins_4op_flags.num_4op = data->ins_4op_flags.num_4op;
        memcpy(songdata->ins_4op_flags.idx_4op, data->ins_4op_flags.idx_4op, 128);
        memcpy(songdata->reserved_data, data->reserved_data, 1024);

        // v14
        songdata->bpm_data.rows_per_beat = data->bpm_data.rows_per_beat;
        songdata->bpm_data.tempo_finetune = // Note: not used anywhere
            INT16LE(data->bpm_data.tempo_finetune);

        free(data);
    }

    speed_update    = (songdata->common_flag >> 0) & 1;
    lockvol         = (songdata->common_flag >> 1) & 1;
    lockVP          = (songdata->common_flag >> 2) & 1;
    tremolo_depth   = (songdata->common_flag >> 3) & 1;
    vibrato_depth   = (songdata->common_flag >> 4) & 1;
    panlock         = (songdata->common_flag >> 5) & 1;
    percussion_mode = (songdata->common_flag >> 6) & 1;
    volume_scaling  = (songdata->common_flag >> 7) & 1;

#if 0
    FILE *f = fopen("songdata.dmp", "wb");
    fwrite(songdata, 1, sizeof(*songdata), f);
    fclose(f);
#endif

    return len[0];
}

static int a2m_read_patterns(char *src)
{
    a2_read_patterns(src, 1);

    return 0;
}

static void a2m_import(char *tune)
{
    A2M_HEADER *header = (A2M_HEADER *)tune;
    char *blockptr = tune + sizeof(A2M_HEADER);

    if(strncmp(header->id, "_A2module_", 10))
        return;

    memset(songdata, 0, sizeof(_songdata));

    patterns_alloc(header->npatt);

    memset(len, 0, sizeof(len));

    ffver = header->ffver;
    songdata->patt_len = 64;
    songdata->nm_tracks = 18;
    songdata->macro_speedup = 1;

    // Read variable part after header, fill len[] with values
    blockptr += a2m_read_varheader(blockptr, header->npatt);

    // Read songdata
    blockptr += a2m_read_songdata(blockptr);

    // Read patterns
    a2m_read_patterns(blockptr);

    printf("A2M version: %d\n", header->ffver);
    printf("Number of patterns: %d\n", header->npatt);
    printf("Rows per pattern: %d\n", songdata->patt_len);
    printf("Tempo: %d\n", songdata->tempo);
    printf("Speed: %d\n", songdata->speed);
    printf("Volume scaling: %d\n", volume_scaling);
    printf("Percussion mode: %d\n", percussion_mode);
    printf("Track volume lock: %d\n", lockvol);
}

static int a2_import(char *tune)
{
    if(!strncmp(tune, "_A2module_", 10)) {
        a2m_import(tune);
        return 0;
    }

    if(!strncmp(tune, "_A2tiny_module_", 15)) {
        a2t_import(tune);
        return 0;
    }

    return -1;
}

static int freqhz = 44100;
static int framesmpl = 44100 / 50;
static int irq_freq = 50;
static opl3_chip opl;

static void opl_out(uint8_t port, uint8_t val)
{
    static uint8_t reg = 0;
    if ((port & 1) == 0) {
        reg = val;
    } else {
        OPL3_WriteRegBuffered(&opl, ((port / 2) << 8) | reg, val);
    }
}

void a2t_init(int freq)
{
    freqhz = freq;
    framesmpl = freq / 50;

    OPL3_Reset(&opl, freqhz);
}

void a2t_shut()
{
    patterns_free();
    OPL3_Reset(&opl, freqhz);
}

// 'len' is the buffer length in bytes, not samples!
void a2t_update(unsigned char *stream, int len)
{
    static int ticklooper, macro_ticklooper;
    static int cnt = 0;

    if (play_status != isPlaying) {
        memset(stream, 0, len);
        return;
    }

    for (int cntr = 0; cntr < len; cntr += 4) {
        if (cnt >= framesmpl) {
            cnt = 0;
            if (ticklooper == 0) {
                poll_proc();
                if (irq_freq != tempo * macro_speedup) {
                    irq_freq = (tempo < 18 ? 18 : tempo) * macro_speedup;
                    framesmpl = freqhz / irq_freq;
                }
            }

            if (macro_ticklooper == 0)
                macro_poll_proc();

            ticklooper++;
            if (ticklooper >= irq_freq / tempo)
                ticklooper = 0;

            macro_ticklooper++;
            if (macro_ticklooper >= irq_freq / (tempo * macro_speedup))
                macro_ticklooper = 0;
        }

        // this writes 4 bytes, i.e. one 16-bit stereo sample
        OPL3_GenerateStream(&opl, (int16_t *)(stream + cntr), 1);
        cnt++;
    }
}
