/* Updated to handle string-based ION watch character function
 * Jay L. Gao August 28, 2023  */

/* From the given token, build a watch character message in the given buffer.
 * The message format is
 * <ms-since-epoch> <token-string>\n */
static void buildWatchMessage(char *buf, size_t buflen, char* token_str)
{
    struct timeval time; 
    getCurrentTime(&time);

    /* Build message buffer */
    isprintf(buf, buflen, "%ld%03d %s\n", time.tv_sec, time.tv_usec / 1000,
            token_str); 
}

/* Write the specified character as a watch char to the file ion.watch. 
 * Copied from buildLogMessage() in ici/library/ion.c, with differences:
 * - Takes a single char as a parameter, not a char*. Do not check if the token
 *   is NULL or \0.
 * - File name is "ion.watch".
 * - Variables renamed, names referring to "log" things now refer to "watch"
 *   things.
 * - Uses buildWatchMessage() to build the watch message. As a consequence, some 
 *   variables are unused and are removed. */
static void	writeCharToIonWatch(char* token_str)
{
	static ResourceLock	watchFileLock;
	static char		ionWatchFileName[264] = "";
	static int		ionWatchFile = -1;
	int			textLen;
	static char		msgbuf[256];

	/*	The watch file is shared, so access to it must be
	 *	mutexed.						*/

	if (initResourceLock(&watchFileLock) < 0)
	{
		return;
	}

	lockResource(&watchFileLock);
	if (ionWatchFile == -1)
	{
		if (ionWatchFileName[0] == '\0')
		{
			isprintf(ionWatchFileName, sizeof ionWatchFileName,
					"%.255s%cion.watch",
					getIonWorkingDirectory(),
					ION_PATH_DELIMITER);
		}

		ionWatchFile = iopen(ionWatchFileName,
				O_WRONLY | O_APPEND | O_CREAT, 0666);
		if (ionWatchFile == -1)
		{
			unlockResource(&watchFileLock);
			perror("Can't redirect ION watch characters to ion.watch");
			return;
		}
	}

    /* Build watch message using buildWatchMessage() */
    buildWatchMessage(msgbuf, sizeof(msgbuf), token_str); 

	textLen = strlen(msgbuf);
	if (write(ionWatchFile, msgbuf, textLen) < 0)
	{
		perror("Can't write ION watch characters to ion.watch");
	}
#ifdef TargetFFS
	close(ionWatchFile);
	ionWatchFile = -1;
#endif
	unlockResource(&watchFileLock);
}

static void ionRedirectWatchCharacters(void)
{ 
    setWatcher(writeCharToIonWatch);
}

