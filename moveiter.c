#define _GNU_SOURCE
#include <stdlib.h>
#include "moveiter.h"

static Move moveiter_next_nosort(Moveiter *iter);
static Move moveiter_next_selection(Moveiter *iter);

static int moveiter_comparator(void *tm, const void *m1, const void *m2);
static int moveiter_score(Move m, Move best);
static void moveiter_qsort(Movelist *moves, Move best);

void moveiter_init(Moveiter *iter, Movelist *list, int mode, Move forced_first)
{
	iter->movelist = list;
	iter->forced_first = forced_first;
	iter->last_score = 100000;
	iter->pos = 0;

	switch (mode) {
		case MOVEITER_SORT_FULL:
			moveiter_qsort(list, forced_first);
			// fallthrough
		case MOVEITER_SORT_NONE:
			iter->next = moveiter_next_nosort;
			break;
		case MOVEITER_SORT_ONDEMAND:
			iter->next = moveiter_next_selection;
			break;
		default:
			abort();
	}
}

int moveiter_has_next(Moveiter *iter)
{
	return iter->pos < iter->movelist->num;
}

Move moveiter_next(Moveiter *iter)
{
	return iter->next(iter);
}

static Move moveiter_next_nosort(Moveiter *iter)
{
	return iter->movelist->moves[iter->pos++];
}

static Move moveiter_next_selection(Moveiter *iter)
{
	iter->pos++;
	/*
	if (iter->pos == 1 && iter->forced_first != MOVE_NULL)
	{
		iter->last_score = moveiter_score(iter->forced_first, iter->forced_first);
		// XXX need to figure out how to deal with adding pos here
		return iter->forced_first;
	}
	*/

	Move best_move = iter->movelist->moves[0];
	int best_move_score = moveiter_score(best_move, iter->forced_first);
	for (int i = 1; i < iter->movelist->num; i++)
	{
		Move m = iter->movelist->moves[i];
		int m_score = moveiter_score(m, iter->forced_first) + i;

		if (m_score > best_move_score && m_score < iter->last_score)
		{
			best_move = m;
			best_move_score = m_score;
		}
	}

	if (iter->last_score == best_move_score) abort();
	iter->last_score = best_move_score;
	return best_move;
}

static int moveiter_comparator(void *tm, const void *m1, const void *m2)
{
	Move dtm = *(Move*)tm;
	Move dm1 = *(const Move*)m1;
	Move dm2 = *(const Move*)m2;

	return moveiter_score(dm2, dtm) - moveiter_score(dm1, dtm);
}

static int moveiter_score(Move m, Move best)
{
	/* sorts in this priority:
	   transposition move first,
	   captures before noncaptures,
	   more valuable captured pieces first,
	   less valuable capturing pieces first */

	if (m == best)
		return 10000;

	if (!move_is_capture(m))
		return 0;

	return 1000 + 100*move_captured_piecetype(m) - 10*move_piecetype(m);
}

#if __APPLE__

static void moveiter_qsort(Movelist *moves, Move best)
{
	qsort_r(&(moves->moves),
		moves->num,
		sizeof(Move),
		&best,
		moveiter_comparator);
}

#elif __GLIBC__

static int moveiter_comparator_glibc(const void *m1, const void *m2, void *tm)
{
	return moveiter_comparator(tm, m1, m2);
}

static void moveiter_qsort(Movelist *moves, Move best)
{
	qsort_r(&(moves->moves),
		moves->num,
		sizeof(Move),
		moveiter_comparator_glibc,
		&best);
}

#else
#error Unknown how to call qsort_r with your libc.
#endif
