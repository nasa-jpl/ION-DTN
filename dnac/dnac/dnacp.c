/*
	dnacp.c:	DTN node auto-configuration utility.

	Author: Scott Burleigh, IPNGROUP

	Copyright (c) 2021, IPNGROUP.  ALL RIGHTS RESERVED.
	
									*/
#include "ionsec.h"
#include "crypto.h"
#include "ipnfw.h"
#include "dtka.h"

#ifndef	INITIAL_CONTACT_DURATION
#define	INITIAL_CONTACT_DURATION	(604800)	/*	1 week.	*/
#endif

#ifndef	INITIAL_CONTACT_RATE
#define	INITIAL_CONTACT_RATE		(10000000)
#endif

#define	MAX_SCRIPT_CMDS			(7)

static char	nodeListDir[256];

static int	announceNewContacts(uint32_t regionNbr, uvast newNode)
{
	time_t		fromTime = getCtime();
	time_t		toTime = fromTime + INITIAL_CONTACT_DURATION;
	uvast		self = getOwnFqnn();
	size_t		xmitRate = INITIAL_CONTACT_RATE;
	PsmAddress	cxaddr;

	/*	Announce registration of new node.			*/

	if (rfx_insert_contact(regionNbr, MAX_POSIX_TIME, MAX_POSIX_TIME,
			newNode, newNode, 0, 1.0, &cxaddr, 1) != 0)
	{
		putErrmsg("dnacp can't announce registration contact.", NULL);
		return -1;
	}

	/*	Announce loopback contact for new node.			*/

	if (rfx_insert_contact(regionNbr, fromTime, fromTime + 315360000,
			newNode, newNode, 1000000000, 1.0, &cxaddr, 1) != 0)
	{
		putErrmsg("dnacp can't announce registration contact.", NULL);
		return -1;
	}

	/*	Announce bidirectional contact with sponsor node.	*/

	if (rfx_insert_contact(regionNbr, fromTime, toTime,
			self, newNode, xmitRate, 1.0, &cxaddr, 1) != 0)
	{
		putErrmsg("dnacp can't announce contact to new node.", NULL);
		return -1;
	}

	if (rfx_insert_contact(regionNbr, fromTime, toTime,
			newNode, self, xmitRate, 1.0, &cxaddr, 1) != 0)
	{
		putErrmsg("dnacp can't announce contact from new node.", NULL);
		return -1;
	}

	return 0;
}

static int	writeIonrcFile(uint32_t regionNbr, uvast nodeNbr)
{
	FILE	*rcfile;
	char	cmd[1024];

	rcfile = fopen("node.ionrc", "w");
	if (rcfile == NULL)
	{
		putSysErrmsg("dnacp can't create node.ionrc file.", NULL);
		return -1;
	}

	isprintf(cmd, sizeof cmd, "^ %lu\n", regionNbr);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("dnacp can't write to node.ionrc", NULL);
		return -1;
	}

	isprintf(cmd, sizeof cmd, "1 " UVAST_FIELDSPEC " node.ionconfig\n",
			nodeNbr);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("dnacp can't write to node.ionrc", NULL);
		return -1;
	}

	fclose(rcfile);
	return 0;
}

static void	scanSpecIonconfigFile(FILE *specFile, char *sdrName,
			int *configFlags, long *heapWords, int *heapKey,
			int *logSize, int *logKey, char *sdrPathName,
			long *wmSize, int *wmKey)
{
	char		cmd[80];
	int		cmdlen;
	char		*cursor;
	char		*token;

	while (1)
	{
		if (fgets(cmd, sizeof cmd, specFile) == NULL)
		{
			return;
		}

		cmdlen = strlen(cmd);
		if (cmdlen == 0 || *(cmd + (cmdlen - 1)) != '\n')
		{
			continue;	/*	Not a valid command.	*/
		}

		cursor = cmd;
		findToken(&cursor, &token);
		if (token == NULL)
		{
			continue;
		}

		if (strcmp(token, "sdrName") == 0)
		{
			findToken(&cursor, &token);
			if (token)
			{
				istrcpy(sdrName, token, 32);
			}

			continue;
		}

		if (strcmp(token, "configFlags") == 0)
		{
			findToken(&cursor, &token);
			if (token)
			{
				*configFlags = atoi(token);
			}

			continue;
		}

		if (strcmp(token, "heapWords") == 0)
		{
			findToken(&cursor, &token);
			if (token)
			{
				*heapWords = strtol(token, NULL, 10);
			}

			continue;
		}

		if (strcmp(token, "heapKey") == 0)
		{
			findToken(&cursor, &token);
			if (token)
			{
				*heapKey = atoi(token);
			}

			continue;
		}

		if (strcmp(token, "logSize") == 0)
		{
			findToken(&cursor, &token);
			if (token)
			{
				*logSize = atoi(token);
			}

			continue;
		}

		if (strcmp(token, "logKey") == 0)
		{
			findToken(&cursor, &token);
			if (token)
			{
				*logKey = atoi(token);
			}

			continue;
		}

		if (strcmp(token, "pathName") == 0)
		{
			findToken(&cursor, &token);
			if (token)
			{
				istrcpy(sdrPathName, token, 32);
			}

			continue;
		}

		if (strcmp(token, "wmSize") == 0)
		{
			findToken(&cursor, &token);
			if (token)
			{
				*wmSize = strtol(token, NULL, 10);
			}

			continue;
		}

		if (strcmp(token, "wmKey") == 0)
		{
			findToken(&cursor, &token);
			if (token)
			{
				*wmKey = atoi(token);
			}

			continue;
		}
	}
}

