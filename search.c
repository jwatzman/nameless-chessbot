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

static const int max_transposition_table_size = 16384;

// this is kept over the entire run of the program, even over multiple games
// zobrist hashes are assumed to be univerally unique... this is perhaps
// an invalid assumption, but i make it anyways :D
static TranspositionNode transposition_table[16384];

static const int max_depth = 10;

static const int max_search_secs = 12;
static int timeup;

static void sigalarm_handler(int signum);

static int search_alpha_beta(Bitboard *board, int alpha, int beta, int depth, Move* pv);
static int search_move_comparator(const void *m1, const void *m2);

// returns INFINITY if unknown
static int search_transposition_get_value(uint64_t zobrist, int alpha, int beta, int depth);

// returns 0 if unknown
static Move search_transposition_get_best_move(uint64_t zobrist);

static void search_transposition_put(uint64_t zobrist, int value, Move best_move, TranspositionType type, int depth);

static void sigalarm_handler(int signum)
{
	timeup = 1;
}

Move search_find_move(Bitboard *board)
{
	Move best_move = 0;
	Move pv[max_depth];

	struct sigaction sigalarm_action;
	sigalarm_action.sa_handler = sigalarm_handler;
	sigemptyset(&sigalarm_action.sa_mask);
	sigalarm_action.sa_flags = 0;
	sigaction(SIGALRM, &sigalarm_action, 0);

	alarm(max_search_secs);

	timeup = 0;

	for (int depth = 1; depth <= max_depth; depth++)
	{
		fprintf(stderr, "SEARCHER depth %i", depth);
		search_alpha_beta(board, -INFINITY, INFINITY, depth, pv);

		if (!timeup)
		{
			char buf[6];
			fprintf(stderr, " pv");

			for (int i = 0; i < depth; i++)
			{
				move_srcdest_form(pv[i], buf);
				fprintf(stderr, " %s", buf);
			}

			fprintf(stderr, "\n");

			best_move = pv[0];
		}
		else
		{
			fprintf(stderr, " timeup\n");
			break;
		}
	}

	alarm(0);

	return best_move;
}

static int search_alpha_beta(Bitboard *board, int alpha, int beta, int depth, Move* pv)
{
	TranspositionType type = TRANSPOSITION_ALPHA;

	int table_val = search_transposition_get_value(board->zobrist, alpha, beta, depth);
	if (table_val != INFINITY)
		return table_val;

	if (depth == 0)
	{
		int eval = evaluate_board(board);
		search_transposition_put(board->zobrist, eval, 0, TRANSPOSITION_EXACT, depth);
		return eval;
	}

	if (board->halfmove_count == 100)
		return 0;

	int reps = 0;
	for (uint8_t i = board->history_index - board->halfmove_count; i < board->history_index; i++)
		if (board->history[i] == board->zobrist)
			reps++;
	if (reps >= 3)
		return 0;

	Movelist moves;
	move_generate_movelist(board, &moves);
	qsort(&(moves.moves), moves.num, sizeof(Move), search_move_comparator);
	int found_move = 0;

	Move transposition_move = search_transposition_get_best_move(board->zobrist);

	for (int i = 0; (i < moves.num) && !timeup; i++)
	{
		Move move;

		if (transposition_move)
		{
			move = transposition_move;
			transposition_move = 0;
			i--;
		}
		else
			move = moves.moves[i];

		board_do_move(board, move);

		if (!board_in_check(board, 1-board->to_move))
		{
			found_move = 1;
			int recursive_value = -search_alpha_beta(board, -beta, -alpha, depth - 1, pv + 1);
			board_undo_move(board, move);

			if (recursive_value >= beta)
			{
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
	}

	if (!found_move)
	{
		int val = board_in_check(board, board->to_move) ? -(MATE + depth) : 0;
		search_transposition_put(board->zobrist, val, *pv, TRANSPOSITION_EXACT, depth);
		return val;
	}
	else
	{
		search_transposition_put(board->zobrist, alpha, *pv, type, depth);
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
	if (timeup)
		return;

	TranspositionNode *node = &transposition_table[zobrist % max_transposition_table_size];
	node->zobrist = zobrist;
	node->depth = depth;
	node->value = value;
	node->best_move = best_move;
	node->type = type;
}
