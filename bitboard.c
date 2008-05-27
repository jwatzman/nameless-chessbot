#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include "bitboard.h"

Bitboard board_init()
{
	Bitboard result;

	// TODO: init all the other fields
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

	return result;
}

uint64_t board_rotate_90(uint64_t board)
{
}

uint64_t board_rotate_45(uint64_t board)
{
}

uint64_t board_rotate_315(uint64_t board)
{
}

void board_print(Bitboard board)
{
	char* separator = "-----------------\n";
	char* template = "| | | | | | | | |\n";
	char* this_line = malloc(sizeof(char) * strlen(template));

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

				uint8_t bits = (uint8_t)((board.boards[color][type] >> (row << 3)) & 0xFF);
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