static int	writeIonconfigFile(uvast nodeNbr, char *sdrName,
			int *configFlags, long *heapWords, int *heapKey,
			int *logSize, int *logKey, char *sdrPathName,
			long *wmSize, int *wmKey)
{
	FILE	*rcfile;
	int	addlConfigFlags;
	FILE	*specFile;
	char	cmd[1024];

	rcfile = fopen("node.ionconfig", "w");
	if (rcfile == NULL)
	{
		putSysErrmsg("dnacp can't create node.ionconfig file.", NULL);
		return -1;
	}

	*sdrName = 0;
       	addlConfigFlags = *configFlags;
	*configFlags = 0;
	*heapWords = 0;
	*heapKey = 0;
	*logSize = 0;
	*logKey = 0;
	*sdrPathName = 0;
	*wmSize = 0;
	*wmKey = 0;
	specFile = fopen("spec.ionconfig", "r");
	if (specFile)
	{
		scanSpecIonconfigFile(specFile, sdrName, configFlags,
				heapWords, heapKey, logSize, logKey,
				sdrPathName, wmSize, wmKey);
		fclose(specFile);
	}

	/*	SDR name.						*/

	if (*sdrName == 0)
	{
		isprintf(sdrName, 32, "ion" UVAST_FIELDSPEC, nodeNbr);
	}

	isprintf(cmd, sizeof cmd, "sdrName %s\n", sdrName);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("dnacp can't write to node.ionrc", NULL);
		return -1;
	}

	/*	Config flags.						*/

	if (*configFlags == 0)
	{
		*configFlags = SDR_IN_DRAM | SDR_REVERSIBLE | SDR_BOUNDED;
	}

	*configFlags |= addlConfigFlags;
	isprintf(cmd, sizeof cmd, "configFlags %d\n", *configFlags);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("dnacp can't write to node.ionrc", NULL);
		return -1;
	}

	/*	Heap words.						*/

	if (*heapWords == 0)
	{
		*heapWords = 10000000;
	}

	isprintf(cmd, sizeof cmd, "heapWords %ld\n", *heapWords);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("dnacp can't write to node.ionrc", NULL);
		return -1;
	}

	/*	Heap key.						*/

	if (*heapKey == 0)
	{
		*heapKey = SM_NO_KEY;
	}

	isprintf(cmd, sizeof cmd, "heapKey %d\n", *heapKey);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("dnacp can't write to node.ionrc", NULL);
		return -1;
	}

	/*	Log size.						*/

	isprintf(cmd, sizeof cmd, "logSize %d\n", *logSize);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("dnacp can't write to node.ionrc", NULL);
		return -1;
	}

	/*	Log key.						*/

	if (*logKey == 0)
	{
		*logKey = SM_NO_KEY;
	}

	isprintf(cmd, sizeof cmd, "logKey %d\n", *logKey);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("dnacp can't write to node.ionrc", NULL);
		return -1;
	}

	/*	Path name.						*/

	if (*sdrPathName == 0)
	{
		isprintf(sdrPathName, 32, "%ctmp", ION_PATH_DELIMITER);
	}

	isprintf(cmd, sizeof cmd, "pathName %s\n", sdrPathName);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("dnacp can't write to node.ionconfig", NULL);
		return -1;
	}

	/*	Working memory size.					*/

	isprintf(cmd, sizeof cmd, "wmSize %d\n", *wmSize);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("dnacp can't write to node.ionrc", NULL);
		return -1;
	}

	/*	Working memory key.					*/

	if (*wmKey == 0)
	{
		*wmKey = nodeNbr;
	}

	isprintf(cmd, sizeof cmd, "wmKey %ld\n", *wmKey);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("dnacp can't write to node.ionrc", NULL);
		return -1;
	}

	fclose(rcfile);
	return 0;
}

static int	writeIonsecrcFile(void)
{
	FILE	*rcfile;
	FILE	*specFile;
	char	cmd[1024];
	int	cmdlen;

	rcfile = fopen("node.ionsecrc", "w");
	if (rcfile == NULL)
	{
		putSysErrmsg("dnacp can't create node.ionsecrc file.", NULL);
		return -1;
	}

	/*	Initialize.						*/

	istrcpy(cmd, "1\n", sizeof cmd);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("dnacp can't write to node.ionsecrc", NULL);
		return -1;
	}

	/*	Add pre-placed keys.					*/

	specFile = fopen("spec.ionsecrc", "r");
	if (specFile)
	{
		while (1)
		{
			if (fgets(cmd, sizeof cmd, specFile) == NULL)
			{
				break;
			}

			cmdlen = strlen(cmd);
			if (cmdlen == 0 || *(cmd + (cmdlen - 1)) != '\n')
			{
				continue;	/*	Not a command.	*/
			}

			if (*cmd != 'a')
			{
				continue;	/*	Not adding key.	*/
			}

			if (fwrite(cmd, cmdlen, 1, rcfile) < 1)
			{
				fclose(specFile);
				fclose(rcfile);
				putSysErrmsg("dnacp can't write node.ionsecrc",
						NULL);
				return -1;
			}
		}

		fclose(specFile);
	}

	fclose(rcfile);
	return 0;
}

static int	writeLtprcFile(void)
{
	FILE	*rcfile;
	FILE	*specFile;
	char	cmd[1024];
	int	cmdlen;

	rcfile = fopen("node.ltprc", "w");
	if (rcfile == NULL)
	{
		putSysErrmsg("dnacp can't create node.ltprc file.", NULL);
		return -1;
	}

	specFile = fopen("spec.ltprc", "r");
	if (specFile)
	{
		while (1)
		{
			if (fgets(cmd, sizeof cmd, specFile) == NULL)
			{
				break;
			}

			cmdlen = strlen(cmd);
			if (cmdlen == 0 || *(cmd + (cmdlen - 1)) != '\n')
			{
				continue;	/*	Not a command.	*/
			}

			switch (*cmd)
			{
			case '1':		/*	Initialize.	*/
			case 'a':		/*	Add span.	*/
			case 'm':		/*	Manage engine.	*/
				break;

			default:
				continue;
			}

			if (fwrite(cmd, cmdlen, 1, rcfile) < 1)
			{
				fclose(specFile);
				fclose(rcfile);
				putSysErrmsg("dnacp can't write to node.ltprc",
						NULL);
				return -1;
			}
		}

		fclose(specFile);
	}

	fclose(rcfile);
	return 0;
}

