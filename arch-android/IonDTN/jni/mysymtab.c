/*
	mysymtab.c:	template private symbol table for RTEMS port
			of the ION stack, with definition of
			sm_FindFunction(), which accesses this table.

	Author: Scott Burleigh, JPL

	Copyright (c) 2010, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/

extern int	ionadmin(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr);
extern int	rfxclock(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr);
extern int	ionsecadmin(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr);
extern int	ionwarn(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr);
extern int	ionunlock(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr);
extern int	bpadmin(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr);
extern int	bpclock(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr);
extern int	bptransit(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr);
extern int	bpclm(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr);
extern int	ipnadmin(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr);
extern int	ipnfw(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr);
extern int	ipnadminep(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr);
extern int	udpcli(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr);
extern int	udpclo(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr);
extern int	tcpcli(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr);
extern int	tcpclo(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr);
extern int	dgrcli(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr);
extern int	dgrclo(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr);
extern int	lgagent(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr);
extern int	dtn2adminep(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr);
extern int	dtn2admin(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr);
extern int	ipndadmin(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr);


#if 0
extern int	ionexit(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr);
extern int	ionrestart(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr);
extern int	ltpadmin(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr);
extern int	ltpclock(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr);
extern int	ltpdeliv(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr);
extern int	ltpmeter(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr);
extern int	ltpcli(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr);
extern int	ltpclo(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr);
extern int	bpsource(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr);
extern int	bpsink(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr);
#ifndef NASA_PROTECTED_FLIGHT_CODE
extern int	cfdpadmin(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr);
extern int	cfdpclock(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr);
extern int	bputa(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr);
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
		{ "lgagent",	(FUNCPTR) lgagent,	ICI_PRIORITY,	24576 },
		{ "dtn2adminep",	(FUNCPTR) dtn2adminep,	ICI_PRIORITY,	24576 },
		{ "dtn2admin",	(FUNCPTR) dtn2admin,	ICI_PRIORITY,	24576 },
		{ "ipndadmin",	(FUNCPTR) ipndadmin,	ICI_PRIORITY,	24576 }
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
