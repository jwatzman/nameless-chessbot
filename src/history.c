#include <stdint.h>
#include <strings.h>

#include "history.h"
#include "move.h"
#include "search.h"
#include "types.h"

static Move killers[max_possible_depth][2];
static uint16_t history[64][64];

void history_clear(void) {
  bzero(killers,
        max_possible_depth * 2 * sizeof(Move));  // Assumes MOVE_NULL is 0!

  // XXX should we keep this across searches? Halve every value upon new search?
  bzero(history, 64 * 64 * sizeof(uint16_t));
}

void history_update(Move m, int8_t ply) {
  if (move_is_capture(m))
    return;

  // XXX this should probably increment by depth*2 or depth << 2 or something,
  // and only record for depth > 2 or something.
  history[move_source_index(m)][move_destination_index(m)]++;

  Move* slot = (Move*)history_get_killers(ply);

  if (slot[0] == m || slot[1] == m)
    return;

  if (slot[0] == MOVE_NULL) {
    slot[0] = m;
  } else {
    slot[1] = slot[0];
    slot[0] = m;
  }
}

const Move* history_get_killers(int8_t ply) {
  return killers[ply - 1];  // Ply starts at 1
}

uint16_t history_get(Move m) {
  return history[move_source_index(m)][move_destination_index(m)];
}
