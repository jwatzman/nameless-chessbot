use crate::r#move::Move;

const MAX_MOVES: usize = 256usize;

pub struct Movelist {
    moves: [Move; MAX_MOVES],
    n: usize,
}

impl Movelist {
    pub fn new() -> Self {
        Movelist {
            // TODO: this preemptively zeroes the array -- do we want
            // MaybeUninit or even arrayvec here instead?
            moves: [Move::NULL; MAX_MOVES],
            n: 0,
        }
    }

    pub fn push(&mut self, m: Move) {
        self.moves[self.n] = m;
        self.n += 1;
    }
}
