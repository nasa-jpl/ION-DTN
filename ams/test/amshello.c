/*
	amshello.c:	A distributed "Hello, world" implemented
			using AMS on a Unix platform (only).
									*/
/*	Copyright (c) 2005, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/

#include "ams.h"

static int	runCatcher()
{
	AmsModule	me;
	AmsEvent	evt;
	int		cn, zn, nn, sn, len, ct, pr;
	unsigned char	fl;
	AmsMsgType	mt;
	char		*txt;

	oK(ams_register(NULL, NULL, "amsdemo", "test", "", "catch", &me));
	ams_invite(me, 0, 0, 0, 1, 8, 0, AmsArrivalOrder, AmsAssured);
	while (1)
	{
		if (ams_get_event(me, AMS_BLOCKING, &evt) < 0) return 0;
		if (ams_get_event_type(evt) == AMS_MSG_EVT) break;
		else ams_recycle_event(evt);
	}

	ams_parse_msg(evt, &cn, &zn, &nn, &sn, &len, &txt, &ct, &mt, &pr, &fl);
	printf("%d received '%s'.\n", (int) getpid(), txt); fflush(stdout);
	ams_recycle_event(evt); ams_unregister(me); return 0;
}

static int	runPitcher()
{
	AmsModule	me;
	AmsEvent	evt;
	AmsStateType	state;
	AmsChangeType	change;
	int		zn, nn, rn, dcn, dzn, sn, pr, textlen;
	unsigned char	fl;
	AmsSequence	sequence;
	AmsDiligence	diligence;
	char		buffer[80];

	isprintf(buffer, sizeof buffer, "Hello from %d.", (int) getpid());
	textlen = strlen(buffer) + 1;
	oK(ams_register(NULL, NULL, "amsdemo", "test", "", "pitch", &me));
	while (1)
	{
		if (ams_get_event(me, AMS_BLOCKING, &evt) < 0) return 0;
		ams_parse_notice(evt, &state, &change, &zn, &nn, &rn, &dcn,
				&dzn, &sn, &pr, &fl, &sequence, &diligence);
		ams_recycle_event(evt);
		if (state == AmsInvitationState && sn == 1)
		{
			printf("%d sending  '%s'.\n", (int) getpid(), buffer);
			fflush(stdout);
			ams_send(me, -1, zn, nn, 1, 0, 0, textlen, buffer, 0);
			ams_unregister(me); return 0;
		}
	}
}

int	main(int argc, char **argv)
{
	if (fork() == 0) return runCatcher(); else return runPitcher();
}
