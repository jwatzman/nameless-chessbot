use crate::{bitboard::Bitboard, color::ByColor, square::BySquare};

mod offsets;

pub static KING_ATTACKS: BySquare<Bitboard> = offsets::king_attacks();
pub static KNIGHT_ATTACKS: BySquare<Bitboard> = offsets::knight_attacks();
pub static PAWN_ATTACKS: ByColor<BySquare<Bitboard>> = offsets::pawn_attacks();

#[cfg(test)]
mod tests {
    use crate::{color::Color, square::Square};

    use super::*;

    #[test]
    fn smoke() {
        let sq = Square::new_from_idx(36);
        assert_eq!(KING_ATTACKS[sq], 0x382838000000);
        assert_eq!(KNIGHT_ATTACKS[sq], 0x28440044280000);
        assert_eq!(PAWN_ATTACKS[Color::White][sq], 0x280000000000);
        assert_eq!(PAWN_ATTACKS[Color::Black][sq], 0x28000000);
    }
}
