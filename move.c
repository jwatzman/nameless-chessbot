#include <stdint.h>
#include "types.h"
#include "move.h"
#include "move-generated.h"
#include "bitboard.h"

static void move_generate_movelist_pawn(Bitboard *board, Movelist *movelist);
static void move_generate_movelist_knight(Bitboard *board, Movelist *movelist);
static void move_generate_movelist_king(Bitboard *board, Movelist *movelist);
static void move_generate_movelist_rook(Bitboard *board, Movelist *movelist);
static void move_generate_movelist_bishop(Bitboard *board, Movelist *movelist);
static void move_generate_movelist_queen(Bitboard *board, Movelist *movelist);
static void move_generate_movelist_castle(Bitboard *board, Movelist *movelist);
static void move_generate_movelist_enpassant(Bitboard *board, Movelist *movelist);

// these expect the composite_board to be of the properly rotated variant
static uint64_t move_generate_attacks_row(uint64_t composite_board, uint8_t index);
static uint64_t move_generate_attacks_col(uint64_t composite_board, uint8_t index);
static uint64_t move_generate_attacks_diag45(uint64_t composite_board, uint8_t index);
static uint64_t move_generate_attacks_diag315(uint64_t composite_board, uint8_t index);


void move_generate_movelist(Bitboard *board, Movelist *movelist)
{
	movelist->num = 0;

	move_generate_movelist_pawn(board, movelist);
	move_generate_movelist_knight(board, movelist);
	move_generate_movelist_king(board, movelist);

	move_generate_movelist_rook(board, movelist);
	move_generate_movelist_bishop(board, movelist);
	move_generate_movelist_queen(board, movelist);

	move_generate_movelist_castle(board, movelist);
	move_generate_movelist_enpassant(board, movelist);
}

int move_square_is_attacked(Bitboard *board, Color attacker, uint8_t square)
{
	return 0;
}

