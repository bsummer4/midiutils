CFLAGS = -O1 -I/usr/include/alsa '-D_POSIX_C_SOURCE=199309L'
LDFLAGS = -lasound -lX11 -lm -lrt midimsg.o
SRC = `{echo *.c}
OBJ = ${SRC:%.c=%.o}
PROGS = dispmidi brainstorm ssynth mjoin midigen mmet mticks fixdrums \
        sample-edit sampler alsabridge

all:V: $PROGS
test:V:
	./test-synth.sh

clean:V:
	rm -f $PROGS $OBJ

%: %.o
	c99 $LDFLAGS $stem.o -o $target

%.o: %.c
	c99 $CFLAGS -c $stem.c

$PROGS: midimsg.o
<| gcc -MM $SRC
