#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "midifile.h"

/*
 * Data Types
 */

struct MF
{
	int file_format;
	MFDivisionType_t division_type;
	int resolution;
	int number_of_tracks;
	struct MFTrack *first_track;
	struct MFTrack *last_track;
	struct MFEvent *first_event;
	struct MFEvent *last_event;
};

struct MFTrack
{
	struct MF *midi_file;
	int number;
	long end_tick;
	struct MFTrack *previous_track;
	struct MFTrack *next_track;
	struct MFEvent *first_event;
	struct MFEvent *last_event;
};

struct MFEvent
{
	struct MFTrack *track;
	struct MFEvent *previous_event_in_track;
	struct MFEvent *next_event_in_track;
	struct MFEvent *previous_event_in_file;
	struct MFEvent *next_event_in_file;
	long tick;
	MFEventType_t type;

	union
	{
		struct
		{
			int channel;
			int note;
			int velocity;
		}
		note_off;

		struct
		{
			int channel;
			int note;
			int velocity;
		}
		note_on;

		struct
		{
			int channel;
			int note;
			int amount;
		}
		key_pressure;

		struct
		{
			int channel;
			int number;
			int value;
		}
		control_change;

		struct
		{
			int channel;
			int number;
		}
		program_change;

		struct
		{
			int channel;
			int amount;
		}
		channel_pressure;

		struct
		{
			int channel;
			int value;
		}
		pitch_wheel;

		struct
		{
			int data_length;
			unsigned char *data_buffer;
		}
		sysex;

		struct
		{
			int number;
			int data_length;
			unsigned char *data_buffer;
		}
		meta;
	}
	u;

	int should_be_visited;
};

/*
 * Helpers
 */

static signed short interpret_int16(unsigned char *buffer)
{
	return ((signed short)(buffer[0]) << 8) | (signed short)(buffer[1]);
}

static signed short read_int16(FILE *in)
{
	unsigned char buffer[2];
	fread(buffer, 1, 2, in);
	return interpret_int16(buffer);
}

static void write_int16(FILE *out, signed short value)
{
	unsigned char buffer[2];
	buffer[0] = (unsigned char)((value >> 8) & 0xFF);
	buffer[1] = (unsigned char)(value & 0xFF);
	fwrite(buffer, 1, 2, out);
}

static unsigned short interpret_uint16(unsigned char *buffer)
{
	return ((unsigned short)(buffer[0]) << 8) | (unsigned short)(buffer[1]);
}

static unsigned short read_uint16(FILE *in)
{
	unsigned char buffer[2];
	fread(buffer, 1, 2, in);
	return interpret_uint16(buffer);
}

static void write_uint16(FILE *out, unsigned short value)
{
	unsigned char buffer[2];
	buffer[0] = (unsigned char)((value >> 8) & 0xFF);
	buffer[1] = (unsigned char)(value & 0xFF);
	fwrite(buffer, 1, 2, out);
}

static unsigned long interpret_uint32(unsigned char *buffer)
{
	return ((unsigned long)(buffer[0]) << 24) | ((unsigned long)(buffer[1]) << 16) | ((unsigned long)(buffer[2]) << 8) | (unsigned long)(buffer[3]);
}

static unsigned long read_uint32(FILE *in)
{
	unsigned char buffer[4];
	fread(buffer, 1, 4, in);
	return interpret_uint32(buffer);
}

static void write_uint32(FILE *out, unsigned long value)
{
	unsigned char buffer[4];
	buffer[0] = (unsigned char)(value >> 24);
	buffer[1] = (unsigned char)((value >> 16) & 0xFF);
	buffer[2] = (unsigned char)((value >> 8) & 0xFF);
	buffer[3] = (unsigned char)(value & 0xFF);
	fwrite(buffer, 1, 4, out);
}

static unsigned long read_variable_length_quantity(FILE *in)
{
	unsigned char b;
	unsigned long value = 0;

	do
	{
		b = fgetc(in);
		value = (value << 7) | (b & 0x7F);
	}
	while ((b & 0x80) == 0x80);

	return value;
}

static void write_variable_length_quantity(FILE *out, unsigned long value)
{
	unsigned char buffer[4];
	int offset = 3;

	while (1)
	{
		buffer[offset] = (unsigned char)(value & 0x7F);
		if (offset < 3) buffer[offset] |= 0x80;
		value >>= 7;
		if ((value == 0) || (offset == 0)) break;
		offset--;
	}

	fwrite(buffer + offset, 1, 4 - offset, out);
}

static void add_event(MFEvent_t new_event)
{
	/* Add in proper sorted order.  Search backwards to optimize for appending. */

	MFEvent_t event;

	for (event = new_event->track->last_event; (event != NULL) && (new_event->tick < event->tick); event = event->previous_event_in_track) {}

	new_event->previous_event_in_track = event;

	if (event == NULL)
	{
		new_event->next_event_in_track = new_event->track->first_event;
		new_event->track->first_event = new_event;
	}
	else
	{
		new_event->next_event_in_track = event->next_event_in_track;
		event->next_event_in_track = new_event;
	}

	if (new_event->next_event_in_track == NULL)
	{
		new_event->track->last_event = new_event;
	}
	else
	{
		new_event->next_event_in_track->previous_event_in_track = new_event;
	}

	for (event = new_event->track->midi_file->last_event; (event != NULL) && (new_event->tick < event->tick); event = event->previous_event_in_file) {}

	new_event->previous_event_in_file = event;

	if (event == NULL)
	{
		new_event->next_event_in_file = new_event->track->midi_file->first_event;
		new_event->track->midi_file->first_event = new_event;
	}
	else
	{
		new_event->next_event_in_file = event->next_event_in_file;
		event->next_event_in_file = new_event;
	}

	if (new_event->next_event_in_file == NULL)
	{
		new_event->track->midi_file->last_event = new_event;
	}
	else
	{
		new_event->next_event_in_file->previous_event_in_file = new_event;
	}

	if (new_event->tick > new_event->track->end_tick) new_event->track->end_tick = new_event->tick;
}

static void remove_event(MFEvent_t event)
{
	if (event->previous_event_in_track == NULL)
	{
		event->track->first_event = event->next_event_in_track;
	}
	else
	{
		event->previous_event_in_track->next_event_in_track = event->next_event_in_track;
	}

	if (event->next_event_in_track == NULL)
	{
		event->track->last_event = event->previous_event_in_track;
	}
	else
	{
		event->next_event_in_track->previous_event_in_track = event->previous_event_in_track;
	}

	if (event->previous_event_in_file == NULL)
	{
		event->track->midi_file->first_event = event->next_event_in_file;
	}
	else
	{
		event->previous_event_in_file->next_event_in_file = event->next_event_in_file;
	}

	if (event->next_event_in_file == NULL)
	{
		event->track->midi_file->last_event = event->previous_event_in_file;
	}
	else
	{
		event->next_event_in_file->previous_event_in_file = event->previous_event_in_file;
	}
}

static void free_events_in_track(MFTrack_t track)
{
	MFEvent_t event, next_event_in_track;

	for (event = track->first_event; event != NULL; event = next_event_in_track)
	{
		next_event_in_track = event->next_event_in_track;

		switch (event->type)
		{
			case MFEVENT_TYPE_SYSEX:
			{
				free(event->u.sysex.data_buffer);
				break;
			}
			case MFEVENT_TYPE_META:
			{
				free(event->u.meta.data_buffer);
				break;
			}
		}

		free(event);
	}
}

/*
 * Public API
 */

