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

		if (pass % 2 == 0)
		{
			best = search_find_move(board);
		}
		else
		{
			Movelist moves;
			move_generate_movelist(board, &moves);

			// Make a vaugely reasonable move.
			Moveiter iter;
			moveiter_init(&iter, &moves, MOVEITER_SORT_ONDEMAND, MOVE_NULL);
			best = moveiter_next(&iter);
		}

		u = malloc(sizeof(Undo));
		board_do_move(board, best, u);

		char move_srcdest[6];
		move_srcdest_form(best, move_srcdest);
		printf("-- MOVE %s\n", move_srcdest);
	}

	free(board);
	board_free_undos(u);
	return 0;
}
