IN="../src/$2.c"
redo-ifchange CC FLAGS ../gen/evaluate.h ../gen/move.h "$IN"
$(cat CC) $(cat FLAGS) -MD -MF "$2.d" -c -o "$3" "$IN"
read DEPS < "$2.d"
rm "$2.d"
redo-ifchange ${DEPS#*:}
