#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <stdbool.h>
#include "midimsg.h"

#define LOOP while (true)

struct mm_msg m;
int main (void) {
	while (mm_read (0, &m)) {
		switch (m.type) {
		case MM_NOTEOFF:
			printf("%d\tNote Off\t\t%d\t%d\n", m.chan, m.arg1, m.arg2);
			break;
		case MM_NOTEON:
			printf("%d\tNote On\t\t\t%d\t%d\n", m.chan, m.arg1, m.arg2);
			break;
		case MM_KEYPRESS:
			printf("%d\tAftertouch (key)\t\t%d\t%d\n", m.chan, m.arg1, m.arg2);
			break;
		case MM_CNTL:
			printf("%d\tController\t\t%d\t%d\n", m.chan, m.arg1, m.arg2);
			break;
		case MM_PROG:
			printf("%d\tProgram\t\t%d\n", m.chan, m.arg1);
			break;
		case MM_CHANPRESS:
			printf("%d\tAftertouch (channel)\t%d\n", m.chan, m.arg1);
			break;
		case MM_PITCHWHEEL:
			printf("%d\tPitch Bend\t\t%04x\n", m.chan, mm_bendvalue(m.arg2, m.arg1));
			break;
		default:
			printf("-\tRealtime msg\t0x%x", m.type); }}
   return 0; }
