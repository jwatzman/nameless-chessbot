#ifndef _BITBOARD_H
#define _BITBOARD_H

#include <stdint.h>
#include "types.h"

void board_init(Bitboard *board);
uint64_t board_rotate_90(uint64_t board);
uint64_t board_rotate_45(uint64_t board);
uint64_t board_rotate_315(uint64_t board);

// this modifies move so that undo works properly!
void board_do_move(Bitboard *board, Move move);

// requires the original move passed to board_do_move
void board_undo_move(Bitboard *board, Move move);

void board_print(uint64_t boards[2][6]);

// for all of these conversions, 0 <= row,col < 8
static inline uint8_t board_index_of(uint8_t row, uint8_t col)
{
	return col | (row << 3);
}

static inline uint8_t board_row_of(uint8_t index)
{
	return (uint8_t)(index / 8);
}

static inline uint8_t board_col_of(uint8_t index)
{
	return index % 8;
}

#endif
