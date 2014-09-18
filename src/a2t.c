
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
	uint8_t adsrw_car;
	struct {
		uint8_t attck,dec,sustn,rel,
		wform;
	} adsrw_mod;
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
	//tREGISTER_TABLE instr_macros[255];   // array[1..255] of tREGISTER_TABLE;
	uint8_t         instr_macros[255][3831];
	//tMACRO_TABLE    macro_table[255];    // array[1..255] of tMACRO_TABLE;
	uint8_t         macro_table[255][521];
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

const uint8_t ef_fix1 = 0x80;
const uint8_t ef_fix2 = 0x90;
const uint8_t keyoff_flag        = 0x80;
const uint8_t fixed_note_flag    = 0x90;
const uint8_t pattern_loop_flag  = 0xe0;
const uint8_t pattern_break_flag = 0xf0;

// This would be later moved to class or struct
tFIXED_SONGDATA _songdata, *songdata = &_songdata;
tPATTERN_DATA _pattdata[128], *pattdata = _pattdata;

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

	FILE *f = fopen("0_inst.dmp", "w");
	fwrite(songdata->instr_data, 1, sizeof(songdata->instr_data), f);
	fclose(f);

	free(dst);

	return len[0];
}

int a2t_read_instmacros(char *src)
{
	if (ffver < 9) return 0;

	a2t_depack(src, len[1], songdata->instr_macros);

	FILE *f = fopen("1_inst_macro.dmp", "w");
	fwrite(songdata->instr_macros, 1, sizeof(songdata->instr_macros), f);
	fclose(f);

	return len[1];
}

int a2t_read_macrotable(char *src)
{
	if (ffver < 9) return 0;

	a2t_depack(src, len[2], songdata->macro_table);

	FILE *f = fopen("2_macrotable.dmp", "w");
	fwrite(songdata->macro_table, 1, sizeof(songdata->macro_table), f);
	fclose(f);

	return len[2];
}

int a2t_read_disabled_fmregs(char *src)
{
	if (ffver < 11) return 0;

	a2t_depack(src, len[3], songdata->dis_fmreg_col);

	FILE *f = fopen("3_fm_disregs.dmp", "w");
	fwrite(songdata->dis_fmreg_col, 1, sizeof(songdata->dis_fmreg_col), f);
	fclose(f);

	return len[3];
}

int a2t_read_order(char *src)
{
	int blocknum[11] = {1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 4};
	int i = blocknum[ffver - 1];

	a2t_depack(src, len[i], songdata->pattern_order);

	FILE *f = fopen("4_order.dmp", "w");
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
			a2t_depack(src, len[i+s], &pattdata[i]);
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

	FILE *f = fopen("5_patterns.dmp", "w");
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

	memset(songdata, 0, sizeof(_songdata));
	memset(pattdata, 0, sizeof(_pattdata));
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

	FILE *f = fopen("songdata.dmp", "w");
	fwrite(songdata, 1, sizeof(*songdata), f);
	fclose(f);

	return len[0];
}

int a2m_read_patterns(char *src)
{
	a2_read_patterns(src, 1);

	FILE *f = fopen("patterns.dmp", "w");
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

int main(int argc, char *argv[])
{
	char *a2t;

	// if no arguments
	if(argc == 1) {
		printf("Usage: a2t_play.exe *.a2t\n");
		return 1;
	}

	a2t = a2t_load(argv[1]);
	if(a2t == NULL) {
		printf("Error reading %s\n", argv[1]);
		return 1;
	}

	a2t_import(a2t);
	a2m_import(a2t);

	return 0;
}