CORE="bitboard.o move.o moveiter.o"
ALL="$CORE evaluate.o search.o timer.o"

redo-ifchange FLAGS
case $1 in
	perft|stress)
		redo-ifchange $CORE $1.o
		gcc $(cat FLAGS) -o "$3" $CORE $1.o
		;;
	search-perf|test|xboard)
		redo-ifchange $ALL $1.o
		gcc $(cat FLAGS) -o "$3" $ALL $1.o
		;;
	*)
		echo "Unknown binary $1"
		exit 1
		;;
esac
