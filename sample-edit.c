#include <X11/Xlib.h>
#include <assert.h>
#include <stdio.h>

static int black, white;
static int width=100, height=30;
static Window w;
static GC gc;
static Display *dpy;
#define NSAMPLES 512
static int samples[NSAMPLES] = {0, 64-6, 128+12, 192-6, 255};

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
	double xfrac = ddiv(p.x, NSAMPLES-1);
	double yfrac = ddiv(p.y, 255);
	return (P){xfrac*width, height-(yfrac*height)}; }

// TODO Centerify
static P screen2sample (P p) {
	double xfrac = ddiv(p.x, width);
	double yfrac = ddiv(height-p.y, height);
	return (P){xfrac*(NSAMPLES-1), (yfrac*255)}; }

static void draw ()
{	XClearWindow(dpy, w);
	for (int ii=0, jj=1; jj<NSAMPLES; jj++,ii++) {
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
