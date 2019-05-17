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
	printf("Usage: ./perft [-f fen] -d depth\n");
	exit(1);
}

int main(int argc, char** argv)
{
	int max_depth = -1;
	char *fen = NULL;
	int opt;
	while ((opt = getopt(argc, argv, ":d:f:")) != -1)
	{
		switch (opt)
		{
			case 'd':
				max_depth = atoi(optarg);
				break;
			case 'f':
				fen = optarg;
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
