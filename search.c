#include <stdlib.h>
#include "search.h"
#include "evaluate.h"
#include "move.h"
#include "movelist.h"

typedef enum {TRANSPOSITION_EXACT, TRANSPOSITION_ALPHA, TRANSPOSITION_BETA} TranspositionType;

typedef struct
{
	uint64_t zobrist;
	int depth;
	int value;
	TranspositionType type;
}
TranspositionNode;

static const int max_transposition_table_size = 16384;

// this is kept over the entire run of the program, even over multiple games
// zobrist hashes are assumed to be univerally unique... this is perhaps
// an invalid assumption, but i make it anyways :D
static TranspositionNode transposition_table[16384];

static int search_alpha_beta(Bitboard *board, int alpha, int beta, int depth, Move* best_move);
static int search_move_comparator(const void *m1, const void *m2);

// returns INFINITY if unknown
static int search_transposition_get_value(uint64_t zobrist, int alpha, int beta, int depth);
static void search_transposition_put(uint64_t zobrist, int value, TranspositionType type, int depth);

Move search_find_move(Bitboard *board)
{
	Move best_move;
	search_alpha_beta(board, -INFINITY, INFINITY, 6, &best_move);
	return best_move;
}

static int search_alpha_beta(Bitboard *board, int alpha, int beta, int depth, Move* best_move)
{
	TranspositionType type = TRANSPOSITION_ALPHA;

	int table_val = search_transposition_get_value(board->zobrist, alpha, beta, depth);
	if (table_val != INFINITY)
		return table_val;

	if (depth == 0)
	{
		int eval = evaluate_board(board);
		search_transposition_put(board->zobrist, eval, TRANSPOSITION_EXACT, depth);
		return eval;
	}

	if (board->halfmove_count == 100)
		return 0;

	int reps = 0;
	for (int i = board->history_index - board->halfmove_count; i < board->history_index; i++)
		if (board->history[i] == board->zobrist)
			reps++;
	if (reps >= 3)
		return 0;

	Movelist *moves = movelist_create();
	move_generate_movelist(board, moves);
	int found_move = 0;

	Move move;
	while ((move = movelist_next_move(moves)))
	{
		board_do_move(board, move);

		if (!board_in_check(board, 1-board->to_move))
		{
			found_move = 1;
			int recursive_value = -search_alpha_beta(board, -beta, -alpha, depth - 1, 0);
			board_undo_move(board, move);

			if (recursive_value >= beta)
			{
				search_transposition_put(board->zobrist, beta, TRANSPOSITION_BETA, depth);
				movelist_destroy(moves);
				return beta;
			}

			if (recursive_value > alpha)
			{
				alpha = recursive_value;
				type = TRANSPOSITION_EXACT;
				if (best_move)
					*best_move = move;
			}
		}
		else
			board_undo_move(board, move);
	}

	movelist_destroy(moves);

	if (!found_move)
	{
		int val = board_in_check(board, board->to_move) ? -(MATE + depth) : 0;
		search_transposition_put(board->zobrist, val, TRANSPOSITION_EXACT, depth);
		return val;
	}
	else
	{
		search_transposition_put(board->zobrist, alpha, type, depth);
		return alpha;
	}
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

static int search_transposition_get_value(uint64_t zobrist, int alpha, int beta, int depth)
{
	TranspositionNode *node = &transposition_table[zobrist % max_transposition_table_size];
	if (node->zobrist == zobrist)
	{
		if (node->depth >= depth)
		{
			if (node->type == TRANSPOSITION_EXACT)
				return node->value;

			if (node->type == TRANSPOSITION_ALPHA && node->value <= alpha)
				return alpha;

			if (node->type == TRANSPOSITION_BETA && node->value >= beta)
				return beta;
		}
	}

	return INFINITY;
}

static void search_transposition_put(uint64_t zobrist, int value, TranspositionType type, int depth)
{
	TranspositionNode *node = &transposition_table[zobrist % max_transposition_table_size];
	node->zobrist = zobrist;
	node->depth = depth;
	node->value = value;
	node-> type = type;
}
