
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <string.h>

#include "depack.h"
#include "ymf262.h"
#include "a2t.h"

#ifdef __GNUC__
#define PACK __attribute__((__packed__))
#elif _MSC_VER
#define PACK
#pragma pack(1)
#else
#define PACK
#endif

#ifndef bool
typedef signed char bool;
#endif

typedef struct {
	uint8_t AM_VIB_EG_modulator;
	uint8_t AM_VIB_EG_carrier;
	uint8_t KSL_VOLUM_modulator;
	uint8_t KSL_VOLUM_carrier;
	uint8_t ATTCK_DEC_modulator;
	uint8_t ATTCK_DEC_carrier;
	uint8_t SUSTN_REL_modulator;
	uint8_t SUSTN_REL_carrier;
	uint8_t WAVEFORM_modulator;
	uint8_t WAVEFORM_carrier;
	uint8_t FEEDBACK_FM;
} PACK tFM_INST_DATA;

typedef struct {
	tFM_INST_DATA fm_data;
	uint8_t panning;
	int8_t  fine_tune;
	uint8_t perc_voice;
} PACK tADTRACK2_INS;

typedef struct {
	uint8_t length;
	uint8_t speed;
	uint8_t loop_begin;
	uint8_t loop_length;
	uint8_t keyoff_pos;
	uint8_t data[255]; // array[1..255] of Byte;
} PACK tARPEGGIO_TABLE;

typedef struct {
	uint8_t length;
	uint8_t speed;
	uint8_t delay;
	uint8_t loop_begin;
	uint8_t loop_length;
	uint8_t keyoff_pos;
	int8_t data[255]; // array[1..255] of Shortint;
} PACK tVIBRATO_TABLE;

typedef struct {
	tFM_INST_DATA fm_data;
	int16_t freq_slide;
	uint8_t panning;
	uint8_t duration;
} PACK tREGISTER_TABLE_DEF;

typedef struct {
	uint8_t length;
	uint8_t loop_begin;
	uint8_t loop_length;
	uint8_t keyoff_pos;
	uint8_t arpeggio_table;
	uint8_t vibrato_table;
	tREGISTER_TABLE_DEF data[255]; // array[1..255] of tREGISTER_TABLE_DEF;
} PACK tREGISTER_TABLE;

typedef struct {
	tARPEGGIO_TABLE arpeggio;
	tVIBRATO_TABLE vibrato;
} PACK tMACRO_TABLE;

typedef struct {
	uint8_t adsrw_car;
	struct {
		uint8_t attck,dec,sustn,rel,
		wform;
	} adsrw_mod;
	uint8_t connect;
	uint8_t feedb;
	uint8_t multipM,kslM,tremM,vibrM,ksrM,sustM;
	uint8_t multipC,kslC,tremC,vibrC,ksrC,sustC;
} PACK tFM_PARAMETER_TABLE;

typedef struct {
	uint8_t note;
	uint8_t instr_def;
	uint8_t effect_def;
	uint8_t effect;
	uint8_t effect_def2;
	uint8_t effect2;
} PACK tADTRACK2_EVENT;

typedef bool tDIS_FMREG_COL[28]; // array[0..27] of Boolean;

typedef struct {
	tADTRACK2_INS   instr_data[255];     // array[1..255] of tADTRACK2_INS;
	tREGISTER_TABLE instr_macros[255];   // array[1..255] of tREGISTER_TABLE;
	tMACRO_TABLE    macro_table[255];    // array[1..255] of tMACRO_TABLE;
	uint8_t         pattern_order[0x80]; // array[0..0x7f] of Byte;
	uint8_t         tempo;
	uint8_t         speed;
	uint8_t         common_flag;
	uint16_t        patt_len;
	uint8_t         nm_tracks;
	uint16_t        macro_speedup;
	uint8_t         flag_4op;
	uint8_t         lock_flags[20];      // array[1..20]  of Byte;
	tDIS_FMREG_COL  dis_fmreg_col[255];  // array[1..255] of tDIS_FMREG_COL;
} PACK tFIXED_SONGDATA;

typedef enum {
	isPlaying = 0, isPaused, isStopped
} tPLAY_STATUS;

