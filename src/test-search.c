#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitboard.h"
#include "config.h"
#include "move.h"
#include "mt19937.h"
#include "nnue.h"
#include "search.h"
#include "testlib.h"
#include "timer.h"

typedef struct {
  const char* fen;
  const char* move;
} TestCase;

// clang-format off
static const TestCase cases[] = {
    // https://www.chessprogramming.org/Bill_Gosper#HAKMEM70
    {"5B2/6P1/1p6/8/1N6/kP6/2K5/8 w - -", "g7g8n"},

    // https://www.chessprogramming.org/Null_Move_Test-Positions
    {"8/6B1/p5p1/Pp4kp/1P5r/5P1Q/4q1PK/8 w - - 0 32", "h3h4"}, // zugzwang.004

    // https://www.chessprogramming.org/Lasker-Reichhelm_Position
    {"8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - -", "a1b1"},

    // https://www.chessprogramming.org/CCR_One_Hour_Test
    // {"r1bq1rk1/1pp2pbp/p1np1np1/3Pp3/2P1P3/2N1BP2/PP4PP/R1NQKB1R b KQ -", "c6d4"},  // CCR09
    {"r1bqk1nr/pppnbppp/3p4/8/2BNP3/8/PPP2PPP/RNBQK2R w KQkq -", "c4f7"},  // CCR12
    {"r2q1rk1/2p1bppp/p2p1n2/1p2P3/4P1b1/1nP1BN2/PP3PPP/RN1QR1K1 w - -", "e5f6"},  // CCR15

    // https://www.chessprogramming.org/Win_at_Chess
    {"4r1k1/p1qr1p2/2pb1Bp1/1p5p/3P1n1R/1B3P2/PP3PK1/2Q4R w - -", "c1f4"}, // WAC141

    // http://www.talkchess.com/forum3/viewtopic.php?t=44167
    {"8/7k/1R6/R7/8/7P/8/1K6 w - - 98 200", "h3h4"},

    {NULL, NULL},
};
// clang-format on

int main(void) {
  int ret = 0;

  move_init();
#if ENABLE_NNUE
  nnue_init();
#endif
  timer_init_secs(30);
  mt_srandom(0);

  int num_tests = 0;
  Bitboard board;
  for (const TestCase* tcase = cases; tcase->fen != NULL; tcase++) {
    num_tests++;

    State s;
    board_init_with_fen(&board, &s, tcase->fen);

    SearchDebug debug = {0};
    debug.continueOnMate = 1;
    debug.stopMove = tcase->move;
    debug.out = stderr;
    Move m = search_find_move(&board, &debug);

    char buf[6];
    move_srcdest_form(m, buf);
    if (strcmp(buf, tcase->move) == 0) {
      printf("ok %d - %s\n", num_tests, tcase->fen);
    } else {
      printf("not ok %d - %s expect %s got %s\n", num_tests, tcase->fen,
             tcase->move, buf);
      ret = 1;
    }

#if ENABLE_NNUE
    assert(nnue_evaluate(&board) == nnue_debug_evaluate(&board));
#endif
  }

  fprintf(stderr, "%s in %0.2f seconds\n", ret == 0 ? "Completed" : "FAILED",
          test_elapsed_time());

  printf("1..%d\n", num_tests);

  return ret;
}
