#include <stdlib.h>
#include "bitboard.h"
#include "move.h"

static const int max_tests = 1000000;

int main(int argc, char** argv)
{
	Bitboard *stress = malloc(sizeof(Bitboard));
	board_init(stress);

	for (int i = 0; i < max_tests; i++)
	{
		Movelist moves;
		move_generate_movelist(stress, &moves);
		board_do_move(stress, moves.moves[0]);
		board_undo_move(stress, moves.moves[0]);
	}

	free(stress);
	return 0;
}
