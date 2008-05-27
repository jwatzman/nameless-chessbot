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

	uint8_t has_castled; // bit 0 set iff white castled, bit 1 iff black
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
