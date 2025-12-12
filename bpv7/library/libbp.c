/*
 *	libbp.c:	functions enabling the implementation of
 *			BP applications.
 *
 *	Copyright (c) 2003, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 *
 *	Modification History:
 *	Date	  Who	What
 *	06-27-03  SCB	Original development.
 *	01-09-06  SCB	Revision per version 4 of Bundle Protocol spec.
 */

#include "bpP.h"
#include "ipnfw.h"

extern void	bpEndpointTally(VEndpoint *vpoint, unsigned int idx,
			unsigned int size);

typedef struct
{
	int		interval;	/*	Seconds.		*/
	sm_SemId	semaphore;
} TimerParms;

int	bp_attach(void)
{
	return bpAttach();
}

int	bp_agent_is_started(void)
{
	BpVdb	*vdb = getBpVdb();

	return (vdb && vdb->clockPid != ERROR);
}

Sdr	bp_get_sdr(void)
{
	return getIonsdr();
}

void	bp_detach(void)
{
#if (!(defined (ION_LWT)))
	bpDetach();
#endif
	ionDetach();
}

static int	createBpSAP(Sdr sdr, char *eidString, BpSAP *bpsapPtr,
			VEndpoint **vpointRef)
{
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	Sap		sap;
	VEndpoint	*vpoint;
	PsmAddress	vpointElt;

	/* Parameter intentionally unused. */
	(void)sdr;

	memset((char *) &sap, 0, sizeof(Sap));

	/*	First validate the endpoint ID.				*/

	if (parseEidString(eidString, &metaEid, &vscheme, &vschemeElt) == 0)
	{
		putErrmsg("Malformed EID.", eidString);
		return -1;
	}

	if (metaEid.nullEndpoint)	/*	No SAP is possible.	*/
	{
		restoreEidString(&metaEid);
		return 0;
	}

	if (vschemeElt == 0)
	{
		putErrmsg("Scheme not known.", metaEid.schemeName);
		restoreEidString(&metaEid);
		return -1;
	}

	findEndpoint(NULL, &metaEid, vscheme, &vpoint, &vpointElt);
	if (vpointElt == 0)
	{
		restoreEidString(&metaEid);
		putErrmsg("Endpoint not known.", eidString);
		return -1;
	}

	/*	Endpoint exists; if SAP will be used for bundle
	 *	reception, make sure endpoint is not already opened
	 *	by some other application.				*/


	if (vpointRef)	/*	SAP will be used for bundle reception.	*/
	{
		if (vpoint->appPid != ERROR)	/*	Not closed.	*/
		{
			if (sm_TaskExists(vpoint->appPid))
			{
				restoreEidString(&metaEid);
				if (vpoint->appPid == sm_TaskIdSelf())
				{
					return 0;
				}

				putErrmsg("Endpoint is already open.",
						itoa(vpoint->appPid));
				return -1;
			}

			/*	Application terminated without closing
			 *	the endpoint, so simply close it now.	*/

			vpoint->appPid = ERROR;
		}

		*vpointRef = sap.vpoint = vpoint;
	}

	/*	Construct the service access point.			*/

	memcpy(&sap.endpointMetaEid, &metaEid, sizeof(MetaEid));
	sap.endpointMetaEid.colon = NULL;
	sap.endpointMetaEid.schemeName = MTAKE(metaEid.schemeNameLength + 1);
	if (sap.endpointMetaEid.schemeName == NULL)
	{
		restoreEidString(&metaEid);
		putErrmsg("Can't create BpSAP.", NULL);
		return -1;
	}

	sap.endpointMetaEid.nss = MTAKE(metaEid.nssLength + 1);
	if (sap.endpointMetaEid.nss == NULL)
	{
		MRELEASE(sap.endpointMetaEid.schemeName);
		restoreEidString(&metaEid);
		putErrmsg("Can't create BpSAP.", NULL);
		return -1;
	}

	*bpsapPtr = MTAKE(sizeof(Sap));
	if (*bpsapPtr == NULL)
	{
		MRELEASE(sap.endpointMetaEid.nss);
		MRELEASE(sap.endpointMetaEid.schemeName);
		restoreEidString(&metaEid);
		putErrmsg("Can't create BpSAP.", NULL);
		return -1;
	}

	istrcpy(sap.endpointMetaEid.schemeName, metaEid.schemeName,
			metaEid.schemeNameLength + 1);
	istrcpy(sap.endpointMetaEid.nss, metaEid.nss,
			metaEid.nssLength + 1);
	memcpy((char *) (*bpsapPtr), (char *) &sap, sizeof(Sap));
	restoreEidString(&metaEid);
	return 0;
}

