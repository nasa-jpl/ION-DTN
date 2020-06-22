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
 *	09-04-19  SCB	Revision per version 7 of Bundle Protocol spec.
 */

#include "bpP.h"
#include "sdrhash.h"
#include "smrbt.h"
#include "bei.h"
#include "eureka.h"
#include "bibe.h"

/*	Interfaces to other BP-related components of ION	*	*/

#include "imcP.h"
#include "saga.h"
#include "bpsec_instr.h"
#include "bpsec_util.h"

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

/*	We hitchhike on the ZCO heap space management system to 
 *	manage the space occupied by Bundle objects.  In effect,
 *	the Bundle overhead objects compete with ZCOs for available
 *	SDR heap space.  We don't want this practice to become
 *	widespread, which is why these functions are declared
 *	privately here rather than publicly in the zco.h header.	*/

extern void	zco_increase_heap_occupancy(Sdr sdr, vast delta, ZcoAcct acct);
extern void	zco_reduce_heap_occupancy(Sdr sdr, vast delta, ZcoAcct acct);

static BpVdb	*_bpvdb(char **);

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

int	endpointIsLocal(EndpointId eid)
{
	VScheme		*vscheme;
	VEndpoint	*vpoint;
	int		 result = 0;

	lookUpEidScheme(&eid, &vscheme);
	if (vscheme != NULL)	/*	Destination might be local.	*/
	{
		lookUpEndpoint(&eid, vscheme, &vpoint);
		if (vpoint != NULL)	/*	Destination is here.	*/
		{
			result = 1;
		}
	}

	return result;
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

void	bpPlanTally(VPlan *vplan, unsigned int idx, unsigned int size)
{
	Sdr		sdr = getIonsdr();
	PlanStats	stats;
	Tally		*tally;
	int		offset;

	CHKVOID(vplan && vplan->stats);
	if (!(vplan->updateStats))
	{
		return;
	}

	CHKVOID(ionLocked());
	CHKVOID(idx < BP_PLAN_STATS);
	sdr_stage(sdr, (char *) &stats, vplan->stats, sizeof(PlanStats));
	tally = stats.tallies + idx;
	tally->totalCount += 1;
	tally->totalBytes += size;
	tally->currentCount += 1;
	tally->currentBytes += size;
	offset = (char *) tally - ((char *) &stats);
	sdr_write(sdr, vplan->stats + offset, (char *) tally, sizeof(Tally));
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

void	bpDelTally(unsigned int reason)
{
	Sdr		sdr = getIonsdr();
	BpVdb		*vdb = getBpVdb();
	BpDelStats	stats;

	CHKVOID(vdb && vdb->delStats);
	if (!(vdb->updateStats))
	{
		return;
	}

	CHKVOID(ionLocked());
	CHKVOID(reason < BP_REASON_STATS);
	sdr_stage(sdr, (char *) &stats, vdb->delStats, sizeof(BpDelStats));
	stats.totalDelByReason[reason] += 1;
	stats.currentDelByReason[reason] += 1;
	sdr_write(sdr, vdb->delStats, (char *) &stats, sizeof(BpDelStats));
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
	Sdr		sdr = getIonsdr();
	PsmPartition	bpwm = getIonwm();
	Object		endpointObj;
	Endpoint	endpoint;
	VEndpoint	*vpoint;
	PsmAddress	vpointElt;
	PsmAddress	addr;

	endpointObj = sdr_list_data(sdr, endpointElt);
	sdr_read(sdr, (char *) &endpoint, endpointObj, sizeof(Endpoint));

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
		sm_SemEnd(vpoint->semaphore);
		microsnooze(50000);
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
		sm_SemGive(vscheme->semaphore);
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
	Sdr		sdr = getIonsdr();
	PsmPartition	bpwm = getIonwm();
	Object		schemeObj;
	Scheme		scheme;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	PsmAddress	addr;
	char		hostNameBuf[MAXHOSTNAMELEN + 1];
	Object		elt;

	schemeObj = sdr_list_data(sdr, schemeElt);
	sdr_read(sdr, (char *) &scheme, schemeObj, sizeof(Scheme));
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
	vscheme->codeNumber = scheme.codeNumber;

	/*	Compute admin EID for this scheme.			*/

	if (vscheme->codeNumber != imc)
	{
		if (vscheme->codeNumber == ipn)
		{
			isprintf(vscheme->adminEid, sizeof vscheme->adminEid,
				"%.8s:" UVAST_FIELDSPEC ".0", vscheme->name,
				getOwnNodeNbr());
		}
		else	/*	Assume it's dtn.			*/
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
	}

	vscheme->endpoints = sm_list_create(bpwm);
	if (vscheme->endpoints == 0)
	{
		oK(sm_list_delete(bpwm, vschemeElt, NULL, NULL));
		psm_free(bpwm, addr);
		return -1;
	}

	for (elt = sdr_list_first(sdr, scheme.endpoints); elt;
			elt = sdr_list_next(sdr, elt))
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
		sm_SemEnd(vscheme->semaphore);
		microsnooze(50000);
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
	Sdr		sdr = getIonsdr();
	MetaEid		metaEid;
	VScheme		*vscheme2;
	PsmAddress	vschemeElt;
	VEndpoint	*vpoint;
	PsmAddress	vpointElt;
	Scheme		scheme;
	char		cmdString[SDRSTRING_BUFSZ];

	if (vscheme->codeNumber != imc)
	{
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

	sdr_read(sdr, (char *) &scheme, sdr_list_data(sdr,
			vscheme->schemeElt), sizeof(Scheme));
	if (vscheme->fwdPid == ERROR
	|| sm_TaskExists(vscheme->fwdPid) == 0)
	{
		if (scheme.fwdCmd != 0)
		{
			sdr_string_read(sdr, cmdString, scheme.fwdCmd);
			vscheme->fwdPid = pseudoshell(cmdString);
		}
	}

	if (vscheme->admAppPid == ERROR
	|| sm_TaskExists(vscheme->admAppPid) == 0)
	{
		if (scheme.admAppCmd != 0)
		{
			sdr_string_read(sdr, cmdString, scheme.admAppCmd);
			vscheme->admAppPid = pseudoshell(cmdString);
		}
	}
}

static void	stopScheme(VScheme *vscheme)
{
	PsmPartition	bpwm = getIonwm();
	PsmAddress	elt;
	PsmAddress	endpointAddr;
	VEndpoint	*vpoint;

	/*	Ending the semaphore stops the forwarder task
	 *	(vscheme->fwdPid).					*/

	if (vscheme->semaphore != SM_SEM_NONE)
	{
		sm_SemEnd(vscheme->semaphore);
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

	/*	Don't try to stop the adminep daemon until AFTER
	 *	all endpoint semaphores are ended, because it is
	 *	almost always pended on one of those semaphores.	*/

	if (vscheme->admAppPid != ERROR)
	{
		sm_TaskKill(vscheme->admAppPid, SIGTERM);
	}
}

static void	waitForScheme(VScheme *vscheme)
{
	if (vscheme->fwdPid != ERROR)
	{
		while (sm_TaskExists(vscheme->fwdPid))
		{
			microsnooze(100000);
		}
	}

	if (vscheme->admAppPid != ERROR)
	{
		while (sm_TaskExists(vscheme->admAppPid))
		{
			microsnooze(100000);
		}
	}
}

static void	resetPlan(VPlan *vplan)
{
	if (vplan->semaphore == SM_SEM_NONE)
	{
		vplan->semaphore = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	}
	else
	{
		sm_SemUnend(vplan->semaphore);
		sm_SemGive(vplan->semaphore);
	}

	sm_SemTake(vplan->semaphore);			/*	Lock.	*/
	vplan->clmPid = ERROR;
}

static int	raisePlan(Object planElt, BpVdb *bpvdb)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	bpwm = getIonwm();
	Object		planObj;
	BpPlan		plan;
	PsmAddress	elt;
	VPlan		*vplan;
	int		result;
	PsmAddress	addr;

	planObj = sdr_list_data(sdr, planElt);
	sdr_read(sdr, (char *) &plan, planObj, sizeof(BpPlan));
	for (elt = sm_list_first(bpwm, bpvdb->plans); elt;
			elt = sm_list_next(bpwm, elt))
	{
		vplan = (VPlan *) psp(bpwm, sm_list_data(bpwm, elt));
		result = strcmp(vplan->neighborEid, plan.neighborEid);
		if (result < 0)
		{
			continue;
		}

		if (result > 0)		/*	Insert before this one.	*/
		{
			break;		/*	Same as end of list.	*/
		}

		return 0;		/*	Already raised.		*/
	}

	addr = psm_malloc(bpwm, sizeof(VPlan));
	if (addr == 0)
	{
		return -1;
	}

	if (elt)
	{
		elt = sm_list_insert_before(bpwm, elt, addr);
	}
	else
	{
		elt = sm_list_insert_last(bpwm, bpvdb->plans, addr);
	}

	if (elt == 0)
	{
		psm_free(bpwm, addr);
		return -1;
	}

	vplan = (VPlan *) psp(bpwm, addr);
	memset((char *) vplan, 0, sizeof(VPlan));
	vplan->planElt = planElt;
	vplan->stats = plan.stats;
	vplan->updateStats = plan.updateStats;
	istrcpy(vplan->neighborEid, plan.neighborEid, sizeof plan.neighborEid);
	vplan->neighborNodeNbr = plan.neighborNodeNbr;
	vplan->semaphore = SM_SEM_NONE;
	vplan->xmitThrottle.nominalRate = plan.nominalRate;
	vplan->xmitThrottle.capacity = plan.nominalRate;
	resetPlan(vplan);
	return 0;
}

static void	dropPlan(VPlan *vplan, PsmAddress vplanElt)
{
	PsmPartition	bpwm = getIonwm();
	PsmAddress	vplanAddr;

	vplanAddr = sm_list_data(bpwm, vplanElt);
	if (vplan->semaphore != SM_SEM_NONE)
	{
		sm_SemEnd(vplan->semaphore);
		microsnooze(50000);
		sm_SemDelete(vplan->semaphore);
	}

	oK(sm_list_delete(bpwm, vplanElt, NULL, NULL));
	psm_free(bpwm, vplanAddr);
}

static void	startPlan(VPlan *vplan)
{
	char	cmdString[6 + MAX_EID_LEN + 1];

	if (vplan->clmPid == ERROR || sm_TaskExists(vplan->clmPid) == 0)
	{
		isprintf(cmdString, sizeof cmdString, "bpclm %s",
				vplan->neighborEid);
		vplan->clmPid = pseudoshell(cmdString);
	}
}

static void	stopPlan(VPlan *vplan)
{
	/*	Ending the semaphore stops the bpclm daemon
	 *	(vplan->clmPid).					*/

	if (vplan->semaphore != SM_SEM_NONE)
	{
		sm_SemEnd(vplan->semaphore);
	}
}

static void	waitForPlan(VPlan *vplan)
{
	if (vplan->clmPid != ERROR)
	{
		while (sm_TaskExists(vplan->clmPid))
		{
			microsnooze(100000);
		}
	}
}

static void	resetInduct(VInduct *vduct)
{
	vduct->cliPid = ERROR;
}

static int	raiseInduct(Object inductElt, BpVdb *bpvdb)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	bpwm = getIonwm();
	Object		inductObj;
	Induct		duct;
	ClProtocol	protocol;
	VInduct		*vduct;
	PsmAddress	vductElt;
	PsmAddress	addr;

	inductObj = sdr_list_data(sdr, inductElt);
	sdr_read(sdr, (char *) &duct, inductObj, sizeof(Induct));
	sdr_read(sdr, (char *) &protocol, duct.protocol, sizeof(ClProtocol));
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
	resetInduct(vduct);
	return 0;
}

static void	dropInduct(VInduct *vduct, PsmAddress vductElt)
{
	PsmPartition	bpwm = getIonwm();
	PsmAddress	vductAddr;

	vductAddr = sm_list_data(bpwm, vductElt);
	oK(sm_list_delete(bpwm, vductElt, NULL, NULL));
	psm_free(bpwm, vductAddr);
}

static void	startInduct(VInduct *vduct)
{
	Sdr	sdr = getIonsdr();
	Induct	induct;
	char	cmd[SDRSTRING_BUFSZ];
	char	cmdString[SDRSTRING_BUFSZ + 1 + MAX_CL_DUCT_NAME_LEN + 1];

	sdr_read(sdr, (char *) &induct, sdr_list_data(sdr,
			vduct->inductElt), sizeof(Induct));
	if (vduct->cliPid == ERROR || sm_TaskExists(vduct->cliPid) == 0)
	{
		if (induct.cliCmd != 0)
		{
			sdr_string_read(sdr, cmd, induct.cliCmd);
			isprintf(cmdString, sizeof cmdString, "%s %s", cmd,
					induct.name);
			vduct->cliPid = pseudoshell(cmdString);
		}
	}
}

static void	stopInduct(VInduct *vduct)
{
	if (vduct->cliPid != ERROR)
	{
		sm_TaskKill(vduct->cliPid, SIGTERM);
	}
}

static void	waitForInduct(VInduct *vduct)
{
	if (vduct->cliPid != ERROR)
	{
		while (sm_TaskExists(vduct->cliPid))
		{
			microsnooze(100000);
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
		sm_SemGive(vduct->semaphore);
	}

	sm_SemTake(vduct->semaphore);			/*	Lock.	*/
}

static int	raiseOutduct(Object outductElt, BpVdb *bpvdb)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	bpwm = getIonwm();
	Object		outductObj;
	Outduct		duct;
	ClProtocol	protocol;
	VOutduct	*vduct;
	PsmAddress	vductElt;
	PsmAddress	addr;

	outductObj = sdr_list_data(sdr, outductElt);
	sdr_read(sdr, (char *) &duct, outductObj, sizeof(Outduct));
	sdr_read(sdr, (char *) &protocol, duct.protocol, sizeof(ClProtocol));
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
	vduct->cloPid = ERROR;
	vduct->outductElt = outductElt;
	istrcpy(vduct->protocolName, protocol.name, sizeof vduct->protocolName);
	istrcpy(vduct->ductName, duct.name, sizeof vduct->ductName);
	vduct->semaphore = SM_SEM_NONE;
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
		sm_SemEnd(vduct->semaphore);
		microsnooze(50000);
		sm_SemDelete(vduct->semaphore);
	}

	oK(sm_list_delete(bpwm, vductElt, NULL, NULL));
	psm_free(bpwm, vductAddr);
}

static void	startOutduct(VOutduct *vduct)
{
	Sdr	sdr = getIonsdr();
	Outduct	outduct;
	char	cmd[SDRSTRING_BUFSZ];
	char	cmdString[SDRSTRING_BUFSZ + 1 + MAX_CL_DUCT_NAME_LEN + 1];

	sdr_read(sdr, (char *) &outduct, sdr_list_data(sdr,
			vduct->outductElt), sizeof(Outduct));
	if (vduct->cloPid == ERROR || sm_TaskExists(vduct->cloPid) == 0)
	{
		if (outduct.cloCmd != 0)
		{
			sdr_string_read(sdr, cmd, outduct.cloCmd);
			isprintf(cmdString, sizeof cmdString, "%s %s", cmd,
					outduct.name);
			vduct->cloPid = pseudoshell(cmdString);
		}
	}
}

static void	stopOutduct(VOutduct *vduct)
{
	/*	Ending the semaphore stops the clo daemon
	 *	(vduct->cloPid).					*/

	if (vduct->semaphore != SM_SEM_NONE)
	{
		sm_SemEnd(vduct->semaphore);
	}
}

static void	waitForOutduct(VOutduct *vduct)
{
	microsnooze(100000);	/*	Maybe thread stops.		*/
	if (vduct->hasThread)
	{
		/*	Duct is drained by a thread rather than a
		 *	process.  Wait until it stops.			*/

		if (pthread_kill(vduct->cloThread, SIGCONT) == 0)
		{
			pthread_join(vduct->cloThread, NULL);
		}

		vduct->hasThread = 0;
	}

	if (vduct->cloPid == ERROR)
	{
		return;
	}

	/*	Duct is being drained by a process.			*/

	while (sm_TaskExists(vduct->cloPid))
	{
		microsnooze(100000);
	}

	vduct->cloPid = ERROR;
}

static int	raiseProtocol(Address protocolAddr, BpVdb *bpvdb)
{
	Sdr	sdr = getIonsdr();
	BpDB	*bpConstants = _bpConstants();
	Object	elt;
		OBJ_POINTER(Induct, induct);
		OBJ_POINTER(Outduct, outduct);

	for (elt = sdr_list_first(sdr, bpConstants->inducts); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, Induct, induct,
				sdr_list_data(sdr, elt));
		if (induct->protocol != protocolAddr)
		{
			continue;	/*	For different protocol.	*/
		}

		if (raiseInduct(elt, bpvdb) < 0)
		{
			putErrmsg("Can't raise induct.", NULL);
			return -1;
		}
	}

	for (elt = sdr_list_first(sdr, bpConstants->outducts); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, Outduct, outduct,
				sdr_list_data(sdr, elt));
		if (outduct->protocol != protocolAddr)
		{
			continue;	/*	For different protocol.	*/
		}
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
		vdb->delStats = db->delStats;
		vdb->dbStats = db->dbStats;
		vdb->updateStats = db->updateStats;
		vdb->bundleCounter = 0;
		vdb->clockPid = ERROR;
		vdb->transitSemaphore = SM_SEM_NONE;
		vdb->transitPid = ERROR;
		vdb->watching = db->watching;
		if ((vdb->schemes = sm_list_create(wm)) == 0
		|| (vdb->plans = sm_list_create(wm)) == 0
		|| (vdb->inducts = sm_list_create(wm)) == 0
		|| (vdb->outducts = sm_list_create(wm)) == 0
		|| (vdb->discoveries = sm_list_create(wm)) == 0
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

		/*	Raise all egress plans.				*/

		for (sdrElt = sdr_list_first(sdr, db->plans); sdrElt;
				sdrElt = sdr_list_next(sdr, sdrElt))
		{
			if (raisePlan(sdrElt, vdb) < 0)
			{
				sdr_exit_xn(sdr);
				putErrmsg("Can't raise all plans.", NULL);
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
	Sdr		sdr;
	Object		bpdbObject;
	BpDB		bpdbBuf;
	BpCosStats	cosStatsInit;
	BpDelStats	delStatsInit;
	BpDbStats	dbStatsInit;
	char		*bpvdbName = _bpvdbName();

	if (ionAttach() < 0)
	{
		putErrmsg("BP can't attach to ION.", NULL);
		return -1;
	}

	writeMemo("[i] This node deploys bundle protocol version 7.");
	sdr = getIonsdr();

	/*	Recover the BP database, creating it if necessary.	*/

	CHKERR(sdr_begin_xn(sdr));
	bpdbObject = sdr_find(sdr, _bpdbName(), NULL);
	switch (bpdbObject)
	{
	case -1:		/*	SDR error.			*/
		putErrmsg("Can't search for BP database in SDR.", NULL);
		sdr_cancel_xn(sdr);
		return -1;

	case 0:			/*	Not found; must create new DB.	*/
		bpdbObject = sdr_malloc(sdr, sizeof(BpDB));
		if (bpdbObject == 0)
		{
			putErrmsg("No space for database.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}

		/*	Initialize the non-volatile database.		*/

		memset((char *) &bpdbBuf, 0, sizeof(BpDB));
		bpdbBuf.schemes = sdr_list_create(sdr);
		bpdbBuf.plans = sdr_list_create(sdr);
		bpdbBuf.protocols = sdr_list_create(sdr);
		bpdbBuf.inducts = sdr_list_create(sdr);
		bpdbBuf.outducts = sdr_list_create(sdr);
		bpdbBuf.saga[0] = sdr_list_create(sdr);
		bpdbBuf.saga[1] = sdr_list_create(sdr);
		bpdbBuf.timeline = sdr_list_create(sdr);
		bpdbBuf.bundles = sdr_hash_create(sdr,
				BUNDLES_HASH_KEY_LEN,
				BUNDLES_HASH_ENTRIES,
				BUNDLES_HASH_SEARCH_LEN);
		bpdbBuf.inboundBundles = sdr_list_create(sdr);
		bpdbBuf.limboQueue = sdr_list_create(sdr);
		bpdbBuf.transit = sdr_list_create(sdr);
		bpdbBuf.clockCmd = sdr_string_create(sdr, "bpclock");
		bpdbBuf.transitCmd = sdr_string_create(sdr, "bptransit");
		bpdbBuf.maxAcqInHeap = 560;
		bpdbBuf.maxBundleCount = (unsigned int) -1;
		bpdbBuf.sourceStats = sdr_malloc(sdr, sizeof(BpCosStats));
		bpdbBuf.recvStats = sdr_malloc(sdr, sizeof(BpCosStats));
		bpdbBuf.discardStats = sdr_malloc(sdr, sizeof(BpCosStats));
		bpdbBuf.xmitStats = sdr_malloc(sdr, sizeof(BpCosStats));
		bpdbBuf.delStats = sdr_malloc(sdr, sizeof(BpDelStats));
		bpdbBuf.dbStats = sdr_malloc(sdr, sizeof(BpDbStats));
		if (bpdbBuf.sourceStats && bpdbBuf.recvStats
		&& bpdbBuf.discardStats && bpdbBuf.xmitStats
		&& bpdbBuf.delStats && bpdbBuf.dbStats)
		{
			memset((char *) &cosStatsInit, 0,
					sizeof(BpCosStats));
			sdr_write(sdr, bpdbBuf.sourceStats,
					(char *) &cosStatsInit,
					sizeof(BpCosStats));
			sdr_write(sdr, bpdbBuf.recvStats,
					(char *) &cosStatsInit,
					sizeof(BpCosStats));
			sdr_write(sdr, bpdbBuf.discardStats,
					(char *) &cosStatsInit,
					sizeof(BpCosStats));
			sdr_write(sdr, bpdbBuf.xmitStats,
					(char *) &cosStatsInit,
					sizeof(BpCosStats));
			memset((char *) &delStatsInit, 0,
					sizeof(BpDelStats));
			sdr_write(sdr, bpdbBuf.delStats,
					(char *) &delStatsInit,
					sizeof(BpDelStats));
			memset((char *) &dbStatsInit, 0,
					sizeof(BpDbStats));
			sdr_write(sdr, bpdbBuf.dbStats,
					(char *) &dbStatsInit,
					sizeof(BpDbStats));
		}

		bpdbBuf.startTime = getCtime();
		bpdbBuf.updateStats = 1;	/*	Default.	*/
		sdr_write(sdr, bpdbObject, (char *) &bpdbBuf, sizeof(BpDB));
		sdr_catlg(sdr, _bpdbName(), 0, bpdbObject);
		if (sdr_end_xn(sdr))
		{
			putErrmsg("Can't create BP database.", NULL);
			return -1;
		}

		break;

	default:		/*	Found DB in the SDR.		*/
		sdr_stage(sdr, (char *) &bpdbBuf, bpdbObject, sizeof(BpDB));
		bpdbBuf.startTime = getCtime();
		sdr_write(sdr, bpdbObject, (char *) &bpdbBuf, sizeof(BpDB));
		if (sdr_end_xn(sdr))
		{
			putErrmsg("Can't reload BP database.", NULL);
			return -1;
		}
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
		bpsec_instr_init();
		writeMemo("[i] Bundle security is enabled.");
	}

	return 0;		/*	BP service is now available.	*/
}

static void	dropVdb(PsmPartition wm, PsmAddress vdbAddress)
{
	BpVdb		*vdb;
	PsmAddress	elt;
	VScheme		*vscheme;
	VPlan		*vplan;
	VInduct		*vinduct;
	VOutduct	*voutduct;

	vdb = (BpVdb *) psp(wm, vdbAddress);
	while ((elt = sm_list_first(wm, vdb->schemes)) != 0)
	{
		vscheme = (VScheme *) psp(wm, sm_list_data(wm, elt));
		dropScheme(vscheme, elt);
	}

	sm_list_destroy(wm, vdb->schemes, NULL, NULL);
	while ((elt = sm_list_first(wm, vdb->plans)) != 0)
	{
		vplan = (VPlan *) psp(wm, sm_list_data(wm, elt));
		dropPlan(vplan, elt);
	}

	sm_list_destroy(wm, vdb->plans, NULL, NULL);
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
	sm_list_destroy(wm, vdb->discoveries, NULL, NULL);
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
	Sdr		sdr = getIonsdr();
	PsmPartition	bpwm = getIonwm();
	BpVdb		*bpvdb = _bpvdb(NULL);
	char		cmdString[SDRSTRING_BUFSZ];
	PsmAddress	elt;

	CHKERR(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/

	/*	Start the bundle expiration clock if necessary.		*/

	if (bpvdb->clockPid == ERROR || sm_TaskExists(bpvdb->clockPid) == 0)
	{
		sdr_string_read(sdr, cmdString, (_bpConstants())->clockCmd);
		bpvdb->clockPid = pseudoshell(cmdString);
	}

	/*	Start the bundle transit daemon if necessary.		*/

	if (bpvdb->transitPid == ERROR || sm_TaskExists(bpvdb->transitPid) == 0)
	{
		sdr_string_read(sdr, cmdString, (_bpConstants())->transitCmd);
		bpvdb->transitPid = pseudoshell(cmdString);
		bpvdb->transitSemaphore = sm_SemCreate(SM_NO_KEY,
				SM_SEM_FIFO);

		/*	Lock transit semaphore.			*/

		sm_SemTake(bpvdb->transitSemaphore);
	}

	/*	Start forwarders and admin endpoints for all schemes.	*/

	for (elt = sm_list_first(bpwm, bpvdb->schemes); elt;
			elt = sm_list_next(bpwm, elt))
	{
		startScheme((VScheme *) psp(bpwm, sm_list_data(bpwm, elt)));
	}

	/*	Start convergence-layer managers for all egress plans.	*/

	for (elt = sm_list_first(bpwm, bpvdb->plans); elt;
			elt = sm_list_next(bpwm, elt))
	{
		startPlan((VPlan *) psp(bpwm, sm_list_data(bpwm, elt)));
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

	sdr_exit_xn(sdr);	/*	Unlock memory.			*/
	return 0;
}

void	bpStop()		/*	Reverses bpStart.		*/
{
	Sdr		sdr = getIonsdr();
	PsmPartition	bpwm = getIonwm();
	BpVdb		*bpvdb = _bpvdb(NULL);
	PsmAddress	elt;
	VScheme		*vscheme;
	VPlan		*vplan;
	VInduct		*vinduct;
	VOutduct	*voutduct;
	Object		zcoElt;
	Object		nextElt;
	Object		zco;

	bpsec_instr_cleanup();

	/*	Tell all BP processes to stop.				*/

	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	for (elt = sm_list_first(bpwm, bpvdb->schemes); elt;
			elt = sm_list_next(bpwm, elt))
	{
		vscheme = (VScheme *) psp(bpwm, sm_list_data(bpwm, elt));
		stopScheme(vscheme);
	}

	for (elt = sm_list_first(bpwm, bpvdb->plans); elt;
			elt = sm_list_next(bpwm, elt))
	{
		vplan = (VPlan *) psp(bpwm, sm_list_data(bpwm, elt));
		stopPlan(vplan);
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
		if (!(voutduct->hasThread))
		{
			stopOutduct(voutduct);
		}
	}

	if (bpvdb->clockPid != ERROR)
	{
		sm_TaskKill(bpvdb->clockPid, SIGTERM);
	}

	sm_SemEnd(bpvdb->transitSemaphore);
	if (bpvdb->transitPid != ERROR)
	{
		sm_TaskKill(bpvdb->transitPid, SIGTERM);
	}

	sdr_exit_xn(sdr);	/*	Unlock memory.			*/

	/*	Wait until all BP processes have stopped.		*/

	for (elt = sm_list_first(bpwm, bpvdb->schemes); elt;
			elt = sm_list_next(bpwm, elt))
	{
		vscheme = (VScheme *) psp(bpwm, sm_list_data(bpwm, elt));
		waitForScheme(vscheme);
	}

	for (elt = sm_list_first(bpwm, bpvdb->plans); elt;
			elt = sm_list_next(bpwm, elt))
	{
		vplan = (VPlan *) psp(bpwm, sm_list_data(bpwm, elt));
		waitForPlan(vplan);
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
		if (!(voutduct->hasThread))
		{
			waitForOutduct(voutduct);
		}
	}

	if (bpvdb->clockPid != ERROR)
	{
		while (sm_TaskExists(bpvdb->clockPid))
		{
			microsnooze(100000);
		}
	}

	if (bpvdb->transitPid != ERROR)
	{
		while (sm_TaskExists(bpvdb->transitPid))
		{
			microsnooze(100000);
		}
	}

	/*	Now erase all the tasks and reset the semaphores.	*/

	CHKVOID(sdr_begin_xn(sdr));
	bpvdb->clockPid = ERROR;
	bpvdb->transitPid = ERROR;
	for (elt = sm_list_first(bpwm, bpvdb->schemes); elt;
			elt = sm_list_next(bpwm, elt))
	{
		vscheme = (VScheme *) psp(bpwm, sm_list_data(bpwm, elt));
		resetScheme(vscheme);
	}

	for (elt = sm_list_first(bpwm, bpvdb->plans); elt;
			elt = sm_list_next(bpwm, elt))
	{
		vplan = (VPlan *) psp(bpwm, sm_list_data(bpwm, elt));
		resetPlan(vplan);
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
		if (!(voutduct->hasThread))
		{
			resetOutduct(voutduct);
		}
	}

	/*	Clear out any partially received bundles, then exit.	*/

	for (zcoElt = sdr_list_first(sdr, (_bpConstants())->inboundBundles);
			zcoElt; zcoElt = nextElt)
	{
		nextElt = sdr_list_next(sdr, zcoElt);
		zco = sdr_list_data(sdr, zcoElt);
		zco_destroy(sdr, zco);
		sdr_list_delete(sdr, zcoElt, NULL, NULL);
	}

	oK(sdr_end_xn(sdr));
}

int	bpAttach()
{
	Object		bpdbObject = _bpdbObject(NULL);
	BpVdb		*bpvdb = _bpvdb(NULL);
	Sdr		sdr;
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

	sdr = getIonsdr();

	/*	Locate the BP database.					*/

	if (bpdbObject == 0)
	{
		CHKERR(sdr_begin_xn(sdr));
		bpdbObject = sdr_find(sdr, _bpdbName(), NULL);
		sdr_exit_xn(sdr);
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

void	bpDetach()
{
	char	*stop = NULL;

	oK(_bpvdb(&stop));
	return;
}

/*	*	*	Database occupancy functions	*	*	*/

void	noteBundleInserted(Bundle *bundle)
{
	CHKVOID(bundle);
	zco_increase_heap_occupancy(getIonsdr(), bundle->dbOverhead,
			bundle->acct);
}

void	noteBundleRemoved(Bundle *bundle)
{
	CHKVOID(bundle);
	zco_reduce_heap_occupancy(getIonsdr(), bundle->dbOverhead,
			bundle->acct);
}

/*	*	*	Useful utility functions	*	*	*/

void	getCurrentDtnTime(DtnTime *dt)
{
	CHKVOID(dt);
	*dt = getCtime() - EPOCH_2000_SEC;		/*	30 yrs	*/
}

Throttle	*applicableThrottle(VPlan *vplan)
{
	Sdr		sdr = getIonsdr();
	Object		planObj;
			OBJ_POINTER(BpPlan, plan);
	IonNeighbor	*neighbor;
	PsmAddress	nextElt;

	CHKNULL(vplan);
	CHKNULL(ionLocked());
	planObj = sdr_list_data(sdr, vplan->planElt);
	GET_OBJ_POINTER(sdr, BpPlan, plan, planObj);
	if (plan->neighborNodeNbr == 0)	/*	No nbr for assigned node.*/
	{
		return &(vplan->xmitThrottle);
	}

	neighbor = findNeighbor(getIonVdb(), plan->neighborNodeNbr, &nextElt);
	if (neighbor == NULL)	/*	Neighbor isn't in contact plan.	*/
	{
		return &(vplan->xmitThrottle);
	}

	return &(neighbor->xmitThrottle);
}

void	computePriorClaims(BpPlan *plan, Bundle *bundle, Scalar *priorClaims,
		Scalar *totalBacklog)
{
	int		priority = bundle->priority;
	VPlan		*vplan;
	PsmAddress	vplanElt;
	Throttle	*throttle;
	vast		committed = 0;
#ifdef ION_BANDWIDTH_RESERVED
	Scalar		limit;
	Scalar		increment;
#endif
	int		i;

	CHKVOID(plan && bundle && priorClaims && totalBacklog);
	findPlan(plan->neighborEid, &vplan, &vplanElt);
	CHKVOID(vplanElt);
	throttle = applicableThrottle(vplan);
	CHKVOID(throttle);

	/*	Prior claims on the first contact along this route
	 *	must include however much transmission the plan
	 *	itself has already committed to (as per bpclm)
	 *	during the current second of operation, pending
	 *	capacity replenishment through the rate control
	 *	mechanism implemented by bpclock.  That commitment
	 *	is given by the applicable throttle's xmit rate
	 *	(multiplied by 1 second) minus its current "capacity"
	 *	(which may be negative).
	 *
	 *	If the applicable throttle is not rate-controlled,
	 *	the commitment volume can't be computed.		*/

	if (throttle->nominalRate > 0)
	{
		committed = throttle->nominalRate - throttle->capacity;
	}

	/*	Since bpclock never increases capacity to a value
	 *	in excess of the nominalRate, committed can never
	 *	be negative.						*/

	loadScalar(totalBacklog, committed);
	addToScalar(totalBacklog, &(plan->urgentBacklog));
	addToScalar(totalBacklog, &(plan->stdBacklog));
	addToScalar(totalBacklog, &(plan->bulkBacklog));
	loadScalar(priorClaims, committed);
	if (priority == 0)
	{
		addToScalar(priorClaims, &(plan->urgentBacklog));
#ifdef ION_BANDWIDTH_RESERVED
		/*	priorClaims increment is the standard
		 *	backlog's prior claims, which is the entire
		 *	standard backlog or twice the bulk backlog,
		 *	whichever is less.				*/

		copyScalar(&limit, &(plan->bulkBacklog));
		multiplyScalar(&limit, 2);
		copyScalar(&increment, &limit);
		subtractFromScalar(&limit, &(plan->stdBacklog));
		if (scalarIsValid(&limit))
		{
			/*	Current standard backlog is less than
			 *	twice the current bulk backlog.		*/

			copyScalar(&increment, &(plan->stdBacklog));
		}

		addToScalar(priorClaims, &increment);
#else
		addToScalar(priorClaims, &(plan->stdBacklog));
#endif
		addToScalar(priorClaims, &(plan->bulkBacklog));
		return;
	}

	if (priority == 1)
	{
		addToScalar(priorClaims, &(plan->urgentBacklog));
		addToScalar(priorClaims, &(plan->stdBacklog));
#ifdef ION_BANDWIDTH_RESERVED
		/*	priorClaims increment is the bulk backlog's
		 *	prior claims, which is the entire bulk backlog
		 *	or half of the standard backlog, whichever is
		 *	less.						*/

		copyScalar(&limit, &(plan->stdBacklog));
		divideScalar(&limit, 2);
		copyScalar(&increment, &limit);
		subtractFromScalar(&limit, &(plan->bulkBacklog));
		if (scalarIsValid(&limit))
		{
			/*	Current bulk backlog is less than half
			 *	of the current std backlog.		*/

			copyScalar(&increment, &(plan->bulkBacklog));
		}

		addToScalar(priorClaims, &increment);
#endif
		return;
	}

	/*	Priority is 2, i.e., urgent (expedited).		*/

	if ((i = bundle->ordinal) == 0)
	{
		addToScalar(priorClaims, &(plan->urgentBacklog));
		return;
	}

	/*	Bundle has non-zero ordinal, so it may jump ahead of
	 *	some other urgent bundles.  Compute sum of backlogs
	 *	for this and all higher ordinals.			*/

	while (i < 256)
	{
		addToScalar(priorClaims, &(plan->ordinals[i].backlog));
		i++;
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
		metaEid->schemeName = "dtn";
		metaEid->schemeNameLength = 3;
		metaEid->schemeCodeNbr = dtn;
		metaEid->colon = NULL;
		metaEid->nss = "none";
		metaEid->nssLength = 4;
		metaEid->elementNbr = 0;
		metaEid->serviceNbr = 0;
		metaEid->nullEndpoint = 1;
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
		writeMemoNote("[?] Unknown scheme for endpoint URI", eidString);
		return 0;
	}

	metaEid->schemeCodeNbr = (*vscheme)->codeNumber;
	switch (metaEid->schemeCodeNbr)
	{
	case dtn:
		metaEid->elementNbr = 0;
		metaEid->serviceNbr = 0;
		return 1;

	case ipn:
	case imc:
		if (sscanf(metaEid->nss, UVAST_FIELDSPEC ".%u",
			&(metaEid->elementNbr), &(metaEid->serviceNbr)) < 2)
		{
			*(metaEid->colon) = ':';
			writeMemoNote("[?] Malformed URI", eidString);
			return 0;
		}

		if (metaEid->elementNbr == 0 && metaEid->serviceNbr == 0)
		{
			metaEid->nullEndpoint = 1;
		}

		return 1;

	default:
		writeMemoNote("[?] URI for this scheme not parseable",
				metaEid->schemeName);
	}

	return 0;
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

int	recordEid(EndpointId *eid, MetaEid *meid, EidMode mode)
{
	Sdr		sdr;
	Object		obj;
	PsmPartition	wm;
	PsmAddress	addr;
	int		nssLength;
	char		*ptr;

	eid->schemeCodeNbr = meid->schemeCodeNbr;
	switch (meid->schemeCodeNbr)
	{
	case dtn:
		if (meid->nullEndpoint)
		{
			/*	We encode "dtn:none" in this way.	*/

			eid->ssp.dtn.endpointName.s = NULL;
			eid->ssp.dtn.nssLength = 0;
			return 0;
		}

		switch (mode)
		{
		case EidNV:
			sdr = getIonsdr();
			obj = sdr_malloc(sdr, meid->nssLength);
			if (obj == 0)
			{
				putErrmsg("No heap space for EID",
						itoa(meid->nssLength));
				return -1;
			}

			sdr_write(sdr, obj, meid->nss, meid->nssLength);
			eid->ssp.dtn.endpointName.nv = obj;
			eid->ssp.dtn.nssLength = meid->nssLength;

			/*	Positive length indicates mode NV.	*/

			return 0;

		case EidV:
       			wm = getIonwm();
			addr = psm_malloc(wm, meid->nssLength);
			if (addr == 0)
			{
				putErrmsg("No working memory space for EID",
						itoa(meid->nssLength));
				return -1;
			}

			memcpy(psp(wm, addr), meid->nss, meid->nssLength);
			eid->ssp.dtn.endpointName.v = addr;
			eid->ssp.dtn.nssLength = (0 - meid->nssLength);

			/*	Negative length indicates mode V.	*/

			return 0;

		default:
			nssLength = meid->nssLength + 1;
			ptr = MTAKE(nssLength);
			if (ptr == NULL)
			{
				putErrmsg("Not enough memory for EID",
						itoa(nssLength));
				return -1;
			}

			istrcpy(ptr, meid->nss, nssLength);
			eid->ssp.dtn.endpointName.s = ptr;
			eid->ssp.dtn.nssLength = 0;

			/*	Zero length indicates mode S.		*/

			return 0;
		}

	case ipn:
		eid->ssp.ipn.nodeNbr = meid->elementNbr;
		eid->ssp.ipn.serviceNbr = meid->serviceNbr;
		return 0;

	case imc:
		eid->ssp.imc.groupNbr = meid->elementNbr;
		eid->ssp.imc.serviceNbr = meid->serviceNbr;
		return 0;

	default:
		putErrmsg("Can't record EID, unknown URI scheme",
				meid->schemeName);
		return -1;
	}
}

void	eraseEid(EndpointId *eid)
{
	PsmPartition	wm;

	CHKVOID(eid);
	switch (eid->schemeCodeNbr)
	{
	case dtn:
		if (eid->ssp.dtn.nssLength > 0)
		{
			sdr_free(getIonsdr(), eid->ssp.dtn.endpointName.nv);
			eid->ssp.dtn.endpointName.nv = 0;
		}
		else if (eid->ssp.dtn.nssLength < 0)
		{
			wm = getIonwm();
			psm_free(wm, eid->ssp.dtn.endpointName.v);
			eid->ssp.dtn.endpointName.v = 0;
		}
		else
		{
			if (eid->ssp.dtn.endpointName.s != NULL)
			{
				MRELEASE(eid->ssp.dtn.endpointName.s);
			}

			eid->ssp.dtn.endpointName.s = NULL;
		}

		eid->ssp.dtn.nssLength = 0;
		break;

	case ipn:
		eid->ssp.ipn.nodeNbr = 0;
		eid->ssp.ipn.serviceNbr = 0;
		break;

	case imc:
		eid->ssp.imc.groupNbr = 0;
		eid->ssp.imc.serviceNbr = 0;
		break;

	default:
		break;
	}

	eid->schemeCodeNbr = unknown;
}

static int	readDtnEid(DtnSSP *ssp, char **buffer)
{
	EidMode	mode;
	int	nssLength;
	int	eidLength;
	char	*eidString;

	*buffer = NULL;		/*	Default.			*/
	if (ssp->nssLength > 0)
	{
		if (ssp->endpointName.nv == 0)
		{
			return 0;
		}

		mode = EidNV;
		nssLength = eidLength = ssp->nssLength;
	}
	else if (ssp->nssLength < 0)
	{
		if (ssp->endpointName.v == 0)
		{
			return 0;
		}

		mode = EidV;
		nssLength = eidLength = 0 - ssp->nssLength;
	}
	else	/*	nssLength == 0 indicates private char string.	*/
	{
		if (ssp->endpointName.s == NULL)
		{
			/*	We decode this as "dtn:none".		*/

			eidLength = strlen(_nullEid()) + 1;
			eidString = MTAKE(eidLength);
			if (eidString == NULL)
			{
				putErrmsg("Can't create EID string.",
						itoa(eidLength));
				return -1;
			}

			istrcpy(eidString, _nullEid(), eidLength);
			*buffer = eidString;
			return 0;
		}

		mode = EidS;
		nssLength = eidLength = istrlen(ssp->endpointName.s,
				MAX_NSS_LEN);
	}

	eidLength += 5;
	eidString = MTAKE(eidLength);
	if (eidString == NULL)
	{
		putErrmsg("Can't create EID string.", itoa(eidLength));
		return -1;
	}

	istrcpy(eidString, "dtn:", eidLength);
	switch (mode)
	{
	case EidNV:
		sdr_read(getIonsdr(), eidString + 4, ssp->endpointName.nv,
				nssLength);
		break;

	case EidV:
		memcpy(eidString + 4, psp(getIonwm(), ssp->endpointName.v),
				nssLength);
		break;

	default:
		memcpy(eidString + 4, ssp->endpointName.s, nssLength);
	}

	eidString[eidLength - 1] = '\0';
	*buffer = eidString;
	return 0;
}

static int	readIpnEid(IpnSSP *ssp, char **buffer)
{
	char	*eidString;
	int	eidLength = 36;

	/*	Printed EID string is
	 *
	 *	   ipn:<nodenbr>.<servicenbr>\0
	 *
	 *	So max EID string length is 3 for "ipn" plus 1 for
	 *	':' plus max length of nodeNbr (which is a 64-bit
	 *	number, so 20 digits) plus 1 for '.' plus max length
	 *	of serviceNbr (which is a 32-bit number, so 10 digits)
	 *	plus 1 for the terminating NULL.			*/

	eidString = MTAKE(eidLength);
	if (eidString == NULL)
	{
		putErrmsg("Can't create EID string.", NULL);
		return -1;
	}

	if (ssp->nodeNbr == 0 && ssp->serviceNbr == 0)
	{
		istrcpy(eidString, _nullEid(), eidLength);
	}
	else
	{
		isprintf(eidString, eidLength, "ipn:" UVAST_FIELDSPEC ".%u",
				ssp->nodeNbr, ssp->serviceNbr);
	}

	*buffer = eidString;
	return 0;
}

static int	readImcEid(ImcSSP *ssp, char **buffer)
{
	char	*eidString;
	int	eidLength = 36;

	/*	Printed EID string is
	 *
	 *	   imc:<nodenbr>.<servicenbr>\0
	 *
	 *	So max EID string length is 3 for "imc" plus 1 for
	 *	':' plus max length of nodeNbr (which is a 64-bit
	 *	number, so 20 digits) plus 1 for '.' plus max length
	 *	of serviceNbr (which is a 32-bit number, so 10 digits)
	 *	plus 1 for the terminating NULL.			*/

	eidString = MTAKE(eidLength);
	if (eidString == NULL)
	{
		putErrmsg("Can't create EID string.", NULL);
		return -1;
	}

	if (ssp->groupNbr == 0 && ssp->serviceNbr == 0)
	{
		istrcpy(eidString, _nullEid(), eidLength);
	}
	else
	{
		isprintf(eidString, eidLength, "imc:" UVAST_FIELDSPEC ".%u",
				ssp->groupNbr, ssp->serviceNbr);
	}

	*buffer = eidString;
	return 0;
}

int	readEid(EndpointId *eid, char **buffer)
{
	CHKERR(eid && buffer);
	*buffer = "";			/*	Default.		*/
	switch(eid->schemeCodeNbr)
	{
	case dtn:
		return readDtnEid(&(eid->ssp.dtn), buffer);

	case ipn:
		return readIpnEid(&(eid->ssp.ipn), buffer);

	case imc:
		return readImcEid(&(eid->ssp.imc), buffer);

	default:
		return 0;
	}
}

int	startBpTask(Object cmd, Object cmdParms, int *pid)
{
	Sdr	sdr = getIonsdr();
	char	buffer[600];
	char	cmdString[SDRSTRING_BUFSZ];
	char	parmsString[SDRSTRING_BUFSZ];

	CHKERR(cmd && pid && sdr);
	if (sdr_string_read(sdr, cmdString, cmd) < 0)
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
		if (sdr_string_read(sdr, parmsString, cmdParms) < 0)
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

void	lookUpEidScheme(EndpointId *eid, VScheme **vscheme)
{
	PsmPartition	bpwm = getIonwm();
	BpVdb		*bpvdb = _bpvdb(NULL);
	PsmAddress	elt;

	CHKVOID(eid && vscheme);
	for (elt = sm_list_first(bpwm, bpvdb->schemes); elt;
			elt = sm_list_next(bpwm, elt))
	{
		*vscheme = (VScheme *) psp(bpwm, sm_list_data(bpwm, elt));
		if ((*vscheme)->codeNumber == eid->schemeCodeNbr)
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
		{ "src", "fwd", "xmt", "rcv", "dlv", "rfw", "exp" };

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

	currentTime = getCtime();
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

	/*	Reforwarded.						*/

	reportStateStats(5, fromTimestamp, toTimestamp, 0, 0, 0, 0, 0, 0,
			dbStats.tallies[BP_DB_REQUEUED_FOR_FWD].currentCount,
			dbStats.tallies[BP_DB_REQUEUED_FOR_FWD].currentBytes);

	/*	Expired.						*/

	reportStateStats(6, fromTimestamp, toTimestamp, 0, 0, 0, 0, 0, 0,
			dbStats.tallies[BP_DB_EXPIRED].currentCount,
			dbStats.tallies[BP_DB_EXPIRED].currentBytes);
	sdr_exit_xn(sdr);
}

/*	*	*	Bundle destruction functions	*	*	*/

static int	destroyIncomplete(IncompleteBundle *incomplete, Object incElt)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	nextElt;
	Object	fragObj;
	Bundle	fragment;

	for (elt = sdr_list_first(sdr, incomplete->fragments); elt;
			elt = nextElt)
	{
		nextElt = sdr_list_next(sdr, elt);
		fragObj = (Object) sdr_list_data(sdr, elt);
		sdr_stage(sdr, (char *) &fragment, fragObj, sizeof(Bundle));
		fragment.fragmentElt = 0;	/*	Lose constraint.*/
		fragment.incompleteElt = 0;
		sdr_write(sdr, fragObj, (char *) &fragment, sizeof(Bundle));
		if (bpDestroyBundle(fragObj, 0) < 0)
		{
			putErrmsg("Can't destroy incomplete bundle.", NULL);
			return -1;
		}

		sdr_list_delete(sdr, elt, NULL, NULL);
	}

	sdr_list_destroy(sdr, incomplete->fragments, NULL, NULL);
	sdr_free(sdr, sdr_list_data(sdr, incElt));
	sdr_list_delete(sdr, incElt, NULL, NULL);
	return 0;
}

void	removeBundleFromQueue(Bundle *bundle, BpPlan *plan)
{
	Sdr		sdr = getIonsdr();
	unsigned int	backlogDecrement;
	OrdinalState	*ord;

	/*	Removal from queue reduces plan's backlog.		*/

	CHKVOID(bundle && plan);
	backlogDecrement = computeECCC(guessBundleSize(bundle));
	switch (bundle->priority)
	{
	case 0:				/*	Bulk priority.		*/
		reduceScalar(&(plan->bulkBacklog), backlogDecrement);
		break;

	case 1:				/*	Standard priority.	*/
		reduceScalar(&(plan->stdBacklog), backlogDecrement);
		break;

	default:			/*	Urgent priority.	*/
		ord = &(plan->ordinals[bundle->ordinal]);
		reduceScalar(&(ord->backlog), backlogDecrement);
		if (ord->lastForOrdinal == bundle->planXmitElt)
		{
			ord->lastForOrdinal = 0;
		}

		reduceScalar(&(plan->urgentBacklog), backlogDecrement);
	}

	/*	Removal from queue detaches queue from bundle.		*/

	sdr_list_delete(sdr, bundle->planXmitElt, NULL, NULL);
	bundle->planXmitElt = 0;
}

static void	purgePlanXmitElt(Bundle *bundle)
{
	Sdr	sdr = getIonsdr();
	Object	queue;
	Object	planObj;
	BpPlan	plan;

	queue = sdr_list_list(sdr, bundle->planXmitElt);
	planObj = sdr_list_user_data(sdr, queue);
	if (planObj == 0)	/*	Bundle is in Limbo queue.	*/
	{
		sdr_list_delete(sdr, bundle->planXmitElt, NULL, NULL);
		bundle->planXmitElt = 0;
		return;
	}

	sdr_stage(sdr, (char *) &plan, planObj, sizeof(BpPlan));
	removeBundleFromQueue(bundle, &plan);
	sdr_write(sdr, planObj, (char *) &plan, sizeof(BpPlan));
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
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	addr;

	if (bundle->stations == 0)
	{
		return;
	}

	/*	Discard all intermediate routing destinations.		*/

	while (1)
	{
		elt = sdr_list_first(sdr, bundle->stations);
		if (elt == 0)
		{
			return;
		}

		addr = sdr_list_data(sdr, elt);
		sdr_list_delete(sdr, elt, NULL, NULL);
		sdr_free(sdr, addr);	/*	It's an sdrstring.	*/
	}
}

int	bpDestroyBundle(Object bundleObj, int ttlExpired)
{
	Sdr		sdr = getIonsdr();
	Bundle		bundle;
			OBJ_POINTER(IncompleteBundle, incomplete);
	Object		bsetObj;
	BundleSet	bset;
	Object		elt;

	CHKERR(ionLocked());
	CHKERR(bundleObj);
	sdr_stage(sdr, (char *) &bundle, bundleObj, sizeof(Bundle));

	/*	Special handling for TTL expiration.			*/

	if (ttlExpired)
	{
		/*	FORCES removal of all references to bundle.	*/

		if (bundle.fwdQueueElt)
		{
			sdr_list_delete(sdr, bundle.fwdQueueElt, NULL, NULL);
			bundle.fwdQueueElt = 0;
		}

		if (bundle.fragmentElt)
		{
			sdr_list_delete(sdr, bundle.fragmentElt, NULL, NULL);
			bundle.fragmentElt = 0;

			/*	If this is the last fragment of an
			 *	Incomplete, destroy the Incomplete.	*/

			GET_OBJ_POINTER(sdr, IncompleteBundle, incomplete,
				sdr_list_data(sdr, bundle.incompleteElt));
			if (sdr_list_length(sdr, incomplete->fragments) == 0)
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
			sdr_list_delete(sdr, bundle.dlvQueueElt, NULL, NULL);
			bundle.dlvQueueElt = 0;
		}

		if (bundle.transitElt)
		{
			sdr_list_delete(sdr, bundle.transitElt, NULL, NULL);
			bundle.transitElt = 0;
		}

		if (bundle.planXmitElt)
		{
			purgePlanXmitElt(&bundle);
		}

		if (bundle.ductXmitElt)
		{
			sdr_list_delete(sdr, bundle.ductXmitElt, NULL, NULL);
			bundle.ductXmitElt = 0;
		}

		/*	Notify sender, if so requested.  But never
		 *	for admin bundles.				*/

		bpDbTally(BP_DB_EXPIRED, bundle.payload.length);
		if ((_bpvdb(NULL))->watching & WATCH_expire)
		{
			iwatch('!');
		}

		if (!(bundle.bundleProcFlags & BDL_IS_ADMIN)
		&& (SRR_FLAGS(bundle.bundleProcFlags) & BP_DELETED_RPT))
		{
			bundle.statusRpt.flags |= BP_DELETED_RPT;
			bundle.statusRpt.reasonCode = SrLifetimeExpired;
			if (bundle.bundleProcFlags & BDL_STATUS_TIME_REQ)
			{
				getCurrentDtnTime
					(&(bundle.statusRpt.deletionTime));
			}

			if (sendStatusRpt(&bundle) < 0)
			{
				putErrmsg("can't send deletion notice", NULL);
				return -1;
			}
		}

		bundle.detained = 0;
		bpDelTally(SrLifetimeExpired);
		sdr_write(sdr, bundleObj, (char *) &bundle, sizeof(Bundle));
	}

	/*	Check for any remaining constraints on deletion.	*/

	if (bundle.fragmentElt || bundle.dlvQueueElt || bundle.fwdQueueElt
	|| bundle.planXmitElt || bundle.ductXmitElt || bundle.detained)
	{
		return 0;	/*	Can't destroy bundle yet.	*/
	}

	/*	Remove bundle from timeline and bundles hash table.	*/

	destroyBpTimelineEvent(bundle.timelineElt);
	if (bundle.hashEntry)
	{
		bsetObj = sdr_hash_entry_value(sdr, (_bpConstants())->bundles,
				bundle.hashEntry);
		sdr_stage(sdr, (char *) &bset, bsetObj, sizeof(BundleSet));
		bset.count--;
		if (bset.count == 0)
		{
			sdr_hash_delete_entry(sdr, bundle.hashEntry);
			sdr_free(sdr, bsetObj);
		}
		else
		{
			sdr_write(sdr, bsetObj, (char *) &bset,
					sizeof(BundleSet));
		}
	}

	/*	Remove transmission metadata.				*/

	if (bundle.proxNodeEid)
	{
		sdr_free(sdr, bundle.proxNodeEid);
		bundle.proxNodeEid = 0;
	}

	/*	Turn off automatic re-forwarding.			*/

	if (bundle.overdueElt)
	{
		destroyBpTimelineEvent(bundle.overdueElt);
	}

	/*	Remove bundle from applications' bundle tracking lists.	*/

	if (bundle.trackingElts)
	{
		while (1)
		{
			elt = sdr_list_first(sdr, bundle.trackingElts);
			if (elt == 0)
			{
				break;	/*	No more tracking elts.	*/
			}

			/*	Data in list element is the address
			 *	of a list element (in a list used by
			 *	some application) that references this
			 *	bundle.  Delete that list element,
			 *	which is no longer usable.		*/

			sdr_list_delete(sdr, sdr_list_data(sdr, elt),
					NULL, NULL);

			/*	Now delete this list element as well.	*/

			sdr_list_delete(sdr, elt, NULL, NULL);
		}

		sdr_list_destroy(sdr, bundle.trackingElts, NULL, NULL);
	}

	/*	Destroy the bundle's payload ZCO.  There may still
	 *	be some application reference to the source data of
	 *	this ZCO, but if not then deleting the payload ZCO
	 *	will implicitly destroy the source data itself.		*/

	if (bundle.payload.content)
	{
		zco_destroy(sdr, bundle.payload.content);
	}

	/*	Destroy all SDR objects managed for this bundle and
	 *	free space occupied by the bundle itself.		*/

	eraseEid(&bundle.clDossier.senderEid);
	destroyExtensionBlocks(&bundle);
	purgeStationsStack(&bundle);
	if (bundle.stations)
	{
		sdr_list_destroy(sdr, bundle.stations, NULL, NULL);
	}

	eraseEid(&bundle.id.source);
	eraseEid(&bundle.destination);
	eraseEid(&bundle.reportTo);
	sdr_free(sdr, bundleObj);
	bpDiscardTally(bundle.classOfService, bundle.payload.length);
	bpDbTally(BP_DB_DISCARD, bundle.payload.length);
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
	Sdr		sdr = getIonsdr();
	char		key[BUNDLES_HASH_KEY_BUFLEN];
	Address		bsetObj;
	Object		hashElt;
	BundleSet	bset;

	CHKERR(sourceEid && creationTime && bundleAddr);
	*bundleAddr = 0;	/*	Default: not found.		*/
	CHKERR(ionLocked());
	if (constructBundleHashKey(key, sourceEid, creationTime->seconds,
			creationTime->count, fragmentOffset, fragmentLength)
			> BUNDLES_HASH_KEY_LEN)
	{
		return 0;	/*	Can't be in hash table.		*/
	}

	switch (sdr_hash_retrieve(sdr, (_bpConstants())->bundles, key,
			&bsetObj, &hashElt))
	{
	case -1:
		putErrmsg("Failed locating bundle in hash table.", NULL);
		return -1;

	case 0:
		return 0;	/*	No such entry in hash table.	*/

	default:
		sdr_read(sdr, (char *) &bset, bsetObj, sizeof(BundleSet));
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
	Sdr		sdr = getIonsdr();
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

	CHKERR(sdr_begin_xn(sdr));
	findScheme(schemeName, &vscheme, &vschemeElt);
	if (vschemeElt != 0)	/*	This is a known scheme.		*/
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Duplicate scheme", schemeName);
		return 0;
	}

	/*	All parameters validated, okay to add the scheme.	*/

	memset((char *) &schemeBuf, 0, sizeof(Scheme));
	istrcpy(schemeBuf.name, schemeName, sizeof schemeBuf.name);
	schemeBuf.nameLength = schemeNameLength;
	if (fwdCmd)
	{
		schemeBuf.fwdCmd = sdr_string_create(sdr, fwdCmd);
	}

	if (admAppCmd)
	{
		schemeBuf.admAppCmd = sdr_string_create(sdr, admAppCmd);
	}

	schemeBuf.forwardQueue = sdr_list_create(sdr);
	schemeBuf.endpoints = sdr_list_create(sdr);

	/*	Code numbers are predefined for three URI schemes.	*/

	if (strcmp(schemeName, "dtn") == 0)
	{
		schemeBuf.codeNumber = dtn;
		schemeBuf.bclas = sdr_list_create(sdr);
	}
	else if (strcmp(schemeName, "ipn") == 0)
	{
		schemeBuf.codeNumber = ipn;
		schemeBuf.bclas = sdr_list_create(sdr);
	}
	else if (strcmp(schemeName, "imc") == 0)
	{
		schemeBuf.codeNumber = imc;
	}

	addr = sdr_malloc(sdr, sizeof(Scheme));
	if (addr)
	{
		sdr_write(sdr, addr, (char *) &schemeBuf, sizeof(Scheme));
		schemeElt = sdr_list_insert_last(sdr,
				(_bpConstants())->schemes, addr);
	}

	if (sdr_end_xn(sdr) < 0 || schemeElt == 0)
	{
		putErrmsg("Can't add scheme.", schemeName);
		return -1;
	}

	CHKERR(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	if (raiseScheme(schemeElt, _bpvdb(NULL)) < 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't raise scheme.", NULL);
		return -1;
	}

	sdr_exit_xn(sdr);	/*	Unlock memory.			*/
	return 1;
}

int	updateScheme(char *schemeName, char *fwdCmd, char *admAppCmd)
{
	Sdr		sdr = getIonsdr();
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

	CHKERR(sdr_begin_xn(sdr));
	findScheme(schemeName, &vscheme, &vschemeElt);
	if (vschemeElt == 0)	/*	This is an unknown scheme.	*/
	{
		sdr_exit_xn(sdr);
		writeMemoNote("Unknown scheme", schemeName);
		return 0;
	}

	/*	All parameters validated, okay to update the scheme.
	 *	First wipe out current cmds, then establish new ones.	*/

	addr = (Object) sdr_list_data(sdr, vscheme->schemeElt);
	sdr_stage(sdr, (char *) &schemeBuf, addr, sizeof(Scheme));
	if (schemeBuf.fwdCmd)
	{
		sdr_free(sdr, schemeBuf.fwdCmd);
		schemeBuf.fwdCmd = 0;
	}

	if (fwdCmd)
	{
		schemeBuf.fwdCmd = sdr_string_create(sdr, fwdCmd);
	}

	if (schemeBuf.admAppCmd)
	{
		sdr_free(sdr, schemeBuf.admAppCmd);
		schemeBuf.admAppCmd = 0;
	}

	if (admAppCmd)
	{
		schemeBuf.admAppCmd = sdr_string_create(sdr, admAppCmd);
	}

	sdr_write(sdr, addr, (char *) &schemeBuf, sizeof(Scheme));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update scheme.", schemeName);
		return -1;
	}

	return 1;
}

int	removeScheme(char *schemeName)
{
	Sdr		sdr = getIonsdr();
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	Object		schemeElt;
	Object		addr;
	Scheme		schemeBuf;

	CHKERR(schemeName);

	/*	Must stop the scheme before trying to remove it.	*/

	CHKERR(sdr_begin_xn(sdr));	/*	Lock memory.		*/
	findScheme(schemeName, &vscheme, &vschemeElt);
	if (vschemeElt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Unknown scheme", schemeName);
		return 0;
	}

	/*	All parameters validated.				*/

	stopScheme(vscheme);
	sdr_exit_xn(sdr);
	waitForScheme(vscheme);
	CHKERR(sdr_begin_xn(sdr));
	findScheme(schemeName, &vscheme, &vschemeElt);
	if (vschemeElt == 0)
	{
		sdr_exit_xn(sdr);
		return 0;
	}

	resetScheme(vscheme);
	schemeElt = vscheme->schemeElt;
	addr = sdr_list_data(sdr, schemeElt);
	sdr_read(sdr, (char *) &schemeBuf, addr, sizeof(Scheme));
	if (sdr_list_length(sdr, schemeBuf.forwardQueue) != 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Scheme has backlog, can't be removed",
				schemeName);
		return 0;
	}

	if (sdr_list_length(sdr, schemeBuf.endpoints) != 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Scheme has endpoints, can't be removed",
				schemeName);
		return 0;
	}

	if (schemeBuf.bclas)
	{
		if (sdr_list_length(sdr, schemeBuf.bclas) != 0)
		{
			sdr_exit_xn(sdr);
			writeMemoNote("[?] Scheme has BCLAs, can't be removed",
					schemeName);
			return 0;
		}
	}

	/*	Okay to remove this scheme from the database.		*/

	dropScheme(vscheme, vschemeElt);
	if (schemeBuf.fwdCmd)
	{
		sdr_free(sdr, schemeBuf.fwdCmd);
	}

	if (schemeBuf.admAppCmd)
	{
		sdr_free(sdr, schemeBuf.admAppCmd);
	}

	sdr_list_destroy(sdr, schemeBuf.forwardQueue, NULL, NULL);
	sdr_list_destroy(sdr, schemeBuf.endpoints, NULL, NULL);
	if (schemeBuf.bclas)
	{
		sdr_list_destroy(sdr, schemeBuf.endpoints, NULL, NULL);
	}

	sdr_free(sdr, addr);
	sdr_list_delete(sdr, schemeElt, NULL, NULL);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove scheme.", NULL);
		return -1;
	}

	return 1;
}

int	bpStartScheme(char *name)
{
	Sdr		sdr = getIonsdr();
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	int		result = 1;

	CHKZERO(name);
	CHKZERO(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
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

	sdr_exit_xn(sdr);	/*	Unlock memory.			*/
	return result;
}

void	bpStopScheme(char *name)
{
	Sdr		sdr = getIonsdr();
	VScheme		*vscheme;
	PsmAddress	vschemeElt;

	CHKVOID(name);
	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	findScheme(name, &vscheme, &vschemeElt);
	if (vschemeElt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Unknown scheme", name);
		return;
	}

	stopScheme(vscheme);
	sdr_exit_xn(sdr);	/*	Unlock memory.			*/
	waitForScheme(vscheme);
	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	findScheme(name, &vscheme, &vschemeElt);
	if (vschemeElt == 0)
	{
		sdr_exit_xn(sdr);
		return;
	}

	resetScheme(vscheme);
	sdr_exit_xn(sdr);	/*	Unlock memory.			*/
}

void	findEndpoint(char *schemeName, char *nss, VScheme *vscheme,
		VEndpoint **vpoint, PsmAddress *vpointElt)
{
	PsmPartition	bpwm = getIonwm();
	PsmAddress	elt;

	CHKVOID(vpoint && vpointElt);
	if (vscheme == NULL)
	{
		CHKVOID(schemeName);
		findScheme(schemeName, &vscheme, &elt);
		if (elt == 0)
		{
			*vpointElt = 0;
			return;
		}
	}

	CHKVOID(nss);
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

static int	addEndpoint_IMC(VScheme *vscheme, char *eid)
{
	MetaEid		metaEid;
	PsmAddress	elt;
	int		result;

	if (vscheme->codeNumber != imc || eid == NULL)
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

	CHKERR(parseEidString(eid, &metaEid, &vscheme, &elt));
	if (metaEid.serviceNbr != 0)
	{
		restoreEidString(&metaEid);
		writeMemoNote("[?] IMC EID service nbr must be zero", eid);
		return 0;
	}

	result = imcJoin(metaEid.elementNbr);
	restoreEidString(&metaEid);
	return result;
}

int	addEndpoint(char *eid, BpRecvRule recvRule, char *script)
{
	Sdr		sdr = getIonsdr();
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	elt;
	VEndpoint	*vpoint;
	Endpoint	endpointBuf;
	Scheme		scheme;
	EndpointStats	statsInit;
	Object		addr;
	Object		endpointElt = 0;	/*	To hush gcc.	*/
	Object		dbObject;
	BpDB		db;

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

	CHKERR(sdr_begin_xn(sdr));
	findEndpoint(NULL, metaEid.nss, vscheme, &vpoint, &elt);
	if (elt != 0)	/*	This is a known endpoint.	*/
	{
		sdr_exit_xn(sdr);
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
		endpointBuf.recvScript = sdr_string_create(sdr, script);
	}

	endpointBuf.incompletes = sdr_list_create(sdr);
	endpointBuf.deliveryQueue = sdr_list_create(sdr);
	endpointBuf.scheme = (Object) sdr_list_data(sdr, vscheme->schemeElt);
	endpointBuf.stats = sdr_malloc(sdr, sizeof(EndpointStats));
	if (endpointBuf.stats)
	{
		memset((char *) &statsInit, 0, sizeof(EndpointStats));
		sdr_write(sdr, endpointBuf.stats, (char *) &statsInit,
				sizeof(EndpointStats));
	}

	endpointBuf.updateStats = 1;	/*	Default.		*/
	addr = sdr_malloc(sdr, sizeof(Endpoint));
	if (addr)
	{
		sdr_read(sdr, (char *) &scheme, endpointBuf.scheme,
				sizeof(Scheme));
		endpointElt = sdr_list_insert_last(sdr, scheme.endpoints,
				addr);
		sdr_write(sdr, addr, (char *) &endpointBuf, sizeof(Endpoint));
	}

	dbObject = getBpDbObject();
	sdr_stage(sdr, (char *) &db, dbObject, sizeof(BpDB));
	db.regCount++;
	sdr_write(sdr, dbObject, (char *) &db, sizeof(BpDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add endpoint.", eid);
		return -1;
	}

	CHKERR(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	if (raiseEndpoint(vscheme, endpointElt) < 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("Can't raise endpoint.", NULL);
		return -1;
	}

	sdr_exit_xn(sdr);	/*	Unlock memory.			*/
	if (addEndpoint_IMC(vscheme, eid) < 0)
	{
		return -1;
	}

	return 1;
}

int	updateEndpoint(char *eid, BpRecvRule recvRule, char *script)
{
	Sdr		sdr = getIonsdr();
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

	CHKERR(sdr_begin_xn(sdr));
	findEndpoint(NULL, metaEid.nss, vscheme, &vpoint, &elt);
	restoreEidString(&metaEid);
	if (elt == 0)		/*	This is an unknown endpoint.	*/
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Unknown endpoint", eid);
		return 0;
	}

	/*	All parameters validated, okay to update the endpoint.
	 *	First wipe out current parms, then establish new ones.	*/

	addr = (Object) sdr_list_data(sdr, vpoint->endpointElt);
	sdr_stage(sdr, (char *) &endpointBuf, addr, sizeof(Endpoint));
	endpointBuf.recvRule = recvRule;
	if (endpointBuf.recvScript)
	{
		sdr_free(sdr, endpointBuf.recvScript);
		endpointBuf.recvScript = 0;
	}

	if (script)
	{
		endpointBuf.recvScript = sdr_string_create(sdr, script);
	}

	sdr_write(sdr, addr, (char *) &endpointBuf, sizeof(Endpoint));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update endpoint.", eid);
		return -1;
	}

	return 1;
}

static int	removeEndpoint_IMC(VScheme *vscheme, char *eid)
{
	MetaEid		metaEid;
	PsmAddress	elt;
	int		result;

	if (vscheme->codeNumber != imc || eid == NULL)
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

	CHKERR(parseEidString(eid, &metaEid, &vscheme, &elt));
	if (metaEid.serviceNbr != 0)
	{
		restoreEidString(&metaEid);
		writeMemoNote("[?] IMC EID service nbr must be zero", eid);
		return 0;
	}

	result = imcLeave(metaEid.elementNbr);
	restoreEidString(&metaEid);
	return result;
}

int	removeEndpoint(char *eid)
{
	Sdr		sdr = getIonsdr();
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

	CHKERR(sdr_begin_xn(sdr));
	findEndpoint(NULL, metaEid.nss, vscheme, &vpoint, &elt);
	restoreEidString(&metaEid);
	if (elt == 0)			/*	Not found.		*/
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Unknown endpoint", eid);
		return 0;
	}

	if (vpoint->appPid != ERROR && sm_TaskExists(vpoint->appPid))
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Endpoint can't be removed while open", eid);
		return 0;
	}

	endpointElt = vpoint->endpointElt;
	addr = (Object) sdr_list_data(sdr, endpointElt);
	sdr_read(sdr, (char *) &endpointBuf, addr, sizeof(Endpoint));
	if (sdr_list_length(sdr, endpointBuf.incompletes) != 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Endpoint has incomplete bundles pending",
				eid);
		return 0;
	}

	if (sdr_list_length(sdr, endpointBuf.deliveryQueue) != 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Endpoint has non-empty delivery queue", eid);
		return 0;
	}

	/*	Okay to remove this endpoint from the database.		*/

	dropEndpoint(vpoint, elt);
	sdr_list_destroy(sdr, endpointBuf.incompletes, NULL, NULL);
	sdr_list_destroy(sdr, endpointBuf.deliveryQueue, NULL, NULL);
	sdr_free(sdr, addr);
	sdr_list_delete(sdr, endpointElt, NULL, NULL);
	if (sdr_end_xn(sdr) < 0)
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

static int	filterEid(char *outputEid, char *inputEid)
{
	int	eidLength;
	int	last;

	eidLength = istrlen(inputEid, MAX_SDRSTRING);
	if (eidLength == 0 || eidLength == MAX_SDRSTRING)
	{
		putErrmsg("Invalid EID length.", inputEid);
		return -1;
	}

	last = eidLength - 1;

	/*	Note: the '~' character is used internally to
	 *	indicate "all others" (wild card) because it's the
	 *	last printable ASCII character and therefore always
	 *	sorts last in any list.  If the user wants to use
	 *	'*' instead, we just change it silently.		*/

	memcpy(outputEid, inputEid, eidLength);
	outputEid[eidLength] = '\0';
	if (outputEid[last] == '*')
	{
		outputEid[last] = '~';
	}

	return 0;
}

void	findPlan(char *eidIn, VPlan **vplan, PsmAddress *vplanElt)
{
	PsmPartition	bpwm = getIonwm();
	char		eid[SDRSTRING_BUFSZ];
	PsmAddress	elt;
	int		result;

	CHKVOID(vplanElt);
	*vplanElt = 0;			/*	Default.		*/
	CHKVOID(eidIn);
	CHKVOID(vplan);
	if (filterEid(eid, eidIn) < 0)
	{
		return;
	}

	/*	This function locates the volatile egress plan
	 *	object identified by the specified endpoint ID,
	 *	if any; must be an exact match.  (Note that BpPlans
	 *	are *not* ordered alphabetically by EID; only the
	 *	VPlans in the volatile database are.)			*/

	for (elt = sm_list_first(bpwm, (_bpvdb(NULL))->plans); elt;
			elt = sm_list_next(bpwm, elt))
	{
		*vplan = (VPlan *) psp(bpwm, sm_list_data(bpwm, elt));
		result = strcmp((*vplan)->neighborEid, eid);
		if (result < 0)
		{
			continue;
		}

		if (result == 0)	/*	Found matching plan.	*/
		{
			*vplanElt = elt;
		}

		/*	One way or another, the search is now done.	*/

		return;
	}
}

int	addPlan(char *eidIn, unsigned int nominalRate)
{
	Sdr		sdr = getIonsdr();
	BpDB		*bpConstants = _bpConstants();
	VPlan		*vplan;
	PsmAddress	vplanElt;
	char		eid[SDRSTRING_BUFSZ];
	uvast		neighborNodeNbr = 0;
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	Object		elt = 0;
	BpPlan		planBuf;
	PlanStats	statsInit;
	Object		addr;

	CHKERR(eidIn);
	CHKERR(sdr_begin_xn(sdr));
	findPlan(eidIn, &vplan, &vplanElt);
	if (vplanElt != 0)	/*	This is a known egress plan.	*/
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Duplicate egress plan", eidIn);
		return 0;
	}

	if (filterEid(eid, eidIn) < 0)
	{
		sdr_exit_xn(sdr);
		return 0;
	}

	if (strlen(eid) > MAX_EID_LEN)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Plan eid is too long", eidIn);
		return 0;
	}

	if (parseEidString(eidIn, &metaEid, &vscheme, &vschemeElt) == 0)
	{
		sdr_exit_xn(sdr);
		restoreEidString(&metaEid);
		writeMemoNote("[?] Malformed eid for egress plan", eidIn);
		return 0;
	}

	if (metaEid.schemeCodeNbr == ipn)
	{
		neighborNodeNbr = metaEid.elementNbr;
	}

	restoreEidString(&metaEid);

	/*	All parameters validated, okay to add the plan.		*/

	memset((char *) &planBuf, 0, sizeof(BpPlan));
	istrcpy(planBuf.neighborEid, eid, sizeof planBuf.neighborEid);
	planBuf.neighborNodeNbr = neighborNodeNbr;
	planBuf.nominalRate = nominalRate;
	planBuf.stats = sdr_malloc(sdr, sizeof(PlanStats));
	if (planBuf.stats)
	{
		memset((char *) &statsInit, 0, sizeof(PlanStats));
		sdr_write(sdr, planBuf.stats, (char *) &statsInit,
				sizeof(PlanStats));
	}

	planBuf.updateStats = 1;	/*	Default.		*/
	planBuf.bulkQueue = sdr_list_create(sdr);
	planBuf.stdQueue = sdr_list_create(sdr);
	planBuf.urgentQueue = sdr_list_create(sdr);
	planBuf.ducts = sdr_list_create(sdr);
	addr = sdr_malloc(sdr, sizeof(BpPlan));
	if (addr)
	{
		elt = sdr_list_insert_last(sdr, bpConstants->plans, addr);
		sdr_write(sdr, addr, (char *) &planBuf, sizeof(BpPlan));
		sdr_list_user_data_set(sdr, bpConstants->plans, getCtime());

		/*	Record plan's address in the "user data" of
		 *	each queue so that we can easily navigate
		 *	from a plan queue element back to the plan
		 *	via the queue.					*/

		sdr_list_user_data_set(sdr, planBuf.bulkQueue, addr);
		sdr_list_user_data_set(sdr, planBuf.stdQueue, addr);
		sdr_list_user_data_set(sdr, planBuf.urgentQueue, addr);
		sdr_list_user_data_set(sdr, planBuf.ducts, addr);
	}

	if (sdr_end_xn(sdr) < 0 || elt == 0)
	{
		putErrmsg("Can't add egress plan.", eid);
		return -1;
	}

	CHKERR(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	if (raisePlan(elt, _bpvdb(NULL)) < 0)
	{
		putErrmsg("Can't raise egress plan.", eid);
		sdr_exit_xn(sdr);
		return -1;
	}

	sdr_exit_xn(sdr);	/*	Unlock memory.			*/
	return 1;
}

int	updatePlan(char *eidIn, unsigned int nominalRate)
{
	Sdr		sdr = getIonsdr();
	BpDB		*bpConstants = _bpConstants();
	char		eid[SDRSTRING_BUFSZ];
	VPlan		*vplan;
	PsmAddress	vplanElt;
	Object		addr;
	BpPlan		planBuf;

	CHKERR(eidIn);
	if (filterEid(eid, eidIn) < 0)
	{
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	findPlan(eid, &vplan, &vplanElt);
	if (vplanElt == 0)	/*	This is an unknown egress plan.	*/
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Unknown egress plan", eid);
		return 0;
	}

	/*	All parameters validated, okay to update the plan.	*/

	addr = (Object) sdr_list_data(sdr, vplan->planElt);
	sdr_stage(sdr, (char *) &planBuf, addr, sizeof(BpPlan));
	planBuf.nominalRate = nominalRate;
	sdr_write(sdr, addr, (char *) &planBuf, sizeof(BpPlan));
	sdr_list_user_data_set(sdr, bpConstants->plans, getCtime());
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update egress plan.", eid);
		return -1;
	}

	return 1;
}

int	removePlan(char *eidIn)
{
	Sdr		sdr = getIonsdr();
	BpDB		*bpConstants = _bpConstants();
	char		eid[SDRSTRING_BUFSZ];
	VPlan		*vplan;
	PsmAddress	vplanElt;
	Object		planElt;
	Object		addr;
	BpPlan		planBuf;
	Object		elt;

	CHKERR(eidIn);
	if (filterEid(eid, eidIn) < 0)
	{
		return 0;
	}

	/*	Must stop the egress plan before trying to remove it.	*/

	CHKERR(sdr_begin_xn(sdr));	/*	Lock memory.		*/
	findPlan(eid, &vplan, &vplanElt);
	if (vplanElt == 0)		/*	Not found.		*/
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Unknown egress plan", eid);
		return 0;
	}

	/*	All parameters validated.				*/

	stopPlan(vplan);
	sdr_exit_xn(sdr);
	waitForPlan(vplan);
	CHKERR(sdr_begin_xn(sdr));
	findPlan(eid, &vplan, &vplanElt);
	if (vplanElt == 0)		/*	Not found.		*/
	{
		sdr_exit_xn(sdr);
		return 0;
	}

	resetPlan(vplan);
	planElt = vplan->planElt;
	addr = sdr_list_data(sdr, planElt);
	sdr_read(sdr, (char *) &planBuf, addr, sizeof(BpPlan));
	if (sdr_list_length(sdr, planBuf.bulkQueue) != 0
	|| sdr_list_length(sdr, planBuf.stdQueue) != 0
	|| sdr_list_length(sdr, planBuf.urgentQueue) != 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Plan has data to transmit", eid);
		return 0;
	}

	/*	Okay to remove this egress plan from the database.	*/

	/*	First remove the plan's volatile state.			*/

	dropPlan(vplan, vplanElt);

	/*	Then remove the plan's non-volatile state.		*/

	sdr_list_delete(sdr, planElt, NULL, NULL);
	while (1)
	{
		elt = sdr_list_first(sdr, planBuf.ducts);
		if (elt == 0)
		{
			break;
		}

		/*	Each member of planBuf.ducts points to an
		 *	outducts list element referencing an outduct.
		 *	Detaching that outduct from this plan's list
		 *	of outducts removes it from planBuf.ducts.	*/ 

		oK(detachPlanDuct(sdr_list_data(sdr, elt)));
	}

	sdr_list_destroy(sdr, planBuf.ducts, NULL,NULL);
	sdr_list_destroy(sdr, planBuf.bulkQueue, NULL, NULL);
	sdr_list_destroy(sdr, planBuf.stdQueue, NULL, NULL);
	sdr_list_destroy(sdr, planBuf.urgentQueue, NULL,NULL);
	if (planBuf.context)
	{
		sdr_free(sdr, planBuf.context);
	}

	sdr_free(sdr, addr);
	sdr_list_user_data_set(sdr, bpConstants->plans, getCtime());
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove egress plan.", NULL);
		return -1;
	}

	return 1;
}

int	bpStartPlan(char *eid)
{
	Sdr		sdr = getIonsdr();
	VPlan		*vplan;
	PsmAddress	vplanElt;
	int		result = 1;

	CHKZERO(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	findPlan(eid, &vplan, &vplanElt);
	if (vplanElt == 0)	/*	This is an unknown egress plan.	*/
	{
		writeMemoNote("[?] Unknown egress plan", eid);
		result = 0;
	}
	else
	{
		startPlan(vplan);
	}

	sdr_exit_xn(sdr);	/*	Unlock memory.			*/
	return result;
}

void	bpStopPlan(char *eid)
{
	Sdr		sdr = getIonsdr();
	VPlan		*vplan;
	PsmAddress	vplanElt;

	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	findPlan(eid, &vplan, &vplanElt);
	if (vplanElt == 0)	/*	This is an unknown egress plan.	*/
	{
		sdr_exit_xn(sdr);	/*	Unlock memory.		*/
		writeMemoNote("[?] Unknown egress plan", eid);
		return;
	}

	stopPlan(vplan);
	sdr_exit_xn(sdr);	/*	Unlock memory.			*/
	waitForPlan(vplan);
	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	findPlan(eid, &vplan, &vplanElt);
	if (vplanElt == 0)	/*	This is an unknown egress plan.	*/
	{
		sdr_exit_xn(sdr);	/*	Unlock memory.		*/
		return;
	}

	resetPlan(vplan);
	sdr_exit_xn(sdr);	/*	Unlock memory.			*/
}

int	setPlanViaEid(char *eid, char *viaEid)
{
	Sdr		sdr = getIonsdr();
	size_t		viaEidLength;
	VPlan		*vplan;
	PsmAddress	vplanElt;
	Object		planObj;
	BpPlan		plan;

	CHKERR(eid);
	CHKERR(viaEid);
	viaEidLength = istrlen(viaEid, MAX_SDRSTRING);
	if (viaEidLength == MAX_SDRSTRING)
	{
		putErrmsg("Via EID length invalid.", viaEid);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	findPlan(eid, &vplan, &vplanElt);
	if (vplanElt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Unknown plan, can't set via EID", eid);
		return 0;
	}

	planObj = sdr_list_data(sdr, vplan->planElt);
	sdr_stage(sdr, (char *) &plan, planObj, sizeof(BpPlan));
	if (plan.viaEid)
	{
		sdr_free(sdr, plan.viaEid);
	}

	if (viaEidLength == 0)
	{
		plan.viaEid = 0;
	}
	else
	{
		plan.viaEid = sdr_string_create(sdr, viaEid);
	}

	sdr_write(sdr, planObj, (char *) &plan, sizeof(BpPlan));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't set via EID for plan.", eid);
		return -1;
	}

	return 1;
}

int	attachPlanDuct(char *eid, Object outductElt)
{
	Sdr		sdr = getIonsdr();
	Object		outductObj;
	Outduct		outduct;
	VPlan		*vplan;
	PsmAddress	vplanElt;
			OBJ_POINTER(BpPlan, plan);

	CHKERR(eid);
	CHKERR(outductElt);
	CHKERR(sdr_begin_xn(sdr));
	outductObj = sdr_list_data(sdr, outductElt);
	sdr_stage(sdr, (char *) &outduct, outductObj, sizeof(Outduct));
	if (outduct.planDuctListElt != 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Duct is already attached to a plan",
				outduct.name);
		return 0;
	}

	findPlan(eid, &vplan, &vplanElt);
	if (vplanElt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Unknown plan, can't attach duct", eid);
		return 0;
	}

	GET_OBJ_POINTER(sdr, BpPlan, plan, sdr_list_data(sdr, vplan->planElt));
	outduct.planDuctListElt = sdr_list_insert_last(sdr, plan->ducts,
			outductElt);
	sdr_write(sdr, outductObj, (char *) &outduct, sizeof(Outduct));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't attach duct to this plan.", eid);
		return -1;
	}

	return 1;
}

int	detachPlanDuct(Object outductElt)
{
	Sdr		sdr = getIonsdr();
	Object		outductObj;
	Outduct		outduct;

	CHKERR(sdr_begin_xn(sdr));
	outductObj = sdr_list_data(sdr, outductElt);
	sdr_stage(sdr, (char *) &outduct, outductObj, sizeof(Outduct));
	if (outduct.planDuctListElt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Duct is not attached to any plan",
				outduct.name);
		return 0;
	}

	/*	Remove this duct from the duct list of the plan
	 *	that the duct is attached to.				*/

	sdr_list_delete(sdr, outduct.planDuctListElt, NULL, NULL);
	outduct.planDuctListElt = 0;
	sdr_write(sdr, outductObj, (char *) &outduct, sizeof(Outduct));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't detach duct from its plan.", outduct.name);
		return -1;
	}

	return 1;
}

void	lookupPlan(char *eid, VPlan **answer)
{
	PsmPartition	bpwm = getIonwm();
	BpVdb		*vdb = getBpVdb();
	PsmAddress	elt;
	VPlan		*vplan;
	int		result;
	int		eidLen;
	int		last;

	/*	This function determines the applicable egress plan
	 *	for the specified eid, if any. 				*/

	CHKVOID(eid);
	CHKVOID(answer);
	CHKVOID(ionLocked());

	/*	Find the matching plan.  The search argument eid must
	 *	be a node ID, but that eid's best matching plan may be
	 *	one that is for a "wild card" eid, i.e., a string that
	 *	ends in a '~' character indicating "whatever".
	 *
	 *	Increasingly comprehensive wild-card plans sort toward
	 *	the end of the list, so there's no way to terminate
	 *	the search early.					*/

	*answer = NULL;			/*	Default.		*/
	for (elt = sm_list_first(bpwm, vdb->plans); elt;
			elt = sm_list_next(bpwm, elt))
	{
		vplan = (VPlan *) psp(bpwm, sm_list_data(bpwm, elt));
		result = strcmp(vplan->neighborEid, eid);
		if (result < 0)
		{
			continue;	/*	No matching EID yet.	*/
		}

		if (result == 0)	/*	Exact match.		*/
		{
			*answer = vplan;
			return;
		}

		if (result > 0)		/*	May be wild-card match.	*/
		{
			eidLen = istrlen(vplan->neighborEid, MAX_EID_LEN);
			CHKVOID(eidLen > 0 && eidLen != MAX_EID_LEN);
			last = eidLen - 1;
			if (vplan->neighborEid[last] == '~'
			&& (eidLen == 1
			|| strncmp(vplan->neighborEid, eid, eidLen - 1) == 0))
			{
				*answer = vplan;
				return;
			}
		}
	}
}

void	fetchProtocol(char *protocolName, ClProtocol *clp, Object *clpElt)
{
	Sdr	sdr = getIonsdr();
	Object	elt;

	CHKVOID(protocolName && clp && clpElt);
	for (elt = sdr_list_first(sdr, (_bpConstants())->protocols); elt;
			elt = sdr_list_next(sdr, elt))
	{
		sdr_read(sdr, (char *) clp, sdr_list_data(sdr, elt),
				sizeof(ClProtocol));
		if (strcmp(clp->name, protocolName) == 0)
		{
			break;
		}
	}

	*clpElt = elt;
}

int	addProtocol(char *protocolName, int payloadPerFrame, int ohdPerFrame,
		int protocolClass)
{
	Sdr		sdr = getIonsdr();
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

	CHKERR(sdr_begin_xn(sdr));
	fetchProtocol(protocolName, &clpbuf, &elt);
	if (elt != 0)		/*	This is a known protocol.	*/
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Duplicate protocol", protocolName);
		return 0;
	}

	/*	All parameters validated, okay to add the protocol.	*/

	memset((char *) &clpbuf, 0, sizeof(ClProtocol));
	istrcpy(clpbuf.name, protocolName, sizeof clpbuf.name);
	clpbuf.payloadBytesPerFrame = payloadPerFrame;
	clpbuf.overheadPerFrame = ohdPerFrame;
	if (protocolClass == 26 || strcmp(protocolName, "bssp") == 0)
	{
		clpbuf.protocolClass = BP_PROTOCOL_ANY;
	}
	else if (protocolClass == 2 || strcmp(protocolName, "udp") == 0)
	{
		clpbuf.protocolClass = BP_BEST_EFFORT;
	}
	else if (protocolClass == 10
	|| strcmp(protocolName, "ltp") == 0
	|| strcmp(protocolName, "bibe") == 0)
	{
		clpbuf.protocolClass = BP_BEST_EFFORT | BP_RELIABLE;
	}
	else	/*	Default: most CL protocols do retransmission.	*/
	{
		clpbuf.protocolClass = BP_RELIABLE;
	}

	addr = sdr_malloc(sdr, sizeof(ClProtocol));
	if (addr)
	{
		sdr_write(sdr, addr, (char *) &clpbuf, sizeof(ClProtocol));
		sdr_list_insert_last(sdr, (_bpConstants())->protocols, addr);
	}

	if (sdr_end_xn(sdr) < 0)
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
	Sdr		sdr = getIonsdr();
	ClProtocol	clpbuf;
	Object		elt;
	Object		addr;

	CHKERR(protocolName);
	CHKERR(sdr_begin_xn(sdr));
	fetchProtocol(protocolName, &clpbuf, &elt);
	if (elt == 0)				/*	Not found.	*/
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Unknown protocol", protocolName);
		return 0;
	}

	/*	Okay to remove this protocol from the database.		*/

	addr = (Object) sdr_list_data(sdr, elt);
	sdr_free(sdr, addr);
	sdr_list_delete(sdr, elt, NULL, NULL);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove protocol.", NULL);
		return -1;
	}

	return 1;
}

