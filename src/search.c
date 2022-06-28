#include "search.h"

#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "bitops.h"
#include "evaluate.h"
#include "move.h"
#include "moveiter.h"
#include "timer.h"

typedef enum {
  TRANSPOSITION_EXACT,
  TRANSPOSITION_ALPHA,
  TRANSPOSITION_BETA,
  TRANSPOSITION_INVALIDATED
} __attribute__((__packed__)) TranspositionType;

typedef struct {
  uint64_t zobrist;
  int8_t depth;
  uint16_t generation;
  int value;
  Move best_move;
  TranspositionType type;
} __attribute__((__packed__)) TranspositionNode;

#define transposition_width 4
#define transposition_entries 524288
static TranspositionNode transposition_table[transposition_entries]
                                            [transposition_width];

#define max_possible_depth 30
#define max_quiescent_depth 50
#define aspiration_window 30

static volatile sig_atomic_t timeup;
static uint64_t nodes_searched;

// main search workhorse
static int search_alpha_beta(Bitboard* board,
                             int alpha,
                             int beta,
                             int8_t depth,
                             int8_t ply,
                             Move* pv);

/* asks the transposition table if we already know a good value for this
   position. If we do, return it. Otherwise, return INFINITY but adjust
   *alpha and *beta if we know better bounds for them */
static int search_transposition_get_value(uint64_t zobrist,
                                          int alpha,
                                          int beta,
                                          int8_t depth);

// if we have a previous best move for this zobrist, return it; 0 otherwise
static Move search_transposition_get_best_move(uint64_t zobrist);

// add to transposition table
static void search_transposition_put(uint64_t zobrist,
                                     int value,
                                     Move best_move,
                                     TranspositionType type,
                                     uint16_t generation,
                                     int8_t depth);

static void search_print_pv(Move* pv, int8_t depth);

