#ifndef _TIMER_H
#define _TIMER_H

#include <signal.h>

void timer_init_xboard(char* level);
void timer_init_secs(unsigned int n);
void timer_begin(volatile sig_atomic_t* i);
void timer_end(void);

#endif
