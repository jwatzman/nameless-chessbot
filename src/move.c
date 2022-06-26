#include <stdint.h>
#include <string.h>

#include "../gen/move.h"
#include "bitboard.h"
#include "bitops.h"
#include "move.h"
#include "movemagic.h"
#include "types.h"

static void move_generate_movelist_pawn_push(Bitboard* board,
                                             Movelist* movelist);
static void move_generate_movelist_castle(Bitboard* board, Movelist* movelist);
static void move_generate_movelist_enpassant(Bitboard* board,
                                             Movelist* movelist);

void move_init(void) {
  movemagic_init();
}

void move_generate_movelist(Bitboard* board, Movelist* movelist) {
  movelist->num_promo = movelist->num_capture = movelist->num_other = 0;

  Color to_move = board->to_move;

  // In double check, the king must move, so skip generating other moves.
  int in_double_check = popcnt(board->state->king_attackers) > 1;

  for (Piecetype piece = in_double_check ? KING : 0; piece < 6; piece++) {
    uint64_t pieces = board->boards[to_move][piece];

    while (pieces) {
      uint8_t src = bitscan(pieces);
      pieces &= pieces - 1;

      uint64_t dests = move_generate_attacks(board, piece, to_move, src);
      dests &=
          ~(board->composite_boards[to_move]);  // can't capture your own piece
      if (piece == KING)
        dests &= ~(board->state->king_danger);  // King can't move into check.

      uint64_t captures = dests & board->composite_boards[1 - to_move];
      uint64_t non_captures =
          piece == PAWN ? 0 : dests & ~(board->composite_boards[1 - to_move]);

      while (captures) {
        uint8_t dest = bitscan(captures);
        captures &= captures - 1;

        Move move = 0;
        move |= src << move_source_index_offset;
        move |= dest << move_destination_index_offset;
        move |= piece << move_piecetype_offset;
        move |= to_move << move_color_offset;

        move |= 1ULL << move_is_capture_offset;
        move |= board_piecetype_at_index(board, dest)
                << move_captured_piecetype_offset;

        if (piece == PAWN &&
            (board_row_of(dest) == 0 || board_row_of(dest) == 7)) {
          move |= 1ULL << move_is_promotion_offset;

          movelist->moves_promo[movelist->num_promo++] =
              (move | (QUEEN << move_promoted_piecetype_offset));
          movelist->moves_promo[movelist->num_promo++] =
              (move | (ROOK << move_promoted_piecetype_offset));
          movelist->moves_promo[movelist->num_promo++] =
              (move | (BISHOP << move_promoted_piecetype_offset));
          movelist->moves_promo[movelist->num_promo++] =
              (move | (KNIGHT << move_promoted_piecetype_offset));
        } else {
          movelist->moves_capture[movelist->num_capture++] = move;
        }
      }

      while (non_captures) {
        uint8_t dest = bitscan(non_captures);
        non_captures &= non_captures - 1;

        Move move = 0;
        move |= src << move_source_index_offset;
        move |= dest << move_destination_index_offset;
        move |= piece << move_piecetype_offset;
        move |= to_move << move_color_offset;

        movelist->moves_other[movelist->num_other++] = move;
      }
    }
  }

  if (!in_double_check) {
    move_generate_movelist_pawn_push(board, movelist);
    move_generate_movelist_castle(board, movelist);
    move_generate_movelist_enpassant(board, movelist);
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
      break;

    case BISHOP:
      return movemagic_bishop(index, board->full_composite);
      break;

    case KNIGHT:
      return knight_attacks[index];
      break;

    case ROOK:
      return movemagic_rook(index, board->full_composite);
      break;

    case QUEEN:
      return move_generate_attacks(board, BISHOP, color, index) |
             move_generate_attacks(board, ROOK, color, index);
      break;

    case KING:
      return king_attacks[index];
      break;

    default:  // err...
      return 0;
      break;
  }
}

