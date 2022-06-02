redo-ifchange ../bin/CC "$2.c"
$(cat ../bin/CC) -o "$3" -I ../src/ -O2 -Wall -Wextra -MD -MF "$2.d" "$2.c"
read DEPS < "$2.d"
rm "$2.d"
redo-ifchange ${DEPS#*:}
