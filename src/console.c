#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "bitboard.h"
#include "move.h"
#include "moveiter.h"
#include "evaluate.h"
#include "search.h"

static Move get_human_move(Bitboard *board, Movelist *orig_moves);
static Move get_computer_move(Bitboard *board);

int main(void)
{
	srandom(time(NULL));
	move_init();

	Bitboard *test = malloc(sizeof(Bitboard));
	board_init(test);
	Undo *u = NULL;
	
	while (1)
	{
		board_print(test);
		printf("Evaluation: %i\n", evaluate_board(test));

		Movelist moves;
		move_generate_movelist(test, &moves);

		Moveiter it;
		moveiter_init(&it, &moves, MOVEITER_SORT_NONE, MOVE_NULL);

		int num_legal_moves = 0;
		while (moveiter_has_next(&it))
		{
			Move move = moveiter_next(&it);
			Undo u_tmp;
			board_do_move(test, move, &u_tmp);
			if (!board_in_check(test, 1-test->to_move))
				num_legal_moves++;
			board_undo_move(test);
		}

		if (num_legal_moves == 0)
		{
			if (board_in_check(test, test->to_move))
				printf("CHECKMATE!\n");
			else
				printf("STALEMATE!\n");

			break;
		}

		if (test->halfmove_count == 50)
		{
			printf("DRAW BY 50 MOVE RULE\n");
			break;
		}

		Move next_move;
		if (test->to_move == WHITE)
		{
			next_move = get_human_move(test, &moves);
			if (next_move == MOVE_NULL)
				break;
		}
		else
		{
			next_move = get_computer_move(test);
		}

		u = malloc(sizeof(Undo));
		board_do_move(test, next_move, u);
	}

	free(test);
	board_free_undos(u);
	return 0;
}

static Move get_human_move(Bitboard *board, Movelist *orig_moves)
{
	char* srcdest_form = malloc(6 * sizeof(char));
	char* input_move = malloc(6 * sizeof(char));

	Move result = 0;

	Undo u;
	Movelist moves;
	Moveiter it;

	memcpy(&moves, orig_moves, sizeof(Movelist));
	moveiter_init(&it, &moves, MOVEITER_SORT_NONE, MOVE_NULL);

	while (moveiter_has_next(&it))
	{
		Move move = moveiter_next(&it);
		board_do_move(board, move, &u);
		if (!board_in_check(board, 1-board->to_move))
		{
			move_srcdest_form(move, srcdest_form);
			printf("%s ", srcdest_form);
		}
		board_undo_move(board);
	}
	printf("\n");

	while (!result)
	{
		if (scanf("%5s", input_move) == EOF)
			return MOVE_NULL;

		memcpy(&moves, orig_moves, sizeof(Movelist));
		moveiter_init(&it, &moves, MOVEITER_SORT_NONE, MOVE_NULL);

		while (moveiter_has_next(&it))
		{
			Move move = moveiter_next(&it);
			move_srcdest_form(move, srcdest_form);
			if (!strcmp(input_move, srcdest_form))
			{
				board_do_move(board, move, &u);
				if (board_in_check(board, 1-board->to_move))
					printf("Can't leave king in check\n");
				else
					result = move;

				board_undo_move(board);
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
	return search_find_move(board, NULL);
}
