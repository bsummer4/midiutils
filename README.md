# Fork

This is my (Benjamin Summers) fork of Div's MIDI Utillities.  I want to
make more programs like this, and I want them to be easier to chain
together in the shell.  My goals for the existing programs are:

- Remove Windows support; Nobody uses that shit anyways.
- Use stdin/stdout instead of fifos for midi data.  I want to create
  pipelines with these programs.  Use fifos or socketfiles for non-midi
  input.
- Make the code more readable (to me).
- Understand all the code.
- Add many more utilities including:
  - A program that takes an smf file and produces a realtime stream on
    stdout.  [midiplay foo.mid > /dev/midi]
  - A program that combines multiple inputs syncronusly.
    [mcomb <(rtcat </dev/midi | mtee in.smf) <(midiplay b.mid) >/dev/midi]
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

# Div's MIDI Utilities
## Introduction
Here is a collection of MIDI utilities I wrote for myself, which you may find
useful as well. I previously kept separate collections of MIDI utilities for
Windows and for Unix, but due to an increasing amount of overlap, I have
combined them into a single package.

Regardless of platform, the utilities are all written according to the Unix
design philosophy; most run from the command line instead of providing a GUI,
and each is small and dedicated to a specific task. Some work in realtime,
while others act upon saved MIDI files.

The Windows utilities should work on any recent version of Windows. They come
with ready-to-run binaries in addition to a Microsoft-compatible build harness.

Most of the Unix utilities work on raw MIDI streams, so they should be
compatible with just about everything, including ALSA, OpenSound, and MacOS/X.
They communicate via the standard Unix file interface, so they can read and
write directly to a device, to anonymous pipes, or to named FIFOs. The standard
tee(1) and mkfifo(1) utilities will come in very useful when working with them.

Only a few of the Unix utilities have dependencies beyond the standard C
library, and they are built conditionally. One is ALSA-specific, and thus
requires the ALSA library and headers to be installed. Another interacts with X
Windows (although it has no visible GUI), so it requires Xlib to be installed.

## License
These utilities are free and open source, provided under terms of the BSD
license. Specifically:

(C) Copyright 1998-2006 David G. Slomin, all rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

  • Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
  • Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
  • Neither the name of David G. Slomin, Div, Sreal, nor the names of any other
    contributors to this software may be used to endorse or promote products
    derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

## News
### 2006-06-28
Combined the Unix and Windows packages into one (the Unix utilities
are now under the same licence as the Windows ones). Updated the Unix
utilities to handle active sensing properly. Added a fully modern,
ALSA-specific version of brainstorm. Also added a preview version of imp,
a fancy successor to qwertymidi which understands multiple USB keyboards
plugged in at the same time.

### 2005-03-24
Added a significant amount of functionality to the midifile library,
making it suitable for building a much broader range of software.  Added
tempo-map, joycc, and the mish compiler. Added new maps for qwertymidi.
Also changed the package's license from LGPL to BSD, since it is even
more generous to users.

### 2004-10-09
Added a simple sampler (beatbox) and MIDI file player (mciplaysmf). The
sampler introduces new dependencies on the free sndfile and portaudio
libraries, but the necessary parts are included in the kit. The player
is here by popular user request, although I cheated and used the MCI
sequencer which is part of Windows rather than implementing my own
from scratch. Also rewrote brainstorm to use the midifile library for
robustness, and gave it some new features as well.

### 2003-06-27
By user request, the midifile library now provides control over
the MIDI file format (0, 1, or 2).

### 2003-04-27
Added pulsar, padpedal, and sendmidi. No more external dependencies to
build; the necessary parts of expat are now included in the kit.

### 2003-03-11
Added fakesustain and multiecho.

### 2003-02-22
Introducing metercaster, a new way to adjust the timing of MIDI sequences
without the dehumanizing side effects of quantization. When I went to
implement it, I was surprised to find that there was no readily available,
free C library for reading and writing standard MIDI files, so I'm also
introducing the midifile library, which fills the niche.

### 2003-01-23
Brainstorm now outputs more conventional MIDI files (format 1 with
PPQ timing, rather than format 0 with SMPTE timing), so they will be
compatible with more software.

### 2003-01-20
Fixed a critical bug in brainstorm. Also made significant improvements
to the build system and packaging.

## The Utilities
### beatbox (Windows)
This command line utility is a very simple, percussion-oriented
sampler. It plays prerecorded audio files (WAV, AIFF, or AU) in response
to MIDI note events. I do plan to make a pattern sequencer which,
combined with this, will be equivalent to a drum machine, but for now
you'll have to generate the events on your own.

  Usage: beatbox [ --in <n> ] [ --voices <n> ] ( [ --config <filename> ] ) ...

