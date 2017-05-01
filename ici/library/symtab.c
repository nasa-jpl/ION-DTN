/*
	symtab.c:	baseline private symbol table for ION stack
			with definition of sm_FindFunction(), which 
			accesses this table.
			
			Note that this is a canonical private symbol
			table, provided as a model for mission-
			specific symbol tables.  A mission desiring
			to override this file should place a copy
			of it in a mission-specific "include"
			directory, modify that copy, and modify
			the ici Makefile to ensure that the compiler
			looks for symtab.c in that mission-specific
			include directory rather than in ici/library.

	Author: Scott Burleigh, JPL

	Copyright (c) 2006, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/

extern int	ionadmin(int, int, int, int, int, int, int, int, int, int);
extern int	ionexit(int, int, int, int, int, int, int, int, int, int);
extern int	ltpadmin(int, int, int, int, int, int, int, int, int, int);
extern int	bpadmin(int, int, int, int, int, int, int, int, int, int);
extern int	ipnadmin(int, int, int, int, int, int, int, int, int, int);
extern int	rfxclock(int, int, int, int, int, int, int, int, int, int);
extern int	ltpclock(int, int, int, int, int, int, int, int, int, int);
extern int	ltpmeter(int, int, int, int, int, int, int, int, int, int);
extern int	bpclock(int, int, int, int, int, int, int, int, int, int);
extern int	bptransit(int, int, int, int, int, int, int, int, int, int);
extern int	bpclm(int, int, int, int, int, int, int, int, int, int);
extern int	ipnfw(int, int, int, int, int, int, int, int, int, int);
extern int	ltpcli(int, int, int, int, int, int, int, int, int, int);
extern int	ltpclo(int, int, int, int, int, int, int, int, int, int);
extern int	udplsi(int, int, int, int, int, int, int, int, int, int);
extern int	udplso(int, int, int, int, int, int, int, int, int, int);
extern int	aoslsi(int, int, int, int, int, int, int, int, int, int);
extern int	aoslso(int, int, int, int, int, int, int, int, int, int);
extern int	udpcli(int, int, int, int, int, int, int, int, int, int);
extern int	udpclo(int, int, int, int, int, int, int, int, int, int);
extern int	tcpcli(int, int, int, int, int, int, int, int, int, int);
extern int	tcpclo(int, int, int, int, int, int, int, int, int, int);
extern int	brsccla(int, int, int, int, int, int, int, int, int, int);
extern int	brsscla(int, int, int, int, int, int, int, int, int, int);
extern int	ipnadminep(int, int, int, int, int, int, int, int, int, int);

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
		{ "ionexit",	(FUNCPTR) ionexit,	ICI_PRIORITY,	32768 },
		{ "ltpadmin",	(FUNCPTR) ltpadmin,	ICI_PRIORITY,	32768 },
		{ "bpadmin",	(FUNCPTR) bpadmin,	ICI_PRIORITY,	32768 },
		{ "ipnadmin",	(FUNCPTR) ipnadmin,	ICI_PRIORITY,	32768 },
		{ "rfxclock",	(FUNCPTR) rfxclock,	ICI_PRIORITY,	32768 },
		{ "ltpclock",	(FUNCPTR) ltpclock,	ICI_PRIORITY,	32768 },
		{ "ltpmeter",	(FUNCPTR) ltpmeter,	ICI_PRIORITY,	32768 },
		{ "bpclock",	(FUNCPTR) bpclock,	ICI_PRIORITY,	4096  },
		{ "bptransit",	(FUNCPTR) bptransit,	ICI_PRIORITY,	4096  },
		{ "bpclm",	(FUNCPTR) bpclm,	ICI_PRIORITY,	4096  },
		{ "ipnfw",	(FUNCPTR) ipnfw,	ICI_PRIORITY,	65536 },
		{ "ltpcli",	(FUNCPTR) ltpcli,	ICI_PRIORITY,	32768 },
		{ "ltpclo",	(FUNCPTR) ltpclo,	ICI_PRIORITY,	32768 },
		{ "udplsi",	(FUNCPTR) udplsi,	ICI_PRIORITY,	32768 },
		{ "udplso",	(FUNCPTR) udplso,	ICI_PRIORITY,	32768 },
		{ "aoslsi",	(FUNCPTR) aoslsi,	ICI_PRIORITY,	32768 },
		{ "aoslso",	(FUNCPTR) aoslso,	ICI_PRIORITY,	32768 },
		{ "udpcli",	(FUNCPTR) udpcli,	ICI_PRIORITY,	32768 },
		{ "udpclo",	(FUNCPTR) udpclo,	ICI_PRIORITY,	32768 },
		{ "tcpcli",	(FUNCPTR) tcpcli,	ICI_PRIORITY,	32768 },
		{ "tcpclo",	(FUNCPTR) tcpclo,	ICI_PRIORITY,	32768 },
		{ "brsccla",	(FUNCPTR) brsccla,	ICI_PRIORITY,	32768 },
		{ "brsscla",	(FUNCPTR) brsscla,	ICI_PRIORITY,	32768 },
		{ "ipnadminep",	(FUNCPTR) ipnadminep,	ICI_PRIORITY,	24576 }
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
