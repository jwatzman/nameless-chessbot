use bitflags::bitflags;

use crate::color::Color;

bitflags! {
  #[derive(Clone, Copy, Eq, PartialEq)]
  pub struct CastleRights: u8 {
    const W_KS = 0b0001;
    const B_KS = 0b0010;
    const W_QS = 0b0100;
    const B_QS = 0b1000;
  }
}

impl CastleRights {
    pub fn both_color(c: Color) -> CastleRights {
        match c {
            Color::White => CastleRights::W_KS | CastleRights::W_QS,
            Color::Black => CastleRights::B_KS | CastleRights::B_QS,
        }
    }
}
