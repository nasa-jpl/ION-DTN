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

#define	BP_VERSION		6

#define MAX_STARVATION		10
#define NOMINAL_BYTES_PER_SEC	(256 * 1024)
#define NOMINAL_PRIMARY_BLKSIZE	29

#define SDR_LIST_OVERHEAD	(WORD_SIZE * 4)
#define SDR_LIST_ELT_OVERHEAD	(WORD_SIZE * 4)
#define XMIT_REF_OVERHEAD	((WORD_SIZE * 4) + SDR_LIST_ELT_OVERHEAD)
#define	BASE_BUNDLE_OVERHEAD	(sizeof(Bundle) \
				+ SDR_LIST_OVERHEAD	/*	xmitRefs*/ \
				+ XMIT_REF_OVERHEAD	/*	one Ref	*/)

#ifndef IN_TRANSIT_KEY_LEN
#define	IN_TRANSIT_KEY_LEN	64
#endif

#define	IN_TRANSIT_KEY_BUFLEN	(IN_TRANSIT_KEY_LEN << 1)

#ifndef IN_TRANSIT_EST_ENTRIES
#define	IN_TRANSIT_EST_ENTRIES	1000
#endif

#ifndef IN_TRANSIT_SEARCH_LEN
#define	IN_TRANSIT_SEARCH_LEN	20
#endif

static int	sendCtSignal(Bundle *bundle, char *dictionary, int succeeded,
			BpCtReason reasonCode);
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
	
	if (db == NULL)
	{
		/*	Load constants into a conveniently accessed
		 *	structure.  Note that this CANNOT be treated
		 *	as a current database image in later
		 *	processing.					*/

		sdr_read(getIonsdr(), (char *) &buf, _bpdbObject(NULL),
				sizeof(BpDB));
		db = &buf;
	}
	
	return db;
}

static char	*_nullEid()
{
	return "dtn:none";
}

/*	Note that ION currently supports CBHE compression only for
 *	a single DTN endpoint scheme.					*/

static char		*_cbheSchemeName()
{
	return "ipn";
}

/*	This is the scheme name for the legacy endpoint naming scheme
 *	developed for the DTN2 implementation.				*/

static char		*_dtn2SchemeName()
{
	return "dtn";
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
	vscheme->cbhe = scheme.cbhe;
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

	if (vscheme->cbhe)
	{
		bpvdb->cbheScheme = addr;
	}

	vscheme->semaphore = SM_SEM_NONE;
	resetScheme(vscheme);
	return 0;
}

