#define _GNU_SOURCE
#include <stdlib.h>
#include "moveiter.h"

static Move moveiter_next_nosort(Moveiter *iter);
static Move moveiter_next_selection(Moveiter *iter);

static int moveiter_comparator(void *tm, const void *m1, const void *m2);
static void moveiter_qsort(Movelist *moves, Move best);

void moveiter_init(Moveiter *iter, Movelist *list, int mode, Move forced_first)
{
	iter->movelist = list;
	iter->forced_first = forced_first;
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
	abort();
}

static int moveiter_comparator(void *tm, const void *m1, const void *m2)
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
