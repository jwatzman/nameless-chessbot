#ifndef _MOVEITER_H
#define _MOVEITER_H

#include <stdint.h>
#include "move.h"

#define MOVEITER_SORT_NONE 0
#define MOVEITER_SORT_ONDEMAND 1
#define MOVEITER_SORT_FULL 2

struct Moveiter
{
	Move (*next)(struct Moveiter *);
	Movelist *movelist;
	Move forced_first;
	uint8_t pos;
};

typedef struct Moveiter Moveiter;

// May modify the input list
void moveiter_init(Moveiter *iter, Movelist *list, int mode, Move forced_first);
int moveiter_has_next(Moveiter *iter);
Move moveiter_next(Moveiter *iter);

#endif
