/*
# Div's Standard MIDI File API

    Copyright 2003-2006 by David G. Slomin
    Provided under the terms of the BSD license

## Usage Notes
1.  Running status is eliminated from messages at load time; it should not
    be used at any other time.

2.  MIDI files in formats 0, 1, and 2 are supported, but the caller is
    responsible for placing events into the appropriate tracks.  Format
    0 files should only use a single track.  Format 1 files use their
    first track as a "conductor track" for meta events like tempo and 
    meter changes.

3.  MF_visitEvents() and MFTrack_visitEvents() are specially
    designed so that you can add, delete, or change the tick of events
    (thereby modifying the sorting order) without upsetting the iterator.

4.  Any data passed into these functions is memory-managed by the caller.
    Any data returned from these functions is memory-managed by the API.
    Don't forget to call MF_free().

5.  This API is not thread-safe.

6.  You can navigate through events one track at a time, or through all
    tracks at once in an interwoven, time-sorted manner.

7.  Because a note on event with a velocity of zero is functionally
    equivalent to a note off event, you cannot simply look at the type of 
    an event to see whether it signifies the start or the end of a note.
    To handle this problem, convenience wrappers are provided for pseudo 
    "note start" and "note end" events.

8.  Convenience functions are provided for working with tempo and
    absolute time in files of format 1 or 0.  Tempo events (a particular
    kind of meta event) are only meaningful when using the PPQ division
    type.

9.  Events other than sysex and meta are considered "voice events".  For 
    interaction with other APIs, it is sometimes useful to pack their 
    messages into 32 bit integers.

10. All numbers in this API are zero-based, to correspond with the actual
    byte values of the MIDI protocol, rather than one-based, as they are
    commonly displayed to the user.  Channels range from 0 to 15, notes
    range from 0 to 127, etc.
*/

typedef struct MF *MF_t;
typedef struct MFTrack *MFTrack_t;
typedef struct MFEvent *MFEvent_t;
typedef void (*MFEventVisitorCallback_t)(MFEvent_t event, void *user_data);

typedef enum {
	MFDIVISION_TYPE_INVALID = -1,
	MFDIVISION_TYPE_PPQ,
	MFDIVISION_TYPE_SMPTE24,
	MFDIVISION_TYPE_SMPTE25,
	MFDIVISION_TYPE_SMPTE30DROP,
	MFDIVISION_TYPE_SMPTE30
} MFDivisionType_t;

typedef enum {
	MFEVENT_TYPE_INVALID = -1,
	MFEVENT_TYPE_NOTE_OFF,
	MFEVENT_TYPE_NOTE_ON,
	MFEVENT_TYPE_KEY_PRESSURE,
	MFEVENT_TYPE_CONTROL_CHANGE,
	MFEVENT_TYPE_PROGRAM_CHANGE,
	MFEVENT_TYPE_CHANNEL_PRESSURE,
	MFEVENT_TYPE_PITCH_WHEEL,
	MFEVENT_TYPE_SYSEX,
	MFEVENT_TYPE_META
} MFEventType_t;

MF_t MF_load(char *filename);
int MF_save(MF_t midi_file, const char* filename);

MF_t MF_new(int file_format, MFDivisionType_t division_type, int resolution);
int MF_free(MF_t midi_file);
int MF_getFileFormat(MF_t midi_file);
int MF_setFileFormat(MF_t midi_file, int file_format);
MFDivisionType_t MF_getDivisionType(MF_t midi_file);
int MF_setDivisionType(MF_t midi_file, MFDivisionType_t division_type);
int MF_getResolution(MF_t midi_file);
int MF_setResolution(MF_t midi_file, int resolution);
MFTrack_t MF_createTrack(MF_t midi_file);
int MF_getNumberOfTracks(MF_t midi_file);
MFTrack_t MF_getTrackByNumber(MF_t midi_file, int number, int create);
MFTrack_t MF_getFirstTrack(MF_t midi_file);
MFTrack_t MF_getLastTrack(MF_t midi_file);
MFEvent_t MF_getFirstEvent(MF_t midi_file);
MFEvent_t MF_getLastEvent(MF_t midi_file);
int MF_visitEvents(MF_t midi_file, MFEventVisitorCallback_t visitor_callback, void *user_data);
float MF_getTimeFromTick(MF_t midi_file, long tick); /* time is in seconds */
long MF_getTickFromTime(MF_t midi_file, float time);
float MF_getBeatFromTick(MF_t midi_file, long tick);
long MF_getTickFromBeat(MF_t midi_file, float beat);

