DEPS="bitboard.o move.o moveiter.o stress.o"
redo-ifchange $DEPS FLAGS
gcc $(cat FLAGS) -o "$3" $DEPS