int	bpStartProtocol(char *name)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	bpwm = getIonwm();
	BpVdb		*bpvdb = _bpvdb(NULL);
	PsmAddress	elt;
	VInduct		*vinduct;
	VOutduct	*voutduct;

	CHKZERO(name);
	CHKZERO(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
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

	sdr_exit_xn(sdr);	/*	Unlock memory.			*/
	return 1;
}

void	bpStopProtocol(char *name)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	bpwm = getIonwm();
	BpVdb		*bpvdb = _bpvdb(NULL);
	PsmAddress	elt;
	VInduct		*vinduct;
	VOutduct	*voutduct;

	CHKVOID(name);
	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	for (elt = sm_list_first(bpwm, bpvdb->inducts); elt;
			elt = sm_list_next(bpwm, elt))
	{
		vinduct = (VInduct *) psp(bpwm, sm_list_data(bpwm, elt));
		if (strcmp(vinduct->protocolName, name) == 0)
		{
			stopInduct(vinduct);
			sdr_exit_xn(sdr);
			waitForInduct(vinduct);
			CHKVOID(sdr_begin_xn(sdr));
			resetInduct(vinduct);
		}
	}

	for (elt = sm_list_first(bpwm, bpvdb->outducts); elt;
			elt = sm_list_next(bpwm, elt))
	{
		voutduct = (VOutduct *) psp(bpwm, sm_list_data(bpwm, elt));
		if (strcmp(voutduct->protocolName, name) == 0)
		{
			if (!(voutduct->hasThread))
			{
				stopOutduct(voutduct);
				sdr_exit_xn(sdr);
				waitForOutduct(voutduct);
				CHKVOID(sdr_begin_xn(sdr));
				resetOutduct(voutduct);
			}
		}
	}

	sdr_exit_xn(sdr);	/*	Unlock memory.			*/
}

