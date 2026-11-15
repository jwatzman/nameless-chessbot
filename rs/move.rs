#[derive(Clone, Copy, Eq, PartialEq)]
pub struct Move(u32);

impl Move {
    pub const NULL: Move = Move(0);
}
