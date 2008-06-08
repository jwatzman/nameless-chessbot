#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bitboard.h"
#include "move.h"
#include "evaluate.h"
#include "search.h"

static Move get_human_move(Bitboard *board, Movelist legal_moves);
static Move get_computer_move(Bitboard *board);

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

	while (1)
	{
		board_print(test->boards);

		printf("Enpassant index: %x\tHalfmove count: %x\tCastle status: %x\n", test->enpassant_index, test->halfmove_count, test->castle_status);
		printf("Zobrist: %.16llx\n", test->zobrist);
		printf("Evaluation: %i\n", evaluate_board(test));
		printf("To move: %i\n", test->to_move);

		Movelist all_moves, legal_moves;
		legal_moves.num = 0;
		move_generate_movelist(test, &all_moves);
		for (int i = 0; i < all_moves.num; i++)
		{
			Move move = all_moves.moves[i];
			board_do_move(test, move);
			if (!board_in_check(test, 1-test->to_move))
				legal_moves.moves[legal_moves.num++] = move;
			board_undo_move(test, move);
		}

		if (legal_moves.num == 0)
		{
			if (board_in_check(test, test->to_move))
				printf("CHECKMATE!\n");
			else
				printf("STALEMATE!\n");

			break;
		}

		if (board->halfmove_count == 50)
		{
			printf("DRAW BY 50 MOVE RULE"\n);
			break;
		}

		Move next_move;
		if (test->to_move == WHITE)
			next_move = get_human_move(test, legal_moves);
		else
			next_move = get_computer_move(test);

		board_do_move(test, next_move);
	}

	free(test);
	return 0;
}

static Move get_human_move(Bitboard *board, Movelist legal_moves)
{
	char* srcdest_form = malloc(6 * sizeof(char));
	char* input_move = malloc(6 * sizeof(char));

	Move result = 0;

	for (int i = 0; i < legal_moves.num; i++)
	{
		Move move = legal_moves.moves[i];
		move_srcdest_form(move, srcdest_form);
		printf("%s ", srcdest_form);
	}
	printf("\n");

	while (!result)
	{
		scanf("%5s", input_move);

		for (int i = 0; i < legal_moves.num; i++)
		{
			Move move = legal_moves.moves[i];
			move_srcdest_form(move, srcdest_form);
			if (!strcmp(input_move, srcdest_form))
			{
				board_do_move(board, move);
				if (board_in_check(board, 1-board->to_move))
					printf("Can't leave king in check\n");
				else
					result = move;

				board_undo_move(board, move);
				break;
			}
		}
	}

	free(srcdest_form);
	free(input_move);
	return result;
}

static Move get_computer_move(Bitboard *board)
{
	return search_find_move(board);
}