int MFTrack_delete(MFTrack_t track);
MF_t MFTrack_getMF(MFTrack_t track);
int MFTrack_getNumber(MFTrack_t track);
long MFTrack_getEndTick(MFTrack_t track);
int MFTrack_setEndTick(MFTrack_t track, long end_tick);
MFTrack_t MFTrack_createTrackBefore(MFTrack_t track);
MFTrack_t MFTrack_getPreviousTrack(MFTrack_t track);
MFTrack_t MFTrack_getNextTrack(MFTrack_t track);
MFEvent_t MFTrack_createNoteOffEvent(MFTrack_t track, long tick, int channel, int note, int velocity);
MFEvent_t MFTrack_createNoteOnEvent(MFTrack_t track, long tick, int channel, int note, int velocity);
MFEvent_t MFTrack_createKeyPressureEvent(MFTrack_t track, long tick, int channel, int note, int amount);
MFEvent_t MFTrack_createControlChangeEvent(MFTrack_t track, long tick, int channel, int number, int value);
MFEvent_t MFTrack_createProgramChangeEvent(MFTrack_t track, long tick, int channel, int number);
MFEvent_t MFTrack_createChannelPressureEvent(MFTrack_t track, long tick, int channel, int amount);
MFEvent_t MFTrack_createPitchWheelEvent(MFTrack_t track, long tick, int channel, int value);
MFEvent_t MFTrack_createSysexEvent(MFTrack_t track, long tick, int data_length, unsigned char *data_buffer);
MFEvent_t MFTrack_createMetaEvent(MFTrack_t track, long tick, int number, int data_length, unsigned char *data_buffer);
MFEvent_t MFTrack_createNoteStartAndEndEvents(MFTrack_t track, long start_tick, long end_tick, int channel, int note, int start_velocity, int end_velocity); /* returns the start event */
MFEvent_t MFTrack_createTempoEvent(MFTrack_t track, long tick, float tempo); /* tempo is in BPM */
MFEvent_t MFTrack_createVoiceEvent(MFTrack_t track, long tick, unsigned long data);
MFEvent_t MFTrack_getFirstEvent(MFTrack_t track);
MFEvent_t MFTrack_getLastEvent(MFTrack_t track);
int MFTrack_visitEvents(MFTrack_t track, MFEventVisitorCallback_t visitor_callback, void *user_data);

int MFEvent_delete(MFEvent_t event);
MFTrack_t MFEvent_getTrack(MFEvent_t event);
MFEvent_t MFEvent_getPreviousEvent(MFEvent_t event); /* deprecated:  use MFEvent_getPreviousEventInTrack() */
MFEvent_t MFEvent_getNextEvent(MFEvent_t event); /* deprecated:  use MFEvent_getNextEventInTrack() */
MFEvent_t MFEvent_getPreviousEventInTrack(MFEvent_t event);
MFEvent_t MFEvent_getNextEventInTrack(MFEvent_t event);
MFEvent_t MFEvent_getPreviousEventInFile(MFEvent_t event);
MFEvent_t MFEvent_getNextEventInFile(MFEvent_t event);
long MFEvent_getTick(MFEvent_t event);
int MFEvent_setTick(MFEvent_t event, long tick);
MFEventType_t MFEvent_getType(MFEvent_t event);
int MFEvent_isNoteStartEvent(MFEvent_t event);
int MFEvent_isNoteEndEvent(MFEvent_t event);
int MFEvent_isTempoEvent(MFEvent_t event);
int MFEvent_isVoiceEvent(MFEvent_t event);

int MFNoteOffEvent_getChannel(MFEvent_t event);
int MFNoteOffEvent_setChannel(MFEvent_t event, int channel);
int MFNoteOffEvent_getNote(MFEvent_t event);
int MFNoteOffEvent_setNote(MFEvent_t event, int note);
int MFNoteOffEvent_getVelocity(MFEvent_t event);
int MFNoteOffEvent_setVelocity(MFEvent_t event, int velocity);