static void move_generate_movelist_pawn(Bitboard *board, Movelist *movelist)
{
	Color to_move = board->to_move;
	uint64_t pawns = board->boards[to_move][PAWN];
	uint8_t src = 0;

	while (pawns)
	{
		if (pawns & 1ULL)
		{
			uint8_t row = board_row_of(src);
			uint8_t col = board_col_of(src);

			// try to move one space forward
			if ((to_move == WHITE && row < 7) || (to_move == BLACK && row > 0))
			{
				uint8_t dest;
				if (to_move == WHITE) dest = board_index_of(row + 1, col);
				else dest = board_index_of(row - 1, col);

				if (!((board->composite_boards[0] | board->composite_boards[1]) & (1ULL << dest)))
				{
					Move move = 0;
					move |= src << move_source_index_offset;
					move |= dest << move_destination_index_offset;
					move |= PAWN << move_piecetype_offset;
					move |= to_move << move_color_offset;

					// promote if needed
					if ((to_move == WHITE && row == 6) || (to_move == BLACK && row == 1))
					{
						move |= 1ULL << move_is_promotion_offset;

						movelist->moves[movelist->num++] = (move | (QUEEN << move_promoted_piecetype_offset));
						movelist->moves[movelist->num++] = (move | (ROOK << move_promoted_piecetype_offset));
						movelist->moves[movelist->num++] = (move | (BISHOP << move_promoted_piecetype_offset));
						movelist->moves[movelist->num++] = (move | (KNIGHT << move_promoted_piecetype_offset));
					}
					else
						movelist->moves[movelist->num++] = move;
				}
			}

			// try to move two spaces forward
			if ((to_move == WHITE && row == 1) || (to_move == BLACK && row == 6))
			{
				uint8_t dest;
				if (to_move == WHITE) dest = board_index_of(row + 2, col);
				else dest = board_index_of(row - 2, col);

				if (!((board->composite_boards[0] | board->composite_boards[1]) & (1ULL << dest)))
				{
					Move move = 0;
					move |= src << move_source_index_offset;
					move |= dest << move_destination_index_offset;
					move |= PAWN << move_piecetype_offset;
					move |= to_move << move_color_offset;

					movelist->moves[movelist->num++] = move;
				}
			}

			// try to capture left
			if (((to_move == WHITE && row < 7) || (to_move == BLACK && row > 0)) && col > 0)
			{
				uint8_t dest;
				if (to_move == WHITE) dest = board_index_of(row + 1, col - 1);
				else dest = board_index_of(row - 1, col - 1);

				if (board->composite_boards[1-to_move] & (1ULL << dest))
				{
					Move move = 0;
					move |= src << move_source_index_offset;
					move |= dest << move_destination_index_offset;
					move |= PAWN << move_piecetype_offset;
					move |= to_move << move_color_offset;
					move |= 1ULL << move_is_capture_offset;
					move |= board_piecetype_at_index(board, dest) << move_captured_piecetype_offset;

					// promote if needed
					if ((to_move == WHITE && row == 6) || (to_move == BLACK && row == 1))
					{
						move |= 1ULL << move_is_promotion_offset;

						movelist->moves[movelist->num++] = (move | (QUEEN << move_promoted_piecetype_offset));
						movelist->moves[movelist->num++] = (move | (ROOK << move_promoted_piecetype_offset));
						movelist->moves[movelist->num++] = (move | (BISHOP << move_promoted_piecetype_offset));
						movelist->moves[movelist->num++] = (move | (KNIGHT << move_promoted_piecetype_offset));
					}
					else
						movelist->moves[movelist->num++] = move;
				}
			}

			// try to capture right
			if (((to_move == WHITE && row < 7) || (to_move == BLACK && row > 0)) && col < 7)
			{
				uint8_t dest;
				if (to_move == WHITE) dest = board_index_of(row + 1, col + 1);
				else dest = board_index_of(row - 1, col + 1);

				if (board->composite_boards[1-to_move] & (1ULL << dest))
				{
					Move move = 0;
					move |= src << move_source_index_offset;
					move |= dest << move_destination_index_offset;
					move |= PAWN << move_piecetype_offset;
					move |= to_move << move_color_offset;
					move |= 1ULL << move_is_capture_offset;
					move |= board_piecetype_at_index(board, dest) << move_captured_piecetype_offset;

					// promote if needed
					if ((to_move == WHITE && row == 6) || (to_move == BLACK && row == 1))
					{
						move |= 1ULL << move_is_promotion_offset;

						movelist->moves[movelist->num++] = (move | (QUEEN << move_promoted_piecetype_offset));
						movelist->moves[movelist->num++] = (move | (ROOK << move_promoted_piecetype_offset));
						movelist->moves[movelist->num++] = (move | (BISHOP << move_promoted_piecetype_offset));
						movelist->moves[movelist->num++] = (move | (KNIGHT << move_promoted_piecetype_offset));
					}
					else
						movelist->moves[movelist->num++] = move;
				}
			}
		}

		pawns >>= 1;
		src++;
	}
}

static void move_generate_movelist_knight(Bitboard *board, Movelist *movelist)
{
	Color to_move = board->to_move;
	uint64_t knights = board->boards[to_move][KNIGHT];
	uint8_t src = 0;

	while (knights)
	{
		if (knights & 1ULL)
		{
			uint64_t dests = knight_attacks[src];
			dests &= ~(board->composite_boards[to_move]);
			uint8_t dest = 0;

			while (dests)
			{
				if (dests & 1ULL)
				{
					Move move = 0;
					move |= src << move_source_index_offset;
					move |= dest << move_destination_index_offset;
					move |= KNIGHT << move_piecetype_offset;
					move |= to_move << move_color_offset;

					if (board->composite_boards[1-to_move] & (1ULL << dest))
					{
						move |= 1ULL << move_is_capture_offset;
						move |= board_piecetype_at_index(board, dest) << move_captured_piecetype_offset;
					}

					movelist->moves[movelist->num++] = move;
				}

				dest++;
				dests >>= 1;
			}
		}

		knights >>= 1;
		src++;
	}
}

