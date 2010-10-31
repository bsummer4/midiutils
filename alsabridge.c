/*
	# alsasink
	## TODO This currently only support note-on and note-off.
	## TODO This currently only supports reading from alsa.
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <asoundlib.h>
#include <signal.h>
#include <err.h>
#include <stdint.h>
#include "midimsg.h"

#define CAPS (SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE)
#define TYPE (SND_SEQ_PORT_TYPE_MIDI_GENERIC|SND_SEQ_PORT_TYPE_APPLICATION)
static snd_seq_t *seq;
static snd_seq_addr_t ports;
static char *portname = "20";

void dumpevent (snd_seq_event_t *e) {
	struct mm_msg m =
	{	.type = 0,
		.chan = e->data.control.channel,
		.arg1 = e->data.note.note,
		.arg2 = e->data.note.velocity };
	switch (e->type) {
	 case SND_SEQ_EVENT_NOTEON: m.type = MM_NOTEON; break;
	 case SND_SEQ_EVENT_NOTEOFF: m.type = MM_NOTEOFF; break;
	 default: return; }
	mm_write(1, &m); }

int main (int argc, char* argv[]) {
	close(0);
	int good=0;
	if (argc >1) portname = argv[1];

	(	!snd_seq_open(&seq, "default", SND_SEQ_OPEN_INPUT, 0) &&
		!snd_seq_set_client_name(seq, "alsabridge") &&
		(0 <= snd_seq_parse_address(seq, &ports, portname)) &&
		!snd_seq_create_simple_port(seq, "alsabridge", CAPS, TYPE) &&
		(0 <= snd_seq_connect_from(seq, 0, ports.client, ports.port)) &&
		(good=1)
	);

	if (!good) errx(1, "Couldn't connect on port %s", portname);

	int r, count = snd_seq_poll_descriptors_count(seq, POLLIN);
	struct pollfd pfds[count];
	snd_seq_event_t *event;
	for (;;)
	{	snd_seq_poll_descriptors(seq, pfds, count, POLLIN);
		if (0 > poll(pfds, count, -1)) break;
	 moar:
		r = snd_seq_event_input(seq, &event);
		if (r >= 0 && event) dumpevent(event);
		if (r > 0) goto moar; }
	/* Cleanup is for pussies!  */
	return 0; }
