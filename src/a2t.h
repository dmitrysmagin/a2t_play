#ifndef _A2T_H_
#define _A2T_H_

#include <stdint.h>
#include <stdbool.h>

void a2t_init(int);
void a2t_shut();
char *a2t_load(char *name);
bool a2t_play(char *tune);
void a2t_update(unsigned char *, int);
void set_overall_volume(unsigned char level);
void a2t_stop();

extern uint8_t current_order;
extern uint8_t current_pattern;
extern uint8_t current_line;

#ifndef STATIC_ASSERT
#define STATIC_ASSERT(e) typedef char __STATIC_ASSERT__[(e) ? 1 : -1]
#endif

#ifndef bool
typedef signed char bool;
#endif

#ifndef false
#define false 0
#endif
#ifndef true
#define true 1
#endif

#define keyoff_flag         0x80
#define fixed_note_flag     0x90
#define pattern_loop_flag   0xe0
#define pattern_break_flag  0xf0

typedef enum {
    isPlaying = 0, isPaused, isStopped
} tPLAY_STATUS;

#define BYTE_NULL (uint8_t)(0xFFFFFFFF)

#define MIN_IRQ_FREQ        50
#define MAX_IRQ_FREQ        1000

/*
    When loading A2T/A2M, FreePascal structures (no padding and little-endian) should be emulated,
    because AdlibTracker 2 was saving structures directly from memory into the file.

    That's why:
    1) only chars are used in structs to avoid any padding or alignment (default C/C++ behaviour)
    2) ints and longs are represented as arrays of chars, little-endian order is implied
    3) STATIC_ASSERT is used to make sure structs have the correct size
*/

// Make sure compiler doesn't pad byte arrays
STATIC_ASSERT(sizeof(uint8_t[2]) == 2);
STATIC_ASSERT(sizeof(uint8_t[4]) == 4);

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
#define ef_GlobalFreqSlideUpXF 48 // ef_fix2 replacement for >xx + ZFE
#define ef_GlobalFreqSlideDnXF 49 // ef_fix2 replacement for <xx + ZFE

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

#define EFGR_ARPVOLSLIDE 1
#define EFGR_FSLIDEVOLSLIDE 2
#define EFGR_TONEPORTAMENTO 3
#define EFGR_VIBRATO 4
#define EFGR_TREMOLO 5
#define EFGR_VIBRATOVOLSLIDE 6
#define EFGR_PORTAVOLSLIDE 7
#define EFGR_RETRIGNOTE 8

// Macros for extracting little-endian integers from filedata
#define INT8LE(A)       (int8_t)((A)[0])
#define UINT8LE(A)      (uint8_t)((A)[0])
#define INT16LE(A)      (int16_t)(((A)[0]) | ((A)[1] << 8))
#define UINT16LE(A)     (uint16_t)(((A)[0]) | ((A)[1] << 8))
#define INT32LE(A)      (int32_t)(((A)[0]) | ((A)[1] << 8) | ((A)[2] << 16) | ((A)[3] << 24))
#define UINT32LE(A)     (uint32_t)(((A)[0]) | ((A)[1] << 8) | ((A)[2] << 16) | ((A)[3] << 24))

/* Structures for importing A2T format (all versions) */
#define A2T_HEADER_FFVER(P)                 UINT8LE((P)+19)
#define A2T_HEADER_NPATT(P)                 UINT8LE((P)+20)
#define A2T_HEADER_TEMPO(P)                 UINT8LE((P)+21)
#define A2T_HEADER_SPEED(P)                 UINT8LE((P)+22)
#define A2T_HEADER_SIZE                     (23)

#define A2T_VARHEADER_V1234_LEN(P, I)       UINT16LE((P)+0+I*2)
#define A2T_VARHEADER_V1234_SIZE            (12)

#define A2T_VARHEADER_V5678_COMMON_FLAG(P)  UINT8LE ((P)+0)
#define A2T_VARHEADER_V5678_LEN(P, I)       UINT16LE((P)+1+I*2)
#define A2T_VARHEADER_V5678_SIZE            (21)

