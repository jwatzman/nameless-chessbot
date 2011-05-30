#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdint.h>
#include "bitboard.h"
#include "move.h"
#include "bitscan.h"

/* to get the rotated index of the unrotated index 0 <= n < 64, you would
   say board_index_90[n] for example. Also, remember that these are
   upside-down, since a1/LSB must come first */
static const uint8_t board_rotation_index_90[64] = {
0, 8,  16, 24, 32, 40, 48, 56,
1, 9,  17, 25, 33, 41, 49, 57,
2, 10, 18, 26, 34, 42, 50, 58,
3, 11, 19, 27, 35, 43, 51, 59,
4, 12, 20, 28, 36, 44, 52, 60,
5, 13, 21, 29, 37, 45, 53, 61,
6, 14, 22, 30, 38, 46, 54, 62,
7, 15, 23, 31, 39, 47, 55, 63
};

static const uint8_t board_rotation_index_45[64] = {
0,  2,  5,  9,  14, 20, 27, 35, 
1,  4,  8,  13, 19, 26, 34, 42,
3,  7,  12, 18, 25, 33, 41, 48,
6,  11, 17, 24, 32, 40, 47, 53,
10, 16, 23, 31, 39, 46, 52, 57,
15, 22, 30, 38, 45, 51, 56, 60,
21, 29, 37, 44, 50, 55, 59, 62,
28, 36, 43, 49, 54, 58, 61, 63
};

static const uint8_t board_rotation_index_315[64] = {
28, 21, 15, 10, 6,  3,  1,  0,
36, 29, 22, 16, 11, 7,  4,  2,
43, 37, 30, 23, 17, 12, 8,  5,
49, 44, 38, 31, 24, 18, 13, 9,
54, 50, 45, 39, 32, 25, 19, 14,
58, 55, 51, 46, 40, 33, 26, 20,
61, 59, 56, 52, 47, 41, 34, 27,
63, 62, 60, 57, 53, 48, 42, 35
};

// generate a 64-bit random number
static inline uint64_t board_rand64(void);

// generate a bunch of random numbers to put in for zobrists
static void board_init_zobrist(Bitboard *board);

/* for rotating board states when creating the board. The rotated versions
   are normally mutated along with the normal state, so this should not
   need to be used when making and undoing moved */
static uint64_t board_rotate_90(uint64_t board);
static uint64_t board_rotate_45(uint64_t board);
static uint64_t board_rotate_315(uint64_t board);
static uint64_t board_rotate_internal(uint64_t board,
		const uint8_t rotation_index[64]);

// common bits of making and undoing moves that can be easily factored out
static void board_doundo_move_common(Bitboard *board, Move move);
static void board_toggle_piece(Bitboard *board, Piecetype piece,
		Color color, uint8_t loc);

