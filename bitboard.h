#include <stdint.h>

// for all of these conversions, 0 <= row,col < 8
static inline uint8_t bb_index_of(uint8_t row, uint8_t col)
{
	return row | (col << 3);
}

static inline uint8_t bb_row_of(uint8_t index)
{
	return index % 8;
}

static inline uint8_t bb_col_of(uint8_t index)
{
	return (uint8_t)(index / 8);
}