#define A2T_VARHEADER_V9_COMMON_FLAG(P)     UINT8LE ((P)+0)
#define A2T_VARHEADER_V9_PATT_LEN(P)        UINT16LE((P)+1)
#define A2T_VARHEADER_V9_NM_TRACKS(P)       UINT8LE ((P)+3)
#define A2T_VARHEADER_V9_MACRO_SPEEDUP(P)   UINT16LE((P)+4)
#define A2T_VARHEADER_V9_LEN(P, I)          UINT32LE((P)+6+I*4)
#define A2T_VARHEADER_V9_SIZE               (86)

#define A2T_VARHEADER_V10_COMMON_FLAG(P)    UINT8LE ((P)+0)
#define A2T_VARHEADER_V10_PATT_LEN(P)       UINT16LE((P)+1)
#define A2T_VARHEADER_V10_NM_TRACKS(P)      UINT8LE ((P)+3)
#define A2T_VARHEADER_V10_MACRO_SPEEDUP(P)  UINT16LE((P)+4)
#define A2T_VARHEADER_V10_FLAG_4OP(P)       UINT8LE ((P)+6)
#define A2T_VARHEADER_V10_LOCK_FLAGS(P, I)  UINT8LE ((P)+7+I)
#define A2T_VARHEADER_V10_LEN(P, I)         UINT32LE((P)+27+I*4)
#define A2T_VARHEADER_V10_SIZE              (107)

#define A2T_VARHEADER_V11_COMMON_FLAG(P)    UINT8LE ((P)+0)
#define A2T_VARHEADER_V11_PATT_LEN(P)       UINT16LE((P)+1)
#define A2T_VARHEADER_V11_NM_TRACKS(P)      UINT8LE ((P)+3)
#define A2T_VARHEADER_V11_MACRO_SPEEDUP(P)  UINT16LE((P)+4)
#define A2T_VARHEADER_V11_FLAG_4OP(P)       UINT8LE ((P)+6)
#define A2T_VARHEADER_V11_LOCK_FLAGS(P, I)  UINT8LE ((P)+7+I)
#define A2T_VARHEADER_V11_LEN(P, I)         UINT32LE((P)+27+I*4)
#define A2T_VARHEADER_V11_SIZE              (111)

#if 0
typedef struct {
    char id[15];    // 0 '_a2tiny_module_'
    uint8_t crc[4]; // 15-16-17-18 uint32_t
    uint8_t ffver;  // 19
    uint8_t npatt;  // 20
    uint8_t tempo;  // 21
    uint8_t speed;  // 22
} A2T_HEADER;
STATIC_ASSERT(sizeof(A2T_HEADER) == 23);

typedef struct {
    uint8_t len[6][2]; // uint16_t
} A2T_VARHEADER_V1234;

typedef struct {
    uint8_t common_flag;      // 0
    uint8_t len[10][2];       // 1 uint16_t
} A2T_VARHEADER_V5678;

typedef struct {
    uint8_t common_flag;      // 0
    uint8_t patt_len[2];      // 1,2 uint16_t
    uint8_t nm_tracks;        // 3
    uint8_t macro_speedup[2]; // 4,5 uint16_t
    uint8_t len[20][4];       // 6-85 uint32_t
} A2T_VARHEADER_V9;

typedef struct {
    uint8_t common_flag;      // 0
    uint8_t patt_len[2];      // 1,2 uint16_t
    uint8_t nm_tracks;        // 3
    uint8_t macro_speedup[2]; // 4,5 uint16_t
    uint8_t flag_4op;         // 6
    uint8_t lock_flags[20];   // 7-26
    uint8_t len[20][4];       // 27-106 uint32_t
} A2T_VARHEADER_V10;

typedef struct {
    uint8_t common_flag;      // 0
    uint8_t patt_len[2];      // 1,2 uint16_t
    uint8_t nm_tracks;        // 3
    uint8_t macro_speedup[2]; // 4,5 uint16_t
    uint8_t flag_4op;         // 6
    uint8_t lock_flags[20];   // 7-26
    uint8_t len[21][4];       // 27-110 uint32_t
} A2T_VARHEADER_V11;

typedef union {
    A2T_VARHEADER_V1234 v1234;
    A2T_VARHEADER_V5678 v5678;
    A2T_VARHEADER_V9    v9;
    A2T_VARHEADER_V10   v10;
    A2T_VARHEADER_V11   v11;
} A2T_VARHEADER;

