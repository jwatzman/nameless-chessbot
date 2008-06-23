#include <stdlib.h>
#include <stdio.h>
#include "bitboard.h"
#include "move.h"
#include "movelist.h"

Bitboard *board;
unsigned long int nodes;

void perft(int depth);

int main(int argc, char** argv)
{
	board = malloc(sizeof(Bitboard));
	nodes = 0;
	int max_depth = atoi(argv[1]);
	
	board_init(board);

	perft(max_depth);

	printf("zobrist %.16llx\n%lu nodes\n", board->zobrist, nodes);
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

	Movelist *moves = movelist_create();
	move_generate_movelist(board, moves);

	Move m;
	while ((m = movelist_next_move(moves)))
	{
		board_do_move(board, m);

		if (!board_in_check(board, 1-board->to_move))
			perft(depth - 1);

		board_undo_move(board, m);
	}
}
