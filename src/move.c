#include <stdint.h>
#include <string.h>

#include "../gen/move.h"
#include "bitboard.h"
#include "bitops.h"
#include "move.h"
#include "movemagic.h"
#include "types.h"

static void move_generate_movelist_pawn_push(Bitboard* board,
                                             Movelist* movelist,
                                             uint64_t non_capture_mask,
                                             MoveGenMode m);
static void move_generate_movelist_castle(Bitboard* board, Movelist* movelist);
static void move_generate_movelist_enpassant(Bitboard* board,
                                             Movelist* movelist,
                                             int in_single_check,
                                             uint64_t non_capture_mask);
static Move move_generate_enpassant_move(Bitboard* board, uint8_t src);

static inline Move make_move(uint8_t src,
                             uint8_t dest,
                             Piecetype piece,
                             Color to_move) {
  return (unsigned)src << move_source_index_offset |
         (unsigned)dest << move_destination_index_offset |
         (unsigned)piece << move_piecetype_offset |
         (unsigned)to_move << move_color_offset;
}

static inline Move make_move_capture(Move move, Piecetype captured) {
  return move | 1U << move_is_capture_offset |
         (unsigned)captured << move_captured_piecetype_offset;
}

static inline Move make_move_promotion(Move move, Piecetype promoted) {
  return move | 1U << move_is_promotion_offset |
         (unsigned)promoted << move_promoted_piecetype_offset;
}

static inline Move make_move_castle(uint8_t src, uint8_t dest, Color to_move) {
  return make_move(src, dest, KING, to_move) | 1U << move_is_castle_offset;
}

static inline Move make_move_enpassant(uint8_t src,
                                       uint8_t dest,
                                       Color to_move) {
  return make_move(src, dest, PAWN, to_move) | 1U << move_is_enpassant_offset;
}

void move_init(void) {
  movemagic_init();
}

void move_generate_movelist(Bitboard* board,
                            Movelist* movelist,
                            MoveGenMode m) {
  movelist->num_promo = movelist->num_capture = movelist->num_other = 0;

  Color to_move = board->to_move;

  int in_double_check = twobits(board->state->king_attackers);
  int in_single_check = !in_double_check && board->state->king_attackers > 0;
  uint8_t king_loc = bitscan(board->boards[board->to_move][KING]);

  uint64_t non_capture_mask = ~0ULL;
  if (in_single_check) {
    // To block a check, need to move to one of the squares the attacking piece
    // is attacking.
    uint8_t index = bitscan(board->state->king_attackers);
    Piecetype checker = board_piecetype_at_index(board, index);
    switch (checker) {
      case QUEEN:
        non_capture_mask = movemagic_bishop(index, board->full_composite) |
                           movemagic_rook(index, board->full_composite);
        break;
      case ROOK:
        non_capture_mask = movemagic_rook(index, board->full_composite);
        break;
      case BISHOP:
        non_capture_mask = movemagic_bishop(index, board->full_composite);
        break;
      default:
        non_capture_mask = 0;
        break;
    }

    uint64_t between = raycast[king_loc][index] & raycast[index][king_loc];
    non_capture_mask &= between;
  }

  for (Piecetype piece = 0; piece < 6; piece++) {
    // In double check, the king must move, so skip generating other moves.
    if (in_double_check && piece != KING)
      continue;

    uint64_t pieces = board->boards[to_move][piece];

    while (pieces) {
      uint8_t src = bitscan(pieces);
      pieces &= pieces - 1;

      uint64_t dests = move_generate_attacks(board, piece, to_move, src);
      dests &=
          ~(board->composite_boards[to_move]);  // can't capture your own piece
      // Do not bother to mask off dests for the king which are in check:
      // move_is_legal tests that.
      if (board->state->pinned & (1ULL << src))
        dests &= raycast[king_loc][src];  // Pinned movement restricted.

      uint64_t captures = dests & board->composite_boards[1 - to_move];
      if (in_single_check && piece != KING)
        // Other pieces can only get us out of check by capturing the checking
        // piece.
        captures &= board->state->king_attackers;

      uint64_t non_captures;
      if (m == MOVE_GEN_QUIET || piece == PAWN) {
        non_captures = 0;
      } else {
        non_captures = dests & ~(board->composite_boards[1 - to_move]);
        if (in_single_check && piece != KING)
          non_captures &= non_capture_mask;
      }

      while (captures) {
        uint8_t dest = bitscan(captures);
        captures &= captures - 1;

        Move move = make_move_capture(make_move(src, dest, piece, to_move),
                                      board_piecetype_at_index(board, dest));

        if (piece == PAWN &&
            (board_row_of(dest) == 0 || board_row_of(dest) == 7)) {
          movelist->moves_promo[movelist->num_promo++] =
              make_move_promotion(move, QUEEN);
          movelist->moves_promo[movelist->num_promo++] =
              make_move_promotion(move, ROOK);
          movelist->moves_promo[movelist->num_promo++] =
              make_move_promotion(move, BISHOP);
          movelist->moves_promo[movelist->num_promo++] =
              make_move_promotion(move, KNIGHT);
        } else {
          movelist->moves_capture[movelist->num_capture++] = move;
        }
      }

      while (non_captures) {
        uint8_t dest = bitscan(non_captures);
        non_captures &= non_captures - 1;
        movelist->moves_other[movelist->num_other++] =
            make_move(src, dest, piece, to_move);
      }
    }
  }

  if (!in_double_check) {
    move_generate_movelist_pawn_push(board, movelist, non_capture_mask, m);
    if (m != MOVE_GEN_QUIET) {
      if (!in_single_check)
        move_generate_movelist_castle(board, movelist);
      move_generate_movelist_enpassant(board, movelist, in_single_check,
                                       non_capture_mask);
    }
  }

  movelist->num_total =
      movelist->num_promo + movelist->num_capture + movelist->num_other;
}

