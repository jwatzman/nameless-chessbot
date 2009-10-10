#!/bin/bash

cd `dirname $0`

echo "// this is generated code! do not modify!"
echo

for i in *.c
do
	${CC} ${CFLAGS} -I../../ -o gen $i
	./gen
	echo
done

rm gen
