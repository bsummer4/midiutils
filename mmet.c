#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <fcntl.h>
#include "midimsg.h"
#include <stdio.h>
#include "util.h"

int main (void) {
	struct mm_msg m;
	for (int ticks = 0;;) {
		if (!mm_read(0, &m)) return 0;
		mm_write(1, &m);
		if (m.type != MM_CLOCK) continue;
		ticks++;
		switch (ticks) {
			case 24: ticks = 0;
			case 0: m = (struct mm_msg){MM_NOTEON, 9, 60, 128}; break;
			case 2: m = (struct mm_msg){MM_NOTEOFF, 9, 60, 0}; break;
			default: continue; }
		mm_write(1, &m); }
	return 0; }
