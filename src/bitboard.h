#ifndef _BITBOARD_H
#define _BITBOARD_H

#include <stdint.h>
#include <stdlib.h>
#include "types.h"

// writes initial state to a board
void board_init(Bitboard *board);

// writes board state from the fen to the board. Assumes a valid fen
void board_init_with_fen(Bitboard *board, const char *fen);

// make and reverse moves on a board
void board_do_move(Bitboard *board, Move move, Undo *undo);
void board_undo_move(Bitboard *board);

// last move to be applied to this board
Move board_last_move(Bitboard *board);

// returns 1 if color's king is in check, 0 otherwise
int board_in_check(Bitboard *board, Color color);

// dump a primitive printout of the board to stdout
void board_print(Bitboard *board);

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

static inline void board_free_undos(Undo *u)
{
	while (u)
	{
		Undo *prev = u->prev;
		free(u);
		u = prev;
	}
}

#endif
