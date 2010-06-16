CC = 9c
LD = 9l
CFLAGS = '-std=c99 -Os -Wall -pedantic'
LFLAGS = '-static'

PROGS = dispmidi brainstorm
SRC = dispmidi.c midimsg.c brainstorm.c

all:V: $PROGS
clean:V:
  rm -f $PROGS *.o

brainstorm:
	$LD $LFLAGS $prereq -o $target
dispmidi:
	$LD $LFLAGS $prereq -o $target

%.o: %.c
	$CC $CFLAGS -c $stem.c -o $target

brainstorm: brainstorm.o midimsg.o
dispmidi: dispmidi.o midimsg.o
<| gcc -MM $SRC
