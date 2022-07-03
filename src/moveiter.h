#ifndef _MOVEITER_H
#define _MOVEITER_H

#include <stdint.h>
#include "move.h"

#define MOVEITER_SORT_NONE 0
#define MOVEITER_SORT_FULL 1

struct Moveiter {
  Move* m;
};

typedef struct Moveiter Moveiter;

// May modify the input list
void moveiter_init(Moveiter* iter, Movelist* list, int mode, Move tt_move);
int moveiter_has_next(Moveiter* iter);
Move moveiter_next(Moveiter* iter);

#endif