int	bp_open(char *eidString, BpSAP *bpsapPtr)
{
	Sdr		sdr;
	VEndpoint	*vpoint;
	int		result;

	CHKERR(eidString && *eidString && bpsapPtr);
	*bpsapPtr = NULL;	/*	Default, in case of failure.	*/
	sdr = getIonsdr();
	CHKERR(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	result = createBpSAP(sdr, eidString, bpsapPtr, &vpoint);
	if (*bpsapPtr)
	{
		(*bpsapPtr)->recvSemaphore = vpoint->semaphore;
		(*bpsapPtr)->detain = 0;

		/*	Give owner of this SAP exclusive reception
		 *	access on the endpoint.				*/

		vpoint->appPid = sm_TaskIdSelf();
	}

	sdr_exit_xn(sdr);		/*	Unlock memory.		*/
	return result;
}

int	bp_open_source(char *eidString, BpSAP *bpsapPtr, int detain)
{
	Sdr		sdr;
	int		result;

	CHKERR(eidString && *eidString && bpsapPtr);
	*bpsapPtr = NULL;	/*	Default, in case of failure.	*/
	sdr = getIonsdr();
	CHKERR(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	result = createBpSAP(sdr, eidString, bpsapPtr, NULL);
	if (*bpsapPtr)
	{
		(*bpsapPtr)->recvSemaphore = SM_SEM_NONE;
		(*bpsapPtr)->detain = detain;
	}

	sdr_exit_xn(sdr);		/*	Unlock memory.		*/
	return result;
}

void	bp_close(BpSAP sap)
{
	if (sap == NULL)
	{
		return;
	}

	if (sap->vpoint)	/*	SAP is configured for reception.*/
	{
		if (sap->vpoint->appPid == sm_TaskIdSelf())
		{
			/*	Must detach the endpoint.		*/

			sap->vpoint->appPid = ERROR;
		}
	}

	MRELEASE(sap->endpointMetaEid.nss);
	MRELEASE(sap->endpointMetaEid.schemeName);
	MRELEASE(sap);
}

int	bp_parse_quality_of_service(const char *token,
		BpAncillaryData *ancillaryData, BpCustodySwitch *custodySwitch,
		int *priority)
{
	int	count;
	unsigned int myCustodyRequested;
	unsigned int myPriority;
	unsigned int myOrdinal;
	unsigned int myUnreliable;
	unsigned int myCritical;
	unsigned int myDataLabel = 0;

	count = sscanf(token, "%11u.%11u.%11u.%11u.%11u.%11u",
			&myCustodyRequested, &myPriority, &myOrdinal,
			&myUnreliable, &myCritical, &myDataLabel);
	switch (count)
	{
	case 6:
		/*	All unsigned ints are valid data labels.	*/
		/* FALLTHROUGH */

	case 5:
		if ((myCritical != 0 && myCritical != 1)
		|| (myUnreliable != 0 && myUnreliable != 1))
		{
			return 0;	/*	Invalid format.		*/
		}

		/* FALLTHROUGH */

	case 3:
		if (myOrdinal > 254)
		{
			return 0;	/*	Invalid format.		*/
		}

		/* FALLTHROUGH */

	case 2:
		if (myPriority > 2 || myCustodyRequested > 1)
		{
			return 0;	/*	Invalid format.		*/
		}

		break;

	default:
		return 0;		/*	Invalid format.		*/
	}

	/*	Syntax and bounds-checking passed; assign to outputs.	*/

	ancillaryData->flags = 0;
	ancillaryData->dataLabel = myDataLabel;
	if (count >= 5)
	{
		ancillaryData->flags |= ((myUnreliable ? BP_BEST_EFFORT : 0)
				| (myCritical ? BP_MINIMUM_LATENCY : 0));
	}
	else
	{
		ancillaryData->flags = 0;
	}

	if (count >= 3)
	{
		ancillaryData->ordinal = myOrdinal;
	}
 
	*priority = myPriority;
	*custodySwitch = (myCustodyRequested ? 
			SourceCustodyRequired : NoCustodyRequested);
	return 1;
}

char	*_nullEid(void)
{
	return "dtn:none";
}

int	parseEidString(char *eidString, MetaEid *metaEid, VScheme **vscheme,
		PsmAddress *vschemeElt)
{
	unsigned long	allocatorNbr;
	unsigned long	nodeNbr;
	unsigned long	groupNbr;

	/*	parseEidString is a Boolean function, returning 1 if
	 *	the EID string was successfully parsed.			*/

	CHKZERO(eidString && metaEid && vscheme && vschemeElt);
	memset((char *) metaEid, 0, sizeof(MetaEid));

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

	/*	Make a copy of the EID string to avoid modifying the
	 *	original, which may be in shared memory or may be
	 *	read by other threads/processes concurrently.		*/

	metaEid->eidCopy = strdup(eidString);
	if (metaEid->eidCopy == NULL)
	{
		putErrmsg("No memory for EID copy.", eidString);
		return 0;
	}

	/*	EID string does not identify the special null endpoint.	*/

	metaEid->colon = strchr(metaEid->eidCopy, ':');
	if (metaEid->colon == NULL)
	{
		writeMemoNote("[?] Malformed EID", eidString);
		free(metaEid->eidCopy);
		metaEid->eidCopy = NULL;
		return 0;
	}

	*(metaEid->colon) = '\0';
	metaEid->schemeName = metaEid->eidCopy;
	metaEid->schemeNameLength = metaEid->colon - metaEid->eidCopy;
	metaEid->nss = metaEid->colon + 1;
	metaEid->nssLength = strlen(metaEid->nss);

	/*	Look up scheme of endpoint URI.				*/

	findScheme(metaEid->schemeName, vscheme, vschemeElt);
	if (*vschemeElt == 0)
	{
		writeMemoNote("[?] Unknown scheme for endpoint URI", eidString);
		free(metaEid->eidCopy);
		metaEid->eidCopy = NULL;
		return 0;
	}

	metaEid->schemeCodeNbr = (*vscheme)->codeNumber;
	switch (metaEid->schemeCodeNbr)
	{
	case dtn:
		if (metaEid->nssLength < 3
		|| *(metaEid->nss) != '/'
		|| *(metaEid->nss + 1) != '/')
		{
			writeMemoNote("[?] Malformed URI", eidString);
			free(metaEid->eidCopy);
			metaEid->eidCopy = NULL;
			return 0;
		}

		metaEid->nodeName = metaEid->nss + 2;
		metaEid->delimiter = strchr(metaEid->nodeName, '/');
		if (metaEid->delimiter)
		{
			metaEid->demux = metaEid->delimiter + 1;
		}

		return 1;

	case ipn:
		if (sscanf(metaEid->nss, "%lu.%lu.%lu", &allocatorNbr,
			&nodeNbr, &(metaEid->serviceNbr)) == 3)
		{
			metaEid->elementNbr = ((allocatorNbr << 32)
				& 0xffffffff00000000) +
				(nodeNbr & 0x00000000ffffffff);
		}
		else if (sscanf(metaEid->nss, UVAST_FIELDSPEC ".%lu",
			&(metaEid->elementNbr), &(metaEid->serviceNbr)) != 2)
		{
			writeMemoNote("[?] Malformed URI", eidString);
			free(metaEid->eidCopy);
			metaEid->eidCopy = NULL;
			return 0;
		}

		if (metaEid->elementNbr == 0 && metaEid->serviceNbr == 0)
		{
			metaEid->nullEndpoint = 1;
		}

		return 1;

	case imc:
		if (sscanf(metaEid->nss, "%lu.%lu.%lu", &allocatorNbr,
			&groupNbr, &(metaEid->serviceNbr)) == 3)
		{
			metaEid->elementNbr = ((allocatorNbr << 32)
				& 0xffffffff00000000) +
				(groupNbr & 0x00000000ffffffff);
		}
		else if (sscanf(metaEid->nss, UVAST_FIELDSPEC ".%lu",
			&(metaEid->elementNbr), &(metaEid->serviceNbr)) < 2)
		{
			writeMemoNote("[?] Malformed URI", eidString);
			free(metaEid->eidCopy);
			metaEid->eidCopy = NULL;
			return 0;
		}

		return 1;

	default:
		writeMemoNote("[?] URI for this scheme not parseable",
				metaEid->schemeName);
		free(metaEid->eidCopy);
		metaEid->eidCopy = NULL;
	}

	return 0;
}

void	restoreEidString(MetaEid *metaEid)
{
	if (metaEid)
	{
		if (metaEid->eidCopy)
		{
			free(metaEid->eidCopy);
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
		eid->ssp.ipn.fqnn = meid->elementNbr;
		eid->ssp.ipn.serviceNbr = meid->serviceNbr;
		return 0;

	case imc:
		eid->ssp.imc.fqgn = meid->elementNbr;
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
		eid->ssp.ipn.fqnn = 0;
		eid->ssp.ipn.serviceNbr = 0;
		break;

	case imc:
		eid->ssp.imc.fqgn = 0;
		eid->ssp.imc.serviceNbr = 0;
		break;

	default:
		break;
	}

	eid->schemeCodeNbr = unknown;
}

static void	readDtnEid(DtnSSP *ssp, char **buffer)
{
	EidMode	mode;
	int	nssLength;
	int	eidLength;
	char	*eidString;

	if (ssp->nssLength > 0)
	{
		if (ssp->endpointName.nv == 0)
		{
			writeMemo("[?] dtn-scheme EID endpoint name missing.");
			return;
		}

		mode = EidNV;
		nssLength = eidLength = ssp->nssLength;
	}
	else if (ssp->nssLength < 0)
	{
		if (ssp->endpointName.v == 0)
		{
			writeMemo("[?] dtn-scheme EID endpoint name missing.");
			return;
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
				return;
			}

			istrcpy(eidString, _nullEid(), eidLength);
			*buffer = eidString;
			return;
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
		return;
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
}

static void	readIpnEid(IpnSSP *ssp, char **buffer)
{
	char	*eidString;
	int	eidLength = MAX_EID_LEN;
	char	nbrBuf[FQN_MAX_LENGTH];

	/*	Printed EID string is
	 *
	 *	   ipn:[allocatornbr.]<nodenbr>.<servicenbr>\0
	 *
	 *	So max EID string length is 3 for "ipn" plus 1 for
	 *	':' plus max length of allocatornbr (which is a
	 *	32-bit number, so 10 digits) plus 1 for '.' plus max
	 *	length of nodenbr (which is a 32-bit number, so 10
	 *	digits) plus 1 for '.' plus max length of servicenbr
	 *	(which is a 64-bit number, so 20 digits) plus 1 for
	 *	the terminating NULL.					*/

	eidString = MTAKE(eidLength);
	if (eidString == NULL)
	{
		putErrmsg("Can't create EID string.", NULL);
		return;
	}

	if (ssp->fqnn == 0 && ssp->serviceNbr == 0)
	{
		istrcpy(eidString, _nullEid(), eidLength);
	}
	else
	{
		putFqn(nbrBuf, ssp->fqnn);
		isprintf(eidString, eidLength, "ipn:%s.%lu", nbrBuf,
				ssp->serviceNbr);
	}

	*buffer = eidString;
}

static void	readImcEid(ImcSSP *ssp, char **buffer)
{
	char		*eidString;
	int		eidLength = MAX_EID_LEN;
	char		nbrBuf[FQN_MAX_LENGTH];

	/*	Printed EID string is
	 *
	 *	   imc:[allocatornbr.]<groupnbr>.<servicenbr>\0
	 *
	 *	So max EID string length is 3 for "imc" plus 1 for
	 *	':' plus max length of allocatornbr (which is a
	 *	32-bit number, so 10 digits) plus 1 for '.' plus max
	 *	length of groupnbr (which is a 32-bit number, so 10
	 *	digits) plus 1 for '.' plus max length of servicenbr
	 *	(which is a 64-bit number, so 20 digits) plus 1 for
	 *	the terminating NULL.					*/

	eidString = MTAKE(eidLength);
	if (eidString == NULL)
	{
		putErrmsg("Can't create EID string.", NULL);
		return;
	}

	putFqn(nbrBuf, ssp->fqgn);
	isprintf(eidString, eidLength, "imc:%s.%lu", nbrBuf,
			ssp->serviceNbr);
	*buffer = eidString;
}

void	readEid(EndpointId *eid, char **buffer)
{
	CHKVOID(eid && buffer);
	*buffer = NULL;			/*	Default.		*/
	switch(eid->schemeCodeNbr)
	{
	case dtn:
		readDtnEid(&(eid->ssp.dtn), buffer);
		break;

	case ipn:
		readIpnEid(&(eid->ssp.ipn), buffer);
		break;

	case imc:
		readImcEid(&(eid->ssp.imc), buffer);
		break;

	default:
		break;
	}
}

int	bp_send(BpSAP sap, char *destEid, char *reportToEid, int lifespan,
		int classOfService, BpCustodySwitch custodySwitch,
		unsigned char srrFlags, int ackRequested,
		BpAncillaryData *ancillaryData, Object adu, Object *bundleObj)
{
	BpAncillaryData	defaultAncillaryData = {0};
	MetaEid		*sourceMetaEid;

	if (adu == 0)
	{
		writeMemo("[?] No application data unit to send.");
		return 0;
	}

	if (ancillaryData == NULL)
	{
		ancillaryData = &defaultAncillaryData;
	}
	else
	{
		if (ancillaryData->ordinal == 255)	/*	Reserve	*/
		{
			ancillaryData->ordinal = 254;
		}
	}

	if (sap)
	{
		sourceMetaEid = &(sap->endpointMetaEid);
		if (sap->detain)
		{
			if (bundleObj == NULL)
			{
				writeMemo("[?] Can't return bundle address.");
				return 0;
			}
		}
		else
		{
			if (bundleObj)
			{
				*bundleObj = 0;
				bundleObj = NULL;
			}
		}
	}
	else					/*	Anonymous.	*/
	{
		sourceMetaEid = NULL;
		if (bundleObj)
		{
			*bundleObj = 0;
			bundleObj = NULL;
		}
	}

	/* PICS-16: Enforce node number constraint at the point of transmission. */
	/* If a source EID is provided, its node number must match the local node. */
	
	if (sourceMetaEid)	/*	Only check non-anonymous sources. */
	{
		uvast localNodeNbr = getOwnFqnn();

		if (sourceMetaEid->elementNbr != 0 &&
				sourceMetaEid->elementNbr != localNodeNbr)
		{
			char	errorMsg[512];

			isprintf(errorMsg, sizeof(errorMsg),
				"[?] Bundle source EID's node number (" UVAST_FIELDSPEC \
				") is not owned by this node (" UVAST_FIELDSPEC ").",
				(uvast) sourceMetaEid->elementNbr,
				(uvast) localNodeNbr);
			writeMemo(errorMsg);

			/* Abort the send and return an error to the application. */
			return -1;
		}
	}
	
	/*	Note: lifespan must be converted from seconds to
	 *	millisecnods for BP processing.				*/

	return bpSend(sourceMetaEid, destEid, reportToEid,
			(uvast) lifespan * 1000, classOfService,
			custodySwitch, srrFlags, ackRequested,
			ancillaryData, adu, bundleObj, 0);
}

int	bp_track(Object bundleObj, Object trackingElt)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(Bundle, bundle);

	CHKERR(bundleObj && trackingElt);
	CHKERR(sdr_begin_xn(sdr));
	GET_OBJ_POINTER(sdr, Bundle, bundle, bundleObj);
	if (bundle->trackingElts == 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("Corrupt bundle?  Has no trackingElts list.", NULL);
		return -1;
	}

	sdr_list_insert_last(sdr, bundle->trackingElts, trackingElt);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Failed adding bundle tracking elt.", NULL);
		return -1;
	}

	return 0;
}

void	bp_untrack(Object bundleObj, Object trackingElt)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(Bundle, bundle);
	Object	elt;

	CHKVOID(bundleObj && trackingElt);
	CHKVOID(sdr_begin_xn(sdr));
	GET_OBJ_POINTER(sdr, Bundle, bundle, bundleObj);
	if (bundle->trackingElts == 0)
	{
		sdr_exit_xn(sdr);
		return;
	}

	for (elt = sdr_list_first(sdr, bundle->trackingElts); elt;
			elt = sdr_list_next(sdr, elt))
	{
		if (sdr_list_data(sdr, elt) == trackingElt)
		{
			break;
		}
	}

	if (elt == 0)		/*	Not found.			*/
	{
		sdr_exit_xn(sdr);
		return;
	}

	sdr_list_delete(sdr, elt, NULL, NULL);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Failed removing bundle tracking elt.", NULL);
	}
}

