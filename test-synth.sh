#!/bin/sh -e

comp () { gcc -lm -std=gnu99 $1.c midimsg.c -o $1 2>/dev/null; }
  
[ ! -e midigen ] && comp midigen && true
[ ! -e ssynth ] && comp ssynth && true

while true
do
	echo 64; sleep 0.5
	echo -64
	echo 62; sleep 0.5
	echo -62
	echo 40; sleep 0.3
	echo 43; sleep 0.3
	echo 47; sleep 0.6
	echo -e '-40\n-43\n-47'
done | ./midigen | ./ssynth >/dev/dsp
