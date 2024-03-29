#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitboard.h"
#include "bitops.h"
#include "config.h"
#include "move.h"
#include "mt19937.h"
#include "nnue.h"

// generate a 64-bit random number
static inline uint64_t board_rand64(void);

// generate a bunch of random numbers to put in for zobrists
static void board_init_zobrist(Bitboard* board);

// common bits of making and undoing moves that can be easily factored out
static void board_doundo_move_common(Bitboard* board,
                                     Move move,
                                     int nnue_activate);
static void board_toggle_piece(Bitboard* board,
                               Piecetype piece,
                               Color color,
                               uint8_t loc,
                               int nnue_activate);
static uint64_t board_gen_king_attackers(const Bitboard* board, Color color);
static void board_update_expensive_state(Bitboard* board);

static char board_sigil(int color, Piecetype type);

void board_init(Bitboard* board, State* state) {
  board_init_with_fen(
      board, state, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

void board_init_with_fen(Bitboard* board, State* state, const char* fen) {
  memset(board->boards, 0, 2 * 6 * sizeof(uint64_t));  // clear out boards
  board->state = state;

  uint8_t row = 0;
  while (row < 8) {
    uint8_t col = 0;
    while (col < 8) {
      if (*fen >= '1' && *fen <= '8') {
        // skip that number of slots
        col += (*fen - '0');
      } else {
        // figure out what piece to put here
        Color color = WHITE;
        Piecetype piece;

        switch (*fen) {
          case 'p':
            color = BLACK;  // FALLTHROUGH
          case 'P':
            piece = PAWN;
            break;

          case 'n':
            color = BLACK;  // FALLTHROUGH
          case 'N':
            piece = KNIGHT;
            break;

          case 'b':
            color = BLACK;  // FALLTHROUGH
          case 'B':
            piece = BISHOP;
            break;

          case 'r':
            color = BLACK;  // FALLTHROUGH
          case 'R':
            piece = ROOK;
            break;

          case 'q':
            color = BLACK;  // FALLTHROUGH
          case 'Q':
            piece = QUEEN;
            break;

          case 'k':
            color = BLACK;  // FALLTHROUGH
          case 'K':
            piece = KING;
            break;

          default:
            piece = KING;
            break;  // bad fen
        }

        // set the proper bit
        board->boards[color][piece] |= (1ULL << board_index_of(7 - row, col));

        col++;
      }

      fen++;  // go to the next letter
    }

    fen++;  // skip the slash or space
    row++;  // next row
  }

  // calculate the composite and rotated boards
  for (Color c = WHITE; c <= BLACK; c++) {
    board->composite_boards[c] = 0;
    for (Piecetype p = PAWN; p <= KING; p++) {
      board->composite_boards[c] |= board->boards[c][p];
    }
  }

  board->full_composite =
      board->composite_boards[WHITE] | board->composite_boards[BLACK];

  // w or b to move
  board->to_move = (*fen == 'w' ? WHITE : BLACK);
  fen += 2;  // skip the w or b and then the space

  board->state->castle_rights = 0;
  if (*fen != '-') {
    while (*fen != ' ') {
      switch (*fen) {
        case 'q':
          board->state->castle_rights |= CASTLE_R(CASTLE_R_QS, BLACK);
          break;
        case 'Q':
          board->state->castle_rights |= CASTLE_R(CASTLE_R_QS, WHITE);
          break;
        case 'k':
          board->state->castle_rights |= CASTLE_R(CASTLE_R_KS, BLACK);
          break;
        case 'K':
          board->state->castle_rights |= CASTLE_R(CASTLE_R_KS, WHITE);
          break;
        default:
          break;  // bad fen
      }

      fen++;  // next castling bit
    }
  } else
    fen++;  // skip the dash

  fen++;  // skip the space

  // enpassant index
  if (*fen != '-') {
    uint8_t col = (uint8_t)(*fen - 'a');
    fen++;
    uint8_t row = (uint8_t)(*fen - '1');
    fen++;

    // the board keeps a different notion of enpassant row than FEN
    if (row == 2)
      row = 3;
    else if (row == 5)
      row = 4;

    board->state->enpassant_index = board_index_of(row, col);
  } else {
    board->state->enpassant_index = 0;
    fen++;  // skip the dash
  }

  /* this is hax. The space does not need to be skipped, since strtol
     will ignore it. However we don't have any of the board history,
     but the halfmove count says how far back we need to check the
     history. So zero it out, which will hopefully not conencide
     with any used zobrist */
  board->state->halfmove_count = (uint8_t)strtol(fen, NULL, 10);

  // set up the mess of zobrist random numbers and the rest of the state
  board_init_zobrist(board);
  board->state->last_move = MOVE_NULL;
  board->state->prev = NULL;
  board_update_expensive_state(board);
  board->generation = 0;

#if ENABLE_NNUE
  nnue_reset(board);
#endif
}

static inline uint64_t board_rand64(void) {
  // OR two 32-bit randoms together
  return (((uint64_t)mt_random()) << 32) | ((uint64_t)mt_random());
}

static void board_init_zobrist(Bitboard* board) {
  // initially start with a random zobrist
  board->state->zobrist = board_rand64();

  // fill in each color/piece/position combination with a random mask
  for (int i = 0; i < 2; i++)
    for (int j = 0; j < 6; j++)
      for (int k = 0; k < 64; k++)
        board->zobrist_pos[i][j][k] = board_rand64();

  // same for each castle status ...
  for (int i = 0; i < 256; i++)
    board->zobrist_castle[i] = board_rand64();

  // ... and enpassant index ...
  for (int i = 0; i < 8; i++)
    board->zobrist_enpassant[i] = board_rand64();

  // ... and to move
  board->zobrist_black = board_rand64();
}

void board_do_move(Bitboard* board, Move move, State* state) {
  assert(
      move == MOVE_NULL || move_is_capture(move) ||
      (((1ULL << move_destination_index(move)) & board->full_composite) == 0));
  assert(move_captured_piecetype(move) != KING);

  memcpy(state, board->state, sizeof(State));
  state->prev = board->state;
  board->state = state;

  board->state->last_move = move;

  if (move != MOVE_NULL) {
    // halfmove_count is reset on pawn moves or captures
    if (move_piecetype(move) == PAWN || move_is_capture(move))
      board->state->halfmove_count = 0;
    else
      board->state->halfmove_count++;

    /* moving to or from a rook square means you can no longer castle on
       that side */
    uint8_t src = move_source_index(move);
    uint8_t dest = move_destination_index(move);

    // Once castling rights are gone, you can't get them back, so no need to
    // compute.
    if (board->state->castle_rights) {
      board->state->zobrist ^=
          board->zobrist_castle[board->state->castle_rights];

      if (src == 0 || dest == 0)
        board->state->castle_rights &= ~(CASTLE_R(CASTLE_R_QS, WHITE));
      if (src == 7 || dest == 7)
        board->state->castle_rights &= ~(CASTLE_R(CASTLE_R_KS, WHITE));
      if (src == 56 || dest == 56)
        board->state->castle_rights &= ~(CASTLE_R(CASTLE_R_QS, BLACK));
      if (src == 63 || dest == 63)
        board->state->castle_rights &= ~(CASTLE_R(CASTLE_R_KS, BLACK));

      /* moving your king at all means you can no longer castle on either
              side. Castling also means you can no longer castle (again) on
         either side */
      if (move_is_castle(move) || move_piecetype(move) == KING)
        board->state->castle_rights &=
            ~(CASTLE_R(CASTLE_R_BOTH, move_color(move)));

      board->state->zobrist ^=
          board->zobrist_castle[board->state->castle_rights];
    }

    /* if src and dest are 16 or -16 units apart (two rows) on a pawn move,
       update the enpassant index with the destination square. If this
       didn't happen, clear the enpassant index */
    if (board->state->enpassant_index) {
      board->state->zobrist ^=
          board->zobrist_enpassant[board_col_of(board->state->enpassant_index)];
    }
    int delta = src - dest;
    if (move_piecetype(move) == PAWN && (delta == 16 || delta == -16))
      board->state->enpassant_index = dest;
    else
      board->state->enpassant_index = 0;
    if (board->state->enpassant_index) {
      board->state->zobrist ^=
          board->zobrist_enpassant[board_col_of(board->state->enpassant_index)];
    }

    // common bits of doing and undoing moves (bulk of the logic in here)
    board_doundo_move_common(board, move, 1);
  } else {
    if (board->state->enpassant_index) {
      board->state->zobrist ^=
          board->zobrist_enpassant[board_col_of(board->state->enpassant_index)];
      board->state->enpassant_index = 0;
    }
    board->to_move = (1 - board->to_move);
    board->state->zobrist ^= board->zobrist_black;
  }

  assert(popcnt(board->boards[WHITE][KING]) == 1);
  assert(popcnt(board->boards[BLACK][KING]) == 1);

  board_update_expensive_state(board);
}

void board_undo_move(Bitboard* board) {
  Move move = board->state->last_move;
  board->state = board->state->prev;
  assert(board->state);
  uint64_t tmp_zobrist = board->state->zobrist;

  if (move != MOVE_NULL)
    board_doundo_move_common(board, move, -1);
  else
    board->to_move = (1 - board->to_move);

  // By resetting state to prev, we have already restored the zobrist, but
  // board_doundo_move_common still changes it. So save and restore. (TODO:
  // should probably fix that for perf!)
  board->state->zobrist = tmp_zobrist;
}

static void board_doundo_move_common(Bitboard* board,
                                     Move move,
                                     int nnue_activate) {
  // extract basic data
  uint8_t src = move_source_index(move);
  uint8_t dest = move_destination_index(move);
  Piecetype piece = move_piecetype(move);
  Color color = move_color(move);

  if (piece == KING)
    nnue_activate = 0;

  // remove piece at source
  board_toggle_piece(board, piece, color, src, -nnue_activate);

  // add piece at destination
  if (!move_is_promotion(move))
    board_toggle_piece(board, piece, color, dest, nnue_activate);
  else
    board_toggle_piece(board, move_promoted_piecetype(move), color, dest,
                       nnue_activate);

  // remove captured piece, if applicable
  if (move_is_capture(move))
    board_toggle_piece(board, move_captured_piecetype(move), 1 - color, dest,
                       -nnue_activate);

  // Put the rook in the right place for a castle. The king is dealt with
  // as the main "piece" of the move.
  if (move_is_castle(move)) {
    if (board_col_of(dest) == 2)  // queenside castle
    {
      board_toggle_piece(board, ROOK, color, dest - 2, -nnue_activate);
      board_toggle_piece(board, ROOK, color, dest + 1, nnue_activate);
    } else  // kingside castle
    {
      board_toggle_piece(board, ROOK, color, dest + 1, -nnue_activate);
      board_toggle_piece(board, ROOK, color, dest - 1, nnue_activate);
    }
  }

  if (move_is_enpassant(move)) {
    if (color == WHITE)  // the captured pawn is one row behind
      board_toggle_piece(board, PAWN, BLACK, dest - 8, -nnue_activate);
    else  // the captured pawn is one row up
      board_toggle_piece(board, PAWN, WHITE, dest + 8, -nnue_activate);
  }

  board->to_move = (1 - board->to_move);
  board->state->zobrist ^= board->zobrist_black;

#if ENABLE_NNUE
  if (piece == KING)
    nnue_reset(board);
#endif
}

static void board_toggle_piece(Bitboard* board,
                               Piecetype piece,
                               Color color,
                               uint8_t loc,
                               int nnue_activate) {
  // flip the bit in all of the copies of the board state
  // TODO: try recomputing composities instead of xor all the time
  board->boards[color][piece] ^= 1ULL << loc;
  board->composite_boards[color] ^= 1ULL << loc;
  board->full_composite ^= 1ULL << loc;
  board->state->zobrist ^= board->zobrist_pos[color][piece][loc];

#if ENABLE_NNUE
  nnue_toggle_piece(board, piece, color, loc, nnue_activate);
#else
  (void)nnue_activate;
#endif
}

static uint64_t board_gen_king_attackers(const Bitboard* board, Color color) {
  return move_generate_attackers(board, 1 - color,
                                 bitscan(board->boards[color][KING]),
                                 board->full_composite);
}

static void board_update_expensive_state(Bitboard* board) {
  board->state->king_attackers =
      board_gen_king_attackers(board, board->to_move);
  board->state->pinned = move_generate_pinned(board, board->to_move);
}

int board_in_check(const Bitboard* board, Color color) {
  uint64_t king_attackers;
  if (color == board->to_move)
    king_attackers = board->state->king_attackers;
  else
    // Board is not in a legal position if the person not to-move is in check.
    // We only do this as the final move legality check, so don't bother caching
    // it.
    king_attackers = board_gen_king_attackers(board, color);

  return king_attackers > 0;
}

static char board_sigil(int color, Piecetype type) {
  char sigil;
  switch (type) {
    case PAWN:
      sigil = 'P';
      break;
    case BISHOP:
      sigil = 'B';
      break;
    case KNIGHT:
      sigil = 'N';
      break;
    case ROOK:
      sigil = 'R';
      break;
    case QUEEN:
      sigil = 'Q';
      break;
    case KING:
      sigil = 'K';
      break;
    default:
      sigil = '?';
      break;
  }

  return color == WHITE ? sigil : (char)tolower(sigil);
}

void board_fen(const Bitboard* board, FILE* f) {
  for (int row = 7; row >= 0; row--) {
    int empty = 0;
    for (int col = 0; col < 8; col++) {
      uint8_t idx = (uint8_t)board_index_of(row, col);
      uint64_t mask = 1ULL << idx;

      if (board->full_composite & mask) {
        if (empty > 0)
          fprintf(f, "%d", empty);
        empty = 0;

        int color = board->composite_boards[WHITE] & mask ? WHITE : BLACK;
        Piecetype type = board_piecetype_at_index(board, idx);
        char sigil = board_sigil(color, type);
        fputc(sigil, f);
      } else {
        empty++;
      }
    }

    if (empty > 0)
      fprintf(f, "%d", empty);

    if (row > 0)
      fputc('/', f);
  }

  fputc(' ', f);
  fputc(board->to_move == WHITE ? 'w' : 'b', f);
  fputc(' ', f);

  if (board->state->castle_rights != 0) {
    if (board->state->castle_rights & CASTLE_R(CASTLE_R_KS, WHITE))
      fputc('K', f);
    if (board->state->castle_rights & CASTLE_R(CASTLE_R_QS, WHITE))
      fputc('Q', f);
    if (board->state->castle_rights & CASTLE_R(CASTLE_R_KS, BLACK))
      fputc('k', f);
    if (board->state->castle_rights & CASTLE_R(CASTLE_R_QS, BLACK))
      fputc('q', f);
  } else {
    fputc('-', f);
  }

  fputc(' ', f);

  if (board->state->enpassant_index != 0) {
    uint8_t row = board_row_of(board->state->enpassant_index);
    uint8_t col = board_col_of(board->state->enpassant_index);

    // We store as square pawn moved to, FEN stores as square pawn jumped over.
    if (board->to_move == WHITE)
      row++;
    else
      row--;

    fprintf(f, "%c%c", 'a' + col, '1' + row);

  } else {
    fputc('-', f);
  }

  // XXX we don't track the fullmove count, I don't think it matters for
  // anything we use these FENs for?
  fprintf(f, " %d 1", board->state->halfmove_count);
}

void board_print(const Bitboard* board) {
  char* separator = "-----------------";
  char* template = "| | | | | | | | |  ";
  char* this_line = malloc(sizeof(char) * (strlen(template) + 1));

  puts(separator);

  /* print every row in sequence. Check each color and each type to see
     if pieces are in that row, filling in the right slot in the template
     with the sigil. Add the row number after each row, and print
     column letters after each column. */
  for (int8_t row = 7; row >= 0; row--) {
    strcpy(this_line, template);

    for (int color = 0; color <= 1; color++) {
      for (Piecetype type = 0; type <= 5; type++) {
        char sigil = board_sigil(color, type);

        int column = 0;
        uint8_t bits =
            (uint8_t)((board->boards[color][type] >> (row << 3)) & 0xFF);

        while (bits) {
          if (bits & 0x01)
            this_line[column * 2 + 1] = sigil;

          bits >>= 1;
          column++;
        }

        this_line[18] = '1' + (char)row;
      }
    }

    puts(this_line);
  }

  puts(separator);
  puts(" a b c d e f g h ");

  free(this_line);

  printf("\nFEN: ");
  board_fen(board, stdout);
  printf("\n");

  printf("Enpassant index: %x\tHalfmove count: %x\tCastle rights: %x\n",
         board->state->enpassant_index, board->state->halfmove_count,
         board->state->castle_rights);
  printf("Zobrist: %.16" PRIx64 "\n", board->state->zobrist);
  printf("To move: %i\n", board->to_move);
}