int	bp_memo(Object bundleObj, unsigned int interval)
{
	/* Parameter intentionally unused. */
	(void)bundleObj;
	(void)interval;

	return 0;
}

int	bp_suspend(Object bundleObj)
{
	Sdr		sdr = getIonsdr();
	Bundle		bundle;
	Object		queue;
	Object		planObj;
	BpPlan		plan;
#if BPDEBUG
	char		*eidString;
#endif

	CHKERR(bundleObj);
	CHKERR(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &bundle, bundleObj, sizeof(Bundle));

#if BPDEBUG
	readEid(&bundle.id.source, &eidString);
	writeMemoNote("[i] Attempting to suspend bundle", eidString);
	MRELEASE(eidString);
#endif

	if (bundle.ancillaryData.flags & BP_MINIMUM_LATENCY)
	{
		writeMemo("[?] Attempt to suspend a 'critical' bundle.");
		sdr_exit_xn(sdr);	/*	Nothing to do.		*/
		return 0;
	}

	if (bundle.suspended == 1)	/*	Already suspended.	*/
	{
#if BPDEBUG
		writeMemo("[i] Bundle already suspended.");
#endif
		sdr_exit_xn(sdr);	/*	Nothing to do.		*/
		return 0;
	}

	queue = sdr_list_list(sdr, bundle.planXmitElt);
	planObj = sdr_list_user_data(sdr, queue);
	if (planObj == 0)
	{
		/*	Object is already in limbo for other reasons.
		 *	Just record the suspension flag.		*/

#if BPDEBUG
		writeMemo("[i] Bundle already in limbo, setting suspended flag.");
#endif
		bundle.suspended = 1;
		sdr_write(sdr, bundleObj, (char *) &bundle, sizeof(Bundle));
	}
	else
	{
		/*	Must reverse the enqueuing of this bundle
		 *	and place it in limbo.				*/

#if BPDEBUG
		writeMemo("[i] Moving bundle to limbo via reverseEnqueue.");
#endif
		sdr_stage(sdr, (char *) &plan, planObj, sizeof(BpPlan));
		if (reverseEnqueue(bundle.planXmitElt, &plan, 1))
		{
			putErrmsg("Can't reverse bundle enqueue.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}

		sdr_write(sdr, planObj, (char *) &plan, sizeof(BpPlan));

		/*	reverseEnqueue updated the bundle in SDR (moved
		 *	it to limbo and updated planXmitElt). Re-stage
		 *	to get those changes, then set suspended flag.	*/

#if BPDEBUG
		writeMemo("[i] Re-staging bundle to set suspended flag.");
#endif
		sdr_stage(sdr, (char *) &bundle, bundleObj, sizeof(Bundle));

#if BPDEBUG
		{
			char msg[256];
			snprintf(msg, sizeof(msg),
				"[i] After re-stage: planXmitElt=%lu, suspended=%d",
				(unsigned long)bundle.planXmitElt, bundle.suspended);
			writeMemo(msg);
		}
#endif

		bundle.suspended = 1;
		sdr_write(sdr, bundleObj, (char *) &bundle, sizeof(Bundle));

#if BPDEBUG
		{
			char msg[256];
			snprintf(msg, sizeof(msg),
				"[i] After write: planXmitElt=%lu, suspended=%d",
				(unsigned long)bundle.planXmitElt, bundle.suspended);
			writeMemo(msg);
		}

		writeMemo("[i] Bundle suspended and moved to limbo.");
#endif
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Failure in bundle suspension.", NULL);
		return -1;
	}

#if BPDEBUG
	writeMemo("[i] Bundle suspension completed successfully.");
#endif
	return 0;
}

int	bp_resume(Object bundleObj)
{
	Sdr	sdr = getIonsdr();
	Bundle	bundle;
#if BPDEBUG
	char	*eidString;
#endif

	CHKERR(bundleObj);
	CHKERR(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &bundle, bundleObj, sizeof(Bundle));

#if BPDEBUG
	readEid(&bundle.id.source, &eidString);
	writeMemoNote("[i] Attempting to resume bundle", eidString);
	MRELEASE(eidString);

	{
		char msg[256];
		snprintf(msg, sizeof(msg),
			"[i] Bundle read: planXmitElt=%lu, suspended=%d",
			(unsigned long)bundle.planXmitElt, bundle.suspended);
		writeMemo(msg);
	}
#endif

	if (bundle.suspended == 0)
	{
#if BPDEBUG
		writeMemo("[i] Bundle not suspended, nothing to do.");
#endif
		sdr_exit_xn(sdr);	/*	Nothing to do.		*/
		return 0;
	}

#if BPDEBUG
	writeMemo("[i] Releasing bundle from limbo.");
#endif
	if (releaseFromLimbo(bundle.planXmitElt, 1) < 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't resume transmission of bundle.", NULL);
		return -1;
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Failure in bundle resumption.", NULL);
		return -1;
	}

#if BPDEBUG
	writeMemo("[i] Bundle resumed successfully.");
#endif
	return 0;
}

int	bp_cancel(Object bundleObj)
{
	Sdr	sdr = getIonsdr();

	CHKERR(bundleObj);
	CHKERR(sdr_begin_xn(sdr));
	if (bpDestroyBundle(bundleObj, 1) < 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't cancel bundle.", NULL);
		return -1;
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Failure in bundle cancellation.", NULL);
		return -1;
	}

	return 0;
}

int	bp_release(Object bundleObj)
{
	Sdr	sdr = getIonsdr();
	Bundle	bundle;

	CHKERR(bundleObj);
	CHKERR(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &bundle, bundleObj, sizeof(Bundle));
	bundle.detained = 0;
	sdr_write(sdr, bundleObj, (char *) &bundle, sizeof(Bundle));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Failure in bundle release.", NULL);
		return -1;
	}

	return 0;
}

static void	*timerMain(void *parm)
{
	TimerParms	*timer = (TimerParms *) parm;
	pthread_mutex_t	mutex;
	pthread_cond_t	cv;
	struct timeval	workTime;
	struct timespec	deadline;
	int		result;

	memset((char *) &mutex, 0, sizeof mutex);
	if (pthread_mutex_init(&mutex, NULL))
	{
		putSysErrmsg("can't start timer, mutex init failed", NULL);
		sm_SemGive(timer->semaphore);
		return NULL;
	}

	memset((char *) &cv, 0, sizeof cv);
	if (pthread_cond_init(&cv, NULL))
	{
		putSysErrmsg("can't start timer, cond init failed", NULL);
		sm_SemGive(timer->semaphore);
		return NULL;
	}

	getCurrentTime(&workTime);
	deadline.tv_sec = workTime.tv_sec + timer->interval;
	deadline.tv_nsec = workTime.tv_usec * 1000;
	pthread_mutex_lock(&mutex);
	result = pthread_cond_timedwait(&cv, &mutex, &deadline);
	pthread_mutex_unlock(&mutex);
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cv);
	if (result)
	{
		errno = result;
		if (errno != ETIMEDOUT)
		{
			putSysErrmsg("timer failure", NULL);
			sm_SemGive(timer->semaphore);
			return NULL;
		}
	}

	/*	Timed out; must wake up the main thread.		*/

	timer->interval = 0;	/*	Indicate genuine timeout.	*/
	sm_SemGive(timer->semaphore);
	return NULL;
}

