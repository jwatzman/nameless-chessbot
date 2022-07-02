#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "bitboard.h"
#include "bitops.h"

static int sign(int x) {
  if (x > 0)
    return 1;
  else if (x < 0)
    return -1;
  else
    return 0;
}

static uint64_t raycast(uint8_t src, uint8_t dest) {
  if (src == dest)
    return 0;

  int src_row = board_row_of(src);
  int src_col = board_col_of(src);

  int dest_row = board_row_of(dest);
  int dest_col = board_col_of(dest);

  if (src_row != dest_row && src_col != dest_col &&
      abs(src_row - dest_row) != abs(src_col - dest_col))
    return 0;

  int delta_row = sign(dest_row - src_row);
  int delta_col = sign(dest_col - src_col);

  uint64_t result = 0;
  for (; src_row >= 0 && src_row < 8 && src_col >= 0 && src_col < 8;
       src_row += delta_row, src_col += delta_col)
    result |= (1ULL << board_index_of((uint8_t)src_row, (uint8_t)src_col));

  return result;
}

void gen_move_raycast(void) {
  printf("uint64_t raycast[64][64] = {\n");

  for (uint8_t src = 0; src < 64; src++) {
    printf("{\n\t");

    for (uint8_t dest = 0; dest < 64; dest++) {
      uint64_t btw = raycast(src, dest);
      printf("0x%.16" PRIx64, btw);
      if (dest < 63) {
        printf(",");
        if (dest % 8 == 7)
          printf("\n\t");
        else
          printf(" ");
      }
    }

    printf("\n}");
    if (src < 63)
      printf(",");
    printf("\n");
  }

  printf("};\n");
}
