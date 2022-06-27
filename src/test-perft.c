#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/time.h>

#include "bitboard.h"
#include "move.h"
#include "perftfn.h"

typedef struct {
  const char* fen;
  int depth;
  uint64_t nodes;
} TestCase;

// clang-format off
static const TestCase cases[] = {
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 6, 119060324},

    // https://www.chessprogramming.org/Perft_Results
    {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 5, 193690690},
    {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 6, 11030083},
    {"r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", 5, 15833292},
    {"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 4, 2103487},

    // https://gist.github.com/peterellisjones/8c46c28141c162d1d8a0f0badbc9cff9
    {"r6r/1b2k1bq/8/8/7B/8/8/R3K2R b KQ - 3 2", 1, 8},
    {"8/8/8/2k5/2pP4/8/B7/4K3 b - d3 0 3", 1, 8},
    {"r1bqkbnr/pppppppp/n7/8/8/P7/1PPPPPPP/RNBQKBNR w KQkq - 2 2", 1, 19},
    {"r3k2r/p1pp1pb1/bn2Qnp1/2qPN3/1p2P3/2N5/PPPBBPPP/R3K2R b KQkq - 3 2", 1, 5},
    {"2kr3r/p1ppqpb1/bn2Qnp1/3PN3/1p2P3/2N5/PPPBBPPP/R3K2R b KQ - 3 2", 1, 44},
    {"rnb2k1r/pp1Pbppp/2p5/q7/2B5/8/PPPQNnPP/RNB1K2R w KQ - 3 9", 1, 39},
    {"2r5/3pk3/8/2P5/8/2K5/8/8 w - - 5 4", 1, 9},
    {"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 3, 62379},
    {"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 3, 89890},
    {"3k4/3p4/8/K1P4r/8/8/8/8 b - - 0 1", 6, 1134888},
    {"8/8/4k3/8/2p5/8/B2P2K1/8 w - - 0 1", 6, 1015133},
    {"8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 0 1", 6, 1440467},
    {"5k2/8/8/8/8/8/8/4K2R w K - 0 1", 6, 661072},
    {"3k4/8/8/8/8/8/8/R3K3 w Q - 0 1", 6, 803711},
    {"r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq - 0 1", 4, 1274206},
    {"r3k2r/8/3Q4/8/8/5q2/8/R3K2R b KQkq - 0 1", 4, 1720476},
    {"2K2r2/4P3/8/8/8/8/8/3k4 w - - 0 1", 6, 3821001},
    {"8/8/1P2K3/8/2n5/1q6/8/5k2 b - - 0 1", 5, 1004658},
    {"4k3/1P6/8/8/8/8/K7/8 w - - 0 1", 6, 217342},
    {"8/P1k5/K7/8/8/8/8/8 w - - 0 1", 6, 92683},
    {"K1k5/8/P7/8/8/8/8/8 w - - 0 1", 6, 2217},
    {"8/k1P5/8/1K6/8/8/8/8 w - - 0 1", 7, 567584},
    {"8/8/2k5/5q2/5n2/8/5K2/8 b - - 0 1", 4, 23527},

    // https://github.com/peterellisjones/rust_move_gen/blob/master/src/gen/mod.rs
    {"5k2/8/8/q7/8/2Q5/8/4K3 w - -", 1, 8},
    {"5k2/5pb1/5Q1B/8/8/8/8/4K3 b - - 1 1", 1, 3},
    {"4k3/8/5r2/8/8/8/4PPpP/5K2 w - - 0 1", 1, 3},
    {"4k3/3pq3/4Q3/1B2N3/1p2P3/2N5/PPPB1PP1/R3K2R b KQ - 0 1", 1, 5},
    {"8/8/8/6k1/4Pp2/8/8/K1B5 b - e3 0 1", 1, 7},
    {"8/8/8/8/R3Ppk1/8/8/K7 b - e3 0 1", 1, 7},
    {"4k3/3NqQ2/8/8/8/8/4p3/4K3 b - - 0 1", 1, 4},
    {"8/8/5k2/8/4Pp2/8/8/4KR2 b - e3 0 1", 1, 8},

    {NULL, 0, 0},
};
// clang-format on

int main(void) {
  int ret = 0;

  move_init();
  srandom(0);

  int num_tests = 0;
  Bitboard board;
  for (const TestCase* tcase = cases; tcase->fen != NULL; tcase++) {
    num_tests++;

    State s;
    board_init_with_fen(&board, &s, tcase->fen);
    uint64_t zobrist = board.state->zobrist;
    uint64_t nodes = perft(&board, tcase->depth);

    if (nodes != tcase->nodes) {
      printf("not ok %d - %s depth %d expected %" PRIu64 " got %" PRIu64 "\n",
             num_tests, tcase->fen, tcase->depth, tcase->nodes, nodes);
      ret = 1;
    } else if (zobrist != board.state->zobrist) {
      printf("not ok %d - %s depth %d zobrist mismatch\n", num_tests,
             tcase->fen, tcase->depth);
      ret = 1;
    } else {
      printf("ok - %s depth %d\n", tcase->fen, tcase->depth);
    }
  }

  struct rusage usage;
  getrusage(RUSAGE_SELF, &usage);
  double elapsed_time =
      (double)(usage.ru_utime.tv_sec + usage.ru_stime.tv_sec) +
      (1.0e-6) * (usage.ru_utime.tv_usec + usage.ru_stime.tv_usec);
  fprintf(stderr, "%s in %0.2f seconds\n", ret == 0 ? "Completed" : "FAILED",
          elapsed_time);

  return ret;
}
