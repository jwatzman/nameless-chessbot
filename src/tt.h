#ifndef _TT_H
#define _TT_H

#include <inttypes.h>

#include "types.h"

#define TRANSPOSITION_EXACT 0
#define TRANSPOSITION_ALPHA 1
#define TRANSPOSITION_BETA 2
typedef uint8_t TranspositionType;

struct TranspositionNode;
typedef struct TranspositionNode TranspositionNode;

void tt_init(void);

const TranspositionNode* tt_get(uint64_t zobrist);
int tt_value(const TranspositionNode* n);
Move tt_move(const TranspositionNode* n);
TranspositionType tt_type(const TranspositionNode* n);
int8_t tt_depth(const TranspositionNode* n);

/*
// If we have an appropriate value for the given parameters, return it;
int tt_get_value(uint64_t zobrist, int alpha, int beta, int8_t depth);

// If we have a previous best move, return it; otherwise, return MOVE_NULL.
Move tt_get_best_move(uint64_t zobrist);
*/

// add to transposition table
void tt_put(uint64_t zobrist,
            int value,
            Move best_move,
            TranspositionType type,
            uint16_t generation,
            int8_t depth);

#endif
