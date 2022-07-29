#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>

#include "../gen/evaluate.h"
#include "bitboard.h"
#include "bitops.h"
#include "config.h"
#include "evaluate.h"
#include "move.h"
#include "nnue.h"

// clang-format off

// position bonuses; remember that
// since a1 is the first number, it is in the upper left
static const int rook_pos[] = {
0, 3, 7,  8,  8, 7, 3, 0,
0, 3, 7,  8,  8, 7, 3, 0,
0, 3, 7,  8,  8, 7, 3, 0,
0, 3, 7, 10, 10, 7, 3, 0,
0, 3, 7, 10, 10, 7, 3, 0,
0, 3, 7,  8,  8, 7, 3, 0,
0, 3, 7,  8,  8, 7, 3, 0,
0, 3, 7,  8,  8, 7, 3, 0
};

static const int bishop_pos[] = {
-5, -5, -7, -5, -5, -7, -5, -5,
-5, 10,  5,  8,  8,  5, 10, -5,
-5,  5,  3,  8,  8,  3,  5, -5,
-5,  3, 10,  3,  3, 10,  3, -5,
-5,  3, 10,  3,  3, 10,  3, -5,
-5,  5,  3,  8,  8,  3,  5, -5,
-5, 10,  5,  8,  8,  5, 10, -5,
-5, -5, -7, -5, -5, -7, -5, -5
};

static const int knight_pos[] = {
-10, -7, -5, -5, -5, -5, -7,-10,
 -8,  0,  0,  3,  3,  0,  0, -8,
 -8,  0, 10,  8,  8, 10,  0, -8,
 -8,  0,  8, 10, 10,  8,  0, -8,
 -8,  0,  8, 10, 10,  8,  0, -8,
 -8,  0, 10,  8,  8, 10,  0, -8,
 -8,  0,  0,  3,  3,  0,  0, -8,
-10, -7, -5, -5, -5, -5, -7,-10
};

// pawns aren't hoizontally symmetric;
// this needs to be special-cased, see below
static const int pawn_pos[] = {
0,  0,  0,  0,  0,  0,  0,  0,
0,  0,  0, -9, -9,  0,  0,  0,
0,  2,  3,  5,  5,  3,  2,  0,
0,  4,  6, 15, 15,  6,  4,  0,
0,  6,  9, 10, 10,  9,  6,  0,
4,  8, 12, 16, 16, 12,  8,  4,
15,15, 20, 25, 25, 20, 15, 15,
0,  0,  0,  0,  0,  0,  0,  0
};

static const int passed_pawn_bonus[] = {
  0,  0,  0,  0,  0,  0,  0,  0,
 10, 10, 10, 10, 10, 10, 10, 10,
 15, 15, 15, 15, 15, 15, 15, 15,
 25, 25, 25, 25, 25, 25, 25, 25,
 50, 50, 50, 50, 50, 50, 50, 50,
 65, 65, 65, 65, 65, 65, 65, 65,
 80, 80, 80, 80, 80, 80, 80, 80,
  0,  0,  0,  0,  0,  0,  0,  0
};

// endgame only
static const int king_endgame_pos[] = {
0,  0,  1,  3,  3,  1,  0,  0,
0,  5,  5,  5,  5,  5,  5,  0,
1,  5,  8,  8,  8,  8,  5,  1,
3,  5,  8, 10, 10,  8,  5,  3,
3,  5,  8, 10, 10,  8,  5,  3,
1,  5,  8,  8,  8,  8,  5,  1,
0,  5,  5,  5,  5,  5,  5,  0,
0,  0,  1,  3,  3,  1,  0,  0
};

// clang-format on

// metatables
static const int* pos_tables[] = {pawn_pos, bishop_pos, knight_pos,
                                  rook_pos, 0,          0};

static const int* endgame_pos_tables[] = {
    pawn_pos, bishop_pos, knight_pos, rook_pos, 0, king_endgame_pos};

// piece intrinsic values, in order of:
// pawn, bishop, knight, rook, queen, king
// (as defined in types.h)
static const int16_t values[] = {100, 300, 300, 500, 900, 0};
static const int16_t endgame_values[] = {175, 300, 300, 500, 1000, 0};

