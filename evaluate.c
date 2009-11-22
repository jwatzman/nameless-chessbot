#define _GNU_SOURCE
#include <string.h>
#include "evaluate.h"
#include "evaluate-generated.h"
#include "bitboard.h"
#include "move.h"

#define fancy_popcnt 1

// position bonuses; remember that
// since a1 is the first number, it is in the upper left
static const int rook_pos[] = {
0, 3, 7,  8,  8, 7, 3, 0,
0, 3, 7,  8,  8, 7, 3, 0,
0, 3, 7,  8,  8, 7, 3, 0,
0, 3, 7, 10, 10, 7, 3, 0,
0, 3, 7, 10, 10, 7, 3, 0,
0, 3, 7,  8,  8, 7, 3, 0,
0, 3, 7,  8,  8, 7, 3, 0,
0, 3, 7,  8,  8, 7, 3, 0
};

static const int bishop_pos[] = {
-5, -5, -7, -5, -5, -7, -5, -5,
-5, 10,  5,  8,  8,  5, 10, -5,
-5,  5,  3,  8,  8,  3,  5, -5,
-5,  3, 10,  3,  3, 10,  3, -5,
-5,  3, 10,  3,  3, 10,  3, -5,
-5,  5,  3,  8,  8,  3,  5, -5,
-5, 10,  5,  8,  8,  5, 10, -5,
-5, -5, -7, -5, -5, -7, -5, -5
};

static const int knight_pos[] = {
-10, -7, -5, -5, -5, -5, -7,-10,
 -8,  0,  0,  3,  3,  0,  0, -8,
 -8,  0, 10,  8,  8, 10,  0, -8,
 -8,  0,  8, 10, 10,  8,  0, -8,
 -8,  0,  8, 10, 10,  8,  0, -8,
 -8,  0, 10,  8,  8, 10,  0, -8,
 -8,  0,  0,  3,  3,  0,  0, -8,
-10, -7, -5, -5, -5, -5, -7,-10
};

// the only one that isn't hoizontally symmetric;
// this needs to be special-cased, see below
static const int pawn_pos[] = {
0,  0,  0,  0,  0,  0,  0,  0,
0,  0,  0, -9, -9,  0,  0,  0,
0,  2,  3,  5,  5,  3,  2,  0,
0,  4,  6, 15, 15,  6,  4,  0,
0,  6,  9, 10, 10,  9,  6,  0,
4,  8, 12, 16, 16, 12,  8,  4,
15,15, 20, 25, 25, 20, 15, 15,
0,  0,  0,  0,  0,  0,  0,  0
};

// endgame only
static const int king_endgame_pos[] = {
0,  0,  1,  3,  3,  1,  0,  0,
0,  5,  5,  5,  5,  5,  5,  0,
1,  5,  8,  8,  8,  8,  5,  1,
3,  5,  8, 10, 10,  8,  5,  3,
3,  5,  8, 10, 10,  8,  5,  3,
1,  5,  8,  8,  8,  8,  5,  1,
0,  5,  5,  5,  5,  5,  5,  0,
0,  0,  1,  3,  3,  1,  0,  0
};

// metatables
static const int* pos_tables[] = { pawn_pos, bishop_pos, knight_pos, rook_pos, 0, 0 };

static const int* endgame_pos_tables[] = { pawn_pos, bishop_pos, knight_pos, rook_pos, 0, king_endgame_pos };

// piece intrinsic values, in order of:
// pawn, bishop, knight, rook, queen, king
// (as defined in types.h)
static const int values[] = { 100, 300, 300, 500, 900, 0 };
static const int endgame_values[] = { 250, 300, 300, 500, 1000, 0 };

#define castle_bonus 10
#define passed_pawn_bonus 50
#define doubled_pawn_penalty -10

static int popcnt(uint64_t x);

int evaluate_board(Bitboard *board)
{
	int result = 0;
	int endgame = popcnt(board->full_composite) < 6;
	Color to_move = board->to_move;

	for (Color color = 0; color < 2; color++)
	{
		// add values for the current player, and subtract them for the opponent
		int modifier = (color == to_move ? 1 : -1);

		// 5 == 00000101
		// iff white has castled, exactly one of those bits will be set
		// iff black has castled, exactly one of the bits to the left of one of those bits will be set
		// (see description of castle_status in types.h)
		if (board->castle_status & (5 << color))
			result += modifier * castle_bonus;

		// doubled pawns
		// 0x0101010101010101 masks a single column
		for (int col = 0; col < 8; col++)
			result += modifier * doubled_pawn_penalty *
				(popcnt(board->boards[color][PAWN] & (0x0101010101010101 << col)) - 1);

		for (Piecetype piece = 0; piece < 6; piece++)
		{
			uint64_t pieces = board->boards[color][piece];
			while (pieces)
			{
				uint8_t loc = ffsll(pieces) - 1;
				pieces ^= 1ULL << loc;

				// pawns have some special stuff done to them
				if (piece == PAWN)
				{
					// passed pawn; do this before the location swap below
					if ((front_spans[color][loc] & board->boards[1-color][PAWN]) == 0)
						result += modifier * passed_pawn_bonus;

					// since the pawn table is not hoizontally symmetric,
					// we need to flip it for black
					if (color == BLACK)
						loc = 63 - loc;
				}

				// add in piece intrinsic value, and bonus for its location
				const int *table = endgame ? endgame_pos_tables[piece] : pos_tables[piece];
				if (table)
					result += modifier * table[loc];
				result += modifier * (endgame ? endgame_values[piece] : values[piece]);

				// add in a bonus for every square that this piece can attack
				// only do this for bishops and rooks; knights are sufficiently taken
				// care of by positional bonus, and this will emphasize queens too much
				if (piece == BISHOP || piece == ROOK)
				{
					uint64_t attacks = move_generate_attacks(board, piece, color, loc);
					result += modifier * popcnt(attacks);
				}
			}
		}
	}

	return result;
}

static int popcnt(uint64_t x)
{
#if fancy_popcnt
	uint64_t r;
	__asm__ __volatile__ ("popcnt %1, %0;" : "=r"(r) : "r"(x));
	return r;
#else
	x -= (x >> 1) & 0x5555555555555555;
	x = (x & 0x3333333333333333) + ((x >> 2) & 0x3333333333333333);
	x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0f;
	return (x * 0x0101010101010101)>>56;
#endif
}
