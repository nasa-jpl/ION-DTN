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
		clearMetaEid(&metaEid);
		return 0;
	}

	if (vschemeElt == 0)
	{
		putErrmsg("Scheme not known.", metaEid.schemeName);
		clearMetaEid(&metaEid);
		return -1;
	}

	findEndpoint(NULL, &metaEid, vscheme, &vpoint, &vpointElt);
	if (vpointElt == 0)
	{
		clearMetaEid(&metaEid);
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
				/*	The recorded owner PID exists.
				 *	Two sub-cases.  Compare the per-
				 *	process-instance cookie to tell
				 *	them apart.			*/

				if (vpoint->appPid == sm_TaskIdSelf()
				&& vpoint->appCookie == sm_ProcessCookie())
				{
					/*	Same process instance:
					 *	another thread in THIS
					 *	process already owns the
					 *	endpoint.  A reception
					 *	endpoint is single-owner,
					 *	so refuse.		*/

					clearMetaEid(&metaEid);
					putErrmsg("Endpoint is already \
open.", itoa(vpoint->appPid));
					return -1;
				}

				if (vpoint->appPid != sm_TaskIdSelf())
				{
					/*	A different live process
					 *	owns the endpoint.  Refuse.
					 *	(If PID matched but cookie
					 *	did not, fall through to
					 *	the recycle-reclaim path
					 *	below: the recorded PID
					 *	now belongs to an unrelated
					 *	process, so the original
					 *	owner is dead.)		*/

					clearMetaEid(&metaEid);
					putErrmsg("Endpoint is already \
open.", itoa(vpoint->appPid));
					return -1;
				}
			}

			/*	Either the original owning process is
			 *	gone (PID no longer exists), or this is
			 *	our own PID after recycling but with a
			 *	new process-instance cookie.  In both
			 *	cases the endpoint is effectively closed;
			 *	reclaim it.				*/

			vpoint->appPid = ERROR;
			vpoint->appCookie = 0;
		}

		*vpointRef = sap.vpoint = vpoint;
	}

	/*	Construct the service access point.			*/

	memcpy(&sap.endpointMetaEid, &metaEid, sizeof(MetaEid));
	sap.endpointMetaEid.colon = NULL;
	sap.endpointMetaEid.schemeName = MTAKE(metaEid.schemeNameLength + 1);
	if (sap.endpointMetaEid.schemeName == NULL)
	{
		clearMetaEid(&metaEid);
		putErrmsg("Can't create BpSAP.", NULL);
		return -1;
	}

	sap.endpointMetaEid.nss = MTAKE(metaEid.nssLength + 1);
	if (sap.endpointMetaEid.nss == NULL)
	{
		MRELEASE(sap.endpointMetaEid.schemeName);
		clearMetaEid(&metaEid);
		putErrmsg("Can't create BpSAP.", NULL);
		return -1;
	}

	*bpsapPtr = MTAKE(sizeof(Sap));
	if (*bpsapPtr == NULL)
	{
		MRELEASE(sap.endpointMetaEid.nss);
		MRELEASE(sap.endpointMetaEid.schemeName);
		clearMetaEid(&metaEid);
		putErrmsg("Can't create BpSAP.", NULL);
		return -1;
	}

	istrcpy(sap.endpointMetaEid.schemeName, metaEid.schemeName,
			metaEid.schemeNameLength + 1);
	istrcpy(sap.endpointMetaEid.nss, metaEid.nss,
			metaEid.nssLength + 1);
	memcpy((char *) (*bpsapPtr), (char *) &sap, sizeof(Sap));
	clearMetaEid(&metaEid);
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
		vpoint->appCookie = sm_ProcessCookie();
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
			sap->vpoint->appCookie = 0;
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
	int	     count;
	unsigned int myCustodyRequested = 0;
	unsigned int myPriority = 1;
	unsigned int myOrdinal = 0;
	unsigned int myUnreliable = 0;
	unsigned int myCritical = 0;
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
		if (myCritical != 0 && myCritical != 1)
		{
			return 0; /* Invalid value. */
		}

		/* FALLTHROUGH */

	case 4:
		if (myUnreliable != 0 && myUnreliable != 1)
		{
			return 0; /* Invalid value. */
		}

		/* FALLTHROUGH */

	case 3:
		if (myOrdinal > 254)
		{
			return 0; /* Invalid value. */
		}

		/* FALLTHROUGH */

	case 2:
		if (myPriority > 2)
		{
			return 0; /* Invalid value. */
		}

		/* FALLTHROUGH */

	case 1:
		if (myCustodyRequested > 1)
		{
			return 0; /* Invalid value. */
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
		ancillaryData->flags |= (myCritical ? BP_MINIMUM_LATENCY : 0);
	}

	if (count >= 4)
	{
		ancillaryData->flags |= (myUnreliable ? BP_BEST_EFFORT : 0);
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

/*	*	*	Operations on endpoint IDs	*	*	*/

int	parseEidString(char *eidString, MetaEid *metaEid, VScheme **vscheme,
		PsmAddress *vschemeElt)
{
	unsigned long	allocatorNbr;
	unsigned long	nodeNbr;

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

	int eidLen = strlen(eidString) + 1;
	metaEid->eidCopy = MTAKE(eidLen);
	if (metaEid->eidCopy == NULL)
	{
		putErrmsg("No memory for EID copy.", eidString);
		return 0;
	}
	istrcpy(metaEid->eidCopy, eidString, eidLen);

	/*	EID string does not identify the special null endpoint.	*/

	metaEid->colon = strchr(metaEid->eidCopy, ':');
	if (metaEid->colon == NULL)
	{
		writeMemoNote("[?] Malformed EID", eidString);
		clearMetaEid(metaEid);
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
		clearMetaEid(metaEid);
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
			clearMetaEid(metaEid);
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
			metaEid->elementNbr = (((uvast) allocatorNbr << 32)
				& 0xffffffff00000000) +
				(nodeNbr & 0x00000000ffffffff);
			metaEid->eidFormat = EidFormat3Element;
		}
		else if (sscanf(metaEid->nss, UVAST_FIELDSPEC ".%lu",
			&(metaEid->elementNbr), &(metaEid->serviceNbr)) == 2)
		{
			metaEid->eidFormat = EidFormat2Element;
		}
		else
		{
			writeMemoNote("[?] Malformed URI", eidString);
			clearMetaEid(metaEid);
			metaEid->eidCopy = NULL;
			return 0;
		}

		if (metaEid->elementNbr == 0 && metaEid->serviceNbr == 0)
		{
			metaEid->nullEndpoint = 1;
		}

		return 1;

#ifdef ENABLE_IMC
	case imc:
	{
		unsigned long groupNbr;
		if (sscanf(metaEid->nss, "%lu.%lu.%lu", &allocatorNbr,
			&groupNbr, &(metaEid->serviceNbr)) == 3)
		{
			metaEid->elementNbr = (((uvast) allocatorNbr << 32)
				& 0xffffffff00000000) +
				(groupNbr & 0x00000000ffffffff);
			metaEid->eidFormat = EidFormat3Element;
		}
		else if (sscanf(metaEid->nss, UVAST_FIELDSPEC ".%lu",
			&(metaEid->elementNbr), &(metaEid->serviceNbr)) == 2)
		{
			metaEid->eidFormat = EidFormat2Element;
		}
		else
		{
			writeMemoNote("[?] Malformed URI", eidString);
			clearMetaEid(metaEid);
			metaEid->eidCopy = NULL;
			return 0;
		}

		return 1;
	}
#endif

	default:
		writeMemoNote("[?] URI for this scheme not parseable",
				metaEid->schemeName);
		clearMetaEid(metaEid);
		metaEid->eidCopy = NULL;
	}

	return 0;
}

