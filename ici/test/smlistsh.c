/*

	smlistsh.c:	a test driver for the "smlist" shared memory
			list library.

									*/
/*									*/
/*	Copyright (c) 2000, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include "platform.h"
#include "psm.h"
#include "smlist.h"

#if defined (ION_LWT)
int	smlistsh(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
#else
int	main(int argc, char **argv)
#endif
{
	int		cmdFile;
	char		line[256];
	int		len;
	int		count;
	char		command;
	long		arg1;
	long		arg2;
	unsigned long	keyValue;
	PsmAddress	datum;
	PsmUsageSummary	summary;
	PsmAddress	elt;
	int		key;
	int		length;
	unsigned char	*smlistsh_space = NULL;
	uaddr		smlistsh_partitionId = 0;
	PsmPartition	smlistsh_partition = NULL;
	PsmAddress	smlistsh_list = 0;
	PsmMgtOutcome	outcome;

#ifdef FSWLOGGER
	return 0;	/*	No stdin/stdout, can't be interactive.	*/
#endif
	sm_ipc_init();
	cmdFile = fileno(stdin);
	while (1)
	{
		printf(": ");
		fflush(stdout);
		if (igets(cmdFile, line, sizeof line, &len) == NULL)
		{
			putErrmsg("igets failed.", NULL);
			break;
		}

		if (len == 0)
		{
			continue;
		}

		count = sscanf(line, "%c %ld %ld", &command, &arg1, &arg2);
		switch (count)
		{
		case 1:
			switch (command)
			{
			case 'h':
			case '?':
				puts("smlistsh valid commands are:");
				puts("   key    : k");
				puts("   attach : + <key value> <size>");
				puts("   report : r");
				puts("   detach : -");
				puts("   new    : n");
				puts("   share  : s <list address>");
				puts("   walk   : w");
				puts("   prepend: p <element value>");
				puts("   append : a <element value>");
				puts("   find   : f <element value>");
				puts("   delete : d <element address>");
				puts("   help  : h or ?");
				puts("   quit  : q");
				continue;

			case 'k':
				keyValue = sm_GetUniqueKey();
				printf("New key value: %ld\n", keyValue);
				continue;

			case '-':
				if (smlistsh_partition == NULL)
				{
					continue;
				}

				sm_ShmDetach((char *) smlistsh_space);
				smlistsh_list = 0;
				continue;

			case 'r':
				if (smlistsh_partition == NULL)
				{
					puts("No shared memory partition.");
					continue;
				}

				psm_usage(smlistsh_partition, &summary);
				psm_report(&summary);
				continue;

			case 'n':
				if (smlistsh_list != 0)
				{
					puts("already have a list");
					break;
				}

				smlistsh_list =
					sm_list_create(smlistsh_partition);
				printf("List is at " ADDR_FIELDSPEC ".\n",
					smlistsh_list);
				continue;

			case 'w':
				if (smlistsh_partition == NULL)
				{
					puts("No shared memory partition.");
					continue;
				}

				if (smlistsh_list == 0)
				{
					puts("do not have a list");
					break;
				}

				for (elt = sm_list_first(smlistsh_partition,
					smlistsh_list); elt; elt =
					sm_list_next(smlistsh_partition, elt))
				{
					printf("element at " ADDR_FIELDSPEC \
" contains " ADDR_FIELDSPEC ".\n", elt, sm_list_data(smlistsh_partition, elt));
				}

				continue;

			case 'q':
				if (smlistsh_partition != NULL)
				{
					sm_ShmDetach((char *) smlistsh_space);
				}

				return 0;

			default:
				puts("invalid command");
				continue;
			}

			continue;

		case 2:
			if (smlistsh_partition == NULL)
			{
				puts("No shared memory partition.");
				continue;
			}

			switch (command)
			{
			case 's':
				if (smlistsh_list != 0)
				{
					puts("already have a list");
					break;
				}

				smlistsh_list = arg1;
				break;

			case 'p':
				if (smlistsh_list == 0)
				{
					puts("do not have a list");
					break;
				}

				datum = arg1;
				if (sm_list_insert_first(smlistsh_partition,
						smlistsh_list, datum) == 0)
				{
					puts("unable to insert first elt");
				}

				break;

			case 'a':
				if (smlistsh_list == 0)
				{
					puts("do not have a list");
					break;
				}

				datum = arg1;
				if (sm_list_insert_last(smlistsh_partition,
						smlistsh_list, datum) == 0)
				{
					puts("unable to insert last elt");
				}

				break;

			case 'f':
				if (smlistsh_list == 0)
				{
					puts("do not have a list");
					break;
				}

				datum = arg1;
				elt = sm_list_search(smlistsh_partition,
					sm_list_first(smlistsh_partition,
					smlistsh_list), NULL, (void *) datum);
				printf("value %ld is in element at " \
ADDR_FIELDSPEC ".\n", arg1, elt);

			case 'd':
				if (smlistsh_list == 0)
				{
					puts("do not have a list");
					break;
				}

				CHKZERO(sm_list_delete(smlistsh_partition,
					(PsmAddress) arg1, NULL, NULL) == 0);
				break;

			default:
				puts("invalid command");
			}

			continue;

		case 3:
			if (command == '+')
			{
				if (smlistsh_partition != NULL)
				{
					puts("Already attached.  Detach and \
try again.");
					continue;
				}

				smlistsh_partitionId = 0;
				key = arg1;
				length = arg2;
				if (sm_ShmAttach(key, length,
						(char **) &smlistsh_space,
						&smlistsh_partitionId) == ERROR)
				{
					perror("sm_ShmAttach failed");
					continue;
				}

				if (psm_manage((char *) smlistsh_space,
						(unsigned int) length,
						"smlistsh", &smlistsh_partition,
						&outcome) < 0
				|| outcome == Refused)
				{
					puts("psm_manage failed.");
				}

				continue;
			}

			/*	Intentional fall-through to default.	*/

		default:
			puts("invalid command");
		}
	}

	return 0;
}