typedef tADTRACK2_EVENT tVARIABLE_DATA[8][20][256];
// array[0..7] of array[1..20] of array[0..0x0ff] of tADTRACK2_EVENT;

typedef tVARIABLE_DATA tPATTERN_DATA[16];
// array[0..15] of tVARIABLE_DATA;

const uint8_t _panning[3] = {0x30, 0x10, 0x20};

const uint8_t _instr[12] = {
	0x20, 0x20, 0x40, 0x40, 0x60, 0x60, 0x80, 0x80, 0xe0, 0xe0, 0xc0, 0xbd
};

typedef uint16_t tTRACK_ADDR[20]; // array[1..20] of Word;

#define INSTRUMENT_SIZE sizeof(tADTRACK2_INS)
#define CHUNK_SIZE sizeof(tADTRACK2_EVENT)
#define PATTERN_SIZE (20*256*CHUNK_SIZE)
#define NONE 0xFF


const tTRACK_ADDR _chmm_n = {
	0x003,0x000,0x004,0x001,0x005,0x002,0x006,0x007,0x008,0x103,
	0x100,0x104,0x101,0x105,0x102,0x106,0x107,0x108,NONE,NONE
};
const tTRACK_ADDR _chmm_m = {
	0x008,0x000,0x009,0x001,0x00a,0x002,0x010,0x011,0x012,0x108,
	0x100,0x109,0x101,0x10a,0x102,0x110,0x111,0x112,NONE,NONE
};
const tTRACK_ADDR _chmm_c = {
	0x00b,0x003,0x00c,0x004,0x00d,0x005,0x013,0x014,0x015,0x10b,
	0x103,0x10c,0x104,0x10d,0x105,0x113,0x114,0x115,NONE,NONE
};

const tTRACK_ADDR _chpm_n = {
	0x003,0x000,0x004,0x001,0x005,0x002,0x106,0x107,0x108,0x103,
	0x100,0x104,0x101,0x105,0x102,0x006,0x007,0x008,0x008,0x007
};
const tTRACK_ADDR _chpm_m = {
	0x008,0x000,0x009,0x001,0x00a,0x002,0x110,0x111,0x112,0x108,
	0x100,0x109,0x101,0x10a,0x102,0x010,0x014,0x012,0x015,0x011
};
const tTRACK_ADDR _chpm_c = {
	0x00b,0x003,0x00c,0x004,0x00d,0x005,0x113,0x114,0x115,0x10b,
	0x103,0x10c,0x104,0x10d,0x105,0x013,NONE,NONE,NONE,NONE
};

tTRACK_ADDR _chan_n, _chan_m, _chan_c;