MF_t MF_load(char *filename)
{
	MF_t midi_file;
	FILE *in;
	unsigned char chunk_id[4], division_type_and_resolution[4];
	long chunk_size, chunk_start;
	int file_format, number_of_tracks, number_of_tracks_read = 0;

	if ((filename == NULL) || ((in = fopen(filename, "rb")) == NULL)) return NULL;

	fread(chunk_id, 1, 4, in);
	chunk_size = read_uint32(in);
	chunk_start = ftell(in);

	/* check for the RMID variation on SMF */

	if (memcmp(chunk_id, "RIFF", 4) == 0)
	{
		fread(chunk_id, 1, 4, in); /* technically this one is a type id rather than a chunk id, but we'll reuse the buffer anyway */

		if (memcmp(chunk_id, "RMID", 4) != 0)
		{
			fclose(in);
			return NULL;
		}

		fread(chunk_id, 1, 4, in);
		chunk_size = read_uint32(in);

		if (memcmp(chunk_id, "data", 4) != 0)
		{
			fclose(in);
			return NULL;
		}

		fread(chunk_id, 1, 4, in);
		chunk_size = read_uint32(in);
		chunk_start = ftell(in);
	}

	if (memcmp(chunk_id, "MThd", 4) != 0)
	{
		fclose(in);
		return NULL;
	}

	file_format = read_uint16(in);
	number_of_tracks = read_uint16(in);
	fread(division_type_and_resolution, 1, 2, in);

	switch ((signed char)(division_type_and_resolution[0]))
	{
		case -24:
		{
			midi_file = MF_new(file_format, MFDIVISION_TYPE_SMPTE24, division_type_and_resolution[1]);
			break;
		}
		case -25:
		{
			midi_file = MF_new(file_format, MFDIVISION_TYPE_SMPTE25, division_type_and_resolution[1]);
			break;
		}
		case -29:
		{
			midi_file = MF_new(file_format, MFDIVISION_TYPE_SMPTE30DROP, division_type_and_resolution[1]);
			break;
		}
		case -30:
		{
			midi_file = MF_new(file_format, MFDIVISION_TYPE_SMPTE30, division_type_and_resolution[1]);
			break;
		}
		default:
		{
			midi_file = MF_new(file_format, MFDIVISION_TYPE_PPQ, interpret_uint16(division_type_and_resolution));
			break;
		}
	}

	/* forwards compatibility:  skip over any extra header data */
	fseek(in, chunk_start + chunk_size, SEEK_SET);

	while (number_of_tracks_read < number_of_tracks)
	{
		fread(chunk_id, 1, 4, in);
		chunk_size = read_uint32(in);
		chunk_start = ftell(in);

		if (memcmp(chunk_id, "MTrk", 4) == 0)
		{
			MFTrack_t track = MF_createTrack(midi_file);
			long tick, previous_tick = 0;
			unsigned char status, running_status = 0;
			int at_end_of_track = 0;

			while ((ftell(in) < chunk_start + chunk_size) && !at_end_of_track)
			{
				tick = read_variable_length_quantity(in) + previous_tick;
				previous_tick = tick;

				status = fgetc(in);

				if ((status & 0x80) == 0x00)
				{
					status = running_status;
					fseek(in, -1, SEEK_CUR);
				}
				else
				{
					running_status = status;
				}

				switch (status & 0xF0)
				{
					case 0x80:
					{
						int channel = status & 0x0F;
						int note = fgetc(in);
						int velocity = fgetc(in);
						MFTrack_createNoteOffEvent(track, tick, channel, note, velocity);
						break;
					}
					case 0x90:
					{
						int channel = status & 0x0F;
						int note = fgetc(in);
						int velocity = fgetc(in);
						MFTrack_createNoteOnEvent(track, tick, channel, note, velocity);
						break;
					}
					case 0xA0:
					{
						int channel = status & 0x0F;
						int note = fgetc(in);
						int amount = fgetc(in);
						MFTrack_createKeyPressureEvent(track, tick, channel, note, amount);
						break;
					}
					case 0xB0:
					{
						int channel = status & 0x0F;
						int number = fgetc(in);
						int value = fgetc(in);
						MFTrack_createControlChangeEvent(track, tick, channel, number, value);
						break;
					}
					case 0xC0:
					{
						int channel = status & 0x0F;
						int number = fgetc(in);
						MFTrack_createProgramChangeEvent(track, tick, channel, number);
						break;
					}
					case 0xD0:
					{
						int channel = status & 0x0F;
						int amount = fgetc(in);
						MFTrack_createChannelPressureEvent(track, tick, channel, amount);
						break;
					}
					case 0xE0:
					{
						int channel = status & 0x0F;
						int value = fgetc(in);
						value = (value << 7) | fgetc(in);
						MFTrack_createPitchWheelEvent(track, tick, channel, value);
						break;
					}
					case 0xF0:
					{
						switch (status)
						{
							case 0xF0:
							case 0xF7:
							{
								int data_length = read_variable_length_quantity(in) + 1;
								unsigned char *data_buffer = malloc(data_length);
								data_buffer[0] = status;
								fread(data_buffer + 1, 1, data_length - 1, in);
								MFTrack_createSysexEvent(track, tick, data_length, data_buffer);
								free(data_buffer);
								break;
							}
							case 0xFF:
							{
								int number = fgetc(in);
								int data_length = read_variable_length_quantity(in);
								unsigned char *data_buffer = malloc(data_length);
								fread(data_buffer, 1, data_length, in);

								if (number == 0x2F)
								{
									MFTrack_setEndTick(track, tick);
									at_end_of_track = 1;
								}
								else
								{
									MFTrack_createMetaEvent(track, tick, number, data_length, data_buffer);
								}

								free(data_buffer);
								break;
							}
						}

						break;
					}
				}
			}

			number_of_tracks_read++;
		}

		/* forwards compatibility:  skip over any unrecognized chunks, or extra data at the end of tracks */
		fseek(in, chunk_start + chunk_size, SEEK_SET);
	}

	fclose(in);
	return midi_file;
}