static int	writeBssprcFile(void)
{
	FILE	*rcfile;
	FILE	*specFile;
	char	cmd[1024];
	int	cmdlen;

	rcfile = fopen("node.bssprc", "w");
	if (rcfile == NULL)
	{
		putSysErrmsg("dnacp can't create node.bssprc file.", NULL);
		return -1;
	}

	specFile = fopen("spec.bssprc", "r");
	if (specFile)
	{
		while (1)
		{
			if (fgets(cmd, sizeof cmd, specFile) == NULL)
			{
				break;
			}

			cmdlen = strlen(cmd);
			if (cmdlen == 0 || *(cmd + (cmdlen - 1)) != '\n')
			{
				continue;	/*	Not a command.	*/
			}

			switch (*cmd)
			{
			case '1':		/*	Initialize.	*/
			case 'a':		/*	Add span.	*/
			case 'm':		/*	Manage engine.	*/
				break;

			default:
				continue;
			}

			if (fwrite(cmd, cmdlen, 1, rcfile) < 1)
			{
				fclose(specFile);
				fclose(rcfile);
				putSysErrmsg("dnacp can't write to node.bssprc",
						NULL);
				return -1;
			}
		}

		fclose(specFile);
	}

	fclose(rcfile);
	return 0;
}

static int	writeBprcFile(uvast nodeNbr, char *sponsorInductName,
			char *childHostName, unsigned short childPortNbr)
{
	FILE	*rcfile;
	FILE	*specFile;
	char	cmd[1024];
	int	cmdlen;

	rcfile = fopen("node.bprc", "w");
	if (rcfile == NULL)
	{
		putSysErrmsg("dnacp can't create node.bprc file.", NULL);
		return -1;
	}

	/*	initialize						*/

	istrcpy(cmd, "1\n", sizeof cmd);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("dnacp can't write to node.bprc", NULL);
		return -1;
	}

	/*	ipn scheme						*/

	istrcpy(cmd, "a scheme ipn ipnfw ipnadminep\n", sizeof cmd);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("dnacp can't write to node.bprc", NULL);
		return -1;
	}

	isprintf(cmd, sizeof cmd, "a endpoint ipn:" UVAST_FIELDSPEC ".0 x\n",
			nodeNbr);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("dnacp can't write to node.bprc", NULL);
		return -1;
	}

	isprintf(cmd, sizeof cmd, "a endpoint ipn:" UVAST_FIELDSPEC ".1 x\n",
			nodeNbr);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("dnacp can't write to node.bprc", NULL);
		return -1;
	}

	isprintf(cmd, sizeof cmd, "a endpoint ipn:" UVAST_FIELDSPEC ".271 q\n",
			nodeNbr);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("dnacp can't write to node.bprc", NULL);
		return -1;
	}

	/*	imc scheme						*/

	istrcpy(cmd, "a scheme imc imcfw imcadminep\n", sizeof cmd);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("dnacp can't write to node.bprc", NULL);
		return -1;
	}

	istrcpy(cmd, "a endpoint imc:0.1 q\n", sizeof cmd);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("dnacp can't write to node.bprc", NULL);
		return -1;
	}

	istrcpy(cmd, "a endpoint imc:0.271 q\n", sizeof cmd);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("dnacp can't write to node.bprc", NULL);
		return -1;
	}

	istrcpy(cmd, "a endpoint imc:203.0 q\n", sizeof cmd);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("dnacp can't write to node.bprc", NULL);
		return -1;
	}

	/*	stcp protocol, induct, and two outducts		*/

	istrcpy(cmd, "a protocol stcp\n", sizeof cmd);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("dnacp can't write to node.bprc", NULL);
		return -1;
	}

	isprintf(cmd, sizeof cmd, "a induct stcp %s:%u stcpcli\n",
			childHostName, childPortNbr);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("dnacp can't write to node.bprc", NULL);
		return -1;
	}

	isprintf(cmd, sizeof cmd, "a outduct stcp %s:%u stcpclo\n",
			childHostName, childPortNbr);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("dnacp can't write to node.bprc", NULL);
		return -1;
	}

	isprintf(cmd, sizeof cmd, "a outduct stcp %s stcpclo\n",
			sponsorInductName);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("dnacp can't write to node.bprc", NULL);
		return -1;
	}

	/*	Loopback egress plan.					*/

	isprintf(cmd, sizeof cmd, "a plan ipn:" UVAST_FIELDSPEC ".0\n",
			nodeNbr);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("dnacp can't write to node.bprc", NULL);
		return -1;
	}

	isprintf(cmd, sizeof cmd, "a planduct ipn:" UVAST_FIELDSPEC ".0 stcp \
%s:%u\n", nodeNbr, childHostName, childPortNbr);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("dnacp can't write to node.bprc", NULL);
		return -1;
	}

	/*	Egress plan to sponsor node				*/

	isprintf(cmd, sizeof cmd, "a plan ipn:" UVAST_FIELDSPEC ".0\n",
			getOwnFqnn());
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("dnacp can't write to node.bprc", NULL);
		return -1;
	}

	isprintf(cmd, sizeof cmd, "a planduct ipn:" UVAST_FIELDSPEC ".0 stcp \
%s\n", getOwnFqnn(), sponsorInductName);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("dnacp can't write to node.bprc", NULL);
		return -1;
	}

	/*	predefined configuration elements			*/

	specFile = fopen("spec.bprc", "r");
	if (specFile)
	{
		while (1)
		{
			if (fgets(cmd, sizeof cmd, specFile) == NULL)
			{
				break;
			}

			cmdlen = strlen(cmd);
			if (cmdlen == 0 || *(cmd + (cmdlen - 1)) != '\n')
			{
				continue;	/*	Not a command.	*/
			}

			switch (*cmd)
			{
			case 'a':		/*	Add something.	*/
			case 'g':		/*	Add gateway.	*/
			case 'm':		/*	Manage BPA.	*/
			case 'w':		/*	Watch chars.	*/
				break;

			default:
				continue;
			}

			if (fwrite(cmd, cmdlen, 1, rcfile) < 1)
			{
				fclose(specFile);
				fclose(rcfile);
				putSysErrmsg("dnacp can't write to node.bprc",
						NULL);
				return -1;
			}
		}

		fclose(specFile);
	}

	fclose(rcfile);
	return 0;
}

