/// Newtype wrapper for a bitboard. LSB is A1, up through MSB is H8. (See
/// [`Square`].)
#[derive(Default)]
pub struct Bitboard(pub u64);

// TODO: implement iterator?
