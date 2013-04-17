#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "search.h"
#include "evaluate.h"
#include "move.h"
#include "timer.h"

typedef enum
{
	TRANSPOSITION_EXACT,
	TRANSPOSITION_ALPHA,
	TRANSPOSITION_BETA,
	TRANSPOSITION_INVALIDATED
}
TranspositionType;

typedef struct
{
	uint64_t zobrist;
	int depth;
	int generation;
	int value;
	Move best_move;
	TranspositionType type;
}
TranspositionNode;

/* the transposition table. It's kept over the lifetime of the program, but the
   values (not moves) in it are invalidated each new search */
#define max_transposition_table_size 16777216
static TranspositionNode transposition_table[max_transposition_table_size];

static int generation = 0;

#define max_depth 15
#define max_quiescent_depth 50
#define aspiration_window 30

static volatile int timeup;

// main search workhorse
static int search_alpha_beta(Bitboard *board,
	int alpha, int beta, int depth, int ply, Move *pv);

// used to sort the moves for move ordering
static int search_move_comparator(void *tm, const void *m1, const void *m2);

/* asks the transposition table if we already know a good value for this
   position. If we do, return it. Otherwise, return INFINITY but adjust
   *alpha and *beta if we know better bounds for them */
static int search_transposition_get_value(uint64_t zobrist,
		int alpha, int beta, int depth);

// if we have a previous best move for this zobrist, return it; 0 otherwise
static Move search_transposition_get_best_move(uint64_t zobrist);

// add to transposition table
static void search_transposition_put(uint64_t zobrist,
	int value, Move best_move, TranspositionType type, int depth);

// try and opportunistically print the pv out of the transposition table
static void search_transposition_print_pv(Bitboard *board, Move move, int depth);

Move search_find_move(Bitboard *board)
{
	Move best_move = 0;

	/* note that due to the way that this is maintained, sibling nodes will
	   destroy each other's values. Only pv[0] is garunteed to be valid when
	   we get all the way back up here (the other slots are used for scratch
	   space, and are actually valid when the recursion layer using them
	   is the top level layer -- it's only siblings that ruin things :)) */
	Move pv[max_depth + max_quiescent_depth + 1];

	timeup = 0;
	timer_begin(&timeup);

	int alpha = -INFINITY;
	int beta = INFINITY;

	// for each depth, call the main workhorse, search_alpha_beta
	for (int depth = 1; depth <= max_depth; depth++)
	{
		fprintf(stderr, "SEARCHER depth %i ", depth);

		// here we go...
		int val = search_alpha_beta(board, alpha, beta, depth, 1, pv);

		if (((val <= alpha) || (val >= beta)) && !timeup)
		{
			// aspiration window failure
			fprintf(stderr, "aspiration failure (%i)\n", val);
			alpha = -INFINITY;
			beta = INFINITY;
			depth--;
			continue;
		}

		/* if we sucessfully completed a depth (i.e. did not early terminate
		   due to time), pull the first move off of the pv so that we can
		   return it later, and then print it. Note that there is a minor
		   race condition here: if we get the sigalarm after the very last
		   check in the search_alpha_beta recursive calls, but before we get
		   here, we can potentially throw away an entire depth's worth of
		   work. This is unforunate but won't really hurt anything */
		if (!timeup)
		{
			best_move = pv[0];

			/* use max_depth here, not depth -- we want to print out as much
			   of the PV as we have, but need to cut off repetitions somehow */
			search_transposition_print_pv(board, best_move, max_depth);

			fprintf(stderr, "(%i)\n", val);

			alpha = val - aspiration_window;
			beta = val + aspiration_window;

			if ((val >= MATE) || (val <= -MATE))
			{
				fprintf(stderr, "SEARCHER found mate\n");
				break;
			}
		}
		else
		{
			fprintf(stderr, "timeup\n");
			break;
		}
	}

	timer_end();

	generation++;
	return best_move;
}

