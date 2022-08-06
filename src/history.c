#include <stdint.h>
#include <string.h>

#include "assert.h"
#include "config.h"
#include "history.h"
#include "move.h"
#include "search.h"
#include "types.h"

#define MAX_HISTORY_PLY MAX_POSSIBLE_DEPTH

static Move killers[MAX_HISTORY_PLY][2];
static Move countermoves[2][6][64];
static uint16_t history[64][64];

void history_clear(void) {
  // Assumes MOVE_NULL is 0!
  memset(killers, 0, sizeof(killers));
  memset(countermoves, 0, sizeof(countermoves));

  // XXX should we keep this across searches? Halve every value upon new search?
  memset(history, 0, sizeof(history));
}

void history_update(const Bitboard* board, Move m, int8_t depth, int8_t ply) {
  if (move_is_capture(m))
    return;

#if ENABLE_HISTORY
  if (depth > 2)
    history[move_source_index(m)][move_destination_index(m)] += depth * 2;
#endif

#if ENABLE_KILLERS
  if (ply < MAX_HISTORY_PLY) {
    Move* slot = (Move*)history_get_killers(ply);
    assert(slot);

    if (slot[0] == m || slot[1] == m)
      return;

    if (slot[0] == MOVE_NULL) {
      slot[0] = m;
    } else {
      slot[1] = slot[0];
      slot[0] = m;
    }
  }
#endif

#if ENABLE_COUNTERMOVES
  Move last_move = board->state->last_move;
  if (last_move != MOVE_NULL) {
    assert((!board->to_move) == move_color(last_move));
    countermoves[!board->to_move][move_piecetype(last_move)]
                [move_destination_index(last_move)] = m;
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

uint16_t history_get(Move m) {
  return history[move_source_index(m)][move_destination_index(m)];
}