static int	writeTccrcFile(void)
{
	FILE	*rcfile;
	FILE	*specFile;
	char	cmd[1024];
	int	cmdlen;

	rcfile = fopen("node.tccrc", "w");
	if (rcfile == NULL)
	{
		putSysErrmsg("dnacp can't create node.tccrc file.", NULL);
		return -1;
	}

	/*	Initialize.						*/

	istrcpy(cmd, "1 6 50 .2\n", sizeof cmd);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("dnacp can't write to node.tccrc", NULL);
		return -1;
	}

	/*	Predefined configuration.				*/

	specFile = fopen("spec.tccrc", "r");
	if (specFile)
	{
		while (1)
		{
			if (fgets(cmd, sizeof cmd, specFile) == NULL)
			{
				break;
			}

			cmdlen = strlen(cmd);
			if (cmdlen == 0 || *(cmd + (cmdlen - 1)) != '\n')
			{
				continue;	/*	Not a command.	*/
			}

			if (*cmd != 'm')
			{
				continue;	/*	No configure.	*/
			}

			if (fwrite(cmd, cmdlen, 1, rcfile) < 1)
			{
				fclose(specFile);
				fclose(rcfile);
				putSysErrmsg("dnacp can't write node.tccrc",
						NULL);
				return -1;
			}
		}

		fclose(specFile);
	}

	fclose(rcfile);
	return 0;
}

static int	writeDtkarcFile(void)
{
	FILE	*rcfile;
	FILE	*specFile;
	char	cmd[1024];
	int	cmdlen;

	rcfile = fopen("node.dtkarc", "w");
	if (rcfile == NULL)
	{
		putSysErrmsg("dnacp can't create node.dtkarc file.", NULL);
		return -1;
	}

	/*	Initialize.						*/

	istrcpy(cmd, "1\n", sizeof cmd);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("dnacp can't write to node.dtkarc", NULL);
		return -1;
	}

	/*	Predefined configuration.				*/

	specFile = fopen("spec.dtkarc", "r");
	if (specFile)
	{
		while (1)
		{
			if (fgets(cmd, sizeof cmd, specFile) == NULL)
			{
				break;
			}

			cmdlen = strlen(cmd);
			if (cmdlen == 0 || *(cmd + (cmdlen - 1)) != '\n')
			{
				continue;	/*	Not a command.	*/
			}

			if (*cmd != 'm')
			{
				continue;	/*	No configure.	*/
			}

			if (fwrite(cmd, cmdlen, 1, rcfile) < 1)
			{
				fclose(specFile);
				fclose(rcfile);
				putSysErrmsg("dnacp can't write node.dtkarc",
						NULL);
				return -1;
			}
		}

		fclose(specFile);
	}

	fclose(rcfile);
	return 0;
}

static int	writeCfdprcFile(void)
{
	FILE	*rcfile;
	FILE	*specFile;
	char	cmd[1024];
	int	cmdlen;

	rcfile = fopen("node.cfdprc", "w");
	if (rcfile == NULL)
	{
		putSysErrmsg("dnacp can't create node.cfdprc file.", NULL);
		return -1;
	}

	specFile = fopen("spec.cfdprc", "r");
	if (specFile)
	{
		while (1)
		{
			if (fgets(cmd, sizeof cmd, specFile) == NULL)
			{
				break;
			}

			cmdlen = strlen(cmd);
			if (cmdlen == 0 || *(cmd + (cmdlen - 1)) != '\n')
			{
				continue;	/*	Not a command.	*/
			}

			switch (*cmd)
			{
			case '1':		/*	Initialize.	*/
			case 'a':		/*	Add entity.	*/
			case 'm':		/*	Manage entity.	*/
				break;

			default:
				continue;
			}

			if (fwrite(cmd, cmdlen, 1, rcfile) < 1)
			{
				fclose(specFile);
				fclose(rcfile);
				putSysErrmsg("dnacp can't write to node.cfdprc",
						NULL);
				return -1;
			}
		}

		fclose(specFile);
	}

	return 0;
}

static int	constructScripts(char **startCmds, char **stopCmds)
{
	FILE	*ionstartScript;
	FILE	*ionstopScript;
	int	i;
	char	*dtkaCmd = "dtka &";

	ionstartScript = fopen("ionstart", "w");
	if (ionstartScript == NULL)
	{
		putSysErrmsg("dnacp can't open ionstart script", NULL);
		return -1;
	}

	ionstopScript = fopen("ionstop", "w");
	if (ionstopScript == NULL)
	{
		fclose(ionstartScript);
		putSysErrmsg("dnacp can't open ionstop script", NULL);
		return -1;
	}

	for (i = 0; i < MAX_SCRIPT_CMDS; i++)
	{
		if (startCmds[i])
		{
			if (fwrite(startCmds[i], strlen(startCmds[i]), 1,
					ionstartScript) < 1)
			{
				fclose(ionstartScript);
				fclose(ionstopScript);
				putSysErrmsg("dnacp can't write script", NULL);
				return -1;
			}
		}

		if (stopCmds[i])
		{
			if (fwrite(stopCmds[i], strlen(stopCmds[i]), 1,
					ionstopScript) < 1)
			{
				fclose(ionstartScript);
				fclose(ionstopScript);
				putSysErrmsg("dnacp can't write script", NULL);
				return -1;
			}
		}
	}

	fclose(ionstopScript);
	if (fwrite(dtkaCmd, strlen(dtkaCmd), 1, ionstartScript) < 1)
	{
		fclose(ionstartScript);
		putSysErrmsg("dnacp can't write script", NULL);
		return -1;
	}

	/*	TODO: add a command that starts ipnd, passing it an
	 *	appropriately constructed ipnd configuration file.	*/

	fclose(ionstartScript);

	/*	Make scripts executable.				*/

	if (chmod("ionstart", 00777) < 0)
	{
		putSysErrmsg("dnacp can't make ionstart executable.", NULL);
		return -1;
	}

	if (chmod("ionstop", 00777) < 0)
	{
		putSysErrmsg("dnacp can't make ionstop executable.", NULL);
		return -1;
	}

	return 0;
}

