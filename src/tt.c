#include <stdint.h>

#include "move.h"
#include "tt.h"
#include "types.h"

typedef struct {
  uint32_t zobrist_check;
  int8_t depth;
  uint16_t generation;
  int value;
  Move best_move;
  TranspositionType type;
} __attribute__((__packed__)) TranspositionNode;

#define TT_WIDTH 4
#define TT_ENTRIES_EXPONENT 19
#define TT_ENTRIES (1 << TT_ENTRIES_EXPONENT)
static TranspositionNode transposition_table[TT_ENTRIES][TT_WIDTH];

// As is standard for hash tables, we use the lower bits of the zobrist to find
// the right index. On a read, we need to make sure the node we found is
// actually for our position, so we need to store and compare zobrist. However,
// we don't really need the whole thing -- we've used the lower bits for the
// index, so we just need the upper bits to compare. While this does leave a few
// bits in the middle unused, saving the 4 bytes per tt entry is a win.
#define TT_INDEX(zobrist) ((zobrist) & (TT_ENTRIES - 1))
#define TT_ZOBRIST_CHECK(zobrist) ((zobrist) >> 32)

int tt_get_value(uint64_t zobrist, int alpha, int beta, int8_t depth) {
  int index = zobrist % TT_ENTRIES;
  uint32_t zobrist_check = TT_ZOBRIST_CHECK(zobrist);

  if (depth < 1)
    return INFINITY;

  for (int i = 0; i < TT_WIDTH; i++) {
    /* Since many zobrists may map to a single slot in the table, we
    want to make sure we got a match; also, we want to make sure that
    the entry was not made with a shallower depth than what we're
    currently using. */
    TranspositionNode* node = &transposition_table[index][i];
    if (node->zobrist_check == zobrist_check && node->depth >= depth) {
      int val = node->value;

      if (node->type == TRANSPOSITION_EXACT)
        return val;
      else if ((node->type == TRANSPOSITION_ALPHA) && (val <= alpha))
        return val;
      else if ((node->type == TRANSPOSITION_BETA) && (val >= beta))
        return val;
    }
  }

  return INFINITY;
}

Move tt_get_best_move(uint64_t zobrist) {
  int index = zobrist % TT_ENTRIES;
  uint32_t zobrist_check = TT_ZOBRIST_CHECK(zobrist);

  for (int i = 0; i < TT_WIDTH; i++) {
    TranspositionNode* node = &transposition_table[index][i];

    if (node->zobrist_check == zobrist_check)
      return node->best_move;
  }

  return MOVE_NULL;
}

void tt_put(uint64_t zobrist,
            int value,
            Move best_move,
            TranspositionType type,
            uint16_t generation,
            int8_t depth) {
  /* we might not store in the transnposition table if:
      - the depth is not deep enough to be useful
      - the value is dependent upon the ply at which it was searched and the
        depth to which it was searched (currently, only MATE moves)
      - it would replace a deeper search of this node
   */
  if ((depth < 1) || (value >= MATE) || (value <= -MATE))
    return;

  int index = TT_INDEX(zobrist);
  uint32_t zobrist_check = TT_ZOBRIST_CHECK(zobrist);

  TranspositionNode* target = NULL;

  for (int i = 0; i < TT_WIDTH; i++) {
    if (transposition_table[index][i].zobrist_check == zobrist_check) {
      target = &transposition_table[index][i];

      // Don't blow away a best_move if we already have one. Beyond that, you
      // would think that only overwriting if the new data is a bigger
      // generation or a deeper depth (otherwise keeping the original entry)
      // would be good, but every variation I've tried to do that ends up being
      // a big loss for some reason?
      if (best_move == MOVE_NULL)
        best_move = target->best_move;

      break;
    }
  }

  // XXX commenting this out might help too? Is it because it saves another
  // pass/read through the table? (Can we do this and the below check in one
  // pass?) Or because the replacement strategy is bad? Is *that* because
  // generation is updated on every move and not just on "final" moves? (What is
  // Stockfish's replacement strategy?)
  if (!target) {
    int replace_depth = 999;
    for (int i = 0; i < TT_WIDTH; i++) {
      TranspositionNode* node = &transposition_table[index][i];
      if (node->generation != generation && replace_depth > node->depth) {
        replace_depth = node->depth;
        target = node;
      }
    }
  }

  if (!target) {
    int replace_depth = 999;
    for (int i = 0; i < TT_WIDTH; i++) {
      TranspositionNode* node = &transposition_table[index][i];
      if (replace_depth > node->depth) {
        replace_depth = node->depth;
        target = node;
      }
    }
  }

  if (target) {
    target->zobrist_check = zobrist_check;
    target->depth = depth;
    target->generation = generation;
    target->value = value;
    target->best_move = best_move;
    target->type = type;
  }
}
