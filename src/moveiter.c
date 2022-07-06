#include <limits.h>
#include <stdlib.h>

#include "moveiter.h"

#define SCORE_TT SCHAR_MAX
#define SCORE_CAPTURE 0
#define SCORE_KILLER (SCHAR_MIN + 1)
#define SCORE_OTHER SCHAR_MIN

typedef struct {
  Move m;
  int8_t s;
} MoveAndScore;

static int moveiter_comparator(const void* m1, const void* m2);
static int8_t moveiter_score(Move m, Move tt_move, Move* killers);
static void moveiter_qsort(Movelist* moves, Move tt_move, Move* killers);

void moveiter_init(Moveiter* iter,
                   Movelist* list,
                   int mode,
                   Move tt_move,
                   Move* killers) {
  iter->m = list->moves;
  list->moves[list->n] = MOVE_NULL;

  if (mode == MOVEITER_SORT_FULL)
    moveiter_qsort(list, tt_move, killers);
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
  int8_t s1 = ((const MoveAndScore*)p1)->s;
  int8_t s2 = ((const MoveAndScore*)p2)->s;

  return s2 - s1;
}

static int8_t moveiter_score(Move m, Move tt_move, Move* killers) {
  // Move ordering:
  // - Transposition table move
  // - Captures, MVV/LVA
  // - Killer moves (empirically seems to work better *after* captures)
  // - Everything else (XXX including promotions?)

  if (m == tt_move)
    return SCORE_TT;

  if (killers && (m == killers[0] || m == killers[1]))
    return SCORE_KILLER;

  if (!move_is_capture(m))
    return SCORE_OTHER;

  return SCORE_CAPTURE + 2 * move_captured_piecetype(m) - move_piecetype(m);
}

static void moveiter_qsort(Movelist* list, Move tt_move, Move* killers) {
  MoveAndScore pairs[MAX_MOVES];
  uint8_t n = list->n;
  for (int i = 0; i < n; i++) {
    Move m = list->moves[i];
    int8_t score = moveiter_score(m, tt_move, killers);
    pairs[i].m = m;
    pairs[i].s = score;
  }

  qsort(pairs, n, sizeof(MoveAndScore), moveiter_comparator);

  for (int i = 0; i < n; i++)
    list->moves[i] = pairs[i].m;
}
