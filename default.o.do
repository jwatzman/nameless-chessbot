redo-ifchange FLAGS gen/evaluate.h gen/move.h "$2.c"
gcc $(cat FLAGS) -MD -MF "$2.d" -c -o "$3" "$2.c"
read DEPS < "$2.d"
rm "$2.d"
redo-ifchange ${DEPS#*:}
