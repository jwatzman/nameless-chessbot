#include <assert.h>
#include <stdint.h>

#include "config.h"
#include "move.h"
#include "tt.h"
#include "types.h"

struct TranspositionNode {
  uint32_t zobrist_check;
  Move best_move;
  int value;
  int8_t depth;
  uint16_t generation;
} __attribute__((__packed__));

#define TT_WIDTH 4
#define TT_ENTRIES_EXPONENT 20
#define TT_ENTRIES (1 << TT_ENTRIES_EXPONENT)
static TranspositionNode transposition_table[TT_ENTRIES][TT_WIDTH];

#define INJECT_TYPE(move, type) ((move) | (Move)((type) << move_unused_offset))
#define EXTRACT_TYPE(move) (((move) >> move_unused_offset) & 0xff)
#define CLEAN_MOVE(move) ((move)&0x00ffffff)

// As is standard for hash tables, we use the lower bits of the zobrist to find
// the right index. On a read, we need to make sure the node we found is
// actually for our position, so we need to store and compare zobrist. However,
// we don't really need the whole thing -- we've used the lower bits for the
// index, so we just need the upper bits to compare. While this does leave a few
// bits in the middle unused, saving the 4 bytes per tt entry is a win.
#define TT_INDEX(zobrist) ((zobrist) & (TT_ENTRIES - 1))
#define TT_ZOBRIST_CHECK(zobrist) ((zobrist) >> 32)

const TranspositionNode* tt_get(uint64_t zobrist) {
  int index = zobrist % TT_ENTRIES;
  uint32_t zobrist_check = TT_ZOBRIST_CHECK(zobrist);

  for (int i = 0; i < TT_WIDTH; i++)
    __builtin_prefetch(&transposition_table[index][i]);

  for (int i = 0; i < TT_WIDTH; i++) {
    TranspositionNode* node = &transposition_table[index][i];
    if (node->zobrist_check == zobrist_check) {
      return node;
    }
  }

  return NULL;
}

int tt_value(const TranspositionNode* n) {
  assert(n);
  return n->value;
}

Move tt_move(const TranspositionNode* n) {
  assert(n);
  return CLEAN_MOVE(n->best_move);
}

TranspositionType tt_type(const TranspositionNode* n) {
  assert(n);
  return EXTRACT_TYPE(n->best_move);
}

int8_t tt_depth(const TranspositionNode* n) {
  assert(n);
  return n->depth;
}

void tt_put(uint64_t zobrist,
            int value,
            Move best_move,
            TranspositionType type,
            uint16_t generation,
            int8_t depth) {
  // XXX why is this here, shouldn't the table work for quiescent search too?
  if (depth < 1)
    return;

  // An incredibly hacky way of trying to deal with the GHI problem: don't store
  // path-dependent scores in the table. (Swap the value/type to a pair that
  // will never be used, but keep the best move.) This doesn't actually solve
  // the problem, especially for draws, since other evaluations can be based on
  // a false draw value. But it helps and is straightforward.
  // XXX try multiplying the eval by <100% as we get closer to 50 move rule (so
  // scales down smoothly).
  // XXX try distinguishing between reps in a search (0 + 2 = draw) and reps in
  // the actual game continuation (1 + 2 = draw).
  if (value >= MATE || value <= -MATE || value == DRAW) {
    type = TRANSPOSITION_ALPHA;
    value = NFINITY;
  }

  int index = TT_INDEX(zobrist);
  uint32_t zobrist_check = TT_ZOBRIST_CHECK(zobrist);

  for (int i = 0; i < TT_WIDTH; i++)
    __builtin_prefetch(&transposition_table[index][i], 1);

  TranspositionNode* target = NULL;

  for (int i = 0; i < TT_WIDTH; i++) {
    if (transposition_table[index][i].zobrist_check == zobrist_check) {
      target = &transposition_table[index][i];

      // Don't blow away a best_move if we already have one. Beyond that, you
      // would think that only overwriting if the new data is a bigger
      // generation or a deeper depth (otherwise keeping the original entry)
      // would be good, but every variation I've tried to do that ends up
      // being a big loss for some reason?
      if (best_move == MOVE_NULL)
        best_move = CLEAN_MOVE(target->best_move);

      break;
    }
  }

  // XXX commenting this out might help too? Is it because it saves another
  // pass/read through the table? (Can we do this and the below check in one
  // pass?) Or because the replacement strategy is bad? Is *that* because
  // generation is updated on every move and not just on "final" moves? (What
  // is Stockfish's replacement strategy?)
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
    target->best_move = INJECT_TYPE(best_move, type);
  }
}