void	findInduct(char *protocolName, char *ductName, VInduct **vduct,
		PsmAddress *vductElt)
{
	PsmPartition	bpwm = getIonwm();
	PsmAddress	elt;

	CHKVOID(protocolName && ductName && vduct && vductElt);
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
	Sdr		sdr = getIonsdr();
	BpDB		*bpConstants = _bpConstants();
	ClProtocol	clpbuf;
	Object		clpElt;
	VInduct		*vduct;
	PsmAddress	vductElt;
	Induct		ductBuf;
	InductStats	statsInit;
	Object		addr;
	Object		elt = 0;

	CHKERR(protocolName && ductName);
	if (*protocolName == 0 || *ductName == 0)
	{
		writeMemoNote("[?] Zero-length Induct parm(s)", ductName);
		return 0;
	}

	if (strlen(ductName) > MAX_CL_DUCT_NAME_LEN)
	{
		writeMemoNote("[?] Induct name is too long", ductName);
		return 0;
	}

	if (cliCmd)
	{
		if (*cliCmd == '\0')
		{
			cliCmd = NULL;
		}
		else
		{
			if (strlen(cliCmd) > MAX_SDRSTRING)
			{
				writeMemoNote("[?] CLI command string too long",
						cliCmd);
				return 0;
			}
		}
	}

	CHKERR(sdr_begin_xn(sdr));
	fetchProtocol(protocolName, &clpbuf, &clpElt);
	if (clpElt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Protocol is unknown", protocolName);
		return 0;
	}

	findInduct(protocolName, ductName, &vduct, &vductElt);
	if (vductElt != 0)	/*	This is a known duct.		*/
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Duplicate induct", ductName);
		return 0;
	}

	/*	All parameters validated, okay to add the duct.		*/

	memset((char *) &ductBuf, 0, sizeof(Induct));
	istrcpy(ductBuf.name, ductName, sizeof ductBuf.name);
	if (cliCmd)
	{
		ductBuf.cliCmd = sdr_string_create(sdr, cliCmd);
	}

	ductBuf.protocol = (Object) sdr_list_data(sdr, clpElt);
	ductBuf.stats = sdr_malloc(sdr, sizeof(InductStats));
	if (ductBuf.stats)
	{
		memset((char *) &statsInit, 0, sizeof(InductStats));
		sdr_write(sdr, ductBuf.stats, (char *) &statsInit,
				sizeof(InductStats));
	}

	ductBuf.updateStats = 1;	/*	Default.		*/
	addr = sdr_malloc(sdr, sizeof(Induct));
	if (addr)
	{
		elt = sdr_list_insert_last(sdr, bpConstants->inducts, addr);
		sdr_write(sdr, addr, (char *) &ductBuf, sizeof(Induct));
	}

	if (sdr_end_xn(sdr) < 0 || elt == 0)
	{
		putErrmsg("Can't add induct.", ductName);
		return -1;
	}

	CHKERR(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	if (raiseInduct(elt, _bpvdb(NULL)) < 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't raise induct.", NULL);
		return -1;
	}

	sdr_exit_xn(sdr);	/*	Unlock memory.			*/
	return 1;
}

int	updateInduct(char *protocolName, char *ductName, char *cliCmd)
{
	Sdr		sdr = getIonsdr();
	VInduct		*vduct;
	PsmAddress	vductElt;
	Object		addr;
	Induct		ductBuf;

	CHKERR(protocolName && ductName);
	if (*protocolName == 0 || *ductName == 0)
	{
		writeMemoNote("[?] Zero-length Induct parm(s)", ductName);
		return 0;
	}

	if (cliCmd)
	{
		if (*cliCmd == '\0')
		{
			cliCmd = NULL;
		}
		else
		{
			if (strlen(cliCmd) > MAX_SDRSTRING)
			{
				writeMemoNote("[?] CLI command string too long",
						cliCmd);
				return 0;
			}
		}
	}

	CHKERR(sdr_begin_xn(sdr));
	findInduct(protocolName, ductName, &vduct, &vductElt);
	if (vductElt == 0)	/*	This is an unknown duct.	*/
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Unknown induct", ductName);
		return 0;
	}

	/*	All parameters validated, okay to update the duct.
	 *	First wipe out current cliCmd, then establish new one.	*/

	addr = (Object) sdr_list_data(sdr, vduct->inductElt);
	sdr_stage(sdr, (char *) &ductBuf, addr, sizeof(Induct));
	if (ductBuf.cliCmd)
	{
		sdr_free(sdr, ductBuf.cliCmd);
		ductBuf.cliCmd = 0;
	}

	if (cliCmd)
	{
		ductBuf.cliCmd = sdr_string_create(sdr, cliCmd);
	}

	sdr_write(sdr, addr, (char *) &ductBuf, sizeof(Induct));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update induct.", ductName);
		return -1;
	}

	return 1;
}

int	removeInduct(char *protocolName, char *ductName)
{
	Sdr		sdr = getIonsdr();
	VInduct		*vduct;
	PsmAddress	vductElt;
	Object		ductElt;
	Object		addr;
	Induct		inductBuf;

	CHKERR(protocolName && ductName);

	/*	Must stop the induct before trying to remove it.	*/

	CHKERR(sdr_begin_xn(sdr));	/*	Lock memory.		*/
	findInduct(protocolName, ductName, &vduct, &vductElt);
	if (vductElt == 0)		/*	Not found.		*/
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Unknown induct", ductName);
		return 0;
	}

	/*	All parameters validated.				*/

	stopInduct(vduct);
	sdr_exit_xn(sdr);
	waitForInduct(vduct);
	CHKERR(sdr_begin_xn(sdr));
	findInduct(protocolName, ductName, &vduct, &vductElt);
	if (vductElt == 0)		/*	Not found.		*/
	{
		sdr_exit_xn(sdr);
		return 0;
	}

	resetInduct(vduct);
	ductElt = vduct->inductElt;
	addr = sdr_list_data(sdr, ductElt);
	sdr_read(sdr, (char *) &inductBuf, addr, sizeof(Induct));

	/*	Okay to remove this duct from the database.		*/

	dropInduct(vduct, vductElt);
	if (inductBuf.cliCmd)
	{
		sdr_free(sdr, inductBuf.cliCmd);
	}

	sdr_free(sdr, addr);
	sdr_list_delete(sdr, ductElt, NULL, NULL);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove duct.", NULL);
		return -1;
	}

	return 1;
}

int	bpStartInduct(char *protocolName, char *ductName)
{
	Sdr		sdr = getIonsdr();
	VInduct		*vduct;
	PsmAddress	vductElt;
	int		result = 1;

	CHKERR(protocolName && ductName);
	CHKZERO(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
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

	sdr_exit_xn(sdr);	/*	Unlock memory.			*/
	return result;
}

void	bpStopInduct(char *protocolName, char *ductName)
{
	Sdr		sdr = getIonsdr();
	VInduct		*vduct;
	PsmAddress	vductElt;

	CHKVOID(protocolName && ductName);
	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	findInduct(protocolName, ductName, &vduct, &vductElt);
	if (vductElt == 0)	/*	This is an unknown duct.	*/
	{
		sdr_exit_xn(sdr);	/*	Unlock memory.		*/
		writeMemoNote("[?] Unknown induct", ductName);
		return;
	}

	stopInduct(vduct);
	sdr_exit_xn(sdr);	/*	Unlock memory.			*/
	waitForInduct(vduct);
	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	findInduct(protocolName, ductName, &vduct, &vductElt);
	if (vductElt == 0)	/*	This is an unknown duct.	*/
	{
		sdr_exit_xn(sdr);	/*	Unlock memory.		*/
		return;
	}

	resetInduct(vduct);
	sdr_exit_xn(sdr);	/*	Unlock memory.			*/
}

void	findOutduct(char *protocolName, char *ductName, VOutduct **vduct,
		PsmAddress *vductElt)
{
	PsmPartition	bpwm = getIonwm();
	PsmAddress	elt;

	CHKVOID(protocolName && ductName && vduct && vductElt);
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

static int	flushOutduct(Outduct *outduct)
{
	Sdr		sdr = getIonsdr();
	ClProtocol	protocol;
	Object		elt;
	Object		nextElt;
	Object		bundleObj;
	Bundle		bundle;
	int		protocolClassReqd;
	int		protocolClassApplied;

	/*	Any bundle previously enqueued for transmission via
	 *	this outduct that has not yet been transmitted is
	 *	treated as a convergence-layer transmission failure:
	 *	try again or destroy the bundle, depending on
	 *	reliability commitment.					*/

	CHKERR(ionLocked());
	sdr_read(sdr, (char *) &protocol, outduct->protocol,
			sizeof(ClProtocol));
	for (elt = sdr_list_first(sdr, outduct->xmitBuffer); elt; elt = nextElt)
	{
		nextElt = sdr_list_next(sdr, elt);
		bundleObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &bundle, bundleObj, sizeof(Bundle));
		protocolClassReqd = bundle.ancillaryData.flags
				& BP_PROTOCOL_ANY;
		protocolClassApplied = protocolClassReqd
				& protocol.protocolClass;
		if (protocolClassApplied & BP_RELIABLE
		|| protocolClassApplied & BP_RELIABLE_STREAMING)
		{
			if (bpReforwardBundle(bundleObj) < 0)
			{
				putErrmsg("Inferred CL-failure failed",
						outduct->name);
				return -1;
			}
		}
		else
		{
			if (bpDestroyBundle(bundleObj, 1) < 0)
			{
				putErrmsg("Inferred CL-failure failed",
						outduct->name);
				return -1;
			}
		}
	}

	return 0;
}

int	addOutduct(char *protocolName, char *ductName, char *cloCmd,
		unsigned int maxPayloadLength)
{
	Sdr		sdr = getIonsdr();
	BpDB		*bpConstants = _bpConstants();
	ClProtocol	clpbuf;
	Object		clpElt;
	VOutduct	*vduct;
	PsmAddress	vductElt;
	Outduct		ductBuf;
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

	if (*ductName == '*')
	{
		writeMemoNote("[?] ION no longer supports promiscuous ('*') \
outduct expressions for any convergence-layer protocols", ductName);
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

	if (maxPayloadLength == 0)
	{
		if (strcmp(protocolName, "udp") == 0
		|| strcmp(protocolName, "dgr") == 0
		|| strcmp(protocolName, "dccp") == 0)
		{
			maxPayloadLength = 65000;
		}
	}

	CHKERR(sdr_begin_xn(sdr));
	fetchProtocol(protocolName, &clpbuf, &clpElt);
	if (clpElt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Protocol is unknown", protocolName);
		return 0;
	}

	findOutduct(protocolName, ductName, &vduct, &vductElt);
	if (vductElt != 0)	/*	This is a known duct.		*/
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Duplicate outduct", ductName);
		return 0;
	}

	/*	All parameters validated, okay to add the duct.		*/

	memset((char *) &ductBuf, 0, sizeof(Outduct));
	istrcpy(ductBuf.name, ductName, sizeof ductBuf.name);
	if (cloCmd)
	{
		ductBuf.cloCmd = sdr_string_create(sdr, cloCmd);
	}

	ductBuf.maxPayloadLen = maxPayloadLength;
	ductBuf.xmitBuffer = sdr_list_create(sdr);
	ductBuf.protocol = (Object) sdr_list_data(sdr, clpElt);
	addr = sdr_malloc(sdr, sizeof(Outduct));
	if (addr)
	{
		elt = sdr_list_insert_last(sdr, bpConstants->outducts, addr);
		sdr_write(sdr, addr, (char *) &ductBuf, sizeof(Outduct));

		/*	Record duct's address in the "user data" of
		 *	each queue so that we can easily navigate
		 *	from a duct queue element back to the duct
		 *	via the queue.					*/

		sdr_list_user_data_set(sdr, ductBuf.xmitBuffer, addr);
	}

	if (sdr_end_xn(sdr) < 0 || elt == 0)
	{
		putErrmsg("Can't add outduct.", ductName);
		return -1;
	}

	CHKERR(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	if (raiseOutduct(elt, _bpvdb(NULL)) < 0)
	{
		putErrmsg("Can't raise outduct.", NULL);
		sdr_exit_xn(sdr);
		return -1;
	}

	sdr_exit_xn(sdr);	/*	Unlock memory.			*/
	return 1;
}

int	updateOutduct(char *protocolName, char *ductName, char *cloCmd,
		unsigned int maxPayloadLength)
{
	Sdr		sdr = getIonsdr();
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

	if (maxPayloadLength == 0)
	{
		if (strcmp(protocolName, "udp") == 0
		|| strcmp(protocolName, "dgr") == 0
		|| strcmp(protocolName, "dccp") == 0)
		{
			maxPayloadLength = 65000;
		}
	}

	CHKERR(sdr_begin_xn(sdr));
	findOutduct(protocolName, ductName, &vduct, &vductElt);
	if (vductElt == 0)	/*	This is an unknown duct.	*/
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Unknown outduct", ductName);
		return 0;
	}

	/*	All parameters validated, okay to update the duct.
	 *	First wipe out current cloCmd, then establish new one.	*/

	addr = (Object) sdr_list_data(sdr, vduct->outductElt);
	sdr_stage(sdr, (char *) &ductBuf, addr, sizeof(Outduct));
	if (ductBuf.cloCmd)
	{
		sdr_free(sdr, ductBuf.cloCmd);
		ductBuf.cloCmd = 0;
	}

	if (cloCmd)
	{
		ductBuf.cloCmd = sdr_string_create(sdr, cloCmd);
	}

	ductBuf.maxPayloadLen = maxPayloadLength;
	sdr_write(sdr, addr, (char *) &ductBuf, sizeof(Outduct));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update outduct.", ductName);
		return -1;
	}

	return 1;
}

