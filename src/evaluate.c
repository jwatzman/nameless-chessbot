#include <stdint.h>
#include <string.h>

#include "../gen/evaluate.h"
#include "bitboard.h"
#include "bitops.h"
#include "config.h"
#include "evaluate.h"
#include "move.h"
#include "movemagic.h"
#include "nnue.h"

// Horizontal reflection (B2 <-> B7 etc).
#define FLIP(x) (56 ^ x)

// For ease of reading, these tables are written as you are "used to", from
// white's perspective, with A1 in the lower-left corner. It turns out this is
// the correct orientation for black, but we need to do a flip for white --
// which is a horizontal reflection, not a 180 rotation.

// clang-format off
static const int rook_pos[] = {
 0,  0,  0,  0,  0,  0,  0,  0,
10, 10, 10, 10, 10, 10, 10, 10,
 0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  1,  7,  7,  0,  0,  0,
};

static const int bishop_pos[] = {
-15,  0,  0,  0,  0,  0,  0, -15,
 -5,  0,  0,  0,  0,  0,  0,  -5,
  0,  0,  0,  0,  0,  0,  0,   0,
  0,  0,  7, 12, 12,  7,  0,   0,
  0,  0,  7, 10, 10,  7,  0,   0,
  0,  0,  5,  7,  7,  5,  0,   0,
 -5,  5,  5,  7,  7,  5,  5,  -5,
-15, -7, -7, -7, -7, -7, -7, -15,
};

static const int knight_pos[] = {
-15, -5, -5, -5, -5, -5, -5, -15,
-10,  0,  0,  0,  0,  0,  0, -10,
-10,  0, 10,  8,  8, 10,  0, -10,
-10,  0,  8, 10, 10,  8,  0, -10,
-10,  0,  8, 10, 10,  8,  0, -10,
-10,  0, 10,  8,  8, 10,  0, -10,
-10,  0,  0,  0,  0,  0,  0, -10,
-15, -3, -5, -5, -5, -5, -3, -15,
};

static const int pawn_pos[] = {
0,  0,  0,  0,  0,  0,  0,  0,
15,15, 20, 25, 25, 20, 15, 15,
4,  8, 12, 16, 16, 12,  8,  4,
0,  6,  9, 10, 10,  9,  6,  0,
0,  4,  6, 15, 15,  6,  4,  0,
0,  2,  3,  5,  5,  3,  2,  0,
0,  0,  0, -9, -9,  0,  0,  0,
0,  0,  0,  0,  0,  0,  0,  0,
};

static const int king_pos[] = {
0, 0,  0,   0,   0, 0,  0, 0,
0, 0,  0,   0,   0, 0,  0, 0,
0, 0,  0,   0,   0, 0,  0, 0,
0, 0,  0,   0,   0, 0,  0, 0,
0, 0,  0,   0,   0, 0,  0, 0,
0, 0,  0,   0,   0, 0,  0, 0,
0, 0,  0, -10, -10, 0,  0, 0,
0, 0, 12,  -5,  -5, 0, 12, 0,
};

static const int passed_pawn_bonus[] = {
  0,  0,  0,  0,  0,  0,  0,  0,
 80, 80, 80, 80, 80, 80, 80, 80,
 65, 65, 65, 65, 65, 65, 65, 65,
 50, 50, 50, 50, 50, 50, 50, 50,
 25, 25, 25, 25, 25, 25, 25, 25,
 15, 15, 15, 15, 15, 15, 15, 15,
 10, 10, 10, 10, 10, 10, 10, 10,
  0,  0,  0,  0,  0,  0,  0,  0,
};

static const int king_endgame_pos[] = {
0,  0,  1,  3,  3,  1,  0,  0,
0,  5,  5,  5,  5,  5,  5,  0,
1,  5,  8,  8,  8,  8,  5,  1,
3,  5,  8, 10, 10,  8,  5,  3,
3,  5,  8, 10, 10,  8,  5,  3,
1,  5,  8,  8,  8,  8,  5,  1,
0,  5,  5,  5,  5,  5,  5,  0,
0,  0,  1,  3,  3,  1,  0,  0,
};
// clang-format on

// metatables
static const int* pos_tables[] = {pawn_pos, bishop_pos, knight_pos,
                                  rook_pos, 0,          king_pos};

static const int* endgame_pos_tables[] = {
    pawn_pos, bishop_pos, knight_pos, rook_pos, 0, king_endgame_pos};

// piece intrinsic values, in order of:
// pawn, bishop, knight, rook, queen, king
// (as defined in types.h)
static const int values[] = {100, 300, 300, 500, 900, 0};
static const int endgame_values[] = {175, 300, 300, 500, 1000, 0};

#define doubled_pawn_penalty -10

int evaluate_traditional(const Bitboard* board) {
  int result = 0;
  int endgame = popcnt(board->full_composite ^ board->boards[WHITE][PAWN] ^
                       board->boards[BLACK][PAWN]) < 8;
  Color to_move = board->to_move;

  for (Color color = 0; color < 2; color++) {
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
        uint8_t flipped_loc = color == WHITE ? FLIP(loc) : loc;
        pieces &= pieces - 1;

        if (piece == PAWN)
          if ((front_spans[color][loc] & board->boards[1 - color][PAWN]) == 0)
            color_result += passed_pawn_bonus[flipped_loc];

        const int* table =
            endgame ? endgame_pos_tables[piece] : pos_tables[piece];
        if (table)
          color_result += table[flipped_loc];
        color_result += (endgame ? endgame_values[piece] : values[piece]);

        // add in a bonus for every square that this piece can attack
        // only do this for bishops and rooks; knights are sufficiently taken
        // care of by positional bonus, and this will emphasize queens too much
        if (piece == BISHOP)
          color_result += popcnt(movemagic_bishop(loc, board->full_composite));
        if (piece == ROOK)
          color_result += popcnt(movemagic_rook(loc, board->full_composite));
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
