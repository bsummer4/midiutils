CC = gcc
LD = gcc
CFLAGS = -std=gnu99 -Os # -O0 -g -Wall -pedantic
LDFLAGS = -lrt -lm
PROGS = dispmidi brainstorm ssynth mjoin midigen mmet mticks fixdrums \
        sample-edit sampler
SRC = dispmidi.c midimsg.c brainstorm.c ssynth.c mjoin.c midigen.c mmet.c \
      mticks.c fixdrums.c sample-edit.c sampler.c
OBJ = ${SRC:%.c=%.o}

all:V: $PROGS
test:V:
	./test-synth.sh

clean:V:
	rm -f $PROGS $OBJ

%: %.o
	$LD $LDFLAGS $prereq -o $target

sample-edit: sample-edit.o
	$LD $LDFLAGS -lX11 $prereq -o $target

%.o: %.c
	$CC $CFLAGS -c $stem.c

$PROGS: midimsg.o
<| gcc -MM $SRC
