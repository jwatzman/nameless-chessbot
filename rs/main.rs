use bitflags::bitflags;

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

impl<T> ByPiece<T> {
    const ALL: [Piece; MAX_PIECE] = [
        Piece::Pawn,
        Piece::Bishop,
        Piece::Knight,
        Piece::Rook,
        Piece::Queen,
        Piece::King,
    ];
    fn iter(&self) -> impl Iterator<Item = (Piece, &T)> {
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

#[derive(Clone, Copy)]
struct Square(u8);

impl Square {
    pub fn new_from_row_col(row: u8, col: u8) -> Self {
        assert!(row < 8);
        assert!(col < 8);
        Square(col | (row << 3))
    }

    pub fn new_from_str(s: &str) -> Option<Self> {
        let mut chars = s.chars();

        let row = u8::try_from(chars.next()?.to_digit(36)?)
            .ok()?
            .checked_sub(10)?;
        let col = u8::try_from(chars.next()?.to_digit(10)?).ok()?;

        if row >= 8 || col >= 8 || chars.next().is_some() {
            None
        } else {
            Some(Self::new_from_row_col(row, col))
        }
    }

    pub fn row(&self) -> u8 {
        self.0 >> 3
    }

    pub fn col(&self) -> u8 {
        self.0 & 0b111
    }

    pub fn mask(&self) -> u64 {
        1u64 << self.0
    }
}

bitflags! {
  struct CastleRights: u8 {
    const W_KS = 0b0001;
    const B_KS = 0b0010;
    const W_QS = 0b0100;
    const B_QS = 0b1000;
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

    enpassant_index: Option<Square>,
    castle_rights: CastleRights,
    halfmove_count: u8,
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

                    let mask: u64 = Square::new_from_row_col(row, col).mask();
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
            Some('w') | Some('W') => Color::White,
            Some('b') | Some('B') => Color::Black,
            _ => return Err("Invalid active color"),
        };

        let mut castle_rights = CastleRights::empty();
        for castle_char in fen_fields.next().ok_or("Missing castle rights")?.chars() {
            let new_right = match castle_char {
                'K' => CastleRights::W_KS,
                'k' => CastleRights::B_KS,
                'Q' => CastleRights::W_QS,
                'q' => CastleRights::B_QS,
                _ => return Err("Unknown castle right"),
            };

            castle_rights |= new_right;
        }

        let enpassant_index_str = fen_fields.next().ok_or("Missing enpassant target")?;
        let enpassant_index = if enpassant_index_str.eq("-") {
            None
        } else {
            // Adjust: FEN records the square in the middle that the pawn passed
            // over. We record the destination square of the pawn move.
            let passed_over = Square::new_from_str(enpassant_index_str)
                .ok_or("Invalid enpassant target string")?;
            match passed_over.row() {
                2 => Some(Square::new_from_row_col(3, passed_over.col())),
                5 => Some(Square::new_from_row_col(4, passed_over.col())),
                _ => return Err("Invalid enpassant target square"),
            }
        };

        let halfmove_count: u8 = fen_fields
            .next()
            .ok_or("Missing halfmove count")?
            .parse()
            .map_err(|_| "Invalid halfmove count")?;

        Ok(Board {
            boards,
            composite_boards,
            full_composite,
            to_move,
            enpassant_index,
            castle_rights,
            halfmove_count,
        })
    }

    fn at(&self, sq: Square) -> Option<(Color, Piece)> {
        let mask = sq.mask();
        if self.full_composite.0 & mask == 0 {
            return None;
        }

        for (color, color_pieces) in self.composite_boards.iter() {
            if color_pieces.0 & mask == 0 {
                continue;
            }
            for (piece, pieces) in self.boards[color].iter() {
                if pieces.0 & mask != 0 {
                    return Some((color, piece));
                }
            }
            panic!("Composite color board marks piece but not found individual board")
        }

        panic!("Full composite board marks piece but not found in color composites")
    }
}

impl std::fmt::Display for Board {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        const SEPARATOR: &str = "+---+---+---+---+---+---+---+---+";
        write!(f, "{}\n", SEPARATOR)?;

        for row in (0u8..8u8).rev() {
            write!(f, "|")?;
            for col in 0u8..8u8 {
                let sigil = match self.at(Square::new_from_row_col(row, col)) {
                    None => ' ',
                    Some((Color::White, Piece::Pawn)) => 'P',
                    Some((Color::Black, Piece::Pawn)) => 'p',
                    Some((Color::White, Piece::Bishop)) => 'B',
                    Some((Color::Black, Piece::Bishop)) => 'b',
                    Some((Color::White, Piece::Knight)) => 'N',
                    Some((Color::Black, Piece::Knight)) => 'n',
                    Some((Color::White, Piece::Rook)) => 'R',
                    Some((Color::Black, Piece::Rook)) => 'r',
                    Some((Color::White, Piece::Queen)) => 'Q',
                    Some((Color::Black, Piece::Queen)) => 'q',
                    Some((Color::White, Piece::King)) => 'K',
                    Some((Color::Black, Piece::King)) => 'k',
                };
                write!(f, " {} |", sigil)?;
            }
            write!(f, "\n{}\n", SEPARATOR)?;
        }

        Ok(())
    }
}

fn main() {
    let b = Board::new();
    println!("{}", b);
}
