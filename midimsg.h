// This file is not meant to be included from other header files.

typedef unsigned char byte;

#define MM_NOTEOFF 0x80
#define MM_NOTEON 0x90
#define MM_KEYPRESS 0xa0
#define MM_PARAM 0xb0
#define MM_PROG 0xc0
#define MM_CHANPRESS 0xd0
#define MM_PITCHWHEEL 0xe0

#define MSGTYPE 0
#define PITCH 1
#define VELOCITY 2
#define PARAMID 1
#define PARAMVALUE 2

bool mm_read (int fd, byte *out);
void mm_write (int fd, byte *out);

int mm_chan (byte *msg);
int mm_msgtype (byte *msg);
int mm_parameternumber (byte *msg);
int mm_parametervalue (byte *msg);
int mm_pitch (byte *msg);
int mm_pitchwheel (byte *msg);
int mm_velocity (byte *msg);
void mm_setchan (byte *msg, int chan);
void mm_setmsgtype (byte *msg, int type);
void mm_setparamnumber (byte *msg, int number);
void mm_setparamvalue (byte *msg, int value);
void mm_setpitch (byte *msg, int pitch);
void mm_setvel (byte *msg, int vel);
