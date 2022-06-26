#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "bitboard.h"
#include "bitops.h"

static uint64_t between(uint8_t sm, uint8_t lg) {
  if (sm == lg)
    return 0;

  int sm_row = board_row_of(sm);
  int sm_col = board_col_of(sm);

  int lg_row = board_row_of(lg);
  int lg_col = board_col_of(lg);

  int delta_row, delta_col;
  if (sm_row == lg_row) {
    delta_row = 0;
    delta_col = 1;
  } else if (sm_col == lg_col) {
    delta_row = 1;
    delta_col = 0;
  } else if (abs(sm_row - lg_row) == abs(sm_col - lg_col)) {
    delta_row = sm_row < lg_row ? 1 : -1;
    delta_col = sm_col < lg_col ? 1 : -1;
  } else {
    return 0;
  }

  uint64_t result = 0;
  for (; sm_row != lg_row || sm_col != lg_col;
       sm_row += delta_row, sm_col += delta_col) {
    result |= (1ULL << board_index_of(sm_row, sm_col));
  }
  result |= (1ULL << board_index_of(lg_row, lg_col));

  return result;
}

void gen_move_between(void) {
  printf("uint64_t between[64][64] = {\n");

  for (uint8_t pos1 = 0; pos1 < 64; pos1++) {
    printf("{\n\t");

    for (uint8_t pos2 = 0; pos2 < 64; pos2++) {
      uint64_t btw = between(min(pos1, pos2), max(pos1, pos2));
      printf("0x%.16" PRIx64, btw);
      if (pos2 < 63) {
        printf(",");
        if (pos2 % 8 == 7)
          printf("\n\t");
        else
          printf(" ");
      }
    }

    printf("\n}");
    if (pos1 < 63)
      printf(",");
    printf("\n");
  }

  printf("};\n");
}
