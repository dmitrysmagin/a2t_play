
#include <stdio.h>
#include <stdlib.h>
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

typedef tADTRACK2_EVENT tPATTERN_DATA[16][8][20][256];
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
	uint8_t         instr_data[255][14];
	uint8_t         instr_macros[255][3831];
	uint8_t         macro_table[255][521];
	uint8_t         fm_disregs[255][28];
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

// This would be later moved to class or struct
SONGDATA _songdata, *songdata = &_songdata;
//tPATTERN_DATA _pattdata, *pattdata = &_pattdata;
uint8_t _pattdata[16*8*20*256*6], *pattdata = _pattdata;

/* Data for importing A2T format */
typedef struct PACK {
	uint16_t len[6];
} A2T_VARHEADER_V1234;

typedef struct {
	uint8_t flags;
	uint16_t len[10];
} A2T_VARHEADER_V5678;

typedef struct PACK {
	uint8_t flags;
	uint16_t pattlen;
	uint8_t noftracks;
	uint16_t macrospeedup;
	uint32_t len[20];
} A2T_VARHEADER_V9;

typedef struct PACK {
	uint8_t flags;
	uint16_t pattlen;
	uint8_t noftracks;
	uint16_t macrospeedup;
	uint8_t op4flags;
	uint8_t lockflags[20];
	uint32_t len[20];
} A2T_VARHEADER_V10;

typedef struct PACK {
	uint8_t flags;
	uint16_t pattlen;
	uint8_t noftracks;
	uint16_t macrospeedup;
	uint8_t op4flags;
	uint8_t lockflags[20];
	uint32_t len[21];
} A2T_VARHEADER_V11;

typedef union PACK {
	A2T_VARHEADER_V1234 v1234;
	A2T_VARHEADER_V5678 v5678;
	A2T_VARHEADER_V9    v9;
	A2T_VARHEADER_V10   v10;
	A2T_VARHEADER_V11   v11;
} A2T_VARHEADER;

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
static int a2t_read_varheader(int ffver, char *blockptr, int len[])
{
	int blockoffsets[11] = {
		0x23, 0x23, 0x23, 0x23, 0x2c, 0x2c, 0x2c, 0x2c, 0x6d, 0x82, 0x86
	};

	A2T_VARHEADER *varheader = (A2T_VARHEADER *)blockptr;

	switch (ffver) {
	case 1 ... 4:
		for (int i = 0; i < 6; i++)
			len[i] = varheader->v1234.len[i];
		break;
	case 5 ... 8:
		songdata->flags = varheader->v5678.flags;
		for (int i = 0; i < 10; i++)
			len[i] = varheader->v5678.len[i];
		break;
	case 9:
		songdata->flags = varheader->v9.flags;
		songdata->pattlen = varheader->v9.pattlen;
		songdata->noftracks = varheader->v9.noftracks;
		songdata->macrospeedup = varheader->v9.macrospeedup;
		for (int i = 0; i < 20; i++)
			len[i] = varheader->v9.len[i];
		break;
	case 10:
		songdata->flags = varheader->v10.flags;
		songdata->pattlen = varheader->v10.pattlen;
		songdata->noftracks = varheader->v10.noftracks;
		songdata->macrospeedup = varheader->v10.macrospeedup;
		songdata->op4flags = varheader->v10.op4flags;
		for (int i = 0; i < 20; i++)
			songdata->lockflags[i] = varheader->v10.lockflags[i];
		for (int i = 0; i < 20; i++)
			len[i] = varheader->v10.len[i];
		break;
	case 11:
		songdata->flags = varheader->v11.flags;
		songdata->pattlen = varheader->v11.pattlen;
		songdata->noftracks = varheader->v11.noftracks;
		songdata->macrospeedup = varheader->v11.macrospeedup;
		songdata->op4flags = varheader->v11.op4flags;
		for (int i = 0; i < 20; i++)
			songdata->lockflags[i] = varheader->v10.lockflags[i];
		for (int i = 0; i < 21; i++)
			len[i] = varheader->v11.len[i];
		break;
	}

	return blockoffsets[ffver - 1];
}

int a2t_read_instruments(int ffver, char *src, int len[])
{
	int instsize = (ffver < 9 ? 13 : 14);
	int dstsize = ffver < 9 ? 250 * 13 : 255 * 14;
	char *p = (char *)malloc(dstsize);

	if (!aP_depack(src, p)) {
		printf("Error\n");
		free(p);
		return 0;
	}

	FILE *f = fopen("0_inst.dmp", "w");
	fwrite(p, 1, dstsize, f);
	fclose(f);

	for (int i = 0; i < (ffver < 9 ? 250 : 255); i++) {
		memcpy(songdata->instr_data, src + i * instsize, instsize);
	}

	free(p);

	return len[0];
}

int a2t_read_instmacros(int ffver, char *src, int len[])
{
	if (ffver < 9) return 0;

	if (!aP_depack(src, songdata->instr_macros)) {
		printf("Error\n");

		return 0;
	}

	FILE *f = fopen("1_inst_macro.dmp", "w");
	fwrite(songdata->instr_macros, 1, sizeof(songdata->instr_macros), f);
	fclose(f);

	return len[1];
}

int a2t_read_macrotable(int ffver, char *src, int len[])
{
	if (ffver < 9) return 0;

	if (!aP_depack(src, songdata->macro_table)) {
		printf("Error\n");

		return 0;
	}

	FILE *f = fopen("2_macrotable.dmp", "w");
	fwrite(songdata->macro_table, 1, sizeof(songdata->macro_table), f);
	fclose(f);

	return len[2];
}

