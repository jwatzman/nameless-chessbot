#ifndef _TYPES_H
#define _TYPES_H

#include <stdint.h>

typedef enum { WHITE=0, BLACK } Color;
typedef enum { PAWN=0, BISHOP, KNIGHT, ROOK, QUEEN, KING } Piecetype;

static const int INFINITY = 1000000;
static const int MATE = 300000;

typedef struct
{
	uint64_t boards[2][6];

	uint64_t composite_boards[2];
	uint64_t full_composite;
	uint64_t full_composite_45;
	uint64_t full_composite_90;
	uint64_t full_composite_315;

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

	uint8_t halfmove_count;
	Color to_move;

	// undo information is stored in a ring buffer
	// we let the index wrap around 255 <-> 0 since it's an unsigned int
	// from LSB to MSB:
	// old enpassant_index (6 bits)
	// old castling rights [upper 4 bits of castle_status] (4 bits)
	// old halfmove_count [max value 50] (6 bits)
	uint8_t undo_index;
	uint16_t undo_ring_buffer[256];

	uint64_t zobrist;
	uint64_t zobrist_pos[2][6][64];
	uint64_t zobrist_castle[256];
	uint64_t zobrist_enpassant[64];
	uint64_t zobrist_black;

	// previous zobrist hashes of this game
	uint8_t history_index;
	uint64_t history[256];

	// stores attacked bitboards, and their history in the game
	// (the history is managed in the same manner as the undo_ring_bufffer)
	//
	// if, for example, the LSB is set in all_attacks[WHITE] then that means
	// that white could capture a black piece sitting on a1, if there were to
	// be one
	uint64_t all_attacks[2];
	uint8_t all_attacks_index;
	uint64_t all_attacks_ring_buffer[256][2];
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
