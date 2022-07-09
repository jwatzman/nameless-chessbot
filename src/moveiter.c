#include <limits.h>
#include <stdlib.h>

#include "moveiter.h"

#define SCORE_TT SCHAR_MAX
#define SCORE_CAPTURE 0
#define SCORE_KILLER (SCHAR_MIN + 1)
#define SCORE_OTHER SCHAR_MIN

#define INJECT_SCORE(move, score) \
  ((move) | (Move)((score) << move_unused_offset))
#define EXTRACT_SCORE(move) ((int32_t)(move) >> move_unused_offset)
#define CLEAN_MOVE(move) ((move)&0x00ffffff)

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
  return CLEAN_MOVE(m);
}

static int moveiter_comparator(const void* p1, const void* p2) {
  int8_t s1 = EXTRACT_SCORE(*(const Move*)p1);
  int8_t s2 = EXTRACT_SCORE(*(const Move*)p2);

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

  return SCORE_CAPTURE + 6 * move_captured_piecetype(m) +
         (5 - move_piecetype(m));
}

static void moveiter_qsort(Movelist* list, Move tt_move, Move* killers) {
  uint8_t n = list->n;
  for (int i = 0; i < n; i++) {
    Move m = list->moves[i];
    int8_t s = moveiter_score(m, tt_move, killers);
    list->moves[i] = INJECT_SCORE(m, s);
  }

  qsort(list->moves, n, sizeof(Move), moveiter_comparator);
}
