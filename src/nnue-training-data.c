#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "bitboard.h"
#include "config.h"
#include "move.h"
#include "search.h"
#include "statelist.h"
#include "timer.h"
#include "types.h"

#define NUM_RANDOM_MOVES 6
#define FIXED_DEPTH 5

int main(void) {
#if ENABLE_NNUE
  printf("Turn off ENABLE_NNUE.\n");
  return 1;
#endif
  srandom(time(NULL));
  timer_init_secs(9999);
  move_init();

  FILE* f = fopen("training.txt", "a");
  if (!f)
    abort();
  setbuf(f, NULL);
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);

  Bitboard board;
  Statelist* sl = statelist_alloc();

  while (1) {
    int halfmoves = 0;
    statelist_clear(sl);
    board_init(&board, statelist_new_state(sl));

    while (1) {
      int score;
      SearchDebug debug = {0};
      debug.maxDepth = FIXED_DEPTH;
      debug.score = &score;

      Move best = search_find_move(&board, &debug);

      if (score > MATE || score < -MATE || score == DRAW) {
        if (score > MATE)
          putchar('+');
        if (score < -MATE)
          putchar('-');
        if (score == DRAW)
          putchar('=');
        break;
      }

      if (halfmoves > 0) {
        board_fen(&board, f);
        fprintf(f, " | %d | 0\n", board.to_move == WHITE ? score : -score);
        putchar('.');
      }

      if (halfmoves < NUM_RANDOM_MOVES) {
        Movelist ml;
        move_generate_movelist(&board, &ml, MOVE_GEN_ALL);

        int i = random() % ml.n;
        while (!move_is_legal(&board, ml.moves[i])) {
          i++;
          i %= ml.n;
        }

        board_do_move(&board, ml.moves[i], statelist_new_state(sl));
      } else {
        board_do_move(&board, best, statelist_new_state(sl));
      }

      halfmoves++;
    }

    putchar('\n');
  }
}
