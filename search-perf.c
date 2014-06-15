#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "bitboard.h"
#include "move.h"
#include "search.h"
#include "timer.h"

Bitboard *board;

int main(int argc, char** argv)
{
	srandom(0);

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

	timer_init("level 1 1 9999");
	search_force_max_depth(atoi(argv[1]));

	int max_pass = atoi(argv[2]);
	for (int pass = 0; pass < max_pass; pass++)
	{
		printf("-- PASS %d\n", pass + 1);
		Move best = search_find_move(board);
		board_do_move(board, best);

		char move_srcdest[6];
		move_srcdest_form(best, move_srcdest);
		printf("-- MOVE %s\n", move_srcdest);
	}

	free(board);
	return 0;
}
