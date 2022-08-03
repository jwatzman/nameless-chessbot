#include <stdint.h>
#include <string.h>
#include <strings.h>

#include "assert.h"
#include "config.h"
#include "history.h"
#include "move.h"
#include "search.h"
#include "types.h"

#define MAX_HISTORY_PLY MAX_POSSIBLE_DEPTH

static Move killers[MAX_HISTORY_PLY][2];
static uint16_t history[64][64];

void history_clear(void) {
  bzero(killers, sizeof(killers));  // Assumes MOVE_NULL is 0!
  bzero(history, sizeof(history));
}

void history_next_search(void) {
#if ENABLE_HISTORY
  for (int i = 0; i < 64; i++)
    for (int j = 0; j < 64; j++)
      history[i][j] /= 2;
#endif

#if ENABLE_KILLERS
  memmove(killers, &killers[2][0], sizeof(killers) - 2 * sizeof(killers[0]));
#endif
}

void history_update(Move m, int8_t depth, int8_t ply) {
  if (move_is_capture(m))
    return;

#if ENABLE_HISTORY
  if (depth > 2)
    history[move_source_index(m)][move_destination_index(m)] += depth * 2;
#endif

#if ENABLE_KILLERS
  if (ply - 1 < MAX_HISTORY_PLY) {
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
}

const Move* history_get_killers(int8_t ply) {
  if (ply - 1 >= MAX_HISTORY_PLY)
    return NULL;
  return killers[ply - 1];  // Ply starts at 1
}

uint16_t history_get(Move m) {
  return history[move_source_index(m)][move_destination_index(m)];
}
