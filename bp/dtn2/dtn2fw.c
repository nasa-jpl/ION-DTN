/*
	dtn2fw.c:	scheme-specific forwarder for the "dtn2"
			scheme.

	Author: Scott Burleigh, JPL

	Copyright (c) 2006, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include "dtn2fw.h"

static Sdr	dtn2Sdr;
static sm_SemId	dtn2fwSemaphore;

static void	shutDown()	/*	Commands forwarder termination.	*/
{
	sm_SemEnd(dtn2fwSemaphore);
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

		strcpy(nodeName, startOfNodeName);
		*demux = '\0';
		return 1;		/*	Fully parsed.		*/
	}

	if ((endOfNodeName - startOfNodeName) >= SDRSTRING_BUFSZ)
	{
		return 0;		/*	Too long.		*/
	}

	*endOfNodeName = '\0';		/*	Temporary.		*/
	strcpy(nodeName, startOfNodeName);
	*endOfNodeName = '/';		/*	Restore original.	*/
	startOfDemux = endOfNodeName + 1;
	if (strlen(startOfDemux) >= SDRSTRING_BUFSZ)
	{
		return 0;
	}

	strcpy(demux, startOfDemux);
	return 1;
}

static int	enqueueBundle(Bundle *bundle, Object bundleObj)
{
	Object		elt;
	char		eidString[SDRSTRING_BUFSZ];
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	char		nodeName[SDRSTRING_BUFSZ];
	char		demux[SDRSTRING_BUFSZ];
	int		result;
	FwdDirective	directive;

	elt = sdr_list_first(dtn2Sdr, bundle->stations);
	if (elt == 0)
	{
		putErrmsg("Forwarding error; stations stack is empty.", NULL);
		return -1;
	}

	sdr_string_read(dtn2Sdr, eidString, sdr_list_data(dtn2Sdr, elt));
	if (parseEidString(eidString, &metaEid, &vscheme, &vschemeElt) == 0)
	{
		putErrmsg("Can't parse node EID string.", eidString);
		return bpAbandon(bundleObj, bundle);
	}

	if (strcmp(vscheme->name, DTN2_SCHEME_NAME) != 0)
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
		if (enqueueToDuct(&directive, bundle, bundleObj, eidString) < 0)
		{
			putErrmsg("Can't enqueue bundle.", NULL);
			return -1;
		}

		if (sdr_list_length(dtn2Sdr, bundle->xmitRefs) > 0)
		{
			/*	Enqueued.				*/

			return bpAccept(bundle);
		}
		else
		{
			return bpAbandon(bundleObj, bundle);
		}
	}

	/*	Can't transmit to indicated next node directly, must
	 *	forward through some other node.			*/

	sdr_string_read(dtn2Sdr, eidString, directive.eid);
	return forwardBundle(bundleObj, bundle, eidString);
}

#if defined (VXWORKS) || defined (RTEMS)
int	dtn2fw(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
#else
int	main(int argc, char *argv[])
{
#endif
	int		running = 1;
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

	dtn2Sdr = getIonsdr();
	findScheme(DTN2_SCHEME_NAME, &vscheme, &vschemeElt);
	if (vschemeElt == 0)
	{
		putErrmsg("Scheme name for dtn2 is unknown.", DTN2_SCHEME_NAME);
		return 1;
	}

	sdr_read(dtn2Sdr, (char *) &scheme, sdr_list_data(dtn2Sdr,
			vscheme->schemeElt), sizeof(Scheme));
	dtn2fwSemaphore = vscheme->semaphore;
	signal(SIGTERM, shutDown);

	/*	Main loop: wait until forwarding queue is non-empty,
	 *	then drain it.						*/

	sm_TaskVarAdd(&dtn2fwSemaphore);
	writeMemo("[i] dtn2fw is running.");
	while (running && !(sm_SemEnded(dtn2fwSemaphore)))
	{
		/*	We wrap forwarding in an SDR transaction to
		 *	prevent race condition with bpclock (which
		 *	is destroying bundles as their TTLs expire).	*/

		sdr_begin_xn(dtn2Sdr);
		elt = sdr_list_first(dtn2Sdr, scheme.forwardQueue);
		if (elt == 0)	/*	Wait for forwarding notice.	*/
		{
			sdr_exit_xn(dtn2Sdr);
			if (sm_SemTake(dtn2fwSemaphore) < 0)
			{
				putErrmsg("Can't take forwarder semaphore.",
						NULL);
				running = 0;
			}

			continue;
		}

		bundleAddr = (Object) sdr_list_data(dtn2Sdr, elt);
		sdr_stage(dtn2Sdr, (char *) &bundle, bundleAddr,
				sizeof(Bundle));
		sdr_list_delete(dtn2Sdr, elt, NULL, NULL);
		bundle.fwdQueueElt = 0;

		/*	Must rewrite bundle to note removal of
		 *	fwdQueueElt, in case the bundle is abandoned
		 *	and bpDestroyBundle re-reads it from the
		 *	database.					*/

		sdr_write(dtn2Sdr, bundleAddr, (char *) &bundle,
				sizeof(Bundle));
		if (enqueueBundle(&bundle, bundleAddr) < 0)
		{
			sdr_cancel_xn(dtn2Sdr);
			putErrmsg("Can't enqueue bundle.", NULL);
			running = 0;	/*	Terminate loop.		*/
			continue;
		}

		sdr_write(dtn2Sdr, bundleAddr, (char *) &bundle,
				sizeof(Bundle));
		if (sdr_end_xn(dtn2Sdr) < 0)
		{
			putErrmsg("Can't enqueue bundle.", NULL);
			running = 0;	/*	Terminate loop.		*/
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	writeErrmsgMemos();
	writeMemo("[i] dtn2fw forwarder has ended.");
	return 0;
}
