#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	uint64_t brd = strtoull(argv[1], NULL, 0);
	char row;
	int i, j;
	for (i = 7; i >= 0; i--)
	{
		row = (brd >> (i*8)) & 0xFF;
		for (j = 0; j < 8; j++)
		{
			char square;
			square = ((row >> j) & 0x1) ? '#' : '.';
			printf("%c ", square);
		}
		printf("\n");
	}
	return 0;
}