use crate::{
    bitboard::Bitboard,
    movemagic::{self, consts},
    square::Square,
};

pub const fn get_rook_entries() -> [movemagic::Entry; 64] {
    get_entries(&consts::R_SHIFT, &consts::R_MAGIC, &consts::R_MASK)
}

pub const fn get_bishop_entries() -> [movemagic::Entry; 64] {
    get_entries(&consts::B_SHIFT, &consts::B_MAGIC, &consts::B_MASK)
}

const fn get_entries(
    shifts: &[u8; 64],
    magics: &[u64; 64],
    masks: &[u64; 64],
) -> [movemagic::Entry; 64] {
    let mut entries: [movemagic::Entry; 64] = [const {
        movemagic::Entry {
            mask: 0,
            magic: 0,
            base: 0,
            shift: 0,
        }
    }; 64];
    let mut base: usize = 0;

    let mut idx: usize = 0;
    while idx < 64 {
        entries[idx] = movemagic::Entry {
            mask: masks[idx],
            magic: magics[idx],
            base,
            shift: shifts[idx],
        };
        base += shift_num_entries(shifts, idx);
        idx += 1;
    }

    entries
}

pub const fn get_rook_attacks_size() -> usize {
    get_attacks_size(&consts::R_SHIFT)
}

pub const fn get_bishop_attacks_size() -> usize {
    get_attacks_size(&consts::B_SHIFT)
}

const fn get_attacks_size(shifts: &[u8; 64]) -> usize {
    let mut tot: usize = 0;

    let mut idx: usize = 0;
    while idx < shifts.len() {
        tot += shift_num_entries(shifts, idx);
        idx += 1;
    }

    tot
}

const fn shift_num_entries(shifts: &[u8; 64], idx: usize) -> usize {
    1 << (64 - shifts[idx])
}

pub fn get_rook_attacks() -> [Bitboard; movemagic::ROOK_ATTACKS_SIZE] {
    let mut rook_attacks: [Bitboard; movemagic::ROOK_ATTACKS_SIZE] =
        [const { Bitboard(0) }; movemagic::ROOK_ATTACKS_SIZE];

    for idx in 0u8..64 {
        let sq = Square::new_from_idx(idx);
        let mask_squares: Vec<_> = Bitboard(consts::R_MASK[usize::from(sq.idx())])
            .square_iterator()
            .collect();
        for n in 0u64..(1 << mask_squares.len()) {
            let occ = nth_subset(&mask_squares, n);
            let attacks = make_rook_attacks(sq, occ);

            let e = &movemagic::ROOK_ENTRIES[usize::from(sq.idx())];
            let off = movemagic::offset(e, occ);
            debug_assert!(rook_attacks[off].0 == 0 || rook_attacks[off] == attacks);
            rook_attacks[off] = attacks;
        }
    }

    rook_attacks
}

/// If `squares` has length `k`, then there are `2^k` possible subsets. This
/// computes the `n`th subset and combines the masks of those squares.
fn nth_subset(squares: &Vec<Square>, n: u64) -> Bitboard {
    debug_assert!(squares.len() < 64);
    debug_assert!(n < (1 << squares.len()));

    let mut ret = Bitboard(0);
    for (i, sq) in squares.iter().enumerate() {
        if (n & (1 << i)) != 0 {
            ret.0 |= sq.mask();
        }
    }

    ret
}

fn make_rook_attacks(sq: Square, occ: Bitboard) -> Bitboard {
    todo!()
}
