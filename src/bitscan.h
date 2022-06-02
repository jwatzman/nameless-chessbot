#ifndef _BITSCAN_H
#define _BITSCAN_H

#include <stdint.h>

static inline uint8_t bitscan(uint64_t x)
{
	return __builtin_ctzll(x);
}

#endif
