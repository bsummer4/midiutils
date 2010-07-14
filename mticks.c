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

unsigned long bpm = 59;
const unsigned long bil = 1000 * 1000 * 1000;

struct timespec timediff (int bpm) {
	double tpm = bpm * 24.0;
	double tps = tpm / 60.0;
	double spb = 1.0 / tps;
	return (struct timespec){spb, (long)(spb*bil) % bil}; }

int main (int argc, char **argv) {
	close(0); // we don't use stdin
	if (argc>=2) bpm = atol(argv[1]);
	struct timespec in = timediff(bpm), out;
	struct mm_msg m = {MM_CLOCK, 0, 0, 0};
	do mm_write(1, &m); while (-1 != nanosleep(&in, &out));
	return 0; }
