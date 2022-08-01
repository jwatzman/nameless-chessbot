#ifndef _MOVEITER_H
#define _MOVEITER_H

#include <stdint.h>

#include "move.h"
#include "types.h"

typedef int32_t MoveScore;

typedef struct {
  Movelist* list;
  uint8_t n;
  MoveScore scores[MAX_MOVES];
} Moveiter;

// May modify the input list
void moveiter_init(Moveiter* iter,
                   const Bitboard* board,
                   Movelist* list,
                   Move tt_move,
                   const Move* killers);
int moveiter_has_next(Moveiter* iter);
Move moveiter_next(Moveiter* iter, MoveScore* s_out);
int16_t moveiter_score_to_see(MoveScore s);

#endif