int MFNoteOnEvent_getChannel(MFEvent_t event);
int MFNoteOnEvent_setChannel(MFEvent_t event, int channel);
int MFNoteOnEvent_getNote(MFEvent_t event);
int MFNoteOnEvent_setNote(MFEvent_t event, int note);
int MFNoteOnEvent_getVelocity(MFEvent_t event);
int MFNoteOnEvent_setVelocity(MFEvent_t event, int velocity);

int MFKeyPressureEvent_getChannel(MFEvent_t event);
int MFKeyPressureEvent_setChannel(MFEvent_t event, int channel);
int MFKeyPressureEvent_getNote(MFEvent_t event);
int MFKeyPressureEvent_setNote(MFEvent_t event, int note);
int MFKeyPressureEvent_getAmount(MFEvent_t event);
int MFKeyPressureEvent_setAmount(MFEvent_t event, int amount);

int MFControlChangeEvent_getChannel(MFEvent_t event);
int MFControlChangeEvent_setChannel(MFEvent_t event, int channel);
int MFControlChangeEvent_getNumber(MFEvent_t event);
int MFControlChangeEvent_setNumber(MFEvent_t event, int number);
int MFControlChangeEvent_getValue(MFEvent_t event);
int MFControlChangeEvent_setValue(MFEvent_t event, int value);

int MFProgramChangeEvent_getChannel(MFEvent_t event);
int MFProgramChangeEvent_setChannel(MFEvent_t event, int channel);
int MFProgramChangeEvent_getNumber(MFEvent_t event);
int MFProgramChangeEvent_setNumber(MFEvent_t event, int number);

int MFChannelPressureEvent_getChannel(MFEvent_t event);
int MFChannelPressureEvent_setChannel(MFEvent_t event, int channel);
int MFChannelPressureEvent_getAmount(MFEvent_t event);
int MFChannelPressureEvent_setAmount(MFEvent_t event, int amount);

int MFPitchWheelEvent_getChannel(MFEvent_t event);
int MFPitchWheelEvent_setChannel(MFEvent_t event, int channel);
int MFPitchWheelEvent_getValue(MFEvent_t event);
int MFPitchWheelEvent_setValue(MFEvent_t event, int value);

int MFSysexEvent_getDataLength(MFEvent_t event);
unsigned char *MFSysexEvent_getData(MFEvent_t event);
int MFSysexEvent_setData(MFEvent_t event, int data_length, unsigned char *data_buffer);

int MFMetaEvent_getNumber(MFEvent_t event);
int MFMetaEvent_setNumber(MFEvent_t event, int number);
int MFMetaEvent_getDataLength(MFEvent_t event);
unsigned char *MFMetaEvent_getData(MFEvent_t event);
int MFMetaEvent_setData(MFEvent_t event, int data_length, unsigned char *data_buffer);

int MFNoteStartEvent_getChannel(MFEvent_t event);
int MFNoteStartEvent_setChannel(MFEvent_t event, int channel);
int MFNoteStartEvent_getNote(MFEvent_t event);
int MFNoteStartEvent_setNote(MFEvent_t event, int note);
int MFNoteStartEvent_getVelocity(MFEvent_t event);
int MFNoteStartEvent_setVelocity(MFEvent_t event, int velocity);
MFEvent_t MFNoteStartEvent_getNoteEndEvent(MFEvent_t event);

int MFNoteEndEvent_getChannel(MFEvent_t event);
int MFNoteEndEvent_setChannel(MFEvent_t event, int channel);
int MFNoteEndEvent_getNote(MFEvent_t event);
int MFNoteEndEvent_setNote(MFEvent_t event, int note);
int MFNoteEndEvent_getVelocity(MFEvent_t event);
/* caution:  will replace a note on event with a note off */
int MFNoteEndEvent_setVelocity(MFEvent_t event, int velocity);
MFEvent_t MFNoteEndEvent_getNoteStartEvent(MFEvent_t event);

float MFTempoEvent_getTempo(MFEvent_t event);
int MFTempoEvent_setTempo(MFEvent_t event, float tempo);

unsigned long MFVoiceEvent_getData(MFEvent_t event);
int MFVoiceEvent_setData(MFEvent_t event, unsigned long data);
