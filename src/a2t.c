
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <string.h>

#include "depacks.h"
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

typedef struct PACK {
	uint8_t note;
	uint8_t instr_def;
	uint8_t effect_def;
	uint8_t effect;
	uint8_t effect_def2;
	uint8_t effect2;
} tADTRACK2_EVENT;

typedef bool tDIS_FMREG_COL[28]; // array[0..27] of Boolean;

typedef struct PACK {
	tADTRACK2_INS   instr_data[255];     // array[1..255] of tADTRACK2_INS;
	tREGISTER_TABLE instr_macros[255];   // array[1..255] of tREGISTER_TABLE;
	tMACRO_TABLE    macro_table[255];    // array[1..255] of tMACRO_TABLE;
	uint8_t         pattern_order[0x80]; // array[0..0x7f] of Byte;
	uint8_t         tempo;
	uint8_t         speed;
	uint8_t         flags;
	uint16_t        pattlen;
	uint8_t         noftracks;
	uint16_t        macrospeedup;
	uint8_t         op4flags;
	uint8_t         lockflags[20];      // array[1..20]  of Byte;
	tDIS_FMREG_COL  dis_fmreg_col[255];  // array[1..255] of tDIS_FMREG_COL;
} tFIXED_SONGDATA;

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

const uint8_t ef_fix1 = 0x80;
const uint8_t ef_fix2 = 0x90;
const uint8_t keyoff_flag        = 0x80;
const uint8_t fixed_note_flag    = 0x90;
const uint8_t pattern_loop_flag  = 0xe0;
const uint8_t pattern_break_flag = 0xf0;


// FIXME: this will be used by actual player
typedef struct {
	uint8_t         order[128];
	uint8_t         tempo;
	uint8_t         speed;
	uint8_t         flags;
	uint16_t        pattlen;
	uint8_t         noftracks;
	uint16_t        macrospeedup;
	uint8_t         op4flags;
	uint8_t         lockflags[20];      // array[1..20]  of Byte;
} SONGDATA;

SONGDATA _songdata, *songdata = &_songdata;

/* Data for importing A2T format */
typedef struct PACK {
	char id[15];	// '_a2tiny_module_'
	uint32_t crc;
	uint8_t ffver;
	uint8_t npatt;
	uint8_t tempo;
	uint8_t speed;

	union PACK {
		struct PACK {
			uint16_t len[6];
		} v1234;

		struct {
			uint8_t flags;
			uint16_t len[10];
		} v5678;

		struct PACK {
			uint8_t flags;
			uint16_t pattlen;
			uint8_t noftracks;
			uint16_t macrospeedup;
			uint32_t len[20];
		} v9;

		struct PACK {
			uint8_t flags;
			uint16_t pattlen;
			uint8_t noftracks;
			uint16_t macrospeedup;
			uint8_t op4flags;
			uint8_t lockflags[20];
			uint32_t len[20];
		} v10;

		struct PACK {
			uint8_t flags;
			uint16_t pattlen;
			uint8_t noftracks;
			uint16_t macrospeedup;
			uint8_t op4flags;
			uint8_t lockflags[20];
			uint32_t len[21];
		} v11;
	};
} A2T_HEADER;

struct A2M_HEADER {
	char id[10];	// '_a2module_'
	uint32_t crc;
	uint8_t ffver;
	uint8_t npatt;
};

/*
a2m
1,2,3,4 - uint16_t len[5];
5,6,7,8 - uint16_t len[9];
9,10,11 - uint32_t len[17];
a2t
1,2,3,4 - uint16_t len[6];
5,6,7,8 - uint8_t flags; uint16_t len[10];
9       - uint8_t flags; uint16_t pattlen; uint8_t noftracks; uint16_t macrospeedup; uint32_t len[20];
10      - uint8_t flags; uint16_t pattlen; uint8_t noftracks; uint16_t macrospeedup; uint8_t op4ext; uint8_t lockflags[20]; uint32_t len[20];
11      - uint8_t flags; uint16_t pattlen; uint8_t noftracks; uint16_t macrospeedup; uint8_t op4ext; uint8_t lockflags[20]; uint32_t len[21];
*/

// a2m songdata (block 0)
struct A2M_SONGDATA_V1234 {
	char songname[43];
	char composer[43];
	char instnames[250][33];
	uint8_t inst[250][13];
	uint8_t pattorder[128];
	uint8_t tempo;
	uint8_t speed;
	uint8_t flags;		// A2M_SONGDATA_V5678
};

