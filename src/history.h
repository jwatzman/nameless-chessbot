#ifndef _HISTORY_H
#define _HISTORY_H

#include <stdint.h>

#include "types.h"

void history_clear(void);
void history_update(Move m, int8_t ply);
const Move* history_get_killers(int8_t ply);

#endif