int	removeOutduct(char *protocolName, char *ductName)
{
	Sdr		sdr = getIonsdr();
	VOutduct	*vduct;
	PsmAddress	vductElt;
	Object		outductElt;
	Object		outductObj;
	Outduct		outduct;

	CHKERR(protocolName && ductName);
	if (*protocolName == 0 || *ductName == 0)
	{
		writeMemoNote("[?] Zero-length Outduct parm(s)", ductName);
		return 0;
	}

	/*	Must stop the outduct before trying to remove it.	*/

	CHKERR(sdr_begin_xn(sdr));	/*	Lock memory.		*/
	findOutduct(protocolName, ductName, &vduct, &vductElt);
	if (vductElt == 0)		/*	Not found.		*/
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Unknown outduct", ductName);
		return 0;
	}

	/*	All parameters validated.				*/

	stopOutduct(vduct);
	sdr_exit_xn(sdr);
	waitForOutduct(vduct);
	CHKERR(sdr_begin_xn(sdr));
	findOutduct(protocolName, ductName, &vduct, &vductElt);
	if (vductElt == 0)		/*	Not found.		*/
	{
		sdr_exit_xn(sdr);
		return 0;
	}

	resetOutduct(vduct);
	outductElt = vduct->outductElt;
	outductObj = sdr_list_data(sdr, outductElt);
	sdr_read(sdr, (char *) &outduct, outductObj, sizeof(Outduct));

	/*	First flush any bundles currently in the outduct's
	 *	transmission buffer.					*/

	if (flushOutduct(&outduct) < 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Failed flushing outduct.", ductName);
		return -1;
	}

	/*	Then detach the outduct from the egress plan that 
	 *	cites it, if any.					*/

	if (outduct.planDuctListElt)
	{
		sdr_list_delete(sdr, outduct.planDuctListElt, NULL, NULL);
		outduct.planDuctListElt = 0;
	}

	/*	Can then remove the duct's volatile state.		*/

	dropOutduct(vduct, vductElt);

	/*	Finally, remove the duct's non-volatile state.		*/

	if (outduct.cloCmd)
	{
		sdr_free(sdr, outduct.cloCmd);
	}

	sdr_list_destroy(sdr, outduct.xmitBuffer, NULL, NULL);
	if (outduct.context)
	{
		sdr_free(sdr, outduct.context);
	}

	sdr_free(sdr, outductObj);
	sdr_list_delete(sdr, outductElt, NULL, NULL);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove outduct.", NULL);
		return -1;
	}

	return 1;
}

