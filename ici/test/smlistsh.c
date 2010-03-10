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

static unsigned char	*smlistsh_space;
static int		smlistsh_partitionId;
static PsmPartition	smlistsh_partition;
static PsmAddress	smlistsh_list;

static void	attach(unsigned long key, unsigned long length)
{
	if (smlistsh_partition != NULL)
	{
		puts("Already attached.  Detach and try again.");
		return;
	}

	smlistsh_partitionId = 0;
	if (sm_ShmAttach((int) key, (int) length, (char **) &smlistsh_space,
			&smlistsh_partitionId) == ERROR)
	{
		perror("sm_ShmAttach failed");
		return;
	}

	if (psm_manage((char *) smlistsh_space, (unsigned int) length,
			"smlistsh", &smlistsh_partition) == Refused)
	{
		puts("psm_manage failed.");
	}
}

#if defined (VXWORKS) || defined (RTEMS)
int	smlistsh(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
#else
int	main(int argc, char **argv)
#endif
{
	unsigned long	keyValue;
	char		line[256];
	int		count;
	char		command;
	long		arg1;
	long		arg2;
	PsmUsageSummary	summary;
	PsmAddress	elt;

	sm_ipc_init();
	while (1)
	{
		printf(": ");
		if (fgets(line, 256, stdin) == NULL)
		{
			perror("fgets failed");
			break;
		}

		count = sscanf(line, "%c %ld %ld", &command, &arg1, &arg2);
		switch (count)
		{
		case 0:
			puts("Empty line ignored.");
			continue;

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
				printf("List is at %ld.\n", smlistsh_list);
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
					printf("element at %ld contains %ld.\n",
						elt,
						sm_list_data(smlistsh_partition,
						elt));
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

				if (sm_list_insert_first(smlistsh_partition,
					smlistsh_list, (PsmAddress) arg1) == 0)
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

				if (sm_list_insert_last(smlistsh_partition,
					smlistsh_list, (PsmAddress) arg1) == 0)
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

				elt = sm_list_search(smlistsh_partition,
					sm_list_first(smlistsh_partition,
					smlistsh_list), NULL, (void *) arg1);
				printf("value %ld is in element at %ld.\n",
						arg1, elt);

			case 'd':
				if (smlistsh_list == 0)
				{
					puts("do not have a list");
					break;
				}

				sm_list_delete(smlistsh_partition,
					(PsmAddress) arg1, NULL, NULL);
				break;

			default:
				puts("invalid command");
			}

			continue;

		case 3:
			switch (command)
			{
			case '+':
				attach(arg1, arg2);
				continue;
			}

			/*	Deliberate fall-through to default.	*/

		default:
			puts("invalid command");
		}
	}

	return 0;
}
