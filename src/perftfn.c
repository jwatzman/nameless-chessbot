#include <assert.h>
#include <inttypes.h>

#include "bitboard.h"
#include "move.h"
#include "moveiter.h"
#include "nnue.h"
#include "perftfn.h"

uint64_t perft(Bitboard* board, int depth) {
#if ENABLE_NNUE
  assert(nnue_evaluate(board) == nnue_debug_evaluate(board));
#endif

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
#ifndef NDEBUG
      int gives_check = move_gives_check(board, m);
#endif
      board_do_move(board, m, &s);
      assert(gives_check == board_in_check(board, board->to_move));
      nodes += perft(board, depth - 1);
      board_undo_move(board, m);
    }
  }

  return nodes;
}
