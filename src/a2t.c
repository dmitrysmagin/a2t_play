
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <string.h>

#include "depack.h"
#include "sixpack.h"
#include "lzh.h"
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
	struct PACK {
		uint8_t		num_4op;
		uint8_t		idx_4op[128];
	} ins_4op_flags;
	uint8_t			reserved_data[1024];
	struct PACK {
		uint8_t		rows_per_beat;
		int16_t		tempo_finetune;
	} bpm_data;
} tFIXED_SONGDATA;

typedef enum {
	isPlaying = 0, isPaused, isStopped
} tPLAY_STATUS;

typedef struct PACK {
	uint8_t note;
	uint8_t instr_def;
	//uint8_t effect_def;
	//uint8_t effect;
	//uint8_t effect_def2;
	//uint8_t effect2;
	struct PACK {
		uint8_t def;
		uint8_t val;
	} eff[2];
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

#define INSTRUMENT_SIZE sizeof(tADTRACK2_INS)
#define CHUNK_SIZE sizeof(tADTRACK2_EVENT)
#define PATTERN_SIZE (20*256*CHUNK_SIZE)
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

int _4op_tracks_hi[] = {1, 3, 5, 10, 12, 14 };
int _4op_tracks_lo[] = {2, 4, 6, 11, 13, 15 };

#define INCLUDES(ARRAY, VALUE) \
	({ \
		int len = (sizeof((ARRAY))/sizeof((ARRAY)[0])); \
		int res = FALSE; \
		while (--len >= 0) { \
			res = (ARRAY[len] == (VALUE)); \
		} \
		res; })

uint8_t _4op_main_chan[6] = { 2, 4, 6, 11, 13, 15 };

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
bool force_macro_keyon = FALSE;

const uint8_t pattern_loop_flag  = 0xe0;
const uint8_t pattern_break_flag = 0xf0;

tFM_PARAMETER_TABLE fmpar_table[20];	// array[1..20] of tFM_PARAMETER_TABLE;
bool volume_lock[20];			// array[1..20] of Boolean;
bool vol4op_lock[20] ;			// array[1..20] of Boolean;
uint16_t volume_table[20];		// array[1..20] of Word;
uint16_t vscale_table[20];		// array[1..20] of Word;
bool peak_lock[20];			// array[1..20] of Boolean;
bool pan_lock[20];			// array[1..20] of Boolean;
uint8_t modulator_vol[20];		// array[1..20] of Byte;
uint8_t carrier_vol[20];		// array[1..20] of Byte;
tADTRACK2_EVENT event_table[20];	// array[1..20] of tADTRACK2_EVENT;
uint8_t voice_table[20];		// array[1..20] of Byte;
uint16_t freq_table[20];		// array[1..20] of Word;
uint16_t zero_fq_table[20];		// array[1..20] of Word;
uint16_t effect_table[2][20];	// array[1..20] of Word;
uint8_t fslide_table[2][20];		// array[1..20] of Byte;
uint16_t glfsld_table[2][20];	// array[1..20] of Word;
struct PACK {
	uint16_t freq;
	uint8_t speed;
} porta_table[2][20];	// array[1..20] of Record freq: Word; speed: Byte; end;
bool portaFK_table[20]; // array[1..20] of Boolean;;
struct PACK {
	uint8_t state, note, add1, add2;
} arpgg_table[2][20];		// array[1..20] of Record state,note,add1,add2: Byte; end;
struct PACK {
	uint8_t pos, dir, speed, depth;
	bool fine;
} vibr_table[2][20];		// array[1..20] of Record pos,speed,depth: Byte; fine: Boolean; end;
struct PACK {
	uint8_t pos, dir, speed, depth;
	bool fine;
} trem_table[2][20];		// array[1..20] of Record pos,speed,depth: Byte; fine: Boolean; end;
uint8_t retrig_table[2][20];	// array[1..20] of Byte;
struct PACK {
	int pos;
	uint16_t volume;
} tremor_table[2][20];		// array[1..20] of Record pos: Integer; volume: Word; end;
uint8_t panning_table[20];	// array[1..20] of Byte;
uint16_t last_effect[2][20];	// array[1..20] of Byte;
uint8_t volslide_type[20];	// array[1..20] of Byte;
bool event_new[20];			// array[1..20] of Boolean;
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

double time_playing;

int ticks, tickD, tickXF;

#define FreqStart 0x156
#define FreqEnd   0x2ae
#define FreqRange  (FreqEnd - FreqStart)

/* PLAYER */
static void opl_out(uint8_t port, uint8_t val); // forward def
static bool is_4op_chan(int chan); // forward def


static void opl2out(uint16_t reg, uint16_t data)
{
	opl_out(0, reg);
	opl_out(1, data);
}

static void opl3out(uint16_t reg, uint16_t data)
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

static bool is_data_empty(void *data, unsigned int size)
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

static uint16_t calc_vibrato_shift(uint8_t depth, uint8_t position)
{
	uint8_t vibr[32] = {
		0,24,49,74,97,120,141,161,180,197,212,224,235,244,250,253,255,
		253,250,244,235,224,212,197,180,161,141,120,97,74,49,24
	};

	/* ATTENTION: wtf this calculation should be ? */
	return (vibr[position & 0x1f] * depth) >> 6;
}

#define LO(A) ((A) & 0xFF)
#define HI(A) (((A) >> 8) & 0xFF)

static void change_freq(int chan, uint16_t freq)
{
	freq_table[chan] &= ~0x1fff;
	freq_table[chan] |= (freq & 0x1fff);
	opl3out(0xa0 + _chan_n[chan], LO(freq_table[chan]));
	opl3out(0xb0 + _chan_n[chan], HI(freq_table[chan]));

	if (is_4op_chan(chan)) {
		freq_table[chan - 1] = freq_table[chan];
	}
}

static inline uint8_t ins_parameter(uint8_t ins, uint8_t param)
{
	// NOTE: adjust ins
	return songdata->instr_data[ins-1][param];
}

static bool is_chan_adsr_data_empty(int chan)
{
	return
		((fmpar_table[chan].adsrw_car.attck == 0) &&
		(fmpar_table[chan].adsrw_mod.attck == 0) &&
		(fmpar_table[chan].adsrw_car.dec == 0) &&
		(fmpar_table[chan].adsrw_mod.dec == 0) &&
		(fmpar_table[chan].adsrw_car.sustn == 0) &&
		(fmpar_table[chan].adsrw_mod.sustn == 0) &&
		(fmpar_table[chan].adsrw_car.rel == 0) &&
		(fmpar_table[chan].adsrw_mod.rel == 0));
}

static bool is_ins_adsr_data_empty(int ins)
{
	return
		(((ins_parameter(ins, 5) >> 4) == 0) &&
		 ((ins_parameter(ins, 4) >> 4) == 0) &&
		 ((ins_parameter(ins, 5) & 0x0f) == 0) &&
		 ((ins_parameter(ins, 4) & 0x0f) == 0) &&
		 ((ins_parameter(ins, 7) >> 4) == 0) &&
		 ((ins_parameter(ins, 6) >> 4) == 0) &&
		 ((ins_parameter(ins, 7) & 0x0f) == 0) &&
		 ((ins_parameter(ins, 6) & 0x0f) == 0));
}

static inline uint16_t max(uint16_t value, uint16_t maximum)
{
	return (value > maximum ? maximum : value);
}

static inline uint16_t concw(uint8_t lo, uint8_t hi)
{
	return (lo | (hi << 8));
}

static void change_frequency(int chan, uint16_t freq)
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

static void key_on(int chan)
{
	if (is_4op_chan(chan) && INCLUDES(_4op_tracks_hi, chan)) {
		opl3out(0xb0 + _chan_n[chan + 1], 0);
	} else {
		opl3out(0xb0 + _chan_n[chan], 0);
	}
}

static void key_off(int chan)
{
	freq_table[chan] &= ~0x2000;
	change_frequency(chan, freq_table[chan]);
}

static void release_sustaining_sound(int chan)
{
	opl3out(_instr[2] + _chan_m[chan], 63);
	opl3out(_instr[3] + _chan_c[chan], 63);

	memset(&fmpar_table[chan].adsrw_car, 0,
		sizeof(fmpar_table[chan].adsrw_car));
	memset(&fmpar_table[chan].adsrw_mod, 0,
		sizeof(fmpar_table[chan].adsrw_mod));

	key_on(chan);
	opl3out(_instr[4] + _chan_m[chan], BYTE_NULL);
	opl3out(_instr[5] + _chan_c[chan], BYTE_NULL);
	opl3out(_instr[6] + _chan_m[chan], BYTE_NULL);
	opl3out(_instr[7] + _chan_c[chan], BYTE_NULL);

	key_off(chan);
	event_table[chan].instr_def = 0;
	reset_chan[chan] = TRUE;
}

static uint8_t scale_volume(uint8_t volume, uint8_t scale_factor)
{
	return 63 - ((63 - volume) *
		(63 - scale_factor) / 63);
}

static bool _4op_vol_valid_chan(int chan)
{
	uint32_t _4op_flag = 0; // = _4op_data_flag(chan);
	return ((_4op_flag & 1) && vol4op_lock[chan] &&
			((_4op_flag >> 11) & 1) &&
			((_4op_flag >> 19) & 1));
}

static void set_ins_volume(uint8_t modulator, uint8_t carrier, int chan)
{
	uint8_t temp;

	// ** OPL3 emulation workaround **
	// force muted instrument volume with missing channel ADSR data
	// when there is additionally no FM-reg macro defined for this instrument
	if (is_chan_adsr_data_empty(chan) &&
		!(songdata->instr_macros[voice_table[chan]].length)) {
			modulator = 63;
			carrier = 63;
	}

	if (modulator != BYTE_NULL) {
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

	if (carrier != BYTE_NULL) {
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

static void reset_ins_volume(int chan)
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
				set_ins_volume(BYTE_NULL, HI(volume_table[chan]), chan);
			} else {
				set_ins_volume(LO(volume_table[chan]), HI(volume_table[chan]), chan);
			}
		}
	}
}

