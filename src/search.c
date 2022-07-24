#include "search.h"

#include <assert.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "bitops.h"
#include "config.h"
#include "evaluate.h"
#include "history.h"
#include "move.h"
#include "moveiter.h"
#include "timer.h"
#include "tt.h"

#define max_quiescent_depth 50
#define aspiration_window 30

#define DISALLOW_NULL_MOVE 0
#define ALLOW_NULL_MOVE 1

#if ENABLE_FUTILITY_DEPTH > 0
static const int futility_margins[] = {0, 75, 500};
#endif

#if ENABLE_REVERSE_FUTILITY_DEPTH > 0
static const int reverse_futility_margins[] = {0, 300, 500};
#endif

static int timeup;
static uint64_t nodes_searched;

// main search workhorse
static int search_alpha_beta(Bitboard* board,
                             int alpha,
                             int beta,
                             int8_t depth,
                             int8_t ply,
                             Move* pv,
                             uint8_t allow_null);

static void search_print_pv(Move* pv, int8_t depth);

Move search_find_move(Bitboard* board, const SearchDebug* debug) {
  Move best_move = 0;
  nodes_searched = 0;
  history_clear();

  Move pv[max_possible_depth + 1];

  time_t start_cs = timer_get_centiseconds();

  timeup = 0;
  timer_begin();

  int alpha = -INFINITY;
  int beta = INFINITY;

  // for each depth, call the main workhorse, search_alpha_beta
  uint8_t max_depth = debug && debug->maxDepth > 0
                          ? min(debug->maxDepth, max_possible_depth)
                          : max_possible_depth;
  for (int8_t depth = 1; depth <= max_depth; depth++) {
    // here we go...
    int val =
        search_alpha_beta(board, alpha, beta, depth, 1, pv, ALLOW_NULL_MOVE);

    time_t centiseconds_taken = timer_get_centiseconds() - start_cs;

    if (((val <= alpha) || (val >= beta)) && !timeup) {
      // aspiration window failure
      fprintf(stderr, "%i\t%i\t%lu\t%" PRIu64 "\taspiration failure\n", depth,
              board->to_move == WHITE ? val : -val, centiseconds_taken,
              nodes_searched);
      alpha = -INFINITY;
      beta = INFINITY;
      depth--;
      continue;
    }

    if (!timeup) {
      best_move = pv[0];
      if (debug && debug->score)
        *debug->score = val;

      fprintf(stderr, "%i\t%i\t%lu\t%" PRIu64 "\t", depth,
              board->to_move == WHITE ? val : -val, centiseconds_taken,
              nodes_searched);
      search_print_pv(pv, depth);

      alpha = val - aspiration_window;
      beta = val + aspiration_window;

      if ((val >= MATE) || (val <= -MATE)) {
        fprintf(stderr, "-> mate");

        if (!(debug && debug->continueOnMate)) {
          fprintf(stderr, "\n");
          break;
        }
      }

      fprintf(stderr, "\n");

      if (debug && debug->stopMove) {
        char buf[6];
        move_srcdest_form(best_move, buf);

        int eval = evaluate_board(board);
        if (val > eval + 50 && !strcmp(buf, debug->stopMove)) {
          fprintf(stderr, "Found stop-move.\n");
          break;
        }
      }
    } else {
      fprintf(stderr, "%i\t%i\t%lu\t%" PRIu64 "\ttimeup\n", depth, 0,
              centiseconds_taken, nodes_searched);
      break;
    }
  }

  timer_end();

  board->generation++;
  return best_move;
}

