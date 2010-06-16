
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <alsa/asoundlib.h>
#include <midifile.h>

static char *prefix = "brainstorm-";
static int timeout = 5;
static MidiFile_t midi_file = NULL;
static MidiFileTrack_t track = NULL;
static struct timeval first_event_timestamp;

static void create_midi_file_for_first_event(void)
{
	if (midi_file != NULL) return;

	midi_file = MidiFile_new(1, MIDI_FILE_DIVISION_TYPE_PPQ, 960);
	track = MidiFile_createTrack(midi_file); /* conductor track */

	{
		unsigned char time_signature[] = { 4, 2, 24, 8 }; /* 4/4 */
		MidiFileTrack_createMetaEvent(track, 0, 0x58, 4, time_signature);
	}

	{
		unsigned char key_signature[] = { 0, 0 }; /* C major */
		MidiFileTrack_createMetaEvent(track, 0, 0x59, 2, key_signature);
	}

	{
		unsigned char tempo[] = { 0x09, 0x27, 0xC0 }; /* 100 BPM */
		MidiFileTrack_createMetaEvent(track, 0, 0x51, 3, tempo);
	}

	track = MidiFile_createTrack(midi_file); /* main track */

	gettimeofday(&first_event_timestamp, NULL);
}

static long get_tick(void)
{
	struct timeval current_timestamp;
	gettimeofday(&current_timestamp, NULL);
	return MidiFile_getTickFromTime(midi_file, (float)(current_timestamp.tv_sec - first_event_timestamp.tv_sec) + ((float)(current_timestamp.tv_usec - first_event_timestamp.tv_usec) / 1000000.0));
}

static void alarm_handler(int signum)
{
	char filename[1024];

	{
		time_t current_time;
		struct tm *current_time_struct;
		char current_time_string[1024];

		current_time = time(NULL);
		current_time_struct = localtime(&current_time);
		strftime(current_time_string, 1024, "%Y%m%d%H%M%S", current_time_struct);

		sprintf(filename, "%s%s.mid", prefix, current_time_string);
	}

	MidiFile_save(midi_file, filename);
	MidiFile_free(midi_file);
	midi_file = NULL;
}

static void usage(char *program_name)
{
	fprintf(stderr, "%s [ --prefix <filename prefix> ] [ --timeout <seconds> ]\n", program_name);
	exit(1);
}

int main(int argc, char **argv)
{
	int i;

	for (i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "--prefix") == 0)
		{
			if (++i == argc) usage(argv[0]);
			prefix = argv[i];
		}
		else if (strcmp(argv[i], "--timeout") == 0)
		{
			if (++i == argc) usage(argv[0]);
			timeout = atoi(argv[i]);
		}
		else
		{
			usage(argv[0]);
		}
	}

	{
		snd_seq_t *seq;

		if (snd_seq_open(&seq, "default", SND_SEQ_OPEN_INPUT, 0) < 0)
		{
			fprintf(stderr, "Cannot open the ALSA sequencer.\n");
			exit(1);
		}

		snd_seq_set_client_name(seq, "Brainstorm");
		snd_seq_create_simple_port(seq, "Brainstorm recording port", SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE, SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION);

		signal(SIGALRM, alarm_handler);

		{
			snd_seq_event_t *event;

			while (snd_seq_event_input(seq, &event) >= 0)
			{
				switch (event->type)
				{
					case SND_SEQ_EVENT_NOTEOFF:
					{
						alarm(timeout);
						create_midi_file_for_first_event();
						MidiFileTrack_createNoteOffEvent(track, get_tick(), event->data.note.channel, event->data.note.note, event->data.note.velocity);
						break;
					}
					case SND_SEQ_EVENT_NOTEON:
					{
						alarm(timeout);
						create_midi_file_for_first_event();
						MidiFileTrack_createNoteOnEvent(track, get_tick(), event->data.note.channel, event->data.note.note, event->data.note.velocity);
						break;
					}
					case SND_SEQ_EVENT_KEYPRESS:
					{
						alarm(timeout);
						create_midi_file_for_first_event();
						MidiFileTrack_createKeyPressureEvent(track, get_tick(), event->data.note.channel, event->data.note.note, event->data.note.velocity);
						break;
					}
					case SND_SEQ_EVENT_CONTROLLER:
					{
						alarm(timeout);
						create_midi_file_for_first_event();
						MidiFileTrack_createControlChangeEvent(track, get_tick(), event->data.control.channel, event->data.control.param, event->data.control.value);
						break;
					}
					case SND_SEQ_EVENT_PGMCHANGE:
					{
						alarm(timeout);
						create_midi_file_for_first_event();
						MidiFileTrack_createProgramChangeEvent(track, get_tick(), event->data.control.channel, event->data.control.value);
						break;
					}
					case SND_SEQ_EVENT_CHANPRESS:
					{
						alarm(timeout);
						create_midi_file_for_first_event();
						MidiFileTrack_createChannelPressureEvent(track, get_tick(), event->data.control.channel, event->data.control.value);
						break;
					}
					case SND_SEQ_EVENT_PITCHBEND:
					{
						alarm(timeout);
						create_midi_file_for_first_event();
						MidiFileTrack_createPitchWheelEvent(track, get_tick(), event->data.control.channel, event->data.control.value);
						break;
					}
					default:
					{
						/* I'm ignoring the pseudoevents which ALSA provides as convenience features (SND_SEQ_EVENT_NOTE, SND_SEQ_EVENT_CONTROL14, SND_SEQ_EVENT_NONREGPARAM, and SND_SEQ_EVENT_REGPARAM).  Hopefully I can find some way to convince ALSA to normalize them into true MIDI events. */
						break;
					}
				}
			}
		}
	}

	return 0;
}

