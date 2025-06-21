/*
	sppcli.c:	BP SPP-based convergence-layer input
			daemon, designed to serve as an input
			duct.

	Author: Gregory Miles

	Copyright (c) 2025, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "sppcla.h"
#include "ipnfw.h"
#include "dtn2fw.h"

//static void	*handleSpacePackets(void *parm)
//{
//    char			*buffer;
//    int			bundleLength;
    
//    return 0;
//}

/*	*	*	Main thread functions	*	*	*	*/

#if defined (ION_LWT)
int	sppcli(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*ductName = (char *) a1;
#else
int	main(int argc, char *argv[])
{
	char	*ductName = (argc > 1 ? argv[1] : NULL);
#endif
	if (ductName == NULL)
	{
		PUTS("Usage: sppcli <FILE PATH>");
		return 0;
	}
	return 0;
}
