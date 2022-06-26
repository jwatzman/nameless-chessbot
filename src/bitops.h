#ifndef _BITOPS_H
#define _BITOPS_H

#include <stdint.h>

static inline uint8_t bitscan(uint64_t x) {
  return __builtin_ctzll(x);
}

static inline uint8_t popcnt(uint64_t x) {
  return __builtin_popcountll(x);
}

#endif
