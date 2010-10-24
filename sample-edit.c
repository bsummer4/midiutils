/*
	# sample-edit
	- TODO Description of this program should go here.

	## Note
	For now, the sample size is fixed to 1 unsigned byte, and the
	samplerate is fixed to 44100.  This should certainly be an option
	later.

	## TODO
	- TODO Better Mouse Control
	- TODO mmap 'samples' from a files named '1' in the current directoy.
	- TODO Switch between samples with the keyboard.
	- TODO Display the current sample number in a corner of the screen.
	- TODO Dynamically change sample size?
*/


#include <X11/Xlib.h>
#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>

#define SI static inline


static int black, white;
static int width=100, height=30;
static Window w;
static GC gc;
static Display *dpy;
typedef unsigned char byte;
#define NSAMPLES 512
int nsamples = NSAMPLES;
static byte tmpsamples[NSAMPLES] = {0};
static byte *samples = tmpsamples;

SI size_t filelen (int fd) { struct stat s; fstat(fd, &s); return s.st_size; }
static void opensamples () {
	int fd = open("1", O_RDWR);
	if (fd < 0) return;
	int ns = filelen(fd);
   byte *s = mmap(NULL, nsamples, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (!s) return;
	puts("opened"),samples=s,nsamples=ns; }

static void init ()
{	dpy = XOpenDisplay(NULL);
	black = BlackPixel(dpy, DefaultScreen(dpy));
	white = WhitePixel(dpy, DefaultScreen(dpy));
	w = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 
	                        width, height, 0, black, black);
	XSelectInput(dpy, w, StructureNotifyMask | ExposureMask |
	                     Button1MotionMask | ButtonPressMask);
	XMapWindow(dpy, w);
	gc = XCreateGC(dpy, w, 0, NULL);
	XSetForeground(dpy, gc, white);
	assert(dpy); }

typedef struct point { int x, y; } P;
double ddiv (double x, double y) { return x/y; }
static P sample2screen (P p) {
	double xfrac = ddiv(p.x, nsamples-1);
	double yfrac = ddiv(p.y, 255);
	return (P){xfrac*width, height-(yfrac*height)}; }

// TODO Centerify
static P screen2sample (P p) {
	double xfrac = ddiv(p.x, width);
	double yfrac = ddiv(height-p.y, height);
	return (P){xfrac*(nsamples-1), (yfrac*255)}; }

static void draw ()
{	XClearWindow(dpy, w);
	for (int ii=0, jj=1; jj<nsamples; jj++,ii++) {
		P p1 = sample2screen((P){ii, samples[ii]});
		P p2 = sample2screen((P){jj, samples[jj]});
		XDrawLine(dpy, w, gc, p1.x, p1.y, p2.x, p2.y); }
	XFlush(dpy); }

// TODO Draw a proper line between the two points
static void change (P p1, P p2)
{	p1 = screen2sample(p1);
	p2 = screen2sample(p2);
	int avg = (p1.y+p2.y)/2;
	int from = p1.x < p2.x ? p1.x : p2.x;
	int to = p1.x > p2.x ? p1.x : p2.x;
	for (int ii=from; ii<to; ii++)
		samples[ii] = avg; }

int main()
{	init();
	opensamples();
	for(;;)
	{	P mousepos;
		XEvent e;
		XNextEvent(dpy, &e);
		switch (e.type)
		{case ConfigureNotify:
			{	XConfigureEvent *ce = (void*) &e;
				width = ce->width;
				height = ce->height; }
			break;
		 case MotionNotify:
			{	XButtonEvent *xbe = (void*) &e;
				change(mousepos, (P){xbe->x, xbe->y});
				mousepos = (P){xbe->x, xbe->y}; }
			draw();
			break;
		 case ButtonPress: 
			{	XButtonEvent *xbe = (void*) &e;
				mousepos = (P){xbe->x, xbe->y}; }
			break;
		 case Expose: draw(); break; }}
	return 0; }
