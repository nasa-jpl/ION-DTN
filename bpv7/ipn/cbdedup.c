/*
	cbdedup.c:	per-node "already-forwarded" seen-set for
			critical bundles.  See cbdedup.h.
									*/

#include "cbdedup.h"
#include "smlist.h"
#include "smrbt.h"

/*	Name under which the table root is anchored in the working-
 *	memory PSM catalog.  Anchoring (rather than relying on a
 *	process-static handle) lets a restarted ipnfw find the
 *	existing table instead of orphaning it in the partition.	*/
#define	CBDEDUP_CATLG_NAME	"cbdedup"

typedef struct
{
	PsmAddress	rbt;	/*	Seen-set, keyed by bundle ID.	*/
	PsmAddress	queue;	/*	FIFO of entries, oldest first.	*/
} CbdTable;

typedef struct
{
	uvast	      sourceFqnn;
	unsigned long sourceServiceNbr;
	uvast	      creationMsec;
	unsigned int  creationCount;
	unsigned int  fragmentOffset;
	unsigned int  fragmentLength;
	time_t	      expirationTime;
	PsmAddress    queueElt; /*	Position in FIFO queue.	*/
} CbdEntry;

static PsmAddress _rbt(PsmAddress *newRbt)
{
	static PsmAddress rbt = 0;

	if (newRbt)
	{
		rbt = *newRbt;
	}

	return rbt;
}

static PsmAddress _queue(PsmAddress *newQueue)
{
	static PsmAddress queue = 0;

	if (newQueue)
	{
		queue = *newQueue;
	}

	return queue;
}

static int cbd_compare(PsmPartition wm, PsmAddress nodeData, void *arg)
{
	CbdEntry *a = (CbdEntry *) psp(wm, nodeData);
	CbdEntry *b = (CbdEntry *) arg;

	if (a->sourceFqnn != b->sourceFqnn)
	{
		return a->sourceFqnn < b->sourceFqnn ? -1 : 1;
	}

	if (a->sourceServiceNbr != b->sourceServiceNbr)
	{
		return a->sourceServiceNbr < b->sourceServiceNbr ? -1 : 1;
	}

	if (a->creationMsec != b->creationMsec)
	{
		return a->creationMsec < b->creationMsec ? -1 : 1;
	}

	if (a->creationCount != b->creationCount)
	{
		return a->creationCount < b->creationCount ? -1 : 1;
	}

	if (a->fragmentOffset != b->fragmentOffset)
	{
		return a->fragmentOffset < b->fragmentOffset ? -1 : 1;
	}

	if (a->fragmentLength != b->fragmentLength)
	{
		return a->fragmentLength < b->fragmentLength ? -1 : 1;
	}

	return 0;
}

static void cbd_free(PsmPartition wm, PsmAddress nodeData, void *arg)
{
	(void) arg;
	psm_free(wm, nodeData);
}

/*	Drop one entry from both the rbt and the FIFO queue, given
 *	a queue elt whose data is the entry's PsmAddress.		*/
static void dropByQueueElt(PsmPartition wm, PsmAddress rbt, PsmAddress elt)
{
	PsmAddress entryAddr = sm_list_data(wm, elt);
	CbdEntry  *entry = (CbdEntry *) psp(wm, entryAddr);

	sm_list_delete(wm, elt, NULL, NULL);
	sm_rbt_delete(wm, rbt, cbd_compare, entry, cbd_free, NULL);
}

/*	Pop entries from the head of the queue (= oldest by insertion
 *	order) until the queue length is at or below `target`.        */
static void evictHeadDownTo(PsmPartition wm, PsmAddress rbt, PsmAddress queue,
		size_t target)
{
	PsmAddress elt;

	while (sm_list_length(wm, queue) > target)
	{
		elt = sm_list_first(wm, queue);
		if (elt == 0)
		{
			break;
		}

		dropByQueueElt(wm, rbt, elt);
	}
}

static int fillKey(Bundle *bundle, CbdEntry *key)
{
	if (bundle->id.source.schemeCodeNbr != ipn)
	{
		return 0; /*	Non-ipn source: skip.		*/
	}

	memset(key, 0, sizeof *key);
	key->sourceFqnn = bundle->id.source.ssp.ipn.fqnn;
	key->sourceServiceNbr = bundle->id.source.ssp.ipn.serviceNbr;
	key->creationMsec = bundle->id.creationTime.msec;
	key->creationCount = bundle->id.creationTime.count;
	key->fragmentOffset = bundle->id.fragmentOffset;
	key->fragmentLength = (bundle->totalAduLength == 0) ?
			0 :
			bundle->payload.length;
	return 1;
}

