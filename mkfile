CC = gcc
LD = gcc
CFLAGS = -std=gnu99 -Os # -O0 -g -Wall -pedantic
LDFLAGS = -lrt -lm
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
	$LD $LDFLAGS $prereq -o $target

%.o: %.c
	$CC $CFLAGS -c $stem.c

$PROGS: midimsg.o
<| gcc -MM $SRC
