#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "bitboard.h"
#include "move.h"
#include "moveiter.h"
#include "search.h"
#include "statelist.h"
#include "timer.h"

Bitboard* board;

int main(int argc, char** argv) {
  srandom(0);
  move_init();

  if (argc != 3) {
    printf("Usage: ./search-perf depth passes\n");
    return 1;
  }

  Statelist* sl = statelist_alloc();
  board = malloc(sizeof(Bitboard));
  board_init_with_fen(
      board, statelist_new_state(sl),
      "r2q3k/pn2bprp/4pNp1/2p1PbQ1/3p1P2/5NR1/PPP3PP/2B2RK1 b - - 0 1");

  timer_init_secs(180);
  SearchDebug debug = {0};
  debug.maxDepth = (uint8_t)atoi(argv[1]);

  int max_pass = atoi(argv[2]);
  for (int pass = 0; pass < max_pass; pass++) {
    printf("-- PASS %d\n", pass + 1);
    Move best;

    best = search_find_move(board, &debug);
    board_do_move(board, best, statelist_new_state(sl));

    // Invalidate the transposition table, so that we are perft-testing
    // a more complete search every time. Not doing this is fine for
    // corectness, but means the first N-1 ply are probably already cached.
    board->state->zobrist = (uint64_t)random();

    char move_srcdest[6];
    move_srcdest_form(best, move_srcdest);
    printf("-- MOVE %s\n", move_srcdest);
  }

  statelist_free(sl);
  free(board);
  return 0;
}
