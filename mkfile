CC = 9c
LD = 9l
CFLAGS = '--std=c99 -Os -Wall -pedantic'
LFLAGS = '-static'
PROGS = dispmidi
SRC = dispmidi.c midimsg.c

all:V: $PROGS
clean:V:
  rm -f $PROGS *.o

<| gcc -MM $SRC

%.o: %.c
	$CC $CFLAGS -c $stem.c -o $target

dispmidi: dispmidi.o midimsg.o
	$LD $LFLAGS $prereq -o $target
