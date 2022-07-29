#include <assert.h>
#include <limits.h>
#include <stdlib.h>

#include "config.h"
#include "evaluate.h"
#include "history.h"
#include "moveiter.h"
#include "types.h"

#define SCORE_TT INT_MAX
#define SCORE_CAPTURE 0
#define SCORE_KILLER SHRT_MIN
#define SCORE_OTHER INT_MIN

static MoveScore moveiter_score(const Bitboard* board,
                                Move m,
                                Move tt_move,
                                const Move* killers);

void moveiter_init(Moveiter* iter,
                   const Bitboard* board,
                   Movelist* list,
                   Move tt_move,
                   const Move* killers) {
  iter->list = list;
  iter->n = 0;

  for (uint8_t i = 0; i < list->n; i++)
    iter->scores[i] = moveiter_score(board, list->moves[i], tt_move, killers);
}

int moveiter_has_next(Moveiter* iter) {
  return iter->n < iter->list->n;
}

Move moveiter_next(Moveiter* iter) {
  assert(moveiter_has_next(iter));

  // Selection sort for next best move. Much slower than qsort if we need to
  // sort the whole list, but the number of cases where we do that are so
  // outweighed by the cases were we need part of a list that doing this is way
  // faster.
  Move best_m = iter->list->moves[iter->n];
  MoveScore best_s = iter->scores[iter->n];
  uint8_t best_i = iter->n;
  for (uint8_t i = iter->n + 1; i < iter->list->n; i++) {
    Move m = iter->list->moves[i];
    MoveScore s = iter->scores[i];
    if (s > best_s) {
      best_s = s;
      best_m = m;
      best_i = i;
    }
  }

  // Selection sort swaps the best with the first, but we only need to do half
  // of that since we only make one pass through the list: write the first into
  // the slot where the best was, and then return the best. (Do not bother to
  // write the best back into the first slot, which we will never look at
  // again.)
  iter->list->moves[best_i] = iter->list->moves[iter->n];
  iter->scores[best_i] = iter->scores[iter->n];
  iter->n++;
  return best_m;
}

static MoveScore moveiter_score(const Bitboard* board,
                                Move m,
                                Move tt_move,
                                const Move* killers) {
  // Move ordering:
  // - Transposition table move
  // - Captures, MVV/LVA or SEE
  // - Killer moves (empirically seems to work better *after* captures -- XXX
  //   losing captures too?)
  // - Everything else (XXX including promotions?)

  if (m == tt_move)
    return SCORE_TT;

  if (move_is_capture(m)) {
#if ENABLE_SEE_SORTING
    return SCORE_CAPTURE + evaluate_see(board, m);
#else
    (void)board;
    MoveScore s = SCORE_CAPTURE + 6 * move_captured_piecetype(m) +
                  (5 - move_piecetype(m));
    assert(s > SCORE_KILLER);
    assert(s < SCORE_TT);
    return s;
#endif
  }

  if (killers && (m == killers[0] || m == killers[1])) {
    assert(!move_is_capture(m));
    return SCORE_KILLER;
  }

  // Having moveiter call directly into history here isn't fantastic
  // factoring...
  // XXX in the past history was consulted during moveiter_next instead of in
  // one pass beforehand here. Is that better? (It's certainly very messy...)
  MoveScore s = SCORE_OTHER + history_get(m);
  assert(s < SCORE_KILLER);
  return s;
}
