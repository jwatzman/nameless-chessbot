mod bitboard;
mod board;
mod castle;
mod color;
mod r#move;
mod movelist;
mod movemagic;
mod piece;
mod sigil;
mod square;

use board::Board;
use rand_core::SeedableRng;

fn bbviz(mut args: impl Iterator<Item = String>) -> () {
    let bb_str = match args.next() {
        Some(s) => s,
        None => {
            println!("bbviz: missing board");
            return;
        }
    };

    let maybe_bb = match bb_str.strip_prefix("0x") {
        Some(s) => u64::from_str_radix(s, 16),
        None => u64::from_str_radix(bb_str.as_str(), 10),
    };

    let bb = match maybe_bb {
        Err(_) => {
            println!("bbviz: unable to parse {}", bb_str);
            return;
        }
        Ok(n) => n,
    };

    print!("{}", bitboard::Bitboard(bb));
}

fn main() {
    let mut args = std::env::args().skip(1);
    match args.next().as_deref() {
        Some("bbviz") => bbviz(args),
        Some(unk) => {
            println!("Unknown command: {}", unk);
        }
        None => {
            let b = Board::new(&mut rand_xoshiro::Xoshiro256PlusPlus::seed_from_u64(1));
            println!("{}", b);

            let sq = square::Square::new_from_idx(36);
            let occ = bitboard::Bitboard(0x08001000);
            println!("{}", sq.mask());
            println!("{}", occ);
            println!("{}", movemagic::rook_attacks(sq, occ));
            println!("{}", movemagic::bishop_attacks(sq, occ));
        }
    }
}
