#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "bitboard.h"
#include "move.h"
#include "perftfn.h"

typedef struct {
	const char *fen;
	int depth;
	uint64_t nodes;
} TestCase;

static const TestCase cases[] =
{
	{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 6, 119060324},

	// https://www.chessprogramming.org/Perft_Results
	{"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 5, 193690690},
	{"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 6, 11030083},
	{"r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", 5, 15833292},
	{"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 4, 2103487},

	{NULL, 0, 0},
};

int main(void)
{
	move_init();
	srandom(0);

	Bitboard board;
	for (const TestCase *tcase = cases; tcase->fen != NULL; tcase++)
	{
		printf("CASE: \"%s\" depth %d tot %"PRIu64"\n", tcase->fen, tcase->depth, tcase->nodes);
		board_init_with_fen(&board, tcase->fen);

		uint64_t zobrist = board.zobrist;
		uint64_t nodes = perft(&board, tcase->depth);

		if (nodes != tcase->nodes)
		{
			printf("FAIL: nodes: expected %"PRIu64" got %"PRIu64"\n", tcase->nodes, nodes);
			return 1;
		}

		if (zobrist != board.zobrist)
		{
			printf("FAIL: zobrist: expected %"PRIx64" got %"PRIx64"\n", zobrist, board.zobrist);
			return 1;
		}

		printf("PASS\n");
	}

	return 0;
}
