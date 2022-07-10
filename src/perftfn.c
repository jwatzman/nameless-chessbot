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
  move_generate_movelist(board, &moves, MOVE_GEN_ALL);

  uint64_t nodes = 0;
  for (int i = 0; i < moves.n; i++) {
    Move m = moves.moves[i];
    State s;

    if (!move_is_legal(board, m))
      continue;

    if (depth == 1) {
      nodes += 1;
    } else {
      board_do_move(board, m, &s);
      nodes += perft(board, depth - 1);
      board_undo_move(board, m);
    }
  }

  return nodes;
}
