#ifndef _SEARCH_H
#define _SEARCH_H

#include "types.h"

typedef struct {
	uint8_t maxDepth;
	const char *stopMove;
} SearchDebug;

Move search_find_move(Bitboard *board, const SearchDebug *debug);

#endif
