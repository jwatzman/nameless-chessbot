#ifndef _BITBOARD_H
#define _BITBOARD_H

#include <stdint.h>
#include "types.h"

void board_init(Bitboard *board);
uint64_t board_rotate_90(uint64_t board);
uint64_t board_rotate_45(uint64_t board);
uint64_t board_rotate_315(uint64_t board);

void board_do_move(Bitboard *board, Move move);
void board_undo_move(Bitboard *board, Move move);

// returns 1 if color's king is in check, 0 otherwise
int board_in_check(Bitboard *board, Color color);

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

// undefined return value if there is no piece at index
static inline Piecetype board_piecetype_at_index(Bitboard *board, uint8_t index)
{
	uint64_t bit_at_index = 1ULL << index;
	Color color;

	if (board->composite_boards[WHITE] & bit_at_index)
		color = WHITE;
	else
		color = BLACK;

	for (Piecetype result = 0; result <= 5; result++)
	{
		if (board->boards[color][result] & bit_at_index)
			return result;
	}

	return -1; // should not get here...
}

#endif