#define doubled_pawn_penalty -10

int evaluate_traditional(const Bitboard* board) {
  int result = 0;
  int endgame = popcnt(board->full_composite ^ board->boards[WHITE][PAWN] ^
                       board->boards[BLACK][PAWN]) < 8;
  Color to_move = board->to_move;

  for (Color color = 0; color < 2; color++) {
    // add values for the current player, and subtract them for the opponent
    int color_result = 0;

    // XXX bring back has-castled bonus? (10)

    // doubled pawns
    // 0x0101010101010101 masks a single column
    for (int col = 0; col < 8; col++)
      color_result +=
          doubled_pawn_penalty *
          (popcnt(board->boards[color][PAWN] & (0x0101010101010101ULL << col)) -
           1);

    for (Piecetype piece = 0; piece < 6; piece++) {
      uint64_t pieces = board->boards[color][piece];
      while (pieces) {
        uint8_t loc = bitscan(pieces);
        pieces &= pieces - 1;

        // pawns have some special stuff done to them
        if (piece == PAWN) {
          // passed pawn; do this before the location swap below
          int passed_pawn = 0;
          if ((front_spans[color][loc] & board->boards[1 - color][PAWN]) == 0)
            passed_pawn = 1;

          // since the pawn table is not hoizontally symmetric,
          // we need to flip it for black
          if (color == BLACK)
            loc = 63 - loc;

          if (passed_pawn)
            color_result += passed_pawn_bonus[loc];
        }

        // add in piece intrinsic value, and bonus for its location
        const int* table =
            endgame ? endgame_pos_tables[piece] : pos_tables[piece];
        if (table)
          color_result += table[loc];
        color_result += (endgame ? endgame_values[piece] : values[piece]);

        // add in a bonus for every square that this piece can attack
        // only do this for bishops and rooks; knights are sufficiently taken
        // care of by positional bonus, and this will emphasize queens too much
        if (piece == BISHOP || piece == ROOK) {
          uint64_t attacks = move_generate_attacks(board, piece, color, loc);
          color_result += popcnt(attacks);
        }
      }
    }

    if (color == to_move)
      result += color_result;
    else
      result -= color_result;
  }

  return result;
}

int evaluate_board(const Bitboard* board) {
#if ENABLE_NNUE
  return nnue_evaluate(board);
#else
  return evaluate_traditional(board);
#endif
}

int16_t evaluate_see(const Bitboard* board, Move m) {
  assert(move_is_capture(m));
  assert(board->to_move == move_color(m));

  uint64_t composite = board->full_composite;
  Color to_move = board->to_move;
  const uint8_t dest = move_destination_index(m);
  Piecetype to_capture = move_piecetype(m);
  Piecetype to_be_captured = move_captured_piecetype(m);

  composite &= ~(1ULL << move_source_index(m));
  to_move = !to_move;
  uint64_t attackers = move_generate_attackers(board, to_move, dest, composite);
  if (attackers == 0)
    return values[to_be_captured];

  int16_t minmax_sums[32];
  minmax_sums[0] = values[to_be_captured];
  int i = 1;

  while (attackers) {
    to_be_captured = to_capture;
    if (to_be_captured == KING) {
      // King put themself in check -- can't do that. Put a huge value on it so
      // the minmaxing below won't pick this.
      minmax_sums[i] = SHRT_MAX;
      i++;
      break;
    }

    for (to_capture = PAWN;
         (board->boards[to_move][to_capture] & attackers) == 0; to_capture++) {
      assert(to_capture <= KING);
    }

    assert((size_t)i < sizeof(minmax_sums) / sizeof(int16_t));
    minmax_sums[i] = -minmax_sums[i - 1] + values[to_be_captured];
    i++;

    uint64_t to_capture_board = board->boards[to_move][to_capture] & attackers;
    composite ^= (to_capture_board & -to_capture_board);
    to_move = !to_move;
    attackers =
        // XXX we can do better than recomputing this every time.
        move_generate_attackers(board, to_move, dest, composite) & composite;
  }

  assert(i >= 2);
  for (i--; i > 0; i--)
    minmax_sums[i - 1] = min16(minmax_sums[i - 1], -minmax_sums[i]);

  return minmax_sums[0];
}
