#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>

#include "bitboard.h"

int main(void)
{
	printf("uint64_t knight_attacks[64] = {\n");

	for (int position = 0; position < 64; position++)
	{
		uint64_t attack_mask = 0;
		uint64_t bit = 1;
		uint8_t row = board_row_of(position);
		uint8_t col = board_col_of(position);

		// up2 right1
		if (row < 6 && col < 7)
			attack_mask |= bit << board_index_of(row + 2, col + 1);

		// up2 left1
		if (row < 6 && col > 0)
			attack_mask |= bit << board_index_of(row + 2, col - 1);

		// up1 left2
		if (row < 7 && col > 1)
			attack_mask |= bit << board_index_of(row + 1, col - 2);

		// down1 left2
		if (row > 0 && col > 1)
			attack_mask |= bit << board_index_of(row - 1, col - 2);

		// down2 left1
		if (row > 1 && col > 0)
			attack_mask |= bit << board_index_of(row - 2, col - 1);

		// down2 right1
		if (row > 1 && col < 7)
			attack_mask |= bit << board_index_of(row - 2, col + 1);

		// down1 right2
		if (row > 0 && col < 6)
			attack_mask |= bit << board_index_of(row - 1, col + 2);

		// up1 right2
		if (row < 7 && col < 6)
			attack_mask |= bit << board_index_of(row + 1, col + 2);

		printf("0x%.16"PRIx64, attack_mask);
		if (position < 63) printf(", ");
	}

	printf("\n};\n");
}
