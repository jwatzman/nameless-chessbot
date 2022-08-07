#include <limits.h>
#include <stdint.h>
#include <string.h>

#include "assert.h"
#include "config.h"
#include "history.h"
#include "move.h"
#include "search.h"
#include "types.h"

#define MAX_HISTORY_PLY MAX_POSSIBLE_DEPTH
#define MAX_HISTORY_VALUE SHRT_MAX

static Move killers[MAX_HISTORY_PLY][2];
static Move countermoves[2][6][64];
static int16_t history[2][64][64];

#if ENABLE_HISTORY_DECREMENT
static void history_incr(Move m, int8_t depth, int good) {
  int16_t* p =
      &history[move_color(m)][move_source_index(m)][move_destination_index(m)];

  int incr = depth * depth;
  int decay = incr * *p / MAX_HISTORY_VALUE;

  if (!good)
    incr = -incr;

  int new = *p + incr - decay;
  assert(new > SHRT_MIN);
  assert(new < SHRT_MAX);

  *p = (int16_t) new;
}
#endif

void history_clear(void) {
  // Assumes MOVE_NULL is 0!
  memset(killers, 0, sizeof(killers));
  memset(countermoves, 0, sizeof(countermoves));

  // XXX should we keep this across searches? Halve every value upon new search?
  memset(history, 0, sizeof(history));
}

void history_update(const Bitboard* board,
                    Move best,
                    const Move* bad,
                    int num_bad,
                    int8_t depth,
                    int8_t ply) {
  if (move_is_capture(best))
    return;

#if ENABLE_HISTORY
  if (depth > 2) {
#if ENABLE_HISTORY_DECREMENT
    history_incr(best, depth, 1);
    for (int i = 0; i < num_bad; i++)
      history_incr(bad[i], depth, 0);
#else
    (void)bad;
    (void)num_bad;
    history[move_color(best)][move_source_index(best)]
           [move_destination_index(best)] += depth * 2;
#endif
  }
#endif

#if ENABLE_KILLERS
  if (ply < MAX_HISTORY_PLY) {
    Move* slot = (Move*)history_get_killers(ply);
    assert(slot);

    if (slot[0] == best || slot[1] == best)
      return;

    if (slot[0] == MOVE_NULL) {
      slot[0] = best;
    } else {
      slot[1] = slot[0];
      slot[0] = best;
    }
  }
#endif

#if ENABLE_COUNTERMOVES
  Move last_move = board->state->last_move;
  if (last_move != MOVE_NULL) {
    assert((!board->to_move) == move_color(last_move));
    countermoves[!board->to_move][move_piecetype(last_move)]
                [move_destination_index(last_move)] = best;
  }
#else
  (void)board;
#endif
}

const Move* history_get_killers(int8_t ply) {
  if (ply >= MAX_HISTORY_PLY)
    return NULL;
  return killers[ply];
}

Move history_get_countermove(const Bitboard* board) {
#if ENABLE_COUNTERMOVES
  Move last_move = board->state->last_move;
  if (last_move == MOVE_NULL)
    return MOVE_NULL;

  assert((!board->to_move) == move_color(last_move));
  return countermoves[!board->to_move][move_piecetype(last_move)]
                     [move_destination_index(last_move)];
#else
  (void)board;
  return MOVE_NULL;
#endif
}

int16_t history_get(Move m) {
  return history[move_color(m)][move_source_index(m)]
                [move_destination_index(m)];
}
