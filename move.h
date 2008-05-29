#ifndef _MOVE_H
#define _MOVE_H

#include <stdint.h>
#include "bitboard.h"

// moves are stored in 30 bits
// from LSB to MSB:
// source square (6)
// destination square (6)
// type (3)
// color (1)
// is castle (1)
// is en passant (1)
// is capture (1)
// is promotion (1)
// captured type (3)
// promoted type (3)
// undo data for board [castling rights] (4)
// which leaves the two MSB unused
typedef uint32_t Move;

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

#endif
