#[derive(Clone, Copy)]
pub enum Piece {
    Pawn = 0,
    Bishop = 1,
    Knight = 2,
    Rook = 3,
    Queen = 4,
    King = 5,
}
const MAX_PIECE: usize = 6;

#[derive(Default)]
pub struct ByPiece<T>([T; MAX_PIECE]);

impl<T> ByPiece<T> {
    const ALL: [Piece; MAX_PIECE] = [
        Piece::Pawn,
        Piece::Bishop,
        Piece::Knight,
        Piece::Rook,
        Piece::Queen,
        Piece::King,
    ];
    pub fn iter(&self) -> impl Iterator<Item = (Piece, &T)> {
        Self::ALL.into_iter().zip(self.0.iter())
    }
}

impl<T> std::ops::Index<Piece> for ByPiece<T> {
    type Output = T;
    fn index(&self, p: Piece) -> &Self::Output {
        &self.0[p as usize]
    }
}

impl<T> std::ops::IndexMut<Piece> for ByPiece<T> {
    fn index_mut(&mut self, p: Piece) -> &mut Self::Output {
        &mut self.0[p as usize]
    }
}
