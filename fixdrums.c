/*
	# mjoin

	Joins multiple midi streams (stdin and any number of fifos)
	into one.

		mticks 120 | mjoin /dev/midi1 /dev/midi2 | ./ssynth >/dev/dsp
*/

#include <stdio.h>
#include "midimsg.h"
#include "util.h"

int main (int argc, char **argv) {
	struct mm_msg m;
	for (;;) {
		if (!mm_read(0, &m)) break;
		if (m.type == MM_NOTEON || m.type == MM_NOTEOFF)
			switch (m.arg1) { case 31: m.arg1 = 38; }
		mm_write(1, &m); }
	return 0; }
