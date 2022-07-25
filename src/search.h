#ifndef _SEARCH_H
#define _SEARCH_H

#include "types.h"

#define max_possible_depth 30

typedef struct {
  uint8_t maxDepth;
  uint8_t continueOnMate;
  const char* stopMove;
  int* score;
} SearchDebug;

Move search_find_move(Bitboard* board, const SearchDebug* debug);

uint64_t search_benchmark(void);

#endif
