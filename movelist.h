#ifndef _MOVELIST_H
#define _MOVELIST_H

#include "types.h"

Movelist *movelist_create();
void movelist_destroy(Movelist *list);

Movelist *movelist_clone(Movelist *list);

// returns 0 on an empty list
Move movelist_next_move(Movelist *list);

void movelist_prepend_move(Movelist *list, Move move);
void movelist_append_move(Movelist *list, Move move);

// don't touch or worry about append after calling this
// (it's actually linked into list -- calling movelist_destroy
// on it will screw up both of them)
void movelist_append_movelist(Movelist *list, Movelist *append);

int movelist_is_empty(Movelist *list);

#endif
