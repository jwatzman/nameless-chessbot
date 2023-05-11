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
#include "search.h"
#include "statelist.h"
#include "timer.h"
#include "types.h"

static Move get_human_move(Bitboard* board, const Movelist* moves);
static Move get_computer_move(Bitboard* board);

int main(void) {
  mt_srandom((unsigned)time(NULL));
  timer_init_secs(5);
  move_init();
  search_init();
#if ENABLE_NNUE
  nnue_init();
#endif

  Statelist* sl = statelist_alloc();
  Bitboard test;
  board_init(&test, statelist_new_state(sl));

  while (1) {
    board_print(&test);
#if ENABLE_NNUE
    printf("Traditional eval: %i\nNNUE eval: %i\n", evaluate_traditional(&test),
           nnue_evaluate(&test));
#else
    printf("Evaluation: %i\n", evaluate_board(test));
#endif

    Movelist moves;
    move_generate_movelist(&test, &moves, MOVE_GEN_ALL);

    int num_legal_moves = 0;
    for (int i = 0; i < moves.n; i++) {
      Move move = moves.moves[i];
      State s_tmp;
      board_do_move(&test, move, &s_tmp);
      if (!board_in_check(&test, 1 - test.to_move))
        num_legal_moves++;
      board_undo_move(&test);
    }

    if (num_legal_moves == 0) {
      if (board_in_check(&test, test.to_move))
        printf("CHECKMATE!\n");
      else
        printf("STALEMATE!\n");

      break;
    }

    if (test.state->halfmove_count == 50) {
      printf("DRAW BY 50 MOVE RULE\n");
      break;
    }

    Move next_move;
    if (test.to_move == WHITE) {
      next_move = get_human_move(&test, &moves);
      if (next_move == MOVE_NULL)
        break;
    } else {
      next_move = get_computer_move(&test);
    }

    board_do_move(&test, next_move, statelist_new_state(sl));
  }

  statelist_free(sl);
  return 0;
}

static Move get_human_move(Bitboard* board, const Movelist* moves) {
  char srcdest_form[6];
  char input_move[6];

  Move result = 0;

  State s;

  for (int i = 0; i < moves->n; i++) {
    Move move = moves->moves[i];
    board_do_move(board, move, &s);
    if (!board_in_check(board, 1 - board->to_move)) {
      move_srcdest_form(move, srcdest_form);
      printf("%s ", srcdest_form);
    }
    board_undo_move(board);
  }
  printf("\n");

  while (!result) {
    if (scanf("%5s", input_move) == EOF)
      return MOVE_NULL;

    for (int i = 0; i < moves->n; i++) {
      Move move = moves->moves[i];
      move_srcdest_form(move, srcdest_form);
      if (!strcmp(input_move, srcdest_form)) {
        board_do_move(board, move, &s);
        if (board_in_check(board, 1 - board->to_move))
          printf("Can't leave king in check\n");
        else
          result = move;

        board_undo_move(board);
        break;
      }
    }
  }

  return result;
}

static Move get_computer_move(Bitboard* board) {
  return search_find_move(board, NULL);
}
