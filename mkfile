MKSHELL=$PLAN9/bin/rc
CC = 9c
LD = 9l
CFLAGS = -std=c99 -Os -Wall -pedantic
LFLAGS = -static
PROGS = dispmidi brainstorm ssynth
SRC = dispmidi.c midimsg.c brainstorm.c ssynth.c
OBJ = ${SRC:%.c=%.o}

all:V: $PROGS
clean:V:
	rm -f $PROGS $OBJ

%: %.o
	$LD $LFLAGS $prereq -o $target

%.o: %.c
	$CC $CFLAGS -c $stem.c -o $target

brainstorm dispmidi ssynth: midimsg.o
<| gcc -MM $SRC
