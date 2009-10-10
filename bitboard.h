#ifndef _BITBOARD_H
#define _BITBOARD_H

#include <stdint.h>
#include "types.h"

// writes initial state to a board
void board_init(Bitboard *board);

/* make and reverse moves on a board. You can only undo moves on the board
   they were originally made, in the reverse order they were made */
void board_do_move(Bitboard *board, Move move);
void board_undo_move(Bitboard *board, Move move);

// returns 1 if color's king is in check, 0 otherwise
int board_in_check(Bitboard *board, Color color);

// dump a primitive printout of the board to stdout
void board_print(uint64_t boards[2][6]);

// for all of these conversions, 0 <= row,col < 8
#define board_index_of(row, col) ((col) | ((row) << 3))
#define board_row_of(index) ((uint8_t)((index) / 8))
#define board_col_of(index) ((index) % 8)

// undefined return value if there is no piece at index
static inline Piecetype board_piecetype_at_index(Bitboard *board,
		uint8_t index)
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