uint64_t move_generate_attacks(Bitboard* board,
                               Piecetype piece,
                               Color color,
                               uint8_t index) {
  switch (piece) {
    case PAWN:
      return pawn_attacks[color][index];

    case BISHOP:
      return movemagic_bishop(index, board->full_composite);

    case KNIGHT:
      return knight_attacks[index];

    case ROOK:
      return movemagic_rook(index, board->full_composite);

    case QUEEN:
      return movemagic_bishop(index, board->full_composite) |
             movemagic_rook(index, board->full_composite);

    case KING:
      return king_attacks[index];

    default:  // err...
      return 0;
  }
}

int move_is_legal(Bitboard* board, Move m) {
  if (move_is_castle(m)) {
    // Do not allow castling through check.
    uint8_t src = move_source_index(m);
    uint8_t dest = move_destination_index(m);
    int8_t dir = src < dest ? 1 : -1;
    for (uint8_t pos = (uint8_t)(src + dir); pos != (dest + dir); pos += dir) {
      if (move_generate_attackers(board, 1 - board->to_move, pos,
                                  board->full_composite) > 0)
        return 0;
    }

    return 1;
  } else if (move_piecetype(m) == KING) {
    // Do not allow moving the king into check.
    uint64_t composite =
        // Mask out the king because moving away from a sliding piece does not
        // get you out of check.
        board->full_composite & ~board->boards[board->to_move][KING];
    uint8_t dest = move_destination_index(m);
    return move_generate_attackers(board, 1 - board->to_move, dest,
                                   composite) == 0;
  } else {
    // All of the other cases are dealt with when intially generating the moves.
    return 1;
  }
}

uint64_t move_generate_pinned(Bitboard* board, Color color) {
  // We compute pinned pieces using an algorithm inspired by Stockfish:
  // - compute "snipers": the enemy sliding pieces which could hit the king if
  //   there were no other pieces in the way (empty occupancy to movemagic)
  // - compute "targets": everything that could get in the way of the sniper to
  //   the king (i.e., anything except the snipers or the king)
  // - for each sniper, if there is exactly one target in the area between the
  //   sniper and the king, and that piece is our piece, then it is pinned
  //
  // When doing movegen, if a piece is pinned, moving it is only legal if its
  // destination is the same ray from the king to its source location (since its
  // pinning attacker must be further along that ray, that means it stays in
  // between the two).

  uint8_t king_loc = bitscan(board->boards[color][KING]);
  uint64_t king_bishop = movemagic_bishop(king_loc, 0);
  uint64_t king_rook = movemagic_rook(king_loc, 0);

  uint64_t snipers = 0;
  snipers |= king_bishop & (board->boards[1 - color][BISHOP] |
                            board->boards[1 - color][QUEEN]);
  snipers |= king_rook &
             (board->boards[1 - color][ROOK] | board->boards[1 - color][QUEEN]);

  uint64_t targets =
      board->full_composite & ~snipers & ~board->boards[color][KING];

  uint64_t pinned = 0;
  while (snipers) {
    uint8_t sniper_loc = bitscan(snipers);
    snipers &= snipers - 1;

    uint64_t between =
        raycast[king_loc][sniper_loc] & raycast[sniper_loc][king_loc];

    uint64_t hits = targets & between;
    if (hits > 0 && !twobits(hits) && (hits & board->composite_boards[color]))
      pinned |= hits;
  }

  return pinned;
}