void	clearMetaEid(MetaEid *metaEid)
{
	if (metaEid)
	{
		if (metaEid->eidCopy)
		{
			MRELEASE(metaEid->eidCopy);
		}

		memset((char *) metaEid, 0, sizeof(MetaEid));
	}
}

int	recordEid(EndpointId *eid, MetaEid *meid, EidMode mode)
{
	Sdr		sdr;
	SdrObject	obj;
	PsmPartition	wm;
	PsmAddress	addr;
	int		nssLength;
	char		*ptr;

	eid->schemeCodeNbr = meid->schemeCodeNbr;
	eid->eidFormat = meid->eidFormat;	/*	Preserve CBOR form.	*/
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

#ifdef ENABLE_IMC
	case imc:
		eid->ssp.imc.fqgn = meid->elementNbr;
		eid->ssp.imc.serviceNbr = meid->serviceNbr;
		return 0;
#endif

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

#ifdef ENABLE_IMC
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
#endif

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

#ifdef ENABLE_IMC
	case imc:
		readImcEid(&(eid->ssp.imc), buffer);
		break;
#endif

	default:
		break;
	}
}

/*	String <-> CBOR-structured EID convenience wrappers, intended
 *	for extension blocks (CTEB, CREB) and compressed signals (CBR
 *	Bundle Sequences) whose CDDL types these fields as `eid` --
 *	the same RFC 9171 / RFC 9758 structure used by primary-block
 *	EIDs.  These wrap parseEidString + jotEid + serializeEid (and
 *	the inverse: acquireEid + readEid) so each call site stays a
 *	one-liner.  Storage at the call sites remains string-based;
 *	only the wire bytes change.					*/

