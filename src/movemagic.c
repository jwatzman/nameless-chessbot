#define _GNU_SOURCE
#include <stdint.h>
#include "bitboard.h"
#include "bitscan.h"
#include "movemagic.h"
#include "movemagic-consts.h"

static uint64_t rookdb[64][1<<(64-MIN_R_SHIFT)];
static uint64_t bishopdb[64][1<<(64-MIN_B_SHIFT)];

static void movemagic_init_findsetbits(uint64_t board, uint8_t *setbits, uint8_t *numsetbits)
{
	int i = 0;
	while (board)
	{
		uint8_t bit = bitscan(board);
		board &= board - 1;
		setbits[i] = bit;
		i++;
	}

	*numsetbits = i;
}

/**
 * If you think of setbits as a set of numsetbits elements, then there are 2**numsetbits
 * subsets. This function generates the nth subset, sets those bits, and returns it.
 */
static uint64_t movemagic_init_nth_subset(uint64_t n, uint8_t *setbits, uint8_t numsetbits)
{
	uint64_t ret = 0;
	for (uint8_t i = 0; i < numsetbits; i++)
		if (n & (1ULL << i))
			ret |= (1ULL << setbits[i]);

	return ret;
}

static int8_t add_one(int8_t x) { return x + 1; }
static int8_t sub_one(int8_t x) { return x - 1; }
static int8_t add_zero(int8_t x) { return x; }
static uint64_t movemagic_init_compute_attacks(uint8_t pos, uint64_t occ,
		int8_t (*row_incr)(int8_t), int8_t (*col_incr)(int8_t))
{
	uint64_t attacks = 0;

	int8_t init_row = board_row_of(pos);
	int8_t init_col = board_col_of(pos);

	for (int8_t row = row_incr(init_row), col = col_incr(init_col);
			row < 8 && col < 8 && row >= 0 && col >= 0;
			row = row_incr(row), col = col_incr(col))
	{
		uint64_t bit = 1ULL << board_index_of(row, col);
		attacks |= bit;
		if (occ & bit)
			break;
	}

	return attacks;
}

static uint64_t movemagic_init_rook_attacks(uint8_t pos, uint64_t occ)
{
	return movemagic_init_compute_attacks(pos, occ, add_one, add_zero)
		| movemagic_init_compute_attacks(pos, occ, sub_one, add_zero)
		| movemagic_init_compute_attacks(pos, occ, add_zero, add_one)
		| movemagic_init_compute_attacks(pos, occ, add_zero, sub_one);
}

static uint64_t movemagic_init_bishop_attacks(uint8_t pos, uint64_t occ)
{
	return movemagic_init_compute_attacks(pos, occ, add_one, add_one)
		| movemagic_init_compute_attacks(pos, occ, add_one, sub_one)
		| movemagic_init_compute_attacks(pos, occ, sub_one, add_one)
		| movemagic_init_compute_attacks(pos, occ, sub_one, sub_one);
}

void movemagic_init(void)
{
	uint8_t setbits[64];
	uint8_t numsetbits;

	for (uint8_t pos = 0; pos < 64; pos++)
	{
		movemagic_init_findsetbits(r_mask[pos], setbits, &numsetbits);
		for (uint64_t n = 0; n < (1ULL << numsetbits); n++)
		{
			uint64_t occ = movemagic_init_nth_subset(n, setbits, numsetbits);
			uint64_t attacks = movemagic_init_rook_attacks(pos, occ);
			rookdb[pos][occ*r_magic[pos] >> r_shift[pos]] = attacks;
		}
	}

	for (uint8_t pos = 0; pos < 64; pos++)
	{
		movemagic_init_findsetbits(b_mask[pos], setbits, &numsetbits);
		for (uint64_t n = 0; n < (1ULL << numsetbits); n++)
		{
			uint64_t occ = movemagic_init_nth_subset(n, setbits, numsetbits);
			uint64_t attacks = movemagic_init_bishop_attacks(pos, occ);
			bishopdb[pos][occ*b_magic[pos] >> b_shift[pos]] = attacks;
		}
	}
}

uint64_t movemagic_rook(uint8_t pos, uint64_t occ)
{
	occ = occ & r_mask[pos];
	return rookdb[pos][occ*r_magic[pos] >> r_shift[pos]];
}

uint64_t movemagic_bishop(uint8_t pos, uint64_t occ)
{
	occ = occ & b_mask[pos];
	return bishopdb[pos][occ*b_magic[pos] >> b_shift[pos]];
}