### brainstorm (Windows, Unix)
This command line utility functions as a dictation machine for MIDI. It listens
for incoming MIDI events and saves them to a new MIDI file every time you pause
in your playing for a few seconds. The filenames are generated automatically
based on the current time, so it requires no interaction. I find it useful for
recording brainstorming sessions, hence the name.

    Usage (Windows version): brainstorm [ --in <device id> ] [ --timeout <seconds> ] [ --extra-time <seconds> ] [ --prefix <filename prefix> ] [ --verbose ]
    Usage (original Unix version): brainstorm <input fifo> <filename prefix> <timeout in seconds>
    Usage (ALSA-specific version): abrainstorm [ --prefix <filename prefix> ] [ --timeout <seconds> ]

Note that the ALSA version does not connect up to an existing MIDI output port;
rather it creates a new MIDI input port which you can then connect up as you
see fit. aconnect, aconnectgui, and qjackctl are common utilities which can be
used to do this.

### delta (Windows)
This is a fun to play, monophonic variation on qwertymidi which maps keys on
the computer keyboard to relative intervals instead of absolute pitches. It was
inspired by an unusual MIDI controller I read about online. It also supports
custom keyboard mappings.

  Usage: delta [ --out <n> ] [ --channel <n> ] [ --program <n> ] [ --velocity <n> ] [ --map <str> ]

### dispmidi (Unix)
Like midimon for Windows, this utility pretty-prints incoming MIDI messages.
Why two different names? The answer is lost to history.

  Usage: dispmidi <input fifo>

fakesustain (Windows)

This command line utility pre-applies sustain to note events. It is useful for
software-based synthesizers which have incomplete MIDI implementations that
don't respond to real sustain messages. (VST is a good idea, but can be
frustrating in practice.)

  Usage: fakesustain [ --in <n> ] [ --out <n> ]

### imp (Windows)
This true GUI program requires Windows XP or newer because it uses Microsoft's
new "rawinput" API. This allows it to distinguish amongst multiple USB
keyboards plugged into the same computer at the same time. You can use it to
achieve the same effect as qwertymidi, but with larger and more complex
mappings. Beware that this is currently a preview release, and its interface
and file format will most likely change.

  Usage: imp [ <filename> ]

### intervals (Unix)
This filter-style utility plays parallel intervals for incoming MIDI messages.

  Usage: intervals <input_fifo> <output_fifo> <interval 1> <interval 2> ...

### joycc (Windows)
This command line utility is functionally similar to joypedal, but is designed
to work with real joysticks. Up to three directional axes and four buttons on
each of two joysticks can be mapped to separate MIDI controllers.

  Usage: joycc [ --out <n> ] [ --channel <n> ] [ --x1 <n> ] [ --y1 <n> ] [ --z1
<n> ] [ --a1 <n> ] [ --b1 <n> ] [ --c1 <n> ] [ --d1 <n> ] [ --x2 <n> ] [ --y2
<n> ] [ --z2 <n> ] [ --a2 <n> ] [ --b2 <n> ] [ --c2 <n> ] [ --d2 <n> ]

### joypedal (Windows)
This command line/text mode interactive utility requires custom hardware. It
looks for an organ style volume/expression pedal attached to the computer's
joystick port, and translates its value into MIDI controller messages. I wrote
this because my synthesizer hardwires its expression pedal to control volume,
rather than making it assignable through MIDI. Extremely simple instructions
for building the joystick port adapter (for around 5 USD in parts) are
included, but ATTEMPT THIS OR ANY OTHER CUSTOM HARDWARE PROJECT AT YOUR OWN
RISK.

  Usage: joypedal [ --out <n> ] [ --channel <n> ] [ --controller <n> ] [ --graph
]

### jumpoctave (Unix)
This filter-style utility lets you use the pitch bend wheel as an octave jump
control.

  Usage: jumpoctave <input fifo> <output fifo>

### lsmidiins (Windows)
This command line utility displays a numbered list of the Windows MIDI input
ports. Most of the other Windows utilities here refer to ports by number.

  Usage: lsmidiins

### lsmidiouts (Windows)
This command line utility displays a numbered list of the Windows MIDI output
ports.

  Usage: lsmidiouts

### metercaster (Windows)
This command line utility implements a brand new, unique algorithm for
adjusting the timing of a MIDI sequence. Unlike conventional quantization,
meter casting does not replace the human characteristics of your playing with a
mechanistic feel. Instead, you place waypoints in the sequence to tell
metercaster what the proper time should be for events of your choosing, and it
interpolates among them, coming up with the proper times for the rest of the
events. This version is meant to be used iteratively, as you specify a single
waypoint per invocation. Metercaster is especially useful in conjunction with
brainstorm, whose interfaceless design makes a metronome inappropriate.

  Usage: metercaster [ --left ( marker <name> | cc <number> <value> ) ] [
--old-right ( marker <name> | cc <number> <value> ) ] [ --new-right ( marker
<name> | cc <number> <value> ) ] [ --verbose ] <filename>

