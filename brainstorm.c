/*
	# Brainstorm
	A dictatation machine for MIDI
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <err.h>
#include "midimsg.h"
#include "util.h"

typedef struct midievent {
	struct timeval time;
	struct mm_msg m;
} ME;

static ME *events = NULL;
int activevents = 0;

void growevents (int num) {
	static int eventslots = 0;
	if (num <= eventslots) return;
	eventslots = num;
	events = realloc(events, eventslots * sizeof(ME));
	if (!events) errx(1,"Couldn't allocate memory"); }

const byte trailer[] = { 0, 0xff, 0x2f, 0 };
const char header[] = {
	'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 0, 0, 1, 0, 120,
	'M', 'T', 'r', 'k', 0, 0, 0, 0 };

static char *prefix;
static int timeout;

void makename (char *out) {
	time_t current_time;
	struct tm *current_time_struct;
	char current_time_string[1024];
	current_time = time(NULL);
	current_time_struct = localtime(&current_time);
	strftime(current_time_string, 1024, "%Y.%m.%d.%H.%M.%S", current_time_struct);
	snprintf(out, 1024, "%s.%s.mid", prefix, current_time_string); }

void write_variable_length_quantity(long value) {
	byte buffer[4];
	int offset = 3;
	for (;;) {
		buffer[offset] = value & 0x7f;
		if (offset < 3) buffer[offset] |= 0x80;
		value >>= 7;
		if ((value == 0) || (offset == 0)) break;
		offset--; }
	E("write", write(1, buffer + offset, 4 - offset)); }

void write_four_byte_int(long value, int fd) {
	byte buffer[4];
	buffer[0] = (byte)(value >> 24);
	buffer[1] = (byte)((value >> 16) & 0xff);
	buffer[2] = (byte)((value >> 8) & 0xff);
	buffer[3] = (byte)(value & 0xff);
	E("write", write(fd, buffer, 4)); }

long dtime (struct timeval a, struct timeval b) {
	long secdiff = b.tv_sec - a.tv_sec;
	long usecdiff = b.tv_usec - a.tv_usec;
	return ((secdiff*1000000 + usecdiff) / 1000) * 240 / 1000; }

void writefile () {
	if (!activevents) errx(1,"Internal error");
	E("write", write(1, header, sizeof(header)));
	struct timeval lastime = events[0].time;
	int remain = activevents;
	for (ME* cur = events; remain; remain--, cur++) {
		write_variable_length_quantity(dtime(lastime, cur->time));
		lastime = cur->time;
		mm_write(1, &cur->m); }
	E("write", write(1, trailer, sizeof(trailer)));
	off_t ending_offset;
 	E("lseek", (ending_offset = lseek(1, 0, SEEK_CUR)));
	E("lseek", lseek(1, 18, SEEK_SET));
	write_four_byte_int(ending_offset - sizeof(header), 1); }

void dump2file (int signum) {
	static char filename[1024];
	if (!activevents) goto end;
	makename(filename);
	E("creat", creat(filename, 0666));
	// TODO I think creat() will always return 1 here.  Be sure.
	writefile();
	close(1);
	activevents = 0; // Removes all events.
 end:
	alarm(timeout);
	signal(SIGALRM, dump2file); }

void eventloop (void) {
	ME e;
	if (!mm_read(0, &e.m)) exit(0);
	gettimeofday(&e.time, NULL);
	growevents(activevents+1);
	events[activevents] = e;
	activevents++;
	alarm(timeout); }

int main (int argc, char **argv) {
	close(1); // We don't use stdout
	timeout = 2;
	prefix = "brainstorm-output";
	switch (argc) {
		case 3:
			timeout = atoi(argv[2]);
			timeout = timeout > 0 ? timeout : 2;
		case 2: prefix = argv[1]; 
		case 1: break;
		default:
			fprintf(stderr, "usage: %s [prefix] [timeout]\n", argv[0]);
			return 1; }
	signal (SIGALRM, dump2file);
	alarm (timeout);
	for (;;) eventloop();
	return 0; }
