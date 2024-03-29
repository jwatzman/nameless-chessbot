#ifndef _TYPES_H
#define _TYPES_H

#include <stdalign.h>
#include <stdint.h>

#include "config.h"

typedef unsigned char Color;
#define WHITE 0
#define BLACK 1

typedef unsigned char Piecetype;
#define PAWN 0
#define BISHOP 1
#define KNIGHT 2
#define ROOK 3
#define QUEEN 4
#define KING 5

#define CASTLE_R_KS (1 << 0)
#define CASTLE_R_QS (1 << 2)
#define CASTLE_R_BOTH (CASTLE_R_KS | CASTLE_R_QS)
#define CASTLE_R(side, color) ((side) << (color))

#define MAX_MOVES 256

#define NFINITY 1000000
#define MATE 300000
#define DRAW 0

// moves are stored in 24 bits
// from LSB to MSB:
// source square (6)
// destination square (6)
// type (3)
// color (1)
// move type normal/capture/castle/enpassant (2)
// captured type (3)
// promoted type (3)
// which leaves the eight MSB unused
typedef uint32_t Move;

#define NNUE_HIDDEN_LAYER 128

/**
 * State represents the portion of the board state which is either impossible or
 * expensive to recompute when doing/undoing a move. (e.g., it is impossible to
 * recompute the castle rights or halfmove count when undoing a move.)
 *
 * A State is passed into several bitboard functions. That state must remain
 * valid until the move associated with it (if any) is undone, or the board is
 * reinitalized.
 *
 * There are two different ways to allocate a State:
 * - On the stack. Used for search functions which will undo all their moves;
 *   the State on the stack will trivially remain in scope until the move is
 *   undone.
 * - With the statelist functions. Used for moves which are "permanent".
 */
struct State;
typedef struct State {
  Move last_move;

  // destination square of a pawn moving up two squares
  uint8_t enpassant_index;

  // Use CASTLE_R to access, e.g., CASTLE_R(CASTLE_R_KS, BLACK)
  uint8_t castle_rights;
  uint8_t halfmove_count;

  // TODO: search perf might be really sensitive to the exact layout of this
  // struct?
  uint64_t zobrist;

  // ----- Everything above prev is copied into the new state when a move is
  //       made!
  struct State* prev;
  // ----- Everything below prev is not!

  uint64_t king_attackers;
  uint64_t pinned;
} State;

typedef struct {
  uint64_t boards[2][6];
  uint64_t composite_boards[2];
  uint64_t full_composite;

  State* state;

  Color to_move;
  uint16_t generation;

#if ENABLE_NNUE
  int16_t alignas(32) nnue_hidden[2][NNUE_HIDDEN_LAYER];
#endif

  uint64_t zobrist_pos[2][6][64];
  uint64_t zobrist_castle[256];
  uint64_t zobrist_enpassant[8];
  uint64_t zobrist_black;
} Bitboard;

typedef struct {
  Move moves[MAX_MOVES];
  uint8_t n;
} Movelist;

#endif
