use crate::{piece::Piece, square::Square};

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum MoveSpecial {
    EnPassant,
    Castle, // TODO: do we need this, or should we compute from src/dest/piece?
}

// TODO: is this better bitpacked or as an struct/enum?
#[repr(transparent)]
#[derive(Clone, Copy, Eq, PartialEq)]
pub struct Move(u32);

impl Move {
    pub const NULL: Move = Move(0);

    const SRC_OFFSET: usize = 0;
    const SRC_MASK: u32 = 0b111111;
    const DEST_OFFSET: usize = 6;
    const DEST_MASK: u32 = 0b111111;
    const PIECE_TYPE_OFFSET: usize = 12;
    const PIECE_TYPE_MASK: u32 = 0b111;
    const CAPTURED_TYPE_OFFSET: usize = 15;
    const CAPTURED_TYPE_MASK: u32 = 0b111;
    const PROMOTED_TYPE_OFFSET: usize = 18;
    const PROMOTED_TYPE_MASK: u32 = 0b111;
    const SPECIAL_OFFSET: usize = 21;
    const SPECIAL_MASK: u32 = 0b11;

    pub fn new(
        src: Square,
        dest: Square,
        piece: Piece,
        capture: Option<Piece>,
        promo: Option<Piece>,
        special: Option<MoveSpecial>,
    ) -> Self {
        debug_assert_ne!(capture, Some(Piece::King));
        if let Some(pp) = promo {
            debug_assert_ne!(pp, Piece::Pawn);
            debug_assert_eq!(piece, Piece::Pawn);
            debug_assert_eq!(special, None);
            debug_assert!(src.row() == 1 || src.row() == 6);
            debug_assert!(dest.row() == 0 || dest.row() == 7);
        }
        match special {
            None => (),
            Some(MoveSpecial::EnPassant) => {
                debug_assert!(src.row() == 2 || src.row() == 5);
                debug_assert!(dest.row() == 1 || dest.row() == 6);
                debug_assert_eq!(piece, Piece::Pawn);
                debug_assert_eq!(capture, None);
                debug_assert_eq!(promo, None);
            }
            Some(MoveSpecial::Castle) => {
                debug_assert!(src.row() == 0 || src.row() == 7);
                debug_assert_eq!(src.row(), dest.row());
                debug_assert_eq!(src.col(), 4);
                debug_assert!(dest.col() == 2 || dest.col() == 6);
                debug_assert_eq!(piece, Piece::King);
                debug_assert_eq!(capture, None);
                debug_assert_eq!(promo, None);
            }
        }

        let mut m = Move(0);

        m.0 |= u32::from(src.idx()) << Self::SRC_OFFSET;
        m.0 |= u32::from(dest.idx()) << Self::DEST_OFFSET;
        m.0 |= (piece as u32) << Self::PIECE_TYPE_OFFSET;

        let captured_type = match capture {
            None => 0u8,
            Some(p) => (p as u8) + 1,
        };
        m.0 |= u32::from(captured_type) << Self::CAPTURED_TYPE_OFFSET;

        let promoted_type = match promo {
            None => 0u8,
            Some(p) => (p as u8) + 1,
        };
        m.0 |= u32::from(promoted_type) << Self::PROMOTED_TYPE_OFFSET;

        let special_bits = match special {
            None => 0u32,
            Some(MoveSpecial::EnPassant) => 1,
            Some(MoveSpecial::Castle) => 2,
        };
        m.0 |= special_bits << Self::SPECIAL_OFFSET;

        m
    }

    pub fn is_null(&self) -> bool {
        *self == Self::NULL
    }

    pub fn src(&self) -> Square {
        debug_assert!(!self.is_null());
        Square::new_from_idx(((self.0 >> Self::SRC_OFFSET) & Self::SRC_MASK) as u8)
    }

    pub fn dest(&self) -> Square {
        debug_assert!(!self.is_null());
        Square::new_from_idx(((self.0 >> Self::DEST_OFFSET) & Self::DEST_MASK) as u8)
    }

    pub fn piece(&self) -> Piece {
        debug_assert!(!self.is_null());
        let p8 = ((self.0 >> Self::PIECE_TYPE_OFFSET) & Self::PIECE_TYPE_MASK) as u8;
        Piece::try_from(p8).unwrap()
    }

    pub fn capture(&self) -> Option<Piece> {
        debug_assert!(!self.is_null());
        let p8 = ((self.0 >> Self::CAPTURED_TYPE_OFFSET) & Self::CAPTURED_TYPE_MASK) as u8;
        if p8 == 0 {
            None
        } else {
            Some(Piece::try_from(p8 - 1).unwrap())
        }
    }

    pub fn promo(&self) -> Option<Piece> {
        debug_assert!(!self.is_null());
        let p8 = ((self.0 >> Self::PROMOTED_TYPE_OFFSET) & Self::PROMOTED_TYPE_MASK) as u8;
        if p8 == 0 {
            None
        } else {
            Some(Piece::try_from(p8 - 1).unwrap())
        }
    }

    pub fn special(&self) -> Option<MoveSpecial> {
        debug_assert!(!self.is_null());
        match (self.0 >> Self::SPECIAL_OFFSET) & Self::SPECIAL_MASK {
            0 => None,
            1 => Some(MoveSpecial::EnPassant),
            2 => Some(MoveSpecial::Castle),
            _ => panic!("Invalid special bits"),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn simple_roundtrip() {
        let src = Square::new_from_row_col(0, 0);
        let dest = Square::new_from_row_col(7, 0);
        let piece = Piece::Rook;
        let capture = None;
        let promo = None;
        let special = None;

        let m = Move::new(src, dest, piece, capture, promo, special);
        assert_eq!(m.src(), src);
        assert_eq!(m.dest(), dest);
        assert_eq!(m.piece(), piece);
        assert_eq!(m.capture(), capture);
        assert_eq!(m.promo(), promo);
        assert_eq!(m.special(), special);
    }

    #[test]
    fn capture_promo_roundtrip() {
        let src = Square::new_from_row_col(6, 3);
        let dest = Square::new_from_row_col(7, 2);
        let piece = Piece::Pawn;
        let capture = Some(Piece::Bishop);
        let promo = Some(Piece::Queen);
        let special = None;

        let m = Move::new(src, dest, piece, capture, promo, special);
        assert_eq!(m.src(), src);
        assert_eq!(m.dest(), dest);
        assert_eq!(m.piece(), piece);
        assert_eq!(m.capture(), capture);
        assert_eq!(m.promo(), promo);
        assert_eq!(m.special(), special);
    }

    #[test]
    fn enpassant_roundtrip() {
        let src = Square::new_from_row_col(2, 1);
        let dest = Square::new_from_row_col(1, 0);
        let piece = Piece::Pawn;
        let capture = None;
        let promo = None;
        let special = Some(MoveSpecial::EnPassant);

        let m = Move::new(src, dest, piece, capture, promo, special);
        assert_eq!(m.src(), src);
        assert_eq!(m.dest(), dest);
        assert_eq!(m.piece(), piece);
        assert_eq!(m.capture(), capture);
        assert_eq!(m.promo(), promo);
        assert_eq!(m.special(), special);
    }
}
