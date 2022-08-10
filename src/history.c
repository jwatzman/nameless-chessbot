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
static int16_t countermove_history[2][6][64][6][64];
static int16_t followupmove_history[2][6][64][6][64];

#define HISTORY_ELEM(m) \
  (history[move_color(m)][move_source_index(m)][move_destination_index(m)])
#define CM_HISTORY_ELEM(prev, m)                                        \
  (countermove_history[move_color(m)][move_piecetype(prev)]             \
                      [move_destination_index(prev)][move_piecetype(m)] \
                      [move_destination_index(m)])
#define FM_HISTORY_ELEM(prev, m)                                         \
  (followupmove_history[move_color(m)][move_piecetype(prev)]             \
                       [move_destination_index(prev)][move_piecetype(m)] \
                       [move_destination_index(m)])

static void history_incr_impl(int16_t* p, int8_t depth, int good) {
  int incr = depth * depth;
  int decay = incr * *p / MAX_HISTORY_VALUE;

  if (!good)
    incr = -incr;

  int new = *p + incr - decay;
  assert(new >= SHRT_MIN);
  assert(new <= SHRT_MAX);

  *p = (int16_t) new;
}

static void history_incr(const Bitboard* board,
                         Move best,
                         int8_t depth,
                         int good) {
  history_incr_impl(&HISTORY_ELEM(best), depth, good);

#if ENABLE_COUNTERMOVE_HISTORY
  Move last_move = board->state->last_move;
  if (last_move != MOVE_NULL)
    history_incr_impl(&CM_HISTORY_ELEM(last_move, best), depth, good);

  Move last_move_2 =
      board->state->prev ? board->state->prev->last_move : MOVE_NULL;
  if (last_move_2 != MOVE_NULL)
    history_incr_impl(&FM_HISTORY_ELEM(last_move_2, best), depth, good);
#else
  (void)board;
#endif
}

void history_clear(void) {
  // Assumes MOVE_NULL is 0!
  memset(killers, 0, sizeof(killers));
  memset(countermoves, 0, sizeof(countermoves));

  // XXX should we keep this across searches? Halve every value upon new search?
  memset(history, 0, sizeof(history));
  memset(countermove_history, 0, sizeof(countermove_history));
  memset(followupmove_history, 0, sizeof(followupmove_history));
}

void history_update(const Bitboard* board,
                    Move best,
                    const Move* bad,
                    int num_bad,
                    int8_t depth,
                    int8_t ply) {
  if (move_is_capture(best))
    return;

  if (depth > 2) {
    history_incr(board, best, depth, 1);
    for (int i = 0; i < num_bad; i++)
      history_incr(board, bad[i], depth, 0);
  }

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

  Move last_move = board->state->last_move;
  if (last_move != MOVE_NULL) {
    assert((!board->to_move) == move_color(last_move));
    countermoves[!board->to_move][move_piecetype(last_move)]
                [move_destination_index(last_move)] = best;
  }
}

const Move* history_get_killers(int8_t ply) {
  if (ply >= MAX_HISTORY_PLY)
    return NULL;
  return killers[ply];
}

Move history_get_countermove(const Bitboard* board) {
  Move last_move = board->state->last_move;
  if (last_move == MOVE_NULL)
    return MOVE_NULL;

  assert((!board->to_move) == move_color(last_move));
  return countermoves[!board->to_move][move_piecetype(last_move)]
                     [move_destination_index(last_move)];
}

int16_t history_get_combined(const Bitboard* board, Move m) {
  int h = HISTORY_ELEM(m);

#if ENABLE_COUNTERMOVE_HISTORY
  Move last_move = board->state->last_move;
  if (last_move != MOVE_NULL)
    h += CM_HISTORY_ELEM(last_move, m);

  Move last_move_2 =
      board->state->prev ? board->state->prev->last_move : MOVE_NULL;
  if (last_move_2 != MOVE_NULL)
    h += FM_HISTORY_ELEM(last_move_2, m);
#else
  (void)board;
#endif

  if (h > SHRT_MAX)
    return SHRT_MAX;
  else if (h < SHRT_MIN)
    return SHRT_MIN;

  assert((int16_t)h == h);
  return (int16_t)h;
}

int16_t history_get_uncombined(Move m) {
  return HISTORY_ELEM(m);
}