int	bp_receive(BpSAP sap, BpDelivery *dlvBuffer, int timeoutSeconds)
{
	Sdr		sdr = getIonsdr();
	VEndpoint	*vpoint;
			OBJ_POINTER(Endpoint, endpoint);
	Object		dlvElt;
	Object		bundleAddr;
	Bundle		bundle;
	TimerParms	timerParms;
	pthread_t	timerThread;
	int		result;

	CHKERR(sap && dlvBuffer);
	if (timeoutSeconds < BP_BLOCKING)
	{
		putErrmsg("Illegal timeout interval.", itoa(timeoutSeconds));
		return -1;
	}

	vpoint = sap->vpoint;
	CHKERR(sdr_begin_xn(sdr));
	if (vpoint->appPid != sm_TaskIdSelf())
	{
		sdr_exit_xn(sdr);
		putErrmsg("Can't receive: not owner of endpoint.",
				itoa(vpoint->appPid));
		return -1;
	}

	if (sm_SemEnded(vpoint->semaphore))
	{
		sdr_exit_xn(sdr);
		writeMemo("[?] Endpoint has been stopped.");
		dlvBuffer->result = BpEndpointStopped;

		/*	End task, but without error.			*/

		return 0;
	}

	/*	Get oldest bundle in delivery queue, if any; wait
	 *	for one if necessary.					*/

	GET_OBJ_POINTER(sdr, Endpoint, endpoint, sdr_list_data(sdr,
			vpoint->endpointElt));
	dlvElt = sdr_list_first(sdr, endpoint->deliveryQueue);
	if (dlvElt == 0)
	{
		sdr_exit_xn(sdr);
		if (timeoutSeconds == BP_POLL)
		{
			dlvBuffer->result = BpReceptionTimedOut;
			return 0;
		}

		/*	Wait for semaphore to be given, either by the
		 *	deliverBundle() function or by timer thread.	*/

		if (timeoutSeconds == BP_BLOCKING)
		{
			timerParms.interval = -1;
		}
		else	/*	This is a receive() with a deadline.	*/
		{
			timerParms.interval = timeoutSeconds;
			timerParms.semaphore = vpoint->semaphore;
			if (pthread_begin(&timerThread, NULL, timerMain,
					&timerParms, "bprcvTimer") < 0)
			{
				putSysErrmsg("Can't enable interval timer",
						NULL);
				return -1;
			}
		}

		/*	Take endpoint semaphore.			*/

		if (sm_SemTake(vpoint->semaphore) < 0)
		{
			putErrmsg("Can't take endpoint semaphore.", NULL);
			return -1;
		}

		if (sm_SemEnded(vpoint->semaphore))
		{
			writeMemo("[i] Endpoint has been stopped.");
			dlvBuffer->result = BpEndpointStopped;

			/*	End task, but without error.		*/

			return 0;
		}

		/*	Have taken the semaphore, one way or another.	*/

		CHKERR(sdr_begin_xn(sdr));
		dlvElt = sdr_list_first(sdr, endpoint->deliveryQueue);
		if (dlvElt == 0)	/*	Still nothing.		*/
		{
			/*	Either sm_SemTake() was interrupted
			 *	or else timer thread gave semaphore.	*/

			sdr_exit_xn(sdr);
			if (timerParms.interval == 0)
			{
				/*	Timer expired.			*/

				dlvBuffer->result = BpReceptionTimedOut;
				pthread_join(timerThread, NULL);
			}
			else	/*	Interrupted.			*/
			{
				dlvBuffer->result = BpReceptionInterrupted;
				if (timerParms.interval != -1)
				{
					pthread_end(timerThread);
					pthread_join(timerThread, NULL);
				}
			}

			return 0;
		}
		else		/*	Bundle was delivered.		*/
		{
			if (timerParms.interval != -1)
			{
				pthread_end(timerThread);
				pthread_join(timerThread, NULL);
			}
		}
	}

	/*	At this point, we have got a dlvElt and are in an SDR
	 *	transaction.						*/

	bundleAddr = sdr_list_data(sdr, dlvElt);
	sdr_stage(sdr, (char *) &bundle, bundleAddr, sizeof(Bundle));

	/*	Now fill in the data indication structure.		*/

	dlvBuffer->result = BpPayloadPresent;
	readEid(&bundle.id.source, &dlvBuffer->bundleSourceEid);
	if (dlvBuffer->bundleSourceEid == NULL)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't get source EID.", NULL);
		return -1;
	}

	dlvBuffer->bundleCreationTime.msec = bundle.id.creationTime.msec;
	dlvBuffer->bundleCreationTime.count = bundle.id.creationTime.count;
	dlvBuffer->timeToLive = bundle.timeToLive;
	dlvBuffer->adminRecord = bundle.bundleProcFlags & BDL_IS_ADMIN;
	dlvBuffer->adu = bundle.payload.content;
	dlvBuffer->ackRequested = bundle.bundleProcFlags & BDL_APP_ACK_REQUEST;

	dlvBuffer->metadataType = bundle.ancillaryData.metadataType;
	dlvBuffer->metadataLen = bundle.ancillaryData.metadataLen;
	memcpy(dlvBuffer->metadata, bundle.ancillaryData.metadata,
			BP_MAX_METADATA_LEN);

	/*	Now before returning we send delivery status report
	 *	if it is requested.					*/

	if (SRR_FLAGS(bundle.bundleProcFlags) & BP_DELIVERED_RPT)
	{
		bundle.statusRpt.flags |= BP_DELIVERED_RPT;
		if (bundle.bundleProcFlags & BDL_STATUS_TIME_REQ)
		{
			getCurrentDtnTime(&bundle.statusRpt.deliveryTime);
		}
	}

	if (bundle.statusRpt.flags)
	{
		result = sendStatusRpt(&bundle);
		if (result < 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("Can't send status report.", NULL);
			return -1;
		}
	}

	/*	Finally delete the delivery list element and destroy
	 *	the bundle itself.					*/

	sdr_list_delete(sdr, dlvElt, (SdrListDeleteFn) NULL, NULL);
	bundle.dlvQueueElt = 0;
	bundle.payload.content = 0;
	sdr_write(sdr, bundleAddr, (char *) &bundle, sizeof(Bundle));
	bpEndpointTally(vpoint, BP_ENDPOINT_DELIVERED, bundle.payload.length);
	if (bpDestroyBundle(bundleAddr, 0) < 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't destroy bundle.", NULL);
		return -1;
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Failure in bundle reception.", NULL);
		return -1;
	}

	return 0;
}

