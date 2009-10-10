#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "search.h"
#include "evaluate.h"
#include "move.h"

typedef enum {TRANSPOSITION_EXACT, TRANSPOSITION_ALPHA, TRANSPOSITION_BETA} TranspositionType;

typedef struct
{
	uint64_t zobrist;
	int depth;
	int value;
	Move best_move;
	TranspositionType type;
}
TranspositionNode;

#define max_transposition_table_size 16777216

// this is kept over the entire run of the program, even over multiple games
// zobrist hashes are assumed to be univerally unique... this is perhaps
// an invalid assumption, but i make it anyways :D
static TranspositionNode transposition_table[max_transposition_table_size];

#define max_depth 8
#define max_quiescent_depth 30
static volatile int current_max_depth; // how deep *this* iteration goes

#define max_search_secs 5
static int timeup;

static void sigalarm_handler(int signum);

static int search_alpha_beta(Bitboard *board, int alpha, int beta, int depth, Move* pv);
static int search_move_comparator(const void *m1, const void *m2);

// returns INFINITY if no immediate cutoff
// may write to alpha/beta if it can better the bound
static int search_transposition_get_value(uint64_t zobrist, int *alpha, int *beta, int depth);

// returns 0 if unknown
static Move search_transposition_get_best_move(uint64_t zobrist);

static void search_transposition_put(uint64_t zobrist, int value, Move best_move, TranspositionType type, int depth);

static void sigalarm_handler(int signum)
{
	(void)signum;
	timeup = 1;
}

Move search_find_move(Bitboard *board)
{
	Move best_move = 0;

	// note that due to the way that this is maintained, sibling nodes *will*
	// destroy each other's values -- ONLY pv[0] is garunteed to be valid when
	// we get all the way back up here (the other slots are used for scratch
	// space, and are actually valid when the recursion layer using them
	// is the top level layer -- it's only siblings that ruin things :))
	Move pv[max_depth + max_quiescent_depth + 1];

	// set up the timer; a sigalarm is sent when the search should prematurely
	// end due to time, which in turn sets timeup = 1
	struct sigaction sigalarm_action;
	sigalarm_action.sa_handler = sigalarm_handler;
	sigemptyset(&sigalarm_action.sa_mask);
	sigalarm_action.sa_flags = 0;
	sigaction(SIGALRM, &sigalarm_action, 0);

	alarm(max_search_secs);

	timeup = 0;

	// for each depth, call the main workhorse, search_alpha_beta
	for (current_max_depth = 1; current_max_depth <= max_depth; current_max_depth++)
	{
		fprintf(stderr, "SEARCHER depth %i", current_max_depth);
		int val = search_alpha_beta(board, -INFINITY, INFINITY, current_max_depth, pv);

		// if we sucessfully completed a depth (i.e. did not early terminate due to time),
		// pull the first move off of the pv so that we can return it later,
		// and then print it. Note that there is a minor race condition here:
		// if we get the sigalarm after the very last check in the search_alpha_beta
		// recursive calls, but before we get here, we can potentially throw away an entire
		// depth's worth of work. This is unforunate but won't really hurt anything
		if (!timeup)
		{
			char buf[6];

			best_move = pv[0];

			move_srcdest_form(best_move, buf);
			fprintf(stderr, " %s (%i)\n", buf, val);

			if (val >= MATE)
			{
				fprintf(stderr, "SEARCHER found mate\n");
				break;
			}
		}
		else
		{
			fprintf(stderr, " timeup\n");
			break;
		}
	}

	// this shouldn't matter, but it can't hurt
	alarm(0);

	return best_move;
}