static void	dropScheme(VScheme *vscheme, PsmAddress vschemeElt)
{
	PsmPartition	bpwm = getIonwm();
	BpVdb		*bpvdb = _bpvdb(NULL);
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

	if (strcmp(vscheme->name, _cbheSchemeName()) == 0)
	{
		bpvdb->cbheScheme = 0;
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

	/*	Compute custodian EID for this scheme.			*/

	if (strcmp(vscheme->name, _cbheSchemeName()) == 0)
	{
		isprintf(vscheme->custodianEidString,
			sizeof vscheme->custodianEidString, "%.8s:%lu.0",
			_cbheSchemeName(), getOwnNodeNbr());
	}
	else if (strcmp(vscheme->name, _dtn2SchemeName()) == 0)
	{
#ifdef ION_NO_DNS
		istrcpy(hostNameBuf, "localhost", sizeof hostNameBuf);
#else
		getNameOfHost(hostNameBuf, MAXHOSTNAMELEN);
#endif
		isprintf(vscheme->custodianEidString,
				sizeof vscheme->custodianEidString,
				"%.15s://%.60s.dtn", _dtn2SchemeName(),
				hostNameBuf);
	}
	else	/*	Unknown scheme; no known custodian EID format.	*/
	{
		istrcpy(vscheme->custodianEidString, _nullEid(),
				sizeof vscheme->custodianEidString);
	}

	if (parseEidString(vscheme->custodianEidString,
			&metaEid, &vscheme2, &vschemeElt) == 0)
	{
		restoreEidString(&metaEid);
		writeMemoNote("[?] Malformed custodian EID string",
				vscheme->custodianEidString);
		vscheme->custodianSchemeNameLength = 0;
		vscheme->custodianNssLength = 0;
	}
	else
	{
		vscheme->custodianSchemeNameLength = metaEid.schemeNameLength;
		vscheme->custodianNssLength = metaEid.nssLength;

		/*	Make sure endpoint exists for custodian EID.	*/

		findEndpoint(vscheme->name, metaEid.nss, vscheme, &vpoint,
				&vpointElt);
		restoreEidString(&metaEid);
		if (vpointElt == 0)
		{
			if (addEndpoint(vscheme->custodianEidString,
					EnqueueBundle, NULL) < 1)
			{
				putErrmsg("Can't add custodian endpoint.",
						vscheme->custodianEidString);
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

static BpVdb	*_bpvdb(char **name)
{
	static BpVdb	*vdb = NULL;
	PsmPartition	wm;
	PsmAddress	vdbAddress;
	PsmAddress	elt;
	Sdr		sdr;
	Object		sdrElt;
	Object		addr;

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
		sdr_begin_xn(sdr);	/*	Just to lock memory.	*/
		vdbAddress = psm_zalloc(wm, sizeof(BpVdb));
		if (vdbAddress == 0)
		{
			sdr_exit_xn(sdr);
			putErrmsg("No space for dynamic database.", NULL);
			return NULL;
		}

		vdb = (BpVdb *) psp(wm, vdbAddress);
		memset((char *) vdb, 0, sizeof(BpVdb));
		vdb->creationTimeSec = 0;
		vdb->bundleCounter = 0;
		vdb->clockPid = ERROR;
		vdb->productionThrottle.semaphore = sm_SemCreate(SM_NO_KEY,
				SM_SEM_FIFO);
		sm_SemTake(vdb->productionThrottle.semaphore);
		vdb->productionThrottle.nominalRate = 0;
		if ((vdb->schemes = sm_list_create(wm)) == 0
		|| (vdb->inducts = sm_list_create(wm)) == 0
		|| (vdb->outducts = sm_list_create(wm)) == 0
		|| psm_catlg(wm, *name, vdbAddress) < 0)
		{
			sdr_exit_xn(sdr);
			putErrmsg("Can't initialize volatile database.", NULL);
			return NULL;
		}

		/*	Raise all schemes and all of their endpoints.	*/

		for (sdrElt = sdr_list_first(sdr, (_bpConstants())->schemes);
				sdrElt; sdrElt = sdr_list_next(sdr, sdrElt))
		{
			if (raiseScheme(sdrElt, vdb) < 0)
			{
				sdr_exit_xn(sdr);
				putErrmsg("Can't raise all schemes.", NULL);
				return NULL;
			}
		}

		/*	Raise all ducts for all CL protocol adapters.	*/

		for (sdrElt = sdr_list_first(sdr, (_bpConstants())->protocols);
				sdrElt; sdrElt = sdr_list_next(sdr, sdrElt))
		{
			addr = sdr_list_data(sdr, sdrElt);
			if (raiseProtocol(addr, vdb) < 0)
			{
				sdr_exit_xn(sdr);
				putErrmsg("Can't raise all protocols.", NULL);
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
	char		*bpvdbName = _bpvdbName();

	if (ionAttach() < 0)
	{
		putErrmsg("BP can't attach to ION.", NULL);
		return -1;
	}

	bpSdr = getIonsdr();

	/*	Recover the BP database, creating it if necessary.	*/

	sdr_begin_xn(bpSdr);
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
		bpdbBuf.inTransitHash = sdr_hash_create(bpSdr,
				IN_TRANSIT_KEY_LEN,
				IN_TRANSIT_EST_ENTRIES,
				IN_TRANSIT_SEARCH_LEN);
		bpdbBuf.inboundBundles = sdr_list_create(bpSdr);
		bpdbBuf.limboQueue = sdr_list_create(bpSdr);
		bpdbBuf.clockCmd = sdr_string_create(bpSdr, "bpclock");
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

	sdr_begin_xn(bpSdr);	/*	Just to lock memory.		*/

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

	sdr_begin_xn(bpSdr);	/*	Just to lock memory.		*/
	if (bpvdb->productionThrottle.semaphore != SM_SEM_NONE)
	{
		sm_SemEnd(bpvdb->productionThrottle.semaphore);
	}

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

	sdr_begin_xn(bpSdr);
	bpvdb->clockPid = ERROR;
	if (bpvdb->productionThrottle.semaphore == SM_SEM_NONE)
	{
		bpvdb->productionThrottle.semaphore = sm_SemCreate(SM_NO_KEY,
				SM_SEM_FIFO);
	}
	else
	{
		sm_SemUnend(bpvdb->productionThrottle.semaphore);
	}

	sm_SemTake(bpvdb->productionThrottle.semaphore);/*	Lock.	*/
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
		zco_destroy_reference(bpSdr, zco);
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
		sdr_begin_xn(bpSdr);
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

/*	*	*	Database occupancy functions	*	*	*/

void	manageProductionThrottle(BpVdb *bpvdb)
{
	IonDB		iondb;
	Throttle	*throttle;
	long		occupancy;
	long		unoccupied;

	sdr_read(getIonsdr(), (char *) &iondb, getIonDbObject(), sizeof(IonDB));
	throttle = &(bpvdb->productionThrottle);
	if (iondb.maxForecastInTransit > iondb.currentOccupancy)
	{
		occupancy = iondb.maxForecastInTransit;
	}
	else
	{
		occupancy = iondb.currentOccupancy;
	}

	unoccupied = iondb.occupancyCeiling - occupancy;
	throttle->capacity = unoccupied - iondb.receptionSpikeReserve;
	if (throttle->capacity > 0)
	{
		sm_SemGive(throttle->semaphore);
	}
}

static void	noteBundleInserted(Bundle *bundle)
{
	ionOccupy(bundle->dbTotal);
	manageProductionThrottle(getBpVdb());
}

static void	noteBundleRemoved(Bundle *bundle)
{
	ionVacate(bundle->dbTotal);
	manageProductionThrottle(getBpVdb());
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

	if (sscanf(metaEid->nss, "%20lu.%20lu", &(metaEid->nodeNbr),
			&(metaEid->serviceNbr)) < 2
	|| metaEid->nodeNbr == 0	/*	Must use "dtn:none".	*/
	|| metaEid->nodeNbr > MAX_CBHE_NODE_NBR
	|| metaEid->serviceNbr > MAX_CBHE_SERVICE_NBR)
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

static int	printCbheEid(CbheEid *eid, char **result)
{
	char	*eidString;
	int	eidLength = 46;

	/*	Printed EID string is
	 *
	 *	   ipn:<elementnbr>.<servicenbr>\0
	 *
	 *	so max EID string length is 3 for "ipn" plus 1 for
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

	if (eid->nodeNbr == 0)
	{
		istrcpy(eidString, _nullEid(), eidLength);
	}
	else
	{
		isprintf(eidString, eidLength, "ipn:%lu.%lu", eid->nodeNbr,
				eid->serviceNbr);
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
		return printCbheEid(&eid->c, result);
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

		switch (lookupEid(*eidBuffer, neighborClEid))
		{
		case -1:
			putErrmsg("Failed getting sender EID.", NULL);
			sm_Abort();

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

static void	lookUpDestScheme(Bundle *bundle, char *dictionary,
			VScheme **vscheme)
{
	PsmPartition	bpwm = getIonwm();
	BpVdb		*bpvdb = _bpvdb(NULL);
	PsmAddress	addr;
	char		*schemeName;
	PsmAddress	elt;

	if (dictionary == NULL)
	{
		*vscheme = NULL;	/*	Default.		*/
		if (!bundle->destination.cbhe)
		{
			return;		/*	Can't determine scheme.	*/
		}

		addr = bpvdb->cbheScheme;
		if (addr == 0)
		{
			return;		/*	Not active.		*/
		}

		*vscheme = (VScheme *) psp(bpwm, addr);
		return;
	}

	schemeName = dictionary + bundle->destination.d.schemeNameOffset;
	for (elt = sm_list_first(bpwm, bpvdb->schemes); elt;
			elt = sm_list_next(bpwm, elt))
	{
		*vscheme = (VScheme *) psp(bpwm, sm_list_data(bpwm, elt));
		if (strcmp((*vscheme)->name, schemeName) == 0)
		{
			return;
		}
	}

	if (elt == 0)			/*	No match found.		*/
	{
		*vscheme = NULL;
	}
}

void	noteStateStats(int stateIdx, Bundle *bundle)
{
	BpVdb		*bpvdb = _bpvdb(NULL);
	int		priority;
	BpClassStats	*stats;
	
	if (stateIdx < 0 || stateIdx > 7 || bundle == NULL)
	{
		return;
	}

	if (bundle->bundleProcFlags & BDL_IS_ADMIN)
	{
		priority = 3;		/*	Isolate admin traffic.	*/
	}
	else
	{
		priority = COS_FLAGS(bundle->bundleProcFlags) & 0x03;
	}

	stats = &(bpvdb->stateStats[stateIdx].stats[priority]);
	stats->bytes += bundle->payload.length;
	stats->bundles++;
}

static void	clearStateStats(int stateIdx)
{
	BpVdb		*bpvdb = _bpvdb(NULL);
	BpStateStats	*state;
	int		i;
	
	if (stateIdx < 0 || stateIdx > 7)
	{
		return;
	}

	state = &(bpvdb->stateStats[stateIdx]);
	for (i = 0; i < 4; i++)
	{
		state->stats[i].bytes = 0;
		state->stats[i].bundles = 0;
	}
}

void	clearAllStateStats()
{
	BpVdb		*bpvdb = _bpvdb(NULL);
	int		i;
	BpStateStats	*state;
	time_t		currentTime;

	for (i = 0; i < 8; i++)
	{
		state = &(bpvdb->stateStats[i]);
		if (state->stats[0].bundles > 0
		|| state->stats[1].bundles > 0
		|| state->stats[2].bundles > 0
		|| state->stats[3].bundles > 0)
		{
			break;	/*	Found activity.			*/
		}
	}

	if (i == 8)		/*	No activity.			*/
	{
		return;		/*	Don't clear.			*/
	}

	currentTime = getUTCTime();
	for (i = 0; i < 8; i++)
	{
		clearStateStats(i);
	}

	bpvdb->statsStartTime = currentTime;
}

static void	reportStateStats(int stateIdx)
{
	BpVdb		*bpvdb = _bpvdb(NULL);
	time_t		startTime;
	time_t		currentTime;
	char		fromTimestamp[20];
	char		toTimestamp[20];
	BpStateStats	*state;
	char		buffer[256];
	static char	*classnames[] =
		{ "src", "fwd", "xmt", "rcv", "dlv", "ctr", "rfw", "exp" };
	
	if (stateIdx < 0 || stateIdx > 7)
	{
		return;
	}

	currentTime = getUTCTime();
	writeTimestampLocal(currentTime, toTimestamp);
	startTime = bpvdb->statsStartTime;
	writeTimestampLocal(startTime, fromTimestamp);
	state = &(bpvdb->stateStats[stateIdx]);
	isprintf(buffer, sizeof buffer, "[x] %s from %s to %s: (0) %u %lu (1) \
%u %lu (2) %u %lu (@) %u %lu", classnames[stateIdx], fromTimestamp, toTimestamp,
			state->stats[0].bundles, state->stats[0].bytes,
			state->stats[1].bundles, state->stats[1].bytes,
			state->stats[2].bundles, state->stats[2].bytes,
			state->stats[3].bundles, state->stats[3].bytes);
	writeMemo(buffer);
}

void	reportAllStateStats()
{
	BpVdb		*bpvdb = _bpvdb(NULL);
	int		i;
	BpStateStats	*state;

	for (i = 0; i < 8; i++)
	{
		state = &(bpvdb->stateStats[i]);
		if (state->stats[0].bundles > 0
		|| state->stats[1].bundles > 0
		|| state->stats[2].bundles > 0
		|| state->stats[3].bundles > 0)
		{
			break;	/*	Found activity.			*/
		}
	}

	if (i == 8)		/*	No activity.			*/
	{
		return;		/*	Don't report.			*/
	}

	for (i = 0; i < 8; i++)
	{
		reportStateStats(i);
	}
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

	sdr_free(bpSdr, sdr_list_data(bpSdr, incElt));
	sdr_list_delete(bpSdr, incElt, NULL, NULL);
	return 0;
}

static void	removeBundleFromQueue(Object ductXmitElt, Bundle *bundle,
			ClProtocol *protocol, Object outductObj,
			Outduct *outduct)
{
	Sdr		bpSdr = getIonsdr();
	int		backlogDecrement;
	OrdinalState	*ord;

	sdr_list_delete(bpSdr, ductXmitElt, NULL, NULL);

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
		if (ord->lastForOrdinal == ductXmitElt)
		{
			ord->lastForOrdinal = 0;
		}

		reduceScalar(&(outduct->urgentBacklog), backlogDecrement);
	}
}

static void	purgeDuctXmitElt(Bundle *bundle, Object ductXmitElt)
{
	Sdr		bpSdr = getIonsdr();
	Object		queue;
	Object		outductObj;
	Outduct		outduct;
	ClProtocol	protocol;

	queue = sdr_list_list(bpSdr, ductXmitElt);
	outductObj = sdr_list_user_data(bpSdr, queue);
	if (outductObj == 0)	/*	Bundle is in Limbo queue.	*/
	{
		sdr_list_delete(bpSdr, ductXmitElt, NULL, NULL);
		return;
	}

	sdr_stage(bpSdr, (char *) &outduct, outductObj, sizeof(Outduct));
	sdr_read(bpSdr, (char *) &protocol, outduct.protocol,
			sizeof(ClProtocol));
	removeBundleFromQueue(ductXmitElt, bundle, &protocol, outductObj,
			&outduct);
	sdr_write(bpSdr, outductObj, (char *) &outduct, sizeof(Outduct));
}

static void	purgeXmitRefs(Bundle *bundle)
{
	Sdr	bpSdr = getIonsdr();
	Object	elt;
	Object	nextElt;
	Object	xrAddr;
		OBJ_POINTER(XmitRef, xr);

	if (bundle->xmitRefs == 0)
	{
		return;
	}

	bundle->xmitsNeeded = 0;

	/*	Remove bundle from all transmission queues.		*/

	for (elt = sdr_list_first(bpSdr, bundle->xmitRefs); elt; elt = nextElt)
	{
		nextElt = sdr_list_next(bpSdr, elt);
		xrAddr = sdr_list_data(bpSdr, elt);
		sdr_list_delete(bpSdr, elt, NULL, NULL);
		GET_OBJ_POINTER(bpSdr, XmitRef, xr, xrAddr);
		purgeDuctXmitElt(bundle, xr->ductXmitElt);
		if (xr->proxNodeEid)
		{
			sdr_free(bpSdr, xr->proxNodeEid);
		}

		if (xr->destDuctName)
		{
			sdr_free(bpSdr, xr->destDuctName);
		}

		sdr_free(bpSdr, xrAddr);
	}
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

	/*	Remove bundle from all transmission queues.		*/

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
	Sdr	bpSdr = getIonsdr();
	Bundle	bundle;
		OBJ_POINTER(IncompleteBundle, incomplete);
	char	*dictionary;
	int	result;
	Object	elt;

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

		/*	Remove any remaining transmission references.	*/

		if (bundle.xmitRefs)
		{
			purgeXmitRefs(&bundle);
		}

		/*	Notify sender, if so requested or if custodian.	*/

		noteStateStats(BPSTATS_EXPIRE, &bundle);
		if ((_bpvdb(NULL))->watching & WATCH_expire)
		{
			putchar('!');
			fflush(stdout);
		}

		if (bundle.custodyTaken
		|| (SRR_FLAGS(bundle.bundleProcFlags) & BP_DELETED_RPT))
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

	if (bundle.dlvQueueElt || bundle.fragmentElt || bundle.fwdQueueElt
	|| bundle.xmitsNeeded > 0 || bundle.custodyTaken)
	{
		return 0;	/*	Can't destroy bundle yet.	*/
	}

	/*	Remove bundle from timeline.				*/

	sdr_free(bpSdr, sdr_list_data(bpSdr, bundle.timelineElt));
	sdr_list_delete(bpSdr, bundle.timelineElt, NULL, NULL);

	/*	Turn off automatic re-forwarding.			*/

	if (bundle.overdueElt)
	{
		sdr_free(bpSdr, sdr_list_data(bpSdr, bundle.overdueElt));
		sdr_list_delete(bpSdr, bundle.overdueElt, NULL, NULL);
	}

	if (bundle.ctDueElt)
	{
		sdr_free(bpSdr, sdr_list_data(bpSdr, bundle.ctDueElt));
		sdr_list_delete(bpSdr, bundle.ctDueElt, NULL, NULL);
	}

	if (bundle.inTransitEntry)
	{
		sdr_hash_delete_entry(bpSdr, bundle.inTransitEntry);
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

	/*	Remove BP's own reference to the bundle's payload
	 *	ZCO.  There may still be some application reference
	 *	to this ZCO, but if not then deleting the bundle's
	 *	reference will implicitly destroy the ZCO itself.	*/

	if (bundle.payload.content)
	{
		zco_destroy_reference(bpSdr, bundle.payload.content);
	}

	/*	Destroy all SDR objects managed for this bundle and
	 *	free space occupied by the bundle itself.		*/

	if (bundle.clDossier.senderEid.text)
	{
		sdr_free(bpSdr, bundle.clDossier.senderEid.text);
	}

	if (bundle.dictionary)
	{
		sdr_free(bpSdr, bundle.dictionary);
	}

	if (bundle.xmitRefs)
	{
		sdr_list_destroy(bpSdr, bundle.xmitRefs, NULL, NULL);
	}

	destroyExtensionBlocks(&bundle);
        destroyCollaborationBlocks(&bundle);
	purgeStationsStack(&bundle);
	if (bundle.stations)
	{
		sdr_list_destroy(bpSdr, bundle.stations, NULL, NULL);
	}

	sdr_free(bpSdr, bundleObj);
	noteBundleRemoved(&bundle);
	return 0;
}

/*	*	*	BP database mgt and access functions	*	*/

int	findBundle(char *sourceEid, BpTimestamp *creationTime,
		unsigned long fragmentOffset, unsigned long fragmentLength,
		Object *bundleAddr, Object *timelineElt)
{
	Sdr	bpSdr = getIonsdr();
	Object	elt;
		OBJ_POINTER(BpEvent, event);
		OBJ_POINTER(Bundle, bundle);
	int	dictionaryBuflen = 0;
	char	*dictionary = NULL;
	char	*eidString;
	int	result;

	CHKERR(ionLocked());
	*timelineElt = 0;
	for (elt = sdr_list_first(bpSdr, (_bpConstants())->timeline); elt;
			elt = sdr_list_next(bpSdr, elt))
	{
		GET_OBJ_POINTER(bpSdr, BpEvent, event,
				sdr_list_data(bpSdr, elt));
		if (event->type != expiredTTL)
		{
			continue;
		}

		/*	Event references a bundle that still exists.	*/

		*bundleAddr = event->ref;
		GET_OBJ_POINTER(bpSdr, Bundle, bundle, *bundleAddr);

		/*	See if bundle's ID matches search arguments.	*/

		if (bundle->id.creationTime.seconds != creationTime->seconds
		|| bundle->id.creationTime.count != creationTime->count)
		{
			continue;	/*	Different time tags.	*/
		}

		if (bundle->bundleProcFlags & BDL_IS_FRAGMENT)
		{
			if (fragmentLength == 0)
			{
				continue;	/*	Fragment n.g.	*/
			}

			if (bundle->id.fragmentOffset != fragmentOffset)
			{
				continue;	/*	Wrong fragment.	*/
			}

			if (bundle->payload.length != fragmentLength)
			{
				continue;	/*	Wrong fragment.	*/
			}
		}
		else	/*	This bundle is not a fragment.		*/
		{
			if (fragmentLength != 0)
			{
				continue;	/*	Need fragment.	*/
			}
		}

		/*	Note: we don't use retrieveDictionary() here
		 *	because it would allocate dictionary buffer
		 *	space every time.  This is faster.		*/

		if (bundle->dictionaryLength == 0)	/*	CBHE.	*/
		{
			dictionary = NULL;
		}
		else
		{
			if (bundle->dictionaryLength > dictionaryBuflen)
			{
				if (dictionary)
				{
					MRELEASE(dictionary);
					dictionary = NULL;
				}

				dictionaryBuflen = bundle->dictionaryLength;
				dictionary = MTAKE(dictionaryBuflen);
				if (dictionary == NULL)
				{
					putErrmsg("Can't retrieve dictionary.",
							itoa(dictionaryBuflen));
					return -1;
				}
			}

			sdr_read(bpSdr, dictionary, bundle->dictionary,
					bundle->dictionaryLength);
		}

		if (printEid(&(bundle->id.source), dictionary, &eidString) < 0)
		{
			putErrmsg("Can't print EID string.", NULL);
			if (dictionary)
			{
				MRELEASE(dictionary);
			}

			return -1;
		}

		result = strcmp(eidString, sourceEid);
		MRELEASE(eidString);
		if (result != 0)	/*	Different sources.	*/
		{
			continue;
		}

		/*	Found the matching bundle.			*/

		*timelineElt = elt;
		break;
	}

	if (dictionary)
	{
		MRELEASE(dictionary);
	}

	return 0;
}

void	findScheme(char *schemeName, VScheme **scheme, PsmAddress *schemeElt)
{
	PsmPartition	bpwm = getIonwm();
	PsmAddress	elt;

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

	if (strlen(schemeName) > MAX_SCHEME_NAME_LEN)
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

	sdr_begin_xn(bpSdr);
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
	if (strcmp(schemeName, _cbheSchemeName()) == 0)
	{
		schemeBuf.cbhe = 1;
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

	sdr_begin_xn(bpSdr);	/*	Just to lock memory.		*/
	if (raiseScheme(schemeElt, _bpvdb(NULL)) < 0)
	{
		sdr_cancel_xn(bpSdr);
		putErrmsg("Can't raise scheme.", NULL);
		return -1;
	}

	sdr_exit_xn(bpSdr);	/*	Unlock memory.			*/

	/*	Note: we don't call raiseScheme here because the
	 *	scheme has no endpoints yet.				*/

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

	sdr_begin_xn(bpSdr);
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

	sdr_begin_xn(bpSdr);		/*	Lock memory.		*/
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
	sdr_begin_xn(bpSdr);
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

	sdr_begin_xn(bpSdr);	/*	Just to lock memory.		*/
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

	sdr_begin_xn(bpSdr);	/*	Just to lock memory.		*/
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
	sdr_begin_xn(bpSdr);	/*	Just to lock memory.		*/
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

	sdr_begin_xn(bpSdr);
	findEndpoint(NULL, metaEid.nss, vscheme, &vpoint, &elt);
	if (elt != 0)	/*	This is a known endpoint.	*/
	{
		sdr_exit_xn(bpSdr);
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

	sdr_begin_xn(bpSdr);	/*	Just to lock memory.		*/
	if (raiseEndpoint(vscheme, endpointElt) < 0)
	{
		sdr_exit_xn(bpSdr);
		putErrmsg("Can't raise endpoint.", NULL);
		return -1;
	}

	sdr_exit_xn(bpSdr);	/*	Unlock memory.			*/
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

	sdr_begin_xn(bpSdr);
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

	sdr_begin_xn(bpSdr);
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
		long nominalRate)
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

	sdr_begin_xn(bpSdr);
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
	sdr_begin_xn(bpSdr);
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

	sdr_begin_xn(bpSdr);	/*	Just to lock memory.		*/
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

	sdr_begin_xn(bpSdr);	/*	Just to lock memory.		*/
	for (elt = sm_list_first(bpwm, bpvdb->inducts); elt;
			elt = sm_list_next(bpwm, elt))
	{
		vinduct = (VInduct *) psp(bpwm, sm_list_data(bpwm, elt));
		if (strcmp(vinduct->protocolName, name) == 0)
		{
			stopInduct(vinduct);
			sdr_exit_xn(bpSdr);
			waitForInduct(vinduct);
			sdr_begin_xn(bpSdr);
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
			sdr_begin_xn(bpSdr);
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

	sdr_begin_xn(bpSdr);
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

	sdr_begin_xn(bpSdr);	/*	Just to lock memory.		*/
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

	sdr_begin_xn(bpSdr);
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

	sdr_begin_xn(bpSdr);	/*	Lock memory.			*/
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
	sdr_begin_xn(bpSdr);
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

	sdr_begin_xn(bpSdr);	/*	Just to lock memory.		*/
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

	sdr_begin_xn(bpSdr);	/*	Just to lock memory.		*/
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
	sdr_begin_xn(bpSdr);	/*	Just to lock memory.		*/
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

int	addOutduct(char *protocolName, char *ductName, char *cloCmd)
{
	Sdr		bpSdr = getIonsdr();
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

	sdr_begin_xn(bpSdr);
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

	ductBuf.bulkQueue = sdr_list_create(bpSdr);
	ductBuf.stdQueue = sdr_list_create(bpSdr);
	ductBuf.urgentQueue = sdr_list_create(bpSdr);
	ductBuf.protocol = (Object) sdr_list_data(bpSdr, clpElt);
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

	sdr_begin_xn(bpSdr);	/*	Just to lock memory.		*/
	if (raiseOutduct(elt, _bpvdb(NULL)) < 0)
	{
		putErrmsg("Can't raise outduct.", NULL);
		sdr_exit_xn(bpSdr);
		return -1;
	}

	sdr_exit_xn(bpSdr);	/*	Unlock memory.			*/
	return 1;
}

int	updateOutduct(char *protocolName, char *ductName, char *cloCmd)
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

	sdr_begin_xn(bpSdr);
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

	sdr_begin_xn(bpSdr);	/*	Lock memory.			*/
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
	sdr_begin_xn(bpSdr);
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

	sdr_begin_xn(bpSdr);	/*	Just to lock memory.		*/
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

	sdr_begin_xn(bpSdr);	/*	Just to lock memory.		*/
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
	sdr_begin_xn(bpSdr);	/*	Just to lock memory.		*/
	resetOutduct(vduct);
	sdr_exit_xn(bpSdr);	/*	Unlock memory.			*/
}

static int	findIncomplete(Bundle *bundle, VEndpoint *vpoint,
			Object *incompleteAddr, Object *incompleteElt)
{
	Sdr		bpSdr = getIonsdr();
	char		*argDictionary;
			OBJ_POINTER(Endpoint, endpoint);
	Object		elt;
			OBJ_POINTER(IncompleteBundle, incomplete);
			OBJ_POINTER(Bundle, fragment);
	char		*fragDictionary;
	int		result;

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
	Sdr	bpSdr = getIonsdr();
	BpDB	*bpConstants = _bpConstants();
	Address	addr;
	Object	elt;
		OBJ_POINTER(BpEvent, event);

	CHKZERO(ionLocked());
	addr = sdr_malloc(bpSdr, sizeof(BpEvent));
	if (addr == 0)
	{
		putErrmsg("No space for timeline event.", NULL);
		return 0;
	}

	sdr_write(bpSdr, addr, (char *) newEvent, sizeof(BpEvent));
	for (elt = sdr_list_last(bpSdr, bpConstants->timeline); elt;
			elt = sdr_list_prev(bpSdr, elt))
	{
		GET_OBJ_POINTER(bpSdr, BpEvent, event,
				sdr_list_data(bpSdr, elt));
		if (event->time <= newEvent->time)
		{
			return sdr_list_insert_after(bpSdr, elt, addr);
		}
	}

	return sdr_list_insert_first(bpSdr, bpConstants->timeline, addr);
}

/*	*	*	Bundle origination functions	*	*	*/

static int	setBundleTTL(Bundle *bundle, Object bundleObj)
{
	BpEvent	event;

	CHKERR(ionLocked());
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

static int	copyBundle(Bundle *newBundle, Object *newBundleObj,
			Bundle *oldBundle)
{
	Sdr	bpSdr = getIonsdr();
	char	*dictionaryBuffer;

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

	if (copyExtensionBlocks(newBundle, oldBundle) < 0)
	{
		putErrmsg("Can't copy extensions.", NULL);
		return -1;
	}

	if (setBundleTTL(newBundle, *newBundleObj) < 0)
	{
		putErrmsg("Can't insert copy of bundle into timeline", NULL);
		return -1;
	}

	newBundle->payload.content = zco_add_reference(bpSdr,
			oldBundle->payload.content);
	if (newBundle->payload.content == 0)
	{
		putErrmsg("Can't add reference to payload content", NULL);
		return -1;
	}

	/*	Note that, since we don't copy the payload but instead
	 *	just add a reference to it, total database occupancy
	 *	increases by only the new bundle's overhead size.	*/

	newBundle->dbTotal = newBundle->dbOverhead;
	noteBundleInserted(newBundle);
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
	Bundle		newBundle;
	Object		newBundleObj;
	Scheme		schemeBuf;

	if (bundle->fwdQueueElt || bundle->xmitsNeeded > 0)
	{
		/*	Bundle is already being forwarded.  An
		 *	additional CT signal indicating custody
		 *	refusal has been received, and we haven't
		 *	yet finished responding to the prior one.
		 *	So nothing to do at this point.			*/

		return 0;
	}

	if (strlen(eid) >= SDRSTRING_BUFSZ)
	{
		/*	EID is too long to insert into the stations
		 *	stack, which is only sdrstrings.  We cannot
		 *	forward this bundle.
		 *
		 *	Must write the bundle to the SDR in order to
		 *	destroy it successfully.  This might already
		 *	have been done by deliverBundle() as invoked
		 *	by dispatchBundle(), in which case our write
		 *	here is redundant but harmless.  However, we
		 *	have no assurance that this happened.		*/

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

	/*	Make copy of bundle as necessary.			*/

	if (bundle->dlvQueueElt || bundle->fragmentElt)
	{
		/*	Bundle is already queued for delivery as well.
		 *	Delivery and forwarding will affect the bundle
		 *	in incompatible ways, so we must create a copy
		 *	of the bundle and forward the copy.		*/

		if (copyBundle(&newBundle, &newBundleObj, bundle) < 0)
		{
			putErrmsg("Can't create copy of bundle.", NULL);
			return -1;
		}

		bundleObj = newBundleObj;
		bundle = &newBundle;
		bundle->fragmentElt = 0;
		bundle->dlvQueueElt = 0;
		bundle->incompleteElt = 0;
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
	bundle->destination.c.nodeNbr = destMetaEid->nodeNbr;
	bundle->destination.c.serviceNbr = destMetaEid->serviceNbr;

	/*	Custodian (none).					*/

	bundle->custodian.cbhe = 1;
	bundle->custodian.c.nodeNbr = 0;
	bundle->custodian.c.serviceNbr = 0;

	/*	Source.							*/

	bundle->id.source.cbhe = 1;
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
			int *stringCount, unsigned long *dictionaryLength,
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
	bundle->custodian.d.schemeNameOffset
		= addStringToDictionary(strings, stringLengths, &stringCount,
			&bundle->dictionaryLength, "dtn", 3);
	bundle->custodian.d.nssOffset
		= addStringToDictionary(strings, stringLengths, &stringCount,
			&bundle->dictionaryLength, "none", 4);

	/*	Destination.						*/

	bundle->destination.cbhe = 0;
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
	int		bundleProcFlags = BDL_DEST_IS_SINGLETON;
			/*	Note: for now, we have no way of
			 *	recognizing non-singleton destination
			 *	endpoints.				*/
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
	CHKERR(destEidString);
	if (*destEidString == 0)
	{
		writeMemo("[?] Zero-length destination EID.");
		return 0;
	}

	if (lifespan <= 0)
	{
		writeMemoNote("[?] Invalid lifespan", itoa(lifespan));
		return 0;
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
			writeMemo("[?] Source can't be anonymous when asking \
for custody transfer and/or status reports.");
			return 0;
		}

		if (adminRecordType != 0)
		{
			writeMemo("[?] Can't ask for custody transfer and/or \
status reports for admin records.");
			return 0;
		}
	}

	if (sourceMetaEid == NULL)
	{
		bundleProcFlags |= BDL_DOES_NOT_FRAGMENT;
	}

	if (ackRequested)
	{
		bundleProcFlags |= BDL_APP_ACK_REQUEST;
	}

	if (custodySwitch != NoCustodyRequested)
	{
		bundleProcFlags |= BDL_IS_CUSTODIAL;
	}

	/*	Figure out how to express all endpoint IDs.		*/

	if (parseEidString(destEidString, &destMetaEid, &vscheme, &vschemeElt)
			== 0)
	{
		restoreEidString(&destMetaEid);
		writeMemoNote("[?] Destination EID malformed", destEidString);
		return 0;
	}

	if (destMetaEid.nullEndpoint)	/*	Do not send bundle.	*/
	{
		zco_destroy_reference(bpSdr, adu);
		return 1;
	}

	if (!destMetaEid.cbhe
	|| strcmp(destMetaEid.schemeName, _cbheSchemeName()) != 0)
	{
		nonCbheEidCount++;
	}

	if (adminRecordType != 0)
	{
		bundleProcFlags |= BDL_IS_ADMIN;

		/*	Administrative bundles must not be anonymous.
		 *	Recipient needs to know source of the status
		 *	report or custody signal; use own custodian
		 *	EID for the scheme cited by the destination
		 *	EID.						*/

		if (sourceMetaEid == NULL)
		{
			istrcpy(sourceEidString, vscheme->custodianEidString,
					sizeof sourceEidString);
			if (parseEidString(sourceEidString, &tempMetaEid,
					&vscheme2, &vschemeElt2) == 0)
			{
				restoreEidString(&tempMetaEid);
				writeMemoNote("[?] Custodian EID malformed",
						sourceEidString);
				return 0;
			}

			sourceMetaEid = &tempMetaEid;
		}
	}

	if (sourceMetaEid != NULL)	/*	Not "dtn:none".		*/
	{
		/*	Submitted by application on open endpoint.	*/

		if (!sourceMetaEid->cbhe
		|| strcmp(sourceMetaEid->schemeName,
				destMetaEid.schemeName) != 0)
		{
			nonCbheEidCount++;
		}
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
			if (!reportToMetaEid->cbhe
			|| strcmp(reportToMetaEid->schemeName,
					destMetaEid.schemeName) != 0)
			{
				nonCbheEidCount++;
			}
		}
	}

	/*	Create the outbound bundle.				*/

	memset((char *) &bundle, 0, sizeof(Bundle));
	bundle.dbOverhead = BASE_BUNDLE_OVERHEAD;

	/*	The bundle protocol specification authorizes the
	 *	implementation to fragment an ADU to satisfy, for
	 *	example, MIB-specified requirements for routing
	 *	granularity.  This would result in multiple ZCOs,
	 *	multiple bundles, multiple calls to bpDequeue in
	 *	the CLO, multiple bundle catenations.  It would
	 *	degrade performance, as there would have to be a
	 *	lot of zco_copy activity up and down the stack;
	 *	there would be more processing.  So in this BP
	 *	implementation we choose instead to leave ADU
	 *	fragment granularity decisions to the application.	*/

	aduLength = zco_length(bpSdr, adu);
	if (aduLength < 0)
	{
		restoreEidString(&destMetaEid);
		restoreEidString(reportToMetaEid);
		putErrmsg("Can't get length of ADU.", NULL);
		return -1;
	}

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
			restoreEidString(&destMetaEid);
			restoreEidString(reportToMetaEid);
			putErrmsg("Can't load dictionary.", NULL);
			return -1;
		}
	}

	restoreEidString(&destMetaEid);
	restoreEidString(reportToMetaEid);
	bundle.id.fragmentOffset = 0;
	bundle.custodyTaken = 0;
	bundle.catenated = 0;
	bundle.bundleProcFlags = bundleProcFlags;
	bundle.bundleProcFlags += (classOfService << 7);
	bundle.bundleProcFlags += (srrFlags << 14);

	/*	Note: bundle is not a fragment when initially created,
	 *	so totalAduLength is left at zero.			*/

	bundle.payloadBlockProcFlags = BLK_MUST_BE_COPIED;
	bundle.payload.length = aduLength;
	bundle.payload.content = adu;
	bundle.xmitsNeeded = 0;

	/*	Bundle is almost fully constructed at this point.	*/

	sdr_begin_xn(bpSdr);
	getCurrentDtnTime(&currentDtnTime);
	if (currentDtnTime.seconds != bpvdb->creationTimeSec)
	{
		bpvdb->creationTimeSec = currentDtnTime.seconds;
		bpvdb->bundleCounter = 0;
	}

	bundle.id.creationTime.seconds = bpvdb->creationTimeSec;
	bundle.id.creationTime.count = ++(bpvdb->bundleCounter);
	bundle.expirationTime = bundle.id.creationTime.seconds + lifespan;
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
	bundle.xmitRefs = sdr_list_create(bpSdr);
	*bundleObj = sdr_malloc(bpSdr, sizeof(Bundle));
	if (*bundleObj == 0
	|| bundle.xmitRefs == 0
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

	bundle.dbTotal = bundle.dbOverhead;
	if (bundle.payload.content)
	{
		bundle.dbTotal += zco_occupancy(bpSdr, bundle.payload.content);
	}

	noteBundleInserted(&bundle);
	sdr_write(bpSdr, *bundleObj, (char *) &bundle, sizeof(Bundle));

	/*	Note: custodial reporting, as requested, is perfomed
	 *	by the destination scheme's forwarder.			*/

	if (forwardBundle(*bundleObj, &bundle, destEidString) < 0)
	{
		putErrmsg("Can't queue bundle for forwarding.", NULL);
		sdr_cancel_xn(bpSdr);
		return -1;
	}

	if (sdr_end_xn(bpSdr) < 0)
	{
		putErrmsg("Can't send bundle.", NULL);
		return -1;
	}

	noteStateStats(BPSTATS_SOURCE, &bundle);
	if (bpvdb->watching & WATCH_a)
	{
		putchar('a');
		fflush(stdout);
	}

	return 1;
}

/*	*	*	Bundle reception functions	*	*	*/

int	sendCtSignal(Bundle *bundle, char *dictionary, int succeeded,
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
		putErrmsg("Can't print CBHE custodian EID.", NULL);
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
	else
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

	ttl = bundle->expirationTime - bundle->id.creationTime.seconds;
	if (ttl < 1)
	{
		ttl = 1;
	}

	if (printEid(&bundle->id.source, dictionary,
			&bundle->ctSignal.sourceEid) < 0)
	{
		putErrmsg("Can't print source EID.", NULL);
		MRELEASE(custodianEid);
		return -1;
	}

	result = bpConstructCtSignal(&(bundle->ctSignal), &payloadZco);
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

	ttl = bundle->expirationTime - bundle->id.creationTime.seconds;
	if (ttl < 1) ttl = 1;
	if (printEid(&bundle->id.source, dictionary,
			&bundle->statusRpt.sourceEid) < 0)
	{
		putErrmsg("Can't print source EID.", NULL);
		return -1;
	}

	result = bpConstructStatusRpt(&(bundle->statusRpt), &payloadZco);
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

static void	lookUpDestEndpoint(Bundle *bundle, char *dictionary,
			VScheme *vscheme, VEndpoint **vpoint)
{
	PsmPartition	bpwm = getIonwm();
	char		nssBuf[42];
	char		*nss;
	PsmAddress	elt;

	if (dictionary == NULL)
	{
		isprintf(nssBuf, sizeof nssBuf, "%lu.%lu",
				bundle->destination.c.nodeNbr,
				bundle->destination.c.serviceNbr);
		nss = nssBuf;
	}
	else
	{
		nss = dictionary + bundle->destination.d.nssOffset;
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

	sdr_write(bpSdr, bundleObj, (char *) bundle, sizeof(Bundle));
	if (vpoint->semaphore != SM_SEM_NONE)
	{
		sm_SemGive(vpoint->semaphore);
	}

	return 0;
}

static int	extendIncomplete(IncompleteBundle *incomplete, Object incElt,
			Object bundleObj, Bundle *bundle, VEndpoint *vpoint)
{
	Sdr		bpSdr = getIonsdr();
	Object		elt;
			OBJ_POINTER(Bundle, fragment);
	unsigned int	endOfFurthestFragment;
	unsigned int	endOfFragment;
	Bundle		aggregateBundle;
	Object		aggregateBundleObj;
	int		buflen = 65536;
	char		*buffer;
	unsigned int	aggregateAduLength;
	Object		fragmentObj;
	Bundle		fragBuf;
	ZcoReader	reader;
	unsigned int	bytesToSkip;
	unsigned int	bytesToExtract;
	int		bytesExtracted;
	unsigned int	bytesToCopy;
	Object		extent;

	/*	First look for fragment insertion point and insert
	 *	the new bundle at this point.				*/

	for (elt = sdr_list_first(bpSdr, incomplete->fragments); elt;
			elt = sdr_list_next(bpSdr, elt))
	{
		GET_OBJ_POINTER(bpSdr, Bundle, fragment,
				sdr_list_data(bpSdr, elt));
		if (fragment->id.fragmentOffset > bundle->id.fragmentOffset)
		{
			break;	/*	Insert before this one.		*/
		}
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

	sdr_write(bpSdr, bundleObj, (char *) bundle, sizeof(Bundle));

	/*	Now see if entire ADU has been received.		*/

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

	buffer = MTAKE(buflen);
	if (buffer == NULL)
	{
		putErrmsg("Can't create buffer for ADU reassembly.", NULL);
		return -1;
	}

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

	for (elt = sdr_list_next(bpSdr, elt); elt;
			elt = sdr_list_next(bpSdr, elt))
	{
		fragmentObj = sdr_list_data(bpSdr, elt);
		sdr_stage(bpSdr, (char *) &fragBuf, fragmentObj,
				sizeof(Bundle));
		bytesToSkip = aggregateAduLength - fragBuf.id.fragmentOffset;
		if (bytesToSkip > fragBuf.payload.length)
		{
			bytesToSkip = fragBuf.payload.length;
		}

		bytesToCopy = fragBuf.payload.length - bytesToSkip;
		aggregateAduLength += bytesToCopy;
		zco_start_receiving(bpSdr, fragBuf.payload.content, &reader);
		while (bytesToSkip > 0)
		{
			bytesToExtract = buflen;
			if (bytesToExtract > bytesToSkip)
			{
				bytesToExtract = bytesToSkip;
			}

			bytesExtracted = zco_receive_source(bpSdr, &reader,
					bytesToExtract, buffer);
			if (bytesExtracted < 0)
			{
				zco_stop_receiving(bpSdr, &reader);
				MRELEASE(buffer);
				putErrmsg("Can't extract from ADU fragment.",
						NULL);
				return -1;
			}

			bytesToSkip -= bytesExtracted;
		}

		while (bytesToCopy > 0)
		{
			bytesToExtract = buflen;
			if (bytesToExtract > bytesToCopy)
			{
				bytesToExtract = bytesToCopy;
			}

			bytesExtracted = zco_receive_source(bpSdr, &reader,
					bytesToExtract, buffer);
			if (bytesExtracted < 0)
			{
				zco_stop_receiving(bpSdr, &reader);
				MRELEASE(buffer);
				putErrmsg("Can't extract from ADU fragment.",
						NULL);
				return -1;
			}

			extent = sdr_insert(bpSdr, buffer, bytesExtracted);
			if (extent == 0)
			{
				zco_stop_receiving(bpSdr, &reader);
				MRELEASE(buffer);
				putErrmsg("Can't copy fragment content.",
						NULL);
				return -1;
			}

			if (zco_append_extent(bpSdr,
				aggregateBundle.payload.content,
				ZcoSdrSource, extent, 0, bytesExtracted) < 0)
			{
				zco_stop_receiving(bpSdr, &reader);
				MRELEASE(buffer);
				putErrmsg("Can't append extent.", NULL);
				return -1;
			}

			bytesToCopy -= bytesExtracted;
		}

		zco_stop_receiving(bpSdr, &reader);
		sdr_list_delete(bpSdr, fragBuf.fragmentElt, NULL, NULL);
		fragBuf.fragmentElt = 0;
		sdr_write(bpSdr, fragmentObj, (char *) &fragBuf,
				sizeof(Bundle));
		if (bpDestroyBundle(fragmentObj, 0) < 0)
		{
			MRELEASE(buffer);
			putErrmsg("Can't destroy fragment.", NULL);
			return -1;
		}
	}

	MRELEASE(buffer);

	/*	Note effect on database occupancy of bundle's
	 *	revised size.						*/

	aggregateBundle.dbTotal = aggregateBundle.dbOverhead;
	if (aggregateBundle.payload.content)
	{
		aggregateBundle.dbTotal += zco_occupancy(bpSdr,
				aggregateBundle.payload.content);
	}

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
		putErrmsg("No space for incomplete object.", NULL);
		return -1;
	}

	sdr_write(bpSdr, incObj, (char *) &incomplete,
			sizeof(IncompleteBundle));
	GET_OBJ_POINTER(bpSdr, Endpoint, endpoint, sdr_list_data(bpSdr,
			vpoint->endpointElt));
	bundle->incompleteElt =
		sdr_list_insert_last(bpSdr, endpoint->incompletes, incObj);
	if (bundle->incompleteElt == 0)
	{
		putErrmsg("No space for list element for incomplete.", NULL);
		return -1;
	}

	/*	Bundle becomes first element in the fragments list.	*/

	bundle->fragmentElt = sdr_list_insert_last(bpSdr, incomplete.fragments,
			bundleObj);
	if (bundle->fragmentElt == 0)
	{
		putErrmsg("No space for fragment list elt.", NULL);
		return -1;
	}

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
	 *	(if any) may now release custody.  NOTE: the local
	 *	node has NOT taken custody of this bundle.		*/

	if (bundle->bundleProcFlags & BDL_IS_CUSTODIAL)
	{
		if ((dictionary = retrieveDictionary(bundle))
				== (char *) bundle)
		{
			putErrmsg("Can't retrieve dictionary.", NULL);
			return -1;
		}

		result = sendCtSignal(bundle, dictionary, 1, 0);
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
	 *	reconstructed original bundle to the application)
	 *	and return for possible forwarding of this fragment.	*/

	if (findIncomplete(bundle, vpoint, &incompleteAddr, &elt) < 0)
	{
		putErrmsg("Failed seeking incomplete bundle.", NULL);
		return -1;
	}

	if (elt)	/*	Matching IncompleteBundle found.	*/
	{
		GET_OBJ_POINTER(getIonsdr(), IncompleteBundle, incomplete,
				incompleteAddr);
		return extendIncomplete(incomplete, elt, bundleObj, bundle,
				vpoint);
	}

	/*	No existing incomplete bundle to extend.  But if the
	 *	current bundle is a fragment we still can't do anything
	 *	with it; start a new IncompleteBundle and return for
	 *	possible forwarding of this fragment.			*/

	if (bundle->bundleProcFlags & BDL_IS_FRAGMENT)
	{
		return createIncompleteBundle(bundleObj, bundle, vpoint);
	}

	/*	Bundle is not a fragment, so we can deliver it right
	 *	away if that's the current policy for this endpoint.	*/

	return enqueueForDelivery(bundleObj, bundle, vpoint);
}

static int	dispatchBundle(Object bundleObj, Bundle *bundle)
{
	Sdr		bpSdr = getIonsdr();
	char		*dictionary;
	VScheme		*vscheme;
	VEndpoint	*vpoint;
	char		*eidString;
	int		result;

	CHKERR(ionLocked());
	if ((dictionary = retrieveDictionary(bundle)) == (char *) bundle)
	{
		putErrmsg("Can't retrieve dictionary.", NULL);
		return -1;
	}

	lookUpDestScheme(bundle, dictionary, &vscheme);
	if (vscheme != NULL)	/*	Destination might be local.	*/
	{
		lookUpDestEndpoint(bundle, dictionary, vscheme, &vpoint);
		if (vpoint != NULL)	/*	Destination is here.	*/
		{
			if (deliverBundle(bundleObj, bundle, vpoint) < 0)
			{
				releaseDictionary(dictionary);
				putErrmsg("Bundle delivery failed.", NULL);
				return -1;
			}

			noteStateStats(BPSTATS_DELIVER, bundle);
			if ((_bpvdb(NULL))->watching & WATCH_z)
			{
				putchar('z');
				fflush(stdout);
			}

			if (bundle->bundleProcFlags & BDL_DEST_IS_SINGLETON)
			{
				/*	Can't be forwarded to any other
				 *	node.  Destroy the bundle as
				 *	necessary.			*/

				releaseDictionary(dictionary);

				/*	Note that we have to write the
				 *	bundle to the SDR in order to
				 *	destroy it successfully: the
				 *	deliverBundle() function will
				 *	normally do this (in which case
				 *	our write here is redundant but
				 *	harmless) but might not have
				 *	done so in the event that the
				 *	endpoint is not currently active
				 *	(i.e., is not currently opened
				 *	by any application) and the
				 *	delivery failure action for this
				 *	endpoint is DiscardBundle.	*/

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
			&& bundle->destination.c.nodeNbr == getOwnNodeNbr())
			{
				/*	Destination is known to be the
				 *	local bundle agent.  Since the
				 *	bundle can't be delivered, it
				 *	must be abandoned.  It can't
				 *	be forwarded, as this would be
				 *	an infinite forwarding loop.
				 *	But must first accept it, to
				 *	prevent current custodian from
				 *	re-forwarding it endlessly back
				 *	to the local bundle agent.	*/

				releaseDictionary(dictionary);
				if (bpAccept(bundle) < 0)
				{
					putErrmsg("Failed dispatching bundle.",
							NULL);
					return -1;
				}

				/*	As above, must write the bundle
				 *	to the SDR in order to destroy
				 *	it successfully.		*/

				sdr_write(bpSdr, bundleObj, (char *) bundle,
						sizeof(Bundle));
				return bpAbandon(bundleObj, bundle);
			}
		}
	}

	/*	There may be a non-local destination.			*/

	if (printEid(&bundle->destination, dictionary, &eidString) < 0)
	{
		putErrmsg("Can't print destination EID.", NULL);
		return -1;
	}

	if (patchExtensionBlocks(bundle) < 0)
	{
		putErrmsg("Can't insert missing extensions.", NULL);
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
		sdr_begin_xn(bpSdr);
		sdr_list_delete(bpSdr, work->zcoElt, NULL, NULL);

		/*	Destroying the last reference to the ZCO will
		 *	destroy the ZCO, which will in turn cause the
		 *	acquisition FileRef to be deleted, which will
		 *	unlink the acquisition file when the FileRef's
		 *	cleanup script is executed.			*/

		zco_destroy_reference(bpSdr, work->zco);
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

	sdr_begin_xn(bpSdr);
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
	static unsigned long	acqCount = 0;
	static int		maxAcqInHeap = 0;
	Sdr			sdr = getIonsdr();
	BpDB			*bpConstants = _bpConstants();
	BpDB			bpdb;
	char			cwd[200];
	char			fileName[SDRSTRING_BUFSZ];
	int			fd;
	long			fileLength;

	CHKERR(work);
	CHKERR(bytes);
	CHKERR(length >= 0);
	sdr_begin_xn(sdr);
	if (maxAcqInHeap == 0)
	{
		/*	Initialize threshold for acquiring bundle
		 *	into a file rather than directly into the
		 *	heap.  Minimum threshold is the amount of
		 *	heap space that would be occupied by a ZCO
		 *	file reference object anyway, even if the
		 *	bundle were entirely acquired into a file.	*/

		maxAcqInHeap = zco_file_ref_occupancy(sdr, 0);
		sdr_read(sdr, (char *) &bpdb, getBpDbObject(), sizeof(BpDB));
		if (bpdb.maxAcqInHeap > maxAcqInHeap)
		{
			maxAcqInHeap = bpdb.maxAcqInHeap;
		}
	}

	if (work->zco == 0)	/*	First extent of acquisition.	*/
	{
		work->zco = zco_create(sdr, ZcoSdrSource, 0, 0, 0);
		work->zcoElt = sdr_list_insert_last(sdr,
				bpConstants->inboundBundles, work->zco);
		if (work->zco == 0 || work->zcoElt == 0)
		{
			putErrmsg("Can't start inbound bundle ZCO.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}
	}

	/*	Now add extent.  Acquire extents of bundle into
	 *	database heap up to the stated limit; after that,
	 *	acquire all remaining extents into a file.		*/

	if ((length + zco_length(sdr, work->zco)) <= maxAcqInHeap)
	{
		oK(zco_append_extent(sdr, work->zco, ZcoSdrSource,
				sdr_insert(sdr, bytes, length), 0, length));
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
		isprintf(fileName, sizeof fileName, "%s%cbpacq.%lu", cwd,
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
	oK(zco_append_extent(sdr, work->zco, ZcoFileSource, work->acqFileRef,
			fileLength, length));

	/*	Flag file reference for deletion as soon as the last
	 *	ZCO extent that references it is deleted.		*/

	zco_destroy_file_ref(sdr, work->acqFileRef);
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

	sdr_begin_xn(bpSdr);		/*	Just to lock memory.	*/
	GET_OBJ_POINTER(bpSdr, Induct, induct, sdr_list_data(bpSdr,
			work->vduct->inductElt));
	GET_OBJ_POINTER(bpSdr, ClProtocol, protocol, induct->protocol);
	recvLength = computeECCC(bundle->payload.length
			+ NOMINAL_PRIMARY_BLKSIZE, protocol);
	throttle = &(work->vduct->acqThrottle);
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

		sdr_begin_xn(bpSdr);
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
	memcpy(work->buffer, work->buffer + bytesParsed, work->bytesBuffered);

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
	unsigned long	residualBlockLength;
	int		i;
	unsigned long	eidSdnvValues[8];
	unsigned long	lifetime;
	int		bytesParsed;

	/*	Create the inbound bundle.				*/

	bundle = &(work->bundle);
	bundle->dbOverhead = BASE_BUNDLE_OVERHEAD;
	bundle->custodyTaken = 0;
	bundle->catenated = 0;
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

	extractSdnv(&(bundle->bundleProcFlags), &cursor, &unparsedBytes);

	/*	Note status report information as necessary.		*/

	if (SRR_FLAGS(bundle->bundleProcFlags) & BP_RECEIVED_RPT)
	{
		bundle->statusRpt.flags |= BP_RECEIVED_RPT;
		getCurrentDtnTime(&(bundle->statusRpt.receiptTime));
	}

	/*	Get length of rest of primary block.			*/

	extractSdnv(&residualBlockLength, &cursor, &unparsedBytes);

	/*	Get all EID SDNV values.				*/

	for (i = 0; i < 8; i++)
	{
		extractSdnv(&(eidSdnvValues[i]), &cursor, &unparsedBytes);
	}

	/*	Get creation timestamp, lifetime, dictionary length.
	 *	Expiration time is creation time plus interval.		*/

	extractSdnv(&(bundle->id.creationTime.seconds), &cursor,
			&unparsedBytes);
	bundle->expirationTime = bundle->id.creationTime.seconds;

	extractSdnv(&(bundle->id.creationTime.count), &cursor,
			&unparsedBytes);

	extractSdnv(&(lifetime), &cursor, &unparsedBytes);
	bundle->expirationTime += lifetime;

	extractSdnv(&(bundle->dictionaryLength), &cursor, &unparsedBytes);
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
		bundle->destination.c.nodeNbr = eidSdnvValues[0];
		bundle->destination.c.serviceNbr = eidSdnvValues[1];
		bundle->id.source.cbhe = 1;
		bundle->id.source.c.nodeNbr = eidSdnvValues[2];
		bundle->id.source.c.serviceNbr = eidSdnvValues[3];
		bundle->reportTo.cbhe = 1;
		bundle->reportTo.c.nodeNbr = eidSdnvValues[4];
		bundle->reportTo.c.serviceNbr = eidSdnvValues[5];
		bundle->custodian.cbhe = 1;
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
		bundle->destination.d.schemeNameOffset = eidSdnvValues[0];
		bundle->destination.d.nssOffset = eidSdnvValues[1];
		bundle->id.source.cbhe = 0;
		bundle->id.source.d.schemeNameOffset = eidSdnvValues[2];
		bundle->id.source.d.nssOffset = eidSdnvValues[3];
		bundle->reportTo.cbhe = 0;
		bundle->reportTo.d.schemeNameOffset = eidSdnvValues[4];
		bundle->reportTo.d.nssOffset = eidSdnvValues[5];
		bundle->custodian.cbhe = 0;
		bundle->custodian.d.schemeNameOffset = eidSdnvValues[6];
		bundle->custodian.d.nssOffset = eidSdnvValues[7];
	}

	if (bundle->bundleProcFlags & BDL_IS_FRAGMENT)
	{
		extractSdnv(&(bundle->id.fragmentOffset), &cursor,
				&unparsedBytes);
		extractSdnv(&(bundle->totalAduLength), &cursor,
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
	unsigned long	blkProcFlags;
	Lyst		eidReferences = NULL;
	unsigned long	eidReferencesCount;
	unsigned long	schemeOffset;
	unsigned long	nssOffset;
	unsigned long	dataLength;
	unsigned long	lengthOfBlock;
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

	extractSdnv(&blkProcFlags, &cursor, &unparsedBytes);
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

		extractSdnv(&eidReferencesCount, &cursor, &unparsedBytes);
		while (eidReferencesCount > 0)
		{
			extractSdnv(&schemeOffset, &cursor, &unparsedBytes);
			if (lyst_insert_last(eidReferences,
					(void *) schemeOffset) == NULL)
			{
				return -1;
			}

			extractSdnv(&nssOffset, &cursor, &unparsedBytes);
			if (lyst_insert_last(eidReferences,
					(void *) nssOffset) == NULL)
			{
				return -1;
			}

			eidReferencesCount--;
		}
	}

	extractSdnv(&dataLength, &cursor, &unparsedBytes);

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
		sdr_begin_xn(bpSdr);
		zco_destroy_reference(bpSdr, work->bundle.payload.content);
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

	if (bundle->bundleProcFlags & BDL_IS_CUSTODIAL)
	{
		if (sendCtSignal(bundle, dictionary, 0, ctReason) < 0)
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
	 *	supporting schemes other than "ipn" and "dtn", but
	 *	for now it's the simplest and most efficient approach.	*/

	if (bundle != NULL && bundle->destination.cbhe)
	{
		return _cbheSchemeName();
	}

	return _dtn2SchemeName();
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
	if (vschemeElt == 0
	|| strcmp(vscheme->custodianEidString, _nullEid()) == 0)
	{
		/*	Can't look for BAB rule, so can't authenticate.	*/

		return;			/*	So can't be authentic.	*/
	}

	/*	Check the applicable BAB rule, if any.			*/

	sec_findBspBabRule(work->senderEid, vscheme->custodianEidString,
			&ruleAddr, &elt);
	if (elt)
	{
		GET_OBJ_POINTER(bpSdr, BspBabRule, rule, ruleAddr);
		if (rule->ciphersuiteName[0] == '\0')
		{
			work->authentic = 1;	/*	Trusted node.	*/
		}
	}
}

static int	acquireBundle(Sdr bpSdr, AcqWorkArea *work)
{
	Bundle		*bundle = &(work->bundle);
	unsigned int	bundleOccupancy;
			OBJ_POINTER(IonDB, iondb);
	char		*custodialSchemeName;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	int		result;
	char		*eidString;
	Object		bundleZco;
	ZcoReader	reader;
	Object		bundleAddr;
	Object		timelineElt;
	MetaEid		senderMetaEid;
	Object		bundleObj;

	sdr_begin_xn(bpSdr);
	if (acqFromWork(work) < 0)
	{
		putErrmsg("Acquisition from work area failed.", NULL);
		sdr_cancel_xn(bpSdr);
		return -1;
	}

	if (work->bundleLength > 0)
	{
		/*	Bundle has been parsed out of the work area's
		 *	ZCO.  Split it off into a separate ZCO.		*/

		bundle->payload.content = zco_clone(bpSdr, work->zco,
				work->zcoBytesConsumed, work->bundleLength);
		bundleOccupancy = zco_occupancy(bpSdr, bundle->payload.content);
		work->zcoBytesConsumed += work->bundleLength;
	}
	else
	{
		bundle->payload.content = 0;
		bundleOccupancy = 0;
	}

	if (sdr_end_xn(bpSdr) < 0)
	{
		putErrmsg("Can't add reference work ZCO.", NULL);
		return -1;
	}

	if (bundle->payload.content == 0)
	{
		return 0;	/*	No bundle at front of work ZCO.	*/
	}

	/*	Check bundle for problems.				*/

	if (work->malformed || work->lastBlockParsed == 0)
	{
		writeMemo("[?] Malformed bundle.");
		return abortBundleAcq(work);
	}

	initAuthenticity(work);	/*	Set default.			*/
	if (checkExtensionBlocks(work) < 0)
	{
		putErrmsg("Can't check bundle authenticity.", NULL);
		return -1;
	}

	if (bundle->clDossier.authentic == 0)
	{
		writeMemo("[?] Bundle judged inauthentic.");
		return abortBundleAcq(work);
	}

	if (work->decision == AcqNG)
	{
		writeMemo("[?] Extension-related problem found in bundle.");
		return abortBundleAcq(work);
	}

	/*	Unintelligible extension headers don't make a bundle
	 *	malformed, but they may make it necessary to discard
	 *	the bundle.						*/

	if (work->mustAbort)
	{
		return discardReceivedBundle(work, CtBlockUnintelligible,
				SrBlockUnintelligible, work->dictionary);
	}

	/*	Bundle acquisition was uneventful but bundle may have
	 *	to be refused due to insufficient resources.  Must
	 *	guard against congestion collapse; use bundle's
	 *	dbOverhead plus SDR occupancy of the bundle's payload
	 *	ZCO as upper limit on the total SDR occupancy increment
	 *	that would result from accepting this bundle.		*/

	GET_OBJ_POINTER(bpSdr, IonDB, iondb, getIonDbObject());
	if (iondb->currentOccupancy + bundle->dbOverhead + bundleOccupancy
			> iondb->occupancyCeiling)
	{
		/*	Not enough heap space for bundle.		*/

		return discardReceivedBundle(work, CtDepletedStorage,
				SrDepletedStorage, work->dictionary);
	}

	/*	Redundant reception of a custodial bundle after
	 *	we have already taken custody forces us to discard
	 *	this bundle and refuse custody, but we don't report
	 *	deletion of the bundle because it's not really deleted
	 *	-- we still have a local copy for which we have
	 *	accepted custody.					*/

	if (bundle->bundleProcFlags & BDL_IS_CUSTODIAL)
	{
		if (printEid(&(bundle->custodian), work->dictionary,
				&eidString) < 0)
		{
			putErrmsg("Can't print custodian EID string.", NULL);
			return -1;
		}

		custodialSchemeName = getCustodialSchemeName(bundle);
		findScheme(custodialSchemeName, &vscheme, &vschemeElt);
		if (vschemeElt != 0
		&& strcmp(eidString, vscheme->custodianEidString) == 0)
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
				return -1;
			}

			sdr_begin_xn(bpSdr);
			result = findBundle(eidString,
				&(bundle->id.creationTime),
				bundle->id.fragmentOffset,
				(bundle->bundleProcFlags & BDL_IS_FRAGMENT ?
					bundle->payload.length : 0),
				&bundleAddr, &timelineElt);
			MRELEASE(eidString);
			sdr_exit_xn(bpSdr);
			if (result < 0)
			{
				putErrmsg("Can't fetch bundle.", NULL);
				return -1;
			}

			if (timelineElt)	/*	Redundant.	*/
			{
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
	sdr_begin_xn(bpSdr);

	/*	Reduce payload ZCO to just its source data, discarding
	 *	BP header and trailer.					*/

	bundleZco = zco_add_reference(bpSdr, bundle->payload.content);
	zco_start_receiving(bpSdr, bundleZco, &reader);
	zco_receive_headers(bpSdr, &reader, work->headerLength, NULL);
	zco_delimit_source(bpSdr, &reader, work->bundle.payload.length);
	zco_strip(bpSdr, bundleZco);
	zco_stop_receiving(bpSdr, &reader);
	zco_destroy_reference(bpSdr, bundleZco);

	/*	Record bundle's sender EID, if known.			*/

	if (work->senderEid)
	{
		if (parseEidString(work->senderEid, &senderMetaEid, &vscheme,
				&vschemeElt) == 0)
		{
			sdr_exit_xn(bpSdr);
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
	bundle->xmitRefs = sdr_list_create(bpSdr);
	bundleObj = sdr_malloc(bpSdr, sizeof(Bundle));
	if (bundleObj == 0)
	{
		putErrmsg("No space for bundle object.", NULL);
		sdr_cancel_xn(bpSdr);
		return -1;
	}

	if (setBundleTTL(bundle, bundleObj) < 0)
	{
		putErrmsg("Can't insert new bundle into timeline.", NULL);
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

	if (recordExtensionBlocks(work) < 0)
	{
		putErrmsg("Can't record extensions.", NULL);
		sdr_cancel_xn(bpSdr);
		return -1;
	}

	bundle->dbTotal = bundle->dbOverhead;
	bundle->dbTotal += zco_occupancy(bpSdr, bundle->payload.content);
	noteBundleInserted(bundle);
	noteStateStats(BPSTATS_RECEIVE, bundle);
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

	if (dispatchBundle(bundleObj, bundle) < 0)
	{
		putErrmsg("Can't dispatch bundle.", NULL);
		sdr_cancel_xn(bpSdr);
		return -1;
	}

	if (sdr_end_xn(bpSdr) < 0)
	{
		putErrmsg("Can't acquire bundle.", NULL);
		return -1;
	}

	/*	Finally, apply reception rate control: delay
	 *	acquisition of the next bundle until we have
	 *	consumed as much time in receiving and acquiring
	 *	this one as we had said we would.			*/

	if (applyRecvRateControl(work) < 0)
	{
		putErrmsg("Can't apply reception rate control.", NULL);
		return -1;
	}

	return 0;
}

int	bpEndAcq(AcqWorkArea *work)
{
	Sdr		bpSdr = getIonsdr();
	int		acqLength;

	CHKERR(work);
	CHKERR(work->zco);
	work->zcoLength = zco_length(bpSdr, work->zco);
	sdr_begin_xn(bpSdr);
	zco_start_receiving(bpSdr, work->zco, &(work->reader));
	CHKERR(advanceWorkBuffer(work, 0) == 0);
	if (sdr_end_xn(bpSdr) < 0)
	{
		putErrmsg("Acq buffer initialization failed.", NULL);
		return -1;
	}

	/*	Acquire bundles from acquisition ZCO.			*/

	acqLength = work->zcoLength;
	while (acqLength > 0)
	{
		if (acquireBundle(bpSdr, work) < 0)
		{
			putErrmsg("Bundle acquisition failed.", NULL);
			return -1;
		}

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

	zco_stop_receiving(bpSdr, &(work->reader));
	return eraseWorkZco(work);
}

/*	*	*	Administrative payload functions	*	*/

int	bpConstructCtSignal(BpCtSignal *csig, Object *zcoRef)
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
	CHKERR(zcoRef);
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

	sdr_begin_xn(bpSdr);
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
	*zcoRef = zco_create(bpSdr, ZcoSdrSource, sourceData, 0,
			ctSignalLength);
	if (sdr_end_xn(bpSdr) < 0 || *zcoRef == 0)
	{
		putErrmsg("Can't create CT signal.", NULL);
		return -1;
	}

	return 0;
}

static int	bpParseCtSignal(BpCtSignal *csig, unsigned char *cursor,
			int unparsedBytes, int isFragment)
{
	unsigned char	head1;
	unsigned long	eidLength;

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
		extractSdnv(&(csig->fragmentOffset), &cursor, &unparsedBytes);
		extractSdnv(&(csig->fragmentLength), &cursor, &unparsedBytes);
	}

	extractSdnv(&(csig->signalTime.seconds), &cursor, &unparsedBytes);
	extractSdnv(&(csig->signalTime.nanosec), &cursor, &unparsedBytes);
	extractSdnv(&(csig->creationTime.seconds), &cursor, &unparsedBytes);
	extractSdnv(&(csig->creationTime.count), &cursor, &unparsedBytes);
	extractSdnv(&eidLength, &cursor, &unparsedBytes);
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

void	bpEraseCtSignal(BpCtSignal *csig)
{
	MRELEASE(csig->sourceEid);
}

int	bpConstructStatusRpt(BpStatusRpt *rpt, Object *zcoRef)
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
	CHKERR(zcoRef);
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

	sdr_begin_xn(bpSdr);
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
	*zcoRef = zco_create(bpSdr, ZcoSdrSource, sourceData, 0, rptLength);
	if (sdr_end_xn(bpSdr) < 0 || *zcoRef == 0)
	{
		putErrmsg("Can't create status report.", NULL);
		return -1;
	}

	return 0;
}

static int	bpParseStatusRpt(BpStatusRpt *rpt, unsigned char *cursor,
	       		int unparsedBytes, int isFragment)
{
	unsigned long	eidLength;

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
		extractSdnv(&(rpt->fragmentOffset), &cursor, &unparsedBytes);
		extractSdnv(&(rpt->fragmentLength), &cursor, &unparsedBytes);
	}

	if (rpt->flags & BP_RECEIVED_RPT)
	{
		extractSdnv(&(rpt->receiptTime.seconds), &cursor,
				&unparsedBytes);
		extractSdnv(&(rpt->receiptTime.nanosec), &cursor,
				&unparsedBytes);
	}

	if (rpt->flags & BP_CUSTODY_RPT)
	{
		extractSdnv(&(rpt->acceptanceTime.seconds), &cursor,
				&unparsedBytes);
		extractSdnv(&(rpt->acceptanceTime.nanosec), &cursor,
				&unparsedBytes);
	}

	if (rpt->flags & BP_FORWARDED_RPT)
	{
		extractSdnv(&(rpt->forwardTime.seconds), &cursor,
				&unparsedBytes);
		extractSdnv(&(rpt->forwardTime.nanosec), &cursor,
				&unparsedBytes);
	}

	if (rpt->flags & BP_DELIVERED_RPT)
	{
		extractSdnv(&(rpt->deliveryTime.seconds), &cursor,
				&unparsedBytes);
		extractSdnv(&(rpt->deliveryTime.nanosec), &cursor,
				&unparsedBytes);
	}

	if (rpt->flags & BP_DELETED_RPT)
	{
		extractSdnv(&(rpt->deletionTime.seconds), &cursor,
				&unparsedBytes);
		extractSdnv(&(rpt->deletionTime.nanosec), &cursor,
				&unparsedBytes);
	}

	extractSdnv(&(rpt->creationTime.seconds), &cursor, &unparsedBytes);
	extractSdnv(&(rpt->creationTime.count), &cursor, &unparsedBytes);
	extractSdnv(&eidLength, &cursor, &unparsedBytes);
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

void	bpEraseStatusRpt(BpStatusRpt *rpt)
{
	MRELEASE(rpt->sourceEid);
}

int	bpParseAdminRecord(int *adminRecordType, BpStatusRpt *rpt,
		BpCtSignal *csig, Object payload)
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
	sdr_begin_xn(bpSdr);
	buflen = zco_source_data_length(bpSdr, payload);
	if ((buffer = MTAKE(buflen)) == NULL)
	{
		putErrmsg("Can't start parsing admin record.", NULL);
		sdr_cancel_xn(bpSdr);
		return -1;
	}

	zco_start_receiving(bpSdr, payload, &reader);
	bytesToParse = zco_receive_source(bpSdr, &reader, buflen, buffer);
	if (bytesToParse < 0)
	{
		putErrmsg("Can't receive admin record.", NULL);
		sdr_cancel_xn(bpSdr);
		MRELEASE(buffer);
		return -1;
	}

	cursor = buffer;
	unparsedBytes = bytesToParse;
	if (unparsedBytes < 1)
	{
		writeMemoNote("[?] Incoming admin record too short",
				itoa(unparsedBytes));
		sdr_cancel_xn(bpSdr);
		MRELEASE(buffer);
		result = 0;
	}
	else
	{
		*adminRecordType = (*cursor >> 4 ) & 0x0f;
		bundleIsFragment = *cursor & 0x01;
		cursor++;
		unparsedBytes--;
		switch (*adminRecordType)
		{
		case BP_STATUS_REPORT:
			result = bpParseStatusRpt(rpt, (unsigned char *) cursor,
					unparsedBytes, bundleIsFragment);
			break;

		case BP_CUSTODY_SIGNAL:
			result = bpParseCtSignal(csig, (unsigned char *) cursor,
					unparsedBytes, bundleIsFragment);
			break;

		default:
			writeMemoNote("[?] Unknown admin record type",
					itoa(*adminRecordType));
			result = 0;
		}
	}

	MRELEASE(buffer);
	zco_stop_receiving(bpSdr, &reader);
	if (sdr_end_xn(bpSdr) < 0)
	{
		putErrmsg("Can't update status report after parsing.", NULL);
		return -1;
	}

	return result;
}

/*	*	*	Bundle catenation functions	*	*	*/

static Object	catenateBundle(Bundle *bundle)
{
	Sdr		bpSdr = getIonsdr();
	Sdnv		bundleProcFlagsSdnv;
	int		residualBlkLength;
	Sdnv		residualBlkLengthSdnv;
	Sdnv		eidSdnvs[8];
	int		totalLengthOfEidSdnvs;
	Sdnv		creationTimestampTimeSdnv;
	Sdnv		creationTimestampCountSdnv;
	unsigned long	lifetime;
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
	lifetime = bundle->expirationTime - bundle->id.creationTime.seconds;
	encodeSdnv(&lifetimeSdnv, lifetime);
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
		return 0;
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
			return 0;
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

	return zco_add_reference(bpSdr, bundle->payload.content);
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

	result = sendCtSignal(bundle, dictionary, 1, 0);
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
		&bundle->dictionaryLength, vscheme->custodianEidString,
			vscheme->custodianSchemeNameLength);
	bundle->custodian.d.nssOffset
		= addStringToDictionary(strings, stringLengths, &stringCount,
		&bundle->dictionaryLength, vscheme->custodianEidString
			+ (vscheme->custodianSchemeNameLength + 1),
			vscheme->custodianNssLength);

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
	if (vschemeElt == 0
	|| strcmp(vscheme->custodianEidString, _nullEid()) == 0)
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
	bundle->custodian.c.nodeNbr = getOwnNodeNbr();
	bundle->custodian.c.serviceNbr = 0;
	if (processExtensionBlocks(bundle, PROCESS_ON_TAKE_CUSTODY, NULL) < 0)
	{
		putErrmsg("Can't process extensions.", "take custody");
		return -1;
	}

	return 0;
}

int	bpAccept(Bundle *bundle)
{
	Sdr	bpSdr = getIonsdr();
	char	*dictionary;
	int	result;

	CHKERR(ionLocked());
	purgeStationsStack(bundle);
	if (bundle->catenated)	/*	Re-forwarding custodial bundle.	*/
	{
		zco_discard_first_header(bpSdr, bundle->payload.content);
		if (bundle->extensionsLength[POST_PAYLOAD] > 0)
		{
			zco_discard_last_trailer(bpSdr,
					bundle->payload.content);
		}

		bundle->catenated = 0;
		return 0;
	}

	if (bundle->bundleProcFlags & BDL_IS_CUSTODIAL)
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

static Object	insertXrefIntoList(Object queue, Object lastElt,
			Object xmitRefAddr, int priority,
			unsigned char ordinal, time_t enqueueTime)
{
	Sdr	bpSdr = getIonsdr();
		OBJ_POINTER(XmitRef, xr);
		OBJ_POINTER(Bundle, bundle);

	/*	Bundles have transmission seniority which must be
	 *	honored.  A bundle that was enqueued for transmission
	 *	a while ago and now is being reforwarded must jump
	 *	the queue ahead of bundles of the same priority that
	 *	were enqueued more recently.				*/

	GET_OBJ_POINTER(bpSdr, XmitRef, xr, sdr_list_data(bpSdr, lastElt));
	GET_OBJ_POINTER(bpSdr, Bundle, bundle, xr->bundleObj);
	while (enqueueTime < bundle->enqueueTime)
	{
		lastElt = sdr_list_prev(bpSdr, lastElt);
		if (lastElt == 0)
		{
			break;		/*	Reached head of queue.	*/
		}

		GET_OBJ_POINTER(bpSdr, XmitRef, xr,
				sdr_list_data(bpSdr, lastElt));
		GET_OBJ_POINTER(bpSdr, Bundle, bundle, xr->bundleObj);
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
		return sdr_list_insert_after(bpSdr, lastElt, xmitRefAddr);
	}

	return sdr_list_insert_first(bpSdr, queue, xmitRefAddr);
}

static Object	enqueueUrgentBundle(Outduct *duct, int ordinal,
			Object xmitRefAddr, time_t enqueueTime,
			int backlogIncrement)
{
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
				xmitRefAddr);
	}
	else		/*	Enqueue after this one.			*/
	{
		xmitElt = insertXrefIntoList(duct->urgentQueue, lastElt,
				xmitRefAddr, 2, ordinal, enqueueTime);
	}

	if (xmitElt)
	{
		ord->lastForOrdinal = xmitElt;
		increaseScalar(&(ord->backlog), backlogIncrement);
	}

	return xmitElt;
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
	Object		xmitRefAddr;
	XmitRef		xr;
	int		backlogIncrement;
	ClProtocol	protocol;
	time_t		enqueueTime;
	int		priority;
	Object		lastElt;

	CHKERR(ionLocked());
	CHKERR(directive && bundle && bundleObj && proxNodeEid);
	CHKERR(*proxNodeEid && strlen(proxNodeEid) < MAX_SDRSTRING);

	/*	We have settled on the best node to send this bundle
	 *	to; if it can't get there because the duct to that
	 *	node is blocked, then the bundle goes into limbo
	 *	until something changes.
	 *
	 *	First check to see if the duct is blocked.		*/

	ductAddr = sdr_list_data(bpSdr, directive->outductElt);
	sdr_stage(bpSdr, (char *) &duct, ductAddr, sizeof(Outduct));
	if (duct.blocked)
	{
		return enqueueToLimbo(bundle, bundleObj);
	}

	/*      Now construct a transmission reference.			*/

	xr.bundleObj = bundleObj;
	xr.proxNodeEid = sdr_string_create(bpSdr, proxNodeEid);

	/*	Retrieve destination induct name, if applicable.	*/

	if (directive->destDuctName)
	{
		if (sdr_string_read(getIonsdr(), destDuctName,
				directive->destDuctName) < 0)
		{
			putErrmsg("Can't retrieve dest duct name.", NULL);
			return -1;
		}

		xr.destDuctName = sdr_string_create(bpSdr, destDuctName);
	}
	else
	{
		xr.destDuctName = 0;
	}

	xmitRefAddr = sdr_malloc(bpSdr, sizeof(XmitRef));
	if (xmitRefAddr == 0)
	{
		putErrmsg("Can't enqueue bundle.", NULL);
		return -1;
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

	/*	Insert bundle's xmitRef into the appropriate
	 *	transmission queue of the selected Duct.		*/

	priority = COS_FLAGS(bundle->bundleProcFlags) & 0x03;
	switch (priority)
	{
	case 0:
		lastElt = sdr_list_last(bpSdr, duct.bulkQueue);
		if (lastElt == 0)
		{
			xr.ductXmitElt = sdr_list_insert_first(bpSdr,
				duct.bulkQueue, xmitRefAddr);
		}
		else
		{
			xr.ductXmitElt = insertXrefIntoList(duct.bulkQueue,
				lastElt, xmitRefAddr, 0, 0, enqueueTime);
		}

		increaseScalar(&duct.bulkBacklog, backlogIncrement);
		break;

	case 1:
		lastElt = sdr_list_last(bpSdr, duct.stdQueue);
		if (lastElt == 0)
		{
			xr.ductXmitElt = sdr_list_insert_first(bpSdr,
				duct.stdQueue, xmitRefAddr);
		}
		else
		{
			xr.ductXmitElt = insertXrefIntoList(duct.stdQueue,
				lastElt, xmitRefAddr, 1, 0, enqueueTime);
		}

		increaseScalar(&duct.stdBacklog, backlogIncrement);
		break;

	default:
		xr.ductXmitElt = enqueueUrgentBundle(&duct,
				bundle->extendedCOS.ordinal, xmitRefAddr,
				enqueueTime, backlogIncrement);
		increaseScalar(&duct.urgentBacklog, backlogIncrement);
	}

	sdr_write(bpSdr, ductAddr, (char *) &duct, sizeof(Outduct));
	xr.bundleXmitElt = sdr_list_insert_last(bpSdr, bundle->xmitRefs,
			xmitRefAddr);
	sdr_write(bpSdr, xmitRefAddr, (char *) &xr, sizeof(XmitRef));
	bundle->xmitsNeeded += 1;
	sdr_write(bpSdr, bundleObj, (char *) bundle, sizeof(Bundle));
	noteStateStats(BPSTATS_FORWARD, bundle);
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
	Object	xmitRefAddr;
	XmitRef	xr;

	/*      ION has determined that this bundle must wait
	 *      in limbo until a duct is unblocked, enabling
	 *      transmission.  So construct a "limbo" transmission
	 *      reference.						*/

	CHKERR(ionLocked());
	CHKERR(bundleObj && bundle);
	if (bundle->extendedCOS.flags & BP_MINIMUM_LATENCY)
	{
		/*	"Critical" bundles are never reforwarded
		 *	(see notes on this in bpReforwardBundle),
		 *	so they can never be reforwarded after
		 *	insertion into limbo, so there's no point
		 *	in putting them into limbo.  So we don't.	*/

		return 0;
	}

	xmitRefAddr = sdr_malloc(bpSdr, sizeof(XmitRef));
	if (xmitRefAddr == 0)
	{
		putErrmsg("Can't put bundle in limbo.", NULL);
		return -1;
	}

	xr.bundleObj = bundleObj;
	xr.proxNodeEid = 0;
	xr.destDuctName = 0;
	xr.ductXmitElt = sdr_list_insert_last(bpSdr,
			bpConstants->limboQueue, xmitRefAddr);
	xr.bundleXmitElt = sdr_list_insert_last(bpSdr,
			bundle->xmitRefs, xmitRefAddr);
	sdr_write(bpSdr, xmitRefAddr, (char *) &xr, sizeof(XmitRef));
	bundle->xmitsNeeded += 1;
	sdr_write(bpSdr, bundleObj, (char *) bundle, sizeof(Bundle));
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
	Sdr		bpSdr = getIonsdr();
	Object		xrAddr;
	XmitRef		xr;
	Bundle		bundle;

	xrAddr = sdr_list_data(bpSdr, xmitElt);
	sdr_read(bpSdr, (char *) &xr, xrAddr, sizeof(XmitRef));
	sdr_read(bpSdr, (char *) &bundle, xr.bundleObj, sizeof(Bundle));
	sdr_list_delete(bpSdr, xr.bundleXmitElt, NULL, NULL);
	removeBundleFromQueue(xmitElt, &bundle, protocol, outductObj, outduct);
	if (xr.proxNodeEid)
	{
		sdr_free(bpSdr, xr.proxNodeEid);
	}

	if (xr.destDuctName)
	{
		sdr_free(bpSdr, xr.destDuctName);
	}

	sdr_free(bpSdr, xrAddr);

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

		return bpReforwardBundle(xr.bundleObj);
	}

	/*	Must queue the bundle into limbo unconditionally.	*/

	sdr_stage(bpSdr, (char *) &bundle, xr.bundleObj, 0);
	if (bundle.overdueElt)
	{
		/*	Bundle was un-queued before "overdue"
	 	*	alarm went off, so disable the alarm.		*/

		sdr_free(bpSdr, sdr_list_data(bpSdr, bundle.overdueElt));
		sdr_list_delete(bpSdr, bundle.overdueElt, NULL, NULL);
		bundle.overdueElt = 0;
	}

	if (bundle.ctDueElt)
	{
		sdr_free(bpSdr, sdr_list_data(bpSdr, bundle.ctDueElt));
		sdr_list_delete(bpSdr, bundle.ctDueElt, NULL, NULL);
		bundle.ctDueElt = 0;
	}

	if (bundle.inTransitEntry)
	{
		sdr_hash_delete_entry(bpSdr, bundle.inTransitEntry);
		bundle.inTransitEntry = 0;
	}

	bundle.xmitsNeeded -= 1;
	return enqueueToLimbo(&bundle, xr.bundleObj);
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

	outductObj = sdr_list_data(bpSdr, vduct->outductElt);
	sdr_begin_xn(bpSdr);
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
	Object	xrAddr;
	XmitRef	xr;
	Bundle	bundle;

	CHKERR(ionLocked());
	CHKERR(xmitElt);
	xrAddr = sdr_list_data(bpSdr, xmitElt);
	sdr_read(bpSdr, (char *) &xr, xrAddr, sizeof(XmitRef));
	sdr_stage(bpSdr, (char *) &bundle, xr.bundleObj, sizeof(Bundle));
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
	 *	by deleting the two XmitElt objects in this bundle
	 *	we are deleting the xmitElt that was passed to
	 *	this function -- don't count on being able to
	 *	navigate to the next xmitElt in limboQueue from it!	*/

	sdr_free(bpSdr, xrAddr);
	sdr_list_delete(bpSdr, xr.bundleXmitElt, NULL, NULL);
	sdr_list_delete(bpSdr, xr.ductXmitElt, NULL, NULL);
	bundle.xmitsNeeded -= 1;
	sdr_write(bpSdr, xr.bundleObj, (char *) &bundle, sizeof(Bundle));
	if ((_bpvdb(NULL))->watching & WATCH_delimbo)
	{
		putchar('k');
		fflush(stdout);
	}

	/*	Now see if the bundle can finally be transmitted.	*/

	if (bpReforwardBundle(xr.bundleObj) < 0)
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

	outductObj = sdr_list_data(bpSdr, vduct->outductElt);
	sdr_begin_xn(bpSdr);
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

		sdr_free(bpSdr, sdr_list_data(bpSdr, bundle->ctDueElt));
		sdr_list_delete(bpSdr, bundle->ctDueElt, NULL, NULL);
		bundle->ctDueElt = 0;
	}

	sdr_write(bpSdr, bundleAddr, (char *) bundle, sizeof(Bundle));
}

int	bpAbandon(Object bundleObj, Bundle *bundle)
{
	char 	*dictionary = NULL;
	int	result1 = 0;
	int	result2 = 0;

	CHKERR(bundleObj && bundle);
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
		if (bundle->bundleProcFlags & BDL_IS_CUSTODIAL)
		{
			result2 = sendCtSignal(bundle, dictionary, 0,
					CtNoKnownRoute);
			if (result2 < 0)
			{
				putErrmsg("Can't send custody signal.", NULL);
			}
		}
	}

	releaseDictionary(dictionary);
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

	*winner = NULL;		/*	N/A for strict priority.	*/
	*eltp = 0;		/*	Default: nothing ready.		*/
	i = EXPEDITED_FLOW;	/*	Start with highest priority.	*/
	while (1)
	{
		flow = flows + i;
		elt = sdr_list_first(getIonsdr(), flow->outboundBundles);
		if (elt)
		{
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
			Object *bundleObj, Bundle *bundle,
			Object *proxNodeEid, Object *destDuctName)
{
	Sdr		bpSdr = getIonsdr();
	PsmPartition	bpwm = getIonwm();
	ClProtocol	protocol;
	Outflow		*selectedFlow;
	Object		xmitElt;
	Object		xrAddr;
	XmitRef		xr;
	unsigned long	neighborNodeNbr;
	IonNode		*destNode;
	PsmAddress	nextNode;
	PsmAddress	snubElt;
	PsmAddress	nextSnub;
	IonSnub		*snub;

	CHKERR(ionLocked());
	sdr_read(bpSdr, (char *) &protocol, outduct->protocol,
			sizeof(ClProtocol));
	while (1)	/*	Might do one or more reforwards.	*/
	{
		selectNextBundleForTransmission(flows, &selectedFlow, &xmitElt);
		if (xmitElt == 0)		/*	Nothing ready.	*/
		{
			sdr_exit_xn(bpSdr);

			/*	Wait until forwarder announces an outbound
		 	*	bundle by giving duct's semaphore.	*/

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

				return -1;
			}

			sdr_begin_xn(bpSdr);
			sdr_stage(bpSdr, (char *) outduct, outductObj,
					sizeof(Outduct));
			continue;	/*	Should succeed now.	*/
		}

		/*	Got next outbound transmission.  Remove it
		 *	from the queue for this duct and from the list
		 *	of transmissions for the referenced bundle.
		 *	Do NOT delete destDuctName (if present); it
		 *	must be passed back to bpDequeue.		*/

		xrAddr = sdr_list_data(bpSdr, xmitElt);
		sdr_read(bpSdr, (char *) &xr, xrAddr, sizeof(XmitRef));
		sdr_free(bpSdr, xrAddr);
		sdr_list_delete(bpSdr, xr.bundleXmitElt, NULL, NULL);
		sdr_stage(bpSdr, (char *) bundle, xr.bundleObj, sizeof(Bundle));
#ifdef ION_BANDWIDTH_RESERVED
		if (selectedFlow != NULL)	/*	Not urgent.	*/
		{
			selectedFlow->totalBytesSent += bundle->payload.length;
			resyncFlows(flows);
		}
#endif
		removeBundleFromQueue(xmitElt, bundle, &protocol, outductObj,
				outduct);
		sdr_write(bpSdr, outductObj, (char *) outduct, sizeof(Outduct));

		/*	If the neighbor for this duct has begun
		 *	snubbing bundles for the indicated destination
		 *	since this bundle was enqueued to this duct,
		 *	re-forward the bundle on a different route
		 *	(if possible).  Note that we can only track
		 *	snubs for CBHE-conformant destination nodes.	*/

		if (bundle->id.source.cbhe)
		{
			neighborNodeNbr = strtoul(vduct->ductName, NULL, 0);
			destNode = findNode(getIonVdb(),
				bundle->id.source.c.nodeNbr, &nextNode);
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

					if (bpReforwardBundle(xr.bundleObj) < 0)
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

					sdr_begin_xn(bpSdr);
					sdr_stage(bpSdr, (char *) outduct,
						outductObj, sizeof(Outduct));
					continue;
				}
			}
		}

		/*	Return this bundle to the CLO.			*/

		*bundleObj = xr.bundleObj;
		*proxNodeEid = xr.proxNodeEid;
		*destDuctName = xr.destDuctName;
		return 0;
	}
}

static int	constructInTransitHashKey(char *buffer, char *sourceEid,
			unsigned long seconds, unsigned long count,
			unsigned long offset, unsigned long length)
{
	memset(buffer, 0, IN_TRANSIT_KEY_BUFLEN);
	isprintf(buffer, IN_TRANSIT_KEY_BUFLEN, "%s:%lu:%lu:%lu:%lu",
			sourceEid, seconds, count, offset, length);
	return strlen(buffer);
}

int	bpDequeue(VOutduct *vduct, Outflow *flows, Object *bundleZco,
		BpExtendedCOS *extendedCOS, char *destDuctName,
		int stewardshipAccepted)
{
	Sdr		bpSdr = getIonsdr();
	Object		outductObj;
	Outduct		outduct;
			OBJ_POINTER(ClProtocol, protocol);
	Object		bundleObj;
	Bundle		bundle;
	Object		proxNodeEidObj;
	Object		destDuctNameObj;
	char		proxNodeEid[SDRSTRING_BUFSZ];
	DequeueContext	context;
	char		*dictionary;
	int		xmitLength;
	char		*sourceEid;
	char		inTransitKey[IN_TRANSIT_KEY_BUFLEN];

	CHKERR(vduct && flows && bundleZco && extendedCOS && destDuctName);
	*bundleZco = 0;			/*	Default behavior.	*/
	*destDuctName = '\0';		/*	Default behavior.	*/
	sdr_begin_xn(bpSdr);

	/*	Transmission rate control: wait for capacity.		*/

	while (vduct->xmitThrottle.capacity <= 0)
	{
		sdr_exit_xn(bpSdr);
		if (sm_SemTake(vduct->xmitThrottle.semaphore) < 0)
		{
			putErrmsg("CLO can't take throttle semaphore.", NULL);
			return -1;
		}

		if (sm_SemEnded(vduct->xmitThrottle.semaphore))
		{
			writeMemo("[i] Outduct has been stopped.");

			/*	End task, but without error.		*/

			return -1;
		}

		sdr_begin_xn(bpSdr);
	}

	outductObj = sdr_list_data(bpSdr, vduct->outductElt);
	sdr_stage(bpSdr, (char *) &outduct, outductObj, sizeof(Outduct));
	GET_OBJ_POINTER(bpSdr, ClProtocol, protocol, outduct.protocol);

	/*	Get a transmittable bundle.				*/

	if (getOutboundBundle(flows, vduct, &outduct, outductObj,
		&bundleObj, &bundle, &proxNodeEidObj, &destDuctNameObj) < 0)
	{
		writeMemo("[?] CLO can't get next outbound bundle.");
		sdr_cancel_xn(bpSdr);

		/*	End task; may be with or without error.		*/

		return -1;
	}

	if (proxNodeEidObj)
	{
		sdr_string_read(bpSdr, proxNodeEid, proxNodeEidObj);
		sdr_free(bpSdr, proxNodeEidObj);
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

	bundle.xmitsNeeded -= 1;
	if (bundle.overdueElt)
	{
		/*	Bundle was transmitted before "overdue"
		 *	alarm went off, so disable the alarm.		*/

		sdr_free(bpSdr, sdr_list_data(bpSdr, bundle.overdueElt));
		sdr_list_delete(bpSdr, bundle.overdueElt, NULL, NULL);
		bundle.overdueElt = 0;
	}

	/*	This bundle may have been queued for transmission via
	 *	multiple outducts, in which case it might have been
	 *	catenated for transmission by a previous bpDequeue().
	 *	If so, strip off the BP header and trailer: the
	 *	extension blocks for the bundle we are issuing now
	 *	may be entirely different.
	 *
	 *	NOTE the potential for problems here!  If the
	 *	previously catenated bundle is still in the system
	 *	and liable to retransmission, by rebuilding its
	 *	header and trailer we may corrupt the retransmission
	 *	of that earlier catenated bundle.  The only way to
	 *	avoid this is to create entirely new, distinct copies
	 *	of the bundle rather than use the zero-copy system
	 *	and reference counting.  For the foreseeable future
	 *	this should be no problem, since we're not doing any
	 *	flooding or multicast or even any "critical bundle"
	 *	routing.  But in more complex topologies and operating
	 *	scenarios this problem will have to be addressed.	*/

	if (bundle.catenated)
	{
		zco_discard_first_header(bpSdr, bundle.payload.content);
		if (bundle.extensionsLength[POST_PAYLOAD] > 0)
		{
			zco_discard_last_trailer(bpSdr, bundle.payload.content);
		}
	}

	/*	In any case, we now serialize the bundle header and
	 *	prepend that header to the payload of the bundle;
	 *	if there are post-payload extension blocks we also
	 *	serialize them into a single trailer and append it
	 *	to the payload.  The output of the catenateBundle
	 *	function is a new ZCO reference to the ZCO that was
	 *	originally just the payload of the selected bundle
	 *	but is now the fully catenated bundle, ready for
	 *	transmission.						*/

	*bundleZco = catenateBundle(&bundle);
	bundle.catenated = 1;

	/*	Some final extension-block processing may be necessary
	 *	after catenation of the bundle, notably BAB hash
	 *	calculation.						*/

	if (processExtensionBlocks(&bundle, PROCESS_ON_TRANSMIT, NULL) < 0)
	{
		putErrmsg("Can't process extensions.", "dequeue");
		sdr_cancel_xn(bpSdr);
		return -1;
	}

	/*	There are a couple of things that we may need the
	 *	dictionary for.						*/

	if ((dictionary = retrieveDictionary(&bundle)) == (char *) &bundle)
	{
		putErrmsg("Can't retrieve dictionary.", NULL);
		sdr_cancel_xn(bpSdr);
		return -1;
	}

	/*	At this point we check the stewardshipAccepted flag.
	 *	If the bundle is critical then it has been queued
	 *	for transmission on all possible routes and is not
	 *	subject to reforwarding (see notes on this in
	 *	bpReforwardBundle); since it is not subject to
	 *	reforwarding, stewardship is meaningless -- on
	 *	convergence-layer transmission failure the attempt
	 *	to transmit the bundle is simply abandoned.  So
	 *	in this event the stewardshipAccepted flag is forced
	 *	to zero even if the convergence-layer adapter for
	 *	this outduct is one that normally accepts stewardship.	*/

	if (bundle.extendedCOS.flags & BP_MINIMUM_LATENCY)
	{
		stewardshipAccepted = 0;
	}

	/*	If stewardship of this bundle is accepted by the CLO
	 *	then we know that the bundle's inTransitEntry is zero:
	 *	the bundle can't be critical, therefore it can't have
	 *	been queued for transmission at any other outduct, and
	 *	if it was previously queued for transmission on this
	 *	outduct it can only be subject to re-dequeue in the
	 *	event that it was reforwarded -- and all paths to
	 *	reforwarding it entailed erasing the prior value of
	 *	inTransitEntry.  So we now attempt to construct a key
	 *	for inserting the bundle into the inTransitHash; if
	 *	the key is small enough, then the bundle goes into
	 *	the inTransit hash table (if possible) pending a
	 *	determination by the CLO as to the success or
	 *	failure of convergence-layer transmission.
	 *
	 *	Note that the convergence-layer adapter task *must*
	 *	call either the bpHandleXmitSuccess function or
	 *	the bpHandleXmitFailure function for *every*
	 *	bundle it obtains via bpDequeue for which it has
	 *	accepted stewardship.
	 *
	 *	If the bundle is not inserted into the inTransit
	 *	hash table, for any reason, then the bundle object
	 *	is subject to destruction immediately unless it is
	 *	pending delivery or it is pending another transmis-
	 *	sion or custody of the bundle has been accepted.
	 *
	 *	Note that the bundle's *payload* object (the ZCO we
	 *	are delivering for transmission) is protected from
	 *	destruction in any case because *bundleZco will be
	 *	an additional reference to that reference-counted
	 *	ZCO.  Even if bpDestroyBundle is called and the
	 *	bundle is in fact destroyed (destroying its reference
	 *	to the payload ZCO) at least one reference to that ZCO
	 *	will survive.  Since a ZCO is never destroyed until
	 *	its reference count drops to zero, the payload ZCO
	 *	will remain available to the CLO for transmission and
	 *	retransmission.						*/

	if (stewardshipAccepted)
	{
		if (printEid(&bundle.id.source, dictionary, &sourceEid) < 0)
		{
			releaseDictionary(dictionary);
			putErrmsg("Can't print source EID.", NULL);
			sdr_cancel_xn(bpSdr);
			return -1;
		}

		if (constructInTransitHashKey(inTransitKey, sourceEid,
				bundle.id.creationTime.seconds,
				bundle.id.creationTime.count,
				bundle.id.fragmentOffset,
				bundle.totalAduLength == 0 ? 0
						: bundle.payload.length)
				<= IN_TRANSIT_KEY_LEN)
		{
			if (sdr_hash_insert(bpSdr,
					(_bpConstants())->inTransitHash,
					inTransitKey, bundleObj,
					&bundle.inTransitEntry) < 0)
			{
				MRELEASE(sourceEid);
				releaseDictionary(dictionary);
				putErrmsg("Can't post to in-transit hash.",
						NULL);
				sdr_cancel_xn(bpSdr);
				return -1;
			}
		}

		MRELEASE(sourceEid);
	}

	/*	That's the end of the changes to the bundle.		*/

	sdr_write(bpSdr, bundleObj, (char *) &bundle, sizeof(Bundle));

	/*	Track this transmission event.				*/

	noteStateStats(BPSTATS_XMIT, &bundle);
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

	if (destDuctNameObj)
	{
		sdr_string_read(bpSdr, destDuctName, destDuctNameObj);
		sdr_free(bpSdr, destDuctNameObj);
	}

	/*	Finally, authorize transmission of applicable status
	 *	report message and destruction of the bundle object
	 *	unless stewardship was successfully accepted.		*/

	if (bundle.inTransitEntry == 0)
	{
		if (SRR_FLAGS(bundle.bundleProcFlags) & BP_FORWARDED_RPT)
		{
			bundle.statusRpt.flags |= BP_FORWARDED_RPT;
			getCurrentDtnTime(&bundle.statusRpt.forwardTime);
			if (sendStatusRpt(&bundle, dictionary) < 0)
			{
				releaseDictionary(dictionary);
				putErrmsg("Can't send status report.", NULL);
				sdr_cancel_xn(bpSdr);
				return -1;
			}
		}

		if (bpDestroyBundle(bundleObj, 0) < 0)
		{
			releaseDictionary(dictionary);
			putErrmsg("Can't destroy bundle.", NULL);
			sdr_cancel_xn(bpSdr);
			return -1;
		}
	}

	releaseDictionary(dictionary);
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
	bytesReceived = zco_receive_source(sdr, reader, bytesToReceive,
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

static int	decodeHeader(Sdr sdr, Object zco, ZcoReader *reader,
			unsigned char *buffer, int bytesBuffered, Bundle *image,
			char **dictionary, unsigned int *bundleLength)
{
	unsigned char	*endOfBuffer;
	unsigned char	*cursor;
	int		sdnvLength;
	unsigned long	longNumber;
	int		i;
	unsigned long	eidSdnvValues[8];
	unsigned char	blkType;
	unsigned long	blkProcFlags;
	unsigned long	eidReferencesCount;
	unsigned long	blockDataLength;

	cursor = buffer;
	endOfBuffer = buffer + bytesBuffered;

	/*	Skip over version number.				*/

	if (bufAdvance(1, bundleLength, &cursor, endOfBuffer) == 0)
	{
		return 0;
	}

	/*	Parse out the bundle processing flags.			*/

	sdnvLength = decodeSdnv(&(image->bundleProcFlags), cursor);
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

	sdnvLength = decodeSdnv(&(image->id.creationTime.seconds), cursor);
	if (bufAdvance(sdnvLength, bundleLength, &cursor, endOfBuffer) == 0)
	{
		return 0;
	}

	sdnvLength = decodeSdnv(&(image->id.creationTime.count), cursor);
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

	sdnvLength = decodeSdnv(&(image->dictionaryLength), cursor);
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
		image->id.source.c.nodeNbr = eidSdnvValues[2];
		image->id.source.c.serviceNbr = eidSdnvValues[3];
	}
	else
	{
		*dictionary = (char *) cursor;
		*bundleLength += image->dictionaryLength;
		cursor += image->dictionaryLength;
		image->id.source.cbhe = 0;
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

	sdnvLength = decodeSdnv(&(image->id.fragmentOffset), cursor);
	if (bufAdvance(sdnvLength, bundleLength, &cursor, endOfBuffer) == 0)
	{
		return 0;
	}

	sdnvLength = decodeSdnv(&(image->totalAduLength), cursor);
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

		sdnvLength = decodeSdnv(&blkProcFlags, cursor);
		if (bufAdvance(sdnvLength, bundleLength, &cursor, endOfBuffer)
				== 0)
		{
			return 0;
		}

		if (blkProcFlags & BLK_HAS_EID_REFERENCES)
		{
			/*	Skip over EID references.		*/

			sdnvLength = decodeSdnv(&eidReferencesCount, cursor);
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

		sdnvLength = decodeSdnv(&blockDataLength, cursor);
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
	Object		handle;
	ZcoReader	reader;
	int		bytesBuffered;

	*bundleLength = 0;	/*	Initialize to default.		*/
	memset((char *) image, 0, sizeof(Bundle));

	/*	Must use a *different* ZCO reference for extraction,
	 *	because the original may be needed for other purposes.	*/

	sdr_begin_xn(sdr);
	handle = zco_add_reference(sdr, zco);
	if (handle == 0)
	{
		putErrmsg("Can't get new handle for catenated bundle.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	zco_start_transmitting(sdr, handle, &reader);
	bytesBuffered = zco_transmit(sdr, &reader, BP_MAX_BLOCK_SIZE,
			(char *) buffer);
	if (bytesBuffered < 0)
	{
		putErrmsg("Can't extract primary block.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	if (decodeHeader(sdr, handle, &reader, buffer,
			bytesBuffered, image, dictionary, bundleLength) < 0)
	{
		putErrmsg("Can't decode bundle header.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	zco_stop_transmitting(sdr, &reader);
	zco_destroy_reference(sdr, handle);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't decode bundle.", NULL);
		return -1;
	}

	return 0;
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
	Object		timelineElt;

	CHKERR(bundleZco);
	CHKERR(bundleObj);
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
		*bundleObj = 0;		/*	Bundle not located.	*/
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

	sdr_begin_xn(bpSdr);		/*	Just to lock memory.	*/
	result = findBundle(sourceEid, &image.id.creationTime,
			image.id.fragmentOffset,
			image.totalAduLength == 0 ? 0 : image.payload.length,
			bundleObj, &timelineElt);
	sdr_exit_xn(bpSdr);
	MRELEASE(sourceEid);
	if (result < 0)
	{
		putErrmsg("Failed seeking bundle.", NULL);
		return -1;
	}

	if (timelineElt == 0)		/*	Probably TTL expired.	*/
	{
		*bundleObj = 0;		/*	Bundle not located.	*/
	}

	return 0;
}

int	bpMemo(Object bundleObj, int interval)
{
	Sdr	bpSdr = getIonsdr();
	Bundle	bundle;
	BpEvent	event;

	CHKERR(bundleObj);
	CHKERR(interval > 0);
	event.type = ctDue;
	event.time = getUTCTime() + interval;
	event.ref = bundleObj;
	sdr_begin_xn(bpSdr);
	sdr_stage(bpSdr, (char *) &bundle, bundleObj, sizeof(Bundle));
	if (bundle.ctDueElt)
	{
		writeMemo("Revising a custody acceptance due timer.");
		sdr_free(bpSdr, sdr_list_data(bpSdr, bundle.ctDueElt));
		sdr_list_delete(bpSdr, bundle.ctDueElt, NULL, NULL);
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

static int	extractInTransitBundle(Object *bundleAddr, char *sourceEid,
			BpTimestamp *creationTime,
			unsigned long fragmentOffset,
			unsigned long fragmentLength)
{
	Sdr	bpSdr = getIonsdr();
	char	key[IN_TRANSIT_KEY_BUFLEN];

	CHKERR(ionLocked());
	if (constructInTransitHashKey(key, sourceEid,
			creationTime->seconds, creationTime->count,
			fragmentOffset, fragmentLength)
			> IN_TRANSIT_KEY_LEN)
	{
		return 0;	/*	Can't be in hash table.		*/
	}

	if (sdr_hash_remove(bpSdr, (_bpConstants())->inTransitHash, key,
			(Address *) bundleAddr) < 0)
	{
		putErrmsg("Can't extract bundle from in-transit hash.", NULL);
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

	result = extractInTransitBundle(bundleObj, sourceEid,
			&image.id.creationTime, image.id.fragmentOffset,
			image.totalAduLength == 0 ? 0 : image.payload.length);
	MRELEASE(sourceEid);
	if (result < 0)
	{
		putErrmsg("Failed retrieving in-transit bundle.", NULL);
		return -1;
	}

	return 0;
}

int	bpHandleXmitSuccess(Object bundleZco)
{
	Sdr	bpSdr = getIonsdr();
	Object	bundleAddr;
	Bundle	bundle;
	char	*dictionary;
	int	result;

	sdr_begin_xn(bpSdr);
	if (retrieveInTransitBundle(bundleZco, &bundleAddr) < 0)
	{
		sdr_cancel_xn(bpSdr);
		putErrmsg("Can't locate bundle for okay transmission.", NULL);
		return -1;
	}

	if (bundleAddr == 0)	/*	Bundle not found in inTransit.	*/
	{
		if (sdr_end_xn(bpSdr) < 0)
		{
			putErrmsg("Failed handling xmit success.", NULL);
			return -1;
		}

		return 0;	/*	bpDestroyBundle already called.	*/
	}

	sdr_stage(bpSdr, (char *) &bundle, bundleAddr, sizeof(Bundle));
	bundle.inTransitEntry = 0;
	sdr_write(bpSdr, bundleAddr, (char *) &bundle, sizeof(Bundle));

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

	sdr_begin_xn(bpSdr);
	if (retrieveInTransitBundle(bundleZco, &bundleAddr) < 0)
	{
		sdr_cancel_xn(bpSdr);
		putErrmsg("Can't locate bundle for failed transmission.", NULL);
		return -1;
	}

	if (bundleAddr == 0)	/*	Bundle not found in inTransit.	*/
	{
		if (sdr_end_xn(bpSdr) < 0)
		{
			putErrmsg("Failed handling xmit failure.", NULL);
			return -1;
		}

		return 0;	/*	No bundle, can't retransmit.	*/
	}

	sdr_stage(bpSdr, (char *) &bundle, bundleAddr, sizeof(Bundle));
	bundle.inTransitEntry = 0;
	sdr_write(bpSdr, bundleAddr, (char *) &bundle, sizeof(Bundle));

	/*	Note that the "timeout" statistics count failures of
	 *	convergence-layer transmission of bundles for which
	 *	stewardship was accepted.  This has nothing to do
	 *	with custody transfer.					*/

	noteStateStats(BPSTATS_TIMEOUT, &bundle);
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

	purgeXmitRefs(&bundle);
	if (bundle.overdueElt)
	{
		sdr_free(bpSdr, sdr_list_data(bpSdr, bundle.overdueElt));
		sdr_list_delete(bpSdr, bundle.overdueElt, NULL, NULL);
		bundle.overdueElt = 0;
	}

	if (bundle.ctDueElt)
	{
		sdr_free(bpSdr, sdr_list_data(bpSdr, bundle.ctDueElt));
		sdr_list_delete(bpSdr, bundle.ctDueElt, NULL, NULL);
		bundle.ctDueElt = 0;
	}

	if (bundle.inTransitEntry)
	{
		sdr_hash_delete_entry(bpSdr, bundle.inTransitEntry);
		bundle.inTransitEntry = 0;
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
	static BpSAP	sap = NULL;

	if (newSap)
	{
		sap = *newSap;
		sm_TaskVarAdd((int *) &sap);
	}

	return sap;
}

static void	shutDownAdminApp()
{
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
	|| (bundle->extendedCOS.flags & BP_MINIMUM_LATENCY))
	{
		/*	For non-cbhe bundles we have no node number,
		 *	so we can't manage routing snubs.  If the
		 *	bundle is critical then it was already sent
		 *	on all possible routes, so there's no point
		 *	in responding to the routing snub.		*/

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
	|| (bundle->extendedCOS.flags & BP_MINIMUM_LATENCY))
	{
		/*	For non-cbhe bundles we have no node number,
		 *	so we can't manage routing snubs.  If the
		 *	bundle is critical then it was already sent
		 *	on all possible routes, so there's no point
		 *	in responding to the routing snub.		*/

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

int	_handleAdminBundles(char *adminEid, StatusRptCB handleStatusRpt,
		CtSignalCB handleCtSignal)
{
	Sdr		bpSdr = getIonsdr();
	BpVdb		*bpvdb = _bpvdb(NULL);
	int		running = 1;
	BpSAP		sap;
	BpDelivery	dlv;
	int		adminRecType;
	BpStatusRpt	rpt;
	BpCtSignal	cts;
	Object		timelineElt;
	Object		bundleAddr;
	Bundle		bundleBuf;
	Bundle		*bundle = &bundleBuf;
	char		*dictionary;
	char		*eidString;
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

		switch (bpParseAdminRecord(&adminRecType, &rpt, &cts, dlv.adu))
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

			sdr_begin_xn(bpSdr);
			if (findBundle(cts.sourceEid, &cts.creationTime,
					cts.fragmentOffset, cts.fragmentLength,
					&bundleAddr, &timelineElt) < 0)
			{
				sdr_exit_xn(bpSdr);
				putErrmsg("Can't fetch bundle.", NULL);
				running = 0;
				bpEraseCtSignal(&cts);
				break;		/*	Out of switch.	*/
			}

			if (timelineElt == 0)
			{
				/*	No such bundle; ignore CTS.	*/

				sdr_exit_xn(bpSdr);
				bpEraseCtSignal(&cts);
				break;		/*	Out of switch.	*/
			}

			/*	If custody was accepted, or if custody
			 *	was refused due to redundant reception
			 *	(meaning the receiver had previously
			 *	accepted custody and we just never got
			 *	the signal) we destroy the copy of the
			 *	bundle retained here.  Otherwise we
			 *	immediately re-dispatch the bundle,
			 *	hoping that a change in the condition
			 *	of the network (reduced congestion,
			 *	revised routing) has occurred since
			 *	the previous transmission so that
			 *	re-transmission will succeed.		*/

			sdr_stage(bpSdr, (char *) bundle, bundleAddr,
					sizeof(Bundle));
			if (cts.succeeded
			|| cts.reasonCode == CtRedundantReception)
			{
				if (bpvdb->watching & WATCH_m)
				{
					putchar('m');
					fflush(stdout);
				}

				forgetSnub(bundle, bundleAddr,
						dlv.bundleSourceEid);
				releaseCustody(bundleAddr, bundle);
				if (bpDestroyBundle(bundleAddr, 0) < 0)
				{
					putErrmsg("Can't destroy bundle.",
							NULL);
					sdr_cancel_xn(bpSdr);
					running = 0;
					bpEraseCtSignal(&cts);
					break;	/*	Out of switch.	*/
				}
			}
			else	/*	Custody refused; try again.	*/
			{
				noteSnub(bundle, bundleAddr,
						dlv.bundleSourceEid);
				if ((dictionary = retrieveDictionary(bundle))
						== (char *) bundle)
				{
					putErrmsg("Can't retrieve dictionary.",
							NULL);
					sdr_cancel_xn(bpSdr);
					running = 0;
					bpEraseCtSignal(&cts);
					break;	/*	Out of switch.	*/
				}

				if (printEid(&bundle->destination, dictionary,
						&eidString) < 0)
				{
					putErrmsg("Can't print dest EID.",
							NULL);
					sdr_cancel_xn(bpSdr);
					running = 0;
					bpEraseCtSignal(&cts);
					break;	/*	Out of switch.	*/
				}

				result = forwardBundle(bundleAddr, bundle,
						eidString);
				MRELEASE(eidString);
				releaseDictionary(dictionary);
				if (result < 0)
				{
					putErrmsg("Can't re-queue bundle for \
forwarding.", NULL);
					sdr_cancel_xn(bpSdr);
					running = 0;
					bpEraseCtSignal(&cts);
					break;	/*	Out of switch.	*/
				}

				noteStateStats(BPSTATS_REFUSE, &bundleBuf);
				if (bpvdb->watching & WATCH_refusal)
				{
					putchar('&');
					fflush(stdout);
				}
			}

			bpEraseCtSignal(&cts);
			if (sdr_end_xn(bpSdr) < 0)
			{
				putErrmsg("Can't handle custody signal.",
						NULL);
				running = 0;
			}

			break;			/*	Out of switch.	*/

		default:	/*	Unknown admin payload type.	*/
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
