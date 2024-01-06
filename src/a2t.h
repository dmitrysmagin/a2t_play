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

typedef struct {
    uint8_t def, val;
} tEFFECT_TABLE;

extern uint8_t voice_table[20];
extern uint16_t freq_table[20];
extern tEFFECT_TABLE effect_table[2][20];
extern tEFFECT_TABLE glfsld_table[2][20];

#endif // _A2T_H_
