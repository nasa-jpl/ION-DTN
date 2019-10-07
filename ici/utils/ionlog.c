/*
	ionlog.c:	Unix program to copy to the ion.log file
			all text received via stdin.
									*/
/*	Copyright (c) 2019, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include "ion.h"

#if defined (ION_LWT)
int	ionlog(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
#else
int	main(int argc, char **argv)
{
#endif
	char	buffer[260] = "[*] ";

	if (ionAttach())
	{
		PUTS("ionlog unable to attach to ION.");
		return 0;
	}

	while (1)
	{
		if (fgets(buffer + 4, sizeof buffer - 4, stdin) == NULL)
		{
			break;
		}

		writeMemo(buffer);
	}

	writeMemo("[i] ionlog terminated.");
	return 0;
}
