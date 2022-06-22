#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "bitboard.h"
#include "move.h"
#include "perftfn.h"

int main(int argc, char** argv)
{
	if (argc < 2 || argc > 3)
	{
		printf("Usage: ./perft depth [fen]\n");
		return 1;
	}

	int max_depth = strtol(argv[1], NULL, 10);
	if (max_depth < 1)
	{
		printf("Invalid depth: %s\n", argv[1]);
		return 1;
	}

	char *fen = NULL;
	if (argc == 3)
	{
		fen = argv[2];
	}

	move_init();
	srandom(time(NULL));
	Bitboard *board = malloc(sizeof(Bitboard));

	State s;
	if (fen)
		board_init_with_fen(board, &s, fen);
	else
		board_init(board, &s);

	printf("initial zobrist %.16"PRIx64"\n", board->zobrist);

	uint64_t nodes = perft(board, max_depth);

	printf("final zobrist %.16"PRIx64"\n%"PRIu64" nodes\n", board->zobrist, nodes);

	struct rusage usage;
	getrusage(RUSAGE_SELF, &usage);
	double elapsed_time = (double)(usage.ru_utime.tv_sec + usage.ru_stime.tv_sec) + (1.0e-6)*(usage.ru_utime.tv_usec + usage.ru_stime.tv_usec);
	printf("Completed in %0.2f seconds\n", elapsed_time);

	free(board);
	return 0;
}
