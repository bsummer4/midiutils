/*
	# Midigen
	This is a quick hack; It's currently unsafe and downright-bad code.

	We translate ascii numbers into midi messages.  Each number must be on
	it's own line.  Here's an example:

		midigen | ssynth >/dev/dsp
		60 # -> midi "Note On" message with midi pitch 60
		80 # -> midi "Note On" message with midi pitch 80
		-60 # -> midi "Note Off" message with midi pitch 60
		-80 # -> midi "Note Off" message with midi pitch 80
*/

#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "midimsg.h"

typedef struct mm_msg M;

int main (void) {
	char buf[1024];
	struct mm_msg m;
	while (fgets(buf, 1024, stdin)) {
		int num = atoi(buf);
		int off = num<0;
		int type = off?MM_NOTEOFF:MM_NOTEON;
		m = (M){type, 0, num>0?num:-num, off?0:128};
		mm_write(1, &m); }
	return 0; }
