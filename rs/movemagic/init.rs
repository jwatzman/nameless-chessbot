use crate::movemagic::{Entry, consts};

pub const fn get_rook_entries() -> [Entry; 64] {
    get_entries(&consts::R_SHIFT, &consts::R_MAGIC, &consts::R_MASK)
}

pub const fn get_bishop_entries() -> [Entry; 64] {
    get_entries(&consts::B_SHIFT, &consts::B_MAGIC, &consts::B_MASK)
}

const fn get_entries(shifts: &[u8; 64], magics: &[u64; 64], masks: &[u64; 64]) -> [Entry; 64] {
    let mut entries: [Entry; 64] = [const {
        Entry {
            mask: 0,
            magic: 0,
            base: 0,
            shift: 0,
        }
    }; 64];
    let mut base: usize = 0;

    let mut idx: usize = 0;
    while idx < 64 {
        entries[idx] = Entry {
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
