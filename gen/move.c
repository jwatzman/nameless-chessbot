void gen_move_king(void);
void gen_move_knight(void);
void gen_move_pawn(void);
void gen_move_between();

int main(void) {
  gen_move_king();
  gen_move_knight();
  gen_move_pawn();
  gen_move_between();
  return 0;
}
