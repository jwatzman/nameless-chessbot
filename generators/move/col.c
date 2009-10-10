#include <stdio.h>
#include <stdint.h>

uint64_t up_attack(int position_mask, int row)
{
	uint64_t result = 0;

	for (int cur_row = row + 1; cur_row < 8; cur_row++)
	{
		result |= (1ULL << (cur_row << 3));

		if (position_mask & (1 << cur_row))
			break;
	}
	
	return result;
}

uint64_t down_attack(int position_mask, int row)
{
	uint64_t result = 0;

	for (int cur_row = row - 1; cur_row >= 0; cur_row--)
	{
		result |= (1ULL << (cur_row << 3));

		if (position_mask & (1 << cur_row))
			break;
	}

	return result;
}

int main(void)
{
	printf("uint64_t col_attacks[256][8] = {\n");

	for (int position_mask = 0; position_mask < 256; position_mask++)
	{
		printf("\t{");

		for (int row = 0; row < 8; row++)
		{
			uint64_t attack_mask = up_attack(position_mask, row);
			attack_mask |= down_attack(position_mask, row);

			printf("0x%.16llx", attack_mask);
			if (row < 7) printf(", ");
		}

		printf("}");
		if (position_mask < 255) printf(",");
		printf(" // 0x%.2x\n", position_mask);
	}

	printf("};\n");
}
