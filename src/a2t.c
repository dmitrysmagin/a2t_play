
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <string.h>

#include "depack.h"
#include "sixpack.h"
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

#ifndef FALSE
#define FALSE 0
#define TRUE !FALSE
#endif

typedef struct PACK {
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
} tFM_INST_DATA;

typedef struct PACK {
	tFM_INST_DATA fm_data;
	uint8_t panning;
	int8_t  fine_tune;
	uint8_t perc_voice;
} tADTRACK2_INS;

typedef struct PACK {
	uint8_t length;
	uint8_t speed;
	uint8_t loop_begin;
	uint8_t loop_length;
	uint8_t keyoff_pos;
	uint8_t data[255]; // array[1..255] of Byte;
} tARPEGGIO_TABLE;

typedef struct PACK {
	uint8_t length;
	uint8_t speed;
	uint8_t delay;
	uint8_t loop_begin;
	uint8_t loop_length;
	uint8_t keyoff_pos;
	int8_t data[255]; // array[1..255] of Shortint;
} tVIBRATO_TABLE;

typedef struct PACK {
	tFM_INST_DATA fm_data;
	int16_t freq_slide;
	uint8_t panning;
	uint8_t duration;
} tREGISTER_TABLE_DEF;

typedef struct PACK {
	uint8_t length;
	uint8_t loop_begin;
	uint8_t loop_length;
	uint8_t keyoff_pos;
	uint8_t arpeggio_table;
	uint8_t vibrato_table;
	tREGISTER_TABLE_DEF data[255]; // array[1..255] of tREGISTER_TABLE_DEF;
} tREGISTER_TABLE;

typedef struct PACK {
	tARPEGGIO_TABLE arpeggio;
	tVIBRATO_TABLE vibrato;
} tMACRO_TABLE;

typedef struct PACK {
	struct {
		uint8_t attck,dec,sustn,rel,
		wform;
	} adsrw_car, adsrw_mod;
	uint8_t connect;
	uint8_t feedb;
	uint8_t multipM,kslM,tremM,vibrM,ksrM,sustM;
	uint8_t multipC,kslC,tremC,vibrC,ksrC,sustC;
} tFM_PARAMETER_TABLE;

typedef bool tDIS_FMREG_COL[28]; // array[0..27] of Boolean;

// ATTENTION: may not be packed in future!
typedef struct PACK {
	char            songname[43];        // pascal String[42];
	char            composer[43];        // pascal String[42];
	char            instr_names[255][43];// array[1..255] of String[42];
	//tADTRACK2_INS   instr_data[255];     // array[1..255] of tADTRACK2_INS;
	uint8_t         instr_data[255][14];
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
	uint8_t         lock_flags[20];
	char            pattern_names[128][43];  // array[0..$7f] of String[42];
	//tDIS_FMREG_COL  dis_fmreg_col[255];  // array[1..255] of tDIS_FMREG_COL;
	int8_t          dis_fmreg_col[255][28];
} tFIXED_SONGDATA;

typedef enum {
	isPlaying = 0, isPaused, isStopped
} tPLAY_STATUS;

typedef struct PACK {
	uint8_t note;
	uint8_t instr_def;
	uint8_t effect_def;
	uint8_t effect;
	uint8_t effect_def2;
	uint8_t effect2;
} tADTRACK2_EVENT;

//type
//  tVARIABLE_DATA = array[0..7]    of
//                   array[1..20]   of
//                   array[0..$0ff] of tADTRACK2_EVENT;
//type
//  tPATTERN_DATA = array[0..15] of tVARIABLE_DATA;

// as C doesn't support pointers to typedef'ed arrays, make a struct
// pattdata[1].ch[2].row[3].ev.note;
typedef struct PACK {
  struct PACK {
    struct PACK {
      tADTRACK2_EVENT ev;
    } row[256];
  } ch[20];
} tPATTERN_DATA;

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
#define ef_ex_MacroKOffLoop    14
#define ef_ex_ExtendedCmd      15
#define ef_ex_cmd_RSS          0
#define ef_ex_cmd_ResetVol     1
#define ef_ex_cmd_LockVol      2
#define ef_ex_cmd_UnlockVol    3
#define ef_ex_cmd_LockVP       4
#define ef_ex_cmd_UnlockVP     5
#define ef_ex_cmd_VSlide_mod   6
#define ef_ex_cmd_VSlide_car   7
#define ef_ex_cmd_VSlide_def   8
#define ef_ex_cmd_LockPan      9
#define ef_ex_cmd_UnlockPan    10
#define ef_ex_cmd_VibrOff      11
#define ef_ex_cmd_TremOff      12
#define ef_ex_cmd_FineVibr     13
#define ef_ex_cmd_FineTrem     14
#define ef_ex_cmd_NoRestart    15
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

/*
  opl3port: Word = $388;
  error_code: Integer = 0;
*/
uint8_t current_order = 0;
uint8_t current_pattern = 0;
uint8_t current_line = 0;

uint8_t tempo = 50;
uint8_t speed = 6;

uint16_t macro_speedup = 1;
bool irq_mode = FALSE;

int16_t IRQ_freq = 50;
bool irq_initialized = FALSE;
const bool timer_fix = TRUE;
bool pattern_break = FALSE;
bool pattern_delay = FALSE;
uint8_t next_line = 0;
const uint8_t max_patterns = 128;
tPLAY_STATUS play_status = isStopped;
uint8_t overall_volume = 63;
uint8_t global_volume = 63;

const uint8_t keyoff_flag        = 0x80;
const uint8_t fixed_note_flag    = 0x90;
const uint8_t pattern_loop_flag  = 0xe0;
const uint8_t pattern_break_flag = 0xf0;

