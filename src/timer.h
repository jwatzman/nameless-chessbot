#ifndef _TIMER_H
#define _TIMER_H

void timer_init_xboard(char *level);
void timer_init_secs(int n);
void timer_begin(volatile int *i);
void timer_end(void);

#endif
