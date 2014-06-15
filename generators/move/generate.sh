#!/bin/bash

cd `dirname $0`

echo "// this is generated code! do not modify!"
echo

for i in *.c
do
	${CC} ${CFLAGS} -c -I../../ -o gen.o $i
	${CC} ${LDFLAGS} -I../../ -o gen gen.o
	./gen
	echo
done

rm gen
