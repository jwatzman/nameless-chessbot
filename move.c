#define _GNU_SOURCE
#include <stdint.h>
#include <string.h>
#include "types.h"
#include "move.h"
#include "move-generated.h"
#include "bitboard.h"
#include "bitscan.h"

static void move_generate_movelist_pawn_push(Bitboard *board, Movelist *movelist);
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

	Color to_move = board->to_move;

	for (Piecetype piece = 0; piece < 6; piece++)
	{
		uint64_t pieces = board->boards[to_move][piece];

		while (pieces)
		{
			uint8_t src = bitscan(pieces);
			pieces ^= 1ULL << src;

			uint64_t dests = move_generate_attacks(board, piece, to_move, src);
			dests &= ~(board->composite_boards[to_move]); // can't capture your own piece

			uint64_t captures = dests & board->composite_boards[1-to_move];
			uint64_t non_captures = piece == PAWN ? 0 : dests & ~(board->composite_boards[1-to_move]);

			while (captures)
			{
				uint8_t dest = bitscan(captures);
				captures ^= 1ULL << dest;

				Move move = 0;
				move |= src << move_source_index_offset;
				move |= dest << move_destination_index_offset;
				move |= piece << move_piecetype_offset;
				move |= to_move << move_color_offset;

				move |= 1ULL << move_is_capture_offset;
				move |= board_piecetype_at_index(board, dest) << move_captured_piecetype_offset;

				if (piece == PAWN && (board_row_of(dest) == 0 || board_row_of(dest) == 7))
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

			while (non_captures)
			{
				uint8_t dest = bitscan(non_captures);
				non_captures ^= 1ULL << dest;

				Move move = 0;
				move |= src << move_source_index_offset;
				move |= dest << move_destination_index_offset;
				move |= piece << move_piecetype_offset;
				move |= to_move << move_color_offset;

				movelist->moves[movelist->num++] = move;
			}
		}
	}

	move_generate_movelist_pawn_push(board, movelist);
	move_generate_movelist_castle(board, movelist);
	move_generate_movelist_enpassant(board, movelist);
}

uint64_t move_generate_attacks(Bitboard *board, Piecetype piece, Color color, uint8_t index)
{
	switch (piece)
	{
		case PAWN:
			return pawn_attacks[color][index];
			break;

		case BISHOP:
			return move_generate_attacks_diag45(board->full_composite_45, index)
				| move_generate_attacks_diag315(board->full_composite_315, index);
			break;

		case KNIGHT:
			return knight_attacks[index];
			break;

		case ROOK:
			return move_generate_attacks_row(board->full_composite, index)
				| move_generate_attacks_col(board->full_composite_90, index);
			break;

		case QUEEN:
			return move_generate_attacks(board, BISHOP, color, index)
				| move_generate_attacks(board, ROOK, color, index);
			break;

		case KING:
			return king_attacks[index];
			break;

		default: // err...
			return 0;
			break;
	}
}

int move_square_is_attacked(Bitboard *board, Color attacker, uint8_t square)
{
	if (knight_attacks[square] & board->boards[attacker][KNIGHT])
		return 1;

	if (king_attacks[square] & board->boards[attacker][KING])
		return 1;

	uint64_t row_attacks = move_generate_attacks_row(board->full_composite, square);
	if (row_attacks & board->boards[attacker][ROOK])
		return 1;
	if (row_attacks & board->boards[attacker][QUEEN])
		return 1;

	uint64_t col_attacks = move_generate_attacks_col(board->full_composite_90, square);
	if (col_attacks & board->boards[attacker][ROOK])
		return 1;
	if (col_attacks & board->boards[attacker][QUEEN])
		return 1;

	uint64_t diag45_attacks = move_generate_attacks_diag45(board->full_composite_45, square);
	if (diag45_attacks & board->boards[attacker][BISHOP])
		return 1;
	if (diag45_attacks & board->boards[attacker][QUEEN])
		return 1;

	uint64_t diag315_attacks = move_generate_attacks_diag315(board->full_composite_315, square);
	if (diag315_attacks & board->boards[attacker][BISHOP])
		return 1;
	if (diag315_attacks & board->boards[attacker][QUEEN])
		return 1;

	uint8_t row = board_row_of(square);
	uint8_t col = board_col_of(square);

	if (attacker == WHITE && row > 1)
	{
		if (col > 0 && (board->boards[attacker][PAWN] & (1ULL << (square - 9))))
			return 1;
		if (col < 7 && (board->boards[attacker][PAWN] & (1ULL << (square - 7))))
			return 1;
	}

	if (attacker == BLACK && row < 6)
	{
		if (col > 0 && (board->boards[attacker][PAWN] & (1ULL << (square + 7))))
			return 1;
		if (col < 7 && (board->boards[attacker][PAWN] & (1ULL << (square + 9))))
			return 1;
	}

	return 0;
}

