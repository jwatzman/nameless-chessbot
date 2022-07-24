#ifndef _NNUE_H
#define _NNUE_H

#include <stdint.h>

#include "config.h"
#include "types.h"

#if ENABLE_NNUE
void nnue_init(void);
void nnue_reset(Bitboard* board);
int16_t nnue_evaluate(const Bitboard* board);
int16_t nnue_debug_evaluate(const Bitboard* board);

void nnue_toggle_piece(Bitboard* board,
                       Piecetype piece,
                       Color color,
                       uint8_t loc,
                       int activate);
#endif

#endif
