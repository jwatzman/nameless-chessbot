use crate::{piece::Piece, square::Square};

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

    pub fn new(
        src: Square,
        dest: Square,
        piece: Piece,
        capture: Option<Piece>,
        promo: Option<Piece>,
    ) -> Self {
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
}
