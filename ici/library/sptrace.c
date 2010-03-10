/*
 *	sptrace.c:	space utilization trace system.
 *
 *	Copyright (c) 2002, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 *
 *	Modification History:
 *	Date	  Who	What
 *	11-30-02  SCB	Original development.
 */

#include "platform.h"
#include "smlist.h"
#include "sptrace.h"

/*	Operation types for tracing	*/
#define OP_ALLOCATE	1
#define OP_FREE		2
#define OP_MEMO		3

typedef struct
{
	int		traceSmId;	/*	For shared memory ops.	*/
	char		name[32];	/*	for concurrent tracing	*/
	int		opCount;	/*	count of operations	*/
	PsmAddress	files;		/*	SmList source filenames	*/
	PsmAddress	log;		/*	SmList of TraceItems	*/
} TraceHeader;

typedef struct
{
	int		taskId;
	PsmAddress	fileName;
	int		lineNbr;
	int		opNbr;
	int		opType;
	unsigned long	objectAddress;
	int		objectSize;
	int		refTaskId;
	PsmAddress	refFileName;
	int		refLineNbr;
	int		refOpNbr;
	PsmAddress	msg;
} TraceItem;

static void	sptracePrint(char *txt)
{
	char	buffer[256];
	int	len;

	memset(buffer, 0, sizeof buffer);
	len = sprintf(buffer, "sptrace (pid %d) ", sm_TaskIdSelf());
	strncpy(buffer + len, txt, sizeof buffer - (len + 1));
	putErrmsg(buffer, NULL);
}

PsmPartition	sptrace_start(int smkey, int smsize, char *sm,
			PsmPartition trace, char *name)
{
	int		nameLen;
	int		smid;
	TraceHeader	*trh;
	PsmAddress	traceHeaderAddress;

	REQUIRE(trace);
	REQUIRE(smsize > 0);
	REQUIRE(name);
	if ((nameLen = strlen(name)) < 1 || nameLen > 31)
	{
		errno = EINVAL;
		sptracePrint("start: name must be 1-31 characters.");
		return NULL;
	}

	/*	Attach to shared memory used for trace operations.	*/

	if (sm_ShmAttach(smkey, smsize, &sm, &smid) < 0)
	{
		sptracePrint("start: can't attach shared memory for trace");
		return NULL;
	}

	/*	Manage the shared memory region.  "Trace" argument
	 *	is normally NULL.					*/

	switch (psm_manage(sm, smsize, name, &trace))
	{
	case Refused:
		sptracePrint("start: can't psm_manage shared memory");
		return NULL;

	case Redundant:
		trh = (TraceHeader *) psp(trace, psm_get_root(trace));
		if (trh == NULL || strcmp(name, trh->name) != 0)
		{
			errno = EINVAL;
			sptracePrint("start: shared memory used otherwise.");
			return NULL;
		}

		return trace;		/*	Trace already started.	*/

	default:
		break;
	}

	/*	Initialize the shared memory region for tracing.	*/

	traceHeaderAddress = psm_zalloc(trace, sizeof(TraceHeader));
	if (traceHeaderAddress == 0)
	{
		sptracePrint("start: not enough memory for header.");
		sm_ShmDetach(sm);
		sm_ShmDestroy(smid);
		return NULL;
	}

	oK(psm_set_root(trace, traceHeaderAddress));
	trh = (TraceHeader *) psp(trace, traceHeaderAddress);
	REQUIRE(trh);
	trh->traceSmId = smid;
	memset(trh->name, 0, sizeof trh->name);
	strcpy(trh->name, name);
	trh->opCount = 0;
	trh->files = sm_list_create(trace);
	if (trh->files == 0)
	{
		sptracePrint("start: not enough memory for files list.");
		sm_ShmDetach(sm);
		sm_ShmDestroy(smid);
		return NULL;
	}

	trh->log = sm_list_create(trace);
	if (trh->log == 0)
	{
		sptracePrint("start: not enough memory for log.");
		sm_ShmDetach(sm);
		sm_ShmDestroy(smid);
		return NULL;
	}

	return trace;
}

PsmPartition	sptrace_join(int smkey, int smsize, char *sm,
			PsmPartition trace, char *name)
{
	int		nameLen;
	int		smid;
	TraceHeader	*trh;

	REQUIRE(trace);
	REQUIRE(smsize > 0);
	REQUIRE(name);
	if ((nameLen = strlen(name)) < 1 || nameLen > 31)
	{
		errno = EINVAL;
		sptracePrint("start: name must be 1-31 characters.");
		return NULL;
	}

	/*	Attach to shared memory used for trace operations.	*/

	if (sm_ShmAttach(smkey, smsize, (char **) &sm, &smid) < 0)
	{
		sptracePrint("join: can't attach shared memory for trace");
		return NULL;
	}

	/*	Examine the shared memory region.			*/

	switch (psm_manage(sm, smsize, name, &trace))
	{
	case Refused:
		sptracePrint("join: can't psm_manage shared memory");
		return NULL;

	case Redundant:
		trh = (TraceHeader *) psp(trace, psm_get_root(trace));
		if (trh == NULL || strcmp(name, trh->name) != 0)
		{
			errno = EINVAL;
			sptracePrint("join: shared memory used otherwise.");
			return NULL;
		}

		return trace;		/*	Have joined the trace.	*/

	default:
		break;
	}

	errno = EINVAL;
	sptracePrint("join: trace episode not yet started.");
	return NULL;
}

