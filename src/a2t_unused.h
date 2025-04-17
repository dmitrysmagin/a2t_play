#ifndef _A2T_UNUSED_H_
#define _A2T_UNUSED_H_

#include <stdint.h>
#include <stdbool.h>

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


/* Structures for importing A2T format (all versions) */

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

/* Structures for importing A2M format V1-8 */

typedef struct {
    char id[10];    // 0 '_a2module_'
    uint8_t crc[4]; // 10-11-12-13 uint32_t
    uint8_t ffver;  // 14
    uint8_t npatt;  // 15
} A2M_HEADER;

STATIC_ASSERT(sizeof(A2M_HEADER) == 16);

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
    char songname[43];                  // 0-42
    char composer[43];                  // 43-85
    char instr_names[250][33];          // 86-8335
    tINSTR_DATA_V1_8 instr_data[250];   // 8336 uint8_t instr_data[13][250];
    uint8_t pattern_order[128];         // 11586
    uint8_t tempo;                      // 11714
    uint8_t speed;                      // 11715
    uint8_t common_flag;                // 11716 A2M_SONGDATA_V5678
} A2M_SONGDATA_V1_8;

STATIC_ASSERT(sizeof(A2M_SONGDATA_V1_8) == 11717);

/* Structures for importing A2M format V9-14 */

typedef struct {
    tFM_INST_DATA_V1_14 fm;
    uint8_t panning;        // 11
    int8_t  fine_tune;      // 12
    uint8_t perc_voice;     // 13
} tINSTR_DATA_V9_14;

STATIC_ASSERT(sizeof(tINSTR_DATA_V9_14) == 14);

typedef struct {
    tFM_INST_DATA_V1_14 fm;     // 0
    uint8_t freq_slide[2];      // 11-12 int16_t
    uint8_t panning;            // 13
    uint8_t duration;           // 14
} tREGISTER_TABLE_DEF_V9_14;

STATIC_ASSERT(sizeof(tREGISTER_TABLE_DEF_V9_14) == 15);

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
    char songname[43];                      // 0-43
    char composer[43];                      // 43-43
    char instr_names[255][43];              // 86-43*255
    tINSTR_DATA_V9_14 instr_data[255];      // 11051
    tFMREG_TABLE_V9_14 fmreg_table[255];    // 14621
    tARPVIB_TABLE_V9_14 arpvib_table[255];  // 991526
    uint8_t pattern_order[128];             // 1124381
    uint8_t tempo;                          // 1124509
    uint8_t speed;                          // 1124510
    uint8_t common_flag;                    // 1124511
    uint8_t patt_len[2];                    // 1124512 uint16_t
    uint8_t nm_tracks;                      // 1124514
    uint8_t macro_speedup[2];               // 1124515 uint16_t
    uint8_t flag_4op;                       // 1124517 A2M_SONGDATA_V10
    uint8_t lock_flags[20];                 // 1124518 A2M_SONGDATA_V10
    char pattern_names[128][43];            // 1124538 A2M_SONGDATA_V11
    bool dis_fmreg_col[255][28];            // 1130042 A2M_SONGDATA_V11
    tINS_4OP_FLAGS ins_4op_flags;           // 1137182 A2M_SONGDATA_V12_13
    tRESERVED reserved_data;                // 1137311 A2M_SONGDATA_V12_13
    tBPM_DATA bpm_data;                     // 1138335 A2M_SONGDATA_V14
} A2M_SONGDATA_V9_14;

STATIC_ASSERT(sizeof(A2M_SONGDATA_V9_14) == 1138338);



#endif // _A2T_UNUSED_H_