void	bp_interrupt(BpSAP sap)
{
	/*	Give semaphore, simulating reception notice.		*/

	if (sap != NULL && sap->recvSemaphore != SM_SEM_NONE)
	{
		sm_SemGive(sap->recvSemaphore);
	}
}

void	bp_release_delivery(BpDelivery *dlvBuffer, int releasePayload)
{
	Sdr	sdr = getIonsdr();

	CHKVOID(dlvBuffer);
	if (dlvBuffer->result == BpPayloadPresent)
	{
		if (dlvBuffer->bundleSourceEid)
		{
			MRELEASE(dlvBuffer->bundleSourceEid);
			dlvBuffer->bundleSourceEid = NULL;
		}

		if (releasePayload)
		{
			if (dlvBuffer->adu)
			{
				CHKVOID(sdr_begin_xn(sdr));
				zco_destroy(sdr, dlvBuffer->adu);
				if (sdr_end_xn(sdr) < 0)
				{
					putErrmsg("Failed releasing delivery.",
							NULL);
				}

				dlvBuffer->adu = 0;
			}
		}
	}
}

/********************************************
 * BP Management APIs (initial)
 * 
 * Listing APIs will be updated to return data in 
 * structure structure.   
 **********************************************/

int	bp_init(void)
{
	return bpInit();
}

