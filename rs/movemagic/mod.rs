use crate::{bitboard, square};

mod consts;
mod init;

#[derive(Clone)]
struct Entry {
    mask: u64,
    magic: u64,
    base: usize,
    shift: u8,
}

static ROOK_ENTRIES: [Entry; 64] = init::get_rook_entries();
static BISHOP_ENTRIES: [Entry; 64] = init::get_bishop_entries();

const ROOK_ATTACKS_SIZE: usize = init::get_rook_attacks_size();
const BISHOP_ATTACKS_SIZE: usize = init::get_bishop_attacks_size();

static ROOK_ATTACKS: std::sync::LazyLock<[bitboard::Bitboard; ROOK_ATTACKS_SIZE]> =
    std::sync::LazyLock::new(init::get_rook_attacks);
// static BISHOP_ATTACKS: [Bitboard; BISHOP_ATTACKS_SIZE] = todo!();

fn offset(e: &Entry, occ: bitboard::Bitboard) -> usize {
    e.base + usize::try_from((occ.0 * e.magic) >> e.shift).unwrap()
}

pub fn rook_attacks(sq: square::Square, occ: bitboard::Bitboard) -> bitboard::Bitboard {
    let e = &ROOK_ENTRIES[usize::from(sq.idx())];
    ROOK_ATTACKS[offset(e, occ)]
}
