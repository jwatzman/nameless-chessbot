#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "nnue.h"

#define INPUT_LAYER 40960
#define HIDDEN_LAYER 128
#define OUTPUT_LAYER 1

int16_t input2hidden_weight[INPUT_LAYER][HIDDEN_LAYER];
int16_t hidden_bias[HIDDEN_LAYER];
int8_t hidden2output_weight[HIDDEN_LAYER * 2][OUTPUT_LAYER];
int8_t output_bias[OUTPUT_LAYER];

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
  FILE* f = fopen("nnue.bin", "rb");
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

  for (int i = 0; i < HIDDEN_LAYER * 2; i++)
    for (int j = 0; j < OUTPUT_LAYER; j++)
      hidden2output_weight[i][j] = read_i8(f);

  for (int i = 0; i < OUTPUT_LAYER; i++)
    output_bias[i] = read_i8(f);

  printf("loaded\n");

  if (getc(f) != EOF)
    abort();

  fclose(f);
}
