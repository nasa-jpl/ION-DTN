/*
 * hmackeys.c — Convenience utility for generating HMAC keys.
 *
 * Copyright © 2009 California Institute of Technology
 * Author: Scott Burleigh, Jet Propulsion Laboratory
 *
 * Modification History:
 * MM/DD/YY  AUTHOR         DESCRIPTION
 * --------  ------------   ---------------------------------------------
 *           S. Burleigh    Initial Implementation
 * 06/13/25  S. DeBaun      Replaced rand with the
 *                          cross-platform poll_entropy_src API, added
 *                          command-line arg for variable key length keys.
 *
 * SYNOPSIS
 *
 * hmackeys [key_length_in_bytes] [key_names_file]
 *
 * This utility generates one or more HMAC keys of a specified length.
 * The key length is provided in BYTES.
 *
 * If no arguments are provided, it runs in interactive mode with the
 * default key length of 32 bytes (256 bits).
 *
 * USAGE EXAMPLES
 *
 * 1. Interactive mode, default 256-bit key (32 bytes):
 * $ hmackeys
 * : my_first_key_name
 * : my_second_key_name
 *
 * 2. Interactive mode, custom 512-bit key (64 bytes):
 * $ hmackeys 64
 * Using custom key length: 64 bytes (512 bits).
 * : my_512bit_key
 *
 * 3. Scripted mode from a file, custom 160-bit keys (20 bytes):
 * $ cat keylist.txt
 * key_for_node_A
 * key_for_node_B
 *
 * $ hmackeys 20 keylist.txt
 * Using custom key length: 20 bytes (160 bits).
 *
 * Standard HMAC Key Lengths (bits to bytes):
 * - HMAC-SHA1:   160 bits = 20 bytes
 * - HMAC-SHA256: 256 bits = 32 bytes (Default)
 * - HMAC-SHA384: 384 bits = 48 bytes
 * - HMAC-SHA512: 512 bits = 64 bytes
 */


#include <platform.h>
#include <fcntl.h>          /* open                   */
#include <unistd.h>         /* read, close            */
#include "entropy_src.h"    /* For poll_entropy_src() */

#define DEFAULT_KEY_LEN 32  /* Default to 256-bit HMAC key */

static int processLine(char *line, int lineLength, int keyLen)
{
	char	       fileName[256];
	int	       fd;
	unsigned char *key = NULL;
	int	       result = 0;

	/* Parameter intentionally unused. */
	(void)lineLength;

	if (*line == '#')       /* Comment.        */
	{
		return 0;
	}

	if (strcmp(line, "q") == 0)
	{
		return 1;
	}

	/* Dynamically allocate memory for the key based on requested length. */
	key = malloc(keyLen);
	if (key == NULL)
	{
		fprintf(stderr, "Failed to allocate memory for key.\n");
		return -1;
	}

	isprintf(fileName, sizeof fileName, "./%.80s.hmk", line);
	/* 0600 so only the current user can read the secret key     */
	fd = iopen(fileName, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (fd < 0)
	{
		fprintf(stderr, "Can't create file '%s': %s\n", fileName,
				system_error_msg());
		free(key);
		return -1;
	}

	/* --- Use cross-platform entropy source for secure key material --- */
	uvast bytes_generated = 0;
	size_t temp_bytes_generated;
	result = poll_entropy_src(NULL, key, keyLen, &temp_bytes_generated);
	bytes_generated = temp_bytes_generated;
	if (result < 0 || bytes_generated != (uvast)keyLen)
	{
		fprintf(stderr, "Could not generate key material: %s\n",
				get_error_message(result));
		close(fd);
		free(key);
		return -1;
	}

	if (write(fd, key, keyLen) < keyLen)
	{
		fprintf(stderr, "Can't write key to %s: %s\n", fileName,
				system_error_msg());
		result = -1;
	}

	close(fd);
	free(key); /* Clean up allocated memory */
	return result;
}

int main(int argc, char **argv)
{
	int   keyLen = DEFAULT_KEY_LEN;
	char *cmdFileName = NULL;
	int   argNbr = 1;
	int   cmdFile;
	char  line[80];
	int   len;

	/* Check for optional key length argument. */
	if (argc > argNbr)
	{
		long val = strtol(argv[argNbr], NULL, 10);
		if (val > 0)	  /* It's a valid number. */
		{
			keyLen = val;
			argNbr++; /* Move to next argument. */
			printf("Using custom key length: %d bytes (%d bits).\n",
					keyLen, keyLen * 8);
		}
	}

	/* Check for optional file name argument. */
	if (argc > argNbr)
	{
		cmdFileName = argv[argNbr];
	}

	if (cmdFileName == NULL)        /* Interactive.    */
	{
		cmdFile = fileno(stdin);
		while (1)
		{
			printf(": ");
			fflush(stdout);
			if (igets(cmdFile, line, sizeof line, &len) == NULL)
			{
				if (len == 0) break;
				putErrmsg("igets failed.", NULL);
				break;
			}

			if (len == 0) continue;

			if (processLine(line, len, keyLen))
			{
				break;
			}
		}
	}
	else                    /* Scripted.   */
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
				if (igets(cmdFile, line, sizeof line, &len) == NULL)
				{
					if (len == 0) break;
					putErrmsg("igets failed.", NULL);
					break;
				}

				if (len == 0 || line[0] == '#') continue;

				if (processLine(line, len, keyLen))
				{
					break;
				}
			}

			close(cmdFile);
		}
	}

	PUTS("Stopping hmackeys.");
	return 0;
}
