mod bitboard;
mod board;
mod castle;
mod color;
mod piece;
mod sigil;
mod square;

use board::Board;

fn main() {
    let b = Board::new();
    println!("{}", b);
}