int MF_save(MF_t midi_file, const char* filename)
{
	FILE *out;
	MFTrack_t track;

	if ((midi_file == NULL) || (filename == NULL) || ((out = fopen(filename, "wb")) == NULL)) return -1;

	fwrite("MThd", 1, 4, out);
	write_uint32(out, 6);
	write_uint16(out, (unsigned short)(MF_getFileFormat(midi_file)));
	write_uint16(out, (unsigned short)(MF_getNumberOfTracks(midi_file)));

	switch (MF_getDivisionType(midi_file))
	{
		case MFDIVISION_TYPE_PPQ:
		{
			write_uint16(out, (unsigned short)(MF_getResolution(midi_file)));
			break;
		}
		case MFDIVISION_TYPE_SMPTE24:
		{
			fputc(-24, out);
			fputc(MF_getResolution(midi_file), out);
			break;
		}
		case MFDIVISION_TYPE_SMPTE25:
		{
			fputc(-25, out);
			fputc(MF_getResolution(midi_file), out);
			break;
		}
		case MFDIVISION_TYPE_SMPTE30DROP:
		{
			fputc(-29, out);
			fputc(MF_getResolution(midi_file), out);
			break;
		}
		case MFDIVISION_TYPE_SMPTE30:
		{
			fputc(-30, out);
			fputc(MF_getResolution(midi_file), out);
			break;
		}
	}

	for (track = MF_getFirstTrack(midi_file); track != NULL; track = MFTrack_getNextTrack(track))
	{
		MFEvent_t event;
		long track_size_offset, track_start_offset, track_end_offset, tick, previous_tick;

		fwrite("MTrk", 1, 4, out);

		track_size_offset = ftell(out);
		write_uint32(out, 0);

		track_start_offset = ftell(out);

		previous_tick = 0;

		for (event = MFTrack_getFirstEvent(track); event != NULL; event = MFEvent_getNextEventInTrack(event))
		{
			tick = MFEvent_getTick(event);
			write_variable_length_quantity(out, tick - previous_tick);

			switch (MFEvent_getType(event))
			{
				case MFEVENT_TYPE_NOTE_OFF:
				{
					fputc(0x80 | MFNoteOffEvent_getChannel(event) & 0x0F, out);
					fputc(MFNoteOffEvent_getNote(event) & 0x7F, out);
					fputc(MFNoteOffEvent_getVelocity(event) & 0x7F, out);
					break;
				}
				case MFEVENT_TYPE_NOTE_ON:
				{
					fputc(0x90 | MFNoteOnEvent_getChannel(event) & 0x0F, out);
					fputc(MFNoteOnEvent_getNote(event) & 0x7F, out);
					fputc(MFNoteOnEvent_getVelocity(event) & 0x7F, out);
					break;
				}
				case MFEVENT_TYPE_KEY_PRESSURE:
				{
					fputc(0xA0 | MFKeyPressureEvent_getChannel(event) & 0x0F, out);
					fputc(MFKeyPressureEvent_getNote(event) & 0x7F, out);
					fputc(MFKeyPressureEvent_getAmount(event) & 0x7F, out);
					break;
				}
				case MFEVENT_TYPE_CONTROL_CHANGE:
				{
					fputc(0xB0 | MFControlChangeEvent_getChannel(event) & 0x0F, out);
					fputc(MFControlChangeEvent_getNumber(event) & 0x7F, out);
					fputc(MFControlChangeEvent_getValue(event) & 0x7F, out);
					break;
				}
				case MFEVENT_TYPE_PROGRAM_CHANGE:
				{
					fputc(0xC0 | MFProgramChangeEvent_getChannel(event) & 0x0F, out);
					fputc(MFProgramChangeEvent_getNumber(event) & 0x7F, out);
					break;
				}
				case MFEVENT_TYPE_CHANNEL_PRESSURE:
				{
					fputc(0xD0 | MFChannelPressureEvent_getChannel(event) & 0x0F, out);
					fputc(MFChannelPressureEvent_getAmount(event) & 0x7F, out);
					break;
				}
				case MFEVENT_TYPE_PITCH_WHEEL:
				{
					int value = MFPitchWheelEvent_getValue(event);
					fputc(0xE0 | MFPitchWheelEvent_getChannel(event) & 0x0F, out);
					fputc((value >> 7) & 0x7F, out);
					fputc(value & 0x7F, out);
					break;
				}
				case MFEVENT_TYPE_SYSEX:
				{
					int data_length = MFSysexEvent_getDataLength(event);
					unsigned char *data = MFSysexEvent_getData(event);
					fputc(data[0], out);
					write_variable_length_quantity(out, data_length - 1);
					fwrite(data + 1, 1, data_length - 1, out);
					break;
				}
				case MFEVENT_TYPE_META:
				{
					int data_length = MFMetaEvent_getDataLength(event);
					unsigned char *data = MFMetaEvent_getData(event);
					fputc(0xFF, out);
					fputc(MFMetaEvent_getNumber(event) & 0x7F, out);
					write_variable_length_quantity(out, data_length);
					fwrite(data, 1, data_length, out);
					break;
				}
			}

			previous_tick = tick;
		}

		write_variable_length_quantity(out, MFTrack_getEndTick(track) - previous_tick);
		fwrite("\xFF\x2F\x00", 1, 3, out);

		track_end_offset = ftell(out);

		fseek(out, track_size_offset, SEEK_SET);
		write_uint32(out, track_end_offset - track_start_offset);

		fseek(out, track_end_offset, SEEK_SET);
	}

	fclose(out);
	return 0;
}

MF_t MF_new(int file_format, MFDivisionType_t division_type, int resolution)
{
	MF_t midi_file = (MF_t)(malloc(sizeof(struct MF)));
	midi_file->file_format = file_format;
	midi_file->division_type = division_type;
	midi_file->resolution = resolution;
	midi_file->number_of_tracks = 0;
	midi_file->first_track = NULL;
	midi_file->last_track = NULL;
	midi_file->first_event = NULL;
	midi_file->last_event = NULL;
	return midi_file;
}

int MF_free(MF_t midi_file)
{
	MFTrack_t track, next_track;

	if (midi_file == NULL) return -1;

	for (track = midi_file->first_track; track != NULL; track = next_track)
	{
		next_track = track->next_track;
		free_events_in_track(track);
		free(track);
	}

	free(midi_file);
	return 0;
}

int MF_getFileFormat(MF_t midi_file)
{
	if (midi_file == NULL) return -1;
	return midi_file->file_format;
}

int MF_setFileFormat(MF_t midi_file, int file_format)
{
	if (midi_file == NULL) return -1;
	midi_file->file_format = file_format;
	return 0;
}

MFDivisionType_t MF_getDivisionType(MF_t midi_file)
{
	if (midi_file == NULL) return MFDIVISION_TYPE_INVALID;
	return midi_file->division_type;
}

int MF_setDivisionType(MF_t midi_file, MFDivisionType_t division_type)
{
	if (midi_file == NULL) return -1;
	midi_file->division_type = division_type;
	return 0;
}

int MF_getResolution(MF_t midi_file)
{
	if (midi_file == NULL) return -1;
	return midi_file->resolution;
}

int MF_setResolution(MF_t midi_file, int resolution)
{
	if (midi_file == NULL) return -1;
	midi_file->resolution = resolution;
	return 0;
}

MFTrack_t MF_createTrack(MF_t midi_file)
{
	MFTrack_t new_track;

	if (midi_file == NULL) return NULL;

	new_track = (MFTrack_t)(malloc(sizeof(struct MFTrack)));
	new_track->midi_file = midi_file;
	new_track->number = midi_file->number_of_tracks;
	new_track->end_tick = 0;
	new_track->previous_track = midi_file->last_track;
	new_track->next_track = NULL;
	midi_file->last_track = new_track;

	if (new_track->previous_track == NULL)
	{
		midi_file->first_track = new_track;
	}
	else
	{
		new_track->previous_track->next_track = new_track;
	}

	(midi_file->number_of_tracks)++;

	new_track->first_event = NULL;
	new_track->last_event = NULL;

	return new_track;
}

int MF_getNumberOfTracks(MF_t midi_file)
{
	if (midi_file == NULL) return -1;
	return midi_file->number_of_tracks;
}

MFTrack_t MF_getTrackByNumber(MF_t midi_file, int number, int create)
{
	int current_track_number;
	MFTrack_t track = NULL;

	for (current_track_number = 0; current_track_number <= number; current_track_number++)
	{
		if (track == NULL)
		{
			track = MF_getFirstTrack(midi_file);
		}
		else
		{
			track = MFTrack_getNextTrack(track);
		}

		if ((track == NULL) && create)
		{
			track = MF_createTrack(midi_file);
		}
	}

	return track;
}

MFTrack_t MF_getFirstTrack(MF_t midi_file)
{
	if (midi_file == NULL) return NULL;
	return midi_file->first_track;
}

MFTrack_t MF_getLastTrack(MF_t midi_file)
{
	if (midi_file == NULL) return NULL;
	return midi_file->last_track;
}

MFEvent_t MF_getFirstEvent(MF_t midi_file)
{
	if (midi_file == NULL) return NULL;
	return midi_file->first_event;
}

MFEvent_t MF_getLastEvent(MF_t midi_file)
{
	if (midi_file == NULL) return NULL;
	return midi_file->last_event;
}

int MF_visitEvents(MF_t midi_file, MFEventVisitorCallback_t visitor_callback, void *user_data)
{
	MFEvent_t event, next_event;

	if ((midi_file == NULL) || (visitor_callback == NULL)) return -1;

	for (event = MF_getFirstEvent(midi_file); event != NULL; event = MFEvent_getNextEventInFile(event)) event->should_be_visited = 1;

	for (event = MF_getFirstEvent(midi_file); event != NULL; event = next_event)
	{
		next_event = MFEvent_getNextEventInFile(event);

		if (event->should_be_visited)
		{
			event->should_be_visited = 0;
			(*visitor_callback)(event, user_data);
		}
	}

	return 0;
}