### midimon (Windows)
This command line utility pretty-prints incoming MIDI messages from any number
of Windows MIDI input ports. It can be useful for debugging complex MIDI
setups, but there's nothing special about it.

  Usage: midimon [ --in <n> ] ...

### midithru (Windows)
This command line utility reads from any number of Windows MIDI input ports,
merges the streams together, and copies the result to any number of Windows
MIDI output ports. You can think of it as a combination of a hardware thru box
and merge box.

  Usage: midithru [ --in <input device id> ... ] [ --out <output device id> ... ]

### mish (Windows, Unix)
This command line utility is a compiler for a text-based music notation
language which I invented, called "Mish" (MIDI shorthand). It converts Mish
files into standard MIDI files.

  Usage: mish --in <input.mish> --out <output.mid>

### multiecho (Windows)
This command line utility can add multiple echoes to your performance, with
configurable delay, pitch offset, and velocity scaling.

  Usage: multiecho [ --in <n> ] [ --out <n> ]
                   [ --echo <delay in ms> <note interval>
                            <velocity percent> ... ]

### netmidic (Windows)
This command line utility is a client for the NetMIDI protocol. It forwards
messages from a Windows MIDI input port to a NetMIDI server. NetMIDI can be
used to connect up the MIDI systems on two different machines over TCP/IP, even
if they are running different operating systems.

  Usage: netmidic [ --in <n> ] [ --hostname <name> ] [ --port <n> ]

### netmidid (Windows)
This command line utility is a daemon (server) for the NetMIDI protocol. It
forwards messages from NetMIDI clients to a Windows MIDI output port.

  Usage: netmidid [ --out <n> ] [ --port <n> ]

