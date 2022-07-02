#ifndef _BITOPS_H
#define _BITOPS_H

#include <stdint.h>

static inline uint8_t bitscan(uint64_t x) {
  return (uint8_t)__builtin_ctzll(x);
}

static inline uint8_t popcnt(uint64_t x) {
  return (uint8_t)__builtin_popcountll(x);
}

static inline int twobits(uint64_t x) {
  return (x & (x - 1)) > 0;
}

static inline uint8_t min(uint8_t a, uint8_t b) {
  return a < b ? a : b;
}

static inline uint8_t max(uint8_t a, uint8_t b) {
  return a > b ? a : b;
}

#endif
