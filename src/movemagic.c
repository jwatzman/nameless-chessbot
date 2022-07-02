#include <stdint.h>

#include "bitboard.h"
#include "bitops.h"
#include "movemagic-consts.h"
#include "movemagic.h"

typedef struct {
  uint64_t mask;
  uint64_t magic;
  uint64_t* attacks;
  uint8_t shift;
} Entry;

static uint64_t* rook_attacks;
static uint64_t* bishop_attacks;

static Entry rook_entries[64];
static Entry bishop_entries[64];

static void movemagic_init_findsetbits(uint64_t board,
                                       uint8_t* setbits,
                                       uint8_t* numsetbits) {
  uint8_t i = 0;
  while (board) {
    uint8_t bit = bitscan(board);
    board &= board - 1;
    setbits[i] = bit;
    i++;
  }

  *numsetbits = i;
}

/**
 * If you think of setbits as a set of numsetbits elements, then there are
 * 2**numsetbits subsets. This function generates the nth subset, sets those
 * bits, and returns it.
 */
static uint64_t movemagic_init_nth_subset(uint64_t n,
                                          uint8_t* setbits,
                                          uint8_t numsetbits) {
  uint64_t ret = 0;
  for (uint8_t i = 0; i < numsetbits; i++)
    if (n & (1ULL << i))
      ret |= (1ULL << setbits[i]);

  return ret;
}

static int8_t add_one(int8_t x) {
  return x + 1;
}
static int8_t sub_one(int8_t x) {
  return x - 1;
}
static int8_t add_zero(int8_t x) {
  return x;
}
static uint64_t movemagic_init_compute_attacks(uint8_t pos,
                                               uint64_t occ,
                                               int8_t (*row_incr)(int8_t),
                                               int8_t (*col_incr)(int8_t)) {
  uint64_t attacks = 0;

  int8_t init_row = (int8_t)board_row_of(pos);
  int8_t init_col = (int8_t)board_col_of(pos);

  for (int8_t row = row_incr(init_row), col = col_incr(init_col);
       row < 8 && col < 8 && row >= 0 && col >= 0;
       row = row_incr(row), col = col_incr(col)) {
    uint64_t bit = 1ULL << board_index_of((uint8_t)row, (uint8_t)col);
    attacks |= bit;
    if (occ & bit)
      break;
  }

  return attacks;
}

static uint64_t movemagic_init_rook_attacks(uint8_t pos, uint64_t occ) {
  return movemagic_init_compute_attacks(pos, occ, add_one, add_zero) |
         movemagic_init_compute_attacks(pos, occ, sub_one, add_zero) |
         movemagic_init_compute_attacks(pos, occ, add_zero, add_one) |
         movemagic_init_compute_attacks(pos, occ, add_zero, sub_one);
}

static uint64_t movemagic_init_bishop_attacks(uint8_t pos, uint64_t occ) {
  return movemagic_init_compute_attacks(pos, occ, add_one, add_one) |
         movemagic_init_compute_attacks(pos, occ, add_one, sub_one) |
         movemagic_init_compute_attacks(pos, occ, sub_one, add_one) |
         movemagic_init_compute_attacks(pos, occ, sub_one, sub_one);
}

static void movemagic_init_tables(void) {
  // XXX this can be worked out statically. Is heap or BSS better, or does it
  // matter? This is simple for now.
  size_t r_tot = 0;
  size_t b_tot = 0;
  for (uint8_t pos = 0; pos < 64; pos++) {
    r_tot += 1 << (64 - r_shift[pos]);
    b_tot += 1 << (64 - b_shift[pos]);
  }
  rook_attacks = malloc(r_tot * sizeof(uint64_t));
  bishop_attacks = malloc(b_tot * sizeof(uint64_t));

  uint64_t* p_rook = rook_attacks;
  uint64_t* p_bishop = bishop_attacks;
  for (uint8_t pos = 0; pos < 64; pos++) {
    Entry* r = &rook_entries[pos];
    r->mask = r_mask[pos];
    r->magic = r_magic[pos];
    r->attacks = p_rook;
    r->shift = r_shift[pos];

    Entry* b = &bishop_entries[pos];
    b->mask = b_mask[pos];
    b->magic = b_magic[pos];
    b->attacks = p_bishop;
    b->shift = b_shift[pos];

    p_rook += 1 << (64 - r_shift[pos]);
    p_bishop += 1 << (64 - b_shift[pos]);
  }
}

void movemagic_init(void) {
  movemagic_init_tables();

  uint8_t setbits[64];
  uint8_t numsetbits;

  for (uint8_t pos = 0; pos < 64; pos++) {
    movemagic_init_findsetbits(r_mask[pos], setbits, &numsetbits);
    for (uint64_t n = 0; n < (1ULL << numsetbits); n++) {
      uint64_t occ = movemagic_init_nth_subset(n, setbits, numsetbits);
      uint64_t attacks = movemagic_init_rook_attacks(pos, occ);

      Entry* e = &rook_entries[pos];
      e->attacks[occ * e->magic >> e->shift] = attacks;
    }
  }

  for (uint8_t pos = 0; pos < 64; pos++) {
    movemagic_init_findsetbits(b_mask[pos], setbits, &numsetbits);
    for (uint64_t n = 0; n < (1ULL << numsetbits); n++) {
      uint64_t occ = movemagic_init_nth_subset(n, setbits, numsetbits);
      uint64_t attacks = movemagic_init_bishop_attacks(pos, occ);

      Entry* e = &bishop_entries[pos];
      e->attacks[occ * e->magic >> e->shift] = attacks;
    }
  }
}

uint64_t movemagic_rook(uint8_t pos, uint64_t occ) {
  Entry* e = &rook_entries[pos];
  occ = occ & e->mask;
  return e->attacks[occ * e->magic >> e->shift];
}

uint64_t movemagic_bishop(uint8_t pos, uint64_t occ) {
  Entry* e = &bishop_entries[pos];
  occ = occ & e->mask;
  return e->attacks[occ * e->magic >> e->shift];
}
