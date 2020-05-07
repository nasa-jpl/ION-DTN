/*
	smrbtsh.c:	test program for shared-memory red-black trees.
									*/
/*									*/
/*	Copyright (c) 2011, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <smrbt.h>

#define	TEST_MEM_SIZE	(1000000)

#if SMRBT_DEBUG
extern int	printTree(PsmPartition partition, PsmAddress rbt);
extern int	treeBroken(PsmPartition partition, PsmAddress rbt);
#endif

static int	stopped(int *newState)
{
	static int	state = 0;

	if (newState)
	{
		state = *newState;
	}

	return state;
}

static void	handleQuit(int signum)
{
	int	stop = 1;

	oK(stopped(&stop));
}

static void	printUsage()
{
	PUTS("Valid commands are:");
	PUTS("\tq\tQuit");
	PUTS("\th\tHelp");
	PUTS("\t?\tHelp");
	PUTS("\ts\tSeed");
	PUTS("\t   s [<seed value for rand(); default is current time>]");
	PUTS("\tr\tRandom");
	PUTS("\t   r [<nbr of random values to insert into tree; default 1>]");
	PUTS("\ti\tInsert");
	PUTS("\t   i <value to insert into tree>");
	PUTS("\tf\tFind");
	PUTS("\t   f <value to locate in tree>");
	PUTS("\td\tDelete");
	PUTS("\t   d <value to delete from tree>");
	PUTS("\tp\tPrint entire tree");
	PUTS("\t   p");
	PUTS("\tk\tCheck tree for errors");
	PUTS("\t   k");
	PUTS("\tl\tList nodes in order");
	PUTS("\t   l");
	PUTS("\t#\tComment");
	PUTS("\t   # <comment text>");
}

static int	compareNodes(PsmPartition partition, PsmAddress nodeData,
			void *dataBuffer)
{
	return ((unsigned long) nodeData) - *((unsigned long *) dataBuffer);
}

static void	destroyNode(PsmPartition partition, PsmAddress nodeData,
			void *argument)
{
	unsigned int	data;

	data = (unsigned long) nodeData;
	PUTMEMO("Destroying node", utoa(data)); 
}

static int	processLine(PsmPartition partition, PsmAddress rbt, char *line,
			int lineLength)
{
	int			tokenCount;
	char			*cursor;
	int			i;
	char			*tokens[9];
	static unsigned int	seed;
	unsigned long		data;
	unsigned int		count;
	PsmAddress		node;
	PsmAddress		next;
	unsigned long		prevdata;
	char			*memo = "";

	tokenCount = 0;
	for (cursor = line, i = 0; i < 9; i++)
	{
		if (*cursor == '\0')
		{
			tokens[i] = NULL;
		}
		else
		{
			findToken(&cursor, &(tokens[i]));
			tokenCount++;
		}
	}

	if (tokenCount == 0)
	{
		return 0;
	}

	/*	Skip over any trailing whitespace.			*/

	while (isspace((int) *cursor))
	{
		cursor++;
	}

	/*	Make sure we've parsed everything.			*/

	if (*cursor != '\0')
	{
		PUTS("Too many tokens.");
		return 0;
	}

	/*	Have parsed the command.  Now execute it.		*/

	switch (*(tokens[0]))		/*	Command code.		*/
	{
		case 0:			/*	Empty line.		*/
		case '#':		/*	Comment.		*/
			return 0;

		case '?':
		case 'h':
			printUsage();
			return 0;

		case 's':		/*	Seed.			*/
			if (tokenCount < 2)
			{
				seed = time(NULL);
			}
			else
			{
				seed = strtol(tokens[1], NULL, 0);
				if (seed == 0)
				{
					seed = time(NULL);
				}
			}

			srand(seed);
			return 0;

		case 'r':		/*	Insert random node(s).	*/
			if (tokenCount < 2)
			{
				count = 1;
			}
			else
			{
				count = strtol(tokens[1], NULL, 0);
				if (count == 0)
				{
					count = 1;
				}
			}

			while (count > 0)
			{
				data = rand_r(&seed);
				if (sm_rbt_insert(partition, rbt, data,
						compareNodes, &data) == 0)
				{
					PUTS("Insertion failed");
					break;
				}
#if SMRBT_DEBUG
if (treeBroken(partition, rbt)) PUTS("Tree is broken.");
#endif
				count--;
			}

			return 0;

		case 'i':		/*	Insert specific value.	*/
			if (tokenCount < 2)
			{
				PUTS("Insert what?");
			}
			else
			{
				data = strtol(tokens[1], NULL, 0);
				if (sm_rbt_insert(partition, rbt, data,
						compareNodes, &data) == 0)
				{
					PUTS("Insertion failed");
				}
#if SMRBT_DEBUG
if (treeBroken(partition, rbt)) PUTS("Tree is broken.");
#endif
			}

			return 0;

		case 'f':		/*	Find node by value.	*/
			if (tokenCount < 2)
			{
				PUTS("Find what?");
			}
			else
			{
				data = strtol(tokens[1], NULL, 0);
				node = sm_rbt_search(partition, rbt,
						compareNodes, &data, &next);
				PUTMEMO("Node address", utoa(node));
				PUTMEMO("Successor address", utoa(next));
			}

			return 0;

		case 'd':		/*	Delete node by value.	*/
			if (tokenCount < 2)
			{
				PUTS("Delete what?");
			}
			else
			{
				data = strtol(tokens[1], NULL, 0);
				node = sm_rbt_search(partition, rbt,
						compareNodes, &data, NULL);
				if (node == 0)
				{
					PUTS("Node not found.");
				}
				else
				{
					sm_rbt_delete(partition, rbt,
							compareNodes, &data,
							destroyNode, NULL);
					PUTS("Node deleted.");
#if SMRBT_DEBUG
if (treeBroken(partition, rbt)) PUTS("Tree is broken.");
#endif
				}
			}

			return 0;

		case 'p':
#if SMRBT_DEBUG
printTree(partition, rbt);
#else
			PUTS("Rebuild with -DSMRBT_DEBUG=1 and try again.");
#endif
			return 0;

		case 'k':
#if SMRBT_DEBUG
if (treeBroken(partition, rbt))
{
	PUTS("Tree is corrupt.");
}
else
{
	PUTS("Tree is okay.");
}
#else
			PUTS("Rebuild with -DSMRBT_DEBUG=1 and try again.");
#endif
			return 0;

		case 'l':
			node = sm_rbt_first(partition, rbt);
			if (node == 0)
			{
				PUTS("Empty tree.");
				return 0;
			}

			prevdata = (unsigned long) sm_rbt_data(partition, node);
			data = prevdata;
			while (1)
			{
				if (data < prevdata)
				{
					memo = " <-- out of order";
				}
				else
				{
					memo = "";
				}

				PUTMEMO(utoa(data), memo);
				prevdata = data;
				node = sm_rbt_next(partition, node);
				if (node == 0)
				{
					break;
				}

				data = (unsigned long) sm_rbt_data(partition,
						node);
			}

			return 0;

		case 'q':
			return -1;	/*	End program.		*/

		default:
			PUTS("Invalid command.  Enter '?' for help.");
			return 0;
	}
}