static int search_alpha_beta(Bitboard *board,
	int alpha, int beta, int depth, int ply, Move *pv)
{
	if (timeup)
		return 0;

	TranspositionType type = TRANSPOSITION_ALPHA;

	int quiescent = depth <= 0;
	int in_check = board_in_check(board, board->to_move);

	/* *pv is used as the best move, but we have none yet if this gets
	   stuck in the transposition table, make sure no one uses some random
	   data */
	*pv = MOVE_NULL;

	// 50-move rule
	if (board->halfmove_count == 100)
		return 0;

	/* only check for repetitions down at least 1 ply, since it results in a
	   search termination without the game actually being over. Similarly, only
	   check the transposition table two at least 1 ply */
	if (ply > 1)
	{
		// 3 repetition rule
		// TODO test i += 2, start only if halfmove_count is >= 4, cut out on 1 rep
		for (uint8_t i = board->history_index - board->halfmove_count;
			i != board->history_index;
			i++)
		{
			if (board->history[i] == board->zobrist)
				return 0;
		}

		// check transposition table for a useful value
		int table_val = search_transposition_get_value(board->zobrist,
		                                               alpha,
		                                               beta,
		                                               depth);

		if (table_val != INFINITY)
			return table_val;
	}

	// leaf node
	if (depth <= -max_quiescent_depth)
	{
		int eval = evaluate_board(board);
		return eval;
	}

	// null move pruning
	if (quiescent)
	{
		// quiescent null-move; this is necessary for correctness
		int null_move_value = evaluate_board(board);

		if (null_move_value >= beta)
			return beta;
		else if (null_move_value > alpha)
		{
			alpha = null_move_value;
			type = TRANSPOSITION_EXACT;
		}
	}

	// generate pseudolegal moves
	Movelist moves;
	move_generate_movelist(board, &moves);

	// grab move from transposition table if we are not quiescent
	Move transposition_move = quiescent ? MOVE_NULL
		: search_transposition_get_best_move(board->zobrist);

	// move ordering; order transposition move first
	qsort_r(&(moves.moves),
		moves.num,
		sizeof(Move),
		&transposition_move,
		search_move_comparator);

	/* since we generate only pseudolegal moves, we need to keep track if
	   there actually are any legal moves at all */
	int legal_moves = 0;

	// loop thru all moves
	for (int i = 0; i < moves.num; i++)
	{
		Move move = moves.moves[i];

		/* if we're quiescent, we only want capture moves unless the
		   original position was in check, then do everything */
		if (quiescent && !in_check && !move_is_capture(move))
			break;

		board_do_move(board, move);

		// make the final legality check
		if (!board_in_check(board, 1-board->to_move))
		{
			// value from recursive call to alpha-beta search
			int recursive_value;

			// keeps track of various re-search conditions
			int search_completed = 0;

			int move_causes_check = board_in_check(board, board->to_move);
			int extensions = move_causes_check;

			if (type == TRANSPOSITION_EXACT)
			{
				// PV search
				search_completed = 1;
				recursive_value = -search_alpha_beta(board,
					-alpha - 1, -alpha,
					depth - 1 + extensions, ply + 1, pv + 1);

				if ((recursive_value > alpha) && (recursive_value < beta))
				{
					// PV search failed
					search_completed = 0;
				}
			}
			else if (i > 4 && depth > 2 && extensions == 0 &&
				!move_is_capture(move) && !move_is_promotion(move) &&
				!in_check && !move_causes_check)
			{
				// LMR
				search_completed = 1;
				recursive_value = -search_alpha_beta(board,
					-alpha - 1, -alpha,
					depth - 2, ply + 1, pv + 1);

				if (recursive_value > alpha)
				{
					// LMR failed
					search_completed = 0;
				}
			}

			if (!search_completed)
			{
				// normal search
				recursive_value = -search_alpha_beta(board,
					-beta, -alpha, depth - 1 + extensions,
					ply + 1, pv + 1);
			}

			board_undo_move(board);

			if (recursive_value >= beta)
			{
				/* since this move caused a beta cutoff, we don't want
				   to bother storing it in the pv *however*, we most
				   definitely want to put it in the transposition table,
				   since it will be searched first next time, and will
				   thus immediately cause a cutoff again */
				search_transposition_put(board->zobrist,
						recursive_value, move, TRANSPOSITION_BETA, depth);

				return recursive_value;
			}

			if (recursive_value > alpha)
			{
				alpha = recursive_value;
				type = TRANSPOSITION_EXACT;
				*pv = move;
			}

			legal_moves++;
		}
		else
			board_undo_move(board);
	}

	if (timeup)
		return 0;

	if (legal_moves == 0 && !quiescent)
	{
		// there are no legal moves for this game state -- check why
		int val = in_check ? -(MATE + depth) : 0;
		return val;
	}
	else if (legal_moves == 0 && quiescent)
	{
		int eval = evaluate_board(board);
		return eval;
	}
	else
	{
		search_transposition_put(board->zobrist, alpha, *pv, type, depth);
		return alpha;
	}
}

