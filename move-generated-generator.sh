#!/bin/bash

echo "// this is generated code! do not modify!" > move-generated.h
echo >> move-generated.h

for i in bb-tablegen-*.c; do
	echo "$i ..."
	gcc -o gen --std=c99 -O3 $i
	./gen >> move-generated.h
	echo >> move-generated.h
done;

rm gen