static int	getPublicKey(int sock, unsigned short bufLen,
			unsigned char *publicKey, unsigned short *keyLen,
			time_t *effectiveTime, time_t *assertionTime)
{
	int		newSocket;
	struct sockaddr	dnaccSocketName;
	socklen_t	dnaccNameLength = sizeof dnaccSocketName;
	int		bytesReceived;

	/*	Accept only a single connection.  dnaccNameLength must be
	 *	initialized to the address buffer size; passing an
	 *	uninitialized value makes accept() fail with EINVAL.	*/

	newSocket = accept(sock, &dnaccSocketName, &dnaccNameLength);
	closesocket(sock);
	if (newSocket < 0)
	{
		putSysErrmsg("dnacp accept() failed", NULL);
		return -1;
	}

	/*	Receive node's public key parameters.			*/

	bytesReceived = itcp_recv(&newSocket, (char *) effectiveTime,
			sizeof(time_t));
	if (bytesReceived < 1)
	{
		closesocket(newSocket);
		putSysErrmsg("dnacp key effective time reception failed",
				itoa(bytesReceived));
		return -1;
	}

	*effectiveTime = ntohl(*effectiveTime);
	bytesReceived = itcp_recv(&newSocket, (char *) assertionTime,
			sizeof(time_t));
	if (bytesReceived < 1)
	{
		closesocket(newSocket);
		putSysErrmsg("dnacp key assertion time reception failed",
				itoa(bytesReceived));
		return -1;
	}

	*assertionTime = ntohl(*assertionTime);
	bytesReceived = itcp_recv(&newSocket, (char *) keyLen,
			sizeof(unsigned short));
	if (bytesReceived < 1)
	{
		closesocket(newSocket);
		putSysErrmsg("dnacp key length reception failed",
				itoa(bytesReceived));
		return -1;
	}

	*keyLen = ntohs(*keyLen);
	if (*keyLen > bufLen)
	{
		closesocket(newSocket);
		putErrmsg("dnacp key length is invalid", itoa(*keyLen));
		return -1;
	}

	bytesReceived = itcp_recv(&newSocket, (char *) publicKey, *keyLen);
	closesocket(newSocket);
	if (bytesReceived < 1)
	{
		putSysErrmsg("dnacp public key reception failed",
				itoa(bytesReceived));
		return -1;
	}

	return 0;
}

static int	assertPublicKey(uvast nodeNbr, time_t effectiveTime,
			time_t assertionTime, unsigned short keyLen,
			unsigned char *publicKey)
{
	Sdr	sdr = getIonsdr();
	char	ownEid[32];
	BpSAP	sap;
	char	recordBuffer[TC_MAX_REC];
	int	recordLen;
	Object	extent;
	Object	bundleZco;
	char	destEid[32];
	Object	newBundle;
	int	result;

	isprintf(ownEid, sizeof ownEid, "ipn:" UVAST_FIELDSPEC ".0",
			getOwnFqnn());
	if (bp_open_source(ownEid, &sap, 0) < 0)
	{
		putErrmsg("Can't open own endpoint.", ownEid);
		return -1;
	}

	if (sec_addPublicKey(nodeNbr, effectiveTime, assertionTime, keyLen,
			publicKey) < 0)
	{
		bp_close(sap);
		putErrmsg("Can't add new node's public key.", itoa(nodeNbr));
		return -1;
	}

	recordLen = tc_serialize(recordBuffer, sizeof recordBuffer, nodeNbr,
			effectiveTime, assertionTime, keyLen, publicKey);
	if (recordLen < 0)
	{
		bp_close(sap);
		putErrmsg("Can't serialize key declaration record.", NULL);
		return -1;
	}

	CHKERR(sdr_begin_xn(sdr));
	extent = sdr_malloc(sdr, recordLen);
	if (extent)
	{
		sdr_write(sdr, extent, (char *) recordBuffer, recordLen);
	}

	if (sdr_end_xn(sdr) < 0)
	{
		bp_close(sap);
		putErrmsg("Can't create ZCO extent.", NULL);
		return -1;
	}

	bundleZco = ionCreateZco(ZcoSdrSource, extent, 0, recordLen,
			BP_STD_PRIORITY, 0, ZcoOutbound, NULL);
	if (bundleZco == 0 || bundleZco == (Object) -1)
	{
		bp_close(sap);
		putErrmsg("Can't create ZCO.", NULL);
		return -1;
	}

	isprintf(destEid, sizeof destEid, "imc:%d.0", DTKA_DECLARE);
	result = bp_send(sap, destEid, NULL, 432000, BP_STD_PRIORITY,
			NoCustodyRequested, 0, 0, NULL, bundleZco, &newBundle);
	bp_close(sap);
	if (result < 1)
	{
		putErrmsg("Can't publish key declaration bundle.", NULL);
		return -1;
	}

	return 0;
}

