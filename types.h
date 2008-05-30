#ifndef _TYPES_H
#define _TYPES_H

typedef enum { WHITE=0, BLACK } Color;
typedef enum { PAWN=0, BISHOP, KNIGHT, ROOK, QUEEN, KING } Piecetype;

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

// moves are stored in 30 bits
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
// undo data for board [castling rights] (4)
// which leaves the two MSB unused
typedef uint32_t Move;

typedef struct
{
	// according to Wikipedia, any given board has at most about 200 moves
	Move moves[256];
	uint8_t num;
}
Movelist;

#endif