struct A2M_SONGDATA_V9 {
	char songname[43];
	char composer[43];
	char instnames[255][33];
	uint8_t inst[255][14];
	uint8_t macrodef[255][3831];
	uint8_t arpvibmacro[255][521];
	uint8_t pattorder[128];
	uint8_t tempo;
	uint8_t speed;
	uint8_t flags;
	uint16_t pattlength;
	uint8_t numoftracks;
	uint16_t macrospeedup;
};

struct A2M_SONGDATA_V10 {
	char songname[43];
	char composer[43];
	char instnames[255][43];
	uint8_t inst[255][14];
	uint8_t macrodef[255][3831];
	uint8_t arpvibmacro[255][521];
	uint8_t pattorder[128];
	uint8_t tempo;
	uint8_t speed;
	uint8_t flags;
	uint16_t pattlength;
	uint8_t numoftracks;
	uint16_t macrospeedup;
	uint8_t op4ext;
	uint8_t initlocks[20];
	char pattnames[128][43];	// A2M_SONGDATA_V11
	uint8_t fm_disregs[255][28];	// A2M_SONGDATA_V11
};

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

// read the variable part of the header
static int a2t_read_varheader(char *tune, int len[])
{
	A2T_HEADER *header = (A2T_HEADER *)tune;

	switch (header->ffver) {
	case 1 ... 4:
		for (int i = 0; i < 6; i++)
			len[i] = header->v1234.len[i];
		break;
	case 5 ... 8:
		songdata->flags = header->v5678.flags;
		for (int i = 0; i < 10; i++)
			len[i] = header->v5678.len[i];
		break;
	case 9:
		songdata->flags = header->v9.flags;
		songdata->pattlen = header->v9.pattlen;
		songdata->noftracks = header->v9.noftracks;
		songdata->macrospeedup = header->v9.macrospeedup;
		for (int i = 0; i < 20; i++)
			len[i] = header->v9.len[i];
		break;
	case 10:
		songdata->flags = header->v10.flags;
		songdata->pattlen = header->v10.pattlen;
		songdata->noftracks = header->v10.noftracks;
		songdata->macrospeedup = header->v10.macrospeedup;
		songdata->op4flags = header->v10.op4flags;
		for (int i = 0; i < 20; i++)
			songdata->lockflags[i] = header->v10.lockflags[i];
		for (int i = 0; i < 20; i++)
			len[i] = header->v10.len[i];
		break;
	case 11:
		songdata->flags = header->v11.flags;
		songdata->pattlen = header->v11.pattlen;
		songdata->noftracks = header->v11.noftracks;
		songdata->macrospeedup = header->v11.macrospeedup;
		songdata->op4flags = header->v11.op4flags;
		for (int i = 0; i < 20; i++)
			songdata->lockflags[i] = header->v10.lockflags[i];
		for (int i = 0; i < 21; i++)
			len[i] = header->v11.len[i];
		break;
	}
}

void a2t_import(char *tune)
{
	A2T_HEADER *header = (A2T_HEADER *)tune;
	int ffver; // FIXME: move to global or songdata
	int maxlengths[11] = {6, 6, 6, 6, 10, 10, 10, 10, 20, 20, 21};
	int len[21], maxlen; // max possible blocks
	int blockoffsets[11] = {
		0x23, 0x23, 0x23, 0x23, 0x2c, 0x2c, 0x2c, 0x2c, 0x6d, 0x82, 0x86
	};
	char *blockptr;

	if(strncmp(header->id, "_A2tiny_module_", 15))
		return;

	memset(songdata, 0, sizeof(*songdata));
	memset(len, 0, sizeof(len));

	ffver = header->ffver;
	songdata->tempo = header->tempo;
	songdata->speed = header->speed;
	songdata->pattlen = 64;
	songdata->noftracks = 18;
	songdata->macrospeedup = 1;

	printf("Version: %d\n", header->ffver);
	printf("Number of patterns: %d\n", header->npatt);
	printf("Tempo: %d\n", header->tempo);
	printf("Speed: %d\n", header->speed);

	a2t_read_varheader(tune, len);

	// Roll thru blocks
	maxlen = maxlengths[ffver - 1];
	blockptr = tune + blockoffsets[ffver - 1];

	// Read instruments; all versions
	int instrsize = ffver < 9 ? 250 * 13 : 255 * 14;
	char *p = (char *)malloc(instrsize);

	if (!aP_depack_safe(blockptr, len[0], p, instrsize)) {
		printf("Error\n");
	}

	FILE *f = fopen("inst.dmp", "w");
	fwrite(p, 1, instrsize, f);
	fclose(f);
	free(p);

	// Read instrument macro (v >= 9,10,11)
	if (ffver >= 9) {

	}

	for (int i = 0; i < maxlen; i++) {
		printf("Block %d, offset: %x, size: %d\n", i, blockptr - tune, len[i]);

		blockptr += len[i];
	}
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

	return 0;
}