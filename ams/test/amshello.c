/*
amshello.c
"Hello world" demonstration using AMS - Unix platform (only)

Copyright (c) 2023, California Institute of Technology.	
Sky DeBaun, Jet Propulsion Laboratory.


This program assumes the following conditions---------------
1.) ION is running
2.) An AMS Registrar is running 	
3.) An AMS Configuration Server is running 
4.) An MIB configuration file has been created

NOTE: the following command completes steps 2, 3, and 4 (run this after ION starts):
amsd @ @ amsdemo test "" &

*/

#include "ams.h"

static int	runPitcher()
{
	AmsModule	    me;
	AmsEvent	    evt;
	AmsStateType	state;
	AmsChangeType	change;
	int		        zn, nn, rn, dcn, dzn, sn, pr, textlen;
	unsigned char	fl;
	AmsSequence	    sequence;
	AmsDiligence	diligence;
	char		    buffer[80];

	isprintf(buffer, sizeof buffer, "Hello from process %d", (int) getpid());
	textlen = strlen(buffer) + 1;

	//register pitch module using default in-memory MIB (i.e. @)
	oK(ams_register("@", NULL, "amsdemo", "test", "", "pitch", &me));
	
	while (1)
	{
		if (ams_get_event(me, AMS_BLOCKING, &evt) < 0) return 0;
			ams_parse_notice(evt, &state, &change, &zn, &nn, &rn, &dcn,
					&dzn, &sn, &pr, &fl, &sequence, &diligence);
			ams_recycle_event(evt);

		if (state == AmsInvitationState && sn == 1)
		{
			printf("Process %d sending:  '%s'\n", (int) getpid(), buffer);
			fflush(stdout);
			ams_send(me, -1, zn, nn, 1, 0, 0, textlen, buffer, 0);
			ams_unregister(me); return 0;
		}
	}
}

static int	runCatcher()
{
	AmsModule	    me;
	AmsEvent	    evt;
	int		        cn, zn, nn, sn, len, ct, pr;
	unsigned char	fl;
	AmsMsgType	    mt;
	char		    *txt;

	//register catch module using default in-memory MIB (i.e. @)
	oK(ams_register("@", NULL, "amsdemo", "test", "", "catch", &me));
	ams_invite(me, 0, 0, 0, 1, 8, 0, AmsArrivalOrder, AmsAssured);
	
	while (1)
	{
		if (ams_get_event(me, AMS_BLOCKING, &evt) < 0) return 0;
		if (ams_get_event_type(evt) == AMS_MSG_EVT) break;
		else ams_recycle_event(evt);
	}

	ams_parse_msg(evt, &cn, &zn, &nn, &sn, &len, &txt, &ct, &mt, &pr, &fl);
	printf("Process %d received: '%s'\n", (int) getpid(), txt); fflush(stdout);
	ams_recycle_event(evt); ams_unregister(me); return 0;
}


int main(void) 
{
    pid_t pid = fork();

    if (pid == -1) {
        fprintf(stderr, "Failed to create child process.\n");
        return EXIT_FAILURE;
    }

    if (pid == 0)
        //child process runs transmitter----------------------
        runPitcher();
    else 
    {
        //parent process runs receiver------------------------
        runCatcher();
    }

	return 0;
}
