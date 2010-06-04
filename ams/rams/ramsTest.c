/*
	ramsTest.c:	an ramsTest program for UNIX.  Subscribes
			to subject "bench", counts the number of such
			messages received and their agregate size, and
			computes and prints performance statistics.
			The first 4 bytes of each message contain the
			total number of messages that will be published,
			including this one.
									*/
/*	Copyright (c) 2005, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/

#include "rams.h"

static RamsGate    rNode;

#if defined (VXWORKS) || defined (RTEMS)
int ramstest(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char	*application = (char *) a1;
	char	*authority = (char *) a2;
	int	lifetime = a3;
	int	mSize = a4;
	char	*mmName = (char *) a5; 
#else
int main(int argc, char **argv)
{
	char	*application;
	char	*authority;
	int	lifetime;
	int	mSize;
	char	*mmName; 

	if (argc < 4)
	{
		PUTS("Usage: ramstest <application Name> <authority name> \
<TTL for bundles> [memory manager name] [memorySize]");
		return 0;
	}

	application = argv[1];
	authority = argv[2];
	lifetime = atoi(argv[3]);

	mSize = 0;
	mmName = NULL;
	if (argc == 4)
	{
		mSize = 200000;
		mmName = NULL;
	}
	else if (argc == 5)
	{
		mSize = 200000;
		mmName = argv[4];
	}
	else 
	{
		mSize = (unsigned) atol(argv[5]);
		mmName = argv[4];
	}
#endif
	// mibSource,
	// transport service selection order
	// memory name
	// memory address
	// memory size
	// application name
	// authority name
	// unit name
	// role name
	// RamsGateway

	//signal(SIGINT, handleQuit);

	if (rams_register("amsmib.xml", NULL, mmName, NULL, mSize, application,
			authority, "", "RAMS", &rNode, lifetime) <= 0)
	{
		putSysErrmsg("ramstest can't register", NULL);
		writeErrmsgMemos();
	}

	//rams_unregister(&rNode);
	PUTS("disconnected....");
	return 1;
}
