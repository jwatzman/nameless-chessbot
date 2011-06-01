#ifndef _TIMER_H
#define _TIMER_H

void timer_init(char *level);
void timer_begin(volatile int *i);
void timer_end(void);

#endif
