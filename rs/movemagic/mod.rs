use crate::{bitboard, square};
use std::sync::LazyLock;

mod consts;
mod init;

#[derive(Clone)]
struct Entry {
    mask: u64,
    magic: u64,
    base: usize,
    shift: u8,
}

static ROOK_ENTRIES: [Entry; 64] = init::rook_entries();
static BISHOP_ENTRIES: [Entry; 64] = init::bishop_entries();

const ROOK_ATTACKS_SIZE: usize = init::rook_attacks_size();
const BISHOP_ATTACKS_SIZE: usize = init::bishop_attacks_size();

static ROOK_ATTACKS: LazyLock<[bitboard::Bitboard; ROOK_ATTACKS_SIZE]> =
    LazyLock::new(init::rook_attacks);
static BISHOP_ATTACKS: LazyLock<[bitboard::Bitboard; BISHOP_ATTACKS_SIZE]> =
    LazyLock::new(init::bishop_attacks);

fn offset(e: &Entry, occ: bitboard::Bitboard) -> usize {
    e.base + usize::try_from(occ.0.wrapping_mul(e.magic) >> e.shift).unwrap()
}

pub fn rook_attacks(sq: square::Square, occ: bitboard::Bitboard) -> bitboard::Bitboard {
    let e = &ROOK_ENTRIES[usize::from(sq.idx())];
    ROOK_ATTACKS[offset(e, occ & e.mask)]
}

pub fn bishop_attacks(sq: square::Square, occ: bitboard::Bitboard) -> bitboard::Bitboard {
    let e = &BISHOP_ENTRIES[usize::from(sq.idx())];
    BISHOP_ATTACKS[offset(e, occ & e.mask)]
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn smoke() {
        let sq = square::Square::new_from_idx(36);
        let occ = bitboard::Bitboard(0x08001000);
        assert_eq!(rook_attacks(sq, occ), 0x101010ef10101000);
        assert_eq!(bishop_attacks(sq, occ), 0x8244280028408000);
    }
}
