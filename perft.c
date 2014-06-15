#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "bitboard.h"
#include "move.h"
#include "moveiter.h"

Bitboard *board;
unsigned long int nodes;
int sort_mode = 0;

void perft(int depth);

int main(int argc, char** argv)
{
	srandom(time(NULL));

	if (argc < 2 || argc > 3)
	{
		printf("Usage: ./perft depth [sortmode]\n");
		return 1;
	}

	board = malloc(sizeof(Bitboard));
	nodes = 0;
	int max_depth = atoi(argv[1]);

	if (argc == 3)
		sort_mode = atoi(argv[2]);
	
	board_init(board);
	printf("initial zobrist %.16llx\n", board->zobrist);

	perft(max_depth);

	printf("final zobrist %.16llx\n%lu nodes\n", board->zobrist, nodes);
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

	Moveiter iter;
	moveiter_init(&iter, &moves, sort_mode, MOVE_NULL);

	while (moveiter_has_next(&iter))
	{
		Move m = moveiter_next(&iter);
		board_do_move(board, m);

		if (!board_in_check(board, 1-board->to_move))
			perft(depth - 1);

		board_undo_move(board);
	}
}
