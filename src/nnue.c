#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitops.h"
#include "config.h"
#include "nnue.h"
#include "types.h"

#if ENABLE_NNUE_SIMD
#include <immintrin.h>
#endif

#define NNUE_INPUT_LAYER 64 * 2 * 5 * 64

#define RELU_MIN 0
#define RELU_MAX 255

// Argument passed to training script.
#define SCALE 400

#if ENABLE_NNUE

extern unsigned char nn_nnue_bin[];
extern unsigned int nn_nnue_bin_len;

static int initalized = 0;

static int16_t input2hidden_weight[NNUE_INPUT_LAYER][NNUE_HIDDEN_LAYER];
static int16_t hidden_bias[NNUE_HIDDEN_LAYER];
static int8_t hidden2output_weight[2 * NNUE_HIDDEN_LAYER];
static int32_t output_bias;

static Piecetype pmap[6] = {PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING};

static inline uint32_t read_u32(FILE* f) {
  int a = getc(f);
  int b = getc(f);
  int c = getc(f);
  int d = getc(f);

  if (a == EOF || b == EOF || c == EOF || d == EOF)
    abort();

  return (uint32_t)(a | b << 8 | c << 16 | d << 24);
}

static inline int16_t read_i16(FILE* f) {
  int a = getc(f);
  int b = getc(f);

  if (a == EOF || b == EOF)
    abort();

  return (int16_t)(a | b << 8);
}

static inline int8_t read_i8(FILE* f) {
  int a = getc(f);

  if (a == EOF)
    abort();

  return (int8_t)a;
}

void nnue_init(void) {
  FILE* f = fmemopen(nn_nnue_bin, nn_nnue_bin_len, "rb");
  if (!f)
    abort();

  if (read_u32(f) != NNUE_INPUT_LAYER)
    abort();

  if (read_u32(f) != NNUE_HIDDEN_LAYER)
    abort();

  if (read_u32(f) != 1)
    abort();

  for (int i = 0; i < NNUE_INPUT_LAYER; i++)
    for (int j = 0; j < NNUE_HIDDEN_LAYER; j++)
      input2hidden_weight[i][j] = read_i16(f);

  for (int i = 0; i < NNUE_HIDDEN_LAYER; i++)
    hidden_bias[i] = read_i16(f);

  for (int i = 0; i < 2 * NNUE_HIDDEN_LAYER; i++)
    hidden2output_weight[i] = read_i8(f);

  output_bias = read_i16(f);

  if (getc(f) != EOF)
    abort();

  fclose(f);
  initalized = 1;
}

static void nnue_toggle_piece_into(const Bitboard* board,
                                   Piecetype piece,
                                   Color color,
                                   uint8_t loc,
                                   int activate,
                                   int16_t hidden[2][NNUE_HIDDEN_LAYER]) {
  assert(activate >= -1 && activate <= 1);
  if (activate == 0)
    return;
  assert(piece != KING);

  uint8_t king_loc_white = bitscan(board->boards[WHITE][KING]);
  uint8_t king_loc_black = bitscan(board->boards[BLACK][KING]);
  uint8_t king_loc_black_swapped = king_loc_black ^ 56;

  int mapped_piece = pmap[piece];
  uint8_t loc_swapped = loc ^ 56;

  int idx_white =
      king_loc_white * 64 * 2 * 5 + (color * 5 + mapped_piece) * 64 + loc;
  int idx_black = king_loc_black_swapped * 64 * 2 * 5 +
                  (!color * 5 + mapped_piece) * 64 + loc_swapped;

  for (size_t i = 0; i < NNUE_HIDDEN_LAYER; i++) {
    if (activate > 0) {
      hidden[WHITE][i] += input2hidden_weight[idx_white][i];
      hidden[BLACK][i] += input2hidden_weight[idx_black][i];
    } else {
      hidden[WHITE][i] -= input2hidden_weight[idx_white][i];
      hidden[BLACK][i] -= input2hidden_weight[idx_black][i];
    }
  }
}

void nnue_toggle_piece(Bitboard* board,
                       Piecetype piece,
                       Color color,
                       uint8_t loc,
                       int activate) {
  nnue_toggle_piece_into(board, piece, color, loc, activate,
                         board->nnue_hidden);
}

