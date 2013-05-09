/*
	dtn2fw.c:	scheme-specific forwarder for the "dtn2"
			scheme.

	Author: Scott Burleigh, JPL

	Copyright (c) 2006, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include "dtn2fw.h"

static sm_SemId	_dtn2fwSemaphore(sm_SemId *newValue)
{
	long		temp;
	void		*value;
	sm_SemId	sem;

	if (newValue)			/*	Add task variable.	*/
	{
		temp = *newValue;
		value = (void *) temp;
		value = sm_TaskVar(&value);
	}
	else				/*	Retrieve task variable.	*/
	{
		value = sm_TaskVar(NULL);
	}

	temp = (long) value;
	sem = temp;
	return sem;
}

static void	shutDown()	/*	Commands forwarder termination.	*/
{
	isignal(SIGTERM, shutDown);
	sm_SemEnd(_dtn2fwSemaphore(NULL));
}

static int	parseDtn2Nss(char *nss, char *nodeName, char *demux)
{
	int	nssLength;
	char	*startOfNodeName;
	char	*endOfNodeName;
	char	*startOfDemux;

	nssLength = strlen(nss);
	if (nssLength < 3 || strncmp(nss, "//", 2) != 0)
	{
		return 0;		/*	Wrong NSS format.	*/
	}

	startOfNodeName = nss;
	endOfNodeName = strchr(startOfNodeName + 2, '/');
	if (endOfNodeName == NULL)	/*	No delimiter, no demux.	*/
	{
		if (nssLength >= SDRSTRING_BUFSZ)
		{
			return 0;	/*	Too long.		*/
		}

		istrcpy(nodeName, startOfNodeName, SDRSTRING_BUFSZ);
		*demux = '\0';
		return 1;		/*	Fully parsed.		*/
	}

	if ((endOfNodeName - startOfNodeName) >= SDRSTRING_BUFSZ)
	{
		return 0;		/*	Too long.		*/
	}

	*endOfNodeName = '\0';		/*	Temporary.		*/
	istrcpy(nodeName, startOfNodeName, SDRSTRING_BUFSZ);
	*endOfNodeName = '/';		/*	Restore original.	*/
	startOfDemux = endOfNodeName + 1;
	if (strlen(startOfDemux) >= SDRSTRING_BUFSZ)
	{
		return 0;
	}

	istrcpy(demux, startOfDemux, SDRSTRING_BUFSZ);
	return 1;
}

static int	enqueueBundle(Bundle *bundle, Object bundleObj)
{
	Sdr		sdr = getIonsdr();
	Object		elt;
	char		eidString[SDRSTRING_BUFSZ];
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	char		nodeName[SDRSTRING_BUFSZ];
	char		demux[SDRSTRING_BUFSZ];
	int		result;
	FwdDirective	directive;

	elt = sdr_list_first(sdr, bundle->stations);
	if (elt == 0)
	{
		putErrmsg("Forwarding error; stations stack is empty.", NULL);
		return -1;
	}

	sdr_string_read(sdr, eidString, sdr_list_data(sdr, elt));
	if (parseEidString(eidString, &metaEid, &vscheme, &vschemeElt) == 0)
	{
		putErrmsg("Can't parse node EID string.", eidString);
		return bpAbandon(bundleObj, bundle);
	}

	if (strcmp(vscheme->name, "dtn") != 0)
	{
		putErrmsg("Forwarding error; EID scheme wrong for dtn2fw.",
				vscheme->name);
		return -1;
	}

	result = parseDtn2Nss(metaEid.nss, nodeName, demux);
	restoreEidString(&metaEid);
	if (result == 0)
	{
		putErrmsg("Invalid nss in EID string, cannot forward.",
				eidString);
		return bpAbandon(bundleObj, bundle);
	}

	if (dtn2_lookupDirective(nodeName, demux, &directive) == 0)
	{
		putErrmsg("Can't find forwarding directive for EID.",
				eidString);
		return bpAbandon(bundleObj, bundle);
	}

	if (directive.action == xmit)
	{
		if (bpEnqueue(&directive, bundle, bundleObj, eidString) < 0)
		{
			putErrmsg("Can't enqueue bundle.", NULL);
			return -1;
		}

		if (bundle->ductXmitElt)
		{
			/*	Enqueued.				*/

			return bpAccept(bundleObj, bundle);
		}
		else
		{
			return bpAbandon(bundleObj, bundle);
		}
	}

	/*	Can't transmit to indicated next node directly, must
	 *	forward through some other node.			*/

	sdr_write(sdr, bundleObj, (char *) &bundle, sizeof(Bundle));
	sdr_string_read(sdr, eidString, directive.eid);
	return forwardBundle(bundleObj, bundle, eidString);
}

#if defined (VXWORKS) || defined (RTEMS) || defined (bionic)
int	dtn2fw(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
#else
int	main(int argc, char *argv[])
{
#endif
	int		running = 1;
	Sdr		sdr;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	Scheme		scheme;
	Object		elt;
	Object		bundleAddr;
	Bundle		bundle;

	if (bpAttach() < 0)
	{
		putErrmsg("dtn2fw can't attach to BP.", NULL);
		return 1;
	}

	if (dtn2Init(NULL) < 0)
	{
		putErrmsg("dtn2fw can't load routing database.", NULL);
		return 1;
	}

	sdr = getIonsdr();
	findScheme("dtn", &vscheme, &vschemeElt);
	if (vschemeElt == 0)
	{
		putErrmsg("Scheme name for dtn2 is unknown.", "dtn");
		return 1;
	}

	CHKZERO(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &scheme, sdr_list_data(sdr,
			vscheme->schemeElt), sizeof(Scheme));
	sdr_exit_xn(sdr);
	oK(_dtn2fwSemaphore(&vscheme->semaphore));
	isignal(SIGTERM, shutDown);

	/*	Main loop: wait until forwarding queue is non-empty,
	 *	then drain it.						*/

	writeMemo("[i] dtn2fw is running.");
	while (running && !(sm_SemEnded(vscheme->semaphore)))
	{
		/*	We wrap forwarding in an SDR transaction to
		 *	prevent race condition with bpclock (which
		 *	is destroying bundles as their TTLs expire).	*/

		CHKZERO(sdr_begin_xn(sdr));
		elt = sdr_list_first(sdr, scheme.forwardQueue);
		if (elt == 0)	/*	Wait for forwarding notice.	*/
		{
			sdr_exit_xn(sdr);
			if (sm_SemTake(vscheme->semaphore) < 0)
			{
				putErrmsg("Can't take forwarder semaphore.",
						NULL);
				running = 0;
			}

			continue;
		}

		bundleAddr = (Object) sdr_list_data(sdr, elt);
		sdr_stage(sdr, (char *) &bundle, bundleAddr, sizeof(Bundle));
		sdr_list_delete(sdr, elt, NULL, NULL);
		bundle.fwdQueueElt = 0;

		/*	Must rewrite bundle to note removal of
		 *	fwdQueueElt, in case the bundle is abandoned
		 *	and bpDestroyBundle re-reads it from the
		 *	database.					*/

		sdr_write(sdr, bundleAddr, (char *) &bundle, sizeof(Bundle));
		if (enqueueBundle(&bundle, bundleAddr) < 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("Can't enqueue bundle.", NULL);
			running = 0;	/*	Terminate loop.		*/
			continue;
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't enqueue bundle.", NULL);
			running = 0;	/*	Terminate loop.		*/
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	writeErrmsgMemos();
	writeMemo("[i] dtn2fw forwarder has ended.");
	ionDetach();
	return 0;
}
