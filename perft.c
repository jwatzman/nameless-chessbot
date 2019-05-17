#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "bitboard.h"
#include "move.h"
#include "moveiter.h"

uint64_t perft(Bitboard *board, int depth);

int main(int argc, char** argv)
{
	srandom(time(NULL));

	if (argc != 2)
	{
		printf("Usage: ./perft depth\n");
		return 1;
	}

	Bitboard *board = malloc(sizeof(Bitboard));
	int max_depth = atoi(argv[1]);

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
