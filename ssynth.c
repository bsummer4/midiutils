/*
	A simple synth; just uses a square-wave for every instrument.
   MIDI comes from stdin and audio goes to stdout.  The output must go to a
   oss audio device (like /dev/dsp).  Most MIDI messages are ignored; only
   noteon and noteoff are used.

  * TODO Percussion!
  * TODO Modulation, pitchbend, and vibrato.  Drop this if it's
         complicated; we want to keep the implementation as simple as
         possible.
  * TODO Look for ways to simplify/shorten the code.
  * TODO Check for failed ioctl() calls.
*/

#include <stdbool.h>
#include <unistd.h>
#include <linux/soundcard.h>
#include <stropts.h>
#include <math.h>
#include <sys/select.h>
#include "midimsg.h"
#include <stdio.h> // err()
#include <stdlib.h> // exit()
#define NSYNTHS 8

typedef struct synth {
	double percent; // - progress through waveform
	double inc;     // - percent of waveform per sample;  Should be < 1
	double vol;	    // - Value of the waveform during the high part.  The
                   //   biggest value we can send is 255.
} S;

// for ioctl()
int samplerate = 44100;
int samplesize = AFMT_U8;
int mono = 1;
int fragment = 0x00040009; // TODO What does this mean? (this was lifted from
                           //      the oss softsynth example)

// synth state
int usednotes = 0;
struct synth ns[NSYNTHS];
double freq = 440; // hertz

#define err(...) fprintf(stderr, __VA_ARGS__), exit(1)
#define perr(x) perror(x), exit(1)
#define MAX(x, y) (x>y)?x:y

void send (byte s) {
 top:
	switch (write(1, &s, 1)) {
		case -1: goto top;
		case 1: return;
		case 0: exit(0);
		default: err("wtf"); }}

byte nextsample () {
	int remain=usednotes;
	double sample = 0.0;
	for (struct synth *s=ns; remain; s++, remain--) {
		s->percent += s->inc;
		while (s->percent > 1) s->percent -= 1;
		if (s->percent <= 0.5) sample += s->vol; }
	if (sample >= 255.0) return 255;
	if (sample <= 0.0) return 0;
	return (byte) sample; }

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

void noteon (double f, double vol) {
	fprintf(stderr, "noteon %lf %lf\n", f, vol);
	if (f <= 0) exit(1);
	double inc = f/(double)samplerate;
	while (inc > 1.0) inc -= 1.0;
	int index = find(f);
	if (index == -1) {
		if (usednotes < NSYNTHS)
			ns[usednotes++] = (S){0.0, inc, vol};
		return; }
	else
		ns[index].vol = MAX(vol, ns[index].vol); }

// TODO Derived by hand; might not be perfect.
// TODO Move this into a midi library?
double midifreq (byte code) {
	return powl(2, ((double) code + 36.37361656)/12.0); }

void midirdy () {
	byte buf[81];
	int bytes = read(0, buf, 80);
	if (-1 == bytes) return;
	if (!bytes) exit(0);
	for (int i=0; i<bytes; i++) {
		byte msg[3];
		switch (mm_inject(buf[i], msg)) {
			case 2: exit(1);
			case 1: continue;
			case 0: {
				int type = mm_msgtype(msg);
				double freq = midifreq(msg[1]);
				// There's nothing special about x/2, it just gives reasonable
				// results.
				if (type == MM_NOTEON) noteon(freq, msg[2]/2);
				if (type == MM_NOTEOFF) noteoff(freq); }}}}

/*
// This is not used, but it sounds cool.
void funky () {
	int i, j=0;
	for (i=samplerate;; i++) {
		send(nextsample());
		if (i > samplerate/(15*NSYNTHS)) {
			if (usednotes == NSYNTHS) { usednotes = 0; freq *= 1.01; j=0; }
			j++; noteon(freq * j); i = 0; }}}
*/

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