int	bp_start(void)
{
	return bpStart();
}


void bp_stop(void)
{
	bpStop();
}

int ipn_init(void)
{
	return ipnInit();
}

int	add_scheme(char *name, char *fwdCmd, char *admAppCmd)
{
	return addScheme(name, fwdCmd, admAppCmd);
}

int	remove_scheme(char *name)
{
	return removeScheme(name);
}

int	bp_start_scheme(char *name)
{
	return bpStartScheme(name);
}

void bp_stop_scheme(char *name)
{
	bpStopScheme(name);
}

int	add_endpoint(char *eid, BpRecvRule recvRule, char *script)
{
	return addEndpoint(eid, recvRule, script);
}

int	remove_endpoint(char *eid)
{
	return removeEndpoint(eid);
}

/* Protocol management wrappers */

int add_protocol(char *protocol_name, int protocol_class)
{
	return addProtocol(protocol_name, protocol_class);
}

int remove_protocol(char *protocol_name)
{
	return removeProtocol(protocol_name);
}

int bp_start_protocol(char *protocol_name)
{
	return bpStartProtocol(protocol_name);
}

void bp_stop_protocol(char *protocol_name)
{
	bpStopProtocol(protocol_name);
}

/* Induct management wrappers */

int add_induct(char *protocol_name, char *duct_name, char *cli_command)
{
	return addInduct(protocol_name, duct_name, cli_command);
}

