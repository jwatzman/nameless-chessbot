#include <stdint.h>
#include "types.h"
#include "move.h"
#include "move-generated.h"

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
}

void move_generate_movelist_pawn(Bitboard *board, Color to_move, Movelist *movelist)
{
}

void move_generate_movelist_knight(Bitboard *board, Color to_move, Movelist *movelist)
{
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

