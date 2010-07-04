#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <stdbool.h>
#include "midimsg.h"

#define LOOP while (true)

byte midimsg[3];
int main (int argc, char *argv[]) {
	while (mm_read (0, midimsg)) {
		switch (mm_msgtype (midimsg)) {
		case MM_NOTEOFF:
			printf("%d\tNote Off\t\t%d\t%d\n", 
			       mm_chan(midimsg), 
			       midimsg[1],
			       midimsg[2]);
			break;
		case MM_NOTEON:
			printf("%d\tNote On\t\t\t%d\t%d\n",
					mm_chan(midimsg), 
					midimsg[1],
					midimsg[2]);
			break;
		case MM_KEYPRESS:
			printf("%d\tAftertouch (key)\t\t%d\t%d\n", 
					mm_chan(midimsg), 
					midimsg[1],
					midimsg[2]);
			break;
		case MM_PARAM:
			printf("%d\tController\t\t%d\t%d\n",
					mm_chan(midimsg), 
					midimsg[1],
					midimsg[2]);
			break;
		case MM_PROG:
			printf("%d\tProgram\t\t%d\n",
					mm_chan(midimsg), 
					midimsg[1]);
			break;
		case MM_CHANPRESS:
			printf("%d\tAftertouch (channel)\t%d\n",
					mm_chan(midimsg), 
					midimsg[1]);
			break;
		case MM_PITCHWHEEL:
			printf("%d\tPitch Bend\t\t%x\n",
					mm_chan(midimsg),
					mm_pitchwheel(midimsg));
			break; }}
   return 0; }
