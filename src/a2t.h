#ifndef _A2T_H_
#define _A2T_H_

#include <stdint.h>

void a2t_init(int);
void a2t_shut();
char *a2t_load(char *name);
void a2t_play(char *tune);
void a2t_update(unsigned char *, int);
void set_overall_volume(unsigned char level);
void a2t_stop();

extern uint8_t current_order;
extern uint8_t current_pattern;
extern uint8_t current_line;

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

// Macros for extracting little-endian integers from filedata
#define INT16LE(A) (int16_t)((A[0]) | (A[1] << 8))
#define UINT16LE(A) (uint16_t)((A[0]) | (A[1] << 8))
#define INT32LE(A) (int32_t)((A[0]) | (A[1] << 8) | (A[2] << 16) | (A[3] << 24))
#define UINT32LE(A) (uint32_t)((A[0]) | (A[1] << 8) | (A[2] << 16) | (A[3] << 24))


/*
    Structures for importing A2T/A2M with no padding.
    AdlibTracker 2 was saving structures directly from memory into the file, so we emulate
    how FreePascal was handling them. We use chars everywhere and imply that ints are always
    little-endian. If only chars are used, C compiler doesn't insert any padding.
*/

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
} tFMREG_TABLE;

typedef struct {
    tARPEGGIO_TABLE arpeggio;
    tVIBRATO_TABLE vibrato;
} tARPVIB_TABLE;

C_ASSERT(sizeof(tFMREG_TABLE) == 3831);
C_ASSERT(sizeof(tARPVIB_TABLE) == 521);

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

/* Data for importing A2T format */
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

/* Data for importing A2M format */
typedef struct {
    tFM_INST_DATA fm;
    uint8_t panning;
    int8_t  fine_tune;
} tINSTR_DATA_V1_8;

C_ASSERT(sizeof(tINSTR_DATA_V1_8) == 13);

typedef struct {
    char songname[43];
    char composer[43];
    char instr_names[250][33];
    tINSTR_DATA_V1_8 instr_data[250];
    uint8_t pattern_order[128];
    uint8_t tempo;
    uint8_t speed;
    uint8_t common_flag; // A2M_SONGDATA_V5678
} A2M_SONGDATA_V1_8;

C_ASSERT(sizeof(A2M_SONGDATA_V1_8) == 11717);

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
    char songname[43];
    char composer[43];
    char instr_names[255][43];
    tINSTR_DATA instr_data[255];
    tFMREG_TABLE fmreg_table[255];
    tARPVIB_TABLE arpvib_table[255];
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
    tINS_4OP_FLAGS ins_4op_flags;  // A2M_SONGDATA_V12_13
    tRESERVED reserved_data;       // A2M_SONGDATA_V12_13
    tBPM_DATA bpm_data;            // A2M_SONGDATA_V14
} A2M_SONGDATA_V9_14;

C_ASSERT(sizeof(A2M_SONGDATA_V9_14) == 1138338);

/* Extern data*/

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
    tFM_INST_DATA fmpar_table[20]; // TODO: rename to 'fm'
    bool volume_lock[20];
    bool vol4op_lock[20];
    bool peak_lock[20];
    bool pan_lock[20];
    uint8_t modulator_vol[20];
    uint8_t carrier_vol[20];
} tCHDATA;

extern tCHDATA *ch;

extern uint8_t voice_table[20];
extern uint16_t freq_table[20];
extern tEFFECT_TABLE effect_table[2][20];
extern tEFFECT_TABLE last_effect[2][20];
extern tEFFECT_TABLE glfsld_table[2][20];
extern tCH_MACRO_TABLE macro_table[20];
extern tADTRACK2_EVENT event_table[20];

#endif // _A2T_H_
