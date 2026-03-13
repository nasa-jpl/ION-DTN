/*
 *	gdswatcher.c:	GDS watch character file logger for ION.
 *
 *	This file is #include'd directly into ici/library/ion.c when
 *	compiled with -DGDSWATCHER.  It redirects watch characters to a
 *	timestamped log file for offline analysis.
 *
 *	Build:
 *	  ./configure --enable-ewchar CFLAGS="-DGDSWATCHER -Itools/gdswatcher"
 *	  make clean && make && sudo make install
 *
 *	The log file path is taken from the ION_WATCH_LOG environment
 *	variable; if unset it defaults to "ion_watch.log" in the current
 *	working directory.  The file is opened in append mode.
 *
 *	Output format (one line per watch event):
 *	  <seconds>.<microseconds> <watch_string>
 *
 *	Example:
 *	  1710000001.234567 (ds42)g
 *	  1710000003.890456 (d42)s
 *	  1710000004.123456 (42)h
 *
 *	Copyright (c) 2026, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 */

#include <sys/time.h>

static FILE	*watchLogFile = NULL;

static void	watchToFile(char *token)
{
	struct timeval	tv;

	if (watchLogFile == NULL)
	{
		/*	File was not opened or was closed; fall back
		 *	to stdout.					*/
		printf("%s", token);
		fflush(stdout);
		return;
	}

	gettimeofday(&tv, NULL);
	fprintf(watchLogFile, "%ld.%06ld %s\n",
			(long) tv.tv_sec, (long) tv.tv_usec, token);
	fflush(watchLogFile);
}

static void	ionRedirectWatchCharacters(void)
{
	char	*logPath;

	logPath = getenv("ION_WATCH_LOG");
	if (logPath == NULL)
	{
		logPath = "ion_watch.log";
	}

	watchLogFile = fopen(logPath, "a");
	if (watchLogFile == NULL)
	{
		putSysErrmsg("Can't open watch log file, using stdout",
				logPath);
		setWatcher(NULL);	/*	Defaults to stdout.	*/
		return;
	}

	setWatcher(watchToFile);
}
