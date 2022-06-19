#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>

#include "bitboard.h"
#include "move.h"
#include "search.h"
#include "timer.h"

typedef struct {
	const char *fen;
	const char *move;
} TestCase;

static const TestCase cases[] =
{
	// https://www.chessprogramming.org/Bill_Gosper#HAKMEM70
	{"5B2/6P1/1p6/8/1N6/kP6/2K5/8 w - -", "g7g8n"},

	// https://www.chessprogramming.org/Lasker-Reichhelm_Position
	{"8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - -", "a1b1"},

	{NULL, NULL},
};

int main(void)
{
	move_init();
	timer_init("level 0 1 30");
	srandom(0);

	Bitboard board;
	for (const TestCase *tcase = cases; tcase->fen != NULL; tcase++)
	{
		printf("CASE: \"%s\" expect %s\n", tcase->fen, tcase->move);

		board_init_with_fen(&board, tcase->fen);
		Move m = search_find_move(&board);

		char buf[6];
		move_srcdest_form(m, buf);
		if (strcmp(buf, tcase->move) != 0)
		{
			printf("FAIL: got %s\n", buf);
			return 1;
		}

		printf("PASS\n");
	}


	struct rusage usage;
	getrusage(RUSAGE_SELF, &usage);
	double elapsed_time = (double)(usage.ru_utime.tv_sec + usage.ru_stime.tv_sec) + (1.0e-6)*(usage.ru_utime.tv_usec + usage.ru_stime.tv_usec);
	printf("Completed in %0.2f seconds\n", elapsed_time);

	return 0;
}