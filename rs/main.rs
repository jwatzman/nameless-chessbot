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

fn main() {
    let b = Board::new();
    println!("{}", b);
}
