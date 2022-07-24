#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitops.h"
#include "config.h"
#include "nnue.h"
#include "types.h"

#define INPUT_LAYER 64 * 2 * 5 * 64
#define HIDDEN_LAYER 128
#define OUTPUT_LAYER 1

#define RELU_MIN 0
#define RELU_MAX 255

// Argument passed to training script.
#define SCALE 400

#if ENABLE_NNUE

extern unsigned char nn_nnue_bin[];
extern unsigned int nn_nnue_bin_len;

static int16_t input2hidden_weight[INPUT_LAYER][HIDDEN_LAYER];
static int16_t hidden_bias[HIDDEN_LAYER];
static int8_t hidden2output_weight[2 * HIDDEN_LAYER][OUTPUT_LAYER];
static int32_t output_bias[OUTPUT_LAYER];

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

  if (read_u32(f) != INPUT_LAYER)
    abort();

  if (read_u32(f) != HIDDEN_LAYER)
    abort();

  if (read_u32(f) != OUTPUT_LAYER)
    abort();

  for (int i = 0; i < INPUT_LAYER; i++)
    for (int j = 0; j < HIDDEN_LAYER; j++)
      input2hidden_weight[i][j] = read_i16(f);

  for (int i = 0; i < HIDDEN_LAYER; i++)
    hidden_bias[i] = read_i16(f);

  for (int i = 0; i < 2 * HIDDEN_LAYER; i++)
    for (int j = 0; j < OUTPUT_LAYER; j++)
      hidden2output_weight[i][j] = read_i8(f);

  for (int i = 0; i < OUTPUT_LAYER; i++)
    output_bias[i] = read_i16(f);

  if (getc(f) != EOF)
    abort();

  fclose(f);
}

static void nnue_relu(uint8_t* out, const int16_t* in, size_t sz) {
  for (size_t i = 0; i < sz; i++) {
    int16_t in_v = in[i];
    int16_t clamped =
        in_v > RELU_MAX ? RELU_MAX : (in_v < RELU_MIN ? RELU_MIN : in_v);
    out[i] = (uint8_t)clamped;
  }
}

static int pmap[6] = {PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING};
int16_t nnue_evaluate(Bitboard* board) {
  uint8_t king_loc_white = bitscan(board->boards[WHITE][KING]);
  uint8_t king_loc_black = bitscan(board->boards[BLACK][KING]);
  uint8_t king_loc_black_swapped = king_loc_black ^ 56;

  int16_t hidden[2][HIDDEN_LAYER];
  memcpy(hidden[0], hidden_bias, HIDDEN_LAYER * sizeof(int16_t));
  memcpy(hidden[1], hidden_bias, HIDDEN_LAYER * sizeof(int16_t));

  // XXX make this incremental.
  for (int color = WHITE; color <= BLACK; color++) {
    // Strict < KING, do not include king.
    for (int piece = PAWN; piece < KING; piece++) {
      int mapped_piece = pmap[piece];

      uint64_t pieces = board->boards[color][piece];
      while (pieces) {
        uint8_t loc = bitscan(pieces);
        uint8_t loc_swapped = loc ^ 56;
        pieces &= pieces - 1;

        int idx_white =
            king_loc_white * 64 * 2 * 5 + (color * 5 + mapped_piece) * 64 + loc;
        int idx_black = king_loc_black_swapped * 64 * 2 * 5 +
                        (!color * 5 + mapped_piece) * 64 + loc_swapped;

        for (size_t i = 0; i < HIDDEN_LAYER; i++) {
          hidden[0][i] += input2hidden_weight[idx_white][i];
          hidden[1][i] += input2hidden_weight[idx_black][i];
        }
      }
    }
  }

  uint8_t hidden_clipped[2][HIDDEN_LAYER];
  nnue_relu(hidden_clipped[0], hidden[board->to_move], HIDDEN_LAYER);
  nnue_relu(hidden_clipped[1], hidden[!board->to_move], HIDDEN_LAYER);
  uint8_t* hidden_clipped_p = &hidden_clipped[0][0];

  int32_t output[OUTPUT_LAYER];
  memcpy(output, output_bias, OUTPUT_LAYER * sizeof(int32_t));
  for (size_t i = 0; i < OUTPUT_LAYER; i++) {
    for (size_t j = 0; j < HIDDEN_LAYER * 2; j++) {
      // XXX try flipping hidden2output_weight so we iterate in a more
      // cache-friendly way.
      output[i] += hidden2output_weight[j][i] * hidden_clipped_p[j];
    }
  }

  // I think this 255/64 are from here -- not sure, seems to work.
  // https://github.com/dsekercioglu/marlinflow/blob/0f22ad6f0f1ac05e20e6edba1d181ca392c762a4/convert/src/main.rs#L12
  return (int16_t)(output[0] * SCALE / (255 * 64));
}

#else

void nnue_init(void) {}
// No nnue_evaluate -- force link error.

#endif
