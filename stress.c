#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "bitboard.h"
#include "move.h"

static const int max_tests = 1000000;
static const int max_moves = 100;

int main(void)
{
	srandom(time(NULL));

	Bitboard *stress = malloc(sizeof(Bitboard));
	Movelist *moves = malloc(sizeof(Movelist));
	board_init(stress);

	printf("initial zobrist %.16llx\n", stress->zobrist);

	int cur_move = 0;

	for (int i = 0; i < max_tests; i++)
	{
		move_generate_movelist(stress, moves);
		
		Move m = moves->moves[random() % moves->num];
		board_do_move(stress, m);
		cur_move++;

		if (board_in_check(stress, 1-stress->to_move))
		{
			board_undo_move(stress);
			cur_move--;
		}

		if (cur_move == max_moves)
		{
			while (cur_move)
			{
				board_undo_move(stress);
				cur_move--;
			}
		}
	}

	while (cur_move)
	{
		board_undo_move(stress);
		cur_move--;
	}

	printf("final zobrist %.16llx\n", stress->zobrist);

	free(moves);
	free(stress);

	return 0;
}
