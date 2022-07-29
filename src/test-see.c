#include <stdio.h>
#include <string.h>

#include "bitboard.h"
#include "evaluate.h"
#include "move.h"
#include "mt19937.h"
#include "testlib.h"
#include "types.h"

typedef struct {
  const char* fen;
  const char* movestr;
  int16_t see;
} TestCase;

// clang-format off
static const TestCase cases[] = {
  // http://www.open-aurec.com/wbforum/viewtopic.php?t=6104#p29316
  {"6k1/7p/8/7R/4B3/8/7r/4K3 w - - 0 1", "e4h7", 42}, // XXX what should this be?
  {"6k1/7p/8/7r/4B3/8/7R/4K3 w - - 0 1", "e4h7", -200},

  {NULL, 0, 0},
};
// clang-format on

int main(void) {
  int ret = 0;

  move_init();
  mt_srandom(0);

  int num_tests = 0;
  Bitboard board;
  for (const TestCase* tcase = cases; tcase->fen != NULL; tcase++) {
    num_tests++;

    State s;
    board_init_with_fen(&board, &s, tcase->fen);

    Movelist list;
    move_generate_movelist(&board, &list, MOVE_GEN_QUIET);

    Move m = MOVE_NULL;
    char buf[6];
    for (int i = 0; i < list.n; i++) {
      move_srcdest_form(list.moves[i], buf);
      if (!strcmp(buf, tcase->movestr)) {
        m = list.moves[i];
        break;
      }
    }

    if (m == MOVE_NULL) {
      printf("not ok %d - %s invalid move %s\n", num_tests, tcase->fen,
             tcase->movestr);
      ret = 1;
      continue;
    }

    int16_t see = evaluate_see(&board, m);
    if (see != tcase->see) {
      printf("not ok %d - %s move %s expected %d got %d\n", num_tests,
             tcase->fen, tcase->movestr, tcase->see, see);
      ret = 1;
      continue;
    }

    printf("ok - %s move %s\n", tcase->fen, tcase->movestr);
  }

  fprintf(stderr, "%s in %0.2f seconds\n", ret == 0 ? "Completed" : "FAILED",
          test_elapsed_time());

  return ret;
}