static int search_alpha_beta(Bitboard *board, int alpha, int beta, int depth, Move* pv)
{
	TranspositionType type = TRANSPOSITION_ALPHA;

	// check if we're quiescent, and if we are set an in_check flag
	// this flag is only used to know what moves to skip for a quiescent
	// search, so don't bother doing the expensive computation if we're
	// not quiescent -- we will do it again if it becomes necessary
	int quiescent = depth <= 0;
	int quiescent_in_check = quiescent ? board_in_check(board, board->to_move) : 0;

	// *pv is used as the best move, but we have none yet
	// if this gets stuck in the transposition table, make
	// sure no one uses some random data
	*pv = 0;

	// 50-move rule
	if (board->halfmove_count == 100)
		return 0;

	// 3 repitition rule
	int reps = 0;
	for (uint8_t i = board->history_index - board->halfmove_count; i != board->history_index; i++)
		if (board->history[i] == board->zobrist)
			reps++;
	if (reps >= 2)
		return 0;

	// if we know an acceptable value in the table, use it;
	// however, only check the table after we get down a couple of ply
	if (depth < current_max_depth - 1)
	{
		int table_val = search_transposition_get_value(board->zobrist, &alpha, &beta, depth);
		if (table_val != INFINITY)
		{
			*pv = search_transposition_get_best_move(board->zobrist);
			return table_val;
		}
	}

	// leaf node
	if (depth == -max_quiescent_depth)
	{
		int eval = evaluate_board(board);
		//search_transposition_put(board->zobrist, eval, 0, TRANSPOSITION_EXACT, depth);
		return eval;
	}

	// quiescent null-move
	// this is necessary for correctness
	if (quiescent)
	{
		int null_move_value = evaluate_board(board);

		if (null_move_value >= beta)
			return beta;
		else if (null_move_value > alpha)
		{
			alpha = null_move_value;
			type = TRANSPOSITION_EXACT;
		}
	}

	// generate the list of pseudolegal moves for this board, and due move ordering via qsort
	// and the order defined by search_move_comparator
	Movelist moves;
	move_generate_movelist(board, &moves);
	qsort(&(moves.moves), moves.num, sizeof(Move), search_move_comparator);

	// since we generate only pseudolegal moves, we need to keep track if there actually are
	// any legal moves at all
	int found_move = 0;

	// since i in the loop below only tracks an index, we want an indication
	// if this is the first move
	int first_move = 1;

	Move transposition_move = quiescent ? 0 : search_transposition_get_best_move(board->zobrist);

	for (int i = 0; (i < moves.num) && !timeup; i++)
	{
		Move move;

		// if we sucessfully got a move out of the transposition table and have not tried it yet,
		// try it first; otherwise continue moving through the main body of moves
		if (first_move && transposition_move)
		{
			move = transposition_move;
			i--;
		}
		else
			move = moves.moves[i];

		// if we're quiescent, we only want capture moves
		// unless the original position was in check, then do everything
		if (quiescent && !quiescent_in_check && !move_is_capture(move))
		{
			first_move = 0;
			continue;
		}

		// only try the transposition move once
		if (!first_move && move == transposition_move)
			continue;

		board_do_move(board, move);

		// make the final legality check
		if (!board_in_check(board, 1-board->to_move))
		{
			int recursive_value;
			found_move = 1;

			// search down the tree from this node -- negascout
			if (first_move)
			{
				// first move, full window search
				recursive_value = -search_alpha_beta(board, -beta, -alpha, depth - 1, pv + 1);
			}
			else
			{
				// subsequent moves, try null window
				recursive_value = -search_alpha_beta(board, -(alpha+1), -alpha, depth - 1, pv + 1);

				// if null window failed, do a full window search
				if (alpha < recursive_value && recursive_value < beta)
				{
					recursive_value = -search_alpha_beta(board, -beta, -alpha, depth - 1, pv + 1);
				}
			}

			board_undo_move(board, move);

			if (recursive_value >= beta)
			{
				// since this move caused a beta cutoff, we don't want to bother storing it in the pv
				// *however*, we most definately want to put it in the transposition table,
				// since it will be searched first next time, and will thus immediately cause a cutoff again
				search_transposition_put(board->zobrist, beta, move, TRANSPOSITION_BETA, depth);
				return beta;
			}

			if (recursive_value > alpha)
			{
				alpha = recursive_value;
				type = TRANSPOSITION_EXACT;
				*pv = move;
			}
		}
		else
			board_undo_move(board, move);

		first_move = 0;
	}

	if (!found_move && !quiescent)
	{
		// there are no legal moves for this game state -- check why
		int val = board_in_check(board, board->to_move) ? -(MATE + depth) : 0;
		//search_transposition_put(board->zobrist, val, 0, TRANSPOSITION_EXACT, depth);
		return val;
	}
	else if (!found_move && quiescent)
	{
		int eval = evaluate_board(board);
		//search_transposition_put(board->zobrist, eval, 0, TRANSPOSITION_EXACT, depth);
		return eval;
	}
	else
	{
		search_transposition_put(board->zobrist, alpha, *pv, type, depth);
		return alpha;
	}
}

static int search_move_comparator(const void *m1, const void *m2)
{
	// sorts in this priority:
	// captures before noncaptures,
	// more valuable captured pieces first,
	// less valuable capturing pieces first

	Move dm1 = *(const Move*)m1;
	Move dm2 = *(const Move*)m2;

	int score1 = move_is_capture(dm1) ?
		15 + 2*move_captured_piecetype(dm1) - move_piecetype(dm1) : 0;
	int score2 = move_is_capture(dm2) ?
		15 + 2*move_captured_piecetype(dm2) - move_piecetype(dm2) : 0;

	return score2 - score1;
}

static int search_transposition_get_value(uint64_t zobrist, int *alpha, int *beta, int depth)
{
	TranspositionNode *node = &transposition_table[zobrist % max_transposition_table_size];

	// since many zobrists may map to a single slot in the table, we want to make sure we got
	// a match; also, we want to make sure that the entry was not made with a shallower
	// depth than what we're currently using
	if (node->zobrist == zobrist)
	{
		if (node->depth >= depth)
		{
			int val = node->value;

			if (node->type == TRANSPOSITION_EXACT)
				return val;

			if (node->type == TRANSPOSITION_ALPHA)
			{
				if (val <= *alpha)
					return *alpha;
				else if (val < *beta)
					*beta = val;
			}

			if (node->type == TRANSPOSITION_BETA)
			{
				if (val >= *beta)
					return *beta;
				else if (val > *alpha)
					*alpha = val;
			}
		}
	}

	return INFINITY;
}

static Move search_transposition_get_best_move(uint64_t zobrist)
{
	Move result = 0;
	TranspositionNode node = transposition_table[zobrist % max_transposition_table_size];

	if (node.zobrist == zobrist)
		result = node.best_move;

	return result;
}

static void search_transposition_put(uint64_t zobrist, int value, Move best_move, TranspositionType type, int depth)
{
	// make sure that only data from completed subtrees makes it in the table;
	// since this is only called right before search_alpha_beta returns,
	// we just want to make sure the main search loop finished for that subtree
	if (timeup)
		return;

	TranspositionNode *node = &transposition_table[zobrist % max_transposition_table_size];
	node->zobrist = zobrist;
	node->depth = depth;
	node->value = value;
	node->best_move = best_move;
	node->type = type;
}