int a2t_read_disabled_fmregs(int ffver, char *src, int len[])
{
	if (ffver < 11) return 0;

	if (!aP_depack(src, songdata->fm_disregs)) {
		printf("Error\n");

		return 0;
	}

	FILE *f = fopen("3_fm_disregs.dmp", "w");
	fwrite(songdata->fm_disregs, 1, sizeof(songdata->fm_disregs), f);
	fclose(f);

	return len[3];
}

int a2t_read_order(int ffver, char *src, int len[])
{
	int blocknum[11] = {1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 4};
	int i = blocknum[ffver - 1];

	if (!aP_depack(src, songdata->order)) {
		printf("Error\n");

		return 0;
	}

	FILE *f = fopen("4_order.dmp", "w");
	fwrite(songdata->order, 1, sizeof(songdata->order), f);
	fclose(f);

	return len[i];
}

int a2t_read_patterns(int ffver, char *src, int len[])
{
	int blockstart[11] = {2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 5};
	int blockend[11] = {5, 5, 5, 5, 9, 9, 9, 9, 19, 19, 20};
	int s, e;

	s = blockstart[ffver - 1];
	e = blockend[ffver - 1];

	for (int i = s; i <= e; i++) {
		if (!len[i]) continue;

		switch (ffver) {
		case 1 ... 4:
		case 5 ... 8:
			// FIXME: add loading for old formats
			break;
		case 9 ... 11:
			printf("Block %d, len %d\n", i, len[i]);
			printf("Pattern: %x\n", (i-s) * 8*20*256*6);
			if (!aP_depack(src, pattdata + (i-s)* 8*20*256*6)) {
				printf("Pattern unpack error\n");
			}
			break;
		}

		src += len[i];
	}

	FILE *f = fopen("5_patterns.dmp", "w");
	fwrite(pattdata, 1, 16*8*20*256*6, f);
	fclose(f);

	return 0;
}

void a2t_import(char *tune)
{
	A2T_HEADER *header = (A2T_HEADER *)tune;
	int ffver; // FIXME: move to global or songdata

	int len[21]; // FIXME: move to global or songdata
	char *blockptr = tune + sizeof(A2T_HEADER);

	if(strncmp(header->id, "_A2tiny_module_", 15))
		return;

	memset(songdata, 0, sizeof(*songdata));
	memset(pattdata, 0, sizeof(*pattdata));
	memset(len, 0, sizeof(len));

	ffver = header->ffver;
	songdata->tempo = header->tempo;
	songdata->speed = header->speed;
	songdata->pattlen = 64;
	songdata->noftracks = 18;
	songdata->macrospeedup = 1;

	printf("A2T version: %d\n", header->ffver);
	printf("Number of patterns: %d\n", header->npatt);
	printf("Tempo: %d\n", header->tempo);
	printf("Speed: %d\n", header->speed);

	// Read variable part after header, fill len[] with values
	blockptr += a2t_read_varheader(ffver, blockptr, len);

	// Read instruments; all versions
	blockptr += a2t_read_instruments(ffver, blockptr, len);

	// Read instrument macro (v >= 9,10,11)
	blockptr += a2t_read_instmacros(ffver, blockptr, len);

	// Read arpeggio/vibrato macro table (v >= 9,10,11)
	blockptr += a2t_read_macrotable(ffver, blockptr, len);

	// Read disabled fm regs (v == 11)
	blockptr += a2t_read_disabled_fmregs(ffver, blockptr, len);

	// Read order
	blockptr += a2t_read_order(ffver, blockptr, len);

	// Read patterns
	a2t_read_patterns(ffver, blockptr, len);
}

/*
a2m
1,2,3,4 - uint16_t len[5];
5,6,7,8 - uint16_t len[9];
9,10,11 - uint32_t len[17];
*/

static int a2m_read_varheader(int ffver, char *blockptr, int len[])
{
	int lensize;
	uint16_t *src16 = (uint16_t *)blockptr;
	uint32_t *src32 = (uint32_t *)blockptr;

	if (ffver < 5) lensize = 5;
	else if (ffver < 9) lensize = 9;
	else lensize = 17;

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

void a2m_import(char *tune)
{
	A2M_HEADER *header = (A2M_HEADER *)tune;
	int ffver; // FIXME: move to global or songdata

	int len[17]; // FIXME: move to global or songdata
	char *blockptr = tune + sizeof(A2M_HEADER);

	if(strncmp(header->id, "_A2module_", 10))
		return;

	memset(songdata, 0, sizeof(*songdata));
	memset(pattdata, 0, sizeof(*pattdata));
	memset(len, 0, sizeof(len));

	ffver = header->ffver;
	//songdata->tempo = header->tempo;
	//songdata->speed = header->speed;
	songdata->pattlen = 64;
	songdata->noftracks = 18;
	songdata->macrospeedup = 1;

	printf("A2M version: %d\n", header->ffver);
	printf("Number of patterns: %d\n", header->npatt);
	//printf("Tempo: %d\n", header->tempo);
	//printf("Speed: %d\n", header->speed);

	// Read variable part after header, fill len[] with values
	blockptr += a2m_read_varheader(ffver, blockptr, len);

	for (int i = 0; i < 17; i++) {
		printf("Block %d size %x\n", i, len[i]);
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
	a2m_import(a2t);

	return 0;
}