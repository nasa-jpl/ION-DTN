/* Build log message containing specified text in char[] buffer of specified 
 * length. The message format is
 * <ms-since-epoch> <text>\n */
static void buildLogMessage(char *buf, size_t buflen, char *text)
{
    struct timeval time; 
    getCurrentTime(&time);

    /* Build message buffer */
    isprintf(buf, buflen, "%ld%03d %s\n", time.tv_sec, time.tv_usec / 1000, 
            text); 
}

/* Append the specified text as a log message to the file ion.log. 
 * Copied from writeMemoToIonLog() in ici/library/ion.c, with some differences: 
 * - Uses buildLogMessage() to build the log message. As a consequence, some 
 *   variables are unused and are removed. */
static void	writeMemoToIonLog(char *text)
{
	static ResourceLock	logFileLock;
	static char		ionLogFileName[264] = "";
	static int		ionLogFile = -1;
	int			textLen;
	static char		msgbuf[256];

	if (text == NULL) return;
	if (*text == '\0')	/*	Claims that log file is closed.	*/
	{
		if (ionLogFile != -1)
		{
			close(ionLogFile);	/*	To be sure.	*/
			ionLogFile = -1;
		}

		return;		/*	Ignore zero-length memo.	*/
	}

	/*	The log file is shared, so access to it must be
	 *	mutexed.						*/

	if (initResourceLock(&logFileLock) < 0)
	{
		return;
	}

	lockResource(&logFileLock);
	if (ionLogFile == -1)
	{
		if (ionLogFileName[0] == '\0')
		{
			isprintf(ionLogFileName, sizeof ionLogFileName,
					"%.255s%cion.log",
					getIonWorkingDirectory(),
					ION_PATH_DELIMITER);
		}

		ionLogFile = iopen(ionLogFileName,
				O_WRONLY | O_APPEND | O_CREAT, 0666);
		if (ionLogFile == -1)
		{
			unlockResource(&logFileLock);
			perror("Can't redirect ION error msgs to log");
			return;
		}
	}

    /* Build log message using buildLogMessage() */
    buildLogMessage(msgbuf, sizeof(msgbuf), text); 

	textLen = strlen(msgbuf);
	if (write(ionLogFile, msgbuf, textLen) < 0)
	{
		perror("Can't write ION error message to log file");
	}
#ifdef TargetFFS
	close(ionLogFile);
	ionLogFile = -1;
#endif
	unlockResource(&logFileLock);
}

static void	ionRedirectMemos(void)
{
	setLogger(writeMemoToIonLog);
}

