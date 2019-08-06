/*
	kaboot.c:	DTKA authority adminstration interface.
									*/
/*	Copyright (c) 2013, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Nodeor: Scott Burleigh, Jet Propulsion Laboratory		*/

#include "kauth.h"

#if defined (VXWORKS) || defined (RTEMS) || defined (bionic)
int	kaboot(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	int	delay = a1 ? strtol((char *) a1, NULL, 0) : 5;
#else
int	main(int argc, char **argv)
{
	int	delay = argc > 1 ? strtol(argv[1], NULL, 0) : 5;
#endif
	int	fd;
	time_t	currentTime;
	char	timestamp[TIMESTAMPBUFSZ];
	char	line[256];
	int	len;

	fd = iopen("boot.karc", O_WRONLY | O_CREAT | O_TRUNC, 0777);
	if (fd < 0)
	{
		putSysErrmsg("Can't open cmd file", "boot.karc");
		return 1;
	}

	istrcpy(line, "1\n", sizeof line);
	if (write(fd, line, 2) < 0)
	{
		putSysErrmsg("Can't write to cmd file", line);
		return 1;
	}

	currentTime = getCtime();
	currentTime += delay;
	writeTimestampUTC(currentTime, timestamp);
	len = _isprintf(line, sizeof line, "m compiletime %s\n", timestamp);
	if (write(fd, line, len) < 0)
	{
		putSysErrmsg("Can't write to cmd file", line);
		return 1;
	}

	close(fd);
	PUTS("Stopping kaboot.");
	return 0;
}