float MF_getTimeFromTick(MF_t midi_file, long tick)
{
	switch (MF_getDivisionType(midi_file))
	{
		case MFDIVISION_TYPE_PPQ:
		{
			MFTrack_t conductor_track = MF_getFirstTrack(midi_file);
			MFEvent_t event;
			float tempo_event_time = 0.0;
			long tempo_event_tick = 0;
			float tempo = 120.0;

			for (event = MFTrack_getFirstEvent(conductor_track); (event != NULL) && (MFEvent_getTick(event) < tick); event = MFEvent_getNextEventInTrack(event))
			{
				if (MFEvent_isTempoEvent(event))
				{
					tempo_event_time += (((float)(MFEvent_getTick(event) - tempo_event_tick)) / MF_getResolution(midi_file) / (tempo / 60));
					tempo_event_tick = MFEvent_getTick(event);
					tempo = MFTempoEvent_getTempo(event);
				}
			}

			return tempo_event_time + (((float)(tick - tempo_event_tick)) / MF_getResolution(midi_file) / (tempo / 60));
		}
		case MFDIVISION_TYPE_SMPTE24:
		{
			return (float)(tick) / (MF_getResolution(midi_file) * 24.0);
		}
		case MFDIVISION_TYPE_SMPTE25:
		{
			return (float)(tick) / (MF_getResolution(midi_file) * 25.0);
		}
		case MFDIVISION_TYPE_SMPTE30DROP:
		{
			return (float)(tick) / (MF_getResolution(midi_file) * 29.97);
		}
		case MFDIVISION_TYPE_SMPTE30:
		{
			return (float)(tick) / (MF_getResolution(midi_file) * 30.0);
		}
		default:
		{
			return -1;
		}
	}
}

long MF_getTickFromTime(MF_t midi_file, float time)
{
	switch (MF_getDivisionType(midi_file))
	{
		case MFDIVISION_TYPE_PPQ:
		{
			MFTrack_t conductor_track = MF_getFirstTrack(midi_file);
			MFEvent_t event;
			float tempo_event_time = 0.0;
			long tempo_event_tick = 0;
			float tempo = 120.0;

			for (event = MFTrack_getFirstEvent(conductor_track); event != NULL; event = MFEvent_getNextEventInTrack(event))
			{
				if (MFEvent_isTempoEvent(event))
				{
					float next_tempo_event_time = tempo_event_time + (((float)(MFEvent_getTick(event) - tempo_event_tick)) / MF_getResolution(midi_file) / (tempo / 60));
					if (next_tempo_event_time >= time) break;
					tempo_event_time = next_tempo_event_time;
					tempo_event_tick = MFEvent_getTick(event);
					tempo = MFTempoEvent_getTempo(event);
				}
			}

			return tempo_event_tick + ((time - tempo_event_time) * (tempo / 60) * MF_getResolution(midi_file));
		}
		case MFDIVISION_TYPE_SMPTE24:
		{
			return (long)(time * MF_getResolution(midi_file) * 24.0);
		}
		case MFDIVISION_TYPE_SMPTE25:
		{
			return (long)(time * MF_getResolution(midi_file) * 25.0);
		}
		case MFDIVISION_TYPE_SMPTE30DROP:
		{
			return (long)(time * MF_getResolution(midi_file) * 29.97);
		}
		case MFDIVISION_TYPE_SMPTE30:
		{
			return (long)(time * MF_getResolution(midi_file) * 30.0);
		}
		default:
		{
			return -1;
		}
	}
}

float MF_getBeatFromTick(MF_t midi_file, long tick)
{
	switch (MF_getDivisionType(midi_file))
	{
		case MFDIVISION_TYPE_PPQ:
		{
			return (float)(tick) / MF_getResolution(midi_file);
		}
		case MFDIVISION_TYPE_SMPTE24:
		{
			return -1.0; /* TODO */
		}
		case MFDIVISION_TYPE_SMPTE25:
		{
			return -1.0; /* TODO */
		}
		case MFDIVISION_TYPE_SMPTE30DROP:
		{
			return -1.0; /* TODO */
		}
		case MFDIVISION_TYPE_SMPTE30:
		{
			return -1.0; /* TODO */
		}
		default:
		{
			return -1.0;
		}
	}
}

long MF_getTickFromBeat(MF_t midi_file, float beat)
{
	switch (MF_getDivisionType(midi_file))
	{
		case MFDIVISION_TYPE_PPQ:
		{
			return (long)(beat * MF_getResolution(midi_file));
		}
		case MFDIVISION_TYPE_SMPTE24:
		{
			return -1; /* TODO */
		}
		case MFDIVISION_TYPE_SMPTE25:
		{
			return -1; /* TODO */
		}
		case MFDIVISION_TYPE_SMPTE30DROP:
		{
			return -1; /* TODO */
		}
		case MFDIVISION_TYPE_SMPTE30:
		{
			return -1; /* TODO */
		}
		default:
		{
			return -1;
		}
	}
}

int MFTrack_delete(MFTrack_t track)
{
	MFTrack_t subsequent_track;

	if (track == NULL) return -1;

	for (subsequent_track = track->next_track; subsequent_track != NULL; subsequent_track = subsequent_track->next_track)
	{
		(subsequent_track->number)--;
	}
	
	(track->midi_file->number_of_tracks)--;

	if (track->previous_track == NULL)
	{
		track->midi_file->first_track = track->next_track;
	}
	else
	{
		track->previous_track->next_track = track->next_track;
	}

	if (track->next_track == NULL)
	{
		track->midi_file->last_track = track->previous_track;
	}
	else
	{
		track->next_track->previous_track = track->previous_track;
	}

	free_events_in_track(track);
	free(track);
	return 0;
}

MF_t MFTrack_getMF(MFTrack_t track)
{
	if (track == NULL) return NULL;
	return track->midi_file;
}

int MFTrack_getNumber(MFTrack_t track)
{
	if (track == NULL) return -1;
	return track->number;
}

long MFTrack_getEndTick(MFTrack_t track)
{
	if (track == NULL) return -1;
	return track->end_tick;
}

int MFTrack_setEndTick(MFTrack_t track, long end_tick)
{
	if ((track == NULL) || ((track->last_event != NULL) && (end_tick < track->last_event->tick))) return -1;
	track->end_tick = end_tick;
	return 0;
}

MFTrack_t MFTrack_createTrackBefore(MFTrack_t track)
{
	MFTrack_t new_track, subsequent_track;

	if (track == NULL) return NULL;

	new_track = (MFTrack_t)(malloc(sizeof(struct MFTrack)));
	new_track->midi_file = track->midi_file;
	new_track->number = track->number;
	new_track->end_tick = 0;
	new_track->previous_track = track->previous_track;
	new_track->next_track = track;
	track->previous_track = new_track;

	if (new_track->previous_track == NULL)
	{
		track->midi_file->first_track = new_track;
	}
	else
	{
		new_track->previous_track->next_track = new_track;
	}

	for (subsequent_track = track; subsequent_track != NULL; subsequent_track = subsequent_track->next_track)
	{
		(subsequent_track->number)++;
	}

	new_track->first_event = NULL;
	new_track->last_event = NULL;

	return new_track;
}

MFTrack_t MFTrack_getPreviousTrack(MFTrack_t track)
{
	if (track == NULL) return NULL;
	return track->previous_track;
}

MFTrack_t MFTrack_getNextTrack(MFTrack_t track)
{
	if (track == NULL) return NULL;
	return track->next_track;
}

MFEvent_t MFTrack_createNoteOffEvent(MFTrack_t track, long tick, int channel, int note, int velocity)
{
	MFEvent_t new_event;

	if (track == NULL) return NULL;

	new_event = (MFEvent_t)(malloc(sizeof(struct MFEvent)));
	new_event->track = track;
	new_event->tick = tick;
	new_event->type = MFEVENT_TYPE_NOTE_OFF;
	new_event->u.note_off.channel = channel;
	new_event->u.note_off.note = note;
	new_event->u.note_off.velocity = velocity;
	new_event->should_be_visited = 0;
	add_event(new_event);

	return new_event;
}