static void move_generate_movelist_pawn_push(Bitboard *board, Movelist *movelist)
{
	Color to_move = board->to_move;
	uint64_t pawns = board->boards[to_move][PAWN];

	while (pawns)
	{
		uint8_t src = bitscan(pawns);
		pawns ^= 1ULL << src;

		uint8_t row = board_row_of(src);
		uint8_t col = board_col_of(src);

		// try to move one space forward
		if ((to_move == WHITE && row < 7) || (to_move == BLACK && row > 0))
		{
			uint8_t dest;
			if (to_move == WHITE) dest = board_index_of(row + 1, col);
			else dest = board_index_of(row - 1, col);

			if (!(board->full_composite & (1ULL << dest)))
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

				// try to move two spaces forward
				if ((to_move == WHITE && row == 1) || (to_move == BLACK && row == 6))
				{
					if (to_move == WHITE) dest = board_index_of(row + 2, col);
					else dest = board_index_of(row - 2, col);
		
					if (!(board->full_composite & (1ULL << dest)))
					{
						move = 0;
						move |= src << move_source_index_offset;
						move |= dest << move_destination_index_offset;
						move |= PAWN << move_piecetype_offset;
						move |= to_move << move_color_offset;
		
						movelist->moves[movelist->num++] = move;
					}
				}
			}
		}
	}
}

static void move_generate_movelist_castle(Bitboard *board, Movelist *movelist)
{
	// squares that must be clear for a castle: (along with the king not being in check)
	// W QS: empty 1 2 3, not attacked 2 3
	// W KS: empty 5 6, not attacked 5 6
	// B QS: empty 57 58 59, not attacked 58 59
	// B KS: empty 61 62, not attacked 61 62
	// 
	// XXX this assumes the validity of the castling rights, in particular
	// that there actually are a rook/king in the right spot, funny things
	// happen if you, e.g., load a FEN with an invalid set of rights

	Color color = board->to_move;
	if (board_in_check(board, color)) // TODO :(
		return;

	// white queenside
	if ((color == WHITE) && (board->castle_status & (1 << 6)))
	{
		uint64_t clear = (1ULL << 1) | (1ULL << 2) | (1ULL << 3);
		if (!(board->full_composite & clear))
		{
			if (!move_square_is_attacked(board, 1-color, 2)
				&& !move_square_is_attacked(board, 1-color, 3))
			{
				Move move = 0;
				move |= 4 << move_source_index_offset;
				move |= 2 << move_destination_index_offset;
				move |= KING << move_piecetype_offset;
				move |= color << move_color_offset;
				move |= 1 << move_is_castle_offset;
				movelist->moves[movelist->num++] = move;
			}
		}
	}

	// white kingside
	if ((color == WHITE) && (board->castle_status & (1 << 4)))
	{
		uint64_t clear = (1ULL << 5) | (1ULL << 6);
		if (!(board->full_composite & clear))
		{
			if (!move_square_is_attacked(board, 1-color, 5)
				&& !move_square_is_attacked(board, 1-color, 6))
			{
				Move move = 0;
				move |= 4 << move_source_index_offset;
				move |= 6 << move_destination_index_offset;
				move |= KING << move_piecetype_offset;
				move |= color << move_color_offset;
				move |= 1 << move_is_castle_offset;
				movelist->moves[movelist->num++] = move;
			}
		}
	}

	// black queenside
	if ((color == BLACK) && (board->castle_status & (1 << 7)))
	{
		uint64_t clear = (1ULL << 57) | (1ULL << 58) | (1ULL << 59);
		if (!(board->full_composite & clear))
		{
			if (!move_square_is_attacked(board, 1-color, 58)
				&& !move_square_is_attacked(board, 1-color, 59))
			{
				Move move = 0;
				move |= 60 << move_source_index_offset;
				move |= 58 << move_destination_index_offset;
				move |= KING << move_piecetype_offset;
				move |= color << move_color_offset;
				move |= 1 << move_is_castle_offset;
				movelist->moves[movelist->num++] = move;
			}
		}
	}

	// black kingside
	if ((color == BLACK) && (board->castle_status & (1 << 5)))
	{
		uint64_t clear = (1ULL << 61) | (1ULL << 62);
		if (!(board->full_composite & clear))
		{
			if (!move_square_is_attacked(board, 1-color, 61)
				&& !move_square_is_attacked(board, 1-color, 62))
			{
				Move move = 0;
				move |= 60 << move_source_index_offset;
				move |= 62 << move_destination_index_offset;
				move |= KING << move_piecetype_offset;
				move |= color << move_color_offset;
				move |= 1 << move_is_castle_offset;
				movelist->moves[movelist->num++] = move;
			}
		}
	}
}