int cbdedup_init(void)
{
	PsmPartition wm = getIonwm();
	Sdr	     sdr = getIonsdr();
	PsmAddress   tableAddr;
	PsmAddress   elt;
	CbdTable    *table;
	PsmAddress   rbt;
	PsmAddress   queue;

	if (_rbt(NULL))
	{
		return 0;	/*	Already attached in this process. */
	}

	if (psm_locate(wm, CBDEDUP_CATLG_NAME, &tableAddr, &elt) < 0)
	{
		putErrmsg("cbdedup can't search working memory.", NULL);
		return -1;
	}

	if (elt)		/*	Table already exists; reuse it.	*/
	{
		table = (CbdTable *) psp(wm, tableAddr);
		rbt = table->rbt;
		queue = table->queue;
		_rbt(&rbt);
		_queue(&queue);
		return 0;
	}

	/*	Table doesn't exist yet.  Allocate it; take the SDR
	 *	transaction to serialize working-memory allocation,
	 *	matching the idiom used to create the volatile DBs.	*/

	CHKERR(sdr_begin_xn(sdr));
	tableAddr = psm_zalloc(wm, sizeof(CbdTable));
	if (tableAddr == 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("No space for cbdedup table.", NULL);
		return -1;
	}

	rbt = sm_rbt_create(wm);
	queue = sm_list_create(wm);
	if (rbt == 0 || queue == 0)
	{
		if (rbt) sm_rbt_destroy(wm, rbt, NULL, NULL);
		if (queue) sm_list_destroy(wm, queue, NULL, NULL);
		psm_free(wm, tableAddr);
		sdr_exit_xn(sdr);
		putErrmsg("Can't create cbdedup table.", NULL);
		return -1;
	}

	table = (CbdTable *) psp(wm, tableAddr);
	table->rbt = rbt;
	table->queue = queue;
	if (psm_catlg(wm, CBDEDUP_CATLG_NAME, tableAddr) < 0)
	{
		sm_rbt_destroy(wm, rbt, NULL, NULL);
		sm_list_destroy(wm, queue, NULL, NULL);
		psm_free(wm, tableAddr);
		sdr_exit_xn(sdr);
		putErrmsg("Can't catalog cbdedup table.", NULL);
		return -1;
	}

	sdr_exit_xn(sdr);
	_rbt(&rbt);
	_queue(&queue);
	return 0;
}

void cbdedup_shutdown(void)
{
	PsmAddress zero = 0;

	/*	The table is owned by the working-memory partition and
	 *	stays anchored in the PSM catalog for reuse by the next
	 *	ipnfw; it is reclaimed when working memory is torn down.
	 *	Just forget this process's cached handles.		*/

	_rbt(&zero);
	_queue(&zero);
}

int cbdedup_seen(Bundle *bundle)
{
	PsmPartition wm = getIonwm();
	PsmAddress   rbt = _rbt(NULL);
	CbdEntry     key;
	PsmAddress   node;

	if (rbt == 0 || !(bundle->ancillaryData.flags & BP_MINIMUM_LATENCY)
			|| !fillKey(bundle, &key))
	{
		return 0;
	}

	node = sm_rbt_search(wm, rbt, cbd_compare, &key, NULL);
	if (node == 0)
	{
		return 0;
	}

	return 1;
}

int cbdedup_record(Bundle *bundle)
{
	PsmPartition wm = getIonwm();
	PsmAddress   rbt = _rbt(NULL);
	PsmAddress   queue = _queue(NULL);
	CbdEntry     key;
	PsmAddress   node;
	PsmAddress   entryAddr;
	PsmAddress   queueElt;
	CbdEntry    *entry;

	if (rbt == 0 || !(bundle->ancillaryData.flags & BP_MINIMUM_LATENCY)
			|| !fillKey(bundle, &key))
	{
		return 0;
	}

	node = sm_rbt_search(wm, rbt, cbd_compare, &key, NULL);
	if (node)
	{
		return 0; /*	Already recorded.	*/
	}

	entryAddr = psm_zalloc(wm, sizeof(CbdEntry));
	if (entryAddr == 0)
	{
		return -1;
	}

	entry = (CbdEntry *) psp(wm, entryAddr);
	*entry = key;
	entry->expirationTime = bundle->expirationTime;

	queueElt = sm_list_insert_last(wm, queue, entryAddr);
	if (queueElt == 0)
	{
		psm_free(wm, entryAddr);
		return -1;
	}

	entry->queueElt = queueElt;

	if (sm_rbt_insert(wm, rbt, entryAddr, cbd_compare, entry) == 0)
	{
		sm_list_delete(wm, queueElt, NULL, NULL);
		psm_free(wm, entryAddr);
		return -1;
	}

	/*	Drop expired entries from the head.  Head is the
	 *	oldest by insertion order, so this catches expired
	 *	entries cheaply when TTLs are similar across bundles.	*/

	{
		time_t	   now = getCtime();
		PsmAddress headElt;
		CbdEntry  *headEntry;

		while ((headElt = sm_list_first(wm, queue)) != 0)
		{
			headEntry = (CbdEntry *) psp(wm,
					sm_list_data(wm, headElt));
			if (headEntry->expirationTime > now)
			{
				break;
			}

			dropByQueueElt(wm, rbt, headElt);
		}
	}

	/*	Bound size by FIFO eviction.				*/

	if (sm_list_length(wm, queue) > CBDEDUP_HIGH_WATER)
	{
		evictHeadDownTo(wm, rbt, queue, CBDEDUP_LOW_WATER);
	}

	return 0;
}
