/*
	# Ssynth
	A simple synth; just uses a square-wave for every instrument.
	Requires only a POSIX system with OSS support.

	MIDI comes from stdin and audio goes to stdout.  The output must go to a
	oss audio device (like /dev/dsp).  Most MIDI messages are ignored; only
	noteon and noteoff are used.

	The goal is decent support for playback of midi files and
	interaction with midi controlers with very simple code.

	## TODO
	- TODO Look for ways to simplify/shorten the code.
	- TODO Assign different waves to diffent MIDI notes
	- TODO White Noise in addition to simple waves.
	- TODO Very high and low notes sound bad.  This may not be avoidable.
	- TODO Modulation, pitchbend, and vibrato.
		Drop this if it's complicated; we want to keep the
		implementation as simple as possible.
*/


#include <stdbool.h>
#include <unistd.h>
#include <linux/soundcard.h>
#include <stropts.h> // ioctl()
#include <math.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include "midimsg.h"
#include "util.h"

// ## Compile-time options

#define POLYPHONY 64
#define SAMPLERATE 44100
#define SYNTH REVSAWTOOTH

// ## Synth stuff

// Two synths are considered to be the same if they have the same values
// for 'id'.  There are never two equal synths in the system at the
// same time.  Since our biggest sample is 255, 'vol' values should be
// significantly smaller than that.  If a sample greater than 255 is
// generated, then we must cap it; this will cause bad-sounding
// clipping.
typedef struct synth {
	double percent; // Current progress through waveform.
	double inc;     // Percent of waveform per sample;  Must be < 1.
	double vol;
	int id;
} S;

int activesynths = 0;
struct synth synths[POLYPHONY];

// Generate a sample then move forward in time by one sample.
byte nextsample();
void noteoff (int id);
void noteon (double f, double vol, int id);

#define SQUAREWAVE  if (s->percent <= 0.5) sample += s->vol;
#define SAWTOOTH    sample += s->vol * s->percent;
#define REVSAWTOOTH sample += s->vol * (1 - s->percent);
byte nextsample () {
	int remain=activesynths;
	double sample = 0.0;
	for (struct synth *s=synths; remain; s++, remain--) {
		s->percent += s->inc;
		if (s->percent > 1) s->percent -= 1;
		SYNTH; }
	if (sample >= 255.0) return 255;
	if (sample <= 0.0) return 0;
	return (byte) sample; }

void killsynth (int index) {
	if (activesynths-1 != index)
		synths[index] = synths[activesynths-1];
	activesynths--; }

double dmod (double x, double y) {
	if (x<0 || y<0) errx(1,"Internal error");
	return x;
	double result = x;
	while (x > y) x -= y;
	return result; }

double f2inc (double f) { return dmod(f/(double)SAMPLERATE, 1.0); }

// Returns the index of a synth in ns[] or -1
int find (int id) {
	for (int ii=0; ii<activesynths; ii++)
		if (synths[ii].id == id) return ii;
	return -1; }

void noteoff (int id) {
	int index = find(id);
	if (index != -1) killsynth(index); }

void noteon (double f, double vol, int id) {
	if (f <= 0) exit(1);
	if (find(id) == -1 && activesynths < POLYPHONY)
		synths[activesynths++] = (S){0.0, f2inc(f), vol, id}; }

// ## Input/Output handling

void send (byte s) {
	switch (write(1, &s, 1)) {
		case 1: return;
		case 0: exit(0);
		case -1: err(1, "write"); }}

int makeid (int note, int chan) { return note<<4 | chan; }

// Accept MIDI bytes then setup/remove synths as requested.
void midirdy () {
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

void setup_output (int samplerate, int samplesize, int chans, int frag) {
	E("ioctl", ioctl(1, SNDCTL_DSP_CHANNELS, &chans));
	E("ioctl", ioctl(1, SNDCTL_DSP_SETFMT, &samplesize));
	E("ioctl", ioctl(1, SNDCTL_DSP_SPEED, &samplerate));
	E("ioctl", ioctl(1, SNDCTL_DSP_SETFRAGMENT, &frag)); }

int main (int argc, char **argv) {
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
