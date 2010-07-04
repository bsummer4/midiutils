/*
	A simple synth; just uses a square-wave for every instrument.
   MIDI comes from stdin and audio goes to stdout.  The output must go to a
   oss audio device (like /dev/dsp).  Most MIDI messages are ignored; only
   noteon and noteoff are used.

  * TODO Percussion!
  * TODO Modulation, pitchbend, and vibrato.  Drop this if it's complicated
         we want to keep the implementation simple.
  * TODO Look for ways to simplify/shorten the code.
  * TODO Check for failed ioctl() and fcntl() calls.
*/

#include <stdbool.h>
#include <unistd.h>
#include <linux/soundcard.h>
#include <fcntl.h>
#include <stropts.h>
#include <math.h>
#include <sys/select.h>
#include "midimsg.h"
#include <stdio.h> // err()
#include <stdlib.h> // exit()
#define NSYNTHS 8

typedef struct notes {
	double percent; // progress through waveform
	double inc; // Frequency: percent of waveform per sample;
} N;

int samplerate = 44100;
int samplesize = AFMT_U8;
double freq = 440; // hertz
int mono = 1;
int usednotes = 0;
int fragment = 0x00040009; // TODO What does this mean? (this was lifted from
                           //      the oss softsynth example)
N ns[NSYNTHS];

#define err(...) fprintf(stderr, __VA_ARGS__), exit(1)
#define perr(x) perror(x), exit(1)
void send (byte s) {
 top:
	switch (write(1, &s, 1)) {
		case -1: goto top;
		case 1: return;
		case 0: exit(0);
		default: err("wtf"); }}

byte nextsample () {
	byte sample = 0;
	int i;
	for (i=0; i<usednotes; i++) {
		ns[i].percent += ns[i].inc;
		while (ns[i].percent > 1) ns[i].percent -= 1;
		if (ns[i].percent <= 0.5) sample += 255/NSYNTHS; }
	return sample; }

void freenote (int index) {
	if (usednotes-1 != index)
		ns[index] = ns[usednotes-1];
	usednotes--; }

int find (double f) {
	double inc = f/(double)samplerate;
	int i;
	for (i=0; i<usednotes; i++)
		if (ns[i].inc == inc)
			return i;
	return -1; }

void noteoff (double f) {
	if (f<=0) return;
	int index = find(f);
	if (index != -1) freenote(index); }

void noteon (double f) {
	if (f <= 0) return;
	if (usednotes>=NSYNTHS) return;
	if (-1 != find(f)) return;
	ns[usednotes++] = (N){0, f/(double)samplerate}; }

double midifreq (byte code) {
	return powl(2, ((double) code + 36.37361656)/12.0); }

void midirdy () {
	static byte buf[81];
	static int bytes;
	static int i, type, freq;
	bytes = read(0, buf, 80);
	if (-1 == bytes) return;
	if (!bytes) exit(0);
	for (i=0; i<bytes; i++) {
		static byte msg[3];
		switch (mm_inject(buf[i], msg)) {
			case 2: exit(1);
			case 1: continue;
			case 0: {
				type = mm_msgtype(msg);
				freq = midifreq(msg[1]);
				if (type == MM_NOTEON) noteon(freq);
				if (type == MM_NOTEOFF) noteoff(freq); }}}}

void funky () {
	int i, j=0;
	for (i=samplerate;; i++) {
		send(nextsample());
		if (i > samplerate/(15*NSYNTHS)) {
			if (usednotes == NSYNTHS) { usednotes = 0; freq *= 1.01; j=0; }
			j++; noteon(freq * j); i = 0; }}}

void go () { midirdy(); send(nextsample()); }

void noblock (int fd) {
	int flags = fcntl(fd, F_GETFL);
	flags = (-1==flags?0:flags) | O_NONBLOCK;
	fcntl(fd, F_SETFL, flags); }

int main (int argc, char **argv) {
	ioctl(1, SNDCTL_DSP_CHANNELS, &mono);
	ioctl(1, SNDCTL_DSP_SETFMT, &samplesize);
	ioctl(1, SNDCTL_DSP_SPEED, &samplerate);
	ioctl(1, SNDCTL_DSP_SETFRAGMENT, &fragment);

	fd_set readfds, writefds;
	for (;;) {
		FD_ZERO (&readfds);
		FD_ZERO (&writefds);
		FD_SET (1, &writefds);
		FD_SET (0, &readfds);
		switch (select (2, &readfds, &writefds, NULL, NULL)) {
		case -1: perr("select");
		case 0: continue;
		default:
			if (FD_ISSET (0, &readfds)) midirdy();
			if (FD_ISSET (1, &writefds)) send(nextsample()); }}
	return 0; }
