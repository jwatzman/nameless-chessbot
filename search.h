#ifndef _SEARCH_H
#define _SEARCH_H

#include "types.h"

void search_force_max_depth(const int depth);
Move search_find_move(Bitboard *board);

#endif
