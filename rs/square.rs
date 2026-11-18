/// Represents a square on the board. Everything is zero-indexed. "Row 0" is
/// rank 1, up through "row 7" is rank 8. "Col 0" is file A, up through "col 7"
/// is file H. "Pos 0" is then A1, "pos 1" is B1, etc up through "pos 63" is H8.
#[derive(Clone, Copy, Eq, PartialEq)]
pub struct Square(u8);

impl Square {
    pub fn new_from_idx(idx: u8) -> Self {
        debug_assert!(idx < 64);
        Square(idx)
    }

    pub fn new_from_row_col(row: u8, col: u8) -> Self {
        debug_assert!(row < 8);
        debug_assert!(col < 8);
        Self::new_from_idx(col | (row << 3))
    }

    pub fn new_from_str(s: &str) -> Option<Self> {
        let mut chars = s.chars();

        let row = u8::try_from(chars.next()?.to_digit(36)?)
            .ok()?
            .checked_sub(10)?;
        let col = u8::try_from(chars.next()?.to_digit(10)?).ok()?;

        if row >= 8 || col >= 8 || chars.next().is_some() {
            None
        } else {
            Some(Self::new_from_row_col(row, col))
        }
    }

    pub fn idx(&self) -> u8 {
        return self.0;
    }

    pub fn row(&self) -> u8 {
        self.0 >> 3
    }

    pub fn col(&self) -> u8 {
        self.0 & 0b111
    }

    pub fn mask(&self) -> u64 {
        1u64 << self.0
    }
}
