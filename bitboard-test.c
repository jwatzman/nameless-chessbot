#include "bitboard.h"

int main(int argc, char** argv)
{
	Bitboard test = board_init();
	board_print(test.boards);
	board_print(test.boards90);
	board_print(test.boards45);
	board_print(test.boards315);
	return 0;
}
