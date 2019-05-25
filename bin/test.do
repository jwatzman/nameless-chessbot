DEPS="bitboard.o move.o moveiter.o evaluate.o search.o timer.o test.o"
redo-ifchange $DEPS FLAGS
gcc $(cat FLAGS) -o "$3" $DEPS
