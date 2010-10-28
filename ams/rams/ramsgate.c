/*
	ramsgate.c:	a simple Remote AMS gateway program.
									*/
/*	Author: Shin-Ywan (Cindy) Wang, Jet Propulsion Laboratory	*/
/*	Copyright (c) 2005, California Institute of Technology.		*/
/*	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.	*/

#include "rams.h"

#if defined (VXWORKS) || defined (RTEMS)
int ramsgate(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char		*application = (char *) a1;
	char		*authority = (char *) a2;
	int		lifetime = (unsigned) strtol((char *) a3, NULL, 0);
	int		msize = (unsigned) strtol((char *) a4, NULL, 0);
	char		*mmName = (char *) a5; 
	RamsGate	gWay;
#else
int main(int argc, char **argv)
{
	char		*application;
	char		*authority;
	int		lifetime;
	int		mSize = 200000;
	char		*mmName = NULL; 
	RamsGate	gWay;

	if (argc < 4)
	{
		PUTS("Usage: ramsgate <application name> <authority name> \
<TTL for bundles> [memorySize [memory manager name]]");
		return 0;
	}

	application = argv[1];
	authority = argv[2];
	lifetime = (unsigned) strtol(argv[3], NULL, 0);
	if (argc > 4)
	{
		mSize = (unsigned) strtol(argv[4], NULL, 0);
		if (argc > 5)
		{
			mmName = argv[5];
		}
	}
#endif
	if (rams_run("amsmib.xml", NULL, mmName, NULL, mSize, application,
			authority, "", "RAMS", &gWay, lifetime) < 0)
	{
		putErrmsg("ramsgate can't run.", NULL);
		writeErrmsgMemos();
	}

	PUTS("ramsgate terminated.");
	return 1;
}
