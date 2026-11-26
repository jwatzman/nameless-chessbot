use crate::{bitboard::Bitboard, square::BySquare};

mod king_knight;

pub static KING_ATTACKS: BySquare<Bitboard> = king_knight::king_attacks();
pub static KNIGHT_ATTACKS: BySquare<Bitboard> = king_knight::knight_attacks();

#[cfg(test)]
mod tests {
    use crate::square::Square;

    use super::*;

    #[test]
    fn smoke() {
        let sq = Square::new_from_idx(36);
        assert_eq!(KING_ATTACKS[sq], 0x382838000000);
        assert_eq!(KNIGHT_ATTACKS[sq], 0x28440044280000);
    }
}
