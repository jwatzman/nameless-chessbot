#ifndef _TT_H
#define _TT_H

#include <inttypes.h>

#include "types.h"

#define TRANSPOSITION_EXACT 0
#define TRANSPOSITION_ALPHA 1
#define TRANSPOSITION_BETA 2
typedef uint8_t TranspositionType;

/* asks the transposition table if we already know a good value for this
   position. If we do, return it. Otherwise, return INFINITY but adjust
   *alpha and *beta if we know better bounds for them */
int tt_get_value(uint64_t zobrist, int alpha, int beta, int8_t depth);

// if we have a previous best move for this zobrist, return it; 0 otherwise
Move tt_get_best_move(uint64_t zobrist);

// add to transposition table
void tt_put(uint64_t zobrist,
            int value,
            Move best_move,
            TranspositionType type,
            uint16_t generation,
            int8_t depth);

#endif
