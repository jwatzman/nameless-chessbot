#include <assert.h>
#include <limits.h>
#include <stdlib.h>

#include "config.h"
#include "evaluate.h"
#include "history.h"
#include "moveiter.h"
#include "see.h"
#include "types.h"

// NB: any move with a negative score may be reduced by LMR and pruned by
// qsearch.
#define SCORE_TT INT_MAX
#define SCORE_WINNING_CAPTURE 2
#define SCORE_KILLER 1
#define SCORE_COUNTERMOVE 0
#define SCORE_OTHER (2 * SHRT_MIN - 1)
#define SCORE_LOSING_CAPTURE (4 * SHRT_MIN - 1)

static MoveScore moveiter_score(const Bitboard* board,
                                Move m,
                                Move tt_move,
                                const Move* killers,
                                Move countermove);

void moveiter_init(Moveiter* iter,
                   const Bitboard* board,
                   Movelist* list,
                   Move tt_move,
                   const Move* killers,
                   Move countermove) {
  iter->list = list;
  iter->n = 0;

  for (uint8_t i = 0; i < list->n; i++)
    iter->scores[i] =
        moveiter_score(board, list->moves[i], tt_move, killers, countermove);
}

int moveiter_has_next(Moveiter* iter) {
  return iter->n < iter->list->n;
}

Move moveiter_next(Moveiter* iter, MoveScore* s_out) {
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
  if (s_out)
    *s_out = best_s;

  assert(best_m != MOVE_NULL);
  return best_m;
}

static MoveScore moveiter_score(const Bitboard* board,
                                Move m,
                                Move tt_move,
                                const Move* killers,
                                Move countermove) {
  // Move ordering:
  // - Transposition table move
  // - Winning and equal captures
  // - Killer moves
  // - Non-captures (XXX including promotions?)
  // - Losing captures

  if (m == tt_move)
    return SCORE_TT;

  if (move_is_capture(m)) {
    int16_t see = see_see(board, m);
    MoveScore s;
    if (see >= 0) {
      s = SCORE_WINNING_CAPTURE + see;
      assert(s > SCORE_KILLER);
      assert(s > SCORE_COUNTERMOVE);
    } else {
      s = SCORE_LOSING_CAPTURE + see;
      assert(s < SCORE_KILLER);
      assert(s < SCORE_COUNTERMOVE);
      assert(s < SCORE_OTHER);
    }
    assert(s < SCORE_TT);
    return s;
  }

  if (killers && (m == killers[0] || m == killers[1])) {
    assert(!move_is_capture(m));
    return SCORE_KILLER;
  }

  if (m == countermove) {
    assert(!move_is_capture(m));
    return SCORE_COUNTERMOVE;
  }

  // Having moveiter call directly into history here isn't fantastic
  // factoring...
  // XXX in the past history was consulted during moveiter_next instead of in
  // one pass beforehand here. Is that better? (It's certainly very messy...)
  MoveScore s = SCORE_OTHER + history_get_combined(board, m);
  assert(s < SCORE_KILLER);
  assert(s < SCORE_COUNTERMOVE);
  assert(s > SCORE_LOSING_CAPTURE);
  return s;
}

int16_t moveiter_score_to_see(MoveScore s) {
  assert(s < SCORE_TT);
  assert(s != SCORE_KILLER);
  assert(s != SCORE_COUNTERMOVE);
  int see;
  if (s > 0)
    see = s - SCORE_WINNING_CAPTURE;
  else
    see = s - SCORE_LOSING_CAPTURE;

  assert(see < SHRT_MAX);
  assert(see > SHRT_MIN);
  return (int16_t)see;
}
