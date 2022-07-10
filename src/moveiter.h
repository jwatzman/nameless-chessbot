#ifndef _MOVEITER_H
#define _MOVEITER_H

#include <stdint.h>
#include "move.h"

struct Moveiter {
  Move* m;
};

typedef struct Moveiter Moveiter;

// May modify the input list
void moveiter_init(Moveiter* iter,
                   Movelist* list,
                   Move tt_move,
                   const Move* killers);
int moveiter_has_next(Moveiter* iter);
Move moveiter_next(Moveiter* iter);

#endif