MFEvent_t MFTrack_createNoteOnEvent(MFTrack_t track, long tick, int channel, int note, int velocity)
{
	MFEvent_t new_event;

	if (track == NULL) return NULL;

	new_event = (MFEvent_t)(malloc(sizeof(struct MFEvent)));
	new_event->track = track;
	new_event->tick = tick;
	new_event->type = MFEVENT_TYPE_NOTE_ON;
	new_event->u.note_on.channel = channel;
	new_event->u.note_on.note = note;
	new_event->u.note_on.velocity = velocity;
	new_event->should_be_visited = 0;
	add_event(new_event);

	return new_event;
}

MFEvent_t MFTrack_createKeyPressureEvent(MFTrack_t track, long tick, int channel, int note, int amount)
{
	MFEvent_t new_event;

	if (track == NULL) return NULL;

	new_event = (MFEvent_t)(malloc(sizeof(struct MFEvent)));
	new_event->track = track;
	new_event->tick = tick;
	new_event->type = MFEVENT_TYPE_KEY_PRESSURE;
	new_event->u.key_pressure.channel = channel;
	new_event->u.key_pressure.note = note;
	new_event->u.key_pressure.amount = amount;
	new_event->should_be_visited = 0;
	add_event(new_event);

	return new_event;
}

MFEvent_t MFTrack_createControlChangeEvent(MFTrack_t track, long tick, int channel, int number, int value)
{
	MFEvent_t new_event;

	if (track == NULL) return NULL;

	new_event = (MFEvent_t)(malloc(sizeof(struct MFEvent)));
	new_event->track = track;
	new_event->tick = tick;
	new_event->type = MFEVENT_TYPE_CONTROL_CHANGE;
	new_event->u.control_change.channel = channel;
	new_event->u.control_change.number = number;
	new_event->u.control_change.value = value;
	new_event->should_be_visited = 0;
	add_event(new_event);

	return new_event;
}

MFEvent_t MFTrack_createProgramChangeEvent(MFTrack_t track, long tick, int channel, int number)
{
	MFEvent_t new_event;

	if (track == NULL) return NULL;

	new_event = (MFEvent_t)(malloc(sizeof(struct MFEvent)));
	new_event->track = track;
	new_event->tick = tick;
	new_event->type = MFEVENT_TYPE_PROGRAM_CHANGE;
	new_event->u.program_change.channel = channel;
	new_event->u.program_change.number = number;
	new_event->should_be_visited = 0;
	add_event(new_event);

	return new_event;
}

MFEvent_t MFTrack_createChannelPressureEvent(MFTrack_t track, long tick, int channel, int amount)
{
	MFEvent_t new_event;

	if (track == NULL) return NULL;

	new_event = (MFEvent_t)(malloc(sizeof(struct MFEvent)));
	new_event->track = track;
	new_event->tick = tick;
	new_event->type = MFEVENT_TYPE_CHANNEL_PRESSURE;
	new_event->u.channel_pressure.channel = channel;
	new_event->u.channel_pressure.amount = amount;
	new_event->should_be_visited = 0;
	add_event(new_event);

	return new_event;
}

MFEvent_t MFTrack_createPitchWheelEvent(MFTrack_t track, long tick, int channel, int value)
{
	MFEvent_t new_event;

	if (track == NULL) return NULL;

	new_event = (MFEvent_t)(malloc(sizeof(struct MFEvent)));
	new_event->track = track;
	new_event->tick = tick;
	new_event->type = MFEVENT_TYPE_PITCH_WHEEL;
	new_event->u.pitch_wheel.channel = channel;
	new_event->u.pitch_wheel.value = value;
	new_event->should_be_visited = 0;
	add_event(new_event);

	return new_event;
}

MFEvent_t MFTrack_createSysexEvent(MFTrack_t track, long tick, int data_length, unsigned char *data_buffer)
{
	MFEvent_t new_event;

	if ((track == NULL) || (data_length < 1) || (data_buffer == NULL)) NULL;

	new_event = (MFEvent_t)(malloc(sizeof(struct MFEvent)));
	new_event->track = track;
	new_event->tick = tick;
	new_event->type = MFEVENT_TYPE_SYSEX;
	new_event->u.sysex.data_length = data_length;
	new_event->u.sysex.data_buffer = malloc(data_length);
	memcpy(new_event->u.sysex.data_buffer, data_buffer, data_length);
	new_event->should_be_visited = 0;
	add_event(new_event);

	return new_event;
}

MFEvent_t MFTrack_createMetaEvent(MFTrack_t track, long tick, int number, int data_length, unsigned char *data_buffer)
{
	MFEvent_t new_event;

	if (track == NULL) return NULL;

	new_event = (MFEvent_t)(malloc(sizeof(struct MFEvent)));
	new_event->track = track;
	new_event->tick = tick;
	new_event->type = MFEVENT_TYPE_META;
	new_event->u.meta.number = number;
	new_event->u.meta.data_length = data_length;
	new_event->u.meta.data_buffer = malloc(data_length);
	memcpy(new_event->u.meta.data_buffer, data_buffer, data_length);
	new_event->should_be_visited = 0;
	add_event(new_event);

	return new_event;
}

MFEvent_t MFTrack_createNoteStartAndEndEvents(MFTrack_t track, long start_tick, long end_tick, int channel, int note, int start_velocity, int end_velocity)
{
	MFEvent_t start_event = MFTrack_createNoteOnEvent(track, start_tick, channel, note, start_velocity);
	MFEvent_t end_event = MFTrack_createNoteOffEvent(track, end_tick, channel, note, end_velocity);
	return start_event;
}

MFEvent_t MFTrack_createTempoEvent(MFTrack_t track, long tick, float tempo)
{
	long midi_tempo = 60000000 / tempo;
	unsigned char buffer[3];
	buffer[0] = (midi_tempo >> 16) & 0xFF;
	buffer[1] = (midi_tempo >> 8) & 0xFF;
	buffer[2] = midi_tempo & 0xFF;
	return MFTrack_createMetaEvent(track, tick, 0x51, 3, buffer);
}

MFEvent_t MFTrack_createVoiceEvent(MFTrack_t track, long tick, unsigned long data)
{
	MFEvent_t new_event;

	if (track == NULL) return NULL;

	new_event = (MFEvent_t)(malloc(sizeof(struct MFEvent)));
	new_event->track = track;
	new_event->tick = tick;
	MFVoiceEvent_setData(new_event, data);
	new_event->should_be_visited = 0;
	add_event(new_event);

	return new_event;
}

MFEvent_t MFTrack_getFirstEvent(MFTrack_t track)
{
	if (track == NULL) return NULL;
	return track->first_event;
}

MFEvent_t MFTrack_getLastEvent(MFTrack_t track)
{
	if (track == NULL) return NULL;
	return track->last_event;
}

int MFTrack_visitEvents(MFTrack_t track, MFEventVisitorCallback_t visitor_callback, void *user_data)
{
	MFEvent_t event, next_event;

	if ((track == NULL) || (visitor_callback == NULL)) return -1;

	for (event = MFTrack_getFirstEvent(track); event != NULL; event = MFEvent_getNextEventInTrack(event)) event->should_be_visited = 1;

	for (event = MFTrack_getFirstEvent(track); event != NULL; event = next_event)
	{
		next_event = MFEvent_getNextEventInTrack(event);

		if (event->should_be_visited)
		{
			event->should_be_visited = 0;
			(*visitor_callback)(event, user_data);
		}
	}

	return 0;
}

int MFEvent_delete(MFEvent_t event)
{
	if (event == NULL) return -1;
	remove_event(event);

	switch (event->type)
	{
		case MFEVENT_TYPE_SYSEX:
		{
			free(event->u.sysex.data_buffer);
			break;
		}
		case MFEVENT_TYPE_META:
		{
			free(event->u.meta.data_buffer);
			break;
		}
	}

	free(event);
	return 0;
}

MFTrack_t MFEvent_getTrack(MFEvent_t event)
{
	if (event == NULL) return NULL;
	return event->track;
}

