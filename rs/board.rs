use crate::bitboard::Bitboard;
use crate::castle::CastleRights;
use crate::color::ByColor;
use crate::color::Color;
use crate::moves::Move;
use crate::piece::ByPiece;
use crate::piece::Piece;
use crate::sigil;
use crate::square::BySquare;
use crate::square::Square;

pub struct Board {
    boards: ByColor<ByPiece<Bitboard>>,
    composite_boards: ByColor<Bitboard>,
    full_composite: Bitboard,

    to_move: Color,

    enpassant_index: Option<Square>,
    castle_rights: CastleRights,
    halfmove_count: u8,

    zobrist: u64,
    zobrist_sq: ByColor<ByPiece<BySquare<u64>>>,
    zobrist_castle: [u64; 16],
    zobrist_enpassant: [u64; 8],
    zobrist_black: u64,
}

pub struct UnmakeMove {
    m: Move,
    enpassant_index: Option<Square>,
    castle_rights: CastleRights,
    halfmove_count: u8,
}

impl Board {
    pub fn new(rand: &mut dyn rand_core::Rng) -> Self {
        Self::new_with_fen(
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            rand,
        )
        .unwrap()
    }

    pub fn new_with_fen(fen: &str, rand: &mut dyn rand_core::Rng) -> Result<Self, &'static str> {
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
                    let (color, piece) = sigil::parse(c).ok_or("Invalid piece")?;

                    let mask = Square::new_from_row_col(row, col).mask();
                    assert!(boards[color][piece] & mask == 0);
                    assert!(composite_boards[color] & mask == 0);
                    assert!(full_composite & mask == 0);
                    boards[color][piece] |= mask;
                    composite_boards[color] |= mask;
                    full_composite |= mask;
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

            castle_rights.insert(new_right);
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

        let zobrist = rand.next_u64();
        let zobrist_sq = ByColor::new(std::array::from_fn(|_| {
            ByPiece::new(std::array::from_fn(|_| {
                BySquare::new(std::array::from_fn(|_| rand.next_u64()))
            }))
        }));
        let zobrist_castle = std::array::from_fn(|_| rand.next_u64());
        let zobrist_enpassant = std::array::from_fn(|_| rand.next_u64());
        let zobrist_black = rand.next_u64();

        Ok(Board {
            boards,
            composite_boards,
            full_composite,
            to_move,
            enpassant_index,
            castle_rights,
            halfmove_count,
            zobrist,
            zobrist_sq,
            zobrist_castle,
            zobrist_enpassant,
            zobrist_black,
        })
    }

    fn at(&self, sq: Square) -> Option<(Color, Piece)> {
        let mask = sq.mask();
        if self.full_composite & mask == 0 {
            return None;
        }

        for (color, color_pieces) in self.composite_boards.iter() {
            if *color_pieces & mask == 0 {
                continue;
            }
            for (piece, pieces) in self.boards[color].iter() {
                if *pieces & mask != 0 {
                    return Some((color, piece));
                }
            }
            panic!("Composite color board marks piece but not found individual board")
        }

        panic!("Full composite board marks piece but not found in color composites")
    }

    pub fn make_move(&mut self, m: Move) -> UnmakeMove {
        let u = UnmakeMove {
            m,
            enpassant_index: self.enpassant_index,
            castle_rights: self.castle_rights,
            halfmove_count: self.halfmove_count,
        };

        let src = m.src();
        let dest = m.dest();
        let p = m.piece();
        let capture = m.capture();
        let promo = m.promo();
        let c = self.to_move;

        debug_assert_eq!(self.at(src), Some((c, p)));
        match capture {
            None => debug_assert_eq!(self.at(dest), None),
            Some(cp) => debug_assert_eq!(self.at(dest), Some((!c, cp))),
        }

        if p == Piece::Pawn || capture.is_some() {
            self.halfmove_count = 0;
        } else {
            self.halfmove_count += 1;
        }

        if !self.castle_rights.is_empty() {
            self.zobrist ^= self.zobrist_castle[usize::from(self.castle_rights.bits())];

            // TODO -- castle rights
            if src.idx() == 0 || dest.idx() == 0 {
                self.castle_rights.remove(CastleRights::W_QS);
            }
            if src.idx() == 7 || dest.idx() == 7 {
                self.castle_rights.remove(CastleRights::W_KS);
            }
            if src.idx() == 56 || dest.idx() == 56 {
                self.castle_rights.remove(CastleRights::B_QS);
            }
            if src.idx() == 63 || dest.idx() == 63 {
                self.castle_rights.remove(CastleRights::B_KS);
            }

            if p == Piece::King {
                self.castle_rights.remove(CastleRights::both_color(c));
            }

            self.zobrist ^= self.zobrist_castle[usize::from(self.castle_rights.bits())];
        }

        // TODO -- enpassant

        self.make_unmake_common(m);
        self.to_move = !self.to_move;

        debug_assert_eq!(self.boards[Color::White][Piece::King].0.count_ones(), 1);
        debug_assert_eq!(self.boards[Color::Black][Piece::King].0.count_ones(), 1);

        debug_assert_eq!(
            self.full_composite,
            self.composite_boards[Color::White] | self.composite_boards[Color::Black]
        );

        debug_assert_eq!(
            self.composite_boards[Color::White] & self.composite_boards[Color::Black],
            0
        );

        u
    }

    pub fn unmake_move(&mut self, u: UnmakeMove) -> () {
        self.to_move = !self.to_move;
        self.make_unmake_common(u.m);

        // TODO -- copy out from UnmakeMove
    }

    // TODO: should this take a const template parameter to skip computing
    // zobrist, which we can put into the UnmakeMove? And we can use to compute
    // the NNUE value when we have that?
    fn make_unmake_common(&mut self, m: Move) -> () {
        // TODO

        self.zobrist ^= self.zobrist_black;
    }

    fn toggle(&mut self, color: Color, piece: Piece, sq: Square) {
        let mask = sq.mask();
        self.boards[color][piece] ^= mask;
        self.composite_boards[color] ^= mask;
        self.full_composite ^= mask;

        self.zobrist ^= self.zobrist_sq[color][piece][sq];
    }

    pub fn zobrist(&self) -> u64 {
        self.zobrist
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
                    Some((c, p)) => sigil::unparse(c, p),
                };
                write!(f, " {} |", sigil)?;
            }
            write!(f, "\n{}\n", SEPARATOR)?;
        }

        Ok(())
    }
}