int	serializeEidString(char *eidString, unsigned char *buffer)
{
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	EndpointId	eid;
	int		length;

	CHKERR(eidString);
	CHKERR(buffer);

	if (parseEidString(eidString, &metaEid, &vscheme, &vschemeElt) == 0)
	{
		putErrmsg("Can't parse EID for serialization.", eidString);
		return -1;
	}

	memset((char *) &eid, 0, sizeof eid);
	if (jotEid(&eid, &metaEid) < 0)
	{
		clearMetaEid(&metaEid);
		putErrmsg("Can't build EndpointId for serialization.",
				eidString);
		return -1;
	}

	clearMetaEid(&metaEid);
	length = serializeEid(&eid, buffer);
	eraseEid(&eid);
	if (length < 1)
	{
		putErrmsg("Can't serialize EID.", eidString);
		return -1;
	}

	return length;
}

int	acquireEidString(char *eidString, size_t eidStrLen,
			unsigned char **cursor, unsigned int *bytesRemaining)
{
	EndpointId	eid;
	char		*str = NULL;
	int		length;

	CHKERR(eidString);
	CHKERR(eidStrLen > 0);
	CHKERR(cursor);
	CHKERR(bytesRemaining);

	memset((char *) &eid, 0, sizeof eid);
	length = acquireEid(&eid, cursor, bytesRemaining);
	if (length < 1)
	{
		eraseEid(&eid);
		return 0;	/*	Malformed; matches acquireEid.	*/
	}

	readEid(&eid, &str);
	eraseEid(&eid);
	if (str == NULL)
	{
		putErrmsg("Can't render EID to string.", NULL);
		return -1;
	}

	istrcpy(eidString, str, (int) eidStrLen);
	MRELEASE(str);
	return length;
}

/*	*	Operations on endpoint ID patterns	*	*	*/

static void	destroyEidpInterval(LystElt elt, void *arg)
{
	EidpIpnInterval		*interval;

	(void)arg;	/*	Unused parameter			*/
	interval = (EidpIpnInterval *) lyst_data(elt);
	MRELEASE(interval);
}

static void	destroyEidpIpnSSP(EidpItem *item)
{
	int			i;
	EidpIpnComponent	component;

	for (i = 0; i < 3; i++)
	{
		component = item->ssp.ipnSSP.components[i];
		if (component.type == RangeValue)
		{
			lyst_destroy(component.value.range.intervals);
		}
	}
}

static void	destroyEidpItem(LystElt elt, void *arg)
{
	EidpItem	*item = (EidpItem *) lyst_data(elt);

	(void)arg;	/*	Unused parameter			*/
	if (item->schemeCodeNbr == ipn)
	{
		destroyEidpIpnSSP(item);
	}

	if (item->schemeName)
	{
		MRELEASE(item->schemeName);
	}

	memset((char *) item, 0, sizeof(EidpItem));
	MRELEASE(item);
}

EidPattern	*createEidPattern(void)
{
	EidPattern	*eidp;

	eidp = MTAKE(sizeof(EidPattern));
	if (eidp == NULL)
	{
		writeMemo("[?] Not enough memory for EID pattern.");
		return NULL;
	}

        memset(eidp, 0, sizeof(EidPattern));
	eidp->items = lyst_create_using(getIonMemoryMgr());
	if (eidp->items == NULL)
	{
		MRELEASE(eidp);
		writeMemo("[?] Not enough memory for EID pattern items list.");
		return NULL;
	}

	lyst_delete_set(eidp->items, destroyEidpItem, NULL);
	return eidp;
}

void	destroyEidPattern(EidPattern *eidp)
{
	CHKVOID(eidp);
	lyst_destroy(eidp->items);
    	memset(eidp, 0, sizeof(EidPattern));
	MRELEASE(eidp);
}

static int	loadIpnInterval(Lyst intervals, uvast startNbr, uvast endNbr)
{
	EidpIpnInterval	*interval;

	interval = MTAKE(sizeof(EidpIpnInterval));
	if (interval == NULL)
	{
		writeMemo("[?] Not enough memory for IPN EID Range Interval.");
		return -1;
	}

	interval->first = startNbr;
	interval->last = endNbr;
	if (lyst_insert_last(intervals, interval) == NULL)
	{
		MRELEASE(interval);
		writeMemo("[?] Can't record IPN EID Range interval.");
		return -1;
	}

	return 0;
}