static void nnue_reset_into(const Bitboard* board,
                            int16_t hidden[2][NNUE_HIDDEN_LAYER]) {
  memcpy(hidden[0], hidden_bias, NNUE_HIDDEN_LAYER * sizeof(int16_t));
  memcpy(hidden[1], hidden_bias, NNUE_HIDDEN_LAYER * sizeof(int16_t));

  for (Color color = WHITE; color <= BLACK; color++) {
    // Strict < KING, do not include king.
    for (Piecetype piece = PAWN; piece < KING; piece++) {
      uint64_t pieces = board->boards[color][piece];
      while (pieces) {
        uint8_t loc = bitscan(pieces);
        pieces &= pieces - 1;
        nnue_toggle_piece_into(board, piece, color, loc, 1, hidden);
      }
    }
  }
}

void nnue_reset(Bitboard* board) {
  assert(initalized);
  nnue_reset_into(board, board->nnue_hidden);
}

#if !ENABLE_NNUE_SIMD
static void nnue_relu(uint8_t* out, const int16_t* in, size_t sz) {
  for (size_t i = 0; i < sz; i++) {
    int16_t in_v = in[i];
    int16_t clamped =
        in_v > RELU_MAX ? RELU_MAX : (in_v < RELU_MIN ? RELU_MIN : in_v);
    out[i] = (uint8_t)clamped;
  }
}
#endif

static int16_t nnue_compute_output(const Bitboard* board,
                                   const int16_t hidden[2][NNUE_HIDDEN_LAYER]) {
#if ENABLE_NNUE_SIMD
  static_assert(NNUE_HIDDEN_LAYER % 32 == 0, "Not correct multiple");
  __m256i accum = _mm256_set1_epi64x(0);
  for (size_t i = 0; i < NNUE_HIDDEN_LAYER * 2; i += 32) {
    int idx1 = board->to_move;
    int idx2 = i;
    if (i >= NNUE_HIDDEN_LAYER) {
      idx1 = !idx1;
      idx2 -= NNUE_HIDDEN_LAYER;
    }

    __m256i hidden1 = _mm256_loadu_si256(&hidden[idx1][idx2]);
    __m256i hidden2 = _mm256_loadu_si256(&hidden[idx1][idx2 + 16]);
    __m256i hidden_relu_vec = _mm256_permute4x64_epi64(
        _mm256_packus_epi16(hidden1, hidden2), 0b11011000);
    __m256i weight_vec = _mm256_load_si256(hidden2output_weight + i);
    __m256i v32s =
        _mm256_madd_epi16(_mm256_maddubs_epi16(hidden_relu_vec, weight_vec),
                          _mm256_set1_epi16(1));
    accum = _mm256_add_epi32(accum, v32s);
  }

  // There are smarter ways to do this horizontal sum, but thankfully clang at
  // least turns this into that.
  __m128i sum = _mm_add_epi32(_mm256_extracti128_si256(accum, 0),
                              _mm256_extracti128_si256(accum, 1));
  int32_t output = _mm_extract_epi32(sum, 0) + _mm_extract_epi32(sum, 1) +
                   _mm_extract_epi32(sum, 2) + _mm_extract_epi32(sum, 3) +
                   output_bias;
#else
  uint8_t hidden_clipped[2][NNUE_HIDDEN_LAYER];
  nnue_relu(hidden_clipped[0], hidden[board->to_move], NNUE_HIDDEN_LAYER);
  nnue_relu(hidden_clipped[1], hidden[!board->to_move], NNUE_HIDDEN_LAYER);
  uint8_t* hidden_clipped_p = &hidden_clipped[0][0];

  int32_t output;
  output = output_bias;
  for (size_t i = 0; i < NNUE_HIDDEN_LAYER * 2; i++) {
    output += hidden2output_weight[i] * hidden_clipped_p[i];
  }
#endif

  // I think this 255/64 are from here -- not sure, seems to work.
  // https://github.com/dsekercioglu/marlinflow/blob/0f22ad6f0f1ac05e20e6edba1d181ca392c762a4/convert/src/main.rs#L12
  return (int16_t)(output * SCALE / (255 * 64));
}

int16_t nnue_debug_evaluate(const Bitboard* board) {
  int16_t hidden[2][NNUE_HIDDEN_LAYER];
  nnue_reset_into(board, hidden);
  return nnue_compute_output(board, hidden);
}

int16_t nnue_evaluate(const Bitboard* board) {
  return nnue_compute_output(board, board->nnue_hidden);
}

#endif
