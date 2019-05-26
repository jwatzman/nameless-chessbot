#ifndef _MOVEMAGIC_H
#define _MOVEMAGIC_H

#include <stdint.h>

void movemagic_init(void);
uint64_t movemagic_rook(uint8_t pos, uint64_t occ);
uint64_t movemagic_bishop(uint8_t pos, uint64_t occ);

#endif
