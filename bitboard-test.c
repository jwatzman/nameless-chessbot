#include <stdlib.h>
#include <stdio.h>
#include "bitboard.h"
#include "move.h"

int main(int argc, char** argv)
{
	Bitboard *test = malloc(sizeof(Bitboard));
	board_init(test);
	
	/*
	Move test_move = 0;
	test_move |= board_index_of(1, 4);
	test_move |= board_index_of(3, 4) << 6;
	test_move |= PAWN << 12;
	test_move |= WHITE << 15;

	char* srcdest_form = malloc(5 * sizeof(char));
	move_srcdest_form(test_move, srcdest_form);
	printf("%s\n", srcdest_form);
	free(srcdest_form);

	board_do_move(test, test_move);
	*/

	board_print(test->boards);

	Movelist moves;
	move_generate_movelist(test, WHITE, &moves);
	char* srcdest_form = malloc(5 * sizeof(char));

	for (int i = 0; i < moves.num; i++)
	{
		Move move = moves.moves[i];
		if (move_verify(test, move))
		{
			move_srcdest_form(moves.moves[i], srcdest_form);
			printf("%s ", srcdest_form);
		}
	}
	printf("\n");

	free(srcdest_form);
	free(test);
	return 0;
}
