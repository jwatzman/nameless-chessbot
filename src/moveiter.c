#include <stdlib.h>

#include "moveiter.h"

static Move moveiter_nth(Moveiter* iter, int n);
static Move moveiter_next_nosort(Moveiter* iter);

static int moveiter_comparator(const void* m1, const void* m2);
static int moveiter_score(Move m);
static void moveiter_qsort(Movelist* moves);

void moveiter_init(Moveiter* iter,
                   Movelist* list,
                   int mode,
                   Move forced_first) {
  iter->movelist = list;
  iter->forced_first = forced_first;
  iter->pos = 0;

  if (mode == MOVEITER_SORT_FULL)
    moveiter_qsort(list);
}

int moveiter_has_next(Moveiter* iter) {
  return iter->pos < iter->movelist->num_total;
}

Move moveiter_next(Moveiter* iter) {
  return moveiter_next_nosort(iter);
}

static Move moveiter_nth(Moveiter* iter, int n) {
  if (n < iter->movelist->num_promo)
    return iter->movelist->moves_promo[n];
  n -= iter->movelist->num_promo;

  if (n < iter->movelist->num_capture)
    return iter->movelist->moves_capture[n];
  n -= iter->movelist->num_capture;

  return iter->movelist->moves_other[n];
}

static Move moveiter_next_nosort(Moveiter* iter) {
  iter->pos++;

  if (iter->pos == 1 && iter->forced_first != MOVE_NULL)
    return iter->forced_first;

  int n = iter->pos - 1;
  if (iter->forced_first != MOVE_NULL)
    n--;

  Move m = moveiter_nth(iter, n);
  if (m == iter->forced_first) {
    n++;
    iter->forced_first = MOVE_NULL;
    m = moveiter_nth(iter, n);
  }

  return m;
}

static int moveiter_comparator(const void* m1, const void* m2) {
  Move dm1 = *(const Move*)m1;
  Move dm2 = *(const Move*)m2;

  return moveiter_score(dm2) - moveiter_score(dm1);
}

static int moveiter_score(Move m) {
  /* sorts in this priority:
     transposition move first,
     captures before noncaptures,
     more valuable captured pieces first,
     less valuable capturing pieces first */

  if (!move_is_capture(m))
    return 0;

  return 15 + 2 * move_captured_piecetype(m) - move_piecetype(m);
}

static void moveiter_qsort(Movelist* moves) {
  qsort(&(moves->moves_capture), moves->num_capture, sizeof(Move),
        moveiter_comparator);
}
