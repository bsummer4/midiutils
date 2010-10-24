/*
	# sampler
*/


#include <stdbool.h>
#include <unistd.h>
#include <linux/soundcard.h>
#include <stropts.h> // ioctl()
#include <math.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "midimsg.h"
#include "util.h"
#define SI static inline

#define POLYPHONY 64
#define SAMPLERATE 44100

/* TODO Mmap a file here.  */
byte hack[20] = {0, 64, 128, 192, 255, 0};
byte *samples = hack;
int nsamples = 20;

SI size_t filelen (int fd) { struct stat s; fstat(fd, &s); return
s.st_size; }
static void opensamples () {
   int fd = open("1", O_RDWR);
   if (fd < 0) return;
   int ns = filelen(fd);
   byte *s = mmap(NULL, nsamples, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
   if (!s) return;
   fputs("opened\n",stderr),samples=s,nsamples=ns; }


/*
	'percent' is the Current progress through waveform, and 'inc' is the how
	much to move 'percent' forword after each sample.  'inc' must be in
	[0,1].
*/
typedef struct synth {
	byte *samples;
	int nsamples, id;
	double percent, inc, vol;
} S;

int activesynths = 0;
struct synth synths[POLYPHONY];

// Generate a sample then move forward in time by one sample.
byte nextsample();
void noteoff (int id);
void noteon (double f, double vol, int id);

byte nextsample () {
	int remain=activesynths;
	double sample = 0.0;
	for (struct synth *s=synths; remain; s++, remain--) {
		s->percent += s->inc;
		if (s->percent > 1) s->percent -= 1;
		sample += s->samples[(int)floor(nsamples*s->percent)]; }
	if (sample >= 255.0) return 255;
	if (sample <= 0.0) return 0;
	return (byte) sample; }

void killsynth (int index) {
	if (activesynths-1 != index)
		synths[index] = synths[activesynths-1];
	activesynths--; }

double dmod (double x, double y) {
	if (x<0 || y<0) err("Internal error");
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
		synths[activesynths++] = (S){samples, nsamples, id, 0.0, f2inc(f), vol}; }

// ## Input/Output handling

void send (byte s) {
	switch (write(1, &s, 1)) {
		case 1: return;
		case 0: exit(0);
		case -1: perr("write"); }}

int makeid (int note, int chan) { return note<<4 | chan; }

// Accept MIDI bytes then setup/remove synths as requested.
void midirdy () {
	byte buf[40];
	int bytes = read(0, buf, 40);
	if (-1 == bytes) perr("read");
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
	opensamples();
	setup_output(SAMPLERATE, AFMT_U8, 1, 0x00030008);
	fd_set readfds, writefds;
	for (;;) {
		FD_ZERO (&readfds);
		FD_ZERO (&writefds);
		FD_SET (1, &writefds);
		FD_SET (0, &readfds);
		switch (select(2, &readfds, &writefds, NULL, NULL)) {
			case -1: perr("select");
			case 0: continue;
			default:
				if (FD_ISSET(0, &readfds)) midirdy();
				if (FD_ISSET(1, &writefds)) send(nextsample()); }}
	return 0; }
