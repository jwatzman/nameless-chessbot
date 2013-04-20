#define _GNU_SOURCE
#include <stdlib.h>
#include "moveiter.h"

static Move moveiter_next_nosort(Moveiter *iter);
static Move moveiter_next_selection(Moveiter *iter);

void moveiter_init(Moveiter *iter, Movelist *list, int mode, Move forced_first)
{
	iter->movelist = list;
	iter->forced_first = forced_first;
	iter->pos = 0;

	switch (mode) {
		case MOVEITER_SORT_FULL:
			// qsort
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