STATIC_ASSERT(sizeof(A2T_VARHEADER_V1234) == 12);
STATIC_ASSERT(sizeof(A2T_VARHEADER_V5678) == 21);
STATIC_ASSERT(sizeof(A2T_VARHEADER_V9) == 86);
STATIC_ASSERT(sizeof(A2T_VARHEADER_V10) == 107);
STATIC_ASSERT(sizeof(A2T_VARHEADER_V11) == 111);
STATIC_ASSERT(sizeof(A2T_VARHEADER) == 111);

// only for importing v 1,2,3,4,5,6,7,8
typedef struct {
    uint8_t note;
    uint8_t instr_def;
    uint8_t effect_def;
    uint8_t effect;
} tADTRACK2_EVENT_V1_8;

// for importing v 1,2,3,4 patterns
typedef struct {
    struct {
        struct {
            tADTRACK2_EVENT_V1_8 ev;
        } ch[9];
    } row[64];
} tPATTERN_DATA_V1234;

// for importing v 5,6,7,8 patterns
typedef struct {
    struct {
        struct {
            tADTRACK2_EVENT_V1_8 ev;
        } row[64];
    } ch[18];
} tPATTERN_DATA_V5678;

STATIC_ASSERT(sizeof(tADTRACK2_EVENT_V1_8) == 4);
STATIC_ASSERT(sizeof(tPATTERN_DATA_V1234) == 2304);
STATIC_ASSERT(sizeof(tPATTERN_DATA_V5678) == 4608);

typedef struct {
    uint8_t note;       // 0
    uint8_t instr_def;  // 1
    struct {
        uint8_t def;    // 2, 4
        uint8_t val;    // 3, 5
    } eff[2];
} tADTRACK2_EVENT_V9_14;

STATIC_ASSERT(sizeof(tADTRACK2_EVENT_V9_14) == 6);

typedef struct {
    struct {
        struct {
            tADTRACK2_EVENT_V9_14 ev;
        } row[256];
    } ch[20];
} tPATTERN_DATA_V9_14;

STATIC_ASSERT(sizeof(tPATTERN_DATA_V9_14) == 20 * 256 * 6);
#endif

#define tADTRACK2_EVENT_V1_8_SIZE       (4)
#define tPATTERN_DATA_V1234_SIZE        (64 * 9 * 4)
#define tPATTERN_DATA_V5678_SIZE        (18 * 64 * 4)
#define tADTRACK2_EVENT_V9_14_SIZE      (6)
#define tPATTERN_DATA_V9_14_SIZE        (20 * 256 * 6)

/* Structures for importing A2M format V1-8 */

#if 0
typedef struct {
    char id[10];    // 0 '_a2module_'
    uint8_t crc[4]; // 10-11-12-13 uint32_t
    uint8_t ffver;  // 14
    uint8_t npatt;  // 15
} A2M_HEADER;
STATIC_ASSERT(sizeof(A2M_HEADER) == 16);
#endif

#define A2M_HEADER_FFVER(P)     ((P)[14])
#define A2M_HEADER_NPATT(P)     ((P)[15])
#define A2M_HEADER_SIZE         (16)

typedef struct {
    union {
        struct {
            uint8_t multipM: 4, ksrM: 1, sustM: 1, vibrM: 1, tremM : 1; // 0
            uint8_t multipC: 4, ksrC: 1, sustC: 1, vibrC: 1, tremC : 1; // 1
            uint8_t volM: 6, kslM: 2; // 2
            uint8_t volC: 6, kslC: 2; // 3
            uint8_t decM: 4, attckM: 4; // 4
            uint8_t decC: 4, attckC: 4; // 5
            uint8_t relM: 4, sustnM: 4; // 6
            uint8_t relC: 4, sustnC: 4; // 7
            uint8_t wformM: 3, : 5; // 8
            uint8_t wformC: 3, : 5; // 9
            uint8_t connect: 1, feedb: 3, : 4; // 10, panning is not used here
        };
        //uint8_t data[11];
    };
} tFM_INST_DATA_V1_14;

STATIC_ASSERT(sizeof(tFM_INST_DATA_V1_14) == 11);

typedef struct {
    tFM_INST_DATA_V1_14 fm;
    uint8_t panning; // 11
    int8_t  fine_tune; // 12
} tINSTR_DATA_V1_8;

STATIC_ASSERT(sizeof(tINSTR_DATA_V1_8) == 13);