static int search_alpha_beta(Bitboard* board,
                             int alpha,
                             int beta,
                             int8_t depth,
                             int8_t ply,
                             Move* pv,
                             uint8_t allow_null) {
  Move localpv[max_possible_depth + 1];

  if (!timeup)
    timeup = timer_timeup();

  if (timeup)
    return 0;

  nodes_searched++;

  TranspositionType type = TRANSPOSITION_ALPHA;

  int quiescent = depth <= 0;
  int pv_node = beta > alpha + 1;
  int in_check = board_in_check(board, board->to_move);

  Move best_move = MOVE_NULL;
  int best_score = -INFINITY;

  assert(alpha < beta);

  // 50-move rule
  if (board->state->halfmove_count == 100)
    return DRAW;

  /* only check for repetitions down at least 1 ply, since it results in a
     search termination without the game actually being over. Similarly, only
     check the transposition table two at least 1 ply */
  if (ply > 1) {
    // 3 repetition rule
    // XXX is this right?
    State* s = board->state;
    for (int back = board->state->halfmove_count - 2; back >= 0; back -= 2) {
      s = s->prev;
      if (!s)
        break;
      s = s->prev;
      if (!s)
        break;
      if (s->zobrist == board->state->zobrist)
        return DRAW;
    }

    // check transposition table for a useful value
    int table_val = tt_get_value(board->state->zobrist, alpha, beta, depth);

    if (table_val != INFINITY) {
      if (pv) {
        pv[0] = tt_get_best_move(board->state->zobrist);
        pv[1] = MOVE_NULL;  // We don't store pv in the table.
      }
      return table_val;
    }
  }

  // leaf node
  if (depth <= -max_quiescent_depth) {
    int eval = evaluate_board(board);
    return eval;
  }

  // Quiescent stand-pat: "you don't have to take". Disallow when in check
  // since the position isn't "quiet" yet and there's no option to "do nothing".
  if (quiescent && !in_check) {
    pv = NULL;

    int stand_pat = evaluate_board(board);
    best_score = stand_pat;

    if (stand_pat >= beta) {
      return stand_pat;
    } else if (stand_pat > alpha) {
      alpha = stand_pat;
      type = TRANSPOSITION_EXACT;
    }
  }

#if ENABLE_REVERSE_FUTILITY_DEPTH > 0
  static_assert(ENABLE_REVERSE_FUTILITY_DEPTH <
                    sizeof(reverse_futility_margins) / sizeof(int),
                "Margins unspecified");
  if (!in_check && !quiescent && beta < MATE &&
      depth <= ENABLE_REVERSE_FUTILITY_DEPTH && ply > 1 && !pv_node &&
      allow_null == ALLOW_NULL_MOVE) {
    int eval = evaluate_board(board);
    int margin = reverse_futility_margins[depth];
    if (eval - margin >= beta)
      return eval;
  }
#endif

  int threat = 0;
#if ENABLE_NULL_MOVE_PRUNING
  // Null move pruning.
  if (!in_check && !quiescent && depth > 2 && ply > 1 && !pv_node &&
      allow_null == ALLOW_NULL_MOVE) {
    State s;
    board_do_move(board, MOVE_NULL, &s);
    int null_value = -search_alpha_beta(board, -beta, -beta + 1, depth - 2,
                                        ply + 1, NULL, DISALLOW_NULL_MOVE);
    board_undo_move(board, MOVE_NULL);
    if (null_value >= beta) {
      int verify = search_alpha_beta(board, beta - 1, beta, depth - 2, ply,
                                     NULL, DISALLOW_NULL_MOVE);
      if (verify >= beta)
        return verify;
    } else if (null_value <= -MATE) {
      threat = 1;
    }
  }
#else
  (void)allow_null;
#endif

#if ENABLE_FUTILITY_DEPTH > 0
  static_assert(ENABLE_FUTILITY_DEPTH < sizeof(futility_margins) / sizeof(int),
                "Margins unspecified");
  int futile = 0;
  // XXX and if !pv_node?
  if (!quiescent && depth <= ENABLE_FUTILITY_DEPTH && !in_check && !threat &&
      alpha > -MATE) {
    int eval = evaluate_board(board);
    int margin = futility_margins[depth];
    if (eval + margin < alpha)
      futile = 1;
  }
#endif

  // generate pseudolegal moves
  Movelist moves;
  move_generate_movelist(
      board, &moves, quiescent && !in_check ? MOVE_GEN_QUIET : MOVE_GEN_ALL);

  // grab move from transposition table for move ordering -- but don't bother
  // for quiescent searches, since we don't write to the table for those and
  // have a reduced search space anyway, the memory lookup isn't worth it
  Move tt_move = MOVE_NULL;
  if (!quiescent) {
    tt_move = tt_get_best_move(board->state->zobrist);
  }

  Moveiter iter;
  const Move* killer_moves = history_get_killers(ply);
  moveiter_init(&iter, &moves, tt_move, killer_moves);

  /* since we generate only pseudolegal moves, we need to keep track if
     there actually are any legal moves at all */
  int legal_moves = 0;

  // loop thru all moves
  while (moveiter_has_next(&iter)) {
    Move move = moveiter_next(&iter);

    if (!move_is_legal(board, move))
      continue;

    legal_moves++;

    int gives_check = move_gives_check(board, move);

#if ENABLE_FUTILITY_DEPTH > 0
    int move_dest = move_destination_index(move);
    int is_pawn_to_prepromotion =
        move_piecetype(move) == PAWN &&
        (board_row_of(move_dest) == 1 || board_row_of(move_dest) == 6);
    if (futile && move != tt_move && move != killer_moves[0] &&
        move != killer_moves[1] && !move_is_promotion(move) &&
        !move_is_capture(move) && !gives_check && !is_pawn_to_prepromotion)
      continue;
#endif

    State s;
    board_do_move(board, move, &s);

    // value from recursive call to alpha-beta search
    int recursive_value;

    // keeps track of various re-search conditions
    int search_completed = 0;

    int extensions = gives_check;  // XXX try adding threat or 2*threat

    if (type == TRANSPOSITION_EXACT) {
      assert(pv_node);
      // PV search
      search_completed = 1;
      recursive_value = -search_alpha_beta(board, -alpha - 1, -alpha,
                                           (int8_t)(depth - 1 + extensions),
                                           ply + 1, NULL, ALLOW_NULL_MOVE);

      if ((recursive_value > alpha) && (recursive_value < beta)) {
        // PV search failed
        search_completed = 0;
      }
#if ENABLE_LMR
    } else if (legal_moves > 5 && depth > 2 && extensions == 0 &&
               !move_is_promotion(move) && !move_is_capture(move) &&
               move_piecetype(move) != PAWN && !threat && !in_check &&
               !gives_check && !quiescent) {
      // LMR
      search_completed = 1;
      recursive_value = -search_alpha_beta(board, -alpha - 1, -alpha, depth - 2,
                                           ply + 1, NULL, ALLOW_NULL_MOVE);

      if (recursive_value > alpha) {
        // LMR failed
        search_completed = 0;
      }
#endif
    }

    if (!search_completed) {
      // normal search
      recursive_value = -search_alpha_beta(
          board, -beta, -alpha, (int8_t)(depth - 1 + extensions), ply + 1,
          pv ? localpv : NULL, ALLOW_NULL_MOVE);
    }

    board_undo_move(board, move);

    if (recursive_value >= beta) {
      /* since this move caused a beta cutoff, we don't want
         to bother storing it in the pv *however*, we most
         definitely want to put it in the transposition table,
         since it will be searched first next time, and will
         thus immediately cause a cutoff again */
      if (!timeup) {
        tt_put(board->state->zobrist, recursive_value, move, TRANSPOSITION_BETA,
               board->generation, depth);
        history_update(move, ply);
      }

      return recursive_value;
    }

    if (recursive_value > best_score)
      best_score = recursive_value;

    if (recursive_value > alpha) {
      alpha = recursive_value;
      type = TRANSPOSITION_EXACT;
      best_move = move;
      if (pv) {
        pv[0] = best_move;
        memcpy(pv + 1, localpv, (size_t)depth * sizeof(Move));
      }
    }
  }

  if (timeup)
    return 0;

  if (legal_moves == 0 && !quiescent) {
    // No legal moves. Either we are in stalemate or checkmate.
    // Prefer checkmates which are closer to the current game state.
    // Use ply not depth for that because depth is affected by
    // extensions/reductions. Offset by max_depth so a toplevel result >= MATE
    // check still works.
    return in_check ? -(MATE + max_possible_depth - ply) : 0;
  } else if (legal_moves == 0 && quiescent) {
    int eval = evaluate_board(board);
    return eval;
  } else {
    // All moves pruned.
    if (best_score == -INFINITY)
      best_score = alpha;

    // Do not need to check for timeup here since we do it a few lines above,
    // after which the search of this position is complete and so we are still
    // safe to store even if time is up right now.
    tt_put(board->state->zobrist, best_score, best_move, type,
           board->generation, depth);
    history_update(best_move, ply);
    return best_score;
  }
}

static void search_print_pv(Move* pv, int8_t depth) {
  char buf[6];

  for (int i = 0; i < depth && pv[i] != MOVE_NULL; i++) {
    move_srcdest_form(pv[i], buf);
    fprintf(stderr, "%s ", buf);
  }
}
