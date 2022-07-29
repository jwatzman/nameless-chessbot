#ifndef _EVALUATE_H
#define _EVALUATE_H

#include "types.h"

int evaluate_board(const Bitboard* board);
int evaluate_traditional(const Bitboard* board);
int16_t evaluate_see(const Bitboard* board, Move m);

#endif
