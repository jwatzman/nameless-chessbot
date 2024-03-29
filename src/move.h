#ifndef _MOVE_H
#define _MOVE_H

#include <stdint.h>
#include "bitboard.h"
#include "types.h"

#define MOVE_NULL ((Move)0)

typedef enum {
  MOVE_GEN_ALL = 0,

  // Only moves for quiescent search: captures and queen promotions.
  MOVE_GEN_QUIET = 1,
} MoveGenMode;

// Must be called at program startup.
void move_init(void);

// *Most*, but not *all*, of the moves returned here are legal. Need to still
// call move_is_legal as a final check before applying the move.
void move_generate_movelist(const Bitboard* board,
                            Movelist* movelist,
                            MoveGenMode m);

// does not consider enpassant (this is basically for checking king move and
// castling viability)
uint64_t move_generate_attackers(const Bitboard* board,
                                 Color attacker,
                                 uint8_t square,
                                 uint64_t composite);

// assuming this piece were to be at this location, where could it attack?
// note: does *not* generate pawn pushes or mask out friendly pieces
uint64_t move_generate_attacks(const Bitboard* board,
                               Piecetype piece,
                               Color color,
                               uint8_t index);

// Final move legality checks that are too expensive to do in
// move_generate_movelist.
int move_is_legal(const Bitboard* board, Move m);

// Will making this move give a check to the opponent?
int move_gives_check(const Bitboard* board, Move m);

// Which pieces are pinned to the king?
uint64_t move_generate_pinned(const Bitboard* board, Color color);

#define move_source_index_offset 0
#define move_source_index(move) ((move)&0x3F)

#define move_destination_index_offset 6
#define move_destination_index(move) \
  (((move) >> move_destination_index_offset) & 0x3F)

#define move_piecetype_offset 12
#define move_piecetype(move) (((move) >> move_piecetype_offset) & 0x07)

#define move_color_offset 15
#define move_color(move) (((move) >> move_color_offset) & 0x01)

#define move_type_normal 0U
#define move_type_capture 1U
#define move_type_castle 2U
#define move_type_enpassant 3U
#define move_type_offset 16
#define move_type(move) (((move) >> move_type_offset) & 0x03)
#define move_is_capture(move) (move_type(move) == move_type_capture)
#define move_is_castle(move) (move_type(move) == move_type_castle)
#define move_is_enpassant(move) (move_type(move) == move_type_enpassant)

#define move_captured_piecetype_offset 18
#define move_captured_piecetype(move) \
  (((move) >> move_captured_piecetype_offset) & 0x07)

// Promoted piecetype being set to > 0 means the move is a promotion (since you
// can never promote to a pawn).
#define move_promoted_piecetype_offset 21
#define move_promoted_piecetype(move) \
  (((move) >> move_promoted_piecetype_offset) & 0x07)
#define move_is_promotion(move) (move_promoted_piecetype(move) > 0)

#define move_unused_offset 24

static inline void move_srcdest_form(Move move, char srcdest_form[6]) {
  uint8_t src = move_source_index(move);
  uint8_t dest = move_destination_index(move);

  srcdest_form[0] = (char)board_col_of(src) + 'a';
  srcdest_form[1] = (char)board_row_of(src) + '1';
  srcdest_form[2] = (char)board_col_of(dest) + 'a';
  srcdest_form[3] = (char)board_row_of(dest) + '1';

  if (move_is_promotion(move)) {
    switch (move_promoted_piecetype(move)) {
      case QUEEN:
        srcdest_form[4] = 'q';
        break;
      case ROOK:
        srcdest_form[4] = 'r';
        break;
      case BISHOP:
        srcdest_form[4] = 'b';
        break;
      case KNIGHT:
        srcdest_form[4] = 'n';
        break;
      default:
        srcdest_form[4] = '?';
        break;
    }

    srcdest_form[5] = 0;
  } else
    srcdest_form[4] = 0;
}

#endif