static void move_generate_movelist_enpassant(Bitboard *board, Movelist *movelist)
{
	uint8_t ep_index = board->enpassant_index;
	Color color = board->to_move;
	if (ep_index)
	{
		if (board_col_of(ep_index) > 0 && (board->boards[color][PAWN] & (1ULL << (ep_index - 1))))
		{
			Move move = 0;
			move |= (ep_index - 1) << move_source_index_offset;
			move |= (color == WHITE ? ep_index + 8 : ep_index - 8) << move_destination_index_offset;
			move |= PAWN << move_piecetype_offset;
			move |= color << move_color_offset;
			move |= 1 << move_is_enpassant_offset;
			movelist->moves[movelist->num++] = move;
		}

		if (board_col_of(ep_index) < 7 && (board->boards[color][PAWN] & (1ULL << (ep_index + 1))))
		{
			Move move = 0;
			move |= (ep_index + 1) << move_source_index_offset;
			move |= (color == WHITE ? ep_index + 8 : ep_index - 8) << move_destination_index_offset;
			move |= PAWN << move_piecetype_offset;
			move |= color << move_color_offset;
			move |= 1 << move_is_enpassant_offset;
			movelist->moves[movelist->num++] = move;
		}
	}
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
		{ 0, 1, 3, 6, 10, 15, 21, 28, 35, 41, 46, 50, 53, 55, 56 };


	uint8_t diagonal_number = diagonal_numbers[index];
	uint8_t occupied_diag = (composite_board >> diagonal_number_shift[diagonal_number]) & 0xFF;
	uint64_t attacks = diag_attacks_45[occupied_diag][board_col_of(index)];

	return (attacks >> result_shift_right[diagonal_number]) << result_shift_left[diagonal_number];
}

// like 45, but we move the lower diagonals to align with the MSB and the higher ones
// to align with the LSB
// (this makes the shifting a bit less clean than 45)
static uint64_t move_generate_attacks_diag315(uint64_t composite_board, uint8_t index)
{
	static const uint8_t diagonal_numbers[] = {
	7,  6,  5,  4,  3,  2,  1,  0,
	8,  7,  6,  5,  4,  3,  2,  1,
	9,  8,  7,  6,  5,  4,  3,  2,
	10,  9,  8,  7,  6,  5,  4,  3,
	11, 10,  9,  8,  7,  6,  5,  4,
	12, 11, 10,  9,  8,  7,  6,  5,
	13, 12, 11, 10,  9,  8,  7,  6,
	14, 13, 12, 11, 10,  9,  8,  7
	};

	static const uint8_t diagonal_number_shift_left[] =
		{ 7, 5, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	static const uint8_t diagonal_number_shift_right[] =
		{ 0, 0, 0, 2, 7, 13, 20, 28, 36, 43, 49, 54, 58, 61, 63 };

	
	uint8_t diagonal_number = diagonal_numbers[index];
	uint8_t occupied_diag =
		((composite_board >> diagonal_number_shift_right[diagonal_number])
			<< diagonal_number_shift_left[diagonal_number])
			& 0xFF;
	uint64_t attacks = diag_attacks_315[occupied_diag][board_col_of(index)];

	return (attacks >> result_shift_right[diagonal_number]) << result_shift_left[diagonal_number];

}

