use crate::{
    bitboard::Bitboard,
    movemagic::{self, consts},
    square,
};

pub const fn rook_entries() -> square::BySquare<movemagic::Entry> {
    entries(&consts::R_SHIFT, &consts::R_MAGIC, &consts::R_MASK)
}

pub const fn bishop_entries() -> square::BySquare<movemagic::Entry> {
    entries(&consts::B_SHIFT, &consts::B_MAGIC, &consts::B_MASK)
}

const fn entries(
    shifts: &[u8; 64],
    magics: &[u64; 64],
    masks: &[u64; 64],
) -> square::BySquare<movemagic::Entry> {
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

    square::BySquare::new(entries)
}

pub const fn rook_attacks_size() -> usize {
    attacks_size(&consts::R_SHIFT)
}

pub const fn bishop_attacks_size() -> usize {
    attacks_size(&consts::B_SHIFT)
}

const fn attacks_size(shifts: &[u8; 64]) -> usize {
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

pub fn rook_attacks() -> [Bitboard; movemagic::ROOK_ATTACKS_SIZE] {
    let mut rook_attacks: [Bitboard; movemagic::ROOK_ATTACKS_SIZE] =
        [const { Bitboard(0) }; movemagic::ROOK_ATTACKS_SIZE];

    attacks(
        &mut rook_attacks,
        &consts::R_MASK,
        &movemagic::ROOK_ENTRIES,
        rook_attacks_board,
    );

    rook_attacks
}

pub fn bishop_attacks() -> [Bitboard; movemagic::BISHOP_ATTACKS_SIZE] {
    let mut bishop_attacks: [Bitboard; movemagic::BISHOP_ATTACKS_SIZE] =
        [const { Bitboard(0) }; movemagic::BISHOP_ATTACKS_SIZE];

    attacks(
        &mut bishop_attacks,
        &consts::B_MASK,
        &movemagic::BISHOP_ENTRIES,
        bishop_attacks_board,
    );

    bishop_attacks
}

fn attacks(
    attacks_arr: &mut [Bitboard],
    masks: &[u64; 64],
    entries: &square::BySquare<movemagic::Entry>,
    attacks_board: fn(square::Square, Bitboard) -> Bitboard,
) {
    for sq in square::Square::all() {
        let mask_squares: Vec<_> = Bitboard(masks[usize::from(sq.idx())])
            .square_iterator()
            .collect();
        for n in 0u64..(1 << mask_squares.len()) {
            let occ = nth_subset(&mask_squares, n);
            let attacks = attacks_board(sq, occ);

            let e = &entries[sq];
            let off = movemagic::offset(e, occ);
            debug_assert!(attacks_arr[off].0 == 0 || attacks_arr[off] == attacks);
            attacks_arr[off] = attacks;
        }
    }
}

/// If `squares` has length `k`, then there are `2^k` possible subsets. This
/// computes the `n`th subset and combines the masks of those squares.
fn nth_subset(squares: &Vec<square::Square>, n: u64) -> Bitboard {
    debug_assert!(squares.len() < 64);
    debug_assert!(n < (1 << squares.len()));

    let mut ret = Bitboard(0);
    for (i, sq) in squares.iter().enumerate() {
        if (n & (1 << i)) != 0 {
            ret |= sq.mask();
        }
    }

    ret
}

fn rook_attacks_board(sq: square::Square, occ: Bitboard) -> Bitboard {
    attacks_board(sq, occ, 1, 0)
        | attacks_board(sq, occ, -1, 0)
        | attacks_board(sq, occ, 0, 1)
        | attacks_board(sq, occ, 0, -1)
}

fn bishop_attacks_board(sq: square::Square, occ: Bitboard) -> Bitboard {
    attacks_board(sq, occ, 1, 1)
        | attacks_board(sq, occ, 1, -1)
        | attacks_board(sq, occ, -1, 1)
        | attacks_board(sq, occ, -1, -1)
}

fn attacks_board(start: square::Square, occ: Bitboard, row_incr: i16, col_incr: i16) -> Bitboard {
    let mut attacks = Bitboard(0);

    let mut row = i16::from(start.row()) + row_incr;
    let mut col = i16::from(start.col()) + col_incr;
    while row < 8 && row >= 0 && col < 8 && col >= 0 {
        let sq = square::Square::new_from_row_col(row as u8, col as u8);
        let mask = sq.mask();
        attacks |= mask;
        if (occ & mask) != 0 {
            break;
        }

        row += row_incr;
        col += col_incr;
    }

    attacks
}