### net2pipe (Unix)
Most of the other Unix utilities here communicate over FIFOs, but this utility
can be used to bridge them to NetMIDI. Actually, this utility will copy any
data from a TCP connection to a FIFO, so it is not MIDI specific; it is similar
to netcat (which I didn't know about at the time I wrote this).

  Usage: net2pipe <port> [ <output fifo< ]

### normalizesmf (Windows)
This command line utility is a minimal demonstration of the midifile library.
It reads in a MIDI file, then writes it out again. If your sequencer complains
that a file is invalid, this normalizer might make it more palatable.

  Usage: normalizesmf <filename>

### notemap (Windows)
This command line utility lets you remap the layout of notes on your MIDI
keyboard. I originally wrote it to see whether the guitar concept of "alternate
tunings" could be applied to the piano. That experiment was a failure, but
notemap has since become essential to me for playing drums from the piano
keyboard.

  Usage: notemap <filename>.xml

### onpedal (Windows)
This command line utility runs a program or script every time you press the
sustain pedal. If you hold down the pedal for more time, it runs a second
program. I use it for turning pages in scanned copies of sheet music displayed
on screen (I have more than one pedal, so I don't have to give up sustain).

  Usage: onpedal --in <n> --command <str> [ --hold-length <n> ]
                 [ --hold-command <str> ]

### padpedal (Windows)
This command line utility is a variation on the idea of a sustenuto pedal (the
middle pedal on most grand pianos) which adds a "pad" accompaniment to your
playing.

  Usage: padpedal [ --in <port> <channel> ]
                  [ --pad-in <port> <channel> <controller> ] 
                  [ --out <port> <channel> ] [ --pad-out <port> <channel> ]

### pedalnote (Windows)
This command line utility plays a note whenever you press the sustain pedal. I
use it for kick drums.

  Usage: pedalnote [ --in <n> ] [ --out <n> ] [ --channel <n> ] [ --note <n> ]
                   [ --velocity <n> ]

### pipe2net (Unix)
As you could probably tell from its name, this utility provides the inverse of
net2pipe.

  Usage: pipe2net [ <input fifo< ] <hostname> <port>

### playsmf (Windows)
This command line utility is a generic MIDI file player. Nothing special, but
it gets along well with the other utilities. Now based on the midifile library,
this replaces the older mciplaysmf.

  Usage: playsmf [ --out <device id> ] [ -- ] <filename.mid>

### pulsar (Windows)
This command line utility pulses any depressed notes according to a
configurable rhythm.

  Usage: pulsar [ --in <n> ] [ --out <n> ]
                [ --pulse <time before pulse> <length of pulse>
                          <time after pulse> ] ...

### qwertymidi (Windows)
This command line/text mode interactive utility lets you use your computer
keyboard as if it were a synthesizer keyboard. It supports custom mapping of
the keyboard layout by means of a config file. A list of keyboard scan codes
will be useful for configuring the mapping. Sample maps are included for the
common piano-like tracker keyboard layout, the experimental von Janko piano
keyboard layout from the 1850's, the Hayden accordian button layout, and a
General MIDI drum kit.

  Usage: qwertymidi [ --out <n> ] [ --channel <n> ] [ --program <n> ]
                    [ --velocity <n> ] [ --map <str> ]

### rw (Unix)
On some operating systems, devices such as /dev/midi can only be opened by a
single program at a time, which is a problem if you want different programs to
read and write to it simultaneously. This utility, while not necessarily MIDI
specific, can be used to split out a device's input and output to separate
FIFOs.

  Usage: rw <device file> <input fifo> <output fifo>

### sendmidi (Windows)
This command line utility sends a single specified MIDI message. It can be used
for scripting, or as a poor man's knob box.

  Usage: sendmidi [ --out <port> ] 
        ( --note-off <channel> <pitch> <velocity> |
          --note-on <channel> <pitch> <velocity> |
          --key-pressure <channel> <pitch> <amount> |
          --control-change <channel> <number> <value> |
          --program-change <channel> <number> |
          --channel-pressure <channel> <amount> |
          --pitch-wheel <channel> <amount> )

### smftoxml (Windows, Unix)
This command line utility is another demonstration of the midifile library. It
converts a MIDI file into an ad hoc XML equivalent. This can be useful for
seeing exactly what is in the file, and how it is arranged in the data
structures of midifilelib.

  Usage: smftoxml <filename>

### tempo-map (Windows)
This command line utility can be used to acheive a similar effect to
metercaster, but is far faster and more intuitive to use. Inspired by, but more
powerful than the "fit to improvisation" feature in Cakewalk, it expects the
following usage model: First, you record your performance without a metronome,
as with brainstorm. Next, ignoring any beat markers your sequencer might
display, you add a click track consisting of one note per beat, synchronized
with your performance. Tempo-map then processes the file, adjusting the
timestamps of existing events and inserting new tempo events so that
performance sounds the same when played back, but the notes will line up with
beats when displayed in the sequencer. You can then selectively delete the
inserted tempo events, resulting in a steady but still completely nuanced
recording.

  Usage: tempo-map --click-track <n> [ --out <filename.mid> ] <filename.mid>

### transpose (Unix)
This filter-style utility transposes incoming MIDI messages by a specified
interval.

  Usage: transpose <input fifo> <output fifo> <interval>

### velocityfader (Windows)
This command line utility scales incoming note velocities according to the
current value of a specified continuous controller. This gives you a different
sort of volume control than normal, in that it only affects new notes coming
in, leaving the sustained ones alone. It can also be used to add pedal-operated
volume control to a synth that doesn't support standard expression messages.

    Usage: velocityfader [ --in <n> ] [ --out <n> ] [ --controller <n> ]
                         [ --inverted ]

### verbosify (Unix)
This filter-style utility normalizes incoming MIDI messages to a canonical
form, like a realtime equivalent to normalizesmf. It should not change the way
things sound, but may make it easier to postprocess the stream.

  Usage: verbosify <input fifo> <output fifo>

### xmidiqwerty (Unix)
This is an experimental, partially completed program which lets you use a MIDI
keyboard as a chording text keyboard in X11. It runs as a standard program
sending synthetic keyboard events rather than an IME, so it does not require
any complex installation nor disrupt the use of your normal keyboard.

  Usage: xmidiqwerty (reads from standard input)

### xmltosmf (Windows)
This command line utility converts the ad hoc XML format emitted from smftoxml
back into a valid MIDI file. By combining the two of them, you can modify MIDI
files with a text editor or even with XSLT.

  Usage: xmltosmf <filename>.xml <filename>.mid

Useful Libraries Also Provided

### midifile (Windows, Unix)
This powerful and practical library allows you to read and write Standard MIDI
Files (SMF), and provides a data structure for MIDI sequences. Essentially, it
is the core of a MIDI sequencer without the user interface and the realtime
recording and playback functionality.

### midimsg (Unix)
This library provides an abstraction around raw realtime MIDI streams. It hides
annoying complexities like running status and active sensing. Unnecessary for
Windows or native ALSA programs, since this functionality is already provided
for you there.

## Related Links
You can find the latest version of these utilities on my website.

On Windows, a MIDI loopback utility is necessary to get the most out of these
programs. It will provide extra Windows MIDI ports which allow you to wire
multiple MIDI programs together instead of speaking directly to your sound card
or synthesizer. While several such programs exist, both free and commercial, I
have had good luck with the free MIDI Yoke. Make sure you download the correct
version, as there are separate ones for Windows 95/98/ME and NT/2000/XP.

Dave Phillips maintains the central directory of sound and MIDI software for
Linux. If you're using that platform, anything else you might need is either
there, or you'll have to write it yourself. ☺

dgslomin (at) alumni (dot) princeton (dot) edu
