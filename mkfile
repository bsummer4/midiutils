MKSHELL = $PLAN9/bin/rc
CC = 9c
LD = 9l
CFLAGS = -std=gnu99 -Os -Wall -pedantic
LFLAGS = -static
PROGS = dispmidi brainstorm ssynth mjoin midigen mmet mticks
SRC = dispmidi.c midimsg.c brainstorm.c ssynth.c mjoin.c midigen.c mmet.c \
      mticks.c
OBJ = ${SRC:%.c=%.o}

all:V: $PROGS
test:V:
	./test-synth.sh

clean:V:
	rm -f $PROGS $OBJ

%: %.o
	$LD $LFLAGS $prereq -o $target

%.o: %.c
	$CC $CFLAGS -c $stem.c -o $target

$PROGS: midimsg.o
<| gcc -MM $SRC
