#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "bitboard.h"
#include "config.h"
#include "history.h"
#include "move.h"
#include "moveiter.h"
#include "mt19937.h"
#include "nnue.h"
#include "search.h"
#include "statelist.h"
#include "timer.h"

extern int optind;

int main(int argc, char** argv) {
  mt_srandom(0);
  move_init();
#if ENABLE_NNUE
  nnue_init();
#endif

  int keep_table = 0;
  int c;
  while ((c = getopt(argc, argv, "k")) != -1) {
    switch (c) {
      case 'k':
        keep_table = 1;
        break;
      default:
        abort();
    }
  }

  argc -= optind;
  argv += optind;

  if (argc != 2) {
    printf("Usage: ./search-perf [-k] depth passes\n");
    return 1;
  }

  Statelist* sl = statelist_alloc();
  Bitboard* board = malloc(sizeof(Bitboard));
  board_init_with_fen(
      board, statelist_new_state(sl),
      "r2q3k/pn2bprp/4pNp1/2p1PbQ1/3p1P2/5NR1/PPP3PP/2B2RK1 b - - 0 1");

  history_clear();
  timer_init_secs(180);
  SearchDebug debug = {0};
  debug.maxDepth = (uint8_t)atoi(argv[0]);

  int max_pass = atoi(argv[1]);
  for (int pass = 0; pass < max_pass; pass++) {
    printf("-- PASS %d\n", pass + 1);
    Move best;

    best = search_find_move(board, &debug);
    board_do_move(board, best, statelist_new_state(sl));

    if (!keep_table) {
      // Invalidate the transposition table, so that we are perft-testing
      // a more complete search every time. Not doing this is fine for
      // corectness, but means the first N-1 ply are probably already cached. We
      // don't bother to clear history, since that doesn't cause a degenerate
      // search as easily.
      board->state->zobrist = (uint64_t)mt_random();
    }

    char move_srcdest[6];
    move_srcdest_form(best, move_srcdest);
    printf("-- MOVE %s\n", move_srcdest);
  }

  statelist_free(sl);
  free(board);
  return 0;
}
