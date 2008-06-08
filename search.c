#include "search.h"
#include "evaluate.h"
#include "move.h"

static int search_alpha_beta(Bitboard *board, int alpha, int beta, int depth, Move* best_move);

Move search_find_move(Bitboard *board)
{
	Move best_move;
	search_alpha_beta(board, -INFINITY, INFINITY, 5, &best_move);
	return best_move;
}

static int search_alpha_beta(Bitboard *board, int alpha, int beta, int depth, Move* best_move)
{
	if (depth == 0)
		return evaluate_board(board);

	Movelist moves;
	move_generate_movelist(board, &moves);
	int found_move = 0;

	for (int done_captures = 0; done_captures <= 1; done_captures++)
	{
		for (int i = 0; i < moves.num; i++)
		{
			Move move = moves.moves[i];

			int is_capture = move_is_capture(move);
			if ((!done_captures && !is_capture) || (done_captures && is_capture))
				continue;

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
	}

	if (!found_move)
		return board_in_check(board, board->to_move) ? -MATE : 0;
	else
		return alpha;
}