uint64_t move_generate_attackers(Bitboard* board,
                                 Color attacker,
                                 uint8_t square,
                                 uint64_t composite) {
  uint64_t rook_attacks = movemagic_rook(square, composite);
  uint64_t bishop_attacks = movemagic_bishop(square, composite);

  return (knight_attacks[square] & board->boards[attacker][KNIGHT]) |
         (king_attacks[square] & board->boards[attacker][KING]) |
         (rook_attacks & board->boards[attacker][ROOK]) |
         (rook_attacks & board->boards[attacker][QUEEN]) |
         (bishop_attacks & board->boards[attacker][BISHOP]) |
         (bishop_attacks & board->boards[attacker][QUEEN]) |
         (pawn_attacks[1 - attacker][square] & board->boards[attacker][PAWN]);
}

static void move_generate_movelist_pawn_push(Bitboard* board,
                                             Movelist* movelist,
                                             uint64_t non_capture_mask,
                                             MoveGenMode m) {
  Color to_move = board->to_move;
  uint64_t pawns = board->boards[to_move][PAWN];

  uint8_t king_loc = bitscan(board->boards[board->to_move][KING]);

  while (pawns) {
    uint8_t src = bitscan(pawns);
    pawns &= pawns - 1;

    int pinned = (board->state->pinned & (1ULL << src)) > 0;

    uint8_t row = board_row_of(src);
    uint8_t col = board_col_of(src);

    // try to move one space forward
    if ((to_move == WHITE && row < 7) || (to_move == BLACK && row > 0)) {
      uint8_t dest;
      if (to_move == WHITE)
        dest = board_index_of(row + 1, col);
      else
        dest = board_index_of(row - 1, col);

      if (pinned && (raycast[king_loc][src] & (1ULL << dest)) == 0)
        continue;

      uint64_t one_forward = 1ULL << dest;
      uint64_t one_forward_blocked = board->full_composite & one_forward;
      uint64_t one_forward_unmasked = non_capture_mask & one_forward;
      if (!one_forward_blocked && one_forward_unmasked) {
        Move move = make_move(src, dest, PAWN, to_move);

        // promote if needed
        if ((to_move == WHITE && row == 6) || (to_move == BLACK && row == 1)) {
          movelist->moves_promo[movelist->num_promo++] =
              make_move_promotion(move, QUEEN);
          if (m != MOVE_GEN_QUIET) {
            movelist->moves_promo[movelist->num_promo++] =
                make_move_promotion(move, ROOK);
            movelist->moves_promo[movelist->num_promo++] =
                make_move_promotion(move, BISHOP);
            movelist->moves_promo[movelist->num_promo++] =
                make_move_promotion(move, KNIGHT);
          }
        } else if (m != MOVE_GEN_QUIET) {
          movelist->moves_other[movelist->num_other++] = move;
        }
      }

      if (!one_forward_blocked && m != MOVE_GEN_QUIET) {
        // try to move two spaces forward
        if ((to_move == WHITE && row == 1) || (to_move == BLACK && row == 6)) {
          if (to_move == WHITE)
            dest = board_index_of(row + 2, col);
          else
            dest = board_index_of(row - 2, col);

          uint64_t two_forward = (1ULL << dest);
          uint64_t two_forward_blocked = board->full_composite & two_forward;
          uint64_t two_forward_unmasked = non_capture_mask & two_forward;
          if (!two_forward_blocked && two_forward_unmasked) {
            movelist->moves_other[movelist->num_other++] =
                make_move(src, dest, PAWN, to_move);
          }
        }
      }
    }
  }
}

