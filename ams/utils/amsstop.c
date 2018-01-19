/*
	amsstop.c:	an AMS utility program that simply shuts down
			a single message space.
									*/
/*									*/
/*	Copyright (c) 2011, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include "ams.h"

#if defined (ION_LWT)
int	amsstop(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char		*applicationName = (char *) a1;
	char		*authorityName = (char *) a2;
#else
int	main(int argc, char **argv)
{
	char		*applicationName = (argc > 1 ? argv[1] : NULL);
	char		*authorityName = (argc > 2 ? argv[2] : NULL);
#endif
	int		amsstopSubj;
	AmsModule	me;

	if (applicationName == NULL || authorityName == NULL)
	{
		PUTS("Usage: amsstop <application name> <authority name>");
		return 0;
	}

	if (ams_register("", NULL, applicationName, authorityName, "",
			"amsstop", &me) < 0)
	{
		putErrmsg("amsstop can't register.", NULL);
		return 1;
	}

	snooze(4);
	amsstopSubj = ams_lookup_subject_nbr(me, "amsstop");
	if (amsstopSubj < 0)
	{
		writeMemo("[?] amsstop subject undefined.");
	}
	else
	{
		if (ams_publish(me, amsstopSubj, 1, 0, 0, NULL, 0) < 0)
		{
			putErrmsg("amsstop can't publish 'amsstop' message.",
					NULL);
		}
	}

	snooze(1);
	ams_unregister(me);
	writeErrmsgMemos();
	return 0;
}