Move search_find_move(Bitboard* board, const SearchDebug* debug) {
  Move best_move = 0;
  nodes_searched = 0;

  Move pv[max_possible_depth + 1];

  struct timespec start_time;
  clock_gettime(CLOCK_MONOTONIC, &start_time);

  timeup = 0;
  timer_begin(&timeup);

  int alpha = -INFINITY;
  int beta = INFINITY;

  // for each depth, call the main workhorse, search_alpha_beta
  uint8_t max_depth = debug && debug->maxDepth > 0
                          ? min(debug->maxDepth, max_possible_depth)
                          : max_possible_depth;
  for (int8_t depth = 1; depth <= max_depth; depth++) {
    // here we go...
    int val = search_alpha_beta(board, alpha, beta, depth, 1, pv);

    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    unsigned long int centiseconds_taken =
        100 * (end_time.tv_sec - start_time.tv_sec) +
        (end_time.tv_nsec - start_time.tv_nsec) / 10000000;

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

    /* if we sucessfully completed a depth (i.e. did not early terminate
       due to time), pull the first move off of the pv so that we can
       return it later, and then print it. Note that there is a minor
       race condition here: if we get the sigalarm after the very last
       check in the search_alpha_beta recursive calls, but before we get
       here, we can potentially throw away an entire depth's worth of
       work. This is unforunate but won't really hurt anything */
    if (!timeup) {
      best_move = pv[0];

      fprintf(stderr, "%i\t%i\t%lu\t%" PRIu64 "\t", depth,
              board->to_move == WHITE ? val : -val, centiseconds_taken,
              nodes_searched);
      search_print_pv(pv, depth);

      alpha = val - aspiration_window;
      beta = val + aspiration_window;

      if ((val >= MATE) || (val <= -MATE)) {
        fprintf(stderr, "-> mate\n");
        break;
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
                             Move* pv) {
  Move localpv[max_possible_depth + 1];

  if (timeup)
    return 0;

  nodes_searched++;

  TranspositionType type = TRANSPOSITION_ALPHA;

  int quiescent = depth <= 0;
  int in_check = board_in_check(board, board->to_move);

  Move best_move = MOVE_NULL;

  // 50-move rule
  if (board->state->halfmove_count == 100)
    return 0;

  /* only check for repetitions down at least 1 ply, since it results in a
     search termination without the game actually being over. Similarly, only
     check the transposition table two at least 1 ply */
  if (ply > 1) {
    // 3 repetition rule
    // XXX is this right?
    State* s = board->state;
    for (int back = board->state->halfmove_count - 2; back >= 0; back -= 2) {
      s = s->prev->prev;
      if (s->zobrist == board->state->zobrist)
        return 0;
    }

    // check transposition table for a useful value
    int table_val = search_transposition_get_value(board->state->zobrist, alpha,
                                                   beta, depth);

    if (table_val != INFINITY) {
      if (pv) {
        pv[0] = search_transposition_get_best_move(board->state->zobrist);
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

  // null move pruning
  if (quiescent) {
    pv = NULL;

    // quiescent null-move; this is necessary for correctness
    int null_move_value = evaluate_board(board);

    if (null_move_value >= beta)
      return null_move_value;
    else if (null_move_value > alpha) {
      alpha = null_move_value;
      type = TRANSPOSITION_EXACT;
    }
  }

  // generate pseudolegal moves
  Movelist moves;
  move_generate_movelist(board, &moves);

  // grab move from transposition table for move ordering -- but don't bother
  // for quiescent searches, since we don't write to the table for those and
  // have a reduced search space anyway, the memory lookup isn't worth it
  Move transposition_move = MOVE_NULL;
  if (!quiescent) {
    transposition_move =
        search_transposition_get_best_move(board->state->zobrist);
  }

  // move ordering; order transposition move first
  Moveiter iter;
  moveiter_init(&iter, &moves,
                alpha == beta - 1 ? MOVEITER_SORT_ONDEMAND : MOVEITER_SORT_FULL,
                transposition_move);

  /* since we generate only pseudolegal moves, we need to keep track if
     there actually are any legal moves at all */
  int legal_moves = 0;

  // loop thru all moves
  while (moveiter_has_next(&iter)) {
    Move move = moveiter_next(&iter);

    /* if we're quiescent, we only want capture moves unless the
       original position was in check, then do everything */
    if (quiescent && !in_check && !move_is_capture(move))
      break;

    if (!move_is_legal(board, move))
      continue;

    State s;
    board_do_move(board, move, &s);

    // value from recursive call to alpha-beta search
    int recursive_value;

    // keeps track of various re-search conditions
    int search_completed = 0;

    int move_causes_check = board_in_check(board, board->to_move);
    int extensions = move_causes_check;

    if (type == TRANSPOSITION_EXACT) {
      // PV search
      search_completed = 1;
      recursive_value = -search_alpha_beta(
          board, -alpha - 1, -alpha, depth - 1 + extensions, ply + 1, NULL);

      if ((recursive_value > alpha) && (recursive_value < beta)) {
        // PV search failed
        search_completed = 0;
      }
    }
    // LMR disabled -- move ordering not smart enough?
    /*
    else if (legal_moves > 4 && depth > 2 && extensions == 0 &&
             !move_is_promotion(move) && !move_is_capture(move) &&
             move_piecetype(move) != PAWN && !in_check &&
             !move_causes_check && !quiescent) {
      // LMR
      search_completed = 1;
      recursive_value = -search_alpha_beta(board, -alpha - 1, -alpha,
                                           depth - 2, ply + 1, pv + 1);

      if (recursive_value > alpha) {
        // LMR failed
        search_completed = 0;
      }
    }
    */

    if (!search_completed) {
      // normal search
      recursive_value =
          -search_alpha_beta(board, -beta, -alpha, depth - 1 + extensions,
                             ply + 1, pv ? localpv : NULL);
    }

    board_undo_move(board, move);

    if (recursive_value >= beta) {
      /* since this move caused a beta cutoff, we don't want
         to bother storing it in the pv *however*, we most
         definitely want to put it in the transposition table,
         since it will be searched first next time, and will
         thus immediately cause a cutoff again */
      search_transposition_put(board->state->zobrist, recursive_value, move,
                               TRANSPOSITION_BETA, board->generation, depth);

      return recursive_value;
    }

    if (recursive_value > alpha) {
      alpha = recursive_value;
      type = TRANSPOSITION_EXACT;
      best_move = move;
      if (pv) {
        pv[0] = best_move;
        memcpy(pv + 1, localpv, depth * sizeof(Move));
      }
    }

    legal_moves++;
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
    search_transposition_put(board->state->zobrist, alpha, best_move, type,
                             board->generation, depth);
    return alpha;
  }
}

static int search_transposition_get_value(uint64_t zobrist,
                                          int alpha,
                                          int beta,
                                          int8_t depth) {
  int index = zobrist % transposition_entries;

  if (depth < 1)
    return INFINITY;

  for (int i = 0; i < transposition_width; i++) {
    /* Since many zobrists may map to a single slot in the table, we
    want to make sure we got a match; also, we want to make sure that
    the entry was not made with a shallower depth than what we're
    currently using. */
    TranspositionNode* node = &transposition_table[index][i];
    if (node->zobrist == zobrist) {
      if (node->depth >= depth) {
        int val = node->value;

        if (node->type == TRANSPOSITION_EXACT)
          return val;
        else if ((node->type == TRANSPOSITION_ALPHA) && (val <= alpha))
          return val;
        else if ((node->type == TRANSPOSITION_BETA) && (val >= beta))
          return val;
      }
    }
  }

  return INFINITY;
}

static Move search_transposition_get_best_move(uint64_t zobrist) {
  int index = zobrist % transposition_entries;

  for (int i = 0; i < transposition_width; i++) {
    TranspositionNode* node = &transposition_table[index][i];

    if (node->zobrist == zobrist)
      return node->best_move;
  }

  return MOVE_NULL;
}

static void search_transposition_put(uint64_t zobrist,
                                     int value,
                                     Move best_move,
                                     TranspositionType type,
                                     uint16_t generation,
                                     int8_t depth) {
  /* we might not store in the transnposition table if:
      - time is up, thus we can't garuntee this was a full search
      - the depth is not deep enough to be useful
      - the value is dependent upon the ply at which it was searched and the
        depth to which it was searched (currently, only MATE moves)
      - it would replace a deeper search of this node
   */
  if (timeup || (depth < 1) || (value >= MATE) || (value <= -MATE))
    return;

  int index = zobrist % transposition_entries;
  TranspositionNode* target = NULL;

  for (int i = 0; i < transposition_width; i++) {
    if (transposition_table[index][i].zobrist == zobrist) {
      target = &transposition_table[index][i];
      break;
    }
  }

  if (!target) {
    int replace_depth = 999;
    for (int i = 0; i < transposition_width; i++) {
      TranspositionNode* node = &transposition_table[index][i];
      if (node->generation != generation && replace_depth > node->depth) {
        replace_depth = node->depth;
        target = node;
      }
    }
  }

  if (!target) {
    int replace_depth = 999;
    for (int i = 0; i < transposition_width; i++) {
      TranspositionNode* node = &transposition_table[index][i];
      if (replace_depth > node->depth) {
        replace_depth = node->depth;
        target = node;
      }
    }
  }

  if (target) {
    target->zobrist = zobrist;
    target->depth = depth;
    target->generation = generation;
    target->value = value;
    target->best_move = best_move;
    target->type = type;
  }
}

static void search_print_pv(Move* pv, int8_t depth) {
  char buf[6];

  for (int i = 0; i < depth && pv[i] != MOVE_NULL; i++) {
    move_srcdest_form(pv[i], buf);
    fprintf(stderr, "%s ", buf);
  }
}
