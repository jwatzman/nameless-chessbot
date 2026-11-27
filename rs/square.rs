use crate::bitboard;

/// Represents a square on the board. Everything is zero-indexed. "Row 0" is
/// rank 1, up through "row 7" is rank 8. "Col 0" is file A, up through "col 7"
/// is file H. "Pos 0" is then A1, "pos 1" is B1, etc up through "pos 63" is H8.
#[repr(transparent)]
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct Square(u8);

impl Square {
    pub const fn new_from_idx(idx: u8) -> Self {
        debug_assert!(idx < 64);
        Square(idx)
    }

    pub const fn new_from_row_col(row: u8, col: u8) -> Self {
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

    pub const fn with_row_col_offset(&self, row_off: i8, col_off: i8) -> Option<Self> {
        let new_row = (self.row() as i16) + (row_off as i16);
        let new_col = (self.col() as i16) + (col_off as i16);

        if new_row < 0 || new_row >= 8 || new_col < 0 || new_col >= 8 {
            None
        } else {
            Some(Self::new_from_row_col(new_row as u8, new_col as u8))
        }
    }

    pub const fn idx(&self) -> u8 {
        return self.0;
    }

    pub const fn row(&self) -> u8 {
        self.0 >> 3
    }

    pub const fn col(&self) -> u8 {
        self.0 & 0b111
    }

    pub const fn mask(&self) -> bitboard::Bitboard {
        bitboard::Bitboard(1u64 << self.0)
    }
}

#[repr(transparent)]
pub struct BySquare<T>([T; 64]);

impl<T> BySquare<T> {
    pub const fn new(arr: [T; 64]) -> Self {
        BySquare(arr)
    }

    pub fn iter(&self) -> impl Iterator<Item = (Square, &T)> {
        (0u8..64).map(Square::new_from_idx).zip(self.0.iter())
    }
}

impl<T> std::ops::Index<Square> for BySquare<T> {
    type Output = T;

    fn index(&self, sq: Square) -> &Self::Output {
        &self.0[usize::from(sq.idx())]
    }
}

impl<T> std::ops::IndexMut<Square> for BySquare<T> {
    fn index_mut(&mut self, sq: Square) -> &mut Self::Output {
        &mut self.0[usize::from(sq.idx())]
    }
}
