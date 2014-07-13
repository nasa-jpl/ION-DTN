/*
	ramsgate.c:	a simple Remote AMS gateway program.
									*/
/*	Author: Shin-Ywan (Cindy) Wang, Jet Propulsion Laboratory	*/
/*	Copyright (c) 2005, California Institute of Technology.		*/
/*	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.	*/

#include "rams.h"

#if defined (ION_LWT)
int ramsgate(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char	*application = (char *) a1;
	char	*authority = (char *) a2;
	long	lifetime = strtol((char *) a3, NULL, 86400);
#else
int main(int argc, char **argv)
{
	char	*application = (argc > 1 ? argv[1] : NULL);
	char	*authority = (argc > 2 ? argv[2] : NULL);
	long	lifetime = (argc > 3 ?  strtol(argv[3], NULL, 0) : 86400);
#endif
	if (application == NULL || authority == NULL || lifetime < 1)
	{
		PUTS("Usage: ramsgate <application name> <authority name> \
<TTL for bundles>");
		return 0;
	}

	if (rams_run("", NULL, application, authority, "", "RAMS",
			(int) lifetime) < 0)
	{
		putErrmsg("ramsgate can't run.", NULL);
		writeErrmsgMemos();
		return 1;
	}

	PUTS("ramsgate terminated.");
	return 0;
}
