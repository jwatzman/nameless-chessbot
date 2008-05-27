#include <stdio.h>
#include <stdint.h>
#include "bitboard.h"

int main(int argc, char** argv)
{
	printf("uint64_t king_attacks[64] = {\n");

	for (int position = 0; position < 64; position++)
	{
		uint64_t attack_mask = 0;
		uint64_t bit = 1;
		uint8_t row = board_row_of(position);
		uint8_t col = board_col_of(position);

		// up
		if (row < 7)
			attack_mask |= bit << board_index_of(row + 1, col);

		// up-left
		if (row < 7 && col > 0)
			attack_mask |= bit << board_index_of(row + 1, col - 1);

		// left
		if (col > 0)
			attack_mask |= bit << board_index_of(row, col - 1);

		// down-left
		if (row > 0 && col > 0)
			attack_mask |= bit << board_index_of(row - 1, col - 1);

		// down
		if (row > 0)
			attack_mask |= bit << board_index_of(row - 1, col);

		// down-right
		if (row > 0 && col < 7)
			attack_mask |= bit << board_index_of(row - 1, col + 1);

		// right
		if (col < 7)
			attack_mask |= bit << board_index_of(row, col + 1);

		// up-right
		if (row < 7 && col < 7)
			attack_mask |= bit << board_index_of(row + 1, col + 1);

		printf("0x%.16llx", attack_mask);
		if (position < 63) printf(", ");
	}

	printf("\n};\n");
}
