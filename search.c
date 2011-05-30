#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include "search.h"
#include "evaluate.h"
#include "move.h"

#define threads_enabled 1

typedef enum
{
	TRANSPOSITION_EXACT,
	TRANSPOSITION_ALPHA,
	TRANSPOSITION_BETA
}
TranspositionType;

typedef struct
{
	uint64_t zobrist;
	int depth;
	int value;
	Move best_move;
	TranspositionType type;
}
TranspositionNode;

/* the transposition table is kept over the lifetime of the program. This
   makes the assumption that zobrists will never conflict, which is
   incorrect but very unlikely to be an issue. Each block of entries in the
   table is protected by a mutex */
#define max_transposition_table_size 16777216
#define num_transposition_muticies (max_transposition_table_size / 10000)
static TranspositionNode transposition_table[max_transposition_table_size];
static int transposition_table_initalized = 0;

#if threads_enabled
static pthread_mutex_t transposition_muticies[num_transposition_muticies];
#endif

#define max_depth 8
#define max_quiescent_depth 30
static int current_max_depth; // how deep *this* iteration goes

/* info for use with pthreads calls; embodies all arguments to
   search_alpha_beta, return value, and thread id */
typedef struct
{
	Bitboard *board;
	int alpha;
	int beta;
	int depth;
	Move move;
	Move pv[max_depth + max_quiescent_depth + 1];
	int result;
	pthread_t tid;
}
SearchThread;

#define max_search_secs 5
static volatile int timeup;
static void sigalarm_handler(int signum);

// main search workhorse
static int search_alpha_beta(Bitboard *board,
	int alpha, int beta, int depth, Move *pv);

// pthread wrapper for search_alpha_beta
static void* search_alpha_beta_thread(void *thread);

// used to sort the moves for move ordering
static int search_move_comparator(const void *m1, const void *m2);

// sets up the array of muticies
static void search_transposition_initalize(void);

/* asks the transposition table if we already know a good value for this
   position. If we do, return it. Otherwise, return INFINITY but adjust
   *alpha and *beta if we know better bounds for them */
static int search_transposition_get_value(uint64_t zobrist,
		int *alpha, int *beta, int depth);

// if we have a previous best move for this zobrist, return it; 0 otherwise
static Move search_transposition_get_best_move(uint64_t zobrist);

// add to transposition table
static void search_transposition_put(uint64_t zobrist,
	int value, Move best_move, TranspositionType type, int depth);

static void sigalarm_handler(int signum)
{
	(void)signum;
	timeup = 1;
}

