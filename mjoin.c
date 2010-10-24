/*
	# mjoin

	Joins multiple midi streams (stdin and any number of fifos)
	into one.

		mticks 120 | mjoin /dev/midi1 /dev/midi2 | ./ssynth >/dev/dsp
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <fcntl.h>
#include <err.h>
#include "midimsg.h"
#include "util.h"

void handleinput (int fd) {
	static struct mm_msg m;
	mm_read(fd, &m); // TODO hack: this could block!
	mm_write(1, &m); }

#define FORII(X) for (int ii=0; ii<X; ii++)
int main (int argc, char **argv) {
	argc--; argv++;
	int fds[argc+1];
	int maxfd = fds[argc] = 0;
	FORII (argc) {
		E("open", (fds[ii] = open(argv[ii], O_RDONLY)));
		maxfd = MAX(fds[ii], maxfd); }
	fd_set readfds, writefds;
	for (;;) {
		FD_ZERO (&readfds);
		FD_ZERO (&writefds);
		FORII (argc+1)
			if (fds[ii] != -1)
				FD_SET (fds[ii], &readfds);
		switch (select (maxfd+1, &readfds, &writefds, NULL, NULL)) {
		case -1: err(1, "select");
		case 0: continue;
		default:
			FORII (argc+1)
				if (FD_ISSET (fds[ii], &readfds))
					handleinput(fds[ii]); }}
	return 0; }
