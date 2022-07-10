#include <limits.h>
#include <stdlib.h>

#include "history.h"
#include "moveiter.h"

typedef int8_t Score;

#define SCORE_TT SCHAR_MAX
#define SCORE_CAPTURE 0
#define SCORE_KILLER (SCHAR_MIN + 1)
#define SCORE_OTHER SCHAR_MIN

#define INJECT_SCORE(move, score) \
  ((move) | (Move)((score) << move_unused_offset))
#define EXTRACT_SCORE(move) ((int32_t)(move) >> move_unused_offset)
#define CLEAN_MOVE(move) ((move)&0x00ffffff)

static Score moveiter_score(Move m, Move tt_move, const Move* killers);
static void moveiter_inject_scores(Movelist* moves,
                                   Move tt_move,
                                   const Move* killers);

void moveiter_init(Moveiter* iter,
                   Movelist* list,
                   Move tt_move,
                   const Move* killers) {
  iter->m = list->moves;
  list->moves[list->n] = MOVE_NULL;

  moveiter_inject_scores(list, tt_move, killers);
}

int moveiter_has_next(Moveiter* iter) {
  return *iter->m != MOVE_NULL;
}

Move moveiter_next(Moveiter* iter) {
  // Selection sort for next best move. Much slower than qsort if we need to
  // sort the whole list, but the number of cases where we do that are so
  // outweighed by the cases were we need part of a list that doing this is way
  // faster.
  Move* best_m = iter->m;
  Score best_s = EXTRACT_SCORE(*best_m);
  for (Move* p = iter->m + 1; *p != MOVE_NULL; p++) {
    Score s = EXTRACT_SCORE(*p);
    if (s > best_s) {
      best_s = s;
      best_m = p;
    } else if (s == best_s) {
      // This is an incredibly ugly hacky way to deal with this -- history is
      // "separate" from the score for not a terribly good reason (annoying to
      // stuff it into the 8 bits available for score), and moveiter directly
      // calls into history which is not great factoring. But it works and
      // there's not a clear better way to deal with this right now?
      uint16_t best_h = history_get(*best_m);
      uint16_t h = history_get(*p);
      if (h > best_h) {
        best_s = s;
        best_m = p;
      }
    }
  }

  // Selection sort swaps the best with the first, but we only need to do half
  // of that since we only make one pass through the list: write the first into
  // the slot where the best was, and then return the best. (Do not bother to
  // write the best back into the first slot, which we will never look at
  // again.)
  Move ret = CLEAN_MOVE(*best_m);
  *best_m = *iter->m;
  iter->m++;
  return ret;
}

static Score moveiter_score(Move m, Move tt_move, const Move* killers) {
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

static void moveiter_inject_scores(Movelist* list,
                                   Move tt_move,
                                   const Move* killers) {
  uint8_t n = list->n;
  for (int i = 0; i < n; i++) {
    Move m = list->moves[i];
    Score s = moveiter_score(m, tt_move, killers);
    list->moves[i] = INJECT_SCORE(m, s);
  }
}