static void move_generate_movelist_castle(Bitboard* board, Movelist* movelist) {
  // squares that must be clear for a castle: (along with the king not being in
  // check) W QS: empty 1 2 3, not attacked 2 3 W KS: empty 5 6, not attacked 5
  // 6 B QS: empty 57 58 59, not attacked 58 59 B KS: empty 61 62, not attacked
  // 61 62
  //
  // XXX this assumes the validity of the castling rights, in particular
  // that there actually are a rook/king in the right spot, funny things
  // happen if you, e.g., load a FEN with an invalid set of rights
  //
  // Finally, this does not check for castling through check: move_is_legal
  // tests that. (And it assumes it will not be called in the first place if the
  // king is already in check.)

  Color color = board->to_move;

  // white queenside
  if ((color == WHITE) &&
      (board->state->castle_rights & CASTLE_R(CASTLE_R_QS, WHITE))) {
    uint64_t clear = (1ULL << 1) | (1ULL << 2) | (1ULL << 3);
    if ((board->full_composite & clear) == 0) {
      movelist->moves_other[movelist->num_other++] =
          make_move_castle(4, 2, color);
    }
  }

  // white kingside
  if ((color == WHITE) &&
      (board->state->castle_rights & CASTLE_R(CASTLE_R_KS, WHITE))) {
    uint64_t clear = (1ULL << 5) | (1ULL << 6);
    if ((board->full_composite & clear) == 0) {
      movelist->moves_other[movelist->num_other++] =
          make_move_castle(4, 6, color);
    }
  }

  // black queenside
  if ((color == BLACK) &&
      (board->state->castle_rights & CASTLE_R(CASTLE_R_QS, BLACK))) {
    uint64_t clear = (1ULL << 57) | (1ULL << 58) | (1ULL << 59);
    if ((board->full_composite & clear) == 0) {
      movelist->moves_other[movelist->num_other++] =
          make_move_castle(60, 58, color);
    }
  }

  // black kingside
  if ((color == BLACK) &&
      (board->state->castle_rights & CASTLE_R(CASTLE_R_KS, BLACK))) {
    uint64_t clear = (1ULL << 61) | (1ULL << 62);
    if ((board->full_composite & clear) == 0) {
      movelist->moves_other[movelist->num_other++] =
          make_move_castle(60, 62, color);
    }
  }
}

static void move_generate_movelist_enpassant(Bitboard* board,
                                             Movelist* movelist,
                                             int in_single_check,
                                             uint64_t non_capture_mask) {
  uint8_t ep_index = board->state->enpassant_index;
  if (ep_index == 0)
    return;

  if (in_single_check) {
    uint8_t dest = board->to_move == WHITE ? ep_index + 8 : ep_index - 8;
    uint64_t dest_mask = 1ULL << dest;
    uint64_t captured_mask = 1ULL << ep_index;

    // If we are in check, an enpassant move is invalid unless we either capture
    // the checking piece or land somewhere to block the check.
    if ((dest_mask & non_capture_mask) == 0 &&
        (captured_mask & board->state->king_attackers) == 0)
      return;
  }

  if (board_col_of(ep_index) > 0) {
    Move move = move_generate_enpassant_move(board, ep_index - 1);
    if (move != MOVE_NULL)
      movelist->moves_other[movelist->num_other++] = move;
  }

  if (board_col_of(ep_index) < 7) {
    Move move = move_generate_enpassant_move(board, ep_index + 1);
    if (move != MOVE_NULL)
      movelist->moves_other[movelist->num_other++] = move;
  }
}

static Move move_generate_enpassant_move(Bitboard* board, uint8_t src) {
  Color color = board->to_move;
  if ((board->boards[color][PAWN] & (1ULL << src)) == 0)
    return MOVE_NULL;

  uint8_t ep_index = board->state->enpassant_index;
  uint8_t dest = color == WHITE ? ep_index + 8 : ep_index - 8;
  uint8_t king_loc = bitscan(board->boards[color][KING]);

  // Standard pin check: if we are pinned we need to land along the pinning ray.
  if ((board->state->pinned & (1ULL << src)) != 0 &&
      (raycast[king_loc][src] & (1ULL << dest)) == 0)
    return MOVE_NULL;

  // This is a check like the pin checks, but it's not quite a pin: an enpassant
  // capture (re)moves both the capturing and captured piece from the same row,
  // so we need to make sure there isn't a rook or queen behind them. (Neither
  // is pinned since they are *both* in the way at the same time, but then
  // removed at the same time exposing the king.)
  if (board_row_of(ep_index) == board_row_of(king_loc)) {
    uint64_t removed = (1ULL << ep_index) | (1ULL << src);
    // We don't need to be especially careful checking that the attacks are on
    // that row -- if we hit a rook/queen in a different direction, we're in
    // check anyway and this move won't help.
    uint64_t attacks =
        movemagic_rook(king_loc, board->full_composite & ~removed);
    if ((attacks & board->boards[1 - board->to_move][ROOK]) ||
        (attacks & board->boards[1 - board->to_move][QUEEN]))
      return MOVE_NULL;
  }

  return make_move_enpassant(src, dest, color);
}
