static void	writeMemoToStdout(char *text)
{
	time_t	currentTime = getUTCTime();
	char	timestampBuffer[20];
	char	msgbuf[256];

	writeTimestampLocal(currentTime, timestampBuffer);
	isprintf(msgbuf, sizeof msgbuf, "[%s] %s", timestampBuffer, text);
	puts(msgbuf);
}

static void	ionRedirectMemos()
{
	setLogger(writeMemoToStdout);
}
