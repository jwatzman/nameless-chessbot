CORE="bitboard.o move.o moveiter.o movemagic.o"
ALL="$CORE evaluate.o search.o timer.o"

redo-ifchange CC FLAGS
case $1 in
	perft|stress)
		redo-ifchange $CORE $1.o
		$(cat CC) $(cat FLAGS) -o "$3" $CORE $1.o
		;;
	search-perf|test|xboard)
		redo-ifchange $ALL $1.o
		$(cat CC) $(cat FLAGS) -o "$3" $ALL $1.o
		;;
	*)
		echo "Unknown binary $1"
		exit 1
		;;
esac
