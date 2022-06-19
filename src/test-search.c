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
	int ret = 0;

	move_init();
	timer_init_secs(30);
	srandom(0);

	int num_tests = 0;
	Bitboard board;
	for (const TestCase *tcase = cases; tcase->fen != NULL; tcase++)
	{
		num_tests++;

		board_init_with_fen(&board, tcase->fen);
		Move m = search_find_move(&board);

		char buf[6];
		move_srcdest_form(m, buf);
		if (strcmp(buf, tcase->move) == 0)
		{
			printf("ok %d - %s\n", num_tests, tcase->fen);
		} else {
			printf("not ok %d - %s expect %s got %s\n", num_tests, tcase->fen, tcase->move, buf);
			ret = 1;
		}
	}

	struct rusage usage;
	getrusage(RUSAGE_SELF, &usage);
	double elapsed_time = (double)(usage.ru_utime.tv_sec + usage.ru_stime.tv_sec) + (1.0e-6)*(usage.ru_utime.tv_usec + usage.ru_stime.tv_usec);
	fprintf(stderr, "Completed in %0.2f seconds\n", elapsed_time);

	printf("1..%d\n", num_tests);

	return ret;
}