#if defined (ION_LWT)
int	dnacp(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char			*dirName = a1 ? (char *) a1 : ".";
	uvast			nodeNbr = a2 ? strtol((char *) a2, NULL, 10): 0;
	char			*pathname = a3 ? (char *) a3 : "";
	char			*hostname = a4 ? (char *) a4 : "";
	char			*passwdPathName = a5 ? (char *) a5 : "";
#else
int	main(int argc, char *argv[])
{
	char			*dirName = (argc > 1 ? argv[1] : ".");
	uvast			nodeNbr = (argc > 2 ?
					strtol(argv[2], NULL, 10) : 0);
	char			*pathname = (argc > 3 ? argv[3] : "");
	char			*hostname = (argc > 4 ? argv[4] : "");
	char			*passwdPathName = (argc > 5 ? argv[5] : "");
#endif
	char			envString[256];
	IonDB			iondb;
	uint32_t		regionNbr;
	time_t			currentTime;
	char			*childHostName;
	unsigned short		childPortNbr;
	VInduct			*vinduct;
	PsmAddress		vductElt;
	struct sockaddr		socketName;
	struct sockaddr_in	*inetName;
	unsigned int		hostNbr;
	int			sock;
	unsigned short		dnacpPortNbr;
	socklen_t		socketNameLen;
	struct stat		statbuf;
	char			*startCmds[MAX_SCRIPT_CMDS];
	char			*stopCmds[MAX_SCRIPT_CMDS];
	int			i;
	int			j;
	char			contactsFileName[64];
	char			passagewaysFileName[64];
	char			sdrName[32];
	int			configFlags;
	long			heapWords;
	int			heapKey;
	int			logSize;
	int			logKey;
	char			sdrPathName[MAXPATHLEN];
	long			wmSize;
	int			wmKey;
	char			*restartCmd = "ionrestart";
	int			ltp = 0;
	int			bssp = 0;
	int			cfdp = 0;
	char			cmd[1024];
	char			adminEid[MAX_EID_LEN + 1];
	char			ductname[MAX_CL_DUCT_NAME_LEN];
	VOutduct		*voutduct;
	unsigned short		keyLen;
	unsigned char		publicKey[16000];
	time_t			effectiveTime;
	time_t			assertionTime;

	if (dirName)
	{
		istrcpy(nodeListDir, dirName, sizeof nodeListDir);
	}
	else
	{
		istrcpy(nodeListDir, ".", sizeof nodeListDir);
	}

	isprintf(envString, sizeof envString, "ION_NODE_LIST_DIR=%s",
			nodeListDir);
	putenv(envString);
	if (dtkaAttach() < 0)
	{
		putErrmsg("dnacp can't attach to DTKA system.", NULL);
		return 1;
	}

	sdr_read(getIonsdr(), (char *) &iondb, getIonDbObject(), sizeof(IonDB));
	regionNbr = iondb.regions[0].regionNbr;
	if (regionNbr == 0)
	{
		putErrmsg("dnacp fails: home region number unknown.", NULL);
		return 1;
	}

	/*	Compute default values.					*/

	if (nodeNbr == 0)	/*	Not provided by user.		*/
	{
		/*	Must make up a node number.  Use DTN time.	*/

		currentTime = getCtime() - EPOCH_2000_SEC;
		nodeNbr = regionNbr;
		nodeNbr <<= 32;
		nodeNbr += currentTime;
	}

	if (hostname)
	{
		if (*hostname == 0)
		{
			hostname = NULL;
		}
	}

	if (hostname)
	{
		childHostName = hostname;
	}
	else
	{
		childHostName = "localhost";
	}

	childPortNbr = (nodeNbr % 8192) + 16384;

	/*	Find sponsor node's STCP induct.			*/

	findInduct("stcp", NULL, &vinduct, &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("dnacp can't find sponsor node's stcp induct.",
				itoa(nodeNbr));
		return 1;
	}

	/*	Prepare to listen for new node's public key on a
	 *	randomly chosen TCP port number.			*/

	memset((char *) &socketName, 0, sizeof(struct sockaddr));
	inetName = (struct sockaddr_in *) &socketName;
	inetName->sin_family = AF_INET;
	hostNbr = getInternetAddress("localhost");
	hostNbr = htonl(hostNbr);
	memcpy((char *) &(inetName->sin_addr.s_addr), (char *) &hostNbr, 4);
	inetName->sin_port = 0;		/*	Any available port.	*/
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0)
	{
		putSysErrmsg("Can't open DNAC TCP socket", NULL);
		return 1;
	}

	socketNameLen = sizeof(struct sockaddr);
	if (bind(sock, &socketName, socketNameLen) < 0)
	{
		closesocket(sock);
		putSysErrmsg("Can't bind DNAC TCP socket", NULL);
		return 1;
	}

	socketNameLen = sizeof(struct sockaddr);
	if (getsockname(sock, &socketName, &socketNameLen) < 0)
	{
		closesocket(sock);
		putSysErrmsg("Can't get port number for DNAC TCP socket", NULL);
		return 1;
	}

	dnacpPortNbr = inetName->sin_port;
	dnacpPortNbr = ntohs(dnacpPortNbr);

	/*	Now add own outduct and plan for transmission to the
	 *	new node as prescribed by the new contacts.		*/

	snooze(1);
	isprintf(adminEid, sizeof adminEid, "ipn:" UVAST_FIELDSPEC ".0",
			nodeNbr);
	if (addPlan(adminEid, 0) < 0)
	{
		closesocket(sock);
		putErrmsg("dnacp can't add egress plan for new node.",
				itoa(nodeNbr));
		return 1;
	}

	isprintf(ductname, sizeof ductname, "%s:%u", childHostName,
			childPortNbr);
	if (addOutduct("stcp", ductname, "stcpclo", 0) < 0)
	{
		closesocket(sock);
		putErrmsg("dnacp can't add outduct to new node.", itoa(nodeNbr));
		return 1;
	}

	findOutduct("stcp", ductname, &voutduct, &vductElt);
	if (vductElt == 0)
	{
		closesocket(sock);
		putErrmsg("dnacp can't find newly added outduct to new node.",
				itoa(nodeNbr));
		return 1;
	}

	if (bpStartOutduct("stcp", ductname) < 1)
	{
		closesocket(sock);
		putErrmsg("dnacp can't start newly added outduct to new node.",
				itoa(nodeNbr));
		return 1;
	}
	
	if (attachPlanDuct(adminEid, voutduct->outductElt) < 0)
	{
		closesocket(sock);
		putErrmsg("dnacp can't add plan duct for new node.",
				itoa(nodeNbr));
		return 1;
	}

	if (bpStartPlan(adminEid) < 1)
	{
		closesocket(sock);
		putErrmsg("dnacp can't start egress plan for new node.",
				itoa(nodeNbr));
		return 1;
	}

	/*	Announce new node's registration contact and own
	 *	bidirectional contact with new node, enabling
	 *	generation of a current contact plan for that node.	*/

	if (announceNewContacts(regionNbr, nodeNbr) < 0)
	{
		closesocket(sock);
		putErrmsg("dnacp failed announcing new node's contacts", NULL);
		return 1;
	}

	/*	Changes to the state of the sponsor node are now
	 *	complete.  Next, navigate to the directory in which
	 *	the new node will be instantiated.			*/

	if (chdir(pathname) < 0)
	{
		if (errno == ENOENT)	/*	No preloaded config.	*/
		{
			if (mkdir(pathname, S_IRWXU | S_IRWXG | S_IRWXO) < 0)
			{
				closesocket(sock);
				putSysErrmsg("dnacp node directory name \
invalid", pathname);
				return 1;
			}

			if (chdir(pathname) < 0)
			{
				closesocket(sock);
				putSysErrmsg("dnacp node directory name \
invalid", pathname);
				return 1;
			}
		}
		else
		{
			closesocket(sock);
			putSysErrmsg("dnacp node directory name invalid",
					pathname);
			return 1;
		}
	}

	/*	Construct all configuration files for the new node.	*/

	memset((char *) startCmds, 0, sizeof startCmds);
	memset((char *) stopCmds, 0, sizeof stopCmds);
	i = 0;
	j = MAX_SCRIPT_CMDS - 1;
	if (hostname)
	{
		/*	SDR must be configured with a copy in file
		 *	so that node state can be transmitted to the
		 *	destination machine.				*/

		configFlags = 2;	/*	Will be OR'd.		*/
	}

	/*	First, the ionrc config and initialization files.	*/

	oK(unlink("node.ionrc"));
	if (writeIonrcFile(regionNbr, nodeNbr) < 0)
	{
		closesocket(sock);
		putErrmsg("dnacp can't write ionrc file.", NULL);
		return 1;
	}

	startCmds[i] = "ionadmin !\n";
	stopCmds[j] = "ionadmin .\n";
	i++;
	j--;

	oK(unlink("node.ionconfig"));
	if (writeIonconfigFile(nodeNbr, sdrName, &configFlags, &heapWords,
		&heapKey, &logSize, &logKey, sdrPathName, &wmSize, &wmKey) < 0)
	{
		closesocket(sock);
		putErrmsg("dnacp can't write ionconfig file.", NULL);
		return 1;
	}

	/*	(Including the updated contact plan.)			*/

	isprintf(contactsFileName, sizeof contactsFileName,
			"contacts.%lu.ionrc", regionNbr);
	oK(unlink(contactsFileName));
	rfx_brief_contacts(regionNbr);

	isprintf(passagewaysFileName, sizeof passagewaysFileName,
			"passageways.%lu.ionrc", regionNbr);
	oK(unlink(passagewaysFileName));
	rfx_brief_passageways(regionNbr);

	oK(unlink("ranges.ionrc"));
	rfx_brief_ranges();

	/*	Then ionsecrc.						*/

	oK(unlink("node.ionsecrc"));
	if (writeIonsecrcFile() < 0)
	{
		closesocket(sock);
		putErrmsg("dnacp can't write ionsecrc file.", NULL);
		return 1;
	}

	/*	Possibly ltprc.						*/

	oK(unlink("node.ltprc"));
	if (stat("./spec.ltprc", &statbuf) == 0)
	{
		if (writeLtprcFile() < 0)
		{
			closesocket(sock);
			putErrmsg("dnacp can't write ltprc file.", NULL);
			return 1;
		}

		ltp = 1;
		startCmds[i] = "ltpadmin !\n";
		stopCmds[j] = "ltpadmin .\n";
		i++;
		j--;
	}

	/*	Possibly bssprc.					*/

	oK(unlink("node.bssprc"));
	if (stat("./spec.bssprc", &statbuf) == 0)
	{
		if (writeBssprcFile() < 0)
		{
			closesocket(sock);
			putErrmsg("dnacp can't write bssprc file.", NULL);
			return 1;
		}

		bssp = 1;
		startCmds[i] = "bsspadmin !\n";
		stopCmds[j] = "bsspadmin .\n";
		i++;
		j--;
	}

	/*	Then bprc.						*/

	oK(unlink("node.bprc"));
	if (writeBprcFile(nodeNbr, vinduct->ductName, childHostName,
			childPortNbr) < 0)
	{
		closesocket(sock);
		putErrmsg("dnacp can't write bprc file.", NULL);
		return 1;
	}

	startCmds[i] = "bpadmin !\n";
	stopCmds[j] = "bpadmin .\n";
	i++;
	j--;

	/*	Then tccrc.						*/

	oK(unlink("node.tccrc"));
	if (writeTccrcFile() < 0)
	{
		closesocket(sock);
		putErrmsg("dnacp can't write tccrc file.", NULL);
		return 1;
	}

	startCmds[i] = "tccadmin 203 !\n";
	stopCmds[j] = "tccadmin 203 .\n";
	i++;
	j--;

	/*	Then dtkarc.						*/

	oK(unlink("node.dtkcarc"));
	if (writeDtkarcFile() < 0)
	{
		closesocket(sock);
		putErrmsg("dnacp can't write dtkarc file.", NULL);
		return 1;
	}

	/*	And possibly cfdprc.					*/

	oK(unlink("node.cfdprc"));
	if (stat("./spec.cfdprc", &statbuf) == 0)
	{
		if (writeCfdprcFile() < 0)
		{
			closesocket(sock);
			putErrmsg("dnacp can't write cfdprc file.", NULL);
			return 1;
		}

		cfdp = 1;
		startCmds[i] = "cfdpadmin !\n";
		stopCmds[j] = "cfdpadmin .\n";
		i++;
		j--;
	}

	/*	Now instantiate the new node, WITHOUT starting it.	*/

	if (pseudoshell("ionadmin node.ionrc") < 0)
	{
		closesocket(sock);
		putErrmsg("dnacp can't load node.ionrc.", NULL);
		return 1;
	}

	snooze(1);
	isprintf(cmd, sizeof cmd, "ionadmin %s", contactsFileName);
	if (pseudoshell(cmd) < 0)
	{
		closesocket(sock);
		putErrmsg("dnacp can't load contacts.", contactsFileName);
		return 1;
	}

	snooze(1);
	isprintf(cmd, sizeof cmd, "ionadmin %s", passagewaysFileName);
	if (pseudoshell(cmd) < 0)
	{
		closesocket(sock);
		putErrmsg("dnacp can't load passageways.", passagewaysFileName);
		return 1;
	}

	snooze(1);
	if (pseudoshell("ionadmin ranges.ionrc") < 0)
	{
		closesocket(sock);
		putErrmsg("dnacp can't load ranges.", NULL);
		return 1;
	}

	snooze(1);
	if (pseudoshell("ionsecadmin node.ionsecrc") < 0)
	{
		closesocket(sock);
		putErrmsg("dnacp can't load node.ionsecrc.", NULL);
		return 1;
	}

	if (ltp)
	{
		snooze(1);
		if (pseudoshell("ltpadmin node.ltprc") < 0)
		{
			closesocket(sock);
			putErrmsg("dnacp can't load node.ltprc.", NULL);
			return 1;
		}
	}

	if (bssp)
	{
		snooze(1);
		if (pseudoshell("bsspadmin node.bssprc") < 0)
		{
			closesocket(sock);
			putErrmsg("dnacp can't load node.bssprc.", NULL);
			return 1;
		}
	}

	snooze(1);
	if (pseudoshell("bpadmin node.bprc") < 0)
	{
		closesocket(sock);
		putErrmsg("dnacp can't load node.bprc.", NULL);
		return 1;
	}

	snooze(1);
	if (pseudoshell("tccadmin 203 node.tccrc") < 0)
	{
		closesocket(sock);
		putErrmsg("dnacp can't load node.tccrc.", NULL);
		return 1;
	}

	snooze(1);
	if (pseudoshell("dtkaadmin node.dtkarc") < 0)
	{
		closesocket(sock);
		putErrmsg("dnacp can't load node.dtkarc.", NULL);
		return 1;
	}

	if (cfdp)
	{
		snooze(1);
		if (pseudoshell("cfdpadmin node.cfdprc") < 0)
		{
			closesocket(sock);
			putErrmsg("dnacp can't load node.cfdprc.", NULL);
			return 1;
		}
	}

	/*	Now start dnacc for the new node and wait for it to
	 *	send back the new node's public key.			*/

	if (listen(sock, 1) < 0)
	{
		closesocket(sock);
		putSysErrmsg("Can't listen on DNAC TCP socket", NULL);
		return 1;
	}

	isprintf(cmd, sizeof cmd, "dnacc %u %s &", dnacpPortNbr,
			passwdPathName);
	if (pseudoshell(cmd) < 0)
	{
		closesocket(sock);
		putErrmsg("dnacp can't run dnacc.", NULL);
		return 1;
	}

	if (getPublicKey(sock, sizeof publicKey, publicKey, &keyLen, 
			&effectiveTime, &assertionTime) < 0)
	{
		putErrmsg("dnacp can't get new node's public key from dnacc.",
				NULL);
		return 1;
	}

	/*	Send the new node's public key to the DTKA public
	 *	key authority.						*/

	if (assertPublicKey(nodeNbr, effectiveTime, assertionTime, keyLen,
			publicKey) < 0)
	{
		putErrmsg("dnacp failed asserting new node's public key", NULL);
		return 1;
	}

	/*	Finally, start the new node.				*/

	if (constructScripts(startCmds, stopCmds) < 0)
	{
		closesocket(sock);
		putErrmsg("dnacp can't construct admin scripts.", NULL);
		return 1;
	}

	if (hostname)	/*	Start new node on remote machine.	*/
	{
		/*	Make sure the required directories exist.	*/

		isprintf(cmd, sizeof cmd, "ssh ion@%s mkdir %s", hostname,
				pathname);
		oK(pseudoshell(cmd));

		isprintf(cmd, sizeof cmd, "ssh ion@%s mkdir %s", hostname,
				sdrPathName);
		oK(pseudoshell(cmd));

		/*	Copy the start and stop scripts to the home
		 *	directory for this node on the remote machine.	*/

		isprintf(cmd, sizeof cmd, "scp -P ionstart ion@%s:%s", hostname,
				pathname);
		oK(pseudoshell(cmd));

		isprintf(cmd, sizeof cmd, "scp -P ionstop ion@%s:%s", hostname,
				pathname);
		oK(pseudoshell(cmd));

		/*	Copy the in-file representation of the SDR data
	 	*	space (the node's state) to the remote machine.	*/

		isprintf(cmd, sizeof cmd, "scp -P %s%c%s.sdr ion@%s:%s%c%s.sdr",
			sdrPathName, ION_PATH_DELIMITER, sdrName,
	       		hostname, sdrPathName, ION_PATH_DELIMITER, sdrName);
		oK(pseudoshell(cmd));

		/*	Reload node's SDR profile on the remote machine.*/

		isprintf(cmd, sizeof cmd,
			"ssh ion@%s cd %s; sdrmend %s %d %ld %d %d %d %s %s",
	       		hostname, pathname, sdrName, configFlags, heapWords,
			heapKey, logSize, logKey, sdrPathName, restartCmd);
		oK(pseudoshell(cmd));

		/*	Now start all of the daemons.			*/

		isprintf(cmd, sizeof cmd, "ssh ion@%s cd %s; .%cionstart",
	       		hostname, pathname, ION_PATH_DELIMITER);
		oK(pseudoshell(cmd));
	}
	else	/*	Otherwise start daemons on the local machine.	*/
	{
		isprintf(cmd, sizeof cmd, ".%cionstart", ION_PATH_DELIMITER);
		oK(pseudoshell(cmd));
	}

	/*	Finish node auto-configuration.				*/

	writeErrmsgMemos();
	writeMemo("[i] dnacp has ended.");
	ionDetach();
	return 0;
}
