use crate::movemagic::consts::{B_SHIFT, R_SHIFT};

pub const fn get_rook_attacks_size() -> usize {
    let mut tot: usize = 0;

    let mut idx: usize = 0;
    while idx < R_SHIFT.len() {
        tot += R_SHIFT[idx] as usize;
        idx += 1;
    }

    tot
}

pub const fn get_bishop_attacks_size() -> usize {
    let mut tot: usize = 0;

    let mut idx: usize = 0;
    while idx < B_SHIFT.len() {
        tot += B_SHIFT[idx] as usize;
        idx += 1;
    }

    tot
}
