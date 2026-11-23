use bitflags::bitflags;

bitflags! {
  #[derive(Clone, Copy)]
  pub struct CastleRights: u8 {
    const W_KS = 0b0001;
    const B_KS = 0b0010;
    const W_QS = 0b0100;
    const B_QS = 0b1000;
  }
}