int remove_induct(char *protocol_name, char *duct_name)
{
	return removeInduct(protocol_name, duct_name);
}

int bp_start_induct(char *protocol_name, char *duct_name)
{
	return bpStartInduct(protocol_name, duct_name);
}

void bp_stop_induct(char *protocol_name, char *duct_name)
{
	bpStopInduct(protocol_name, duct_name);
}

/* Outduct management wrappers */

int add_outduct(char *protocol_name, char *duct_name,
                char *clo_command, unsigned int max_payload_length)
{
	return addOutduct(protocol_name, duct_name, clo_command,
	                  max_payload_length);
}

int remove_outduct(char *protocol_name, char *duct_name)
{
	return removeOutduct(protocol_name, duct_name);
}

int bp_start_outduct(char *protocol_name, char *duct_name)
{
	return bpStartOutduct(protocol_name, duct_name);
}

void bp_stop_outduct(char *protocol_name, char *duct_name)
{
	bpStopOutduct(protocol_name, duct_name);
}

/* Egress plan management wrappers */

int add_plan(char *eid, unsigned int nominal_rate)
{
	return addPlan(eid, nominal_rate);
}

int remove_plan(char *eid)
{
	return removePlan(eid);
}

int bp_start_plan(char *eid)
{
	return bpStartPlan(eid);
}

void bp_stop_plan(char *eid)
{
	bpStopPlan(eid);
}

/* Planduct (plan-outduct attachment) management wrappers */

int add_planduct(char *eid, char *protocol_name, char *duct_name)
{
	VOutduct	*vduct;
	PsmAddress	vductElt;

	CHKERR(eid && protocol_name && duct_name);

	/* Find the outduct by protocol and duct name */
	findOutduct(protocol_name, duct_name, &vduct, &vductElt);
	if (vductElt == 0)
	{
		writeMemoNote("[?] Unknown outduct, can't attach to plan",
		              duct_name);
		return 0;
	}

	/* Attach the outduct to the plan using its element pointer */
	return attachPlanDuct(eid, vduct->outductElt);
}

int remove_planduct(char *protocol_name, char *duct_name)
{
	VOutduct	*vduct;
	PsmAddress	vductElt;

	CHKERR(protocol_name && duct_name);

	/* Find the outduct by protocol and duct name */
	findOutduct(protocol_name, duct_name, &vduct, &vductElt);
	if (vductElt == 0)
	{
		writeMemoNote("[?] Unknown outduct, can't detach from plan",
		              duct_name);
		return 0;
	}

	/* Detach the outduct from its plan using its element pointer */
	return detachPlanDuct(vduct->outductElt);
}