static void move_generate_movelist_king(Bitboard *board, Movelist *movelist)
{
	Color to_move = board->to_move;
	uint64_t kings = board->boards[to_move][KING];
	uint8_t src = 0;

	while (kings)
	{
		if (kings & 1ULL)
		{
			uint64_t dests = king_attacks[src];
			dests &= ~(board->composite_boards[to_move]);
			uint8_t dest = 0;

			while (dests)
			{
				if (dests & 1ULL)
				{
					Move move = 0;
					move |= src << move_source_index_offset;
					move |= dest << move_destination_index_offset;
					move |= KING << move_piecetype_offset;
					move |= to_move << move_color_offset;

					if (board->composite_boards[1-to_move] & (1ULL << dest))
					{
						move |= 1ULL << move_is_capture_offset;
						move |= board_piecetype_at_index(board, dest) << move_captured_piecetype_offset;
					}

					movelist->moves[movelist->num++] = move;
				}

				dest++;
				dests >>= 1;
			}
		}

		kings >>= 1;
		src++;
	}
}

static void move_generate_movelist_rook(Bitboard *board, Movelist *movelist)
{
	Color to_move = board->to_move;
	uint64_t rooks = board->boards[to_move][ROOK];
	uint8_t src = 0;

	while (rooks)
	{
		if (rooks & 1ULL)
		{
			uint64_t dests = move_generate_attacks_row(board->full_composite, src);
			dests |= move_generate_attacks_col(board->full_composite_90, src);

			dests &= ~(board->composite_boards[to_move]);
			uint8_t dest = 0;

			while (dests)
			{
				if (dests & 1ULL)
				{
					Move move = 0;
					move |= src << move_source_index_offset;
					move |= dest << move_destination_index_offset;
					move |= ROOK << move_piecetype_offset;
					move |= to_move << move_color_offset;

					if (board->composite_boards[1-to_move] & (1ULL << dest))
					{
						move |= 1ULL << move_is_capture_offset;
						move |= board_piecetype_at_index(board, dest) << move_captured_piecetype_offset;
					}

					movelist->moves[movelist->num++] = move;
				}

				dest++;
				dests >>= 1;
			}
		}

		rooks >>= 1;
		src++;
	}
}

static void move_generate_movelist_bishop(Bitboard *board, Movelist *movelist)
{
	Color to_move = board->to_move;
	uint64_t bishops = board->boards[to_move][BISHOP];
	uint8_t src = 0;

	while (bishops)
	{
		if (bishops & 1ULL)
		{
			uint64_t dests = move_generate_attacks_diag45(board->full_composite_45, src);
			dests |= move_generate_attacks_diag315(board->full_composite_315, src);

			dests &= ~(board->composite_boards[to_move]);
			uint8_t dest = 0;

			while (dests)
			{
				if (dests & 1ULL)
				{
					Move move = 0;
					move |= src << move_source_index_offset;
					move |= dest << move_destination_index_offset;
					move |= BISHOP << move_piecetype_offset;
					move |= to_move << move_color_offset;

					if (board->composite_boards[1-to_move] & (1ULL << dest))
					{
						move |= 1ULL << move_is_capture_offset;
						move |= board_piecetype_at_index(board, dest) << move_captured_piecetype_offset;
					}

					movelist->moves[movelist->num++] = move;
				}

				dest++;
				dests >>= 1;
			}
		}

		bishops >>= 1;
		src++;
	}
}

