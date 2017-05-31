#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "bitboard.h"
#include "move.h"
#include "moveiter.h"

Bitboard *board;
unsigned long int nodes;
int sort_mode = MOVEITER_SORT_NONE;

void perft(int depth);

int main(int argc, char** argv)
{
	srandom(time(NULL));

	if (argc < 2 || argc > 4)
	{
		printf("Usage: ./perft depth [sortmode [fen]]\n");
		return 1;
	}

	board = malloc(sizeof(Bitboard));
	nodes = 0;
	int max_depth = atoi(argv[1]);

	if (argc >= 3)
		sort_mode = atoi(argv[2]);

	if (argc >=4)
		board_init_with_fen(board, argv[3]);
	else
		board_init(board);

	printf("initial zobrist %.16"PRIx64"\n", board->zobrist);

	perft(max_depth);

	printf("final zobrist %.16"PRIx64"\n%lu nodes\n", board->zobrist, nodes);
	free(board);
	return 0;
}

void perft(int depth)
{
	if (depth == 0)
	{
		nodes++;
		return;
	}

	Movelist moves;
	move_generate_movelist(board, &moves);

	Move best = MOVE_NULL;
	if (sort_mode != MOVEITER_SORT_NONE && moves.num_other > 2)
		best = moves.moves_other[2];

	Moveiter iter;
	moveiter_init(&iter, &moves, sort_mode, best);

	while (moveiter_has_next(&iter))
	{
		Move m = moveiter_next(&iter);
		Undo u;
		board_do_move(board, m, &u);

		if (!board_in_check(board, 1-board->to_move))
			perft(depth - 1);

		board_undo_move(board);
	}
}
