#include <stdint.h>
#include "types.h"
#include "move.h"
#include "move-generated.h"
#include "bitboard.h"

void move_generate_movelist_pawn(Bitboard *board, Color to_move, Movelist *movelist);
void move_generate_movelist_knight(Bitboard *board, Color to_move, Movelist *movelist);
void move_generate_movelist_king(Bitboard *board, Color to_move, Movelist *movelist);
void move_generate_movelist_sliding_row(Bitboard *board, Color to_move, Piecetype piece, Movelist *movelist);
void move_generate_movelist_sliding_col(Bitboard *board, Color to_move, Piecetype piece, Movelist *movelist);
void move_generate_movelist_sliding_diag_45(Bitboard *board, Color to_move, Piecetype piece, Movelist *movelist);
void move_generate_movelist_sliding_diag_315(Bitboard *board, Color to_move, Piecetype piece, Movelist *movelist);
void move_generate_movelist_castle(Bitboard *board, Color to_move, Movelist *movelist);
void move_generate_movelist_enpassant(Bitboard *board, Color to_move, Movelist *movelist);

void move_generate_movelist(Bitboard *board, Color to_move, Movelist *movelist)
{
	movelist->num = 0;

	move_generate_movelist_pawn(board, to_move, movelist);
	move_generate_movelist_knight(board, to_move, movelist);
	move_generate_movelist_king(board, to_move, movelist);

	move_generate_movelist_sliding_row(board, to_move, ROOK, movelist);
	move_generate_movelist_sliding_row(board, to_move, QUEEN, movelist);
	move_generate_movelist_sliding_col(board, to_move, ROOK, movelist);
	move_generate_movelist_sliding_col(board, to_move, QUEEN, movelist);

	move_generate_movelist_sliding_diag_45(board, to_move, BISHOP, movelist);
	move_generate_movelist_sliding_diag_45(board, to_move, QUEEN, movelist);
	move_generate_movelist_sliding_diag_315(board, to_move, BISHOP, movelist);
	move_generate_movelist_sliding_diag_315(board, to_move, QUEEN, movelist);

	move_generate_movelist_castle(board, to_move, movelist);
	move_generate_movelist_enpassant(board, to_move, movelist);
}

int move_verify(Bitboard *board, Move move)
{
	return 1;
}

void move_generate_movelist_pawn(Bitboard *board, Color to_move, Movelist *movelist)
{
}

void move_generate_movelist_knight(Bitboard *board, Color to_move, Movelist *movelist)
{
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
					uint32_t move = 0;
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

void move_generate_movelist_king(Bitboard *board, Color to_move, Movelist *movelist)
{
}

void move_generate_movelist_sliding_row(Bitboard *board, Color to_move, Piecetype piece, Movelist *movelist)
{
}

void move_generate_movelist_sliding_col(Bitboard *board, Color to_move, Piecetype piece, Movelist *movelist)
{
}

void move_generate_movelist_sliding_diag_45(Bitboard *board, Color to_move, Piecetype piece, Movelist *movelist)
{
}

void move_generate_movelist_sliding_diag_315(Bitboard *board, Color to_move, Piecetype piece, Movelist *movelist)
{
}

void move_generate_movelist_castle(Bitboard *board, Color to_move, Movelist *movelist)
{
}

void move_generate_movelist_enpassant(Bitboard *board, Color to_move, Movelist *movelist)
{
}

