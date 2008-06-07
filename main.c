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
	Move last_move = 0;
	char* srcdest_form = malloc(6 * sizeof(char));
	char* input_move = malloc(6 * sizeof(char));

	while (1)
	{
		board_print(test->boards);
		move_generate_movelist(test, &moves);

		printf("Enpassant index: %x\tHalfmove count: %x\tCastle status: %x\n", test->enpassant_index, test->halfmove_count, test->castle_status);
		printf("Zobrist: %.16llx\n", test->zobrist);
		printf("To move: %i\tAvailable moves: %i\n", test->to_move, moves.num);

		for (int i = 0; i < moves.num; i++)
		{
			Move move = moves.moves[i];
			move_srcdest_form(move, srcdest_form);
			printf("%s ", srcdest_form);
		}
		printf("\n");

		scanf("%5s", input_move);
		if (!strcmp(input_move, "undo"))
			board_undo_move(test, last_move);

		for (int i = 0; i < moves.num; i++)
		{
			Move move = moves.moves[i];
			move_srcdest_form(move, srcdest_form);
			if (!strcmp(input_move, srcdest_form))
			{
				last_move = move;
				board_do_move(test, last_move);
				if (board_in_check(test, 1-test->to_move))
				{
					printf("Can't leave king in check\n");
					board_undo_move(test, last_move);
				}
				break;
			}
		}
	}

	free(srcdest_form);
	free(input_move);
	free(test);
	return 0;
}