#if 0
const
  ef_Arpeggio          = 0;
  ef_FSlideUp          = 1;
  ef_FSlideDown        = 2;
  ef_TonePortamento    = 3;
  ef_Vibrato           = 4;
  ef_TPortamVolSlide   = 5;
  ef_VibratoVolSlide   = 6;
  ef_FSlideUpFine      = 7;
  ef_FSlideDownFine    = 8;
  ef_SetModulatorVol   = 9;
  ef_VolSlide          = 10;
  ef_PositionJump      = 11;
  ef_SetInsVolume      = 12;
  ef_PatternBreak      = 13;
  ef_SetTempo          = 14;
  ef_SetSpeed          = 15;
  ef_TPortamVSlideFine = 16;
  ef_VibratoVSlideFine = 17;
  ef_SetCarrierVol     = 18;
  ef_SetWaveform       = 19;
  ef_VolSlideFine      = 20;
  ef_RetrigNote        = 21;
  ef_Tremolo           = 22;
  ef_Tremor            = 23;
  ef_ArpggVSlide       = 24;
  ef_ArpggVSlideFine   = 25;
  ef_MultiRetrigNote   = 26;
  ef_FSlideUpVSlide    = 27;
  ef_FSlideDownVSlide  = 28;
  ef_FSlUpFineVSlide   = 29;
  ef_FSlDownFineVSlide = 30;
  ef_FSlUpVSlF         = 31;
  ef_FSlDownVSlF       = 32;
  ef_FSlUpFineVSlF     = 33;
  ef_FSlDownFineVSlF   = 34;
  ef_Extended          = 35;
  ef_Extended2         = 36;
  ef_SetGlobalVolume   = 37;
  ef_SwapArpeggio      = 38;
  ef_SwapVibrato       = 39;
  ef_ForceInsVolume    = 40;
  ef_Extended3         = 41;
  ef_ExtraFineArpeggio = 42;
  ef_ExtraFineVibrato  = 43;
  ef_ExtraFineTremolo  = 44;
  ef_ex_SetTremDepth   = 0;
  ef_ex_SetVibDepth    = 1;
  ef_ex_SetAttckRateM  = 2;
  ef_ex_SetDecayRateM  = 3;
  ef_ex_SetSustnLevelM = 4;
  ef_ex_SetRelRateM    = 5;
  ef_ex_SetAttckRateC  = 6;
  ef_ex_SetDecayRateC  = 7;
  ef_ex_SetSustnLevelC = 8;
  ef_ex_SetRelRateC    = 9;
  ef_ex_SetFeedback    = 10;
  ef_ex_SetPanningPos  = 11;
  ef_ex_PatternLoop    = 12;
  ef_ex_PatternLoopRec = 13;
  ef_ex_MacroKOffLoop  = 14;
  ef_ex_ExtendedCmd    = 15;
  ef_ex_cmd_RSS        = 0;
  ef_ex_cmd_ResetVol   = 1;
  ef_ex_cmd_LockVol    = 2;
  ef_ex_cmd_UnlockVol  = 3;
  ef_ex_cmd_LockVP     = 4;
  ef_ex_cmd_UnlockVP   = 5;
  ef_ex_cmd_VSlide_mod = 6;
  ef_ex_cmd_VSlide_car = 7;
  ef_ex_cmd_VSlide_def = 8;
  ef_ex_cmd_LockPan    = 9;
  ef_ex_cmd_UnlockPan  = 10;
  ef_ex_cmd_VibrOff    = 11;
  ef_ex_cmd_TremOff    = 12;
  ef_ex_cmd_FineVibr   = 13;
  ef_ex_cmd_FineTrem   = 14;
  ef_ex_cmd_NoRestart  = 15;
  ef_ex2_PatDelayFrame = 0;
  ef_ex2_PatDelayRow   = 1;
  ef_ex2_NoteDelay     = 2;
  ef_ex2_NoteCut       = 3;
  ef_ex2_FineTuneUp    = 4;
  ef_ex2_FineTuneDown  = 5;
  ef_ex2_GlVolSlideUp  = 6;
  ef_ex2_GlVolSlideDn  = 7;
  ef_ex2_GlVolSlideUpF = 8;
  ef_ex2_GlVolSlideDnF = 9;
  ef_ex2_GlVolSldUpXF  = 10;
  ef_ex2_GlVolSldDnXF  = 11;
  ef_ex2_VolSlideUpXF  = 12;
  ef_ex2_VolSlideDnXF  = 13;
  ef_ex2_FreqSlideUpXF = 14;
  ef_ex2_FreqSlideDnXF = 15;
  ef_ex3_SetConnection = 0;
  ef_ex3_SetMultipM    = 1;
  ef_ex3_SetKslM       = 2;
  ef_ex3_SetTremoloM   = 3;
  ef_ex3_SetVibratoM   = 4;
  ef_ex3_SetKsrM       = 5;
  ef_ex3_SetSustainM   = 6;
  ef_ex3_SetMultipC    = 7;
  ef_ex3_SetKslC       = 8;
  ef_ex3_SetTremoloC   = 9;
  ef_ex3_SetVibratoC   = 10;
  ef_ex3_SetKsrC       = 11;
  ef_ex3_SetSustainC   = 12;
#endif

const uint8_t ef_fix1 = 0x80;
const uint8_t ef_fix2 = 0x90;
const uint8_t keyoff_flag        = 0x80;
const uint8_t fixed_note_flag    = 0x90;
const uint8_t pattern_loop_flag  = 0xe0;
const uint8_t pattern_break_flag = 0xf0;


int main(void)
{
	_chan_n[2] = 0;

	return 0;
}