void a2t_volume(unsigned char level)
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
	macro_table[chan].fmreg_table = ins-1;
	macro_table[chan].arpg_count = 1;
	macro_table[chan].arpg_pos = 0;
	macro_table[chan].arpg_table =
		songdata->instr_macros[ins-1].arpeggio_table;
	macro_table[chan].arpg_note = note;
	macro_table[chan].vib_count = 1;
	macro_table[chan].vib_pos = 0;
	macro_table[chan].vib_table =
		songdata->instr_macros[ins-1].vibrato_table;
	macro_table[chan].vib_freq = freq;
	macro_table[chan].vib_delay =
		songdata->macro_table[macro_table[chan].vib_table-1].vibrato.delay;
	zero_fq_table[chan] = 0;
}

static void set_ins_data(uint8_t ins, int chan)
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

static void update_modulator_adsrw(int chan)
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

static void update_carrier_adsrw(int chan)
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

static void update_fmpar(int chan)
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

static bool is_4op_chan(int chan) // 0..17
{
	char mask[18] = {
		(1<<0), (1<<0), (1<<1), (1<<1), (1<<2), (1<<2),
		0, 0, 0,
		(1<<3), (1<<3), (1<<4), (1<<4), (1<<5), (1<<5),
		0, 0, 0
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
	return (chan > 17 ? FALSE : songdata->flag_4op & mask[chan]);
}

static void output_note(uint8_t note, uint8_t ins, int chan, bool restart_macro, int NR)
{
	uint16_t freq;

	if ((note == 0) && (ftune_table[chan] == 0)) return;

	if ((note == 0) || (note > 12*8+1)) { // If NOT (note in [1..12*8+1])
		freq = freq_table[chan];
	} else {
		freq = nFreq(note - 1) + (int8_t)ins_parameter(ins, 12);
		if (!NR)
			opl3out(0xb0 + _chan_n[chan], 0);
		freq_table[chan] = concw(LO(freq_table[chan]),
					  HI(freq_table[chan]) | 0x20);
	}

	if (ftune_table[chan] == -127)
		ftune_table[chan] = 0;

	freq = freq + ftune_table[chan];
	change_frequency(chan, freq);

	if (note != 0) {
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

static void process_effects(tADTRACK2_EVENT *event, int slot, int chan)
{
	unsigned int def = event->eff[slot].def;
	unsigned int val = event->eff[slot].val;

	// Use previous effect value if needed
	if (!val && def && def == event_table[chan].eff[slot].def)
		val = event_table[chan].eff[slot].val;

	event_table[chan].eff[slot].def = def;
	event_table[chan].eff[slot].val = val;

#if 0
	// FIXME: Check if this code is really needed
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

	if ((arpgg_table[slot][chan].state != 1) && (def != ef_ExtraFineArpeggio)) {
		arpgg_table[slot][chan].state = 1;
		change_frequency(chan, nFreq(arpgg_table[slot][chan].note - 1) +
			(int8_t)ins_parameter(event_table[chan].instr_def, 12));
	}

	if ((tremor_table[slot][chan].pos != 0) && (def != ef_Tremor)) {
		tremor_table[slot][chan].pos = 0;
		set_ins_volume(LO(tremor_table[slot][chan].volume),
			       HI(tremor_table[slot][chan].volume), chan);
	}
#endif

	switch (def) {
	case ef_Arpeggio:
		if (def + val == 0)
			break;

	case ef_ExtraFineArpeggio:
	case ef_ArpggVSlide:
	case ef_ArpggVSlideFine:
		if (((event->note & 0x7f) >= 1) && ((event->note & 0x7f) <= 12 * 8 + 1)) {
			arpgg_table[slot][chan].state = 0;
			arpgg_table[slot][chan].note = event->note & 0x7f;
			if ((def == ef_Arpeggio) ||
				(def == ef_ExtraFineArpeggio)) {
				arpgg_table[slot][chan].add1 = val / 16;
				arpgg_table[slot][chan].add2 = val % 16;
			}
		} else {
			if ((event->note == 0) &&
				(((event_table[chan].note & 0x7f) >= 1) &&
				(event_table[chan].note & 0x7f) <= 12 * 8 + 1)) {
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
				// Check if this really needed
				event_table[chan].eff[slot].def = 0;
				event_table[chan].eff[slot].val = 0;
			}
		}
		break;

	case ef_FSlideUp:
	case ef_FSlideDown:
	case ef_FSlideUpFine:
	case ef_FSlideDownFine:
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
		break;

	case ef_TonePortamento:
		if ((event->note >= 1) && (event->note <= 12 * 8 + 1)) {
			porta_table[slot][chan].freq = nFreq(event->note - 1) +
				(int8_t)ins_parameter(event_table[chan].instr_def, 12);
		}
		porta_table[slot][chan].speed = val;
		break;

	case ef_TPortamVolSlide:
	case ef_TPortamVSlideFine:
		break;

	case ef_Vibrato:
	case ef_ExtraFineVibrato:
		if ((event->eff[slot ^ 1].def == ef_Extended) &&
		    (event->eff[slot ^ 1].val == ef_ex_ExtendedCmd2 * 16 + ef_ex_cmd2_FVib_FGFS)) {
			vibr_table[slot][chan].fine = TRUE;
		}

		vibr_table[slot][chan].speed = val / 16;
		vibr_table[slot][chan].depth = val % 16;
		break;

	case ef_Tremolo:
	case ef_ExtraFineTremolo:
		if ((event->eff[slot ^ 1].def == ef_Extended) &&
		    (event->eff[slot ^ 1].val == ef_ex_ExtendedCmd2 * 16 + ef_ex_cmd2_FTrm_XFGFS)) {
			trem_table[slot][chan].fine = TRUE;
		}

		trem_table[slot][chan].speed = val / 16;
		trem_table[slot][chan].depth = val % 16;
		break;

	case ef_VibratoVolSlide:
	case ef_VibratoVSlideFine:
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
		if (percussion_mode && ((chan >= 16) && (chan <= 19))) { //  in [17..20]
			set_ins_volume(63 - val, BYTE_NULL, chan);
		} else {
			if ((ins_parameter(voice_table[chan], 10) & 1) == 0) {
				set_ins_volume(BYTE_NULL, 63 - val, chan);
			} else {
				set_ins_volume(63 - val, 63 - val, chan);
			}
		}
		break;

	case ef_ForceInsVolume:
		if (percussion_mode && ((chan >= 16) && (chan <= 19))) { //  in [17..20]
			set_ins_volume(63 - val, BYTE_NULL, chan);
		} else {
			set_ins_volume(scale_volume(ins_parameter(voice_table[chan], 2) & 0x3f,
				       63 - val), 63 - val, chan);
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
			next_line = max(val, songdata->patt_len - 1);
		}
		break;

	case ef_SetSpeed:
		ticks = speed = val;
		break;

	case ef_SetTempo:
		update_timer(val);
		break;

	case ef_SetWaveform:
		if (val / 16 <= 7) { // in [0..7]
			fmpar_table[chan].adsrw_car.wform = val / 16;
			update_carrier_adsrw(chan);
		}

		if (val % 16 <= 7) { // in [0..7]
			fmpar_table[chan].adsrw_mod.wform = val % 16;
			update_modulator_adsrw(chan);
		}
		break;

	case ef_VolSlide:
		break;

	case ef_VolSlideFine:
		break;

	case ef_RetrigNote:
		break;

	case ef_SetGlobalVolume:
		global_volume = val;
		set_global_volume();
		break;

	case ef_MultiRetrigNote:
		break;

	case ef_Tremor:
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
			fmpar_table[chan].adsrw_mod.attck = val % 16;
			update_modulator_adsrw(chan);
			break;

		case ef_ex_SetDecayRateM:
			fmpar_table[chan].adsrw_mod.dec = val % 16;
			update_modulator_adsrw(chan);
			break;

		case ef_ex_SetSustnLevelM:
			fmpar_table[chan].adsrw_mod.sustn = val % 16;
			update_modulator_adsrw(chan);
			break;

		case ef_ex_SetRelRateM:
			fmpar_table[chan].adsrw_mod.rel = val % 16;
			update_modulator_adsrw(chan);
			break;

		case ef_ex_SetAttckRateC:
			fmpar_table[chan].adsrw_car.attck = val % 16;
			update_carrier_adsrw(chan);
			break;

		case ef_ex_SetDecayRateC:
			fmpar_table[chan].adsrw_car.dec = val % 16;
			update_carrier_adsrw(chan);
			break;

		case ef_ex_SetSustnLevelC:
			fmpar_table[chan].adsrw_car.sustn = val % 16;
			update_carrier_adsrw(chan);
			break;

		case ef_ex_SetRelRateC:
			fmpar_table[chan].adsrw_car.rel = val % 16;
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
					if (INCLUDES(_4op_tracks_hi, chan)) {
						vol4op_lock[chan + 1] = FALSE;
					} else {
						vol4op_lock[chan - 1] = FALSE;
					}
				}
				break;
			case ef_ex_cmd_4opVlockOn:
				if (is_4op_chan(chan)) {
					vol4op_lock[chan] = TRUE;
					if (INCLUDES(_4op_tracks_hi, chan)) {
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
				set_ins_volume(LO(volume_table[chan]),
					       HI(volume_table[chan]), chan);
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
			notedel_table[chan] = val % 16;
			break;

		case ef_ex2_NoteCut:
			notecut_table[chan] = val % 16;
			break;

		case ef_ex2_FineTuneUp:
			ftune_table[chan] += val % 16;
			break;

		case ef_ex2_FineTuneDown:
			ftune_table[chan] -= val % 16;
			break;

		case ef_ex2_GlVolSlideUp:
			break;

		case ef_ex2_GlVolSlideDn:
			break;

		case ef_ex2_GlVolSlideUpF:
			break;

		case ef_ex2_GlVolSlideDnF:
			break;

		case ef_ex2_GlVolSldUpXF:
			break;

		case ef_ex2_GlVolSldDnXF:
			break;

		case ef_ex2_VolSlideUpXF:
			break;

		case ef_ex2_VolSlideDnXF:
			break;

		case ef_ex2_FreqSlideUpXF:
			break;

		case ef_ex2_FreqSlideDnXF:
			break;
		}
		break;

	case ef_Extended3:
		switch  (val / 16) {
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

static void process_note(tADTRACK2_EVENT *event, int chan)
{
	if (event->note == BYTE_NULL) {
		key_off(chan);
		event_table[chan].note = 0;
	} else {
		event_table[chan].note = event->note;

		int notedelay1 = ((event->eff[0].def == ef_Extended2) &&
				  (event->eff[0].val / 16 == ef_ex2_NoteDelay));
		int notedelay2 = ((event->eff[1].def == ef_Extended2) &&
				  (event->eff[0].val / 16 == ef_ex2_NoteDelay));

		int def0 = event_table[chan].eff[0].def;
		int def1 = event_table[chan].eff[1].def;

		if (((def0 != ef_TonePortamento) &&
		     (def0 != ef_TPortamVolSlide) &&
		     (def0 != ef_TPortamVSlideFine) &&
		     (!notedelay1)) &&
		    ((def1 != ef_TonePortamento) &&
		     (def1 != ef_TPortamVolSlide) &&
		     (def1 != ef_TPortamVSlideFine) &&
		     (!notedelay2))) {
			if (!(((event->eff[1].def == ef_SwapArpeggio) ||
			       (event->eff[1].def == ef_SwapVibrato)) &&
			       (event->eff[0].def == ef_Extended) &&
			       (event->eff[0].val == ef_ex_ExtendedCmd2 * 16 + ef_ex_cmd2_NoRestart)) &&
			    !(((event->eff[0].def == ef_SwapArpeggio) ||
			       (event->eff[0].def == ef_SwapVibrato)) &&
			       (event->eff[1].def == ef_Extended) &&
			       (event->eff[1].val == ef_ex_ExtendedCmd2 * 16 + ef_ex_cmd2_NoRestart))) {
				output_note(event->note, voice_table[chan], chan, TRUE, 0);
			} else {
				output_note(event->note, voice_table[chan], chan, TRUE, 1);
			}
		}
	}
}

static int calc_following_order(uint8_t order);

static void play_line()
{
	tADTRACK2_EVENT *event;

	if ((current_line == 0) &&
		(current_order == calc_following_order(0)))
		time_playing = 0;

	if (!(pattern_break && ((next_line & 0xf0) == pattern_loop_flag)) &&
		 (current_order != last_order)) {
		memset(loopbck_table, BYTE_NULL, sizeof(loopbck_table));
		memset(loop_table, BYTE_NULL, sizeof(loop_table));
		last_order = current_order;
	}

	for (int chan = 0; chan < songdata->nm_tracks; chan++) {
		event = &pattdata[current_pattern].ch[chan].row[current_line].ev;

		// put inside process_effects ?
		for (int slot = 0; slot < 2; slot++) {
			if (effect_table[slot][chan] != 0) {
				last_effect[slot][chan] = effect_table[slot][chan];
			}
			if (glfsld_table[slot][chan] != 0) {
				effect_table[slot][chan] = glfsld_table[slot][chan];
			} else {
				effect_table[slot][chan] = effect_table[slot][chan] & 0xff00;
			}
		}
		ftune_table[chan] = 0;

        process_note(event, chan);

		if (event->instr_def != 0) {
			// NOTE: adjust ins
			if (is_data_empty(songdata->instr_data[event->instr_def-1], INSTRUMENT_SIZE)) {
				release_sustaining_sound(chan);
			}
			set_ins_data(event->instr_def, chan);
		}

		process_effects(event, 0, chan);
		process_effects(event, 1, chan);

		// process_note(event, chan); // was here

		check_swap_arp_vibr(event, 0, chan);
		check_swap_arp_vibr(event, 1, chan);

		update_fine_effects(0, chan);
		update_fine_effects(1, chan);
	}
}

static void check_swap_arp_vibr(tADTRACK2_EVENT *event, int slot, int chan)
{
	// Check if second effect is ZFF - force no restart
	bool is_norestart = ((((event->eff[slot ^ 1].def << 8) | event->eff[slot ^ 1].val)) ==
			    ((ef_Extended << 8) | (ef_ex_ExtendedCmd2 << 4) | ef_ex_cmd2_NoRestart));

	switch (event->eff[slot].def) {
	case ef_SwapArpeggio:
		if (is_norestart) {
			if (macro_table[chan].arpg_pos >
			    songdata->macro_table[event->eff[slot].val-1].arpeggio.length)
				macro_table[chan].arpg_pos =
					songdata->macro_table[event->eff[slot].val-1].arpeggio.length;
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
			    songdata->macro_table[event->eff[slot].val-1].vibrato.length)
				macro_table[chan].vib_pos =
					songdata->macro_table[event->eff[slot].val-1].vibrato.length;
			macro_table[chan].vib_table = event->eff[slot].val;
		} else {
			macro_table[chan].vib_count = 1;
			macro_table[chan].vib_pos = 0;
			macro_table[chan].vib_table = event->eff[slot].val;
			macro_table[chan].vib_delay =
				songdata->macro_table[macro_table[chan].vib_table-1].vibrato.delay;
		}
		break;
	}
}

static void portamento_up(int chan, uint16_t slide, uint16_t limit)
{
	uint16_t freq;

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

	freq = calc_freq_shift_down(freq_table[chan] & 0x1fff, slide);
	if (freq >= limit) {
		change_frequency(chan, freq);
	} else {
		change_frequency(chan, limit);
	}
}

static void macro_vibrato__porta_up(int chan, uint8_t depth)
{
	uint16_t freq;

	freq = calc_freq_shift_up(macro_table[chan].vib_freq & 0x1fff, depth);
	if (freq <= nFreq(12*8+1)) {
		change_freq(chan, freq);
	} else {
		change_freq(chan, nFreq(12*8+1));
	}
}

static void macro_vibrato__porta_down(int chan, uint8_t depth)
{
	uint16_t freq;
	freq = calc_freq_shift_down(macro_table[chan].vib_freq & 0x1fff, depth);
	if (freq >= nFreq(0)) {
		change_freq(chan, freq);
	} else {
		change_freq(chan,nFreq(0));
	}
}

static void tone_portamento(int slot, int chan)
{
	if ((freq_table[chan] & 0x1fff) > porta_table[slot][chan].freq) {
		portamento_down(chan, porta_table[slot][chan].speed, porta_table[slot][chan].freq);
	} else {
		if ((freq_table[chan] & 0x1fff) < porta_table[slot][chan].freq)
			portamento_up(chan, porta_table[slot][chan].speed, porta_table[slot][chan].freq);
	}
}

static void slide_volume_up(int chan, uint8_t slide)
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

		set_ins_volume(BYTE_NULL, HI(temp), chan);
		volume_table[chan] = temp;
		if (((ins_parameter(voice_table[chan], 10) & 1) == 1) ||
		     (percussion_mode && (chan >= 17 && chan <= 20))) {
			vLo = LO(temp);
			vHi = HI(temp);
			if (vLo - slide >= limit2)
				temp = concw(vLo - slide, vHi);
			else
				temp = concw(limit2, vHi);
			set_ins_volume(LO(temp), BYTE_NULL, chan);
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
		set_ins_volume(BYTE_NULL, HI(temp), chan);
		volume_table[chan] = temp;
	break;

	case 2:
		vLo = LO(temp);
		vHi = HI(temp);
		if (vLo - slide >= limit2)
			temp = concw(vLo - slide, vHi);
		else
			temp = concw(limit2, vHi);
		set_ins_volume(LO(temp), BYTE_NULL, chan);
		volume_table[chan] = temp;
	break;

	case 3:
		vLo = LO(temp);
		vHi = HI(temp);
		if (vHi - slide >= limit1)
			temp = concw(vLo, vHi - slide);
		else
			temp = concw(vLo, limit1);
		set_ins_volume(BYTE_NULL, HI(temp), chan);
		volume_table[chan] = temp;
		vLo = LO(temp);
		vHi = HI(temp);
		if (vLo - slide >= limit2)
			temp = concw(vLo - slide, vHi);
		else
			temp = concw(limit2, vHi);
		set_ins_volume(LO(temp), BYTE_NULL, chan);
		volume_table[chan] = temp;
	break;
	}
}

static void slide_volume_down(int chan, uint8_t slide)
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
		set_ins_volume(BYTE_NULL, HI(temp), chan);
		volume_table[chan] = temp;
		if (((ins_parameter(voice_table[chan], 10) & 1) == 1) ||
		     (percussion_mode && (chan >= 17 && chan <= 20))) {
			vLo = LO(temp);
			vHi = HI(temp);
			if (vLo + slide <= 63)
				temp = concw(vLo + slide, vHi);
			else
				temp = concw(63, vHi);
			set_ins_volume(LO(temp), BYTE_NULL, chan);
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
		set_ins_volume(BYTE_NULL, HI(temp), chan);
		volume_table[chan] = temp;
	break;

	case 2:
		vLo = LO(temp);
		vHi = HI(temp);
		if (vLo + slide <= 63)
			temp = concw(vLo + slide, vHi);
		else
			temp = concw(63, vHi);
		set_ins_volume(LO(temp), BYTE_NULL, chan);
		volume_table[chan] = temp;
	break;

	case 3:
		vLo = LO(temp);
		vHi = HI(temp);
		if (vHi + slide <= 63)
			temp = concw(vLo, vHi + slide);
		else
			temp = concw(vLo, 63);
		set_ins_volume(BYTE_NULL, HI(temp), chan);
		volume_table[chan] = temp;
		vLo = LO(temp);
		vHi = HI(temp);
		if (vLo + slide <= 63)
			temp = concw(vLo + slide, vHi);
		else
			temp = concw(63, vHi);
		set_ins_volume(LO(temp), BYTE_NULL, chan);
		volume_table[chan] = temp;
	break;
	}
}

static void volume_slide(int chan, uint8_t up_speed, uint8_t down_speed)
{
	if (up_speed != 0)
		slide_volume_up(chan, up_speed);
	else {
		if (down_speed != 0)
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
	}

	arpgg_table[slot][chan].state = arpgg_state[arpgg_table[slot][chan].state];
	change_frequency(chan, freq +
			(int8_t)(ins_parameter(event_table[chan].instr_def, 12)));
}

static void vibrato(int slot, int chan)
{
	uint16_t freq, old_freq;
	uint8_t direction;

	vibr_table[slot][chan].pos += vibr_table[slot][chan].speed;
	freq = calc_vibrato_shift(vibr_table[slot][chan].depth, vibr_table[slot][chan].pos);
	direction = vibr_table[slot][chan].pos & 0x20;
	old_freq = freq_table[chan];
	if (direction == 0)
		portamento_down(chan, freq, nFreq(0));
	else
		portamento_up(chan, freq, nFreq(12*8+1));
	freq_table[chan] = old_freq;
}

static void tremolo(int slot, int chan)
{
	uint16_t vol, old_vol;
	uint8_t direction;

	trem_table[slot][chan].pos += trem_table[slot][chan].speed;
	vol = calc_vibrato_shift(trem_table[slot][chan].depth, trem_table[slot][chan].pos);
	direction = trem_table[slot][chan].pos & 0x20;
	old_vol = volume_table[chan];
	if (direction == 0)
		slide_volume_down(chan, vol);
	else
		slide_volume_up(chan, vol);
	volume_table[chan] = old_vol;
}

static int chanvol(int chan)
{
	if ((ins_parameter(voice_table[chan], 10) & 1) == 0)
		return 63 - HI(volume_table[chan]);
	else
		return 63 - (LO(volume_table[chan]) + HI(volume_table[chan])) / 2;
}

static void update_effects_slot(int slot, int chan)
{
	int def, val;

	def = event_table[chan].eff[slot].def;
	val = event_table[chan].eff[slot].val;

	switch (def) {
	case ef_Arpeggio:
		if (def + val == 0)
			break;
		arpeggio(slot, chan);
		break;

	case ef_ArpggVSlide:
		volume_slide(chan, val / 16, val % 16);
		arpeggio(slot, chan);
		break;

	case ef_ArpggVSlideFine:
		arpeggio(slot, chan);
		break;

	case ef_FSlideUp:
		portamento_up(chan, val, nFreq(12*8+1));
		break;

	case ef_FSlideDown:
		portamento_down(chan, val, nFreq(0));
		break;

	case ef_FSlideUpVSlide:
		portamento_up(chan, fslide_table[slot][chan], nFreq(12*8+1));
		volume_slide(chan, val / 16, val % 16);
		break;

	case ef_FSlUpVSlF:
		portamento_up(chan, fslide_table[slot][chan], nFreq(12*8+1));
		break;

	case ef_FSlideDownVSlide:
		portamento_down(chan, fslide_table[slot][chan], nFreq(0));
		volume_slide(chan, val / 16, val % 16);
		break;

	case ef_FSlDownVSlF:
		portamento_down(chan, fslide_table[slot][chan], nFreq(0));
		break;

	case ef_FSlUpFineVSlide:
		volume_slide(chan, val / 16, val % 16);
		break;

	case ef_FSlDownFineVSlide:
		volume_slide(chan, val / 16, val % 16);
		break;

	case ef_TonePortamento:
		tone_portamento(slot, chan);
		break;

	case ef_TPortamVolSlide:
		volume_slide(chan, val / 16, val % 16);
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
		volume_slide(chan, val / 16, val % 16);
		if (!vibr_table[slot][chan].fine)
			vibrato(slot, chan);
		break;

	case ef_VibratoVSlideFine:
		if (!vibr_table[slot][chan].fine)
			vibrato(slot, chan);
		break;

	case ef_VolSlide:
		volume_slide(chan, val / 16, val % 16);
		break;

	case ef_RetrigNote:
		if (retrig_table[slot][chan] >= val) {
			retrig_table[slot][chan] = 0;
			output_note(event_table[chan].note,
				    event_table[chan].instr_def,
				    chan, TRUE, 0);
		} else {
			retrig_table[slot][chan]++;
		}
		break;

	case ef_MultiRetrigNote:
		if (retrig_table[slot][chan] >= val / 16) {
			switch (val % 16) {
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

			retrig_table[slot][chan] = 0;
			output_note(event_table[chan].note,
				    event_table[chan].instr_def,
				    chan, TRUE, 0);
		} else {
			retrig_table[slot][chan]++;
		}
		break;

	case ef_Tremor:
		if (tremor_table[slot][chan].pos >= 0) {
			if ((tremor_table[slot][chan].pos + 1) <= val / 16) {
				tremor_table[slot][chan].pos++;
			} else {
				slide_volume_down(chan, 63);
				tremor_table[slot][chan].pos = -1;
			}
		} else {
			if ((tremor_table[slot][chan].pos - 1) >= -(val % 16)) {
				tremor_table[slot][chan].pos--;
			} else {
				set_ins_volume(LO(tremor_table[slot][chan].volume),
					       HI(tremor_table[slot][chan].volume), chan);
				tremor_table[slot][chan].pos = 1;
			}
		}
		break;

	case ef_Extended2:
		switch (val / 16) {
		case ef_ex2_NoteDelay:
			if (notedel_table[chan] == 0) {
				notedel_table[chan] = BYTE_NULL;
				output_note(event_table[chan].note,
					    event_table[chan].instr_def,
					    chan, TRUE, 0);
			} else {
				notedel_table[chan]--;
			}
			break;

		case ef_ex2_NoteCut:
			if (notecut_table[chan] == 0) {
				notecut_table[chan] = BYTE_NULL;
				key_off(chan);
			} else {
				notecut_table[chan]--;
			}
			break;

		case ef_ex2_GlVolSlideUp:
			global_volume_slide(val % 16, BYTE_NULL);
			break;

		case ef_ex2_GlVolSlideDn:
			global_volume_slide(BYTE_NULL, val % 16);
			break;
		}
		break;
	}
}

static void update_effects()
{
	for (int chan = 0; chan < 20; chan++) {
		update_effects_slot(0, chan);
		update_effects_slot(1, chan);
	}
}

static void update_fine_effects(int slot, int chan)
{
	int def, val;

	def = event_table[chan].eff[slot].def;
	val = event_table[chan].eff[slot].val;

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

	case ef_Extended2:
		switch (val / 16) {
		case ef_ex2_GlVolSlideUpF:
			global_volume_slide(val % 16, BYTE_NULL);
			break;
		case ef_ex2_GlVolSlideDnF:
			global_volume_slide(BYTE_NULL, val % 16);
			break;
		}
		break;
	}
}

static void update_extra_fine_effects_slot(int slot, int chan)
{
	int def, val;

	def = event_table[chan].eff[slot].def;
	val = event_table[chan].eff[slot].val;

	switch (def) {
	case ef_Extended2:
		switch (val / 16) {
		case ef_ex2_GlVolSldUpXF:  global_volume_slide(val % 16, BYTE_NULL); break;
		case ef_ex2_GlVolSldDnXF:  global_volume_slide(BYTE_NULL, val % 16); break;
		case ef_ex2_VolSlideUpXF:  volume_slide(chan, val % 16, 0); break;
		case ef_ex2_VolSlideDnXF:  volume_slide(chan, 0, val % 16); break;
		case ef_ex2_FreqSlideUpXF: portamento_up(chan, val % 16, nFreq(12*8+1)); break;
		case ef_ex2_FreqSlideDnXF: portamento_down(chan, val % 16, nFreq(0)); break;
		}
		break;

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
	for (int chan = 0; chan < 20; chan++) {
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
		glfsld_table[0][chan] = 0;
		glfsld_table[1][chan] = 0;
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

						// force KEY-ON with missing ADSR instrument data
						force_macro_keyon = FALSE;
						if (mt->fmreg_pos == 1) {
							if (is_ins_adsr_data_empty(voice_table[chan]) &&
								!(songdata->dis_fmreg_col[mt->fmreg_table][0] &&
								  songdata->dis_fmreg_col[mt->fmreg_table][1] &&
								  songdata->dis_fmreg_col[mt->fmreg_table][2] &&
								  songdata->dis_fmreg_col[mt->fmreg_table][3] &&
								  songdata->dis_fmreg_col[mt->fmreg_table][12] &&
								  songdata->dis_fmreg_col[mt->fmreg_table][13] &&
								  songdata->dis_fmreg_col[mt->fmreg_table][14] &&
								  songdata->dis_fmreg_col[mt->fmreg_table][15])) {
								force_macro_keyon = TRUE;
							}
						}

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
								       BYTE_NULL, chan);

						if (!songdata->dis_fmreg_col[mt->fmreg_table][17])
							set_ins_volume(BYTE_NULL,
									63 - (d->fm_data.KSL_VOLUM_carrier & 0x3f), chan);

						update_modulator_adsrw(chan);
						update_carrier_adsrw(chan);
						update_fmpar(chan);

						if (force_macro_keyon ||
							!((d->fm_data.FEEDBACK_FM | 0x80) != d->fm_data.FEEDBACK_FM)) { // MACRO_NOTE_RETRIG_FLAG
							if (!((is_4op_chan(chan) && INCLUDES(_4op_tracks_hi, chan)))) {
								output_note(event_table[chan].note,
											event_table[chan].instr_def, chan, FALSE, 0);
								if ((is_4op_chan(chan) && INCLUDES(_4op_tracks_lo, chan)))
									init_macro_table(chan - 1, 0, voice_table[chan - 1], 0);
							}
						} else {
							if (!((d->fm_data.FEEDBACK_FM | 0x40) != d->fm_data.FEEDBACK_FM)) { // MACRO_ENVELOPE_RESTART_FLAG
								key_on(chan);
								change_freq(chan, freq_table[chan]);
							} else {
								if (!((d->fm_data.FEEDBACK_FM | 0x40) != d->fm_data.FEEDBACK_FM)) { // MACRO_ZERO_FREQ_FLAG
									if (freq_table[chan]) {
										zero_fq_table[chan] = freq_table[chan];
										freq_table[chan] = freq_table[chan] & ~0x1fff;
										change_freq(chan, freq_table[chan]);
									} else {} // ??? original has 'else else'
								} else {
									if (zero_fq_table[chan]) {
										freq_table[chan] = zero_fq_table[chan];
										zero_fq_table[chan] = 0;
										change_freq(chan, freq_table[chan]);
									}
								}
							}
						}

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

		tARPEGGIO_TABLE *at = &songdata->macro_table[mt->arpg_table-1].arpeggio;

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

		tVIBRATO_TABLE *vt = &songdata->macro_table[mt->vib_table-1].vibrato;

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
	memset(event_new, FALSE, sizeof(event_new));
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
		arpgg_table[0][i].state = 1;
		voice_table[i] = i;
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
	case 12 ... 14: // lzh
		LZH_decompress(src, dst, srcsize);
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
	case 11 ... 14:
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
		memcpy(songdata->instr_data[i], dst + i * instsize, instsize);
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
	tADTRACK2_EVENT_V1234 new = *ev;

	switch (ev->effect_def) {
	case fx_Arpeggio:			new.effect_def = ef_Arpeggio;		break;
	case fx_FSlideUp:			new.effect_def = ef_FSlideUp;		break;
	case fx_FSlideDown:			new.effect_def = ef_FSlideDown;		break;
	case fx_FSlideUpFine:		new.effect_def = ef_FSlideUpFine;	break;
	case fx_FSlideDownFine:		new.effect_def = ef_FSlideDownFine;	break;
	case fx_TonePortamento:		new.effect_def = ef_TonePortamento;	break;
	case fx_TPortamVolSlide:	new.effect_def = ef_TPortamVolSlide;break;
	case fx_Vibrato:			new.effect_def = ef_Vibrato;		break;
	case fx_VibratoVolSlide:	new.effect_def = ef_VibratoVolSlide;break;
	case fx_SetInsVolume:		new.effect_def = ef_SetInsVolume;	break;
	case fx_PatternJump:		new.effect_def = ef_PositionJump;	break;
	case fx_PatternBreak:		new.effect_def = ef_PatternBreak;	break;
	case fx_SetTempo:			new.effect_def = ef_SetSpeed;		break;
	case fx_SetTimer:			new.effect_def = ef_SetTempo;		break;
	case fx_SetOpIntensity: {
		if (ev->effect & 0xf0) {
			new.effect_def = ef_SetCarrierVol;
			new.effect = (ev->effect >> 4) * 4 + 3;
		} else if (ev->effect & 0x0f) {
			new.effect_def = ef_SetModulatorVol;
			new.effect = (ev->effect & 0x0f) * 4 + 3;
		} else new.effect_def = 0;
		break;
	}
	case fx_Extended: {
		switch (ev->effect >> 4) {
		case fx_ex_DefAMdepth:
			new.effect_def = ef_Extended;
			new.effect = ef_ex_SetTremDepth << 4 | (ev->effect & 0x0f);
			break;
		case fx_ex_DefVibDepth:
			new.effect_def = ef_Extended;
			new.effect = ef_ex_SetVibDepth << 4 | (ev->effect & 0x0f);
			break;
		case fx_ex_DefWaveform:
			new.effect_def = ef_SetWaveform;
			if ((ev->effect & 0x0f) < 4) {
				new.effect = ((ev->effect & 0x0f) << 4) | 0x0f; // 0..3
			} else {
				new.effect = ((ev->effect & 0x0f) - 4) | 0xf0; // 4..7
			}
			break;
		case fx_ex_VSlideUp:
			new.effect_def = ef_VolSlide;
			new.effect = (ev->effect & 0x0f) << 4;
			break;
		case fx_ex_VSlideDown:
			new.effect_def = ef_VolSlide;
			new.effect = ev->effect & 0x0f;
			break;
		case fx_ex_VSlideUpFine:
			new.effect_def = ef_VolSlideFine;
			new.effect = (ev->effect & 0x0f) << 4;
			break;
		case fx_ex_VSlideDownFine:
			new.effect_def = ef_VolSlideFine;
			new.effect = ev->effect & 0x0f;
			break;
		case fx_ex_ManSlideUp:
			new.effect_def = ef_Extended2;
			new.effect = (ef_ex2_FineTuneUp << 4) | (ev->effect & 0x0f);
			break;
		case fx_ex_ManSlideDown:
			new.effect_def = ef_Extended2;
			new.effect = (ef_ex2_FineTuneDown << 4) | (ev->effect & 0x0f);
			break;
		case fx_ex_RetrigNote:
			new.effect_def = ef_RetrigNote;
			new.effect = (ev->effect & 0x0f) + 1;
			break;
		case fx_ex_SetAttckRate:
			new.effect_def = ef_Extended;
			new.effect = ev->effect & 0x0f;
			if (!adsr_carrier[chan]) {
				new.effect |= ef_ex_SetAttckRateM << 4;
			} else {
				new.effect |= ef_ex_SetAttckRateC << 4;
			}
			break;
		case fx_ex_SetDecayRate:
			new.effect_def = ef_Extended;
			new.effect = ev->effect & 0x0f;
			if (!adsr_carrier[chan]) {
				new.effect |= ef_ex_SetDecayRateM << 4;
			} else {
				new.effect |= ef_ex_SetDecayRateC << 4;
			}
			break;
		case fx_ex_SetSustnLevel:
			new.effect_def = ef_Extended;
			new.effect = ev->effect & 0x0f;
			if (!adsr_carrier[chan]) {
				new.effect |= ef_ex_SetSustnLevelM << 4;
			} else {
				new.effect |= ef_ex_SetSustnLevelC << 4;
			}
			break;
		case fx_ex_SetReleaseRate:
			new.effect_def = ef_Extended;
			new.effect = ev->effect & 0x0f;
			if (!adsr_carrier[chan]) {
				new.effect |= ef_ex_SetRelRateM << 4;
			} else {
				new.effect |= ef_ex_SetRelRateC << 4;
			}
			break;
		case fx_ex_SetFeedback:
			new.effect_def = ef_Extended;
			new.effect = (ef_ex_SetFeedback << 4) | (ev->effect & 0x0f);
			break;
		case fx_ex_ExtendedCmd:
			new.effect_def = ef_Extended;
			new.effect = ef_ex_ExtendedCmd2 << 4;
			if ((ev->effect & 0x0f) < 10) {
				// FIXME: Should be a parameter
				bool whole_song = FALSE;

				switch (ev->effect & 0x0f) {
				case 0: new.effect |= ef_ex_cmd2_RSS;		break;
				case 1: new.effect |= ef_ex_cmd2_LockVol;	break;
				case 2: new.effect |= ef_ex_cmd2_UnlockVol;	break;
				case 3: new.effect |= ef_ex_cmd2_LockVP;	break;
				case 4: new.effect |= ef_ex_cmd2_UnlockVP;	break;
				case 5:
					new.effect_def = (whole_song ? 255 : 0);
					new.effect = 0;
					adsr_carrier[chan] = TRUE;
					break;
				case 6:
					new.effect_def = (whole_song ? 255 : 0);
					new.effect = (whole_song ? 1 : 0);
					adsr_carrier[chan] = FALSE;
					break;
				case 7: new.effect |= ef_ex_cmd2_VSlide_car;	break;
				case 8: new.effect |= ef_ex_cmd2_VSlide_mod;	break;
				case 9: new.effect |= ef_ex_cmd2_VSlide_def;	break;
				}
			} else {
				new.effect_def = 0;
				new.effect = 0;
			}
			break;
		}
		break;
	}
	default:
		new.effect_def = 0;
		new.effect = 0;
	}

	*ev = new;
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

			for (int p = 0; p < 16; p++) // pattern
			for (int r = 0; r < 64; r++) // row
			for (int c = 0; c < 9; c++) { // channel
				convert_v1234_event(&old[p].row[r].ch[c].ev, c);
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
	case 9 ... 14:	// [16][8][20][256][6]
		for (int i = 0; i < 16; i++) {
			if (!len[i+s]) continue;
			a2t_depack(src, len[i+s], &pattdata[i * 8]);
			src += len[i+s];
		}
		break;
	}

	return 0;
}

static int a2t_read_patterns(char *src)
{
	int blockstart[14] = {2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 5, 5, 5, 5};
	int s = blockstart[ffver - 1];

	a2_read_patterns(src, s);

#if 0
	FILE *f = fopen("5_patterns.dmp", "wb");
	fwrite(pattdata, 1, 16*8*20*256*6, f);
	fclose(f);
#endif

	return 0;
}

static void a2t_import(char *tune)
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
}

static int a2m_read_varheader(char *blockptr, int npatt)
{
	int lensize;
	int maxblock = (ffver < 5 ? npatt / 16 : npatt / 8) + 1;
	uint16_t *src16 = (uint16_t *)blockptr;
	uint32_t *src32 = (uint32_t *)blockptr;

	if (ffver < 5) lensize = 5;		// 1,2,3,4 - uint16_t len[5];
	else if (ffver < 9) lensize = 9;	// 5,6,7,8 - uint16_t len[9];
	else lensize = 17;			// 9,10,11 - uint32_t len[17];

	switch (ffver) {
	case 1 ... 8:
		// skip possible rubbish (MARIO.A2M)
		for (int i = 0; (i < lensize) && (i <= maxblock); i++)
			len[i] = src16[i];

		return lensize * sizeof(uint16_t);
	case 9 ... 14:
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
	uint8_t flag_4op;				// A2M_SONGDATA_V10
	uint8_t lock_flags[20];			// A2M_SONGDATA_V10
	char pattern_names[128][43];	// A2M_SONGDATA_V11
	int8_t dis_fmreg_col[255][28];	// A2M_SONGDATA_V11
	struct PACK {
		uint8_t		num_4op;
		uint8_t		idx_4op[128];
	} ins_4op_flags;				// A2M_SONGDATA_V12_13
	uint8_t			reserved_data[1024]; // A2M_SONGDATA_V12_13
	struct PACK {
		uint8_t		rows_per_beat;
		int16_t		tempo_finetune;
	} bpm_data;						// A2M_SONGDATA_V14
} A2M_SONGDATA_V9_14;

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
	} else {			// 9 - 14
		A2M_SONGDATA_V9_14 *data =
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
		songdata->bpm_data.tempo_finetune = data->bpm_data.tempo_finetune;

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

	printf("Tempo: %d\n", songdata->tempo);
	printf("Speed: %d\n", songdata->speed);

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

#if 0
	FILE *f = fopen("patterns.dmp", "wb");
	fwrite(pattdata, 1, sizeof(_pattdata), f);
	fclose(f);
#endif

	return 0;
}

static void a2m_import(char *tune)
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
	blockptr += a2m_read_varheader(blockptr, header->npatt);

	// Read songdata
	blockptr += a2m_read_songdata(blockptr);

	// Read patterns
	a2m_read_patterns(blockptr);
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
static int ym;

static void opl_out(uint8_t port, uint8_t val)
{
	YMF262Write(ym, port, val);
}

void a2t_init(int freq)
{
	freqhz = freq;
	framesmpl = freq / 50;
	ym = YMF262Init(1, OPL3_INTERNAL_FREQ, freq);
	YMF262ResetChip(ym);
}

void a2t_shut()
{
	YMF262Shutdown();
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
		YMF262UpdateOne(ym, (int16_t *)(stream + cntr), 1);
		cnt++;
	}
}
