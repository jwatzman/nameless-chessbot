#include <stdint.h>
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
  bzero(killers,
        MAX_HISTORY_PLY * 2 * sizeof(Move));  // Assumes MOVE_NULL is 0!

  // XXX should we keep this across searches? Halve every value upon new search?
  bzero(history, 64 * 64 * sizeof(uint16_t));
}

void history_update(Move m, int8_t ply) {
  if (move_is_capture(m))
    return;

#if ENABLE_HISTORY
  // XXX this should probably increment by depth*2 or depth << 2 or something,
  // and only record for depth > 2 or something.
  history[move_source_index(m)][move_destination_index(m)]++;
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
