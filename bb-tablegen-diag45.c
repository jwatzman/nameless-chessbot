#include <stdio.h>
#include <stdint.h>
#include "bitboard.h"

uint64_t up_left_attack(int position_mask, int pos)
{
	uint64_t result = 0;

	for (int cur_pos = pos + 1; cur_pos < 8; cur_pos++)
	{
		result |= (1ULL << board_index_of(cur_pos, 7 - cur_pos));

		if (position_mask & (1 << cur_pos))
			break;
	}
	
	return result;
}

uint64_t down_right_attack(int position_mask, int pos)
{
	uint64_t result = 0;

	for (int cur_pos = pos - 1; cur_pos >= 0; cur_pos--)
	{
		result |= (1ULL << board_index_of(cur_pos, 7 - cur_pos));

		if (position_mask & (1 << cur_pos))
			break;
	}

	return result;
}

int main(int argc, char** argv)
{
	printf("uint64_t diag_attacks_45[256][8] = {\n");

	for (int position_mask = 0; position_mask < 256; position_mask++)
	{
		printf("\t{");

		for (int pos = 7; pos >= 0; pos--)
		{
			uint64_t attack_mask = up_left_attack(position_mask, pos);
			attack_mask |= down_right_attack(position_mask, pos);

			printf("0x%.16llx", attack_mask);
			if (pos > 0) printf(", ");
		}

		printf("}");
		if (position_mask < 255) printf(",");
		printf(" // 0x%.2x\n", position_mask);
	}

	printf("};\n");
}