static void move_generate_movelist_queen(Bitboard *board, Movelist *movelist)
{
	Color to_move = board->to_move;
	uint64_t queens = board->boards[to_move][QUEEN];
	uint8_t src = 0;

	while (queens)
	{
		if (queens & 1ULL)
		{
			uint64_t dests = move_generate_attacks_row(board->full_composite, src);
			dests |= move_generate_attacks_col(board->full_composite_90, src);
			dests |= move_generate_attacks_diag45(board->full_composite_45, src);
			dests |= move_generate_attacks_diag315(board->full_composite_315, src);

			dests &= ~(board->composite_boards[to_move]);
			uint8_t dest = 0;

			while (dests)
			{
				if (dests & 1ULL)
				{
					Move move = 0;
					move |= src << move_source_index_offset;
					move |= dest << move_destination_index_offset;
					move |= QUEEN << move_piecetype_offset;
					move |= to_move << move_color_offset;

					if (board->composite_boards[1-to_move] & (1ULL << dest))
					{
						move |= 1ULL << move_is_capture_offset;
						move |= board_piecetype_at_index(board, dest) << move_captured_piecetype_offset;
					}

					movelist->moves[movelist->num++] = move;
				}

				dest++;
				dests >>= 1;
			}
		}

		queens >>= 1;
		src++;
	}
}

static void move_generate_movelist_castle(Bitboard *board, Movelist *movelist)
{
}

static void move_generate_movelist_enpassant(Bitboard *board, Movelist *movelist)
{
}

static uint64_t move_generate_attacks_row(uint64_t composite_board, uint8_t index)
{
	uint8_t shift = board_row_of(index) << 3;
	uint8_t occupied_row = (composite_board >> shift) & 0xFF;
	uint64_t attacks = row_attacks[occupied_row][board_col_of(index)];

	return attacks << shift;
}

static uint64_t move_generate_attacks_col(uint64_t composite_board, uint8_t index)
{
	uint8_t occupied_col = (composite_board >> (board_col_of(index) << 3)) & 0xFF;
	uint64_t attacks = col_attacks[occupied_col][board_row_of(index)];

	return attacks << board_col_of(index);
}

// for moving minor diagonals back into place when they are actually located on the major
uint8_t result_shift_left[] = { 0, 0, 0, 0, 0, 0, 0, 0, 8, 16, 24, 32, 40, 48, 56 };
uint8_t result_shift_right[] = { 56, 48, 40, 32, 24, 16, 8, 0, 0, 0, 0, 0, 0, 0, 0 };

// for the shorter diagonals, we move them up or down the right number of columns so that
// when we take out the 8 LSB corresponding to the "full" diagonal, there is some garbage.
// this will fall off the end of the board when we shift back
static uint64_t move_generate_attacks_diag45(uint64_t composite_board, uint8_t index)
{
	static const uint8_t diagonal_numbers[] = {
    	0,  1,  2,  3,  4,  5,  6,  7,
    	1,  2,  3,  4,  5,  6,  7,  8,
    	2,  3,  4,  5,  6,  7,  8,  9,
    	3,  4,  5,  6,  7,  8,  9, 10,
    	4,  5,  6,  7,  8,  9, 10, 11,
    	5,  6,  7,  8,  9, 10, 11, 12,
    	6,  7,  8,  9, 10, 11, 12, 13,
    	7,  8,  9, 10, 11, 12, 13, 14
	};

	static const uint8_t diagonal_number_shift[] =
		{ 0, 1, 3, 6, 10, 15, 21, 28, 35, 41, 46, 60, 53, 55, 56 };


	uint8_t diagonal_number = diagonal_numbers[index];
	uint8_t occupied_diag = (composite_board >> diagonal_number_shift[diagonal_number]) & 0xFF;
	uint64_t attacks = diag_attacks_45[occupied_diag][board_col_of(index)];

	return (attacks >> result_shift_right[diagonal_number]) << result_shift_left[diagonal_number];
}

static uint64_t move_generate_attacks_diag315(uint64_t composite_board, uint8_t index)
{
	return 0;
}