MFEvent_t MFEvent_getPreviousEvent(MFEvent_t event)
{
	return MFEvent_getPreviousEventInTrack(event);
}

MFEvent_t MFEvent_getNextEvent(MFEvent_t event)
{
	return MFEvent_getNextEventInTrack(event);
}

MFEvent_t MFEvent_getPreviousEventInTrack(MFEvent_t event)
{
	if (event == NULL) return NULL;
	return event->previous_event_in_track;
}

MFEvent_t MFEvent_getNextEventInTrack(MFEvent_t event)
{
	if (event == NULL) return NULL;
	return event->next_event_in_track;
}

MFEvent_t MFEvent_getPreviousEventInFile(MFEvent_t event)
{
	if (event == NULL) return NULL;
	return event->previous_event_in_file;
}

MFEvent_t MFEvent_getNextEventInFile(MFEvent_t event)
{
	if (event == NULL) return NULL;
	return event->next_event_in_file;
}

long MFEvent_getTick(MFEvent_t event)
{
	if (event == NULL) return -1;
	return event->tick;
}

int MFEvent_setTick(MFEvent_t event, long tick)
{
	if (event == NULL) return -1;
	remove_event(event);
	event->tick = tick;
	add_event(event);
	return 0;
}

MFEventType_t MFEvent_getType(MFEvent_t event)
{
	if (event == NULL) return MFEVENT_TYPE_INVALID;
	return event->type;
}

int MFEvent_isNoteStartEvent(MFEvent_t event)
{
	return ((MFEvent_getType(event) == MFEVENT_TYPE_NOTE_ON) && (MFNoteOnEvent_getVelocity(event) > 0));
}

int MFEvent_isNoteEndEvent(MFEvent_t event)
{
	return ((MFEvent_getType(event) == MFEVENT_TYPE_NOTE_OFF) || ((MFEvent_getType(event) == MFEVENT_TYPE_NOTE_ON) && (MFNoteOnEvent_getVelocity(event) == 0)));
}

int MFEvent_isTempoEvent(MFEvent_t event)
{
	return ((MFEvent_getType(event) == MFEVENT_TYPE_META) && (MFMetaEvent_getNumber(event) == 0x51));
}

int MFEvent_isVoiceEvent(MFEvent_t event)
{
	switch (event->type)
	{
		case MFEVENT_TYPE_NOTE_OFF:
		case MFEVENT_TYPE_NOTE_ON:
		case MFEVENT_TYPE_KEY_PRESSURE:
		case MFEVENT_TYPE_CONTROL_CHANGE:
		case MFEVENT_TYPE_PROGRAM_CHANGE:
		case MFEVENT_TYPE_CHANNEL_PRESSURE:
		case MFEVENT_TYPE_PITCH_WHEEL:
		{
			return 1;
		}
		default:
		{
			return 0;
		}
	}
}

int MFNoteOffEvent_getChannel(MFEvent_t event)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_NOTE_OFF)) return -1;
	return event->u.note_off.channel;
}

int MFNoteOffEvent_setChannel(MFEvent_t event, int channel)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_NOTE_OFF)) return -1;
	event->u.note_off.channel = channel;
	return 0;
}

int MFNoteOffEvent_getNote(MFEvent_t event)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_NOTE_OFF)) return -1;
	return event->u.note_off.note;
}

int MFNoteOffEvent_setNote(MFEvent_t event, int note)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_NOTE_OFF)) return -1;
	event->u.note_off.note = note;
	return 0;
}

int MFNoteOffEvent_getVelocity(MFEvent_t event)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_NOTE_OFF)) return -1;
	return event->u.note_off.velocity;
}

int MFNoteOffEvent_setVelocity(MFEvent_t event, int velocity)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_NOTE_OFF)) return -1;
	event->u.note_off.velocity = velocity;
	return 0;
}

int MFNoteOnEvent_getChannel(MFEvent_t event)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_NOTE_ON)) return -1;
	return event->u.note_on.channel;
}

int MFNoteOnEvent_setChannel(MFEvent_t event, int channel)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_NOTE_ON)) return -1;
	event->u.note_on.channel = channel;
	return 0;
}

int MFNoteOnEvent_getNote(MFEvent_t event)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_NOTE_ON)) return -1;
	return event->u.note_on.note;
}

int MFNoteOnEvent_setNote(MFEvent_t event, int note)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_NOTE_ON)) return -1;
	event->u.note_on.note = note;
	return 0;
}

int MFNoteOnEvent_getVelocity(MFEvent_t event)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_NOTE_ON)) return -1;
	return event->u.note_on.velocity;
}

int MFNoteOnEvent_setVelocity(MFEvent_t event, int velocity)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_NOTE_ON)) return -1;
	event->u.note_on.velocity = velocity;
	return 0;
}

int MFKeyPressureEvent_getChannel(MFEvent_t event)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_KEY_PRESSURE)) return -1;
	return event->u.key_pressure.channel;
}

int MFKeyPressureEvent_setChannel(MFEvent_t event, int channel)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_KEY_PRESSURE)) return -1;
	event->u.key_pressure.channel = channel;
	return 0;
}

int MFKeyPressureEvent_getNote(MFEvent_t event)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_KEY_PRESSURE)) return -1;
	return event->u.key_pressure.note;
}

int MFKeyPressureEvent_setNote(MFEvent_t event, int note)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_KEY_PRESSURE)) return -1;
	event->u.key_pressure.note = note;
	return 0;
}

int MFKeyPressureEvent_getAmount(MFEvent_t event)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_KEY_PRESSURE)) return -1;
	return event->u.key_pressure.amount;
}

int MFKeyPressureEvent_setAmount(MFEvent_t event, int amount)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_KEY_PRESSURE)) return -1;
	event->u.key_pressure.amount = amount;
	return 0;
}

int MFControlChangeEvent_getChannel(MFEvent_t event)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_CONTROL_CHANGE)) return -1;
	return event->u.control_change.channel;
}

int MFControlChangeEvent_setChannel(MFEvent_t event, int channel)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_CONTROL_CHANGE)) return -1;
	event->u.control_change.channel = channel;
	return 0;
}

int MFControlChangeEvent_getNumber(MFEvent_t event)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_CONTROL_CHANGE)) return -1;
	return event->u.control_change.number;
}

int MFControlChangeEvent_setNumber(MFEvent_t event, int number)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_CONTROL_CHANGE)) return -1;
	event->u.control_change.number = number;
	return 0;
}

int MFControlChangeEvent_getValue(MFEvent_t event)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_CONTROL_CHANGE)) return -1;
	return event->u.control_change.value;
}

int MFControlChangeEvent_setValue(MFEvent_t event, int value)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_CONTROL_CHANGE)) return -1;
	event->u.control_change.value = value;
	return 0;
}

int MFProgramChangeEvent_getChannel(MFEvent_t event)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_PROGRAM_CHANGE)) return -1;
	return event->u.program_change.channel;
}

int MFProgramChangeEvent_setChannel(MFEvent_t event, int channel)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_PROGRAM_CHANGE)) return -1;
	event->u.program_change.channel = channel;
	return 0;
}

int MFProgramChangeEvent_getNumber(MFEvent_t event)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_PROGRAM_CHANGE)) return -1;
	return event->u.program_change.number;
}

int MFProgramChangeEvent_setNumber(MFEvent_t event, int number)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_PROGRAM_CHANGE)) return -1;
	event->u.program_change.number = number;
	return 0;
}

int MFChannelPressureEvent_getChannel(MFEvent_t event)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_CHANNEL_PRESSURE)) return -1;
	return event->u.channel_pressure.channel;
}

int MFChannelPressureEvent_setChannel(MFEvent_t event, int channel)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_CHANNEL_PRESSURE)) return -1;
	event->u.channel_pressure.channel = channel;
	return 0;
}

int MFChannelPressureEvent_getAmount(MFEvent_t event)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_CHANNEL_PRESSURE)) return -1;
	return event->u.channel_pressure.amount;
}

