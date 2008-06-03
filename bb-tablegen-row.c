#include <stdio.h>
#include <stdint.h>

uint8_t left_attack(int position_mask, int col)
{
	uint8_t result = 0;
	for (int cur_col = col + 1; cur_col < 8; cur_col++)
	{
		uint8_t cur_bit = 1 << cur_col;
		result |= cur_bit;

		if (position_mask & cur_bit)
			break;
	}

	return result;
}

uint8_t right_attack(int position_mask, int col)
{
	uint8_t result = 0;
	for (int cur_col = col - 1; cur_col >= 0; cur_col--)
	{
		uint8_t cur_bit = 1 << cur_col;
		result |= cur_bit;

		if (position_mask & cur_bit)
			break;
	}

	return result;
}

int main(int argc, char** argv)
{
	printf("uint64_t row_attacks[256][8] = {\n");

	for (int position_mask = 0; position_mask < 256; position_mask++)
	{
		printf("\t{");

		for (int col = 0; col < 8; col++)
		{
			uint8_t attack_mask = left_attack(position_mask, col);
			attack_mask |= right_attack(position_mask, col);

			printf("0x%.2x", attack_mask);
			if (col < 7) printf(", ");
		}

		printf("}");
		if (position_mask < 255) printf(",");
		printf(" // 0x%.2x\n", position_mask);
	}

	printf("};\n");
}
