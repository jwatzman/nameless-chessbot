#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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

	Movelist moves;
	char* srcdest_form = malloc(6 * sizeof(char));
	char* input_move = malloc(6 * sizeof(char));

	while (1)
	{
		board_print(test->boards);
		move_generate_movelist(test, &moves);

		printf("To move: %i\tAvailable moves: %i\n", test->to_move, moves.num);

		for (int i = 0; i < moves.num; i++)
		{
			Move move = moves.moves[i];
			move_srcdest_form(move, srcdest_form);
			printf("%s ", srcdest_form);
		}
		printf("\n");

		scanf("%5s", input_move);

		for (int i = 0; i < moves.num; i++)
		{
			Move move = moves.moves[i];
			move_srcdest_form(move, srcdest_form);
			if (!strcmp(input_move, srcdest_form))
			{
				board_do_move(test, move);
				break;
			}
		}
	}

	free(srcdest_form);
	free(input_move);
	free(test);
	return 0;
}
