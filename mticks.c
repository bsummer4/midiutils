#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "midimsg.h"
#include <stdio.h>
#include "util.h"
#include <stdbool.h>
#include <limits.h>

#define CLK CLOCK_REALTIME

unsigned long bpm = 120;
const unsigned long bil = 1000 * 1000 * 1000;

struct timespec timediff (int bpm) {
	double tpm = bpm * 24.0;
	double tps = tpm / 60.0;
	double spb = 1.0 / tps;
	return (struct timespec){spb, (long)(spb*bil) % bil}; }

void addto (struct timespec *x, struct timespec *y) {
	x->tv_sec += y->tv_sec;
	x->tv_nsec += y->tv_nsec;
	while (x->tv_nsec > 999999999) x->tv_sec++, x->tv_nsec-=999999999; }

int main (int argc, char **argv) {
	struct timespec next;
	E("clock_gettime", clock_gettime(CLK, &next));
	close(0); // we don't use stdin
	if (argc>=2) bpm = atol(argv[1]);
	struct timespec diff=timediff(bpm);
	struct mm_msg m = {MM_CLOCK, 0, 0, 0};
	for (;;) {
		mm_write(1, &m);
		addto(&next, &diff);
		if (clock_nanosleep(CLK, TIMER_ABSTIME, &next, NULL))
			perr("clock_nanosleep"); }
	return 0; }
