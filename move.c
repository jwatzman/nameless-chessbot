#include <stdint.h>
#include "types.h"
#include "move.h"
#include "move-generated.h"
#include "bitboard.h"

void move_generate_movelist_pawn(Bitboard *board, Movelist *movelist);
void move_generate_movelist_knight(Bitboard *board, Movelist *movelist);
void move_generate_movelist_king(Bitboard *board, Movelist *movelist);
void move_generate_movelist_sliding_row(Bitboard *board, Piecetype piece, Movelist *movelist);
void move_generate_movelist_sliding_col(Bitboard *board, Piecetype piece, Movelist *movelist);
void move_generate_movelist_sliding_diag_45(Bitboard *board, Piecetype piece, Movelist *movelist);
void move_generate_movelist_sliding_diag_315(Bitboard *board, Piecetype piece, Movelist *movelist);
void move_generate_movelist_castle(Bitboard *board, Movelist *movelist);
void move_generate_movelist_enpassant(Bitboard *board, Movelist *movelist);

void move_generate_movelist(Bitboard *board, Movelist *movelist)
{
	movelist->num = 0;

	move_generate_movelist_pawn(board, movelist);
	move_generate_movelist_knight(board, movelist);
	move_generate_movelist_king(board, movelist);

	move_generate_movelist_sliding_row(board, ROOK, movelist);
	move_generate_movelist_sliding_row(board, QUEEN, movelist);
	move_generate_movelist_sliding_col(board, ROOK, movelist);
	move_generate_movelist_sliding_col(board, QUEEN, movelist);

	move_generate_movelist_sliding_diag_45(board, BISHOP, movelist);
	move_generate_movelist_sliding_diag_45(board, QUEEN, movelist);
	move_generate_movelist_sliding_diag_315(board, BISHOP, movelist);
	move_generate_movelist_sliding_diag_315(board, QUEEN, movelist);

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
			if ((to_move == WHITE && row < 6) || (to_move == BLACK && row > 1))
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

			// TODO: promotions and captures
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

void move_generate_movelist_sliding_row(Bitboard *board, Piecetype piece, Movelist *movelist)
{
}

void move_generate_movelist_sliding_col(Bitboard *board, Piecetype piece, Movelist *movelist)
{
}

void move_generate_movelist_sliding_diag_45(Bitboard *board, Piecetype piece, Movelist *movelist)
{
}

void move_generate_movelist_sliding_diag_315(Bitboard *board, Piecetype piece, Movelist *movelist)
{
}

void move_generate_movelist_castle(Bitboard *board, Movelist *movelist)
{
}

void move_generate_movelist_enpassant(Bitboard *board, Movelist *movelist)
{
}

