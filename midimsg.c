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
#include "util.h"
#include <math.h>
#include <string.h>

#define local static inline
#define perr(x) perror(x), exit(1)

static int msgsize (byte type) {
	switch (type) {
	case MM_NOTEON:
	case MM_NOTEOFF:
	case MM_KEYPRESS:
	case MM_CNTL:
	case MM_PITCHWHEEL:
		return 3;
	case MM_PROG:
	case MM_CHANPRESS:
		return 2;
	default:
		// realtime message
		return 1; }}

double mm_notefreq (byte code) {
   return powl(2, ((double) code + 36.37361656)/12.0); }

local bool realtime (byte type) { return type>=0xF0; }

#define E(X,Y) if (-1 == Y) perr(X)
void mm_write (int fd, struct mm_msg *m) {
	static byte lasttype = MM_RESET;
	int size = msgsize(m->type);
	byte status = 0x80 | (m->chan&0x0F) | ((m->type&0x07)<<4);
	// fprintf(stderr, "type: 0x%02x -> 0x%02x\n", m->type, status);
	byte buf[3] = {status, m->arg1, m->arg2};
	if (realtime(m->type)) { E("write", write(fd, &m->type, 1)); return; }
	{	byte *out = buf;
		if (lasttype == status) { size--; out++; }
		for (;;) {
			int written = write(fd, out, size);
			// fprintf(stderr, "write %d bytes", written);
			// for (int i=0; i<written; i++)
				// fprintf(stderr, " 0x%02x", out[i]);
			// fprintf(stderr, "\n");
			if (!written) exit(0);
			if (written == size) break;
			if (written < 0) perr("write");
			size -= written;
			out += written; }}}

// 1 is incomplete, 2 is fail, 0 is complete.  Nothing will be written
// to 'out' unless we returned 0.
int mm_inject (byte b, struct mm_msg *out) {
	static byte lasttype = 0;
	static byte lastchan = 0;
	static struct mm_msg bytes;
	static int nbytes = 0;
	if (realtime(b)) { out->type = b; return 0; }
	switch (nbytes) {
		case 0:
			if (b & 0x80) { // New status
				lasttype = bytes.type = (b&0x70)>>4;
				lastchan = bytes.chan = b & 0x0F;
				nbytes = 1; }
			else { // Running status
				bytes.type = lasttype;
				bytes.chan = lastchan;
				bytes.arg1 = b;
				nbytes = 2; }
			return 1;
		case 1:
			bytes.arg1 = b= b;
			if (2 == msgsize(bytes.type)) break;
			nbytes++;
			return 1;
		case 2: bytes.arg2 = b; break; }
	nbytes = 0;
	if (bytes.type == MM_NOTEON && !bytes.arg2) bytes.type = MM_NOTEOFF;
	memcpy(out, &bytes, 4);
	return 0; }

local byte mm_readbyte (int fd, bool *eof) {
       byte result;
top:
       switch (read (fd, (void*) &result, 1)) {
       case 0: *eof=true; return 0;
       case 1: return result;
       case -1:
               if (errno == EINTR) goto top;
       default: perr("read"); }}


int mm_read (int fd, struct mm_msg *out) {
	bool eof = false;
	while (true) {
		byte b = mm_readbyte(fd, &eof);
		if (eof) return false;
		switch(mm_inject(b, out)) {
		case 0: return true;
		case 1: continue;
		case 2: return false; }}}