int	bpStartOutduct(char *protocolName, char *ductName)
{
	Sdr		sdr = getIonsdr();
	VOutduct	*vduct;
	PsmAddress	vductElt;
	int		result = 1;

	CHKERR(protocolName && ductName);
	CHKZERO(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
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

	sdr_exit_xn(sdr);	/*	Unlock memory.			*/
	return result;
}

void	bpStopOutduct(char *protocolName, char *ductName)
{
	Sdr		sdr = getIonsdr();
	VOutduct	*vduct;
	PsmAddress	vductElt;

	CHKVOID(protocolName && ductName);
	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	findOutduct(protocolName, ductName, &vduct, &vductElt);
	if (vductElt == 0)	/*	This is an unknown duct.	*/
	{
		sdr_exit_xn(sdr);	/*	Unlock memory.		*/
		writeMemoNote("[?] Unknown outduct", ductName);
		return;
	}

	stopOutduct(vduct);
	sdr_exit_xn(sdr);	/*	Unlock memory.			*/
	waitForOutduct(vduct);
	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	findOutduct(protocolName, ductName, &vduct, &vductElt);
	if (vductElt == 0)	/*	This is an unknown duct.	*/
	{
		sdr_exit_xn(sdr);	/*	Unlock memory.		*/
		return;
	}

	resetOutduct(vduct);
	sdr_exit_xn(sdr);	/*	Unlock memory.			*/
}

static int	findIncomplete(Bundle *bundle, VEndpoint *vpoint,
			Object *incompleteAddr, Object *incompleteElt)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(Endpoint, endpoint);
	char	*bundleEid = NULL;
	Object	elt;
		OBJ_POINTER(IncompleteBundle, incomplete);
		OBJ_POINTER(Bundle, fragment);
	char	*fragmentEid;
	int	result;

	CHKERR(ionLocked());
	*incompleteElt = 0;		/*	Default: not found.	*/
	GET_OBJ_POINTER(sdr, Endpoint, endpoint, 
			sdr_list_data(sdr, vpoint->endpointElt));
	if (bundle->id.source.schemeCodeNbr == dtn)
	{
		if (readEid(&(bundle->id.source), &bundleEid) < 0)
		{
			putErrmsg("Can't get bundle's source EID.", NULL);
			return -1;
		}
	}
	else					/*	Must be ipn.	*/
	{
		/*	Note: only destination can ever be multicast.	*/

		if (bundle->id.source.schemeCodeNbr != ipn)
		{
			return 0;	/*	Can't reassemble.	*/
		}
	}

	for (elt = sdr_list_first(sdr, endpoint->incompletes); elt;
			elt = sdr_list_next(sdr, elt))
	{
		*incompleteAddr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, IncompleteBundle, incomplete, 
				*incompleteAddr);

		/*	See if ID of Incomplete's first fragment
		 *	matches ID of the bundle we're looking for.	*/

		GET_OBJ_POINTER(sdr, Bundle, fragment, sdr_list_data(sdr,
				sdr_list_first(sdr, incomplete->fragments)));

		/*	First compare source endpoint IDs.		*/

		if (fragment->id.source.schemeCodeNbr
			       	!= bundle->id.source.schemeCodeNbr)
		{
			continue;
		}

		if (fragment->id.source.schemeCodeNbr == ipn)
		{
			if (fragment->id.source.ssp.ipn.nodeNbr !=
					bundle->id.source.ssp.ipn.nodeNbr
			|| fragment->id.source.ssp.ipn.serviceNbr !=
					bundle->id.source.ssp.ipn.serviceNbr)
			{
				continue;
			}
		}
		else	/*	Source EID scheme must be dtn.		*/
		{
			if (readEid(&(fragment->id.source), &fragmentEid) < 0)
			{
				putErrmsg("Can't get bundle's source EID.",
						NULL);
				MRELEASE(bundleEid);
				return -1;
			}

			result = strcmp(fragmentEid, bundleEid);
			MRELEASE(fragmentEid);
			if (result != 0)
			{
				continue;	/*	No match.	*/
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

	if (bundleEid)
	{
		MRELEASE(bundleEid);
	}

	return 0;
}

Object	insertBpTimelineEvent(BpEvent *newEvent)
{
	PsmPartition	wm = getIonwm();
	PsmAddress	timeline = (getBpVdb())->timeline;
	PsmAddress	node;
	PsmAddress	successor;
	Sdr		sdr = getIonsdr();
	BpDB		*bpConstants = _bpConstants();
	Address		addr;
	Object		nextElt;
	Object		elt;

	CHKZERO(newEvent);
	CHKZERO(ionLocked());
	node = sm_rbt_search(wm, timeline, orderBpEvents, newEvent, &successor);
	if (node != 0)
	{
		/*	Event already exists; return its list elt.	*/

		return sm_rbt_data(wm, node);
	}

	addr = sdr_malloc(sdr, sizeof(BpEvent));
	if (addr == 0)
	{
		putErrmsg("No space for timeline event.", NULL);
		return 0;
	}

	sdr_write(sdr, addr, (char *) newEvent, sizeof(BpEvent));
	if (successor)
	{
		nextElt = (Object) sm_rbt_data(wm, successor);
		elt = sdr_list_insert_before(sdr, nextElt, addr);
	}
	else
	{
		elt = sdr_list_insert_last(sdr, bpConstants->timeline, addr);
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

	/*	Note: bundle creation time and expiration time are
		DTN times, which are ctimes less EPOCH_2000_SEC.

		Bundle arrival time is simply a ctime (Unix epoch time).

		The events in the BP timeline are tagged by ctime.	*/

	if (ionClockIsSynchronized() && bundle->id.creationTime.seconds > 0)
	{
		bundle->expirationTime = bundle->id.creationTime.seconds
				+ bundle->timeToLive;
	}
	else
	{
		/*	Expiration time must be computed as the
			current time plus the difference between
			the bundle's time to live and the bundle's
			current age.

			(If the bundle's current age exceeds its time
			to live then the bundle's expiration time has
			already been reached: it is the current time.)

			So initialize expiration time to the current
			DTN time (ctime less the Epoch 2000 offset).	*/

		bundle->expirationTime = getCtime() - EPOCH_2000_SEC;

		/*	Compute remaining lifetime for bundle, by
			subtracting bundle age from the bundle's TTL.	*/

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

		/*	Add remaining lifetime to bundle's arrival
		 *	time (in ctime) to get expiration time in
		 *	ctime, then subtract EPOCH_2000_SEC to convert 
		 *	to DTN time.				.	*/

		expirationTime.tv_sec = bundle->arrivalTime.tv_sec
				+ timeRemaining.tv_sec;
		expirationTime.tv_usec = bundle->arrivalTime.tv_usec
				+ timeRemaining.tv_usec;
		if (expirationTime.tv_usec > 1000000)
		{
			expirationTime.tv_sec += 1;
			expirationTime.tv_usec -= 1000000;
		}

		/*	Round expiration time to the nearest second.	*/

		if (expirationTime.tv_usec >= 500000)
		{
			expirationTime.tv_sec += 1;
		}

		bundle->expirationTime = expirationTime.tv_sec - EPOCH_2000_SEC;
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
	char		*sourceEid;
	char		bundleKey[BUNDLES_HASH_KEY_BUFLEN];
	Address		bsetObj;
	Object		hashElt;
	BundleSet	bset;
	int		result = 0;

	CHKERR(ionLocked());

	/*	Insert bundle into hashtable of all bundles.		*/

	if (readEid(&(bundle->id.source), &sourceEid) < 0)
	{
		putErrmsg("Can't print source EID.", NULL);
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
	return result;
}

int	bpClone(Bundle *oldBundle, Bundle *newBundle, Object *newBundleObj,
		unsigned int offset, unsigned int length)
{
	Sdr		sdr = getIonsdr();
	char		*eidString;
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;

	CHKERR(oldBundle && newBundle && newBundleObj);
	if (oldBundle->payload.content == 0)
	{
		putErrmsg("Nothing to clone.", utoa(oldBundle->payload.length));
		return -1;
	}

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

	*newBundleObj = sdr_malloc(sdr, sizeof(Bundle));
	if (*newBundleObj == 0)
	{
		putErrmsg("Can't create copy of bundle.", NULL);
		return -1;
	}

	memcpy((char *) newBundle, (char *) oldBundle, sizeof(Bundle));
	if (oldBundle->id.source.schemeCodeNbr == dtn)
	{
		if (readEid(&(oldBundle->id.source), &eidString) < 0)
		{
			putErrmsg("Can't recover source EID string.", NULL);
			return -1;
		}

		CHKERR(parseEidString(eidString, &metaEid, &vscheme,
					&vschemeElt));
		if (writeEid(&(newBundle->id.source), &metaEid) < 0)
		{
			putErrmsg("Can't copy source EID.", NULL);
			return -1;
		}

		MRELEASE(eidString);
	}

	if (oldBundle->destination.schemeCodeNbr == dtn)
	{
		if (readEid(&(oldBundle->destination), &eidString) < 0)
		{
			putErrmsg("Can't recover dest EID string.", NULL);
			return -1;
		}

		CHKERR(parseEidString(eidString, &metaEid, &vscheme,
					&vschemeElt));
		if (writeEid(&(newBundle->destination), &metaEid) < 0)
		{
			putErrmsg("Can't copy dest EID.", NULL);
			return -1;
		}

		MRELEASE(eidString);
	}

	if (oldBundle->reportTo.schemeCodeNbr == dtn)
	{
		if (readEid(&(oldBundle->reportTo), &eidString) < 0)
		{
			putErrmsg("Can't recover report-to EID string.", NULL);
			return -1;
		}

		CHKERR(parseEidString(eidString, &metaEid, &vscheme,
					&vschemeElt));
		if (writeEid(&(newBundle->reportTo), &metaEid) < 0)
		{
			putErrmsg("Can't copy reportTo EID.", NULL);
			return -1;
		}

		MRELEASE(eidString);
	}

	/*	Clone part or all of payload.				*/

	newBundle->payload.content = zco_clone(sdr,
			oldBundle->payload.content, offset, length);
	if (newBundle->payload.content <= 0)
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

	/*	Copy extension blocks.					*/

	newBundle->extensions = 0;
	newBundle->extensionsLength = 0;
	if (copyExtensionBlocks(newBundle, oldBundle) < 0)
	{
		putErrmsg("Can't copy extensions.", NULL);
		return -1;
	}

	/*	Copy sender endpoint ID as needed.			*/

	if (oldBundle->clDossier.senderEid.schemeCodeNbr == dtn)
	{
		if (readEid(&(oldBundle->clDossier.senderEid), &eidString) < 0)
		{
			putErrmsg("Can't recover sender EID string.", NULL);
			return -1;
		}

		CHKERR(parseEidString(eidString, &metaEid, &vscheme,
					&vschemeElt));
		if (writeEid(&(newBundle->clDossier.senderEid), &metaEid) < 0)
		{
			putErrmsg("Can't copy sender EID.", NULL);
			return -1;
		}

		MRELEASE(eidString);
	}

	/*	Initialize stations stack.				*/

	newBundle->stations = sdr_list_create(sdr);

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
	newBundle->fwdQueueElt = 0;
	newBundle->fragmentElt = 0;
	newBundle->dlvQueueElt = 0;
	newBundle->trackingElts = sdr_list_create(sdr);
	newBundle->incompleteElt = 0;
	newBundle->ductXmitElt = 0;
	newBundle->planXmitElt = 0;

	/*	Retain relevant routing information.			*/

	if (oldBundle->proxNodeEid)
	{
		newBundle->proxNodeEid = sdr_string_dup(sdr,
				oldBundle->proxNodeEid);
		if (newBundle->proxNodeEid == 0)
		{
			putErrmsg("Can't copy proxNodeEid.", NULL);
			return -1;
		}
	}

	/*	Adjust database occupancy, record bundle, return.	*/

	noteBundleInserted(newBundle);
	sdr_write(sdr, *newBundleObj, (char *) newBundle, sizeof(Bundle));
	return 0;
}

int	forwardBundle(Object bundleObj, Bundle *bundle, char *eid)
{
	Sdr		sdr = getIonsdr();
	Object		elt;
	char		eidBuf[SDRSTRING_BUFSZ];
	MetaEid		stationMetaEid;
	Object		stationEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	Scheme		schemeBuf;

	CHKERR(bundleObj && bundle && eid);

	/*	Error if this bundle is already in the process of
	 *	being delivered (or reassembled for delivery) locally.
	 *	A bundle that is being delivered and must also be
	 *	forwarded must be cloned, and only the clone may be
	 *	passed to this function.				*/

	CHKERR(bundle->dlvQueueElt == 0);
	CHKERR(bundle->fragmentElt == 0);

	if (bundle->corrupt)
	{
		return bpAbandon(bundleObj, bundle, BP_REASON_BLK_MALFORMED);
	}

	/*	If bundle is already being forwarded, then a
	 *	redundant failure of stewardship notification (is
	 *	this possible?) has been received, and we haven't
	 *	yet finished responding to the prior one.  So nothing
	 *	to do at this point.					*/

	if (bundle->fwdQueueElt || bundle->planXmitElt || bundle->ductXmitElt)
	{
		return 0;
	}

	/*	Count as queued for forwarding, but only when the
	 *	station stack depth is 0.  Forwarders handing the
	 *	bundle off to one another doesn't count as queuing
	 *	a bundle for forwarding.				*/

	if (sdr_list_length(sdr, bundle->stations) == 0)
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

		sdr_write(sdr, bundleObj, (char *) bundle, sizeof(Bundle));
		return bpAbandon(bundleObj, bundle, BP_REASON_NO_ROUTE);
	}

	/*	Prevent routing loop: eid must not already be in the
	 *	bundle's stations stack.				*/

	for (elt = sdr_list_first(sdr, bundle->stations); elt;
			elt = sdr_list_next(sdr, elt))
	{
		sdr_string_read(sdr, eidBuf, sdr_list_data(sdr, elt));
		if (strcmp(eidBuf, eid) == 0)	/*	Routing loop.	*/
		{
			sdr_write(sdr, bundleObj, (char *) bundle,
					sizeof(Bundle));
			return bpAbandon(bundleObj, bundle, BP_REASON_NO_ROUTE);
		}
	}

	CHKERR(ionLocked());
	if (parseEidString(eid, &stationMetaEid, &vscheme, &vschemeElt) == 0)
	{
		/*	Can't forward: can't make sense of this EID.	*/

		restoreEidString(&stationMetaEid);
		writeMemoNote("[?] Can't parse neighbor EID", eid);
		sdr_write(sdr, bundleObj, (char *) bundle, sizeof(Bundle));
		return bpAbandon(bundleObj, bundle, BP_REASON_NO_ROUTE);
	}

	restoreEidString(&stationMetaEid);
	if (stationMetaEid.nullEndpoint)
	{
		/*	Forwarder has determined that the bundle
		 *	should be forwarded to the bit bucket, so
		 *	we must do so.					*/

		sdr_write(sdr, bundleObj, (char *) bundle, sizeof(Bundle));
		return bpAbandon(bundleObj, bundle, BP_REASON_NO_ROUTE);
	}

	/*	We're going to queue this bundle for processing by
	 *	the forwarder for the station EID's scheme name.
	 *	Push the station EID onto the stations stack in case
	 *	we need to do further re-queuing and check for routing
	 *	loops again.						*/

	stationEid = sdr_string_create(sdr, eid);
	if (stationEid == 0
	|| sdr_list_insert_first(sdr, bundle->stations, stationEid) == 0)
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

	sdr_read(sdr, (char *) &schemeBuf, sdr_list_data(sdr,
			vscheme->schemeElt), sizeof(Scheme));
	bundle->fwdQueueElt = sdr_list_insert_last(sdr,
			schemeBuf.forwardQueue, bundleObj);
	sdr_write(sdr, bundleObj, (char *) bundle, sizeof(Bundle));

	/*	Wake up the scheme-specific forwarder as necessary.	*/

	if (vscheme->semaphore != SM_SEM_NONE)
	{
		sm_SemGive(vscheme->semaphore);
	}

	return 0;
}

static int	loadEids(Bundle *bundle, MetaEid *destMetaEid,
			MetaEid *sourceMetaEid, MetaEid *reportToMetaEid)
{
	static MetaEid	nullMetaEid = {"dtn", 3, dtn, NULL, "none", 4, 0, 0, 1};

	if (writeEid(&(bundle->destination), destMetaEid) < 0)
	{
		putErrmsg("Can't load destination EID.", NULL);
		return -1;
	}
		
	if (sourceMetaEid == NULL)	/*	Anonymous.		*/
	{
		sourceMetaEid = &nullMetaEid;
	}

	if (writeEid(&(bundle->id.source), sourceMetaEid) < 0)
	{
		putErrmsg("Can't load source EID.", NULL);
		return -1;
	}

	if (reportToMetaEid == NULL)	/*	Default to source.	*/
	{
		reportToMetaEid = sourceMetaEid;
	}

	if (writeEid(&(bundle->reportTo), reportToMetaEid) < 0)
	{
		putErrmsg("Can't load reportTo EID.", NULL);
		return -1;
	}

	return 0;
}

static int	insertExtensions(Bundle *bundle, ExtensionSpec *extensions,
			int extensionsCt)
{
	int		i;
	ExtensionSpec	*spec;
	ExtensionDef	*def;
	ExtensionBlock	blk;

	for (i = 0, spec = extensions; i < extensionsCt; i++, spec++)
	{
		def = findExtensionDef(spec->type);
		if (def != NULL && def->offer != NULL)
		{
			memset((char *) &blk, 0, sizeof(ExtensionBlock));
			blk.type = spec->type;
			blk.tag1 = spec->tag1;
			blk.tag2 = spec->tag2;
			blk.tag3 = spec->tag3;
			blk.crcType = spec->crcType;
			if (def->offer(&blk, bundle) < 0)
			{
				putErrmsg("Failed offering extension block.",
						NULL);
				return -1;
			}

			if (blk.length == 0 && blk.size == 0)
			{
				continue;
			}

			if (attachExtensionBlock(spec, &blk, bundle) < 0)
			{
				putErrmsg("Failed attaching extension block.",
						NULL);
				return -1;
			}
		}
	}

	return 0;
}

int	bpSend(MetaEid *sourceMetaEid, char *destEidString,
		char *reportToEidString, int lifespan, int classOfService,
		BpCustodySwitch custodySwitch, unsigned char srrFlagsByte,
		int ackRequested, BpAncillaryData *ancillaryData, Object adu,
		Object *bundleObj, int adminRecordType)
{
	Sdr		sdr = getIonsdr();
	BpVdb		*bpvdb = _bpvdb(NULL);
	Object		bpDbObject = getBpDbObject();
	PsmPartition	bpwm = getIonwm();
	BpDB		bpdb;
	PsmAddress	discoveryElt;
	Discovery	*discovery;
	int		bundleProcFlags = 0;
	unsigned int	srrFlags = srrFlagsByte;
	int		aduLength;
	Bundle		bundle;
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
	Object		bundleAddr;
	ExtensionSpec	*userExtensions = NULL;
	int		userExtensionsCt = 0;
	ExtensionSpec	*extensions;
	int		extensionsCt;

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

	discoveryElt = bp_find_discovery(destEidString);
	if (parseEidString(destEidString, &destMetaEid, &vscheme, &vschemeElt)
			== 0)
	{
		restoreEidString(&destMetaEid);
		writeMemoNote("[?] Destination EID malformed", destEidString);
		return 0;
	}

	if (destMetaEid.nullEndpoint)	/*	Do not send bundle.	*/
	{
		CHKERR(sdr_begin_xn(sdr));
		zco_destroy(sdr, adu);
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't send bundle to null endpoint.", NULL);
			return -1;
		}

		return 1;
	}

	/*	Prevent unnecessary NDP beaconing.			*/

	if (discoveryElt)
	{
		discovery = (Discovery *) psp(bpwm, sm_list_data(bpwm,
				discoveryElt));
		discovery->lastContactTime = getCtime();
	}

	/*	To simplify processing, reduce all representations of
	 *	null source endpoint to a sourceMetaEid value of NULL.	*/

	if (sourceMetaEid)
	{
		if (sourceMetaEid->nullEndpoint)
		{
			sourceMetaEid = NULL;
		}
	}

	/*	The bundle protocol specification authorizes the
	 *	implementation to issue bundles whose source endpoint
	 *	ID is "dtn:none".  Since this could result in the
	 *	presence in the network of bundles lacking unique IDs,
	 *	making meaningful status reporting impossible, this
	 *	option is supported only when status reporting is
	 *	not requested.						*/

	if (srrFlags != 0)
	{
		if (sourceMetaEid == NULL)
		{
			restoreEidString(&destMetaEid);
			writeMemo("[?] Source can't be anonymous when asking \
for status reports.");
			return 0;
		}

		/*	Also can't get status reports for
		 *	administrative records.				*/

		if (adminRecordType != 0)
		{
			restoreEidString(&destMetaEid);
			writeMemo("[?] Can't ask for status reports for admin \
records.");
			return 0;
		}

		/*	Also can't get status reports for bundles
		 *	sent as "critical" (multiple copies).		*/

		if (ancillaryData)
		{
			if (ancillaryData->flags & BP_MINIMUM_LATENCY)
			{
				restoreEidString(&destMetaEid);
				writeMemo("[?] Can't flag bundle as 'critical' \
when asking for status reports.");
				return 0;
			}
		}
	}

	/*	Set bundle processing flags.				*/

	bundleProcFlags = srrFlags;
	bundleProcFlags <<= 8;	/*	Other flags in low-order byte.	*/
	bundleProcFlags |= BDL_STATUS_TIME_REQ;
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
		 *	report; use own admin EID for the scheme
		 *	cited by destination EID.			*/

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
		/*	Submitted by application on open endpoint.
		 *
		 *	For network management statistics....		*/

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
	}

	/*	Create the outbound bundle.				*/

	memset((char *) &bundle, 0, sizeof(Bundle));
	if (sourceMetaEid == NULL)
	{
		bundle.anonymous = 1;
	}

	bundle.dbOverhead = BASE_BUNDLE_OVERHEAD;
	bundle.acct = ZcoOutbound;
	bundle.priority = bundle.classOfService = classOfService;

	/*	The bundle protocol specification authorizes the
	 *	implementation to fragment an ADU.  In the ION
	 *	implementation we fragment only when necessary, in
	 *	CGR (anticipatory fragmentation) or at the moment
	 *	a bundle is dequeued for transmission.			*/

	CHKERR(sdr_begin_xn(sdr));
	aduLength = zco_length(sdr, adu);
	if (aduLength < 0)
	{
		sdr_exit_xn(sdr);
		restoreEidString(&destMetaEid);
		restoreEidString(reportToMetaEid);
		putErrmsg("Can't get length of ADU.", NULL);
		return -1;
	}

	bundle.bundleProcFlags = bundleProcFlags;
	if (loadEids(&bundle, &destMetaEid, sourceMetaEid, reportToMetaEid) < 0)
	{
		sdr_exit_xn(sdr);
		restoreEidString(&destMetaEid);
		restoreEidString(reportToMetaEid);
		putErrmsg("Can't load endpoint IDs.", NULL);
		return -1;
	}

	restoreEidString(&destMetaEid);
	restoreEidString(reportToMetaEid);
	bundle.id.fragmentOffset = 0;

	/*	Note: bundle is not a fragment when initially created,
	 *	so totalAduLength is left at zero.			*/

	bundle.payloadBlockProcFlags = BLK_MUST_BE_COPIED;
	bundle.payload.length = aduLength;
	bundle.payload.content = adu;

	/*	Convert all payload header and trailer capsules
	 *	into source data extents.  From the BP perspective,
	 *	everything in the ADU is source data.			*/

	if (zco_bond(sdr, bundle.payload.content) < 0)
	{
		putErrmsg("Can't convert payload capsules to extents.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	/*	Set creationTime of bundle.				*/

	sdr_stage(sdr, (char *) &bpdb, bpDbObject, sizeof(BpDB));
	if (ionClockIsSynchronized())
	{
		getCurrentDtnTime(&currentDtnTime);
		if (bpdb.creationTimeSec == 0)	/*	First bundle.	*/
		{
			bpdb.creationTimeSec = currentDtnTime;
		}
	}
	else
	{
		/*	If no synchronized clock, then creationTime
		 *	seconds is always zero.				*/
	       
		currentDtnTime = 0;
	}

	bundle.id.creationTime.seconds = currentDtnTime;

	/*	In either case, a non-volatile bundle counter is
	 *	incremented monotonically, resetting to zero only
	 *	when a managed limit is reached -- except that the
	 *	counter is NOT reset to zero if that would result
	 *	in multiple bundles having the same creationTime.	*/

	bundle.id.creationTime.count = bpdb.bundleCounter;
	bpdb.bundleCounter++;
	if (bpdb.bundleCounter > bpdb.maxBundleCount)
	{
		if (ionClockIsSynchronized())
		{
			if (currentDtnTime > bpdb.creationTimeSec)
			{
				/*	Safe to roll the counter over.	*/

				bpdb.bundleCounter = 0;
				bpdb.creationTimeSec = currentDtnTime;
			}
		}
		else	/*	No synchronized clock, just counter.	*/
		{
			/*	Counter and limit are managed; assume
			 *	always safe.				*/

			bpdb.bundleCounter = 0;
		}
	}

	sdr_write(sdr, bpDbObject, (char *) &bpdb, sizeof(BpDB));

	/*	Load other bundle properties.				*/

	getCurrentTime(&bundle.arrivalTime);
	bundle.timeToLive = lifespan;
	computeExpirationTime(&bundle);
	bundle.extensions = sdr_list_create(sdr);
	bundle.extensionsLength = 0;
	bundle.stations = sdr_list_create(sdr);
	bundle.trackingElts = sdr_list_create(sdr);
	bundleAddr = sdr_malloc(sdr, sizeof(Bundle));
	if (bundleAddr == 0
	|| bundle.stations == 0
	|| bundle.trackingElts == 0
	|| bundle.extensions == 0)
	{
		putErrmsg("No space for bundle object.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	if (setBundleTTL(&bundle, bundleAddr) < 0)
	{
		putErrmsg("Can't insert new bundle into timeline.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	if (catalogueBundle(&bundle, bundleAddr) < 0)
	{
		putErrmsg("Can't catalogue new bundle in hash table.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	if (ancillaryData)
	{
		bundle.ancillaryData.dataLabel = ancillaryData->dataLabel;
		bundle.ancillaryData.flags = ancillaryData->flags;
		bundle.ancillaryData.ordinal = ancillaryData->ordinal;
		bundle.ancillaryData.metadataType = ancillaryData->metadataType;
		bundle.ancillaryData.metadataLen = ancillaryData->metadataLen;
		memcpy(bundle.ancillaryData.metadata, ancillaryData->metadata,
				sizeof bundle.ancillaryData.metadata);

		/*	Default value of bundle's ordinal.		*/

		bundle.ordinal = ancillaryData->ordinal;

		/*	Any bundle-specific extension blocks?		*/

		if (ancillaryData->extensions
		&& ancillaryData->extensionsCt > 0)
		{
			userExtensions = ancillaryData->extensions;
			userExtensionsCt = ancillaryData->extensionsCt;
		}
	}

	if (custodySwitch != NoCustodyRequested)
	{
		bundle.ancillaryData.flags |= BP_RELIABLE;
	}

	/*	Insert all applicable extension blocks into the bundle.	*/

	getExtensionSpecs(&extensions, &extensionsCt);
	if (insertExtensions(&bundle, extensions, extensionsCt) < 0)
	{
		putErrmsg("Failed inserting baseline extension blocks.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	if (userExtensions)
	{
		if (insertExtensions(&bundle, userExtensions, userExtensionsCt)
				< 0)
		{
			putErrmsg("Failed inserting extension blocks.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}
	}

	noteBundleInserted(&bundle);
	if (bundleObj)	/*	App. needs to reference the bundle.	*/
	{
		*bundleObj = bundleAddr;
		bundle.detained = 1;
	}
	else
	{
		bundle.detained = 0;
	}

	/*	Here's where we finally write bundle to the database.	*/

	sdr_write(sdr, bundleAddr, (char *) &bundle, sizeof(Bundle));
	if (forwardBundle(bundleAddr, &bundle, destEidString) < 0)
	{
		putErrmsg("Can't queue bundle for forwarding.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	if (vpoint)
	{
		bpEndpointTally(vpoint, BP_ENDPOINT_SOURCED,
				bundle.payload.length);
	}

	bpSourceTally(bundle.classOfService, bundle.payload.length);
	if (bpvdb->watching & WATCH_a)
	{
		iwatch('a');
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't send bundle.", NULL);
		return -1;
	}

	return 1;
}

/*	*	*	Bundle reception functions	*	*	*/

void	lookUpEndpoint(EndpointId *eid, VScheme *vscheme, VEndpoint **vpoint)
{
	PsmPartition	wm = getIonwm();
	unsigned int	nssLength;
	char		nssBuf[MAX_NSS_LEN + 1];
	PsmAddress	elt;

	CHKVOID(eid && vscheme && vpoint);
	switch (eid->schemeCodeNbr)
	{
	case dtn:
		if (eid->ssp.dtn.nssLength > 0)		/*	NV.	*/
		{
			nssLength = eid->ssp.dtn.nssLength;
			sdr_read(getIonsdr(), nssBuf,
				eid->ssp.dtn.endpointName.nv, nssLength);
			nssBuf[nssLength] = '\0';
		}
		else if (eid->ssp.dtn.nssLength < 0)	/*	V.	*/
		{
			nssLength = 0 - eid->ssp.dtn.nssLength;
			memcpy(nssBuf, psp(wm, eid->ssp.dtn.endpointName.v),
				nssLength);
			nssBuf[nssLength] = '\0';
		}
		else					/*	String	*/
		{
			istrcpy(nssBuf, eid->ssp.dtn.endpointName.s,
				MAX_NSS_LEN);
		}

		break;

	case ipn:
		isprintf(nssBuf, sizeof nssBuf, UVAST_FIELDSPEC ".%u",
				eid->ssp.ipn.nodeNbr, eid->ssp.ipn.serviceNbr);
		break;

	case imc:
		isprintf(nssBuf, sizeof nssBuf, UVAST_FIELDSPEC ".%u",
				eid->ssp.imc.groupNbr, eid->ssp.imc.serviceNbr);
		break;

	default:
		putErrmsg("Unknown endpoint scheme.", itoa(eid->schemeCodeNbr));
		*vpoint = NULL;
		return;
	}

	for (elt = sm_list_first(wm, vscheme->endpoints); elt;
			elt = sm_list_next(wm, elt))
	{
		*vpoint = (VEndpoint *) psp(wm, sm_list_data(wm, elt));
		if (strcmp((*vpoint)->nss, nssBuf) == 0)
		{
			return;		/*	Found it.		*/
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
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(Endpoint, endpoint);
	char	scriptBuf[SDRSTRING_BUFSZ];

	GET_OBJ_POINTER(sdr, Endpoint, endpoint, sdr_list_data(sdr,
				vpoint->endpointElt));
	if (vpoint->appPid == ERROR)	/*	Not open by any app.	*/
	{
		/*	Execute reanimation script, if any.		*/

		if (endpoint->recvScript != 0)
		{
			if (sdr_string_read(sdr, scriptBuf,
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
			sdr_write(sdr, bundleObj, (char *) bundle,
					sizeof(Bundle));
			bpEndpointTally(vpoint, BP_ENDPOINT_ABANDONED,
					bundle->payload.length);
			bpDbTally(BP_DB_ABANDON, bundle->payload.length);
			return 0;
		}
	}

	/*	Queue bundle up for delivery when requested by
	 *	application.						*/

	bundle->dlvQueueElt = sdr_list_insert_last(sdr,
			endpoint->deliveryQueue, bundleObj);
	if (bundle->dlvQueueElt == 0)
	{
		putErrmsg("Can't append to delivery queue.", NULL);
		return -1;
	}

	bundle->delivered = 1;
	sdr_write(sdr, bundleObj, (char *) bundle, sizeof(Bundle));
	if (vpoint->semaphore != SM_SEM_NONE)
	{
		sm_SemGive(vpoint->semaphore);
	}

	bpEndpointTally(vpoint, BP_ENDPOINT_QUEUED, bundle->payload.length);
	return 0;
}

static int	sendRequestedStatusReports(Bundle *bundle)
{
	/*	Send any applicable status report that has been
	 *	requested, as previously noted.				*/

	if (bundle->statusRpt.flags)
	{
		if (sendStatusRpt(bundle) < 0)
		{
			putErrmsg("Can't send status report.", NULL);
			return -1;
		}
	}

	return 0;
}

static int	extendIncomplete(IncompleteBundle *incomplete, Object incElt,
			Object bundleObj, Bundle *bundle)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
		OBJ_POINTER(Bundle, fragment);

	bundle->incompleteElt = incElt;

	/*	First look for fragment insertion point and insert
	 *	the new bundle at this point.				*/

	for (elt = sdr_list_first(sdr, incomplete->fragments); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, Bundle, fragment,
				sdr_list_data(sdr, elt));
		if (fragment->id.fragmentOffset < bundle->id.fragmentOffset)
		{
			continue;
		}

		if (fragment->id.fragmentOffset == bundle->id.fragmentOffset)
		{
			bundle->delivered = 1;
			sdr_write(sdr, bundleObj, (char *) bundle,
					sizeof(Bundle));
			return 0;	/*	Duplicate fragment.	*/
		}

		break;	/*	Insert before this fragment.		*/
	}

	if (elt)
	{
		bundle->fragmentElt = sdr_list_insert_before(sdr, elt,
				bundleObj);
	}
	else
	{
		bundle->fragmentElt = sdr_list_insert_last(sdr,
				incomplete->fragments, bundleObj);
	}

	if (bundle->fragmentElt == 0)
	{
		putErrmsg("Can't insert bundle into fragments list.", NULL);
		return -1;
	}

	if (sendRequestedStatusReports(bundle) < 0)
	{
		putErrmsg("Failed sending status reports.", NULL);
		return -1;
	}

	bundle->delivered = 1;
	sdr_write(sdr, bundleObj, (char *) bundle, sizeof(Bundle));
	return 0;
}

static int	createIncompleteBundle(Object bundleObj, Bundle *bundle,
			VEndpoint *vpoint)
{
	Sdr			sdr = getIonsdr();
	IncompleteBundle	incomplete;
	Object			incObj;
				OBJ_POINTER(Endpoint, endpoint);

	incomplete.fragments = sdr_list_create(sdr);
	if (incomplete.fragments == 0)
	{
		putErrmsg("No space for fragments list.", NULL);
		return -1;
	}

	incomplete.totalAduLength = bundle->totalAduLength;
	incObj = sdr_malloc(sdr, sizeof(IncompleteBundle));
	if (incObj == 0)
	{
		putErrmsg("No space for Incomplete object.", NULL);
		return -1;
	}

	sdr_write(sdr, incObj, (char *) &incomplete,
			sizeof(IncompleteBundle));
	GET_OBJ_POINTER(sdr, Endpoint, endpoint, sdr_list_data(sdr,
			vpoint->endpointElt));
	bundle->incompleteElt = sdr_list_insert_last(sdr,
			endpoint->incompletes, incObj);
	if (bundle->incompleteElt == 0)
	{
		putErrmsg("No space for Incomplete list element.", NULL);
		return -1;
	}

	/*	Enable navigation from fragment back to Incomplete.	*/

	sdr_list_user_data_set(sdr, incomplete.fragments,
			bundle->incompleteElt);

	/*	Bundle becomes first element in the fragments list.	*/

	bundle->fragmentElt = sdr_list_insert_last(sdr, incomplete.fragments,
			bundleObj);
	if (bundle->fragmentElt == 0)
	{
		putErrmsg("No space for fragment list elt.", NULL);
		return -1;
	}

	if (sendRequestedStatusReports(bundle) < 0)
	{
		putErrmsg("Failed sending status reports.", NULL);
		return -1;
	}

	bundle->delivered = 1;
	sdr_write(sdr, bundleObj, (char *) bundle, sizeof(Bundle));
	return 0;
}

int	deliverBundle(Object bundleObj, Bundle *bundle, VEndpoint *vpoint)
{
	Object	incompleteAddr = 0;
		OBJ_POINTER(IncompleteBundle, incomplete);
	Object	elt;

	CHKERR(bundleObj && bundle && vpoint);
	CHKERR(ionLocked());

	/*	Check to see if we've already got one or more
	 *	fragments of this bundle; if so, invoke reassembly
	 *	(which may or may not result in delivery of a new
	 *	reconstructed original bundle to the application).
	 *
	 *	Note: if this bundle were the final missing fragment
	 *	of some other bundle, that fact would have been
	 *	determined during bundle acquisition (in the
	 *	checkIncompleteBundle function); the original
	 *	bundle would have been reassembled and the fragment
	 *	would have been discarded.  Since the bundle is
	 *	instead being delivered, it can't be the final
	 *	missing fragment of some other bundle.			*/

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
	Sdr		sdr = getIonsdr();
	BpDB		*db = getBpConstants();
	BpVdb		*vdb = getBpVdb();
	VScheme		*vscheme;
	Bundle		newBundle;
	Object		newBundleObj;

	CHKERR(ionLocked());
	lookUpEidScheme(&bundle->destination, &vscheme);
	if (vscheme != NULL)	/*	Destination might be local.	*/
	{
		lookUpEndpoint(&bundle->destination, vscheme, vpoint);
		if (*vpoint != NULL)	/*	Destination is here.	*/
		{
			if (deliverBundle(bundleObj, bundle, *vpoint) < 0)
			{
				putErrmsg("Bundle delivery failed.", NULL);
				return -1;
			}

			/*	Bundle delivery did not fail.		*/

			if ((_bpvdb(NULL))->watching & WATCH_z)
			{
				iwatch('z');
			}

			if (bundle->destination.schemeCodeNbr != imc)
			{
				/*	This is not a multicast bundle.
				 *	So we now write the bundle state
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

				sdr_write(sdr, bundleObj, (char *) bundle,
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
			if (bundle->destination.schemeCodeNbr == ipn
			&& bundle->destination.ssp.ipn.nodeNbr ==
					getOwnNodeNbr())
			{
				/*	Destination is known to be the
				 *	local bundle agent.  Since the
				 *	bundle can't be delivered, it
				 *	must be abandoned.  (It can't
				 *	be forwarded, as this would be
				 *	an infinite forwarding loop.)
				 *	But must first accept it, to
				 *	prevent potential re-forwarding
				 *	back to the local bundle agent.	*/

				if (bpAccept(bundleObj, bundle) < 0)
				{
					putErrmsg("Failed dispatching bundle.",
							NULL);
					return -1;
				}

				/*	Accepting the bundle wrote it
				 *	to the SDR, so we can now
				 *	destroy it successfully.  We
				 *	count the bundle as "forwarded"
				 *	because bpAbandon will count it
				 *	as "forwarding failed".		*/

				bpDbTally(BP_DB_QUEUED_FOR_FWD,
						bundle->payload.length);
				return bpAbandon(bundleObj, bundle,
						BP_REASON_NO_ROUTE);
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

	/*	Queue the bundle for insertion into Outbound ZCO
	 *	space.							*/

	bundle->transitElt = sdr_list_insert_last(sdr, db->transit,
			bundleObj);
	sm_SemGive(vdb->transitSemaphore);
	sdr_write(sdr, bundleObj, (char *) bundle, sizeof(Bundle));
	return 0;
}

/*	*	*	Bundle acquisition functions	*	*	*/

AcqWorkArea	*bpGetAcqArea(VInduct *vduct)
{
	int		memIdx = getIonMemoryMgr();
	AcqWorkArea	*work;

	work = (AcqWorkArea *) MTAKE(sizeof(AcqWorkArea));
	if (work)
	{
		work->vduct = vduct;
		work->extBlocks = lyst_create_using(memIdx);
		if (work->extBlocks == NULL)
		{
			lyst_destroy(work->extBlocks);
			MRELEASE(work);
			work = NULL;
		}
	}

	return work;
}

static void	clearAcqArea(AcqWorkArea *work)
{
	LystElt	elt;

	/*	Destroy all extension blocks in the work area.		*/

	work->bundle.extensionsLength = 0;
	while (1)
	{
		elt = lyst_first(work->extBlocks);
		if (elt == NULL)
		{
			break;
		}

		deleteAcqExtBlock(elt);
	}

	/*	Reset all other per-bundle parameters.			*/

	memset((char *) &(work->bundle), 0, sizeof(Bundle));
	work->authentic = 0;
	work->decision = AcqTBD;
	work->malformed = 0;
	work->congestive = 0;
	work->mustAbort = 0;
	work->headerLength = 0;
	work->bundleLength = 0;
}

static int	eraseWorkZco(AcqWorkArea *work)
{
	Sdr	sdr;

	if (work->zco)
	{
		sdr = getIonsdr();
		CHKERR(sdr_begin_xn(sdr));
		sdr_list_delete(sdr, work->zcoElt, NULL, NULL);

		/*	Destroying the ZCO will cause the acquisition
		 *	FileRef to be deleted, which will unlink the
		 *	acquisition file when the FileRef's cleanup
		 *	script is executed.				*/

		zco_destroy(sdr, work->zco);
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't clear inbound bundle ZCO.", NULL);
			return -1;
		}
	}

	work->allAuthentic = 0;
	eraseEid(&(work->senderEid));
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
	lyst_destroy(work->extBlocks);
	MRELEASE(work);
}

int	bpBeginAcq(AcqWorkArea *work, int authentic, char *senderEid)
{
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	elt;

	CHKERR(work);

	/*	Re-initialize the per-bundle parameters.		*/

	clearAcqArea(work);

	/*	Load the per-acquisition parameters.			*/

	work->allAuthentic = authentic ? 1 : 0;
	eraseEid(&(work->senderEid));
	if (senderEid)
	{
		if (parseEidString(senderEid, &metaEid, &vscheme, &elt) == 0)
		{
			writeMemoNote("[?] Sender EID malformed, ignored.",
					senderEid);
		}
		else
		{
			if (jotEid(&(work->senderEid), &metaEid) < 0)
			{
				putErrmsg("Can't jot sender EID.", senderEid);
				return -1;
			}
		}
	}

	return 0;
}

int	bpLoadAcq(AcqWorkArea *work, Object zco)
{
	Sdr	sdr = getIonsdr();
	BpDB	*bpConstants = _bpConstants();

	CHKERR(work && zco);
	if (work->zco)
	{
		putErrmsg("Can't replace ZCO in acq work area.",  NULL);
		return -1;
	}

	CHKERR(sdr_begin_xn(sdr));
	work->zcoElt = sdr_list_insert_last(sdr, bpConstants->inboundBundles,
			zco);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't note inbound bundle ZCO.", NULL);
		return -1;
	}

	work->zco = zco;
	return 0;
}

int	bpContinueAcq(AcqWorkArea *work, char *bytes, int length,
		ReqAttendant *attendant, unsigned char priority)
{
	static unsigned int	acqCount = 0;
	Sdr			sdr = getIonsdr();
				OBJ_POINTER(BpDB, bpdb);
	vast			currentZcoLength;
	ZcoMedium		source;
	vast			heapSpaceNeeded = 0;
	vast			fileSpaceNeeded = 0;
	ReqTicket		ticket;
	Object			extentObj;
	char			cwd[200];
	char			fileName[SDRSTRING_BUFSZ];
	int			fd;
	int			fileLength;

	CHKERR(work && bytes);
	CHKERR(length >= 0);
	if (work->congestive)
	{
		return 0;	/*	No ZCO space; append no more.	*/
	}

	GET_OBJ_POINTER(sdr, BpDB, bpdb, getBpDbObject());

	/*	Acquire extents of bundle into SDR heap up to the
	 *	stated limit; after that, acquire all remaining
	 *	extents into a file.					*/

	if (work->zco)
	{
		currentZcoLength = zco_length(sdr, work->zco);
	}
	else
	{
		currentZcoLength = 0;
	}

	if ((length + currentZcoLength) <= bpdb->maxAcqInHeap)
	{
		source = ZcoSdrSource;
		heapSpaceNeeded = length;
	}
	else
	{
		source = ZcoFileSource;
		fileSpaceNeeded = length;

		/*	We don't really need any heap space for the
		 *	ZCO, but we do claim a need for 1 byte of heap
		 *	space.  This is just to prevent the file space
		 *	request from succeeding so long as heap space
		 *	is fully occupied (i.e., available heap space
		 *	is zero which is less than 1).			*/

		heapSpaceNeeded = 1;
	}

	/*	Reserve space for new ZCO extent.			*/

	if (ionRequestZcoSpace(ZcoInbound, fileSpaceNeeded, 0, heapSpaceNeeded,
			priority, 0, attendant, &ticket) < 0)
	{
		putErrmsg("Failed trying to reserve ZCO space.", NULL);
		return -1;
	}

	if (!(ionSpaceAwarded(ticket)))
	{
		/*	Space not currently available.			*/

		if (attendant == NULL)	/*	Non-blocking.		*/
		{
			work->congestive = 1;
			ionShred(ticket);	/*	Cancel request.	*/
			return 0;	/*	Out of ZCO space.	*/
		}

		/*	Ticket is req list element for the request.
		 *	Wait until space is available.			*/

		if (sm_SemTake(attendant->semaphore) < 0)
		{
			putErrmsg("Failed taking attendant semaphore.", NULL);
			ionShred(ticket);	/*	Cancel request.	*/
			return -1;
		}

		if (sm_SemEnded(attendant->semaphore))
		{
			writeMemo("[i] ZCO space reservation interrupted.");
			ionShred(ticket);	/*	Cancel request.	*/
			return 0;
		}

		/*	ZCO space has now been reserved.		*/
	}

	/*	At this point, ZCO space is known to be available.	*/

	CHKERR(sdr_begin_xn(sdr));
	if (work->zco == 0)	/*	First extent of acquisition.	*/
	{
		work->zco = zco_create(sdr, ZcoSdrSource, 0, 0, 0, ZcoInbound);
		if (work->zco == (Object) ERROR)
		{
			putErrmsg("Can't start inbound bundle ZCO.", NULL);
			sdr_cancel_xn(sdr);
			ionShred(ticket);	/*	Cancel request.	*/
			return -1;
		}

		work->zcoElt = sdr_list_insert_last(sdr, bpdb->inboundBundles,
				work->zco);
		if (work->zcoElt == 0)
		{
			putErrmsg("Can't start inbound bundle ZCO.", NULL);
			sdr_cancel_xn(sdr);
			ionShred(ticket);	/*	Cancel request.	*/
			return -1;
		}
	}

	/*	Now add extent.  
	 *
	 *	Note that this procedure assumes that bundle extents
	 *	are acquired in increasing offset order, without gaps;
	 *	out-of-order extent acquisition would mandate an
	 *	extent reordering mechanism similar to the one that
	 *	is implemented in LTP.  This is not a problem in BP
	 *	because, for all convergence-layer protocols, either
	 *	the CL delivers a complete bundle (as in LTP, UDP,
	 *	and BIBE) or else the bundle increments are acquired
	 *	in order of increasing offset because the CL itself
	 *	enforces data ordering (as in TCP).			*/

	if (source == ZcoSdrSource)
	{
		extentObj = sdr_insert(sdr, bytes, length);
		if (extentObj)
		{
			/*	Pass additive inverse of length to
			 *	zco_append_extent to indicate that
			 *	space has already been awarded.		*/

			switch (zco_append_extent(sdr, work->zco, ZcoSdrSource,
					extentObj, 0, 0 - length))
			{
			case ERROR:
			case 0:
				putErrmsg("Can't append heap extent.", NULL);
				sdr_cancel_xn(sdr);
				ionShred(ticket);	/*	Cancel.	*/
				return -1;

			default:
				break;		/*	Out of switch.	*/
			}
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't acquire extent into heap.", NULL);
			ionShred(ticket);	/*	Cancel request.	*/
			return -1;
		}

		ionShred(ticket);	/*	Dismiss reservation.	*/
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
			ionShred(ticket);	/*	Cancel request.	*/
			return -1;
		}

		acqCount++;
		isprintf(fileName, sizeof fileName, "%s%cbpacq.%u.%u", cwd,
				ION_PATH_DELIMITER, sm_TaskIdSelf(), acqCount);
		fd = iopen(fileName, O_WRONLY | O_CREAT, 0666);
		if (fd < 0)
		{
			putSysErrmsg("Can't create acq file", fileName);
			sdr_cancel_xn(sdr);
			ionShred(ticket);	/*	Cancel request.	*/
			return -1;
		}

		fileLength = 0;
		work->acqFileRef = zco_create_file_ref(sdr, fileName, "",
				ZcoInbound);
		if (work->acqFileRef == 0)
		{
			putErrmsg("Can't create file ref.", NULL);
			sdr_cancel_xn(sdr);
			close(fd);
			ionShred(ticket);	/*	Cancel request.	*/
			return -1;
		}
	}
	else				/*	Writing more to file.	*/
	{
		oK(zco_file_ref_path(sdr, work->acqFileRef, fileName,
				sizeof fileName));
		fd = iopen(fileName, O_WRONLY, 0666);
		if (fd < 0)
		{
			putSysErrmsg("Can't reopen acq file", fileName);
			sdr_cancel_xn(sdr);
			ionShred(ticket);	/*	Cancel request.	*/
			return -1;
		}

		if ((fileLength = lseek(fd, 0, SEEK_END)) < 0)
		{
			putSysErrmsg("Can't get acq file length", fileName);
			sdr_cancel_xn(sdr);
			close(fd);
			ionShred(ticket);	/*	Cancel request.	*/
			return -1;
		}
	}

	if (write(fd, bytes, length) < 0)
	{
		putSysErrmsg("Can't append to acq file", fileName);
		sdr_cancel_xn(sdr);
		close(fd);
		ionShred(ticket);		/*	Cancel request.	*/
		return -1;
	}

	close(fd);

	/*	Pass additive inverse of length to zco_append_extent
	 *	to indicate that space has already been awarded.	*/

	switch (zco_append_extent(sdr, work->zco, ZcoFileSource,
			work->acqFileRef, fileLength, 0 - length))
	{
	case ERROR:
	case 0:
		putErrmsg("Can't append file extent.", NULL);
		sdr_cancel_xn(sdr);
		ionShred(ticket);		/*	Cancel request.	*/
		return -1;

	default:
		/*	Flag file reference for deletion as soon as
		 *	the last ZCO extent that references it is
		 *	deleted.					*/

		zco_destroy_file_ref(sdr, work->acqFileRef);
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't acquire extent into file.", NULL);
		ionShred(ticket);		/*	Cancel request.	*/
		return -1;
	}

	ionShred(ticket);	/*	Dismiss reservation.		*/
	return 0;
}

void	bpCancelAcq(AcqWorkArea *work)
{
	CHKVOID(work);
	oK(eraseWorkZco(work));
}

unsigned int	guessBundleSize(Bundle *bundle)
{
	CHKZERO(bundle);
	return (NOMINAL_PRIMARY_BLKSIZE + bundle->extensionsLength
			+ bundle->payload.length);
}

unsigned int	computeECCC(unsigned int bundleSize)
{
	unsigned int	stackOverhead;

	/*	Assume 6.25% convergence-layer stack overhead.		*/

	stackOverhead = (bundleSize >> 4) & 0x0fffffff;
	if (stackOverhead < TYPICAL_STACK_OVERHEAD)
	{
		stackOverhead = TYPICAL_STACK_OVERHEAD;
	}

	return bundleSize + stackOverhead;
}

static int	advanceWorkBuffer(AcqWorkArea *work, int bytesParsed)
{
	int	bytesRemaining;
	int	bytesToReceive;
	int	bytesReceived;

	/*	Shift buffer left by number of bytes parsed.		*/

	work->bytesBuffered -= bytesParsed;
	memmove(work->buffer, work->buffer + bytesParsed, work->bytesBuffered);

	/*	Now read from ZCO to fill the buffer space that was
	 *	vacated.						*/

	bytesToReceive = sizeof work->buffer - work->bytesBuffered;
	bytesRemaining = work->zcoLength - work->zcoBytesReceived;
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

int	acquireEid(EndpointId *eid, unsigned char **cursor,
		unsigned int *bytesRemaining)
{
	uvast		arrayLength;
	uvast		uvtemp;
	int		totalLength = 0;
	int		length;
	int		majorType;
	int		additionalInfo;
	char		eidString[300];
	uvast		sspLen;
	uvast		nodeNbr;
	unsigned int	serviceNbr;
	uvast		groupNbr;
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	elt;

	/*	Start parsing the endpoint ID.				*/

	arrayLength = 2;	/*	Decode array of size 2.		*/
	length = cbor_decode_array_open(&arrayLength, cursor, bytesRemaining);
       	if (length < 1)
	{
		writeMemo("[?] Can't decode endpoint ID.");
		return 0;
	}

	totalLength += length;

	/*	Acquire the EID scheme ID number.			*/

	length = cbor_decode_integer(&uvtemp, CborAny, cursor, bytesRemaining);
	if (length < 1)
	{
		writeMemo("[?] Can't decode EID scheme.");
		return 0;
	}

	eid->schemeCodeNbr = uvtemp;
	totalLength += length;

	/*	Based on scheme number, acquire scheme-specific part.	*/

	switch (eid->schemeCodeNbr)
	{
	case dtn:
		istrcpy(eidString, "dtn:", sizeof eidString);
		length = cbor_decode_initial_byte(cursor, bytesRemaining,
				&majorType, &additionalInfo);
	       	if (length < 1)
		{
			writeMemo("[?] Can't decode dtn EID NSS type.");
			return 0;
		}

		/*	Move BACK one byte so that the field can be
		 *	parsed, now that we know what to expect (i.e.,
		 *	whether "none" or not).				*/

		*cursor -= 1;
		*bytesRemaining += 1;
		if (majorType == CborUnsignedInteger)
		{
			/*	Only 0 (for "none") is valid.		*/

			length = cbor_decode_integer(&uvtemp, CborAny, cursor,
					bytesRemaining);
		       	if (length < 1)
			{
				writeMemo("[?] Can't decode dtn numeric SSP.");
				return 0;
			}

			if (uvtemp != 0)
			{
				writeMemoNote("[?] Invalid dtn SSP",
					itoa(uvtemp));
				return 0;
			}

			istrcpy(eidString + 4, "none", sizeof eidString - 4);
			sspLen = 4;
		}
		else			/*	Must be text array.	*/
		{
			if (majorType != CborTextString)
			{
				writeMemoNote("[?] Invalid dtn SSP type",
					itoa(majorType));
				return 0;
			}

			sspLen = 255;
			length = cbor_decode_text_string(eidString + 4,
					&sspLen, cursor, bytesRemaining);
		       	if (length < 1)
			{
				writeMemo("[?] Can't decode dtn string SSP.");
				return 0;
			}
		}

		eidString[4 + sspLen] = '\0';
		totalLength += length;
		break;

	case ipn:
		arrayLength = 2;	/*	Decode array of 2.	*/
		length = cbor_decode_array_open(&arrayLength, cursor,
				bytesRemaining);
       		if (length < 1)
		{
			writeMemo("[?] Can't decode ipn SSP.");
			return 0;
		}

		totalLength += length;

		/*	Acquire the node number.			*/

		length = cbor_decode_integer(&nodeNbr, CborAny, cursor,
				bytesRemaining);
		if (length < 1)
		{
			writeMemo("[?] Can't decode node number.");
			return 0;
		}

		totalLength += length;

		/*	Acquire the service number.			*/

		length = cbor_decode_integer(&uvtemp, CborAny, cursor,
				bytesRemaining);
		if (length < 1)
		{
			writeMemo("[?] Can't decode service number.");
			return 0;
		}

		totalLength += length;

		/*	Validate service number, compose EID string.	*/

		if (uvtemp > 4294967295UL)
		{
			writeMemo("[?] Service number too large.");
			return 0;

		}

		serviceNbr = uvtemp;
		isprintf(eidString, sizeof eidString, "ipn:" UVAST_FIELDSPEC
".%lu", nodeNbr, serviceNbr);
		break;

	case imc:
		arrayLength = 2;	/*	Decode array of 2.	*/
		length = cbor_decode_array_open(&arrayLength, cursor,
				bytesRemaining);
       		if (length < 1)
		{
			writeMemo("[?] Can't decode imc SSP.");
			return 0;
		}

		totalLength += length;

		/*	Acquire the group number.			*/

		length = cbor_decode_integer(&groupNbr, CborAny, cursor,
				bytesRemaining);
		if (length < 1)
		{
			writeMemo("[?] Can't decode group number.");
			return 0;
		}

		totalLength += length;

		/*	Acquire the service number.			*/

		length = cbor_decode_integer(&uvtemp, CborAny, cursor,
				bytesRemaining);
		if (length < 1)
		{
			writeMemo("[?] Can't decode service number.");
			return 0;
		}

		totalLength += length;

		/*	Validate service number, compose EID string.	*/

		if (uvtemp > 4294967295UL)
		{
			writeMemo("[?] Service number too large.");
			return 0;

		}

		serviceNbr = uvtemp;
		isprintf(eidString, sizeof eidString,
			"imc:" UVAST_FIELDSPEC ".%lu", groupNbr, serviceNbr);
		break;

	default:
		writeMemo("[?] Can't decode endpoint ID.");
		return 0;
	}

	/*	Store the EID and return length parsed.  There must
	 *	be an easier way to do all this.			*/

	CHKERR(parseEidString(eidString, &metaEid, &vscheme,&elt));
	if (jotEid(eid, &metaEid) < 0)
	{
		putErrmsg("Can't jot eid.", NULL);
		return -1;
	}

	return totalLength;
}

uvast	computeBufferCrc(BpCrcType crcType, unsigned char *buffer,
		int bytesToProcess, int endOfBlock, uvast aggregateCrc,
		uvast *extractedCrc)
{
	int		bytesToMask = 0;
	uint16_t	crc16;
	uint32_t	crc32;
	uint16_t	extractedCrc16;
	uint32_t	extractedCrc32;
	uint16_t	insertedCrc16;
	uint32_t	insertedCrc32;

	/*	If end of block, set value of CRC itself (the last 2
	 *	or 4 bytes of the block) to zero before computing;
	 *	also, plug the newly computed CRC value into that
	 *	location when finished.					*/

	if (crcType == X25CRC16)
	{
		crc16 = aggregateCrc;
		if (endOfBlock)
		{
			bytesToMask = 2;
		}
		
		if (bytesToMask > 0)
		{
			/*	Extract last 2 bytes, then set them
			 *	to zero for CRC computation.		*/

			if (extractedCrc)
			{
				memcpy((char *) &extractedCrc16, buffer +
					(bytesToProcess - bytesToMask), 2);
				*extractedCrc = ntohs(extractedCrc16);
			}

			memset(buffer + (bytesToProcess - bytesToMask), 0, 2);
		}

		crc16 = ion_CRC16_1021_X25((char *) buffer, bytesToProcess,
				crc16);
		if (bytesToMask > 0)
		{
			insertedCrc16 = htons(crc16);
			memcpy(buffer + (bytesToProcess - bytesToMask),
					(char *) &insertedCrc16, 2);
		}

		return crc16;
	}

	/*	Must be CRC32C.						*/

	crc32 = aggregateCrc;
	if (endOfBlock)
	{
		bytesToMask = 4;
	}
	
	if (bytesToMask > 0)
	{
		/*	Extract last 4 bytes, then set them to zero
		 *	for CRC computation.				*/

		if (extractedCrc)
		{
			memcpy((char *) &extractedCrc32, buffer +
				(bytesToProcess - bytesToMask), 4);
			*extractedCrc = ntohl(extractedCrc32);
		}

		memset(buffer + (bytesToProcess - bytesToMask), 0, 4);
	}

	crc32 = ion_CRC32_1EDC6F41_C((char *) buffer, bytesToProcess, crc32);
	if (bytesToMask > 0)
	{
		insertedCrc32 = htonl(crc32);
		memcpy(buffer + (bytesToProcess - bytesToMask),
				(char *) &insertedCrc32, 4);
	}

	return crc32;
}

int	computeZcoCrc(BpCrcType crcType, ZcoReader *reader, int bytesToProcess,
		uvast *crc, uvast *extractedCrc)
{
	char	buffer[10000];
	int	reloadLimit;
	int	crcSize;
	int	endOfBlock = 0;
	int	bytesToReceive;
	int	bytesReceived;

	if (crcType == X25CRC16)
	{
		crcSize = 3;		/*	CBOR 16-bit integer.	*/
	}
	else
	{
		crcSize = 5;		/*	CBOR 32-bit integer.	*/
	}

	reloadLimit = sizeof buffer - crcSize;
	while (bytesToProcess > 0)
	{
		if (bytesToProcess > reloadLimit)
		{
			/*	Can't load all remaining data bytes
			 *	plus CRC in this buffer, so don't
			 *	load all remaining data bytes.		*/

			bytesToReceive = reloadLimit;
		}
		else
		{
			/*	Load all remaining data bytes plus CRC.	*/

			endOfBlock = 1;
			bytesToProcess += crcSize;
			bytesToReceive = bytesToProcess;
		}

		bytesReceived = zco_receive_source(getIonsdr(), reader,
				bytesToReceive, buffer);
	       	if (bytesReceived != bytesToReceive)
		{
			return -1;
		}

		*crc = computeBufferCrc(crcType, (unsigned char *) buffer,
				bytesReceived, endOfBlock, *crc, extractedCrc);
		bytesToProcess -= bytesReceived;
	}

	return 0;
}

static int	acquirePrimaryBlock(AcqWorkArea *work)
{
	Bundle		*bundle;
	int		bytesToParse;
	unsigned int	unparsedBytes;
	unsigned char	*cursor;
	unsigned char	*startOfBlock;
	uvast		arrayLength;
	uvast		itemsRemaining;
	int		version;
	uvast		uvtemp;
	BpCrcType	crcType;
	int		length;
	char		*eidString;
	int		nullEidLen;
	DtnTime		currentDtnTime;
	int		crcLength;
	uvast		crcReceived;
	uvast		crcComputed;
	int		bytesParsed;

	bundle = &(work->bundle);
	bytesToParse = work->bytesBuffered;
	unparsedBytes = bytesToParse;
	cursor = (unsigned char *) (work->buffer);
	startOfBlock = cursor;

	/*	Start parsing the primary block.			*/

	arrayLength = 0;	/*	Decode array of any size.	*/
	if (cbor_decode_array_open(&arrayLength, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode primary block array.");
		return 0;
	}

	itemsRemaining = arrayLength;

	/*	Acquire version number.					*/

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Missing version number in primary block.");
		return 0;
	}

	version = uvtemp;
	if (version != BP_VERSION)
	{
		writeMemoNote(_versionMemo(), itoa(version));
		return 0;
	}

	itemsRemaining -= 1;

	/*	Acquire bundle processing flags.			*/

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Missing bundle flags in primary block.");
		return 0;
	}

	bundle->bundleProcFlags = uvtemp;

	/*	Check for processing flags conflicts.			*/

	if (bundle->bundleProcFlags & BDL_IS_ADMIN)
	{
		if (SRR_FLAGS(bundle->bundleProcFlags) != 0)
		{
			/*	BPbis 4.1.3				*/

			writeMemo("[?] Status report requests prohibited for \
bundle containing administrative record.");
			work->mustAbort = 1;
		}
	}

	/*	Note status report information as necessary.		*/

	if (SRR_FLAGS(bundle->bundleProcFlags) & BP_RECEIVED_RPT)
	{
		bundle->statusRpt.flags |= BP_RECEIVED_RPT;
		if (bundle->bundleProcFlags & BDL_STATUS_TIME_REQ)
		{
			getCurrentDtnTime(&(bundle->statusRpt.receiptTime));
		}
	}

	itemsRemaining -= 1;

	/*	Acquire CRC type.					*/

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Missing CRC type in primary block.");
		return 0;
	}

	if (uvtemp > 2)
	{
		writeMemo("[?] Invalid CRC type in primary block.");
		return 0;
	}

	crcType = uvtemp;
	itemsRemaining -= 1;

	/*	Acquire destination EID.				*/

	length = acquireEid(&(bundle->destination), &cursor, &unparsedBytes);
       	if (length < 1)
	{
		writeMemo("[?] Can't acquire destination EID.");
		return length;
	}

	itemsRemaining -= 1;

	/*	Acquire source EID.  					*/

	length = acquireEid(&(bundle->id.source), &cursor, &unparsedBytes);
	if (length < 1)
	{
		writeMemo("[?] Can't acquire source EID.");
		return length;
	}

	/*	Determine whether or not the bundle is anonymous.
	 *	There must be a more efficient way to do this.		*/

	if (readEid(&(bundle->id.source), &eidString) < 0)
	{
		putErrmsg("Can't print source EID string.", NULL);
		return -1;
	}

	nullEidLen = strlen(_nullEid());
	if (istrlen(eidString, nullEidLen + 1) == nullEidLen
	&& strcmp(eidString, _nullEid()) == 0)
	{
		bundle->anonymous = 1;
	}

	MRELEASE(eidString);

	/*	Check for other processing flags conflicts.		*/

	if (bundle->anonymous)
	{
		if ((bundle->bundleProcFlags & BDL_DOES_NOT_FRAGMENT) == 0
		|| SRR_FLAGS(bundle->bundleProcFlags) != 0)
		{
			/*	BPbis 4.1.3				*/

			writeMemo("[?] Fragmentation and status report \
requests prohibited for anonymous bundle.");
			work->mustAbort = 1;
		}
	}

	itemsRemaining -= 1;

	/*	Acquire report-to EID.					*/

	length = acquireEid(&(bundle->reportTo), &cursor, &unparsedBytes);
       	if (length < 1)
	{
		writeMemo("[?] Can't acquire destination EID.");
		return length;
	}

	itemsRemaining -= 1;

	/*	Acquire creation timestamp array.			*/

	arrayLength = 2;	/*	Decode array of size 2.		*/
	if (cbor_decode_array_open(&arrayLength, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode creation timestamp.");
		return 0;
	}

	itemsRemaining -= 1;

	/*	Acquire creation timestamp seconds.			*/

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't acquire bundle creation time.");
		return 0;
	}

	bundle->id.creationTime.seconds = uvtemp;

	/*	Acquire creation timestamp count.			*/

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't acquire bundle creation count.");
		return 0;
	}

	bundle->id.creationTime.count = uvtemp;

	/*	Acquire time-to-live (lifetime).			*/

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't acquire bundle lifetime.");
		return 0;
	}

	bundle->timeToLive = uvtemp;
	itemsRemaining -= 1;

	/*	Initialize bundle age.					*/

	if (ionClockIsSynchronized() && bundle->id.creationTime.seconds > 0)
	{
		/*	Default bundle age, pending override by BAE.	*/

		getCurrentDtnTime(&currentDtnTime);
		bundle->age = currentDtnTime - bundle->id.creationTime.seconds;
	}
	else
	{
		bundle->age = 0;
	}

	/*	Handle bundle fragment status.				*/

	if (bundle->bundleProcFlags & BDL_IS_FRAGMENT)
	{
		if (itemsRemaining < 2)
		{
			writeMemo("[?] Can't acquire bundle fragment ID.");
			return 0;
		}

		/*	Acquire fragment offset.			*/

		if (cbor_decode_integer(&uvtemp, CborAny, &cursor,
				&unparsedBytes) < 1)
		{
			writeMemo("[?] Can't acquire fragment offset.");
			return 0;
		}

		bundle->id.fragmentOffset = uvtemp;
		itemsRemaining -= 1;

		/*	Acquire total ADU length.			*/

		if (cbor_decode_integer(&uvtemp, CborAny, &cursor,
				&unparsedBytes) < 1)
		{
			writeMemo("[?] Can't acquire total ADU length.");
			return 0;
		}

		bundle->totalAduLength = uvtemp;
		itemsRemaining -= 1;
	}
	else
	{
		bundle->id.fragmentOffset = 0;
		bundle->totalAduLength = 0;
	}

	/*	Compute and check the CRC, if present.			*/

	if (crcType == NoCRC)
	{
		if (itemsRemaining != 0)
		{
			writeMemo("[?] Primary block has too many items.");
			return 0;
		}
	}
	else	/*	Must check CRC of primary block.		*/
	{
		if (itemsRemaining != 1)
		{
			writeMemo("[?] Primary block has wrong nbr of items.");
			return 0;
		}

		length = cursor - startOfBlock;

		/*	Must include CRC in block for CRC calculation.	*/

		if (crcType == X25CRC16)
		{
			crcLength = 3;	/*	CBOR 16-bit CRC		*/
		}
		else
		{
			crcLength = 5;	/*	CBOR 32-bit CRC		*/
		}

		if (crcLength > unparsedBytes)
		{
			writeMemo("[?] Primary block truncated.");
			return 0;
		}

		crcComputed = computeBufferCrc(crcType, startOfBlock, 
				length + crcLength, 1, 0, &crcReceived);
		if (crcComputed != crcReceived)
		{
			writeMemo("[?] CRC check failed for primary block.");
			return 0;
		}

		unparsedBytes -= crcLength;
		itemsRemaining -= 1;
	}

	/*	Have got primary block; include its length in the
	 *	value of header length.					*/

	bytesParsed = bytesToParse - unparsedBytes;
	work->headerLength += bytesParsed;
	return bytesParsed;
}

static int	acquireBlock(AcqWorkArea *work)
{
	Bundle		*bundle;
	int		bytesToParse;
	unsigned int	unparsedBytes;
	unsigned char	*cursor;
	unsigned char	*startOfBlock;
	uvast		arrayLength;
	uvast		itemsRemaining;
	uvast		uvtemp;
	int		length;
	BpBlockType	blkType;
	unsigned int	blkNumber;
	unsigned int	blkProcFlags;
	BpCrcType	crcType;
	vast		dataLength;
	ExtensionDef	*def;
	unsigned int	lengthOfBlock;
	int		crcLength;
	uvast		crcReceived;
	uvast		crcComputed;
	unsigned int	bytesParsed;

	if (work->malformed)
	{
		return 0;	/*	Don't bother to acquire.	*/
	}

	bundle = &(work->bundle);
	bytesToParse = work->bytesBuffered;
	unparsedBytes = bytesToParse;
	cursor = (unsigned char *) (work->buffer);
	startOfBlock = cursor;

	/*	Start parsing this canonical block.			*/

	arrayLength = 0;	/*	Decode array of any size.	*/
	if (cbor_decode_array_open(&arrayLength, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode canonical block array.");
		return 0;
	}

	itemsRemaining = arrayLength;

	/*	Acquire block type.					*/

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Missing block type in canonical block.");
		return 0;
	}

	blkType = uvtemp;
	itemsRemaining -= 1;

	/*	Acquire block number.					*/

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Missing block number in canonical block.");
		return 0;
	}

	blkNumber = uvtemp;
	itemsRemaining -= 1;

	/*	Acquire block processing flags.				*/

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Missing block flags in canonical block.");
		return 0;
	}

	blkProcFlags = uvtemp;

	/*	Check for processing flags conflicts.			*/

	if (bundle->anonymous || bundle->bundleProcFlags & BDL_IS_ADMIN)
	{
			/*	RFC BPbis 5.6 Step 4	*/

		if (blkProcFlags & BLK_REPORT_IF_NG)
		{
			writeMemo("[?] Status report request prohibited for \
undefined block.");
			work->mustAbort = 1;
		}
	}

	itemsRemaining -= 1;

	/*	Acquire CRC type.					*/

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Missing CRC type in canonical block.");
		return 0;
	}

	if (uvtemp > 2)
	{
		writeMemo("[?] Invalid CRC type in canonical block.");
		return 0;
	}

	crcType = uvtemp;
	itemsRemaining -= 1;

	/*	Acquire byte string tag for block-type-specific data.	*/

	uvtemp = (uvast) -1;
	if (cbor_decode_byte_string(NULL, &uvtemp, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode block-type-specific data.");
		return 0;
	}

	/*	Note: because the decoding destination is NULL, the
	 *	cursor was advanced only to the end of the *size* of
	 *	the block-type-specific data, not to the end of the
	 *	data itself.  The unparsedBytes counter was reduced
	 *	only by the length of the *size* of the block-type-
	 *	specific data, not by the length of the block-type-
	 *	specific data itself. However, itemsRemaining is
	 *	reduced by 1 at this time.				*/

	dataLength = uvtemp;
	itemsRemaining -= 1;

	/*	Acquire the block-type-specific data if possible.
	 *	Check first to see if this is the payload block.	*/

	if (blkType == PayloadBlk)	/*	LAST block in bundle.	*/
	{
		/*	Note: length of payload bytes currently in
		 *	the buffer is given by unparsedBytes, starting
		 *	at cursor.  CRC, if any, will be checked by
		 *	the calling function after the payload has
		 *	been acquired.					*/

		bundle->payloadBlockProcFlags = blkProcFlags;
		bundle->payload.length = dataLength;
		bundle->payload.crcType = crcType;
		bytesParsed = bytesToParse - unparsedBytes;
		work->headerLength += bytesParsed;
		return bytesParsed;
	}

	/*	This is an extension block.  Cursor is pointing at
	 *	start of block data.					*/

	if (unparsedBytes < dataLength)	/*	Doesn't fit in buffer.	*/
	{
		writeMemoNote("[?] Extension block too long", utoa(dataLength));
		return 0;	/*	Block is too long for buffer.	*/
	}

	/*	Here we calculate the entire length of this canonical
	 *	block EXCEPT for any terminating CRC.			*/

	lengthOfBlock = (cursor - startOfBlock) + dataLength;

	/*	Now we do block-type-specific parsing if possible.	*/

	def = findExtensionDef(blkType);
	if (def)	/*	This is a known extension block type.	*/
	{
		if (acquireExtensionBlock(work, def, startOfBlock,
			lengthOfBlock, blkType, blkNumber, blkProcFlags,
			dataLength) < 0)
		{
			return -1;
		}
	}
	else		/*	An unrecognized extension.		*/
	{
		if (blkProcFlags & BLK_REPORT_IF_NG)
		{
			/*	RFC BPbis 5.6 Step 4	*/

			bundle->statusRpt.flags |= BP_RECEIVED_RPT;
			bundle->statusRpt.reasonCode = SrBlockUnintelligible;
			if (bundle->bundleProcFlags & BDL_STATUS_TIME_REQ)
			{
				getCurrentDtnTime
					(&(bundle->statusRpt.receiptTime));
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
				/*	Acquire the block as just a
				 *	bag of bits.			*/

				if (acquireExtensionBlock(work, def,
						startOfBlock, lengthOfBlock,
						blkType, blkNumber,
						blkProcFlags, dataLength) < 0)
				{
					return -1;
				}
			}
		}
	}

	cursor += dataLength;
	unparsedBytes -= dataLength;

	/*	Finally, compute and check the CRC, if present.		*/

	if (crcType == NoCRC)
	{
		if (itemsRemaining != 0)
		{
			writeMemo("[?] Extension block has too many items.");
			return 0;
		}
	}
	else	/*	Must check CRC of extension block.		*/
	{
		if (itemsRemaining != 1)
		{
			writeMemo("[?] Extension blocks has too few items.");
			return 0;
		}

		length = cursor - startOfBlock;

		/*	Must include CRC in block for CRC calculation.	*/

		if (crcType == X25CRC16)
		{
			crcLength = 3;	/*	CBOR 16-bit CRC		*/
		}
		else
		{
			crcLength = 5;	/*	CBOR 32-bit CRC		*/
		}

		if (crcLength > unparsedBytes)
		{
			writeMemo("[?] Extension block truncated.");
			return 0;
		}

		crcComputed = computeBufferCrc(crcType, startOfBlock,
				length + crcLength, 1, 0, &crcReceived);
		if (crcComputed != crcReceived)
		{
			writeMemo("[?] CRC check failed for extension block.");
			return 0;
		}

		unparsedBytes -= crcLength;
		itemsRemaining -= 1;
	}

	/*	Have got extension block; include its length in the
	 *	value of header length;					*/

	bytesParsed = bytesToParse - unparsedBytes;
	work->headerLength += bytesParsed;
	return bytesParsed;
}

static int	checkPayloadCrc(AcqWorkArea *work)
{
	ZcoReader	reader;
	unsigned int	bytesToSkip;
	unsigned int	bytesSkipped;
	int		crcSize;
	uvast		computedCrc;
	uvast		extractedCrc;
	
	zco_start_receiving(work->zco, &reader);
	bytesToSkip = work->zcoBytesReceived - work->bytesBuffered;

	/*	Skipping this far at this time positions us at the
	 *	first byte of data for the payload block (which is
	 *	currently at the very beginning of work->buffer).	*/

	if (bytesToSkip > 0)
	{
		bytesSkipped = zco_receive_source(getIonsdr(), &reader,
				bytesToSkip, NULL);
		CHKERR(bytesSkipped == bytesToSkip);
	}

	/*	Now compute the payload block's CRC and, in the
	 *	process, extract the CRC value attached to the block.	*/

	crcSize = computeZcoCrc(work->bundle.payload.crcType, &reader,
			work->bundle.payload.length, &computedCrc,
			&extractedCrc);
	if (crcSize < 0)
	{
		putErrmsg("Failed computing inbound payload CRC.", NULL);
		return -1;
	}

	if (computedCrc != extractedCrc)
	{
		writeMemo("[?] CRC check failed for payload block.");
		return 0;
	}

	return crcSize;
}

static int	acqFromWork(AcqWorkArea *work)
{
	Sdr		sdr = getIonsdr();
	unsigned char	*cursor;
	unsigned int	bytesBuffered;
	uvast		arrayLength;
	int		bytesParsed;
	Bundle		*bundle;
	int		bytesToSkip;
	int		crcSize;
	int		unreceivedPayload;
	int		bytesRecd;

	/*	First acquire CBOR indefinite-length array token.	*/

	bytesBuffered = work->bytesBuffered;
	cursor = (unsigned char *) (work->buffer);
	arrayLength = ((uvast) -1);
	bytesParsed = cbor_decode_array_open(&arrayLength, &cursor,
			&bytesBuffered);
	if (bytesParsed < 1)
	{
		return 0;
	}

	work->headerLength = bytesParsed;
	work->bundleLength = bytesParsed;
	CHKERR(advanceWorkBuffer(work, bytesParsed) == 0);

	/*	Bundle structure initialization.			*/

	bundle = &(work->bundle);
	bundle->dbOverhead = BASE_BUNDLE_OVERHEAD;
	bundle->detained = 0;
	bundle->payload.length = -1;		/*	Not parsed yet.	*/

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

	/*	Aquire all extension blocks following the primary block,
	 *	stopping after the block header for the payload block
	 *	itself.							*/

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

		work->bundleLength += bytesParsed;
		CHKERR(advanceWorkBuffer(work, bytesParsed) == 0);
		if (bundle->payload.length < 0)
		{
			/*	No payload block yet.			*/

			continue;
		}

		/*	Last parsed block was payload block, of which
		 *	only the header was parsed.			*/

		break;
	}

	/*	Now acquire all payload bytes and check CRC of payload
	 *	block (if any).  Note that the actual bytes of payload
	 *	data are already received into the ZCO we are parsing;
	 *	all we need to do now is seek past all of the payload
	 *	data to its end.
	 *
	 *	All bytes of the bundle's "header" were cleared by
	 *	advanceWorkBuffer above; the first byte of payload
	 *	data is now in the first byte of the buffer.
	 *
	 *	If the payload block has a CRC, now is a convenient
	 *	time to check it.
	 *
	 *	After checking the payload block's CRC (as necessary)
	 *	we need to clear all of the payload (and possibly CRC)
	 *	bytes out of the buffer; then, if there are more
	 *	payload bytes in the ZCO beyond those, we must seek
	 *	past all of them and past the CRC if any.  Then we
	 *	reload the buffer from the ZCO bytes that immediately
	 *	follow the payload data bytes and its CRC; that is,
	 *	we reload the buffer from the next bundle in the ZCO.	*/

	if (bundle->payload.crcType == NoCRC)
	{
		bytesToSkip = bundle->payload.length;
	}
	else
	{
		/*	If the payload block has a CRC, check it.
		 *	To do so, we re-read the payload from the
		 *	ZCO; the buffer in the AcqWorkArea is
		 *	unaffected.					*/

		crcSize = checkPayloadCrc(work);
		switch (crcSize)
		{
		case -1:			/*	System failure.	*/
			return -1;

		case 0:				/*	CRC failed.	*/
			work->malformed = 1;
			return 0;

		default:			/*	CRC is okay.	*/
			break;
		}

		bytesToSkip = bundle->payload.length + crcSize;
	}

	if (bytesToSkip <= work->bytesBuffered)
	{
		/*	All bytes of payload data are currently in the
		 *	work area's buffer.				*/

		work->bundleLength += bytesToSkip;
		CHKERR(advanceWorkBuffer(work, bytesToSkip) == 0);
	}
	else
	{
		/*	All bytes in the work area's buffer are
		 *	payload, and some number of additional bytes
		 *	not yet received are also part of the payload.	*/

		unreceivedPayload = bytesToSkip - work->bytesBuffered;
		bytesRecd = zco_receive_source(sdr, &(work->reader),
				unreceivedPayload, NULL);
		CHKERR(bytesRecd >= 0);
		if (bytesRecd != unreceivedPayload)
		{
			work->bundleLength += (work->bytesBuffered + bytesRecd);
writeMemoNote("    Wanted", itoa(bytesToSkip));
writeMemoNote("       Got", itoa(work->bytesBuffered + bytesRecd));
			writeMemoNote("[?] Payload truncated",
					itoa(unreceivedPayload - bytesRecd));
			work->malformed = 1;
			return 0;
		}

		work->zcoBytesReceived += bytesRecd;
		work->bytesBuffered = 0;
		work->bundleLength += bytesToSkip;
		CHKERR(advanceWorkBuffer(work, 0) == 0);
	}

	/*	Finally, acquire CBOR break character.			*/

	bytesBuffered = work->bytesBuffered;
	cursor = (unsigned char *) (work->buffer);
	bytesParsed = cbor_decode_break(&cursor, &bytesBuffered);
	if (bytesParsed != 1)
	{
		return 0;	/*	Array is not terminated.	*/
	}

	work->bundleLength += bytesParsed;
	CHKERR(advanceWorkBuffer(work, bytesParsed) == 0);
	return 0;
}

static int	abortBundleAcq(AcqWorkArea *work)
{
	Sdr	sdr = getIonsdr();

	if (work->bundle.payload.content)
	{
		CHKERR(sdr_begin_xn(sdr));
		zco_destroy(sdr, work->bundle.payload.content);
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't destroy bundle ZCO.", NULL);
			return -1;
		}
	}

	return 0;
}

static int	discardReceivedBundle(AcqWorkArea *work, BpSrReason srReason)
{
	Bundle	*bundle = &(work->bundle);

	/*	If we must discard the bundle, we send any reception
	 *	status report(s) previously noted and we discard the
	 *	bundle's payload content.  We may also send send a
	 *	deletion status report if requested.  
	 *
	 *	Note that negative status reporting is performed here
	 *	(in the CLI) before the bundle is enqueued for forwarding,
	 *	but that positive reporting is performed later by the
	 *	forwarder because it applies to bundles sourced locally
	 *	as well as to bundles accepted from other endpoints.	*/

	if (srReason != 0
	&& (SRR_FLAGS(bundle->bundleProcFlags) & BP_DELETED_RPT))
	{
		bundle->statusRpt.flags |= BP_DELETED_RPT;
		bundle->statusRpt.reasonCode = srReason;
		if (bundle->bundleProcFlags & BDL_STATUS_TIME_REQ)
		{
			getCurrentDtnTime(&(bundle->statusRpt.deletionTime));
		}
	}

	if (bundle->statusRpt.flags)
	{
		if (sendStatusRpt(bundle) < 0)
		{
			putErrmsg("Can't send status report.", NULL);
			return -1;
		}
	}

	bpDelTally(srReason);
	return abortBundleAcq(work);
}

static void	initAuthenticity(AcqWorkArea *work)
{
	Object		secdbObj;

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

	work->authentic = 1;		/*	But check BIBs.		*/
	return;
}

static int	recordBundleEid(Bundle *bundle, EndpointId *eid)
{
	char		*eidString;
	MetaEid		meid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;

	if (readEid(eid, &eidString) < 0)
	{
		putErrmsg("Can't read EID.", NULL);
		return -1;
	} 

	if (parseEidString(eidString, &meid, &vscheme, &vschemeElt) == 0)
	{
		putErrmsg("Can't parse bundle EID.", NULL);
		return -1;
	}

	eraseEid(eid);	/*	Lose the copy in private memory.	*/
	if (writeEid(eid, &meid) < 0)
	{
		putErrmsg("No space for bundle EID.", NULL);
		return -1;
	}

	MRELEASE(eidString);
	if (meid.schemeCodeNbr == dtn)
	{
		bundle->dbOverhead += eid->ssp.dtn.nssLength;
	}

	return 0;
}

static int	acquireBundle(Sdr sdr, AcqWorkArea *work, VEndpoint **vpoint)
{
	Bundle		*bundle = &(work->bundle);
	char		*eidString;
	MetaEid		senderMetaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
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

		work->rawBundle = zco_clone(sdr, work->zco,
				work->zcoBytesConsumed, work->bundleLength);
		if (work->rawBundle <= 0)
		{
			putErrmsg("Can't clone bundle out of work area", NULL);
			return -1;
		}

		work->zcoBytesConsumed += work->bundleLength;
	}
	else
	{
		work->rawBundle = 0;
	}

	if (work->rawBundle == 0)
	{
		return 0;	/*	No bundle at front of work ZCO.	*/
	}

	/*	Reduce payload ZCO to just its source data, discarding
	 *	BP header and trailer.  This simplifies decryption.	*/

	bundle->payload.content = zco_clone(sdr, work->rawBundle,
			work->headerLength, bundle->payload.length);

	/*	Do all decryption indicated by extension blocks.	*/

	if (decryptPerExtensionBlocks(work) < 0)
	{
		putErrmsg("Failed parsing extension blocks.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	/*	Can now finish block acquisition for any blocks
	 *	that were originally encrypted.				*/

	if (parseExtensionBlocks(work) < 0)
	{
		putErrmsg("Failed parsing extension blocks.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	/*	Make sure all required security blocks are present.	*/

	switch (reviewExtensionBlocks(work))
	{
	case -1:
		putErrmsg("Failed reviewing extension blocks.", NULL);
		sdr_cancel_xn(sdr);
		return -1;

	case 0:
		writeMemo("[?] Malformed bundle: missing extension block(s).");
		bpInductTally(work->vduct, BP_INDUCT_MALFORMED,
				bundle->payload.length);
		return abortBundleAcq(work);
	}

	/*	Now that acquisition is complete, check the bundle
	 *	for problems.						*/

	if (work->malformed)
	{
		writeMemo("[?] Malformed bundle: extension blocks processing.");
		bpInductTally(work->vduct, BP_INDUCT_MALFORMED,
				bundle->payload.length);
		return abortBundleAcq(work);
	}

	if (work->congestive)
	{
		writeMemo("[?] ZCO space is congested; discarding bundle.");
		bpInductTally(work->vduct, BP_INDUCT_CONGESTIVE,
				bundle->payload.length);
		return discardReceivedBundle(work, SrDepletedStorage);
	}

	/*	Check authenticity and integrity.			*/

	initAuthenticity(work);	/*	Set default.			*/
	if (checkPerExtensionBlocks(work) < 0)
//<<-- Must call bpsec_securityPolicyViolated inside this check, somehow.
	{
		putErrmsg("Can't check bundle authenticity.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	if (bundle->corrupt)
	{
		writeMemo("[?] Corrupt bundle.");
		bpInductTally(work->vduct, BP_INDUCT_MALFORMED,
				bundle->payload.length);
		return abortBundleAcq(work);
	}

	if (bundle->clDossier.authentic == 0)
	{
		writeMemo("[?] Bundle judged inauthentic.");
		bpInductTally(work->vduct, BP_INDUCT_INAUTHENTIC,
				bundle->payload.length);
		return abortBundleAcq(work);
	}

	if (bundle->altered)
	{
		writeMemo("[?] Altered bundle.");
		bpInductTally(work->vduct, BP_INDUCT_INAUTHENTIC,
				bundle->payload.length);
		return abortBundleAcq(work);
	}
/*
	if (bpsec_securityPolicyViolated(work))
	{
		writeMemo("[?] Security policy violated.");
		bpInductTally(work->vduct, BP_INDUCT_INAUTHENTIC,
				bundle->payload.length);
		return abortBundleAcq(work);
	}
*/
	/*	Unintelligible extension headers don't make a bundle
	 *	malformed (though we count it that way), but they may
	 *	make it necessary to discard the bundle.		*/

	if (work->mustAbort)
	{
		bpInductTally(work->vduct, BP_INDUCT_MALFORMED,
				bundle->payload.length);
		return discardReceivedBundle(work, SrBlockUnintelligible);
	}

	/*	We have decided to accept the bundle rather than
	 *	simply discard it, so now we commit it to SDR space.
	 *	Re-do calculation of dbOverhead, since the extension
	 *	recording function may result in extension scratchpad
	 *	objects of different sizes than were calculated during
	 *	extension block acquisition.				*/

	bundle->dbOverhead = BASE_BUNDLE_OVERHEAD;
	bundle->acct = ZcoInbound;

	/*	Record bundle's sender EID, if known.			*/

	if (work->senderEid.schemeCodeNbr != unknown)
	{
		if (readEid(&(work->senderEid), &eidString) < 0)
		{
		       putErrmsg("Can't read EID.", NULL);
		       sdr_cancel_xn(sdr);
		       return -1;
		} 

		if (parseEidString(eidString, &senderMetaEid, &vscheme,
				&vschemeElt) == 0)
		{
			restoreEidString(&senderMetaEid);
			putErrmsg("Can't parse sender EID.", eidString);
			sdr_cancel_xn(sdr);
			return -1;
		}

		bundle->clDossier.senderNodeNbr = senderMetaEid.elementNbr;
		if (writeEid(&bundle->clDossier.senderEid, &senderMetaEid) < 0)
		{
			restoreEidString(&senderMetaEid);
			putErrmsg("No space for sender EID.", eidString);
			sdr_cancel_xn(sdr);
			return -1;
		}

		MRELEASE(eidString);
		if (bundle->clDossier.senderEid.schemeCodeNbr == dtn)
		{
			bundle->dbOverhead +=
				bundle->clDossier.senderEid.ssp.dtn.nssLength;
		}
	}

	/*	Record all bundle EIDs as well.				*/

	if (recordBundleEid(bundle, &(bundle->id.source)) < 0)
	{
		putErrmsg("Can't record source EID.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	if (recordBundleEid(bundle, &(bundle->destination)) < 0)
	{
		putErrmsg("Can't record destination EID.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	if (recordBundleEid(bundle, &(bundle->reportTo)) < 0)
	{
		putErrmsg("Can't record reportTo EID.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	/*	Construct other bundle stuctures.			*/

	bundle->stations = sdr_list_create(sdr);
	bundle->trackingElts = sdr_list_create(sdr);
	bundleObj = sdr_malloc(sdr, sizeof(Bundle));
	if (bundleObj == 0)
	{
		putErrmsg("No space for bundle object.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	computeExpirationTime(bundle);
	if (setBundleTTL(bundle, bundleObj) < 0)
	{
		putErrmsg("Can't insert new bundle into timeline.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	if (catalogueBundle(bundle, bundleObj) < 0)
	{
		putErrmsg("Can't catalogue new bundle in hash table.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	if (recordExtensionBlocks(work) < 0)
	{
		putErrmsg("Can't record extensions.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	noteBundleInserted(bundle);
	bpInductTally(work->vduct, BP_INDUCT_RECEIVED, bundle->payload.length);
	bpRecvTally(bundle->classOfService, bundle->payload.length);
	if ((_bpvdb(NULL))->watching & WATCH_y)
	{
		iwatch('y');
	}

	/*	Other decisions and reporting are left to the
	 *	forwarder, as they depend on route availability.
	 *
	 *	Note that at this point we have NOT yet written
	 *	the bundle structure itself into the SDR space we
	 *	have allocated for it (bundleObj).  dispatchBundle()
	 *	will do this, in enqueuing it for delivery and/or
	 *	in enqueuing it for forwarding.				*/

	if (dispatchBundle(bundleObj, bundle, vpoint) < 0)
	{
		putErrmsg("Can't dispatch bundle.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	return 0;
}

static int	checkIncompleteBundle(Bundle *newFragment, VEndpoint *vpoint)
{
	Sdr		sdr = getIonsdr();
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

	fragmentsList = sdr_list_list(sdr, newFragment->fragmentElt);
	incElt = sdr_list_user_data(sdr, fragmentsList);
	incObj = sdr_list_data(sdr, incElt);
	GET_OBJ_POINTER(sdr, IncompleteBundle, incomplete, incObj);
	endOfFurthestFragment = 0;
	for (elt = sdr_list_first(sdr, incomplete->fragments); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, Bundle, fragment,
				sdr_list_data(sdr, elt));
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

	elt = sdr_list_first(sdr, incomplete->fragments);
	aggregateBundleObj = sdr_list_data(sdr, elt);
	sdr_stage(sdr, (char *) &aggregateBundle, aggregateBundleObj,
			sizeof(Bundle));
	sdr_list_delete(sdr, aggregateBundle.fragmentElt, NULL, NULL);
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
		elt = sdr_list_first(sdr, incomplete->fragments);
		if (elt == 0)
		{
			break;
		}

		fragmentObj = sdr_list_data(sdr, elt);
		sdr_stage(sdr, (char *) &fragBuf, fragmentObj,
				sizeof(Bundle));
		bytesToSkip = aggregateAduLength - fragBuf.id.fragmentOffset;
		if (bytesToSkip < fragBuf.payload.length)
		{
			bytesToCopy = fragBuf.payload.length - bytesToSkip;
			if (zco_clone_source_data(sdr,
					aggregateBundle.payload.content,
					fragBuf.payload.content,
					bytesToSkip, bytesToCopy) < 0)
			{
				putErrmsg("Can't append extent.", NULL);
				return -1;
			}

			aggregateAduLength += bytesToCopy;
		}

		sdr_list_delete(sdr, fragBuf.fragmentElt, NULL, NULL);
		fragBuf.fragmentElt = 0;
		sdr_write(sdr, fragmentObj, (char *) &fragBuf,
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
	Sdr		sdr = getIonsdr();
	int		result;
	VEndpoint	*vpoint;
	int		acqLength;

	CHKERR(work);
	CHKERR(work->zco);
	CHKERR(sdr_begin_xn(sdr));
	work->zcoLength = zco_length(sdr, work->zco);
	zco_start_receiving(work->zco, &(work->reader));
	result = advanceWorkBuffer(work, 0);
	if (sdr_end_xn(sdr) < 0 || result < 0)
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
		CHKERR(sdr_begin_xn(sdr));
		result = acquireBundle(sdr, work, &vpoint);
		if (work->rawBundle)
		{
			zco_destroy(sdr, work->rawBundle);
		}

		if (sdr_end_xn(sdr) < 0 || result < 0)
		{
			putErrmsg("Bundle acquisition failed.", NULL);
			return -1;
		}

		/*	Has acquisition of this bundle enabled
		 *	reassembly of an original bundle?		*/

		if (vpoint != NULL && work->bundle.fragmentElt != 0)
		{
			CHKERR(sdr_begin_xn(sdr));
			result = checkIncompleteBundle(&work->bundle, vpoint);
			if (sdr_end_xn(sdr) < 0 || result < 0)
			{
				putErrmsg("Bundle acquisition failed.", NULL);
				return -1;
			}
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

/*	*	*	Status report functions		*	*	*/

static int	serializeStatusRpt(Bundle *bundle, Object *zco)
{
	Sdr		sdr = getIonsdr();
	BpStatusRpt	*rpt = &(bundle->statusRpt);
	uvast		uvtemp;
	unsigned char	rptbuf[500];
	unsigned char	*cursor;
	int		eidLength;
	int		rptLength;
	Object		sourceData;

	CHKERR(bundle);
	CHKERR(zco);
	cursor = rptbuf;
	memset(rptbuf, 0, sizeof rptbuf);

	/*	Sending an admin record, an array of 2 items.		*/

	uvtemp = 2;
	oK(cbor_encode_array_open(uvtemp, &cursor));

	/*	First item of admin record is record type code.		*/

	uvtemp = BP_STATUS_REPORT;
	oK(cbor_encode_integer(uvtemp, &cursor));

	/*	Second item of admin record is content, the status
	 *	report, which is an array of either 4 or 6 items.	*/

	if (rpt->isFragment)
	{
		uvtemp = 6;
	}
	else
	{
		uvtemp = 4;
	}

	oK(cbor_encode_array_open(uvtemp, &cursor));

	/*	The first item of the status report is an array
	 *	of status assertions.					*/

	uvtemp = 4;		/*	Array of status assertions.	*/
	oK(cbor_encode_array_open(uvtemp, &cursor));

	/*	Received.						*/

	if (rpt->flags & BP_RECEIVED_RPT)
	{
		if (rpt->receiptTime)
		{
			uvtemp = 2;
			oK(cbor_encode_array_open(uvtemp, &cursor));
			uvtemp = 1;
			oK(cbor_encode_integer(uvtemp, &cursor));
			uvtemp = rpt->receiptTime;
			oK(cbor_encode_integer(uvtemp, &cursor));
		}
		else
		{
			uvtemp = 1;
			oK(cbor_encode_array_open(uvtemp, &cursor));
			uvtemp = 1;
			oK(cbor_encode_integer(uvtemp, &cursor));
		}
	}
	else
	{
		uvtemp = 1;
		oK(cbor_encode_array_open(uvtemp, &cursor));
		uvtemp = 0;
		oK(cbor_encode_integer(uvtemp, &cursor));
	}

	/*	Forwarded.						*/

	if (rpt->flags & BP_FORWARDED_RPT)
	{
		if (rpt->forwardTime)
		{
			uvtemp = 2;
			oK(cbor_encode_array_open(uvtemp, &cursor));
			uvtemp = 1;
			oK(cbor_encode_integer(uvtemp, &cursor));
			uvtemp = rpt->forwardTime;
			oK(cbor_encode_integer(uvtemp, &cursor));
		}
		else
		{
			uvtemp = 1;
			oK(cbor_encode_array_open(uvtemp, &cursor));
			uvtemp = 1;
			oK(cbor_encode_integer(uvtemp, &cursor));
		}
	}
	else
	{
		uvtemp = 1;
		oK(cbor_encode_array_open(uvtemp, &cursor));
		uvtemp = 0;
		oK(cbor_encode_integer(uvtemp, &cursor));
	}

	/*	Delivered.						*/

	if (rpt->flags & BP_DELIVERED_RPT)
	{
		if (rpt->deliveryTime)
		{
			uvtemp = 2;
			oK(cbor_encode_array_open(uvtemp, &cursor));
			uvtemp = 1;
			oK(cbor_encode_integer(uvtemp, &cursor));
			uvtemp = rpt->deliveryTime;
			oK(cbor_encode_integer(uvtemp, &cursor));
		}
		else
		{
			uvtemp = 1;
			oK(cbor_encode_array_open(uvtemp, &cursor));
			uvtemp = 1;
			oK(cbor_encode_integer(uvtemp, &cursor));
		}
	}
	else
	{
		uvtemp = 1;
		oK(cbor_encode_array_open(uvtemp, &cursor));
		uvtemp = 0;
		oK(cbor_encode_integer(uvtemp, &cursor));
	}

	/*	Deleted.						*/

	if (rpt->flags & BP_DELETED_RPT)
	{
		if (rpt->deletionTime)
		{
			uvtemp = 2;
			oK(cbor_encode_array_open(uvtemp, &cursor));
			uvtemp = 1;
			oK(cbor_encode_integer(uvtemp, &cursor));
			uvtemp = rpt->deletionTime;
			oK(cbor_encode_integer(uvtemp, &cursor));
		}
		else
		{
			uvtemp = 1;
			oK(cbor_encode_array_open(uvtemp, &cursor));
			uvtemp = 1;
			oK(cbor_encode_integer(uvtemp, &cursor));
		}
	}
	else
	{
		uvtemp = 1;
		oK(cbor_encode_array_open(uvtemp, &cursor));
		uvtemp = 0;
		oK(cbor_encode_integer(uvtemp, &cursor));
	}

	/*	The second item is the reason code.			*/

	uvtemp = rpt->reasonCode;
	oK(cbor_encode_integer(uvtemp, &cursor));

	/*	The third item is the source node ID.			*/

	eidLength = serializeEid(&(bundle->id.source), cursor);
	cursor += eidLength;

	/*	The fourth item is the creation timestamp.		*/

	uvtemp = 2;
	oK(cbor_encode_array_open(uvtemp, &cursor));
	uvtemp = bundle->id.creationTime.seconds;
	oK(cbor_encode_integer(uvtemp, &cursor));
	uvtemp = bundle->id.creationTime.count;
	oK(cbor_encode_integer(uvtemp, &cursor));

	/*	If applicable, the fifth and sixth items are the
	 *	fragment offset & total ADU length.			*/

	if (rpt->isFragment)
	{
		uvtemp = bundle->id.fragmentOffset;
		oK(cbor_encode_integer(uvtemp, &cursor));
		uvtemp = bundle->totalAduLength;
		oK(cbor_encode_integer(uvtemp, &cursor));
	}

	/*	Administrative record has now been serialized.		*/

	rptLength = cursor - rptbuf;
	CHKERR(sdr_begin_xn(sdr));
	sourceData = sdr_malloc(sdr, rptLength);
	if (sourceData == 0)
	{
		putErrmsg("No space for source data.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	sdr_write(sdr, sourceData, (char *) rptbuf, rptLength);

	/*	Pass additive inverse of length to zco_create to
	 *	indicate that allocating this ZCO space is non-
	 *	negotiable: for status reports, allocation of
	 *	ZCO space can never be denied or delayed.		*/

	*zco = zco_create(sdr, ZcoSdrSource, sourceData, 0, 0 - rptLength,
			ZcoOutbound);
	if (sdr_end_xn(sdr) < 0 || *zco == (Object) ERROR || *zco == 0)
	{
		putErrmsg("Can't create status report.", NULL);
		return -1;
	}

	return 0;
}

int	sendStatusRpt(Bundle *bundle)
{
	int		priority = bundle->priority;
	BpAncillaryData	ecos = { 0, 0, bundle->ancillaryData.ordinal };
	Object		payloadZco = 0;
	unsigned int	ttl;	/*	Original bundle's TTL.		*/
	char		*reportToEid;
	int		result;

	CHKERR(bundle);
	result = serializeStatusRpt(bundle, &payloadZco);
	if (result < 0)
	{
		putErrmsg("Can't construct status report.", NULL);
		return -1;
	}

	ttl = bundle->timeToLive;
	if (ttl < 1) ttl = 1;
	if (readEid(&bundle->reportTo, &reportToEid) < 0)
	{
		putErrmsg("Can't recover report-to EID string.", NULL);
		return -1;
	}

	result = bpSend(NULL, reportToEid, NULL, ttl, priority,
			NoCustodyRequested, 0, 0, &ecos, payloadZco, NULL,
			BP_STATUS_REPORT);
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

	/*	Erase flags and times in case another status report for
	 *	the same bundle needs to be sent later.			*/

	bundle->statusRpt.flags = 0;
	bundle->statusRpt.reasonCode = 0;
	bundle->statusRpt.receiptTime = 0;
	bundle->statusRpt.forwardTime = 0;
	bundle->statusRpt.deliveryTime = 0;
	bundle->statusRpt.deletionTime = 0;
	return 0;
}

int	parseStatusRpt(BpStatusRpt *rpt, unsigned char *cursor,
	       		unsigned int unparsedBytes)
{
	uvast		arrayLength;
	uvast		uvtemp;
	int		length;
	uvast		itemsRemaining;
	uvast		statusAssertionsRemaining;
	int		i;
	DtnTime		statusTime;

	memset((char *) rpt, 0, sizeof(BpStatusRpt));
	
	/*	Start parsing of status report.				*/

	arrayLength = 0;	/*	Decode array of any size.	*/
	if (cbor_decode_array_open(&arrayLength, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode status report array.");
		return 0;
	}

	itemsRemaining = arrayLength;
	switch (itemsRemaining)
	{
	case 6:
		rpt->isFragment = 1;
		break;

	case 4:
		rpt->isFragment = 0;
		break;

	default:
		writeMemoNote("[?] Malformed status report",
				itoa(itemsRemaining));
		return 0;
	}

	/*	Start parsing of status information.			*/

	arrayLength = 0;	/*	Decode array of any size.	*/
	if (cbor_decode_array_open(&arrayLength, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode status report array.");
		return 0;
	}

	statusAssertionsRemaining = arrayLength;
	if (statusAssertionsRemaining < 4)
	{
		writeMemoNote("[?] Malformed status information",
				itoa(statusAssertionsRemaining));
		return 0;
	}

	/*	Parse all status assertions.				*/

	for (i = 0; i < statusAssertionsRemaining; i++)
	{
		/*	Start parsing of status assertion.		*/

		arrayLength = 0;	/*	Decode array.		*/
		if (cbor_decode_array_open(&arrayLength, &cursor,
				&unparsedBytes) < 1)
		{
			writeMemo("[?] Can't decode status report array.");
			return 0;
		}

		/*	Decode status switch.				*/

		if (cbor_decode_integer(&uvtemp, CborAny, &cursor,
					&unparsedBytes) < 1)
		{
			writeMemo("[?] Can't decode status assertion switch.");
			return 0;
		}

		if (uvtemp == 1)	/*	Status i is reported.	*/
		{
			if (arrayLength == 2)
			{
				if (cbor_decode_integer(&uvtemp, CborAny,
						&cursor, &unparsedBytes) < 1)
				{
					writeMemo("[?] Can't decode SR time.");
					return 0;
				}

				statusTime = uvtemp;
			}
			else
			{
				statusTime = 0;
			}

			switch (i)
			{
			case 0:
				rpt->flags |= BP_RECEIVED_RPT;
				rpt->receiptTime = statusTime;
				break;

			case 1:
				rpt->flags |= BP_FORWARDED_RPT;
				rpt->forwardTime = statusTime;
				break;

			case 2:
				rpt->flags |= BP_DELIVERED_RPT;
				rpt->deliveryTime = statusTime;
				break;

			case 3:
				rpt->flags |= BP_DELETED_RPT;
				rpt->deletionTime = statusTime;
				break;

			default:
				break;
			}
		}
	}

	/*	Parse reason code.					*/

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode reason code.");
		return 0;
	}

	rpt->reasonCode = uvtemp;

	/*	Parse source.						*/

	length = acquireEid(&(rpt->sourceEid), &cursor, &unparsedBytes);
	if (length < 1)
	{
		writeMemo("[?] Can't decode status report bundle source.");
		return length;
	}

	/*	Parse creation timestamp: array of seconds, count.	*/

	arrayLength = 2;	/*	Decode array of size 2.		*/
	if (cbor_decode_array_open(&arrayLength, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode status report creation timestamp.");
		return 0;
	}

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't acquire bundle creation time.");
		return 0;
	}

	rpt->creationTime.seconds = uvtemp;
	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't acquire bundle creation count.");
		return 0;
	}

	rpt->creationTime.count = uvtemp;
	if (rpt->isFragment == 0)
	{
		return 1;	/*	Done.				*/
	}

	/*	Bundle was a fragment, so parse its offset and length.	*/

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't acquire bundle fragment offset.");
		return 0;
	}

	rpt->fragmentOffset = uvtemp;
	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't acquire bundle fragment length.");
		return 0;
	}

	rpt->fragmentLength = uvtemp;
	return 1;
}

/*	*	*	Bundle catenation functions	*	*	*/

int	serializeEid(EndpointId *eid, unsigned char *buffer)
{
	/*	Note: we assume that the buffer is large enough to
	 *	hold a serialized EID, i.e., 300 bytes.  The largest 
	 *	allowable DTN SSP is 255 (which lets the SSP be stored
	 *	in an sdrstring).					*/

	uvast		uvtemp;
	unsigned char	*cursor = buffer;
	char		*eidbuf;
	char		*nss;

	uvtemp = 2;
	oK(cbor_encode_array_open(uvtemp, &cursor));
	uvtemp = eid->schemeCodeNbr;
	oK(cbor_encode_integer(uvtemp, &cursor));
	switch (eid->schemeCodeNbr)
	{
	case dtn:
		if (readEid(eid, &eidbuf) < 0)
		{
			putErrmsg("Can't serialize EID NSS.", NULL);
			return -1;
		}

		if (strcmp(eidbuf, "dtn:none") == 0)
		{
			oK(cbor_encode_integer(0, &cursor));
		}
		else
		{
			nss = eidbuf + 4;
			uvtemp = istrlen(nss, MAX_NSS_LEN);
			oK(cbor_encode_text_string(nss, uvtemp, &cursor));
		}

		MRELEASE(eidbuf);
		break;

	case ipn:
		uvtemp = 2;
		oK(cbor_encode_array_open(uvtemp, &cursor));
		uvtemp = eid->ssp.ipn.nodeNbr;
		oK(cbor_encode_integer(uvtemp, &cursor));
		uvtemp = eid->ssp.ipn.serviceNbr;
		oK(cbor_encode_integer(uvtemp, &cursor));
		break;

	case imc:
		uvtemp = 2;
		oK(cbor_encode_array_open(uvtemp, &cursor));
		uvtemp = eid->ssp.imc.groupNbr;
		oK(cbor_encode_integer(uvtemp, &cursor));
		uvtemp = eid->ssp.imc.serviceNbr;
		oK(cbor_encode_integer(uvtemp, &cursor));
		break;

	default:
		putErrmsg("Can't serialize EID, unknown scheme",
				itoa(eid->schemeCodeNbr));
		return 0;
	}

	return cursor - buffer;
}

static int	catenateBundle(Bundle *bundle)
{
	Sdr		sdr = getIonsdr();
	unsigned char	destinationEid[300];
	int		destinationEidLength;
	unsigned char	sourceEid[300];
	int		sourceEidLength;
	unsigned char	reportToEid[300];
	int		reportToEidLength;
	int		maxHeaderLength;
	unsigned char	*buffer;
	unsigned char	*cursor;
	uvast		uvtemp;
	unsigned char	*startOfPrimaryBlock;
	int		bundleIsFragment = 0;
	uint16_t	crc16;
	Object		elt;
	Object		blkAddr;
	ExtensionBlock	blk;
	int		totalHeaderLength;
	unsigned char	payloadBuffer[50];
	unsigned char	*cursor2;
	int		payloadBlockHeaderLength;
	uvast		crc;
	uint32_t	crc32;
	unsigned char	crcBuffer[8];
	unsigned char	*cursor3;
	ZcoReader	reader;
	unsigned char	breakChar[1];
	unsigned char	*cursor4;

	CHKZERO(ionLocked());

	/*	We assume that the bundle to be issued is valid:
	 *	either it was sourced locally (in which case we
	 *	created it ourselves, so it should be valid) or
	 *	else it was received from elsewhere (in which case
	 *	it was created by the acquisition functions, which
	 *	would have discarded the inbound bundle if it were
	 *	not well-formed).					*/

	/*	Must first determine size of buffer in which to
	 *	serialize the header of the bundle.  To do this,
	 *	we need to serialize all three endpoint IDs.		*/

	CHKERR((destinationEidLength = serializeEid(&(bundle->destination),
			destinationEid)) > 0);
	CHKERR((sourceEidLength = serializeEid(&(bundle->id.source),
			sourceEid)) > 0);
	CHKERR((reportToEidLength = serializeEid(&(bundle->reportTo),
			reportToEid)) > 0);

	/*	Can now compute max header length: 50 for remainder
	 *	of primary block, plus 20 times the total number of
	 *	canonical blocks (including the payload block), plus
	 *	the aggregate length of all extension blocks' block-
	 *	specific data length.					*/

	maxHeaderLength = 50
			+ destinationEidLength
			+ sourceEidLength
			+ reportToEidLength
			+ (20 * (1 + sdr_list_length(sdr, bundle->extensions)))
			+ bundle->extensionsLength;
	buffer = MTAKE(maxHeaderLength);
	if (buffer == NULL)
	{
		putErrmsg("Can't serialize bundle header.", NULL);
		return -1;
	}

	cursor = buffer;

	/*	Serialize indefinite-length array buffer.		*/

	uvtemp = ((uvast) -1);
	oK(cbor_encode_array_open(uvtemp, &cursor));

	/*	Serialize primary block.				*/

	startOfPrimaryBlock = cursor;
	if (bundle->bundleProcFlags & BDL_IS_FRAGMENT)
	{
		bundleIsFragment = 1;
	}

	if (bundleIsFragment)
	{
		uvtemp = 11;
	}
	else
	{
		uvtemp = 9;
	}

	oK(cbor_encode_array_open(uvtemp, &cursor));

	/*	Version.						*/

	uvtemp = BP_VERSION;
	oK(cbor_encode_integer(uvtemp, &cursor));

	/*	Bundle processing flags.				*/

	uvtemp = bundle->bundleProcFlags;
	oK(cbor_encode_integer(uvtemp, &cursor));

	/*	Primary block CRC type.					*/

	uvtemp = X25CRC16;
	oK(cbor_encode_integer(uvtemp, &cursor));

	/*	Destination.						*/

	memcpy(cursor, destinationEid, destinationEidLength);
	cursor += destinationEidLength;

	/*	Source.							*/

	memcpy(cursor, sourceEid, sourceEidLength);
	cursor += sourceEidLength;

	/*	Report-to.						*/

	memcpy(cursor, reportToEid, reportToEidLength);
	cursor += reportToEidLength;

	/*	Creation timestamp array.				*/

	uvtemp = 2;
	oK(cbor_encode_array_open(uvtemp, &cursor));

	/*	Creation time seconds.					*/

	uvtemp = bundle->id.creationTime.seconds;
	oK(cbor_encode_integer(uvtemp, &cursor));

	/*	Creation time count.					*/

	uvtemp = bundle->id.creationTime.count;
	oK(cbor_encode_integer(uvtemp, &cursor));

	/*	TTL.							*/

	uvtemp = bundle->timeToLive;
	oK(cbor_encode_integer(uvtemp, &cursor));

	/*	Fragment ID, if applicable.				*/

	if (bundleIsFragment)
	{
		/*	Fragment offset.				*/

		uvtemp = bundle->id.fragmentOffset;
		oK(cbor_encode_integer(uvtemp, &cursor));

		/*	Total ADU length.				*/

		uvtemp = bundle->totalAduLength;
		oK(cbor_encode_integer(uvtemp, &cursor));
	}

	/*	Compute and insert primary block CRC.			*/

	crc16 = 0;
	oK(cbor_encode_byte_string((unsigned char *) &crc16, 2, &cursor));
	crc16 = ion_CRC16_1021_X25((char *) startOfPrimaryBlock,
				cursor - startOfPrimaryBlock, 0);
	crc16 = htons(crc16);
	memcpy(cursor - 2, (char *) &crc16, 2);

	/*	Done with primary block, now insert extension blocks.	*/

	for (elt = sdr_list_first(sdr, bundle->extensions); elt;
			elt = sdr_list_next(sdr, elt))
	{
		blkAddr = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &blk, blkAddr, sizeof(ExtensionBlock));
		if (blk.suppressed)
		{
			continue;
		}

		/*	"bytes" array of extension block is already
		 *	serialized.					*/

		sdr_read(sdr, (char *) cursor, blk.bytes, blk.length);
		cursor += blk.length;
	}

	/*	Done with everything preceding the payload block.	*/

	totalHeaderLength = cursor - buffer;

	/*	Serialize payload block.				*/

	cursor2 = payloadBuffer;
	uvtemp = (bundle->payload.crcType == NoCRC ? 5 : 6);
	oK(cbor_encode_array_open(uvtemp, &cursor2));

	/*	Block type and number are fixed.			*/

	uvtemp = 1;		/*	Payload block type is 1.	*/
	oK(cbor_encode_integer(uvtemp, &cursor2));
	uvtemp = 1;		/*	Payload block number is 1.	*/
	oK(cbor_encode_integer(uvtemp, &cursor2));

	/*	Block processing flags.					*/

	uvtemp = bundle->payloadBlockProcFlags;
	oK(cbor_encode_integer(uvtemp, &cursor2));

	/*	Payload block CRC type.					*/

	uvtemp = bundle->payload.crcType;
	oK(cbor_encode_integer(uvtemp, &cursor2));

	/*	Payload byte string (CBOR header only at this point).	*/

	uvtemp = bundle->payload.length;
	oK(cbor_encode_byte_string(NULL, uvtemp, &cursor2));

	/*	Done with payload block header.				*/

	payloadBlockHeaderLength = cursor2 - payloadBuffer;

	/*	Prepend payload block header to payload ZCO.		*/

	oK(zco_prepend_header(sdr, bundle->payload.content,
			(char *) payloadBuffer, payloadBlockHeaderLength));

	/*	Compute and serialize payload block CRC if applicable.	*/

	if (bundle->payload.crcType != NoCRC)
	{
		/*	Compute CRC over the entire payload block,
		 *	including the CRC itself (temporarily 0).	*/

		cursor3 = crcBuffer;
		zco_start_transmitting(bundle->payload.content, &reader);
		if (computeZcoCrc(bundle->payload.crcType, &reader,
			bundle->payload.length + payloadBlockHeaderLength,
			&crc, NULL) < 0)
		{
			MRELEASE(buffer);
			putErrmsg("Can't serialize payload block.", NULL);
			return -1;
		}

		/*	Append the computed CRC to the payload ZCO.	*/

		if (bundle->payload.crcType == X25CRC16)
		{
			crc16 = crc;
			crc16 = htons(crc16);
			oK(cbor_encode_byte_string((unsigned char *) &crc16,
					2, &cursor3));
			oK(zco_append_trailer(sdr, bundle->payload.content,
					(char *) crcBuffer, 3));
		}
		else
		{
			crc32 = crc;
			crc32 = htonl(crc32);
			oK(cbor_encode_byte_string((unsigned char *) &crc32,
					4, &cursor3));
			oK(zco_append_trailer(sdr, bundle->payload.content,
					(char *) crcBuffer, 5));
		}
	}

	/*	Prepend bundle header (all other blocks) to payload ZCO.*/

	oK(zco_prepend_header(sdr, bundle->payload.content, (char *) buffer,
			totalHeaderLength));
	MRELEASE(buffer);

	/*	Terminate indefinite array by appending break character
	 *	to the payload ZCO (now the concatenated bundle).	*/

	cursor4 = breakChar;
	oK(cbor_encode_break(&cursor4));
	oK(zco_append_trailer(sdr, bundle->payload.content, (char *) breakChar,
			1));
	return 0;
}

/*	*	*	Bundle transmission queue functions	*	*/

int	bpAccept(Object bundleObj, Bundle *bundle)
{
	CHKERR(bundleObj && bundle);
	CHKERR(ionLocked());
	if (!bundle->accepted)	/*	Accept bundle only once.	*/
	{
		if (sendRequestedStatusReports(bundle) < 0)
		{
			putErrmsg("Failed sending status reports.", NULL);
			return -1;
		}

		bundle->accepted = 1;
	}

	sdr_write(getIonsdr(), bundleObj, (char *) bundle, sizeof(Bundle));
	return 0;
}

static void	noteFragmentation(Bundle *bundle)
{
	Sdr	sdr = getIonsdr();
	Object	dbObject;
	BpDB	db;

	dbObject = getBpDbObject();
	sdr_stage(sdr, (char *) &db, dbObject, sizeof(BpDB));
	db.currentFragmentsProduced++;
	db.totalFragmentsProduced++;
	if (!(bundle->fragmented))
	{
		bundle->fragmented = 1;
		db.currentBundlesFragmented++;
		db.totalBundlesFragmented++;
	}

	sdr_write(sdr, dbObject, (char *) &db, sizeof(BpDB));
}

int	bpFragment(Bundle *bundle, Object bundleObj,
		Object *queueElt, size_t fragmentLength,
		Bundle *firstBundle, Object *firstBundleObj,
		Bundle *secondBundle, Object *secondBundleObj)
{
	Sdr	sdr = getIonsdr();

	CHKERR(ionLocked());

	/*	Create two clones of the original bundle with
	 *	fragmentary payloads.					*/

	if (bpClone(bundle, firstBundle, firstBundleObj, 0, fragmentLength) < 0
	|| bpClone(bundle, secondBundle, secondBundleObj, fragmentLength,
			bundle->payload.length - fragmentLength) < 0)
	{
		return -1;
	}

	/*	Lose the original bundle, inserting the two fragments
	 *	in its place.  No significant change to resource
	 *	occupancy.						*/

	if (queueElt)
	{
		sdr_list_delete(sdr, *queueElt, NULL, NULL);
		*queueElt = 0;
	}

	/*	Destroy the original bundle.				*/

	sdr_write(sdr, bundleObj, (char *) bundle, sizeof(Bundle));
	if (bpDestroyBundle(bundleObj, 0) < 0)
	{
		return -1;
	}

	/*	Note bundle has been fragmented, and return.		*/

	noteFragmentation(secondBundle);
	return 0;
}

static Object	insertBundleIntoQueue(Object queue, Object firstElt,
			Object lastElt, Object bundleAddr, int priority,
			unsigned char ordinal, time_t enqueueTime)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(Bundle, bundle);
	Object	nextElt;

	/*	Bundles have transmission seniority which must be
	 *	honored.  A bundle that was enqueued for transmission
	 *	a while ago and now is being reforwarded must jump
	 *	the queue ahead of bundles of the same priority that
	 *	were enqueued more recently.				*/

	GET_OBJ_POINTER(sdr, Bundle, bundle, sdr_list_data(sdr, lastElt));
	if (enqueueTime < bundle->enqueueTime)
	{
		/*	Must be re-enqueuing a previously queued
		 *	bundle.						*/

		nextElt = firstElt;
		while (1)
		{
			if (nextElt == lastElt)
			{
				/*	Insert just before last elt.	*/

				return sdr_list_insert_before(sdr, nextElt,
						bundleAddr);
			}

			GET_OBJ_POINTER(sdr, Bundle, bundle,
					sdr_list_data(sdr, nextElt));
			if (enqueueTime < bundle->enqueueTime)
			{
				/*	Insert before this one.		*/

				return sdr_list_insert_before(sdr, nextElt,
						bundleAddr);
			}

			nextElt = sdr_list_next(sdr, nextElt);
		}
	}

	/*	Enqueue time >= last elt's enqueue time, so insert
	 *	after the last elt.					*/

	return sdr_list_insert_after(sdr, lastElt, bundleAddr);
}

static Object	enqueueUrgentBundle(BpPlan *plan, Bundle *bundle,
			Object bundleObj, int backlogIncrement)
{
	Sdr		sdr = getIonsdr();
	unsigned char	ordinal = bundle->ordinal;
	OrdinalState	*ord = &(plan->ordinals[ordinal]);
	Object		lastElt = 0;
	Object		lastForPriorOrdinal = 0;
	Object		firstElt = 0;
	int		i;
	Object		xmitElt;

	/*	Enqueue the new bundle immediately after the last
	 *	currently enqueued bundle whose ordinal is equal to
	 *	or greater than that of the new bundle.			*/

	for (i = 0; i < 256; i++)
	{
		lastElt = plan->ordinals[i].lastForOrdinal;
		if (lastElt == 0)
		{
			continue;
		}

		if (i < ordinal)
		{
			lastForPriorOrdinal = lastElt;
			continue;
		}

		/*	i >= ordinal, so we have found the end of
		 *	the applicable sub-list.  The first bundle
		 *	after the last bundle of the last preceding
		 *	non-empty sub-list (if any) must be the first
		 *	bundle of the applicable sub-list.  (Possibly
		 *	also the last.)					*/

		if (lastForPriorOrdinal)
		{
			firstElt = sdr_list_next(sdr, lastForPriorOrdinal);
		}
		else
		{
			firstElt = lastElt;
		}

		break;
	}

	if (i == 256)	/*	No more urgent bundle to enqueue after.	*/
	{
		xmitElt = sdr_list_insert_first(sdr, plan->urgentQueue,
			bundleObj);
	}
	else		/*	Enqueue after this one.			*/
	{
		xmitElt = insertBundleIntoQueue(plan->urgentQueue, firstElt,
			lastElt, bundleObj, 2, ordinal, bundle->enqueueTime);
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

int	bpEnqueue(VPlan *vplan, Bundle *bundle, Object bundleObj)
{
	Sdr		sdr = getIonsdr();
	Object		planObj;
	BpPlan		plan;
	unsigned int	backlogIncrement;
	time_t		enqueueTime;
	int		priority;
	Object		lastElt;

	CHKERR(ionLocked());
	CHKERR(vplan && bundle && bundleObj);
	CHKERR(bundle->planXmitElt == 0);
	bpDbTally(BP_DB_FWD_OKAY, bundle->payload.length);
	planObj = sdr_list_data(sdr, vplan->planElt);
	sdr_stage(sdr, (char *) &plan, planObj, sizeof(BpPlan));

	/*	We have settled on a neighboring node to forward
	 *	this bundle to.
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
		if (isLoopback(vplan->neighborEid))
		{
			return 0;
		}
	}

	/*      Now construct transmission parameters.			*/

	bundle->proxNodeEid = sdr_string_create(sdr, vplan->neighborEid);
	if (processExtensionBlocks(bundle, PROCESS_ON_ENQUEUE, NULL) < 0)
	{
		putErrmsg("Can't process extensions.", "enqueue");
		return -1;
	}

	backlogIncrement = computeECCC(guessBundleSize(bundle));
	if (bundle->enqueueTime == 0)
	{
		bundle->enqueueTime = enqueueTime = getCtime();
	}
	else
	{
		enqueueTime = bundle->enqueueTime;
	}

	/*	Insert bundle into the appropriate transmission queue
	 *	of the selected egress plan.				*/

	priority = bundle->priority;
	switch (priority)
	{
	case 0:
		lastElt = sdr_list_last(sdr, plan.bulkQueue);
		if (lastElt == 0)
		{
			bundle->planXmitElt = sdr_list_insert_first(sdr,
				plan.bulkQueue, bundleObj);
		}
		else
		{
			bundle->planXmitElt =
				insertBundleIntoQueue(plan.bulkQueue,
				sdr_list_first(sdr, plan.bulkQueue),
				lastElt, bundleObj, 0, 0, enqueueTime);
		}

		increaseScalar(&plan.bulkBacklog, backlogIncrement);
		break;

	case 1:
		lastElt = sdr_list_last(sdr, plan.stdQueue);
		if (lastElt == 0)
		{
			bundle->planXmitElt = sdr_list_insert_first(sdr,
				plan.stdQueue, bundleObj);
		}
		else
		{
			bundle->planXmitElt =
			       	insertBundleIntoQueue(plan.stdQueue,
				sdr_list_first(sdr, plan.stdQueue),
				lastElt, bundleObj, 1, 0, enqueueTime);
		}

		increaseScalar(&plan.stdBacklog, backlogIncrement);
		break;

	default:
		bundle->planXmitElt = enqueueUrgentBundle(&plan,
				bundle, bundleObj, backlogIncrement);
		increaseScalar(&plan.urgentBacklog, backlogIncrement);
	}

	sdr_write(sdr, planObj, (char *) &plan, sizeof(BpPlan));
	sdr_write(sdr, bundleObj, (char *) bundle, sizeof(Bundle));
	if ((_bpvdb(NULL))->watching & WATCH_b)
	{
		iwatch('b');
	}

	bpPlanTally(vplan, BP_PLAN_ENQUEUED, bundle->payload.length);
	if (vplan->semaphore != SM_SEM_NONE)
	{
		sm_SemGive(vplan->semaphore);
	}

	return 0;
}

int	enqueueToLimbo(Bundle *bundle, Object bundleObj)
{
	Sdr	sdr = getIonsdr();
	BpDB	*bpConstants = getBpConstants();

	/*      ION has determined that this bundle must wait
	 *      in limbo until a plan is unblocked, enabling
	 *      transmission.  So append bundle to the "limbo"
	 *      list.							*/

	CHKERR(ionLocked());
	CHKERR(bundleObj && bundle);
	CHKERR(bundle->planXmitElt == 0);
	if (bundle->ancillaryData.flags & BP_MINIMUM_LATENCY)
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
		sdr_free(sdr, bundle->proxNodeEid);
		bundle->proxNodeEid = 0;
	}

	bundle->planXmitElt = sdr_list_insert_last(sdr,
			bpConstants->limboQueue, bundleObj);
	sdr_write(sdr, bundleObj, (char *) bundle, sizeof(Bundle));
	bpDbTally(BP_DB_TO_LIMBO, bundle->payload.length);
	if ((_bpvdb(NULL))->watching & WATCH_limbo)
	{
		iwatch('j');
	}

	return 0;
}

int	reverseEnqueue(Object xmitElt, BpPlan *plan, int sendToLimbo)
{
	Sdr	sdr = getIonsdr();
	Object	bundleAddr;
	Bundle	bundle;

	CHKERR(xmitElt && plan);
	bundleAddr = sdr_list_data(sdr, xmitElt);
	sdr_stage(sdr, (char *) &bundle, bundleAddr, sizeof(Bundle));
	removeBundleFromQueue(&bundle, plan);
	if (bundle.proxNodeEid)
	{
		sdr_free(sdr, bundle.proxNodeEid);
		bundle.proxNodeEid = 0;
	}

	sdr_write(sdr, bundleAddr, (char *) &bundle, sizeof(Bundle));

	/*	If bundle is MINIMUM_LATENCY, nothing more to do.  We
	 *	never reforward critical bundles or send them to limbo.	*/

	if (bundle.ancillaryData.flags & BP_MINIMUM_LATENCY)
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

	return enqueueToLimbo(&bundle, bundleAddr);
}

int	bpBlockPlan(char *eid)
{
	Sdr		sdr = getIonsdr();
	VPlan		*vplan;
	PsmAddress	vplanElt;
	Object		planObj;
       	BpPlan		plan;
	Object		xmitElt;
	Object		nextElt;

	CHKERR(eid);
	findPlan(eid, &vplan, &vplanElt);
	if (vplanElt == 0)
	{
		writeMemoNote("[?] Can't find plan to block", eid);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	planObj = sdr_list_data(sdr, vplan->planElt);
	sdr_stage(sdr, (char *) &plan, planObj, sizeof(BpPlan));
	if (plan.blocked)
	{
		sdr_exit_xn(sdr);
		return 0;	/*	Already blocked, nothing to do.	*/
	}

	plan.blocked = 1;

	/*	Send into limbo all bundles currently queued for
	 *	transmission to this node.				*/

	for (xmitElt = sdr_list_first(sdr, plan.urgentQueue); xmitElt;
			xmitElt = nextElt)
	{
		nextElt = sdr_list_next(sdr, xmitElt);
		if (reverseEnqueue(xmitElt, &plan, 0))
		{
			putErrmsg("Can't requeue urgent bundle.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}
	}

	for (xmitElt = sdr_list_first(sdr, plan.stdQueue); xmitElt;
			xmitElt = nextElt)
	{
		nextElt = sdr_list_next(sdr, xmitElt);
		if (reverseEnqueue(xmitElt, &plan, 0))
		{
			putErrmsg("Can't requeue std bundle.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}
	}

	for (xmitElt = sdr_list_first(sdr, plan.bulkQueue); xmitElt;
			xmitElt = nextElt)
	{
		nextElt = sdr_list_next(sdr, xmitElt);
		if (reverseEnqueue(xmitElt, &plan, 0))
		{
			putErrmsg("Can't requeue bulk bundle.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}
	}

	sdr_write(sdr, planObj, (char *) &plan, sizeof(BpPlan));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Failed blocking plan.", NULL);
		return -1;
	}

	return 0;
}

int	releaseFromLimbo(Object xmitElt, int resuming)
{
	Sdr	sdr = getIonsdr();
	Object	bundleAddr;
	Bundle	bundle;

	CHKERR(ionLocked());
	CHKERR(xmitElt);
	bundleAddr = sdr_list_data(sdr, xmitElt);
	sdr_stage(sdr, (char *) &bundle, bundleAddr, sizeof(Bundle));
	if (bundle.suspended)
	{
		if (resuming)
		{
			bundle.suspended = 0;
		}
		else	/*	Merely unblocking a blocked plan.	*/
		{
			return 0;	/*	Can't release yet.	*/
		}
	}

	/*	Erase this bogus transmission reference.  Note that
	 *	by deleting the planXmitElt object in this bundle
	 *	we are deleting the xmitElt that was passed to
	 *	this function -- don't count on being able to
	 *	navigate to the next xmitElt in limboQueue from it!	*/

	sdr_list_delete(sdr, bundle.planXmitElt, NULL, NULL);
	bundle.planXmitElt = 0;
	sdr_write(sdr, bundleAddr, (char *) &bundle, sizeof(Bundle));
	bpDbTally(BP_DB_FROM_LIMBO, bundle.payload.length);
	if ((_bpvdb(NULL))->watching & WATCH_delimbo)
	{
		iwatch('k');
	}

	/*	Now see if the bundle can finally be transmitted.	*/

	if (bpReforwardBundle(bundleAddr) < 0)
	{
		putErrmsg("Failed releasing bundle from limbo.", NULL);
		return -1;
	}

	return 1;
}

int	bpUnblockPlan(char *eid)
{
	Sdr		sdr = getIonsdr();
	VPlan		*vplan;
	PsmAddress	vplanElt;
	BpDB		*bpConstants = getBpConstants();
	Object		planObj;
       	BpPlan		plan;
	Object		xmitElt;
	Object		nextElt;

	CHKERR(eid);
	findPlan(eid, &vplan, &vplanElt);
	if (vplanElt == 0)
	{
		writeMemoNote("[?] Can't find plan to unblock", eid);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	planObj = sdr_list_data(sdr, vplan->planElt);
	sdr_stage(sdr, (char *) &plan, planObj, sizeof(BpPlan));
	if (plan.blocked == 0)
	{
		sdr_exit_xn(sdr);
		return 0;	/*	Not blocked, nothing to do.	*/
	}

	plan.blocked = 0;
	sdr_write(sdr, planObj, (char *) &plan, sizeof(BpPlan));

	/*	Release all non-suspended bundles currently in limbo,
	 *	in case the unblocking of this plan enables some
	 *	or all of them to be queued for transmission.		*/

	for (xmitElt = sdr_list_first(sdr, bpConstants->limboQueue);
			xmitElt; xmitElt = nextElt)
	{
		nextElt = sdr_list_next(sdr, xmitElt);
		if (releaseFromLimbo(xmitElt, 0) < 0)
		{
			putErrmsg("Failed releasing bundle from limbo.", NULL);
			break;
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Failed unblocking plan.", NULL);
		return -1;
	}

	return 0;
}

int	bpAbandon(Object bundleObj, Bundle *bundle, int reason)
{
	int		result1 = 0;
	int		result2 = 0;
	BpSrReason	srReason;

	CHKERR(bundleObj && bundle);
	if (reason == BP_REASON_DEPLETION)
	{
		srReason = SrDepletedStorage;
	}
	else
	{
		srReason = SrNoKnownRoute;
	}

	bpDbTally(BP_DB_FWD_FAILED, bundle->payload.length);
	if (SRR_FLAGS(bundle->bundleProcFlags) & BP_DELETED_RPT)
	{
		bundle->statusRpt.flags |= BP_DELETED_RPT;
		bundle->statusRpt.reasonCode = srReason;
		if (bundle->bundleProcFlags & BDL_STATUS_TIME_REQ)
		{
			getCurrentDtnTime(&(bundle->statusRpt.deletionTime));
		}
	}

	if (bundle->statusRpt.flags)
	{
		result1 = sendStatusRpt(bundle);
		if (result1 < 0)
		{
			putErrmsg("Can't send status report.", NULL);
		}
	}

	bpDelTally(srReason);

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
		iwatch('~');
	}

	return ((result1 + result2) == 0 ? 0 : -1);
}

int	bpDequeue(VOutduct *vduct, Object *bundleZco,
		BpAncillaryData *ancillaryData, int timeoutInterval)
{
	Sdr		sdr = getIonsdr();
	int		stewardshipAccepted;
	Object		outductObj;
	Outduct		outduct;
			OBJ_POINTER(ClProtocol, protocol);
	Object		elt;
	Object		bundleObj;
	Bundle		bundle;
	BundleSet	bset;
	char		proxNodeEid[SDRSTRING_BUFSZ];
	VPlan		*vplan;
	PsmAddress	vplanElt;
	DequeueContext	context;

	CHKERR(vduct && bundleZco && ancillaryData);
	*bundleZco = 0;			/*	Default behavior.	*/
	if (timeoutInterval < 0)	/*	Reliable CLA.		*/
	{
		stewardshipAccepted = 1;
	}
	else
	{
		stewardshipAccepted = 0;
	}

	outductObj = sdr_list_data(sdr, vduct->outductElt);
	sdr_read(sdr, (char *) &outduct, outductObj, sizeof(Outduct));
	CHKERR(sdr_begin_xn(sdr));
	GET_OBJ_POINTER(sdr, ClProtocol, protocol, outduct.protocol);

	/*	Get a transmittable bundle.				*/

	elt = sdr_list_first(sdr, outduct.xmitBuffer);
	while (elt == 0)
	{
		sdr_exit_xn(sdr);
		if (sm_SemTake(vduct->semaphore) < 0)
		{
			putErrmsg("CLO failed taking duct semaphore.",
					vduct->ductName);
			return -1;
		}

		if (sm_SemEnded(vduct->semaphore))
		{
			writeMemoNote("[i] Outduct has been stopped",
					vduct->ductName);
			return 0;
		}

		CHKERR(sdr_begin_xn(sdr));
		elt = sdr_list_first(sdr, outduct.xmitBuffer);
	}

	bundleObj = sdr_list_data(sdr, elt);
	if (bundleObj == 0)	/*	Outduct has been stopped.	*/
	{
		sdr_exit_xn(sdr);
		return 0;	/*	End task, but without error.	*/
	}

	sdr_stage(sdr, (char *) &bundle, bundleObj, sizeof(Bundle));
	sdr_list_delete(sdr, bundle.ductXmitElt, NULL, NULL);
	bundle.ductXmitElt = 0;
	vduct->timeOfLastXmit = getCtime();
	if (bundle.proxNodeEid)
	{
		sdr_string_read(sdr, proxNodeEid, bundle.proxNodeEid);
	}
	else
	{
		proxNodeEid[0] = '\0';
	}

	context.protocolName = protocol->name;
	context.proxNodeEid = proxNodeEid;
	findPlan(proxNodeEid, &vplan, &vplanElt);
	if (vplanElt)
	{
		context.xmitRate = vplan->xmitThrottle.nominalRate;
	}
	else
	{
		context.xmitRate = 0;
	}

	if (processExtensionBlocks(&bundle, PROCESS_ON_DEQUEUE, &context) < 0)
	{
		putErrmsg("Can't process extensions.", "dequeue");
		sdr_cancel_xn(sdr);
		return -1;
	}

	if (bundle.corrupt)
	{
		*bundleZco = 1;		/*	Client need not stop.	*/
		sdr_write(sdr, bundleObj, (char *) &bundle, sizeof(Bundle));
		if (bpDestroyBundle(bundleObj, 1) < 0)
		{
			putErrmsg("Failed trying to destroy bundle.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}

		return sdr_end_xn(sdr);
	}

	if (bundle.overdueElt)
	{
		/*	Bundle was transmitted before "overdue"
		 *	alarm went off, so disable the alarm.		*/

		destroyBpTimelineEvent(bundle.overdueElt);
		bundle.overdueElt = 0;
	}

	/*	We now serialize the bundle header and prepend that
	 *	header to the payload of the bundle.  This transforms
	 *	the payload ZCO into a fully catenated bundle, ready
	 *	for transmission.					*/

	if (catenateBundle(&bundle) < 0)
	{
		putErrmsg("Can't catenate bundle.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	/*	Some final extension-block processing may be necessary
	 *	after catenation of the bundle.				*/

	if (processExtensionBlocks(&bundle, PROCESS_ON_TRANSMIT, NULL) < 0)
	{
		putErrmsg("Can't process extensions.", "transmit");
		sdr_cancel_xn(sdr);
		return -1;
	}

	/*	That's the end of the changes to the bundle.  Pass
	 *	the catenated bundle (the payload ZCO) to the calling
	 *	function, then replace it in the bundle structure with
	 *	a clone of the payload source data only; cloning the
	 *	zco strips off the headers added by the catenation
	 *	function.  This is necessary in case the bundle is
	 *	subject to re-forwarding (due to stewardship) and
	 *	thus re-catenation.					*/

	*bundleZco = bundle.payload.content;
	bundle.payload.content = zco_clone(sdr, *bundleZco, 0,
			zco_source_data_length(sdr, *bundleZco));
	if (bundle.payload.content <= 0)
	{
		putErrmsg("Can't clone bundle.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	sdr_write(sdr, bundleObj, (char *) &bundle, sizeof(Bundle));

	/*	At this point we adjust the stewardshipAccepted flag.
	 *	If reliable transmission is requested for this
	 *	bundle, then stewardship is accepted pending further
	 *	checks.							*/

	if (bundle.ancillaryData.flags & BP_RELIABLE)
	{
		stewardshipAccepted = 1;
	}

	/*	If the bundle is critical then copies have been queued
	 *	for transmission on all possible routes and none of
	 *	them are subject to reforwarding (see notes on this in
	 *	bpReforwardBundle).  Since this bundle is not subject
	 *	to reforwarding, stewardship is meaningless -- on
	 *	convergence-layer transmission failure the attempt
	 *	to transmit the bundle is simply abandoned.  So
	 *	in this event the stewardshipAccepted flag is forced
	 *	to zero even if the convergence-layer adapter for
	 *	this outduct is one that normally accepts stewardship.	*/

	if (bundle.ancillaryData.flags & BP_MINIMUM_LATENCY)
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
		sdr_read(sdr, (char *) &bset, sdr_hash_entry_value(sdr,
				(_bpConstants())->bundles, bundle.hashEntry),
				sizeof(BundleSet));
		if (bset.bundleObj != bundleObj)
		{
			stewardshipAccepted = 0;
		}
	}

	/*	Last check: if the bundle is anonymous then its key
	 *	can never be guaranteed to be unique (because the
	 *	sending EID that qualifies the creation time and
	 *	sequence number is omitted), so the bundle can't
	 *	reliably be retrieved via the hash table.  So again
	 *	stewardship cannot be successfully accepted.		*/

	if (bundle.anonymous)
	{
		stewardshipAccepted = 0;
	}

	/*	Note that when stewardship is not accepted, the bundle
	 *	is subject to destruction immediately.
	 *
	 *	Return the outbound bundle's extended class of service.	*/

	memcpy((char *) ancillaryData, (char *) &bundle.ancillaryData,
			sizeof(BpAncillaryData));

	/*	Finally, authorize transmission of applicable status
	 *	report message and destruction of the bundle object
	 *	unless stewardship was successfully accepted.		*/

	if (!stewardshipAccepted)
	{
		if (SRR_FLAGS(bundle.bundleProcFlags) & BP_FORWARDED_RPT)
		{
			bundle.statusRpt.flags |= BP_FORWARDED_RPT;
			if (bundle.bundleProcFlags & BDL_STATUS_TIME_REQ)
			{
				getCurrentDtnTime
					(&(bundle.statusRpt.forwardTime));
			}

			if (sendStatusRpt(&bundle) < 0)
			{
				putErrmsg("Can't send status report.", NULL);
				sdr_cancel_xn(sdr);
				return -1;
			}
		}

		if (bpDestroyBundle(bundleObj, 0) < 0)
		{
			putErrmsg("Can't destroy bundle.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}
	}

	if (sdr_end_xn(sdr))
	{
		putErrmsg("Can't get outbound bundle.", NULL);
		return -1;
	}

	return 0;
}

static int	nextBlock(Sdr sdr, ZcoReader *reader, unsigned char *buffer,
			int *bytesBuffered, int blockLength)
{
	int	bytesToReceive;
	int	bytesReceived;

	/*	Shift buffer left by length of prior block.		*/

	*bytesBuffered -= blockLength;
	memmove(buffer, buffer + blockLength, *bytesBuffered);

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

static int	decodeHeader(Sdr sdr, ZcoReader *reader, unsigned char *buffer,
			int bytesBuffered, Bundle *image)
{
	unsigned char	*cursor;
	unsigned int	unparsedBytes;
	uvast		arrayLength;
	int		length;
	uvast		uvtemp;
	unsigned int	blockLength;
	BpCrcType	crcType;
	BpBlockType	blkType;
	unsigned int	blockDataLength;

	cursor = buffer;
	unparsedBytes = bytesBuffered;

	/*	Skip over bundle array tag.				*/

	arrayLength = 0;
	if (cbor_decode_array_open(&arrayLength, &cursor, &unparsedBytes) < 0)
	{
		return -1;
	}

	/*	Skip over primary block array tag.			*/

	arrayLength = 0;
	if (cbor_decode_array_open(&arrayLength, &cursor, &unparsedBytes) < 0)
	{
		return -1;
	}

	/*	Skip over version number.				*/

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 0)
	{
		return -1;
	}

	/*	Parse out the bundle processing flags.			*/

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 0)
	{
		return -1;
	}

	image->bundleProcFlags = uvtemp;

	/*	Parse out the CRC type.					*/

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 0)
	{
		return -1;
	}

	crcType = uvtemp;

	/*	Extract destination EID.				*/

	if (acquireEid(&(image->destination), &cursor, &unparsedBytes) < 0)
	{
		return -1;
	}

	/*	Extract source EID.					*/

	if (acquireEid(&(image->id.source), &cursor, &unparsedBytes) < 0)
	{
		return -1;
	}

	/*	Extract report-to EID.					*/

	if (acquireEid(&(image->reportTo), &cursor, &unparsedBytes) < 0)
	{
		return -1;
	}

	/*	Skip over creation timestamp array tag.			*/

	arrayLength = 2;
	if (cbor_decode_array_open(&arrayLength, &cursor, &unparsedBytes) < 0)
	{
		return -1;
	}

	/*	Get creation timestamp seconds.				*/

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 0)
	{
		return -1;
	}

	image->id.creationTime.seconds = uvtemp;

	/*	Get creation timestamp count.				*/

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 0)
	{
		return -1;
	}

	image->id.creationTime.count = uvtemp;

	/*	Skip over lifetime.					*/

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 0)
	{
		return -1;
	}

	/*	If bundle is not a fragment, we're done.		*/

	if ((image->bundleProcFlags & BDL_IS_FRAGMENT) == 0)
	{
		image->id.fragmentOffset = 0;
		image->payload.length = 0;
		return 0;	/*	All bundle ID info is known.	*/
	}

	/*	Bundle is a fragment, so fragment offset and length
	 *	(which is payload length) must be recovered in order
	 *	to have the complete ID of the bundle.
	 *
	 *	First get fragment offset and skip over the rest of
	 *	the primary block.  Then skip over all canonical blocks
	 *	until have got the payload block.  At that point the
	 *	payload length is known and the fragmentary bundle
	 *	can be retrieved.					*/

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 0)
	{
		return -1;
	}

	image->id.fragmentOffset = uvtemp;

	/*	Get total ADU length, cues serialized bundle retrieval.	*/

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 0)
	{
		return -1;
	}

	image->totalAduLength = uvtemp;

	/*	Must also skip over primary block's CRC, if any.	*/

	if (crcType != NoCRC)
	{
		if (crcType == X25CRC16)
		{
			length = 3;
		}
		else		/*	Must be 32-bit CRC.		*/
		{
			length = 5;
		}

		if (length > unparsedBytes)
		{
			writeMemo("[?] Can't decode outbound bundle, no CRC.");
			return -1;
		}

		cursor += length;
		unparsedBytes -= length;
	}

	/*	At this point, cursor has (in theory) been advanced
	 *	to first byte in buffer after the end of the primary
	 *	block.  (If there are still unparsed bytes, ignore
	 *	them; an implementation error.)	 Now skip over other
	 *	blocks until payload length is known.			*/

	while (1)
	{
		/*	Move any remaining unparsed data in the buffer
		 *	to the front of the buffer (overwriting the
		 *	block that we just parsed), and fill up the
		 *	buffer with more data from the ZCO so that
		 *	the entire next block is known to be in the
		 *	front of the buffer.				*/

		blockLength = cursor - buffer;
		if (nextBlock(sdr, reader, buffer, &bytesBuffered, blockLength)
				< 0)
		{
			return -1;
		}

		if (bytesBuffered < 1)
		{
			return -1;	/*	No more blocks.		*/
		}

		cursor = buffer;
		unparsedBytes = bytesBuffered;

		/*	Skip over canonical block array tag.		*/

		arrayLength = 0;
		if (cbor_decode_array_open(&arrayLength, &cursor,
				&unparsedBytes) < 0)
		{
			return -1;
		}

		/*	Extract blockType.				*/

		if (cbor_decode_integer(&uvtemp, CborAny, &cursor,
				&unparsedBytes) < 0)
		{
			return -1;
		}

		blkType = uvtemp;

		/*	Skip over block number.				*/

		if (cbor_decode_integer(&uvtemp, CborAny, &cursor,
				&unparsedBytes) < 0)
		{
			return -1;
		}

		/*	Skip over block processing flags.		*/

		if (cbor_decode_integer(&uvtemp, CborAny, &cursor,
				&unparsedBytes) < 0)
		{
			return -1;
		}

		/*	Parse out the CRC type.				*/

		if (cbor_decode_integer(&uvtemp, CborAny, &cursor,
				&unparsedBytes) < 0)
		{
			return -1;
		}

		crcType = uvtemp;

		/*	Get length of block-type-specific data.		*/

		uvtemp = (uvast) -1;
		if (cbor_decode_byte_string(NULL, &uvtemp, &cursor,
				&unparsedBytes) < 0)
		{
			return -1;
		}

		blockDataLength = uvtemp;
		if (blkType == PayloadBlk)
		{
			image->payload.length = blockDataLength;
			return 0;		/*	Done.		*/
		}

		/*	Not the payload block, so we have to keep
		 *	scanning.  Skip over block-type-specific data.	*/

		if (blockDataLength > unparsedBytes)
		{
			writeMemo("[?] Can't decode outbound bundle, extension \
block is too long.");
			return -1;
		}

		cursor += blockDataLength;
		unparsedBytes -= blockDataLength;

		/*	Must also skip over block's CRC, if any.	*/

		if (crcType != NoCRC)
		{
			if (crcType == X25CRC16)
			{
				length = 3;
			}
			else		/*	Must be 32-bit CRC.	*/
			{
				length = 5;
			}

			if (length > unparsedBytes)
			{
				writeMemo("[?] Can't decode outbound bundle, \
no CRC.");
				return -1;
			}

			cursor += length;
			unparsedBytes -= length;
		}
	}
}

int	decodeBundle(Sdr sdr, Object zco, unsigned char *buffer, Bundle *image)
{
	ZcoReader	reader;
	int		bytesBuffered;
	int		result;

	CHKERR(sdr && zco && buffer && image); 

	/*	This is an outbound bundle, so the primary block is
	 *	in a capsule and we use zco_transmit to re-read it.	*/

	zco_start_transmitting(zco, &reader);
	CHKERR(sdr_begin_xn(sdr));
	bytesBuffered = zco_transmit(sdr, &reader, BP_MAX_BLOCK_SIZE,
			(char *) buffer);
	if (bytesBuffered < 0)
	{
		/*	Guessing this memory is no longer occupied
		 *	by a ZCO.  Note decoding failure.		*/

		sdr_exit_xn(sdr);
		return -1;
	}

	result = decodeHeader(sdr, &reader, buffer, bytesBuffered, image);
	sdr_exit_xn(sdr);
	return result;
}

int	retrieveSerializedBundle(Object bundleZco, Object *bundleObj)
{
	Sdr		sdr = getIonsdr();
	unsigned char	*buffer;
	Bundle		image;
	int		result;
	char		*sourceEid;

	CHKERR(bundleZco && bundleObj);
	CHKERR(ionLocked());
	*bundleObj = 0;			/*	Default: not located.	*/
	buffer = (unsigned char *) MTAKE(BP_MAX_BLOCK_SIZE);
	if (buffer == NULL)
	{
		putErrmsg("Can't create buffer for reading blocks.", NULL);
		return -1;
	}

	memset((char *) & image, 0, sizeof image);
	result = decodeBundle(sdr, bundleZco, buffer, &image);
	MRELEASE(buffer);
       	if (result < 0)
	{
		putErrmsg("Can't extract bundle ID.", NULL);
		return -1;
	}

	/*	Recreate the source EID.				*/

	if (readEid(&image.id.source, &sourceEid) < 0)
	{
		putErrmsg("Can't recover source EID string.", NULL);
		return -1;
	}

	/*	Now use this bundle ID to retrieve the bundle.		*/

	result = findBundle(sourceEid, &image.id.creationTime,
			image.id.fragmentOffset, image.totalAduLength == 0 ? 0
			: image.payload.length, bundleObj);
	MRELEASE(sourceEid);
	eraseEid(&image.id.source);
	eraseEid(&image.destination);
	eraseEid(&image.reportTo);
	return (result < 0 ? result : 0);
}

int	bpHandleXmitSuccess(Object bundleZco)
{
	Sdr	sdr = getIonsdr();
	Object	bundleAddr;
	Bundle	bundle;
	int	result;

	CHKERR(bundleZco);
	CHKERR(sdr_begin_xn(sdr));
	if (retrieveSerializedBundle(bundleZco, &bundleAddr) < 0)
	{
		putErrmsg("Can't locate bundle for okay transmission.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	if (bundleAddr == 0)	/*	Bundle not found.		*/
	{
		zco_destroy(sdr, bundleZco);
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Failed handling xmit success.", NULL);
			return -1;
		}

		return 0;	/*	bpDestroyBundle already called.	*/
	}

	sdr_read(sdr, (char *) &bundle, bundleAddr, sizeof(Bundle));

	/*	Send "forwarded" status report if necessary.		*/

	if (SRR_FLAGS(bundle.bundleProcFlags) & BP_FORWARDED_RPT)
	{
		bundle.statusRpt.flags |= BP_FORWARDED_RPT;
		if (bundle.bundleProcFlags & BDL_STATUS_TIME_REQ)
		{
			getCurrentDtnTime(&(bundle.statusRpt.forwardTime));
		}

		result = sendStatusRpt(&bundle);
		if (result < 0)
		{
			putErrmsg("Can't send status report.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}
	}

	/*	At this point the bundle object is subject to
	 *	destruction unless the bundle is pending delivery
	 *	or the bundle is pending another transmission.
	 *	Note that the bundle's *payload* object won't be
	 *	destroyed until the calling CLO function destroys
	 *	bundleZco; that's the remaining reference to this
	 *	reference-counted object, even after the reference
	 *	inside the bundle object is destroyed.			*/

	if (bpDestroyBundle(bundleAddr, 0) < 0)
	{
		putErrmsg("Failed trying to destroy bundle.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	zco_destroy(sdr, bundleZco);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't handle transmission success.", NULL);
		return -1;
	}

	return 1;
}

int	bpHandleXmitFailure(Object bundleZco)
{
	Sdr	sdr = getIonsdr();
	Object	bundleAddr;
	Bundle	bundle;

	CHKERR(bundleZco);
	CHKERR(sdr_begin_xn(sdr));
	if (retrieveSerializedBundle(bundleZco, &bundleAddr) < 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't locate bundle for failed transmission.", NULL);
		return -1;
	}

	if (bundleAddr == 0)	/*	Bundle not found.		*/
	{
		zco_destroy(sdr, bundleZco);
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Failed handling xmit failure.", NULL);
			return -1;
		}

		return 0;	/*	No bundle, can't retransmit.	*/
	}

	sdr_read(sdr, (char *) &bundle, bundleAddr, sizeof(Bundle));

	/*	Note that the "clfail" statistics count failures of
	 *	convergence-layer transmission of bundles for which
	 *	stewardship was accepted.				*/

	if ((_bpvdb(NULL))->watching & WATCH_clfail)
	{
		iwatch('#');
	}

	bpDbTally(BP_DB_REQUEUED_FOR_FWD, bundle.payload.length);
	if (bpReforwardBundle(bundleAddr) < 0)
	{
		putErrmsg("Failed trying to re-forward bundle.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	zco_destroy(sdr, bundleZco);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't handle transmission failure.", NULL);
		return -1;
	}

	return 1;
}

int	bpReforwardBundle(Object bundleAddr)
{
	Sdr	sdr = getIonsdr();
	Bundle	bundle;
	char	*eidString;
	int	result;

	CHKERR(ionLocked());
	CHKERR(bundleAddr);
	sdr_stage(sdr, (char *) &bundle, bundleAddr, sizeof(Bundle));
	if (bundle.ancillaryData.flags & BP_MINIMUM_LATENCY)
	{
		/*	If the bundle is critical it has already
		 *	been queued for transmission on all possible
		 *	routes; re-forwarding would only delay
		 *	transmission.  So return immediately.
		 *
		 *	Some additional remarks on this topic:
		 *
		 *	The BP_MINIMUM_LATENCY QOS flag is all about
		 *	minimizing latency -- not about assuring
		 *	delivery.  If a critical bundle were reforwarded
		 *	due to (for example) stewardship failure, it
		 *	would likely end up being re-queued for multiple
		 *	outducts once again; frequent stewardship
		 *	failures could result in an exponential
		 *	proliferation of transmission references
		 *	for this bundle, severely reducing bandwidth
		 *	utilization and potentially preventing the
		 *	transmission of other bundles of equal or
		 *	greater importance and/or urgency.
		 *
		 *	If delivery of a given bundle is important,
		 *	then use high priority and/or a very long TTL
		 *	(to minimize the chance of it failing trans-
		 *	mission due to contact truncation) and use
		 *	reliable convergence-layer protocols, and
		 *	possibly also flag for end-to-end
		 *	acknowledgement at the application layer.
		 *	If delivery of a given bundle is urgent,
		 *	then use high priority and flag it for minimum
		 *	latency.  If delivery of a given bundle is
		 *	both important and urgent, *send it twice*
		 *	-- once with the high-urgency QOS markings
		 *	and then again with the high-importance QOS
		 *	markings.					*/

		return 0;
	}

	/*	Non-critical bundle, so let's compute another route
	 *	for it.  This may entail back-tracking through the
	 *	node from which we originally received the bundle.	*/

	bundle.returnToSender = 1;
	purgeStationsStack(&bundle);
	if (bundle.planXmitElt)
	{
		purgePlanXmitElt(&bundle);
	}

	if (bundle.ductXmitElt)
	{
		sdr_list_delete(sdr, bundle.ductXmitElt, NULL, NULL);
		bundle.ductXmitElt = 0;
	}

	if (bundle.proxNodeEid)
	{
		sdr_free(sdr, bundle.proxNodeEid);
		bundle.proxNodeEid = 0;
	}

	if (bundle.overdueElt)
	{
		destroyBpTimelineEvent(bundle.overdueElt);
		bundle.overdueElt = 0;
	}

	if (bundle.fwdQueueElt)
	{
		sdr_list_delete(sdr, bundle.fwdQueueElt, NULL, NULL);
		bundle.fwdQueueElt = 0;
	}

	sdr_write(sdr, bundleAddr, (char *) &bundle, sizeof(Bundle));
	if (readEid(&bundle.destination, &eidString) < 0)
	{
		putErrmsg("Can't recover destination EID string.", NULL);
		return -1;
	}

	result = forwardBundle(bundleAddr, &bundle, eidString);
	MRELEASE(eidString);
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

static void	shutDownAdminApp(int signum)
{
	isignal(SIGTERM, shutDownAdminApp);
	sm_SemEnd((_bpadminSap(NULL))->recvSemaphore);
}

static int	defaultSrh(BpDelivery *dlv, unsigned char *cursor,
			unsigned int unparsedBytes)
{
	return 0;
}

int	_handleAdminBundles(char *adminEid, StatusRptCB handleStatusRpt)
{
	Sdr		sdr = getIonsdr();
	int		running = 1;
	BpSAP		sap;
	BpDelivery	dlv;
	vast		recordLen;
	ZcoReader	reader;
	vast		bytesToParse;
	unsigned char	headerBuf[10];
	unsigned char	*cursor;
	unsigned int	unparsedBytes;
	vast		headerLen;
	int		adminRecType;
	unsigned int	buflen;
	unsigned char	*buffer;
	uvast		uvtemp;

	CHKERR(adminEid);
	if (handleStatusRpt == NULL)
	{
		handleStatusRpt = defaultSrh;
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

		/*	Read and strip off the admin record header:
		 *	array open (1 byte), record type code (up to
		 *	9 bytes).					*/

		CHKERR(sdr_begin_xn(sdr));
		recordLen = zco_source_data_length(sdr, dlv.adu);
		zco_start_receiving(dlv.adu, &reader);
		bytesToParse = zco_receive_source(sdr, &reader, 10,
				(char *) headerBuf);
		if (bytesToParse < 2)
		{
			putErrmsg("Can't receive admin record header.", NULL);
			oK(sdr_end_xn(sdr));
			bp_release_delivery(&dlv, 1);
			running = 0;
			continue;
		}

		cursor = headerBuf;
		unparsedBytes = bytesToParse;
		uvtemp = 2;	/*	Decode array of size 2.		*/
		if (cbor_decode_array_open(&uvtemp, &cursor, &unparsedBytes)
				< 1)
		{
			writeMemo("[?] Can't decode admin record array open.");
			oK(sdr_end_xn(sdr));
			bp_release_delivery(&dlv, 1);
			continue;
		}

		if (cbor_decode_integer(&uvtemp, CborAny, &cursor,
				&unparsedBytes) < 1)
		{
			writeMemo("[?] Can't decode admin record type.");
			oK(sdr_end_xn(sdr));
			bp_release_delivery(&dlv, 1);
			continue;
		}

		adminRecType = uvtemp;

		/*	Now strip off the admin record header, leaving
		 *	just the admin record content.			*/

		headerLen = cursor - headerBuf;
		zco_delimit_source(sdr, dlv.adu, headerLen,
				recordLen - headerLen);
		zco_strip(sdr, dlv.adu);
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't strip admin record.", NULL);
			oK(sdr_end_xn(sdr));
			bp_release_delivery(&dlv, 1);
			running = 0;
			continue;
		}

		/*	Now handle the administrative record.		*/

		if (adminRecType == BP_BIBE_PDU)
		{
			if (bibeHandleBpdu(&dlv) < 0)
			{
				putErrmsg("BIBE PDU handler failed.", NULL);
				running = 0;
			}

			bp_release_delivery(&dlv, 0);

			/*	Make sure other tasks have a chance
			 *	to run.					*/

			sm_TaskYield();
			continue;
		}

		/*	For the smaller administrative records, read
		 *	the entire admin record into memory buffer.	*/

		CHKERR(sdr_begin_xn(sdr));
		buflen = zco_source_data_length(sdr, dlv.adu);
		if ((buffer = MTAKE(buflen)) == NULL)
		{
			putErrmsg("Can't handle admin record.", NULL);
			oK(sdr_end_xn(sdr));
			bp_release_delivery(&dlv, 1);
			running = 0;
			continue;
		}

		zco_start_receiving(dlv.adu, &reader);
		bytesToParse = zco_receive_source(sdr, &reader, buflen,
				(char *) buffer);
		if (bytesToParse < 0)
		{
			putErrmsg("Can't receive admin record.", NULL);
			MRELEASE(buffer);
			oK(sdr_end_xn(sdr));
			bp_release_delivery(&dlv, 1);
			running = 0;
			continue;
		}

		oK(sdr_end_xn(sdr));
		cursor = buffer;
		unparsedBytes = bytesToParse;
		switch (adminRecType)
		{
		case BP_STATUS_REPORT:
			if (handleStatusRpt(&dlv, cursor, unparsedBytes) < 0)
			{
				putErrmsg("Status report handler failed.",
						NULL);
				running = 0;
			}

			break;			/*	Out of switch.	*/

		case BP_MULTICAST_PETITION:
			if (imcHandlePetition(&dlv, cursor, unparsedBytes) < 0)
			{
				putErrmsg("Multicast petition handler failed.",
						NULL);
				running = 0;
			}

			break;

		case BP_SAGA_MESSAGE:
			if (saga_receive(&dlv, cursor, unparsedBytes) < 0)
			{
				putErrmsg("Discovery history handler failed.",
						NULL);
				running = 0;
			}

			break;

		case BP_BIBE_SIGNAL:
			if (bibeHandleSignal(&dlv, cursor, unparsedBytes) < 0)
			{
				putErrmsg("BIBE custody signal handler failed.",
						NULL);
				running = 0;
			}

			break;

		default:	/*	Unknown or non-standard.	*/
			writeMemoNote("[?] Unknown admin record",
					itoa(adminRecType));
			break;			/*	Out of switch.	*/
		}

		MRELEASE(buffer);
		bp_release_delivery(&dlv, 1);

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	writeMemo("[i] Administrative endpoint terminated.");
	writeErrmsgMemos();
	return 0;
}
