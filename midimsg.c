#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <errno.h>
#include "midimsg.h"
#include <string.h>

#define STATUS_MASK 0x80
#define MSGTYPE_MASK 0xf0
#define CHAN_MASK 0x0f
#define PITCH 1

static inline void die (char *msg) {
	fputs (msg, stderr);
	exit (EXIT_FAILURE); }

int mm_msgtype (byte *msg) { return (msg[0] & MSGTYPE_MASK); }
void mm_setmsgtype (byte *msg, int type) {
	msg[0] = (msg[0] & ~MSGTYPE_MASK) | type; }

int mm_chan (byte *msg) { return (msg[0] & CHAN_MASK); }
void mm_setchan (byte *msg, int chan) {
	msg[0] = (msg[0] & ~CHAN_MASK) | chan; }

int mm_pitchwheel (byte *msg) {
	return ((int)(msg[2]) << 7) + msg[1]; }

// 'eof' should point to a non false value before the call.
static inline byte mm_readbyte (int fd, bool *eof) {
	byte result;
top:
	switch (read (fd, (void*) &result, 1)) {
	case 0: *eof=true; return 0;
	case 1: return result;
	case -1:
		if (errno == EINTR) goto top;
	default:
		perror("read");
		exit(EXIT_FAILURE); }}

static inline bool realtime_msg (byte msg) { return (msg & 0xf0) == 0xf0; }
static inline int msgsize (byte *msg) {
	switch (mm_msgtype(msg)) {
	case MM_NOTEON:
	case MM_NOTEOFF:
	case MM_KEYPRESS:
	case MM_PARAM:
	case MM_PITCHWHEEL:
		return 3;
	case MM_PROG:
	case MM_CHANPRESS:
		return 2;
	default:
		return 2;
		die ("Invalid Message"); }}

void mm_write(int fd, byte *msg) { write (fd, msg, msgsize(msg)); }

// 1 is incomplete, 2 is fail, 0 is complete.  Nothing will be written
// to 'out' unless we returned 0.  'out' is expected to be at least 3 bytes
// big.
int mm_inject (byte b, byte *out) {
	static byte laststatus = 0;
	static byte bytes[3];
	static int nbytes = 0;
	if (realtime_msg(b)) goto incomplete;
	switch (nbytes) {
	case 0:
		if (b & STATUS_MASK) { // is it a status byte?
			bytes[0] = b;
			laststatus = b;
			nbytes = 1;
		} else {
			// This message must be a running status, unless
			// we're picking up mid-msg (which would be an
			// unhandled error)
			bytes[1] = b;
			bytes[0] = laststatus;
			nbytes = 2; }
		goto incomplete;
	case 1:
		bytes[1] = b;
		if (2 == msgsize(bytes)) goto complete;
		nbytes++;
		goto incomplete;
	case 2:
		bytes[2] = b;
		goto complete; }
 incomplete: return 1;
 complete:
	nbytes = 0;
	memcpy(out, bytes, 3); // 3rd byte might be nonsense, but it doesn't matter.
	return 0; }

bool mm_read (int fd, byte *out) {
	static byte laststatus = 0;
	byte firstbyte;
	bool eof = false;
	while (realtime_msg (firstbyte = mm_readbyte(fd, &eof)) && !eof);
	if (eof) return 0;
	if (firstbyte & STATUS_MASK) {
		out[0] = firstbyte;
		out[1] = mm_readbyte(fd, &eof);
		if (eof) return false;
		laststatus = firstbyte; }
	else {
		// Must be a running status, unless we're picking up mid-msg 
		// (which would be an unhandled error)
		out[0] = laststatus;
		out[1] = firstbyte; }
	if (3 == msgsize(out)) {
		out[2] = mm_readbyte(fd, &eof);
		if (eof) return false;
		if ((mm_msgtype(out) == MM_NOTEON) && !out[VELOCITY])
			mm_setmsgtype(out, MM_NOTEOFF); }
	return true; }
