CC = gcc
LD = c99
CFLAGS = --std=gnu99 -Os -I/usr/include/alsa
LDFLAGS = -lrt -lm
PROGS = dispmidi brainstorm ssynth mjoin midigen mmet mticks fixdrums \
        sample-edit sampler alsabridge
SRC = dispmidi.c midimsg.c brainstorm.c ssynth.c mjoin.c midigen.c mmet.c \
      mticks.c fixdrums.c sample-edit.c sampler.c alsabridge.c
OBJ = ${SRC:%.c=%.o}

all:V: $PROGS
test:V:
	./test-synth.sh

clean:V:
	rm -f $PROGS $OBJ

%: %.o
	$LD $LDFLAGS $prereq -o $target

alsabridge: alsabridge.o
	$LD $LDFLAGS -lasound $prereq -o $target

sample-edit: sample-edit.o
	$LD $LDFLAGS -lX11 $prereq -o $target

%.o: %.c
	$CC $CFLAGS -c $stem.c

$PROGS: midimsg.o
<| gcc -MM $SRC
