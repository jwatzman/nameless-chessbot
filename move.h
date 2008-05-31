#ifndef _MOVE_H
#define _MOVE_H

#include <stdint.h>
#include "types.h"
#include "bitboard.h"

// generates *psdueomoves* -- call move_verify to check if it's actually valid
void move_generate_movelist(Bitboard *board, Color to_move, Movelist *movelist);

// verifies a pseudomove, returns 1 if it's valid, 0 otherwise
int move_verify(Bitboard *board, Move move);

static const uint8_t move_source_index_offset = 0;
static inline uint8_t move_source_index(Move move)
{
	return move & 0x3F;
}

static const uint8_t move_destination_index_offset = 6;
static inline uint8_t move_destination_index(Move move)
{
	return (move >> move_destination_index_offset) & 0x3F;
}

static const uint8_t move_piecetype_offset = 12;
static inline Piecetype move_piecetype(Move move)
{
	return (move >> move_piecetype_offset) & 0x07;
}

static const uint8_t move_color_offset = 15;
static inline Color move_color(Move move)
{
	return (move >> move_color_offset) & 0x01;
}

static const uint8_t move_is_castle_offset = 16;
static inline int move_is_castle(Move move)
{
	return (move >> move_is_castle_offset) & 0x01;
}

static const uint8_t move_is_enpassant_offset = 17;
static inline int move_is_enpassant(Move move)
{
	return (move >> move_is_enpassant_offset) & 0x01;
}

static const uint8_t move_is_capture_offset = 18;
static inline int move_is_capture(Move move)
{
	return (move >> move_is_capture_offset) & 0x01;
}

static const uint8_t move_is_promotion_offset = 19;
static inline int move_is_promotion(Move move)
{
	return (move >> move_is_promotion_offset) & 0x01;
}

static const uint8_t move_captured_piecetype_offset = 20;
static inline int move_captured_piecetype(Move move)
{
	return (move >> move_captured_piecetype_offset) & 0x07;
}

static const uint8_t move_promoted_piecetype_offset = 23;
static inline int move_promoted_piecetype(Move move)
{
	return (move >> move_promoted_piecetype_offset) & 0x07;
}

static inline void move_srcdest_form(Move move, char srcdest_form[5])
{
	uint8_t src = move_source_index(move);
	uint8_t dest = move_destination_index(move);

	srcdest_form[0] = board_col_of(src) + 'a';
	srcdest_form[1] = board_row_of(src) + '1';
	srcdest_form[2] = board_col_of(dest) + 'a';
	srcdest_form[3] = board_row_of(dest) + '1';
	srcdest_form[4] = 0;
}

#endif