uint64_t move_generate_king_danger(Bitboard* board, Color color) {
  uint64_t all_attacks = 0;

  // This is used to prevent 1 - color's king from moving into check. Moving
  // away from a sliding piece is still in check, so temporarily mask out that
  // king to extend the sliding's range.
  board->full_composite ^= board->boards[1 - color][KING];
  for (Piecetype piece = 0; piece < 6; piece++) {
    uint64_t pieces = board->boards[color][piece];

    while (pieces) {
      uint8_t src = bitscan(pieces);
      pieces &= pieces - 1;

      all_attacks |= move_generate_attacks(board, piece, color, src);
    }
  }
  board->full_composite ^= board->boards[1 - color][KING];

  // TODO: enpassant

  return all_attacks;
}

uint64_t move_generate_attackers(Bitboard* board,
                                 Color attacker,
                                 uint8_t square) {
  uint64_t rook_attacks = movemagic_rook(square, board->full_composite);
  uint64_t bishop_attacks = movemagic_bishop(square, board->full_composite);

  return (knight_attacks[square] & board->boards[attacker][KNIGHT]) |
         (king_attacks[square] & board->boards[attacker][KING]) |
         (rook_attacks & board->boards[attacker][ROOK]) |
         (rook_attacks & board->boards[attacker][QUEEN]) |
         (bishop_attacks & board->boards[attacker][BISHOP]) |
         (bishop_attacks & board->boards[attacker][QUEEN]) |
         (pawn_attacks[1 - attacker][square] & board->boards[attacker][PAWN]);
}