static void	discardEvent(PsmPartition trace)
{
	char	buffer[256];

	sprintf(buffer, "sptrace (pid %d) logging: not enough memory for \
trace, ignoring event.", sm_TaskIdSelf());
	writeMemo(buffer);
}

static PsmAddress	findFileName(PsmPartition trace, TraceHeader *trh,
				char *sourceFileName)
{
	PsmAddress	elt;
	PsmAddress	filenameAddress;
	char		*filename;

	for (elt = sm_list_first(trace, trh->files); elt;
			elt = sm_list_next(trace, elt))
	{
		filenameAddress = sm_list_data(trace, elt);
		filename = (char *) psp(trace, filenameAddress);
		if (strcmp(filename, sourceFileName) == 0)
		{
			return filenameAddress;
		}
	}

	/*	This is a source file we haven't heard of before.	*/

	filenameAddress = psm_zalloc(trace, strlen(sourceFileName) + 1);
	if (filenameAddress == 0)
	{
		discardEvent(trace);
		return 0;
	}

	/*	Add this source file to the list for the trace.		*/

	strcpy((char *) psp(trace, filenameAddress), sourceFileName);
	if (sm_list_insert_last(trace, trh->files, filenameAddress) == 0)
	{
		discardEvent(trace);
		return 0;
	}

	return filenameAddress;
}

static void	logEvent(PsmPartition trace, int opType, unsigned long
			addr, int objectSize, char *msg, char *fileName,
			int lineNbr, PsmAddress *eltp)
{
	PsmAddress	traceHeaderAddress;
	TraceHeader	*trh;
	PsmAddress	itemAddr;
	TraceItem	*item;
	PsmAddress	elt;

	if (eltp)
	{
		*eltp = 0;
	}

	traceHeaderAddress = psm_get_root(trace);
	trh = (TraceHeader *) psp(trace, traceHeaderAddress);
	if (trh == NULL) return;
	itemAddr = psm_zalloc(trace, sizeof(TraceItem));
	if (itemAddr == 0)
	{
		discardEvent(trace);
		return;
	}

	item = (TraceItem *) psp(trace, itemAddr);
	memset((char *) item, 0, sizeof(TraceItem));
	if (msg)
	{
		item->msg = psm_zalloc(trace, strlen(msg) + 1);
		if (item->msg == 0)
		{
			discardEvent(trace);
			return;
		}

		strcpy((char *) psp(trace, item->msg), msg);
	}

	item->taskId = sm_TaskIdSelf();
	item->fileName = findFileName(trace, trh, fileName);
	if (item->fileName == 0)
	{
		return;		/*	Events are being discarded.	*/
	}

	item->lineNbr = lineNbr;
	trh->opCount++;
	item->opNbr = trh->opCount;
	item->opType = opType;
	item->objectAddress = addr;
	item->objectSize = objectSize;
	elt = sm_list_insert_last(trace, trh->log, itemAddr);
	if (elt == 0)
	{
		discardEvent(trace);
		return;
	}

	if (eltp)
	{
		*eltp = elt;
	}
}

void	sptrace_log_alloc(PsmPartition trace, unsigned long addr, int size,
		char *fileName, int lineNbr)
{
	if (!trace) return;
	logEvent(trace, OP_ALLOCATE, addr, size, NULL, fileName, lineNbr, NULL);
}

void	sptrace_log_memo(PsmPartition trace, unsigned long addr, char *text,
		char *fileName, int lineNbr)
{
	if (!trace) return;
	logEvent(trace, OP_MEMO, addr, -1, text, fileName, lineNbr, NULL);
}

