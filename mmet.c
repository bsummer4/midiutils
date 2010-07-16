#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <fcntl.h>
#include <stdlib.h>
#include "midimsg.h"
#include <stdio.h>
#include "util.h"

int beatinst = 60;
int offbeatinst = 72;
int beats = 1;

int main (int argc, char **argv) {
	switch (argc) {
	case 4: offbeatinst = atoi(argv[3]);
	case 3: beatinst = atoi(argv[2]);
	case 2: beats = atoi(argv[1]);
	case 1: break;
	default:
		err("Usage: %s [beats-per-measure] [beat-note] [offbeat-note]\n",
		    argv[0]); }
	struct mm_msg m;
	for (int ticks=23, onbeat=0;;) {
		if (!mm_read(0, &m)) return 0;
		mm_write(1, &m);
		if (m.type != MM_CLOCK) continue;
		ticks++;
		switch (ticks) {
			case 24: ticks = 0;
			case 0: m = (struct mm_msg){MM_NOTEON, 9, 0, 128}; break;
			case 8: m = (struct mm_msg){MM_NOTEOFF, 9, 0, 0}; break;
			default: continue; }
		m.arg1 = onbeat<1?beatinst:offbeatinst;
		mm_write(1, &m);
		if (m.type == MM_NOTEOFF) onbeat = (onbeat+1) % beats; }
	return 0; }
