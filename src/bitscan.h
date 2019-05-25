#ifndef _BITSCAN_H
#define _BITSCAN_H

#include <stdint.h>

// amd64 only
static inline uint8_t bitscan(uint64_t x)
{
	uint64_t r;
	__asm__ __volatile__ ("bsfq %1, %0;" : "=r"(r) : "r"(x));
	return r;
}

// for other architectures, try "return ffsll(x) - 1"

#endif
