#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>

#include "../../bitboard.h"

uint64_t front_span_white(int pos)
{
	uint64_t result = 0;

	int col = board_col_of(pos);
	for (int row = board_row_of(pos) + 1; row < 8; row++)
	{
		result |= (1ULL << board_index_of(row, col));

		if (col - 1 >= 0)
			result |= (1ULL << board_index_of(row, col - 1));
		if (col + 1 < 8)
			result |= (1ULL << board_index_of(row, col + 1));
	}

	return result;
}

uint64_t front_span_black(int pos)
{
	uint64_t result = 0;

	int col = board_col_of(pos);
	for (int row = board_row_of(pos) - 1; row >= 0; row--)
	{
		result |= (1ULL << board_index_of(row, col));

		if (col - 1 >= 0)
			result |= (1ULL << board_index_of(row, col - 1));
		if (col + 1 < 8)
			result |= (1ULL << board_index_of(row, col + 1));
	}

	return result;
}

int main(void)
{
	printf("uint64_t front_spans[2][64] = {\n");

	printf("{\n\t");

	for (int pos = 0; pos < 64; pos++)
	{
		uint64_t front_span = front_span_white(pos);
		printf("0x%.16"PRIx64, front_span);

		if (pos < 63)
		{
			printf(",");

			if (pos % 8 == 7)
				printf("\n\t");
			else
				printf(" ");
		}
	}

	printf("\n},\n{\n\t");

	for (int pos = 0; pos < 64; pos++)
	{
		uint64_t front_span = front_span_black(pos);
		printf("0x%.16"PRIx64, front_span);

		if (pos < 63)
		{
			printf(",");

			if (pos % 8 == 7)
				printf("\n\t");
			else
				printf(" ");
		}
	}

	printf("\n}\n};\n");
}
