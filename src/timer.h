#ifndef _TIMER_H
#define _TIMER_H

#include <stdint.h>
#include <time.h>

void timer_init_xboard(char* level);
void timer_init_secs(unsigned int n);
void timer_begin(void);
uint8_t timer_timeup(void);
uint8_t timer_stop_deepening(void);
void timer_end(void);

time_t timer_get_centiseconds(void);

#endif
