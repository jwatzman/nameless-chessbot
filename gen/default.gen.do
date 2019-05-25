redo-ifchange "$2.c"
gcc -o "$3" -O2 -Wall -Wextra -MD -MF "$2.d" "$2.c"
read DEPS < "$2.d"
rm "$2.d"
redo-ifchange ${DEPS#*:}
