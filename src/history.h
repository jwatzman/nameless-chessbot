#ifndef _HISTORY_H
#define _HISTORY_H

#include <stdint.h>

#include "types.h"

void history_clear(void);
void history_update(const Bitboard* board, Move m, int8_t depth, int8_t ply);
const Move* history_get_killers(int8_t ply);
Move history_get_countermove(const Bitboard* board);
uint16_t history_get(Move m);

#endif