typedef struct {
    char songname[43];
    char composer[43];
    char instr_names[250][33];
    tINSTR_DATA_V1_8 instr_data[250]; // uint8_t instr_data[13][250];
    uint8_t pattern_order[128];
    uint8_t tempo;
    uint8_t speed;
    uint8_t common_flag; // A2M_SONGDATA_V5678
} A2M_SONGDATA_V1_8;

STATIC_ASSERT(sizeof(A2M_SONGDATA_V1_8) == 11717);

/* Structures for importing A2M format V9-14 */

typedef struct {
    tFM_INST_DATA_V1_14 fm;
    uint8_t panning; // 11
    int8_t  fine_tune; // 12
    uint8_t perc_voice; // 13
} tINSTR_DATA_V9_14;

STATIC_ASSERT(sizeof(tINSTR_DATA_V9_14) == 14);

typedef struct {
    tFM_INST_DATA_V1_14 fm;     // 0
    uint8_t freq_slide[2];      // 11-12 int16_t
    uint8_t panning;            // 13
    uint8_t duration;           // 14
} tREGISTER_TABLE_DEF_V9_14;

STATIC_ASSERT(sizeof(tREGISTER_TABLE_DEF_V9_14) == 15);
#define tREGISTER_TABLE_DEF_V9_14_SIZE      (15)

typedef struct {
    uint8_t length;                     // 0
    uint8_t loop_begin;                 // 1
    uint8_t loop_length;                // 2
    uint8_t keyoff_pos;                 // 3
    uint8_t arpeggio_table;             // 4
    uint8_t vibrato_table;              // 5
    tREGISTER_TABLE_DEF_V9_14 data[255];    // 6
} tFMREG_TABLE_V9_14;

STATIC_ASSERT(sizeof(tFMREG_TABLE_V9_14) == 3831);
#define tFMREG_TABLE_V9_14_SIZE     (3831)

typedef struct {
    uint8_t length;         // 0
    uint8_t speed;          // 1
    uint8_t loop_begin;     // 2
    uint8_t loop_length;    // 3
    uint8_t keyoff_pos;     // 4
    uint8_t data[255];      // 5
} tARPEGGIO_TABLE_V9_14;

STATIC_ASSERT(sizeof(tARPEGGIO_TABLE_V9_14) == 260);

typedef struct {
    uint8_t length;         // 0
    uint8_t speed;          // 1
    uint8_t delay;          // 2
    uint8_t loop_begin;     // 3
    uint8_t loop_length;    // 4
    uint8_t keyoff_pos;     // 5
    int8_t data[255];       // 6 array[1..255] of Shortint;
} tVIBRATO_TABLE_V9_14;

STATIC_ASSERT(sizeof(tVIBRATO_TABLE_V9_14) == 261);

typedef struct {
    tARPEGGIO_TABLE_V9_14 arpeggio;
    tVIBRATO_TABLE_V9_14 vibrato;
} tARPVIB_TABLE_V9_14;

STATIC_ASSERT(sizeof(tARPVIB_TABLE_V9_14) == 521);
#define tARPVIB_TABLE_V9_14_SIZE    (521)

typedef struct {
    uint8_t num_4op;
    uint8_t idx_4op[128];
} tINS_4OP_FLAGS;

typedef uint8_t tRESERVED[1024];

typedef struct {
    uint8_t rows_per_beat;
    int8_t tempo_finetune[2]; // int16_t
} tBPM_DATA;

typedef struct {
    char songname[43];                      // 0  : 43
    char composer[43];                      // 43 : 43
    char instr_names[255][43];              // 86 : 43*255
    tINSTR_DATA_V9_14 instr_data[255];      // 11051
    tFMREG_TABLE_V9_14 fmreg_table[255];    // 14621
    tARPVIB_TABLE_V9_14 arpvib_table[255];  //
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
    bool dis_fmreg_col[255][28]; // A2M_SONGDATA_V11
    tINS_4OP_FLAGS ins_4op_flags;  // A2M_SONGDATA_V12_13
    tRESERVED reserved_data;       // A2M_SONGDATA_V12_13
    tBPM_DATA bpm_data;            // A2M_SONGDATA_V14
} A2M_SONGDATA_V9_14;

STATIC_ASSERT(sizeof(A2M_SONGDATA_V9_14) == 1138338);

/* Player data */

typedef struct {
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
} tFM_INST_DATA;

