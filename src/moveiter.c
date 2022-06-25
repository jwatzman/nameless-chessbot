#include <stdlib.h>

#include "moveiter.h"

static Move moveiter_nth(Moveiter* iter, int n);
static Move moveiter_next_nosort(Moveiter* iter);
static Move moveiter_next_selection(Moveiter* iter);

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

  switch (mode) {
    case MOVEITER_SORT_FULL:
      moveiter_qsort(list);
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

int moveiter_has_next(Moveiter* iter) {
  return iter->pos < iter->movelist->num_total;
}

Move moveiter_next(Moveiter* iter) {
  return iter->next(iter);
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

static Move moveiter_next_selection(Moveiter* iter) {
  iter->pos++;

  if (iter->pos == 1 && iter->forced_first != MOVE_NULL)
    return iter->forced_first;

  int n = iter->pos - 1;
  if (iter->forced_first != MOVE_NULL)
    n--;

  Move* best_move = NULL;

  if (n < iter->movelist->num_promo) {
    for (int i = 0; i < iter->movelist->num_promo; i++) {
      Move* m = &iter->movelist->moves_promo[i];
      if (*m != MOVE_NULL && *m != iter->forced_first) {
        best_move = m;
        break;
      }
    }

    n = iter->movelist->num_promo;
  }

  n -= iter->movelist->num_promo;
  if (!best_move && n < iter->movelist->num_capture) {
    int best_move_score = -1;
    for (int i = 0; i < iter->movelist->num_capture; i++) {
      Move* m = &iter->movelist->moves_capture[i];
      if (*m == MOVE_NULL || *m == iter->forced_first)
        continue;

      int m_score = moveiter_score(*m);
      if (m_score > best_move_score) {
        best_move = m;
        best_move_score = m_score;
      }
    }

    n = iter->movelist->num_capture;
  }

  n -= iter->movelist->num_capture;
  if (!best_move) {
    for (int i = 0; i < iter->movelist->num_other; i++) {
      Move* m = &iter->movelist->moves_other[i];
      if (*m != MOVE_NULL && *m != iter->forced_first) {
        best_move = m;
        break;
      }
    }
  }

  if (!best_move)
    abort();

  Move result = *best_move;
  *best_move = MOVE_NULL;
  return result;
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
