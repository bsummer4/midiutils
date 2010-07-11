/*
	# mjoin

	Joins multiple midi streams into one.

		mjoin /dev/midi1 /dev/midi2 | ./ssynth >/dev/dsp

	## Complications

	- stdout had better not block for any appreciable amount of time.  This
  	  is a reasonable assumption for most MIDI-receiving programs.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <fcntl.h>
#include "midimsg.h"
#include "util.h"


void handleinput (int fd) {
	static struct mm_msg m;
	mm_read(fd, &m); // TODO hack: this could block!
	mm_write(1, &m); }

int main (int argc, char **argv) {
	argc--; argv++;
	close(0); // we don't use stdin
	int fds[argc];
	int maxfd = 2;
	for (int ii=0; ii < argc; ii++) {
		E("open", (fds[ii] = open(argv[ii], O_RDONLY)));
		maxfd = MAX(fds[ii], maxfd); }
	fd_set readfds, writefds;
	for (;;) {
		FD_ZERO (&readfds);
		FD_ZERO (&writefds);
		for (int ii=0; ii < argc; ii++)
			if (fds[ii] != -1)
				FD_SET (fds[ii], &readfds);
		switch (select (maxfd+1, &readfds, &writefds, NULL, NULL)) {
		case -1: perr("select");
		case 0: continue;
		default:
			for (int ii=0; ii<argc; ii++)
				if (FD_ISSET (fds[ii], &readfds))
					handleinput(fds[ii]); }}
	return 0; }
