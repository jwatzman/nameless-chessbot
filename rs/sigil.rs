use crate::color::Color;
use crate::piece::Piece;

pub fn parse(c: char) -> Option<(Color, Piece)> {
    match c {
        'P' => Some((Color::White, Piece::Pawn)),
        'p' => Some((Color::Black, Piece::Pawn)),
        'B' => Some((Color::White, Piece::Bishop)),
        'b' => Some((Color::Black, Piece::Bishop)),
        'N' => Some((Color::White, Piece::Knight)),
        'n' => Some((Color::Black, Piece::Knight)),
        'R' => Some((Color::White, Piece::Rook)),
        'r' => Some((Color::Black, Piece::Rook)),
        'Q' => Some((Color::White, Piece::Queen)),
        'q' => Some((Color::Black, Piece::Queen)),
        'K' => Some((Color::White, Piece::King)),
        'k' => Some((Color::Black, Piece::King)),
        _ => None,
    }
}

pub fn unparse(c: Color, p: Piece) -> char {
    match (c, p) {
        (Color::White, Piece::Pawn) => 'P',
        (Color::Black, Piece::Pawn) => 'p',
        (Color::White, Piece::Bishop) => 'B',
        (Color::Black, Piece::Bishop) => 'b',
        (Color::White, Piece::Knight) => 'N',
        (Color::Black, Piece::Knight) => 'n',
        (Color::White, Piece::Rook) => 'R',
        (Color::Black, Piece::Rook) => 'r',
        (Color::White, Piece::Queen) => 'Q',
        (Color::Black, Piece::Queen) => 'q',
        (Color::White, Piece::King) => 'K',
        (Color::Black, Piece::King) => 'k',
    }
}
