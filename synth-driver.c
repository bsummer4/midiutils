/*
	# sampler
*/

#include <unistd.h>
#include <linux/soundcard.h>
#include <stropts.h> // ioctl()
#include <sys/select.h>
#include <stdlib.h> // exit()
#include <err.h>
#include "midimsg.h"
#include "util.h"

SI void send (byte s) {
	switch (write(1, &s, 1)) {
		case 1: return;
		case 0: exit(0);
		case -1: err(1, "write"); }}

SI unsigned makeid (int note, int chan) { return note<<4 | chan; }

// Accept MIDI bytes then setup/remove synths as requested.
SI void midirdy () {
	byte buf[40];
	int bytes = read(0, buf, 40);
	if (-1 == bytes) err(1, "read");
	if (!bytes) exit(0);
	for (int i=0; i<bytes; i++) {
		struct mm_msg m;
		switch (mm_inject(buf[i], &m)) {
			case 2: exit(1);
			case 1: continue;
			case 0: {
				double freq = mm_notefreq(m.arg1);
				int id = makeid(m.arg1, m.chan);
				// x/3 isn't special, it just gives decent results.
				if (m.type == MM_NOTEON) noteon(freq, m.arg2/3, id);
				if (m.type == MM_NOTEOFF) noteoff(id); }}}}

SI void setup_output (int samplerate, int samplesize, int chans, int frag) {
	E("ioctl", ioctl(1, SNDCTL_DSP_CHANNELS, &chans));
	E("ioctl", ioctl(1, SNDCTL_DSP_SETFMT, &samplesize));
	E("ioctl", ioctl(1, SNDCTL_DSP_SPEED, &samplerate));
	E("ioctl", ioctl(1, SNDCTL_DSP_SETFRAGMENT, &frag)); }

int main (int argc, char **argv) {
	initsynth();
	setup_output(SAMPLERATE, AFMT_U8, 1, 0x00030008);
	fd_set readfds, writefds;
	for (;;) {
		FD_ZERO (&readfds);
		FD_ZERO (&writefds);
		FD_SET (1, &writefds);
		FD_SET (0, &readfds);
		switch (select(2, &readfds, &writefds, NULL, NULL)) {
			case -1: err(1, "select");
			case 0: continue;
			default:
				if (FD_ISSET(0, &readfds)) midirdy();
				if (FD_ISSET(1, &writefds)) send(nextsample()); }}
	return 0; }
