#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "bitboard.h"
#include "config.h"
#include "move.h"
#include "search.h"
#include "statelist.h"
#include "timer.h"
#include "types.h"

#define MIN_MOVES 4
#define NUM_RANDOM_MOVES 6
#define USAGE "Usage: nnue-training-data -d depth -g games -o output\n"

extern char* optarg;

int main(int argc, char** argv) {
#if ENABLE_NNUE
  printf("Turn off ENABLE_NNUE.\n");
  return 1;
#endif

  srandom(time(NULL));
  timer_init_secs(9999);
  move_init();

  char* filename = NULL;
  unsigned num_games = 0;
  uint8_t depth = 0;

  int opt;

  while ((opt = getopt(argc, argv, "d:g:o:")) != -1) {
    switch (opt) {
      case 'd':
        depth = (uint8_t)strtoul(optarg, NULL, 0);
        break;
      case 'g':
        num_games = strtoul(optarg, NULL, 0);
        break;
      case 'o':
        filename = strdup(optarg);
        break;
      default:
        printf(USAGE);
        exit(1);
    }
  }

  if (!filename || !num_games || !depth) {
    printf(USAGE);
    exit(1);
  }

  FILE* f = fopen(filename, "a");
  if (!f) {
    printf("Could not open %s\n", filename);
    exit(1);
  }

  setbuf(f, NULL);
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);

  Bitboard board;
  Statelist* sl = statelist_alloc();

  for (unsigned game = 0; game < num_games; game++) {
    printf("%u: ", game);

    int halfmoves = 0;
    statelist_clear(sl);
    board_init(&board, statelist_new_state(sl));

    while (1) {
      int score;
      SearchDebug debug = {0};
      debug.maxDepth = depth;
      debug.score = &score;

      Move best = search_find_move(&board, &debug);

      if (score >= MATE) {
        putchar('+');
        break;
      }

      if (score <= -MATE) {
        putchar('-');
        break;
      }

      if (board.state->halfmove_count == 100) {
        putchar('=');
        putchar('h');
        break;
      }

      if (board.state->halfmove_count >= 4) {
        int reps = 1;
        State* s = board.state;
        for (int back = board.state->halfmove_count - 2; back >= 0; back -= 2) {
          s = s->prev->prev;
          if (s->zobrist == board.state->zobrist)
            reps++;
        }
        if (reps >= 3) {
          putchar('=');
          putchar('r');
          break;
        }
      }

      if (halfmoves > MIN_MOVES && !move_is_capture(best) &&
          !move_is_enpassant(best) && !board_in_check(&board, board.to_move)) {
        board_fen(&board, f);
        fprintf(f, " | %d | 0\n", board.to_move == WHITE ? score : -score);
        putchar('.');
      } else {
        putchar('_');
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

  statelist_free(sl);
  fclose(f);
  free(filename);
}
