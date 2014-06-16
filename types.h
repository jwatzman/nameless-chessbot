#ifndef _TYPES_H
#define _TYPES_H

#include <stdint.h>

typedef unsigned char Color;
#define WHITE 0
#define BLACK 1

typedef unsigned char Piecetype;
#define PAWN 0
#define BISHOP 1
#define KNIGHT 2
#define ROOK 3
#define QUEEN 4
#define KING 5

#define INFINITY 1000000
#define MATE 300000

typedef struct
{
	uint64_t boards[2][6];

	uint64_t composite_boards[2];
	uint64_t full_composite;
	uint64_t full_composite_45;
	uint64_t full_composite_90;
	uint64_t full_composite_315;

	// from LSB to MSB:
	// white castled KS,
	// black castled KS,
	// white castled QS,
	// black castled QS,
	// white can castle KS, [...]
	uint8_t castle_status;

	// destination square of a pawn moving up two squares
	uint8_t enpassant_index;

	uint8_t halfmove_count;
	Color to_move;

	// cache for board_in_check, do not access directly
	int8_t in_check[2];

	// undo information is stored in a ring buffer
	// we let the index wrap around 255 <-> 0 since it's an unsigned int
	// from LSB to MSB:
	// old enpassant_index (6 bits)
	// old castling rights [upper 4 bits of castle_status] (4 bits)
	// old halfmove_count [max value 50] (6 bits)
	// next 16 bits unused
	// move to undo (32 bits)
	uint8_t undo_index;
	uint64_t undo_ring_buffer[256];

	uint64_t zobrist;
	uint64_t zobrist_pos[2][6][64];
	uint64_t zobrist_castle[256];
	uint64_t zobrist_enpassant[64];
	uint64_t zobrist_black;

	// previous zobrist hashes of this game
	uint8_t history_index;
	uint64_t history[256];

	uint16_t generation;
}
Bitboard;

// moves are stored in 24 bits
// from LSB to MSB:
// source square (6)
// destination square (6)
// type (3)
// color (1)
// is castle (1)
// is en passant (1)
// is capture (1)
// is promotion (1)
// captured type (3)
// promoted type (3)
// which leaves the six MSB unused
typedef uint32_t Move;

typedef struct
{
	// according to Wikipedia, any given board has at most about 200 moves
	Move moves[256];
	uint8_t num;
}
Movelist;

#endif