static int search_move_comparator(void *tm, const void *m1, const void *m2)
{
	/* sorts in this priority:
	   transposition move first,
	   captures before noncaptures,
	   more valuable captured pieces first,
	   less valuable capturing pieces first */

	Move dtm = *(Move*)tm;
	Move dm1 = *(const Move*)m1;
	Move dm2 = *(const Move*)m2;

	if (dtm == dm1)
		return -1;
	else if (dtm == dm2)
		return 1;

	int score1 = move_is_capture(dm1) ?
		15 + 2*move_captured_piecetype(dm1) - move_piecetype(dm1) : 0;
	int score2 = move_is_capture(dm2) ?
		15 + 2*move_captured_piecetype(dm2) - move_piecetype(dm2) : 0;

	return score2 - score1;
}

static int search_transposition_get_value(uint64_t zobrist,
	int alpha, int beta, int depth)
{
	int index = zobrist % max_transposition_table_size;
	TranspositionNode *node = &transposition_table[index];

	if (depth < 1)
		return INFINITY;

	/* since many zobrists may map to a single slot in the table, we want
	   to make sure we got a match; also, we want to make sure that the
	   entry was not made with a shallower depth than what we're currently
	   using */
	if (node->zobrist == zobrist)
	{
		if (node->depth >= depth)
		{
			int val = node->value;

			if (node->type == TRANSPOSITION_EXACT)
				return val;
			else if ((node->type == TRANSPOSITION_ALPHA) && (val <= alpha))
				return alpha;
			else if ((node->type == TRANSPOSITION_BETA) && (val >= beta))
				return beta;
		}
	}

	return INFINITY;
}

static Move search_transposition_get_best_move(uint64_t zobrist)
{
	Move result = 0;
	int index = zobrist % max_transposition_table_size;
	TranspositionNode *node = &transposition_table[index];

	if (node->zobrist == zobrist)
		result = node->best_move;

	return result;
}

static void search_transposition_put(uint64_t zobrist,
	int value, Move best_move, TranspositionType type, int depth)
{
	/* we might not store in the transnposition table if:
	    - time is up, thus we can't garuntee this was a full search
	    - the depth is not deep enough to be useful
	    - the value is dependent upon the ply at which it was searched and the
	      depth to which it was searched (currently, only MATE moves)
	    - it would replace a deeper search of this node
	 */
	if (timeup || (depth < 1) || (value >= MATE) || (value <= -MATE))
		return;

	int index = zobrist % max_transposition_table_size;
	TranspositionNode *node = &transposition_table[index];

	if ((node->zobrist == zobrist) && (node->depth > depth))
		return;

	// Should this be strict inequality? Seem to get a lot more hits this
	// way.
	if (node->generation >= generation + depth)
		return;

	node->zobrist = zobrist;
	node->depth = depth;
	node->generation = generation + depth;
	node->value = value;
	node->best_move = best_move;
	node->type = type;
}

static void search_transposition_print_pv(Bitboard *board, Move move, int depth) {
	char buf[6];

	if (!move || !depth)
		return;

	move_srcdest_form(move, buf);
	fprintf(stderr, "%s ", buf);

	board_do_move(board, move);
	search_transposition_print_pv(
		board,
		search_transposition_get_best_move(board->zobrist),
		depth - 1
	);
	board_undo_move(board);
}
