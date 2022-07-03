#include <stdlib.h>

#include "moveiter.h"

typedef struct {
  Move m;
  int8_t s;
} MoveAndScore;

static int moveiter_comparator(const void* m1, const void* m2);
static int8_t moveiter_score(Move m, Move tt_move);
static void moveiter_qsort(Movelist* moves, Move tt_move);

void moveiter_init(Moveiter* iter, Movelist* list, int mode, Move tt_move) {
  iter->m = list->moves;
  list->moves[list->n] = MOVE_NULL;

  if (mode == MOVEITER_SORT_FULL)
    moveiter_qsort(list, tt_move);
}

int moveiter_has_next(Moveiter* iter) {
  return *iter->m != MOVE_NULL;
}

Move moveiter_next(Moveiter* iter) {
  Move m = *iter->m;
  iter->m++;
  return m;
}

static int moveiter_comparator(const void* p1, const void* p2) {
  // Move dm1 = *(const Move*)m1;
  // Move dm2 = *(const Move*)m2;
  // const MoveAndScore *p1 = (const MoveAndScore*)m1;
  // const MoveAndScore *p1 = (const MoveAndScore*)m1;
  int8_t s1 = ((const MoveAndScore*)p1)->s;
  int8_t s2 = ((const MoveAndScore*)p2)->s;

  return s2 - s1;
}

static int8_t moveiter_score(Move m, Move tt_move) {
  /* sorts in this priority:
     transposition move first,
     captures before noncaptures,
     more valuable captured pieces first,
     less valuable capturing pieces first */

  if (m == tt_move)
    return 127;

  if (!move_is_capture(m))
    return 0;

  return 15 + 2 * move_captured_piecetype(m) - move_piecetype(m);
}

static void moveiter_qsort(Movelist* list, Move tt_move) {
  MoveAndScore pairs[MAX_MOVES];
  uint8_t n = list->n;
  for (int i = 0; i < n; i++) {
    Move m = list->moves[i];
    int8_t score = moveiter_score(m, tt_move);
    pairs[i].m = m;
    pairs[i].s = score;
  }

  qsort(pairs, n, sizeof(MoveAndScore), moveiter_comparator);

  for (int i = 0; i < n; i++)
    list->moves[i] = pairs[i].m;
}
