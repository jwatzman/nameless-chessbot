#[derive(Clone, Copy)]
enum Color {
    White = 0,
    Black = 1,
}
const MAX_COLOR: usize = 2;

#[derive(Default)]
struct ByColor<T>([T; MAX_COLOR]);

impl<T> ByColor<T> {
    const ALL: [Color; MAX_COLOR] = [Color::White, Color::Black];
    fn iter(&self) -> impl Iterator<Item = (Color, &T)> {
        Self::ALL.into_iter().zip(self.0.iter())
    }
}

impl<T> std::ops::Index<Color> for ByColor<T> {
    type Output = T;
    fn index(&self, c: Color) -> &Self::Output {
        &self.0[c as usize]
    }
}

impl<T> std::ops::IndexMut<Color> for ByColor<T> {
    fn index_mut(&mut self, c: Color) -> &mut Self::Output {
        &mut self.0[c as usize]
    }
}

#[derive(Clone, Copy)]
enum Piece {
    Pawn = 0,
    Bishop = 1,
    Knight = 2,
    Rook = 3,
    Queen = 4,
    King = 5,
}
const MAX_PIECE: usize = 6;

#[derive(Default)]
struct ByPiece<T>([T; MAX_PIECE]);

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

// TODO: implement iterator?
#[derive(Default)]
struct Bitboard(pub u64);

struct Board {
    boards: ByColor<ByPiece<Bitboard>>,
    composite_boards: ByColor<Bitboard>,
    full_composite: Bitboard,

    to_move: Color,
}

fn board_index_of(row: u8, col: u8) -> u8 {
    assert!(row < 8);
    assert!(col < 8);
    col | (row << 3)
}

impl Board {
    pub fn new() -> Self {
        Self::new_with_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1").unwrap()
    }

    pub fn new_with_fen(fen: &str) -> Result<Self, &'static str> {
        let mut boards: ByColor<ByPiece<Bitboard>> = ByColor::default();
        let mut composite_boards: ByColor<Bitboard> = ByColor::default();
        let mut full_composite = Bitboard::default();

        let mut fen_fields = fen.split_whitespace();
        for (inverse_row, row_str) in fen_fields
            .next()
            .ok_or("Missing placements")?
            .split('/')
            .enumerate()
        {
            if inverse_row > 7 {
                return Err("Too many ranks");
            }
            let row = 7u8 - u8::try_from(inverse_row).unwrap();
            let mut col = 0u8;

            for c in row_str.chars() {
                if col > 7 {
                    return Err("Too many files");
                }
                if let Some(n) = c.to_digit(10) {
                    match u8::try_from(n) {
                        Err(_) | Ok(0) | Ok(9) => return Err("Invalid empty files"),
                        Ok(n) => col += n,
                    }
                } else {
                    let (color, piece) = match c {
                        'P' => (Color::White, Piece::Pawn),
                        'p' => (Color::Black, Piece::Pawn),
                        'B' => (Color::White, Piece::Bishop),
                        'b' => (Color::Black, Piece::Bishop),
                        'N' => (Color::White, Piece::Knight),
                        'n' => (Color::Black, Piece::Knight),
                        'R' => (Color::White, Piece::Rook),
                        'r' => (Color::Black, Piece::Rook),
                        'Q' => (Color::White, Piece::Queen),
                        'q' => (Color::Black, Piece::Queen),
                        'K' => (Color::White, Piece::King),
                        'k' => (Color::Black, Piece::King),
                        _ => return Err("Invalid piece"),
                    };

                    let mask: u64 = 1u64 << board_index_of(row, col);
                    assert!(boards[color][piece].0 & mask == 0);
                    assert!(composite_boards[color].0 & mask == 0);
                    assert!(full_composite.0 & mask == 0);
                    boards[color][piece].0 |= mask;
                    composite_boards[color].0 |= mask;
                    full_composite.0 |= mask;
                    col += 1;
                }
            }
        }

        for (_, color_pieces) in boards.iter() {
            if color_pieces[Piece::King].0.count_ones() != 1 {
                return Err("Invalid kings");
            }
        }

        let to_move = match fen_fields
            .next()
            .ok_or("Missing active color")?
            .chars()
            .next()
        {
            Some('w') => Color::White,
            Some('b') => Color::Black,
            _ => return Err("Invalid active color"),
        };

        // TODO: castling
        // TODO: en passant
        // TODO: halfmove clock
        // TODO: fullmove number

        Ok(Board {
            boards,
            composite_boards,
            full_composite,
            to_move,
        })
    }
}

fn main() {
    Board::new();
    println!("Hello, world!");
}
