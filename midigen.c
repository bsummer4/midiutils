// This is a quick hack; It's not safe or good code.

#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "midimsg.h"

typedef struct mm_msg M;

int main (void) {
	char buf[1024];
	struct mm_msg m;
	while (gets(buf)) {
		int num = atoi(buf);
		int off = num<0;
		int type = off?MM_NOTEOFF:MM_NOTEON;
		m = (M){type, 0, num>0?num:-num, off?0:128};
		mm_write(1, &m); }
	return 0; }
