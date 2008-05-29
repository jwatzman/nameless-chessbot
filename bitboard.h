#ifndef _BITBOARD_H
#define _BITBOARD_H

#include <stdint.h>

enum Color { WHITE=0, BLACK };
enum Piecetype { PAWN=0, BISHOP, KNIGHT, ROOK, QUEEN, KING };

typedef struct
{
	uint64_t boards[2][6];
	uint64_t boards45[2][6];
	uint64_t boards90[2][6];
	uint64_t boards315[2][6];

	uint64_t composite_boards[2];

	// from LSB to MSB:
	// white castled KS, black castled KS, white castled QS, black castled QS,
	// white can castle KS, [...]
	uint8_t castle_status;

	// board index, corresponding to a sqaure in 2nd or 5th rows
	// which is the square a pawn who just moved up two can be
	// en passant pseudo-captured in
	// zero if no en passant ability
	// invalid if not zero or in 2nd or 5th row
	uint8_t enpassant_index;

	// other data will probably have to go here
}
Bitboard;

void board_init(Bitboard *board);
uint64_t board_rotate_90(uint64_t board);
uint64_t board_rotate_45(uint64_t board);
uint64_t board_rotate_315(uint64_t board);

// this modifies move so that undo works properly!
void board_do_move(Bitboard *board, Move *move);

// requires the original move passed to board_do_move
void board_undo_move(Bitboard *board, Move *move);

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
