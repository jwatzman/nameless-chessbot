use crate::{
    bitboard::Bitboard,
    square::{BySquare, Square},
};

pub const fn king_attacks() -> BySquare<Bitboard> {
    attacks_from_offsets(&[
        (1, 0),
        (1, -1),
        (0, -1),
        (-1, -1),
        (-1, 0),
        (-1, 1),
        (0, 1),
        (1, 1),
    ])
}

pub const fn knight_attacks() -> BySquare<Bitboard> {
    attacks_from_offsets(&[
        (2, 1),
        (2, -1),
        (1, -2),
        (-1, -2),
        (-2, -1),
        (-2, 1),
        (-1, 2),
        (1, 2),
    ])
}

const fn attacks_from_offsets(offsets: &[(i8, i8); 8]) -> BySquare<Bitboard> {
    let mut arr = [Bitboard(0); 64];

    let mut sq_idx = 0u8;
    while sq_idx < 64 {
        let sq = Square::new_from_idx(sq_idx);
        let mut mask = 0u64;

        let mut offset_idx = 0usize;
        while offset_idx < offsets.len() {
            let (row_off, col_off) = offsets[offset_idx];
            if let Some(next) = sq.with_row_col_offset(row_off, col_off) {
                mask |= next.mask().0;
            }
            offset_idx += 1;
        }

        arr[sq_idx as usize] = Bitboard(mask);
        sq_idx += 1;
    }

    BySquare::new(arr)
}
