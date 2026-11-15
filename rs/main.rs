mod bitboard;
mod board;
mod castle;
mod color;
mod r#move;
mod movelist;
mod piece;
mod sigil;
mod square;

use board::Board;

fn main() {
    let b = Board::new();
    println!("{}", b);
}
