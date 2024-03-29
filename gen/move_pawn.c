#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include "bitboard.h"
#include "types.h"

void gen_move_pawn(void) {
  printf("uint64_t pawn_attacks[2][64] = {\n\t{");

  for (Color color = 0; color < 2; color++) {
    for (uint8_t position = 0; position < 64; position++) {
      uint64_t attack_mask = 0;
      uint64_t bit = 1;
      uint8_t row = board_row_of(position);
      uint8_t col = board_col_of(position);

      if (!(color == WHITE && row == 7) && !(color == BLACK && row == 0)) {
        uint8_t new_row = color == WHITE ? row + 1 : row - 1;

        if (col > 0)
          attack_mask |= bit << board_index_of(new_row, col - 1);

        if (col < 7)
          attack_mask |= bit << board_index_of(new_row, col + 1);
      }

      printf("0x%.16" PRIx64, attack_mask);
      if (position < 63)
        printf(", ");
    }

    if (color < 1)
      printf("},\n\t{");
  }

  printf("}\n};\n");
}
