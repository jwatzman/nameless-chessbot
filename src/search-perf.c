#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "bitboard.h"
#include "move.h"
#include "moveiter.h"
#include "search.h"
#include "timer.h"

Bitboard *board;

int main(int argc, char** argv)
{
	srandom(0);
	move_init();

	if (argc != 3)
	{
		printf("Usage: ./search-perf depth passes\n");
		return 1;
	}

	board = malloc(sizeof(Bitboard));
	board_init_with_fen(
		board,
		"r2q3k/pn2bprp/4pNp1/2p1PbQ1/3p1P2/5NR1/PPP3PP/2B2RK1 b - - 0 1"
	);

	timer_init("level 0 1 9999");
	search_force_max_depth(atoi(argv[1]));

	Undo *u = NULL;
	int max_pass = atoi(argv[2]);
	for (int pass = 0; pass < max_pass; pass++)
	{
		printf("-- PASS %d\n", pass + 1);
		Move best;

		best = search_find_move(board);
		u = malloc(sizeof(Undo));
		board_do_move(board, best, u);

		// Invalidate the transposition table, so that we are perft-testing
		// a more complete search every time. Not doing this is fine for
		// corectness, but means the first N-1 ply are probably already cached.
		board->zobrist = random();

		char move_srcdest[6];
		move_srcdest_form(best, move_srcdest);
		printf("-- MOVE %s\n", move_srcdest);
	}

	free(board);
	board_free_undos(u);
	return 0;
}
