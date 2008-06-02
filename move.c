#include <stdint.h>
#include "types.h"
#include "move.h"
#include "move-generated.h"
#include "bitboard.h"

void move_generate_movelist_pawn(Bitboard *board, Movelist *movelist);
void move_generate_movelist_knight(Bitboard *board, Movelist *movelist);
void move_generate_movelist_king(Bitboard *board, Movelist *movelist);
void move_generate_movelist_rook(Bitboard *board, Movelist *movelist);
void move_generate_movelist_bishop(Bitboard *board, Movelist *movelist);
void move_generate_movelist_queen(Bitboard *board, Movelist *movelist);
void move_generate_movelist_castle(Bitboard *board, Movelist *movelist);
void move_generate_movelist_enpassant(Bitboard *board, Movelist *movelist);

uint64_t move_generate_attacks_row(uint64_t composite_board, uint8_t index);
uint64_t move_generate_attacks_col(uint64_t composite_board, uint8_t index);
uint64_t move_generate_attacks_diag45(uint64_t composite_board, uint8_t index);
uint64_t move_generate_attacks_diag315(uint64_t composite_board, uint8_t index);


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

void move_generate_movelist_pawn(Bitboard *board, Movelist *movelist)
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

void move_generate_movelist_knight(Bitboard *board, Movelist *movelist)
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

void move_generate_movelist_king(Bitboard *board, Movelist *movelist)
{
}

void move_generate_movelist_rook(Bitboard *board, Movelist *movelist)
{
}

void move_generate_movelist_bishop(Bitboard *board, Movelist *movelist)
{
}

void move_generate_movelist_queen(Bitboard *board, Movelist *movelist)
{
}


void move_generate_movelist_castle(Bitboard *board, Movelist *movelist)
{
}

void move_generate_movelist_enpassant(Bitboard *board, Movelist *movelist)
{
}

