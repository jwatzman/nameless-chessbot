use crate::square;

/// Newtype wrapper for a bitboard. LSB is A1, up through MSB is H8. (See
/// [`Square`].)
#[derive(Clone, Copy, Default)]
pub struct Bitboard(pub u64);

impl Bitboard {
    pub fn square_iterator(&self) -> BitboardSquareIterator {
        BitboardSquareIterator(self.0)
    }
}

pub struct BitboardSquareIterator(u64);

impl Iterator for BitboardSquareIterator {
    type Item = square::Square;

    fn next(&mut self) -> Option<Self::Item> {
        if self.0 == 0 {
            None
        } else {
            let idx = self.0.trailing_zeros();
            self.0 &= self.0 - 1;
            debug_assert!(u8::try_from(idx).is_ok());
            Some(square::Square::new_from_idx(idx as u8))
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn iter() {
        let bb = Bitboard(0b1101);
        let mut it = bb.square_iterator();

        assert_eq!(it.next().unwrap().idx(), 0);
        assert_eq!(it.next().unwrap().idx(), 2);
        assert_eq!(it.next().unwrap().idx(), 3);
        assert!(it.next().is_none());
    }
}