static int	loadIpnRange(EidpIpnComponent *component, char *text)
{
	char	*endOfRange;
	char	*startOfInterval;
	char	*endOfInterval;
	char	*hyphen;
	Lyst	intervals;
	uvast	startNbr;
	uvast	endNbr;

	startOfInterval = text + 1;		/*	Skip over '['.	*/
	endOfRange = strchr(startOfInterval, ']');
	if (endOfRange == NULL)
	{
		writeMemo("[?] No end of IPN EID pattern Range.");
		return -1;
	}

	*endOfRange = '\0';			/*	End range.	*/
	intervals = lyst_create_using(getIonMemoryMgr());
	if (intervals == NULL)
	{
		writeMemo("[?] Not enough memory for IPN EID pattern Range.");
		return -1;
	}

	lyst_delete_set(intervals, destroyEidpInterval, NULL);
	while (startOfInterval < endOfRange)
	{
		hyphen = strchr(startOfInterval, '-');
		endOfInterval = strchr(startOfInterval, ',');
		if (endOfInterval == NULL)
		{
			endOfInterval = endOfRange;
		}

		*endOfInterval = '\0';		/*	End interval.	*/
		if (hyphen == NULL || endOfInterval < hyphen)
		{
			/*	Single-value interval.			*/

			startNbr = strtouvast(startOfInterval);
			endNbr = startNbr;
		}
		else	/*	An interval between two numbers.	*/
		{
			*hyphen = '\0';
			startNbr = strtouvast(startOfInterval);
			endNbr = strtouvast(hyphen + 1);
		}

		if (loadIpnInterval(intervals, startNbr, endNbr) < 0)
		{
			lyst_destroy(intervals);
			return -1;
		}

		startOfInterval = endOfInterval + 1;
	}

	component->value.range.intervals = intervals;
	return 0;
}

static int	loadIpnComponent(EidpItem *item, int i, char *text)
{
	EidpIpnComponent	*component;

	component = item->ssp.ipnSSP.components + i;
	switch (*text)
	{
	case '*':
		component->type = AnyValue;
		return 0;

	case '[':
		component->type = RangeValue;
		return loadIpnRange(component, text);

	default:
		component->type = NumValue;
		component->value.number = strtouvast(text);
		return 0;
	}
}

static EidpItem	*loadEidpIpnSSP(EidpItem *item, char *ssl)
{
	int	tokenCount = 0;
	char	*cursor;
	int	i;
	char	*delimiter;
	char	*tokens[3];

	/*	Identify all components of the ipn-scheme EID SSP.	*/

	cursor = ssl;
	for (i = 0; i < 4; i++)	/*	4th, if found, is an error.	*/
	{
		tokens[i] = cursor;	/*	Text of component.	*/
		tokenCount++;

		/*	Find the end of this token.			*/

		delimiter = strchr(cursor, '.');
		if (delimiter == NULL)
		{
			break;	/*	This is the last token.	*/
		}

		*delimiter = '\0';	/*	End of token.	*/
		cursor = delimiter + 1;	/*	Get next token.	*/
	}

	/*	Item must have exactly 2 or 3 tokens. Modern IPN uses
	 *	3 components (allocator.node.service), legacy format
	 *	uses 2 (node.service) with allocator=0 assumed.	*/

	if (tokenCount < 2 || tokenCount > 3)
	{
		MRELEASE(item);
		return NULL;
	}

	/*	If only 2 tokens (legacy format), insert allocator=0.	*/

	if (tokenCount == 2)
	{
		/*	Legacy format: tokens[0]=node, tokens[1]=service
		 *	Modern format: tokens[0]=allocator, tokens[1]=node,
		 *		tokens[2]=service
		 *	Transform: shift right and insert allocator=0.	*/

		tokens[2] = tokens[1];	/* service → position 2 */
		tokens[1] = tokens[0];	/* node → position 1 */
		tokens[0] = "0";	/* allocator=0 → position 0 */
		tokenCount = 3;
	}

	for (i = 0; i < 3; i++)
	{
		if (loadIpnComponent(item, i, tokens[i]) < 0)
		{
			destroyEidpIpnSSP(item);
			MRELEASE(item);
			return NULL;
		}
	}

	return item;
}

