use crate::movemagic::init::{get_bishop_entries, get_rook_entries};

mod consts;
mod init;

#[derive(Clone)]
struct Entry {
    mask: u64,
    magic: u64,
    base: usize,
    shift: u8,
}

static ROOK_ENTRIES: [Entry; 64] = get_rook_entries();
static BISHOP_ENTRIES: [Entry; 64] = get_bishop_entries();

const ROOK_ATTACKS_SIZE: usize = init::get_rook_attacks_size();
const BISHOP_ATTACKS_SIZE: usize = init::get_bishop_attacks_size();

// static ROOK_ATTACKS: [Bitboard; ROOK_ATTACKS_SIZE] = todo!();
// static BISHOP_ATTACKS: [Bitboard; BISHOP_ATTACKS_SIZE] = todo!();
