/*
	mysymtab.c:	template private symbol table for RTEMS port
			of the ION stack, with definition of
			sm_FindFunction(), which accesses this table.
			
	Author: Scott Burleigh, JPL

	Copyright (c) 2010, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/

extern int	ionadmin(int, int, int, int, int, int, int, int, int, int);
extern int	rfxclock(int, int, int, int, int, int, int, int, int, int);
extern int	ionsecadmin(int, int, int, int, int, int, int, int, int, int);
extern int	ionwarn(int, int, int, int, int, int, int, int, int, int);
extern int	ionunlock(int, int, int, int, int, int, int, int, int, int);
extern int	bpadmin(int, int, int, int, int, int, int, int, int, int);
extern int	bpclock(int, int, int, int, int, int, int, int, int, int);
extern int	bptransit(int, int, int, int, int, int, int, int, int, int);
extern int	bpclm(int, int, int, int, int, int, int, int, int, int);
extern int	ipnadmin(int, int, int, int, int, int, int, int, int, int);
extern int	ipnfw(int, int, int, int, int, int, int, int, int, int);
extern int	ipnadminep(int, int, int, int, int, int, int, int, int, int);
extern int	udpcli(int, int, int, int, int, int, int, int, int, int);
extern int	udpclo(int, int, int, int, int, int, int, int, int, int);
extern int	tcpcli(int, int, int, int, int, int, int, int, int, int);
extern int	tcpclo(int, int, int, int, int, int, int, int, int, int);
extern int	dgrcli(int, int, int, int, int, int, int, int, int, int);
extern int	dgrclo(int, int, int, int, int, int, int, int, int, int);
extern int	lgagent(int, int, int, int, int, int, int, int, int, int);

#if 0
extern int	ionexit(int, int, int, int, int, int, int, int, int, int);
extern int	ionrestart(int, int, int, int, int, int, int, int, int, int);
extern int	ltpadmin(int, int, int, int, int, int, int, int, int, int);
extern int	ltpclock(int, int, int, int, int, int, int, int, int, int);
extern int	ltpdeliv(int, int, int, int, int, int, int, int, int, int);
extern int	ltpmeter(int, int, int, int, int, int, int, int, int, int);
extern int	ltpcli(int, int, int, int, int, int, int, int, int, int);
extern int	ltpclo(int, int, int, int, int, int, int, int, int, int);
extern int	bpsource(int, int, int, int, int, int, int, int, int, int);
extern int	bpsink(int, int, int, int, int, int, int, int, int, int);
#ifndef NASA_PROTECTED_FLIGHT_CODE
extern int	cfdpadmin(int, int, int, int, int, int, int, int, int, int);
extern int	cfdpclock(int, int, int, int, int, int, int, int, int, int);
extern int	bputa(int, int, int, int, int, int, int, int, int, int);
#endif
#endif

typedef struct
{
	char	*name;
	FUNCPTR	funcPtr;
	int	priority;
	int	stackSize;
} SymTabEntry;

FUNCPTR	sm_FindFunction(char *name, int *priority, int *stackSize)
{
	static SymTabEntry	symbols[] =
	{
		{ "ionadmin",	(FUNCPTR) ionadmin,	ICI_PRIORITY,	32768 },
		{ "rfxclock",	(FUNCPTR) rfxclock,	ICI_PRIORITY,	32768 },
		{ "ionsecadmin",(FUNCPTR) ionsecadmin,	ICI_PRIORITY,	32768 },
		{ "ionwarn",	(FUNCPTR) ionwarn,	ICI_PRIORITY,	32768 },
		{ "ionunlock",	(FUNCPTR) ionunlock,	ICI_PRIORITY,	32768 },
		{ "bpadmin",	(FUNCPTR) bpadmin,	ICI_PRIORITY,	32768 },
		{ "bpclock",	(FUNCPTR) bpclock,	ICI_PRIORITY,	4096  },
		{ "bptransit",	(FUNCPTR) bptransit,	ICI_PRIORITY,	24576 },
		{ "bpclm",	(FUNCPTR) bpclm,	ICI_PRIORITY,	32768 },
		{ "ipnadmin",	(FUNCPTR) ipnadmin,	ICI_PRIORITY,	32768 },
		{ "ipnfw",	(FUNCPTR) ipnfw,	ICI_PRIORITY,	65536 },
		{ "ipnadminep",	(FUNCPTR) ipnadminep,	ICI_PRIORITY,	24576 },
		{ "udpcli",	(FUNCPTR) udpcli,	ICI_PRIORITY,	32768 },
		{ "udpclo",	(FUNCPTR) udpclo,	ICI_PRIORITY,	32768 },
		{ "tcpcli",	(FUNCPTR) tcpcli,	ICI_PRIORITY,	32768 },
		{ "tcpclo",	(FUNCPTR) tcpclo,	ICI_PRIORITY,	32768 },
		{ "dgrcli",	(FUNCPTR) dgrcli,	ICI_PRIORITY,	32768 },
		{ "dgrclo",	(FUNCPTR) dgrclo,	ICI_PRIORITY,	32768 },
		{ "lgagent",	(FUNCPTR) lgagent,	ICI_PRIORITY,	24576 }
#if 0
		{ "ionexit",	(FUNCPTR) ionexit,	ICI_PRIORITY,	32768 },
		{ "ionrestart",	(FUNCPTR) ionrestart,	ICI_PRIORITY,	32768 },
		{ "ltpadmin",	(FUNCPTR) ltpadmin,	ICI_PRIORITY,	32768 },
		{ "ltpclock",	(FUNCPTR) ltpclock,	ICI_PRIORITY,	32768 },
		{ "ltpdeliv",	(FUNCPTR) ltpdeliv,	ICI_PRIORITY,	32768 },
		{ "ltpmeter",	(FUNCPTR) ltpmeter,	ICI_PRIORITY,	32768 },
		{ "ltpcli",	(FUNCPTR) ltpcli,	ICI_PRIORITY,	32768 },
		{ "ltpclo",	(FUNCPTR) ltpclo,	ICI_PRIORITY,	32768 },
		{ "bpsource",	(FUNCPTR) bpsource,	ICI_PRIORITY,	4096  },
		{ "bpsink",	(FUNCPTR) bpsink,	ICI_PRIORITY,	4096  }
#ifndef NASA_PROTECTED_FLIGHT_CODE
		,{ "cfdpadmin",	(FUNCPTR) cfdpadmin,	ICI_PRIORITY,	24576 },
		{ "cfdpclock",	(FUNCPTR) cfdpclock,	ICI_PRIORITY,	24576 },
		{ "bputa",	(FUNCPTR) bputa,	ICI_PRIORITY,	24576 }
#endif
#endif
	};

	static int	numSymbols = sizeof symbols / sizeof(SymTabEntry);
	int		i;

	CHKNULL(name);
	CHKNULL(priority);
	CHKNULL(stackSize);
	for (i = 0; i < numSymbols; i++)
	{
		if (strcmp(name, symbols[i].name) == 0)
		{
			if (*priority == 0)	/*	Use default.	*/
			{
				*priority = symbols[i].priority;
			}

			if (*stackSize == 0)	/*	Use default.	*/
			{
				*stackSize = symbols[i].stackSize;
			}

			return symbols[i].funcPtr;
		}
	}

	return NULL;
}
