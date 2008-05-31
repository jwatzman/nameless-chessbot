#include <stdlib.h>
#include <stdio.h>
#include "bitboard.h"
#include "move.h"

int main(int argc, char** argv)
{
	Bitboard *test = malloc(sizeof(Bitboard));
	board_init(test);
	
	Move test_move = 0;
	test_move |= board_index_of(1, 4);
	test_move |= board_index_of(3, 4) << 6;
	test_move |= PAWN << 12;
	test_move |= WHITE << 15;

	char* srcdest_form = malloc(5 * sizeof(char));
	move_srcdest_form(test_move, srcdest_form);
	printf("%s\n", srcdest_form);
	free(srcdest_form);

	board_do_move(test, test_move);

	board_print(test->boards);
	board_print(test->boards90);
	board_print(test->boards45);
	board_print(test->boards315);

	free(test);
	return 0;
}
