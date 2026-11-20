use crate::square;

/// Newtype wrapper for a bitboard. LSB is A1, up through MSB is H8. (See
/// [`Square`].)
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub struct Bitboard(pub u64);

impl Bitboard {
    pub fn square_iterator(&self) -> BitboardSquareIterator {
        BitboardSquareIterator(self.0)
    }
}

impl PartialEq<u64> for Bitboard {
    fn eq(&self, other: &u64) -> bool {
        self.0 == *other
    }
}

impl std::ops::BitAnd<u64> for Bitboard {
    type Output = Bitboard;

    fn bitand(self, rhs: u64) -> Self::Output {
        Bitboard(self.0 & rhs)
    }
}

impl std::ops::BitAnd<Bitboard> for Bitboard {
    type Output = Bitboard;

    fn bitand(self, rhs: Bitboard) -> Self::Output {
        Bitboard(self.0 & rhs.0)
    }
}

impl std::ops::BitOr<u64> for Bitboard {
    type Output = Bitboard;

    fn bitor(self, rhs: u64) -> Self::Output {
        Bitboard(self.0 | rhs)
    }
}

impl std::ops::BitOr<Bitboard> for Bitboard {
    type Output = Bitboard;

    fn bitor(self, rhs: Bitboard) -> Self::Output {
        Bitboard(self.0 | rhs.0)
    }
}

impl std::ops::BitOrAssign<u64> for Bitboard {
    fn bitor_assign(&mut self, rhs: u64) {
        self.0 |= rhs
    }
}

impl std::ops::BitOrAssign<Bitboard> for Bitboard {
    fn bitor_assign(&mut self, rhs: Bitboard) {
        self.0 |= rhs.0
    }
}

impl std::fmt::Display for Bitboard {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        for row in (0u8..8u8).rev() {
            for col in 0u8..8u8 {
                let sq = square::Square::new_from_row_col(row, col);
                if *self & sq.mask() == 0 {
                    write!(f, ".")?;
                } else {
                    write!(f, "*")?;
                }
            }
            write!(f, "\n")?;
        }

        Ok(())
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