void board_init(Bitboard *board)
{
	board_init_with_fen(board,
			"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

void board_init_with_fen(Bitboard *board, char *fen)
{
	memset(board->boards, 0, 2*6*sizeof(uint64_t)); // clear out boards

	int row = 7;
	while (row >= 0)
	{
		int col = 0;
		while (col < 8)
		{
			if (*fen >= '1' && *fen <= '8')
			{
				// skip that number of slots
				col += (*fen - '0');
			}
			else
			{
				// figure out what piece to put here
				Color color = WHITE;
				Piecetype piece;

				switch (*fen)
				{
					// fall through on the black ones
					case 'p': color = BLACK;
					case 'P': piece = PAWN; break;

					case 'n': color = BLACK;
					case 'N': piece = KNIGHT; break;

					case 'b': color = BLACK;
					case 'B': piece = BISHOP; break;

					case 'r': color = BLACK;
					case 'R': piece = ROOK; break;

					case 'q': color = BLACK;
					case 'Q': piece = QUEEN; break;

					case 'k': color = BLACK;
					case 'K': piece = KING; break;

					default: piece = KING; break; // bad fen
				}

				// set the proper bit
				board->boards[color][piece] |=
					(1ULL << board_index_of(row, col));

				col++;
			}

			fen++; // go to the next letter
		}

		fen++; // skip the slash or space
		row--; // next row
	}

	// calculate the composite and rotated boards
	for (Color c = WHITE; c <= BLACK; c++)
	{
		board->composite_boards[c] = 0;
		for (Piecetype p = PAWN; p <= KING; p++)
		{
			board->composite_boards[c] |= board->boards[c][p];
		}
	}

	board->full_composite =
		board->composite_boards[WHITE] | board->composite_boards[BLACK];
	board->full_composite_45 = board_rotate_45(board->full_composite);
	board->full_composite_90 = board_rotate_90(board->full_composite);
	board->full_composite_315 = board_rotate_315(board->full_composite);

	// w or b to move
	board->to_move = (*fen == 'w' ? WHITE : BLACK);
	fen += 2; // skip the w or b and then the space

	/* castling rights. Assume no one has castled, since we can't know
	   otherwise */
	board->castle_status = 0;
	if (*fen != '-')
	{
		while (*fen != ' ')
		{
			/* the offset into castle_status is 4 for white KS. It
			   increases from that as below */
			int index = 4;
			switch (*fen)
			{
				case 'q': index++;
				case 'Q': index++;
				case 'k': index++;
				case 'K': break;
				default: break; // bad fen
			}

			board->castle_status |= (1ULL << index);

			fen++; // next castling bit
		}
	}
	else
		fen++; // skip the dash

	fen++; // skip the space

	// enpassant index
	if (*fen != '-')
	{
		int col = *fen - 'a';
		fen++;
		int row = *fen - '1';
		fen++;

		// the board keeps a different notion of enpassant row than FEN
		if (row == 2)
			row = 3;
		else if (row == 5)
			row = 4;

		board->enpassant_index = board_index_of(row, col);
	}
	else
	{
		board->enpassant_index = 0;
		fen++; // skip the dash
	}

	/* this is hax. The space does not need to be skipped, since strtol
	   will ignore it. However we don't have any of the board history,
	   but the halfmove count says how far back we need to check the
	   history. So zero it out, which will hopefully not conencide
	   with any used zobrist */
	board->halfmove_count = strtol(fen, NULL, 10);
	memset(board->history, 0, 256*sizeof(uint64_t));

	// set up the mess of zobrist random numbers and the rest of the state
	board_init_zobrist(board);
	board->undo_index = 0;
	board->history_index = 0;
	board->history[0] = board->zobrist;
}

static inline uint64_t board_rand64()
{
	// OR two 32-bit randoms together
	return (((uint64_t)random()) << 32) | ((uint64_t)random());
}

static void board_init_zobrist(Bitboard *board)
{
	srandom(time(0));

	// initially start with a random zobrist
	board->zobrist = board_rand64();

	// fill in each color/piece/position combination with a random mask
	for (int i = 0; i < 2; i++)
		for (int j = 0; j < 6; j++)
			for (int k = 0; k < 64; k++)
				board->zobrist_pos[i][j][k] = board_rand64();

	// same for each castle status ...
	for (int i = 0; i < 256; i++)
		board->zobrist_castle[i] = board_rand64();

	// ... and enpassant index ...
	for (int i = 0; i < 64; i++)
		board->zobrist_enpassant[i] = board_rand64();
}

static uint64_t board_rotate_90(uint64_t board)
{
	return board_rotate_internal(board, board_rotation_index_90);
}

static uint64_t board_rotate_45(uint64_t board)
{
	return board_rotate_internal(board, board_rotation_index_45);
}

static uint64_t board_rotate_315(uint64_t board)
{
	return board_rotate_internal(board, board_rotation_index_315);
}

static uint64_t board_rotate_internal(uint64_t board,
		const uint8_t rotation_index[64])
{
	uint64_t result = 0;
	static const uint64_t bit = 1;
	uint8_t current_index = 0;

	// scan down each bit, flipping the right bit in the rotated result
	while (board)
	{
		if (board & bit)
			result |= bit << rotation_index[current_index];

		board >>= 1;
		current_index++;
	}

	return result;
}

void board_do_move(Bitboard *board, Move move)
{
	/* save data to undo ring buffer. 0x3F == 63. Max value for
	   enpassant_index is 63, max value for halfmove_count is 50 */
	uint64_t undo_data = 0;
	undo_data |= board->enpassant_index & 0x3F;
	undo_data |= ((board->castle_status >> 4) & 0x0F) << 6;
	undo_data |= (board->halfmove_count & 0x3F) << 10;
	undo_data |= ((uint64_t)move) << 32;
	board->undo_ring_buffer[board->undo_index++] = undo_data;

	if (move != MOVE_NULL)
	{
		// halfmove_count is reset on pawn moves or captures
		if (move_piecetype(move) == PAWN || move_is_capture(move))
			board->halfmove_count = 0;
		else
			board->halfmove_count++;

		/* moving to or from a rook square means you can no longer castle on
		   that side */
		uint8_t src = move_source_index(move);
		uint8_t dest = move_destination_index(move);

		board->zobrist ^= board->zobrist_castle[board->castle_status];

		if (src == 0 || dest == 0)
			board->castle_status &= ~(1 << 6); // white can no longer castle QS
		if (src == 7 || dest == 7)
			board->castle_status &= ~(1 << 4); // white can no longer castle KS
		if (src == 56 || dest == 56)
			board->castle_status &= ~(1 << 7); // black can no longer castle QS
		if (src == 63 || dest == 63)
			board->castle_status &= ~(1 << 5); // black can no longer castle KS

		/* moving your king at all means you can no longer castle on either
		   side. Castling also means you can no longer castle (again) on either
		   side */
		if (move_is_castle(move) || move_piecetype(move) == KING)
			board->castle_status &= ~((5 << 4) << move_color(move));

		board->zobrist ^= board->zobrist_castle[board->castle_status];

		/* if src and dest are 16 or -16 units apart (two rows) on a pawn move,
		   update the enpassant index with the destination square. If this
		   didn't happen, clear the enpassant index */
		board->zobrist ^= board->zobrist_enpassant[board->enpassant_index];
		int delta = src - dest;
		if (move_piecetype(move) == PAWN && (delta == 16 || delta == -16))
			board->enpassant_index = dest;
		else
			board->enpassant_index = 0;
		board->zobrist ^= board->zobrist_enpassant[board->enpassant_index];

		// common bits of doing and undoing moves (bulk of the logic in here)
		board_doundo_move_common(board, move);
	}
	else
		board->to_move = (1 - board->to_move);

	// write new zobrist as a game state in the history
	board->history[board->history_index++] = board->zobrist;
}

void board_undo_move(Bitboard *board)
{
	uint64_t undo_data = board->undo_ring_buffer[--(board->undo_index)];
	Move move = undo_data >> 32;

	if (move != MOVE_NULL)
	{
		// restore from undo ring buffer
		board->enpassant_index = undo_data & 0x3F;
		board->castle_status &= 0x0F;
		board->castle_status |= ((undo_data >> 6) & 0x0F) << 4;
		board->halfmove_count = (undo_data >> 10) & 0x3F;

		// common bits of doing and undoing moves (bulk of the logic in here)
		board_doundo_move_common(board, move);
	}
	else
		board->to_move = (1 - board->to_move);

	// restore old zobrist
	board->zobrist = board->history[--board->history_index];
}

static void board_doundo_move_common(Bitboard *board, Move move)
{
	// extract basic data
	uint8_t src = move_source_index(move);
	uint8_t dest = move_destination_index(move);
	Piecetype piece = move_piecetype(move);
	Color color = move_color(move);

	// remove piece at source
	board_toggle_piece(board, piece, color, src);

	// add piece at destination
	if (!move_is_promotion(move))
		board_toggle_piece(board,  piece, color, dest);
	else
		board_toggle_piece(board, move_promoted_piecetype(move),
				color, dest);

	// remove captured piece, if applicable
	if (move_is_capture(move))
		board_toggle_piece(board, move_captured_piecetype(move),
				1 - color, dest);

	/* put the rook in the proper place for a castle and set the right
	   bits in the castle info */
	if (move_is_castle(move))
	{
		if (board_col_of(dest) == 2) // queenside castle
		{
			board_toggle_piece(board, ROOK, color, dest - 2);
			board_toggle_piece(board, ROOK, color, dest + 1);

			board->zobrist ^= board->zobrist_castle[board->castle_status];
			board->castle_status ^= (4 << color);
			board->zobrist ^= board->zobrist_castle[board->castle_status];
		}
		else // kingside castle
		{
			board_toggle_piece(board, ROOK, color, dest + 1);
			board_toggle_piece(board, ROOK, color, dest - 1);

			board->zobrist ^= board->zobrist_castle[board->castle_status];
			board->castle_status ^= (1 << color);
			board->zobrist ^= board->zobrist_castle[board->castle_status];
		}
	}

	if (move_is_enpassant(move))
	{
		if (color == WHITE) // the captured pawn is one row behind
			board_toggle_piece(board, PAWN, BLACK, dest - 8);
		else // the captured pawn is one row up
			board_toggle_piece(board, PAWN, WHITE, dest + 8);
	}

	board->to_move = (1 - board->to_move);
}

static void board_toggle_piece(Bitboard *board, Piecetype piece,
		Color color, uint8_t loc)
{
	// flip the bit in all of the copies of the board state
	board->boards[color][piece] ^= 1ULL << loc;
	board->composite_boards[color] ^= 1ULL << loc;
	board->full_composite ^= 1ULL << loc;
	board->full_composite_45 ^= 1ULL << board_rotation_index_45[loc];
	board->full_composite_90 ^= 1ULL << board_rotation_index_90[loc];
	board->full_composite_315 ^= 1ULL << board_rotation_index_315[loc];
	board->zobrist ^= board->zobrist_pos[color][piece][loc];
}

int board_in_check(Bitboard *board, Color color)
{
	// said color is in check iff its king is being attacked
	return move_square_is_attacked(board, 1-color,
			bitscan(board->boards[color][KING]));
}

Move board_last_move(Bitboard *board)
{
	return board->undo_ring_buffer[board->undo_index - 1] >> 32;
}

void board_print(Bitboard *board)
{
	char* separator = "-----------------";
	char* template = "| | | | | | | | |  ";
	char* this_line = malloc(sizeof(char) * (strlen(template) + 1));

	puts(separator);

	/* print every row in sequence. Check each color and each type to see
	   if pieces are in that row, filling in the right slot in the template
	   with the sigil. Add the row number after each row, and print
	   column letters after each column. */
	for (int row = 7; row >= 0; row--)
	{
		strcpy(this_line, template);

		for (int color = 0; color <= 1; color++)
		{
			for (int type = 0; type <= 5; type++)
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
				default: sigil = '?'; break;
				}

				if (color == BLACK)
					sigil = tolower(sigil);

				int column = 0;
				uint8_t bits =
					(uint8_t)((board->boards[color][type] >> (row << 3))
							& 0xFF);

				while (bits)
				{
					if (bits & 0x01)
						this_line[column*2 + 1] = sigil;

					bits >>= 1;
					column++;
				}

				this_line[18] = '1' + row;
			}
		}

		puts(this_line);
	}

	puts(separator);
	puts(" a b c d e f g h ");

	free(this_line);

	printf("Enpassant index: %x\tHalfmove count: %x\tCastle status: %x\n",
			board->enpassant_index, board->halfmove_count,
			board->castle_status);
	printf("Zobrist: %.16llx\n", board->zobrist);
	printf("To move: %i\n", board->to_move);
}