typedef struct {
    tFM_INST_DATA fm;
    uint8_t panning;
    int8_t  fine_tune;
    uint8_t perc_voice;
} tINSTR_DATA;

typedef struct {
    uint8_t length;
    uint8_t speed;
    uint8_t loop_begin;
    uint8_t loop_length;
    uint8_t keyoff_pos;
    uint8_t data[255];
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
    int16_t freq_slide;
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
} tFMREG_TABLE;

typedef struct {
    tINSTR_DATA instr_data;
    uint8_t vibrato;
    uint8_t arpeggio;
    tFMREG_TABLE *fmreg;
    uint32_t dis_fmreg_cols;
} tINSTR_DATA_EXT;

typedef struct {
    char            songname[43];        // pascal String[42];
    char            composer[43];        // pascal String[42];
    char            instr_names[255][43];// array[1..255] of String[42];
    uint8_t         pattern_order[0x80]; // array[0..0x7f] of Byte;
    uint8_t         tempo;
    uint8_t         speed;
    uint8_t         common_flag;
    uint16_t        patt_len;
    uint8_t         nm_tracks;
    uint16_t        macro_speedup;
    uint8_t         flag_4op;
    uint8_t         lock_flags[20];
} tSONGINFO;

typedef struct {
    uint16_t freq;
    uint8_t speed;
} tPORTA_TABLE;

typedef struct {
    uint8_t state, note, add1, add2;
} tARPGG_TABLE;

typedef struct {
    int8_t pos;
    uint8_t volM, volC;
} tTREMOR_TABLE;

typedef struct {
    uint8_t pos, dir, speed, depth;
    bool fine;
} tVIBRTREM_TABLE;

typedef struct {
    uint8_t def, val;
} tEFFECT_TABLE;

typedef struct {
    uint16_t fmreg_pos,
         arpg_pos,
         vib_pos;
    uint8_t
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

typedef struct {
    uint8_t note;
    uint8_t instr_def; // TODO: rename to 'ins'
    struct {
        uint8_t def;
        uint8_t val;
    } eff[2];
} tADTRACK2_EVENT;

/*typedef struct {
    struct {
        struct {
            tADTRACK2_EVENT ev;
        } row[256];
    } ch[20];
} tPATTERN_DATA;*/

typedef struct {
    tFM_INST_DATA fmpar_table[20]; // TODO: rename to 'fm'
    bool volume_lock[20];
    bool vol4op_lock[20];
    bool peak_lock[20];
    bool pan_lock[20];
    uint8_t modulator_vol[20];
    uint8_t carrier_vol[20];
    // note/instr_def - memorized across rows
    // effects - change each row
    tADTRACK2_EVENT event_table[20];
    uint8_t voice_table[20];
    uint16_t freq_table[20];
    uint16_t zero_fq_table[20];
    tEFFECT_TABLE effect_table[2][20];
    uint8_t fslide_table[2][20];
    tEFFECT_TABLE glfsld_table[2][20];
    tPORTA_TABLE porta_table[2][20];
    bool portaFK_table[20];
    tARPGG_TABLE arpgg_table[2][20];
    tVIBRTREM_TABLE vibr_table[2][20];
    tVIBRTREM_TABLE trem_table[2][20];
    uint8_t retrig_table[2][20];
    tTREMOR_TABLE tremor_table[2][20];
    uint8_t panning_table[20];
    tEFFECT_TABLE last_effect[2][20];
    uint8_t volslide_type[20];
    uint8_t notedel_table[20];
    uint8_t notecut_table[20];
    int8_t ftune_table[20];
    bool keyoff_loop[20];
    uint8_t loopbck_table[20];
    uint8_t loop_table[20][256];
    bool reset_chan[20];
    tCH_MACRO_TABLE macro_table[20];
} tCHDATA;

typedef struct {
    unsigned int count;
    size_t size;
    tINSTR_DATA_EXT *instruments;
} tINSTR_INFO;

typedef struct {
    int patterns, rows, channels;
    size_t size;
    tADTRACK2_EVENT *events;
} tEVENTS_INFO;

typedef struct _4op_data {
    uint32_t mode: 1, conn: 3, ch1: 4, ch2: 4, ins1: 8, ins2: 8;
} t4OP_DATA;

/* Extern data */

extern tCHDATA *ch;
extern bool songend;

#endif // _A2T_H_