int MFChannelPressureEvent_setAmount(MFEvent_t event, int amount)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_CHANNEL_PRESSURE)) return -1;
	event->u.channel_pressure.amount = amount;
	return 0;
}

int MFPitchWheelEvent_getChannel(MFEvent_t event)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_PITCH_WHEEL)) return -1;
	return event->u.pitch_wheel.channel;
}

int MFPitchWheelEvent_setChannel(MFEvent_t event, int channel)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_PITCH_WHEEL)) return -1;
	event->u.pitch_wheel.channel = channel;
	return 0;
}

int MFPitchWheelEvent_getValue(MFEvent_t event)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_PITCH_WHEEL)) return -1;
	return event->u.pitch_wheel.value;
}

int MFPitchWheelEvent_setValue(MFEvent_t event, int value)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_PITCH_WHEEL)) return -1;
	event->u.pitch_wheel.value = value;
	return 0;
}

int MFSysexEvent_getDataLength(MFEvent_t event)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_SYSEX)) return -1;
	return event->u.sysex.data_length;
}

unsigned char *MFSysexEvent_getData(MFEvent_t event)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_SYSEX)) return NULL;
	return event->u.sysex.data_buffer;
}

int MFSysexEvent_setData(MFEvent_t event, int data_length, unsigned char *data_buffer)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_SYSEX) || (data_length < 1) || (data_buffer == NULL)) return -1;
	free(event->u.sysex.data_buffer);
	event->u.sysex.data_length = data_length;
	event->u.sysex.data_buffer = malloc(data_length);
	memcpy(event->u.sysex.data_buffer, data_buffer, data_length);
	return 0;
}

int MFMetaEvent_getNumber(MFEvent_t event)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_META)) return -1;
	return event->u.meta.number;
}

int MFMetaEvent_setNumber(MFEvent_t event, int number)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_META)) return -1;
	event->u.meta.number = number;
	return 0;
}

int MFMetaEvent_getDataLength(MFEvent_t event)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_META)) return -1;
	return event->u.meta.data_length;
}

unsigned char *MFMetaEvent_getData(MFEvent_t event)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_META)) return NULL;
	return event->u.meta.data_buffer;
}

int MFMetaEvent_setData(MFEvent_t event, int data_length, unsigned char *data_buffer)
{
	if ((event == NULL) || (event->type != MFEVENT_TYPE_META) || (data_length < 1) || (data_buffer == NULL)) return -1;
	free(event->u.meta.data_buffer);
	event->u.meta.data_length = data_length;
	event->u.meta.data_buffer = malloc(data_length);
	memcpy(event->u.meta.data_buffer, data_buffer, data_length);
	return 0;
}

int MFNoteStartEvent_getChannel(MFEvent_t event)
{
	if (! MFEvent_isNoteStartEvent(event)) return -1;
	return MFNoteOnEvent_getChannel(event);
}

int MFNoteStartEvent_setChannel(MFEvent_t event, int channel)
{
	if (! MFEvent_isNoteStartEvent(event)) return -1;
	return MFNoteOnEvent_setChannel(event, channel);
}

int MFNoteStartEvent_getNote(MFEvent_t event)
{
	if (! MFEvent_isNoteStartEvent(event)) return -1;
	return MFNoteOnEvent_getNote(event);
}

int MFNoteStartEvent_setNote(MFEvent_t event, int note)
{
	if (! MFEvent_isNoteStartEvent(event)) return -1;
	return MFNoteOnEvent_setNote(event, note);
}

int MFNoteStartEvent_getVelocity(MFEvent_t event)
{
	if (! MFEvent_isNoteStartEvent(event)) return -1;
	return MFNoteOnEvent_getVelocity(event);
}

int MFNoteStartEvent_setVelocity(MFEvent_t event, int velocity)
{
	if (! MFEvent_isNoteStartEvent(event)) return -1;
	return MFNoteOnEvent_setVelocity(event, velocity);
}

MFEvent_t MFNoteStartEvent_getNoteEndEvent(MFEvent_t event)
{
	MFEvent_t subsequent_event;

	if (! MFEvent_isNoteStartEvent(event)) return NULL;

	for (subsequent_event = MFEvent_getNextEventInTrack(event); subsequent_event != NULL; subsequent_event = MFEvent_getNextEventInTrack(subsequent_event))
	{
		if (MFEvent_isNoteEndEvent(subsequent_event) && (MFNoteEndEvent_getChannel(subsequent_event) == MFNoteStartEvent_getChannel(event)) && (MFNoteEndEvent_getNote(subsequent_event) == MFNoteStartEvent_getNote(event)))
		{
			return subsequent_event;
		}
	}

	return NULL;
}

int MFNoteEndEvent_getChannel(MFEvent_t event)
{
	if (! MFEvent_isNoteEndEvent(event)) return -1;

	switch (MFEvent_getType(event))
	{
		case MFEVENT_TYPE_NOTE_ON:
		{
			return MFNoteOnEvent_getChannel(event);
		}
		case MFEVENT_TYPE_NOTE_OFF:
		{
			return MFNoteOffEvent_getChannel(event);
		}
		default:
		{
			return -1;
		}
	}
}

int MFNoteEndEvent_setChannel(MFEvent_t event, int channel)
{
	if (! MFEvent_isNoteEndEvent(event)) return -1;

	switch (MFEvent_getType(event))
	{
		case MFEVENT_TYPE_NOTE_ON:
		{
			return MFNoteOnEvent_setChannel(event, channel);
		}
		case MFEVENT_TYPE_NOTE_OFF:
		{
			return MFNoteOffEvent_setChannel(event, channel);
		}
		default:
		{
			return -1;
		}
	}
}

int MFNoteEndEvent_getNote(MFEvent_t event)
{
	if (! MFEvent_isNoteEndEvent(event)) return -1;

	switch (MFEvent_getType(event))
	{
		case MFEVENT_TYPE_NOTE_ON:
		{
			return MFNoteOnEvent_getNote(event);
		}
		case MFEVENT_TYPE_NOTE_OFF:
		{
			return MFNoteOffEvent_getNote(event);
		}
		default:
		{
			return -1;
		}
	}
}

int MFNoteEndEvent_setNote(MFEvent_t event, int note)
{
	if (! MFEvent_isNoteEndEvent(event)) return -1;

	switch (MFEvent_getType(event))
	{
		case MFEVENT_TYPE_NOTE_ON:
		{
			return MFNoteOnEvent_setNote(event, note);
		}
		case MFEVENT_TYPE_NOTE_OFF:
		{
			return MFNoteOffEvent_setNote(event, note);
		}
		default:
		{
			return -1;
		}
	}
}

int MFNoteEndEvent_getVelocity(MFEvent_t event)
{
	if (! MFEvent_isNoteEndEvent(event)) return -1;

	switch (MFEvent_getType(event))
	{
		case MFEVENT_TYPE_NOTE_OFF:
		{
			return MFNoteOffEvent_getVelocity(event);
		}
		case MFEVENT_TYPE_NOTE_ON:
		{
			return 0;
		}
		default:
		{
			return -1;
		}
	}
}

int MFNoteEndEvent_setVelocity(MFEvent_t event, int velocity)
{
	if (! MFEvent_isNoteEndEvent(event)) return -1;

	switch (MFEvent_getType(event))
	{
		case MFEVENT_TYPE_NOTE_OFF:
		{
			return MFNoteOffEvent_setVelocity(event, velocity);
		}
		case MFEVENT_TYPE_NOTE_ON:
		{
			MFTrack_createNoteOffEvent(MFEvent_getTrack(event), MFEvent_getTick(event), MFNoteOnEvent_getChannel(event), MFNoteOnEvent_getNote(event), velocity);
			MFEvent_delete(event);
			return 0;
		}
		default:
		{
			return -1;
		}
	}
}

