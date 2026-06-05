/*

	psmshell.c:	a "psm" memory allocation test driver.

									*/
/*									*/
/*	Copyright (c) 1997, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <platform.h>
#include <psm.h>

#define CELL_COUNT	100

static int	run_psmshell(short partitionSize)
{
	unsigned int	length;
	char		*space;
	PsmAddress	*cells;		/*	Array.			*/
	PsmMgtOutcome	outcome;
	PsmPartition	partition = NULL;
	char		line[256];
	int		len;
	int		cmdFile;
	char		command;
	PsmUsageSummary	summary;
	int		cell;
	unsigned int	size;
	char		*cursor;
	char		*tokens[3];
	int		tokenCount;
	int		parsed_int;
	uvast		parsed_uvast;

	if (sm_ipc_init() < 0)
	{
		writeErrmsgMemos();
		puts("IPC initialization failed.");
		return 0;
	}

	length = 1024 * partitionSize;
	printf("psmshell: partition size is %u\n", length);
	space = calloc(1, length);
	if (space == NULL)
	{
		puts("psmshell: can't allocate space; quitting");
		return 0;
	}

	cells = (PsmAddress *) calloc(CELL_COUNT, sizeof(PsmAddress));
	if (cells == NULL)
	{
		free(space);
		puts("psmshell: can't allocate test variables; quitting");
		return 0;
	}

	if (psm_manage(space, length, "psmshell", &partition, &outcome) < 0
	|| outcome == Refused)
	{
		free(space);
		free(cells);
		return -1;
	}

	cmdFile = fileno(stdin);
	while (1)
	{
		printf(": ");
		fflush(stdout);
		if (igets(cmdFile, line, sizeof line, &len) == NULL)
		{
			putErrmsg("igets failed.", NULL);
			psm_erase(partition);
			free(space);
			free(cells);
			return -1;
		}

		if (len == 0)
		{
			continue;
		}

		/*	Tokenize the input line safely.			*/
		tokenCount = 0;
		cursor = line;
		while (*cursor != '\0' && tokenCount < 3)
		{
			while (isspace((unsigned char) *cursor))
			{
				cursor++;
			}

			if (*cursor == '\0')
			{
				break;
			}

			tokens[tokenCount] = cursor;
			tokenCount++;

			while (*cursor != '\0' && !isspace((unsigned char) *cursor))
			{
				cursor++;
			}

			if (*cursor != '\0')
			{
				*cursor = '\0';
				cursor++;
			}
		}

		if (tokenCount == 0)
		{
			continue;
		}

		/*	Extract and validate parameters.		*/
		command = tokens[0][0];

		if (tokenCount > 1)
		{
			if (platform_parse_int(tokens[1], &parsed_int) < 0)
			{
				puts("psmshell: invalid cell number format.");
				continue;
			}
			cell = parsed_int;
		}

		if (tokenCount > 2)
		{
			if (platform_parse_uvast(tokens[2], &parsed_uvast) < 0
				|| parsed_uvast > UINT_MAX)
			{
				puts("psmshell: invalid size format or overflow.");
				continue;
			}
			size = (unsigned int) parsed_uvast;
		}

		switch (tokenCount)
		{
		case 1:
			switch (command)
			{
			case 'h':
			case '?':
				puts("psmshell valid commands are:");
				puts("   malloc - m <cell nbr> <size>");
				puts("   zalloc - z <cell nbr> <size>");
				puts("   print  - p <cell nbr>");
				puts("   free   - f <cell nbr>");
				puts("   panic  - !");
				puts("   relax  - .");
				puts("   usage  - u");
				puts("   help   - h or ?");
				puts("   quit   - q");
				continue;

			case '!':
				psm_panic(partition);
				continue;

			case '.':
				psm_relax(partition);
				continue;

			case 'u':
				psm_usage(partition, &summary);
				psm_report(&summary);
				continue;

			case 'q':
				psm_unmanage(partition);
				free(space);
				free(cells);
				return 0;

			default:
				puts("psmshell: invalid command");
				continue;
			}

		case 2:
			if (cell < 0 || cell >= CELL_COUNT)
			{
				printf("psmshell: cell nbr must be 0-%d\n",
						CELL_COUNT - 1);
				continue;
			}

			switch (command)
			{
			case 'f':
				psm_free(partition, cells[cell]);
				cells[cell] = 0;
				break;

			case 'p':
				printf(ADDR_FIELDSPEC "\n", cells[cell]);
				break;

			default:
				puts("psmshell: invalid command");
			}

			continue;

		case 3:
			if (cell < 0 || cell >= CELL_COUNT)
			{
				printf("psmshell: cell nbr must be 0-%d\n",
						CELL_COUNT - 1);
				continue;
			}

			if (cells[cell])
			{
				puts("psmshell: no allocation, cell not empty");
				continue;
			}

			switch (command)
			{
			case 'm':
				cells[cell] = psm_malloc(partition, size);
				if (cells[cell] == 0)
				{
					puts("psmshell: allocation failed");
					cells[cell] = 0;
				}
				else
				{
					memset(psp(partition, cells[cell]),
							0, size);
				}

				continue;

			case 'z':
				cells[cell] = psm_zalloc(partition, size);
				if (cells[cell] == 0)
				{
					puts("psmshell: allocation failed");
					cells[cell] = 0;
				}
				else
				{
					memset(psp(partition, cells[cell])
							, 0, size);
				}

				continue;
			}

			/* FALLTHROUGH */

		default:
			puts("psmshell: invalid command");
		}
	}
}

#if defined (ION_LWT)
int     psmshell(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	short   partitionSize = a1;
#else
int     main(int argc, char **argv)
{
	short   partitionSize = 0;
#endif
	int     parsed_int;
	char    errMsg[256];

#ifndef ION_LWT
	if (argc < 2)
	{
		PUTS("Usage: psmshell <partition size in kilobytes>");
		return 0;
	}

	if (platform_parse_int(argv[1], &parsed_int) < 0 || parsed_int <= 0 || parsed_int > 32767)
	{
		isprintf(errMsg, sizeof(errMsg), "[?] Invalid partition size. Must be 1-32767: %s", argv[1]);
		PUTS(errMsg);
		return 0;
	}
	partitionSize = (short) parsed_int;
#endif

#ifdef NON_INTERACTIVE
	return 0;       /* No stdin/stdout, can't be interactive.  */
#endif
	return run_psmshell(partitionSize);
}
