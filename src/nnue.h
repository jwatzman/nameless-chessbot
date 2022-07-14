#ifndef _NNUE_H
#define _NNUE_H

#include <stdint.h>

#include "types.h"

void nnue_init(void);
int16_t nnue_evaluate(Bitboard* board);

#endif
