#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "bitboard.h"
#include "move.h"
#include "moveiter.h"

uint64_t perft(Bitboard *board, int depth);

void usage(void)
{
	printf("Usage: ./perft [-f fen] [-t] -d depth\n");
	exit(1);
}

typedef struct {
	const char *fen;
	int depth;
	uint64_t nodes;
} TestCase;

int run_tests(void)
{
	const TestCase cases[] =
	{
		{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 6, 119060324},

		// https://www.chessprogramming.org/Perft_Results
		{"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 5, 193690690},
		{"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 6, 11030083},
		{"r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", 5, 15833292},
		{"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 4, 2103487},

		{NULL, 0, 0},
	};

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

int main(int argc, char** argv)
{
	int max_depth = -1;
	char *fen = NULL;
	int test_mode = 0;
	int opt;
	while ((opt = getopt(argc, argv, ":d:f:t")) != -1)
	{
		switch (opt)
		{
			case 'd':
				max_depth = atoi(optarg);
				break;
			case 'f':
				fen = optarg;
				break;
			case 't':
				test_mode = 1;
				break;
			case '?':
				printf("Unknown option: %c\n", optopt);
				usage();
				break;
			case ':':
				printf("Missing value: %c\n", optopt);
				usage();
				break;
			default:
				usage();
				break;
		}
	}

	if (test_mode)
	{
		return run_tests();
	}

	if (max_depth < 1)
	{
		printf("Invalid/missing depth\n");
		usage();
	}

	srandom(time(NULL));
	Bitboard *board = malloc(sizeof(Bitboard));

	if (fen)
		board_init_with_fen(board, fen);
	else
		board_init(board);

	printf("initial zobrist %.16"PRIx64"\n", board->zobrist);

	uint64_t nodes = perft(board, max_depth);

	printf("final zobrist %.16"PRIx64"\n%"PRIu64" nodes\n", board->zobrist, nodes);
	free(board);
	return 0;
}

uint64_t perft(Bitboard *board, int depth)
{
	if (depth == 0)
	{
		return 1;
	}

	Movelist moves;
	move_generate_movelist(board, &moves);

	Moveiter iter;
	moveiter_init(&iter, &moves, MOVEITER_SORT_NONE, MOVE_NULL);

	uint64_t nodes = 0;
	while (moveiter_has_next(&iter))
	{
		Move m = moveiter_next(&iter);
		Undo u;
		board_do_move(board, m, &u);

		if (!board_in_check(board, 1-board->to_move))
			nodes += perft(board, depth - 1);

		board_undo_move(board);
	}

	return nodes;
}
