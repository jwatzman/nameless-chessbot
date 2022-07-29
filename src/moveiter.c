#include <limits.h>
#include <stdlib.h>

#include "history.h"
#include "moveiter.h"

#define SCORE_TT SCHAR_MAX
#define SCORE_CAPTURE 0
#define SCORE_KILLER (SCHAR_MIN + 1)
#define SCORE_OTHER SCHAR_MIN

static MoveScore moveiter_score(Move m, Move tt_move, const Move* killers);

void moveiter_init(Moveiter* iter,
                   Movelist* list,
                   Move tt_move,
                   const Move* killers) {
  iter->list = list;
  iter->n = 0;

  for (uint8_t i = 0; i < list->n; i++)
    iter->scores[i] = moveiter_score(list->moves[i], tt_move, killers);
}

int moveiter_has_next(Moveiter* iter) {
  return iter->n < iter->list->n;
}

Move moveiter_next(Moveiter* iter) {
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
    } else if (s == best_s) {
      // This is an incredibly ugly hacky way to deal with this -- history is
      // "separate" from the score for not a terribly good reason (annoying to
      // stuff it into the 8 bits available for score), and moveiter directly
      // calls into history which is not great factoring. But it works and
      // there's not a clear better way to deal with this right now?
      uint16_t best_h = history_get(best_m);
      uint16_t h = history_get(m);
      if (h > best_h) {
        best_s = s;
        best_m = m;
        best_i = i;
      }
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

static MoveScore moveiter_score(Move m, Move tt_move, const Move* killers) {
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
