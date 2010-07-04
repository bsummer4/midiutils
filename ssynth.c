/*
  '/dev/dsp' makes write() block until the device is ready.  So we just need
  to:

  1. Know the sample size and the sample rate (and any buffering crap),
     that the device expects.
  2. Use #1 to compute periods-per-write.
  3. Compute number for next sample, and write() it!
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <linux/soundcard.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <stropts.h>
#define NSYNTHS 9

typedef unsigned char byte;
typedef struct notes {
	double percent; // progress through waveform
	double inc; // Frequency: percent of waveform per sample;
} N;

int samplerate = 44100;
int samplesize = AFMT_U8;
double freq = 440; // hertz
int mono = 1;
int usednotes = 0;
N ns[NSYNTHS];

#define err(...) fprintf(stderr, __VA_ARGS__), exit(1)
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
		ns[usednotes-1] = ns[index];
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

void midirdy (int sig) {
	static char buf[81];
	static int bytes;
	bytes = read(0, buf, 80);
	if (!bytes) exit(0);
	if (-1 == bytes) return;
	buf[bytes] = '\0';
	sscanf(buf, "%lf", &freq);
	if (freq > 0) noteon(freq);
	else noteoff(-freq); }

void funkygo () {
	int i, j=0;
	for (i=samplerate;; i++) {
		send(nextsample());
		if (i > samplerate/(15*NSYNTHS)) {
			if (usednotes == NSYNTHS) { usednotes = 0; freq *= 1.01; j=0; }
			j++; noteon(freq * j); i = 0; }}}

void go () { for (;;) send(nextsample()); }

int main (int argc, char **argv) {
   ioctl(1, SNDCTL_DSP_CHANNELS, &mono);
	ioctl(1, SNDCTL_DSP_SETFMT, &samplesize);
	ioctl(1, SNDCTL_DSP_SPEED, &samplerate);
	fcntl(0, F_SETOWN, getpid());
	fcntl(0, F_SETSIG, 23);
	{	int flags = fcntl(0, F_GETFL);
		flags = (flags==-1)?0:(flags|O_NONBLOCK|O_ASYNC);
		fcntl(0, F_SETFL, flags); }
	signal(23, midirdy);
	go();
	return 0; }
