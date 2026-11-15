#[derive(Clone, Copy)]
pub struct Square(u8);

impl Square {
    pub fn new_from_row_col(row: u8, col: u8) -> Self {
        assert!(row < 8);
        assert!(col < 8);
        Square(col | (row << 3))
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
