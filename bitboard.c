#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include "bitboard.h"

// to get the rotated index of the unrotated index 0 <= n < 64, you would say
// board_index_90[n] for example
// also, remember that these are upside-down, since a1/LSB must come first
static const uint8_t board_rotation_index_90[64] = {
7, 15, 23, 31, 39, 47, 55, 63,
6, 14, 22, 30, 38, 46, 54, 62,
5, 13, 21, 29, 37, 45, 53, 61,
4, 12, 20, 28, 36, 44, 52, 60,
3, 11, 19, 27, 35, 43, 51, 59,
2, 10, 18, 26, 34, 42, 50, 58,
1, 9, 17, 25, 33, 41, 49, 57,
0, 8, 16, 24, 32, 40, 48, 56
};

static const uint8_t board_rotation_index_45[64] = {
0, 2, 5, 9, 14, 20, 27, 35, 
1, 4, 8, 13, 19, 26, 34, 42,
3, 7, 12, 18, 25, 33, 41, 48,
6, 11, 17, 24, 32, 40, 47, 53,
10, 16, 23, 31, 39, 46, 52, 57,
15, 22, 30, 38, 45, 51, 56, 60,
21, 29, 37, 44, 50, 55, 59, 62,
28, 36, 43, 49, 54, 58, 61, 63
};

static const uint8_t board_rotation_index_315[64] = {};

uint64_t board_rotate_internal(uint64_t board, const uint8_t rotation_index[64]);

Bitboard board_init()
{
	Bitboard result;

	result.boards[WHITE][PAWN] = 0x000000000000FF00;
	result.boards[WHITE][BISHOP] = 0x0000000000000024;
	result.boards[WHITE][KNIGHT] = 0x0000000000000042;
	result.boards[WHITE][ROOK] = 0x0000000000000081;
	result.boards[WHITE][QUEEN] = 0x0000000000000008;
	result.boards[WHITE][KING] = 0x0000000000000010;

	result.boards[BLACK][PAWN] = 0x00FF000000000000;
	result.boards[BLACK][BISHOP] = 0x2400000000000000;
	result.boards[BLACK][KNIGHT] = 0x4200000000000000;
	result.boards[BLACK][ROOK] = 0x8100000000000000;
	result.boards[BLACK][QUEEN] = 0x0800000000000000;
	result.boards[BLACK][KING] = 0x1000000000000000;

	result.composite_boards[WHITE] = 0x000000000000FFFF;
	result.composite_boards[BLACK] = 0xFFFF000000000000;

	for (int color = WHITE; color <= BLACK; color++)
	{
		for (int piece = PAWN; piece <= KING; piece++)
		{
			uint64_t to_be_rotated = result.boards[color][piece];
			result.boards90[color][piece] = board_rotate_90(to_be_rotated);
			result.boards45[color][piece] = board_rotate_45(to_be_rotated);
			result.boards315[color][piece] = board_rotate_315(to_be_rotated);
		}
	}

	result.castle_status = 0xF0; // we can castle, but have not yet

	return result;
}

uint64_t board_rotate_90(uint64_t board)
{
	return board_rotate_internal(board, board_rotation_index_90);
}

uint64_t board_rotate_45(uint64_t board)
{
	return board_rotate_internal(board, board_rotation_index_45);
}

uint64_t board_rotate_315(uint64_t board)
{
	return board_rotate_internal(board, board_rotation_index_315);
}

uint64_t board_rotate_internal(uint64_t board, const uint8_t rotation_index[64])
{
	uint64_t result = 0;
	static const uint64_t bit = 1;
	uint8_t current_index = 0;

	while (board)
	{
		if (board & bit)
			result |= bit << rotation_index[current_index];

		board >>= 1;
		current_index++;
	}

	return result;
}

void board_print(uint64_t boards[2][6])
{
	char* separator = "-----------------\n";
	char* template = "| | | | | | | | |\n";
	char* this_line = malloc(sizeof(char) * (strlen(template) + 1));

	printf(separator);

	for (int row = 7; row >= 0; row--)
	{
		strcpy(this_line, template);

		for (int color = WHITE; color <= BLACK; color++)
		{
			for (int type = PAWN; type <= KING; type++)
			{
				char sigil = 0;

				switch (type)
				{
				case PAWN: sigil = 'P'; break;
				case BISHOP: sigil = 'B'; break;
				case KNIGHT: sigil = 'N'; break;
				case ROOK: sigil = 'R'; break;
				case QUEEN: sigil = 'Q'; break;
				case KING: sigil = 'K'; break;
				}

				if (color == BLACK)
					sigil = tolower(sigil);

				uint8_t bits = (uint8_t)((boards[color][type] >> (row << 3)) & 0xFF);
				int column = 0;
				while (bits)
				{
					if (bits & 0x01)
						this_line[column*2 + 1] = sigil;

					bits >>= 1;
					column++;
				}
			}
		}

		printf(this_line);
	}

	printf(separator);

	free(this_line);
}
