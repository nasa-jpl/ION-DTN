/*
 * hmackeys.c — Convenience utility for generating 256-bit HMAC keys
 *
 * Copyright © 2009 California Institute of Technology
 * Author: Scott Burleigh, Jet Propulsion Laboratory
 *
 * Modification History:
 * MM/DD/YY  AUTHOR         DESCRIPTION
 * --------  ------------   ---------------------------------------------
 *           S. Burleigh    Initial Implementation
 * 05/29/25  S. DeBaun      Replaced rand() with /dev/urandom.
 * 06/06/25  S. DeBaun      Replaced direct device polling with the
 *                          cross-platform poll_entropy_src API.
 */


#include <platform.h>
#include <fcntl.h>          /* open                   */
#include <unistd.h>         /* read, close            */
#include "entropy_src.h"    /* For poll_entropy_src() */

#define KEY_LEN 32          /* 256-bit HMAC key       */

static int	processLine(char *line, int lineLength)
{
	char		fileName[256];
	int		fd;
	unsigned char	key[KEY_LEN];
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
	/* 0600 so only the current user can read the secret key     */
	fd = iopen(fileName, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (fd < 0)
	{
		printf("Can't create file '%s': %s\n", fileName,
				system_error_msg());
		return -1;
	}

	/* --- Use cross-platform entropy source for secure key material --- */
	uvast bytes_generated = 0;
	result = poll_entropy_src(NULL, key, KEY_LEN, &bytes_generated);
	if (result < 0 || bytes_generated != KEY_LEN)
	{
		fprintf(stderr, "Could not generate key material: %s\n",
			getErrorMessage(result));
		close(fd);
		return -1;
	}

	if (write(fd, key, KEY_LEN) < KEY_LEN)
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
