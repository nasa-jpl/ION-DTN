/*
	ramsgate.c:	a simple Remote AMS gateway program.
									*/
/*	Author: Shin-Ywan (Cindy) Wang, Jet Propulsion Laboratory	*/
/*	Copyright (c) 2005, California Institute of Technology.		*/
/*	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.	*/

#include "rams.h"

#ifdef NOEXPAT
static char	*mibfilename = "mib.amsrc";
#else
static char	*mibfilename = "amsmib.xml";
#endif

#if defined (VXWORKS) || defined (RTEMS)
int ramsgate(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char	*application = (char *) a1;
	char	*authority = (char *) a2;
	int	lifetime = (unsigned) strtol((char *) a3, NULL, 0);
#else
int main(int argc, char **argv)
{
	char	*application;
	char	*authority;
	int	lifetime;

	if (argc < 4)
	{
		PUTS("Usage: ramsgate <application name> <authority name> \
<TTL for bundles>");
		return 0;
	}

	application = argv[1];
	authority = argv[2];
	lifetime = (unsigned) strtol(argv[3], NULL, 0);
#endif
	if (rams_run(mibfilename, NULL, application, authority, "", "RAMS",
				lifetime) < 0)
	{
		putErrmsg("ramsgate can't run.", NULL);
		writeErrmsgMemos();
	}

	PUTS("ramsgate terminated.");
	return 1;
}
