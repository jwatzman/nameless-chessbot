#ifndef _HISTORY_H
#define _HISTORY_H

#include <stdint.h>

#include "types.h"

void history_clear(void);
void history_update(const Bitboard* board,
                    Move best,
                    const Move* bad,
                    int num_bad,
                    int8_t depth,
                    int8_t ply);
const Move* history_get_killers(int8_t ply);
Move history_get_countermove(const Bitboard* board);
int16_t history_get_combined(const Bitboard* board, Move m);

#endif
