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
	uaddr		objectAddress;
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

	isprintf(buffer, sizeof buffer, "sptrace (pid %d) %s", sm_TaskIdSelf(),
			txt);
	writeMemo(buffer);
}

PsmPartition	sptrace_start(int smkey, size_t smsize, char *sm,
			PsmPartition trace, char *name)
{
	int		nameLen;
	uaddr		smid;
	PsmMgtOutcome	outcome;
	TraceHeader	*trh;
	PsmAddress	traceHeaderAddress;

	CHKNULL(trace);
	CHKNULL(smsize > 0);
	CHKNULL(name);
	if ((nameLen = strlen(name)) < 1 || nameLen > 31)
	{
		sptracePrint("start: name must be 1-31 characters.");
		return NULL;
	}

	/*	Attach to shared memory used for trace operations.	*/

	if (sm_ShmAttach(smkey, smsize, &sm, &smid) < 0)
	{
		sptracePrint("start: can't attach shared memory for trace.");
		return NULL;
	}

	/*	Manage the shared memory region.  "Trace" argument
	 *	is normally NULL.					*/

	if (psm_manage(sm, smsize, name, &trace, &outcome) < 0)
	{
		sptracePrint("start: shared memory mgt failed.");
		return NULL;
	}

	switch (outcome)
	{
	case Refused:
		sptracePrint("start: can't psm_manage shared memory.");
		return NULL;

	case Redundant:
		trh = (TraceHeader *) psp(trace, psm_get_root(trace));
		if (trh == NULL || strcmp(name, trh->name) != 0)
		{
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
	CHKNULL(trh);
	trh->traceSmId = smid;
	memset(trh->name, 0, sizeof trh->name);
	istrcpy(trh->name, name, sizeof trh->name);
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

PsmPartition	sptrace_join(int smkey, size_t smsize, char *sm,
			PsmPartition trace, char *name)
{
	int		nameLen;
	uaddr		smid;
	PsmMgtOutcome	outcome;
	TraceHeader	*trh;

	CHKNULL(trace);
	CHKNULL(smsize > 0);
	CHKNULL(name);
	if ((nameLen = strlen(name)) < 1 || nameLen > 31)
	{
		sptracePrint("start: name must be 1-31 characters.");
		return NULL;
	}

	/*	Attach to shared memory used for trace operations.	*/

	if (sm_ShmAttach(smkey, smsize, (char **) &sm, &smid) < 0)
	{
		sptracePrint("join: can't attach shared memory for trace.");
		return NULL;
	}

	/*	Examine the shared memory region.			*/

	if (psm_manage(sm, smsize, name, &trace, &outcome) < 0)
	{
		sptracePrint("join: shared memory mgt failed.");
		return NULL;
	}

	switch (outcome)
	{
	case Refused:
		sptracePrint("join: can't psm_manage shared memory.");
		return NULL;

	case Redundant:
		trh = (TraceHeader *) psp(trace, psm_get_root(trace));
		if (trh == NULL || strcmp(name, trh->name) != 0)
		{
			sptracePrint("join: shared memory used otherwise.");
			return NULL;
		}

		return trace;		/*	Have joined the trace.	*/

	default:
		break;
	}

	sptracePrint("join: trace episode not yet started.");
	return NULL;
}

static void	discardEvent(PsmPartition trace)
{
	char	buffer[256];

	isprintf(buffer, sizeof buffer, "sptrace (pid %d) logging: not enough \
memory for trace, ignoring event.", sm_TaskIdSelf());
	writeMemo(buffer);
}

static PsmAddress	findFileName(PsmPartition trace, TraceHeader *trh,
				const char *sourceFileName)
{
	PsmAddress	elt;
	PsmAddress	filenameAddress;
	char		*filename;
	int		buflen;

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

	buflen = strlen(sourceFileName) + 1;
	filenameAddress = psm_zalloc(trace, buflen);
	if (filenameAddress == 0)
	{
		discardEvent(trace);
		return 0;
	}

	/*	Add this source file to the list for the trace.		*/

	istrcpy((char *) psp(trace, filenameAddress), sourceFileName, buflen);
	if (sm_list_insert_last(trace, trh->files, filenameAddress) == 0)
	{
		discardEvent(trace);
		return 0;
	}

	return filenameAddress;
}

static void	logEvent(PsmPartition trace, int opType, uaddr addr,
			size_t objectSize, char *msg, const char *fileName,
			int lineNbr, PsmAddress *eltp)
{
	PsmAddress	traceHeaderAddress;
	TraceHeader	*trh;
	PsmAddress	itemAddr;
	TraceItem	*item;
	int		buflen;
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
		buflen = strlen(msg) + 1;
		item->msg = psm_zalloc(trace, buflen);
		if (item->msg == 0)
		{
			discardEvent(trace);
			return;
		}

		istrcpy((char *) psp(trace, item->msg), msg, buflen);
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

void	sptrace_log_alloc(PsmPartition trace, uaddr addr, size_t size,
		const char *fileName, int lineNbr)
{
	if (!trace) return;
	logEvent(trace, OP_ALLOCATE, addr, size, NULL, fileName, lineNbr, NULL);
}

void	sptrace_log_memo(PsmPartition trace, uaddr addr, char *text,
		const char *fileName, int lineNbr)
{
	if (!trace) return;
	logEvent(trace, OP_MEMO, addr, -1, text, fileName, lineNbr, NULL);
}

void	sptrace_log_free(PsmPartition trace, uaddr addr,
		const char *fileName, int lineNbr)
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
			CHKVOID(refitem);
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
	int		len;
	char		buf2[256];

	if (!trace) return;
	traceHeaderAddress = psm_get_root(trace);
	trh = (TraceHeader *) psp(trace, traceHeaderAddress);
	CHKVOID(trh);
	for (elt = sm_list_first(trace, trh->log); elt;
			elt = sm_list_next(trace, elt))
	{
		item = (TraceItem *) psp(trace, sm_list_data(trace, elt));
		CHKVOID(item);
		fileName = (char *) psp(trace, item->fileName);
		isprintf(buffer, sizeof buffer, "(%5d) at line %6d of %32.32s \
(pid %5d): ", item->opNbr, item->lineNbr, fileName, item->taskId);
		len = strlen(buffer);
		switch (item->opType)
		{
		case OP_ALLOCATE:
			isprintf(buf2, sizeof buf2, "allocated object %6ld of \
size %6d, ", item->objectAddress, item->objectSize);
			istrcpy(buffer + len, buf2, sizeof buffer - len);
			if (item->refOpNbr == 0)
			{
				len = strlen(buffer);
				istrcpy(buffer + len, "never freed",
						sizeof buffer - len);
			}
			else
			{
				if (!verbose)
				{
					continue;
				}

				len = strlen(buffer);
				fileName = (char *) psp(trace,
						item->refFileName);
				isprintf(buf2, sizeof buf2, "freed in (%5d) \
at line %6d of %32.32s (pid %5d)", item->refOpNbr, item->refLineNbr, fileName,
					 	item->refTaskId);
				istrcpy(buffer + len, buf2,
						sizeof buffer - len);
			}

			break;

		case OP_MEMO:
			isprintf(buf2, sizeof buf2, "re %6ld, '%.128s'",
					item->objectAddress,
					(char *) psp(trace, item->msg));
			istrcpy(buffer + len, buf2, sizeof buffer - len);
			break;

		case OP_FREE:
			isprintf(buf2, sizeof buf2, "freed object %6ld, ",
					item->objectAddress);
			istrcpy(buffer + len, buf2, sizeof buffer - len);
			if (item->refOpNbr == 0)
			{
				len = strlen(buffer);
				istrcpy(buffer + len, "not currently allocated",
						sizeof buffer - len);
			}
			else
			{
				if (!verbose)
				{
					continue;
				}

				len = strlen(buffer);
				fileName = (char *) psp(trace,
						item->refFileName);
				CHKVOID(fileName);
				isprintf(buf2, sizeof buf2, "allocated in \
(%5d) at line %6d of %32.32s (pid %5d)", item->refOpNbr, item->refLineNbr,
						fileName, item->refTaskId);
				istrcpy(buffer + len, buf2,
						sizeof buffer - len);
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
	CHKVOID(trh);
	for (elt = sm_list_first(trace, trh->log); elt; elt = nextElt)
	{
		nextElt = sm_list_next(trace, elt);
		itemAddress = sm_list_data(trace, elt);
		item = (TraceItem *) psp(trace, itemAddress);
		CHKVOID(item);
		if (item->opType == OP_ALLOCATE || item->opType == OP_FREE)
		{
			if (item->refOpNbr == 0)
			{
				continue;	/*	Not matched.	*/
			}

			/*	Delete matched activity from log.	*/

			psm_free(trace, itemAddress);
			CHKVOID(sm_list_delete(trace, elt, NULL, NULL) == 0);
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
