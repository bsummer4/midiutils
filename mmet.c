#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <fcntl.h>
#include <stdlib.h>
#include "midimsg.h"
#include <stdio.h>
#include "util.h"

int note = 60;

int main (int argc, char **argv) {
	if (argc>1) note = atoi(argv[1]);
	struct mm_msg m;
	for (int ticks = 23;;) {
		if (!mm_read(0, &m)) return 0;
		mm_write(1, &m);
		if (m.type != MM_CLOCK) continue;
		ticks++;
		switch (ticks) {
			case 24: ticks = 0;
			case 0: m = (struct mm_msg){MM_NOTEON, 9, note, 128}; break;
			case 8: m = (struct mm_msg){MM_NOTEOFF, 9, note, 0}; break;
			default: continue; }
		mm_write(1, &m); }
	return 0; }
