/*
 *	libbpP.c:	functions enabling the implementation of
 *			ION-BP bundle forwarders.
 *
 *	Copyright (c) 2006, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 *
 *	Modification History:
 *	Date	  Who	What
 *	06-17-03  SCB	Original development.
 *	05-30-04  SCB	Revision per version 3 of Bundle Protocol spec.
 *	01-09-06  SCB	Revision per version 4 of Bundle Protocol spec.
 *	12-29-06  SCB	Revision per version 5 of Bundle Protocol spec.
 *	07-31-07  SCB	Revision per version 6 of Bundle Protocol spec.
 */

#include "bpP.h"
#include "bei.h"
#include "sdrhash.h"
#include "smrbt.h"

#define	BP_VERSION		6

#define MAX_STARVATION		10
#define NOMINAL_BYTES_PER_SEC	(256 * 1024)
#define NOMINAL_PRIMARY_BLKSIZE	29

#define	BASE_BUNDLE_OVERHEAD	(sizeof(Bundle))

#ifndef BUNDLES_HASH_KEY_LEN
#define	BUNDLES_HASH_KEY_LEN	64
#endif

#define	BUNDLES_HASH_KEY_BUFLEN	(BUNDLES_HASH_KEY_LEN << 1)

#ifndef BUNDLES_HASH_ENTRIES
#define	BUNDLES_HASH_ENTRIES	10000
#endif

#ifndef BUNDLES_HASH_SEARCH_LEN
#define	BUNDLES_HASH_SEARCH_LEN	20
#endif

static BpVdb	*_bpvdb(char **);
static int	constructCtSignal(BpCtSignal *csig, Object *zco);
static int	constructStatusRpt(BpStatusRpt *rpt, Object *zco);

/*	*	*	ACS adaptation		*	*	*	*/

#ifdef ENABLE_BPACS
#include "acs.h"

static void	bpDestroyBundle_ACS(Bundle *bundle)
{
	if (bundle->bundleProcFlags & BDL_IS_CUSTODIAL)
	{
		/*	Destroy the metadata that ACS is keeping for
		 *	this bundle.					*/

		destroyAcsMetadata(bundle);
	}
}

static int	parseACS(int adminRecordType, void **otherPtr,
			unsigned char *cursor, int unparsedBytes,
			int bundleIsFragment)
{
	if (adminRecordType != BP_AGGREGATE_CUSTODY_SIGNAL)
	{
		return -2;
	}

	return parseAggregateCtSignal(otherPtr, cursor, unparsedBytes,
			bundleIsFragment);
}

static int	applyACS(int adminRecType, void *other, BpDelivery *dlv,
			CtSignalCB handleCtSignal)
{
	if (adminRecType != BP_AGGREGATE_CUSTODY_SIGNAL)
	{
		return -2;
	}

	if (handleAcs(other, dlv, handleCtSignal) == 0)
	{
		return -1;
	}

	return 0;
}

#else		/*	Stub functions to enable build.			*/

static void	bpDestroyBundle_ACS(Bundle *bundle)
{
	return;
}

static int	offerNoteAcs(Bundle *b, AcqWorkArea *w, char *d, int s,
			BpCtReason r)
{
	return 0;
}

static int	parseACS(int adminRecordType, void **otherPtr,
			unsigned char *cursor, int unparsedBytes,
			int bundleIsFragment)
{
	return -2;
}

static int	applyACS(int adminRecType, void *other, BpDelivery *dlv,
			CtSignalCB handleCtSignal)
{
	return -2;
}

#endif	/*	ENABLE_BPACS	*/

/*	*	*	IMC Multicast adaptation	*	*	*/

#ifdef ENABLE_IMC
#include "imcP.h"

static int	addEndpoint_IMC(VScheme *vscheme, char *eid)
{
	MetaEid		metaEid;
	PsmAddress	elt;
	int		result;

	if (vscheme->unicast || vscheme->cbhe == 0 || eid == NULL)
	{
		return 0;
	}

	if (imcInit() < 0)
	{
		putErrmsg("Can't initialize IMC database.", NULL);
		return -1;
	}

	/*	We know the EID parses okay, because it was already
	 *	parsed earlier in addEndpoint.				*/

	oK(parseEidString(eid, &metaEid, &vscheme, &elt));
	if (metaEid.serviceNbr != 0)
	{
		restoreEidString(&metaEid);
		writeMemoNote("[?] IMC EID service nbr must be zero", eid);
		return 0;
	}

	result = imcJoin(metaEid.nodeNbr);
	restoreEidString(&metaEid);
	return result;
}

static int	removeEndpoint_IMC(VScheme *vscheme, char *eid)
{
	MetaEid		metaEid;
	PsmAddress	elt;
	int		result;

	if (vscheme->unicast || vscheme->cbhe == 0 || eid == NULL)
	{
		return 0;
	}

	if (imcInit() < 0)
	{
		putErrmsg("Can't initialize IMC database.", NULL);
		return -1;
	}

	/*	We know the EID parses okay, because it was already
	 *	parsed earlier in removeEndpoint.			*/

	oK(parseEidString(eid, &metaEid, &vscheme, &elt));
	if (metaEid.serviceNbr != 0)
	{
		restoreEidString(&metaEid);
		writeMemoNote("[?] IMC EID service nbr must be zero", eid);
		return 0;
	}

	result = imcLeave(metaEid.nodeNbr);
	restoreEidString(&metaEid);
	return result;
}

static int	parseImcPetition(int adminRecordType, void **otherPtr,
			unsigned char *cursor, int unparsedBytes)
{
	if (adminRecordType != BP_MULTICAST_PETITION)
	{
		return -2;
	}

	if (imcInit() < 0)
	{
		putErrmsg("Can't initialize IMC database.", NULL);
		return -1;
	}

	return imcParsePetition(otherPtr, cursor, unparsedBytes);
}

static int	applyImcPetition(int adminRecType, void *other, BpDelivery *dlv)
{
	if (adminRecType != BP_MULTICAST_PETITION)
	{
		return -2;
	}

	if (imcInit() < 0)
	{
		putErrmsg("Can't initialize IMC database.", NULL);
		return -1;
	}

	return imcHandlePetition(other, dlv);
}

#else		/*	Stub functions to enable build.			*/

static int	addEndpoint_IMC(VScheme *scheme, char *eid)
{
	return 0;
}

static int	removeEndpoint_IMC(VScheme *scheme, char *eid)
{
	return 0;
}

static int	parseImcPetition(int adminRecordType, void **otherPtr,
			unsigned char *cursor, int unparsedBytes)
{
	return -2;
}

static int	applyImcPetition(int adminRecType, void *other, BpDelivery *dlv)
{
	return -2;
}

#endif	/*	ENABLE_IMC	*/

/*	*	*	Helpful utility functions	*	*	*/

static Object	_bpdbObject(Object *newDbObj)
{
	static Object	obj = 0;

	if (newDbObj)
	{
		obj = *newDbObj;
	}

	return obj;
}

static BpDB	*_bpConstants()
{
	static BpDB	buf;
	static BpDB	*db = NULL;
	Sdr		sdr;
	Object		dbObject;
	
	if (db == NULL)
	{
		/*	Load constants into a conveniently accessed
		 *	structure.  Note that this CANNOT be treated
		 *	as a current database image in later
		 *	processing.					*/

		sdr = getIonsdr();
		CHKNULL(sdr);
		dbObject = _bpdbObject(NULL);
		if (dbObject)
		{
			if (sdr_heap_is_halted(sdr))
			{
				sdr_read(sdr, (char *) &buf, dbObject,
						sizeof(BpDB));
			}
			else
			{
				CHKNULL(sdr_begin_xn(sdr));
				sdr_read(sdr, (char *) &buf, dbObject,
						sizeof(BpDB));
				sdr_exit_xn(sdr);
			}

			db = &buf;
		}
	}
	
	return db;
}

static char	*_nullEid()
{
	return "dtn:none";
}

static int	isCbhe(char *schemeName)
{
	if (strcmp(schemeName, "ipn") == 0)
	{
		return 1;
	}

	if (strcmp(schemeName, "imc") == 0)
	{
		return 1;
	}

	return 0;
}

static int	isUnicast(char *schemeName)
{
	if (strcmp(schemeName, "ipn") == 0)
	{
		return 1;
	}

	if (strcmp(schemeName, "dtn") == 0)
	{
		return 1;
	}

	return 0;
}

/*	*	*	Instrumentation functions	*	*	*/

void	bpEndpointTally(VEndpoint *vpoint, unsigned int idx, unsigned int size)
{
	Sdr		sdr = getIonsdr();
	EndpointStats	stats;
	Tally		*tally;
	int		offset;

	CHKVOID(vpoint && vpoint->stats);
	if (!(vpoint->updateStats))
	{
		return;
	}

	CHKVOID(ionLocked());
	CHKVOID(idx < BP_ENDPOINT_STATS);
	sdr_stage(sdr, (char *) &stats, vpoint->stats, sizeof(EndpointStats));
	tally = stats.tallies + idx;
	tally->totalCount += 1;
	tally->totalBytes += size;
	tally->currentCount += 1;
	tally->currentBytes += size;
	offset = (char *) tally - ((char *) &stats);
	sdr_write(sdr, vpoint->stats + offset, (char *) tally, sizeof(Tally));
}

void	bpInductTally(VInduct *vduct, unsigned int idx, unsigned int size)
{
	Sdr		sdr = getIonsdr();
	InductStats	stats;
	Tally		*tally;
	int		offset;

	CHKVOID(vduct && vduct->stats);
	if (!(vduct->updateStats))
	{
		return;
	}

	CHKVOID(ionLocked());
	CHKVOID(idx < BP_INDUCT_STATS);
	sdr_stage(sdr, (char *) &stats, vduct->stats, sizeof(InductStats));
	tally = stats.tallies + idx;
	tally->totalCount += 1;
	tally->totalBytes += size;
	tally->currentCount += 1;
	tally->currentBytes += size;
	offset = (char *) tally - ((char *) &stats);
	sdr_write(sdr, vduct->stats + offset, (char *) tally, sizeof(Tally));
}

void	bpOutductTally(VOutduct *vduct, unsigned int idx, unsigned int size)
{
	Sdr		sdr = getIonsdr();
	OutductStats	stats;
	Tally		*tally;
	int		offset;

	CHKVOID(vduct && vduct->stats);
	if (!(vduct->updateStats))
	{
		return;
	}

	CHKVOID(ionLocked());
	CHKVOID(idx < BP_OUTDUCT_STATS);
	sdr_stage(sdr, (char *) &stats, vduct->stats, sizeof(OutductStats));
	tally = stats.tallies + idx;
	tally->totalCount += 1;
	tally->totalBytes += size;
	tally->currentCount += 1;
	tally->currentBytes += size;
	offset = (char *) tally - ((char *) &stats);
	sdr_write(sdr, vduct->stats + offset, (char *) tally, sizeof(Tally));
}

void	bpSourceTally(unsigned int priority, unsigned int size)
{
	Sdr		sdr = getIonsdr();
	BpVdb		*vdb = getBpVdb();
	BpCosStats	stats;
	Tally		*tally;
	int		offset;

	CHKVOID(vdb && vdb->sourceStats);
	if (!(vdb->updateStats))
	{
		return;
	}

	CHKVOID(ionLocked());
	CHKVOID(priority < 3);
	sdr_stage(sdr, (char *) &stats, vdb->sourceStats, sizeof(BpCosStats));
	tally = stats.tallies + priority;
	tally->totalCount += 1;
	tally->totalBytes += size;
	tally->currentCount += 1;
	tally->currentBytes += size;
	offset = (char *) tally - ((char *) &stats);
	sdr_write(sdr, vdb->sourceStats + offset, (char *) tally,
			sizeof(Tally));
}

void	bpRecvTally(unsigned int priority, unsigned int size)
{
	Sdr		sdr = getIonsdr();
	BpVdb		*vdb = getBpVdb();
	BpCosStats	stats;
	Tally		*tally;
	int		offset;

	CHKVOID(vdb && vdb->recvStats);
	if (!(vdb->updateStats))
	{
		return;
	}

	CHKVOID(ionLocked());
	CHKVOID(priority < 3);
	sdr_stage(sdr, (char *) &stats, vdb->recvStats, sizeof(BpCosStats));
	tally = stats.tallies + priority;
	tally->totalCount += 1;
	tally->totalBytes += size;
	tally->currentCount += 1;
	tally->currentBytes += size;
	offset = (char *) tally - ((char *) &stats);
	sdr_write(sdr, vdb->recvStats + offset, (char *) tally, sizeof(Tally));
}

void	bpDiscardTally(unsigned int priority, unsigned int size)
{
	Sdr		sdr = getIonsdr();
	BpVdb		*vdb = getBpVdb();
	BpCosStats	stats;
	Tally		*tally;
	int		offset;

	CHKVOID(vdb && vdb->discardStats);
	if (!(vdb->updateStats))
	{
		return;
	}

	CHKVOID(ionLocked());
	CHKVOID(priority < 3);
	sdr_stage(sdr, (char *) &stats, vdb->discardStats, sizeof(BpCosStats));
	tally = stats.tallies + priority;
	tally->totalCount += 1;
	tally->totalBytes += size;
	tally->currentCount += 1;
	tally->currentBytes += size;
	offset = (char *) tally - ((char *) &stats);
	sdr_write(sdr, vdb->discardStats + offset, (char *) tally,
			sizeof(Tally));
}

void	bpXmitTally(unsigned int priority, unsigned int size)
{
	Sdr		sdr = getIonsdr();
	BpVdb		*vdb = getBpVdb();
	BpCosStats	stats;
	Tally		*tally;
	int		offset;

	CHKVOID(vdb && vdb->xmitStats);
	if (!(vdb->updateStats))
	{
		return;
	}

	CHKVOID(ionLocked());
	CHKVOID(priority < 3);
	sdr_stage(sdr, (char *) &stats, vdb->xmitStats, sizeof(BpCosStats));
	tally = stats.tallies + priority;
	tally->totalCount += 1;
	tally->totalBytes += size;
	tally->currentCount += 1;
	tally->currentBytes += size;
	offset = (char *) tally - ((char *) &stats);
	sdr_write(sdr, vdb->xmitStats + offset, (char *) tally, sizeof(Tally));
}

void	bpRptTally(unsigned char status, unsigned int reason)
{
	Sdr		sdr = getIonsdr();
	BpVdb		*vdb = getBpVdb();
	BpRptStats	stats;

	CHKVOID(vdb && vdb->rptStats);
	if (!(vdb->updateStats))
	{
		return;
	}

	CHKVOID(ionLocked());
	CHKVOID(status < 32);
	CHKVOID(reason < BP_REASON_STATS);
	sdr_stage(sdr, (char *) &stats, vdb->rptStats, sizeof(BpRptStats));
	if (status & BP_RECEIVED_RPT)
	{
		stats.totalRptByStatus[BP_STATUS_RECEIVE] += 1;
		stats.currentRptByStatus[BP_STATUS_RECEIVE] += 1;
	}

	if (status & BP_CUSTODY_RPT)
	{
		stats.totalRptByStatus[BP_STATUS_ACCEPT] += 1;
		stats.currentRptByStatus[BP_STATUS_ACCEPT] += 1;
	}

	if (status & BP_FORWARDED_RPT)
	{
		stats.totalRptByStatus[BP_STATUS_FORWARD] += 1;
		stats.currentRptByStatus[BP_STATUS_FORWARD] += 1;
	}

	if (status & BP_DELIVERED_RPT)
	{
		stats.totalRptByStatus[BP_STATUS_DELIVER] += 1;
		stats.currentRptByStatus[BP_STATUS_DELIVER] += 1;
	}

	if (status & BP_DELETED_RPT)
	{
		stats.totalRptByStatus[BP_STATUS_DELETE] += 1;
		stats.currentRptByStatus[BP_STATUS_DELETE] += 1;
	}

	stats.totalRptByReason[reason] += 1;
	stats.currentRptByReason[reason] += 1;
	sdr_write(sdr, vdb->rptStats, (char *) &stats, sizeof(BpRptStats));
}

void	bpCtTally(unsigned int reason, unsigned int size)
{
	Sdr		sdr = getIonsdr();
	BpVdb		*vdb = getBpVdb();
	BpCtStats	stats;
	Tally		*tally;
	int		offset;

	CHKVOID(vdb && vdb->ctStats);
	if (!(vdb->updateStats))
	{
		return;
	}

	CHKVOID(ionLocked());
	CHKVOID(reason < BP_REASON_STATS);
	sdr_stage(sdr, (char *) &stats, vdb->ctStats, sizeof(BpCtStats));
	tally = stats.tallies + reason;
	tally->totalCount += 1;
	tally->totalBytes += size;
	tally->currentCount += 1;
	tally->currentBytes += size;
	offset = (char *) tally - ((char *) &stats);
	sdr_write(sdr, vdb->ctStats + offset, (char *) tally, sizeof(Tally));
}

void	bpDbTally(unsigned int idx, unsigned int size)
{
	Sdr		sdr = getIonsdr();
	BpVdb		*vdb = getBpVdb();
	BpDbStats	stats;
	Tally		*tally;
	int		offset;

	CHKVOID(vdb && vdb->dbStats);
	if (!(vdb->updateStats))
	{
		return;
	}

	CHKVOID(ionLocked());
	CHKVOID(idx < BP_DB_STATS);
	sdr_stage(sdr, (char *) &stats, vdb->dbStats, sizeof(BpDbStats));
	tally = stats.tallies + idx;
	tally->totalCount += 1;
	tally->totalBytes += size;
	tally->currentCount += 1;
	tally->currentBytes += size;
	offset = (char *) tally - ((char *) &stats);
	sdr_write(sdr, vdb->dbStats + offset, (char *) tally, sizeof(Tally));
}

/*	*	*	BP service control functions	*	*	*/

static void	resetEndpoint(VEndpoint *vpoint)
{
	if (vpoint->semaphore == SM_SEM_NONE)
	{
		vpoint->semaphore = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	}
	else
	{
		sm_SemUnend(vpoint->semaphore);

		/*	Endpoint might get reset multiple times without
		 *	being stopped, e.g., when scheme is raised.
		 *	Give semaphore to prevent deadlock when this
		 *	happens.					*/

		sm_SemGive(vpoint->semaphore);
	}

	sm_SemTake(vpoint->semaphore);			/*	Lock.	*/
	vpoint->appPid = ERROR;				/*	None.	*/
}

static int	raiseEndpoint(VScheme *vscheme, Object endpointElt)
{
	Sdr		bpSdr = getIonsdr();
	PsmPartition	bpwm = getIonwm();
	Object		endpointObj;
	Endpoint	endpoint;
	VEndpoint	*vpoint;
	PsmAddress	vpointElt;
	PsmAddress	addr;

	endpointObj = sdr_list_data(bpSdr, endpointElt);
	sdr_read(bpSdr, (char *) &endpoint, endpointObj, sizeof(Endpoint));

	/*	No need to findEndpoint to determine whether or not
	 *	the endpoint has already been raised.  We findScheme
	 *	instead; if the scheme has been raised we don't raise
	 *	it and thus don't call raiseEndpoint at all.  (Can't
	 *	do this with Induct and Outduct because we can't check
	 *	to see if the protocol has been raised -- there is no
	 *	findProtocol function.)					*/

	addr = psm_malloc(bpwm, sizeof(VEndpoint));
	if (addr == 0)
	{
		return -1;
	}

	vpointElt = sm_list_insert_last(bpwm, vscheme->endpoints, addr);
	if (vpointElt == 0)
	{
		psm_free(bpwm, addr);
		return -1;
	}

	vpoint = (VEndpoint *) psp(bpwm, addr);
	memset((char *) vpoint, 0, sizeof(VEndpoint));
	vpoint->endpointElt = endpointElt;
	vpoint->stats = endpoint.stats;
	vpoint->updateStats = endpoint.updateStats;
	istrcpy(vpoint->nss, endpoint.nss, sizeof vpoint->nss);
	vpoint->semaphore = SM_SEM_NONE;
	resetEndpoint(vpoint);
	return 0;
}

static void	dropEndpoint(VEndpoint *vpoint, PsmAddress vpointElt)
{
	PsmAddress	vpointAddr;
	PsmPartition	bpwm = getIonwm();

	vpointAddr = sm_list_data(bpwm, vpointElt);
	if (vpoint->semaphore != SM_SEM_NONE)
	{
		sm_SemDelete(vpoint->semaphore);
	}

	oK(sm_list_delete(bpwm, vpointElt, NULL, NULL));
	psm_free(bpwm, vpointAddr);
}

static void	resetScheme(VScheme *vscheme)
{
	PsmPartition	bpwm = getIonwm();
	PsmAddress	elt;
	PsmAddress	endpointAddr;
	VEndpoint	*vpoint;

	if (vscheme->semaphore == SM_SEM_NONE)
	{
		vscheme->semaphore = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	}
	else
	{
		sm_SemUnend(vscheme->semaphore);
	}

	sm_SemTake(vscheme->semaphore);			/*	Lock.	*/
	for (elt = sm_list_first(bpwm, vscheme->endpoints); elt;
			elt = sm_list_next(bpwm, elt))
	{
		endpointAddr = sm_list_data(bpwm, elt);
		vpoint = (VEndpoint *) psp(bpwm, endpointAddr);
		resetEndpoint(vpoint);
	}

	vscheme->fwdPid = ERROR;
	vscheme->admAppPid = ERROR;
}

static int	raiseScheme(Object schemeElt, BpVdb *bpvdb)
{
	Sdr		bpSdr = getIonsdr();
	PsmPartition	bpwm = getIonwm();
	Object		schemeObj;
	Scheme		scheme;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	PsmAddress	addr;
	Object		elt;

	schemeObj = sdr_list_data(bpSdr, schemeElt);
	sdr_read(bpSdr, (char *) &scheme, schemeObj, sizeof(Scheme));
	findScheme(scheme.name, &vscheme, &vschemeElt);
	if (vschemeElt)		/*	Scheme is already raised.	*/
	{
		return 0;
	}

	addr = psm_malloc(bpwm, sizeof(VScheme));
	if (addr == 0)
	{
		return -1;
	}

	vschemeElt = sm_list_insert_last(bpwm, bpvdb->schemes, addr);
	if (vschemeElt == 0)
	{
		psm_free(bpwm, addr);
		return -1;
	}

	vscheme = (VScheme *) psp(bpwm, addr);
	memset((char *) vscheme, 0, sizeof(VScheme));
	vscheme->schemeElt = schemeElt;
	istrcpy(vscheme->name, scheme.name, sizeof vscheme->name);
	vscheme->nameLength = scheme.nameLength;
	vscheme->cbhe = scheme.cbhe;
	vscheme->unicast = scheme.unicast;
	vscheme->endpoints = sm_list_create(bpwm);
	if (vscheme->endpoints == 0)
	{
		oK(sm_list_delete(bpwm, vschemeElt, NULL, NULL));
		psm_free(bpwm, addr);
		return -1;
	}

	for (elt = sdr_list_first(bpSdr, scheme.endpoints); elt;
			elt = sdr_list_next(bpSdr, elt))
	{
		if (raiseEndpoint(vscheme, elt) < 0)
		{
			oK(sm_list_destroy(bpwm, vscheme->endpoints, NULL,
					NULL));
			oK(sm_list_delete(bpwm, vschemeElt, NULL, NULL));
			psm_free(bpwm, addr);
			return -1;
		}
	}

	vscheme->semaphore = SM_SEM_NONE;
	resetScheme(vscheme);
	return 0;
}

static void	dropScheme(VScheme *vscheme, PsmAddress vschemeElt)
{
	PsmPartition	bpwm = getIonwm();
	PsmAddress	vschemeAddr;
	PsmAddress	elt;
	PsmAddress	nextElt;
	PsmAddress	endpointAddr;
	VEndpoint	*vpoint;

	vschemeAddr = sm_list_data(bpwm, vschemeElt);
	if (vscheme->semaphore != SM_SEM_NONE)
	{
		sm_SemDelete(vscheme->semaphore);
	}

	/*	Drop all endpoints for this scheme.			*/

	for (elt = sm_list_first(bpwm, vscheme->endpoints); elt; elt = nextElt)
	{
		nextElt = sm_list_next(bpwm, elt);
		endpointAddr = sm_list_data(bpwm, elt);
		vpoint = (VEndpoint *) psp(bpwm, endpointAddr);
		dropEndpoint(vpoint, elt);
	}

	oK(sm_list_delete(bpwm, vschemeElt, NULL, NULL));
	psm_free(bpwm, vschemeAddr);
}

static void	startScheme(VScheme *vscheme)
{
	Sdr		bpSdr = getIonsdr();
	char		hostNameBuf[MAXHOSTNAMELEN + 1];
	MetaEid		metaEid;
	VScheme		*vscheme2;
	PsmAddress	vschemeElt;
	VEndpoint	*vpoint;
	PsmAddress	vpointElt;
	Scheme		scheme;
	char		cmdString[SDRSTRING_BUFSZ];

	/*	Compute admin EID for this scheme.			*/

	if (isUnicast(vscheme->name))
	{
		if (isCbhe(vscheme->name))
		{
			isprintf(vscheme->adminEid, sizeof vscheme->adminEid,
				"%.8s:" UVAST_FIELDSPEC ".0", vscheme->name,
				getOwnNodeNbr());
		}
		else	/*	Assume it's "dtn".			*/
		{
#ifdef ION_NO_DNS
			istrcpy(hostNameBuf, "localhost", sizeof hostNameBuf);
#else
			getNameOfHost(hostNameBuf, MAXHOSTNAMELEN);
#endif
			isprintf(vscheme->adminEid, sizeof vscheme->adminEid,
				"%.15s://%.60s.dtn", vscheme->name,
				hostNameBuf);
		}

		if (parseEidString(vscheme->adminEid, &metaEid, &vscheme2,
				&vschemeElt) == 0)
		{
			restoreEidString(&metaEid);
			writeMemoNote("[?] Malformed admin EID string",
					vscheme->adminEid);
			vscheme->adminNSSLength = 0;
		}
		else
		{
			vscheme->adminNSSLength = metaEid.nssLength;

			/*	Make sure admin endpoint exists.	*/

			findEndpoint(vscheme->name, metaEid.nss, vscheme,
					&vpoint, &vpointElt);
			restoreEidString(&metaEid);
			if (vpointElt == 0)
			{
				if (addEndpoint(vscheme->adminEid,
						EnqueueBundle, NULL) < 1)
				{
					restoreEidString(&metaEid);
					writeMemoNote("Can't add admin \
endpoint", vscheme->adminEid);
					vscheme->adminNSSLength = 0;
				}
			}
		}
	}

	/*	Start forwarder and administrative endpoint daemons.	*/

	sdr_read(bpSdr, (char *) &scheme, sdr_list_data(bpSdr,
			vscheme->schemeElt), sizeof(Scheme));
	if (scheme.fwdCmd != 0)
	{
		sdr_string_read(bpSdr, cmdString, scheme.fwdCmd);
		vscheme->fwdPid = pseudoshell(cmdString);
	}

	if (scheme.admAppCmd != 0)
	{
		sdr_string_read(bpSdr, cmdString, scheme.admAppCmd);
		vscheme->admAppPid = pseudoshell(cmdString);
	}
}

static void	stopScheme(VScheme *vscheme)
{
	PsmPartition	bpwm = getIonwm();
	PsmAddress	elt;
	PsmAddress	endpointAddr;
	VEndpoint	*vpoint;

	if (vscheme->semaphore != SM_SEM_NONE)
	{
		sm_SemEnd(vscheme->semaphore);	/*	Stop fwd.	*/
	}

	if (vscheme->admAppPid != ERROR)
	{
		sm_TaskKill(vscheme->admAppPid, SIGTERM);
	}

	for (elt = sm_list_first(bpwm, vscheme->endpoints); elt;
			elt = sm_list_next(bpwm, elt))
	{
		endpointAddr = sm_list_data(bpwm, elt);
		vpoint = (VEndpoint *) psp(bpwm, endpointAddr);
		if (vpoint->semaphore != SM_SEM_NONE)
		{
			sm_SemEnd(vpoint->semaphore);
		}
	}
}

static void	waitForScheme(VScheme *vscheme)
{
	if (vscheme->fwdPid != ERROR)
	{
		while (sm_TaskExists(vscheme->fwdPid))
		{
			microsnooze(1000000);
		}
	}

	if (vscheme->admAppPid != ERROR)
	{
		while (sm_TaskExists(vscheme->admAppPid))
		{
			microsnooze(1000000);
		}
	}
}

static void	resetInduct(VInduct *vduct)
{
	if (vduct->acqThrottle.semaphore == SM_SEM_NONE)
	{
		vduct->acqThrottle.semaphore = sm_SemCreate(SM_NO_KEY,
				SM_SEM_FIFO);
	}
	else
	{
		sm_SemUnend(vduct->acqThrottle.semaphore);
	}

	sm_SemTake(vduct->acqThrottle.semaphore);	/*	Lock.	*/
	vduct->cliPid = ERROR;
}

static int	raiseInduct(Object inductElt, BpVdb *bpvdb)
{
	Sdr		bpSdr = getIonsdr();
	PsmPartition	bpwm = getIonwm();
	Object		inductObj;
	Induct		duct;
	ClProtocol	protocol;
	VInduct		*vduct;
	PsmAddress	vductElt;
	PsmAddress	addr;

	inductObj = sdr_list_data(bpSdr, inductElt);
	sdr_read(bpSdr, (char *) &duct, inductObj, sizeof(Induct));
	sdr_read(bpSdr, (char *) &protocol, duct.protocol, sizeof(ClProtocol));
	findInduct(protocol.name, duct.name, &vduct, &vductElt);
	if (vductElt)		/*	Duct is already raised.		*/
	{
		return 0;
	}

	addr = psm_malloc(bpwm, sizeof(VInduct));
	if (addr == 0)
	{
		return -1;
	}

	vductElt = sm_list_insert_last(bpwm, bpvdb->inducts, addr);
	if (vductElt == 0)
	{
		psm_free(bpwm, addr);
		return -1;
	}

	vduct = (VInduct *) psp(bpwm, addr);
	memset((char *) vduct, 0, sizeof(VInduct));
	vduct->inductElt = inductElt;
	vduct->stats = duct.stats;
	vduct->updateStats = duct.updateStats;
	istrcpy(vduct->protocolName, protocol.name, sizeof vduct->protocolName);
	istrcpy(vduct->ductName, duct.name, sizeof vduct->ductName);
	vduct->acqThrottle.semaphore = SM_SEM_NONE;
	resetInduct(vduct);
	return 0;
}

static void	dropInduct(VInduct *vduct, PsmAddress vductElt)
{
	PsmPartition	bpwm = getIonwm();
	PsmAddress	vductAddr;

	vductAddr = sm_list_data(bpwm, vductElt);
	if (vduct->acqThrottle.semaphore != SM_SEM_NONE)
	{
		sm_SemDelete(vduct->acqThrottle.semaphore);
	}

	oK(sm_list_delete(bpwm, vductElt, NULL, NULL));
	psm_free(bpwm, vductAddr);
}

static void	startInduct(VInduct *vduct)
{
	Sdr	bpSdr = getIonsdr();
	Induct	induct;
	char	cmd[SDRSTRING_BUFSZ];
	char	cmdString[SDRSTRING_BUFSZ + 1 + MAX_CL_DUCT_NAME_LEN + 1];

	sdr_read(bpSdr, (char *) &induct, sdr_list_data(bpSdr,
			vduct->inductElt), sizeof(Induct));
	if (induct.cliCmd != 0)
	{
		sdr_string_read(bpSdr, cmd, induct.cliCmd);
		isprintf(cmdString, sizeof cmdString, "%s %s", cmd,
				induct.name);
		vduct->cliPid = pseudoshell(cmdString);
	}
}

static void	stopInduct(VInduct *vduct)
{
	if (vduct->cliPid != ERROR)
	{
		sm_TaskKill(vduct->cliPid, SIGTERM);
	}

	if (vduct->acqThrottle.semaphore != SM_SEM_NONE)
	{
		sm_SemEnd(vduct->acqThrottle.semaphore);
	}
}

static void	waitForInduct(VInduct *vduct)
{
	if (vduct->cliPid != ERROR)
	{
		while (sm_TaskExists(vduct->cliPid))
		{
			microsnooze(1000000);
		}
	}
}

static void	resetOutduct(VOutduct *vduct)
{
	if (vduct->semaphore == SM_SEM_NONE)
	{
		vduct->semaphore = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	}
	else
	{
		sm_SemUnend(vduct->semaphore);
	}

	sm_SemTake(vduct->semaphore);			/*	Lock.	*/
	if (vduct->xmitThrottle.semaphore == SM_SEM_NONE)
	{
		vduct->xmitThrottle.semaphore = sm_SemCreate(SM_NO_KEY,
				SM_SEM_FIFO);
	}
	else
	{
		sm_SemUnend(vduct->xmitThrottle.semaphore);
	}

	sm_SemTake(vduct->xmitThrottle.semaphore);	/*	Lock.	*/
	vduct->cloPid = ERROR;
}

static int	raiseOutduct(Object outductElt, BpVdb *bpvdb)
{
	Sdr		bpSdr = getIonsdr();
	PsmPartition	bpwm = getIonwm();
	Object		outductObj;
	Outduct		duct;
	ClProtocol	protocol;
	VOutduct	*vduct;
	PsmAddress	vductElt;
	PsmAddress	addr;

	outductObj = sdr_list_data(bpSdr, outductElt);
	sdr_read(bpSdr, (char *) &duct, outductObj, sizeof(Outduct));
	sdr_read(bpSdr, (char *) &protocol, duct.protocol, sizeof(ClProtocol));
	findOutduct(protocol.name, duct.name, &vduct, &vductElt);
	if (vductElt)		/*	Duct is already raised.		*/
	{
		return 0;
	}

	addr = psm_malloc(bpwm, sizeof(VOutduct));
	if (addr == 0)
	{
		return -1;
	}

	vductElt = sm_list_insert_last(bpwm, bpvdb->outducts, addr);
	if (vductElt == 0)
	{
		psm_free(bpwm, addr);
		return -1;
	}

	vduct = (VOutduct *) psp(bpwm, addr);
	memset((char *) vduct, 0, sizeof(VOutduct));
	vduct->outductElt = outductElt;
	vduct->stats = duct.stats;
	vduct->updateStats = duct.updateStats;
	istrcpy(vduct->protocolName, protocol.name, sizeof vduct->protocolName);
	istrcpy(vduct->ductName, duct.name, sizeof vduct->ductName);
	vduct->semaphore = SM_SEM_NONE;
	vduct->xmitThrottle.semaphore = SM_SEM_NONE;
	resetOutduct(vduct);
	return 0;
}

static void	dropOutduct(VOutduct *vduct, PsmAddress vductElt)
{
	PsmPartition	bpwm = getIonwm();
	PsmAddress	vductAddr;

	vductAddr = sm_list_data(bpwm, vductElt);
	if (vduct->semaphore != SM_SEM_NONE)
	{
		sm_SemDelete(vduct->semaphore);
	}

	if (vduct->xmitThrottle.semaphore != SM_SEM_NONE)
	{
		sm_SemDelete(vduct->xmitThrottle.semaphore);
	}

	oK(sm_list_delete(bpwm, vductElt, NULL, NULL));
	psm_free(bpwm, vductAddr);
}

static void	startOutduct(VOutduct *vduct)
{
	Sdr	bpSdr = getIonsdr();
	Outduct	outduct;
	char	cmd[SDRSTRING_BUFSZ];
	char	cmdString[SDRSTRING_BUFSZ + 1 + MAX_CL_DUCT_NAME_LEN + 1];

	sdr_read(bpSdr, (char *) &outduct, sdr_list_data(bpSdr,
			vduct->outductElt), sizeof(Outduct));
	if (outduct.cloCmd != 0)
	{
		sdr_string_read(bpSdr, cmd, outduct.cloCmd);
		isprintf(cmdString, sizeof cmdString, "%s %s", cmd,
				outduct.name);
		vduct->cloPid = pseudoshell(cmdString);
	}
}

static void	stopOutduct(VOutduct *vduct)
{
	if (vduct->semaphore != SM_SEM_NONE)	/*	Stop CLO.	*/
	{
		sm_SemEnd(vduct->semaphore);
	}

	if (vduct->xmitThrottle.semaphore != SM_SEM_NONE)
	{
		sm_SemEnd(vduct->xmitThrottle.semaphore);
	}
}

static void	waitForOutduct(VOutduct *vduct)
{
	if (vduct->cloPid != ERROR)
	{
		while (sm_TaskExists(vduct->cloPid))
		{
			microsnooze(1000000);
		}
	}
}

static int	raiseProtocol(Address addr, BpVdb *bpvdb)
{
	Sdr	bpSdr = getIonsdr();
		OBJ_POINTER(ClProtocol, protocol);
	Object	elt;

	GET_OBJ_POINTER(bpSdr, ClProtocol, protocol, addr);
	for (elt = sdr_list_first(bpSdr, protocol->inducts); elt;
			elt = sdr_list_next(bpSdr, elt))
	{
		if (raiseInduct(elt, bpvdb) < 0)
		{
			putErrmsg("Can't raise induct.", NULL);
			return -1;
		}
	}

	for (elt = sdr_list_first(bpSdr, protocol->outducts); elt;
			elt = sdr_list_next(bpSdr, elt))
	{
		if (raiseOutduct(elt, bpvdb) < 0)
		{
			putErrmsg("Can't raise outduct.", NULL);
			return -1;
		}
	}

	return 0;
}

static char	*_bpvdbName()
{
	return "bpvdb";
}

int	orderBpEvents(PsmPartition partition, PsmAddress nodeData,
		void *dataBuffer)
{
	Sdr	sdr = getIonsdr();
	BpEvent	*argEvent;
	Object	elt;
	BpEvent	event;

	if (partition == NULL || nodeData == 0 || dataBuffer == 0)
	{
		putErrmsg("Error calling smrbt BP timeline compare function.",
				NULL);
		return 0;
	}

	argEvent = (BpEvent *) dataBuffer;
	elt = (Object) nodeData;
	sdr_read(sdr, (char *) &event, sdr_list_data(sdr, elt),
			sizeof(BpEvent));
	if (event.time < argEvent->time)
	{
		return -1;
	}

	if (event.time > argEvent->time)
	{
		return 1;
	}

	/*	Same time.						*/

	if (event.ref < argEvent->ref)
	{
		return -1;
	}

	if (event.ref > argEvent->ref)
	{
		return 1;
	}

	/*	Same object.						*/

	if (event.type < argEvent->type)
	{
		return -1;
	}

	if (event.type > argEvent->type)
	{
		return 1;
	}

	return 0;
}

static BpVdb	*_bpvdb(char **name)
{
	static BpVdb	*vdb = NULL;
	PsmPartition	wm;
	PsmAddress	vdbAddress;
	PsmAddress	elt;
	Sdr		sdr;
	BpDB		*db;
	Object		sdrElt;
	Object		addr;
	BpEvent		event;

	if (name)
	{
		if (*name == NULL)	/*	Terminating.		*/
		{
			vdb = NULL;
			return vdb;
		}

		/*	Attaching to volatile database.			*/

		wm = getIonwm();
		if (psm_locate(wm, *name, &vdbAddress, &elt) < 0)
		{
			putErrmsg("Failed searching for vdb.", NULL);
			return vdb;
		}

		if (elt)
		{
			vdb = (BpVdb *) psp(wm, vdbAddress);
			return vdb;
		}

		/*	BP volatile database doesn't exist yet.		*/
		sdr = getIonsdr();
		CHKNULL(sdr_begin_xn(sdr));	/*	To lock memory.	*/
		vdbAddress = psm_zalloc(wm, sizeof(BpVdb));
		if (vdbAddress == 0)
		{
			sdr_exit_xn(sdr);
			putErrmsg("No space for dynamic database.", NULL);
			return NULL;
		}

		db = _bpConstants();
		vdb = (BpVdb *) psp(wm, vdbAddress);
		memset((char *) vdb, 0, sizeof(BpVdb));
		vdb->sourceStats = db->sourceStats;
		vdb->recvStats = db->recvStats;
		vdb->discardStats = db->discardStats;
		vdb->xmitStats = db->xmitStats;
		vdb->rptStats = db->rptStats;
		vdb->ctStats = db->ctStats;
		vdb->dbStats = db->dbStats;
		vdb->updateStats = db->updateStats;
		vdb->creationTimeSec = 0;
		vdb->bundleCounter = 0;
		vdb->clockPid = ERROR;
		vdb->watching = db->watching;
		if ((vdb->schemes = sm_list_create(wm)) == 0
		|| (vdb->inducts = sm_list_create(wm)) == 0
		|| (vdb->outducts = sm_list_create(wm)) == 0
		|| (vdb->timeline = sm_rbt_create(wm)) == 0
		|| psm_catlg(wm, *name, vdbAddress) < 0)
		{
			sdr_exit_xn(sdr);
			putErrmsg("Can't initialize volatile database.", NULL);
			return NULL;
		}

		/*	Raise all schemes and all of their endpoints.	*/

		for (sdrElt = sdr_list_first(sdr, db->schemes); sdrElt;
				sdrElt = sdr_list_next(sdr, sdrElt))
		{
			if (raiseScheme(sdrElt, vdb) < 0)
			{
				sdr_exit_xn(sdr);
				putErrmsg("Can't raise all schemes.", NULL);
				return NULL;
			}
		}

		/*	Raise all ducts for all CL protocol adapters.	*/

		for (sdrElt = sdr_list_first(sdr, db->protocols); sdrElt;
				sdrElt = sdr_list_next(sdr, sdrElt))
		{
			addr = sdr_list_data(sdr, sdrElt);
			if (raiseProtocol(addr, vdb) < 0)
			{
				sdr_exit_xn(sdr);
				putErrmsg("Can't raise all protocols.", NULL);
				return NULL;
			}
		}

		/*	Raise the timeline.				*/

		for (sdrElt = sdr_list_first(sdr, (_bpConstants())->timeline);
				sdrElt; sdrElt = sdr_list_next(sdr, sdrElt))
		{
			addr = sdr_list_data(sdr, sdrElt);
			sdr_read(sdr, (char *) &event, addr, sizeof(BpEvent));
			if (sm_rbt_insert(wm, vdb->timeline, (PsmAddress)
					sdrElt, orderBpEvents, &event) == 0)
			{
				sdr_exit_xn(sdr);
				putErrmsg("Can't stage event timeline.", NULL);
				return NULL;
			}
		}

		sdr_exit_xn(sdr);	/*	Unlock memory.		*/
	}

	return vdb;
}

static char	*_bpdbName()
{
	return "bpdb";
}

int	bpInit()
{
	Sdr		bpSdr;
	Object		bpdbObject;
	BpDB		bpdbBuf;
	BpCosStats	cosStatsInit;
	BpRptStats	rptStatsInit;
	BpCtStats	ctStatsInit;
	BpDbStats	dbStatsInit;
	char		*bpvdbName = _bpvdbName();

	if (ionAttach() < 0)
	{
		putErrmsg("BP can't attach to ION.", NULL);
		return -1;
	}

	bpSdr = getIonsdr();

	/*	Recover the BP database, creating it if necessary.	*/

	CHKERR(sdr_begin_xn(bpSdr));
	bpdbObject = sdr_find(bpSdr, _bpdbName(), NULL);
	switch (bpdbObject)
	{
	case -1:		/*	SDR error.			*/
		putErrmsg("Can't search for BP database in SDR.", NULL);
		sdr_cancel_xn(bpSdr);
		return -1;

	case 0:			/*	Not found; must create new DB.	*/
		bpdbObject = sdr_malloc(bpSdr, sizeof(BpDB));
		if (bpdbObject == 0)
		{
			putErrmsg("No space for database.", NULL);
			sdr_cancel_xn(bpSdr);
			return -1;
		}

		/*	Initialize the non-volatile database.		*/

		memset((char *) &bpdbBuf, 0, sizeof(BpDB));
		bpdbBuf.schemes = sdr_list_create(bpSdr);
		bpdbBuf.protocols = sdr_list_create(bpSdr);
		bpdbBuf.timeline = sdr_list_create(bpSdr);
		bpdbBuf.bundles = sdr_hash_create(bpSdr,
				BUNDLES_HASH_KEY_LEN,
				BUNDLES_HASH_ENTRIES,
				BUNDLES_HASH_SEARCH_LEN);
		bpdbBuf.inboundBundles = sdr_list_create(bpSdr);
		bpdbBuf.limboQueue = sdr_list_create(bpSdr);
		bpdbBuf.clockCmd = sdr_string_create(bpSdr, "bpclock");
		bpdbBuf.sourceStats = sdr_malloc(bpSdr, sizeof(BpCosStats));
		bpdbBuf.recvStats = sdr_malloc(bpSdr, sizeof(BpCosStats));
		bpdbBuf.discardStats = sdr_malloc(bpSdr, sizeof(BpCosStats));
		bpdbBuf.xmitStats = sdr_malloc(bpSdr, sizeof(BpCosStats));
		bpdbBuf.rptStats = sdr_malloc(bpSdr, sizeof(BpRptStats));
		bpdbBuf.ctStats = sdr_malloc(bpSdr, sizeof(BpCtStats));
		bpdbBuf.dbStats = sdr_malloc(bpSdr, sizeof(BpDbStats));
		if (bpdbBuf.sourceStats && bpdbBuf.recvStats
		&& bpdbBuf.discardStats && bpdbBuf.xmitStats
		&& bpdbBuf.rptStats && bpdbBuf.ctStats && bpdbBuf.dbStats)
		{
			memset((char *) &cosStatsInit, 0,
					sizeof(BpCosStats));
			sdr_write(bpSdr, bpdbBuf.sourceStats,
					(char *) &cosStatsInit,
					sizeof(BpCosStats));
			sdr_write(bpSdr, bpdbBuf.recvStats,
					(char *) &cosStatsInit,
					sizeof(BpCosStats));
			sdr_write(bpSdr, bpdbBuf.discardStats,
					(char *) &cosStatsInit,
					sizeof(BpCosStats));
			sdr_write(bpSdr, bpdbBuf.xmitStats,
					(char *) &cosStatsInit,
					sizeof(BpCosStats));
			memset((char *) &rptStatsInit, 0,
					sizeof(BpRptStats));
			sdr_write(bpSdr, bpdbBuf.rptStats,
					(char *) &rptStatsInit,
					sizeof(BpRptStats));
			memset((char *) &ctStatsInit, 0,
					sizeof(BpCtStats));
			sdr_write(bpSdr, bpdbBuf.ctStats,
					(char *) &ctStatsInit,
					sizeof(BpCtStats));
			memset((char *) &dbStatsInit, 0,
					sizeof(BpDbStats));
			sdr_write(bpSdr, bpdbBuf.dbStats,
					(char *) &dbStatsInit,
					sizeof(BpDbStats));
		}

		bpdbBuf.updateStats = 1;	/*	Default.	*/
		sdr_write(bpSdr, bpdbObject, (char *) &bpdbBuf, sizeof(BpDB));
		sdr_catlg(bpSdr, _bpdbName(), 0, bpdbObject);
		if (sdr_end_xn(bpSdr))
		{
			putErrmsg("Can't create BP database.", NULL);
			return -1;
		}

		break;

	default:		/*	Found DB in the SDR.		*/
		sdr_exit_xn(bpSdr);
	}

	oK(_bpdbObject(&bpdbObject));	/*	Save database location.	*/
	oK(_bpConstants());

	/*	Locate volatile database, initializing as necessary.	*/

	if (_bpvdb(&bpvdbName) == NULL)
	{
		putErrmsg("BP can't initialize vdb.", NULL);
		return -1;
	}

	if (secAttach() < 0)
	{
		writeMemo("[?] Warning: running without bundle security.");
	}
	else
	{
		writeMemo("[i] Bundle security is enabled.");
	}

	return 0;		/*	BP service is now available.	*/
}

static void	dropVdb(PsmPartition wm, PsmAddress vdbAddress)
{
	BpVdb		*vdb;
	PsmAddress	elt;
	VScheme		*vscheme;
	VInduct		*vinduct;
	VOutduct	*voutduct;

	vdb = (BpVdb *) psp(wm, vdbAddress);
	while ((elt = sm_list_first(wm, vdb->schemes)) != 0)
	{
		vscheme = (VScheme *) psp(wm, sm_list_data(wm, elt));
		dropScheme(vscheme, elt);
	}

	sm_list_destroy(wm, vdb->schemes, NULL, NULL);
	while ((elt = sm_list_first(wm, vdb->inducts)) != 0)
	{
		vinduct = (VInduct *) psp(wm, sm_list_data(wm, elt));
		dropInduct(vinduct, elt);
	}

	sm_list_destroy(wm, vdb->inducts, NULL, NULL);
	while ((elt = sm_list_first(wm, vdb->outducts)) != 0)
	{
		voutduct = (VOutduct *) psp(wm, sm_list_data(wm, elt));
		dropOutduct(voutduct, elt);
	}

	sm_list_destroy(wm, vdb->outducts, NULL, NULL);
	sm_rbt_destroy(wm, vdb->timeline, NULL, NULL);
}

void	bpDropVdb()
{
	PsmPartition	wm = getIonwm();
	char		*bpvdbName = _bpvdbName();
	PsmAddress	vdbAddress;
	PsmAddress	elt;
	char		*stop = NULL;

	if (psm_locate(wm, bpvdbName, &vdbAddress, &elt) < 0)
	{
		putErrmsg("Failed searching for vdb.", NULL);
		return;
	}

	if (elt)
	{
		dropVdb(wm, vdbAddress);	/*	Destroy Vdb.	*/
		psm_free(wm, vdbAddress);
		if (psm_uncatlg(wm, bpvdbName) < 0)
		{
			putErrmsg("Failed uncataloging vdb.", NULL);
		}
	}

	oK(_bpvdb(&stop));			/*	Forget old Vdb.	*/
}

void	bpRaiseVdb()
{
	char	*bpvdbName = _bpvdbName();

	if (_bpvdb(&bpvdbName) == NULL)		/*	Create new Vdb.	*/
	{
		putErrmsg("BP can't reinitialize vdb.", NULL);
	}
}

Object	getBpDbObject()
{
	return _bpdbObject(NULL);
}

BpDB	*getBpConstants()
{
	return _bpConstants();
}

BpVdb	*getBpVdb()
{
	return _bpvdb(NULL);
}

int	bpStart()
{
	Sdr		bpSdr = getIonsdr();
	PsmPartition	bpwm = getIonwm();
	BpVdb		*bpvdb = _bpvdb(NULL);
	char		cmdString[SDRSTRING_BUFSZ];
	PsmAddress	elt;

	CHKERR(sdr_begin_xn(bpSdr));	/*	Just to lock memory.	*/

	/*	Start the bundle expiration clock if necessary.		*/

	if (bpvdb->clockPid == ERROR || sm_TaskExists(bpvdb->clockPid) == 0)
	{
		sdr_string_read(bpSdr, cmdString, (_bpConstants())->clockCmd);
		bpvdb->clockPid = pseudoshell(cmdString);
	}

	/*	Start forwarders and admin endpoints for all schemes.	*/

	for (elt = sm_list_first(bpwm, bpvdb->schemes); elt;
			elt = sm_list_next(bpwm, elt))
	{
		startScheme((VScheme *) psp(bpwm, sm_list_data(bpwm, elt)));
	}

	/*	Start all ducts for all convergence-layer adapters.	*/

	for (elt = sm_list_first(bpwm, bpvdb->inducts); elt;
			elt = sm_list_next(bpwm, elt))
	{
		startInduct((VInduct *) psp(bpwm, sm_list_data(bpwm, elt)));
	}

	for (elt = sm_list_first(bpwm, bpvdb->outducts); elt;
			elt = sm_list_next(bpwm, elt))
	{
		startOutduct((VOutduct *) psp(bpwm, sm_list_data(bpwm, elt)));
	}

	sdr_exit_xn(bpSdr);	/*	Unlock memory.			*/
	return 0;
}

void	bpStop()		/*	Reverses bpStart.		*/
{
	Sdr		bpSdr = getIonsdr();
	PsmPartition	bpwm = getIonwm();
	BpVdb		*bpvdb = _bpvdb(NULL);
	PsmAddress	elt;
	VScheme		*vscheme;
	VInduct		*vinduct;
	VOutduct	*voutduct;
	Object		zcoElt;
	Object		nextElt;
	Object		zco;

	/*	Tell all BP processes to stop.				*/

	CHKVOID(sdr_begin_xn(bpSdr));	/*	Just to lock memory.	*/
	for (elt = sm_list_first(bpwm, bpvdb->schemes); elt;
			elt = sm_list_next(bpwm, elt))
	{
		vscheme = (VScheme *) psp(bpwm, sm_list_data(bpwm, elt));
		stopScheme(vscheme);
	}

	for (elt = sm_list_first(bpwm, bpvdb->inducts); elt; elt = 
			sm_list_next(bpwm, elt))
	{
		vinduct = (VInduct *) psp(bpwm, sm_list_data(bpwm, elt));
		stopInduct(vinduct);
	}

	for (elt = sm_list_first(bpwm, bpvdb->outducts); elt; elt = 
			sm_list_next(bpwm, elt))
	{
		voutduct = (VOutduct *) psp(bpwm, sm_list_data(bpwm, elt));
		stopOutduct(voutduct);
	}

	if (bpvdb->clockPid != ERROR)
	{
		sm_TaskKill(bpvdb->clockPid, SIGTERM);
	}

	sdr_exit_xn(bpSdr);	/*	Unlock memory.			*/

	/*	Wait until all BP processes have stopped.		*/

	for (elt = sm_list_first(bpwm, bpvdb->schemes); elt;
			elt = sm_list_next(bpwm, elt))
	{
		vscheme = (VScheme *) psp(bpwm, sm_list_data(bpwm, elt));
		waitForScheme(vscheme);
	}

	for (elt = sm_list_first(bpwm, bpvdb->inducts); elt; elt = 
			sm_list_next(bpwm, elt))
	{
		vinduct = (VInduct *) psp(bpwm, sm_list_data(bpwm, elt));
		waitForInduct(vinduct);
	}

	for (elt = sm_list_first(bpwm, bpvdb->outducts); elt; elt = 
			sm_list_next(bpwm, elt))
	{
		voutduct = (VOutduct *) psp(bpwm, sm_list_data(bpwm, elt));
		waitForOutduct(voutduct);
	}

	if (bpvdb->clockPid != ERROR)
	{
		while (sm_TaskExists(bpvdb->clockPid))
		{
			microsnooze(100000);
		}
	}

	/*	Now erase all the tasks and reset the semaphores.	*/

	CHKVOID(sdr_begin_xn(bpSdr));
	bpvdb->clockPid = ERROR;
	for (elt = sm_list_first(bpwm, bpvdb->schemes); elt;
			elt = sm_list_next(bpwm, elt))
	{
		vscheme = (VScheme *) psp(bpwm, sm_list_data(bpwm, elt));
		resetScheme(vscheme);
	}

	for (elt = sm_list_first(bpwm, bpvdb->inducts); elt; elt = 
			sm_list_next(bpwm, elt))
	{
		vinduct = (VInduct *) psp(bpwm, sm_list_data(bpwm, elt));
		resetInduct(vinduct);
	}

	for (elt = sm_list_first(bpwm, bpvdb->outducts); elt; elt = 
			sm_list_next(bpwm, elt))
	{
		voutduct = (VOutduct *) psp(bpwm, sm_list_data(bpwm, elt));
		resetOutduct(voutduct);
	}

	/*	Clear out any partially received bundles, then exit.	*/

	for (zcoElt = sdr_list_first(bpSdr, (_bpConstants())->inboundBundles);
			zcoElt; zcoElt = nextElt)
	{
		nextElt = sdr_list_next(bpSdr, zcoElt);
		zco = sdr_list_data(bpSdr, zcoElt);
		zco_destroy(bpSdr, zco);
		sdr_list_delete(bpSdr, zcoElt, NULL, NULL);
	}

	oK(sdr_end_xn(bpSdr));
}

int	bpAttach()
{
	Object		bpdbObject = _bpdbObject(NULL);
	BpVdb		*bpvdb = _bpvdb(NULL);
	Sdr		bpSdr;
	char		*bpvdbName = _bpvdbName();

	if (bpdbObject && bpvdb)
	{
		return 0;		/*	Already attached.	*/
	}

	if (ionAttach() < 0)
	{
		putErrmsg("BP can't attach to ION.", NULL);
		return -1;
	}

	bpSdr = getIonsdr();

	/*	Locate the BP database.					*/

	if (bpdbObject == 0)
	{
		CHKERR(sdr_begin_xn(bpSdr));
		bpdbObject = sdr_find(bpSdr, _bpdbName(), NULL);
		sdr_exit_xn(bpSdr);
		if (bpdbObject == 0)
		{
			putErrmsg("Can't find BP database.", NULL);
			ionDetach();
			return -1;
		}

		oK(_bpdbObject(&bpdbObject));
	}

	oK(_bpConstants());

	/*	Locate the BP volatile database.			*/

	if (bpvdb == NULL)
	{
		if (_bpvdb(&bpvdbName) == NULL)
		{
			putErrmsg("BP volatile database not found.", NULL);
			ionDetach();
			return -1;
		}
	}

	oK(secAttach());
	return 0;		/*	BP service is now available.	*/
}

void bpDetach(){
	char *stop=NULL;
	oK(_bpvdb(&stop));
	return;
}

/*	*	*	Database occupancy functions	*	*	*/

static void	noteBundleInserted(Bundle *bundle)
{
	zco_increase_heap_occupancy(getIonsdr(), bundle->dbOverhead);
}

static void	noteBundleRemoved(Bundle *bundle)
{
	zco_reduce_heap_occupancy(getIonsdr(), bundle->dbOverhead);
}

/*	*	*	Useful utility functions	*	*	*/

void	getCurrentDtnTime(DtnTime *dt)
{
	time_t	currentTime;

	currentTime = getUTCTime();
	dt->seconds = currentTime - EPOCH_2000_SEC;	/*	30 yrs	*/
	dt->nanosec = 0;
}

void	computeApplicableBacklog(Outduct *duct, Bundle *bundle, Scalar *backlog)
{
	int	priority = COS_FLAGS(bundle->bundleProcFlags) & 0x03;
#ifdef ION_BANDWIDTH_RESERVED
	Scalar	maxBulkBacklog;
	Scalar	bulkBacklog;
#endif
	int	i;

	if (priority == 0)
	{
		copyScalar(backlog, &(duct->urgentBacklog));
		addToScalar(backlog, &(duct->stdBacklog));
		addToScalar(backlog, &(duct->bulkBacklog));
		return;
	}

	if (priority == 1)
	{
		copyScalar(backlog, &(duct->urgentBacklog));
		addToScalar(backlog, &(duct->stdBacklog));
#ifdef ION_BANDWIDTH_RESERVED
		/*	Additional backlog is the applicable bulk
		 *	backlog, which is the entire bulk backlog
		 *	or 1/2 of the std backlog, whichever is less.	*/

		copyScalar(&maxBulkBacklog, &(duct->stdBacklog));
		divideScalar(&maxBulkBacklog, 2);
		copyScalar(&bulkBacklog, &maxBulkBacklog);
		subtractFromScalar(&maxBulkBacklog, &(duct->bulkBacklog));
		if (scalarIsValid(&maxBulkBacklog))
		{
			/*	Current bulk backlog is less than half
			 *	of the current std backlog.		*/

			copyScalar(&bulkBacklog, &(duct->bulkBacklog));
		}

		addToScalar(backlog, &bulkBacklog);
#endif
		return;
	}

	/*	Priority is 2, i.e., urgent (expedited).		*/

	if ((i = bundle->extendedCOS.ordinal) == 0)
	{
		copyScalar(backlog, &(duct->urgentBacklog));
		return;
	}

	/*	Bundle has non-zero ordinal, so it may jump ahead of
	 *	some other urgent bundles.  Compute sum of backlogs
	 *	for this and all higher ordinals.			*/

	loadScalar(backlog, 0);
	while (i < 256)
	{
		addToScalar(backlog, &(duct->ordinals[i].backlog));
		i++;
	}
}

int	putBpString(BpString *bpString, char *string)
{
	Sdr	bpSdr = getIonsdr();

	if (bpString == NULL || string == NULL)
	{
		return -1;
	}

	CHKERR(ionLocked());
	bpString->textLength = strlen(string);
	bpString->text = sdr_malloc(bpSdr, bpString->textLength);
	if (bpString->text == 0)
	{
		return -1;
	}

	sdr_write(bpSdr, bpString->text, string, bpString->textLength);
	return 0;
}

char	*getBpString(BpString *bpString)
{
	char	*string;

	if (bpString == NULL || bpString->text == 0 || bpString->textLength < 1)
	{
		return NULL;
	}

	string = MTAKE(bpString->textLength + 1);
	if (string == NULL)
	{
		return (char *) bpString;	/*	Out of memory.	*/
	}

	sdr_read(getIonsdr(), string, bpString->text, bpString->textLength);
	*(string + bpString->textLength) = '\0';
	return string;
}

char	*retrieveDictionary(Bundle *bundle)
{
	char	*dictionary;

	if (bundle->dictionary == 0)
	{
		return NULL;
	}

	dictionary = MTAKE(bundle->dictionaryLength);
	if (dictionary == NULL)
	{
		return (char *) bundle;		/*	Out of memory.	*/
	}

	sdr_read(getIonsdr(), dictionary, bundle->dictionary,
			bundle->dictionaryLength);
	return dictionary;
}

void	releaseDictionary(char *dictionary)
{
	if (dictionary)
	{
		MRELEASE(dictionary);
	}
}

int	parseEidString(char *eidString, MetaEid *metaEid, VScheme **vscheme,
		PsmAddress *vschemeElt)
{
	/*	parseEidString is a Boolean function, returning 1 if
	 *	the EID string was successfully parsed.			*/

	CHKZERO(eidString && metaEid && vscheme && vschemeElt);

	/*	Handle special case of null endpoint ID.		*/

	if (strlen(eidString) == 8 && strcmp(eidString, _nullEid()) == 0)
	{
		metaEid->nullEndpoint = 1;
		metaEid->colon = NULL;
		metaEid->schemeName = "dtn";
		metaEid->schemeNameLength = 3;
		metaEid->nss = "none";
		metaEid->nssLength = 4;
		metaEid->nodeNbr = 0;
		metaEid->serviceNbr = 0;
		return 1;
	}

	/*	EID string does not identify the null endpoint.		*/

	metaEid->nullEndpoint = 0;
	metaEid->colon = strchr(eidString, ':');
	if (metaEid->colon == NULL)
	{
		writeMemoNote("[?] Malformed EID", eidString);
		return 0;
	}

	*(metaEid->colon) = '\0';
	metaEid->schemeName = eidString;
	metaEid->schemeNameLength = metaEid->colon - eidString;
	metaEid->nss = metaEid->colon + 1;
	metaEid->nssLength = strlen(metaEid->nss);

	/*	Look up scheme of endpoint URI.				*/

	findScheme(metaEid->schemeName, vscheme, vschemeElt);
	if (*vschemeElt == 0)
	{
		*(metaEid->colon) = ':';
		writeMemoNote("[?] Can't parse endpoint URI", eidString);
		return 0;
	}

	metaEid->cbhe = (*vscheme)->cbhe;
	if (!metaEid->cbhe)		/*	Non-CBHE.		*/
	{
		metaEid->nodeNbr = 0;
		metaEid->serviceNbr = 0;
		return 1;
	}

	/*	A CBHE-conformant endpoint URI.				*/

	if (sscanf(metaEid->nss, UVAST_FIELDSPEC ".%u", &(metaEid->nodeNbr),
			&(metaEid->serviceNbr)) < 2
	|| metaEid->nodeNbr > MAX_CBHE_NODE_NBR
	|| metaEid->serviceNbr > MAX_CBHE_SERVICE_NBR
	|| (metaEid->nodeNbr == 0 && metaEid->serviceNbr == 0))
	{
		*(metaEid->colon) = ':';
		writeMemoNote("[?] Malformed CBHE-conformant URI", eidString);
		return 0;
	}

	return 1;
}

void	restoreEidString(MetaEid *metaEid)
{
	if (metaEid)
	{
		if (metaEid->colon)
		{
			*(metaEid->colon) = ':';
		}

		memset((char *) metaEid, 0, sizeof(MetaEid));
	}
}

static int	printCbheEid(char *schemeName, CbheEid *eid, char **result)
{
	char	*eidString;
	int	eidLength = 46;

	/*	Printed EID string is
	 *
	 *	   xxx:<elementnbr>.<servicenbr>\0
	 *
	 * 	where xxx is either "ipn" or "imc" (multicast).
	 *	So max EID string length is 3 for "xxx" plus 1 for
	 *	':' plus max length of nodeNbr (which is a 64-bit
	 *	number, so 20 digits) plus 1 for '.' plus max lengthx
	 *	of serviceNbr (which is a 64-bit number, so 20 digits)
	 *	plus 1 for the terminating NULL.			*/

	eidString = MTAKE(eidLength);
	if (eidString == NULL)
	{
		putErrmsg("Can't create CBHE EID string.", NULL);
		return -1;
	}

	if (eid->nodeNbr == 0 && eid->serviceNbr == 0)
	{
		istrcpy(eidString, _nullEid(), eidLength);
	}
	else
	{
		isprintf(eidString, eidLength, "%s:" UVAST_FIELDSPEC ".%u",
				schemeName, eid->nodeNbr, eid->serviceNbr);
	}

	*result = eidString;
	return 0;
}

static int	printDtnEid(DtnEid *eid, char *dictionary, char **result)
{
	int	schemeNameLength;
	int	nssLength;
	int	eidLength;
	char	*eidString;

	CHKERR(dictionary);
	schemeNameLength = strlen(dictionary + eid->schemeNameOffset);
	nssLength = strlen(dictionary + eid->nssOffset);
	eidLength = schemeNameLength + nssLength + 2;
	eidString = MTAKE(eidLength);
	if (eidString == NULL)
	{
		putErrmsg("Can't create non-CBHE EID string.", NULL);
		return -1;
	}

	isprintf(eidString, eidLength, "%s:%s",
			dictionary + eid->schemeNameOffset,
			dictionary + eid->nssOffset);
	*result = eidString;
	return 0;
}

int	printEid(EndpointId *eid, char *dictionary, char **result)
{
	CHKERR(eid);
	CHKERR(result);
	if (eid->cbhe)
	{
		if (eid->unicast)
		{
			return printCbheEid("ipn", &eid->c, result);
		}
		else
		{
			return printCbheEid("imc", &eid->c, result);
		}
	}
	else
	{
		return printDtnEid(&(eid->d), dictionary, result);
	}
}

BpEidLookupFn	*senderEidLookupFunctions(BpEidLookupFn fn)
{
	static BpEidLookupFn	fns[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	int			i;

	if (fn == NULL)		/*	Requesting pointer to table.	*/
	{
		return fns;
	}

	for (i = 0; i < 16; i++)
	{
		if (fns[i] == fn)
		{
			break;		/*	Already in table.	*/
		}

		if (fns[i] == NULL)
		{
			fns[i] = fn;	/*	Add to table.		*/
			break;
		}
	}

	if (i == 16)
	{
		writeMemo("[?] EID lookup functions table is full.");
	}

	return NULL;
}

void	getSenderEid(char **eidBuffer, char *neighborClEid)
{
	BpEidLookupFn	*lookupFns;
	int		i;
	BpEidLookupFn	lookupEid;
	Sdr		sdr = getIonsdr();
	int		result;

	CHKVOID(eidBuffer);
	CHKVOID(*eidBuffer);
	CHKVOID(*neighborClEid);
	lookupFns = senderEidLookupFunctions(NULL);
	for (i = 0; i < 16; i++)
	{
		lookupEid = lookupFns[i];
		if (lookupEid == NULL)
		{
			break;		/*	Reached end of table.	*/
		}

		CHKVOID(sdr_begin_xn(sdr));
		result = lookupEid(*eidBuffer, neighborClEid);
		sdr_exit_xn(sdr);
		switch (result)
		{
		case -1:
			putErrmsg("Failed getting sender EID.", NULL);
			sm_Abort();
			break;

		case 0:
			continue;	/*	No match yet.		*/

		default:
			return;		/*	Figured out sender EID.	*/
		}
	}

	*eidBuffer = NULL;	/*	Sender EID not found.		*/
}

int	clIdMatches(char *neighborClId, FwdDirective *dir)
{
	Sdr	sdr = getIonsdr();
	char	*firstNonNumeric;
	char	ductNameBuffer[SDRSTRING_BUFSZ];
	char	*ductClId;
	Object	ductObj;
		OBJ_POINTER(Outduct, duct);
	int	neighborIdLen;
	int	ductIdLen;
	int	idLen;
	int	digitCount UNUSED;

	if (dir->action == fwd)
	{
		return 0;
	}

	/*	This is a directive for transmission to a neighbor.	*/

	neighborIdLen = strlen(neighborClId);
	if (dir->destDuctName)
	{
		if (sdr_string_read(sdr, ductNameBuffer, dir->destDuctName) < 0)
		{
			putErrmsg("Missing dest duct name.", NULL);
			return -1;		/*	DB error.	*/
		}

		ductClId = ductNameBuffer;
	}
	else
	{
		ductObj = sdr_list_data(sdr, dir->outductElt);
		GET_OBJ_POINTER(sdr, Outduct, duct, ductObj);
		ductClId = duct->name;
	}

	if (strcmp(ductClId, "localhost") == 0)
	{
		/*	Convert to dotted-string representation for
		 *	match with canonical form of the IPv4 address
		 *	that the neighbor CL ID must be.		*/

		ductClId = "127.0.0.1";
	}

	ductIdLen = strlen(ductClId);
	digitCount = strtol(ductClId, &firstNonNumeric, 0);
	if (*firstNonNumeric == '\0')
	{
		/*	Neighbor CL ID is a number, e.g., an LTP
		 *	engine number.  IDs must be the same length
		 *	in order to match.				*/

		 if (ductIdLen != neighborIdLen)
		 {
			 return 0;	/*	Different numbers.	*/
		 }

		 idLen = ductIdLen;
	}
	else
	{
		/*	IDs are character strings, e.g., hostnames.
		 *	Matches if shorter string matches the leading
		 *	characters of longer string.			*/

		idLen = neighborIdLen < ductIdLen ? neighborIdLen : ductIdLen;
	}

	if (strncmp(neighborClId, ductClId, idLen) == 0)
	{
		return 1;		/*	Found neighbor's duct.	*/
	}

	return 0;			/*	A different neighbor.	*/
}

int	startBpTask(Object cmd, Object cmdParms, int *pid)
{
	Sdr	bpSdr = getIonsdr();
	char	buffer[600];
	char	cmdString[SDRSTRING_BUFSZ];
	char	parmsString[SDRSTRING_BUFSZ];

	if (sdr_string_read(bpSdr, cmdString, cmd) < 0)
	{
		putErrmsg("Failed reading command.", NULL);
		return -1;
	}

	if (cmdParms == 0)
	{
		istrcpy(buffer, cmdString, sizeof buffer);
	}
	else
	{
		if (sdr_string_read(bpSdr, parmsString, cmdParms) < 0)
		{
			putErrmsg("Failed reading command parms.", NULL);
			return -1;
		}

		isprintf(buffer, sizeof buffer, "%s %s", cmdString,
				parmsString);
	}

	*pid = pseudoshell(buffer);
	if (*pid == ERROR)
	{
		putErrmsg("Can't start task.", buffer);
		return -1;
	}

	return 0;
}

static void	lookUpEidScheme(EndpointId eid, char *dictionary,
			VScheme **vscheme)
{
	PsmPartition	bpwm = getIonwm();
	BpVdb		*bpvdb = _bpvdb(NULL);
	char		*schemeName;
	PsmAddress	elt;

	if (dictionary == NULL)
	{
		if (!eid.cbhe)		/*	Can't determine scheme.	*/
		{
			*vscheme = NULL;
			return;
		}

		if (eid.unicast)
		{
			schemeName = "ipn";
		}
		else
		{
			schemeName = "imc";
		}
	}
	else
	{
		schemeName = dictionary + eid.d.schemeNameOffset;
	}

	for (elt = sm_list_first(bpwm, bpvdb->schemes); elt;
			elt = sm_list_next(bpwm, elt))
	{
		*vscheme = (VScheme *) psp(bpwm, sm_list_data(bpwm, elt));
		if (strcmp((*vscheme)->name, schemeName) == 0)
		{
			return;		/*	Found the scheme.	*/
		}
	}

	/*	No match found.						*/

	*vscheme = NULL;
}

static void	reportStateStats(int i, char *fromTimestamp, char *toTimestamp,
			unsigned int count_0, uvast bytes_0,
			unsigned int count_1, uvast bytes_1,
			unsigned int count_2, uvast bytes_2,
			unsigned int count_total, uvast bytes_total)
{
	char		buffer[256];
	static char	*classnames[] =
		{ "src", "fwd", "xmt", "rcv", "dlv", "ctr", "rfw", "exp" };

	isprintf(buffer, sizeof buffer, "[x] %s from %s to %s: (0) \
%u " UVAST_FIELDSPEC " (1) %u " UVAST_FIELDSPEC " (2) \
%u " UVAST_FIELDSPEC " (+) %u " UVAST_FIELDSPEC, classnames[i],
			fromTimestamp, toTimestamp,
			count_0, bytes_0, count_1, bytes_1,
			count_2, bytes_2, count_total, bytes_total);
	writeMemo(buffer);
}

void	reportAllStateStats()
{
	Sdr		sdr = getIonsdr();
	Object		bpDbObject = getBpDbObject();
	time_t		currentTime;
	char		toTimestamp[20];
	BpDB		bpdb;
	time_t		startTime;
	char		fromTimestamp[20];
	BpCosStats	sourceStats;
	BpCosStats	recvStats;
	BpCosStats	xmitStats;
	BpDbStats	dbStats;

	currentTime = getUTCTime();
	writeTimestampLocal(currentTime, toTimestamp);
	CHKVOID(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &bpdb, bpDbObject, sizeof(BpDB));
	startTime = bpdb.resetTime;
	writeTimestampLocal(startTime, fromTimestamp);

	/*	Sourced.						*/

	sdr_read(sdr, (char *) &sourceStats, bpdb.sourceStats,
			sizeof(BpCosStats));
	reportStateStats(0, fromTimestamp, toTimestamp,
			sourceStats.tallies[0].currentCount,
			sourceStats.tallies[0].currentBytes,
			sourceStats.tallies[1].currentCount,
			sourceStats.tallies[1].currentBytes,
			sourceStats.tallies[2].currentCount,
			sourceStats.tallies[2].currentBytes,
			sourceStats.tallies[0].currentCount +
				sourceStats.tallies[1].currentCount +
				sourceStats.tallies[2].currentCount,
			sourceStats.tallies[0].currentBytes +
				sourceStats.tallies[1].currentBytes +
				sourceStats.tallies[2].currentBytes);

	/*	Forwarded.						*/

	sdr_read(sdr, (char *) &dbStats, bpdb.dbStats, sizeof(BpDbStats));
	reportStateStats(1, fromTimestamp, toTimestamp, 0, 0, 0, 0, 0, 0,
			dbStats.tallies[BP_DB_FWD_OKAY].currentCount,
			dbStats.tallies[BP_DB_FWD_OKAY].currentBytes);

	/*	Transmitted.						*/

	sdr_read(sdr, (char *) &xmitStats, bpdb.xmitStats, sizeof(BpCosStats));
	reportStateStats(2, fromTimestamp, toTimestamp,
			xmitStats.tallies[0].currentCount,
			xmitStats.tallies[0].currentBytes,
			xmitStats.tallies[1].currentCount,
			xmitStats.tallies[1].currentBytes,
			xmitStats.tallies[2].currentCount,
			xmitStats.tallies[2].currentBytes,
			xmitStats.tallies[0].currentCount +
				xmitStats.tallies[1].currentCount +
				xmitStats.tallies[2].currentCount,
			xmitStats.tallies[0].currentBytes +
				xmitStats.tallies[1].currentBytes +
				xmitStats.tallies[2].currentBytes);

	/*	Received.						*/

	sdr_read(sdr, (char *) &recvStats, bpdb.recvStats, sizeof(BpCosStats));
	reportStateStats(3, fromTimestamp, toTimestamp,
			recvStats.tallies[0].currentCount,
			recvStats.tallies[0].currentBytes,
			recvStats.tallies[1].currentCount,
			recvStats.tallies[1].currentBytes,
			recvStats.tallies[2].currentCount,
			recvStats.tallies[2].currentBytes,
			recvStats.tallies[0].currentCount +
				recvStats.tallies[1].currentCount +
				recvStats.tallies[2].currentCount,
			recvStats.tallies[0].currentBytes +
				recvStats.tallies[1].currentBytes +
				recvStats.tallies[2].currentBytes);

	/*	Delivered.  Nothing for now; need to poll endpoints.	*/

	reportStateStats(4, fromTimestamp, toTimestamp, 0, 0, 0, 0, 0, 0, 0, 0);

	/*	Custody refused.  Not counted; check reforwarded.	*/

	reportStateStats(5, fromTimestamp, toTimestamp, 0, 0, 0, 0, 0, 0, 0, 0);

	/*	Reforwarded.						*/

	reportStateStats(6, fromTimestamp, toTimestamp, 0, 0, 0, 0, 0, 0,
			dbStats.tallies[BP_DB_REQUEUED_FOR_FWD].currentCount,
			dbStats.tallies[BP_DB_REQUEUED_FOR_FWD].currentBytes);

	/*	Expired.						*/

	reportStateStats(7, fromTimestamp, toTimestamp, 0, 0, 0, 0, 0, 0,
			dbStats.tallies[BP_DB_EXPIRED].currentCount,
			dbStats.tallies[BP_DB_EXPIRED].currentBytes);
	sdr_exit_xn(sdr);
}

static int	bundleIsCustodial(Bundle *bundle)
{
	/*	Custodial procedures are undefined for bundles destined
	 *	for non-singleton endpoints, so we can't treat any such
	 *	bundle as "custodial" even if its BDL_IS_CUSTODIAL flag
	 *	is set.							*/

	int	reqdFlags = BDL_IS_CUSTODIAL | BDL_DEST_IS_SINGLETON;

	return ((bundle->bundleProcFlags & reqdFlags) == reqdFlags);
}

/*	*	*	Bundle destruction functions	*	*	*/

static int	destroyIncomplete(IncompleteBundle *incomplete, Object incElt)
{
	Sdr	bpSdr = getIonsdr();
	Object	elt;
	Object	nextElt;
	Object	fragObj;
	Bundle	fragment;

	for (elt = sdr_list_first(bpSdr, incomplete->fragments); elt;
			elt = nextElt)
	{
		nextElt = sdr_list_next(bpSdr, elt);
		fragObj = (Object) sdr_list_data(bpSdr, elt);
		sdr_stage(bpSdr, (char *) &fragment, fragObj, sizeof(Bundle));
		fragment.fragmentElt = 0;	/*	Lose constraint.*/
		fragment.incompleteElt = 0;
		sdr_write(bpSdr, fragObj, (char *) &fragment, sizeof(Bundle));
		if (bpDestroyBundle(fragObj, 0) < 0)
		{
			putErrmsg("Can't destroy incomplete bundle.", NULL);
			return -1;
		}

		sdr_list_delete(bpSdr, elt, NULL, NULL);
	}

	sdr_list_destroy(bpSdr, incomplete->fragments, NULL, NULL);
	sdr_free(bpSdr, sdr_list_data(bpSdr, incElt));
	sdr_list_delete(bpSdr, incElt, NULL, NULL);
	return 0;
}

static void	removeBundleFromQueue(Bundle *bundle, Object bundleObj,
			ClProtocol *protocol, Object outductObj,
			Outduct *outduct)
{
	Sdr		bpSdr = getIonsdr();
	int		backlogDecrement;
	OrdinalState	*ord;

	/*	Removal from queue reduces outduct's backlog.		*/

	backlogDecrement = computeECCC(guessBundleSize(bundle), protocol);
	switch (COS_FLAGS(bundle->bundleProcFlags) & 0x03)
	{
	case 0:				/*	Bulk priority.		*/
		reduceScalar(&(outduct->bulkBacklog), backlogDecrement);
		break;

	case 1:				/*	Standard priority.	*/
		reduceScalar(&(outduct->stdBacklog), backlogDecrement);
		break;

	default:			/*	Urgent priority.	*/
		ord = &(outduct->ordinals[bundle->extendedCOS.ordinal]);
		reduceScalar(&(ord->backlog), backlogDecrement);
		if (ord->lastForOrdinal == bundle->ductXmitElt)
		{
			ord->lastForOrdinal = 0;
		}

		reduceScalar(&(outduct->urgentBacklog), backlogDecrement);
	}

	sdr_write(bpSdr, outductObj, (char *) outduct, sizeof(Outduct));
	sdr_list_delete(bpSdr, bundle->ductXmitElt, NULL, NULL);
	bundle->ductXmitElt = 0;
	sdr_write(bpSdr, bundleObj, (char *) bundle, sizeof(Bundle));
}

static void	purgeDuctXmitElt(Bundle *bundle, Object bundleObj)
{
	Sdr		bpSdr = getIonsdr();
	Object		queue;
	Object		outductObj;
	Outduct		outduct;
	ClProtocol	protocol;

	queue = sdr_list_list(bpSdr, bundle->ductXmitElt);
	outductObj = sdr_list_user_data(bpSdr, queue);
	if (outductObj == 0)	/*	Bundle is in Limbo queue.	*/
	{
		sdr_list_delete(bpSdr, bundle->ductXmitElt, NULL, NULL);
		bundle->ductXmitElt = 0;
		return;
	}

	sdr_stage(bpSdr, (char *) &outduct, outductObj, sizeof(Outduct));
	sdr_read(bpSdr, (char *) &protocol, outduct.protocol,
			sizeof(ClProtocol));
	removeBundleFromQueue(bundle, bundleObj, &protocol, outductObj,
			&outduct);
}

void	destroyBpTimelineEvent(Object timelineElt)
{
	Sdr	sdr = getIonsdr();
	Object	eventObj;
	BpEvent	event;

	CHKVOID(timelineElt);
	eventObj = sdr_list_data(sdr, timelineElt);
	sdr_read(sdr, (char *) &event, eventObj, sizeof(BpEvent));
	sm_rbt_delete(getIonwm(), (getBpVdb())->timeline, orderBpEvents,
			&event, NULL, NULL);
	sdr_free(sdr, eventObj);
	sdr_list_delete(sdr, timelineElt, NULL, NULL);
}

static void	purgeStationsStack(Bundle *bundle)
{
	Sdr	bpSdr = getIonsdr();
	Object	elt;
	Object	addr;

	if (bundle->stations == 0)
	{
		return;
	}

	/*	Discard all intermediate routing destinations.		*/

	while (1)
	{
		elt = sdr_list_first(bpSdr, bundle->stations);
		if (elt == 0)
		{
			return;
		}

		addr = sdr_list_data(bpSdr, elt);
		sdr_list_delete(bpSdr, elt, NULL, NULL);
		sdr_free(bpSdr, addr);	/*	It's an sdrstring.	*/
	}
}

int	bpDestroyBundle(Object bundleObj, int ttlExpired)
{
	Sdr		bpSdr = getIonsdr();
	Bundle		bundle;
			OBJ_POINTER(IncompleteBundle, incomplete);
	char		*dictionary;
	int		result;
	Object		bsetObj;
	BundleSet	bset;
	Object		elt;

	CHKERR(ionLocked());
	CHKERR(bundleObj);
	sdr_stage(bpSdr, (char *) &bundle, bundleObj, sizeof(Bundle));

	/*	Special handling for TTL expiration.			*/

	if (ttlExpired)
	{
		/*	FORCES removal of all references to bundle.	*/

		if (bundle.fwdQueueElt)
		{
			sdr_list_delete(bpSdr, bundle.fwdQueueElt, NULL, NULL);
			bundle.fwdQueueElt = 0;
		}

		if (bundle.fragmentElt)
		{
			sdr_list_delete(bpSdr, bundle.fragmentElt, NULL, NULL);
			bundle.fragmentElt = 0;

			/*	If this is the last fragment of an
			 *	Incomplete, destroy the Incomplete.	*/

			GET_OBJ_POINTER(bpSdr, IncompleteBundle, incomplete,
				sdr_list_data(bpSdr, bundle.incompleteElt));
			if (sdr_list_length(bpSdr, incomplete->fragments) == 0)
			{
				if (destroyIncomplete(incomplete,
						bundle.incompleteElt) < 0)
				{
					putErrmsg("Failed destroying \
incomplete bundle.", NULL);
					return -1;
				}
			}

			bundle.incompleteElt = 0;
		}

		if (bundle.dlvQueueElt)
		{
			sdr_list_delete(bpSdr, bundle.dlvQueueElt, NULL, NULL);
			bundle.dlvQueueElt = 0;
		}

		if (bundle.ductXmitElt)
		{
			purgeDuctXmitElt(&bundle, bundleObj);
		}

		/*	Notify sender, if so requested or if custodian.
		 *	But never for admin bundles.			*/

		bpDbTally(BP_DB_EXPIRED, bundle.payload.length);
		if (bundle.custodyTaken)
		{
			bpCtTally(BP_CT_CUSTODY_EXPIRED, bundle.payload.length);
		}

		if ((_bpvdb(NULL))->watching & WATCH_expire)
		{
			putchar('!');
			fflush(stdout);
		}

		if (!(bundle.bundleProcFlags & BDL_IS_ADMIN)
		&& (bundle.custodyTaken
		|| (SRR_FLAGS(bundle.bundleProcFlags) & BP_DELETED_RPT)))
		{
			bundle.statusRpt.flags |= BP_DELETED_RPT;
			bundle.statusRpt.reasonCode = SrLifetimeExpired;
			getCurrentDtnTime(&bundle.statusRpt.deletionTime);
			if ((dictionary = retrieveDictionary(&bundle))
					== (char *) &bundle)
			{
				putErrmsg("Can't retrieve dictionary.", NULL);
				return -1;
			}

			result = sendStatusRpt(&bundle, dictionary);
			releaseDictionary(dictionary);
		       	if (result < 0)
			{
				putErrmsg("can't send deletion notice", NULL);
				return -1;
			}
		}

		bundle.custodyTaken = 0;
	}

	/*	Check for any remaining constraints on deletion.	*/

	if (bundle.fragmentElt || bundle.dlvQueueElt || bundle.fwdQueueElt
	|| bundle.ductXmitElt || bundle.custodyTaken)
	{
		return 0;	/*	Can't destroy bundle yet.	*/
	}

	/*	Remove bundle from timeline and bundles hash table.	*/

	destroyBpTimelineEvent(bundle.timelineElt);
	if (bundle.hashEntry)
	{
		bsetObj = sdr_hash_entry_value(bpSdr, (_bpConstants())->bundles,
				bundle.hashEntry);
		sdr_stage(bpSdr, (char *) &bset, bsetObj, sizeof(BundleSet));
		bset.count--;
		if (bset.count == 0)
		{
			sdr_hash_delete_entry(bpSdr, bundle.hashEntry);
			sdr_free(bpSdr, bsetObj);
		}
		else
		{
			sdr_write(bpSdr, bsetObj, (char *) &bset,
					sizeof(BundleSet));
		}
	}

	/*	Remove transmission metadata.				*/

	if (bundle.proxNodeEid)
	{
		sdr_free(bpSdr, bundle.proxNodeEid);
		bundle.proxNodeEid = 0;
	}

	if (bundle.destDuctName)
	{
		sdr_free(bpSdr, bundle.destDuctName);
		bundle.destDuctName = 0;
	}

	/*	Turn off automatic re-forwarding.			*/

	if (bundle.overdueElt)
	{
		destroyBpTimelineEvent(bundle.overdueElt);
	}

	if (bundle.ctDueElt)
	{
		destroyBpTimelineEvent(bundle.ctDueElt);
	}

	/*	Remove bundle from applications' bundle tracking lists.	*/

	if (bundle.trackingElts)
	{
		while (1)
		{
			elt = sdr_list_first(bpSdr, bundle.trackingElts);
			if (elt == 0)
			{
				break;	/*	No more tracking elts.	*/
			}

			/*	Data in list element is the address
			 *	of a list element (in a list used by
			 *	some application) that references this
			 *	bundle.  Delete that list element,
			 *	which is no longer usable.		*/

			sdr_list_delete(bpSdr, sdr_list_data(bpSdr, elt),
					NULL, NULL);

			/*	Now delete this list element as well.	*/

			sdr_list_delete(bpSdr, elt, NULL, NULL);
		}

		sdr_list_destroy(bpSdr, bundle.trackingElts, NULL, NULL);
	}

	/*	Destroy the bundle's payload ZCO.  There may still
	 *	be some application reference to the source data of
	 *	this ZCO, but if not then deleting the payload ZCO
	 *	will implicitly destroy the source data itself.		*/

	if (bundle.payload.content)
	{
		zco_destroy(bpSdr, bundle.payload.content);
	}

	/*	Destroy all SDR objects managed for this bundle and
	 *	free space occupied by the bundle itself.		*/

	bpDestroyBundle_ACS(&bundle);
	if (bundle.clDossier.senderEid.text)
	{
		sdr_free(bpSdr, bundle.clDossier.senderEid.text);
	}

	if (bundle.dictionary)
	{
		sdr_free(bpSdr, bundle.dictionary);
	}

	destroyExtensionBlocks(&bundle);
	destroyCollaborationBlocks(&bundle);
	purgeStationsStack(&bundle);
	if (bundle.stations)
	{
		sdr_list_destroy(bpSdr, bundle.stations, NULL, NULL);
	}

	sdr_free(bpSdr, bundleObj);
	bpDiscardTally(COS_FLAGS(bundle.bundleProcFlags) & 0x03,
			bundle.payload.length);
	noteBundleRemoved(&bundle);
	return 0;
}

/*	*	*	BP database mgt and access functions	*	*/

static int	constructBundleHashKey(char *buffer, char *sourceEid,
			unsigned int seconds, unsigned int count,
			unsigned int offset, unsigned int length)
{
	memset(buffer, 0, BUNDLES_HASH_KEY_BUFLEN);
	isprintf(buffer, BUNDLES_HASH_KEY_BUFLEN, "%s:%u:%u:%u:%u",
			sourceEid, seconds, count, offset, length);
	return strlen(buffer);
}

int	findBundle(char *sourceEid, BpTimestamp *creationTime,
		unsigned int fragmentOffset, unsigned int fragmentLength,
		Object *bundleAddr)
{
	Sdr		bpSdr = getIonsdr();
	char		key[BUNDLES_HASH_KEY_BUFLEN];
	Address		bsetObj;
	Object		hashElt;
	BundleSet	bset;

	CHKERR(sourceEid);
	CHKERR(creationTime);
	CHKERR(bundleAddr);
	*bundleAddr = 0;	/*	Default: not found.		*/
	CHKERR(ionLocked());
	if (constructBundleHashKey(key, sourceEid, creationTime->seconds,
			creationTime->count, fragmentOffset, fragmentLength)
			> BUNDLES_HASH_KEY_LEN)
	{
		return 0;	/*	Can't be in hash table.		*/
	}

	switch (sdr_hash_retrieve(bpSdr, (_bpConstants())->bundles, key,
			&bsetObj, &hashElt))
	{
	case -1:
		putErrmsg("Failed locating bundle in hash table.", NULL);
		return -1;

	case 0:
		return 0;	/*	No such entry in hash table.	*/

	default:
		sdr_read(bpSdr, (char *) &bset, bsetObj, sizeof(BundleSet));
		*bundleAddr = bset.bundleObj;
		return bset.count;
	}
}

void	findScheme(char *schemeName, VScheme **scheme, PsmAddress *schemeElt)
{
	PsmPartition	bpwm = getIonwm();
	PsmAddress	elt;

	if (schemeName == NULL)
	{
		*schemeElt = 0;
		return;
	}

	for (elt = sm_list_first(bpwm, (_bpvdb(NULL))->schemes); elt;
			elt = sm_list_next(bpwm, elt))
	{
		*scheme = (VScheme *) psp(bpwm, sm_list_data(bpwm, elt));
		if (strcmp((*scheme)->name, schemeName) == 0)
		{
			break;
		}
	}

	*schemeElt = elt;
}

int	addScheme(char *schemeName, char *fwdCmd, char *admAppCmd)
{
	int		schemeNameLength;
	Sdr		bpSdr = getIonsdr();
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	Scheme		schemeBuf;
	Object		addr;
	Object		schemeElt = 0;

	CHKERR(schemeName);
	if (*schemeName == 0)
	{
		writeMemo("[?] Zero-length scheme name.");
		return 0;
	}

	schemeNameLength = istrlen(schemeName, MAX_SCHEME_NAME_LEN + 1);
	if (schemeNameLength > MAX_SCHEME_NAME_LEN)
	{
		writeMemoNote("[?] Scheme name is too long", schemeName);
		return 0;
	}

	if (fwdCmd)
	{
		if (*fwdCmd == '\0')
		{
			fwdCmd = NULL;
		}
		else
		{
			if (istrlen(fwdCmd, MAX_SDRSTRING + 1) > MAX_SDRSTRING)
			{
				writeMemoNote("[?] forwarder command string \
too long", fwdCmd);
				return 0;
			}
		}
	}

	if (admAppCmd)
	{
		if (*admAppCmd == '\0')
		{
			admAppCmd = NULL;
		}
		else
		{
			if (istrlen(admAppCmd, MAX_SDRSTRING + 1)
					> MAX_SDRSTRING)
			{
				writeMemoNote("[?] adminep command string \
too long", admAppCmd);
				return 0;
			}
		}
	}

	CHKERR(sdr_begin_xn(bpSdr));
	findScheme(schemeName, &vscheme, &vschemeElt);
	if (vschemeElt != 0)	/*	This is a known scheme.		*/
	{
		sdr_exit_xn(bpSdr);
		writeMemoNote("[?] Duplicate scheme", schemeName);
		return 0;
	}

	/*	All parameters validated, okay to add the scheme.	*/

	memset((char *) &schemeBuf, 0, sizeof(Scheme));
	istrcpy(schemeBuf.name, schemeName, sizeof schemeBuf.name);
	schemeBuf.nameLength = schemeNameLength;
	if (isCbhe(schemeName))
	{
		schemeBuf.cbhe = 1;
	}

	if (isUnicast(schemeName))
	{
		schemeBuf.unicast = 1;
	}

	if (fwdCmd)
	{
		schemeBuf.fwdCmd = sdr_string_create(bpSdr, fwdCmd);
	}

	if (admAppCmd)
	{
		schemeBuf.admAppCmd = sdr_string_create(bpSdr, admAppCmd);
	}

	schemeBuf.forwardQueue = sdr_list_create(bpSdr);
	schemeBuf.endpoints = sdr_list_create(bpSdr);
	addr = sdr_malloc(bpSdr, sizeof(Scheme));
	if (addr)
	{
		sdr_write(bpSdr, addr, (char *) &schemeBuf, sizeof(Scheme));
		schemeElt = sdr_list_insert_last(bpSdr,
				(_bpConstants())->schemes, addr);
	}

	if (sdr_end_xn(bpSdr) < 0 || schemeElt == 0)
	{
		putErrmsg("Can't add scheme.", schemeName);
		return -1;
	}

	CHKERR(sdr_begin_xn(bpSdr));	/*	Just to lock memory.	*/
	if (raiseScheme(schemeElt, _bpvdb(NULL)) < 0)
	{
		sdr_cancel_xn(bpSdr);
		putErrmsg("Can't raise scheme.", NULL);
		return -1;
	}

	sdr_exit_xn(bpSdr);	/*	Unlock memory.			*/
	return 1;
}

int	updateScheme(char *schemeName, char *fwdCmd, char *admAppCmd)
{
	Sdr		bpSdr = getIonsdr();
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	Scheme		schemeBuf;
	Object		addr;

	CHKERR(schemeName);
	if (*schemeName == 0)
	{
		writeMemo("[?] Zero-length scheme name.");
		return 0;
	}

	if (fwdCmd)
	{
		if (*fwdCmd == '\0')
		{
			fwdCmd = NULL;
		}
		else
		{
			if (strlen(fwdCmd) > MAX_SDRSTRING)
			{
				writeMemoNote("[?] forwarder command string \
too long", fwdCmd);
				return 0;
			}
		}
	}

	if (admAppCmd)
	{
		if (*admAppCmd == '\0')
		{
			admAppCmd = NULL;
		}
		else
		{
			if (strlen(admAppCmd) > MAX_SDRSTRING)
			{
				writeMemoNote("[?] adminep command string \
too long", admAppCmd);
				return 0;
			}
		}
	}

	CHKERR(sdr_begin_xn(bpSdr));
	findScheme(schemeName, &vscheme, &vschemeElt);
	if (vschemeElt == 0)	/*	This is an unknown scheme.	*/
	{
		sdr_exit_xn(bpSdr);
		writeMemoNote("Unknown scheme", schemeName);
		return 0;
	}

	/*	All parameters validated, okay to update the scheme.
	 *	First wipe out current cmds, then establish new ones.	*/

	addr = (Object) sdr_list_data(bpSdr, vscheme->schemeElt);
	sdr_stage(bpSdr, (char *) &schemeBuf, addr, sizeof(Scheme));
	if (schemeBuf.fwdCmd)
	{
		sdr_free(bpSdr, schemeBuf.fwdCmd);
		schemeBuf.fwdCmd = 0;
	}

	if (fwdCmd)
	{
		schemeBuf.fwdCmd = sdr_string_create(bpSdr, fwdCmd);
	}

	if (schemeBuf.admAppCmd)
	{
		sdr_free(bpSdr, schemeBuf.admAppCmd);
		schemeBuf.admAppCmd = 0;
	}

	if (admAppCmd)
	{
		schemeBuf.admAppCmd = sdr_string_create(bpSdr, admAppCmd);
	}

	sdr_write(bpSdr, addr, (char *) &schemeBuf, sizeof(Scheme));
	if (sdr_end_xn(bpSdr) < 0)
	{
		putErrmsg("Can't update scheme.", schemeName);
		return -1;
	}

	return 1;
}

int	removeScheme(char *schemeName)
{
	Sdr		bpSdr = getIonsdr();
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	Object		schemeElt;
	Object		addr;
	Scheme		schemeBuf;

	CHKERR(schemeName);

	/*	Must stop the scheme before trying to remove it.	*/

	CHKERR(sdr_begin_xn(bpSdr));	/*	Lock memory.		*/
	findScheme(schemeName, &vscheme, &vschemeElt);
	if (vschemeElt == 0)
	{
		sdr_exit_xn(bpSdr);
		writeMemoNote("[?] Unknown scheme", schemeName);
		return 0;
	}

	/*	All parameters validated.				*/

	stopScheme(vscheme);
	sdr_exit_xn(bpSdr);
	waitForScheme(vscheme);
	CHKERR(sdr_begin_xn(bpSdr));
	resetScheme(vscheme);
	schemeElt = vscheme->schemeElt;
	addr = sdr_list_data(bpSdr, schemeElt);
	sdr_read(bpSdr, (char *) &schemeBuf, addr, sizeof(Scheme));
	if (sdr_list_length(bpSdr, schemeBuf.forwardQueue) != 0)
	{
		sdr_exit_xn(bpSdr);
		writeMemoNote("[?] Scheme has backlog, can't be removed",
				schemeName);
		return 0;
	}

	if (sdr_list_length(bpSdr, schemeBuf.endpoints) != 0)
	{
		sdr_exit_xn(bpSdr);
		writeMemoNote("[?] Scheme has endpoints, can't be removed",
				schemeName);
		return 0;
	}

	/*	Okay to remove this scheme from the database.		*/

	dropScheme(vscheme, vschemeElt);
	if (schemeBuf.fwdCmd)
	{
		sdr_free(bpSdr, schemeBuf.fwdCmd);
	}

	if (schemeBuf.admAppCmd)
	{
		sdr_free(bpSdr, schemeBuf.admAppCmd);
	}

	sdr_list_destroy(bpSdr, schemeBuf.forwardQueue, NULL, NULL);
	sdr_list_destroy(bpSdr, schemeBuf.endpoints, NULL, NULL);
	sdr_free(bpSdr, addr);
	sdr_list_delete(bpSdr, schemeElt, NULL, NULL);
	if (sdr_end_xn(bpSdr) < 0)
	{
		putErrmsg("Can't remove scheme.", NULL);
		return -1;
	}

	return 1;
}

int	bpStartScheme(char *name)
{
	Sdr		bpSdr = getIonsdr();
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	int		result = 1;

	CHKZERO(sdr_begin_xn(bpSdr));	/*	Just to lock memory.	*/
	findScheme(name, &vscheme, &vschemeElt);
	if (vschemeElt == 0)
	{
		writeMemoNote("Unknown scheme", name);
		result = 0;
	}
	else
	{
		startScheme(vscheme);
	}

	sdr_exit_xn(bpSdr);	/*	Unlock memory.			*/
	return result;
}

void	bpStopScheme(char *name)
{
	Sdr		bpSdr = getIonsdr();
	VScheme		*vscheme;
	PsmAddress	vschemeElt;

	CHKVOID(sdr_begin_xn(bpSdr));	/*	Just to lock memory.	*/
	findScheme(name, &vscheme, &vschemeElt);
	if (vschemeElt == 0)
	{
		sdr_exit_xn(bpSdr);
		writeMemoNote("[?] Unknown scheme", name);
		return;
	}

	stopScheme(vscheme);
	sdr_exit_xn(bpSdr);	/*	Unlock memory.			*/
	waitForScheme(vscheme);
	CHKVOID(sdr_begin_xn(bpSdr));	/*	Just to lock memory.	*/
	resetScheme(vscheme);
	sdr_exit_xn(bpSdr);	/*	Unlock memory.			*/
}

void	findEndpoint(char *schemeName, char *nss, VScheme *vscheme,
		VEndpoint **vpoint, PsmAddress *vpointElt)
{
	PsmPartition	bpwm = getIonwm();
	PsmAddress	elt;

	if (vscheme == NULL)
	{
		findScheme(schemeName, &vscheme, &elt);
		if (elt == 0)
		{
			*vpointElt = 0;
			return;
		}
	}

	for (elt = sm_list_first(bpwm, vscheme->endpoints); elt;
			elt = sm_list_next(bpwm, elt))
	{
		*vpoint = (VEndpoint *) psp(bpwm, sm_list_data(bpwm, elt));
		if (strcmp((*vpoint)->nss, nss) == 0)
		{
			break;
		}
	}

	*vpointElt = elt;
}

int	addEndpoint(char *eid, BpRecvRule recvRule, char *script)
{
	Sdr		bpSdr = getIonsdr();
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	elt;
	VEndpoint	*vpoint;
	Endpoint	endpointBuf;
	Scheme		scheme;
	EndpointStats	statsInit;
	Object		addr;
	Object		endpointElt = 0;	/*	To hush gcc.	*/

	CHKERR(eid);
	if (parseEidString(eid, &metaEid, &vscheme, &elt) == 0)
	{
		restoreEidString(&metaEid);
		writeMemoNote("[?] Can't parse the EID", eid);
		return 0;
	}

	if (strlen(metaEid.nss) > MAX_NSS_LEN)
	{
		writeMemoNote("[?] Endpoint nss is too long", metaEid.nss);
		return 0;
	}

	CHKERR(sdr_begin_xn(bpSdr));
	findEndpoint(NULL, metaEid.nss, vscheme, &vpoint, &elt);
	if (elt != 0)	/*	This is a known endpoint.	*/
	{
		sdr_exit_xn(bpSdr);
		restoreEidString(&metaEid);
		writeMemoNote("[?] Duplicate endpoint", eid);
		return 0;
	}

	/*	All parameters validated, okay to add the endpoint.	*/

	memset((char *) &endpointBuf, 0, sizeof(Endpoint));
	istrcpy(endpointBuf.nss, metaEid.nss, sizeof endpointBuf.nss);
	restoreEidString(&metaEid);
	endpointBuf.recvRule = recvRule;
	if (script)
	{
		endpointBuf.recvScript = sdr_string_create(bpSdr, script);
	}

	endpointBuf.incompletes = sdr_list_create(bpSdr);
	endpointBuf.deliveryQueue = sdr_list_create(bpSdr);
	endpointBuf.scheme = (Object) sdr_list_data(bpSdr, vscheme->schemeElt);
	endpointBuf.stats = sdr_malloc(bpSdr, sizeof(EndpointStats));
	if (endpointBuf.stats)
	{
		memset((char *) &statsInit, 0, sizeof(EndpointStats));
		sdr_write(bpSdr, endpointBuf.stats, (char *) &statsInit,
				sizeof(EndpointStats));
	}

	endpointBuf.updateStats = 1;	/*	Default.		*/
	addr = sdr_malloc(bpSdr, sizeof(Endpoint));
	if (addr)
	{
		sdr_read(bpSdr, (char *) &scheme, endpointBuf.scheme,
				sizeof(Scheme));
		endpointElt = sdr_list_insert_last(bpSdr, scheme.endpoints,
				addr);
		sdr_write(bpSdr, addr, (char *) &endpointBuf, sizeof(Endpoint));
	}

	if (sdr_end_xn(bpSdr) < 0)
	{
		putErrmsg("Can't add endpoint.", eid);
		return -1;
	}

	CHKERR(sdr_begin_xn(bpSdr));	/*	Just to lock memory.	*/
	if (raiseEndpoint(vscheme, endpointElt) < 0)
	{
		sdr_exit_xn(bpSdr);
		putErrmsg("Can't raise endpoint.", NULL);
		return -1;
	}

	sdr_exit_xn(bpSdr);	/*	Unlock memory.			*/
	if (addEndpoint_IMC(vscheme, eid) < 0)
	{
		return -1;
	}

	return 1;
}

int	updateEndpoint(char *eid, BpRecvRule recvRule, char *script)
{
	Sdr		bpSdr = getIonsdr();
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	elt;
	VEndpoint	*vpoint;
	Object		addr;
	Endpoint	endpointBuf;

	CHKERR(eid);
	if (parseEidString(eid, &metaEid, &vscheme, &elt) == 0)
	{
		restoreEidString(&metaEid);
		writeMemoNote("[?] Can't parse the EID", eid);
		return 0;
	}

	CHKERR(sdr_begin_xn(bpSdr));
	findEndpoint(NULL, metaEid.nss, vscheme, &vpoint, &elt);
	restoreEidString(&metaEid);
	if (elt == 0)		/*	This is an unknown endpoint.	*/
	{
		sdr_exit_xn(bpSdr);
		writeMemoNote("[?] Unknown endpoint", eid);
		return 0;
	}

	/*	All parameters validated, okay to update the endpoint.
	 *	First wipe out current parms, then establish new ones.	*/

	addr = (Object) sdr_list_data(bpSdr, vpoint->endpointElt);
	sdr_stage(bpSdr, (char *) &endpointBuf, addr, sizeof(Endpoint));
	endpointBuf.recvRule = recvRule;
	if (endpointBuf.recvScript)
	{
		sdr_free(bpSdr, endpointBuf.recvScript);
		endpointBuf.recvScript = 0;
	}

	if (script)
	{
		endpointBuf.recvScript = sdr_string_create(bpSdr, script);
	}

	sdr_write(bpSdr, addr, (char *) &endpointBuf, sizeof(Endpoint));
	if (sdr_end_xn(bpSdr) < 0)
	{
		putErrmsg("Can't update endpoint.", eid);
		return -1;
	}

	return 1;
}

int	removeEndpoint(char *eid)
{
	Sdr		bpSdr = getIonsdr();
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	elt;
	VEndpoint	*vpoint;
	Object		endpointElt;
	Object		addr;
	Endpoint	endpointBuf;

	CHKERR(eid);
	if (parseEidString(eid, &metaEid, &vscheme, &elt) == 0)
	{
		restoreEidString(&metaEid);
		writeMemoNote("[?] Can't parse the EID", eid);
		return 0;
	}

	CHKERR(sdr_begin_xn(bpSdr));
	findEndpoint(NULL, metaEid.nss, vscheme, &vpoint, &elt);
	restoreEidString(&metaEid);
	if (elt == 0)			/*	Not found.		*/
	{
		sdr_exit_xn(bpSdr);
		writeMemoNote("[?] Unknown endpoint", eid);
		return 0;
	}

	if (vpoint->appPid != ERROR && sm_TaskExists(vpoint->appPid))
	{
		sdr_exit_xn(bpSdr);
		writeMemoNote("[?] Endpoint can't be removed while open", eid);
		return 0;
	}

	endpointElt = vpoint->endpointElt;
	addr = (Object) sdr_list_data(bpSdr, endpointElt);
	sdr_read(bpSdr, (char *) &endpointBuf, addr, sizeof(Endpoint));
	if (sdr_list_length(bpSdr, endpointBuf.incompletes) != 0)
	{
		sdr_exit_xn(bpSdr);
		writeMemoNote("[?] Endpoint has incomplete bundles pending",
				eid);
		return 0;
	}

	if (sdr_list_length(bpSdr, endpointBuf.deliveryQueue) != 0)
	{
		sdr_exit_xn(bpSdr);
		writeMemoNote("[?] Endpoint has non-empty delivery queue", eid);
		return 0;
	}

	/*	Okay to remove this endpoint from the database.		*/

	dropEndpoint(vpoint, elt);
	sdr_list_destroy(bpSdr, endpointBuf.incompletes, NULL, NULL);
	sdr_list_destroy(bpSdr, endpointBuf.deliveryQueue, NULL, NULL);
	sdr_free(bpSdr, addr);
	sdr_list_delete(bpSdr, endpointElt, NULL, NULL);
	if (sdr_end_xn(bpSdr) < 0)
	{
		putErrmsg("Can't remove endpoint.", eid);
		return -1;
	}

	if (removeEndpoint_IMC(vscheme, eid) < 0)
	{
		return -1;
	}

	return 1;
}

void	fetchProtocol(char *protocolName, ClProtocol *clp, Object *clpElt)
{
	Sdr	bpSdr = getIonsdr();
	Object	elt;

	for (elt = sdr_list_first(bpSdr, (_bpConstants())->protocols); elt;
			elt = sdr_list_next(bpSdr, elt))
	{
		sdr_read(bpSdr, (char *) clp, sdr_list_data(bpSdr, elt),
				sizeof(ClProtocol));
		if (strcmp(clp->name, protocolName) == 0)
		{
			break;
		}
	}

	*clpElt = elt;
}

int	addProtocol(char *protocolName, int payloadPerFrame, int ohdPerFrame,
		int nominalRate)
{
	Sdr		bpSdr = getIonsdr();
	ClProtocol	clpbuf;
	Object		elt;
	Object		addr;

	CHKERR(protocolName);
	if (*protocolName == 0
	|| strlen(protocolName) > MAX_CL_PROTOCOL_NAME_LEN)
	{
		writeMemoNote("[?] Invalid protocol name", protocolName);
		return 0;
	}

	if (payloadPerFrame < 1 || ohdPerFrame < 1)
	{
		writeMemoNote("[?] Per-frame payload and overhead must be > 0",
				protocolName);
		return 0;
	}

	CHKERR(sdr_begin_xn(bpSdr));
	fetchProtocol(protocolName, &clpbuf, &elt);
	if (elt != 0)		/*	This is a known protocol.	*/
	{
		sdr_exit_xn(bpSdr);
		writeMemoNote("[?] Duplicate protocol", protocolName);
		return 0;
	}

	/*	All parameters validated, okay to add the protocol.	*/

	memset((char *) &clpbuf, 0, sizeof(ClProtocol));
	istrcpy(clpbuf.name, protocolName, sizeof clpbuf.name);
	clpbuf.payloadBytesPerFrame = payloadPerFrame;
	clpbuf.overheadPerFrame = ohdPerFrame;
	if (nominalRate < 0)
	{
		nominalRate = -1;	/*	Disables rate control.	*/
	}

	clpbuf.nominalRate = nominalRate;
	clpbuf.inducts = sdr_list_create(bpSdr);
	clpbuf.outducts = sdr_list_create(bpSdr);
	addr = sdr_malloc(bpSdr, sizeof(ClProtocol));
	if (addr)
	{
		sdr_write(bpSdr, addr, (char *) &clpbuf, sizeof(ClProtocol));
		sdr_list_insert_last(bpSdr, (_bpConstants())->protocols, addr);
	}

	if (sdr_end_xn(bpSdr) < 0)
	{
		putErrmsg("Can't add protocol.", protocolName);
		return -1;
	}

	/*	Note: we don't call raiseProtocol here because the
	 *	protocol has no inducts or outducts yet.		*/

	return 1;
}

int	removeProtocol(char *protocolName)
{
	Sdr		bpSdr = getIonsdr();
	ClProtocol	clpbuf;
	Object		elt;
	Object		addr;

	CHKERR(protocolName);
	CHKERR(sdr_begin_xn(bpSdr));
	fetchProtocol(protocolName, &clpbuf, &elt);
	if (elt == 0)				/*	Not found.	*/
	{
		sdr_exit_xn(bpSdr);
		writeMemoNote("[?] Unknown protocol", protocolName);
		return 0;
	}

	if (sdr_list_length(bpSdr, clpbuf.inducts) != 0
	|| sdr_list_length(bpSdr, clpbuf.outducts) != 0)
	{
		sdr_exit_xn(bpSdr);
		writeMemoNote("[?] Protocol has ducts, can't be removed",
				protocolName);
		return 0;
	}

	/*	Okay to remove this protocol from the database.		*/

	addr = (Object) sdr_list_data(bpSdr, elt);
	sdr_list_destroy(bpSdr, clpbuf.inducts, NULL, NULL);
	sdr_list_destroy(bpSdr, clpbuf.outducts, NULL, NULL);
	sdr_free(bpSdr, addr);
	sdr_list_delete(bpSdr, elt, NULL, NULL);
	if (sdr_end_xn(bpSdr) < 0)
	{
		putErrmsg("Can't remove protocol.", NULL);
		return -1;
	}

	return 1;
}

int	bpStartProtocol(char *name)
{
	Sdr		bpSdr = getIonsdr();
	PsmPartition	bpwm = getIonwm();
	BpVdb		*bpvdb = _bpvdb(NULL);
	PsmAddress	elt;
	VInduct		*vinduct;
	VOutduct	*voutduct;

	CHKZERO(sdr_begin_xn(bpSdr));	/*	Just to lock memory.	*/
	for (elt = sm_list_first(bpwm, bpvdb->inducts); elt;
			elt = sm_list_next(bpwm, elt))
	{
		vinduct = (VInduct *) psp(bpwm, sm_list_data(bpwm, elt));
		if (strcmp((vinduct)->protocolName, name) == 0)
		{
			startInduct(vinduct);
		}
	}

	for (elt = sm_list_first(bpwm, bpvdb->outducts); elt;
			elt = sm_list_next(bpwm, elt))
	{
		voutduct = (VOutduct *) psp(bpwm, sm_list_data(bpwm, elt));
		if (strcmp((voutduct)->protocolName, name) == 0)
		{
			startOutduct(voutduct);
		}
	}

	sdr_exit_xn(bpSdr);	/*	Unlock memory.			*/
	return 1;
}

void	bpStopProtocol(char *name)
{
	Sdr		bpSdr = getIonsdr();
	PsmPartition	bpwm = getIonwm();
	BpVdb		*bpvdb = _bpvdb(NULL);
	PsmAddress	elt;
	VInduct		*vinduct;
	VOutduct	*voutduct;

	CHKVOID(sdr_begin_xn(bpSdr));	/*	Just to lock memory.	*/
	for (elt = sm_list_first(bpwm, bpvdb->inducts); elt;
			elt = sm_list_next(bpwm, elt))
	{
		vinduct = (VInduct *) psp(bpwm, sm_list_data(bpwm, elt));
		if (strcmp(vinduct->protocolName, name) == 0)
		{
			stopInduct(vinduct);
			sdr_exit_xn(bpSdr);
			waitForInduct(vinduct);
			CHKVOID(sdr_begin_xn(bpSdr));
			resetInduct(vinduct);
		}
	}

	for (elt = sm_list_first(bpwm, bpvdb->outducts); elt;
			elt = sm_list_next(bpwm, elt))
	{
		voutduct = (VOutduct *) psp(bpwm, sm_list_data(bpwm, elt));
		if (strcmp(voutduct->protocolName, name) == 0)
		{
			stopOutduct(voutduct);
			sdr_exit_xn(bpSdr);
			waitForOutduct(voutduct);
			CHKVOID(sdr_begin_xn(bpSdr));
			resetOutduct(voutduct);
		}
	}

	sdr_exit_xn(bpSdr);	/*	Unlock memory.			*/
}

void	findInduct(char *protocolName, char *ductName, VInduct **vduct,
		PsmAddress *vductElt)
{
	PsmPartition	bpwm = getIonwm();
	PsmAddress	elt;

	for (elt = sm_list_first(bpwm, (_bpvdb(NULL))->inducts); elt;
			elt = sm_list_next(bpwm, elt))
	{
		*vduct = (VInduct *) psp(bpwm, sm_list_data(bpwm, elt));
		if (strcmp((*vduct)->protocolName, protocolName) == 0
		&& strcmp((*vduct)->ductName, ductName) == 0)
		{
			break;
		}
	}

	*vductElt = elt;
}

int	addInduct(char *protocolName, char *ductName, char *cliCmd)
{
	Sdr		bpSdr = getIonsdr();
	ClProtocol	clpbuf;
	Object		clpElt;
	VInduct		*vduct;
	PsmAddress	vductElt;
	Induct		ductBuf;
	InductStats	statsInit;
	Object		addr;
	Object		elt = 0;

	CHKERR(protocolName && ductName && cliCmd);
	if (*protocolName == 0 || *ductName == 0 || *cliCmd == 0)
	{
		writeMemoNote("[?] Zero-length Induct parm(s)", ductName);
		return 0;
	}

	if (strlen(ductName) > MAX_CL_DUCT_NAME_LEN)
	{
		writeMemoNote("[?] Induct name is too long", ductName);
		return 0;
	}

	if (strlen(cliCmd) > MAX_SDRSTRING)
	{
		writeMemoNote("[?] CLI command string is too long", cliCmd);
		return 0;
	}

	CHKERR(sdr_begin_xn(bpSdr));
	fetchProtocol(protocolName, &clpbuf, &clpElt);
	if (clpElt == 0)
	{
		sdr_exit_xn(bpSdr);
		writeMemoNote("[?] Protocol is unknown", protocolName);
		return 0;
	}

	findInduct(protocolName, ductName, &vduct, &vductElt);
	if (vductElt != 0)	/*	This is a known duct.		*/
	{
		sdr_exit_xn(bpSdr);
		writeMemoNote("[?] Duplicate induct", ductName);
		return 0;
	}

	/*	All parameters validated, okay to add the duct.		*/

	memset((char *) &ductBuf, 0, sizeof(Induct));
	istrcpy(ductBuf.name, ductName, sizeof ductBuf.name);
	ductBuf.cliCmd = sdr_string_create(bpSdr, cliCmd);
	ductBuf.protocol = (Object) sdr_list_data(bpSdr, clpElt);
	ductBuf.stats = sdr_malloc(bpSdr, sizeof(InductStats));
	if (ductBuf.stats)
	{
		memset((char *) &statsInit, 0, sizeof(InductStats));
		sdr_write(bpSdr, ductBuf.stats, (char *) &statsInit,
				sizeof(InductStats));
	}

	ductBuf.updateStats = 1;	/*	Default.		*/
	addr = sdr_malloc(bpSdr, sizeof(Induct));
	if (addr)
	{
		elt = sdr_list_insert_last(bpSdr, clpbuf.inducts, addr);
		sdr_write(bpSdr, addr, (char *) &ductBuf, sizeof(Induct));
	}

	if (sdr_end_xn(bpSdr) < 0 || elt == 0)
	{
		putErrmsg("Can't add induct.", ductName);
		return -1;
	}

	CHKERR(sdr_begin_xn(bpSdr));	/*	Just to lock memory.	*/
	if (raiseInduct(elt, _bpvdb(NULL)) < 0)
	{
		sdr_cancel_xn(bpSdr);
		putErrmsg("Can't raise induct.", NULL);
		return -1;
	}

	sdr_exit_xn(bpSdr);	/*	Unlock memory.			*/
	return 1;
}

int	updateInduct(char *protocolName, char *ductName, char *cliCmd)
{
	Sdr		bpSdr = getIonsdr();
	VInduct		*vduct;
	PsmAddress	vductElt;
	Object		addr;
	Induct		ductBuf;

	CHKERR(protocolName && ductName && cliCmd);
	if (*protocolName == 0 || *ductName == 0 || *cliCmd == 0)
	{
		writeMemoNote("[?] Zero-length Induct parm(s)", ductName);
		return 0;
	}

	if (strlen(cliCmd) > MAX_SDRSTRING)
	{
		writeMemoNote("[?] CLI command string is too long", cliCmd);
		return 0;
	}

	CHKERR(sdr_begin_xn(bpSdr));
	findInduct(protocolName, ductName, &vduct, &vductElt);
	if (vductElt == 0)	/*	This is an unknown duct.	*/
	{
		sdr_exit_xn(bpSdr);
		writeMemoNote("[?] Unknown induct", ductName);
		return 0;
	}

	/*	All parameters validated, okay to update the duct.
	 *	First wipe out current cliCmd, then establish new one.	*/

	addr = (Object) sdr_list_data(bpSdr, vduct->inductElt);
	sdr_stage(bpSdr, (char *) &ductBuf, addr, sizeof(Induct));
	if (ductBuf.cliCmd)
	{
		sdr_free(bpSdr, ductBuf.cliCmd);
		ductBuf.cliCmd = 0;
	}

	ductBuf.cliCmd = sdr_string_create(bpSdr, cliCmd);
	sdr_write(bpSdr, addr, (char *) &ductBuf, sizeof(Induct));
	if (sdr_end_xn(bpSdr) < 0)
	{
		putErrmsg("Can't update induct.", ductName);
		return -1;
	}

	return 1;
}

int	removeInduct(char *protocolName, char *ductName)
{
	Sdr		bpSdr = getIonsdr();
	VInduct		*vduct;
	PsmAddress	vductElt;
	Object		ductElt;
	Object		addr;
	Induct		inductBuf;

	CHKERR(protocolName && ductName);

	/*	Must stop the induct before trying to remove it.	*/

	CHKERR(sdr_begin_xn(bpSdr));	/*	Lock memory.		*/
	findInduct(protocolName, ductName, &vduct, &vductElt);
	if (vductElt == 0)		/*	Not found.		*/
	{
		sdr_exit_xn(bpSdr);
		writeMemoNote("[?] Unknown induct", ductName);
		return 0;
	}

	/*	All parameters validated.				*/

	stopInduct(vduct);
	sdr_exit_xn(bpSdr);
	waitForInduct(vduct);
	CHKERR(sdr_begin_xn(bpSdr));
	resetInduct(vduct);
	ductElt = vduct->inductElt;
	addr = sdr_list_data(bpSdr, ductElt);
	sdr_read(bpSdr, (char *) &inductBuf, addr, sizeof(Induct));

	/*	Okay to remove this duct from the database.		*/

	dropInduct(vduct, vductElt);
	if (inductBuf.cliCmd)
	{
		sdr_free(bpSdr, inductBuf.cliCmd);
	}

	sdr_free(bpSdr, addr);
	sdr_list_delete(bpSdr, ductElt, NULL, NULL);
	if (sdr_end_xn(bpSdr) < 0)
	{
		putErrmsg("Can't remove duct.", NULL);
		return -1;
	}

	return 1;
}

int	bpStartInduct(char *protocolName, char *ductName)
{
	Sdr		bpSdr = getIonsdr();
	VInduct		*vduct;
	PsmAddress	vductElt;
	int		result = 1;

	CHKZERO(sdr_begin_xn(bpSdr));	/*	Just to lock memory.	*/
	findInduct(protocolName, ductName, &vduct, &vductElt);
	if (vductElt == 0)	/*	This is an unknown duct.	*/
	{
		writeMemoNote("[?] Unknown induct", ductName);
		result = 0;
	}
	else
	{
		startInduct(vduct);
	}

	sdr_exit_xn(bpSdr);	/*	Unlock memory.			*/
	return result;
}

void	bpStopInduct(char *protocolName, char *ductName)
{
	Sdr		bpSdr = getIonsdr();
	VInduct		*vduct;
	PsmAddress	vductElt;

	CHKVOID(sdr_begin_xn(bpSdr));	/*	Just to lock memory.	*/
	findInduct(protocolName, ductName, &vduct, &vductElt);
	if (vductElt == 0)	/*	This is an unknown duct.	*/
	{
		sdr_exit_xn(bpSdr);	/*	Unlock memory.		*/
		writeMemoNote("[?] Unknown induct", ductName);
		return;
	}

	stopInduct(vduct);
	sdr_exit_xn(bpSdr);	/*	Unlock memory.			*/
	waitForInduct(vduct);
	CHKVOID(sdr_begin_xn(bpSdr));	/*	Just to lock memory.	*/
	resetInduct(vduct);
	sdr_exit_xn(bpSdr);	/*	Unlock memory.			*/
}

void	findOutduct(char *protocolName, char *ductName, VOutduct **vduct,
		PsmAddress *vductElt)
{
	PsmPartition	bpwm = getIonwm();
	PsmAddress	elt;

	for (elt = sm_list_first(bpwm, (_bpvdb(NULL))->outducts); elt;
			elt = sm_list_next(bpwm, elt))
	{
		*vduct = (VOutduct *) psp(bpwm, sm_list_data(bpwm, elt));
		if (strcmp((*vduct)->protocolName, protocolName) == 0
		&& strcmp((*vduct)->ductName, ductName) == 0)
		{
			break;
		}
	}

	*vductElt = elt;
}

int	addOutduct(char *protocolName, char *ductName, char *cloCmd,
		unsigned int maxPayloadLength)
{
	Sdr		bpSdr = getIonsdr();
	ClProtocol	clpbuf;
	Object		clpElt;
	VOutduct	*vduct;
	PsmAddress	vductElt;
	Outduct		ductBuf;
	OutductStats	statsInit;
	Object		addr;
	Object		elt = 0;

	CHKERR(protocolName && ductName);
	if (*protocolName == 0 || *ductName == 0)
	{
		writeMemoNote("[?] Zero-length Outduct parm(s)", ductName);
		return 0;
	}

	if (strlen(ductName) > MAX_CL_DUCT_NAME_LEN)
	{
		writeMemoNote("[?] Outduct name is too long", ductName);
		return 0;
	}

	if (cloCmd)
	{
		if (*cloCmd == '\0')
		{
			cloCmd = NULL;
		}
		else
		{
			if (strlen(cloCmd) > MAX_SDRSTRING)
			{
				writeMemoNote("[?] CLO command string too long",
						cloCmd);
				return 0;
			}
		}
	}

	CHKERR(sdr_begin_xn(bpSdr));
	fetchProtocol(protocolName, &clpbuf, &clpElt);
	if (clpElt == 0)
	{
		sdr_exit_xn(bpSdr);
		writeMemoNote("[?] Protocol is unknown", protocolName);
		return 0;
	}

	findOutduct(protocolName, ductName, &vduct, &vductElt);
	if (vductElt != 0)	/*	This is a known duct.		*/
	{
		sdr_exit_xn(bpSdr);
		writeMemoNote("[?] Duplicate outduct", ductName);
		return 0;
	}

	/*	All parameters validated, okay to add the duct.		*/

	memset((char *) &ductBuf, 0, sizeof(Outduct));
	istrcpy(ductBuf.name, ductName, sizeof ductBuf.name);
	if (cloCmd)
	{
		ductBuf.cloCmd = sdr_string_create(bpSdr, cloCmd);
	}

	ductBuf.maxPayloadLen = maxPayloadLength;
	ductBuf.bulkQueue = sdr_list_create(bpSdr);
	ductBuf.stdQueue = sdr_list_create(bpSdr);
	ductBuf.urgentQueue = sdr_list_create(bpSdr);
	ductBuf.protocol = (Object) sdr_list_data(bpSdr, clpElt);
	ductBuf.stats = sdr_malloc(bpSdr, sizeof(OutductStats));
	if (ductBuf.stats)
	{
		memset((char *) &statsInit, 0, sizeof(OutductStats));
		sdr_write(bpSdr, ductBuf.stats, (char *) &statsInit,
				sizeof(OutductStats));
	}

	ductBuf.updateStats = 1;	/*	Default.		*/
	addr = sdr_malloc(bpSdr, sizeof(Outduct));
	if (addr)
	{
		elt = sdr_list_insert_last(bpSdr, clpbuf.outducts, addr);
		sdr_write(bpSdr, addr, (char *) &ductBuf, sizeof(Outduct));

		/*	Record duct's address in the "user data" of
		 *	each queue so that we can easily navigate
		 *	from a duct queue element back to the duct
		 *	via the queue.					*/

		sdr_list_user_data_set(bpSdr, ductBuf.bulkQueue, addr);
		sdr_list_user_data_set(bpSdr, ductBuf.stdQueue, addr);
		sdr_list_user_data_set(bpSdr, ductBuf.urgentQueue, addr);
	}

	if (sdr_end_xn(bpSdr) < 0 || elt == 0)
	{
		putErrmsg("Can't add outduct.", ductName);
		return -1;
	}

	CHKERR(sdr_begin_xn(bpSdr));	/*	Just to lock memory.	*/
	if (raiseOutduct(elt, _bpvdb(NULL)) < 0)
	{
		putErrmsg("Can't raise outduct.", NULL);
		sdr_exit_xn(bpSdr);
		return -1;
	}

	sdr_exit_xn(bpSdr);	/*	Unlock memory.			*/
	return 1;
}

int	updateOutduct(char *protocolName, char *ductName, char *cloCmd,
		unsigned int maxPayloadLength)
{
	Sdr		bpSdr = getIonsdr();
	VOutduct	*vduct;
	PsmAddress	vductElt;
	Object		addr;
	Outduct		ductBuf;

	CHKERR(protocolName && ductName);
	if (*protocolName == 0 || *ductName == 0)
	{
		writeMemoNote("[?] Zero-length Outduct parm(s)", ductName);
		return 0;
	}

	if (cloCmd)
	{
		if (*cloCmd == '\0')
		{
			cloCmd = NULL;
		}
		else
		{
			if (strlen(cloCmd) > MAX_SDRSTRING)
			{
				writeMemoNote("[?] CLO command string too long",
						cloCmd);
				return 0;
			}
		}
	}

	CHKERR(sdr_begin_xn(bpSdr));
	findOutduct(protocolName, ductName, &vduct, &vductElt);
	if (vductElt == 0)	/*	This is an unknown duct.	*/
	{
		sdr_exit_xn(bpSdr);
		writeMemoNote("[?] Unknown outduct", ductName);
		return 0;
	}

	/*	All parameters validated, okay to update the duct.
	 *	First wipe out current cloCmd, then establish new one.	*/

	addr = (Object) sdr_list_data(bpSdr, vduct->outductElt);
	sdr_stage(bpSdr, (char *) &ductBuf, addr, sizeof(Outduct));
	if (ductBuf.cloCmd)
	{
		sdr_free(bpSdr, ductBuf.cloCmd);
		ductBuf.cloCmd = 0;
	}

	if (cloCmd)
	{
		ductBuf.cloCmd = sdr_string_create(bpSdr, cloCmd);
	}

	ductBuf.maxPayloadLen = maxPayloadLength;
	sdr_write(bpSdr, addr, (char *) &ductBuf, sizeof(Outduct));
	if (sdr_end_xn(bpSdr) < 0)
	{
		putErrmsg("Can't update outduct.", ductName);
		return -1;
	}

	return 1;
}

int	removeOutduct(char *protocolName, char *ductName)
{
	Sdr		bpSdr = getIonsdr();
	VOutduct	*vduct;
	PsmAddress	vductElt;
	Object		ductElt;
	Object		addr;
	Outduct		outductBuf;

	CHKERR(protocolName && ductName);

	/*	Must stop the outduct before trying to remove it.	*/

	CHKERR(sdr_begin_xn(bpSdr));	/*	Lock memory.		*/
	findOutduct(protocolName, ductName, &vduct, &vductElt);
	if (vductElt == 0)		/*	Not found.		*/
	{
		sdr_exit_xn(bpSdr);
		writeMemoNote("[?] Unknown outduct", ductName);
		return 0;
	}

	/*	All parameters validated.				*/

	stopOutduct(vduct);
	sdr_exit_xn(bpSdr);
	waitForOutduct(vduct);
	CHKERR(sdr_begin_xn(bpSdr));
	resetOutduct(vduct);
	ductElt = vduct->outductElt;
	addr = sdr_list_data(bpSdr, ductElt);
	sdr_read(bpSdr, (char *) &outductBuf, addr, sizeof(Outduct));
	if (sdr_list_length(bpSdr, outductBuf.bulkQueue) != 0
	|| sdr_list_length(bpSdr, outductBuf.stdQueue) != 0
	|| sdr_list_length(bpSdr, outductBuf.urgentQueue) != 0)
	{
		sdr_exit_xn(bpSdr);
		writeMemoNote("[?] Outduct has data to transmit", ductName);
		return 0;
	}

	/*	Okay to remove this duct from the database.		*/

	dropOutduct(vduct, vductElt);
	if (outductBuf.cloCmd)
	{
		sdr_free(bpSdr, outductBuf.cloCmd);
	}

	sdr_list_destroy(bpSdr, outductBuf.bulkQueue, NULL, NULL);
	sdr_list_destroy(bpSdr, outductBuf.stdQueue, NULL, NULL);
	sdr_list_destroy(bpSdr, outductBuf.urgentQueue, NULL,NULL);
	sdr_free(bpSdr, addr);
	sdr_list_delete(bpSdr, ductElt, NULL, NULL);
	if (sdr_end_xn(bpSdr) < 0)
	{
		putErrmsg("Can't remove outduct.", NULL);
		return -1;
	}

	return 1;
}

int	bpStartOutduct(char *protocolName, char *ductName)
{
	Sdr		bpSdr = getIonsdr();
	VOutduct	*vduct;
	PsmAddress	vductElt;
	int		result = 1;

	CHKZERO(sdr_begin_xn(bpSdr));	/*	Just to lock memory.	*/
	findOutduct(protocolName, ductName, &vduct, &vductElt);
	if (vductElt == 0)	/*	This is an unknown duct.	*/
	{
		writeMemoNote("[?] Unknown outduct", ductName);
		result = 0;
	}
	else
	{
		startOutduct(vduct);
	}

	sdr_exit_xn(bpSdr);	/*	Unlock memory.			*/
	return result;
}

void	bpStopOutduct(char *protocolName, char *ductName)
{
	Sdr		bpSdr = getIonsdr();
	VOutduct	*vduct;
	PsmAddress	vductElt;

	CHKVOID(sdr_begin_xn(bpSdr));	/*	Just to lock memory.	*/
	findOutduct(protocolName, ductName, &vduct, &vductElt);
	if (vductElt == 0)	/*	This is an unknown duct.	*/
	{
		sdr_exit_xn(bpSdr);	/*	Unlock memory.		*/
		writeMemoNote("[?] Unknown outduct", ductName);
		return;
	}

	stopOutduct(vduct);
	sdr_exit_xn(bpSdr);	/*	Unlock memory.			*/
	waitForOutduct(vduct);
	CHKVOID(sdr_begin_xn(bpSdr));	/*	Just to lock memory.	*/
	resetOutduct(vduct);
	sdr_exit_xn(bpSdr);	/*	Unlock memory.			*/
}

static int	findIncomplete(Bundle *bundle, VEndpoint *vpoint,
			Object *incompleteAddr, Object *incompleteElt)
{
	Sdr	bpSdr = getIonsdr();
	char	*argDictionary;
		OBJ_POINTER(Endpoint, endpoint);
	Object	elt;
		OBJ_POINTER(IncompleteBundle, incomplete);
		OBJ_POINTER(Bundle, fragment);
	char	*fragDictionary;
	int	result;

	CHKERR(ionLocked());
	if ((argDictionary = retrieveDictionary(bundle)) == (char *) bundle)
	{
		putErrmsg("Can't retrieve dictionary.", NULL);
		return -1;
	}
	
	*incompleteElt = 0;
	GET_OBJ_POINTER(bpSdr, Endpoint, endpoint, 
			sdr_list_data(bpSdr, vpoint->endpointElt));
	for (elt = sdr_list_first(bpSdr, endpoint->incompletes); elt;
			elt = sdr_list_next(bpSdr, elt))
	{
		*incompleteAddr = sdr_list_data(bpSdr, elt);
		GET_OBJ_POINTER(bpSdr, IncompleteBundle, incomplete, 
				*incompleteAddr);

		/*	See if ID of Incomplete's first fragment
		 *	matches ID of the bundle we're looking for.	*/

		GET_OBJ_POINTER(bpSdr, Bundle, fragment, sdr_list_data(bpSdr,
				sdr_list_first(bpSdr, incomplete->fragments)));

		/*	First compare source endpoint IDs.		*/

		if (fragment->id.source.cbhe != bundle->id.source.cbhe)
		{
			continue;
		}

		/*	Note: only destination can ever be multicast.	*/

		if (fragment->id.source.cbhe == 1)
		{
			if (fragment->id.source.c.nodeNbr !=
					bundle->id.source.c.nodeNbr
			|| fragment->id.source.c.serviceNbr !=
					bundle->id.source.c.serviceNbr)
			{
				continue;
			}
		}
		else	/*	Non-CBHE endpoint ID expression.	*/
		{
			if ((fragDictionary = retrieveDictionary(fragment))
					== (char *) fragment)
			{
				releaseDictionary(argDictionary);
				putErrmsg("Can't retrieve dictionary.", NULL);
				return -1;
			}
	
			result = (strcmp(fragDictionary +
				fragment->id.source.d.schemeNameOffset,
				argDictionary +
				bundle->id.source.d.schemeNameOffset) != 0)
				|| (strcmp(fragDictionary +
				fragment->id.source.d.nssOffset,
				argDictionary +
				bundle->id.source.d.nssOffset) != 0);
			releaseDictionary(fragDictionary);
			if (result != 0)
			{
				continue;
			}
		}

		/*	Compare creation times.				*/

		if (fragment->id.creationTime.seconds ==
				bundle->id.creationTime.seconds
		&& fragment->id.creationTime.count ==
				bundle->id.creationTime.count)
		{
			*incompleteElt = elt;	/*	Got it.		*/
			break;
		}
	}

	releaseDictionary(argDictionary);
	return 0;
}

Object	insertBpTimelineEvent(BpEvent *newEvent)
{
	PsmPartition	wm = getIonwm();
	PsmAddress	timeline = (getBpVdb())->timeline;
	PsmAddress	node;
	PsmAddress	successor;
	Sdr		bpSdr = getIonsdr();
	BpDB		*bpConstants = _bpConstants();
	Address		addr;
	Object		nextElt;
	Object		elt;

	CHKZERO(ionLocked());
	node = sm_rbt_search(wm, timeline, orderBpEvents, newEvent, &successor);
	if (node != 0)
	{
		/*	Event already exists; return its list elt.	*/

		return sm_rbt_data(wm, node);
	}

	addr = sdr_malloc(bpSdr, sizeof(BpEvent));
	if (addr == 0)
	{
		putErrmsg("No space for timeline event.", NULL);
		return 0;
	}

	sdr_write(bpSdr, addr, (char *) newEvent, sizeof(BpEvent));
	if (successor)
	{
		nextElt = (Object) sm_rbt_data(wm, successor);
		elt = sdr_list_insert_before(bpSdr, nextElt, addr);
	}
	else
	{
		elt = sdr_list_insert_last(bpSdr, bpConstants->timeline, addr);
	}

	if (elt == 0)
	{
		return 0;	/*	No room for list element.	*/
	}

	if (sm_rbt_insert(wm, timeline, elt, orderBpEvents, newEvent) == 0)
	{
		return 0;	/*	No room for lookup tree node.	*/
	}

	return elt;
}

/*	*	*	Bundle origination functions	*	*	*/

static void	computeExpirationTime(Bundle *bundle)
{
	unsigned int	secConsumed;
	unsigned int	usecConsumed;
	struct timeval	timeRemaining;
	struct timeval	expirationTime;
	struct timeval	currentTime;

	if (ionClockIsSynchronized() && bundle->id.creationTime.seconds > 0)
	{
		bundle->expirationTime = bundle->id.creationTime.seconds
				+ bundle->timeToLive;
	}
	else	/*	Round remaining time to nearest second.		*/
	{
		/*	Default is current time (in EPOCH_2000,
		 *	like bundle creation time).			*/

		bundle->expirationTime = getUTCTime() - EPOCH_2000_SEC;

		/*	Compute remaining lifetime for bundle.		*/

		timeRemaining.tv_sec = bundle->timeToLive;
		timeRemaining.tv_usec = 0;
		secConsumed = bundle->age / 1000000;
		if (timeRemaining.tv_sec < secConsumed)
		{
			return;
		}

		timeRemaining.tv_sec -= secConsumed;
		usecConsumed = bundle->age % 1000000;
		if (timeRemaining.tv_usec < usecConsumed)
		{
			if (timeRemaining.tv_sec == 0)
			{
				return;
			}

			timeRemaining.tv_sec -= 1;
			timeRemaining.tv_usec += 1000000;
		}

		timeRemaining.tv_usec -= usecConsumed;

		/*	Add remaining lifetime to arrival time,
		 *	to get expiration time in local time scale.	*/

		expirationTime.tv_sec = bundle->arrivalTime.tv_sec
				+ timeRemaining.tv_sec;
		expirationTime.tv_usec = bundle->arrivalTime.tv_usec
				+ timeRemaining.tv_usec;
		if (expirationTime.tv_usec > 1000000)
		{
			expirationTime.tv_sec += 1;
			expirationTime.tv_usec -= 1000000;
		}

		/*	Convert from local expiration time to UTC,
		 *	by subtracting current time from local
		 *	expiration time, rounding to the nearest
		 *	second, and adding the rounded difference
		 *	to the current UTC time.			*/

		getCurrentTime(&currentTime);
		if (expirationTime.tv_usec < currentTime.tv_usec)
		{
			if (expirationTime.tv_sec == 0)
			{
				return;
			}

			expirationTime.tv_usec += 1000000;
			expirationTime.tv_sec -= 1;
		}

		expirationTime.tv_sec -= currentTime.tv_sec;
		expirationTime.tv_usec -= currentTime.tv_usec;
		if (expirationTime.tv_usec >= 500000)
		{
			expirationTime.tv_sec += 1;
		}

		bundle->expirationTime += expirationTime.tv_sec; 
	}
}

static int	setBundleTTL(Bundle *bundle, Object bundleObj)
{
	BpEvent	event;

	/*	Schedule purge of this bundle on expiration of its
	 *	time-to-live.  Bundle expiration time is event time.	*/

	event.type = expiredTTL;
	event.time = bundle->expirationTime + EPOCH_2000_SEC;
	event.ref = bundleObj;
	bundle->timelineElt = insertBpTimelineEvent(&event);
	if (bundle->timelineElt == 0)
	{
		putErrmsg("Can't schedule bundle's TTL expiration.", NULL);
		return -1;
	}

	return 0;
}

static int	catalogueBundle(Bundle *bundle, Object bundleObj)
{
	Sdr		sdr = getIonsdr();
	Object		bundles = (_bpConstants())->bundles;
	char		*dictionary;
	char		*sourceEid;
	char		bundleKey[BUNDLES_HASH_KEY_BUFLEN];
	Address		bsetObj;
	Object		hashElt;
	BundleSet	bset;
	int		result = 0;

	CHKERR(ionLocked());

	/*	Insert bundle into hashtable of all bundles.		*/

	if ((dictionary = retrieveDictionary(bundle)) == (char *) bundle)
	{
		putErrmsg("Can't retrieve dictionary.", NULL);
		return -1;
	}

	if (printEid(&(bundle->id.source), dictionary, &sourceEid) < 0)
	{
		putErrmsg("Can't print source EID.", NULL);
		releaseDictionary(dictionary);
		return -1;
	}

	if (constructBundleHashKey(bundleKey, sourceEid,
			bundle->id.creationTime.seconds,
			bundle->id.creationTime.count,
			bundle->id.fragmentOffset,
			bundle->totalAduLength == 0 ? 0 :
			bundle->payload.length) > BUNDLES_HASH_KEY_LEN)
	{
		writeMemoNote("[?] Max hash key length exceeded; bundle \
cannot be retrieved by key", bundleKey);
		MRELEASE(sourceEid);
		releaseDictionary(dictionary);
		return 0;
	}

	/*	If a hashtable entry for this key already exists, then
	 *	we've got a non-unique key (cloned bundles) and no
	 *	single bundle address can be associated with this key.
	 *	So our first step is to retrieve that entry if it
	 *	exists.  If we find it, we set its bundleObj to zero
	 *	and add 1 to its count.  If not, we insert a new entry.	*/

	switch (sdr_hash_retrieve(sdr, bundles, bundleKey, &bsetObj, &hashElt))
	{
	case -1:
		putErrmsg("Can't revise hash table entry.", NULL);
		result = -1;
		break;

	case 1:		/*	Retrieval succeeded, non-unique key.	*/
		sdr_stage(sdr, (char *) &bset, bsetObj, sizeof(BundleSet));
		bset.bundleObj = 0;
		bset.count++;
		sdr_write(sdr, bsetObj, (char *) &bset, sizeof(BundleSet));
		bundle->hashEntry = hashElt;
#if 0
		writeMemoNote("[?] Bundle hash key is not unique; bundles \
cannot be retrieved by key", bundleKey);
#endif
		break;

	default:	/*	No such pre-existing entry.		*/
		bsetObj = sdr_malloc(sdr, sizeof(BundleSet));
		if (bsetObj == 0)
		{
			putErrmsg("Can't create hash table entry.", NULL);
			result = -1;
			break;
		}

		bset.bundleObj = bundleObj;
		bset.count = 1;
		sdr_write(sdr, bsetObj, (char *) &bset, sizeof(BundleSet));
		if (sdr_hash_insert(sdr, bundles, bundleKey, bsetObj,
				&(bundle->hashEntry)) < 0)
		{
			putErrmsg("Can't insert into hash table.", NULL);
			result = -1;
		}
	}

	MRELEASE(sourceEid);
	releaseDictionary(dictionary);
	return result;
}

int	bpClone(Bundle *oldBundle, Bundle *newBundle, Object *newBundleObj,
		unsigned int offset, unsigned int length)
{
	Sdr		bpSdr = getIonsdr();
	char		*dictionaryBuffer;
	char		*senderEid;
	BpString	*oldSenderEid;
	int		result;

	CHKERR(oldBundle);
	CHKERR(newBundle);
	CHKERR(newBundleObj);
	if (length == 0)	/*	"Clone entire payload."		*/
	{
		length = oldBundle->payload.length;
	}

	if (length == 0
	|| (offset + length) > oldBundle->payload.length)
	{
		putErrmsg("Invalid payload clone scope.", utoa(length));
		return -1;
	}

	*newBundleObj = sdr_malloc(bpSdr, sizeof(Bundle));
	if (*newBundleObj == 0)
	{
		putErrmsg("Can't create copy of bundle.", NULL);
		return -1;
	}

	memcpy((char *) newBundle, (char *) oldBundle, sizeof(Bundle));
	if (oldBundle->dictionary)	/*	Must copy dictionary.	*/
	{
		dictionaryBuffer = retrieveDictionary(oldBundle);
		if (dictionaryBuffer == (char *) oldBundle)
		{
			putErrmsg("Can't retrieve dictionary.", NULL);
			return -1;
		}

		newBundle->dictionary = sdr_malloc(bpSdr,
				oldBundle->dictionaryLength);
		if (newBundle->dictionary == 0)
		{
			releaseDictionary(dictionaryBuffer);
			putErrmsg("Can't create copy of dictionary.", NULL);
			return -1;
		}

		sdr_write(bpSdr, newBundle->dictionary, dictionaryBuffer,
				oldBundle->dictionaryLength);
		releaseDictionary(dictionaryBuffer);
	}

	/*	Clone part or all of payload.				*/

	newBundle->payload.content = zco_clone(bpSdr,
			oldBundle->payload.content, offset, length);
	if (newBundle->payload.content == 0)
	{
		putErrmsg("Can't clone payload content", NULL);
		return -1;
	}

	newBundle->payload.length = length;
	if (length != oldBundle->payload.length)
	{
		/*	The new bundle is a fragment of the original.	*/

		if (oldBundle->bundleProcFlags & BDL_IS_FRAGMENT)
		{
			newBundle->id.fragmentOffset =
					oldBundle->id.fragmentOffset + offset;
			/*	totalAduLength has already been copied.	*/

		}
		else	/*	Original bundle is NOT a fragment.	*/
		{
			newBundle->id.fragmentOffset = offset;
			newBundle->totalAduLength = oldBundle->payload.length;
			newBundle->bundleProcFlags |= BDL_IS_FRAGMENT;
		}
	}

	/*	Copy extension blocks and collaboration blocks.		*/

	newBundle->extensions[0] = 0;
	newBundle->extensionsLength[1] = 0;
	newBundle->extensions[1] = 0;
	newBundle->extensionsLength[0] = 0;
	newBundle->collabBlocks = 0;
	if (copyExtensionBlocks(newBundle, oldBundle) < 0)
	{
		putErrmsg("Can't copy extensions.", NULL);
		return -1;
	}

	/*	Copy sender endpoint ID.				*/

	oldSenderEid = &(oldBundle->clDossier.senderEid);
	senderEid = getBpString(oldSenderEid);
	if (senderEid == NULL)
	{
		memset(&(newBundle->clDossier.senderEid), 0, sizeof(BpString));
	}
	else if (senderEid == (char *) oldSenderEid)
	{
		putErrmsg("Can't copy sender EID.", NULL);
		return -1;
	}
	else	/*	Sender EID must be copied.			*/
	{
		result = putBpString(&(newBundle->clDossier.senderEid), 
				senderEid);
		MRELEASE(senderEid);
		if (result < 0)
		{
			putErrmsg("Can't copy sender EID.", NULL);
			return -1;
		}
	}

	/*	Initialize stations stack.				*/

	newBundle->stations = sdr_list_create(bpSdr);

	/*	Add new bundle to lookup hash table and timeline.	*/

	if (catalogueBundle(newBundle, *newBundleObj) < 0)
	{
		putErrmsg("Can't catalogue bundle copy in hash table.", NULL);
		return -1;
	}

	if (setBundleTTL(newBundle, *newBundleObj) < 0)
	{
		putErrmsg("Can't insert copy of bundle into timeline.", NULL);
		return -1;
	}

	/*	Detach new bundle from all pointers to old bundle.	*/

	newBundle->overdueElt = 0;
	newBundle->ctDueElt = 0;
	newBundle->fwdQueueElt = 0;
	newBundle->fragmentElt = 0;
	newBundle->dlvQueueElt = 0;
	newBundle->trackingElts = sdr_list_create(bpSdr);
	newBundle->incompleteElt = 0;
	newBundle->ductXmitElt = 0;

	/*	Retain relevant routing information.			*/

	if (oldBundle->proxNodeEid)
	{
		newBundle->proxNodeEid = sdr_string_dup(bpSdr,
				oldBundle->proxNodeEid);
		if (newBundle->proxNodeEid == 0)
		{
			putErrmsg("Can't copy proxNodeEid.", NULL);
			return -1;
		}
	}

	if (oldBundle->destDuctName)
	{
		newBundle->destDuctName = sdr_string_dup(bpSdr,
				oldBundle->destDuctName);
		if (newBundle->destDuctName == 0)
		{
			putErrmsg("Can't copy destDuctName.", NULL);
			return -1;
		}
	}

	/*	Adjust database occupancy, record bundle, return.	*/

	noteBundleInserted(newBundle);
	sdr_write(bpSdr, *newBundleObj, (char *) newBundle, sizeof(Bundle));
	return 0;
}

int	forwardBundle(Object bundleObj, Bundle *bundle, char *eid)
{
	Sdr		bpSdr = getIonsdr();
	Object		elt;
	char		eidBuf[SDRSTRING_BUFSZ];
	MetaEid		stationMetaEid;
	Object		stationEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	Scheme		schemeBuf;

	CHKERR(bundleObj);
	CHKERR(bundle);
	CHKERR(eid);

	/*	Error if this bundle is already in the process of
	 *	being delivered locally.  A bundle that is being
	 *	delivered and must also be forwarded must be cloned,
	 *	and only the clone may be passed to this function.	*/

	CHKERR(bundle->dlvQueueElt == 0);
	CHKERR(bundle->fragmentElt == 0);

	/*	If bundle is already being forwarded, then a
	 *	redundant CT signal indicating custody refusal
	 *	has been received, and we haven't yet finished
	 *	responding to the prior one.  So nothing to do
	 *	at this point.						*/

	if (bundle->fwdQueueElt || bundle->ductXmitElt)
	{
		return 0;
	}

	/*	Count as queued for forwarding, but only when the
	 *	station stack depth is 1.  Forwarders handing the
	 *	bundle off to one another doesn't count as queuing
	 *	a bundle for forwarding.				*/

	if (sdr_list_length(bpSdr, bundle->stations) == 1)
	{
		bpDbTally(BP_DB_QUEUED_FOR_FWD, bundle->payload.length);
	}

	/*	Now the forwarding of (i.e., the selection of a
	 *	transmission outduct for) this bundle either fails
	 *	for some reason or succeeds.				*/

	if (strlen(eid) >= SDRSTRING_BUFSZ)
	{
		/*	EID is too long to insert into the stations
		 *	stack, which is only sdrstrings.  We cannot
		 *	forward this bundle.
		 *
		 *	Must write the bundle to the SDR in order to
		 *	destroy it successfully.			*/

		sdr_write(bpSdr, bundleObj, (char *) bundle, sizeof(Bundle));
		return bpAbandon(bundleObj, bundle);
	}

	/*	Prevent routing loop: eid must not already be in the
	 *	bundle's stations stack.				*/

	for (elt = sdr_list_first(bpSdr, bundle->stations); elt;
			elt = sdr_list_next(bpSdr, elt))
	{
		sdr_string_read(bpSdr, eidBuf, sdr_list_data(bpSdr, elt));
		if (strcmp(eidBuf, eid) == 0)	/*	Routing loop.	*/
		{
			sdr_write(bpSdr, bundleObj, (char *) bundle,
					sizeof(Bundle));
			return bpAbandon(bundleObj, bundle);
		}
	}

	CHKERR(ionLocked());
	if (parseEidString(eid, &stationMetaEid, &vscheme, &vschemeElt) == 0)
	{
		/*	Can't forward: can't make sense of this EID.	*/

		restoreEidString(&stationMetaEid);
		writeMemoNote("[?] Can't parse neighbor EID", eid);
		sdr_write(bpSdr, bundleObj, (char *) bundle, sizeof(Bundle));
		return bpAbandon(bundleObj, bundle);
	}

	restoreEidString(&stationMetaEid);
	if (stationMetaEid.nullEndpoint)
	{
		/*	Forwarder has determined that the bundle
		 *	should be forwarded to the bit bucket, so
		 *	we must do so.					*/

		sdr_write(bpSdr, bundleObj, (char *) bundle, sizeof(Bundle));
		return bpAbandon(bundleObj, bundle);
	}

	/*	We're going to queue this bundle for processing by
	 *	the forwarder for the station EID's scheme name.
	 *	Push the station EID onto the stations stack in case
	 *	we need to do further re-queuing and check for routing
	 *	loops again.						*/

	stationEid = sdr_string_create(bpSdr, eid);
	if (stationEid == 0
	|| sdr_list_insert_first(bpSdr, bundle->stations, stationEid) == 0)
	{
		putErrmsg("Can't push EID onto stations stack.", NULL);
		return -1;
	}

	/*	Do any forwarding-triggered extension block processing
	 *	that is necessary.					*/

	if (processExtensionBlocks(bundle, PROCESS_ON_FORWARD, NULL) < 0)
	{
		putErrmsg("Can't process extensions.", "forward");
		return -1;
	}

	/*	Queue this bundle for the scheme-specific forwarder.	*/

	sdr_read(bpSdr, (char *) &schemeBuf, sdr_list_data(bpSdr,
			vscheme->schemeElt), sizeof(Scheme));
	bundle->fwdQueueElt = sdr_list_insert_last(bpSdr,
			schemeBuf.forwardQueue, bundleObj);
	sdr_write(bpSdr, bundleObj, (char *) bundle, sizeof(Bundle));

	/*	Wake up the scheme-specific forwarder as necessary.	*/

	if (vscheme->semaphore != SM_SEM_NONE)
	{
		sm_SemGive(vscheme->semaphore);
	}

	return 0;
}

static void	loadCbheEids(Bundle *bundle, MetaEid *destMetaEid,
		MetaEid *sourceMetaEid, MetaEid *reportToMetaEid)
{
	/*	Destination.						*/

	bundle->destination.cbhe = 1;
	if (bundle->bundleProcFlags & BDL_DEST_IS_SINGLETON)
	{
		bundle->destination.unicast = 1;
	}
	else
	{
		bundle->destination.unicast = 0;
	}

	bundle->destination.c.nodeNbr = destMetaEid->nodeNbr;
	bundle->destination.c.serviceNbr = destMetaEid->serviceNbr;

	/*	Custodian (none).					*/

	bundle->custodian.cbhe = 1;
	bundle->custodian.unicast = 1;
	bundle->custodian.c.nodeNbr = 0;
	bundle->custodian.c.serviceNbr = 0;

	/*	Source.							*/

	bundle->id.source.cbhe = 1;
	bundle->id.source.unicast = 1;
	if (sourceMetaEid == NULL)	/*	Anonymous.		*/
	{
		bundle->id.source.c.nodeNbr = 0;
		bundle->id.source.c.serviceNbr = 0;
	}
	else
	{
		bundle->id.source.c.nodeNbr = sourceMetaEid->nodeNbr;
		bundle->id.source.c.serviceNbr = sourceMetaEid->serviceNbr;
	}

	/*	Report-to.						*/

	bundle->reportTo.cbhe = 1;
	bundle->reportTo.unicast = 1;
	if (reportToMetaEid == NULL)
	{
		if (sourceMetaEid)	/*	Default to source.	*/
		{
			bundle->reportTo.c.nodeNbr = sourceMetaEid->nodeNbr;
			bundle->reportTo.c.serviceNbr
					= sourceMetaEid->serviceNbr;
		}
		else			/*	None.			*/
		{
			bundle->reportTo.c.nodeNbr = 0;
			bundle->reportTo.c.serviceNbr = 0;
		}
	}
	else
	{
		bundle->reportTo.c.nodeNbr = reportToMetaEid->nodeNbr;
		bundle->reportTo.c.serviceNbr = reportToMetaEid->serviceNbr;
	}
}

static int	addStringToDictionary(char **strings, int *stringLengths,
			int *stringCount, unsigned int *dictionaryLength,
			char *string, int stringLength)
{
	int	offset = 0;
	int	i;

	for (i = 0; i < *stringCount; i++)
	{
		if (strcmp(strings[i], string) == 0)	/*	have it	*/
		{
			return offset;
		}

		/*	Try the next one.				*/

		offset += (stringLengths[i] + 1);
	}

	/*	Must append the string to the dictionary.		*/

	strings[i] = string;
	stringLengths[i] = stringLength;
	(*stringCount)++;
	(*dictionaryLength) += (stringLength + 1);
	return offset;
}

static char	*loadDtnEids(Bundle *bundle, MetaEid *destMetaEid,
			MetaEid *sourceMetaEid, MetaEid *reportToMetaEid)
{
	char	*strings[8];
	int	stringLengths[8];
	int	stringCount = 0;
	char	*dictionary;
	char	*cursor;
	int	i;

	bundle->dictionaryLength = 0;
	memset((char *) strings, 0, sizeof strings);
	memset((char *) stringLengths, 0, sizeof stringLengths);

	/*	Custodian (none).					*/

	bundle->custodian.cbhe = 0;
	bundle->custodian.unicast = 1;
	bundle->custodian.d.schemeNameOffset
		= addStringToDictionary(strings, stringLengths, &stringCount,
			&bundle->dictionaryLength, "dtn", 3);
	bundle->custodian.d.nssOffset
		= addStringToDictionary(strings, stringLengths, &stringCount,
			&bundle->dictionaryLength, "none", 4);

	/*	Destination.						*/

	bundle->destination.cbhe = 0;
	bundle->destination.unicast = 1;
	bundle->destination.d.schemeNameOffset
		= addStringToDictionary(strings, stringLengths, &stringCount,
			&bundle->dictionaryLength, destMetaEid->schemeName,
			destMetaEid->schemeNameLength);
	bundle->destination.d.nssOffset
		= addStringToDictionary(strings, stringLengths, &stringCount,
			&bundle->dictionaryLength, destMetaEid->nss,
			destMetaEid->nssLength);

	/*	Source.							*/

	bundle->id.source.cbhe = 0;
	bundle->id.source.unicast = 1;
	if (sourceMetaEid == NULL)	/*	Anonymous.		*/
	{
		/*	Custodian is known to be "none".		*/

		bundle->id.source.d.schemeNameOffset
				= bundle->custodian.d.schemeNameOffset;
		bundle->id.source.d.nssOffset
				= bundle->custodian.d.nssOffset;
	}
	else
	{
		bundle->id.source.d.schemeNameOffset
			= addStringToDictionary(strings, stringLengths,
				&stringCount, &bundle->dictionaryLength,
				sourceMetaEid->schemeName,
				sourceMetaEid->schemeNameLength);
		bundle->id.source.d.nssOffset
			= addStringToDictionary(strings, stringLengths,
				&stringCount, &bundle->dictionaryLength,
				sourceMetaEid->nss,
				sourceMetaEid->nssLength);
	}

	/*	Report-to.						*/

	bundle->reportTo.cbhe = 0;
	bundle->reportTo.unicast = 1;
	if (reportToMetaEid == NULL)
	{
		if (sourceMetaEid)	/*	Default to source.	*/
		{
			bundle->reportTo.d.schemeNameOffset
					= bundle->id.source.d.schemeNameOffset;
			bundle->reportTo.d.nssOffset
					= bundle->id.source.d.nssOffset;
		}
		else			/*	None.			*/
		{
			/*	Custodian is known to be "none".	*/

			bundle->reportTo.d.schemeNameOffset
					= bundle->custodian.d.schemeNameOffset;
			bundle->reportTo.d.nssOffset
					= bundle->custodian.d.nssOffset;
		}
	}
	else
	{
		bundle->reportTo.d.schemeNameOffset
			= addStringToDictionary(strings, stringLengths,
				&stringCount, &bundle->dictionaryLength,
				reportToMetaEid->schemeName,
				reportToMetaEid->schemeNameLength);
		bundle->reportTo.d.nssOffset
			= addStringToDictionary(strings, stringLengths,
				&stringCount, &bundle->dictionaryLength,
				reportToMetaEid->nss,
				reportToMetaEid->nssLength);
	}

	bundle->dbOverhead += bundle->dictionaryLength;

	/*	Now concatenate all the strings into a dictionary.	*/

	dictionary = MTAKE(bundle->dictionaryLength);
	if (dictionary == NULL)
	{
		putErrmsg("No memory for dictionary.", NULL);
		return NULL;
	}

	cursor = dictionary;
	for (i = 0; i < stringCount; i++)
	{
		memcpy(cursor, strings[i], stringLengths[i]);
		cursor += stringLengths[i];
		*cursor = '\0';
		cursor++;
	}

	return dictionary;
}

int	bpSend(MetaEid *sourceMetaEid, char *destEidString,
		char *reportToEidString, int lifespan, int classOfService,
		BpCustodySwitch custodySwitch, unsigned char srrFlagsByte,
		int ackRequested, BpExtendedCOS *extendedCOS, Object adu,
		Object *bundleObj, int adminRecordType)
{
	Sdr		bpSdr = getIonsdr();
	BpVdb		*bpvdb = _bpvdb(NULL);
	Object		bpDbObject = getBpDbObject();
	BpDB		bpdb;
	int		bundleProcFlags = 0;
	unsigned int	srrFlags = srrFlagsByte;
	int		aduLength;
	Bundle		bundle;
	int		nonCbheEidCount = 0;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	MetaEid		destMetaEid;
	char		sourceEidString[MAX_EID_LEN];
	MetaEid		tempMetaEid;
	VScheme		*vscheme2;
	PsmAddress	vschemeElt2;
	VEndpoint	*vpoint = NULL;
	PsmAddress	vpointElt;
	MetaEid		reportToMetaEidBuf;
	MetaEid		*reportToMetaEid;
	DtnTime		currentDtnTime;
	char		*dictionary;
	int		i;
	ExtensionDef	*extensions;
	int		extensionsCt;
	ExtensionDef	*def;
	ExtensionBlock	blk;

	*bundleObj = 0;
	if (lifespan <= 0)
	{
		writeMemoNote("[?] Invalid lifespan", itoa(lifespan));
		return 0;
	}

	/*	Accommodate destination endpoint.			*/

	CHKERR(destEidString);
	if (*destEidString == 0)
	{
		writeMemo("[?] Zero-length destination EID.");
		return 0;
	}

	if (parseEidString(destEidString, &destMetaEid, &vscheme, &vschemeElt)
			== 0)
	{
		restoreEidString(&destMetaEid);
		writeMemoNote("[?] Destination EID malformed", destEidString);
		return 0;
	}

	if (destMetaEid.nullEndpoint)	/*	Do not send bundle.	*/
	{
		CHKERR(sdr_begin_xn(bpSdr));
		zco_destroy(bpSdr, adu);
		if (sdr_end_xn(bpSdr) < 0)
		{
			putErrmsg("Can't send bundle to null endpoint.", NULL);
			return -1;
		}

		return 1;
	}

	if (destMetaEid.cbhe)
	{
		if (vscheme->unicast)
		{
			bundleProcFlags |= BDL_DEST_IS_SINGLETON;
		}
	}
	else	/*	Non-IPN destination: no multicast convention.	*/
	{
		bundleProcFlags |= BDL_DEST_IS_SINGLETON;
		nonCbheEidCount++;
	}

	if (vscheme->unicast)
	{
		if (custodySwitch != NoCustodyRequested)
		{
			bundleProcFlags |= BDL_IS_CUSTODIAL;
		}
	}
	else	/*	Multicast: silently disable custody transfer.	*/
	{
		custodySwitch = NoCustodyRequested;
	}

	/*	The bundle protocol specification authorizes the
	 *	implementation to issue bundles whose source endpoint
	 *	ID is "dtn:none".  Since this could result in the
	 *	presence in the network of bundles lacking unique IDs,
	 *	making custody transfer and meaningful status reporting
	 *	impossible, this option is supported only when custody
	 *	transfer and status reporting are not requested.
	 *
	 *	To simplify processing, reduce all representations of
	 *	null source endpoint to a sourceMetaEid value of NULL.	*/

	if (sourceMetaEid)
	{
		if (sourceMetaEid->nullEndpoint)
		{
			sourceMetaEid = NULL;
		}
	}

	if (srrFlags != 0 || custodySwitch != NoCustodyRequested)
	{
		if (sourceMetaEid == NULL)
		{
			restoreEidString(&destMetaEid);
			writeMemo("[?] Source can't be anonymous when asking \
for custody transfer and/or status reports.");
			return 0;
		}

		if (adminRecordType != 0)
		{
			restoreEidString(&destMetaEid);
			writeMemo("[?] Can't ask for custody transfer and/or \
status reports for admin records.");
			return 0;
		}

		if (extendedCOS)
		{
			if (extendedCOS->flags & BP_MINIMUM_LATENCY)
			{
				restoreEidString(&destMetaEid);
				writeMemo("[?] Can't flag bundle as 'critical' \
when asking for custody transfer and/or status reports.");
				return 0;
			}
		}
	}

	/*	Set additional bundle processing flags.			*/

	if (sourceMetaEid == NULL)
	{
		/*	An anonymous bundle has no unique keys,
		 *	therefore can't be reassembled from fragments,
		 *	therefore can't be fragmented.			*/

		bundleProcFlags |= BDL_DOES_NOT_FRAGMENT;
	}

	if (ackRequested)
	{
		bundleProcFlags |= BDL_APP_ACK_REQUEST;
	}

	if (adminRecordType != 0)
	{
		bundleProcFlags |= BDL_IS_ADMIN;

		/*	Administrative bundles must not be anonymous.
		 *	Recipient needs to know source of the status
		 *	report or custody signal; use own admin EID
		 *	for the scheme cited by destination EID.	*/

		if (sourceMetaEid == NULL)
		{
			istrcpy(sourceEidString, vscheme->adminEid,
					sizeof sourceEidString);
			if (parseEidString(sourceEidString, &tempMetaEid,
					&vscheme2, &vschemeElt2) == 0)
			{
				restoreEidString(&tempMetaEid);
				writeMemoNote("[?] Admin EID malformed",
						sourceEidString);
				return 0;
			}

			sourceMetaEid = &tempMetaEid;
		}
	}

	/*	Check source and report-to endpoint IDs.		*/

	if (sourceMetaEid != NULL)	/*	Not "dtn:none".		*/
	{
		/*	Submitted by application on open endpoint.	*/

		if (!sourceMetaEid->cbhe)
		{
			nonCbheEidCount++;
		}

		/*	For network management....			*/

		findEndpoint(sourceMetaEid->schemeName, sourceMetaEid->nss,
				NULL, &vpoint, &vpointElt);
	}

	if (reportToEidString == NULL)	/*	default to source	*/
	{
		reportToMetaEid = NULL;
	}
	else
	{
		if (parseEidString(reportToEidString, &reportToMetaEidBuf,
				&vscheme, &vschemeElt) == 0)
		{
			restoreEidString(&reportToMetaEidBuf);
			writeMemoNote("[?] Report-to EID malformed",
					reportToEidString);
			return 0;
		}

		reportToMetaEid = &reportToMetaEidBuf;
		if (!reportToMetaEid->nullEndpoint)
		{
			if (!reportToMetaEid->cbhe)
			{
				nonCbheEidCount++;
			}
		}
	}

	/*	Create the outbound bundle.				*/

	memset((char *) &bundle, 0, sizeof(Bundle));
	if (sourceMetaEid == NULL)
	{
		bundle.anonymous = 1;
	}

	bundle.dbOverhead = BASE_BUNDLE_OVERHEAD;

	/*	The bundle protocol specification authorizes the
	 *	implementation to fragment an ADU.  In the ION
	 *	implementation we fragment only when necessary,
	 *	at the moment a bundle is dequeued for transmission.	*/

	CHKERR(sdr_begin_xn(bpSdr));
	aduLength = zco_length(bpSdr, adu);
	if (aduLength < 0)
	{
		sdr_exit_xn(bpSdr);
		restoreEidString(&destMetaEid);
		restoreEidString(reportToMetaEid);
		putErrmsg("Can't get length of ADU.", NULL);
		return -1;
	}

	bundle.bundleProcFlags = bundleProcFlags;
	bundle.bundleProcFlags += (classOfService << 7);
	bundle.bundleProcFlags += (srrFlags << 14);
	if (nonCbheEidCount == 0)
	{
		dictionary = NULL;
		loadCbheEids(&bundle, &destMetaEid, sourceMetaEid,
				reportToMetaEid);
	}
	else	/*	Not all endpoints are in same CBHE scheme.	*/
	{
		dictionary = loadDtnEids(&bundle, &destMetaEid, sourceMetaEid,
				reportToMetaEid);
		if (dictionary == NULL)
		{
			sdr_exit_xn(bpSdr);
			restoreEidString(&destMetaEid);
			restoreEidString(reportToMetaEid);
			putErrmsg("Can't load dictionary.", NULL);
			return -1;
		}
	}

	restoreEidString(&destMetaEid);
	restoreEidString(reportToMetaEid);
	bundle.id.fragmentOffset = 0;

	/*	Note: bundle is not a fragment when initially created,
	 *	so totalAduLength is left at zero.			*/

	bundle.custodyTaken = 0;
	bundle.payloadBlockProcFlags = BLK_MUST_BE_COPIED;
	bundle.payload.length = aduLength;
	bundle.payload.content = adu;

	/*	Convert all payload header and trailer capsules
	 *	into source data extents.  From the BP perspective,
	 *	everything in the ADU is source data.			*/

	if (zco_bond(bpSdr, bundle.payload.content) < 0)
	{
		putErrmsg("Can't convert payload capsules to extents.", NULL);
		sdr_cancel_xn(bpSdr);
		if (dictionary)
		{
			MRELEASE(dictionary);
		}

		return -1;
	}

	/*	Set creationTime of bundle.				*/

	if (ionClockIsSynchronized())
	{
		getCurrentDtnTime(&currentDtnTime);
		if (currentDtnTime.seconds != bpvdb->creationTimeSec)
		{
			bpvdb->creationTimeSec = currentDtnTime.seconds;
			bpvdb->bundleCounter = 0;
		}

		bundle.id.creationTime.seconds = bpvdb->creationTimeSec;
		bundle.id.creationTime.count = ++(bpvdb->bundleCounter);
	}
	else
	{
		/*	If no synchronized clock, then creationTime
		 *	seconds is always zero and a non-volatile
		 *	bundle counter increments monotonically.	*/

		bundle.id.creationTime.seconds = 0;
		sdr_stage(bpSdr, (char *) &bpdb, bpDbObject, sizeof(BpDB));
		bpdb.bundleCounter++;
		bundle.id.creationTime.count = bpdb.bundleCounter;
		sdr_write(bpSdr, bpDbObject, (char *) &bpdb, sizeof(BpDB));
	}

	getCurrentTime(&bundle.arrivalTime);
	bundle.timeToLive = lifespan;
	computeExpirationTime(&bundle);
	if (dictionary)
	{
		bundle.dictionary = sdr_malloc(bpSdr, bundle.dictionaryLength);
		if (bundle.dictionary == 0)
		{
			putErrmsg("No space for dictionary.", NULL);
			sdr_cancel_xn(bpSdr);
			MRELEASE(dictionary);
			return -1;
		}

		sdr_write(bpSdr, bundle.dictionary, dictionary,
				bundle.dictionaryLength);
		MRELEASE(dictionary);
	}

	bundle.extensions[0] = sdr_list_create(bpSdr);
	bundle.extensionsLength[0] = 0;
	bundle.extensions[1] = sdr_list_create(bpSdr);
	bundle.extensionsLength[1] = 0;
	bundle.collabBlocks = sdr_list_create(bpSdr);
	bundle.stations = sdr_list_create(bpSdr);
	bundle.trackingElts = sdr_list_create(bpSdr);
	*bundleObj = sdr_malloc(bpSdr, sizeof(Bundle));
	if (*bundleObj == 0
	|| bundle.stations == 0
	|| bundle.trackingElts == 0
	|| bundle.extensions[0] == 0
	|| bundle.extensions[1] == 0
	|| bundle.collabBlocks == 0)
	{
		putErrmsg("No space for bundle object.", NULL);
		sdr_cancel_xn(bpSdr);
		return -1;
	}

	if (setBundleTTL(&bundle, *bundleObj) < 0)
	{
		putErrmsg("Can't insert new bundle into timeline.", NULL);
		sdr_cancel_xn(bpSdr);
		return -1;
	}

	if (catalogueBundle(&bundle, *bundleObj) < 0)
	{
		putErrmsg("Can't catalogue new bundle in hash table.", NULL);
		sdr_cancel_xn(bpSdr);
		return -1;
	}

	if (extendedCOS)
	{
		bundle.extendedCOS.flowLabel = extendedCOS->flowLabel;
		bundle.extendedCOS.flags = extendedCOS->flags;
		bundle.extendedCOS.ordinal = extendedCOS->ordinal;
	}

	/*	Insert all applicable extension blocks into the bundle.	*/

	getExtensionDefs(&extensions, &extensionsCt);
	for (i = 0, def = extensions; i < extensionsCt; i++, def++)
	{
		if (def->type != 0 && def->offer != NULL)
		{
			memset((char *) &blk, 0, sizeof(ExtensionBlock));
			blk.type = def->type;
			if (def->offer(&blk, &bundle) < 0)
			{
				putErrmsg("Failed offering extension block.",
						NULL);
				sdr_cancel_xn(bpSdr);
				return -1;
			}

			if (blk.length == 0 && blk.size == 0)
			{
				continue;
			}

			if (attachExtensionBlock(def, &blk, &bundle) < 0)
			{
				putErrmsg("Failed attaching extension block.",
						NULL);
				sdr_cancel_xn(bpSdr);
				return -1;
			}
		}
	}

	noteBundleInserted(&bundle);

	/*	Here's where we finally write bundle to the database.	*/

	sdr_write(bpSdr, *bundleObj, (char *) &bundle, sizeof(Bundle));

	/*	Note: custodial reporting, as requested, is perfomed
	 *	by the destination scheme's forwarder.			*/

	if (forwardBundle(*bundleObj, &bundle, destEidString) < 0)
	{
		putErrmsg("Can't queue bundle for forwarding.", NULL);
		sdr_cancel_xn(bpSdr);
		return -1;
	}

	if (vpoint)
	{
		bpEndpointTally(vpoint, BP_ENDPOINT_SOURCED,
				bundle.payload.length);
	}

	bpSourceTally(classOfService, bundle.payload.length);
	if (bpvdb->watching & WATCH_a)
	{
		putchar('a');
		fflush(stdout);
	}

	if (sdr_end_xn(bpSdr) < 0)
	{
		putErrmsg("Can't send bundle.", NULL);
		return -1;
	}

	return 1;
}

/*	*	*	Bundle reception functions	*	*	*/

static int	sendCtSignal(Bundle *bundle, char *dictionary, int succeeded,
			BpCtReason reasonCode)
{
	char		*custodianEid;
	unsigned int	ttl;	/*	Original bundle's TTL.		*/
	BpExtendedCOS	ecos = { 0, 0, 255 };
	Object		payloadZco;
	Object		bundleObj;
	int		result;

	if (printEid(&bundle->custodian, dictionary, &custodianEid) < 0)
	{
		putErrmsg("Can't print custodian EID.", NULL);
		return -1;
	}

	if (bundle->custodian.cbhe)
	{
		if (bundle->custodian.c.nodeNbr == 0)
		{
			MRELEASE(custodianEid);
			return 0;	/*	No current custodian.	*/
		}
	}
	else	/*	Non-CBHE custodian endpoint ID.			*/
	{
		if (strcmp(custodianEid, _nullEid()) == 0)
		{
			MRELEASE(custodianEid);
			return 0;	/*	No current custodian.	*/
		}
	}

	/*	There is a current custodian, so construct and send
	 *	the signal.						*/

	bundle->ctSignal.succeeded = succeeded;
	bundle->ctSignal.reasonCode = reasonCode;
	getCurrentDtnTime(&bundle->ctSignal.signalTime);
	if (bundle->ctSignal.creationTime.seconds == 0)
	{
		bundle->ctSignal.creationTime.seconds
				= bundle->id.creationTime.seconds;
		bundle->ctSignal.creationTime.count
				= bundle->id.creationTime.count;
		if (bundle->bundleProcFlags & BDL_IS_FRAGMENT)
		{
			bundle->ctSignal.isFragment = 1;
			bundle->ctSignal.fragmentOffset =
					bundle->id.fragmentOffset;
			bundle->ctSignal.fragmentLength =
					bundle->payload.length;
		}
		else
		{
			bundle->ctSignal.isFragment = 0;
			bundle->ctSignal.fragmentOffset = 0;
			bundle->ctSignal.fragmentLength = 0;
		}
	}

	ttl = bundle->timeToLive;
	if (ttl < 1) ttl = 1;
	if (printEid(&bundle->id.source, dictionary,
			&bundle->ctSignal.sourceEid) < 0)
	{
		putErrmsg("Can't print source EID.", NULL);
		MRELEASE(custodianEid);
		return -1;
	}

	result = constructCtSignal(&(bundle->ctSignal), &payloadZco);
	MRELEASE(bundle->ctSignal.sourceEid);
	if (result < 0)
	{
		MRELEASE(custodianEid);
		putErrmsg("Can't construct custody transfer signal.", NULL);
		return -1;
	}

	result = bpSend(NULL, custodianEid, NULL, ttl, BP_EXPEDITED_PRIORITY,
			NoCustodyRequested, 0, 0, &ecos, payloadZco,
			&bundleObj, BP_CUSTODY_SIGNAL);
	MRELEASE(custodianEid);
       	switch (result)
	{
	case -1:
		putErrmsg("Can't send custody transfer signal.", NULL);
		return -1;

	case 0:
		putErrmsg("Custody transfer signal not transmitted.", NULL);

			/*	Intentional fall-through to next case.	*/

	default:
		return 0;
	}
}

static int	noteCtEvent(Bundle *bundle, AcqWorkArea *work, char *dictionary,
			int succeeded, BpCtReason reasonCode)
{
	if (offerNoteAcs(bundle, work, dictionary, succeeded, reasonCode) == 1)
	{
		return 0;
	}

	/*	Use standard custody signaling. 			*/

	return sendCtSignal(bundle, dictionary, succeeded, reasonCode);
}

int	sendStatusRpt(Bundle *bundle, char *dictionary)
{
	int		priority = COS_FLAGS(bundle->bundleProcFlags) & 0x03;
	unsigned int	ttl;	/*	Original bundle's TTL.		*/
	BpExtendedCOS	ecos = { 0, 0, bundle->extendedCOS.ordinal };
	Object		payloadZco;
	char		*reportToEid;
	Object		bundleObj;
	int		result;

	if (bundle->statusRpt.creationTime.seconds == 0)
	{
		bundle->statusRpt.creationTime.seconds
				= bundle->id.creationTime.seconds;
		bundle->statusRpt.creationTime.count
				= bundle->id.creationTime.count;
		if (bundle->bundleProcFlags & BDL_IS_FRAGMENT)
		{
			bundle->statusRpt.isFragment = 1;
			bundle->statusRpt.fragmentOffset =
					bundle->id.fragmentOffset;
			bundle->statusRpt.fragmentLength =
					bundle->payload.length;
		}
		else
		{
			bundle->statusRpt.isFragment = 0;
			bundle->statusRpt.fragmentOffset = 0;
			bundle->statusRpt.fragmentLength = 0;
		}
	}

	ttl = bundle->timeToLive;
	if (ttl < 1) ttl = 1;
	if (printEid(&bundle->id.source, dictionary,
			&bundle->statusRpt.sourceEid) < 0)
	{
		putErrmsg("Can't print source EID.", NULL);
		return -1;
	}

	result = constructStatusRpt(&(bundle->statusRpt), &payloadZco);
	MRELEASE(bundle->statusRpt.sourceEid);
	if (result < 0)
	{
		putErrmsg("Can't construct status report.", NULL);
		return -1;
	}

	if (printEid(&bundle->reportTo, dictionary, &reportToEid) < 0)
	{
		putErrmsg("Can't print source EID.", NULL);
		return -1;
	}

	result = bpSend(NULL, reportToEid, NULL, ttl, priority,
			NoCustodyRequested, 0, 0, &ecos, payloadZco,
			&bundleObj, BP_STATUS_REPORT);
	MRELEASE(reportToEid);
       	switch (result)
	{
	case -1:
		putErrmsg("Can't send status report.", NULL);
		return -1;

	case 0:
		writeMemo("[?] Status report not transmitted.");

			/*	Intentional fall-through to next case.	*/

	default:
		break;
	}

	bpRptTally(bundle->statusRpt.flags, bundle->statusRpt.reasonCode);

	/*	Erase flags and times in case another status report for
	 *	the same bundle needs to be sent later.			*/

	bundle->statusRpt.flags = 0;
	bundle->statusRpt.reasonCode = 0;
	memset(&bundle->statusRpt.receiptTime, 0, sizeof(struct timespec));
	memset(&bundle->statusRpt.acceptanceTime, 0, sizeof(struct timespec));
	memset(&bundle->statusRpt.forwardTime, 0, sizeof(struct timespec));
	memset(&bundle->statusRpt.deliveryTime, 0, sizeof(struct timespec));
	memset(&bundle->statusRpt.deletionTime, 0, sizeof(struct timespec));
	return 0;
}

static void	lookUpEndpoint(EndpointId eid, char *dictionary,
			VScheme *vscheme, VEndpoint **vpoint)
{
	PsmPartition	bpwm = getIonwm();
	char		nssBuf[42];
	char		*nss;
	PsmAddress	elt;

	if (dictionary == NULL)
	{
		isprintf(nssBuf, sizeof nssBuf, UVAST_FIELDSPEC ".%u",
				eid.c.nodeNbr, eid.c.serviceNbr);
		nss = nssBuf;
	}
	else
	{
		nss = dictionary + eid.d.nssOffset;
	}

	for (elt = sm_list_first(bpwm, vscheme->endpoints); elt;
			elt = sm_list_next(bpwm, elt))
	{
		*vpoint = (VEndpoint *) psp(bpwm, sm_list_data(bpwm, elt));
		if (strcmp((*vpoint)->nss, nss) == 0)
		{
			break;
		}
	}

	if (elt == 0)			/*	No match found.		*/
	{
		*vpoint = NULL;
	}
}

static int	enqueueForDelivery(Object bundleObj, Bundle *bundle,
			VEndpoint *vpoint)
{
	Sdr	bpSdr = getIonsdr();
		OBJ_POINTER(Endpoint, endpoint);
	char	scriptBuf[SDRSTRING_BUFSZ];

	GET_OBJ_POINTER(bpSdr, Endpoint, endpoint, sdr_list_data(bpSdr,
				vpoint->endpointElt));
	if (vpoint->appPid == ERROR)	/*	Not open by any app.	*/
	{
		/*	Execute reanimation script, if any.		*/

		if (endpoint->recvScript != 0)
		{
			if (sdr_string_read(bpSdr, scriptBuf,
					endpoint->recvScript) < 0)
			{
				putErrmsg("Failed reading recvScript.", NULL);
				return -1;
			}

			if (pseudoshell(scriptBuf) < 0)
			{
				putErrmsg("Script execution failed.", NULL);
				return -1;
			}
		}

		if (endpoint->recvRule == DiscardBundle)
		{
			sdr_write(bpSdr, bundleObj, (char *) bundle,
					sizeof(Bundle));
			bpEndpointTally(vpoint, BP_ENDPOINT_ABANDONED,
					bundle->payload.length);
			return 0;
		}
	}

	/*	Queue bundle up for delivery when requested by
	 *	application.						*/

	bundle->dlvQueueElt = sdr_list_insert_last(bpSdr,
			endpoint->deliveryQueue, bundleObj);
	if (bundle->dlvQueueElt == 0)
	{
		putErrmsg("Can't append to delivery queue.", NULL);
		return -1;
	}

	bundle->delivered = 1;
	sdr_write(bpSdr, bundleObj, (char *) bundle, sizeof(Bundle));
	if (vpoint->semaphore != SM_SEM_NONE)
	{
		sm_SemGive(vpoint->semaphore);
	}

	bpEndpointTally(vpoint, BP_ENDPOINT_QUEUED, bundle->payload.length);
	return 0;
}

static int	extendIncomplete(IncompleteBundle *incomplete, Object incElt,
			Object bundleObj, Bundle *bundle)
{
	Sdr	bpSdr = getIonsdr();
	Object	elt;
		OBJ_POINTER(Bundle, fragment);

	bundle->incompleteElt = incElt;

	/*	First look for fragment insertion point and insert
	 *	the new bundle at this point.				*/

	for (elt = sdr_list_first(bpSdr, incomplete->fragments); elt;
			elt = sdr_list_next(bpSdr, elt))
	{
		GET_OBJ_POINTER(bpSdr, Bundle, fragment,
				sdr_list_data(bpSdr, elt));
		if (fragment->id.fragmentOffset < bundle->id.fragmentOffset)
		{
			continue;
		}

		if (fragment->id.fragmentOffset == bundle->id.fragmentOffset)
		{
			bundle->delivered = 1;
			sdr_write(bpSdr, bundleObj, (char *) bundle,
					sizeof(Bundle));
			return 0;	/*	Duplicate fragment.	*/
		}

		break;	/*	Insert before this fragment.		*/
	}

	if (elt)
	{
		bundle->fragmentElt = sdr_list_insert_before(bpSdr, elt,
				bundleObj);
	}
	else
	{
		bundle->fragmentElt = sdr_list_insert_last(bpSdr,
				incomplete->fragments, bundleObj);
	}

	if (bundle->fragmentElt == 0)
	{
		putErrmsg("Can't insert bundle into fragments list.", NULL);
		return -1;
	}

	bundle->delivered = 1;
	sdr_write(bpSdr, bundleObj, (char *) bundle, sizeof(Bundle));
	return 0;
}

static int	createIncompleteBundle(Object bundleObj, Bundle *bundle,
			VEndpoint *vpoint)
{
	Sdr			bpSdr = getIonsdr();
	IncompleteBundle	incomplete;
	Object			incObj;
				OBJ_POINTER(Endpoint, endpoint);

	incomplete.fragments = sdr_list_create(bpSdr);
	if (incomplete.fragments == 0)
	{
		putErrmsg("No space for fragments list.", NULL);
		return -1;
	}

	incomplete.totalAduLength = bundle->totalAduLength;
	incObj = sdr_malloc(bpSdr, sizeof(IncompleteBundle));
	if (incObj == 0)
	{
		putErrmsg("No space for Incomplete object.", NULL);
		return -1;
	}

	sdr_write(bpSdr, incObj, (char *) &incomplete,
			sizeof(IncompleteBundle));
	GET_OBJ_POINTER(bpSdr, Endpoint, endpoint, sdr_list_data(bpSdr,
			vpoint->endpointElt));
	bundle->incompleteElt = sdr_list_insert_last(bpSdr,
			endpoint->incompletes, incObj);
	if (bundle->incompleteElt == 0)
	{
		putErrmsg("No space for Incomplete list element.", NULL);
		return -1;
	}

	/*	Enable navigation from fragment back to Incomplete.	*/

	sdr_list_user_data_set(bpSdr, incomplete.fragments,
			bundle->incompleteElt);

	/*	Bundle becomes first element in the fragments list.	*/

	bundle->fragmentElt = sdr_list_insert_last(bpSdr, incomplete.fragments,
			bundleObj);
	if (bundle->fragmentElt == 0)
	{
		putErrmsg("No space for fragment list elt.", NULL);
		return -1;
	}

	bundle->delivered = 1;
	sdr_write(bpSdr, bundleObj, (char *) bundle, sizeof(Bundle));
	return 0;
}

static int	deliverBundle(Object bundleObj, Bundle *bundle,
			VEndpoint *vpoint)
{
	char	*dictionary;
	int	result;
	Object	incompleteAddr = 0;
		OBJ_POINTER(IncompleteBundle, incomplete);
	Object	elt;

	CHKERR(ionLocked());

	/*	Regardless of delivery outcome, current custodian
	 *	(if any) may now release custody, because the bundle
	 *	has reached its destination.  NOTE: the local node
	 *	has NOT taken custody of this bundle.			*/

	if (bundleIsCustodial(bundle))
	{
		if ((dictionary = retrieveDictionary(bundle))
				== (char *) bundle)
		{
			putErrmsg("Can't retrieve dictionary.", NULL);
			return -1;
		}

		result = noteCtEvent(bundle, NULL, dictionary, 1, 0);
		releaseDictionary(dictionary);
		if (result < 0)
		{
			putErrmsg("Can't send custody signal.", NULL);
			return -1;
		}
	}

	/*	Next check to see if we've already got one or more
	 *	fragments of this bundle; if so, invoke reassembly
	 *	(which may or may not result in delivery of a new
	 *	reconstructed original bundle to the application).	*/

	if (findIncomplete(bundle, vpoint, &incompleteAddr, &elt) < 0)
	{
		putErrmsg("Failed seeking incomplete bundle.", NULL);
		return -1;
	}

	if (elt)	/*	Matching IncompleteBundle found.	*/
	{
		GET_OBJ_POINTER(getIonsdr(), IncompleteBundle, incomplete,
				incompleteAddr);
		return extendIncomplete(incomplete, elt, bundleObj, bundle);
	}

	/*	No existing incomplete bundle to extend, so if the
	 *	current bundle is a fragment we can't do anything
	 *	with it; start a new IncompleteBundle.			*/

	if (bundle->bundleProcFlags & BDL_IS_FRAGMENT)
	{
		return createIncompleteBundle(bundleObj, bundle, vpoint);
	}

	/*	Bundle is not a fragment, so we can deliver it right
	 *	away if that's the current policy for this endpoint.	*/

	return enqueueForDelivery(bundleObj, bundle, vpoint);
}

static int	dispatchBundle(Object bundleObj, Bundle *bundle,
			VEndpoint **vpoint)
{
	Sdr		bpSdr = getIonsdr();
	char		*dictionary;
	VScheme		*vscheme;
	Bundle		newBundle;
	Object		newBundleObj;
	char		*eidString;
	int		result;

	CHKERR(ionLocked());
	if ((dictionary = retrieveDictionary(bundle)) == (char *) bundle)
	{
		putErrmsg("Can't retrieve dictionary.", NULL);
		return -1;
	}

	lookUpEidScheme(bundle->destination, dictionary, &vscheme);
	if (vscheme != NULL)	/*	Destination might be local.	*/
	{
		lookUpEndpoint(bundle->destination, dictionary, vscheme,
				vpoint);
		if (*vpoint != NULL)	/*	Destination is here.	*/
		{
			if (deliverBundle(bundleObj, bundle, *vpoint) < 0)
			{
				releaseDictionary(dictionary);
				putErrmsg("Bundle delivery failed.", NULL);
				return -1;
			}

			/*	Bundle delivery did not fail.		*/

			if ((_bpvdb(NULL))->watching & WATCH_z)
			{
				putchar('z');
				fflush(stdout);
			}

			if (bundle->bundleProcFlags & BDL_DEST_IS_SINGLETON)
			{
				/*	Can't be forwarded to any other
				 *	node.  No further need for the
				 *	dictionary.			*/

				releaseDictionary(dictionary);

				/*	We now write the bundle state
				 *	object to the SDR and authorize
				 *	destruction of the bundle.  If
				 *	deliverBundle() enqueued the
				 *	bundle at an endpoint or
				 *	retained it as a fragment
				 *	needed for bundle reassembly,
				 *	then the bundle will not be
				 *	destroyed.  But in the event
				 *	that the endpoint is not
				 *	currently active (i.e., is
				 *	not currently opened by any
				 *	application) and the delivery
				 *	failure action for this
				 *	endpoint is DiscardBundle,
				 *	now the the time to destroy
				 *	the bundle.			*/

				sdr_write(bpSdr, bundleObj, (char *) bundle,
						sizeof(Bundle));
				if (bpDestroyBundle(bundleObj, 0) < 0)
				{
					putErrmsg("Can't destroy bundle.",
							NULL);
					return -1;
				}

				return 0;
			}
		}
		else	/*	Not deliverable at this node.		*/
		{
			if (bundle->destination.cbhe
			&& bundle->destination.unicast
			&& bundle->destination.c.nodeNbr == getOwnNodeNbr())
			{
				/*	Destination is known to be the
				 *	local bundle agent.  Since the
				 *	bundle can't be delivered, it
				 *	must be abandoned.  (It can't
				 *	be forwarded, as this would be
				 *	an infinite forwarding loop.)
				 *	But must first accept it, to
				 *	prevent current custodian from
				 *	re-forwarding it endlessly back
				 *	to the local bundle agent.	*/

				releaseDictionary(dictionary);
				if (bpAccept(bundleObj, bundle) < 0)
				{
					putErrmsg("Failed dispatching bundle.",
							NULL);
					return -1;
				}

				/*	As above, must write the bundle
				 *	to the SDR in order to destroy
				 *	it successfully.  We count the
				 *	bundle as "forwarded" because
				 *	bpAbandon will count it as
				 *	"forwarding failed".		*/

				bpDbTally(BP_DB_QUEUED_FOR_FWD,
						bundle->payload.length);
				sdr_write(bpSdr, bundleObj, (char *) bundle,
						sizeof(Bundle));
				return bpAbandon(bundleObj, bundle);
			}
		}
	}

	/*	There may be a non-local destination; let the
	 *	forwarder figure out what to do with the bundle.	*/

	if (bundle->fragmentElt || bundle->dlvQueueElt)
	{
		/*	Bundle has been delivered locally; the bundle
		 *	we forward must be a copy.			*/

		if (bpClone(bundle, &newBundle, &newBundleObj, 0, 0) < 0)
		{
			putErrmsg("Failed cloning bundle.", NULL);
			return -1;
		}

		bundle = &newBundle;
		bundleObj = newBundleObj;
	}

	if (printEid(&bundle->destination, dictionary, &eidString) < 0)
	{
		putErrmsg("Can't print destination EID.", NULL);
		releaseDictionary(dictionary);
		return -1;
	}

	if (patchExtensionBlocks(bundle) < 0)
	{
		putErrmsg("Can't insert missing extensions.", NULL);
		MRELEASE(eidString);
		releaseDictionary(dictionary);
		sdr_cancel_xn(bpSdr);
		return -1;
	}

	result = forwardBundle(bundleObj, bundle, eidString);
	MRELEASE(eidString);
	releaseDictionary(dictionary);
	if (result < 0)
	{
		putErrmsg("Can't enqueue bundle for forwarding.", NULL);
		return -1;
	}

	return 0;
}

/*	*	*	Bundle acquisition functions	*	*	*/

AcqWorkArea	*bpGetAcqArea(VInduct *vduct)
{
	int		memIdx = getIonMemoryMgr();
	AcqWorkArea	*work;
	int		i;

	work = (AcqWorkArea *) MTAKE(sizeof(AcqWorkArea));
	if (work)
	{
		work->collabBlocks = lyst_create_using(memIdx);
		if (work->collabBlocks == NULL)
		{
			MRELEASE(work);
			return NULL;
		}

		work->vduct = vduct;
		for (i = 0; i < 2; i++)
		{
			work->extBlocks[i] = lyst_create_using(memIdx);
			if (work->extBlocks[i] == NULL)
			{
				if (i == 1)
				{
					lyst_destroy(work->extBlocks[0]);
				}

				lyst_destroy(work->collabBlocks);
				MRELEASE(work);
				work = NULL;
				break;
			}
		}
	}

	return work;
}

static void	clearAcqArea(AcqWorkArea *work)
{
	int	i;
	LystElt	elt;

	/*	Destroy copy of dictionary.				*/

	if (work->dictionary)
	{
		MRELEASE(work->dictionary);
		work->dictionary = NULL;
	}

	/*	Destroy all extension blocks in the work area.		*/

	for (i = 0; i < 2; i++)
	{
		work->bundle.extensionsLength[i] = 0;
		while (1)
		{
			elt = lyst_first(work->extBlocks[i]);
			if (elt == NULL)
			{
				break;
			}

			deleteAcqExtBlock(elt, i);
		}
	}

        /* Destroy collaboration blocks */
        destroyAcqCollabBlocks(work);

	/*	Reset all other per-bundle parameters.			*/

	memset((char *) &(work->bundle), 0, sizeof(Bundle));
	work->authentic = 0;
	work->decision = AcqTBD;
	work->lastBlockParsed = 0;
	work->malformed = 0;
	work->congestive = 0;
	work->mustAbort = 0;
	work->headerLength = 0;
	work->trailerLength = 0;
	work->bundleLength = 0;
}

static int	eraseWorkZco(AcqWorkArea *work)
{
	Sdr	bpSdr;

	if (work->zco)
	{
		bpSdr = getIonsdr();
		CHKERR(sdr_begin_xn(bpSdr));
		sdr_list_delete(bpSdr, work->zcoElt, NULL, NULL);

		/*	Destroying the ZCO will cause the acquisition
		 *	FileRef to be deleted, which will unlink the
		 *	acquisition file when the FileRef's cleanup
		 *	script is executed.				*/

		zco_destroy(bpSdr, work->zco);
		if (sdr_end_xn(bpSdr) < 0)
		{
			putErrmsg("Can't clear inbound bundle ZCO.", NULL);
			return -1;
		}
	}

	work->allAuthentic = 0;
	if (work->senderEid)
	{
		MRELEASE(work->senderEid);
		work->senderEid = NULL;
	}

	work->acqFileRef = 0;
	work->zco = 0;
	work->zcoElt = 0;
	work->zcoBytesConsumed = 0;
	memset((char *) &(work->reader), 0, sizeof(ZcoReader));
	work->zcoBytesReceived = 0;
	work->bytesBuffered = 0;
	return 0;
}

void	bpReleaseAcqArea(AcqWorkArea *work)
{
	clearAcqArea(work);
	oK(eraseWorkZco(work));
	lyst_destroy(work->extBlocks[0]);
	lyst_destroy(work->extBlocks[1]);
	MRELEASE(work);
}

int	bpBeginAcq(AcqWorkArea *work, int authentic, char *senderEid)
{
	int	eidLen;

	CHKERR(work);

	/*	Re-initialize the per-bundle parameters.		*/

	clearAcqArea(work);

	/*	Load the per-acquisition parameters.			*/

	work->allAuthentic = authentic ? 1 : 0;
	if (work->senderEid)
	{
		MRELEASE(work->senderEid);
		work->senderEid = NULL;
	}

	if (senderEid)
	{
		eidLen = strlen(senderEid) + 1;
		work->senderEid = MTAKE(eidLen);
		if (work->senderEid == NULL)
		{
			putErrmsg("Can't copy sender EID.", NULL);
			return -1;
		}

		istrcpy(work->senderEid, senderEid, eidLen);
	}

	return 0;
}

int	bpLoadAcq(AcqWorkArea *work, Object zco)
{
	Sdr	bpSdr = getIonsdr();
	BpDB	*bpConstants = _bpConstants();

	if (work->zco)
	{
		putErrmsg("Can't replace ZCO in acq work area.",  NULL);
		return -1;
	}

	CHKERR(sdr_begin_xn(bpSdr));
	work->zcoElt = sdr_list_insert_last(bpSdr, bpConstants->inboundBundles,
			zco);
	if (sdr_end_xn(bpSdr) < 0)
	{
		putErrmsg("Can't note inbound bundle ZCO.", NULL);
		return -1;
	}

	work->zco = zco;
	return 0;
}

int	bpContinueAcq(AcqWorkArea *work, char *bytes, int length)
{
	static unsigned int	acqCount = 0;
	static int		maxAcqInHeap = 0;
	Sdr			sdr = getIonsdr();
	BpDB			*bpConstants = _bpConstants();
	BpDB			bpdb;
	Object			extentObj;
	char			cwd[200];
	char			fileName[SDRSTRING_BUFSZ];
	int			fd;
	int			fileLength;

	CHKERR(work);
	CHKERR(bytes);
	CHKERR(length >= 0);
	if (work->congestive)
	{
		return 0;	/*	No ZCO space; append no more.	*/
	}

	CHKERR(sdr_begin_xn(sdr));
	if (maxAcqInHeap == 0)
	{
		/*	Initialize threshold for acquiring bundle
		 *	into a file rather than directly into the
		 *	heap.  Minimum threshold is the amount of
		 *	heap space that would be occupied by a ZCO
		 *	file reference object anyway, even if the
		 *	bundle were entirely acquired into a file.	*/

		maxAcqInHeap = 560;
		sdr_read(sdr, (char *) &bpdb, getBpDbObject(), sizeof(BpDB));
		if (bpdb.maxAcqInHeap > maxAcqInHeap)
		{
			maxAcqInHeap = bpdb.maxAcqInHeap;
		}
	}

	if (work->zco == 0)	/*	First extent of acquisition.	*/
	{
		work->zco = zco_create(sdr, ZcoSdrSource, 0, 0, 0);
		switch (work->zco)
		{
		case (Object) ERROR:
			putErrmsg("Can't start inbound bundle ZCO.", NULL);
			sdr_cancel_xn(sdr);
			return -1;

		case 0:
			work->congestive = 1;
			return 0;	/*	Out of ZCO space.	*/
		}

		work->zcoElt = sdr_list_insert_last(sdr,
				bpConstants->inboundBundles, work->zco);
		if (work->zcoElt == 0)
		{
			putErrmsg("Can't start inbound bundle ZCO.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}
	}

	/*	Now add extent.  Acquire extents of bundle into
	 *	database heap up to the stated limit; after that,
	 *	acquire all remaining extents into a file.
	 *
	 *	Note that this procedure assumes that bundle extents
	 *	are acquired in increasing offset order, without gaps;
	 *	out-of-order extent acquisition would mandate an
	 *	extent reordering mechanism similar to the one that
	 *	is implemented in LTP.  This is not a problem in BP
	 *	because, for all convergence-layer protocols, either
	 *	the CL delivers a complete bundle (as in LTP and UDP)
	 *	or else the bundle increments are acquired in order
	 *	of increasing offset because the CL itself enforces
	 *	data ordering (as in TCP).				*/

	if ((length + zco_length(sdr, work->zco)) <= maxAcqInHeap)
	{
		extentObj = sdr_insert(sdr, bytes, length);
		if (extentObj)
		{
			switch (zco_append_extent(sdr, work->zco, ZcoSdrSource,
					extentObj, 0, length))
			{
			case ERROR:
				putErrmsg("Can't append heap extent.", NULL);
				sdr_cancel_xn(sdr);
				return -1;

			case 0:
				sdr_free(sdr, extentObj);
				work->congestive = 1;
			}
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't acquire extent into heap.", NULL);
			return -1;
		}

		return 0;
	}

	/*	This extent of this acquisition must be acquired into
	 *	a file.							*/

	if (work->acqFileRef == 0)	/*	First file extent.	*/
	{
		if (igetcwd(cwd, sizeof cwd) == NULL)
		{
			putErrmsg("Can't get CWD for acq file name.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}

		acqCount++;
		isprintf(fileName, sizeof fileName, "%s%cbpacq.%u", cwd,
				ION_PATH_DELIMITER, acqCount);
		fd = iopen(fileName, O_WRONLY | O_CREAT, 0666);
		if (fd < 0)
		{
			putSysErrmsg("Can't create acq file", fileName);
			sdr_cancel_xn(sdr);
			return -1;
		}

		fileLength = 0;
		work->acqFileRef = zco_create_file_ref(sdr, fileName, "");
		if (work->acqFileRef == 0)
		{
			putErrmsg("Can't create file ref.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}
	}
	else				/*	Writing more to file.	*/
	{
		oK(zco_file_ref_path(sdr, work->acqFileRef, fileName,
				sizeof fileName));
		fd = iopen(fileName, O_WRONLY, 0666);
		if (fd < 0 || (fileLength = lseek(fd, 0, SEEK_END)) < 0)
		{
			putSysErrmsg("Can't reopen acq file", fileName);
			sdr_cancel_xn(sdr);
			return -1;
		}
	}

	if (write(fd, bytes, length) < 0)
	{
		putSysErrmsg("Can't append to acq file", fileName);
		sdr_cancel_xn(sdr);
		return -1;
	}

	close(fd);
	switch (zco_append_extent(sdr, work->zco, ZcoFileSource,
				work->acqFileRef, fileLength, length))
	{
	case ERROR:
		putErrmsg("Can't append file reference extent.", NULL);
		sdr_cancel_xn(sdr);
		return -1;

	case 0:
		work->congestive = 1;
		break;

	default:
		/*	Flag file reference for deletion as soon as
		 *	the last ZCO extent that references it is
		 *	deleted.					*/

		zco_destroy_file_ref(sdr, work->acqFileRef);
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't acquire extent into file.", NULL);
		return -1;
	}

	return 0;
}

void	bpCancelAcq(AcqWorkArea *work)
{
	oK(eraseWorkZco(work));
}

int	guessBundleSize(Bundle *bundle)
{
	return (NOMINAL_PRIMARY_BLKSIZE
		+ bundle->dictionaryLength
		+ bundle->extensionsLength[PRE_PAYLOAD]
		+ bundle->payload.length
		+ bundle->extensionsLength[POST_PAYLOAD]);
}

int	computeECCC(int bundleSize, ClProtocol *protocol)
{
	int	framesNeeded;

	/*	Compute estimated consumption of contact capacity.	*/

	framesNeeded = bundleSize / protocol->payloadBytesPerFrame;
	framesNeeded += (bundleSize % protocol->payloadBytesPerFrame) ? 1 : 0;
	framesNeeded += (framesNeeded == 0) ? 1 : 0;
	return bundleSize + (protocol->overheadPerFrame * framesNeeded);
}

static int	applyRecvRateControl(AcqWorkArea *work)
{
	Sdr		bpSdr = getIonsdr();
	Bundle		*bundle = &(work->bundle);
			OBJ_POINTER(Induct, induct);
			OBJ_POINTER(ClProtocol, protocol);
	Throttle	*throttle;
	int		recvLength;

	CHKERR(sdr_begin_xn(bpSdr));	/*	Just to lock memory.	*/
	GET_OBJ_POINTER(bpSdr, Induct, induct, sdr_list_data(bpSdr,
			work->vduct->inductElt));
	throttle = &(work->vduct->acqThrottle);
	if (throttle->nominalRate < 0)	/*	No rate control.	*/
	{
		sdr_exit_xn(bpSdr);
		return 0;
	}

	GET_OBJ_POINTER(bpSdr, ClProtocol, protocol, induct->protocol);
	recvLength = computeECCC(bundle->payload.length
			+ NOMINAL_PRIMARY_BLKSIZE, protocol);
	while (throttle->capacity <= 0)
	{
		sdr_exit_xn(bpSdr);
		if (sm_SemTake(throttle->semaphore) < 0)
		{
			putErrmsg("CLI can't take throttle semaphore.", NULL);
			return -1;
		}

		if (sm_SemEnded(throttle->semaphore))
		{
			putErrmsg("Induct has been stopped.", NULL);
			return -1;
		}

		CHKERR(sdr_begin_xn(bpSdr));
	}

	throttle->capacity -= recvLength;
	if (throttle->capacity > 0)
	{
		sm_SemGive(throttle->semaphore);
	}

	sdr_exit_xn(bpSdr);		/*	Release memory.		*/
	return 0;
}

static int	advanceWorkBuffer(AcqWorkArea *work, int bytesParsed)
{
	int	bytesRemaining = work->zcoLength - work->zcoBytesReceived;
	int	bytesToReceive;
	int	bytesReceived;

	/*	Shift buffer left by number of bytes parsed.		*/

	work->bytesBuffered -= bytesParsed;
	memmove(work->buffer, work->buffer + bytesParsed, work->bytesBuffered);

	/*	Now read from ZCO to fill the buffer space that was
	 *	vacated.						*/

	bytesToReceive = sizeof work->buffer - work->bytesBuffered;
	if (bytesToReceive > bytesRemaining)
	{
		bytesToReceive = bytesRemaining;
	}

	if (bytesToReceive > 0)
	{
		bytesReceived = zco_receive_source(getIonsdr(), &(work->reader),
			bytesToReceive, work->buffer + work->bytesBuffered);
		CHKERR(bytesReceived == bytesToReceive);
		work->zcoBytesReceived += bytesReceived;
		work->bytesBuffered += bytesReceived;
	}

	return 0;
}

static char	*_versionMemo()
{
	static char	buf[80];
	static char	*memo = NULL;

	if (memo == NULL)
	{
		isprintf(buf, sizeof buf,
			"[?] Wrong bundle version (should be %d)", BP_VERSION);
		memo = buf;
	}

	return memo;
}

static int	acquirePrimaryBlock(AcqWorkArea *work)
{
	Bundle		*bundle;
	int		bytesToParse;
	int		unparsedBytes;
	unsigned char	*cursor;
	int		version;
	unsigned int	residualBlockLength;
	int		i;
	uvast		eidSdnvValues[8];
	char		*eidString;
	int		nullEidLen;
	int		bytesParsed;

	/*	Create the inbound bundle.				*/

	bundle = &(work->bundle);
	bundle->dbOverhead = BASE_BUNDLE_OVERHEAD;
	bundle->custodyTaken = 0;
	bytesToParse = work->bytesBuffered;
	unparsedBytes = bytesToParse;
	cursor = (unsigned char *) (work->buffer);

	/*	Start parsing the primary block.			*/

	if (unparsedBytes < MIN_PRIMARY_BLK_LENGTH)
	{
		writeMemoNote("[?] Not enough bytes for primary block",
				itoa(unparsedBytes));
		return 0;
	}

	version = *cursor;
	cursor++; unparsedBytes--;
	if (version != BP_VERSION)
	{
		writeMemoNote(_versionMemo(), itoa(version));
		return 0;
	}

	extractSmallSdnv(&(bundle->bundleProcFlags), &cursor, &unparsedBytes);

	/*	Note status report information as necessary.		*/

	if (SRR_FLAGS(bundle->bundleProcFlags) & BP_RECEIVED_RPT)
	{
		bundle->statusRpt.flags |= BP_RECEIVED_RPT;
		getCurrentDtnTime(&(bundle->statusRpt.receiptTime));
	}

	/*	Get length of rest of primary block.			*/

	extractSmallSdnv(&residualBlockLength, &cursor, &unparsedBytes);

	/*	Get all EID SDNV values.				*/

	for (i = 0; i < 8; i++)
	{
		extractSdnv(&(eidSdnvValues[i]), &cursor, &unparsedBytes);
	}

	/*	Get creation timestamp, lifetime, dictionary length.	*/

	extractSmallSdnv(&(bundle->id.creationTime.seconds), &cursor,
			&unparsedBytes);

	extractSmallSdnv(&(bundle->id.creationTime.count), &cursor,
			&unparsedBytes);

	extractSmallSdnv(&(bundle->timeToLive), &cursor, &unparsedBytes);
	if (ionClockIsSynchronized() && bundle->id.creationTime.seconds > 0)
	{
		/*	Default bundle age, pending override by BAE.	*/

		bundle->age = getUTCTime() - bundle->id.creationTime.seconds;
	}
	else
	{
		bundle->age = 0;
	}

	extractSmallSdnv(&(bundle->dictionaryLength), &cursor, &unparsedBytes);
	bundle->dbOverhead += bundle->dictionaryLength;

	/*	Get the dictionary, if present.				*/

	if (unparsedBytes < bundle->dictionaryLength)
	{
		writeMemo("[?] Primary block too large for buffer.");
		return 0;
	}

	if (bundle->dictionaryLength == 0)	/*	CBHE		*/
	{
		bundle->destination.cbhe = 1;
		if (bundle->bundleProcFlags & BDL_DEST_IS_SINGLETON)
		{
			bundle->destination.unicast = 1;
		}
		else
		{
			bundle->destination.unicast = 0;
		}

		bundle->destination.c.nodeNbr = eidSdnvValues[0];
		bundle->destination.c.serviceNbr = eidSdnvValues[1];
		bundle->id.source.cbhe = 1;
		bundle->id.source.unicast = 1;
		bundle->id.source.c.nodeNbr = eidSdnvValues[2];
		bundle->id.source.c.serviceNbr = eidSdnvValues[3];
		bundle->reportTo.cbhe = 1;
		bundle->reportTo.unicast = 1;
		bundle->reportTo.c.nodeNbr = eidSdnvValues[4];
		bundle->reportTo.c.serviceNbr = eidSdnvValues[5];
		bundle->custodian.cbhe = 1;
		bundle->custodian.unicast = 1;
		bundle->custodian.c.nodeNbr = eidSdnvValues[6];
		bundle->custodian.c.serviceNbr = eidSdnvValues[7];
	}
	else
	{
		work->dictionary = MTAKE(bundle->dictionaryLength);
		if (work->dictionary == NULL)
		{
			putErrmsg("No memory for dictionary.", NULL);
			return -1;
		}

		memcpy(work->dictionary, cursor, bundle->dictionaryLength);
		cursor += bundle->dictionaryLength;
		unparsedBytes -= bundle->dictionaryLength;
		bundle->destination.cbhe = 0;
		bundle->destination.unicast = 1;
		bundle->destination.d.schemeNameOffset = eidSdnvValues[0];
		bundle->destination.d.nssOffset = eidSdnvValues[1];
		bundle->id.source.cbhe = 0;
		bundle->id.source.unicast = 1;
		bundle->id.source.d.schemeNameOffset = eidSdnvValues[2];
		bundle->id.source.d.nssOffset = eidSdnvValues[3];
		bundle->reportTo.cbhe = 0;
		bundle->reportTo.unicast = 1;
		bundle->reportTo.d.schemeNameOffset = eidSdnvValues[4];
		bundle->reportTo.d.nssOffset = eidSdnvValues[5];
		bundle->custodian.cbhe = 0;
		bundle->custodian.unicast = 1;
		bundle->custodian.d.schemeNameOffset = eidSdnvValues[6];
		bundle->custodian.d.nssOffset = eidSdnvValues[7];
	}

	if (printEid(&(bundle->id.source), work->dictionary, &eidString) < 0)
	{
		putErrmsg("Can't print source EID string.", NULL);
		return 0;
	}

	nullEidLen = strlen(_nullEid());
	if (istrlen(eidString, nullEidLen + 1) == nullEidLen
	&& strcmp(eidString, _nullEid()) == 0)
	{
		bundle->anonymous = 1;
	}

	MRELEASE(eidString);
	if (bundle->bundleProcFlags & BDL_IS_FRAGMENT)
	{
		extractSmallSdnv(&(bundle->id.fragmentOffset), &cursor,
				&unparsedBytes);
		extractSmallSdnv(&(bundle->totalAduLength), &cursor,
				&unparsedBytes);
	}
	else
	{
		bundle->id.fragmentOffset = 0;
		bundle->totalAduLength = 0;
	}

	/*	Have got primary block; include its length in the
	 *	value of header length.					*/

	bytesParsed = bytesToParse - unparsedBytes;
	work->headerLength += bytesParsed;
	return bytesParsed;
}

static int	acquireBlock(AcqWorkArea *work)
{
	Bundle		*bundle = &(work->bundle);
	int		memIdx = getIonMemoryMgr();
	int		unparsedBytes = work->bytesBuffered;
	unsigned char	*cursor;
	unsigned char	*startOfBlock;
	unsigned char	blkType;
	unsigned int	blkProcFlags;
	Lyst		eidReferences = NULL;
	unsigned int	eidReferencesCount;
	unsigned int	schemeOffset;
	unsigned int	nssOffset;
	unsigned int	dataLength;
	unsigned int	lengthOfBlock;
	unsigned long	temp;
	ExtensionDef	*def;

	if (work->malformed || work->mustAbort || work->lastBlockParsed)
	{
		return 0;
	}

	if (unparsedBytes < 3)
	{
		return 0;	/*	Can't be a complete block.	*/
	}

	cursor = (unsigned char *) (work->buffer);
	startOfBlock = cursor;

	/*	Get block type.						*/

	blkType = *cursor;
	cursor++; unparsedBytes--;

	/*	Get block processing flags.  If flags indicate that
	 *	EID references are present, get them.			*/

	extractSmallSdnv(&blkProcFlags, &cursor, &unparsedBytes);
	if (blkProcFlags & BLK_IS_LAST)
	{
		work->lastBlockParsed = 1;
	}

	if (blkProcFlags & BLK_HAS_EID_REFERENCES)
	{
		eidReferences = lyst_create_using(memIdx);
		if (eidReferences == NULL)
		{
			return -1;
		}

		extractSmallSdnv(&eidReferencesCount, &cursor, &unparsedBytes);
		while (eidReferencesCount > 0)
		{
			extractSmallSdnv(&schemeOffset, &cursor,
					&unparsedBytes);
			temp = schemeOffset;
			if (lyst_insert_last(eidReferences, (void *) temp)
					== NULL)
			{
				return -1;
			}

			extractSmallSdnv(&nssOffset, &cursor, &unparsedBytes);
			temp = schemeOffset;
			if (lyst_insert_last(eidReferences, (void *) temp)
					== NULL)
			{
				return -1;
			}

			eidReferencesCount--;
		}
	}

	extractSmallSdnv(&dataLength, &cursor, &unparsedBytes);

	/*	Check first to see if this is the payload block.	*/

	if (blkType == 1)	/*	Payload block.			*/
	{
		if (bundle->payload.length)
		{
			writeMemo("[?] Multiple payloads in block.");
			return 0;
		}

		/*	Note: length of payload bytes currently in
		 *	the buffer is up to unparsedBytes, starting
		 *	at cursor.					*/

		if (eidReferences)	/*	Invalid, but possible.	*/
		{
			lyst_destroy(eidReferences);
			eidReferences = NULL;
		}

		bundle->payloadBlockProcFlags = blkProcFlags;
		bundle->payload.length = dataLength;
		return (cursor - startOfBlock);
	}

	/*	This is an extension block.  Cursor is pointing at
	 *	start of block data.					*/

	if (unparsedBytes < dataLength)	/*	Doesn't fit in buffer.	*/
	{
		writeMemoNote("[?] Extension block too long", utoa(dataLength));
		return 0;	/*	Block is too long for buffer.	*/
	}

	lengthOfBlock = (cursor - startOfBlock) + dataLength;
	def = findExtensionDef(blkType, work->currentExtBlocksList);
	if (def)
	{
		if (acquireExtensionBlock(work, def, startOfBlock,
				lengthOfBlock, blkType, blkProcFlags,
				&eidReferences, dataLength) < 0)
		{
			return -1;
		}
	}
	else	/*	An unrecognized extension.		*/
	{
		if (blkProcFlags & BLK_REPORT_IF_NG)
		{
			if (bundle->bundleProcFlags & BDL_IS_ADMIN)
			{
				/*	RFC 5050 4.3	*/

				work->mustAbort = 1;
			}
			else
			{
				bundle->statusRpt.flags |= BP_RECEIVED_RPT;
				bundle->statusRpt.reasonCode =
					SrBlockUnintelligible;
				getCurrentDtnTime(&bundle->
					statusRpt.receiptTime);
			}
		}

		if (bundle->payload.length != 0)
		{
			if ((blkProcFlags & BLK_MUST_BE_COPIED) == 0)
			{
				/*	RFC 5050 4.3	*/

				work->mustAbort = 1;
			}
		}

		if (blkProcFlags & BLK_ABORT_IF_NG)
		{
			work->mustAbort = 1;
		}
		else
		{
			if ((blkProcFlags & BLK_REMOVE_IF_NG) == 0)
			{
				blkProcFlags |= BLK_FORWARDED_OPAQUE;
				if (acquireExtensionBlock(work, def,
						startOfBlock, lengthOfBlock,
						blkType, blkProcFlags,
						&eidReferences, dataLength) < 0)
				{
					return -1;
				}
			}
		}
	}

	if (eidReferences)
	{
		lyst_destroy(eidReferences);
		eidReferences = NULL;
	}

	return lengthOfBlock;
}

static int	acqFromWork(AcqWorkArea *work)
{
	Sdr	sdr = getIonsdr();
	int	bytesParsed;
	int	unreceivedPayload;
	int	bytesRecd;

	/*	Acquire primary block.					*/

	bytesParsed = acquirePrimaryBlock(work);
	switch (bytesParsed)
	{
	case -1:				/*	System failure.	*/
		return -1;

	case 0:					/*	Parsing failed.	*/
		work->malformed = 1;
		return 0;

	default:
		break;
	}

	work->bundleLength += bytesParsed;
	CHKERR(advanceWorkBuffer(work, bytesParsed) == 0);

	/*	Aquire all pre-payload blocks following the primary
	 *	block, stopping after the block header for the payload
	 *	block itself.						*/

	work->currentExtBlocksList = PRE_PAYLOAD;
	while (1)
	{
		bytesParsed = acquireBlock(work);
		switch (bytesParsed)
		{
		case -1:			/*	System failure.	*/
			return -1;

		case 0:				/*	Parsing failed.	*/
			work->malformed = 1;
			return 0;

		default:			/*	Parsed block.	*/
			break;
		}

		work->headerLength += bytesParsed;
		work->bundleLength += bytesParsed;
		CHKERR(advanceWorkBuffer(work, bytesParsed) == 0);
		if (work->bundle.payload.length)
		{
			/*	Last parsed block was payload block,
			 *	of which only the header was parsed.	*/

			break;
		}
	}

	if (work->bundle.payload.length == 0)
	{
		/*	Bundle without any payload.			*/

		return 0;		/*	Nothing more to do.	*/
	}

	/*	Now acquire all payload bytes.  All payload bytes that
	 *	are currently buffered are cleared; if there are more
	 *	unreceived payload bytes beyond those, receive them
	 *	and reload buffer from post-payload bytes.		*/

	if (work->bundle.payload.length <= work->bytesBuffered)
	{
		/*	All bytes of payload are currently in the
		 *	work area's buffer.				*/

		work->bundleLength += work->bundle.payload.length;
		CHKERR(advanceWorkBuffer(work, work->bundle.payload.length)
				== 0);
	}
	else
	{
		/*	All bytes in the work area's buffer are
		 *	payload, and some number of additional bytes
		 *	not yet received are also in the payload.	*/

		unreceivedPayload = work->bundle.payload.length
				- work->bytesBuffered;
		bytesRecd = zco_receive_source(sdr, &(work->reader),
				unreceivedPayload, NULL);
		CHKERR(bytesRecd >= 0);
		if (bytesRecd != unreceivedPayload)
		{
			work->bundleLength += (work->bytesBuffered + bytesRecd);
			writeMemoNote("[?] Payload truncated",
					itoa(unreceivedPayload - bytesRecd));
			work->malformed = 1;
			return 0;
		}

		work->zcoBytesReceived += bytesRecd;
		work->bytesBuffered = 0;
		work->bundleLength += work->bundle.payload.length;
		CHKERR(advanceWorkBuffer(work, 0) == 0);
	}

	/*	Now acquire all post-payload blocks and exit.		*/

	work->currentExtBlocksList = POST_PAYLOAD;
	while (work->lastBlockParsed == 0)
	{
		bytesParsed = acquireBlock(work);
		switch (bytesParsed)
		{
		case -1:			/*	System failure.	*/
			return -1;

		case 0:				/*	Parsing failed.	*/
			work->malformed = 1;
			return 0;

		default:			/*	Parsed block.	*/
			break;
		}

		work->trailerLength += bytesParsed;
		work->bundleLength += bytesParsed;
		CHKERR(advanceWorkBuffer(work, bytesParsed) == 0);
	}

	return 0;
}

static int	abortBundleAcq(AcqWorkArea *work)
{
	Sdr	bpSdr = getIonsdr();

	if (work->bundle.payload.content)
	{
		CHKERR(sdr_begin_xn(bpSdr));
		zco_destroy(bpSdr, work->bundle.payload.content);
		if (sdr_end_xn(bpSdr) < 0)
		{
			putErrmsg("Can't destroy bundle ZCO.", NULL);
			return -1;
		}
	}

	return 0;
}

static int	discardReceivedBundle(AcqWorkArea *work, BpCtReason ctReason,
			BpSrReason srReason, char *dictionary)
{
	Bundle	*bundle = &(work->bundle);
	Sdr	bpSdr;

	/*	If we must discard the bundle, we send any reception
	 *	status report(s) previously noted and we discard the
	 *	bundle's payload content; if custody acceptance
	 *	was requested, we also send a "refusal" custody signal.
	 *	We may also send send a deletion status report if
	 *	requested.  Note that a request for custody acceptance
	 *	does NOT trigger transmission of a deletion status
	 *	report, because that report is only sent when a
	 *	custodian deletes a bundle for which it has taken
	 *	custody; custody of the bundle has not been accepted
	 *	in this case.
	 *
	 *	Note that negative status reporting is performed here
	 *	(in the CLI) before the bundle is enqueued for forwarding,
	 *	but that positive reporting is performed later by the
	 *	forwarder because it applies to bundles sourced locally
	 *	as well as to bundles accepted from other endpoints.	*/

	if (bundleIsCustodial(bundle))
	{
		bpSdr = getIonsdr();
		CHKERR(sdr_begin_xn(bpSdr));
		bpCtTally(ctReason, bundle->payload.length);
		if (sdr_end_xn(bpSdr) < 0
		|| noteCtEvent(bundle, work, work->dictionary, 0, ctReason) < 0)
		{
			putErrmsg("Can't send custody signal.", NULL);
			return -1;
		}

		if ((_bpvdb(NULL))->watching & WATCH_x)
		{
			putchar('x');
			fflush(stdout);
		}
	}

	if (srReason != 0
	&& (SRR_FLAGS(bundle->bundleProcFlags) & BP_DELETED_RPT))
	{
		bundle->statusRpt.flags |= BP_DELETED_RPT;
		bundle->statusRpt.reasonCode = srReason;
		getCurrentDtnTime(&bundle->statusRpt.deletionTime);
	}

	if (bundle->statusRpt.flags)
	{
		if (sendStatusRpt(bundle, dictionary) < 0)
		{
			putErrmsg("Can't send status report.", NULL);
			return -1;
		}
	}

	return abortBundleAcq(work);
}

static char	*getCustodialSchemeName(Bundle *bundle)
{
	/*	Note: the code for retrieving the custodial scheme
	 *	name for a bundle -- for which purpose we use the
	 *	bundle's destination EID scheme name -- will need
	 *	to be made more general in the event that we end up
	 *	supporting unicast schemes other than "ipn" and "dtn",
	 *	but for now it's the simplest, most efficient approach.	*/

	CHKNULL(bundle);
	if (bundle->destination.cbhe)
	{
		return "ipn";
	}

	return "dtn";
}

static void	initAuthenticity(AcqWorkArea *work)
{
	Sdr		bpSdr = getIonsdr();
	Object		secdbObj;
			OBJ_POINTER(SecDB, secdb);
	char		*custodialSchemeName;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	Object		ruleAddr;
	Object		elt;
			OBJ_POINTER(BspBabRule, rule);

	work->authentic = work->allAuthentic;
	if (work->authentic)		/*	Asserted by CL.		*/
	{
		return;
	}

	/*	Bundle is not yet considered authentic.			*/

	secdbObj = getSecDbObject();
	if (secdbObj == 0)
	{
		work->authentic = 1;	/*	No security, proceed.	*/
		return;
	}

	GET_OBJ_POINTER(bpSdr, SecDB, secdb, secdbObj);
	if (sdr_list_length(bpSdr, secdb->bspBabRules) == 0)
	{
		work->authentic = 1;	/*	No rules, proceed.	*/
		return;
	}

	/*	Make sure we've got a security source EID.		*/

	if (work->senderEid == NULL)	/*	Can't authenticate.	*/
	{
		return;			/*	So can't be authentic.	*/
	}

	/*	Figure out the security destination EID.		*/

	custodialSchemeName = getCustodialSchemeName(&work->bundle);
	findScheme(custodialSchemeName, &vscheme, &vschemeElt);
	if (vschemeElt == 0)
	{
		/*	Can't look for BAB rule, so can't authenticate.	*/

		return;			/*	So can't be authentic.	*/
	}

	/*	Check the applicable BAB rule, if any.			*/

	sec_findBspBabRule(work->senderEid, vscheme->adminEid, &ruleAddr, &elt);
	if (elt)
	{
		GET_OBJ_POINTER(bpSdr, BspBabRule, rule, ruleAddr);
		if (rule->ciphersuiteName[0] == '\0')
		{
			work->authentic = 1;	/*	Trusted node.	*/
		}
	}
}

static int	acquireBundle(Sdr bpSdr, AcqWorkArea *work, VEndpoint **vpoint)
{
	Bundle		*bundle = &(work->bundle);
	char		*custodialSchemeName;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	int		count;
	char		*eidString;
	Object		bundleAddr;
	MetaEid		senderMetaEid;
	Object		bundleObj;

	if (acqFromWork(work) < 0)
	{
		putErrmsg("Acquisition from work area failed.", NULL);
		return -1;
	}

	if (work->bundleLength > 0)
	{
		/*	Bundle has been parsed out of the work area's
		 *	ZCO.  Split it off into a separate ZCO.		*/

		bundle->payload.content = zco_clone(bpSdr, work->zco,
				work->zcoBytesConsumed, work->bundleLength);
		work->zcoBytesConsumed += work->bundleLength;
	}
	else
	{
		bundle->payload.content = 0;
	}

	if (bundle->payload.content == 0)
	{
		return 0;	/*	No bundle at front of work ZCO.	*/
	}

	/*	Check bundle for problems.				*/

	if (work->malformed || work->lastBlockParsed == 0)
	{
		writeMemo("[?] Malformed bundle.");
		bpInductTally(work->vduct, BP_INDUCT_MALFORMED,
				bundle->payload.length);
		return abortBundleAcq(work);
	}

	if (work->congestive)
	{
		writeMemo("[?] ZCO space is congested; discarding bundle.");
		bpInductTally(work->vduct, BP_INDUCT_CONGESTIVE,
				bundle->payload.length);
		return abortBundleAcq(work);
	}

	initAuthenticity(work);	/*	Set default.			*/
	if (checkExtensionBlocks(work) < 0)
	{
		putErrmsg("Can't check bundle authenticity.", NULL);
		sdr_cancel_xn(bpSdr);
		return -1;
	}

	if (bundle->clDossier.authentic == 0)
	{
		writeMemo("[?] Bundle judged inauthentic.");
		bpInductTally(work->vduct, BP_INDUCT_INAUTHENTIC,
				bundle->payload.length);
		return abortBundleAcq(work);
	}

	if (bundle->corrupt)
	{
		writeMemo("[?] Corrupt bundle.");
		bpInductTally(work->vduct, BP_INDUCT_MALFORMED,
				bundle->payload.length);
		return abortBundleAcq(work);
	}

	/*	Unintelligible extension headers don't make a bundle
	 *	malformed (though we count it that way), but they may
	 *	make it necessary to discard the bundle.		*/

	if (work->mustAbort)
	{
		bpInductTally(work->vduct, BP_INDUCT_MALFORMED,
				bundle->payload.length);
		return discardReceivedBundle(work, CtBlockUnintelligible,
				SrBlockUnintelligible, work->dictionary);
	}

	/*	Redundant reception of a custodial bundle after
	 *	we have already taken custody forces us to discard
	 *	this bundle and refuse custody, but we don't report
	 *	deletion of the bundle because it's not really deleted
	 *	-- we still have a local copy for which we have
	 *	accepted custody.					*/

	if (bundleIsCustodial(bundle))
	{
		if (printEid(&(bundle->custodian), work->dictionary,
				&eidString) < 0)
		{
			putErrmsg("Can't print custodian EID string.", NULL);
			sdr_cancel_xn(bpSdr);
			return -1;
		}

		custodialSchemeName = getCustodialSchemeName(bundle);
		findScheme(custodialSchemeName, &vscheme, &vschemeElt);
		if (vschemeElt != 0
		&& strcmp(eidString, vscheme->adminEid) == 0)
		{
			/*	Bundle's current custodian is self.	*/

			MRELEASE(eidString);
		}
		else
		{
			/*	NOT a case of the current custodian
			 *	receiving the bundle.			*/

			MRELEASE(eidString);
			if (printEid(&(bundle->id.source), work->dictionary,
					&eidString) < 0)
			{
				putErrmsg("Can't print source EID string.",
						NULL);
				sdr_cancel_xn(bpSdr);
				return -1;
			}

			count = findBundle(eidString,
				&(bundle->id.creationTime),
				bundle->id.fragmentOffset,
				(bundle->bundleProcFlags & BDL_IS_FRAGMENT ?
					bundle->payload.length : 0),
				&bundleAddr);
			MRELEASE(eidString);
			switch (count)
			{
			case -1:
				putErrmsg("Failed seeking bundle.", NULL);
				sdr_cancel_xn(bpSdr);
				return -1;

			case 0:		/*	No entry in table.	*/
				break;	/*	Bundle is new.		*/

			default:	/*	Entry found; redundant.	*/
				return discardReceivedBundle(work,
					CtRedundantReception, 0,
					work->dictionary);
			}
		}
	}

	/*	We have decided to accept the bundle rather than
	 *	simply discard it, so now we commit it to SDR space.
	 *	Re-do calculation of dbOverhead, since the extension
	 *	recording function may result in extension scratchpad
	 *	objects of different sizes than were calculated during
	 *	extension block acquisition.				*/

	bundle->dbOverhead = BASE_BUNDLE_OVERHEAD;

	/*	Reduce payload ZCO to just its source data, discarding
	 *	BP header and trailer.					*/

	zco_delimit_source(bpSdr, bundle->payload.content, work->headerLength,
			bundle->payload.length);
	zco_strip(bpSdr, bundle->payload.content);

	/*	Record bundle's sender EID, if known.			*/

	if (work->senderEid)
	{
		if (parseEidString(work->senderEid, &senderMetaEid, &vscheme,
				&vschemeElt) == 0)
		{
			restoreEidString(&senderMetaEid);
			writeMemoNote("[?] Sender EID malformed",
					work->senderEid);
			return abortBundleAcq(work);
		}

		bundle->clDossier.senderNodeNbr = senderMetaEid.nodeNbr;
		restoreEidString(&senderMetaEid);
		putBpString(&bundle->clDossier.senderEid, work->senderEid);
		bundle->dbOverhead += bundle->clDossier.senderEid.textLength;
	}

	bundle->stations = sdr_list_create(bpSdr);
	bundle->trackingElts = sdr_list_create(bpSdr);
	bundleObj = sdr_malloc(bpSdr, sizeof(Bundle));
	if (bundleObj == 0)
	{
		putErrmsg("No space for bundle object.", NULL);
		sdr_cancel_xn(bpSdr);
		return -1;
	}

	if (bundle->dictionaryLength > 0)
	{
		bundle->dictionary = sdr_malloc(bpSdr,
				bundle->dictionaryLength);
		if (bundle->dictionary == 0)
		{
			putErrmsg("Can't store dictionary.", NULL);
			sdr_cancel_xn(bpSdr);
			return -1;
		}

		sdr_write(bpSdr, bundle->dictionary, work->dictionary,
				bundle->dictionaryLength);
		bundle->dbOverhead += bundle->dictionaryLength;
	}

	computeExpirationTime(bundle);
	if (setBundleTTL(bundle, bundleObj) < 0)
	{
		putErrmsg("Can't insert new bundle into timeline.", NULL);
		sdr_cancel_xn(bpSdr);
		return -1;
	}

	if (catalogueBundle(bundle, bundleObj) < 0)
	{
		putErrmsg("Can't catalogue new bundle in hash table.", NULL);
		sdr_cancel_xn(bpSdr);
		return -1;
	}

	if (recordExtensionBlocks(work) < 0)
	{
		putErrmsg("Can't record extensions.", NULL);
		sdr_cancel_xn(bpSdr);
		return -1;
	}

	noteBundleInserted(bundle);
	bpInductTally(work->vduct, BP_INDUCT_RECEIVED, bundle->payload.length);
	bpRecvTally(COS_FLAGS(bundle->bundleProcFlags) & 0x03,
			bundle->payload.length);
	if ((_bpvdb(NULL))->watching & WATCH_y)
	{
		putchar('y');
		fflush(stdout);
	}

	/*	Other decisions and reporting are left to the
	 *	forwarder, as they depend on route availability.
	 *
	 *	Note that at this point we have NOT yet written
	 *	the bundle structure itself into the SDR space we
	 *	have allocated for it (bundleObj), because it may
	 *	still change in the course of dispatching.		*/

	if (dispatchBundle(bundleObj, bundle, vpoint) < 0)
	{
		putErrmsg("Can't dispatch bundle.", NULL);
		sdr_cancel_xn(bpSdr);
		return -1;
	}

	return 0;
}

static int	checkIncompleteBundle(Bundle *newFragment, VEndpoint *vpoint)
{
	Sdr		bpSdr = getIonsdr();
	Object		fragmentsList;
	Object		incElt;
	Object		incObj;
			OBJ_POINTER(IncompleteBundle, incomplete);
	Object		elt;
			OBJ_POINTER(Bundle, fragment);
	unsigned int	endOfFurthestFragment;
	unsigned int	endOfFragment;
	Bundle		aggregateBundle;
	Object		aggregateBundleObj;
	unsigned int	aggregateAduLength;
	Object		fragmentObj;
	Bundle		fragBuf;
	unsigned int	bytesToSkip;
	unsigned int	bytesToCopy;

	/*	Check to see if entire ADU has been received.		*/

	fragmentsList = sdr_list_list(bpSdr, newFragment->fragmentElt);
	incElt = sdr_list_user_data(bpSdr, fragmentsList);
	incObj = sdr_list_data(bpSdr, incElt);
	GET_OBJ_POINTER(bpSdr, IncompleteBundle, incomplete, incObj);
	endOfFurthestFragment = 0;
	for (elt = sdr_list_first(bpSdr, incomplete->fragments); elt;
			elt = sdr_list_next(bpSdr, elt))
	{
		GET_OBJ_POINTER(bpSdr, Bundle, fragment,
				sdr_list_data(bpSdr, elt));
		if (fragment->id.fragmentOffset > endOfFurthestFragment)
		{
			break;	/*	Found a gap.			*/
		}

		endOfFragment = fragment->id.fragmentOffset
				+ fragment->payload.length;
		if (endOfFragment > endOfFurthestFragment)
		{
			endOfFurthestFragment = endOfFragment;
		}
	}

	if (elt || endOfFurthestFragment < incomplete->totalAduLength)
	{
		return 0;	/*	Nothing more to do for now.	*/
	}

	/*	Have now received all bytes of the original ADU, so
	 *	reconstruct the original ADU as the payload of the
	 *	first fragment for the bundle.
	 *
	 *	First retrieve that first fragment and make it no
	 *	longer fragmentary.					*/

	elt = sdr_list_first(bpSdr, incomplete->fragments);
	aggregateBundleObj = sdr_list_data(bpSdr, elt);
	sdr_stage(bpSdr, (char *) &aggregateBundle, aggregateBundleObj,
			sizeof(Bundle));
	sdr_list_delete(bpSdr, aggregateBundle.fragmentElt, NULL, NULL);
	aggregateBundle.fragmentElt = 0;
	aggregateBundle.incompleteElt = 0;
	aggregateBundle.totalAduLength = 0;
	aggregateBundle.bundleProcFlags &= ~BDL_IS_FRAGMENT;

	/*	Back out of database occupancy this bundle's
	 *	original size, then change its size to reflect the
	 *	size of the reassembled ADU.				*/

	noteBundleRemoved(&aggregateBundle);
	aggregateAduLength = aggregateBundle.payload.length;
	aggregateBundle.payload.length = incomplete->totalAduLength;

	/*	Now collect payload data from all remaining fragments,
	 *	discarding overlaps, and destroy the fragments.		*/

	while (1)
	{
		elt = sdr_list_first(bpSdr, incomplete->fragments);
		if (elt == 0)
		{
			break;
		}

		fragmentObj = sdr_list_data(bpSdr, elt);
		sdr_stage(bpSdr, (char *) &fragBuf, fragmentObj,
				sizeof(Bundle));
		bytesToSkip = aggregateAduLength - fragBuf.id.fragmentOffset;
		if (bytesToSkip < fragBuf.payload.length)
		{
			bytesToCopy = fragBuf.payload.length - bytesToSkip;
			if (zco_append_extent(bpSdr,
					aggregateBundle.payload.content,
					ZcoZcoSource, fragBuf.payload.content,
					bytesToSkip, bytesToCopy) < 0)
			{
				putErrmsg("Can't append extent.", NULL);
				return -1;
			}

			aggregateAduLength += bytesToCopy;
		}

		sdr_list_delete(bpSdr, fragBuf.fragmentElt, NULL, NULL);
		fragBuf.fragmentElt = 0;
		sdr_write(bpSdr, fragmentObj, (char *) &fragBuf,
				sizeof(Bundle));
		if (bpDestroyBundle(fragmentObj, 0) < 0)
		{
			putErrmsg("Can't destroy fragment.", NULL);
			return -1;
		}
	}

	/*	Note effect on database occupancy of bundle's
	 *	revised size.						*/

	noteBundleInserted(&aggregateBundle);

	/*	Deliver the aggregate bundle to the endpoint.		*/

	if (enqueueForDelivery(aggregateBundleObj, &aggregateBundle, vpoint))
	{
		putErrmsg("Reassembled bundle delivery failed.", NULL);
		return -1;
	}

	/*	Finally, destroy the IncompleteBundle and return.	*/

	if (destroyIncomplete(incomplete, incElt) < 0)
	{
		putErrmsg("Can't destroy incomplete bundle.", NULL);
		return -1;
	}

	return 0;
}

int	bpEndAcq(AcqWorkArea *work)
{
	Sdr		bpSdr = getIonsdr();
	int		result;
	VEndpoint	*vpoint;
	int		acqLength;

	CHKERR(work);
	CHKERR(work->zco);
	CHKERR(sdr_begin_xn(bpSdr));
	work->zcoLength = zco_length(bpSdr, work->zco);
	zco_start_receiving(work->zco, &(work->reader));
	result = advanceWorkBuffer(work, 0);
	if (sdr_end_xn(bpSdr) < 0 || result < 0)
	{
		putErrmsg("Acq buffer initialization failed.", NULL);
		return -1;
	}

	/*	Acquire bundles from acquisition ZCO.			*/

	acqLength = work->zcoLength;
	while (acqLength > 0)
	{
		/*	Acquire next bundle in acquisition ZCO.		*/

		vpoint = NULL;
		CHKERR(sdr_begin_xn(bpSdr));
		result = acquireBundle(bpSdr, work, &vpoint);
		if (sdr_end_xn(bpSdr) < 0 || result < 0)
		{
			putErrmsg("Bundle acquisition failed.", NULL);
			return -1;
		}

		/*	Has acquisition of this bundle enabled
		 *	reassembly of an original bundle?		*/

		if (vpoint != NULL && work->bundle.fragmentElt != 0)
		{
			CHKERR(sdr_begin_xn(bpSdr));
			result = checkIncompleteBundle(&work->bundle, vpoint);
			if (sdr_end_xn(bpSdr) < 0 || result < 0)
			{
				putErrmsg("Bundle acquisition failed.", NULL);
				return -1;
			}
		}

		/*	Now apply reception rate control: delay
	 	*	acquisition of the next bundle until we
		*	have consumed as much time in receiving and
		*	acquiring this one as we had said we would.	*/

		if (applyRecvRateControl(work) < 0)
		{
			putErrmsg("Can't apply reception rate control.", NULL);
			return -1;
		}

		/*	Finally, prepare to acquire next bundle.	*/

		if (work->bundleLength == 0)
		{
			/*	No bundle at front of the acquisition
			 *	ZCO, so can't do any more acquisition.	*/

			acqLength = 0;	/*	Terminate loop.		*/
		}
		else
		{
			acqLength -= work->bundleLength;
		}

		clearAcqArea(work);
	}

	return eraseWorkZco(work);
}

/*	*	*	Administrative payload functions	*	*/

static int	constructCtSignal(BpCtSignal *csig, Object *zco)
{
	Sdr		bpSdr = getIonsdr();
	unsigned char	adminRecordFlag = (BP_CUSTODY_SIGNAL << 4);
	Sdnv		fragmentOffsetSdnv;
	Sdnv		fragmentLengthSdnv;
	Sdnv		signalTimeSecondsSdnv;
	Sdnv		signalTimeNanosecSdnv;
	Sdnv		creationTimeSecondsSdnv;
	Sdnv		creationTimeCountSdnv;
	int		eidLength;
	Sdnv		eidLengthSdnv;
	int		ctSignalLength;
	char		*buffer;
	char		*cursor;
	Object		sourceData;

	CHKERR(csig);
	CHKERR(zco);
	if (csig->isFragment)
	{
		adminRecordFlag |= BP_BDL_IS_A_FRAGMENT;
		encodeSdnv(&fragmentOffsetSdnv, csig->fragmentOffset);
		encodeSdnv(&fragmentLengthSdnv, csig->fragmentLength);
	}
	else
	{
		fragmentOffsetSdnv.length = 0;
		fragmentLengthSdnv.length = 0;
	}

	encodeSdnv(&signalTimeSecondsSdnv, csig->signalTime.seconds);
	encodeSdnv(&signalTimeNanosecSdnv, csig->signalTime.nanosec);
	encodeSdnv(&creationTimeSecondsSdnv, csig->creationTime.seconds);
	encodeSdnv(&creationTimeCountSdnv, csig->creationTime.count);
	eidLength = strlen(csig->sourceEid);
	encodeSdnv(&eidLengthSdnv, eidLength);

	ctSignalLength = 2 + fragmentOffsetSdnv.length +
			fragmentLengthSdnv.length +
			signalTimeSecondsSdnv.length +
			signalTimeNanosecSdnv.length +
			creationTimeSecondsSdnv.length +
			creationTimeCountSdnv.length +
			eidLengthSdnv.length +
			eidLength;
	buffer = MTAKE(ctSignalLength);
	if (buffer == NULL)
	{
		putErrmsg("Can't construct CT signal.", NULL);
		return -1;
	}

	cursor = buffer;

	*cursor = (char) adminRecordFlag;
	cursor++;

	*cursor = csig->reasonCode | (csig->succeeded << 7);
	cursor++;

	memcpy(cursor, fragmentOffsetSdnv.text, fragmentOffsetSdnv.length);
	cursor += fragmentOffsetSdnv.length;

	memcpy(cursor, fragmentLengthSdnv.text, fragmentLengthSdnv.length);
	cursor += fragmentLengthSdnv.length;

	memcpy(cursor, signalTimeSecondsSdnv.text,
			signalTimeSecondsSdnv.length);
	cursor += signalTimeSecondsSdnv.length;

	memcpy(cursor, signalTimeNanosecSdnv.text,
			signalTimeNanosecSdnv.length);
	cursor += signalTimeNanosecSdnv.length;

	memcpy(cursor, creationTimeSecondsSdnv.text,
			creationTimeSecondsSdnv.length);
	cursor += creationTimeSecondsSdnv.length;

	memcpy(cursor, creationTimeCountSdnv.text,
			creationTimeCountSdnv.length);
	cursor += creationTimeCountSdnv.length;

	memcpy(cursor, eidLengthSdnv.text, eidLengthSdnv.length);
	cursor += eidLengthSdnv.length;

	memcpy(cursor, csig->sourceEid, eidLength);
	cursor += eidLength;

	CHKERR(sdr_begin_xn(bpSdr));
	sourceData = sdr_malloc(bpSdr, ctSignalLength);
	if (sourceData == 0)
	{
		putErrmsg("No space for source data.", NULL);
		sdr_cancel_xn(bpSdr);
		MRELEASE(buffer);
		return -1;
	}

	sdr_write(bpSdr, sourceData, buffer, ctSignalLength);
	MRELEASE(buffer);
	*zco = zco_create(bpSdr, ZcoSdrSource, sourceData, 0, ctSignalLength);
	if (sdr_end_xn(bpSdr) < 0 || *zco == (Object) ERROR || *zco == 0)
	{
		putErrmsg("Can't create CT signal.", NULL);
		return -1;
	}

	return 0;
}

static int	parseCtSignal(BpCtSignal *csig, unsigned char *cursor,
			int unparsedBytes, int isFragment)
{
	unsigned char	head1;
	unsigned int	eidLength;

	memset((char *) csig, 0, sizeof(BpCtSignal));
	csig->isFragment = isFragment;
	if (unparsedBytes < 1)
	{
		writeMemoNote("[?] CT signal too short to parse",
				itoa(unparsedBytes));
		return 0;
	}

	head1 = *cursor;
	cursor++;
	unparsedBytes -= 1;
	csig->succeeded = ((head1 & 0x80) > 0);
	csig->reasonCode = head1 & 0x7f;

	if (isFragment)
	{
		extractSmallSdnv(&(csig->fragmentOffset), &cursor,
				&unparsedBytes);
		extractSmallSdnv(&(csig->fragmentLength), &cursor,
				&unparsedBytes);
	}

	extractSmallSdnv(&(csig->signalTime.seconds), &cursor, &unparsedBytes);
	extractSmallSdnv(&(csig->signalTime.nanosec), &cursor, &unparsedBytes);
	extractSmallSdnv(&(csig->creationTime.seconds), &cursor,
			&unparsedBytes);
	extractSmallSdnv(&(csig->creationTime.count), &cursor, &unparsedBytes);
	extractSmallSdnv(&eidLength, &cursor, &unparsedBytes);
	if (unparsedBytes != eidLength)
	{
		writeMemoNote("[?] CT signal EID bytes missing...",
				itoa(eidLength - unparsedBytes));
		return 0;
	}

	csig->sourceEid = MTAKE(eidLength + 1);
	if (csig->sourceEid == NULL)
	{
		putErrmsg("Can't acquire CT signal source EID.", NULL);
		return -1;
	}

	memcpy(csig->sourceEid, cursor, eidLength);
	csig->sourceEid[eidLength] = '\0';
	return 1;
}

static void	bpEraseCtSignal(BpCtSignal *csig)
{
	MRELEASE(csig->sourceEid);
}

static int	constructStatusRpt(BpStatusRpt *rpt, Object *zco)
{
	Sdr		bpSdr = getIonsdr();
	unsigned char	adminRecordFlag = (BP_STATUS_REPORT << 4);
	Sdnv		fragmentOffsetSdnv;
	Sdnv		fragmentLengthSdnv;
	Sdnv		receiptTimeSecondsSdnv;
	Sdnv		receiptTimeNanosecSdnv;
	Sdnv		custodyTimeSecondsSdnv;
	Sdnv		custodyTimeNanosecSdnv;
	Sdnv		forwardTimeSecondsSdnv;
	Sdnv		forwardTimeNanosecSdnv;
	Sdnv		deliveryTimeSecondsSdnv;
	Sdnv		deliveryTimeNanosecSdnv;
	Sdnv		deletionTimeSecondsSdnv;
	Sdnv		deletionTimeNanosecSdnv;
	Sdnv		creationTimeSecondsSdnv;
	Sdnv		creationTimeCountSdnv;
	int		eidLength;
	Sdnv		eidLengthSdnv;
	int		rptLength;
	char		*buffer;
	char		*cursor;
	Object		sourceData;

	CHKERR(rpt);
	CHKERR(zco);
	if (rpt->isFragment)
	{
		adminRecordFlag |= BP_BDL_IS_A_FRAGMENT;
		encodeSdnv(&fragmentOffsetSdnv, rpt->fragmentOffset);
		encodeSdnv(&fragmentLengthSdnv, rpt->fragmentLength);
	}
	else
	{
		fragmentOffsetSdnv.length = 0;
		fragmentLengthSdnv.length = 0;
	}

	if (rpt->flags & BP_RECEIVED_RPT)
	{
		encodeSdnv(&receiptTimeSecondsSdnv, rpt->receiptTime.seconds);
		encodeSdnv(&receiptTimeNanosecSdnv, rpt->receiptTime.nanosec);
	}
	else
	{
		receiptTimeSecondsSdnv.length = 0;
		receiptTimeNanosecSdnv.length = 0;
	}

	if (rpt->flags & BP_CUSTODY_RPT)
	{
		encodeSdnv(&custodyTimeSecondsSdnv,rpt->acceptanceTime.seconds);
		encodeSdnv(&custodyTimeNanosecSdnv,rpt->acceptanceTime.nanosec);
	}
	else
	{
		custodyTimeSecondsSdnv.length = 0;
		custodyTimeNanosecSdnv.length = 0;
	}

	if (rpt->flags & BP_FORWARDED_RPT)
	{
		encodeSdnv(&forwardTimeSecondsSdnv, rpt->forwardTime.seconds);
		encodeSdnv(&forwardTimeNanosecSdnv, rpt->forwardTime.nanosec);
	}
	else
	{
		forwardTimeSecondsSdnv.length = 0;
		forwardTimeNanosecSdnv.length = 0;
	}

	if (rpt->flags & BP_DELIVERED_RPT)
	{
		encodeSdnv(&deliveryTimeSecondsSdnv, rpt->deliveryTime.seconds);
		encodeSdnv(&deliveryTimeNanosecSdnv, rpt->deliveryTime.nanosec);
	}
	else
	{
		deliveryTimeSecondsSdnv.length = 0;
		deliveryTimeNanosecSdnv.length = 0;
	}

	if (rpt->flags & BP_DELETED_RPT)
	{
		encodeSdnv(&deletionTimeSecondsSdnv, rpt->deletionTime.seconds);
		encodeSdnv(&deletionTimeNanosecSdnv, rpt->deletionTime.nanosec);
	}
	else
	{
		deletionTimeSecondsSdnv.length = 0;
		deletionTimeNanosecSdnv.length = 0;
	}

	encodeSdnv(&creationTimeSecondsSdnv, rpt->creationTime.seconds);
	encodeSdnv(&creationTimeCountSdnv, rpt->creationTime.count);
	eidLength = strlen(rpt->sourceEid);
	encodeSdnv(&eidLengthSdnv, eidLength);

	rptLength = 3 + fragmentOffsetSdnv.length +
			fragmentLengthSdnv.length +
			receiptTimeSecondsSdnv.length +
			receiptTimeNanosecSdnv.length +
			custodyTimeSecondsSdnv.length +
			custodyTimeNanosecSdnv.length +
			forwardTimeSecondsSdnv.length +
			forwardTimeNanosecSdnv.length +
			deliveryTimeSecondsSdnv.length +
			deliveryTimeNanosecSdnv.length +
			deletionTimeSecondsSdnv.length +
			deletionTimeNanosecSdnv.length +
			creationTimeSecondsSdnv.length +
			creationTimeCountSdnv.length +
			eidLengthSdnv.length +
			eidLength;
	buffer = MTAKE(rptLength);
	if (buffer == NULL)
	{
		putErrmsg("Can't construct status report.", NULL);
		return -1;
	}

	cursor = buffer;

	*cursor = (char) adminRecordFlag;
	cursor++;

	*cursor = rpt->flags;
	cursor++;

	*cursor = rpt->reasonCode;
	cursor++;

	memcpy(cursor, fragmentOffsetSdnv.text, fragmentOffsetSdnv.length);
	cursor += fragmentOffsetSdnv.length;

	memcpy(cursor, fragmentLengthSdnv.text, fragmentLengthSdnv.length);
	cursor += fragmentLengthSdnv.length;

	memcpy(cursor, receiptTimeSecondsSdnv.text,
			receiptTimeSecondsSdnv.length);
	cursor += receiptTimeSecondsSdnv.length;

	memcpy(cursor, receiptTimeNanosecSdnv.text,
			receiptTimeNanosecSdnv.length);
	cursor += receiptTimeNanosecSdnv.length;

	memcpy(cursor, custodyTimeSecondsSdnv.text,
			custodyTimeSecondsSdnv.length);
	cursor += custodyTimeSecondsSdnv.length;

	memcpy(cursor, custodyTimeNanosecSdnv.text,
			custodyTimeNanosecSdnv.length);
	cursor += custodyTimeNanosecSdnv.length;

	memcpy(cursor, forwardTimeSecondsSdnv.text,
			forwardTimeSecondsSdnv.length);
	cursor += forwardTimeSecondsSdnv.length;

	memcpy(cursor, forwardTimeNanosecSdnv.text,
			forwardTimeNanosecSdnv.length);
	cursor += forwardTimeNanosecSdnv.length;

	memcpy(cursor, deliveryTimeSecondsSdnv.text,
			deliveryTimeSecondsSdnv.length);
	cursor += deliveryTimeSecondsSdnv.length;

	memcpy(cursor, deliveryTimeNanosecSdnv.text,
			deliveryTimeNanosecSdnv.length);
	cursor += deliveryTimeNanosecSdnv.length;

	memcpy(cursor, deletionTimeSecondsSdnv.text,
			deletionTimeSecondsSdnv.length);
	cursor += deletionTimeSecondsSdnv.length;

	memcpy(cursor, deletionTimeNanosecSdnv.text,
			deletionTimeNanosecSdnv.length);
	cursor += deletionTimeNanosecSdnv.length;

	memcpy(cursor, creationTimeSecondsSdnv.text,
			creationTimeSecondsSdnv.length);
	cursor += creationTimeSecondsSdnv.length;

	memcpy(cursor, creationTimeCountSdnv.text,
			creationTimeCountSdnv.length);
	cursor += creationTimeCountSdnv.length;

	memcpy(cursor, eidLengthSdnv.text, eidLengthSdnv.length);
	cursor += eidLengthSdnv.length;

	memcpy(cursor, rpt->sourceEid, eidLength);
	cursor += eidLength;

	CHKERR(sdr_begin_xn(bpSdr));
	sourceData = sdr_malloc(bpSdr, rptLength);
	if (sourceData == 0)
	{
		putErrmsg("No space for source data.", NULL);
		sdr_cancel_xn(bpSdr);
		MRELEASE(buffer);
		return -1;
	}

	sdr_write(bpSdr, sourceData, buffer, rptLength);
	MRELEASE(buffer);
	*zco = zco_create(bpSdr, ZcoSdrSource, sourceData, 0, rptLength);
	if (sdr_end_xn(bpSdr) < 0 || *zco == (Object) ERROR || *zco == 0)
	{
		putErrmsg("Can't create status report.", NULL);
		return -1;
	}

	return 0;
}

static int	parseStatusRpt(BpStatusRpt *rpt, unsigned char *cursor,
	       		int unparsedBytes, int isFragment)
{
	unsigned int	eidLength;

	memset((char *) rpt, 0, sizeof(BpStatusRpt));
	rpt->isFragment = isFragment;
	if (unparsedBytes < 1)
	{
		writeMemoNote("[?] Status report too short to parse",
				itoa(unparsedBytes));
		return 0;
	}

	rpt->flags = *cursor;
	cursor++;
	rpt->reasonCode = *cursor;
	cursor++;
	unparsedBytes -= 2;

	if (isFragment)
	{
		extractSmallSdnv(&(rpt->fragmentOffset), &cursor,
				&unparsedBytes);
		extractSmallSdnv(&(rpt->fragmentLength), &cursor,
				&unparsedBytes);
	}

	if (rpt->flags & BP_RECEIVED_RPT)
	{
		extractSmallSdnv(&(rpt->receiptTime.seconds), &cursor,
				&unparsedBytes);
		extractSmallSdnv(&(rpt->receiptTime.nanosec), &cursor,
				&unparsedBytes);
	}

	if (rpt->flags & BP_CUSTODY_RPT)
	{
		extractSmallSdnv(&(rpt->acceptanceTime.seconds), &cursor,
				&unparsedBytes);
		extractSmallSdnv(&(rpt->acceptanceTime.nanosec), &cursor,
				&unparsedBytes);
	}

	if (rpt->flags & BP_FORWARDED_RPT)
	{
		extractSmallSdnv(&(rpt->forwardTime.seconds), &cursor,
				&unparsedBytes);
		extractSmallSdnv(&(rpt->forwardTime.nanosec), &cursor,
				&unparsedBytes);
	}

	if (rpt->flags & BP_DELIVERED_RPT)
	{
		extractSmallSdnv(&(rpt->deliveryTime.seconds), &cursor,
				&unparsedBytes);
		extractSmallSdnv(&(rpt->deliveryTime.nanosec), &cursor,
				&unparsedBytes);
	}

	if (rpt->flags & BP_DELETED_RPT)
	{
		extractSmallSdnv(&(rpt->deletionTime.seconds), &cursor,
				&unparsedBytes);
		extractSmallSdnv(&(rpt->deletionTime.nanosec), &cursor,
				&unparsedBytes);
	}

	extractSmallSdnv(&(rpt->creationTime.seconds), &cursor, &unparsedBytes);
	extractSmallSdnv(&(rpt->creationTime.count), &cursor, &unparsedBytes);
	extractSmallSdnv(&eidLength, &cursor, &unparsedBytes);
	if (unparsedBytes != eidLength)
	{
		writeMemoNote("[?] Status report EID bytes missing...",
				itoa(eidLength - unparsedBytes));
		return 0;
	}

	rpt->sourceEid = MTAKE(eidLength + 1);
	if (rpt->sourceEid == NULL)
	{
		putErrmsg("Can't read status report source EID.", NULL);
		return -1;
	}

	memcpy(rpt->sourceEid, cursor, eidLength);
	rpt->sourceEid[eidLength] = '\0';
	return 1;
}

static void	bpEraseStatusRpt(BpStatusRpt *rpt)
{
	MRELEASE(rpt->sourceEid);
}

static int	parseAdminRecord(int *adminRecordType, BpStatusRpt *rpt,
			BpCtSignal *csig, void **otherPtr, Object payload)
{
	Sdr		bpSdr = getIonsdr();
	unsigned int	buflen;
	char		*buffer;
	ZcoReader	reader;
	char		*cursor;
	int		bytesToParse;
	int		unparsedBytes;
	int		bundleIsFragment;
	int		result;

	CHKERR(adminRecordType && rpt && csig && payload);
	CHKERR(sdr_begin_xn(bpSdr));
	buflen = zco_source_data_length(bpSdr, payload);
	if ((buffer = MTAKE(buflen)) == NULL)
	{
		putErrmsg("Can't start parsing admin record.", NULL);
		oK(sdr_end_xn(bpSdr));
		return -1;
	}

	zco_start_receiving(payload, &reader);
	bytesToParse = zco_receive_source(bpSdr, &reader, buflen, buffer);
	if (bytesToParse < 0)
	{
		putErrmsg("Can't receive admin record.", NULL);
		oK(sdr_end_xn(bpSdr));
		MRELEASE(buffer);
		return -1;
	}

	cursor = buffer;
	unparsedBytes = bytesToParse;
	if (unparsedBytes < 1)
	{
		writeMemoNote("[?] Incoming admin record too short",
				itoa(unparsedBytes));
		oK(sdr_end_xn(bpSdr));
		MRELEASE(buffer);
		return 0;
	}

	*adminRecordType = (*cursor >> 4 ) & 0x0f;
	bundleIsFragment = *cursor & 0x01;
	cursor++;
	unparsedBytes--;
	switch (*adminRecordType)
	{
	case BP_STATUS_REPORT:
		result = parseStatusRpt(rpt, (unsigned char *) cursor,
				unparsedBytes, bundleIsFragment);
		break;

	case BP_CUSTODY_SIGNAL:
		result = parseCtSignal(csig, (unsigned char *) cursor,
				unparsedBytes, bundleIsFragment);
		break;

	default:	/*	Unknown or non-standard admin record.	*/
		result = parseACS(*adminRecordType, otherPtr,
				(unsigned char *) cursor, unparsedBytes,
				bundleIsFragment);
		if (result != -2)	/*	Parsed the record.	*/
		{
			break;
		}

		result = parseImcPetition(*adminRecordType, otherPtr,
				(unsigned char *) cursor, unparsedBytes);
		if (result != -2)	/*	Parsed the record.	*/
		{
			break;
		}

		/*	Unknown admin record type.			*/

		writeMemoNote("[?] Unknown admin record type",
				itoa(*adminRecordType));
		result = 0;
	}

	oK(sdr_end_xn(bpSdr));
	MRELEASE(buffer);
	return result;
}

/*	*	*	Bundle catenation functions	*	*	*/

static int	catenateBundle(Bundle *bundle)
{
	Sdr		bpSdr = getIonsdr();
	Sdnv		bundleProcFlagsSdnv;
	int		residualBlkLength;
	Sdnv		residualBlkLengthSdnv;
	Sdnv		eidSdnvs[8];
	int		totalLengthOfEidSdnvs;
	Sdnv		creationTimestampTimeSdnv;
	Sdnv		creationTimestampCountSdnv;
	Sdnv		lifetimeSdnv;
	Sdnv		dictionaryLengthSdnv;
	Sdnv		fragmentOffsetSdnv;
	Sdnv		totalAduLengthSdnv;
	Sdnv		blkProcFlagsSdnv;
	Sdnv		payloadLengthSdnv;
	int		totalHeaderLength;
	int		totalTrailerLength;
	unsigned char	*buffer;
	unsigned char	*cursor;
	int		i;
	Object		elt;
	Object		nextElt;
	Object		blkAddr;
	ExtensionBlock	blk;
	unsigned char	*flagbyte;

	CHKZERO(ionLocked());

	/*	We assume that the bundle to be issued is valid:
	 *	either it was sourced locally (in which case we
	 *	created it ourselves, so it should be valid) or
	 *	else it was received from elsewhere (in which case
	 *	it was created by the acquisition functions, which
	 *	would have discarded the inbound bundle if it were
	 *	not well-formed).					*/

	encodeSdnv(&bundleProcFlagsSdnv, bundle->bundleProcFlags);
	totalLengthOfEidSdnvs = 0;
	if (bundle->dictionaryLength == 0)
	{
		encodeSdnv(&(eidSdnvs[0]), bundle->destination.c.nodeNbr);
		totalLengthOfEidSdnvs += eidSdnvs[0].length;
		encodeSdnv(&(eidSdnvs[1]), bundle->destination.c.serviceNbr);
		totalLengthOfEidSdnvs += eidSdnvs[1].length;
		encodeSdnv(&(eidSdnvs[2]), bundle->id.source.c.nodeNbr);
		totalLengthOfEidSdnvs += eidSdnvs[2].length;
		encodeSdnv(&(eidSdnvs[3]), bundle->id.source.c.serviceNbr);
		totalLengthOfEidSdnvs += eidSdnvs[3].length;
		encodeSdnv(&(eidSdnvs[4]), bundle->reportTo.c.nodeNbr);
		totalLengthOfEidSdnvs += eidSdnvs[4].length;
		encodeSdnv(&(eidSdnvs[5]), bundle->reportTo.c.serviceNbr);
		totalLengthOfEidSdnvs += eidSdnvs[5].length;
		encodeSdnv(&(eidSdnvs[6]), bundle->custodian.c.nodeNbr);
		totalLengthOfEidSdnvs += eidSdnvs[6].length;
		encodeSdnv(&(eidSdnvs[7]), bundle->custodian.c.serviceNbr);
		totalLengthOfEidSdnvs += eidSdnvs[7].length;
	}
	else
	{
		encodeSdnv(&(eidSdnvs[0]),
				bundle->destination.d.schemeNameOffset);
		totalLengthOfEidSdnvs += eidSdnvs[0].length;
		encodeSdnv(&(eidSdnvs[1]),
				bundle->destination.d.nssOffset);
		totalLengthOfEidSdnvs += eidSdnvs[1].length;
		encodeSdnv(&(eidSdnvs[2]),
				bundle->id.source.d.schemeNameOffset);
		totalLengthOfEidSdnvs += eidSdnvs[2].length;
		encodeSdnv(&(eidSdnvs[3]),
				bundle->id.source.d.nssOffset);
		totalLengthOfEidSdnvs += eidSdnvs[3].length;
		encodeSdnv(&(eidSdnvs[4]),
				bundle->reportTo.d.schemeNameOffset);
		totalLengthOfEidSdnvs += eidSdnvs[4].length;
		encodeSdnv(&(eidSdnvs[5]),
				bundle->reportTo.d.nssOffset);
		totalLengthOfEidSdnvs += eidSdnvs[5].length;
		encodeSdnv(&(eidSdnvs[6]),
				bundle->custodian.d.schemeNameOffset);
		totalLengthOfEidSdnvs += eidSdnvs[6].length;
		encodeSdnv(&(eidSdnvs[7]),
				bundle->custodian.d.nssOffset);
		totalLengthOfEidSdnvs += eidSdnvs[7].length;
	}

	encodeSdnv(&creationTimestampTimeSdnv, bundle->id.creationTime.seconds);
	encodeSdnv(&creationTimestampCountSdnv, bundle->id.creationTime.count);
	encodeSdnv(&lifetimeSdnv, bundle->timeToLive);
	encodeSdnv(&dictionaryLengthSdnv, bundle->dictionaryLength);
	if (bundle->bundleProcFlags & BDL_IS_FRAGMENT)
	{
		encodeSdnv(&fragmentOffsetSdnv, bundle->id.fragmentOffset);
		encodeSdnv(&totalAduLengthSdnv, bundle->totalAduLength);
	}
	else
	{
		fragmentOffsetSdnv.length = 0;
		totalAduLengthSdnv.length = 0;
	}

	residualBlkLength = totalLengthOfEidSdnvs +
			creationTimestampTimeSdnv.length +
			creationTimestampCountSdnv.length +
			lifetimeSdnv.length +
			dictionaryLengthSdnv.length +
			bundle->dictionaryLength +
			fragmentOffsetSdnv.length +
			totalAduLengthSdnv.length;
	encodeSdnv(&residualBlkLengthSdnv, residualBlkLength);
	encodeSdnv(&blkProcFlagsSdnv, bundle->payloadBlockProcFlags);
	encodeSdnv(&payloadLengthSdnv, bundle->payload.length);
	totalHeaderLength = 1 + bundleProcFlagsSdnv.length
			+ residualBlkLengthSdnv.length
			+ residualBlkLength
			+ bundle->extensionsLength[PRE_PAYLOAD]
			+ 1 + blkProcFlagsSdnv.length
			+ payloadLengthSdnv.length;
	buffer = MTAKE(totalHeaderLength);
	if (buffer == NULL)
	{
		putErrmsg("Can't construct bundle header.", NULL);
		return -1;
	}

	cursor = buffer;

	/*	Construct primary block.				*/

	*cursor = BP_VERSION;
	cursor++;

	memcpy(cursor, bundleProcFlagsSdnv.text, bundleProcFlagsSdnv.length);
	cursor += bundleProcFlagsSdnv.length;

	memcpy(cursor, residualBlkLengthSdnv.text,
			residualBlkLengthSdnv.length);
	cursor += residualBlkLengthSdnv.length;

	for (i = 0; i < 8; i++)
	{
		memcpy(cursor, eidSdnvs[i].text, eidSdnvs[i].length);
		cursor += eidSdnvs[i].length;
	}

	memcpy(cursor, creationTimestampTimeSdnv.text,
			creationTimestampTimeSdnv.length);
	cursor += creationTimestampTimeSdnv.length;

	memcpy(cursor, creationTimestampCountSdnv.text,
			creationTimestampCountSdnv.length);
	cursor += creationTimestampCountSdnv.length;

	memcpy(cursor, lifetimeSdnv.text, lifetimeSdnv.length);
	cursor += lifetimeSdnv.length;

	memcpy(cursor, dictionaryLengthSdnv.text, dictionaryLengthSdnv.length);
	cursor += dictionaryLengthSdnv.length;

	if (bundle->dictionaryLength > 0)
	{
		sdr_read(bpSdr, (char *) cursor, bundle->dictionary,
				bundle->dictionaryLength);
		cursor += bundle->dictionaryLength;
	}

	if (bundle->bundleProcFlags & BDL_IS_FRAGMENT)
	{
		memcpy(cursor, fragmentOffsetSdnv.text,
				fragmentOffsetSdnv.length);
		cursor += fragmentOffsetSdnv.length;
		memcpy(cursor, totalAduLengthSdnv.text,
				totalAduLengthSdnv.length);
		cursor += totalAduLengthSdnv.length;
	}

	/*	Insert pre-payload extension blocks.			*/

	for (elt = sdr_list_first(bpSdr, bundle->extensions[PRE_PAYLOAD]);
			elt; elt = sdr_list_next(bpSdr, elt))
	{
		blkAddr = sdr_list_data(bpSdr, elt);
		sdr_read(bpSdr, (char *) &blk, blkAddr, sizeof(ExtensionBlock));
		if (blk.suppressed)
		{
			continue;
		}

		sdr_read(bpSdr, (char *) cursor, blk.bytes, blk.length);
		cursor += blk.length;
	}

	/*	Construct payload block.				*/

	*cursor = 1;			/*	payload block type	*/
	cursor++;

	totalTrailerLength = bundle->extensionsLength[POST_PAYLOAD];
	if (totalTrailerLength == 0)
	{
		blkProcFlagsSdnv.text[blkProcFlagsSdnv.length - 1]
				|= BLK_IS_LAST;
	}
	else	/*	Turn off the BLK_IS_LAST flag.			*/
	{
		blkProcFlagsSdnv.text[blkProcFlagsSdnv.length - 1]
				&= ~BLK_IS_LAST;
	}

	memcpy(cursor, blkProcFlagsSdnv.text, blkProcFlagsSdnv.length);
	cursor += blkProcFlagsSdnv.length;

	memcpy(cursor, payloadLengthSdnv.text, payloadLengthSdnv.length);
	cursor += payloadLengthSdnv.length;

	/*	Prepend header (all blocks) to bundle ZCO.		*/

	oK(zco_prepend_header(bpSdr, bundle->payload.content, (char *) buffer,
			totalHeaderLength));
	MRELEASE(buffer);

	/*	Append all trailing extension blocks (if any) to
	 *	bundle ZCO.						*/

	if (totalTrailerLength > 0)
	{
		buffer = MTAKE(totalTrailerLength);
		if (buffer == NULL)
		{
			putErrmsg("Can't construct bundle trailer.", NULL);
			return -1;
		}

		cursor = buffer;
		for (elt = sdr_list_first(bpSdr,
			bundle->extensions[POST_PAYLOAD]); elt; elt = nextElt)
		{
			nextElt = sdr_list_next(bpSdr, elt);
			blkAddr = sdr_list_data(bpSdr, elt);
			sdr_read(bpSdr, (char *) &blk, blkAddr,
					sizeof(ExtensionBlock));
			if (blk.suppressed)
			{
				continue;
			}

			sdr_read(bpSdr, (char *) cursor, blk.bytes, blk.length);

			/*	Find last byte in the SDNV for the
			 *	block's processing flags; if this is
			 *	the last block in the bundle, turn on
			 *	the BLK_IS_LAST flag -- else turn that
			 *	flag off.				*/

			for (i = 1, flagbyte = cursor + 1;
					i <= blk.length; i++, flagbyte++)
			{
				if (((*flagbyte) & 0x80) == 0)
				{
					if (nextElt == 0)
					{
						(*flagbyte) |= BLK_IS_LAST;
					}
					else
					{
						(*flagbyte) &= ~BLK_IS_LAST;
					}

					break;	/*	Last SDNV byte.	*/
				}
			}

			cursor += blk.length;
		}

		oK(zco_append_trailer(bpSdr, bundle->payload.content,
				(char *) buffer, totalTrailerLength));
		MRELEASE(buffer);
	}

	return 0;
}

/*	*	*	Bundle transmission queue functions	*	*/

static int	signalCustodyAcceptance(Bundle *bundle)
{
	char	*dictionary;
	int	result;

	if ((dictionary = retrieveDictionary(bundle)) == (char *) bundle)
	{
		putErrmsg("Can't retrieve dictionary.", NULL);
		return -1;
	}

	bpCtTally(BP_CT_CUSTODY_ACCEPTED, bundle->payload.length);
	result = noteCtEvent(bundle, NULL, dictionary, 1, 0);
	releaseDictionary(dictionary);
	if (result < 0)
	{
		putErrmsg("Can't send custody signal.", NULL);
	}

	return result;
}

static int	insertNonCbheCustodian(Bundle *bundle, VScheme *vscheme)
{
	Sdr	bpSdr = getIonsdr();
	char	*oldDictionary;
	char	*string;
	int	stringLength;
	char	*strings[8];
	int	stringLengths[8];
	int	stringCount = 0;
	char	*newDictionary;
	char	*cursor;
	int	i;

	if ((oldDictionary = retrieveDictionary(bundle)) == (char *) bundle)
	{
		putErrmsg("Can't retrieve old dictionary for bundle.", NULL);
		return -1;
	}

	memset((char *) strings, 0, sizeof strings);
	memset((char *) stringLengths, 0, sizeof stringLengths);
	bundle->dbOverhead -= bundle->dictionaryLength;
	bundle->dictionaryLength = 0;
	sdr_free(bpSdr, bundle->dictionary);

	/*	Build new table of all strings currently in the
	 *	dictionary except the current custodian.  Append
	 *	the new custodian EID to that table.			*/

	string = oldDictionary + bundle->destination.d.schemeNameOffset;
	stringLength = strlen(string);
	bundle->destination.d.schemeNameOffset
		= addStringToDictionary(strings, stringLengths, &stringCount,
			&bundle->dictionaryLength, string, stringLength);
	string = oldDictionary + bundle->destination.d.nssOffset;
	stringLength = strlen(string);
	bundle->destination.d.nssOffset
		= addStringToDictionary(strings, stringLengths, &stringCount,
			&bundle->dictionaryLength, string, stringLength);
	string = oldDictionary + bundle->id.source.d.schemeNameOffset;
	stringLength = strlen(string);
	bundle->id.source.d.schemeNameOffset
		= addStringToDictionary(strings, stringLengths, &stringCount,
			&bundle->dictionaryLength, string, stringLength);
	string = oldDictionary + bundle->id.source.d.nssOffset;
	stringLength = strlen(string);
	bundle->id.source.d.nssOffset
		= addStringToDictionary(strings, stringLengths, &stringCount,
			&bundle->dictionaryLength, string, stringLength);
	string = oldDictionary + bundle->reportTo.d.schemeNameOffset;
	stringLength = strlen(string);
	bundle->reportTo.d.schemeNameOffset
		= addStringToDictionary(strings, stringLengths, &stringCount,
			&bundle->dictionaryLength, string, stringLength);
	string = oldDictionary + bundle->reportTo.d.nssOffset;
	stringLength = strlen(string);
	bundle->reportTo.d.nssOffset
		= addStringToDictionary(strings, stringLengths, &stringCount,
			&bundle->dictionaryLength, string, stringLength);
	bundle->custodian.d.schemeNameOffset
		= addStringToDictionary(strings, stringLengths, &stringCount,
		&bundle->dictionaryLength, vscheme->adminEid,
			vscheme->nameLength);
	bundle->custodian.d.nssOffset
		= addStringToDictionary(strings, stringLengths, &stringCount,
		&bundle->dictionaryLength, vscheme->adminEid
			+ (vscheme->nameLength + 1),
			vscheme->adminNSSLength);

	/*	Now concatenate all of these strings into the new
	 *	dictionary.						*/

	newDictionary = MTAKE(bundle->dictionaryLength);
	if (newDictionary == NULL)
	{
		MRELEASE(oldDictionary);
		putErrmsg("No memory for new dictionary.", NULL);
		return -1;
	}

	bundle->dictionary = sdr_malloc(bpSdr, bundle->dictionaryLength);
	if (bundle->dictionary == 0)
	{
		MRELEASE(newDictionary);
		MRELEASE(oldDictionary);
		putErrmsg("No space for dictionary.", NULL);
		return -1;
	}

	cursor = newDictionary;
	for (i = 0; i < stringCount; i++)
	{
		memcpy(cursor, strings[i], stringLengths[i]);
		cursor += stringLengths[i];
		*cursor = '\0';
		cursor++;
	}

	MRELEASE(oldDictionary);
	sdr_write(bpSdr, bundle->dictionary, newDictionary,
			bundle->dictionaryLength);
	MRELEASE(newDictionary);
	bundle->dbOverhead += bundle->dictionaryLength;
	return 0;
}

static int	takeCustody(Bundle *bundle)
{
	char		*custodialSchemeName;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;

	custodialSchemeName = getCustodialSchemeName(bundle);
	findScheme(custodialSchemeName, &vscheme, &vschemeElt);
	if (vschemeElt == 0)
	{
		return 0;	/*	Can't take custody; no EID.	*/
	}

	if (signalCustodyAcceptance(bundle) < 0)
	{
		putErrmsg("Can't signal custody acceptance.", NULL);
		return -1;
	}

	bundle->custodyTaken = 1;
	if (SRR_FLAGS(bundle->bundleProcFlags) & BP_CUSTODY_RPT)
	{
		bundle->statusRpt.flags |= BP_CUSTODY_RPT;
		getCurrentDtnTime(&(bundle->statusRpt.acceptanceTime));
	}

	if ((_bpvdb(NULL))->watching & WATCH_w)
	{
		putchar('w');
		fflush(stdout);
	}

	/*	Insert Endpoint ID of custodial endpoint.		*/

	if (bundle->dictionaryLength > 0)
	{
		return insertNonCbheCustodian(bundle, vscheme);
	}

	bundle->custodian.cbhe = 1;
	bundle->custodian.unicast = 1;
	bundle->custodian.c.nodeNbr = getOwnNodeNbr();
	bundle->custodian.c.serviceNbr = 0;
	if (processExtensionBlocks(bundle, PROCESS_ON_TAKE_CUSTODY, NULL) < 0)
	{
		putErrmsg("Can't process extensions.", "take custody");
		return -1;
	}

	return 0;
}

static int	sendAcceptanceAdminRecords(Bundle *bundle)
{
	char	*dictionary;
	int	result;

	if (bundleIsCustodial(bundle))
	{
		if (takeCustody(bundle) < 0)
		{
			putErrmsg("Can't take custody of bundle.", NULL);
			return -1;
		}
	}

	/*	Send any status report that is requested: custody
	 *	acceptance, if applicable, and reception as previously
	 *	noted, if applicable.					*/

	if (bundle->statusRpt.flags)
	{
		if ((dictionary = retrieveDictionary(bundle))
				== (char *) bundle)
		{
			putErrmsg("Can't retrieve dictionary.", NULL);
			return -1;
		}

		result = sendStatusRpt(bundle, dictionary);
		releaseDictionary(dictionary);
	       	if (result < 0)
		{
			putErrmsg("Can't send status report.", NULL);
			return -1;
		}
	}

	return 0;
}

int	bpAccept(Object bundleObj, Bundle *bundle)
{
	CHKERR(ionLocked());
	if (!bundle->accepted)	/*	Accept bundle only once.	*/
	{
		if (sendAcceptanceAdminRecords(bundle) < 0)
		{
			putErrmsg("Bundle acceptance failed.", NULL);
			return -1;
		}

		bundle->accepted = 1;
	}

	sdr_write(getIonsdr(), bundleObj, (char *) bundle, sizeof(Bundle));
	return 0;
}

static Object	insertBundleIntoQueue(Object queue, Object lastElt,
			Object bundleAddr, int priority,
			unsigned char ordinal, time_t enqueueTime)
{
	Sdr	bpSdr = getIonsdr();
		OBJ_POINTER(Bundle, bundle);

	/*	Bundles have transmission seniority which must be
	 *	honored.  A bundle that was enqueued for transmission
	 *	a while ago and now is being reforwarded must jump
	 *	the queue ahead of bundles of the same priority that
	 *	were enqueued more recently.				*/

	GET_OBJ_POINTER(bpSdr, Bundle, bundle, sdr_list_data(bpSdr, lastElt));
	while (enqueueTime < bundle->enqueueTime)
	{
		lastElt = sdr_list_prev(bpSdr, lastElt);
		if (lastElt == 0)
		{
			break;		/*	Reached head of queue.	*/
		}

		GET_OBJ_POINTER(bpSdr, Bundle, bundle,
				sdr_list_data(bpSdr, lastElt));
		if (priority < 2)
		{
			continue;	/*	Don't check ordinal.	*/
		}

		if (bundle->extendedCOS.ordinal > ordinal)
		{
			break;		/*	At head of subqueue.	*/
		}
	}

	if (lastElt)
	{
		return sdr_list_insert_after(bpSdr, lastElt, bundleAddr);
	}

	return sdr_list_insert_first(bpSdr, queue, bundleAddr);
}

static Object	enqueueUrgentBundle(Outduct *duct, Bundle *bundle,
			Object bundleObj, int backlogIncrement)
{
	unsigned char	ordinal = bundle->extendedCOS.ordinal;
	OrdinalState	*ord = &(duct->ordinals[ordinal]);
	Object		lastElt = 0;	// initialized to avoid warning
	int		i;
	Object		xmitElt;

	/*	Enqueue the new bundle immediately after the last
	 *	currently enqueued bundle whose ordinal is equal to
	 *	or greater than that of the new bundle.			*/

	for (i = ordinal; i < 256; i++)
	{
		lastElt = duct->ordinals[i].lastForOrdinal;
		if (lastElt)
		{
			break;
		}
	}

	if (i == 256)	/*	No more urgent bundle to enqueue after.	*/
	{
		xmitElt = sdr_list_insert_first(getIonsdr(), duct->urgentQueue,
				bundleObj);
	}
	else		/*	Enqueue after this one.			*/
	{
		xmitElt = insertBundleIntoQueue(duct->urgentQueue, lastElt,
				bundleObj, 2, ordinal, bundle->enqueueTime);
	}

	if (xmitElt)
	{
		ord->lastForOrdinal = xmitElt;
		increaseScalar(&(ord->backlog), backlogIncrement);
	}

	return xmitElt;
}

static int	isLoopback(char *eid)
{
	int		result;
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	elt;

	result = parseEidString(eid, &metaEid, &vscheme, &elt);
	restoreEidString(&metaEid);
	if (result == 0)
	{
		/*	Unrecognizable EID, so can't be an ION node,
		 *	so can't be the local node, so not loopback.	*/

		return 0;
	}

	if (strncmp(eid, vscheme->adminEid, MAX_EID_LEN) == 0)
	{
		return 1;
	}

	return 0;
}

int	bpEnqueue(FwdDirective *directive, Bundle *bundle, Object bundleObj,
		char *proxNodeEid)
{
	Sdr		bpSdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	BpVdb		*vdb = getBpVdb();
	Object		ductAddr;
	Outduct		duct;
	PsmAddress	vductElt;
	VOutduct	*vduct;
	char		destDuctName[MAX_CL_DUCT_NAME_LEN + 1];
	int		backlogIncrement;
	ClProtocol	protocol;
	time_t		enqueueTime;
	int		priority;
	Object		lastElt;

	CHKERR(ionLocked());
	CHKERR(directive && bundle && bundleObj && proxNodeEid);
	CHKERR(*proxNodeEid && strlen(proxNodeEid) < MAX_SDRSTRING);
	CHKERR(bundle->ductXmitElt == 0);
	bpDbTally(BP_DB_FWD_OKAY, bundle->payload.length);

	/*	We have settled on a neighboring node to forward
	 *	this bundle to; if it can't get there because the
	 *	duct to that node is blocked, then the bundle goes
	 *	into limbo until something changes.
	 *
	 *	But if the selected node is the local node (loopback)
	 *	and the bundle has already been delivered, we prevent
	 *	a loopback routing loop by NOT enqueueing the bundle.
	 *	Note that this is a backup check: the scheme-specific
	 *	forwarder should have checked the "delivered" flag
	 *	itself and refrained from trying to enqueue the bundle
	 *	for transmission to the local node.			*/

	if (bundle->delivered)
	{
		if (isLoopback(proxNodeEid))
		{
			return 0;
		}
	}

	/*	Next we check to see if the duct is blocked.		*/

	ductAddr = sdr_list_data(bpSdr, directive->outductElt);
	sdr_stage(bpSdr, (char *) &duct, ductAddr, sizeof(Outduct));
	if (duct.blocked)
	{
		return enqueueToLimbo(bundle, bundleObj);
	}

	/*      Now construct transmission parameters.			*/

	bundle->proxNodeEid = sdr_string_create(bpSdr, proxNodeEid);

	/*	Retrieve destination induct name, if applicable.	*/

	if (directive->destDuctName)
	{
		if (sdr_string_read(getIonsdr(), destDuctName,
				directive->destDuctName) < 0)
		{
			putErrmsg("Can't retrieve dest duct name.", NULL);
			return -1;
		}

		bundle->destDuctName = sdr_string_create(bpSdr, destDuctName);
	}
	else
	{
		bundle->destDuctName = 0;
	}

	if (processExtensionBlocks(bundle, PROCESS_ON_ENQUEUE, NULL) < 0)
	{
		putErrmsg("Can't process extensions.", "enqueue");
		return -1;
	}

	sdr_read(bpSdr, (char *) &protocol, duct.protocol, sizeof(ClProtocol));
	backlogIncrement = computeECCC(guessBundleSize(bundle), &protocol);
	if (bundle->enqueueTime == 0)
	{
		bundle->enqueueTime = enqueueTime = getUTCTime();
	}
	else
	{
		enqueueTime = bundle->enqueueTime;
	}

	/*	Insert bundle into the appropriate transmission queue
	 *	of the selected Duct.					*/

	priority = COS_FLAGS(bundle->bundleProcFlags) & 0x03;
	switch (priority)
	{
	case 0:
		lastElt = sdr_list_last(bpSdr, duct.bulkQueue);
		if (lastElt == 0)
		{
			bundle->ductXmitElt = sdr_list_insert_first(bpSdr,
				duct.bulkQueue, bundleObj);
		}
		else
		{
			bundle->ductXmitElt =
				insertBundleIntoQueue(duct.bulkQueue,
				lastElt, bundleObj, 0, 0, enqueueTime);
		}

		increaseScalar(&duct.bulkBacklog, backlogIncrement);
		break;

	case 1:
		lastElt = sdr_list_last(bpSdr, duct.stdQueue);
		if (lastElt == 0)
		{
			bundle->ductXmitElt = sdr_list_insert_first(bpSdr,
				duct.stdQueue, bundleObj);
		}
		else
		{
			bundle->ductXmitElt =
			       	insertBundleIntoQueue(duct.stdQueue,
				lastElt, bundleObj, 1, 0, enqueueTime);
		}

		increaseScalar(&duct.stdBacklog, backlogIncrement);
		break;

	default:
		bundle->ductXmitElt = enqueueUrgentBundle(&duct,
				bundle, bundleObj, backlogIncrement);
		increaseScalar(&duct.urgentBacklog, backlogIncrement);
	}

	sdr_write(bpSdr, ductAddr, (char *) &duct, sizeof(Outduct));
	sdr_write(bpSdr, bundleObj, (char *) bundle, sizeof(Bundle));
	if ((_bpvdb(NULL))->watching & WATCH_b)
	{
		putchar('b');
		fflush(stdout);
	}

	/*	Finally, if outduct is started then wake up CLO.	*/

	for (vductElt = sm_list_first(ionwm, vdb->outducts); vductElt;
			vductElt = sm_list_next(ionwm, vductElt))
	{
		vduct = (VOutduct *) psp(ionwm,
				sm_list_data(ionwm, vductElt));
		if (vduct->outductElt == directive->outductElt)
		{
			break;
		}
	}

	if (vductElt != 0)
	{
		bpOutductTally(vduct, BP_OUTDUCT_ENQUEUED,
				bundle->payload.length);
		if (vduct->semaphore != SM_SEM_NONE)
		{
			sm_SemGive(vduct->semaphore);
		}
	}

	return 0;
}

int	enqueueToLimbo(Bundle *bundle, Object bundleObj)
{
	Sdr	bpSdr = getIonsdr();
	BpDB	*bpConstants = getBpConstants();

	/*      ION has determined that this bundle must wait
	 *      in limbo until a duct is unblocked, enabling
	 *      transmission.  So append bundle to the "limbo"
	 *      list.							*/

	CHKERR(ionLocked());
	CHKERR(bundleObj && bundle);
	CHKERR(bundle->ductXmitElt == 0);
	if (bundle->extendedCOS.flags & BP_MINIMUM_LATENCY)
	{
		/*	"Critical" bundles are never reforwarded
		 *	(see notes on this in bpReforwardBundle),
		 *	so they can never be reforwarded after
		 *	insertion into limbo, so there's no point
		 *	in putting them into limbo.  So we don't.	*/

		return 0;
	}

	if (bundle->proxNodeEid)
	{
		sdr_free(bpSdr, bundle->proxNodeEid);
		bundle->proxNodeEid = 0;
	}

	if (bundle->destDuctName)
	{
		sdr_free(bpSdr, bundle->destDuctName);
		bundle->destDuctName = 0;
	}

	bundle->ductXmitElt = sdr_list_insert_last(bpSdr,
			bpConstants->limboQueue, bundleObj);
	sdr_write(bpSdr, bundleObj, (char *) bundle, sizeof(Bundle));
	bpDbTally(BP_DB_TO_LIMBO, bundle->payload.length);
	if ((_bpvdb(NULL))->watching & WATCH_limbo)
	{
		putchar('j');
		fflush(stdout);
	}

	return 0;
}

int	reverseEnqueue(Object xmitElt, ClProtocol *protocol, Object outductObj,
		Outduct *outduct, int sendToLimbo)
{
	Sdr	bpSdr = getIonsdr();
	Object	bundleAddr;
	Bundle	bundle;

	bundleAddr = sdr_list_data(bpSdr, xmitElt);
	sdr_stage(bpSdr, (char *) &bundle, bundleAddr, sizeof(Bundle));
	removeBundleFromQueue(&bundle, bundleAddr, protocol, outductObj,
			outduct);
	if (bundle.proxNodeEid)
	{
		sdr_free(bpSdr, bundle.proxNodeEid);
		bundle.proxNodeEid = 0;
	}

	if (bundle.destDuctName)
	{
		sdr_free(bpSdr, bundle.destDuctName);
		bundle.destDuctName = 0;
	}

	sdr_write(bpSdr, bundleAddr, (char *) &bundle, sizeof(Bundle));

	/*	If bundle is MINIMUM_LATENCY, nothing more to do.  We
	 *	never reforward critical bundles or send them to limbo.	*/

	if (bundle.extendedCOS.flags & BP_MINIMUM_LATENCY)
	{
		return 0;
	}

	if (!sendToLimbo)
	{
		/*	Want to give bundle another chance to be
		 *	transmitted at next opportunity.		*/

		return bpReforwardBundle(bundleAddr);
	}

	/*	Must queue the bundle into limbo unconditionally.	*/

	if (bundle.overdueElt)
	{
		/*	Bundle was un-queued before "overdue"
	 	*	alarm went off, so disable the alarm.		*/

		destroyBpTimelineEvent(bundle.overdueElt);
		bundle.overdueElt = 0;
	}

	if (bundle.ctDueElt)
	{
		destroyBpTimelineEvent(bundle.ctDueElt);
		bundle.ctDueElt = 0;
	}

	return enqueueToLimbo(&bundle, bundleAddr);
}

int	bpBlockOutduct(char *protocolName, char *ductName)
{
	Sdr		bpSdr = getIonsdr();
	VOutduct	*vduct;
	PsmAddress	outductElt;
	Object		outductObj;
       	Outduct		outduct;
	ClProtocol	protocol;
	Object		xmitElt;
	Object		nextElt;

	CHKERR(protocolName);
	CHKERR(ductName);
	findOutduct(protocolName, ductName, &vduct, &outductElt);
	if (outductElt == 0)
	{
		writeMemoNote("[?] Can't find outduct to block", ductName);
		return 0;
	}

	CHKERR(sdr_begin_xn(bpSdr));
	outductObj = sdr_list_data(bpSdr, vduct->outductElt);
	sdr_stage(bpSdr, (char *) &outduct, outductObj, sizeof(Outduct));
	if (outduct.blocked)
	{
		sdr_exit_xn(bpSdr);
		return 0;	/*	Already blocked, nothing to do.	*/
	}

	outduct.blocked = 1;
	sdr_read(bpSdr, (char *) &protocol, outduct.protocol,
			sizeof(ClProtocol));

	/*	Send into limbo all bundles currently queued for
	 *	transmission via this outduct.				*/

	for (xmitElt = sdr_list_first(bpSdr, outduct.urgentQueue); xmitElt;
			xmitElt = nextElt)
	{
		nextElt = sdr_list_next(bpSdr, xmitElt);
		if (reverseEnqueue(xmitElt, &protocol, outductObj, &outduct, 0))
		{
			putErrmsg("Can't requeue urgent bundle.", NULL);
			sdr_cancel_xn(bpSdr);
			return -1;
		}
	}

	for (xmitElt = sdr_list_first(bpSdr, outduct.stdQueue); xmitElt;
			xmitElt = nextElt)
	{
		nextElt = sdr_list_next(bpSdr, xmitElt);
		if (reverseEnqueue(xmitElt, &protocol, outductObj, &outduct, 0))
		{
			putErrmsg("Can't requeue std bundle.", NULL);
			sdr_cancel_xn(bpSdr);
			return -1;
		}
	}

	for (xmitElt = sdr_list_first(bpSdr, outduct.bulkQueue); xmitElt;
			xmitElt = nextElt)
	{
		nextElt = sdr_list_next(bpSdr, xmitElt);
		if (reverseEnqueue(xmitElt, &protocol, outductObj, &outduct, 0))
		{
			putErrmsg("Can't requeue bulk bundle.", NULL);
			sdr_cancel_xn(bpSdr);
			return -1;
		}
	}

	sdr_write(bpSdr, outductObj, (char *) &outduct, sizeof(Outduct));
	if (sdr_end_xn(bpSdr) < 0)
	{
		putErrmsg("Failed blocking outduct.", NULL);
		return -1;
	}

	return 0;
}

int	releaseFromLimbo(Object xmitElt, int resuming)
{
	Sdr	bpSdr = getIonsdr();
	Object	bundleAddr;
	Bundle	bundle;

	CHKERR(ionLocked());
	CHKERR(xmitElt);
	bundleAddr = sdr_list_data(bpSdr, xmitElt);
	sdr_stage(bpSdr, (char *) &bundle, bundleAddr, sizeof(Bundle));
	if (bundle.suspended)
	{
		if (resuming)
		{
			bundle.suspended = 0;
		}
		else	/*	Merely unblocking a blocked outduct.	*/
		{
			return 0;	/*	Can't release yet.	*/
		}
	}

	/*	Erase this bogus transmission reference.  Note that
	 *	by deleting the ductXmitElt object in this bundle
	 *	we are deleting the xmitElt that was passed to
	 *	this function -- don't count on being able to
	 *	navigate to the next xmitElt in limboQueue from it!	*/

	sdr_list_delete(bpSdr, bundle.ductXmitElt, NULL, NULL);
	bundle.ductXmitElt = 0;
	sdr_write(bpSdr, bundleAddr, (char *) &bundle, sizeof(Bundle));
	bpDbTally(BP_DB_FROM_LIMBO, bundle.payload.length);
	if ((_bpvdb(NULL))->watching & WATCH_delimbo)
	{
		putchar('k');
		fflush(stdout);
	}

	/*	Now see if the bundle can finally be transmitted.	*/

	if (bpReforwardBundle(bundleAddr) < 0)
	{
		putErrmsg("Failed releasing bundle from limbo.", NULL);
		return -1;
	}

	return 0;
}

int	bpUnblockOutduct(char *protocolName, char *ductName)
{
	Sdr		bpSdr = getIonsdr();
	VOutduct	*vduct;
	PsmAddress	outductElt;
	BpDB		*bpConstants = getBpConstants();
	Object		outductObj;
       	Outduct		outduct;
	Object		xmitElt;
	Object		nextElt;

	CHKERR(protocolName);
	CHKERR(ductName);
	findOutduct(protocolName, ductName, &vduct, &outductElt);
	if (outductElt == 0)
	{
		writeMemoNote("[?] Can't find outduct to unblock", ductName);
		return 0;
	}

	CHKERR(sdr_begin_xn(bpSdr));
	outductObj = sdr_list_data(bpSdr, vduct->outductElt);
	sdr_stage(bpSdr, (char *) &outduct, outductObj, sizeof(Outduct));
	if (outduct.blocked == 0)
	{
		sdr_exit_xn(bpSdr);
		return 0;	/*	Not blocked, nothing to do.	*/
	}

	outduct.blocked = 0;
	sdr_write(bpSdr, outductObj, (char *) &outduct, sizeof(Outduct));

	/*	Release all non-suspended bundles currently in limbo,
	 *	in case the unblocking of this outduct enables some
	 *	or all of them to be queued for transmission.		*/

	for (xmitElt = sdr_list_first(bpSdr, bpConstants->limboQueue);
			xmitElt; xmitElt = nextElt)
	{
		nextElt = sdr_list_next(bpSdr, xmitElt);
		if (releaseFromLimbo(xmitElt, 0) < 0)
		{
			putErrmsg("Failed releasing bundle from limbo.", NULL);
			sdr_cancel_xn(bpSdr);
			return -1;
		}
	}

	if (sdr_end_xn(bpSdr) < 0)
	{
		putErrmsg("Failed unblocking outduct.", NULL);
		return -1;
	}

	return 0;
}

static void	releaseCustody(Object bundleAddr, Bundle *bundle)
{
	Sdr	bpSdr = getIonsdr();

	bundle->custodyTaken = 0;
	if (bundle->ctDueElt)
	{
		/*	Bundle was transmitted before "CT due" alarm
		 *	went off, so disable the alarm.			*/

		destroyBpTimelineEvent(bundle->ctDueElt);
		bundle->ctDueElt = 0;
	}

	sdr_write(bpSdr, bundleAddr, (char *) bundle, sizeof(Bundle));
	bpCtTally(BP_CT_CUSTODY_RELEASED, bundle->payload.length);
}

int	bpAbandon(Object bundleObj, Bundle *bundle)
{
	char 	*dictionary = NULL;
	int	result1 = 0;
	int	result2 = 0;

	CHKERR(bundleObj && bundle);
	bpDbTally(BP_DB_FWD_FAILED, bundle->payload.length);
	dictionary = retrieveDictionary(bundle);
	if (dictionary == (char *) bundle)
	{
		putErrmsg("Can't retrieve dictionary.", NULL);
		return -1;
	}

	if (SRR_FLAGS(bundle->bundleProcFlags) & BP_DELETED_RPT)
	{
		bundle->statusRpt.flags |= BP_DELETED_RPT;
		bundle->statusRpt.reasonCode = SrNoKnownRoute;
		getCurrentDtnTime(&bundle->statusRpt.deletionTime);
	}

	if (bundle->statusRpt.flags)
	{
		result1 = sendStatusRpt(bundle, dictionary);
		if (result1 < 0)
		{
			putErrmsg("Can't send status report.", NULL);
		}
	}

	if (bundle->custodyTaken)
	{
		releaseCustody(bundleObj, bundle);
	}
	else
	{
		if (bundleIsCustodial(bundle))
		{
			bpCtTally(CtNoKnownRoute, bundle->payload.length);
			result2 = noteCtEvent(bundle, NULL, dictionary, 0,
					CtNoKnownRoute);
			if (result2 < 0)
			{
				putErrmsg("Can't send custody signal.", NULL);
			}
		}
	}

	releaseDictionary(dictionary);

	/*	Must record updated state of bundle in case
	 *	bpDestroyBundle doesn't erase it.			*/

	sdr_write(getIonsdr(), bundleObj, (char *) bundle, sizeof(Bundle));
	if (bpDestroyBundle(bundleObj, 0) < 0)
	{
		putErrmsg("Can't destroy bundle.", NULL);
		return -1;
	}

	if ((_bpvdb(NULL))->watching & WATCH_abandon)
	{
		putchar('~');
		fflush(stdout);
	}

	return ((result1 + result2) == 0 ? 0 : -1);
}

#ifdef ION_BANDWIDTH_RESERVED
static void	selectNextBundleForTransmission(Outflow *flows,
			Outflow **winner, Object *eltp)
{
	Sdr		bpSdr = getIonsdr();
	Outflow		*flow;
	Object		elt;
	Outflow		*selectedFlow;
	unsigned int	selectedFlowSvc;
	int		i;
	unsigned int	svcProvided;
        int             selectedFlowNbr;

	*winner = NULL;		/*	Default.			*/
	*eltp = 0;		/*	Default.			*/

	/*	If any priority traffic, choose it.			*/

	flow = flows + EXPEDITED_FLOW;
	elt = sdr_list_first(bpSdr, flow->outboundBundles);
	if (elt)
	{
		/*	*winner remains NULL to indicate that the
		 *	urgent flow was selected.  Prevents resync.	*/

		*eltp = elt;
		return;
	}

	/*	Otherwise send next PDU on the least heavily serviced
		non-empty non-urgent flow, if any.			*/

	selectedFlow = NULL;
	selectedFlowSvc = (unsigned int) -1;
	for (i = 0; i < EXPEDITED_FLOW; i++)
	{
		flow = flows + i;
		if (sdr_list_length(bpSdr, flow->outboundBundles) == 0)
		{
			continue;	/*	Nothing ready to send.	*/
		}

		/*	Consider flow as transmission candidate.	*/

		svcProvided = flow->totalBytesSent / flow->svcFactor;
		if (svcProvided < selectedFlowSvc) 
		{
			selectedFlow = flow;
			selectedFlowSvc = svcProvided;
			selectedFlowNbr = i;
		}
	}

	/*	At this point, selectedFlow (if any) is the flow for
	 *	which the smallest value of svcProvided was calculated,
	 *	and selectedFlowSvc is that value.			*/

	if (selectedFlow)
	{
		*winner = selectedFlow;
		*eltp = sdr_list_first(bpSdr, selectedFlow->outboundBundles);
	}
}

static void	resyncFlows(Outflow *flows)
{
	unsigned int	minSvcProvided;
	unsigned int	maxSvcProvided;
	int		i;
	Outflow		*flow;
	unsigned int	svcProvided;

	/*	Reset all flows if too unbalanced.			*/

	minSvcProvided = (unsigned int) -1;
	maxSvcProvided = 0;
	for (i = 0; i < EXPEDITED_FLOW; i++)
	{
		flow = flows + i;
		svcProvided = flow->totalBytesSent / flow->svcFactor;
		if (svcProvided < minSvcProvided)
		{
			minSvcProvided = svcProvided;
		}

		if (svcProvided > maxSvcProvided)
		{
			maxSvcProvided = svcProvided;
		}
	}

	if ((maxSvcProvided - minSvcProvided) >
			(MAX_STARVATION * NOMINAL_BYTES_PER_SEC))
	{
	/*	The most heavily serviced flow is at least
		MAX_STARVATION seconds ahead of the least
		serviced flow, so any sustained spike in
		traffic on the least serviced flow would
		starve the most serviced flow for at least
		MAX_STARVATION seconds.  To prevent this,
		we level the playing field by resetting all
		flows' totalBytesSent to zero.				*/

		for (i = 0; i < EXPEDITED_FLOW; i++)
		{
			flows[i].totalBytesSent = 0;
		}
	}
}
#else		/*	Strict priority, which is the default.		*/
static void	selectNextBundleForTransmission(Outflow *flows,
			Outflow **winner, Object *eltp)
{
	int	i;
	Outflow	*flow;
	Object	elt;

	*eltp = 0;		/*	Default: nothing ready.		*/
	i = EXPEDITED_FLOW;	/*	Start with highest priority.	*/
	while (1)
	{
		flow = flows + i;
		elt = sdr_list_first(getIonsdr(), flow->outboundBundles);
		if (elt)
		{
			*winner = flow;
			*eltp = elt;
			return;	/*	Got highest-priority bundle.	*/
		}

		i--;		/*	Try next lower priority flow.	*/
		if (i < 0)
		{
			return;	/*	Nothing is queued.		*/
		}
	}
}
#endif

static int 	getOutboundBundle(Outflow *flows, VOutduct *vduct,
			Outduct *outduct, Object outductObj,
			ClProtocol *protocol, unsigned int maxPayloadLength,
			Object *bundleObj, Bundle *bundle)
{
	Sdr		bpSdr = getIonsdr();
	PsmPartition	bpwm = getIonwm();
	Outflow		*selectedFlow;
	Object		xmitElt;
	Outflow		*sourceFlow;
	Bundle		firstBundle;
	Object		firstBundleObj;
	Bundle		secondBundle;
	Object		secondBundleObj;
	unsigned int	neighborNodeNbr;
	IonNode		*destNode;
	PsmAddress	nextNode;
	PsmAddress	snubElt;
	PsmAddress	nextSnub;
	IonSnub		*snub;

	sdr_stage(bpSdr, (char *) outduct, outductObj, 0);
	while (1)	/*	Might do one or more reforwards.	*/
	{
		selectNextBundleForTransmission(flows, &selectedFlow, &xmitElt);
		if (xmitElt == 0)		/*	Nothing ready.	*/
		{
			sdr_exit_xn(bpSdr);

			/*	Wait until forwarder announces an
			 *	outbound bundle by giving duct's
			 *	semaphore.  Duct might have changed
			 *	in the interim, so re-read it.		*/

			if (sm_SemTake(vduct->semaphore) < 0)
			{
				putErrmsg("CLO can't take duct semaphore.",
						NULL);
				return -1;
			}

			if (sm_SemEnded(vduct->semaphore))
			{
				writeMemo("[i] Outduct has been stopped.");

				/*	End task, but without error.	*/

				*bundleObj = 0;
				return 0;
			}

			CHKERR(sdr_begin_xn(bpSdr));
			sdr_stage(bpSdr, (char *) outduct, outductObj,
					sizeof(Outduct));
			continue;	/*	Should succeed now.	*/
		}

		/*	Got next outbound bundle.  Resync flows as
		 *	necessary.					*/

		*bundleObj = sdr_list_data(bpSdr, xmitElt);
		sdr_stage(bpSdr, (char *) bundle, *bundleObj, sizeof(Bundle));
#ifdef ION_BANDWIDTH_RESERVED
		if (selectedFlow != NULL)	/*	Not urgent.	*/
		{
			selectedFlow->totalBytesSent += bundle->payload.length;
			resyncFlows(flows);
		}
#endif
		/*	Fragment bundle as necessary.			*/

		if (maxPayloadLength > 0
		&& bundle->payload.length > maxPayloadLength)
		{
			/*	Must fragment this bundle.		*/

			if (bundle->bundleProcFlags & BDL_DOES_NOT_FRAGMENT)
			{
				/*	Bundle can't be fragmented,
				 *	shouldn't have been queued
				 *	on this outduct to start
				 *	with.  Reforward it and get
				 *	another bundle.			*/

				removeBundleFromQueue(bundle, *bundleObj,
						protocol, outductObj, outduct);
				if (bpReforwardBundle(*bundleObj) < 0)
				{
					putErrmsg("Frag refwd failed.", NULL);
					return -1;
				}

				/*	Try next bundle.	*/

				if (sdr_end_xn(bpSdr) < 0)
				{
					putErrmsg("Reforward failed.", NULL);
					return -1;
				}

				CHKERR(sdr_begin_xn(bpSdr));
				sdr_stage(bpSdr, (char *) outduct, outductObj,
						sizeof(Outduct));
				continue;
			}

			/*	Okay to fragment.			*/

			if (selectedFlow == NULL)
			{
				sourceFlow = flows + EXPEDITED_FLOW;
			}
			else
			{
				sourceFlow = selectedFlow;
			}

			if (bpClone(bundle, &firstBundle, &firstBundleObj, 0,
					maxPayloadLength) < 0
			|| bpClone(bundle, &secondBundle, &secondBundleObj,
					maxPayloadLength, bundle->payload.length
					- maxPayloadLength) < 0)
			{
				putErrmsg("CLO can't fragment bundle.",
						NULL);
				return -1;
			}

			/*	Lose the original bundle, inserting
			 *	the two fragments in its place.  No
			 *	significant change to backlog, so we
			 *	don't call purgeDuctXmitElt which calls
			 *	removeBundleFromQueue.			*/

			sdr_list_delete(bpSdr, bundle->ductXmitElt, NULL, NULL);
			bundle->ductXmitElt = 0;
			sdr_write(bpSdr, *bundleObj, (char *) bundle,
					sizeof(Bundle));
			if (bpDestroyBundle(*bundleObj, 0) < 0)
			{
				putErrmsg("CLO can't destroy original bundle.",
						NULL);
				return -1;
			}

			secondBundle.ductXmitElt = sdr_list_insert_first(bpSdr,
				sourceFlow->outboundBundles, secondBundleObj);
			sdr_write(bpSdr, secondBundleObj,
				(char *) &secondBundle, sizeof(Bundle));
			firstBundle.ductXmitElt = sdr_list_insert_first(bpSdr,
				sourceFlow->outboundBundles, firstBundleObj);
			sdr_write(bpSdr, firstBundleObj,
				(char *) &firstBundle, sizeof(Bundle));
			xmitElt = firstBundle.ductXmitElt;
			*bundleObj = firstBundleObj;
			memcpy((char *) bundle, (char *) &firstBundle,
					sizeof(Bundle));
		}

		/*	Pop the selected bundle out of its transmission
		 *	queue.						*/

		removeBundleFromQueue(bundle, *bundleObj, protocol, outductObj,
				outduct);

		/*	If the neighbor for this duct has begun
		 *	snubbing bundles for the indicated destination
		 *	since this bundle was enqueued to this duct,
		 *	re-forward the bundle on a different route
		 *	(if possible).  Note that we can only track
		 *	snubs that are issued by neighbors reached
		 *	via LTP ducts and are for nodes identified by
		 *	CBHE-conformant unicast endpoint IDs.		*/

		if (bundle->destination.cbhe
		&& bundle->destination.unicast
		&& strcmp(vduct->protocolName, "ltp") == 0)
		{
			neighborNodeNbr = strtoul(vduct->ductName, NULL, 0);
			destNode = findNode(getIonVdb(),
				bundle->destination.c.nodeNbr, &nextNode);
			if (destNode)	/*	Node might have snubs.	*/
			{
				/*	Check list of nodes that have
				 *	been refusing bundles for this
				 *	destination.			*/

				for (snubElt = sm_list_first(bpwm,
						destNode->snubs); snubElt;
						snubElt = nextSnub)
				{
					nextSnub = sm_list_next(bpwm, snubElt);
					snub = (IonSnub *) psp(bpwm,
						sm_list_data(bpwm, snubElt));
					if (snub->nodeNbr < neighborNodeNbr)
					{
						continue;
					}

					if (snub->nodeNbr == neighborNodeNbr)
					{
						break;
					}

					nextSnub = 0;	/*	End.	*/
				}

				if (snubElt)
				{
					/*	This neighbor has been
					 *	refusing custody of
					 *	bundles for this
					 *	destination.		*/

					if (bpReforwardBundle(*bundleObj) < 0)
					{
						putErrmsg("Snub refwd failed.",
								NULL);
						return -1;
					}

					/*	Try next bundle.	*/

					if (sdr_end_xn(bpSdr) < 0)
					{
						putErrmsg("Reforward failed.",
								NULL);
						return -1;
					}

					CHKERR(sdr_begin_xn(bpSdr));
					sdr_stage(bpSdr, (char *) outduct,
						outductObj, sizeof(Outduct));
					continue;
				}
			}
		}

		/*	Return this bundle to the CLO.			*/

		return 0;
	}
}

int	bpDequeue(VOutduct *vduct, Outflow *flows, Object *bundleZco,
		BpExtendedCOS *extendedCOS, char *destDuctName,
		unsigned int maxPayloadLength, int timeoutInterval)
{
	Sdr		bpSdr = getIonsdr();
	int		stewardshipAccepted;
	Object		outductObj;
	Outduct		outduct;
			OBJ_POINTER(ClProtocol, protocol);
	Object		bundleObj;
	Bundle		bundle;
	BundleSet	bset;
	char		proxNodeEid[SDRSTRING_BUFSZ];
	DequeueContext	context;
	char		*dictionary;
	int		xmitLength;

	CHKERR(vduct && flows && bundleZco && extendedCOS && destDuctName);
	*bundleZco = 0;			/*	Default behavior.	*/
	*destDuctName = '\0';		/*	Default behavior.	*/
	if (timeoutInterval < 0)	/*	CLA is a steward.	*/
	{
		/*	Note that stewardship and custody acceptance
		 *	timeout are mutually exclusive.			*/

		stewardshipAccepted = 1;
		timeoutInterval = 0;
	}
	else
	{
		stewardshipAccepted = 0;
	}

	CHKERR(sdr_begin_xn(bpSdr));

	/*	Transmission rate control: wait for capacity.  But
	 *	no rate control if throttle nominal rate < 0.		*/

	if (vduct->xmitThrottle.nominalRate >= 0)
	{
		while (vduct->xmitThrottle.capacity <= 0)
		{
			sdr_exit_xn(bpSdr);
			if (sm_SemTake(vduct->xmitThrottle.semaphore) < 0)
			{
				putErrmsg("CLO can't take throttle semaphore.",
						NULL);
				return -1;
			}

			if (sm_SemEnded(vduct->xmitThrottle.semaphore))
			{
				writeMemo("[i] Outduct has been stopped.");

				/*	End task, but without error.	*/

				return -1;
			}

			CHKERR(sdr_begin_xn(bpSdr));
		}
	}

	outductObj = sdr_list_data(bpSdr, vduct->outductElt);
	sdr_read(bpSdr, (char *) &outduct, outductObj, sizeof(Outduct));
	GET_OBJ_POINTER(bpSdr, ClProtocol, protocol, outduct.protocol);

	/*	Get a transmittable bundle.				*/

	if (getOutboundBundle(flows, vduct, &outduct, outductObj, protocol,
			maxPayloadLength, &bundleObj, &bundle) < 0)
	{
		putErrmsg("CLO can't get next outbound bundle.", NULL);
		sdr_cancel_xn(bpSdr);
		return -1;
	}

	if (bundleObj == 0)	/*	Outduct has been stopped.	*/
	{
		sdr_exit_xn(bpSdr);
		return -1;	/*	End task, but without error.	*/
	}

	if (bundle.proxNodeEid)
	{
		sdr_string_read(bpSdr, proxNodeEid, bundle.proxNodeEid);
	}
	else
	{
		proxNodeEid[0] = '\0';
	}

	context.protocolName = protocol->name;
	context.proxNodeEid = proxNodeEid;
	if (processExtensionBlocks(&bundle, PROCESS_ON_DEQUEUE, &context) < 0)
	{
		putErrmsg("Can't process extensions.", "dequeue");
		sdr_cancel_xn(bpSdr);
		return -1;
	}

	if (bundle.overdueElt)
	{
		/*	Bundle was transmitted before "overdue"
		 *	alarm went off, so disable the alarm.		*/

		destroyBpTimelineEvent(bundle.overdueElt);
		bundle.overdueElt = 0;
	}

	/*	We now serialize the bundle header and prepend that
	 *	header to the payload of the bundle; if there are
	 *	post-payload extension blocks we also serialize
	 *	them into a single trailer and append it to the
	 *	payload.  This transforms the payload ZCO into a
	 *	fully catenated bundle, ready for transmission.		*/

	if (catenateBundle(&bundle) < 0)
	{
		putErrmsg("Can't catenate bundle.", NULL);
		sdr_cancel_xn(bpSdr);
		return -1;
	}

	/*	Some final extension-block processing may be necessary
	 *	after catenation of the bundle, notably BAB hash
	 *	calculation.						*/

	if (processExtensionBlocks(&bundle, PROCESS_ON_TRANSMIT, NULL) < 0)
	{
		putErrmsg("Can't process extensions.", "dequeue");
		sdr_cancel_xn(bpSdr);
		return -1;
	}

	/*	That's the end of the changes to the bundle.  Pass
	 *	the catenated bundle (the payload ZCO) to the calling
	 *	function, then replace it in the bundle structure with
	 *	a clone of the payload source data only; cloning the
	 *	zco strips off the headers and trailers added by
	 *	the catenation function.  This is necessary in case
	 *	the bundle is subject to re-forwarding (due to either
	 *	stewardship or custody) and thus re-catenation.		*/

	*bundleZco = bundle.payload.content;
	bundle.payload.content = zco_clone(bpSdr, *bundleZco, 0,
			zco_source_data_length(bpSdr, *bundleZco));
	sdr_write(bpSdr, bundleObj, (char *) &bundle, sizeof(Bundle));

	/*	At this point we check the stewardshipAccepted flag.
	 *	If the bundle is critical then copies have been queued
	 *	for transmission on all possible routes and none of
	 *	them are subject to reforwarding (see notes on this in
	 *	bpReforwardBundle).  Since this bundle is not subject
	 *	to reforwarding, stewardship is meaningless -- on
	 *	convergence-layer transmission failure the attempt
	 *	to transmit the bundle is simply abandoned.  So
	 *	in this event the stewardshipAccepted flag is forced
	 *	to zero even if the convergence-layer adapter for
	 *	this outduct is one that normally accepts stewardship.	*/

	if (bundle.extendedCOS.flags & BP_MINIMUM_LATENCY)
	{
		stewardshipAccepted = 0;
	}

	/*	Note that the convergence-layer adapter task *must*
	 *	call either the bpHandleXmitSuccess function or
	 *	the bpHandleXmitFailure function for *every*
	 *	bundle it obtains via bpDequeue for which it has
	 *	accepted stewardship.
	 *
	 *	If the bundle isn't in the node's hash table of all
	 *	bundles -- or if multiple bundles have the same key
	 *	-- then neither of these stewardship resolution
	 *	functions can succeed, so stewardship cannot be
	 *	successfully accepted.  Again, stewardshipAccepted
	 *	is forced to zero.					*/

	if (bundle.hashEntry == 0)
	{
		stewardshipAccepted = 0;
	}
	else	/*	Does hash entry resolve to only this bundle?	*/
	{
		sdr_read(bpSdr, (char *) &bset, sdr_hash_entry_value(bpSdr,
				(_bpConstants())->bundles, bundle.hashEntry),
				sizeof(BundleSet));
		if (bset.bundleObj != bundleObj)
		{
			stewardshipAccepted = 0;
		}
	}

	/*	Finally, if the bundle is anonymous then its key can
	 *	never be guaranteed to be unique (because the sending
	 *	EID that qualifies the creation time and sequence
	 *	number is omitted), so the bundle can't reliably be
	 *	retrieved via the hash table.  So again stewardship
	 *	cannot be successfully accepted.			*/

	if (bundle.anonymous)
	{
		stewardshipAccepted = 0;
	}

	/*	Note that when stewardship is not accepted, the bundle
	 *	is subject to destruction immediately unless custody
	 *	of the bundle has been accepted.
	 *
	 *	If custody of the bundle has indeed been taken and
	 *	a custody acceptance timer has been requested, set
	 *	the timer now.						*/

	if (bundle.custodyTaken && timeoutInterval > 0)
	{
		if (bpMemo(bundleObj, timeoutInterval) < 0)
		{
			putErrmsg("Can't set custody timeout.", NULL);
			sdr_cancel_xn(bpSdr);
			return -1;
		}
	}

	/*	Track this transmission event.				*/

	bpOutductTally(vduct, BP_OUTDUCT_DEQUEUED, bundle.payload.length);
	bpXmitTally(COS_FLAGS(bundle.bundleProcFlags) & 0x03,
			bundle.payload.length);
	if ((_bpvdb(NULL))->watching & WATCH_c)
	{
		putchar('c');
		fflush(stdout);
	}

	/*	Consume estimated transmission capacity.		*/

	xmitLength = computeECCC(bundle.payload.length
			+ NOMINAL_PRIMARY_BLKSIZE, protocol);
	vduct->xmitThrottle.capacity -= xmitLength;
	if (vduct->xmitThrottle.capacity > 0)
	{
		sm_SemGive(vduct->xmitThrottle.semaphore);
	}

	/*	Return the outbound buffer's extended class of service.	*/

	memcpy((char *) extendedCOS, (char *) &bundle.extendedCOS,
			sizeof(BpExtendedCOS));

	/*	Note destination duct name for this bundle, if any.	*/

	if (bundle.destDuctName)
	{
		sdr_string_read(bpSdr, destDuctName, bundle.destDuctName);
	}

	/*	Finally, authorize transmission of applicable status
	 *	report message and destruction of the bundle object
	 *	unless stewardship was successfully accepted.		*/

	if (!stewardshipAccepted)
	{
		if (SRR_FLAGS(bundle.bundleProcFlags) & BP_FORWARDED_RPT)
		{
			if ((dictionary = retrieveDictionary(&bundle))
					== (char *) &bundle)
			{
				putErrmsg("Can't retrieve dictionary.", NULL);
				sdr_cancel_xn(bpSdr);
				return -1;
			}

			bundle.statusRpt.flags |= BP_FORWARDED_RPT;
			getCurrentDtnTime(&bundle.statusRpt.forwardTime);
			if (sendStatusRpt(&bundle, dictionary) < 0)
			{
				releaseDictionary(dictionary);
				putErrmsg("Can't send status report.", NULL);
				sdr_cancel_xn(bpSdr);
				return -1;
			}

			releaseDictionary(dictionary);
		}

		if (bpDestroyBundle(bundleObj, 0) < 0)
		{
			putErrmsg("Can't destroy bundle.", NULL);
			sdr_cancel_xn(bpSdr);
			return -1;
		}
	}

	if (sdr_end_xn(bpSdr))
	{
		putErrmsg("Can't get outbound bundle.", NULL);
		return -1;
	}

	return 0;
}

static int	nextBlock(Sdr sdr, ZcoReader *reader, unsigned char *buffer,
			int *bytesBuffered, int bytesParsed)
{
	int	bytesToReceive;
	int	bytesReceived;

	/*	Shift buffer left by length of prior buffer.		*/

	*bytesBuffered -= bytesParsed;
	memcpy(buffer, buffer + bytesParsed, *bytesBuffered);

	/*	Now read from ZCO to fill the buffer space that was
	 *	vacated.						*/

	bytesToReceive = BP_MAX_BLOCK_SIZE - *bytesBuffered;
	bytesReceived = zco_transmit(sdr, reader, bytesToReceive,
			((char *) buffer) + *bytesBuffered);
	if (bytesReceived < 0)
	{
		putErrmsg("Can't retrieve next block.", NULL);
		return -1;
	}

	*bytesBuffered += bytesReceived;
	return 0;
}

static int	bufAdvance(int length, unsigned int *bundleLength,
			unsigned char **cursor, unsigned char *endOfBuffer)
{
	*cursor += length;
	if (*cursor > endOfBuffer)
	{
		writeMemoNote("[?] Bundle truncated",
				itoa(endOfBuffer - *cursor));
		*bundleLength = 0;
	}
	else
	{
		*bundleLength += length;
	}

	return *bundleLength;
}

static int	decodeHeader(Sdr sdr, ZcoReader *reader, unsigned char *buffer,
			int bytesBuffered, Bundle *image, char **dictionary,
			unsigned int *bundleLength)
{
	unsigned char	*endOfBuffer;
	unsigned char	*cursor;
	int		sdnvLength;
	uvast		longNumber;
	int		i;
	uvast		eidSdnvValues[8];
	unsigned char	blkType;
	unsigned int	blkProcFlags;
	unsigned int	eidReferencesCount;
	unsigned int	blockDataLength;

	cursor = buffer;
	endOfBuffer = buffer + bytesBuffered;

	/*	Skip over version number.				*/

	if (bufAdvance(1, bundleLength, &cursor, endOfBuffer) == 0)
	{
		return 0;
	}

	/*	Parse out the bundle processing flags.			*/

	sdnvLength = decodeSdnv(&longNumber, cursor);
	image->bundleProcFlags = longNumber;
	if (bufAdvance(sdnvLength, bundleLength, &cursor, endOfBuffer) == 0)
	{
		return 0;
	}

	/*	Skip over remaining primary block length.		*/

	sdnvLength = decodeSdnv(&longNumber, cursor);
	if (bufAdvance(sdnvLength, bundleLength, &cursor, endOfBuffer) == 0)
	{
		return 0;
	}

	/*	Get all EID SDNV values.				*/

	for (i = 0; i < 8; i++)
	{
		sdnvLength = decodeSdnv(&(eidSdnvValues[i]), cursor);
		if (bufAdvance(sdnvLength, bundleLength, &cursor, endOfBuffer)
				== 0)
		{
			return 0;
		}
	}

	/*	Get creation time.					*/

	sdnvLength = decodeSdnv(&longNumber, cursor);
	image->id.creationTime.seconds = longNumber;
	if (bufAdvance(sdnvLength, bundleLength, &cursor, endOfBuffer) == 0)
	{
		return 0;
	}

	sdnvLength = decodeSdnv(&longNumber, cursor);
	image->id.creationTime.count = longNumber;
	if (bufAdvance(sdnvLength, bundleLength, &cursor, endOfBuffer) == 0)
	{
		return 0;
	}

	/*	Skip over lifetime.					*/

	sdnvLength = decodeSdnv(&longNumber, cursor);
	if (bufAdvance(sdnvLength, bundleLength, &cursor, endOfBuffer) == 0)
	{
		return 0;
	}

	/*	Get dictionary length.					*/

	sdnvLength = decodeSdnv(&longNumber, cursor);
	image->dictionaryLength = longNumber;
	if (bufAdvance(sdnvLength, bundleLength, &cursor, endOfBuffer) == 0)
	{
		return 0;
	}

	/*	Get the dictionary, if present, and get the source
	 *	endpoint ID.						*/

	if (cursor + image->dictionaryLength > endOfBuffer)
	{
		*bundleLength = 0;
		writeMemoNote("[?] Truncated bundle", itoa(endOfBuffer -
				(cursor + image->dictionaryLength)));
		return 0;
	}

	if (image->dictionaryLength == 0)		/*	CBHE	*/
	{
		*dictionary = NULL;
		image->id.source.cbhe = 1;
		image->id.source.unicast = 1;
		image->id.source.c.nodeNbr = eidSdnvValues[2];
		image->id.source.c.serviceNbr = eidSdnvValues[3];
	}
	else
	{
		*dictionary = (char *) cursor;
		*bundleLength += image->dictionaryLength;
		cursor += image->dictionaryLength;
		image->id.source.cbhe = 0;
		image->id.source.unicast = 1;
		image->id.source.d.schemeNameOffset = eidSdnvValues[2];
		image->id.source.d.nssOffset = eidSdnvValues[3];
	}

	/*	Get fragment offset and total ADU length, if present.	*/

	if ((image->bundleProcFlags & BDL_IS_FRAGMENT) == 0)
	{
		image->id.fragmentOffset = 0;
		image->totalAduLength = 0;
		return 0;	/*	All bundle ID info is known.	*/
	}

	/*	Bundle is a fragment, so fragment offset and length
	 *	(which is payload length) must be determined.		*/

	sdnvLength = decodeSdnv(&longNumber, cursor);
	image->id.fragmentOffset = longNumber;
	if (bufAdvance(sdnvLength, bundleLength, &cursor, endOfBuffer) == 0)
	{
		return 0;
	}

	sdnvLength = decodeSdnv(&longNumber, cursor);
	image->totalAduLength = longNumber;
	if (bufAdvance(sdnvLength, bundleLength, &cursor, endOfBuffer) == 0)
	{
		return 0;
	}

	/*	At this point, cursor has been advanced to first
	 *	byte after the end of the primary block.  Now parse
	 *	other blocks until payload length is known.		*/

	while (1)
	{
		if (nextBlock(sdr, reader, buffer, &bytesBuffered,
				cursor - buffer) < 0)
		{
			return -1;
		}

		cursor = buffer;
		endOfBuffer = buffer + bytesBuffered;
		if (cursor + 1 > endOfBuffer)
		{
			return 0;	/*	No more blocks.		*/
		}

		/*	Get the block type.				*/

		blkType = *cursor;
		if (bufAdvance(1, bundleLength, &cursor, endOfBuffer) == 0)
		{
			return 0;
		}

		/*	Get block processing flags.			*/

		sdnvLength = decodeSdnv(&longNumber, cursor);
		blkProcFlags = longNumber;
		if (bufAdvance(sdnvLength, bundleLength, &cursor, endOfBuffer)
				== 0)
		{
			return 0;
		}

		if (blkProcFlags & BLK_HAS_EID_REFERENCES)
		{
			/*	Skip over EID references.		*/

			sdnvLength = decodeSdnv(&longNumber, cursor);
			eidReferencesCount = longNumber;
			if (bufAdvance(sdnvLength, bundleLength, &cursor,
					endOfBuffer) == 0)
			{
				return 0;
			}

			while (eidReferencesCount > 0)
			{
				/*	Skip scheme name offset.	*/

				sdnvLength = decodeSdnv(&longNumber, cursor);
				if (bufAdvance(sdnvLength, bundleLength,
						&cursor, endOfBuffer) == 0)
				{
					return 0;
				}

				/*	Skip NSS offset.		*/

				sdnvLength = decodeSdnv(&longNumber, cursor);
				if (bufAdvance(sdnvLength, bundleLength,
						&cursor, endOfBuffer) == 0)
				{
					return 0;
				}

				eidReferencesCount--;
			}
		}

		/*	Get length of data in block.			*/

		sdnvLength = decodeSdnv(&longNumber, cursor);
		blockDataLength = longNumber;
		if (bufAdvance(sdnvLength, bundleLength, &cursor, endOfBuffer)
				== 0)
		{
			return 0;
		}

		if (blkType == 1)		/*	Payload block.	*/
		{
			image->payload.length = blockDataLength;
			return 0;		/*	Done.		*/
		}

		/*	Skip over data, to end of this block.		*/

		if (bufAdvance(blockDataLength, bundleLength, &cursor,
				endOfBuffer) == 0)
		{
			return 0;
		}
	}
}

static int	decodeBundle(Sdr sdr, Object zco, unsigned char *buffer,
			Bundle *image, char **dictionary,
			unsigned int *bundleLength)
{
	ZcoReader	reader;
	int		bytesBuffered;

	*bundleLength = 0;	/*	Initialize to default.		*/
	memset((char *) image, 0, sizeof(Bundle));

	/*	This is an outbound bundle, so the headers and trailers
	 *	are in capsules and we use zco_transmit to re-read it.	*/

	zco_start_transmitting(zco, &reader);
	CHKERR(sdr_begin_xn(sdr));
	bytesBuffered = zco_transmit(sdr, &reader, BP_MAX_BLOCK_SIZE,
			(char *) buffer);
	if (bytesBuffered < 0)
	{
		putErrmsg("Can't extract primary block.", NULL);
		oK(sdr_end_xn(sdr));
		return -1;
	}

	if (decodeHeader(sdr, &reader, buffer, bytesBuffered, image,
			dictionary, bundleLength) < 0)
	{
		putErrmsg("Can't decode bundle header.", NULL);
		oK(sdr_end_xn(sdr));
		return -1;
	}

	return sdr_end_xn(sdr);
}

int	bpIdentify(Object bundleZco, Object *bundleObj)
{
	Sdr		bpSdr = getIonsdr();
	unsigned char	*buffer;
	Bundle		image;
	char		*dictionary = 0;	/*	To hush gcc.	*/
	unsigned int	bundleLength;
	int		result;
	char		*sourceEid;

	CHKERR(bundleZco);
	CHKERR(bundleObj);
	*bundleObj = 0;			/*	Default: not located.	*/
	buffer = (unsigned char *) MTAKE(BP_MAX_BLOCK_SIZE);
	if (buffer == NULL)
	{
		putErrmsg("Can't create buffer for reading bundle ID.", NULL);
		return -1;
	}

	if (decodeBundle(bpSdr, bundleZco, buffer, &image, &dictionary,
			&bundleLength) < 0)
	{
		MRELEASE(buffer);
		putErrmsg("Can't extract bundle ID.", NULL);
		return -1;
	}

	if (bundleLength == 0)		/*	Can't get bundle ID.	*/
	{
		return 0;
	}

	/*	Recreate the source EID.				*/

	result = printEid(&image.id.source, dictionary, &sourceEid);
	MRELEASE(buffer);
	if (result < 0)
	{
		putErrmsg("No memory for source EID.", NULL);
		return -1;
	}

	/*	Now use this bundle ID to find the bundle.		*/

	CHKERR(sdr_begin_xn(bpSdr));	/*	Just to lock memory.	*/
	result = findBundle(sourceEid, &image.id.creationTime,
			image.id.fragmentOffset, image.totalAduLength == 0 ? 0
			: image.payload.length, bundleObj);
	sdr_exit_xn(bpSdr);
	MRELEASE(sourceEid);
	if (result < 0)
	{
		putErrmsg("Failed seeking bundle.", NULL);
		return -1;
	}

	return 0;
}

int	bpMemo(Object bundleObj, unsigned int interval)
{
	Sdr	bpSdr = getIonsdr();
	Bundle	bundle;
	BpEvent	event;

	CHKERR(bundleObj);
	CHKERR(interval > 0);
	event.type = ctDue;
	event.time = getUTCTime() + interval;
	event.ref = bundleObj;
	CHKERR(sdr_begin_xn(bpSdr));
	sdr_stage(bpSdr, (char *) &bundle, bundleObj, sizeof(Bundle));
	if (bundle.ctDueElt)
	{
		writeMemo("Revising a custody acceptance due timer.");
		destroyBpTimelineEvent(bundle.ctDueElt);
	}

	bundle.ctDueElt = insertBpTimelineEvent(&event);
	sdr_write(bpSdr, bundleObj, (char *) &bundle, sizeof(Bundle));
	if (sdr_end_xn(bpSdr) < 0)
	{
		putErrmsg("Failed posting ctDue event.", NULL);
		return -1;
	}

	return 0;
}

int	retrieveInTransitBundle(Object bundleZco, Object *bundleObj)
{
	Sdr		bpSdr = getIonsdr();
	unsigned char	*buffer;
	Bundle		image;
	char		*dictionary = 0;	/*	To hush gcc.	*/
	unsigned int	bundleLength;
	int		result;
	char		*sourceEid;

	CHKERR(bundleZco);
	CHKERR(bundleObj);
	CHKERR(ionLocked());
	*bundleObj = 0;			/*	Default: not located.	*/
	buffer = (unsigned char *) MTAKE(BP_MAX_BLOCK_SIZE);
	if (buffer == NULL)
	{
		putErrmsg("Can't create buffer for reading bundle ID.", NULL);
		return -1;
	}

	if (decodeBundle(bpSdr, bundleZco, buffer, &image, &dictionary,
			&bundleLength) < 0)
	{
		MRELEASE(buffer);
		putErrmsg("Can't extract bundle ID.", NULL);
		return -1;
	}

	if (bundleLength == 0)		/*	Can't get bundle ID.	*/
	{
		MRELEASE(buffer);
		return 0;
	}

	/*	Recreate the source EID.				*/

	result = printEid(&image.id.source, dictionary, &sourceEid);
	MRELEASE(buffer);
	if (result < 0)
	{
		putErrmsg("No memory for source EID.", NULL);
		return -1;
	}

	/*	Now use this bundle ID to retrieve the bundle.		*/

	result = findBundle(sourceEid, &image.id.creationTime,
			image.id.fragmentOffset, image.totalAduLength == 0 ? 0
			: image.payload.length, bundleObj);
	MRELEASE(sourceEid);
	return (result < 0 ? result : 0);
}

int	bpHandleXmitSuccess(Object bundleZco, unsigned int timeoutInterval)
{
	Sdr	bpSdr = getIonsdr();
	Object	bundleAddr;
	Bundle	bundle;
	char	*dictionary;
	int	result;

	CHKERR(sdr_begin_xn(bpSdr));
	if (retrieveInTransitBundle(bundleZco, &bundleAddr) < 0)
	{
		putErrmsg("Can't locate bundle for okay transmission.", NULL);
		sdr_cancel_xn(bpSdr);
		return -1;
	}

	if (bundleAddr == 0)	/*	Bundle not found.		*/
	{
		if (sdr_end_xn(bpSdr) < 0)
		{
			putErrmsg("Failed handling xmit success.", NULL);
			return -1;
		}

		return 0;	/*	bpDestroyBundle already called.	*/
	}

	sdr_read(bpSdr, (char *) &bundle, bundleAddr, sizeof(Bundle));
	if (bundle.custodyTaken && timeoutInterval > 0)
	{
		if (bpMemo(bundleAddr, timeoutInterval) < 0)
		{
			putErrmsg("Can't set custody timeout.", NULL);
			sdr_cancel_xn(bpSdr);
			return -1;
		}
	}

	/*	Send "forwarded" status report if necessary.		*/

	if (SRR_FLAGS(bundle.bundleProcFlags) & BP_FORWARDED_RPT)
	{
		dictionary = retrieveDictionary(&bundle);
		if (dictionary == (char *) &bundle)
		{
			putErrmsg("Can't retrieve dictionary.", NULL);
			sdr_cancel_xn(bpSdr);
			return -1;
		}

		bundle.statusRpt.flags |= BP_FORWARDED_RPT;
		getCurrentDtnTime(&bundle.statusRpt.forwardTime);
		result = sendStatusRpt(&bundle, dictionary);
		releaseDictionary(dictionary);
		if (result < 0)
		{
			putErrmsg("Can't send status report.", NULL);
			sdr_cancel_xn(bpSdr);
			return -1;
		}
	}

	/*	At this point the bundle object is subject to
	 *	destruction unless the bundle is pending delivery,
	 *	the bundle is pending another transmission, or custody
	 *	of the bundle has been accepted.  Note that the
	 *	bundle's *payload* object won't be destroyed until
	 *	the calling CLO function destroys bundleZco; that's
	 *	the remaining reference to this reference-counted
	 *	object, even after the reference inside the bundle
	 *	object is destroyed.					*/

	if (bpDestroyBundle(bundleAddr, 0) < 0)
	{
		putErrmsg("Failed trying to destroy bundle.", NULL);
		sdr_cancel_xn(bpSdr);
		return -1;
	}

	if (sdr_end_xn(bpSdr) < 0)
	{
		putErrmsg("Can't handle transmission success.", NULL);
		return -1;
	}

	return 0;
}

int	bpHandleXmitFailure(Object bundleZco)
{
	Sdr	bpSdr = getIonsdr();
	Object	bundleAddr;
	Bundle	bundle;

	CHKERR(sdr_begin_xn(bpSdr));
	if (retrieveInTransitBundle(bundleZco, &bundleAddr) < 0)
	{
		sdr_cancel_xn(bpSdr);
		putErrmsg("Can't locate bundle for failed transmission.", NULL);
		return -1;
	}

	if (bundleAddr == 0)	/*	Bundle not found.		*/
	{
		if (sdr_end_xn(bpSdr) < 0)
		{
			putErrmsg("Failed handling xmit failure.", NULL);
			return -1;
		}

		return 0;	/*	No bundle, can't retransmit.	*/
	}

	sdr_read(bpSdr, (char *) &bundle, bundleAddr, sizeof(Bundle));

	/*	Note that the "timeout" statistics count failures of
	 *	convergence-layer transmission of bundles for which
	 *	stewardship was accepted.  This has nothing to do
	 *	with custody transfer.					*/

	if ((_bpvdb(NULL))->watching & WATCH_timeout)
	{
		putchar('#');
		fflush(stdout);
	}

	if (bpReforwardBundle(bundleAddr) < 0)
	{
		putErrmsg("Failed trying to re-forward bundle.", NULL);
		sdr_cancel_xn(bpSdr);
		return -1;
	}

	if (sdr_end_xn(bpSdr) < 0)
	{
		putErrmsg("Can't handle transmission failure.", NULL);
		return -1;
	}

	return 0;
}

int	bpReforwardBundle(Object bundleAddr)
{
	Sdr	bpSdr = getIonsdr();
	Bundle	bundle;
	char	*dictionary;
	char	*eidString;
	int	result;

	CHKERR(ionLocked());
	CHKERR(bundleAddr);
	sdr_stage(bpSdr, (char *) &bundle, bundleAddr, sizeof(Bundle));
	if (bundle.extendedCOS.flags & BP_MINIMUM_LATENCY)
	{
		/*	If the bundle is critical it has already
		 *	been queued for transmission on all possible
		 *	routes; re-forwarding would only delay
		 *	transmission.  So return immediately.
		 *
		 *	Some additional remarks on this topic:
		 *
		 *	The BP_MINIMUM_LATENCY extended-COS flag is all
		 *	about minimizing latency -- not about assuring
		 *	delivery.  If a critical bundle were reforwarded
		 *	due to (for example) custody refusal, it would
		 *	likely end up being re-queued for multiple
		 *	outducts once again; frequent custody refusals
		 *	could result in an exponential proliferation of
		 *	transmission references for this bundle,
		 *	severely reducing bandwidth utilization and
		 *	potentially preventing the transmission of other
		 *	bundles of equal or greater importance and/or
		 *	urgency.
		 *
		 *	If delivery of a given bundle is important,
		 *	then use high priority and/or a very long TTL
		 *	(to minimize the chance of it failing trans-
		 *	mission due to contact truncation) and use
		 *	custody transfer, and possibly also flag for
		 *	end-to-end acknowledgement at the application
		 *	layer.  If delivery of a given bundle is
		 *	urgent, then use high priority and flag it for
		 *	minimum latency.  If delivery of a given bundle
		 *	is both important and urgent, *send it twice*
		 *	-- once with the high-urgency QOS markings
		 *	and then again with the high-importance QOS
		 *	markings.					*/

		return 0;
	}

	/*	Non-critical bundle, so let's compute another route
	 *	for it.							*/

	purgeStationsStack(&bundle);
	if (bundle.ductXmitElt)
	{
		purgeDuctXmitElt(&bundle, bundleAddr);
	}

	if (bundle.proxNodeEid)
	{
		sdr_free(bpSdr, bundle.proxNodeEid);
		bundle.proxNodeEid = 0;
	}

	if (bundle.destDuctName)
	{
		sdr_free(bpSdr, bundle.destDuctName);
		bundle.destDuctName = 0;
	}

	if (bundle.overdueElt)
	{
		destroyBpTimelineEvent(bundle.overdueElt);
		bundle.overdueElt = 0;
	}

	if (bundle.ctDueElt)
	{
		destroyBpTimelineEvent(bundle.ctDueElt);
		bundle.ctDueElt = 0;
	}

	sdr_write(bpSdr, bundleAddr, (char *) &bundle, sizeof(Bundle));
	if ((dictionary = retrieveDictionary(&bundle)) == (char *) &bundle)
	{
		putErrmsg("Can't retrieve dictionary.", NULL);
		return -1;
	}

	if (printEid(&bundle.destination, dictionary, &eidString) < 0)
	{
		putErrmsg("Can't print destination EID.", NULL);
		return -1;
	}

	bpDbTally(BP_DB_REQUEUED_FOR_FWD, bundle.payload.length);
	result = forwardBundle(bundleAddr, &bundle, eidString);
	MRELEASE(eidString);
	releaseDictionary(dictionary);
	return result;
}

/*	*	*	Admin endpoint managment functions	*	*/

/*	This function provides standard functionality for an
 *	application task that receives and handles adminstrative
 *	bundles.							*/

static BpSAP	_bpadminSap(BpSAP *newSap)
{
	void	*value;
	BpSAP	sap;

	if (newSap)			/*	Add task variable.	*/
	{
		value = (void *) (*newSap);
		sap = (BpSAP) sm_TaskVar(&value);
	}
	else				/*	Retrieve task variable.	*/
	{
		sap = (BpSAP) sm_TaskVar(NULL);
	}

	return sap;
}

static void	shutDownAdminApp()
{
	isignal(SIGTERM, shutDownAdminApp);
	sm_SemEnd((_bpadminSap(NULL))->recvSemaphore);
}

static int	defaultSrh(BpDelivery *dlv, BpStatusRpt *rpt)
{
	return 0;
}

static int	defaultCsh(BpDelivery *dlv, BpCtSignal *cts)
{
	return 0;
}

static void	noteSnub(Bundle *bundle, Object bundleAddr, char *neighborEid)
{
	IonVdb		*ionVdb;
	IonNode		*node;
	PsmAddress	nextNode;
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;

	if (bundle->destination.cbhe == 0	/*	No node nbr.	*/
	|| bundle->destination.unicast == 0	/*	Multicast.	*/
	|| (bundle->extendedCOS.flags & BP_MINIMUM_LATENCY))
	{
		/*	For non-cbhe or multicast bundles we have no
		 *	node number, so we can't manage routing snubs.
		 *	If the bundle is critical then it was already
		 *	sent on all possible routes, so there's no
		 *	point in responding to the routing snub.	*/

		return;
	}

	/*	Reopen the option of routing back to the neighbor
	 *	from which we orginally received the bundle.		*/

	bundle->returnToSender = 1;
	sdr_write(getIonsdr(), bundleAddr, (char *) bundle, sizeof(Bundle));

	/*	Find the affected destination node.			*/

	ionVdb = getIonVdb();
	node = findNode(ionVdb, bundle->destination.c.nodeNbr, &nextNode);
	if (node == NULL)
	{
		return;		/*	Weird, but let it go for now.	*/
	}

	/*	Figure out who the snubbing neighbor is and create a
	 *	Snub object to prevent future routing to that neighbor.	*/

	if (parseEidString(neighborEid, &metaEid, &vscheme, &vschemeElt) == 0
	|| !metaEid.cbhe)	/*	Non-CBHE, somehow.		*/
	{
		return;		/*	Can't construct a Snub.		*/
	}

	oK(addSnub(node, metaEid.nodeNbr));
}

static void	forgetSnub(Bundle *bundle, Object bundleAddr, char *neighborEid)
{
	IonVdb		*ionVdb;
	IonNode		*node;
	PsmAddress	nextNode;
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;

	if (bundle->destination.cbhe == 0	/*	No node nbr.	*/
	|| bundle->destination.unicast == 0	/*	Multicast.	*/
	|| (bundle->extendedCOS.flags & BP_MINIMUM_LATENCY))
	{
		/*	For non-cbhe or multicast bundles we have no
		 *	node number, so we can't manage routing snubs.
		 *	If the bundle is critical then it was already
		 *	sent on all possible routes, so there's no
		 *	point in responding to the routing snub.	*/

		return;
	}

	/*	Find the affected destination node.			*/

	ionVdb = getIonVdb();
	node = findNode(ionVdb, bundle->destination.c.nodeNbr, &nextNode);
	if (node == NULL)
	{
		return;		/*	Weird, but let it go for now.	*/
	}

	/*	If there are no longer any snubs for this destination,
	 *	don't bother to look for this one.			*/

	if (sm_list_length(getIonwm(), node->snubs) == 0)
	{
		return;
	}

	/*	Figure out who the non-snubbing neighbor is and remove
	 *	any snub currently registered for that neighbor.	*/

	if (parseEidString(neighborEid, &metaEid, &vscheme, &vschemeElt) == 0
	|| !metaEid.cbhe)	/*	Non-CBHE, somehow.		*/
	{
		return;		/*	Can't locate any Snub.		*/
	}

	removeSnub(node, metaEid.nodeNbr);
}

int	applyCtSignal(BpCtSignal *cts, char *bundleSourceEid)
{
	Sdr		bpSdr = getIonsdr();
	BpVdb		*bpvdb = _bpvdb(NULL);
	Object		bundleAddr;
	Bundle		bundleBuf;
	Bundle		*bundle = &bundleBuf;
	char		*dictionary;
	char		*eidString;
	int		result;

	CHKERR(sdr_begin_xn(bpSdr));
	if (findBundle(cts->sourceEid, &cts->creationTime, cts->fragmentOffset,
			cts->fragmentLength, &bundleAddr) < 0)
	{
		sdr_exit_xn(bpSdr);
		putErrmsg("Can't fetch bundle.", NULL);
		return -1;
	}

	if (bundleAddr == 0)	/*	Bundle not found or not unique.	*/
	{
		/*	No such bundle; ignore CTS.			*/

		sdr_exit_xn(bpSdr);
		return 0;
	}

	/*	If custody was accepted, or if custody was refused
	 *	due to redundant reception (meaning the receiver had
	 *	previously accepted custody and we just never got the
	 *	signal) we destroy the copy of the bundle retained
	 *	here.  Otherwise we immediately re-dispatch the
	 *	bundle, hoping that a change in the condition of
	 *	the network (reduced congestion, revised routing)
	 *	has occurred since the previous transmission so that
	 *	re-transmission will succeed.				*/

	sdr_stage(bpSdr, (char *) bundle, bundleAddr, sizeof(Bundle));
	if (cts->succeeded || cts->reasonCode == CtRedundantReception)
	{
		if (bpvdb->watching & WATCH_m)
		{
			putchar('m');
                        fflush(stdout);
		}

		forgetSnub(bundle, bundleAddr, bundleSourceEid);
		releaseCustody(bundleAddr, bundle);
		if (bpDestroyBundle(bundleAddr, 0) < 0)
		{
			putErrmsg("Can't destroy bundle.", NULL);
			sdr_cancel_xn(bpSdr);
			return -1;
		}
	}
	else	/*	Custody refused; try again.			*/
	{
		noteSnub(bundle, bundleAddr, bundleSourceEid);
		if ((dictionary = retrieveDictionary(bundle))
				== (char *) bundle)
		{
			putErrmsg("Can't retrieve dictionary.", NULL);
			sdr_cancel_xn(bpSdr);
			return -1;
		}

		if (printEid(&bundle->destination, dictionary, &eidString) < 0)
		{
			putErrmsg("Can't print dest EID.", NULL);
			sdr_cancel_xn(bpSdr);
			return -1;
		}

		result = forwardBundle(bundleAddr, bundle, eidString);
		MRELEASE(eidString);
		releaseDictionary(dictionary);
		if (result < 0)
		{
			putErrmsg("Can't re-queue bundle for forwarding.",
					NULL);
			sdr_cancel_xn(bpSdr);
			return -1;
		}

		if (bpvdb->watching & WATCH_refusal)
		{
			putchar('&');
                        fflush(stdout);
		}
	}

	if (sdr_end_xn(bpSdr) < 0)
	{
		putErrmsg("Can't handle custody signal.", NULL);
		return -1;
	}

	return 0;
}

int	_handleAdminBundles(char *adminEid, StatusRptCB handleStatusRpt,
		CtSignalCB handleCtSignal)
{
	int		running = 1;
	BpSAP		sap;
	BpDelivery	dlv;
	int		adminRecType;
	BpStatusRpt	rpt;
	BpCtSignal	cts;
	void		*other;	/*	Non-standard admin record.	*/
	int		result;

	CHKERR(adminEid);
	if (handleStatusRpt == NULL)
	{
		handleStatusRpt = defaultSrh;
	}

	if (handleCtSignal == NULL)
	{
		handleCtSignal = defaultCsh;
	}

	if (bp_open(adminEid, &sap) < 0)
	{
		putErrmsg("Can't open admin endpoint.", adminEid);
		return -1;
	}

	oK(_bpadminSap(&sap));
	isignal(SIGTERM, shutDownAdminApp);
	while (running && !(sm_SemEnded(sap->recvSemaphore)))
	{
		if (bp_receive(sap, &dlv, BP_BLOCKING) < 0)
		{
			putErrmsg("Admin bundle reception failed.", NULL);
			running = 0;
			continue;
		}

		switch (dlv.result)
		{
		case BpPayloadPresent:
			break;

		case BpEndpointStopped:
			running = 0;

			/*	Intentional fall-through to default.	*/

		default:
			continue;
		}

		if (dlv.adminRecord == 0)
		{
			bp_release_delivery(&dlv, 1);
			continue;
		}

		switch (parseAdminRecord(&adminRecType, &rpt, &cts, &other,
				dlv.adu))
		{
		case 1: 			/*	No problem.	*/
			break;

		case 0:				/*	Parsing failed.	*/
			putErrmsg("Malformed admin record.", NULL);
			bp_release_delivery(&dlv, 1);
			continue;

		default:			/*	System failure.	*/
			putErrmsg("Failed parsing admin record.", NULL);
			running = 0;
			bp_release_delivery(&dlv, 1);
			continue;
		}

		switch (adminRecType)
		{
		case 1:		/*	Status report.			*/
			if (handleStatusRpt(&dlv, &rpt) < 0)
			{
				putErrmsg("Status report handler failed.",
						NULL);
				running = 0;
			}

			bpEraseStatusRpt(&rpt);
			break;			/*	Out of switch.	*/

		case 2:		/*	Custody signal.			*/

			/*	Node-defined handler is given a
			 *	chance to respond to the custody signal
			 *	before the standard procedures are
			 *	invoked; it can, for example, adjust
			 *	routing tables.  If the handler fails,
			 *	it aborts handling of this custody
			 *	signal and all subsequent administrative
			 *	bundles.				*/

			if (handleCtSignal(&dlv, &cts) < 0)
			{
				putErrmsg("Custody signal handler failed",
						NULL);
				running = 0;
				bpEraseCtSignal(&cts);
				break;		/*	Out of switch.	*/
			}

			if (applyCtSignal(&cts, dlv.bundleSourceEid) < 0)
			{
				putErrmsg("Failed applying custody signal.",
					       NULL);
				running = 0;
			}

			bpEraseCtSignal(&cts);
			break;			/*	Out of switch.	*/

		default:	/*	Unknown or non-standard.	*/
			result = applyACS(adminRecType, other, &dlv,
					handleCtSignal);
			if (result != -2)	/*	Applied record.	*/
			{
				if (result < 0)
				{
					running = 0;
				}

				break;		/*	Out of switch.	*/
			}

			result = applyImcPetition(adminRecType, other, &dlv);
			if (result != -2)	/*	Applied record.	*/
			{
				if (result < 0)
				{
					running = 0;
				}

				break;		/*	Out of switch.	*/
			}

			/*	Unknown admin record type.		*/

			break;			/*	Out of switch.	*/
		}

		bp_release_delivery(&dlv, 1);

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	writeMemo("[i] Administrative endpoint terminated.");
	writeErrmsgMemos();
	return 0;
}

int	eidIsLocal(EndpointId eid, char* dictionary)
{
	VScheme		*vscheme;
	VEndpoint	*vpoint;
	int		 result = 0;

	lookUpEidScheme(eid, dictionary, &vscheme);
	if (vscheme != NULL)	/*	Destination might be local.	*/
	{
		lookUpEndpoint(eid, dictionary, vscheme, &vpoint);
		if (vpoint != NULL)	/*	Destination is here.	*/
		{
			result = 1;
		}
	}

	return result;
}
