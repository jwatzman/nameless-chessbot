#include <stdlib.h>
#include "search.h"
#include "evaluate.h"
#include "move.h"

static int search_alpha_beta(Bitboard *board, int alpha, int beta, int depth, Move* best_move);
static int search_move_comparator(const void *m1, const void *m2);

Move search_find_move(Bitboard *board)
{
	Move best_move;
	search_alpha_beta(board, -INFINITY, INFINITY, 6, &best_move);
	return best_move;
}

static int search_alpha_beta(Bitboard *board, int alpha, int beta, int depth, Move* best_move)
{
	if (depth == 0)
		return evaluate_board(board);

	if (board->halfmove_count == 100)
		return 0;

	int reps = 0;
	for (int i = board->history_index - board->halfmove_count; i < board->history_index; i++)
		if (board->history[i] == board->zobrist)
			reps++;
	if (reps >= 3)
		return 0;

	Movelist moves;
	move_generate_movelist(board, &moves);
	qsort(&(moves.moves), moves.num, sizeof(Move), search_move_comparator);
	int found_move = 0;

	for (int i = 0; i < moves.num; i++)
	{
		Move move = moves.moves[i];

		board_do_move(board, move);

		if (!board_in_check(board, 1-board->to_move))
		{
			found_move = 1;
			int recursive_value = -search_alpha_beta(board, -beta, -alpha, depth - 1, 0);
			board_undo_move(board, move);

			if (recursive_value >= beta)
				return beta;

			if (recursive_value > alpha)
			{
				alpha = recursive_value;
				if (best_move)
					*best_move = move;
			}
		}
		else
			board_undo_move(board, move);
	}

	if (!found_move)
		return board_in_check(board, board->to_move) ? -(MATE + depth) : 0;
	else
		return alpha;
}

static int search_move_comparator(const void *m1, const void *m2)
{
	Move dm1 = *(Move*)m1;
	Move dm2 = *(Move*)m2;

	int score1 = move_is_capture(dm1) ?
		15 + 2*move_captured_piecetype(dm1) - move_piecetype(dm1) : 0;
	int score2 = move_is_capture(dm2) ?
		15 + 2*move_captured_piecetype(dm2) - move_piecetype(dm2) : 0;

	return score2 - score1;
}
