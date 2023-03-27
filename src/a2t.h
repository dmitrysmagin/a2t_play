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

#endif // _A2T_H_
