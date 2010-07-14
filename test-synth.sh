#!/bin/sh -e

# This is just a quick script to exercise ssynth and midigen.  We compile
# those two programs if they aren't already there.

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
	echo -40; echo -43; echo -47
done | ./midigen | ./ssynth >/dev/dsp
