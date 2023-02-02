void gen_move_king(void);
void gen_move_knight(void);
void gen_move_pawn(void);
void gen_move_raycast(void);

int main(void) {
  gen_move_king();
  gen_move_knight();
  gen_move_pawn();
  gen_move_raycast();
  return 0;
}
