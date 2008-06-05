#include <stdlib.h>
#include <time.h>
#include "bitboard.h"
#include "move.h"

static const int max_tests = 1000000;
static const int max_moves = 10;

int main(int argc, char** argv)
{
	srandom(time(NULL));

	Bitboard *stress = malloc(sizeof(Bitboard));
	Movelist *moves = malloc(sizeof(Movelist));
	board_init(stress);

	Move move_buffer[max_moves];
	int cur_move = 0;

	for (int i = 0; i < max_tests; i++)
	{
		move_generate_movelist(stress, moves);
		
		Move m = moves->moves[random() % moves->num];
		move_buffer[cur_move++] = m;

		board_do_move(stress, m);

		if (cur_move == max_moves)
			while (cur_move)
				board_undo_move(stress, move_buffer[--cur_move]);
	}

	free(stress);
	return 0;
}
