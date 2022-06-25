#include <inttypes.h>

#include "perftfn.h"

#include "bitboard.h"
#include "move.h"
#include "moveiter.h"

uint64_t perft(Bitboard* board, int depth) {
  if (depth == 0) {
    return 1;
  }

  Movelist moves;
  move_generate_movelist(board, &moves);

  Moveiter iter;
  moveiter_init(&iter, &moves, MOVEITER_SORT_NONE, MOVE_NULL);

  uint64_t nodes = 0;
  while (moveiter_has_next(&iter)) {
    Move m = moveiter_next(&iter);
    State s;
    board_do_move(board, m, &s);

    if (!board_in_check(board, 1 - board->to_move))
      nodes += perft(board, depth - 1);

    board_undo_move(board, m);
  }

  return nodes;
}
