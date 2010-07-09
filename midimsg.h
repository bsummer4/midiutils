#define mm_bendvalue(msb, lsb) ((msb<<7)+lsb)
struct mm_msg { unsigned char type, chan, arg1, arg2; };
enum {
	MM_NOTEOFF=0, MM_NOTEON, MM_KEYPRESS, MM_CNTL, MM_PROG,
	MM_CHANPRESS, MM_PITCHWHEEL, MM_SYSEX=0xF0, MM_SONGPOS=0xF2,
	MM_SONGSEL, MM_TUNE=0xF6, MM_ENDSYSEX, MM_CLOCK,
	MM_START=0xFA, MM_CONT, MM_STOP, MM_ASENSE=0xFE, MM_RESET };

// These are *NOT* reentrant
int mm_inject (unsigned char b, struct mm_msg *);
int mm_read (int fd, struct mm_msg *);
void mm_write (int fd, struct mm_msg *);
double mm_notefreq (unsigned char code);
