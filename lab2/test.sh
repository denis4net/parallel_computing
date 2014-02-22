#!/bin/sh

N=1024
if [ -n "$1" ]; then
	N=$1
fi

echo "Matrix size: $N"

for i in bin/*.elf; do
	echo "Launching $i: "
	time $i $N
	echo "" 
done