void report_all_state_stats(void)
{
	reportAllStateStats();
}

void bp_list_schemes(void)
{
    Sdr          sdr;
    PsmPartition bpwm;
    BpVdb        *vdb;
    PsmAddress   elt;
    VScheme      *vscheme;
    
    sdr = getIonsdr();
    bpwm = getIonwm();
    vdb = getBpVdb();
    
    if (sdr_begin_xn(sdr) < 0)
    {
        writeErrMemo("bpListSchemes: Cannot begin transaction\n");
    }
    
    // Iterate through all schemes
    for (elt = sm_list_first(bpwm, vdb->schemes); elt;
         elt = sm_list_next(bpwm, elt))
    {
        vscheme = (VScheme *) psp(bpwm, sm_list_data(bpwm, elt));
        
        // Access scheme information
        PUTMEMO("Scheme", vscheme->name);
        PUTMEMO("  Code Number", itoa(vscheme->codeNumber));
        PUTMEMO("  Admin EID", vscheme->adminEid);
        PUTMEMO("  Forwarder PID", itoa(vscheme->fwdPid));
        PUTMEMO("  Admin App PID", itoa(vscheme->admAppPid));
    }
    
	/* TO DO: Can add return structure in the future */

    sdr_exit_xn(sdr);

}

void bp_list_endpoints(void)
{
    Sdr          sdr = getIonsdr();
    PsmPartition bpwm = getIonwm();
    BpVdb        *vdb = getBpVdb();
    PsmAddress   schemeElt;
    PsmAddress   endpointElt;
    VScheme      *vscheme;
    VEndpoint    *vpoint;
    Object       endpointObj;
    Endpoint     endpoint;
    Scheme       schemeBuf;
    char         buffer[2048];
    char         recvScriptBuf[SDRSTRING_BUFSZ];
    char         *recvScript;
    char         recvRuleChar;
    int          endpointCount = 0;

    CHKVOID(vdb);
    CHKVOID(sdr_begin_xn(sdr));

    // Iterate through all schemes
    for (schemeElt = sm_list_first(bpwm, vdb->schemes); schemeElt;
         schemeElt = sm_list_next(bpwm, schemeElt))
    {
        vscheme = (VScheme *) psp(bpwm, sm_list_data(bpwm, schemeElt));
        
        // Read scheme details
        sdr_read(sdr, (char *)&schemeBuf, 
                 sdr_list_data(sdr, vscheme->schemeElt), 
                 sizeof(Scheme));

        // Iterate through endpoints for this scheme
        for (endpointElt = sm_list_first(bpwm, vscheme->endpoints); 
             endpointElt;
             endpointElt = sm_list_next(bpwm, endpointElt))
        {
            vpoint = (VEndpoint *) psp(bpwm, sm_list_data(bpwm, endpointElt));
            
            // Read endpoint details from SDR
            endpointObj = sdr_list_data(sdr, vpoint->endpointElt);
            sdr_read(sdr, (char *)&endpoint, endpointObj, sizeof(Endpoint));

            // Format receive rule as character
            recvRuleChar = (endpoint.recvRule == EnqueueBundle) ? 'q' : 'x';

            // Read receive script if present
            if (endpoint.recvScript == 0)
            {
                recvScript = "";
            }
            else
            {
                if (sdr_string_read(sdr, recvScriptBuf, endpoint.recvScript) < 0)
                {
                    recvScript = "?";
                }
                else
                {
                    recvScript = recvScriptBuf;
                }
            }

            // Format and print endpoint information
            isprintf(buffer, sizeof(buffer),
                     "%s:%s  rule=%c  app_pid=%d  script='%s'",
                     vscheme->name,
                     vpoint->nss,
                     recvRuleChar,
                     vpoint->appPid,
                     recvScript);
            PUTS(buffer);

            endpointCount++;
        }
    }

    sdr_exit_xn(sdr);

    // Print summary
    if (endpointCount == 0)
    {
        PUTS("No endpoints registered.");
    }
    else
    {
        isprintf(buffer, sizeof(buffer), 
                 "Total endpoints: %d", endpointCount);
        PUTS(buffer);
    }
}

void bp_list_protocols(void)
{
    Sdr        sdr = getIonsdr();
    BpDB       *bpConstants = getBpConstants();
    Object     elt;
    Object     protocolObj;
    ClProtocol protocol;
    char       buffer[2048];
    int        protocolCount = 0;
    char       *className;

    CHKVOID(bpConstants);
    CHKVOID(sdr_begin_xn(sdr));

    // Iterate through all protocols
    for (elt = sdr_list_first(sdr, bpConstants->protocols); elt;
         elt = sdr_list_next(sdr, elt))
    {
        protocolObj = sdr_list_data(sdr, elt);
        sdr_read(sdr, (char *)&protocol, protocolObj, sizeof(ClProtocol));

        // Determine protocol class name
        switch (protocol.protocolClass)
        {
        case 0:
            className = "Scheduled";
            break;
        case 1:
            className = "Unscheduled";
            break;
        case 2:
            className = "OnDemand";
            break;
        default:
            className = "Unknown";
        }

        // Format and print protocol information
        isprintf(buffer, sizeof(buffer),
                 "Protocol: %-12s  class=%d (%s)",
                 protocol.name,
                 protocol.protocolClass,
                 className);
        PUTS(buffer);

        protocolCount++;
    }

    sdr_exit_xn(sdr);

    // Print summary
    if (protocolCount == 0)
    {
        PUTS("No convergence layer protocols configured.");
    }
    else
    {
        isprintf(buffer, sizeof(buffer), 
                 "Total protocols: %d", protocolCount);
        PUTS(buffer);
    }
}