static EidpItem	*loadPatternItem(char *buffer)
{
	char		*colon;
	EidpItem	*item;
	char		*ssp;
	char		*cursor;
	int		schemeNameLen;

	colon = strchr(buffer, ':');
	if (colon == NULL)	/*	No scheme ID, not an eidp item.	*/
	{
		return NULL;	/*	Nothing for pattern.		*/
	}

	*colon = '\0';		/*	Terminate scheme identifier.	*/
	item = (EidpItem *) MTAKE(sizeof(EidpItem));
	if (item == NULL)
	{
		writeMemo("[?] Not enough memory for EID pattern item.");
		return NULL;
	}

	item->schemeName = NULL;
	ssp = colon + 1;
	cursor = buffer;
	if (*cursor == '*')	/*	Any scheme.			*/
	{
		item->schemeCodeNbr = unknown;
		item->schemeName = NULL;
		item->ssp.anySSP.any = NULL;
		return item;
	}

	if (isalpha((unsigned char)*cursor))	/*	Have scheme name.	*/
	{
		if (strcmp(cursor, "dtn") == 0)
		{
			item->schemeCodeNbr = dtn;
		}
		else if (strcmp(cursor, "ipn") == 0)
		{
			item->schemeCodeNbr = ipn;
		}
		else if (strcmp(cursor, "imc") == 0)
		{
			item->schemeCodeNbr = imc;
		}
		else		/*	Unknown scheme.			*/
		{
			item->schemeCodeNbr = unknown;
			schemeNameLen = strlen(cursor);
			item->schemeName = MTAKE(schemeNameLen + 1);
			if (item->schemeName == NULL)
			{
				writeMemoNote("[?] Not enough memory for EID \
pattern item scheme name", cursor);
				MRELEASE(item);
				return NULL;
			}

			strcpy(item->schemeName, cursor);
		}
	}
	else
	{
		if (isdigit((unsigned char)*cursor))	/*	Have scheme number.	*/
		{
			item->schemeCodeNbr = atoi(cursor);
			item->schemeName = NULL;
		}
		else			/*	No scheme ID at all.	*/
		{
			MRELEASE(item);
			return NULL;
		}
	}

	if (item->schemeCodeNbr == ipn)
	{
		item = loadEidpIpnSSP(item, ssp);
	}
	else	/*	Unrecognized scheme ID, either number or name.	*/
	{
		/*	Matches any SSP formed in this scheme.		*/

		item->ssp.anySSP.any = NULL;
	}

	return item;
}

int	loadEidPattern(EidPattern *eidp, const char *text)
{
	int		textLength;
	char		*buffer;
	char		*nextItem;
	char		*cursor;
	char		*itemDelimiter;
	EidpItem	*item;

	textLength = strlen(text);
	buffer = MTAKE(textLength + 1);
	if (buffer == NULL)
	{
		writeMemo("[?] Not enough memory for EID pattern parsing \
buffer.");
		return -1;
	}

	strcpy(buffer, text);
	nextItem = buffer;
	while (nextItem)
	{
		cursor = nextItem;
		itemDelimiter = strchr(cursor, '|');
		if (itemDelimiter)
		{
			*itemDelimiter = '\0';
			nextItem = itemDelimiter + 1;
		}
		else	/*	This is the last item in the pattern.	*/
		{
			nextItem = NULL;
		}

		item = loadPatternItem(cursor);
		if (item)
		{
			if (lyst_insert_last(eidp->items, item) == NULL)
			{
				writeMemo("[?] Can't record EID pattern item.");
				MRELEASE(buffer);
				return -1;
			}
		}
	}

	MRELEASE(buffer);
	return 0;
}

static int	isInRange(Lyst intervals, uvast val)
{
	LystElt		elt;
	EidpIpnInterval	*interval;

	for (elt = lyst_first(intervals); elt; elt = lyst_next(elt))
	{
		interval = (EidpIpnInterval *) lyst_data(elt);
		if (val >= interval->first && val <= interval->last)
		{
			return 1;
		}
	}

	return 0;
}

static int	ipnEidMatchesItem(EidpItem *item, uvast *eidComponents)
{
	int			matchCount = 0;
	int			i;
	EidpIpnComponent	component;
	uvast			val;

	for (i = 0; i < 3; i++)
	{
		component = item->ssp.ipnSSP.components[i];
		val = eidComponents[i];
		switch (component.type)
		{
		case NoValue:
			break;

		case AnyValue:
			matchCount++;
			break;

		case NumValue:
			if (component.value.number == val)
			{
				matchCount++;
			}

			break;

		case RangeValue:
			if (isInRange(component.value.range.intervals, val))
			{
				matchCount++;
			}

			break;

		default:
			break;
		}
	}

	if (matchCount == 3)
	{
		return 1;
	}

	return 0;
}