tFM_PARAMETER_TABLE fmpar_table[20];	// array[1..20] of tFM_PARAMETER_TABLE;
bool volume_lock[20];			// array[1..20] of Boolean;
uint16_t volume_table[20];		// array[1..20] of Word;
uint16_t vscale_table[20];		// array[1..20] of Word;
bool peak_lock[20];			// array[1..20] of Boolean;
bool pan_lock[20];			// array[1..20] of Boolean;
uint8_t modulator_vol[20];		// array[1..20] of Byte;
uint8_t carrier_vol[20];		// array[1..20] of Byte;
tADTRACK2_EVENT event_table[20];	// array[1..20] of tADTRACK2_EVENT;
uint8_t voice_table[20];		// array[1..20] of Byte;
uint16_t freq_table[20];		// array[1..20] of Word;
uint16_t effect_table[20];		// array[1..20] of Word;
uint16_t effect_table2[20];		// array[1..20] of Word;
uint8_t fslide_table[20];		// array[1..20] of Byte;
uint8_t fslide_table2[20];		// array[1..20] of Byte;
struct PACK {
	uint16_t freq;
	uint8_t speed;
} porta_table[20];	// array[1..20] of Record freq: Word; speed: Byte; end;
struct PACK {
	uint16_t freq;
	uint8_t speed;
} porta_table2[20];	// array[1..20] of Record freq: Word; speed: Byte; end;
struct PACK {
	uint8_t state, note, add1, add2;
} arpgg_table[20];		// array[1..20] of Record state,note,add1,add2: Byte; end;
struct PACK {
	uint8_t state, note, add1, add2;
} arpgg_table2[20];		// array[1..20] of Record state,note,add1,add2: Byte; end;
struct PACK {
	uint8_t pos, speed, depth;
	bool fine;
} vibr_table[20];		// array[1..20] of Record pos,speed,depth: Byte; fine: Boolean; end;
struct PACK {
	uint8_t pos, speed, depth;
	bool fine;
} vibr_table2[20];		// array[1..20] of Record pos,speed,depth: Byte; fine: Boolean; end;
struct PACK {
	uint8_t pos, speed, depth;
	bool fine;
} trem_table[20];		// array[1..20] of Record pos,speed,depth: Byte; fine: Boolean; end;
struct PACK {
	uint8_t pos, speed, depth;
	bool fine;
} trem_table2[20];		// array[1..20] of Record pos,speed,depth: Byte; fine: Boolean; end;
uint8_t retrig_table[20];	// array[1..20] of Byte;
uint8_t retrig_table2[20];	// array[1..20] of Byte;
struct PACK {
	int pos;
	uint16_t volume;
} tremor_table[20];		// array[1..20] of Record pos: Integer; volume: Word; end;
struct PACK {
	int pos;
	uint16_t volume;
} tremor_table2[20];		// array[1..20] of Record pos: Integer; volume: Word; end;
uint8_t panning_table[20];	// array[1..20] of Byte;
uint16_t last_effect[20];	// array[1..20] of Word;
uint16_t last_effect2[20];	// array[1..20] of Word;
uint8_t volslide_type[20];	// array[1..20] of Byte;
bool event_new[20];		// array[1..20] of Boolean;
uint8_t notedel_table[20];	// array[1..20] of Byte;
uint8_t notecut_table[20];	// array[1..20] of Byte;
int8_t ftune_table[20];		// array[1..20] of Shortint;
bool keyoff_loop[20];		// array[1..20] of Boolean;
typedef struct PACK {
	uint16_t fmreg_pos,
		 arpg_pos,
		 vib_pos;
	uint8_t  fmreg_count,
		 fmreg_duration,
		 arpg_count,
		 vib_count,
		 vib_delay,
		 fmreg_table,
		 arpg_table,
		 vib_table,
		 arpg_note;
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
tPATTERN_DATA _pattdata[128], *pattdata = _pattdata;

int ticks, tick0, tickD, tickXF;

#define FreqStart 0x156
#define FreqEnd   0x2ae
#define FreqRange  (FreqEnd - FreqStart)

/* PLAYER */
void opl_out(uint8_t port, uint8_t val); // forward def

void opl2out(uint16_t reg, uint16_t data)
{
	opl_out(0, reg);
	opl_out(1, data);
}

void opl3out(uint16_t reg, uint16_t data)
{
	if (reg < 0x100) {
		opl_out(0, reg);
		opl_out(1, data);
	} else {
		opl_out(2, reg & 0xff);
		opl_out(3, data);
	}
}

void opl3exp(uint16_t data)
{
	opl_out(2, data & 0xff);
	opl_out(3, data >> 8);
}

static bool nul_data(void *data, unsigned int size)
{
	while (size--) {
		if (*(char *)data++)
			return FALSE;
	}

	return TRUE;
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
	uint16_t fr = (freq & 0x3ff);

	if (fr + shift >= FreqEnd) {
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
	uint16_t fr = (freq & 0x3ff);

	if (fr - shift <= FreqStart) {
		if (oc == 0) {
			fr = FreqStart;
		} else {
			oc--;
			fr += FreqRange;
		}
	}

	return (uint16_t)((oc << 10) | fr);
}

static uint16_t calc_vibrato_shift(uint8_t depth, uint8_t position)
{
	uint8_t vibr[32] = {
		0,24,49,74,97,120,141,161,180,197,212,224,235,244,250,253,255,
		253,250,244,235,224,212,197,180,161,141,120,97,74,49,24
	};

	uint16_t val;

	val = vibr[position & 0x1f] * depth;
	val = (val << 1) | (val >> 15); // rol val, 1
	val = (val << 8) | (val >> 8); // xchg ah,al
	val = val & 0x1ff;

	return val;
}

#define LO(A) ((A) & 0xFF)
#define HI(A) (((A) >> 8) & 0xFF)

static void change_freq(uint8_t chan, uint16_t freq)
{
	freq_table[chan] = (freq & 0x1fff) +
			   (freq_table[chan] & ~0x1fff);
	opl3out(0xa0 + _chan_n[chan], LO(freq_table[chan]));
	opl3out(0xb0 + _chan_n[chan], HI(freq_table[chan]));
}

static inline uint8_t ins_parameter(uint8_t ins, uint8_t param)
{
	// NOTE: adjust ins
	return songdata->instr_data[ins-1][param];
}

static inline uint16_t max(uint16_t value, uint16_t maximum)
{
	return (value > maximum ? maximum : value);
}

static inline uint16_t concw(uint8_t lo, uint8_t hi)
{
	return (lo | (hi << 8));
}

static void change_frequency(uint8_t chan, uint16_t freq)
{
	change_freq(chan, freq);
	macro_table[chan].vib_freq = freq;
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

	set_clock_rate(1193180 / IRQ_freq);
}

static void key_off(uint8_t chan)
{
	freq_table[chan] = LO(freq_table[chan]) +
			  ((HI(freq_table[chan]) & ~0x20) << 8);
	change_frequency(chan, freq_table[chan]);
	event_table[chan].note = event_table[chan].note | keyoff_flag;
}

static void release_sustaining_sound(uint8_t chan)
{
	opl3out(_instr[2] + _chan_m[chan], 63);
	opl3out(_instr[3] + _chan_c[chan], 63);

	memset(&fmpar_table[chan].adsrw_car, 0,
		sizeof(fmpar_table[chan].adsrw_car));
	memset(&fmpar_table[chan].adsrw_mod, 0,
		sizeof(fmpar_table[chan].adsrw_mod));

	opl3out(0xb0 + _chan_n[chan], 0);
	opl3out(_instr[4] + _chan_m[chan], NONE);
	opl3out(_instr[5] + _chan_c[chan], NONE);
	opl3out(_instr[6] + _chan_m[chan], NONE);
	opl3out(_instr[7] + _chan_c[chan], NONE);

	key_off(chan);
	event_table[chan].instr_def = 0;
	reset_chan[chan] = TRUE;
}

static uint8_t scale_volume(uint8_t volume, uint8_t scale_factor)
{
	return 63 - ((63 - volume) *
		(63 - scale_factor) / 63);
}

static void set_ins_volume(uint8_t modulator, uint8_t carrier, uint8_t chan)
{
	uint8_t temp;

	if (modulator != NONE) {
		temp = modulator;

		if (volume_scaling)
			if (((ins_parameter(voice_table[chan], 10) & 1) == 1) ||
			    (percussion_mode && (chan >= 17 && chan <= 20)))
				modulator = scale_volume(ins_parameter(voice_table[chan], 2) & 0x3f, modulator);

		if (((ins_parameter(voice_table[chan], 10) & 1) == 1) ||
		    (percussion_mode && (chan >= 17 && chan <= 20)))
			opl3out(_instr[2] + _chan_m[chan],
				scale_volume(scale_volume(modulator, 63 - global_volume),
					     63 - overall_volume) + LO(vscale_table[chan]));
		else
			opl3out(_instr[2] + _chan_m[chan],
				temp + LO(vscale_table[chan]));

		volume_table[chan] = concw(temp, HI(volume_table[chan]));

		if (((ins_parameter(voice_table[chan],10) & 1) == 1) ||
		    (percussion_mode && (chan >= 17 && chan <= 20)))
			modulator_vol[chan] = 63 - scale_volume(modulator, 63 - global_volume);
		else
			modulator_vol[chan] = 63 - modulator;
	}

	if (carrier != NONE) {
	      temp = carrier;
	      if (volume_scaling)
		carrier = scale_volume(ins_parameter(voice_table[chan], 3) & 0x3f, carrier);

	      opl3out(_instr[3] + _chan_c[chan],
		      scale_volume(scale_volume(carrier, 63 - global_volume),
				   63 - overall_volume) + HI(vscale_table[chan]));

	      volume_table[chan] = concw(LO(volume_table[chan]), temp);
	      carrier_vol[chan] = 63 - scale_volume(carrier, 63 - global_volume);
	}
}

static void reset_ins_volume(uint8_t chan)
{
	if (!volume_scaling) {
		set_ins_volume(ins_parameter(voice_table[chan], 2) & 0x3f,
			       ins_parameter(voice_table[chan], 3) & 0x3f, chan);
	} else {
		if ((ins_parameter(voice_table[chan], 10) & 1) == 0) {
			set_ins_volume(ins_parameter(voice_table[chan], 2) & 0x3f, 0, chan);
		} else {
			set_ins_volume(0, 0, chan);
		}
	}
}

static void set_global_volume()
{
	for (int chan = 0; chan < 20; chan++) {
		if (!((carrier_vol[chan] == 0) &&
		    (modulator_vol[chan] == 0))) {
			if ((ins_parameter(voice_table[chan], 10) & 1) == 0) {
				set_ins_volume(NONE, HI(volume_table[chan]), chan);
			} else {
				set_ins_volume(LO(volume_table[chan]), HI(volume_table[chan]), chan);
			}
		}
	}
}

void set_overall_volume(unsigned char level)
{
	overall_volume = max(level, 63);
	set_global_volume();
}

// FIXME: check ins
static void init_macro_table(uint8_t chan, uint8_t note, uint8_t ins, uint16_t freq)
{
	macro_table[chan].fmreg_count = 1;
	macro_table[chan].fmreg_pos = 0;
	macro_table[chan].fmreg_duration = 0;
	macro_table[chan].fmreg_table = ins;
	macro_table[chan].arpg_count = 1;
	macro_table[chan].arpg_pos = 0;
	macro_table[chan].arpg_table =
		songdata->instr_macros[ins].arpeggio_table;
	macro_table[chan].arpg_note = note;
	macro_table[chan].vib_count = 1;
	macro_table[chan].vib_pos = 0;
	macro_table[chan].vib_table =
		songdata->instr_macros[ins].vibrato_table;
	macro_table[chan].vib_freq = freq;
	macro_table[chan].vib_delay =
		songdata->macro_table[macro_table[chan].vib_table].vibrato.delay;
}

static void set_ins_data(uint8_t ins, uint8_t chan)
{
	uint8_t old_ins;

	if ((ins != event_table[chan].instr_def) || reset_chan[chan]) {
		opl3out(_instr[2] + _chan_m[chan], 63);
		opl3out(_instr[3] + _chan_c[chan], 63);

		if (!pan_lock[chan]) {
			panning_table[chan] = ins_parameter(ins, 11);
		} else {
			panning_table[chan] = songdata->lock_flags[chan] & 3;
		}

		opl3out(_instr[0] + _chan_m[chan], ins_parameter(ins, 0));
		opl3out(_instr[1] + _chan_c[chan], ins_parameter(ins, 1));
		opl3out(_instr[4] + _chan_m[chan], ins_parameter(ins, 4));
		opl3out(_instr[5] + _chan_c[chan], ins_parameter(ins, 5));
		opl3out(_instr[6] + _chan_m[chan], ins_parameter(ins, 6));
		opl3out(_instr[7] + _chan_c[chan], ins_parameter(ins, 7));
		opl3out(_instr[8] + _chan_m[chan], ins_parameter(ins, 8));
		opl3out(_instr[9] + _chan_c[chan], ins_parameter(ins, 9));
		opl3out(_instr[10] + _chan_n[chan], ins_parameter(ins, 10) |
			_panning[panning_table[chan]]);

		fmpar_table[chan].connect = ins_parameter(ins, 10) & 1;
		fmpar_table[chan].feedb   = (ins_parameter(ins, 10) >> 1) & 7;
		fmpar_table[chan].multipM = ins_parameter(ins, 0)  & 0x0f;
		fmpar_table[chan].kslM    = ins_parameter(ins, 2)  >> 6;
		fmpar_table[chan].tremM   = ins_parameter(ins, 0)  >> 7;
		fmpar_table[chan].vibrM   = (ins_parameter(ins, 0)  >> 6) & 1;
		fmpar_table[chan].ksrM    = (ins_parameter(ins, 0)  >> 4) & 1;
		fmpar_table[chan].sustM   = (ins_parameter(ins, 0)  >> 5) & 1;
		fmpar_table[chan].multipC = ins_parameter(ins, 1)  & 0x0f;
		fmpar_table[chan].kslC    = ins_parameter(ins, 3)  >> 6;
		fmpar_table[chan].tremC   = ins_parameter(ins, 1)  >> 7;
		fmpar_table[chan].vibrC   = (ins_parameter(ins, 1)  >> 6) & 1;
		fmpar_table[chan].ksrC    = (ins_parameter(ins, 1)  >> 4) & 1;
		fmpar_table[chan].sustC   = (ins_parameter(ins, 1)  >> 5) & 1;

		fmpar_table[chan].adsrw_car.attck = ins_parameter(ins, 5) >> 4;
		fmpar_table[chan].adsrw_mod.attck = ins_parameter(ins, 4) >> 4;
		fmpar_table[chan].adsrw_car.dec   = ins_parameter(ins, 5) & 0x0f;
		fmpar_table[chan].adsrw_mod.dec   = ins_parameter(ins, 4) & 0x0f;
		fmpar_table[chan].adsrw_car.sustn = ins_parameter(ins, 7) >> 4;
		fmpar_table[chan].adsrw_mod.sustn = ins_parameter(ins, 6) >> 4;
		fmpar_table[chan].adsrw_car.rel   = ins_parameter(ins, 7) & 0x0f;
		fmpar_table[chan].adsrw_mod.rel   = ins_parameter(ins, 6) & 0x0f;
		fmpar_table[chan].adsrw_car.wform = ins_parameter(ins, 9) & 0x07;
		fmpar_table[chan].adsrw_mod.wform = ins_parameter(ins, 8) & 0x07;

		if (!reset_chan[chan])
			keyoff_loop[chan] = FALSE;

		if (reset_chan[chan]) {
			voice_table[chan] = ins;
			reset_ins_volume(chan);
			reset_chan[chan] = FALSE;
		}

		if ((event_table[chan].note & 0x7f) > 0 &&
		    (event_table[chan].note & 0x7f) < 12 * 8 + 1) {
			init_macro_table(chan, event_table[chan].note & 0x7f, ins, freq_table[chan]);
		} else {
			init_macro_table(chan, 0, ins, freq_table[chan]);
		}

	}

	vscale_table[chan] = concw(fmpar_table[chan].kslM << 6,
                              fmpar_table[chan].kslC << 6);
	voice_table[chan] = ins;
	old_ins = event_table[chan].instr_def;
	event_table[chan].instr_def = ins;

	if (!volume_lock[chan] || (ins != old_ins))
		reset_ins_volume(chan);
}

static void update_modulator_adsrw(uint8_t chan)
{
	opl3out(_instr[4] + _chan_m[chan],
		(fmpar_table[chan].adsrw_mod.attck << 4) +
		fmpar_table[chan].adsrw_mod.dec);
	opl3out(_instr[6] + _chan_m[chan],
		(fmpar_table[chan].adsrw_mod.sustn << 4) +
		fmpar_table[chan].adsrw_mod.rel);
	opl3out(_instr[8] + _chan_m[chan],
		fmpar_table[chan].adsrw_mod.wform);
}

static void update_carrier_adsrw(uint8_t chan)
{
	opl3out(_instr[5] + _chan_c[chan],
		(fmpar_table[chan].adsrw_car.attck << 4) +
		fmpar_table[chan].adsrw_car.dec);
	opl3out(_instr[7] + _chan_c[chan],
		(fmpar_table[chan].adsrw_car.sustn << 4) +
		fmpar_table[chan].adsrw_car.rel);
	opl3out(_instr[9] + _chan_c[chan],
		fmpar_table[chan].adsrw_car.wform);
}

static void update_fmpar(uint8_t chan)
{
	opl3out(_instr[0] + _chan_m[chan],
		fmpar_table[chan].multipM +
		(fmpar_table[chan].ksrM << 4) +
		(fmpar_table[chan].sustM << 5) +
		(fmpar_table[chan].vibrM << 6) +
		(fmpar_table[chan].tremM << 7));

	opl3out(_instr[1] + _chan_c[chan],
		fmpar_table[chan].multipC +
		(fmpar_table[chan].ksrC << 4) +
		(fmpar_table[chan].sustC << 5) +
		(fmpar_table[chan].vibrC << 6) +
		(fmpar_table[chan].tremC << 7));

	opl3out(_instr[10] + _chan_n[chan],
		(fmpar_table[chan].connect +
		(fmpar_table[chan].feedb << 1)) |
		_panning[panning_table[chan]]);

	vscale_table[chan] = concw(fmpar_table[chan].kslM << 6,
				   fmpar_table[chan].kslC << 6);

	set_ins_volume(LO(volume_table[chan]),
		       HI(volume_table[chan]), chan);
}

static void output_note(uint8_t note, uint8_t ins, uint8_t chan, bool restart_macro)
{
	uint16_t freq;

	if ((note == 0) && (ftune_table[chan] == 0)) return;

	if ((note == 0) || (note > 12*8+1)) { // If NOT (note in [1..12*8+1])
		freq = freq_table[chan];
	} else {
		freq = nFreq(note - 1) + (int8_t)ins_parameter(ins, 12);
		opl3out(0xb0 + _chan_n[chan], 0);
		freq_table[chan] = concw(LO(freq_table[chan]),
					  HI(freq_table[chan]) | 0x20);
	}

	if (ftune_table[chan] == -127)
		ftune_table[chan] = 0;

	freq = freq + ftune_table[chan];
	change_frequency(chan, freq);

	if (note != 0) {
		event_table[chan].note = note;

		if (restart_macro) {
			if (!(((event_table[chan].effect_def == ef_Extended) &&
			      (event_table[chan].effect / 16 == ef_ex_ExtendedCmd) &&
			      (event_table[chan].effect % 16 == ef_ex_cmd_NoRestart)) ||
			      ((event_table[chan].effect_def2 == ef_Extended) &&
			      (event_table[chan].effect2 / 16 == ef_ex_ExtendedCmd) &&
			      (event_table[chan].effect2 % 16 == ef_ex_cmd_NoRestart)))) {
				init_macro_table(chan, note, ins, freq);
			} else {
				macro_table[chan].arpg_note = note;
			}
		}
	}
}

static void output_note_NR(uint8_t note, uint8_t ins, uint8_t chan, bool restart_macro)
{
	uint16_t freq;

	if ((note == 0) && (ftune_table[chan] == 0)) return;

	if ((note == 0) || (note > 12*8+1)) { // If NOT (note in [1..12*8+1])
		freq = freq_table[chan];
	} else {
		freq = nFreq(note - 1) + (int8_t)ins_parameter(ins, 12);
		freq_table[chan] = concw(LO(freq_table[chan]),
					  HI(freq_table[chan]) | 0x20);
	}

	if (ftune_table[chan] == -127)
		ftune_table[chan] = 0;


	freq = freq + ftune_table[chan];
	change_frequency(chan, freq);

	if (note != 0) {
		event_table[chan].note = note;

		if (restart_macro) {
			if (!(((event_table[chan].effect_def == ef_Extended) &&
			      (event_table[chan].effect / 16 == ef_ex_ExtendedCmd) &&
			      (event_table[chan].effect % 16 == ef_ex_cmd_NoRestart)) ||
			      ((event_table[chan].effect_def2 == ef_Extended) &&
			      (event_table[chan].effect2 / 16 == ef_ex_ExtendedCmd) &&
			      (event_table[chan].effect2 % 16 == ef_ex_cmd_NoRestart)))) {
				init_macro_table(chan, note, ins, freq);
			} else {
				macro_table[chan].arpg_note = note;
			}
		}
	}
}

static void update_fine_effects(uint8_t chan); // forward

static bool no_loop(uint8_t current_chan, uint8_t current_line)
{
	for (int chan = 0; chan < current_chan; chan++) {
		if ((loop_table[chan][current_line] != 0) &&
		    (loop_table[chan][current_line] != NONE))
			return FALSE;
	}

	return TRUE;
}

static void play_line()
{
	tADTRACK2_EVENT event;
	uint8_t eLo, eHi, eLo2, eHi2;

	for (int chan = 0; chan < songdata->nm_tracks; chan++) {
		memcpy(&event,
		       &pattdata[current_pattern].ch[chan].row[current_line].ev,
		       sizeof(event));
		//event = pattdata^[current_pattern DIV 8]
		//                  [current_pattern MOD 8][chan][current_line];

		if (effect_table[chan] != 0)
			last_effect[chan] = effect_table[chan];
		effect_table[chan] = effect_table[chan] & 0xff00;

		if (effect_table2[chan] != 0)
			last_effect2[chan] = effect_table2[chan];
		effect_table2[chan] = effect_table2[chan] & 0xff00;

		ftune_table[chan] = 0;

		if (event.note == NONE) {
			event.note = event_table[chan].note | keyoff_flag;
		} else {
			if((event.note >= fixed_note_flag + 1) &&
			   (event.note <= fixed_note_flag + 12 * 8 + 1))
				event.note = event.note - fixed_note_flag;
		}

		if ((event.note != 0) ||
		   (event.effect_def != 0) ||
		   (event.effect_def2 != 0) ||
		  ((event.effect_def == 0) && (event.effect != 0)) ||
		  ((event.effect_def2 == 0) && (event.effect2 != 0))) {
			event_new[chan] = TRUE;
		} else {
			event_new[chan] = FALSE;
		}

		if ((event.note != 0) || (event.instr_def != 0)) {
			event_table[chan].effect_def  = event.effect_def;
			event_table[chan].effect      = event.effect;
			event_table[chan].effect_def2 = event.effect_def2;
			event_table[chan].effect2     = event.effect2;
		}

		if (event.instr_def != 0) {
			// NOTE: adjust ins
			if (!nul_data(songdata->instr_data[event.instr_def-1], INSTRUMENT_SIZE)) {
				set_ins_data(event.instr_def, chan);
			} else {
				release_sustaining_sound(chan);
				set_ins_data(event.instr_def, chan);
			}
		}

		if ((event.effect_def != ef_Vibrato) &&
		    (event.effect_def != ef_ExtraFineVibrato) &&
		    (event.effect_def != ef_VibratoVolSlide) &&
		    (event.effect_def != ef_VibratoVSlideFine))
			memset(&vibr_table[chan], 0, sizeof(vibr_table[chan]));

		if ((event.effect_def2 != ef_Vibrato) &&
		    (event.effect_def2 != ef_ExtraFineVibrato) &&
		    (event.effect_def2 != ef_VibratoVolSlide) &&
		    (event.effect_def2 != ef_VibratoVSlideFine))
			memset(&vibr_table2[chan], 0, sizeof(vibr_table2[chan]));

		if ((event.effect_def != ef_RetrigNote) &&
		    (event.effect_def != ef_MultiRetrigNote))
			memset(&retrig_table[chan], 0, sizeof(retrig_table[chan]));

		if ((event.effect_def2 != ef_RetrigNote) &&
		    (event.effect_def2 != ef_MultiRetrigNote))
			memset(&retrig_table2[chan], 0, sizeof(retrig_table2[chan]));

		if ((event.effect_def != ef_Tremolo) &&
		    (event.effect_def != ef_ExtraFineTremolo))
			memset(&trem_table[chan], 0, sizeof(trem_table[chan]));

		if ((event.effect_def2 != ef_Tremolo) &&
		    (event.effect_def2 != ef_ExtraFineTremolo))
			memset(&trem_table2[chan], 0, sizeof(trem_table2[chan]));

		eLo  = LO(last_effect[chan]);
		eHi  = HI(last_effect[chan]);
		eLo2 = LO(last_effect2[chan]);
		eHi2 = HI(last_effect2[chan]);

		if ((arpgg_table[chan].state != 1) &&
		    (event.effect_def != ef_ExtraFineArpeggio)) {
			arpgg_table[chan].state = 1;
			change_frequency(chan, nFreq(arpgg_table[chan].note - 1) +
			(int8_t)ins_parameter(event_table[chan].instr_def, 12));
		}

		if ((arpgg_table2[chan].state != 1) &&
		    (event.effect_def2 != ef_ExtraFineArpeggio)) {
			arpgg_table2[chan].state = 1;
			change_frequency(chan, nFreq(arpgg_table2[chan].note - 1) +
			(int8_t)ins_parameter(event_table[chan].instr_def, 12));
		}

		if ((arpgg_table2[chan].state != 1) &&
		    (event.effect_def2 != ef_ExtraFineArpeggio)) {
			arpgg_table2[chan].state = 1;
			change_frequency(chan, nFreq(arpgg_table2[chan].note - 1) +
			(int8_t)ins_parameter(event_table[chan].instr_def, 12));
		}

		if ((tremor_table[chan].pos != 0) &&
		    (event.effect_def != ef_Tremor)) {
			tremor_table[chan].pos = 0;
			set_ins_volume(LO(tremor_table[chan].volume),
				       HI(tremor_table[chan].volume), chan);
		}

		if ((tremor_table2[chan].pos != 0) &&
		    (event.effect_def2 != ef_Tremor)) {
			tremor_table2[chan].pos = 0;
			set_ins_volume(LO(tremor_table2[chan].volume),
				       HI(tremor_table2[chan].volume), chan);
		}

		if (!(pattern_break && ((next_line & 0xf0) == pattern_loop_flag)) &&
			(current_order != last_order)) {
			memset(loopbck_table, NONE, sizeof(loopbck_table));
			memset(loop_table, NONE, sizeof(loop_table));
			last_order = current_order;
		}

		switch (event.effect_def) {
		case ef_Arpeggio:
		case ef_ExtraFineArpeggio:
		case ef_ArpggVSlide:
		case ef_ArpggVSlideFine:
			if ((event.effect_def != ef_Arpeggio) ||
			    (event.effect != 0)) {
				switch (event.effect_def) {
				case ef_Arpeggio:
					effect_table[chan] =
						concw(ef_Arpeggio + ef_fix1, event.effect);
					break;
				case ef_ExtraFineArpeggio:
					effect_table[chan] =
						concw(ef_ExtraFineArpeggio, event.effect);
					break;
				case ef_ArpggVSlide:
				case ef_ArpggVSlideFine:
					if (event.effect != 0) {
						effect_table[chan] =
							concw(event.effect_def, event.effect);
					} else {
						if (((eLo == ef_ArpggVSlide) || (eLo == ef_ArpggVSlideFine)) &&
						    (eHi != 0)) {
							effect_table[chan] = concw(event.effect_def, eHi);
						} else {
							effect_table[chan] = effect_table[chan] && 0xff00;
						}
					}
				       break;
				}

				if (((event.note & 0x7f) >= 1) &&
				    ((event.note & 0x7f) <= 12 * 8 + 1)) {
					arpgg_table[chan].state = 0;
					arpgg_table[chan].note = event.note & 0x7f;
					if ((event.effect_def == ef_Arpeggio) ||
					    (event.effect_def == ef_ExtraFineArpeggio)) {
						arpgg_table[chan].add1 = event.effect / 16;
						arpgg_table[chan].add2 = event.effect % 16;
					}
				} else {
					if ((event.note == 0) &&
					  (((event_table[chan].note & 0x7f) >= 1) &&
					    (event_table[chan].note & 0x7f) <= 12 * 8 + 1)) {
						if ((eLo != ef_Arpeggio + ef_fix1) &&
						    (eLo != ef_ExtraFineArpeggio) &&
						    (eLo != ef_ArpggVSlide) &&
						    (eLo != ef_ArpggVSlideFine))
							arpgg_table[chan].state = 0;

						arpgg_table[chan].note = event_table[chan].note & 0x7f;
						if ((event.effect_def == ef_Arpeggio) ||
						    (event.effect_def == ef_ExtraFineArpeggio)) {
							arpgg_table[chan].add1 = event.effect / 16;
							arpgg_table[chan].add2 = event.effect % 16;
						}
					} else {
						effect_table[chan] = 0;
					}
				}
			}
			break;

		case ef_FSlideUp:
		case ef_FSlideDown:
		case ef_FSlideUpFine:
		case ef_FSlideDownFine:
			effect_table[chan] = concw(event.effect_def, event.effect);
			fslide_table[chan] = event.effect;
			break;

		case ef_FSlideUpVSlide:
		case ef_FSlUpVSlF:
		case ef_FSlideDownVSlide:
		case ef_FSlDownVSlF:
		case ef_FSlUpFineVSlide:
		case ef_FSlUpFineVSlF:
		case ef_FSlDownFineVSlide:
		case ef_FSlDownFineVSlF:
			if (event.effect != 0) {
				effect_table[chan] = concw(event.effect_def, event.effect);
			} else {
				if (((eLo == ef_FSlideUpVSlide) ||
				     (eLo == ef_FSlUpVSlF) ||
				     (eLo == ef_FSlideDownVSlide) ||
				     (eLo == ef_FSlDownVSlF) ||
				     (eLo == ef_FSlUpFineVSlide) ||
				     (eLo == ef_FSlUpFineVSlF) ||
				     (eLo == ef_FSlDownFineVSlide) ||
				     (eLo == ef_FSlDownFineVSlF)) &&
				     (eHi != 0)) {
					effect_table[chan] = concw(event.effect_def, eHi);
				} else {
					effect_table[chan] = effect_table[chan] & 0xff00;
				}
			}
			break;

		case ef_TonePortamento:
			if ((event.note >= 1) && (event.note <= 12 * 8 + 1)) {

				if (event.effect != 0) {
					effect_table[chan] =
						concw(ef_TonePortamento, event.effect);
				} else {
					if ((eLo == ef_TonePortamento) && (eHi != 0)) {
						effect_table[chan] =
							concw(ef_TonePortamento, eHi);
					} else {
						effect_table[chan] = ef_TonePortamento;
					}
				}

				porta_table[chan].speed = HI(effect_table[chan]);
				porta_table[chan].freq = nFreq(event.note - 1) +
					(int8_t)ins_parameter(event_table[chan].instr_def, 12);
			} else {
				if (eLo == ef_TonePortamento) {
					if (event.effect != 0) {
						effect_table[chan] =
							concw(ef_TonePortamento, event.effect);
					} else {
						if ((eLo == ef_TonePortamento) && (eHi != 0)) {
							effect_table[chan] = concw(ef_TonePortamento, eHi);
						} else {
							effect_table[chan] = ef_TonePortamento;
						}
					}
					porta_table[chan].speed = HI(effect_table[chan]);
				}
			}
			break;

		case ef_TPortamVolSlide:
		case ef_TPortamVSlideFine:
			if (event.effect != 0) {
				effect_table[chan] = concw(event.effect_def, event.effect);
			} else {
				if (((eLo == ef_TPortamVolSlide) ||
				     (eLo == ef_TPortamVSlideFine)) &&
				     (eHi != 0)) {
					effect_table[chan] = concw(event.effect_def, eHi);
				} else {
					effect_table[chan] = effect_table[chan] & 0xff00;
				}
			}
			break;

		case ef_Vibrato:
		case ef_ExtraFineVibrato:
			if (event.effect != 0) {
				effect_table[chan] =
					concw(event.effect_def, event.effect);
			} else {
				if (((eLo == ef_Vibrato) ||
				     (eLo == ef_ExtraFineVibrato)) &&
				     (eHi != 0)) {
					effect_table[chan] = concw(event.effect_def, eHi);
				} else {
					effect_table[chan] = event.effect_def;
				}
			}

			if ((event.effect_def2 == ef_Extended) &&
			    (event.effect2 == ef_ex_ExtendedCmd * 16 + ef_ex_cmd_FineVibr)) {
				vibr_table[chan].fine = TRUE;
			}

			vibr_table[chan].speed = HI(effect_table[chan]) / 16;
			vibr_table[chan].depth = HI(effect_table[chan]) % 16;
			break;

		case ef_Tremolo:
		case ef_ExtraFineTremolo:
			if (event.effect != 0) {
				effect_table[chan] =
					concw(event.effect_def, event.effect);
			} else {
				if (((eLo == ef_Tremolo) ||
				     (eLo == ef_ExtraFineTremolo)) &&
				     (eHi != 0)) {
					effect_table[chan] = concw(event.effect_def, eHi);
				} else {
					effect_table[chan] = event.effect_def;
				}
			}

			if ((event.effect_def2 == ef_Extended) &&
			    (event.effect2 == ef_ex_ExtendedCmd * 16 + ef_ex_cmd_FineTrem)) {
				trem_table[chan].fine = TRUE;
			}

			trem_table[chan].speed = HI(effect_table[chan]) / 16;
			trem_table[chan].depth = HI(effect_table[chan]) % 16;
			break;

		case ef_VibratoVolSlide:
		case ef_VibratoVSlideFine:
			if (event.effect != 0) {
				effect_table[chan] = concw(event.effect_def, event.effect);
			} else {
				if (((eLo ==  ef_VibratoVolSlide) ||
				     (eLo ==  ef_VibratoVSlideFine)) &&
				     (HI(effect_table[chan]) != 0)) {
					effect_table[chan] = concw(event.effect_def, HI(effect_table[chan]));
				} else {
					effect_table[chan] = effect_table[chan] & 0xff00;
				}
			}

			if ((event.effect_def2 == ef_Extended) &&
			    (event.effect2 == ef_ex_ExtendedCmd * 16 + ef_ex_cmd_FineVibr))
				vibr_table[chan].fine = TRUE;
			break;

		case ef_SetCarrierVol:
			set_ins_volume(NONE, 63 - event.effect, chan);
			break;

		case ef_SetModulatorVol:
			set_ins_volume(63 - event.effect, NONE, chan);
			break;

		case ef_SetInsVolume:
			if (percussion_mode && ((chan >= 16) && (chan <= 19))) { //  in [17..20]
				set_ins_volume(63 - event.effect, NONE, chan);
			} else {
				if ((ins_parameter(voice_table[chan], 10) & 1) == 0) {
					set_ins_volume(NONE, 63 - event.effect, chan);
				} else {
					set_ins_volume(63 - event.effect, 63 - event.effect, chan);
				}
			}
			break;

		case ef_ForceInsVolume:
			if (percussion_mode && ((chan >= 16) && (chan <= 19))) { //  in [17..20]
				set_ins_volume(63 - event.effect, NONE, chan);
			} else {
				set_ins_volume(scale_volume(ins_parameter(voice_table[chan], 2) & 0x3f,
					       63 - event.effect), 63 - event.effect, chan);
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
				next_line = max(event.effect, songdata->patt_len - 1);
			}
			break;

		case ef_SetSpeed:
			speed = event.effect;
			break;

		case ef_SetTempo:
			update_timer(event.effect);
			break;

		case ef_SetWaveform:
			if (event.effect / 16 <= 7) { // in [0..7]
				fmpar_table[chan].adsrw_car.wform = event.effect / 16;
				update_carrier_adsrw(chan);
			}

			if (event.effect % 16 <= 7) { // in [0..7]
				fmpar_table[chan].adsrw_mod.wform = event.effect % 16;
				update_modulator_adsrw(chan);
			}
			break;

		case ef_VolSlide:
			effect_table[chan] = concw(ef_VolSlide, event.effect);
			break;

		case ef_VolSlideFine:
			effect_table[chan] = concw(ef_VolSlideFine, event.effect);
			break;

		case ef_RetrigNote:
			if (event.effect != 0) {
				if ((eLo != ef_RetrigNote) &&
				    (eLo != ef_MultiRetrigNote))
					retrig_table[chan] = 1;

				effect_table[chan] = concw(ef_RetrigNote, event.effect);
			}
			break;

		case ef_SetGlobalVolume:
			global_volume = event.effect;
			set_global_volume();
			break;

		case ef_MultiRetrigNote:
			if (event.effect / 16 != 0) {
				if ((eLo != ef_RetrigNote) &&
				    (eLo != ef_MultiRetrigNote))
					retrig_table[chan] = 1;

				effect_table[chan] = concw(ef_MultiRetrigNote, event.effect);
			}
			break;

		case ef_Tremor:
			if ((event.effect / 16 != 0) &&
			    (event.effect % 16 != 0)) {
				if (eLo != ef_Tremor) {
					tremor_table[chan].pos = 0;
					tremor_table[chan].volume = volume_table[chan];
				}
				effect_table[chan] = concw(ef_Tremor, event.effect);
			}
			break;

		case ef_Extended:
			switch (event.effect / 16) {
			case ef_ex_SetTremDepth:
				switch (event.effect % 16) {
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
				switch (event.effect % 16) {
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
				fmpar_table[chan].adsrw_mod.attck = event.effect % 16;
				update_modulator_adsrw(chan);
				break;

			case ef_ex_SetDecayRateM:
				fmpar_table[chan].adsrw_mod.dec = event.effect % 16;
				update_modulator_adsrw(chan);
				break;

			case ef_ex_SetSustnLevelM:
				fmpar_table[chan].adsrw_mod.sustn = event.effect % 16;
				update_modulator_adsrw(chan);
				break;

			case ef_ex_SetRelRateM:
				fmpar_table[chan].adsrw_mod.rel = event.effect % 16;
				update_modulator_adsrw(chan);
				break;

			case ef_ex_SetAttckRateC:
				fmpar_table[chan].adsrw_car.attck = event.effect % 16;
				update_carrier_adsrw(chan);
				break;

			case ef_ex_SetDecayRateC:
				fmpar_table[chan].adsrw_car.dec = event.effect % 16;
				update_carrier_adsrw(chan);
				break;

			case ef_ex_SetSustnLevelC:
				fmpar_table[chan].adsrw_car.sustn = event.effect % 16;
				update_carrier_adsrw(chan);
				break;

			case ef_ex_SetRelRateC:
				fmpar_table[chan].adsrw_car.rel = event.effect % 16;
				update_carrier_adsrw(chan);
				break;

			case ef_ex_SetFeedback:
				fmpar_table[chan].feedb = event.effect % 16;
				update_fmpar(chan);
				break;

			case ef_ex_SetPanningPos:
				panning_table[chan] = event.effect % 16;
				update_fmpar(chan);
				break;

			case ef_ex_PatternLoop:
			case ef_ex_PatternLoopRec:
				if (event.effect % 16 == 0) {
					loopbck_table[chan] = current_line;
				} else {
					if (loopbck_table[chan] != NONE) {
						if (loop_table[chan][current_line] == NONE)
							loop_table[chan][current_line] = event.effect % 16;

						if (loop_table[chan][current_line] != 0) {
							pattern_break = TRUE;
							next_line = pattern_loop_flag + chan;
						} else {
							if (event.effect / 16 == ef_ex_PatternLoopRec)
								loop_table[chan][current_line] = NONE;
						}
					}
				}
				break;

			case ef_ex_MacroKOffLoop:
				if (event.effect % 16 != 0) {
					keyoff_loop[chan] = TRUE;
				} else {
					keyoff_loop[chan] = FALSE;
				}
				break;

			case ef_ex_ExtendedCmd:
				switch (event.effect % 16) {
				case ef_ex_cmd_RSS:        release_sustaining_sound(chan); break;
				case ef_ex_cmd_ResetVol:   reset_ins_volume(chan); break;
				case ef_ex_cmd_LockVol:    volume_lock  [chan] = TRUE; break;
				case ef_ex_cmd_UnlockVol:  volume_lock  [chan] = FALSE; break;
				case ef_ex_cmd_LockVP:     peak_lock    [chan] = TRUE; break;
				case ef_ex_cmd_UnlockVP:   peak_lock    [chan] = FALSE; break;
				case ef_ex_cmd_VSlide_def: volslide_type[chan] = 0; break;
				case ef_ex_cmd_LockPan:    pan_lock     [chan] = TRUE; break;
				case ef_ex_cmd_UnlockPan:  pan_lock     [chan] = FALSE; break;
				case ef_ex_cmd_VibrOff:    change_frequency(chan, freq_table[chan]); break;
				case ef_ex_cmd_TremOff:
					set_ins_volume(LO(volume_table[chan]),
						       HI(volume_table[chan]), chan);
					break;
				case ef_ex_cmd_VSlide_car:
					if ((event.effect_def2 == ef_Extended) &&
					    (event.effect2 == ef_ex_ExtendedCmd * 16 +
							      ef_ex_cmd_VSlide_mod)) {
						volslide_type[chan] = 3;
					} else {
						volslide_type[chan] = 1;
					}
					break;

				case ef_ex_cmd_VSlide_mod:
					if ((event.effect_def2 == ef_Extended) &&
					    (event.effect2 == ef_ex_ExtendedCmd * 16 +
							      ef_ex_cmd_VSlide_car)) {
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
			switch (event.effect / 16) {
			case ef_ex2_PatDelayFrame:
			case ef_ex2_PatDelayRow:
				pattern_delay = TRUE;
				if (event.effect / 16 == ef_ex2_PatDelayFrame) {
					tickD = event.effect % 16;
				} else {
					tickD = speed * (event.effect % 16);
				}
				break;

			case ef_ex2_NoteDelay:
				effect_table[chan] = concw(ef_Extended2 + ef_fix2 + ef_ex2_NoteDelay, 0);
				notedel_table[chan] = event.effect % 16;
				break;

			case ef_ex2_NoteCut:
				effect_table[chan] = concw(ef_Extended2 + ef_fix2 + ef_ex2_NoteCut, 0);
				notecut_table[chan] = event.effect % 16;
				break;

			case ef_ex2_FineTuneUp:
				ftune_table[chan] += event.effect % 16;
				break;

			case ef_ex2_FineTuneDown:
				ftune_table[chan] -= event.effect % 16;
				break;

			case ef_ex2_GlVolSlideUp:
				effect_table[chan] =
					concw(ef_Extended2 + ef_fix2 + ef_ex2_GlVolSlideUp,
						event.effect % 16);
				break;

			case ef_ex2_GlVolSlideDn:
				effect_table[chan] =
					concw(ef_Extended2 + ef_fix2 + ef_ex2_GlVolSlideDn,
						event.effect % 16);
				break;

			case ef_ex2_GlVolSlideUpF:
				effect_table[chan] =
					concw(ef_Extended2 + ef_fix2 + ef_ex2_GlVolSlideUpF,
						event.effect % 16);
				break;

			case ef_ex2_GlVolSlideDnF:
				effect_table[chan] = 
					concw(ef_Extended2 + ef_fix2 + ef_ex2_GlVolSlideDnF,
						event.effect % 16);
				break;

			case ef_ex2_GlVolSldUpXF:
				effect_table[chan] =
					concw(ef_Extended2 + ef_fix2 + ef_ex2_GlVolSldUpXF,
                                          event.effect % 16);
				break;

			case ef_ex2_GlVolSldDnXF:
				effect_table[chan] =
					concw(ef_Extended2 + ef_fix2 + ef_ex2_GlVolSldDnXF,
						event.effect % 16);
				break;

			case ef_ex2_VolSlideUpXF:
				effect_table[chan] =
					concw(ef_Extended2 + ef_fix2 + ef_ex2_VolSlideUpXF,
						event.effect % 16);
				break;

			case ef_ex2_VolSlideDnXF:
				effect_table[chan] =
					concw(ef_Extended2 + ef_fix2 + ef_ex2_VolSlideDnXF,
						event.effect % 16);
				break;

			case ef_ex2_FreqSlideUpXF:
				effect_table[chan] =
					concw(ef_Extended2 + ef_fix2 + ef_ex2_FreqSlideUpXF,
						event.effect % 16);
				break;

			case ef_ex2_FreqSlideDnXF:
				effect_table[chan] =
					concw(ef_Extended2 + ef_fix2 + ef_ex2_FreqSlideDnXF,
						event.effect % 16);
				break;
			}
			break;

		case ef_Extended3:
			switch  (event.effect / 16) {
			case ef_ex3_SetConnection:
				fmpar_table[chan].connect = event.effect % 16;
				update_fmpar(chan);
				break;

			case ef_ex3_SetMultipM:
				fmpar_table[chan].multipM = event.effect % 16;
				update_fmpar(chan);
				break;

			case ef_ex3_SetKslM:
				fmpar_table[chan].kslM = event.effect % 16;
				update_fmpar(chan);
				break;

			case ef_ex3_SetTremoloM:
				fmpar_table[chan].tremM = event.effect % 16;
				update_fmpar(chan);
				break;

			case ef_ex3_SetVibratoM:
				fmpar_table[chan].vibrM = event.effect % 16;
				update_fmpar(chan);
				break;

			case ef_ex3_SetKsrM:
				fmpar_table[chan].ksrM = event.effect % 16;
				update_fmpar(chan);
				break;

			case ef_ex3_SetSustainM:
				fmpar_table[chan].sustM = event.effect % 16;
				update_fmpar(chan);
				break;

			case ef_ex3_SetMultipC:
				fmpar_table[chan].multipC = event.effect % 16;
				update_fmpar(chan);
				break;

			case ef_ex3_SetKslC:
				fmpar_table[chan].kslC = event.effect % 16;
				update_fmpar(chan);
				break;

			case ef_ex3_SetTremoloC:
				fmpar_table[chan].tremC = event.effect % 16;
				update_fmpar(chan);
				break;

			case ef_ex3_SetVibratoC:
				fmpar_table[chan].vibrC = event.effect % 16;
				update_fmpar(chan);
				break;

			case ef_ex3_SetKsrC:
				fmpar_table[chan].ksrC = event.effect % 16;
				update_fmpar(chan);
				break;

			case ef_ex3_SetSustainC:
				fmpar_table[chan].sustC = event.effect % 16;
				update_fmpar(chan);
				break;
			}
			break;
		}

		switch (event.effect_def2) {
		case ef_Arpeggio:
		case ef_ExtraFineArpeggio:
		case ef_ArpggVSlide:
		case ef_ArpggVSlideFine:
			if ((event.effect_def2 != ef_Arpeggio) ||
			    (event.effect2 != 0)) {
				switch (event.effect_def2) {
				case ef_Arpeggio:
					effect_table2[chan] = concw(ef_Arpeggio + ef_fix1, event.effect2);
					break;

				case ef_ExtraFineArpeggio:
					effect_table2[chan] = concw(ef_ExtraFineArpeggio, event.effect2);
					break;

				case ef_ArpggVSlide:
				case ef_ArpggVSlideFine:
					if (event.effect2 != 0) {
						effect_table2[chan] = concw(event.effect_def2, event.effect2);
					} else {
						if (((eLo2 == ef_ArpggVSlide) || (eLo2 == ef_ArpggVSlideFine)) &&
						     (eHi2 != 0)) {
							effect_table2[chan] = concw(event.effect_def2, eHi2);
						} else {
							effect_table2[chan] = effect_table2[chan] & 0xff00;
						}
					}
				}

				if (((event.note & 0x7f) >= 1) &&
				    ((event.note & 0x7f) <= 12 * 8 + 1)) {
					arpgg_table2[chan].state = 0;
					arpgg_table2[chan].note = event.note & 0x7f;
					if ((event.effect_def2 == ef_Arpeggio) ||
					    (event.effect_def2 == ef_ExtraFineArpeggio)) {
						arpgg_table2[chan].add1 = event.effect2 / 16;
						arpgg_table2[chan].add2 = event.effect2 % 16;
					}
				} else {
					if ((event.note == 0) &&
					  (((event_table[chan].note & 0x7f) >= 1) &&
					    (event_table[chan].note & 0x7f) <= 12 * 8 + 1)) {
						if ((eLo2 != ef_Arpeggio + ef_fix1) &&
						    (eLo2 != ef_ExtraFineArpeggio) &&
						    (eLo2 != ef_ArpggVSlide) &&
						    (eLo2 != ef_ArpggVSlideFine))
							arpgg_table2[chan].state = 0;

						arpgg_table2[chan].note = event_table[chan].note & 0x7f;
						if ((event.effect_def2 == ef_Arpeggio) ||
						    (event.effect_def2 == ef_ExtraFineArpeggio)) {
							arpgg_table2[chan].add1 = event.effect2 / 16;
							arpgg_table2[chan].add2 = event.effect2 % 16;
						}
					} else {
						effect_table2[chan] = 0;
					}
				}
			}
			break;

		case ef_FSlideUp:
		case ef_FSlideDown:
		case ef_FSlideUpFine:
		case ef_FSlideDownFine:
			effect_table2[chan] = concw(event.effect_def2, event.effect2);
			fslide_table2[chan] = event.effect2;
			break;

		case ef_FSlideUpVSlide:
		case ef_FSlUpVSlF:
		case ef_FSlideDownVSlide:
		case ef_FSlDownVSlF:
		case ef_FSlUpFineVSlide:
		case ef_FSlUpFineVSlF:
		case ef_FSlDownFineVSlide:
		case ef_FSlDownFineVSlF:
			if (event.effect2 != 0) {
				effect_table2[chan] = concw(event.effect_def2, event.effect2);
			} else {
				if (((eLo2 == ef_FSlideUpVSlide) ||
				     (eLo2 == ef_FSlUpVSlF) ||
				     (eLo2 == ef_FSlideDownVSlide) ||
				     (eLo2 == ef_FSlDownVSlF) ||
				     (eLo2 == ef_FSlUpFineVSlide) ||
				     (eLo2 == ef_FSlUpFineVSlF) ||
				     (eLo2 == ef_FSlDownFineVSlide) ||
				     (eLo2 == ef_FSlDownFineVSlF)) &&
				     (eHi2 != 0)) {
					effect_table2[chan] = concw(event.effect_def2, eHi2);
				} else {
					effect_table2[chan] = effect_table2[chan] & 0xff00;
				}
			}
			break;

		case ef_TonePortamento:
			if ((event.note >= 1) && (event.note <= 12 * 8 + 1)) {

				if (event.effect2 != 0) {
					effect_table2[chan] =
						concw(ef_TonePortamento, event.effect2);
				} else {
					if ((eLo2 == ef_TonePortamento) && (eHi2 != 0)) {
						effect_table2[chan] =
							concw(ef_TonePortamento, eHi2);
					} else {
						effect_table2[chan] = ef_TonePortamento;
					}
				}

				porta_table2[chan].speed = HI(effect_table2[chan]);
				porta_table2[chan].freq = nFreq(event.note - 1) +
					(int8_t)ins_parameter(event_table[chan].instr_def, 12);
			} else {
				if (eLo2 == ef_TonePortamento) {
					if (event.effect2 != 0) {
						effect_table2[chan] =
							concw(ef_TonePortamento, event.effect2);
					} else {
						if ((eLo2 == ef_TonePortamento) && (eHi2 != 0)) {
							effect_table2[chan] = concw(ef_TonePortamento, eHi2);
						} else {
							effect_table2[chan] = ef_TonePortamento;
						}
					}
					porta_table2[chan].speed = HI(effect_table2[chan]);
				}
			}
			break;

		case ef_TPortamVolSlide:
		case ef_TPortamVSlideFine:
			if (event.effect2 != 0) {
				effect_table2[chan] = concw(event.effect_def2, event.effect2);
			} else {
				if (((eLo2 == ef_TPortamVolSlide) ||
				     (eLo2 == ef_TPortamVSlideFine)) &&
				     (eHi2 != 0)) {
					effect_table2[chan] = concw(event.effect_def2, eHi2);
				} else {
					effect_table2[chan] = effect_table2[chan] & 0xff00;
				}
			}
			break;

		case ef_Vibrato:
		case ef_ExtraFineVibrato:
			if (event.effect2 != 0) {
				effect_table2[chan] =
					concw(event.effect_def2, event.effect2);
			} else {
				if (((eLo2 == ef_Vibrato) ||
				     (eLo2 == ef_ExtraFineVibrato)) &&
				     (eHi2 != 0)) {
					effect_table2[chan] = concw(event.effect_def2, eHi2);
				} else {
					effect_table2[chan] = event.effect_def2;
				}
			}

			if ((event.effect_def == ef_Extended) &&
			    (event.effect == ef_ex_ExtendedCmd * 16 + ef_ex_cmd_FineVibr)) {
				vibr_table2[chan].fine = TRUE;
			}

			vibr_table2[chan].speed = HI(effect_table2[chan]) / 16;
			vibr_table2[chan].depth = HI(effect_table2[chan]) % 16;
			break;

		case ef_Tremolo:
		case ef_ExtraFineTremolo:
			if (event.effect2 != 0) {
				effect_table2[chan] =
					concw(event.effect_def2,event.effect2);
			} else {
				if (((eLo2 == ef_Tremolo) ||
				     (eLo2 == ef_ExtraFineTremolo)) &&
				     (eHi2 != 0)) {
					effect_table2[chan] = concw(event.effect_def2, eHi2);
				} else {
					effect_table2[chan] = event.effect_def2;
				}
			}

			if ((event.effect_def == ef_Extended) &&
			    (event.effect == ef_ex_ExtendedCmd * 16 + ef_ex_cmd_FineTrem)) {
				trem_table2[chan].fine = TRUE;
			}

			trem_table2[chan].speed = HI(effect_table2[chan]) / 16;
			trem_table2[chan].depth = HI(effect_table2[chan]) % 16;
			break;

		case ef_VibratoVolSlide:
		case ef_VibratoVSlideFine:
			if (event.effect2 != 0) {
				effect_table2[chan] = concw(event.effect_def2, event.effect2);
			} else {
				if (((eLo2 == ef_VibratoVolSlide) ||
				     (eLo2 == ef_VibratoVSlideFine)) &&
				     (HI(effect_table2[chan]) != 0)) {
					effect_table2[chan] = concw(event.effect_def2, HI(effect_table2[chan]));
				} else {
					effect_table2[chan] = effect_table2[chan] & 0xff00;
				}
			}

			if ((event.effect_def == ef_Extended) &
			    (event.effect == ef_ex_ExtendedCmd * 16 + ef_ex_cmd_FineVibr))
				vibr_table2[chan].fine = TRUE;
			break;

		case ef_SetCarrierVol:
			set_ins_volume(NONE, 63 - event.effect2,chan);
			break;

		case ef_SetModulatorVol:
			set_ins_volume(63 - event.effect2, NONE, chan);
			break;

		case ef_SetInsVolume:
			if (percussion_mode && ((chan >= 17) && (chan <= 20))) {
				set_ins_volume(63 - event.effect2, NONE, chan);
			} else {
				if ((ins_parameter(voice_table[chan], 10) & 1) == 0) {
					set_ins_volume(NONE, 63 - event.effect2,chan);
				} else {
					set_ins_volume(63 - event.effect2, 63 - event.effect2, chan);
				}
			}
			break;

		case ef_ForceInsVolume:
			if (percussion_mode && ((chan >= 17) && (chan <= 20))) {
				set_ins_volume(63 - event.effect2, NONE, chan);
			} else {
				set_ins_volume(scale_volume(ins_parameter(voice_table[chan], 2) & 0x3f,
						63 - event.effect2), 63 - event.effect2, chan);
			}
			break;

		case ef_PositionJump:
			if (no_loop(chan, current_line)) {
				pattern_break = TRUE;
				next_line = pattern_break_flag + chan;
			}
			break;

		case ef_PatternBreak:
			if (no_loop(chan,current_line)) {
				pattern_break = TRUE;
				next_line = event.effect2;
			}
			break;

		case ef_SetSpeed:
			speed = event.effect2;
			break;

		case ef_SetTempo:
			update_timer(event.effect2);
			break;

		case ef_SetWaveform:
			if (event.effect2 / 16 <= 7) { // in [0..7]
				fmpar_table[chan].adsrw_car.wform = event.effect2 / 16;
				update_carrier_adsrw(chan);
			}

			if (event.effect2 % 16 <= 7) { // in [0..7]
				fmpar_table[chan].adsrw_mod.wform = event.effect2 % 16;
				update_modulator_adsrw(chan);
			}
			break;

		case ef_VolSlide:
			effect_table2[chan] = concw(ef_VolSlide, event.effect2);
			break;

		case ef_VolSlideFine:
			effect_table2[chan] = concw(ef_VolSlideFine, event.effect2);
			break;

		case ef_RetrigNote:
			if (event.effect2 != 0) {
				if ((eLo2 != ef_RetrigNote) &&
				    (eLo2 != ef_MultiRetrigNote))
					retrig_table2[chan] = 1;

				effect_table2[chan] = concw(ef_RetrigNote, event.effect2);
			}
			break;

		case ef_SetGlobalVolume:
			global_volume = event.effect2;
			set_global_volume();
			break;

		case ef_MultiRetrigNote:
			if (event.effect2 / 16 != 0) {
				if ((eLo2 != ef_RetrigNote) &&
				    (eLo2 != ef_MultiRetrigNote))
					retrig_table2[chan] = 1;

				effect_table2[chan] = concw(ef_MultiRetrigNote, event.effect2);
			}
			break;

		case ef_Tremor:
			if ((event.effect2 / 16 != 0) &&
			    (event.effect2 % 16 != 0)) {
				if (eLo2 != ef_Tremor) {
					tremor_table2[chan].pos = 0;
					tremor_table2[chan].volume = volume_table[chan];
				}
				effect_table2[chan] = concw(ef_Tremor, event.effect2);
			}
			break;

		case ef_Extended:
			switch (event.effect2 / 16) {
			case ef_ex_SetTremDepth:
				switch (event.effect2 % 16) {
				case 0:
					opl3out(_instr[11], misc_register & 0x7f);
					current_tremolo_depth = 0;
					break;

				case 1:
					opl3out(_instr[11], misc_register | 0x80);
					current_tremolo_depth = 1;
					break;
				}
				break;

			case ef_ex_SetVibDepth:
				switch (event.effect2 % 16) {
				case 0:
					opl3out(_instr[11], misc_register & 0xbf);
					current_vibrato_depth = 0;
					break;

				case 1:
					opl3out(_instr[11], misc_register | 0x40);
					current_vibrato_depth = 1;
					break;
				}
				break;

			case ef_ex_SetAttckRateM:
				fmpar_table[chan].adsrw_mod.attck = event.effect2 % 16;
				update_modulator_adsrw(chan);
				break;

			case ef_ex_SetDecayRateM:
				fmpar_table[chan].adsrw_mod.dec = event.effect2 % 16;
				update_modulator_adsrw(chan);
				break;

			case ef_ex_SetSustnLevelM:
				fmpar_table[chan].adsrw_mod.sustn = event.effect2 % 16;
				update_modulator_adsrw(chan);
				break;

			case ef_ex_SetRelRateM:
				fmpar_table[chan].adsrw_mod.rel = event.effect2 % 16;
				update_modulator_adsrw(chan);
				break;

			case ef_ex_SetAttckRateC:
				fmpar_table[chan].adsrw_car.attck = event.effect2 % 16;
				update_carrier_adsrw(chan);
				break;

			case ef_ex_SetDecayRateC:
				fmpar_table[chan].adsrw_car.dec = event.effect2 % 16;
				update_carrier_adsrw(chan);
				break;

			case ef_ex_SetSustnLevelC:
				fmpar_table[chan].adsrw_car.sustn = event.effect2 % 16;
				update_carrier_adsrw(chan);
				break;

			case ef_ex_SetRelRateC:
				fmpar_table[chan].adsrw_car.rel = event.effect2 % 16;
				update_carrier_adsrw(chan);
				break;

			case ef_ex_SetFeedback:
				fmpar_table[chan].feedb = event.effect2 % 16;
				update_fmpar(chan);
				break;

			case ef_ex_SetPanningPos:
				panning_table[chan] = event.effect2 % 16;
				update_fmpar(chan);
				break;

			case ef_ex_PatternLoop:
			case ef_ex_PatternLoopRec:
				if (event.effect2 % 16 == 0) {
					loopbck_table[chan] = current_line;
				} else {
					if (loopbck_table[chan] != NONE) {
						if (loop_table[chan][current_line] == NONE)
							loop_table[chan][current_line] = event.effect2 % 16;

						if (loop_table[chan][current_line] != 0) {
							pattern_break = TRUE;
							next_line = pattern_loop_flag + chan;
						} else {
							if (event.effect2 / 16 == ef_ex_PatternLoopRec)
								loop_table[chan][current_line] = NONE;
						}
					}
				}
				break;

			case ef_ex_MacroKOffLoop:
				if (event.effect2 % 16 != 0) {
					keyoff_loop[chan] = TRUE;
				} else {
					keyoff_loop[chan] = FALSE;
				}
				break;

			case ef_ex_ExtendedCmd:
				switch (event.effect2 % 16) {
				case ef_ex_cmd_RSS:        release_sustaining_sound(chan); break;
				case ef_ex_cmd_ResetVol:   reset_ins_volume(chan); break;
				case ef_ex_cmd_LockVol:    volume_lock  [chan] = TRUE; break;
				case ef_ex_cmd_UnlockVol:  volume_lock  [chan] = FALSE; break;
				case ef_ex_cmd_LockVP:     peak_lock    [chan] = TRUE; break;
				case ef_ex_cmd_UnlockVP:   peak_lock    [chan] = FALSE; break;
				case ef_ex_cmd_VSlide_def: volslide_type[chan] = 0; break;
				case ef_ex_cmd_LockPan:    pan_lock     [chan] = TRUE; break;
				case ef_ex_cmd_UnlockPan:  pan_lock     [chan] = FALSE; break;
				case ef_ex_cmd_VibrOff:    change_frequency(chan, freq_table[chan]); break;
				case ef_ex_cmd_TremOff:
					set_ins_volume(LO(volume_table[chan]),
						       HI(volume_table[chan]), chan);
					break;
				case ef_ex_cmd_VSlide_car:
					if (!((event.effect_def == ef_Extended) &&
					      (event.effect == ef_ex_ExtendedCmd * 16 +
								ef_ex_cmd_VSlide_mod)))
						volslide_type[chan] = 1;
					break;

				case ef_ex_cmd_VSlide_mod:
					if (!((event.effect_def == ef_Extended) &&
					      (event.effect == ef_ex_ExtendedCmd * 16 +
								ef_ex_cmd_VSlide_car)))
						volslide_type[chan] = 2;
					break;
				}
				break;
			}
			break;

		case ef_Extended2:
			switch (event.effect2 / 16) {
			case ef_ex2_PatDelayFrame:
			case ef_ex2_PatDelayRow:
				pattern_delay = TRUE;
				if (event.effect2 / 16 == ef_ex2_PatDelayFrame) {
					tickD = event.effect2 % 16;
				} else {
					tickD = speed * (event.effect2 % 16);
				}
				break;

			case ef_ex2_NoteDelay:
				effect_table2[chan] = concw(ef_Extended2 + ef_fix2 + ef_ex2_NoteDelay, 0);
				notedel_table[chan] = event.effect2 % 16;
				break;

			case ef_ex2_NoteCut:
				effect_table2[chan] = concw(ef_Extended2 + ef_fix2 + ef_ex2_NoteCut, 0);
				notecut_table[chan] = event.effect2 % 16;
				break;

			case ef_ex2_FineTuneUp:
				ftune_table[chan] += event.effect2 % 16;
				break;

			case ef_ex2_FineTuneDown:
				ftune_table[chan] -= event.effect2 % 16;
				break;

			case ef_ex2_GlVolSlideUp:
				effect_table2[chan] =
					concw(ef_Extended2 + ef_fix2 + ef_ex2_GlVolSlideUp,
					      event.effect2 % 16);
				break;

			case ef_ex2_GlVolSlideDn:
				effect_table2[chan] =
					concw(ef_Extended2 + ef_fix2 + ef_ex2_GlVolSlideDn,
						event.effect2 % 16);
				break;

			case ef_ex2_GlVolSlideUpF:
				effect_table2[chan] =
					concw(ef_Extended2 + ef_fix2 + ef_ex2_GlVolSlideUpF,
						event.effect2 % 16);
				break;

			case ef_ex2_GlVolSlideDnF:
				effect_table2[chan] =
					concw(ef_Extended2 + ef_fix2 + ef_ex2_GlVolSlideDnF,
						event.effect2 % 16);
				break;

			case ef_ex2_GlVolSldUpXF:
				effect_table2[chan] =
					concw(ef_Extended2 + ef_fix2 + ef_ex2_GlVolSldUpXF,
						event.effect2 % 16);
				break;

			case ef_ex2_GlVolSldDnXF:
				effect_table2[chan] =
					concw(ef_Extended2 + ef_fix2 + ef_ex2_GlVolSldDnXF,
						event.effect2 % 16);
				break;

			case ef_ex2_VolSlideUpXF:
				effect_table2[chan] =
					concw(ef_Extended2 + ef_fix2 + ef_ex2_VolSlideUpXF,
						event.effect2 % 16);
				break;

			case ef_ex2_VolSlideDnXF:
				effect_table2[chan] =
					concw(ef_Extended2 + ef_fix2 + ef_ex2_VolSlideDnXF,
						event.effect2 % 16);
				break;

			case ef_ex2_FreqSlideUpXF:
				effect_table2[chan] =
					concw(ef_Extended2 + ef_fix2 + ef_ex2_FreqSlideUpXF,
						event.effect2 % 16);
				break;

			case ef_ex2_FreqSlideDnXF:
				effect_table2[chan] =
					concw(ef_Extended2 + ef_fix2 + ef_ex2_FreqSlideDnXF,
						event.effect2 % 16);
				break;
			}
			break;

		case ef_Extended3:
			switch (event.effect2 / 16) {
			case ef_ex3_SetConnection:
				fmpar_table[chan].connect = event.effect2 % 16;
				update_fmpar(chan);
				break;

			case ef_ex3_SetMultipM:
				fmpar_table[chan].multipM = event.effect2 % 16;
				update_fmpar(chan);
				break;

			case ef_ex3_SetKslM:
				fmpar_table[chan].kslM = event.effect2 % 16;
				update_fmpar(chan);
				break;

			case ef_ex3_SetTremoloM:
				fmpar_table[chan].tremM = event.effect2 % 16;
				update_fmpar(chan);
				break;

			case ef_ex3_SetVibratoM:
				fmpar_table[chan].vibrM = event.effect2 % 16;
				update_fmpar(chan);
				break;

			case ef_ex3_SetKsrM:
				fmpar_table[chan].ksrM = event.effect2 % 16;
				update_fmpar(chan);
				break;

			case ef_ex3_SetSustainM:
				fmpar_table[chan].sustM = event.effect2 % 16;
				update_fmpar(chan);
				break;

			case ef_ex3_SetMultipC:
				fmpar_table[chan].multipC = event.effect2 % 16;
				update_fmpar(chan);
				break;

			case ef_ex3_SetKslC:
				fmpar_table[chan].kslC = event.effect2 % 16;
				update_fmpar(chan);
				break;

			case ef_ex3_SetTremoloC:
				fmpar_table[chan].tremC = event.effect2 % 16;
				update_fmpar(chan);
				break;

			case ef_ex3_SetVibratoC:
				fmpar_table[chan].vibrC = event.effect2 % 16;
				update_fmpar(chan);
				break;

			case ef_ex3_SetKsrC:
				fmpar_table[chan].ksrC = event.effect2 % 16;
				update_fmpar(chan);
				break;

			case ef_ex3_SetSustainC:
				fmpar_table[chan].sustC = event.effect2 % 16;
				update_fmpar(chan);
				break;
			}
			break;

		}

		if (event.effect_def + event.effect == 0) {
			effect_table[chan] = 0;
		} else {
			event_table[chan].effect_def = event.effect_def;
			event_table[chan].effect = event.effect;
		}

		if (event.effect_def2 + event.effect2 == 0) {
			 effect_table2[chan] = 0;
		} else {
			event_table[chan].effect_def2 = event.effect_def2;
			event_table[chan].effect2 = event.effect2;
		}

		if (event.note == (event.note | keyoff_flag)) {
			key_off(chan);
		} else {
			if (((LO(effect_table[chan]) != ef_TonePortamento) &&
			     (LO(effect_table[chan]) != ef_TPortamVolSlide) &&
			     (LO(effect_table[chan]) != ef_TPortamVSlideFine) &&
			     (LO(effect_table[chan]) != ef_Extended2 + ef_fix2 + ef_ex2_NoteDelay)) &&
			    ((LO(effect_table2[chan]) != ef_TonePortamento) &&
			     (LO(effect_table2[chan]) != ef_TPortamVolSlide) &&
			     (LO(effect_table2[chan]) != ef_TPortamVSlideFine) &&
			     (LO(effect_table2[chan]) != ef_Extended2 + ef_fix2 + ef_ex2_NoteDelay))) {
				if (!(((event.effect_def2 == ef_SwapArpeggio) ||
				       (event.effect_def2 == ef_SwapVibrato)) &&
				       (event.effect_def == ef_Extended) &&
				       (event.effect / 16 == ef_ex_ExtendedCmd) &&
				       (event.effect % 16 == ef_ex_cmd_NoRestart)) &&
				    !(((event.effect_def == ef_SwapArpeggio) ||
				       (event.effect_def == ef_SwapVibrato)) &&
				       (event.effect_def2 == ef_Extended) &&
				       (event.effect2 / 16 == ef_ex_ExtendedCmd) &&
				       (event.effect2 % 16 == ef_ex_cmd_NoRestart))) {
					output_note(event.note, voice_table[chan], chan, TRUE);
				} else {
					output_note_NR(event.note, voice_table[chan], chan, TRUE);
				}
			} else {
				if (event.note != 0)
					event_table[chan].note = event.note;
			}
		}

		switch (event.effect_def) {
		case ef_SwapArpeggio:
			if ((event.effect_def2 == ef_Extended) &&
			    (event.effect2 / 16 == ef_ex_ExtendedCmd) &&
			    (event.effect2 % 16 == ef_ex_cmd_NoRestart)) {
				if (macro_table[chan].arpg_pos >
				    songdata->macro_table[event.effect].arpeggio.length)
					macro_table[chan].arpg_pos =
						songdata->macro_table[event.effect].arpeggio.length;
				macro_table[chan].arpg_table = event.effect;
			} else {
				macro_table[chan].arpg_count = 1;
				macro_table[chan].arpg_pos = 0;
				macro_table[chan].arpg_table = event.effect;
				macro_table[chan].arpg_note = event_table[chan].note;
			}
			break;

		case ef_SwapVibrato:
			if ((event.effect_def2 == ef_Extended) &&
			    (event.effect2 / 16 == ef_ex_ExtendedCmd) &&
			    (event.effect2 % 16 == ef_ex_cmd_NoRestart)) {
				if (macro_table[chan].vib_table >
				    songdata->macro_table[event.effect].vibrato.length) 
					macro_table[chan].vib_pos =
						songdata->macro_table[event.effect].vibrato.length;
			macro_table[chan].vib_table = event.effect;
		      
		    } else {
			   macro_table[chan].vib_count = 1;
			   macro_table[chan].vib_pos = 0;
			   macro_table[chan].vib_table = event.effect;
			   macro_table[chan].vib_delay =
				songdata->macro_table[macro_table[chan].vib_table].vibrato.delay;
			 }
			break;
		}

		switch (event.effect_def2) {
		case ef_SwapArpeggio:
			if ((event.effect_def == ef_Extended) &&
			    (event.effect / 16 == ef_ex_ExtendedCmd) &&
			   (event.effect % 16 == ef_ex_cmd_NoRestart)) {
				if (macro_table[chan].arpg_pos >
				    songdata->macro_table[event.effect2].arpeggio.length)
					macro_table[chan].arpg_pos =
						songdata->macro_table[event.effect2].arpeggio.length;
			macro_table[chan].arpg_table = event.effect2;
			} else {
				macro_table[chan].arpg_count = 1;
				macro_table[chan].arpg_pos = 0;
				macro_table[chan].arpg_table = event.effect2;
				macro_table[chan].arpg_note = event_table[chan].note;
			}
			break;

		case ef_SwapVibrato:
			if ((event.effect_def == ef_Extended) &
			    (event.effect / 16 == ef_ex_ExtendedCmd) &&
			    (event.effect % 16 == ef_ex_cmd_NoRestart)) {

				if (macro_table[chan].vib_table >
				    songdata->macro_table[event.effect2].vibrato.length) 
					macro_table[chan].vib_pos =
						songdata->macro_table[event.effect2].vibrato.length;
				macro_table[chan].vib_table = event.effect2;
			} else {
				macro_table[chan].vib_count = 1;
				macro_table[chan].vib_pos = 0;
				macro_table[chan].vib_table = event.effect2;
				macro_table[chan].vib_delay =
					songdata->macro_table[macro_table[chan].vib_table].vibrato.delay;
			}
			break;
		}

		update_fine_effects(chan);
	}
}

void portamento_up(uint8_t chan, uint16_t slide, uint16_t limit)
{
	uint16_t freq;

	freq = calc_freq_shift_up(freq_table[chan] & 0x1fff, slide);
	if (freq <= limit) {
		change_frequency(chan, freq);
	} else {
		change_frequency(chan, limit);
	}
}

void portamento_down(uint8_t chan, uint16_t slide, uint16_t limit)
{
	uint16_t freq;

	freq = calc_freq_shift_down(freq_table[chan] & 0x1fff, slide);
	if (freq >= limit) {
		change_frequency(chan, freq);
	} else {
		change_frequency(chan, limit);
	}
}

void macro_vibrato__porta_up(uint8_t chan, uint8_t depth)
{
	uint16_t freq;

	freq = calc_freq_shift_up(macro_table[chan].vib_freq & 0x1fff, depth);
	if (freq <= nFreq(12*8+1)) {
		change_freq(chan, freq);
	} else {
		change_freq(chan, nFreq(12*8+1));
	}
}

void macro_vibrato__porta_down(uint8_t chan, uint8_t depth)
{
	uint16_t freq;
	freq = calc_freq_shift_down(macro_table[chan].vib_freq & 0x1fff, depth);
	if (freq >= nFreq(0)) {
		change_freq(chan, freq);
	} else {
		change_freq(chan,nFreq(0));
	}
}

void tone_portamento(uint8_t chan)
{
	if ((freq_table[chan] & 0x1fff) > porta_table[chan].freq) {
		portamento_down(chan, porta_table[chan].speed, porta_table[chan].freq);
	} else {
		if ((freq_table[chan] & 0x1fff) < porta_table[chan].freq)
			portamento_up(chan, porta_table[chan].speed, porta_table[chan].freq);
	}
}

void tone_portamento2(uint8_t chan)
{
	if ((freq_table[chan] & 0x1fff) > porta_table2[chan].freq) {
		portamento_down(chan, porta_table2[chan].speed,porta_table2[chan].freq);
	} else {
		if ((freq_table[chan] & 0x1fff) < porta_table2[chan].freq)
			portamento_up(chan, porta_table2[chan].speed, porta_table2[chan].freq);
	}
}

void slide_volume_up(uint8_t chan, uint8_t slide)
{
	uint16_t temp;
	uint8_t limit1, limit2, vLo, vHi;

	if (!peak_lock[chan])
		limit1 = 0;
	else
		limit1 = ins_parameter(event_table[chan].instr_def, 3) & 0x3f;

	if (!peak_lock[chan])
		limit2 = 0;
	else
		limit2 = ins_parameter(event_table[chan].instr_def, 2) & 0x3f;

	temp = volume_table[chan];

	switch (volslide_type[chan]) {
	case 0:
		vLo = LO(temp);
		vHi = HI(temp);
		if (vHi - slide >= limit1)
			temp = concw(vLo, vHi - slide);
		else
			temp = concw(vLo, limit1);

		set_ins_volume(NONE, HI(temp), chan);
		volume_table[chan] = temp;
		if (((ins_parameter(voice_table[chan], 10) & 1) == 1) ||
		     (percussion_mode && (chan >= 17 && chan <= 20))) {
			vLo = LO(temp);
			vHi = HI(temp);
			if (vLo - slide >= limit2)
				temp = concw(vLo - slide, vHi);
			else
				temp = concw(limit2, vHi);
			set_ins_volume(LO(temp), NONE, chan);
			volume_table[chan] = temp;
		}
	break;

	case 1:
		vLo = LO(temp);
		vHi = HI(temp);
		if (vHi - slide >= limit1)
			temp = concw(vLo, vHi - slide);
		else
			temp = concw(vLo, limit1);
		set_ins_volume(NONE, HI(temp), chan);
		volume_table[chan] = temp;
	break;

	case 2:
		vLo = LO(temp);
		vHi = HI(temp);
		if (vLo - slide >= limit2)
			temp = concw(vLo - slide, vHi);
		else
			temp = concw(limit2, vHi);
		set_ins_volume(LO(temp), NONE, chan);
		volume_table[chan] = temp;
	break;

	case 3:
		vLo = LO(temp);
		vHi = HI(temp);
		if (vHi - slide >= limit1)
			temp = concw(vLo, vHi - slide);
		else
			temp = concw(vLo, limit1);
		set_ins_volume(NONE, HI(temp), chan);
		volume_table[chan] = temp;
		vLo = LO(temp);
		vHi = HI(temp);
		if (vLo - slide >= limit2)
			temp = concw(vLo - slide, vHi);
		else
			temp = concw(limit2, vHi);
		set_ins_volume(LO(temp), NONE, chan);
		volume_table[chan] = temp;
	break;
	}
}

void slide_volume_down(uint8_t chan, uint8_t slide)
{
	uint16_t temp;
	uint8_t vLo, vHi;


	temp = volume_table[chan];

	switch (volslide_type[chan]) {
	case 0:
		vLo = LO(temp);
		vHi = HI(temp);
		if (vHi + slide <= 63)
			temp = concw(vLo, vHi + slide);
		else
			temp = concw(vLo, 63);
		set_ins_volume(NONE, HI(temp), chan);
		volume_table[chan] = temp;
		if (((ins_parameter(voice_table[chan], 10) & 1) == 1) ||
		     (percussion_mode && (chan >= 17 && chan <= 20))) {
			vLo = LO(temp);
			vHi = HI(temp);
			if (vLo + slide <= 63)
				temp = concw(vLo + slide, vHi);
			else
				temp = concw(63, vHi);
			set_ins_volume(LO(temp), NONE, chan);
			volume_table[chan] = temp;
		}
	break;

	case 1:
		vLo = LO(temp);
		vHi = HI(temp);
		if (vHi + slide <= 63)
			temp = concw(vLo, vHi + slide);
		else
			temp = concw(vLo, 63);
		set_ins_volume(NONE, HI(temp), chan);
		volume_table[chan] = temp;
	break;

	case 2:
		vLo = LO(temp);
		vHi = HI(temp);
		if (vLo + slide <= 63)
			temp = concw(vLo + slide, vHi);
		else
			temp = concw(63, vHi);
		set_ins_volume(LO(temp), NONE, chan);
		volume_table[chan] = temp;
	break;

	case 3:
		vLo = LO(temp);
		vHi = HI(temp);
		if (vHi + slide <= 63)
			temp = concw(vLo, vHi + slide);
		else
			temp = concw(vLo, 63);
		set_ins_volume(NONE, HI(temp), chan);
		volume_table[chan] = temp;
		vLo = LO(temp);
		vHi = HI(temp);
		if (vLo + slide <= 63)
			temp = concw(vLo + slide, vHi);
		else
			temp = concw(63, vHi);
		set_ins_volume(LO(temp), NONE, chan);
		volume_table[chan] = temp;
	break;
	}
}

void volume_slide(uint8_t chan, uint8_t up_speed, uint8_t down_speed)
{
	if (up_speed != 0)
		slide_volume_up(chan, up_speed);
	else {
		if (down_speed != 0)
			slide_volume_down(chan, down_speed);
	}
}

void global_volume_slide(uint8_t up_speed, uint8_t down_speed)
{
	if (up_speed != NONE)
		global_volume = max(global_volume + up_speed, 63);

	if (down_speed != NONE) {
		if (global_volume >= down_speed)
			global_volume -= down_speed;
		else
			global_volume = 0;
	}

	set_global_volume();
}

void arpeggio(uint8_t chan)
{
	uint8_t arpgg_state[3] = {1, 2, 0};

	uint16_t freq;

	switch (arpgg_table[chan].state) {
	case 0: freq = nFreq(arpgg_table[chan].note - 1); break;
	case 1: freq = nFreq(arpgg_table[chan].note - 1 + arpgg_table[chan].add1); break;
	case 2: freq = nFreq(arpgg_table[chan].note - 1 + arpgg_table[chan].add2); break;
	}

	arpgg_table[chan].state = arpgg_state[arpgg_table[chan].state];
	change_frequency(chan, freq +
			(int8_t)(ins_parameter(event_table[chan].instr_def, 12)));
}

void arpeggio2(uint8_t chan)
{
	uint8_t arpgg_state[3] = {1, 2, 0};

	uint16_t freq;

	switch (arpgg_table2[chan].state) {
	case 0: freq = nFreq(arpgg_table2[chan].note - 1); break;
	case 1: freq = nFreq(arpgg_table2[chan].note - 1 + arpgg_table2[chan].add1); break;
	case 2: freq = nFreq(arpgg_table2[chan].note - 1 + arpgg_table2[chan].add2); break;
	}

	arpgg_table2[chan].state = arpgg_state[arpgg_table2[chan].state];
	change_frequency(chan, freq +
			(int8_t)(ins_parameter(event_table[chan].instr_def, 12)));
}

void vibrato(uint8_t chan)
{
	uint16_t freq, old_freq;
	uint8_t direction;

	vibr_table[chan].pos += vibr_table[chan].speed;
	freq = calc_vibrato_shift(vibr_table[chan].depth, vibr_table[chan].pos);
	direction = vibr_table[chan].pos & 0x20;
	old_freq = freq_table[chan];
	if (direction == 0)
		portamento_down(chan, freq, nFreq(0));
	else
		portamento_up(chan, freq, nFreq(12*8+1));
	freq_table[chan] = old_freq;
}

void vibrato2(uint8_t chan)
{
	uint16_t freq, old_freq;
	uint8_t direction;

	vibr_table2[chan].pos += vibr_table2[chan].speed;
	freq = calc_vibrato_shift(vibr_table2[chan].depth, vibr_table2[chan].pos);
	direction = vibr_table2[chan].pos & 0x20;
	old_freq = freq_table[chan];
	if (direction == 0)
		portamento_down(chan, freq, nFreq(0));
	else
		portamento_up(chan, freq, nFreq(12*8+1));
	freq_table[chan] = old_freq;
}

void tremolo(uint8_t chan)
{
	uint16_t vol, old_vol;
	uint8_t direction;

	trem_table[chan].pos += trem_table[chan].speed;
	vol = calc_vibrato_shift(trem_table[chan].depth, trem_table[chan].pos);
	direction = trem_table[chan].pos & 0x20;
	old_vol = volume_table[chan];
	if (direction == 0)
		slide_volume_down(chan, vol);
	else
		slide_volume_up(chan, vol);
	volume_table[chan] = old_vol;
}

void tremolo2(uint8_t chan)
{
	uint16_t vol, old_vol;
	uint8_t direction;

	trem_table2[chan].pos += trem_table2[chan].speed;
	vol = calc_vibrato_shift(trem_table2[chan].depth, trem_table2[chan].pos);
	direction = trem_table2[chan].pos & 0x20;
	old_vol = volume_table[chan];
	if (direction == 0)
		slide_volume_down(chan, vol);
	else
		slide_volume_up(chan, vol);
	volume_table[chan] = old_vol;
}

uint8_t chanvol(uint8_t chan)
{
	if ((ins_parameter(voice_table[chan], 10) & 1) == 0)
		return 63 - HI(volume_table[chan]);
	else
		return 63 - (LO(volume_table[chan]) + HI(volume_table[chan])) / 2;
}

void update_effects()
{
	uint8_t chan, eLo, eHi, eLo2, eHi2;

	for(chan = 0; chan < 20; chan++) {
		eLo  = LO(effect_table[chan]);
		eHi  = HI(effect_table[chan]);
		eLo2 = LO(effect_table2[chan]);
		eHi2 = HI(effect_table2[chan]);

		switch (eLo) {
		case ef_Arpeggio + ef_fix1:
			arpeggio(chan);
			break;

		case ef_ArpggVSlide:
			volume_slide(chan, eHi / 16, eHi % 16);
			arpeggio(chan);
			break;

		case ef_ArpggVSlideFine:
			arpeggio(chan);
			break;

		case ef_FSlideUp:
			portamento_up(chan, eHi, nFreq(12*8+1));
			break;

		case ef_FSlideDown:
			portamento_down(chan, eHi, nFreq(0));
			break;

		case ef_FSlideUpVSlide:
			portamento_up(chan, fslide_table[chan], nFreq(12*8+1));
			volume_slide(chan, eHi / 16, eHi % 16);
			break;

		case ef_FSlUpVSlF:
			portamento_up(chan, fslide_table[chan], nFreq(12*8+1));
			break;

		case ef_FSlideDownVSlide:
			portamento_down(chan, fslide_table[chan], nFreq(0));
			volume_slide(chan, eHi / 16, eHi % 16);
			break;

		case ef_FSlDownVSlF:
			portamento_down(chan, fslide_table[chan], nFreq(0));
			break;

		case ef_FSlUpFineVSlide:
			volume_slide(chan, eHi / 16, eHi % 16);
			break;

		case ef_FSlDownFineVSlide:
			volume_slide(chan, eHi / 16, eHi % 16);
			break;

		case ef_TonePortamento:
			tone_portamento(chan);
			break;

		case ef_TPortamVolSlide:
			volume_slide(chan, eHi / 16, eHi % 16);
			tone_portamento(chan);
			break;

		case ef_TPortamVSlideFine:
			tone_portamento(chan);
			break;

		case ef_Vibrato:
			if (!vibr_table[chan].fine)
				vibrato(chan);
			break;

		case ef_Tremolo:
			if (!trem_table[chan].fine)
				tremolo(chan);
			break;

		case ef_VibratoVolSlide:
			volume_slide(chan, eHi / 16, eHi % 16);
			if (!vibr_table[chan].fine)
				vibrato(chan);
			break;

		case ef_VibratoVSlideFine:
			if (!vibr_table[chan].fine)
				vibrato(chan);
			break;

		case ef_VolSlide:
			volume_slide(chan, eHi / 16, eHi % 16);
			break;

		case ef_RetrigNote:
			if (retrig_table[chan] >= eHi) {
				retrig_table[chan] = 0;
				output_note(event_table[chan].note,
					    event_table[chan].instr_def,
					    chan, TRUE);
			} else {
				retrig_table[chan]++;
			}
			break;

		case ef_MultiRetrigNote:
			if (retrig_table[chan] >= eHi / 16) {
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

				case 6: slide_volume_down(chan, chanvol(chan) -
							  chanvol(chan) * 2 / 3);
					break;

				case 7: slide_volume_down(chan, chanvol(chan) -
							  chanvol(chan) * 1 / 2);
					break;

				case 14: slide_volume_up(chan, max(chanvol(chan) * 3 / 2 -
							    chanvol(chan), 63));
					break;

				case 15: slide_volume_up(chan,max(chanvol(chan) * 2 -
							    chanvol(chan), 63));
					break;
				}

				retrig_table[chan] = 0;
				output_note(event_table[chan].note,
					    event_table[chan].instr_def,
					    chan,TRUE);
			} else {
				retrig_table[chan]++;
			}
			break;

		case ef_Tremor:
			if (tremor_table[chan].pos >= 0) {
				if ((tremor_table[chan].pos + 1) <= eHi / 16) {
					tremor_table[chan].pos++;
				} else {
					slide_volume_down(chan, 63);
					tremor_table[chan].pos = -1;
				}
			} else {
				if ((tremor_table[chan].pos - 1) >= -(eHi % 16)) {
					tremor_table[chan].pos--;
				} else {
					set_ins_volume(LO(tremor_table[chan].volume),
						       HI(tremor_table[chan].volume), chan);
					tremor_table[chan].pos = 1;
				}
			}
			break;

		case ef_Extended2 + ef_fix2 + ef_ex2_NoteDelay:
			if (notedel_table[chan] == 0) {
				notedel_table[chan] = NONE;
				output_note(event_table[chan].note,
					    event_table[chan].instr_def,
					    chan, TRUE);
			} else {
				notedel_table[chan]--;
			}
			break;

		case ef_Extended2 + ef_fix2 + ef_ex2_NoteCut:
			if (notecut_table[chan] == 0) {
				notecut_table[chan] = NONE;
				key_off(chan);
			} else {
				notecut_table[chan]--;
			}
			break;

		case ef_Extended2 + ef_fix2 + ef_ex2_GlVolSlideUp:
			global_volume_slide(eHi, NONE);
			break;

		case ef_Extended2 + ef_fix2 + ef_ex2_GlVolSlideDn:
			global_volume_slide(NONE, eHi);
			break;
		}

		switch (eLo2) {
		case ef_Arpeggio + ef_fix1:
			arpeggio2(chan);
			break;

		case ef_ArpggVSlide:
			volume_slide(chan, eHi2 / 16, eHi2 % 16);
			arpeggio2(chan);
			break;

		case ef_ArpggVSlideFine:
			arpeggio2(chan);
			break;

		case ef_FSlideUp:
			portamento_up(chan, eHi2, nFreq(12*8+1));
			break;

		case ef_FSlideDown:
			portamento_down(chan, eHi2, nFreq(0));
			break;

		case ef_FSlideUpVSlide:
			portamento_up(chan, fslide_table2[chan], nFreq(12*8+1));
			volume_slide(chan, eHi2 / 16, eHi2 % 16);
			break;

		case ef_FSlUpVSlF:
			portamento_up(chan, fslide_table2[chan], nFreq(12*8+1));
			break;

		case ef_FSlideDownVSlide:
			portamento_down(chan, fslide_table2[chan], nFreq(0));
			volume_slide(chan,eHi2 / 16, eHi2 % 16);
			break;

		case ef_FSlDownVSlF:
			portamento_down(chan, fslide_table2[chan], nFreq(0));
			break;

		case ef_FSlUpFineVSlide:
			volume_slide(chan, eHi2 / 16, eHi2 % 16);
			break;

		case ef_FSlDownFineVSlide:
			volume_slide(chan, eHi2 / 16, eHi2 % 16);

		case ef_TonePortamento:
			tone_portamento2(chan);
			break;

		case ef_TPortamVolSlide:
			volume_slide(chan, eHi2 / 16, eHi2 % 16);
			tone_portamento2(chan);
			break;

		case ef_TPortamVSlideFine:
			tone_portamento2(chan);
			break;

		case ef_Vibrato:
			if (!vibr_table2[chan].fine)
				vibrato2(chan);
			break;

		case ef_Tremolo:
			if (!trem_table2[chan].fine)
				tremolo2(chan);
			break;

		case ef_VibratoVolSlide:
			volume_slide(chan, eHi2 / 16, eHi2 % 16);
			if (!vibr_table2[chan].fine)
				vibrato2(chan);
			break;

		case ef_VibratoVSlideFine:
			if (!vibr_table2[chan].fine)
				vibrato2(chan);
			break;

		case ef_VolSlide:
			volume_slide(chan, eHi2 / 16, eHi2 % 16);
			break;

		case ef_RetrigNote:
			if (retrig_table2[chan] >= eHi2) {
				retrig_table2[chan] = 0;
				output_note(event_table[chan].note,
					    event_table[chan].instr_def, chan, TRUE);
			} else {
				retrig_table2[chan]++;
			}
			break;

		case ef_MultiRetrigNote:
			if (retrig_table2[chan] >= eHi2 / 16) {
				switch (eHi2 % 16) {
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


				case 6: slide_volume_down(chan, chanvol(chan) -
						chanvol(chan) * 2 / 3);
					break;
				case 7: slide_volume_down(chan, chanvol(chan) -
						chanvol(chan) * 1 / 2);
					break;

				case 14: slide_volume_up(chan, max(chanvol(chan) * 3 / 2 -
						chanvol(chan), 63));
					break;
				case 15: slide_volume_up(chan, max(chanvol(chan) * 2 -
						chanvol(chan), 63));
					break;
				}

				retrig_table2[chan] = 0;
				output_note(event_table[chan].note,
					    event_table[chan].instr_def, chan, TRUE);
			} else {
				retrig_table2[chan]++;
			}
			break;

		case ef_Tremor:
			if (tremor_table2[chan].pos >= 0) {
				if ((tremor_table2[chan].pos + 1) <= eHi2 / 16) {
					tremor_table2[chan].pos++;
				} else {
					slide_volume_down(chan, 63);
					tremor_table2[chan].pos = -1;
				}
			} else {
				if ((tremor_table2[chan].pos - 1) >= -(eHi2 % 16)) {
					tremor_table2[chan].pos--;
				} else {
					set_ins_volume(LO(tremor_table2[chan].volume),
							HI(tremor_table2[chan].volume), chan);
					tremor_table2[chan].pos = 1;
				}
			}
			break;

		case ef_Extended2 + ef_fix2 + ef_ex2_NoteDelay:
			if (notedel_table[chan] == 0) {
				notedel_table[chan] = NONE;
				output_note(event_table[chan].note,
						event_table[chan].instr_def, chan, TRUE);
			} else {
				notedel_table[chan]--;
			}
			break;

		case ef_Extended2 + ef_fix2 + ef_ex2_NoteCut:
			if (notecut_table[chan] == 0) {
				notecut_table[chan] = NONE;
				key_off(chan);
			} else {
				notecut_table[chan]--;
			}
			break;

		case ef_Extended2 + ef_fix2 + ef_ex2_GlVolSlideUp:
			global_volume_slide(eHi2, NONE);
			break;

		case ef_Extended2 + ef_fix2 + ef_ex2_GlVolSlideDn:
			global_volume_slide(NONE, eHi2);
			break;
		}
	}
}

static void update_fine_effects(uint8_t chan)
{
	uint8_t eLo, eHi, eLo2, eHi2;

	eLo  = LO(effect_table[chan]);
	eHi  = HI(effect_table[chan]);
	eLo2 = LO(effect_table2[chan]);
	eHi2 = HI(effect_table2[chan]);

	switch (eLo) {
	case ef_ArpggVSlideFine:
		volume_slide(chan, eHi / 16, eHi % 16);
		break;

	case ef_FSlideUpFine:
		portamento_up(chan, eHi, nFreq(12*8+1));
		break;

	case ef_FSlideDownFine:
		portamento_down(chan, eHi, nFreq(0));
		break;

	case ef_FSlUpVSlF:
		volume_slide(chan, eHi / 16, eHi % 16);
		break;

	case ef_FSlDownVSlF:
		volume_slide(chan, eHi / 16, eHi % 16);
		break;

	case ef_FSlUpFineVSlide:
		portamento_up(chan, fslide_table[chan], nFreq(12*8+1));
		break;

	case ef_FSlUpFineVSlF:
		portamento_up(chan, fslide_table[chan], nFreq(12*8+1));
		volume_slide(chan, eHi / 16, eHi % 16);
		break;

	case ef_FSlDownFineVSlide:
		portamento_down(chan, fslide_table[chan], nFreq(0));
		break;

	case ef_FSlDownFineVSlF:
		portamento_down(chan, fslide_table[chan], nFreq(0));
		volume_slide(chan, eHi / 16, eHi % 16);
		break;

	case ef_TPortamVSlideFine:
		volume_slide(chan, eHi / 16, eHi % 16);
		break;

	case ef_Vibrato:
		if (vibr_table[chan].fine)
			vibrato(chan);
		break;

	case ef_Tremolo:
		if (trem_table[chan].fine)
			tremolo(chan);
		break;

	case ef_VibratoVolSlide:
		if (vibr_table[chan].fine)
			vibrato(chan);
		break;

	case ef_VibratoVSlideFine:
		volume_slide(chan, eHi / 16, eHi % 16);
		if (vibr_table[chan].fine)
			vibrato(chan);
		break;

	case ef_VolSlideFine:
		volume_slide(chan, eHi / 16, eHi % 16);
		break;

	case ef_Extended2 + ef_fix2 + ef_ex2_GlVolSlideUpF:
		global_volume_slide(eHi, NONE);
		break;

	case ef_Extended2 + ef_fix2 + ef_ex2_GlVolSlideDnF:
		global_volume_slide(NONE, eHi);
		break;
	}

	switch (eLo2) {
	case ef_ArpggVSlideFine:
		volume_slide(chan, eHi2 / 16, eHi2 % 16);
		break;

	case ef_FSlideUpFine:
		portamento_up(chan,eHi2,nFreq(12*8+1));
		break;

	case ef_FSlideDownFine:
		portamento_down(chan, eHi2, nFreq(0));
		break;

	case ef_FSlUpVSlF:
		volume_slide(chan, eHi2 / 16, eHi2 % 16);
		break;

	case ef_FSlDownVSlF:
		volume_slide(chan, eHi2 / 16, eHi2 % 16);
		break;

	case ef_FSlUpFineVSlide:
		portamento_up(chan, fslide_table2[chan], nFreq(12*8+1));
		break;

	case ef_FSlUpFineVSlF:
		portamento_up(chan, fslide_table2[chan], nFreq(12*8+1));
		volume_slide(chan, eHi2 / 16, eHi2 % 16);
		break;

	case ef_FSlDownFineVSlide:
		portamento_down(chan, fslide_table2[chan], nFreq(0));
		break;

	case ef_FSlDownFineVSlF:
		portamento_down(chan, fslide_table2[chan], nFreq(0));
		volume_slide(chan, eHi2 / 16, eHi2 % 16);
		break;

	case ef_TPortamVSlideFine:
		volume_slide(chan, eHi2 / 16, eHi2 % 16);
		break;

	case ef_Vibrato:
		if (vibr_table2[chan].fine)
			vibrato2(chan);
		break;

	case ef_Tremolo:
		if (trem_table2[chan].fine)
			tremolo2(chan);
		break;

	case ef_VibratoVolSlide:
		if (vibr_table2[chan].fine)
			vibrato2(chan);
		break;

	case ef_VibratoVSlideFine:
		volume_slide(chan, eHi2 / 16, eHi2 % 16);
		if (vibr_table2[chan].fine)
			vibrato2(chan);
		break;

	case ef_VolSlideFine:
		volume_slide(chan, eHi2 / 16, eHi2 % 16);
		break;

	case ef_Extended2 + ef_fix2 + ef_ex2_GlVolSlideUpF:
		global_volume_slide(eHi2, NONE);
		break;

	case ef_Extended2 + ef_fix2 + ef_ex2_GlVolSlideDnF:
		global_volume_slide(NONE,eHi2);
		break;
	}
}

static void update_extra_fine_effects()
{
	uint8_t chan, eLo, eHi, eLo2, eHi2;

	for (chan = 0; chan < 20; chan++) {
		eLo  = LO(effect_table[chan]);
		eHi  = HI(effect_table[chan]);
		eLo2 = LO(effect_table2[chan]);
		eHi2 = HI(effect_table2[chan]);

		switch (eLo) {
		case ef_Extended2 + ef_fix2 + ef_ex2_GlVolSldUpXF:
			global_volume_slide(eHi, NONE);
			break;

		case ef_Extended2 + ef_fix2 + ef_ex2_GlVolSldDnXF:
			global_volume_slide(NONE, eHi);
			break;

		case ef_Extended2 + ef_fix2 + ef_ex2_VolSlideUpXF:
			volume_slide(chan, eHi, 0);
			break;

		case ef_Extended2 + ef_fix2 + ef_ex2_VolSlideDnXF:
			volume_slide(chan, 0, eHi);
			break;

		case ef_Extended2 + ef_fix2 + ef_ex2_FreqSlideUpXF:
			portamento_up(chan, eHi, nFreq(12*8+1));
			break;

		case ef_Extended2 + ef_fix2 + ef_ex2_FreqSlideDnXF:
			portamento_down(chan, eHi, nFreq(0));
			break;

		case ef_ExtraFineArpeggio:
			arpeggio(chan);
			break;

		case ef_ExtraFineVibrato:
			if (!vibr_table[chan].fine)
				vibrato(chan);
			break;

		case ef_ExtraFineTremolo:
			if (!trem_table[chan].fine)
				tremolo(chan);
			break;
		}

		switch (eLo2) {
		case ef_Extended2 + ef_fix2 + ef_ex2_GlVolSldUpXF:
			global_volume_slide(eHi2, NONE);
			break;

		case ef_Extended2 + ef_fix2 + ef_ex2_GlVolSldDnXF:
			global_volume_slide(NONE, eHi2);
			break;

		case ef_Extended2 + ef_fix2 + ef_ex2_VolSlideUpXF:
			volume_slide(chan, eHi2, 0);
			break;

		case ef_Extended2 + ef_fix2 + ef_ex2_VolSlideDnXF:
			volume_slide(chan, 0, eHi2);
			break;

		case ef_Extended2 + ef_fix2 + ef_ex2_FreqSlideUpXF:
			portamento_up(chan, eHi2, nFreq(12*8+1));
			break;

		case ef_Extended2 + ef_fix2 + ef_ex2_FreqSlideDnXF:
			portamento_down(chan,eHi2,nFreq(0));
			break;

		case ef_ExtraFineArpeggio:
			arpeggio2(chan);
			break;

		case ef_ExtraFineVibrato:
			if (!vibr_table2[chan].fine)
				vibrato2(chan);
			break;

		case ef_ExtraFineTremolo:
			if (!trem_table2[chan].fine)
				tremolo2(chan);
			break;
		}
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
		stop_playing();
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
			memset(loopbck_table, NONE, sizeof(loopbck_table));
			memset(loop_table, NONE, sizeof(loop_table));
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
				current_order = event_table[next_line - pattern_break_flag].effect;
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

	if ((current_line == 0) &&
	    (current_order == calc_following_order(0)) && speed_update) {
		tempo = songdata->tempo;
		speed = songdata->speed;
		update_timer(tempo);
	}
}

void poll_proc()
{
#if 1
	if (pattern_delay) {
		update_effects();
		if (tickD > 1) {
			tickD--;
		} else {
			pattern_delay = FALSE;
		}
	} else {
		if (ticks == 0) {
			ticks = speed;
		}
		if (ticks == speed)
			play_line();
		update_effects();
		ticks--;
		if (ticks == 0) {
			ticks = speed;
			update_song_position();
		}
	}
#else
	if (!pattern_delay && (ticks - tick0 + 1 >= speed))  {
		if ((songdata->pattern_order[current_order] > 0x7f) &&
		    (calc_order_jump() == -1))
			return;

		current_pattern = songdata->pattern_order[current_order];
		play_line();
		update_effects();

		if (!pattern_delay)
			update_song_position();
		tick0 = ticks;
	} else {
		update_effects();
		ticks++;
		if (pattern_delay && (tickD > 1)) {
			tickD--;
		} else {
			if (pattern_delay) {
				tick0 = ticks;
				update_song_position();
			}
			pattern_delay = FALSE;
		}
	}
#endif
	tickXF++;
	if (tickXF % 4 == 0) {
		update_extra_fine_effects();
		tickXF -= 4;
	}
}

void macro_poll_proc()
{
#define  IDLE		0xfff
#define  FINISHED	0xffff
	uint16_t chan;
	uint16_t finished_flag;

	for (chan = 0; chan < 20; chan++) {
		if (!keyoff_loop[chan]) {
			finished_flag = FINISHED;
		} else {
			finished_flag = IDLE;
		}

		tCH_MACRO_TABLE *mt = &macro_table[chan];
		tREGISTER_TABLE *rt = &songdata->instr_macros[mt->fmreg_table];

		if ((mt->fmreg_table != 0) && (speed != 0)) { // FIXME: what speed?
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
					mt->fmreg_duration = rt->data[mt->fmreg_pos].duration;
					if (mt->fmreg_duration != 0) {
						tREGISTER_TABLE_DEF *d = &rt->data[mt->fmreg_pos];

						if (!songdata->dis_fmreg_col[mt->fmreg_table][0])
							fmpar_table[chan].adsrw_mod.attck =
								d->fm_data.ATTCK_DEC_modulator >> 4;

						if (!songdata->dis_fmreg_col[mt->fmreg_table][1])
							fmpar_table[chan].adsrw_mod.dec =
								d->fm_data.ATTCK_DEC_modulator & 0x0f;

						if (!songdata->dis_fmreg_col[mt->fmreg_table][2])
							fmpar_table[chan].adsrw_mod.sustn =
								d->fm_data.SUSTN_REL_modulator >> 4;

						if (!songdata->dis_fmreg_col[mt->fmreg_table][3])
							fmpar_table[chan].adsrw_mod.rel =
								d->fm_data.SUSTN_REL_modulator & 0x0f;

						if (!songdata->dis_fmreg_col[mt->fmreg_table][4])
							fmpar_table[chan].adsrw_mod.wform =
								d->fm_data.WAVEFORM_modulator & 0x07;

						if (!songdata->dis_fmreg_col[mt->fmreg_table][6])
							fmpar_table[chan].kslM =
								d->fm_data.KSL_VOLUM_modulator >> 6;

						if (!songdata->dis_fmreg_col[mt->fmreg_table][7])
							fmpar_table[chan].multipM =
								d->fm_data.AM_VIB_EG_modulator & 0x0f;

						if (!songdata->dis_fmreg_col[mt->fmreg_table][8])
							fmpar_table[chan].tremM =
								d->fm_data.AM_VIB_EG_modulator >> 7;

						if (!songdata->dis_fmreg_col[mt->fmreg_table][9])
							fmpar_table[chan].vibrM =
								(d->fm_data.AM_VIB_EG_modulator >> 6) & 1;

						if (!songdata->dis_fmreg_col[mt->fmreg_table][10])
							fmpar_table[chan].ksrM =
								(d->fm_data.AM_VIB_EG_modulator >> 4) & 1;

						if (!songdata->dis_fmreg_col[mt->fmreg_table][11])
							fmpar_table[chan].sustM =
								(d->fm_data.AM_VIB_EG_modulator >> 5) & 1;

						if (!songdata->dis_fmreg_col[mt->fmreg_table][12])
							fmpar_table[chan].adsrw_car.attck =
								d->fm_data.ATTCK_DEC_carrier >> 4;

						if (!songdata->dis_fmreg_col[mt->fmreg_table][13])
							fmpar_table[chan].adsrw_car.dec =
								d->fm_data.ATTCK_DEC_carrier & 0x0f;

						if (!songdata->dis_fmreg_col[mt->fmreg_table][14])
							fmpar_table[chan].adsrw_car.sustn =
								d->fm_data.SUSTN_REL_carrier >> 4;

						if (!songdata->dis_fmreg_col[mt->fmreg_table][15])
							fmpar_table[chan].adsrw_car.rel =
								d->fm_data.SUSTN_REL_carrier & 0xf;

						if (!songdata->dis_fmreg_col[mt->fmreg_table][16])
							fmpar_table[chan].adsrw_car.wform =
								d->fm_data.WAVEFORM_carrier & 0x07;

						if (!songdata->dis_fmreg_col[mt->fmreg_table][18])
							fmpar_table[chan].kslC =
								d->fm_data.KSL_VOLUM_carrier >> 6;

						if (!songdata->dis_fmreg_col[mt->fmreg_table][19])
							fmpar_table[chan].multipC =
								d->fm_data.AM_VIB_EG_carrier & 0x0f;

						if (!songdata->dis_fmreg_col[mt->fmreg_table][20])
							fmpar_table[chan].tremC =
								d->fm_data.AM_VIB_EG_carrier >> 7;

						if (!songdata->dis_fmreg_col[mt->fmreg_table][21])
							fmpar_table[chan].vibrC =
								(d->fm_data.AM_VIB_EG_carrier >> 6) & 1;

						if (!songdata->dis_fmreg_col[mt->fmreg_table][22])
							fmpar_table[chan].ksrC =
								(d->fm_data.AM_VIB_EG_carrier >> 4) & 1;

						if (!songdata->dis_fmreg_col[mt->fmreg_table][23])
							fmpar_table[chan].sustC =
								(d->fm_data.AM_VIB_EG_carrier >> 5) & 1;

						if (!songdata->dis_fmreg_col[mt->fmreg_table][24])
							fmpar_table[chan].connect =
								d->fm_data.FEEDBACK_FM & 1;

						if (!songdata->dis_fmreg_col[mt->fmreg_table][25])
							fmpar_table[chan].feedb =
								(d->fm_data.FEEDBACK_FM >> 1) & 7;

						if (!songdata->dis_fmreg_col[mt->fmreg_table][27] &&
						    !pan_lock[chan])
							panning_table[chan] = d->panning;

						if (!songdata->dis_fmreg_col[mt->fmreg_table][5])
							set_ins_volume(63 - (d->fm_data.KSL_VOLUM_modulator & 0x3f),
								       NONE, chan);

						if (!songdata->dis_fmreg_col[mt->fmreg_table][17])
							set_ins_volume(NONE,
									63 - (d->fm_data.KSL_VOLUM_carrier & 0x3f), chan);

						update_modulator_adsrw(chan);
						update_carrier_adsrw(chan);
						update_fmpar(chan);

						if (!((d->fm_data.FEEDBACK_FM | 0x80) != d->fm_data.FEEDBACK_FM))
							output_note(event_table[chan].note,
								    event_table[chan].instr_def, chan, FALSE);

						if (!songdata->dis_fmreg_col[mt->fmreg_table][26]) {
							if (d->freq_slide > 0) {
								portamento_up(chan, d->freq_slide, nFreq(12*8+1));
							} else {
								if (d->freq_slide < 0)
									portamento_down(chan, abs(d->freq_slide), nFreq(0));
							}
						}
					}
				}
			}
		}

		tARPEGGIO_TABLE *at = &songdata->macro_table[mt->arpg_table].arpeggio;

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
					switch (at->data[mt->arpg_pos]) {
					case 0:
						change_frequency(chan, nFreq(mt->arpg_note - 1) +
							(int8_t)ins_parameter(event_table[chan].instr_def, 12));
						break;

					case 1 ... 96:
						change_frequency(chan, nFreq(max(mt->arpg_note + at->data[mt->arpg_pos], 97) - 1) +
							(int8_t)ins_parameter(event_table[chan].instr_def, 12));
						break;

					case 0x80 ... 0x80+12*8+1:
						change_frequency(chan, nFreq(at->data[mt->arpg_pos] - 0x80 - 1) +
							(int8_t)ins_parameter(event_table[chan].instr_def, 12));
						break;
                    }
				}
			} else {
				mt->arpg_count++;
			}
		}

		tVIBRATO_TABLE *vt = &songdata->macro_table[mt->vib_table].vibrato;

		if ((mt->vib_table != 0) && (vt->speed != 0)) {
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
						if (vt->data[mt->vib_pos] > 0)
							macro_vibrato__porta_up(chan, vt->data[mt->vib_pos]);
					} else {
						if (vt->data[mt->vib_pos] < 0)
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

	if ((ticklooper == 0) &&
	    (irq_mode))
		poll_proc();

	if ((macro_ticklooper == 0) &&
	    (irq_mode))
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
	memset(volume_table, 0, sizeof(volume_table));
	memset(vscale_table, 0, sizeof(vscale_table));
	memset(modulator_vol, 0, sizeof(modulator_vol));
	memset(carrier_vol, 0, sizeof(carrier_vol));
	memset(event_table, 0, sizeof(event_table));
	memset(freq_table, 0, sizeof(freq_table));
	memset(effect_table, 0, sizeof(effect_table));
	memset(effect_table2, 0, sizeof(effect_table2));
	memset(fslide_table, 0, sizeof(fslide_table));
	memset(fslide_table2, 0, sizeof(fslide_table2));
	memset(porta_table, 0, sizeof(porta_table));
	memset(porta_table2, 0, sizeof(porta_table2));
	memset(arpgg_table, 0, sizeof(arpgg_table));
	memset(arpgg_table2, 0, sizeof(arpgg_table2));
	memset(vibr_table, 0, sizeof(vibr_table));
	memset(vibr_table2, 0, sizeof(vibr_table2));
	memset(trem_table, 0, sizeof(trem_table));
	memset(trem_table2, 0, sizeof(trem_table2));
	memset(retrig_table, 0, sizeof(retrig_table));
	memset(retrig_table2, 0, sizeof(retrig_table2));
	memset(tremor_table, 0, sizeof(tremor_table));
	memset(tremor_table2, 0, sizeof(tremor_table2));
	memset(panning_table, 0, sizeof(panning_table));
	memset(last_effect, 0, sizeof(last_effect));
	memset(last_effect2, 0, sizeof(last_effect2));
	memset(voice_table, 0, sizeof(voice_table));
	memset(event_new, 0, sizeof(event_new));
	memset(notedel_table, NONE, sizeof(notedel_table));
	memset(notecut_table, NONE, sizeof(notecut_table));
	memset(ftune_table, 0, sizeof(ftune_table));
	memset(loopbck_table, NONE, sizeof(loopbck_table));
	memset(loop_table, NONE, sizeof(loop_table));
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

	for (int i = 0; i < 20; i++)
		volslide_type[i] = (songdata->lock_flags[i] >> 2) & 3;
}

static void init_player()
{
	opl2out(0x01, 0);

	for (int i = 0; i < 18; i++)
		opl2out(0xb0 + _chan_n[i], 0);

	for (int i = 0x80; i <= 0x8d; i++)
		opl2out(i, NONE);

	for (int i = 0x90; i <= 0x95; i++)
		opl2out(i, NONE);

	misc_register = (tremolo_depth << 7) +
			(vibrato_depth << 6) +
			(percussion_mode << 5);

	opl2out(0x01, 0x20);
	opl2out(0x08, 0x40);
	opl3exp(0x0105);
	opl3exp(0x04 + (songdata->flag_4op << 8));

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

	for (int i = 0; i < 20; i++) {
		arpgg_table[i].state = 1;
		voice_table[i] = i;
	}
}

void stop_playing()
{
	irq_mode = FALSE;
	play_status = isStopped;
	global_volume = 63;
	current_tremolo_depth = tremolo_depth;
	current_vibrato_depth = vibrato_depth;
	pattern_break = FALSE;
	current_order = 0;
	current_pattern = 0;
	current_line = 0;

	for (int i = 0; i < 20; i++)
		release_sustaining_sound(i);
	opl2out(_instr[11], 0);
	opl3exp(0x0004);
	opl3exp(0x0005);
	init_buffers();

	speed = songdata->speed;
	update_timer(songdata->tempo);
}

/* Clean songdata before importing a2t tune */
static void init_songdata()
{
	if (play_status != isStopped)
		stop_playing();
	else
		init_buffers();

	memset(songdata, 0, sizeof(_songdata));
	memset(songdata->pattern_order, 0x80, sizeof(songdata->pattern_order));
	memset(pattdata, 0, sizeof(_pattdata));

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

void start_playing(char *tune)
{
	stop_playing();
	//a2t_import(tune);

	//if (error_code)
	//	return;

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
	tick0 = 0;
	next_line = 0;
	irq_mode = TRUE;
	play_status = isPlaying;

	ticklooper = 0;
	macro_ticklooper = 0;
	speed = songdata->speed;
	macro_speedup = songdata->macro_speedup;
	update_timer(songdata->tempo);
}

/* LOADER FOR A2M/A2T */

int ffver = 1;
int len[21];

typedef struct PACK {
	char id[15];	// '_a2tiny_module_'
	uint32_t crc;
	uint8_t ffver;
	uint8_t npatt;
	uint8_t tempo;
	uint8_t speed;
} A2T_HEADER;

typedef struct PACK {
	char id[10];	// '_a2module_'
	uint32_t crc;
	uint8_t ffver;
	uint8_t npatt;
} A2M_HEADER;

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
	}
}

/* Data for importing A2T format */
typedef struct PACK {
	uint16_t len[6];
} A2T_VARHEADER_V1234;

typedef struct {
	uint8_t common_flag;
	uint16_t len[10];
} A2T_VARHEADER_V5678;

typedef struct PACK {
	uint8_t common_flag;
	uint16_t patt_len;
	uint8_t nm_tracks;
	uint16_t macro_speedup;
	uint32_t len[20];
} A2T_VARHEADER_V9;

typedef struct PACK {
	uint8_t common_flag;
	uint16_t patt_len;
	uint8_t nm_tracks;
	uint16_t macro_speedup;
	uint8_t flag_4op;
	uint8_t lock_flags[20];
	uint32_t len[20];
} A2T_VARHEADER_V10;

typedef struct PACK {
	uint8_t common_flag;
	uint16_t patt_len;
	uint8_t nm_tracks;
	uint16_t macro_speedup;
	uint8_t flag_4op;
	uint8_t lock_flags[20];
	uint32_t len[21];
} A2T_VARHEADER_V11;

typedef union PACK {
	A2T_VARHEADER_V1234 v1234;
	A2T_VARHEADER_V5678 v5678;
	A2T_VARHEADER_V9    v9;
	A2T_VARHEADER_V10   v10;
	A2T_VARHEADER_V11   v11;
} A2T_VARHEADER;

// read the variable part of the header
static int a2t_read_varheader(char *blockptr)
{
	A2T_VARHEADER *varheader = (A2T_VARHEADER *)blockptr;

	switch (ffver) {
	case 1 ... 4:
		for (int i = 0; i < 6; i++)
			len[i] = varheader->v1234.len[i];
		return sizeof(A2T_VARHEADER_V1234);
	case 5 ... 8:
		songdata->common_flag = varheader->v5678.common_flag;
		for (int i = 0; i < 10; i++)
			len[i] = varheader->v5678.len[i];
		return sizeof(A2T_VARHEADER_V5678);
	case 9:
		songdata->common_flag = varheader->v9.common_flag;
		songdata->patt_len = varheader->v9.patt_len;
		songdata->nm_tracks = varheader->v9.nm_tracks;
		songdata->macro_speedup = varheader->v9.macro_speedup;
		for (int i = 0; i < 20; i++)
			len[i] = varheader->v9.len[i];
		return sizeof(A2T_VARHEADER_V9);
	case 10:
		songdata->common_flag = varheader->v10.common_flag;
		songdata->patt_len = varheader->v10.patt_len;
		songdata->nm_tracks = varheader->v10.nm_tracks;
		songdata->macro_speedup = varheader->v10.macro_speedup;
		songdata->flag_4op = varheader->v10.flag_4op;
		for (int i = 0; i < 20; i++)
			songdata->lock_flags[i] = varheader->v10.lock_flags[i];
		for (int i = 0; i < 20; i++)
			len[i] = varheader->v10.len[i];
		return sizeof(A2T_VARHEADER_V10);
	case 11:
		songdata->common_flag = varheader->v11.common_flag;
		songdata->patt_len = varheader->v11.patt_len;
		songdata->nm_tracks = varheader->v11.nm_tracks;
		songdata->macro_speedup = varheader->v11.macro_speedup;
		songdata->flag_4op = varheader->v11.flag_4op;
		for (int i = 0; i < 20; i++)
			songdata->lock_flags[i] = varheader->v10.lock_flags[i];
		for (int i = 0; i < 21; i++)
			len[i] = varheader->v11.len[i];
		return sizeof(A2T_VARHEADER_V11);
	}

	return 0;
}

int a2t_read_instruments(char *src)
{
	int instsize = (ffver < 9 ? 13 : 14);
	int dstsize = ffver < 9 ? 250 * 13 : 255 * 14;
	char *dst = (char *)malloc(dstsize);
	memset(dst, 0, dstsize);

	a2t_depack(src, len[0], dst);

	for (int i = 0; i < (ffver < 9 ? 250 : 255); i++) {
		memcpy(songdata->instr_data[i], dst + i * instsize, instsize);
	}

	FILE *f = fopen("0_inst.dmp", "wb");
	fwrite(songdata->instr_data, 1, sizeof(songdata->instr_data), f);
	fclose(f);

	free(dst);

	return len[0];
}

int a2t_read_instmacros(char *src)
{
	if (ffver < 9) return 0;

	a2t_depack(src, len[1], songdata->instr_macros);

	FILE *f = fopen("1_inst_macro.dmp", "wb");
	fwrite(songdata->instr_macros, 1, sizeof(songdata->instr_macros), f);
	fclose(f);

	return len[1];
}

int a2t_read_macrotable(char *src)
{
	if (ffver < 9) return 0;

	a2t_depack(src, len[2], songdata->macro_table);

	FILE *f = fopen("2_macrotable.dmp", "wb");
	fwrite(songdata->macro_table, 1, sizeof(songdata->macro_table), f);
	fclose(f);

	return len[2];
}

int a2t_read_disabled_fmregs(char *src)
{
	if (ffver < 11) return 0;

	a2t_depack(src, len[3], songdata->dis_fmreg_col);

	FILE *f = fopen("3_fm_disregs.dmp", "wb");
	fwrite(songdata->dis_fmreg_col, 1, sizeof(songdata->dis_fmreg_col), f);
	fclose(f);

	return len[3];
}

int a2t_read_order(char *src)
{
	int blocknum[11] = {1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 4};
	int i = blocknum[ffver - 1];

	a2t_depack(src, len[i], songdata->pattern_order);

	FILE *f = fopen("4_order.dmp", "wb");
	fwrite(songdata->pattern_order, 1, sizeof(songdata->pattern_order), f);
	fclose(f);

	return len[i];
}

// only for importing v 1,2,3,4,5,6,7,8
typedef struct PACK {
	uint8_t note;
	uint8_t instr_def;
	uint8_t effect_def;
	uint8_t effect;
} tADTRACK2_EVENT_V1234;

// for importing v 1,2,3,4 patterns
typedef struct PACK {
  struct PACK {
    struct PACK {
      tADTRACK2_EVENT_V1234 ev;
    } ch[9];
  } row[64];
} tPATTERN_DATA_V1234;

// for importing v 5,6,7,8 patterns
typedef struct PACK {
  struct PACK {
    struct PACK {
      tADTRACK2_EVENT_V1234 ev;
    } row[64];
  } ch[18];
} tPATTERN_DATA_V5678;

// common for both a2t/a2m
static int a2_read_patterns(char *src, int s)
{
	switch (ffver) {
	case 1 ... 4:	// [4][16][64][9][4]
		{
		tPATTERN_DATA_V1234 *old =
			(tPATTERN_DATA_V1234 *)malloc(sizeof(*old) * 16);

		for (int i = 0; i < 4; i++) {
			if (!len[i+s]) continue;

			a2t_depack(src, len[i+s], old);

			for (int p = 0; p < 16; p++) // pattern
			for (int r = 0; r < 64; r++) // row
			for (int c = 0; c < 9; c++) { // channel
				memcpy(&pattdata[i * 16 + p].ch[c].row[r].ev,
					&old[p].row[r].ch[c].ev, 4);
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

			for (int p = 0; p < 8; p++) // pattern
			for (int c = 0; c < 18; c++) // channel
			for (int r = 0; r < 64; r++) { // row
				memcpy(&pattdata[i * 16 + p].ch[c].row[r].ev,
					&old[p].ch[c].row[r].ev, 4);
			}

			src += len[i+s];
		}

		free(old);
		break;
		}
	case 9 ... 11:	// [16][8][20][256][6]
		for (int i = 0; i < 16; i++) {
			if (!len[i+1]) continue;
			a2t_depack(src, len[i+s], &pattdata[i * 16]);
			src += len[i+s];
		}
		break;
	}

	return 0;
}

int a2t_read_patterns(char *src)
{
	int blockstart[11] = {2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 5};
	int s = blockstart[ffver - 1];

	a2_read_patterns(src, s);

	FILE *f = fopen("5_patterns.dmp", "wb");
	fwrite(pattdata, 1, 16*8*20*256*6, f);
	fclose(f);

	return 0;
}

void a2t_import(char *tune)
{
	A2T_HEADER *header = (A2T_HEADER *)tune;
	char *blockptr = tune + sizeof(A2T_HEADER);

	if(strncmp(header->id, "_A2tiny_module_", 15))
		return;

	init_songdata();

	memset(len, 0, sizeof(len));

	ffver = header->ffver;
	songdata->tempo = header->tempo;
	songdata->speed = header->speed;
	songdata->patt_len = 64;
	songdata->nm_tracks = 18;
	songdata->macro_speedup = 1;

	printf("A2T version: %d\n", header->ffver);
	printf("Number of patterns: %d\n", header->npatt);
	printf("Tempo: %d\n", header->tempo);
	printf("Speed: %d\n", header->speed);

	// Read variable part after header, fill len[] with values
	blockptr += a2t_read_varheader(blockptr);

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
}

static int a2m_read_varheader(char *blockptr)
{
	int lensize;
	uint16_t *src16 = (uint16_t *)blockptr;
	uint32_t *src32 = (uint32_t *)blockptr;

	if (ffver < 5) lensize = 5;		// 1,2,3,4 - uint16_t len[5];
	else if (ffver < 9) lensize = 9;	// 5,6,7,8 - uint16_t len[9];
	else lensize = 17;			// 9,10,11 - uint32_t len[17];

	switch (ffver) {
	case 1 ... 8:
		for (int i = 0; i < lensize; i++)
			len[i] = src16[i];

		return lensize * sizeof(uint16_t);
	case 9 ... 11:

		for (int i = 0; i < lensize; i++)
			len[i] = src32[i];

		return lensize * sizeof(uint32_t);
	}

	return 0;
}

/* Data for importing A2M format */
typedef struct PACK {
	char songname[43];
	char composer[43];
	char instr_names[250][33];
	uint8_t instr_data[250][13];
	uint8_t pattern_order[128];
	uint8_t tempo;
	uint8_t speed;
	uint8_t common_flag;		// A2M_SONGDATA_V5678
} A2M_SONGDATA_V1234;

typedef struct PACK {
	char songname[43];
	char composer[43];
	char instr_names[255][33];
	uint8_t instr_data[255][14];
	uint8_t instr_macros[255][3831];
	uint8_t macro_table[255][521];
	uint8_t pattern_order[128];
	uint8_t tempo;
	uint8_t speed;
	uint8_t common_flag;
	uint16_t patt_len;
	uint8_t nm_tracks;
	uint16_t macro_speedup;
} A2M_SONGDATA_V9;

typedef struct PACK {
	char songname[43];
	char composer[43];
	char instr_names[255][43];
	uint8_t instr_data[255][14];
	uint8_t instr_macros[255][3831];
	uint8_t macro_table[255][521];
	uint8_t pattern_order[128];
	uint8_t tempo;
	uint8_t speed;
	uint8_t common_flag;
	uint16_t patt_len;
	uint8_t nm_tracks;
	uint16_t macro_speedup;
	uint8_t flag_4op;
	uint8_t lock_flags[20];
	char pattern_names[128][43];	// A2M_SONGDATA_V11
	int8_t dis_fmreg_col[255][28];	// A2M_SONGDATA_V11
} A2M_SONGDATA_V10;

static int a2m_read_songdata(char *src)
{
	if (ffver < 9) {		// 1,2,3,4,5,6,7,8
		A2M_SONGDATA_V1234 *data =
			malloc(sizeof(*data));
		a2t_depack(src, len[0], data);

		memcpy(songdata->songname, data->songname, 43);
		memcpy(songdata->composer, data->composer, 43);

		for (int i = 0; i < 250; i++) {
			memcpy(songdata->instr_names[i],
				data->instr_names[i], 33);
			memcpy(songdata->instr_data[i],
				data->instr_data[i], 13);
		}

		memcpy(songdata->pattern_order,
			data->pattern_order, 128);

		songdata->tempo = data->tempo;
		songdata->speed = data->speed;

		if (ffver > 4) { // 5,6,7,8
			songdata->common_flag = data->common_flag;
		}

		free(data);
	} else if (ffver == 9) {	// 9
		A2M_SONGDATA_V9 *data =
			malloc(sizeof(*data));
		a2t_depack(src, len[0], data);

		memcpy(songdata->songname, data->songname, 43);
		memcpy(songdata->composer, data->composer, 43);

		for (int i = 0; i < 255; i++) {
			memcpy(songdata->instr_names[i],
				data->instr_names[i], 33);
			memcpy(songdata->instr_data[i],
				data->instr_data[i], 14);
		}

		memcpy(songdata->instr_macros,
			data->instr_macros, 255 * 3831);

		memcpy(songdata->macro_table,
			data->macro_table, 255 * 521);

		memcpy(songdata->pattern_order,
			data->pattern_order, 128);

		songdata->tempo = data->tempo;
		songdata->speed = data->speed;
		songdata->common_flag = data->common_flag;
		songdata->patt_len = data->patt_len;
		songdata->nm_tracks = data->nm_tracks;
		songdata->macro_speedup = data->macro_speedup;

		free(data);
	} else {			// 10,11
		A2M_SONGDATA_V10 *data =
			malloc(sizeof(*data));
		a2t_depack(src, len[0], data);

		memcpy(songdata->songname, data->songname, 43);
		memcpy(songdata->composer, data->composer, 43);

		for (int i = 0; i < 255; i++) {
			memcpy(songdata->instr_names[i],
				data->instr_names[i], 43);
			memcpy(songdata->instr_data[i],
				data->instr_data[i], 14);
		}

		memcpy(songdata->instr_macros,
			data->instr_macros, 255 * 3831);

		memcpy(songdata->macro_table,
			data->macro_table, 255 * 521);

		memcpy(songdata->pattern_order,
			data->pattern_order, 128);

		songdata->tempo = data->tempo;
		songdata->speed = data->speed;
		songdata->common_flag = data->common_flag;
		songdata->patt_len = data->patt_len;
		songdata->nm_tracks = data->nm_tracks;
		songdata->macro_speedup = data->macro_speedup;

		if (ffver == 11) {
			memcpy(songdata->pattern_names,
				data->pattern_names, 128 * 43);
			memcpy(songdata->dis_fmreg_col,
				data->dis_fmreg_col, 255 * 28);
		}

		free(data);
	}

	printf("Tempo: %d\n", songdata->tempo);
	printf("Speed: %d\n", songdata->speed);

	FILE *f = fopen("songdata.dmp", "wb");
	fwrite(songdata, 1, sizeof(*songdata), f);
	fclose(f);

	return len[0];
}

int a2m_read_patterns(char *src)
{
	a2_read_patterns(src, 1);

	FILE *f = fopen("patterns.dmp", "wb");
	fwrite(pattdata, 1, sizeof(_pattdata), f);
	fclose(f);

	return 0;
}

void a2m_import(char *tune)
{
	A2M_HEADER *header = (A2M_HEADER *)tune;
	char *blockptr = tune + sizeof(A2M_HEADER);

	if(strncmp(header->id, "_A2module_", 10))
		return;

	memset(songdata, 0, sizeof(_songdata));
	memset(pattdata, 0, sizeof(_pattdata));
	memset(len, 0, sizeof(len));

	ffver = header->ffver;
	songdata->patt_len = 64;
	songdata->nm_tracks = 18;
	songdata->macro_speedup = 1;

	printf("A2M version: %d\n", header->ffver);
	printf("Number of patterns: %d\n", header->npatt);

	// Read variable part after header, fill len[] with values
	blockptr += a2m_read_varheader(blockptr);

	// Read songdata
	blockptr += a2m_read_songdata(blockptr);

	// Read patterns
	a2m_read_patterns(blockptr);
}

#include <SDL.h>

#define FREQHZ 44100
#define BUFFSMPL 2048

static int framesmpl = FREQHZ / 50;
static int irq_freq = 50;
static int cnt = 0;
static int ticklooper, macro_ticklooper;
static int ym;
static Uint32 buf[BUFFSMPL];
SDL_AudioSpec audio;

static void playcallback(void *unused, Uint8 *stream, int len)
{
	for (int cntr = 0; cntr < len/4 /*BUFFSMPL*/; cntr++) {

		cnt++;
		if (cnt >= framesmpl) {
			cnt = 0;
			if (ticklooper == 0) {
				poll_proc();
				if (irq_freq != tempo * macro_speedup) {
					irq_freq = (tempo < 18 ? 18 : tempo) * macro_speedup;
					framesmpl = FREQHZ / irq_freq;
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
		YMF262UpdateOne(ym, (Sint16 *)&buf[cntr], 1);
	}

	memcpy(stream, buf, len);
#if 0
	if (wavwriter)
		blockwrite(f,buf[0],len);
#endif
}

void opl_out(uint8_t port, uint8_t val)
{
	YMF262Write(ym, port, val);
}

/* MINGW has built-in kbhit() in conio.h */
#ifdef _WIN32
#include <conio.h>
#else
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

int kbhit(void)
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

#undef main
int main(int argc, char *argv[])
{
	char *a2t;

	// if no arguments
	if(argc == 1) {
		printf("Usage: a2t_play.exe *.a2t\n");
		return 1;
	}

	ym = YMF262Init(1, OPL3_INTERNAL_FREQ, FREQHZ);
	YMF262ResetChip(ym);

	SDL_Init(SDL_INIT_EVERYTHING | SDL_INIT_NOPARACHUTE);

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
	if(a2t == NULL) {
		printf("Error reading %s\n", argv[1]);
		return 1;
	}

	a2t_import(a2t);
	a2m_import(a2t);

	SDL_PauseAudio(0);

	start_playing(0);

	printf("Playing - press anything to exit\n");

	while (!kbhit()) {
		printf("Order %03d, Pattern %03d, Row %03d\r",
		       current_order, current_pattern, current_line);
		SDL_Delay(10);
	}

	stop_playing();
	SDL_PauseAudio(1);
	SDL_CloseAudio();

	SDL_Quit();

	YMF262Shutdown();

	return 0;
}