MFEvent_t MFNoteEndEvent_getNoteStartEvent(MFEvent_t event)
{
	MFEvent_t preceding_event;

	if (! MFEvent_isNoteEndEvent(event)) return NULL;

	for (preceding_event = MFEvent_getPreviousEventInTrack(event); preceding_event != NULL; preceding_event = MFEvent_getPreviousEventInTrack(preceding_event))
	{
		if (MFEvent_isNoteStartEvent(preceding_event) && (MFNoteStartEvent_getChannel(preceding_event) == MFNoteEndEvent_getChannel(event)) && (MFNoteStartEvent_getNote(preceding_event) == MFNoteEndEvent_getNote(event)))
		{
			return preceding_event;
		}
	}

	return NULL;
}

float MFTempoEvent_getTempo(MFEvent_t event)
{
	unsigned char *buffer;
	long midi_tempo;

	if (! MFEvent_isTempoEvent(event)) return -1;

	buffer = MFMetaEvent_getData(event);
	midi_tempo = (buffer[0] << 16) | (buffer[1] << 8) | buffer[2];
	return 60000000.0 / midi_tempo;
}

int MFTempoEvent_setTempo(MFEvent_t event, float tempo)
{
	long midi_tempo;
	unsigned char buffer[3];

	if (! MFEvent_isTempoEvent(event)) return -1;

	midi_tempo = 60000000 / tempo;
	buffer[0] = (midi_tempo >> 16) & 0xFF;
	buffer[1] = (midi_tempo >> 8) & 0xFF;
	buffer[2] = midi_tempo & 0xFF;
	return MFMetaEvent_setData(event, 3, buffer);
}

unsigned long MFVoiceEvent_getData(MFEvent_t event)
{
	switch (MFEvent_getType(event))
	{
		case MFEVENT_TYPE_NOTE_OFF:
		{
			union
			{
				unsigned char data_as_bytes[4];
				unsigned long data_as_uint32;
			}
			u;

			u.data_as_bytes[0] = 0x80 | MFNoteOffEvent_getChannel(event);
			u.data_as_bytes[1] = MFNoteOffEvent_getNote(event);
			u.data_as_bytes[2] = MFNoteOffEvent_getVelocity(event);
			u.data_as_bytes[3] = 0;
			return u.data_as_uint32;
		}
		case MFEVENT_TYPE_NOTE_ON:
		{
			union
			{
				unsigned char data_as_bytes[4];
				unsigned long data_as_uint32;
			}
			u;

			u.data_as_bytes[0] = 0x90 | MFNoteOnEvent_getChannel(event);
			u.data_as_bytes[1] = MFNoteOnEvent_getNote(event);
			u.data_as_bytes[2] = MFNoteOnEvent_getVelocity(event);
			u.data_as_bytes[3] = 0;
			return u.data_as_uint32;
		}
		case MFEVENT_TYPE_KEY_PRESSURE:
		{
			union
			{
				unsigned char data_as_bytes[4];
				unsigned long data_as_uint32;
			}
			u;

			u.data_as_bytes[0] = 0xA0 | MFKeyPressureEvent_getChannel(event);
			u.data_as_bytes[1] = MFKeyPressureEvent_getNote(event);
			u.data_as_bytes[2] = MFKeyPressureEvent_getAmount(event);
			u.data_as_bytes[3] = 0;
			return u.data_as_uint32;
		}
		case MFEVENT_TYPE_CONTROL_CHANGE:
		{
			union
			{
				unsigned char data_as_bytes[4];
				unsigned long data_as_uint32;
			}
			u;

			u.data_as_bytes[0] = 0xB0 | MFControlChangeEvent_getChannel(event);
			u.data_as_bytes[1] = MFControlChangeEvent_getNumber(event);
			u.data_as_bytes[2] = MFControlChangeEvent_getValue(event);
			u.data_as_bytes[3] = 0;
			return u.data_as_uint32;
		}
		case MFEVENT_TYPE_PROGRAM_CHANGE:
		{
			union
			{
				unsigned char data_as_bytes[4];
				unsigned long data_as_uint32;
			}
			u;

			u.data_as_bytes[0] = 0xC0 | MFProgramChangeEvent_getChannel(event);
			u.data_as_bytes[1] = MFProgramChangeEvent_getNumber(event);
			u.data_as_bytes[2] = 0;
			u.data_as_bytes[3] = 0;
			return u.data_as_uint32;
		}
		case MFEVENT_TYPE_CHANNEL_PRESSURE:
		{
			union
			{
				unsigned char data_as_bytes[4];
				unsigned long data_as_uint32;
			}
			u;

			u.data_as_bytes[0] = 0xD0 | MFChannelPressureEvent_getChannel(event);
			u.data_as_bytes[1] = MFChannelPressureEvent_getAmount(event);
			u.data_as_bytes[2] = 0;
			u.data_as_bytes[3] = 0;
			return u.data_as_uint32;
		}
		case MFEVENT_TYPE_PITCH_WHEEL:
		{
			union
			{
				unsigned char data_as_bytes[4];
				unsigned long data_as_uint32;
			}
			u;

			u.data_as_bytes[0] = 0xE0 | MFPitchWheelEvent_getChannel(event);
			u.data_as_bytes[1] = MFPitchWheelEvent_getValue(event) >> 7;
			u.data_as_bytes[2] = MFPitchWheelEvent_getValue(event) & 0x0F;
			u.data_as_bytes[3] = 0;
			return u.data_as_uint32;
		}
		default:
		{
			return 0;
		}
	}
}

int MFVoiceEvent_setData(MFEvent_t event, unsigned long data)
{
	union
	{
		unsigned long data_as_uint32;
		unsigned char data_as_bytes[4];
	}
	u;

	if (event == NULL) return -1;

	u.data_as_uint32 = data;

	switch (u.data_as_bytes[0] & 0xF0)
	{
		case 0x80:
		{
			event->type = MFEVENT_TYPE_NOTE_OFF;
			event->u.note_off.channel = u.data_as_bytes[0] & 0x0F;
			event->u.note_off.note = u.data_as_bytes[1];
			event->u.note_off.velocity = u.data_as_bytes[2];
			return 0;
		}
		case 0x90:
		{
			event->type = MFEVENT_TYPE_NOTE_ON;
			event->u.note_on.channel = u.data_as_bytes[0] & 0x0F;
			event->u.note_on.note = u.data_as_bytes[1];
			event->u.note_on.velocity = u.data_as_bytes[2];
			return 0;
		}
		case 0xA0:
		{
			event->type = MFEVENT_TYPE_KEY_PRESSURE;
			event->u.key_pressure.channel = u.data_as_bytes[0] & 0x0F;
			event->u.key_pressure.note = u.data_as_bytes[1];
			event->u.key_pressure.amount = u.data_as_bytes[2];
			return 0;
		}
		case 0xB0:
		{
			event->type = MFEVENT_TYPE_CONTROL_CHANGE;
			event->u.control_change.channel = u.data_as_bytes[0] & 0x0F;
			event->u.control_change.number = u.data_as_bytes[1];
			event->u.control_change.value = u.data_as_bytes[2];
			return 0;
		}
		case 0xC0:
		{
			event->type = MFEVENT_TYPE_PROGRAM_CHANGE;
			event->u.program_change.channel = u.data_as_bytes[0] & 0x0F;
			event->u.program_change.number = u.data_as_bytes[1];
			return 0;
		}
		case 0xD0:
		{
			event->type = MFEVENT_TYPE_CHANNEL_PRESSURE;
			event->u.channel_pressure.channel = u.data_as_bytes[0] & 0x0F;
			event->u.channel_pressure.amount = u.data_as_bytes[1];
			return 0;
		}
		case 0xE0:
		{
			event->type = MFEVENT_TYPE_PITCH_WHEEL;
			event->u.pitch_wheel.channel = u.data_as_bytes[0] & 0x0F;
			event->u.pitch_wheel.value = (u.data_as_bytes[1] << 7) | u.data_as_bytes[2];
			return 0;
		}
		default:
		{
			return -1;
		}
	}
}

