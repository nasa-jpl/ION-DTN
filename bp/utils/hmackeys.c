/*

	hmackeys.c:	convenience utility for generating good
			HMAC keys.

									*/
/*									*/
/*	Copyright (c) 2009, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include "platform.h"

static int	processLine(char *line)
{
	int		lineLength;
	char		fileName[256];
	int		fd;
	int		i;
	int		val;
	unsigned char	key[20];

	if (line == NULL) return 0;

	lineLength = strlen(line);
	if (lineLength <= 0) return 0;

	if (line[lineLength - 1] == 0x0a)	/*	LF (newline)	*/
	{
		line[lineLength - 1] = '\0';	/*	lose it		*/
		lineLength--;
		if (lineLength <= 0) return 0;
	}

	if (line[lineLength - 1] == 0x0d)	/*	CR (DOS text)	*/
	{
		line[lineLength - 1] = '\0';	/*	lose it		*/
		lineLength--;
		if (lineLength <= 0) return 0;
	}

	if (strcmp(line, "q") == 0)
	{
		return 1;
	}

	isprintf(fileName, sizeof fileName, "./%.80s.hmk", line);
	fd = open(fileName, O_RDWR | O_CREAT, 00777);
	if (fd < 0)
	{
		printf("Can't create file '%s': %s\n", fileName,
				system_error_msg());
		return -1;
	}

	for (i = 0; i < 20; i++)
	{
		val = random();
		key[i] = val & 0xff;
	}

	if (write(fd, key, 20) < 20)
	{
		printf("Can't write key to %s: %s\n", fileName,
				system_error_msg());
		return -1;
	}

	close(fd);
	return 0;
}

int	main(int argc, char **argv)
{
	char	*cmdFileName = (argc > 1 ? argv[1] : NULL);
	FILE	*cmdFile;
	char	line[80];

	srandom(time(NULL));
	if (cmdFileName == NULL)		/*	Interactive.	*/
	{
		while (1)
		{
			printf(": ");
			if (fgets(line, sizeof line, stdin) == NULL)
			{
				if (feof(stdin))
				{
					break;
				}

				perror("hmackeys fgets failed");
				break;		/*	Out of loop.	*/
			}

			if (processLine(line))
			{
				break;		/*	Out of loop.	*/
			}
		}
	}
	else					/*	Scripted.	*/
	{
		cmdFile = fopen(cmdFileName, "r");
		if (cmdFile == NULL)
		{
			perror("Can't open keynames file");
		}
		else
		{
			while (1)
			{
				if (fgets(line, sizeof line, cmdFile) == NULL)
				{
					if (feof(cmdFile))
					{
						break;	/*	Loop.	*/
					}

					perror("hmackeys fgets failed");
					break;		/*	Loop.	*/
				}

				if (processLine(line))
				{
					break;	/*	Out of loop.	*/
				}
			}

			fclose(cmdFile);
		}
	}

	puts("Stopping hmackeys.");
	return 0;
}
