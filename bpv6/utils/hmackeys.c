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

static int	processLine(char *line, int lineLength)
{
	char		fileName[256];
	int		fd;
	int		i;
	int		val;
	unsigned char	key[20];
	int		result = 0;

	if (*line == '#')		/*	Comment.		*/
	{
		return 0;
	}

	if (strcmp(line, "q") == 0)
	{
		return 1;
	}

	isprintf(fileName, sizeof fileName, "./%.80s.hmk", line);
	fd = iopen(fileName, O_RDWR | O_CREAT, 0777);
	if (fd < 0)
	{
		printf("Can't create file '%s': %s\n", fileName,
				system_error_msg());
		return -1;
	}

	for (i = 0; i < 20; i++)
	{
		val = rand();
		key[i] = val & 0xff;
	}

	if (write(fd, key, 20) < 20)
	{
		printf("Can't write key to %s: %s\n", fileName,
				system_error_msg());
		result = -1;
	}

	close(fd);
	return result;
}

int	main(int argc, char **argv)
{
	char	*cmdFileName = (argc > 1 ? argv[1] : NULL);
	int	cmdFile;
	char	line[80];
	int	len;

	srand(time(NULL));
	if (cmdFileName == NULL)		/*	Interactive.	*/
	{
		cmdFile = fileno(stdin);
		while (1)
		{
			printf(": ");
			fflush(stdout);
			if (igets(cmdFile, line, sizeof line, &len) == NULL)
			{
				if (len == 0)
				{
					break;
				}

				putErrmsg("igets failed.", NULL);
				break;		/*	Out of loop.	*/
			}

			if (len == 0)
			{
				continue;
			}

			if (processLine(line, len))
			{
				break;		/*	Out of loop.	*/
			}
		}
	}
	else					/*	Scripted.	*/
	{
		cmdFile = iopen(cmdFileName, O_RDONLY, 0777);
		if (cmdFile < 0)
		{
			PERROR("Can't open keynames file");
		}
		else
		{
			while (1)
			{
				if (igets(cmdFile, line, sizeof line, &len)
						== NULL)
				{
					if (len == 0)
					{
						break;	/*	Loop.	*/
					}

					putErrmsg("igets failed.", NULL);
					break;		/*	Loop.	*/
				}

				if (len == 0
				|| line [0] == '#')	/*	Comment.*/
				{
					continue;
				}

				if (processLine(line, len))
				{
					break;	/*	Out of loop.	*/
				}
			}

			close(cmdFile);
		}
	}

	PUTS("Stopping hmackeys.");
	return 0;
}
