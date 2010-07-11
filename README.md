# Fork

This is my (Benjamin Summers) fork of Div's MIDI Utillities.  I want to
make more programs like this, and I want them to be easier to chain
together in the shell.  My goals for the existing programs are:

- Remove Windows support; Nobody uses windows anyways.  :)
- Use stdin/stdout instead of fifos for midi data.  I want to create
  pipelines with these programs.  Use fifos or socketfiles for non-midi
  input.
- Make the code more readable (to me).
- Understand all the code.
- Add many more utilities including:
  - A program that takes an smf file and produces a realtime stream on
    stdout.

        midiplay foo.mid > /dev/midi

  - A program that combines multiple inputs syncronusly.

        mjoin <(tee in.smf </dev/midi1) <(midiplay b.mid) >/dev/midi1

  - A command language for changing smf (e.g. ed).
  - A streaming version of it (e.g. sed).
  - A looping player that picks up smf file changes as they happen.
  - A visual editor on top of the command language (e.g. vi).
  - pipe-able wrappers around jack/fluidsynth/etc.

## Building

We use the plan9 build tools.  The best way to build is to just install
plan9port and run 'mk'.  If your distro doesn't have plan9port, you can
build it from source.  If you *really* don't want to install plan9port, you
can build with these commands:

	gcc -std=gnu99 -lm midimsg.c dispmidi.c   -o dispmidi
	gcc -std=gnu99 -lm midimsg.c ssynth.c     -o ssynth
	gcc -std=gnu99 -lm midimsg.c brainstorm.c -o brainstorm
	gcc -std=gnu99 -lm midimsg.c mjoin.c      -o mjoin
	gcc -std=gnu99 -lm midimsg.c midigen.c    -o midigen

You can also test (under linux) the synth with:

	test-synth.sh
