#ifndef _MOVE_H
#define _MOVE_H

#include <stdint.h>
#include "types.h"

static inline uint8_t move_source_index(Move move)
{
	return move & 0x3F;
}

static inline uint8_t move_destination_index(Move move)
{
	return (move >> 6) & 0x3F;
}

static inline Piecetype move_piecetype(Move move)
{
	return (move >> 12) & 0x07;
}

static inline Color move_color(Move move)
{
	return (move >> 15) & 0x01;
}

static inline int move_is_castle(Move move)
{
	return (move >> 16) & 0x01;
}

static inline int move_is_enpassant(Move move)
{
	return (move >> 17) & 0x01;
}

static inline int move_is_capture(Move move)
{
	return (move >> 18) & 0x01;
}

static inline int move_is_promotion(Move move)
{
	return (move >> 19) & 0x01;
}

static inline int move_captured_piecetype(Move move)
{
	return (move >> 20) & 0x07;
}

static inline int move_promoted_piecetype(Move move)
{
	return (move >> 23) & 0x07;
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
