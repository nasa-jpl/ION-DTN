/*
	tcaboot.c:	Trusted Collective authority initialization.
									*/
/*	Copyright (c) 2020, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Nodeor: Scott Burleigh, Jet Propulsion Laboratory		*/

#include "tc.h"

#if defined (ION_LWT)
int	tcaboot(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	int	bulletinsGroupNbr = a1 ? atoi((char *) a1) : -1;
	int	recordsGroupNbr = a2 ? atoi((char *) a2) : -1;
	int	auth = a3 ? atoi((char *) a3) : 6;
	int	fec_K = a4 ? atoi((char *) a4) : 50;
	doubtle	fec_R = a5 ? atof((char *) a5) : .2;
	int	delay = a6 ? atoi((char *) a6) : 5;
#else
int	main(int argc, char **argv)
{
	int	bulletinsGroupNbr = argc > 1 ? atoi(argv[1]) : -1;
	int	recordsGroupNbr = argc > 2 ? atoi(argv[2]) : -1;
	int	auth = argc > 3 ? atoi(argv[3]) : 6;
	int	fec_K = argc > 4 ? atoi(argv[4]) : 50;
	double	fec_R = argc > 5 ? atof(argv[5]) : .2;
	int	delay = argc > 6 ? atoi(argv[6]) : 5;
#endif
	int	fd;
	time_t	currentTime;
	char	timestamp[TIMESTAMPBUFSZ];
	char	line[256];

	fd = iopen("boot.tcarc", O_WRONLY | O_CREAT | O_TRUNC, 0777);
	if (fd < 0)
	{
		putSysErrmsg("Can't open cmd file", "boot.tcarc");
		return 1;
	}

	isprintf(line, sizeof line, "1 %d %d %d %d %.2f\n",
			bulletinsGroupNbr, recordsGroupNbr, auth, fec_K, fec_R);
	if (write(fd, line, strlen(line)) < 0)
	{
		putSysErrmsg("Can't write to cmd file", line);
		return 1;
	}

	currentTime = getCtime();
	currentTime += delay;
	writeTimestampUTC(currentTime, timestamp);
	isprintf(line, sizeof line, "m compiletime %s\n", timestamp);
	if (write(fd, line, strlen(line)) < 0)
	{
		putSysErrmsg("Can't write to cmd file", line);
		return 1;
	}

	close(fd);
	writeMemo("Stopping tcaboot.");
	return 0;
}
