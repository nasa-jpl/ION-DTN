/*
	amslogprt.c:	Unix program to print an AMS message log
			file produced by amslog.
									*/
/*	Copyright (c) 2005, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include "platform.h"

#if defined (ION_LWT)
int	amslogprt(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
#else
int	main(int argc, char **argv)
{
#endif
	int		subjectNameLength;
	char		subjectName[33];
	unsigned int	contentLength;
	char		text[8];
	unsigned char	*buffer = NULL;
	int		bufferLength = 0;
	char		codes[] = "0123456789abcdef";
	int		i;
	unsigned int	idx;

	while (1)
	{
		if (fread((char *) &subjectNameLength, sizeof subjectNameLength,
					1, stdin) == 0)
		{
			break;
		}

		if (fread(subjectName, subjectNameLength, 1, stdin) == 0)
		{
			break;
		}

		subjectName[subjectNameLength] = '\0';
		if (fread((char *) &contentLength, sizeof contentLength, 1,
				stdin) == 0)
		{
			break;
		}

		if (contentLength == 0)
		{
			printf("%32s %10d\n", subjectName, contentLength);
			continue;
		}

		if (contentLength > bufferLength)
		{
			if (buffer)
			{
				free(buffer);
				bufferLength = 0;
			}

			buffer = (unsigned char *) calloc(1, contentLength);
			if (buffer == NULL)
			{
				printf("requested buffer too large: %d\n",
						contentLength);
				memcpy(text, (char *) &contentLength,
						sizeof contentLength);
				text[sizeof contentLength] = '\0';
				printf("Might be reading non-messages from \
stdin, e.g. ASCII text ['%s']?\n", text);
				break;
			}

			bufferLength = contentLength;
		}

		memset(buffer, 0, bufferLength);
		if (fread(buffer, contentLength, 1, stdin) == 0)
		{
			break;
		}

		if (buffer[contentLength - 1] == 0)
		{
			printf("%32s %10d '%s'\n", subjectName, contentLength,
					buffer);
			continue;
		}

		/*	Not a NULL-terminated string; print hex dump.	*/

		printf("%32s %10d  ", subjectName, contentLength);
		for (i = 0; i < contentLength; i++)
		{
			idx = buffer[i];
			printf("%c%c", codes[idx / 16], codes[idx % 16]);
		}

		printf("\n");
	}

	if (buffer)
	{
		free(buffer);
	}

	return 0;
}
