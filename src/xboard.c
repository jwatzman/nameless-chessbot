#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "bitboard.h"
#include "config.h"
#include "evaluate.h"
#include "move.h"
#include "moveiter.h"
#include "mt19937.h"
#include "nnue.h"
#include "perftfn.h"
#include "search.h"
#include "statelist.h"
#include "timer.h"
#include "types.h"

static const int max_input_length = 1024;

// returns 0 on illegal move
static Move parse_move(Bitboard* board, char* possible_move) {
  Movelist moves;
  move_generate_movelist(board, &moves, MOVE_GEN_ALL);
  char test[6];

  for (int i = 0; i < moves.n; i++) {
    Move m = moves.moves[i];
    move_srcdest_form(m, test);
    if (!move_is_promotion(m) && !strncmp(test, possible_move, 4))
      return m;
    else if (!strncmp(test, possible_move, 5))
      return m;
  }

  return 0;
}

int main(int argc, char** argv) {
  Color computer_player =
      (Color)-1;  // not WHITE or BLACK if we don't play either (e.g. -1)
  int game_on = 0;
  Statelist* sl = statelist_alloc();
  Bitboard board;
  char* input = malloc(sizeof(char) * max_input_length);
  Move last_move = 0;
  int got_move = 0;

  setbuf(stderr, NULL);
  setbuf(stdout, NULL);

  mt_srandom((unsigned)time(NULL));
  move_init();
  search_init();
#if ENABLE_NNUE
  nnue_init();
#endif

  if (argc == 2 && !strcmp(argv[1], "bench")) {
    mt_srandom(0);
    timer_init_secs(9999);
    time_t start_cs = timer_get_centiseconds();
    uint64_t nodes = search_benchmark();
    unsigned int duration_cs =
        (unsigned int)(timer_get_centiseconds() - start_cs);

    printf("%" PRIu64 " nodes\n%" PRIu64 " nps\n", nodes,
           nodes * 100 / duration_cs);
    return 0;
  }

  while (1) {
    if (game_on && got_move) {
      board_do_move(&board, last_move, statelist_new_state(sl));
      got_move = 0;
    }

    if (game_on && computer_player == board.to_move) {
      last_move = search_find_move(&board, NULL);
      move_srcdest_form(last_move, input);
      printf("move %s\n", input);
      board_do_move(&board, last_move, statelist_new_state(sl));
    }

    if (fgets(input, max_input_length, stdin) == NULL)
      break;

    fprintf(stderr, "%s", input);

    if (!strcmp("xboard\n", input)) {
      printf(
          "feature colors=0 setboard=1 time=0 sigint=0 sigterm=0 "
          "variants=\"normal\" myname=\"nameless\" done=1\n");
    } else if (!strcmp("new\n", input)) {
      statelist_clear(sl);
      board_init(&board, statelist_new_state(sl));
      computer_player = BLACK;
      game_on = 1;
    } else if (!strcmp("quit\n", input)) {
      break;
    } else if (!strcmp("force\n", input)) {
      computer_player = (Color)-1;
    } else if (!strcmp("go\n", input)) {
      computer_player = board.to_move;
    } else if (!strncmp("setboard ", input, 9)) {
      statelist_clear(sl);
      board_init_with_fen(&board, statelist_new_state(sl), input + 9);
    } else if (!strncmp("result", input, 6)) {
      computer_player = (Color)-1;
      game_on = 0;
    } else if (!strncmp("level ", input, 6)) {
      timer_init_xboard(input);
    } else if (!strcmp("_print\n", input) || !strncmp("_perft ", input, 7)) {
      int perft_depth = input[2] == 'e' ? strtol(input + 7, NULL, 10) - 1 : -1;
      uint64_t perft_tot = 0;

      board_print(&board);
#if ENABLE_NNUE
      int nnue = nnue_evaluate(&board);
      printf("Traditional eval: %i\nNNUE eval: %i\n",
             evaluate_traditional(&board), nnue);
      assert(nnue == nnue_debug_evaluate(&board));
#else
      printf("Evaluation: %i\n", evaluate_board(board));
#endif
      puts("Pseudolegal moves: ");

      Movelist all_moves;
      move_generate_movelist(&board, &all_moves, MOVE_GEN_ALL);

      char srcdest_form[6];
      for (int i = 0; i < all_moves.n; i++) {
        Move m = all_moves.moves[i];
        move_srcdest_form(m, srcdest_form);
        printf("%s ", srcdest_form);

        if (perft_depth >= 0) {
          State s;
          if (!move_is_legal(&board, m)) {
            printf("(illegal)\n");
            continue;
          }

          board_do_move(&board, m, &s);
          uint64_t p = perft(&board, perft_depth);
          printf("%" PRIu64 "\n", p);
          perft_tot += p;
          board_undo_move(&board);
        }
      }

      if (perft_depth >= 0)
        printf("Total: %" PRIu64 "\n", perft_tot);
      else
        printf("\n");
    } else if (!strcmp("_searchonly\n", input)) {
      search_find_move(&board, NULL);
    } else if (input[0] >= 'a' && input[0] <= 'h' && input[1] >= '1' &&
               input[1] <= '8' && input[2] >= 'a' && input[2] <= 'h' &&
               input[3] >= '1' && input[3] <= '8') {
      last_move = parse_move(&board, input);
      if (!last_move)
        printf("Illegal move: %s", input);
      else
        got_move = 1;
    } else {
      printf("Error (unknown command): %s", input);
    }
  }

  statelist_free(sl);
  free(input);
  return 0;
}
