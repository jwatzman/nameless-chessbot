#define _GNU_SOURCE
#include <string.h>
#include "evaluate.h"
#include "bitboard.h"

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

static const int pawn_pos[] = {
0,  0,  0,  0,  0,  0,  0,  0,
0,  0,  0, -9, -9,  0,  0,  0,
0,  2,  3,  5,  5,  3,  2,  0,
0,  4,  6, 15, 15,  6,  4,  0,
0,  6,  9, 10, 10,  9,  6,  0,
4,  8, 12, 16, 16, 12,  8,  4,
10,15, 20, 25, 25, 20, 15, 10,
0,  0,  0,  0,  0,  0,  0,  0
};

// pawn, bishop, knight, rook, queen, king
static const int* pos_tables[] = { pawn_pos, bishop_pos, knight_pos, rook_pos, 0, 0 };
static const int values[] = { 100, 300, 300, 500, 900, 0 };
static const int castle_bonus = 10;

int evaluate_board(Bitboard *board)
{
	int result = 0;
	Color to_move = board->to_move;

	for (Color color = 0; color < 2; color++)
	{
		int modifier = (color == to_move ? 1 : -1);

		if (board->castle_status & (5 << color))
			result += modifier * castle_bonus;

		for (Piecetype piece = 0; piece < 6; piece++)
		{
			uint64_t pieces = board->boards[color][piece];
			while (pieces)
			{
				uint8_t loc = ffsll(pieces) - 1;
				pieces ^= 1ULL << loc;

				if (piece == PAWN) loc = 63 - loc;

				const int *table = pos_tables[piece];
				if (table)
					result += table[loc];
				result += modifier * values[piece];
			}
		}
	}

	return result;
}
