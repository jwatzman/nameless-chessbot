#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "types.h"
#include "move.h"
#include "bitboard.h"
#include "evaluate.h"
#include "search.h"
#include "timer.h"

static const int max_input_length = 1024;

// returns 0 on illegal move
static Move parse_move(Bitboard *board, char* possible_move)
{
	Movelist moves;
	move_generate_movelist(board, &moves);
	char test[6];

	for (int i = 0; i < moves.num; i++)
	{
		Move m = moves.moves[i];
		move_srcdest_form(m, test);
		if (!move_is_promotion(m) && !strncmp(test, possible_move, 4))
			return m;
		else if (!strncmp(test, possible_move, 5))
			return m;
	}

	return 0;
}

int main(void)
{
	Color computer_player = -1; // not WHITE or BLACK if we don't play either (e.g. -1)
	int game_on = 0;
	Bitboard *board = malloc(sizeof(Bitboard));
	char* input = malloc(sizeof(char) * max_input_length);
	Move last_move = 0;
	int got_move = 0;

	freopen("/dev/tty", "w", stderr);
	setbuf(stderr, NULL);
	setbuf(stdout, NULL);

	while (1)
	{
		if (game_on && got_move)
		{
			board_do_move(board, last_move);
			got_move = 0;
		}

		if (game_on && computer_player == board->to_move)
		{
			last_move = search_find_move(board);
			move_srcdest_form(last_move, input);
			printf("move %s\n", input);
			board_do_move(board, last_move);
		}

		fgets(input, max_input_length, stdin);
		fprintf(stderr, "%s", input);

		if (!strcmp("xboard\n", input))
			printf("feature colors=0 setboard=1 time=0 sigint=0 sigterm=0 variants=\"normal\" done=1\n");
		else if (!strcmp("new\n", input))
		{
			board_init(board);
			computer_player = BLACK;
			game_on = 1;
		}
		else if (!strcmp("quit\n", input))
			break;
		else if (!strcmp("force\n", input))
			computer_player = -1;
		else if (!strcmp("go\n", input))
			computer_player = board->to_move;
		else if (!strncmp("setboard ", input, 9))
			board_init_with_fen(board, input + 9);
		else if (!strncmp("result", input, 6))
		{
			computer_player = -1;
			game_on = 0;
		}
		else if (!strncmp("level ", input, 6))
			timer_init(input);
		else if (!strcmp("_print\n", input))
			board_print(board);
		else if (input[0] >= 'a' && input[0] <= 'h'
				&& input[1] >= '1' && input[1] <= '8'
				&& input[2] >= 'a' && input[2] <= 'h'
				&& input[3] >= '1' && input[3] <= '8')
		{
			last_move = parse_move(board, input);
			if (!last_move)
				printf("Illegal move: %s", input);
			else
				got_move = 1;
		}
		else
			printf("Error (unknown command): %s", input);
	}

	free(board);
	free(input);
	return 0;
}
