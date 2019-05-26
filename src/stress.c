#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "bitboard.h"
#include "move.h"
#include "moveiter.h"

static const int max_tests = 1000000;
static const int max_moves = 100;

int main(void)
{
	srandom(time(NULL));

	Bitboard *stress = malloc(sizeof(Bitboard));
	Movelist *moves = malloc(sizeof(Movelist));
	Moveiter *it = malloc(sizeof(Moveiter));
	Undo *undos = malloc(sizeof(Undo)*max_moves);
	board_init(stress);

	printf("initial zobrist %.16"PRIx64"\n", stress->zobrist);

	int cur_move = 0;

	for (int i = 0; i < max_tests; i++)
	{
		move_generate_movelist(stress, moves);
		
		moveiter_init(it, moves, MOVEITER_SORT_NONE, MOVE_NULL);
		int n = random() % moves->num_total;

		Move m = MOVE_NULL;
		for (int i = 0; i < n; i++)
			m = moveiter_next(it);
		board_do_move(stress, m, undos + cur_move);
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

	printf("final zobrist %.16"PRIx64"\n", stress->zobrist);

	free(it);
	free(moves);
	free(stress);

	return 0;
}