void	sptrace_log_free(PsmPartition trace, unsigned long addr,
		char *fileName, int lineNbr)
{
	PsmAddress	elt;
	TraceItem	*item;
	TraceItem	*refitem;

	if (!trace) return;
	logEvent(trace, OP_FREE, addr, -1, NULL, fileName, lineNbr, &elt);
	if (elt)	/*	Event was logged.			*/
	{
		item = (TraceItem *) psp(trace, sm_list_data(trace, elt));

		/*	Find matching allocation, close it out.		*/

		elt = sm_list_prev(trace, elt);
		while (elt)
		{
			refitem = (TraceItem *) psp(trace,
					sm_list_data(trace, elt));
			REQUIRE(refitem);
			if (refitem->objectAddress != item->objectAddress)
			{
				elt = sm_list_prev(trace, elt);
				continue;
			}

			/*	Found match.				*/

			switch (refitem->opType)
			{
			case OP_MEMO:	/*	Ignore it.		*/
				elt = sm_list_prev(trace, elt);
				continue;

			case OP_FREE:	/*	Duplicate free.		*/
				return;
			}

			/*	Found most recent open allocation.	*/

			item->refTaskId = refitem->taskId;
			item->refFileName = refitem->fileName;
			item->refLineNbr = refitem->lineNbr;
			item->refOpNbr = refitem->opNbr;
			item->objectSize = refitem->objectSize;
			refitem->refTaskId = item->taskId;
			refitem->refFileName = item->fileName;
			refitem->refLineNbr = item->lineNbr;
			refitem->refOpNbr = item->opNbr;
			return;
		}
	}
}

void	sptrace_report(PsmPartition trace, int verbose)
{
	PsmAddress	traceHeaderAddress;
	TraceHeader	*trh;
	PsmAddress	elt;
	TraceItem	*item;
	char		*fileName;
	char		buffer[384];
	char		buf2[256];

	if (!trace) return;
	traceHeaderAddress = psm_get_root(trace);
	trh = (TraceHeader *) psp(trace, traceHeaderAddress);
	REQUIRE(trh);
	for (elt = sm_list_first(trace, trh->log); elt;
			elt = sm_list_next(trace, elt))
	{
		item = (TraceItem *) psp(trace, sm_list_data(trace, elt));
		REQUIRE(item);
		fileName = (char *) psp(trace, item->fileName);
		sprintf(buffer, "(%5d) at line %6d of %32.32s (pid %5d): ",
			item->opNbr, item->lineNbr, fileName, item->taskId);
		switch (item->opType)
		{
		case OP_ALLOCATE:
			sprintf(buf2, "allocated object %6ld of size %6d, ",
					item->objectAddress, item->objectSize);
			strcat(buffer, buf2);
			if (item->refOpNbr == 0)
			{
				strcat(buffer, "never freed");
			}
			else
			{
				if (!verbose)
				{
					continue;
				}

				fileName = (char *) psp(trace,
						item->refFileName);
				sprintf(buf2, "freed in (%5d) at line %6d of \
%32.32s (pid %5d)", item->refOpNbr, item->refLineNbr, fileName,
					 	item->refTaskId);
				strcat(buffer, buf2);
			}

			break;

		case OP_MEMO:
			sprintf(buf2, " re %6ld, '%.128s'", item->objectAddress,
					(char *) psp(trace, item->msg));
			strcat(buffer, buf2);
			break;

		case OP_FREE:
			sprintf(buf2, "freed object %6ld, ",
					item->objectAddress);
			strcat(buffer, buf2);
			if (item->refOpNbr == 0)
			{
				strcat(buffer, "not currently allocated");
			}
			else
			{
				if (!verbose)
				{
					continue;
				}

				fileName = (char *) psp(trace,
						item->refFileName);
				REQUIRE(fileName);
				sprintf(buf2, "allocated in (%5d) at line \
%6d of %32.32s (pid %5d)", item->refOpNbr, item->refLineNbr, fileName,
						item->refTaskId);
				strcat(buffer, buf2);
			}

			break;
		}

		writeMemo(buffer);
	}
}

void	sptrace_clear(PsmPartition trace)
{
	PsmAddress	traceHeaderAddress;
	TraceHeader	*trh;
	PsmAddress	elt;
	PsmAddress	nextElt;
	PsmAddress	itemAddress;
	TraceItem	*item;

	if (!trace) return;
	traceHeaderAddress = psm_get_root(trace);
	trh = (TraceHeader *) psp(trace, traceHeaderAddress);
	REQUIRE(trh);
	for (elt = sm_list_first(trace, trh->log); elt; elt = nextElt)
	{
		nextElt = sm_list_next(trace, elt);
		itemAddress = sm_list_data(trace, elt);
		item = (TraceItem *) psp(trace, itemAddress);
		REQUIRE(item);
		if (item->opType == OP_ALLOCATE || item->opType == OP_FREE)
		{
			if (item->refOpNbr == 0)
			{
				continue;	/*	Not matched.	*/
			}

			/*	Delete matched activity from log.	*/

			psm_free(trace, itemAddress);
			sm_list_delete(trace, elt, (SmListDeleteFn) NULL, NULL);
		}
	}
}

void	sptrace_stop(PsmPartition trace)
{
	char		*sm;
	TraceHeader	*trh;
	int		smId = 0;

	if (!trace) return;
	trh = (TraceHeader *) psp(trace, psm_get_root(trace));
	if (trh)
	{
		smId = trh->traceSmId;
	}

	sm = psm_space(trace);
	psm_erase(trace);
	sm_ShmDetach(sm);
	if (smId)
	{
		sm_ShmDestroy(smId);
	}
}