static void move_generate_movelist_pawn_push(Bitboard* board,
                                             Movelist* movelist) {
  Color to_move = board->to_move;
  uint64_t pawns = board->boards[to_move][PAWN];

  while (pawns) {
    uint8_t src = bitscan(pawns);
    pawns &= pawns - 1;

    uint8_t row = board_row_of(src);
    uint8_t col = board_col_of(src);

    // try to move one space forward
    if ((to_move == WHITE && row < 7) || (to_move == BLACK && row > 0)) {
      uint8_t dest;
      if (to_move == WHITE)
        dest = board_index_of(row + 1, col);
      else
        dest = board_index_of(row - 1, col);

      if (!(board->full_composite & (1ULL << dest))) {
        Move move = 0;
        move |= src << move_source_index_offset;
        move |= dest << move_destination_index_offset;
        move |= PAWN << move_piecetype_offset;
        move |= to_move << move_color_offset;

        // promote if needed
        if ((to_move == WHITE && row == 6) || (to_move == BLACK && row == 1)) {
          move |= 1ULL << move_is_promotion_offset;

          movelist->moves_promo[movelist->num_promo++] =
              (move | (QUEEN << move_promoted_piecetype_offset));
          movelist->moves_promo[movelist->num_promo++] =
              (move | (ROOK << move_promoted_piecetype_offset));
          movelist->moves_promo[movelist->num_promo++] =
              (move | (BISHOP << move_promoted_piecetype_offset));
          movelist->moves_promo[movelist->num_promo++] =
              (move | (KNIGHT << move_promoted_piecetype_offset));
        } else
          movelist->moves_other[movelist->num_other++] = move;

        // try to move two spaces forward
        if ((to_move == WHITE && row == 1) || (to_move == BLACK && row == 6)) {
          if (to_move == WHITE)
            dest = board_index_of(row + 2, col);
          else
            dest = board_index_of(row - 2, col);

          if (!(board->full_composite & (1ULL << dest))) {
            move = 0;
            move |= src << move_source_index_offset;
            move |= dest << move_destination_index_offset;
            move |= PAWN << move_piecetype_offset;
            move |= to_move << move_color_offset;

            movelist->moves_other[movelist->num_other++] = move;
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

  Color color = board->to_move;
  if (board_in_check(board, color))  // TODO :(
    return;

  // white queenside
  if ((color == WHITE) &&
      (board->state->castle_rights & CASTLE_R(CASTLE_R_QS, WHITE))) {
    uint64_t noattack = (1ULL << 2) | (1ULL << 3);
    uint64_t clear = (1ULL << 1) | noattack;
    if ((board->full_composite & clear) == 0 &&
        (board->state->king_danger & noattack) == 0) {
      Move move = 0;
      move |= 4 << move_source_index_offset;
      move |= 2 << move_destination_index_offset;
      move |= KING << move_piecetype_offset;
      move |= color << move_color_offset;
      move |= 1 << move_is_castle_offset;
      movelist->moves_other[movelist->num_other++] = move;
    }
  }

  // white kingside
  if ((color == WHITE) &&
      (board->state->castle_rights & CASTLE_R(CASTLE_R_KS, WHITE))) {
    uint64_t clear = (1ULL << 5) | (1ULL << 6);
    if ((board->full_composite & clear) == 0 &&
        (board->state->king_danger & clear) == 0) {
      Move move = 0;
      move |= 4 << move_source_index_offset;
      move |= 6 << move_destination_index_offset;
      move |= KING << move_piecetype_offset;
      move |= color << move_color_offset;
      move |= 1 << move_is_castle_offset;
      movelist->moves_other[movelist->num_other++] = move;
    }
  }

  // black queenside
  if ((color == BLACK) &&
      (board->state->castle_rights & CASTLE_R(CASTLE_R_QS, BLACK))) {
    uint64_t noattack = (1ULL << 58) | (1ULL << 59);
    uint64_t clear = (1ULL << 57) | noattack;
    if ((board->full_composite & clear) == 0 &&
        (board->state->king_danger & noattack) == 0) {
      Move move = 0;
      move |= 60 << move_source_index_offset;
      move |= 58 << move_destination_index_offset;
      move |= KING << move_piecetype_offset;
      move |= color << move_color_offset;
      move |= 1 << move_is_castle_offset;
      movelist->moves_other[movelist->num_other++] = move;
    }
  }

  // black kingside
  if ((color == BLACK) &&
      (board->state->castle_rights & CASTLE_R(CASTLE_R_KS, BLACK))) {
    uint64_t clear = (1ULL << 61) | (1ULL << 62);
    if ((board->full_composite & clear) == 0 &&
        (board->state->king_danger & clear) == 0) {
      Move move = 0;
      move |= 60 << move_source_index_offset;
      move |= 62 << move_destination_index_offset;
      move |= KING << move_piecetype_offset;
      move |= color << move_color_offset;
      move |= 1 << move_is_castle_offset;
      movelist->moves_other[movelist->num_other++] = move;
    }
  }
}

static void move_generate_movelist_enpassant(Bitboard* board,
                                             Movelist* movelist) {
  uint8_t ep_index = board->state->enpassant_index;
  Color color = board->to_move;
  if (ep_index) {
    if (board_col_of(ep_index) > 0 &&
        (board->boards[color][PAWN] & (1ULL << (ep_index - 1)))) {
      Move move = 0;
      move |= (ep_index - 1) << move_source_index_offset;
      move |= (color == WHITE ? ep_index + 8 : ep_index - 8)
              << move_destination_index_offset;
      move |= PAWN << move_piecetype_offset;
      move |= color << move_color_offset;
      move |= 1 << move_is_enpassant_offset;
      movelist->moves_other[movelist->num_other++] = move;
    }

    if (board_col_of(ep_index) < 7 &&
        (board->boards[color][PAWN] & (1ULL << (ep_index + 1)))) {
      Move move = 0;
      move |= (ep_index + 1) << move_source_index_offset;
      move |= (color == WHITE ? ep_index + 8 : ep_index - 8)
              << move_destination_index_offset;
      move |= PAWN << move_piecetype_offset;
      move |= color << move_color_offset;
      move |= 1 << move_is_enpassant_offset;
      movelist->moves_other[movelist->num_other++] = move;
    }
  }
}
