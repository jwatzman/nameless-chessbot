#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include "bitops.h"
#include "move.h"
#include "types.h"

static const int16_t piece_values[] = {100, 300, 300, 500, 900};

int16_t see_see(const Bitboard* board, Move m) {
  assert(move_is_capture(m));
  assert(board->to_move == move_color(m));

  uint64_t composite = board->full_composite;
  Color to_move = board->to_move;
  const uint8_t dest = move_destination_index(m);
  Piecetype to_capture = move_piecetype(m);
  Piecetype to_be_captured = move_captured_piecetype(m);

  composite &= ~(1ULL << move_source_index(m));
  to_move = !to_move;
  uint64_t attackers = move_generate_attackers(board, to_move, dest, composite);
  if (attackers == 0)
    return piece_values[to_be_captured];

  int16_t minmax_sums[32];
  minmax_sums[0] = piece_values[to_be_captured];
  int i = 1;

  while (attackers) {
    to_be_captured = to_capture;
    if (to_be_captured == KING) {
      // King put themself in check -- can't do that. Put a huge value on it so
      // the minmaxing below won't pick this.
      minmax_sums[i] = SHRT_MAX;
      i++;
      break;
    }

    for (to_capture = PAWN;
         (board->boards[to_move][to_capture] & attackers) == 0; to_capture++) {
      assert(to_capture <= KING);
    }

    assert((size_t)i < sizeof(minmax_sums) / sizeof(int16_t));
    minmax_sums[i] = -minmax_sums[i - 1] + piece_values[to_be_captured];
    i++;

    uint64_t to_capture_board = board->boards[to_move][to_capture] & attackers;
    composite ^= (to_capture_board & -to_capture_board);
    to_move = !to_move;

    // It seems inefficient to recompute this every time around, and I guess it
    // is? It seems it should be better to update incrementally: if we move a
    // piece which can reveal a slider behind (anything but knight and king),
    // then we can recompute movemagic with the new composite and see if there
    // are any discovered attackers. (This does require either two attacker
    // boards for each color, or better a combined composite attacker board we
    // can compare against to_move composite.) But actually implementing that,
    // empirically it's a bit slower (or, at best, equal but much more complex
    // code)? Weird.
    attackers =
        move_generate_attackers(board, to_move, dest, composite) & composite;
  }

  assert(i >= 2);
  for (i--; i > 0; i--)
    minmax_sums[i - 1] = min16(minmax_sums[i - 1], -minmax_sums[i]);

  return minmax_sums[0];
}