int	eidMatchesPattern(EidPattern *eidp, EndpointId *eid)
{
	uvast		eidComponents[3];
	LystElt		elt;
	EidpItem	*item;

	if (eid->schemeCodeNbr == ipn)
	{
		eidComponents[0] = (eid->ssp.ipn.fqnn >> 32)
				& 0x00000000ffffffff;
		eidComponents[1] = eid->ssp.ipn.fqnn & 0x00000000ffffffff;
		eidComponents[2] = eid->ssp.ipn.serviceNbr;
	}

	for (elt = lyst_first(eidp->items); elt; elt = lyst_next(elt))
	{
		item = (EidpItem *) lyst_data(elt);
		if (item->schemeCodeNbr == ipn)
		{
			if (eid->schemeCodeNbr == ipn)
			{
				if (ipnEidMatchesItem(item, eidComponents))
				{
					return 1;
				}
			}

			/*	EID doesn't match this pattern item.	*/

			continue;
		}

		/*	Item is for a scheme whose SSP structure is
		 *	not yet supported in EID patterns.		*/

		if (item->schemeCodeNbr == eid->schemeCodeNbr)
		{
			/*	This item indicates that any EID
			 *	formed in the indicated known but
			 *	unsupported scheme is considered
			 *	a match to this pattern.		*/

			return 1;
		}
		else
		{
			if (item->schemeCodeNbr == unknown)
			{
				/*	This item indicates that any
				 *	EID formed in any scheme
				 *	formed in any unknown scheme
				 *	is considered a match to this
				 *	pattern.			*/

				return 1;
			}
		}
	}

	/*	EID doesn't match any of this pattern's items.		*/

	return 0;
}

/*	*	*	Operations on bundles	*	*	*	*/

int bp_send(BpSAP sap, char *destEid, char *reportToEid, int lifespan,
		int classOfService, BpCustodySwitch custodySwitch,
		unsigned char srrFlags, int ackRequested,
		BpAncillaryData *ancillaryData, SdrObject adu,
		SdrObject *bundleObj)
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

int bp_track(SdrObject bundleObj, SdrObject trackingElt)
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

void bp_untrack(SdrObject bundleObj, SdrObject trackingElt)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(Bundle, bundle);
	SdrObject elt;

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

int bp_memo(SdrObject bundleObj, unsigned int interval)
{
	/* Parameter intentionally unused. */
	(void)bundleObj;
	(void)interval;

	return 0;
}

