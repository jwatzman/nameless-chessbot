#include <stdint.h>
#include <strings.h>

#include "history.h"
#include "move.h"
#include "search.h"
#include "types.h"

static Move killers[max_possible_depth][2];

void history_clear(void) {
  bzero(killers,
        max_possible_depth * 2 * sizeof(Move));  // Assumes MOVE_NULL is 0!
}

void history_update(Move m, int8_t ply) {
  if (move_is_capture(m))
    return;

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