Move search_find_move(Bitboard *board)
{
	if (!transposition_table_initalized)
		search_transposition_initalize();

	Move best_move = 0;

	/* note that due to the way that this is maintained, sibling nodes will
	   destroy each other's values. Only pv[0] is garunteed to be valid when
	   we get all the way back up here (the other slots are used for scratch
	   space, and are actually valid when the recursion layer using them
	   is the top level layer -- it's only siblings that ruin things :)) */
	Move pv[max_depth + max_quiescent_depth + 1];

	/* set up the timer; a sigalarm is sent when the search should
	   prematurely end due to time, which in turn sets timeup = 1 */
	struct sigaction sigalarm_action;
	sigalarm_action.sa_handler = sigalarm_handler;
	sigemptyset(&sigalarm_action.sa_mask);
	sigalarm_action.sa_flags = 0;
	sigaction(SIGALRM, &sigalarm_action, 0);

	alarm(max_search_secs);

	timeup = 0;

	// for each depth, call the main workhorse, search_alpha_beta
	for (current_max_depth = 1;
	     current_max_depth <= max_depth;
	     current_max_depth++)
	{
		fprintf(stderr, "SEARCHER depth %i", current_max_depth);

		// here we go...
		int val = search_alpha_beta(board,
				-INFINITY, INFINITY, current_max_depth, pv);

		/* if we sucessfully completed a depth (i.e. did not early terminate
		   due to time), pull the first move off of the pv so that we can
		   return it later, and then print it. Note that there is a minor
		   race condition here: if we get the sigalarm after the very last
		   check in the search_alpha_beta recursive calls, but before we get
		   here, we can potentially throw away an entire depth's worth of
		   work. This is unforunate but won't really hurt anything */
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

static int search_alpha_beta(Bitboard *board,
	int alpha, int beta, int depth, Move *pv)
{
	TranspositionType type = TRANSPOSITION_ALPHA;

	// spawn threads only at the top level
	int spawn_threads = threads_enabled && depth == current_max_depth;
	SearchThread *threads;

	/* check if we're quiescent, and if we are set an in_check flag this
	   flag is only used to know what moves to skip for a quiescent search,
	   so don't bother doing the expensive computation if we're not
	   quiescent -- we will do it again if it becomes necessary */
	int quiescent = depth <= 0;
	int quiescent_in_check = quiescent ?
		board_in_check(board, board->to_move) : 0;

	/* *pv is used as the best move, but we have none yet if this gets
	   stuck in the transposition table, make sure no one uses some random
	   data */
	*pv = MOVE_NULL;

	// 50-move rule
	if (board->halfmove_count == 100)
		return 0;

	// 3 repitition rule
	int reps = 0;
	for (uint8_t i = board->history_index - board->halfmove_count;
	     i != board->history_index;
	     i++)
	{
		if (board->history[i] == board->zobrist)
			reps++;
	}
	if (reps >= 2)
		return 0;

	/* if we know an acceptable value in the table, use it; however, only
	   check the table after we get down a couple of ply */
	if (depth < current_max_depth - 1)
	{
		int table_val = search_transposition_get_value(board->zobrist,
		                                               &alpha,
		                                               &beta,
		                                               depth);

		if (table_val != INFINITY)
		{
			*pv = search_transposition_get_best_move(board->zobrist);
			return table_val;
		}
	}

	// leaf node
	if (depth <= -max_quiescent_depth)
	{
		int eval = evaluate_board(board);
		//search_transposition_put(board->zobrist, eval, 0, TRANSPOSITION_EXACT, depth);
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
	/*
	else if (depth < current_max_depth - 1 &&
		     board_last_move(board) != MOVE_NULL)
	{ // TODO add the right conditions to that if statement
		board_do_move(board, MOVE_NULL);

		int recursive_value = -search_alpha_beta(board,
			-beta, -(beta-1), depth - 3, pv + 1);

		board_undo_move(board);

		// TODO figure out if this can go into the table
		if (recursive_value >= beta)
			return beta;
	}
	*/

	// generate pseudolegal moves
	Movelist moves;
	move_generate_movelist(board, &moves);

	// move ordering
	qsort(&(moves.moves), moves.num, sizeof(Move), search_move_comparator);

	if (spawn_threads)
		threads = malloc(sizeof(SearchThread) * moves.num);

	/* since we generate only pseudolegal moves, we need to keep track if
	   there actually are any legal moves at all */
	int legal_moves = 0;

	Move transposition_move = quiescent ? MOVE_NULL
		: search_transposition_get_best_move(board->zobrist);

	/* loop over all of the moves. There are unfortunately two loop
	   indicies; "i" tracks the index into the moves.moves while "move_num"
	   tracks the absolute move number */
	for (int i = 0, move_num = 0;
		 (i < moves.num) && !timeup;
		 i++)
	{
		Move move;

		/* if we sucessfully got a move out of the transposition table and
		   have not tried it yet, try it first; otherwise continue moving
		   through the main body of moves */
		if (move_num == 0 && transposition_move != MOVE_NULL)
		{
			move = transposition_move;
			i--;
		}
		else
			move = moves.moves[i];

		/* if we're quiescent, we only want capture moves unless the
		   original position was in check, then do everything */
		if (quiescent && !quiescent_in_check && !move_is_capture(move))
			continue;

		// only try the transposition move once
		if (move_num > 0 && move == transposition_move)
			continue;

		board_do_move(board, move);

		// make the final legality check
		if (!board_in_check(board, 1-board->to_move))
		{
			if (!spawn_threads)
			{
				// if we're not spawning threads, go as normal
				int recursive_value = -search_alpha_beta(board,
					-beta, -alpha, depth - 1, pv + 1);

				board_undo_move(board);

				if (recursive_value >= beta)
				{
					/* since this move caused a beta cutoff, we don't want
					   to bother storing it in the pv *however*, we most
					   definitely want to put it in the transposition table,
					   since it will be searched first next time, and will
					   thus immediately cause a cutoff again */
					search_transposition_put(board->zobrist,
						beta, move, TRANSPOSITION_BETA, depth);

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
			{
				// we are spawning theads here, just tell everybody to go!
				SearchThread *thread = &(threads[legal_moves]);

				thread->board = malloc(sizeof(Bitboard));
				memcpy(thread->board, board, sizeof(Bitboard));

				thread->alpha = -beta;
				thread->beta = -alpha;
				thread->depth = depth - 1;
				thread->move = move;

				pthread_create(&(thread->tid),
					NULL, search_alpha_beta_thread, thread);
				// search_alpha_beta_thread(thread);

				board_undo_move(board);
			}

			legal_moves++;
		}
		else
			board_undo_move(board);

		move_num++;
	}

	if (spawn_threads)
	{
		// if we spanwed threads, get the result out of all of them
		for (int i = 0; i < legal_moves; i++)
		{
			SearchThread *thread = &(threads[i]);
			pthread_join(thread->tid, NULL);

			/* don't bother to check beta cutoffs; we've already done all
			   of the work */
			if (thread->result > alpha)
			{
				alpha = thread->result;
				type = TRANSPOSITION_EXACT;
				*pv = thread->move;
			}

			free(thread->board);
		}

		free(threads);
	}

	if (legal_moves == 0 && !quiescent)
	{
		// there are no legal moves for this game state -- check why
		int val = board_in_check(board, board->to_move) ? -(MATE + depth) : 0;
		//search_transposition_put(board->zobrist, val, 0, TRANSPOSITION_EXACT, depth);
		return val;
	}
	else if (legal_moves == 0 && quiescent)
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

static void* search_alpha_beta_thread(void *thread)
{
	SearchThread *thread2 = thread;
	thread2->result = -search_alpha_beta(thread2->board,
		thread2->alpha, thread2->beta, thread2->depth, thread2->pv);
	return NULL;
}

static int search_move_comparator(const void *m1, const void *m2)
{
	/* sorts in this priority:
	   captures before noncaptures,
	   more valuable captured pieces first,
	   less valuable capturing pieces first */

	Move dm1 = *(const Move*)m1;
	Move dm2 = *(const Move*)m2;

	int score1 = move_is_capture(dm1) ?
		15 + 2*move_captured_piecetype(dm1) - move_piecetype(dm1) : 0;
	int score2 = move_is_capture(dm2) ?
		15 + 2*move_captured_piecetype(dm2) - move_piecetype(dm2) : 0;

	return score2 - score1;
}

static void search_transposition_initalize(void)
{
#if threads_enabled
	for (int i = 0; i < num_transposition_muticies; i++)
		pthread_mutex_init(&(transposition_muticies[i]), NULL);
#endif

	transposition_table_initalized = 1;
}

static int search_transposition_get_value(uint64_t zobrist,
	int *alpha, int *beta, int depth)
{
	int index = zobrist % max_transposition_table_size;
	TranspositionNode *node = &transposition_table[index];
	int ret = INFINITY;

#if threads_enabled
	pthread_mutex_lock(
			&(transposition_muticies[index % num_transposition_muticies]));
#endif

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
			{
				ret = val;
			}
			else if (node->type == TRANSPOSITION_ALPHA)
			{
				if (val <= *alpha)
					ret = *alpha;
				else if (val < *beta)
					*beta = val;
			}
			else if (node->type == TRANSPOSITION_BETA)
			{
				if (val >= *beta)
					ret = *beta;
				else if (val > *alpha)
					*alpha = val;
			}
		}
	}

#if threads_enabled
	pthread_mutex_unlock(
			&(transposition_muticies[index % num_transposition_muticies]));
#endif

	return ret;
}

static Move search_transposition_get_best_move(uint64_t zobrist)
{
	Move result = 0;
	int index = zobrist % max_transposition_table_size;
	TranspositionNode *node = &transposition_table[index];

#if threads_enabled
	pthread_mutex_lock(
			&(transposition_muticies[index % num_transposition_muticies]));
#endif

	if (node->zobrist == zobrist)
		result = node->best_move;

#if threads_enabled
	pthread_mutex_unlock(
			&(transposition_muticies[index % num_transposition_muticies]));
#endif

	return result;
}

static void search_transposition_put(uint64_t zobrist,
	int value, Move best_move, TranspositionType type, int depth)
{
	/* make sure that only data from completed subtrees makes it in the
	   table; since this is only called right before search_alpha_beta
	   returns, we just want to make sure the main search loop finished for
	   that subtree */
	if (timeup)
		return;

	int index = zobrist % max_transposition_table_size;
	TranspositionNode *node = &transposition_table[index];

#if threads_enabled
	pthread_mutex_lock(
			&(transposition_muticies[index % num_transposition_muticies]));
#endif

	node->zobrist = zobrist;
	node->depth = depth;
	node->value = value;
	node->best_move = best_move;
	node->type = type;

#if threads_enabled
	pthread_mutex_unlock(
			&(transposition_muticies[index % num_transposition_muticies]));
#endif
}