int bp_suspend(SdrObject bundleObj)
{
	Sdr		sdr = getIonsdr();
	Bundle		bundle;
	SdrObject	queue;
	SdrObject	planObj;
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

int bp_resume(SdrObject bundleObj)
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

int bp_cancel(SdrObject bundleObj)
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

int bp_release(SdrObject bundleObj)
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

static void stopBprcvTimer(pthread_t timerThread, int timeoutSeconds)
{
	/*
	 * A timer thread is started only for a receive() with a positive
	 * deadline; BP_BLOCKING and BP_POLL start none. Cancelling and joining
	 * a timer thread that has already terminated on its own is harmless,
	 * so this is safe on every exit path once the thread has been started.
	 * The join is mandatory: it reaps the thread and, together with the
	 * (non-static) stack-local TimerParms in bp_receive(), guarantees the
	 * timer never outlives its argument or races a concurrent receiver in
	 * another thread.
	 */
	if (timeoutSeconds > 0)
	{
		pthread_end(timerThread);
		pthread_join(timerThread, NULL);
	}
}

int	bp_receive(BpSAP sap, BpDelivery *dlvBuffer, int timeoutSeconds)
{
	Sdr		sdr = getIonsdr();
	VEndpoint	*vpoint;
			OBJ_POINTER(Endpoint, endpoint);
	SdrObject	dlvElt;
	SdrObject	bundleAddr;
	Bundle		bundle;
	TimerParms	timerParms;
	pthread_t	timerThread;
	int		result;

	CHKERR(sap && dlvBuffer);
	memset((char *) dlvBuffer, 0, sizeof(BpDelivery));
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
			stopBprcvTimer(timerThread, timeoutSeconds);
			putErrmsg("Can't take endpoint semaphore.", NULL);
			return -1;
		}

		if (sm_SemEnded(vpoint->semaphore))
		{
			stopBprcvTimer(timerThread, timeoutSeconds);
			writeMemo("[i] Endpoint has been stopped.");
			dlvBuffer->result = BpEndpointStopped;

			/*	End task, but without error.		*/

			return 0;
		}

		/*	Have taken the semaphore, one way or another.	*/

		result = sdr_begin_xn(sdr);
		if (result == 0)
		{
			stopBprcvTimer(timerThread, timeoutSeconds);
		}

		CHKERR(result);
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
	SdrObject    endpointObj;
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
	SdrObject  elt;
	SdrObject  protocolObj;
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

/*
 * Bulk removal functions for runtime reconfiguration.
 *
 * These functions collect identifiers first, then remove each item.
 * This avoids iterator invalidation when modifying the list.
 */

#define MAX_BULK_REMOVE_ITEMS	256

int	bp_remove_all_endpoints(char *scheme)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	bpwm = getIonwm();
	BpVdb		*vdb = getBpVdb();
	PsmAddress	schemeElt;
	PsmAddress	endpointElt;
	VScheme		*vscheme;
	VEndpoint	*vpoint;
	char		*eids[MAX_BULK_REMOVE_ITEMS];
	int		count = 0;
	int		removed = 0;
	int		i;
	int		result;
	char		eidBuf[MAX_EID_LEN];

	if (vdb == NULL)
	{
		writeMemo("[?] bp_remove_all_endpoints: BP not initialized.");
		return -1;
	}

	if (scheme == NULL)
	{
		writeMemo("[?] bp_remove_all_endpoints: scheme is NULL.");
		return -1;
	}

	/*	First, collect all EIDs for the specified scheme to
	 *	avoid iterator invalidation when removing items.	*/

	CHKERR(sdr_begin_xn(sdr));
	for (schemeElt = sm_list_first(bpwm, vdb->schemes); schemeElt;
			schemeElt = sm_list_next(bpwm, schemeElt))
	{
		vscheme = (VScheme *) psp(bpwm, sm_list_data(bpwm, schemeElt));
		if (vscheme == NULL)
		{
			continue;
		}

		/*	Check if this is the requested scheme.		*/

		if (strcmp(vscheme->name, scheme) != 0)
		{
			continue;
		}

		/*	Collect all endpoint EIDs in this scheme.	*/

		for (endpointElt = sm_list_first(bpwm, vscheme->endpoints);
				endpointElt;
				endpointElt = sm_list_next(bpwm, endpointElt))
		{
			vpoint = (VEndpoint *) psp(bpwm,
					sm_list_data(bpwm, endpointElt));
			if (vpoint == NULL || count >= MAX_BULK_REMOVE_ITEMS)
			{
				continue;
			}

			/*	Construct the full EID.			*/

			isprintf(eidBuf, sizeof(eidBuf), "%s:%s",
					vscheme->name, vpoint->nss);
			eids[count] = MTAKE(strlen(eidBuf) + 1);
			if (eids[count] != NULL)
			{
				istrcpy(eids[count], eidBuf,
						strlen(eidBuf) + 1);
				count++;
			}
		}

		break;	/*	Found the scheme, no need to continue.	*/
	}

	sdr_exit_xn(sdr);

	/*	Now remove each endpoint.				*/

	for (i = 0; i < count; i++)
	{
		result = removeEndpoint(eids[i]);
		if (result == 1)
		{
			removed++;
		}
		else if (result < 0)
		{
			writeMemoNote("[?] bp_remove_all_endpoints: Error \
removing endpoint", eids[i]);
		}
		/*	result == 0 means endpoint not found or has
		 *	pending data; continue with others.		*/

		MRELEASE(eids[i]);
	}

	return removed;
}

