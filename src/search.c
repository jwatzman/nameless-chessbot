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

#define ASPIRATION_WINDOW 30
#define ASPIRATION_TIMER_EXTENSION_MIN_DEPTH 6

#define DISALLOW_NULL_MOVE 0
#define ALLOW_NULL_MOVE 1

#define MAX_BAD_QUIETS 16

#define FUTILITY_MAX_DEPTH 4
#define FUTILITY_MARGIN(d) (90 * d)

#define REVERSE_FUTILITY_MAX_DEPTH 4
#define REVERSE_FUTILITY_MARGIN(d) (90 * d)

static int timeup;
static uint64_t nodes_searched;

static int search_alpha_beta(Bitboard* board,
                             int alpha,
                             int beta,
                             int8_t depth,
                             int8_t ply,
                             Move* pv,
                             uint8_t allow_null);

static int search_qsearch(Bitboard* board, int alpha, int beta, int8_t ply);

static int search_is_draw(const Bitboard* board, int8_t ply);

static void search_print_pv(Move* pv, int8_t depth, FILE* f);

Move search_find_move(Bitboard* board, const SearchDebug* debug) {
  FILE* f;
  if (debug && debug->out)
    f = debug->out;
  else
    f = stdout;

  Move best_move = 0;
  nodes_searched = 0;
  history_clear();

  Move pv[MAX_POSSIBLE_DEPTH + 1];

  time_t start_cs = timer_get_centiseconds();

  timeup = 0;
  timer_begin();

  int alpha = -NFINITY;
  int beta = NFINITY;

  // for each depth, call the main workhorse, search_alpha_beta
  uint8_t max_depth = debug && debug->maxDepth > 0
                          ? min(debug->maxDepth, MAX_POSSIBLE_DEPTH)
                          : MAX_POSSIBLE_DEPTH;
  for (int8_t depth = 1; depth <= max_depth; depth++) {
    // here we go...
    int val =
        search_alpha_beta(board, alpha, beta, depth, 0, pv, ALLOW_NULL_MOVE);

    time_t centiseconds_taken = timer_get_centiseconds() - start_cs;

    if (timeup) {
      fprintf(f, "%i\t%i\t%lu\t%" PRIu64 "\ttimeup\n", depth, 0,
              centiseconds_taken, nodes_searched);
      break;
    }

    if ((val <= alpha) || (val >= beta)) {
      // aspiration window failure
      fprintf(f, "%i\t%i\t%lu\t%" PRIu64 "\taspiration failure\n", depth, val,
              centiseconds_taken, nodes_searched);
#if ENABLE_TIMER_ASPIRATION_FAILURE
      // XXX this works okay but tends to extend a lot in endgame positions
      // where we are just massively winning/losing, and less in that "critical
      // moment" where we get an aspiration failure at a high depth. Probably
      // should adjust timer to more aggressively extend, and the aspiration
      // window logic to use larger windows in the endgame/repeat-failure case.
      if (depth >= ASPIRATION_TIMER_EXTENSION_MIN_DEPTH)
        timer_extend();
#endif
      alpha = -NFINITY;
      beta = NFINITY;
      depth--;
      continue;
    }

    best_move = pv[0];
    if (debug && debug->score)
      *debug->score = val;

    fprintf(f, "%i\t%i\t%lu\t%" PRIu64 "\t", depth, val, centiseconds_taken,
            nodes_searched);
    search_print_pv(pv, depth, f);

    alpha = val - ASPIRATION_WINDOW;
    beta = val + ASPIRATION_WINDOW;

    if ((val >= MATE) || (val <= -MATE)) {
      fprintf(f, "-> mate");

      if (!(debug && debug->continueOnMate)) {
        fprintf(f, "\n");
        break;
      }
    }

    fprintf(f, "\n");

    if (debug && debug->stopMove) {
      char buf[6];
      move_srcdest_form(best_move, buf);

      int eval = evaluate_board(board);
      if (val > eval + 50 && !strcmp(buf, debug->stopMove)) {
        fprintf(f, "Found stop-move.\n");
        break;
      }
    }

    if (timer_stop_deepening()) {
      fprintf(f, "%i\t%i\t%lu\t%" PRIu64 "\tstopping here\n", depth, val,
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
  Move localpv[MAX_POSSIBLE_DEPTH + 1];

  if (!timeup)
    timeup = timer_timeup();

  if (timeup)
    return 0;

  if (depth <= 0)
    return search_qsearch(board, alpha, beta, ply);

  assert(alpha < beta);
  nodes_searched++;

  TranspositionType type = TRANSPOSITION_ALPHA;
  const int pv_node = beta > alpha + 1;

  Move best_move = MOVE_NULL;
  int best_score = -NFINITY;

  if (search_is_draw(board, ply))
    return DRAW;

  if (ply > 0) {
    // check transposition table for a useful value
    int table_val = tt_get_value(board->state->zobrist, alpha, beta, depth);

    if (table_val != NFINITY) {
      if (pv) {
        pv[0] = tt_get_best_move(board->state->zobrist);
        pv[1] = MOVE_NULL;  // We don't store pv in the table.
      }
      return table_val;
    }
  }

  const int in_check = board_in_check(board, board->to_move);
  if (!in_check && beta < MATE && depth <= REVERSE_FUTILITY_MAX_DEPTH &&
      ply > 0 && !pv_node && allow_null == ALLOW_NULL_MOVE) {
    // Need to deal with zug (since this prune is very similar to null move).
    // Quick-and-dirty is to make sure there is non-pawn material for us to
    // still move.
    uint64_t non_pawn = board->composite_boards[board->to_move] ^
                        board->boards[board->to_move][PAWN] ^
                        board->boards[board->to_move][KING];
    if (non_pawn) {
      int eval = evaluate_board(board);
      int margin = REVERSE_FUTILITY_MARGIN(depth);
      if (eval - margin >= beta)
        return eval;
    }
  }

  int threat = 0;
#if ENABLE_NULL_MOVE_PRUNING
  // Null move pruning.
  if (!in_check && depth > 2 && ply > 0 && !pv_node &&
      allow_null == ALLOW_NULL_MOVE) {
    State s;
    board_do_move(board, MOVE_NULL, &s);
    int null_value = -search_alpha_beta(board, -beta, -beta + 1, depth - 2,
                                        ply + 1, NULL, DISALLOW_NULL_MOVE);
    board_undo_move(board);
    if (null_value >= beta) {
      // Verification search to deal with zug. (Literature unclear if this is
      // actually a good solution but empirically it seems to work?)
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

  int futile = 0;
  if (depth <= FUTILITY_MAX_DEPTH && !in_check && !threat && !pv_node &&
      alpha > -MATE) {
    int eval = evaluate_board(board);
    int margin = FUTILITY_MARGIN(depth);
    // eval + margin + see < alpha
    // see < alpha - eval - margin
    futile = alpha - eval - margin;
  }

  Movelist moves;
  move_generate_movelist(board, &moves, MOVE_GEN_ALL);

  Move bad_quiets[MAX_BAD_QUIETS];
  int num_bad_quiets = 0;

  Move tt_move = tt_get_best_move(board->state->zobrist);

  Moveiter iter;
  const Move* killer_moves = history_get_killers(ply);
  moveiter_init(&iter, board, &moves, tt_move, killer_moves,
                history_get_countermove(board));

  /* since we generate only pseudolegal moves, we need to keep track if
     there actually are any legal moves at all */
  int legal_moves = 0;

  // loop thru all moves
  while (moveiter_has_next(&iter)) {
    MoveScore score;
    Move move = moveiter_next(&iter, &score);

    if (!move_is_legal(board, move))
      continue;

    legal_moves++;

    int gives_check = move_gives_check(board, move);

    int move_dest = move_destination_index(move);
    int is_pawn_to_prepromotion =
        move_piecetype(move) == PAWN &&
        (board_row_of(move_dest) == 1 || board_row_of(move_dest) == 6);
    int move_is_killer =
        killer_moves && (move == killer_moves[0] || move == killer_moves[1]);
    if (futile && move != tt_move && !move_is_killer &&
        !move_is_promotion(move) && !gives_check && !is_pawn_to_prepromotion) {
      if (!move_is_capture(move) && 0 < futile)
        continue;
      else if (move_is_capture(move) && moveiter_score_to_see(score) < futile)
        continue;
    }

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
    } else if (legal_moves > 2 && depth > 2 && extensions == 0 &&
               !move_is_promotion(move) && score < 0 &&
               move_piecetype(move) != PAWN && !threat && !in_check &&
               !gives_check) {
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

    board_undo_move(board);

    if (recursive_value >= beta) {
      /* since this move caused a beta cutoff, we don't want
         to bother storing it in the pv *however*, we most
         definitely want to put it in the transposition table,
         since it will be searched first next time, and will
         thus immediately cause a cutoff again */
      if (!timeup) {
        tt_put(board->state->zobrist, recursive_value, move, TRANSPOSITION_BETA,
               board->generation, depth);
        history_update(board, move, bad_quiets, num_bad_quiets, depth, ply);
      }

      return recursive_value;
    }

    if (num_bad_quiets < MAX_BAD_QUIETS && !move_is_capture(move))
      bad_quiets[num_bad_quiets++] = move;

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

  if (legal_moves == 0) {
    // No legal moves. Either we are in stalemate or checkmate.
    // Prefer checkmates which are closer to the current game state.
    // Use ply not depth for that because depth is affected by
    // extensions/reductions. Offset by max_depth so a toplevel result >= MATE
    // check still works.
    return in_check ? -(MATE + MAX_POSSIBLE_DEPTH - ply) : 0;
  } else {
    // All moves pruned.
    if (best_score == -NFINITY)
      best_score = alpha;

    // Do not need to check for timeup here since we do it a few lines above,
    // after which the search of this position is complete and so we are still
    // safe to store even if time is up right now.
    tt_put(board->state->zobrist, best_score, best_move, type,
           board->generation, depth);
    return best_score;
  }
}

static int search_qsearch(Bitboard* board, int alpha, int beta, int8_t ply) {
  if (!timeup)
    timeup = timer_timeup();

  if (timeup)
    return 0;

  assert(alpha < beta);
  nodes_searched++;

  int do_pv_search = 0;
#ifndef NDEBUG
  const int pv_node = beta > alpha + 1;
#endif

  int best_score = -NFINITY;

  if (search_is_draw(board, ply))
    return DRAW;

  const int in_check = board_in_check(board, board->to_move);

  // Quiescent stand-pat: "you don't have to take". Disallow when in check
  // since the position isn't "quiet" yet and there's no option to "do nothing".
  if (!in_check) {
    int stand_pat = evaluate_board(board);
    best_score = stand_pat;

    if (stand_pat >= beta) {
      return stand_pat;
    } else if (stand_pat > alpha) {
      alpha = stand_pat;
      do_pv_search = 1;
    }
  }

  Movelist moves;
  move_generate_movelist(board, &moves,
                         in_check ? MOVE_GEN_ALL : MOVE_GEN_QUIET);

  Moveiter iter;
  moveiter_init(&iter, board, &moves, MOVE_NULL, history_get_killers(ply),
                history_get_countermove(board));

  int legal_moves = 0;

  while (moveiter_has_next(&iter)) {
#if ENABLE_SEE_Q_PRUNE_LOSING_CAPTURES
    MoveScore score;
    Move move = moveiter_next(&iter, &score);

    if (!in_check && move_is_capture(move) && moveiter_score_to_see(score) < 0)
      continue;
#else
    Move move = moveiter_next(&iter, NULL);
#endif

    if (!move_is_legal(board, move))
      continue;

    legal_moves++;

    State s;
    board_do_move(board, move, &s);

    int recursive_value;
    if (do_pv_search) {
      assert(pv_node);
      recursive_value = -search_qsearch(board, -alpha - 1, -alpha, ply + 1);
      if (recursive_value > alpha && recursive_value < beta)
        recursive_value = -search_qsearch(board, -beta, -alpha, ply + 1);
    } else {
      recursive_value = -search_qsearch(board, -beta, -alpha, ply + 1);
    }

    board_undo_move(board);

    if (recursive_value >= beta) {
      if (!timeup)
        history_update(board, move, NULL, 0, 0,
                       ply);  // XXX should we be doing this?
      return recursive_value;
    }

    if (recursive_value > best_score)
      best_score = recursive_value;

    if (recursive_value > alpha) {
      alpha = recursive_value;
      do_pv_search = 1;
    }
  }

  if (timeup)
    return 0;

  if (legal_moves == 0) {
    return evaluate_board(board);
  } else {
    if (best_score == -NFINITY)
      best_score = alpha;

    return best_score;
  }
}

static int search_is_draw(const Bitboard* board, int8_t ply) {
  // 50-move rule
  if (board->state->halfmove_count == 100)
    return 1;

  // only check for repetitions down at least 1 ply, since it results in a
  // search termination without the game actually being over.
  if (ply > 0) {
    // 3 repetition rule
    State* s = board->state;
    for (int back = board->state->halfmove_count - 2; back >= 0; back -= 2) {
      s = s->prev;
      if (!s)
        break;
      s = s->prev;
      if (!s)
        break;
      if (s->zobrist == board->state->zobrist)
        return 1;
    }
  }

  return 0;
}

static void search_print_pv(Move* pv, int8_t depth, FILE* f) {
  char buf[6];

  for (int i = 0; i < depth && pv[i] != MOVE_NULL; i++) {
    move_srcdest_form(pv[i], buf);
    fprintf(f, "%s ", buf);
  }
}

uint64_t search_benchmark(void) {
  nodes_searched = 0;
  timeup = 0;
  history_clear();

  Bitboard board;
  State s;
  board_init_with_fen(
      &board, &s,
      "r2q3k/pn2bprp/4pNp1/2p1PbQ1/3p1P2/5NR1/PPP3PP/2B2RK1 b - - 0 1");

  timer_begin();
  for (int8_t depth = 1; depth <= 10; depth++) {
    search_alpha_beta(&board, -NFINITY, NFINITY, depth, 1, NULL,
                      ALLOW_NULL_MOVE);
  }
  timer_end();

  return nodes_searched;
}
