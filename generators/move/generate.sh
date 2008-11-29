#!/bin/bash

cd `dirname $0`

echo "// this is generated code! do not modify!"
echo

for i in *.c
do
	gcc -I../../ -o gen --std=c99 -O3 $i
	./gen
	echo
done

rm gen
