#include <stdlib.h>
#include "bitboard.h"

int main(int argc, char** argv)
{
	Bitboard *test = malloc(sizeof(Bitboard));
	board_init(test);

	board_print(test->boards);
	board_print(test->boards90);
	board_print(test->boards45);
	board_print(test->boards315);

	free(test);
	return 0;
}