static int	run_smrbtsh(char *cmdFileName)
{
	int		length = TEST_MEM_SIZE;
	unsigned char	*allocation = NULL;
	PsmPartition	partition = NULL;
	uaddr		partitionId;
	PsmMgtOutcome	outcome;
	PsmAddress	rbt;
	int		cmdFile;
	char		line[256];
	int		len;

	sm_ipc_init();
	if (sm_ShmAttach(SM_NO_KEY, length, (char **) &allocation,
			&partitionId) == ERROR)
	{
		PUTS("sm_ShmAttach failed.");
		return 0;
	}

	if (psm_manage((char *) allocation, (unsigned int) length, "smrbtsh",
			&partition, &outcome) < 0 || outcome == Refused)
	{
		sm_ShmDetach((char *) allocation);
		PUTS("psm_manage failed.");
		return 0;
	}

	rbt = sm_rbt_create(partition);
	if (rbt == 0)
	{
		sm_ShmDetach((char *) allocation);
		PUTS("Can't create red-black table.");
		return 0;
	}

	if (cmdFileName == NULL)		/*	Interactive.	*/
	{
		cmdFile = fileno(stdin);
		isignal(SIGINT, handleQuit);
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

				PUTS("igets failed.");
				break;		/*	Out of loop.	*/
			}

			if (len == 0)
			{
				continue;
			}

			if (processLine(partition, rbt, line, len))
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
			PERROR("Can't open command file");
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

					PUTS("igets failed.");
					break;		/*	Loop.	*/
				}

				if (len == 0
				|| line[0] == '#')	/*	Comment.*/
				{
					continue;
				}

				if (processLine(partition, rbt, line, len))
				{
					break;	/*	Out of loop.	*/
				}
			}

			close(cmdFile);
		}
	}

	PUTS("Stopping smrbtsh.");
	sm_rbt_destroy(partition, rbt, destroyNode, NULL);
	sm_ShmDetach((char *) allocation);
	sm_ShmDestroy(partitionId);
	return 0;
}

#if defined (ION_LWT)
int	smrbtsh(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*cmdFileName = (char *) a1;
#else
int	main(int argc, char **argv)
{
	char	*cmdFileName = (argc > 1 ? argv[1] : NULL);
#endif
	int	result;

#ifdef FSWLOGGER
	if (cmdFileName == NULL)
	{
		PUTS("Can't run smrbtsh interactively: need stdin.");
		return 0;			/*	No stdin.	*/
	}
#endif
	result = run_smrbtsh(cmdFileName);
	if (result < 0)
	{
		puts("smrbtsh failed.");
		return 1;
	}

	return 0;
}