int	bp_remove_all_inducts(char *protocol)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	bpwm = getIonwm();
	BpVdb		*vdb = getBpVdb();
	PsmAddress	elt;
	VInduct		*vinduct;
	char		*protocolNames[MAX_BULK_REMOVE_ITEMS];
	char		*ductNames[MAX_BULK_REMOVE_ITEMS];
	int		count = 0;
	int		removed = 0;
	int		i;
	int		result;

	if (vdb == NULL)
	{
		writeMemo("[?] bp_remove_all_inducts: BP not initialized.");
		return -1;
	}

	if (protocol == NULL)
	{
		writeMemo("[?] bp_remove_all_inducts: protocol is NULL.");
		return -1;
	}

	/*	First, collect all induct identifiers for the specified
	 *	protocol to avoid iterator invalidation.		*/

	CHKERR(sdr_begin_xn(sdr));
	for (elt = sm_list_first(bpwm, vdb->inducts); elt;
			elt = sm_list_next(bpwm, elt))
	{
		vinduct = (VInduct *) psp(bpwm, sm_list_data(bpwm, elt));
		if (vinduct == NULL || count >= MAX_BULK_REMOVE_ITEMS)
		{
			continue;
		}

		/*	Check if this induct matches the protocol.	*/

		if (strcmp(vinduct->protocolName, protocol) != 0)
		{
			continue;
		}

		/*	Store protocol name and duct name.		*/

		protocolNames[count] = MTAKE(strlen(vinduct->protocolName) + 1);
		ductNames[count] = MTAKE(strlen(vinduct->ductName) + 1);
		if (protocolNames[count] != NULL && ductNames[count] != NULL)
		{
			istrcpy(protocolNames[count], vinduct->protocolName,
					strlen(vinduct->protocolName) + 1);
			istrcpy(ductNames[count], vinduct->ductName,
					strlen(vinduct->ductName) + 1);
			count++;
		}
		else
		{
			if (protocolNames[count] != NULL)
			{
				MRELEASE(protocolNames[count]);
			}

			if (ductNames[count] != NULL)
			{
				MRELEASE(ductNames[count]);
			}
		}
	}

	sdr_exit_xn(sdr);

	/*	Now remove each induct.					*/

	for (i = 0; i < count; i++)
	{
		result = removeInduct(protocolNames[i], ductNames[i]);
		if (result == 1)
		{
			removed++;
		}
		else if (result < 0)
		{
			writeMemoNote("[?] bp_remove_all_inducts: Error \
removing induct", ductNames[i]);
		}
		/*	result == 0 means induct not found; continue
		 *	with others.					*/

		MRELEASE(protocolNames[i]);
		MRELEASE(ductNames[i]);
	}

	return removed;
}

int	bp_remove_all_outducts(char *protocol)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	bpwm = getIonwm();
	BpVdb		*vdb = getBpVdb();
	PsmAddress	elt;
	VOutduct	*voutduct;
	char		*protocolNames[MAX_BULK_REMOVE_ITEMS];
	char		*ductNames[MAX_BULK_REMOVE_ITEMS];
	int		count = 0;
	int		removed = 0;
	int		i;
	int		result;

	if (vdb == NULL)
	{
		writeMemo("[?] bp_remove_all_outducts: BP not initialized.");
		return -1;
	}

	if (protocol == NULL)
	{
		writeMemo("[?] bp_remove_all_outducts: protocol is NULL.");
		return -1;
	}

	/*	First, collect all outduct identifiers for the specified
	 *	protocol to avoid iterator invalidation.		*/

	CHKERR(sdr_begin_xn(sdr));
	for (elt = sm_list_first(bpwm, vdb->outducts); elt;
			elt = sm_list_next(bpwm, elt))
	{
		voutduct = (VOutduct *) psp(bpwm, sm_list_data(bpwm, elt));
		if (voutduct == NULL || count >= MAX_BULK_REMOVE_ITEMS)
		{
			continue;
		}

		/*	Check if this outduct matches the protocol.	*/

		if (strcmp(voutduct->protocolName, protocol) != 0)
		{
			continue;
		}

		/*	Store protocol name and duct name.		*/

		protocolNames[count] = MTAKE(strlen(voutduct->protocolName) + 1);
		ductNames[count] = MTAKE(strlen(voutduct->ductName) + 1);
		if (protocolNames[count] != NULL && ductNames[count] != NULL)
		{
			istrcpy(protocolNames[count], voutduct->protocolName,
					strlen(voutduct->protocolName) + 1);
			istrcpy(ductNames[count], voutduct->ductName,
					strlen(voutduct->ductName) + 1);
			count++;
		}
		else
		{
			if (protocolNames[count] != NULL)
			{
				MRELEASE(protocolNames[count]);
			}

			if (ductNames[count] != NULL)
			{
				MRELEASE(ductNames[count]);
			}
		}
	}

	sdr_exit_xn(sdr);

	/*	Now remove each outduct.				*/

	for (i = 0; i < count; i++)
	{
		result = removeOutduct(protocolNames[i], ductNames[i]);
		if (result == 1)
		{
			removed++;
		}
		else if (result < 0)
		{
			writeMemoNote("[?] bp_remove_all_outducts: Error \
removing outduct", ductNames[i]);
		}
		/*	result == 0 means outduct not found or attached
		 *	to a plan; continue with others.		*/

		MRELEASE(protocolNames[i]);
		MRELEASE(ductNames[i]);
	}

	return removed;
}
