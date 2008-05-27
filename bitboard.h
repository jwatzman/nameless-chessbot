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
	// other data will probably have to go here
}
Bitboard;

Bitboard board_init();
uint64_t board_rotate_90(uint64_t board);
uint64_t board_rotate_45(uint64_t board);
uint64_t board_rotate_315(uint64_t board);

void board_print(Bitboard board);

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
