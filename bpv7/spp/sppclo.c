/*
	sppclo.c:	BP Space Packet Protocol-based convergence-layer output
			daemon.

	Author: Gregory Miles JPL

	Copyright (c) 2025, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "sppcla.h"
#include <dlfcn.h>

static sm_SemId		sppcloSemaphore(sm_SemId *semid)
{
	static sm_SemId	semaphore = -1;
	
	if (semid)
	{
		semaphore = *semid;
	}

	return semaphore;
}

static void	shutDownClo(int signum)
{
	sm_SemEnd(sppcloSemaphore(NULL));
}


static int openSPPFunctions(struct SppConfig* sppconfig,void *handle)
{

    char *error = NULL;

    dlerror();

    *(void **)(&sppconfig->packet_request) = dlsym(handle,"packet_request");
    if ((error = dlerror()) != NULL && handle != NULL)
    {
	fprintf(stderr, "%s\n", error);
	dlclose(handle);
	return -1;
    }

    return 0;
}

/*	*	*	Main thread functions	*	*	*	*/

/*static unsigned long	getUsecTimestamp()
{
	struct timeval	tv;

	getCurrentTime(&tv);
	return ((tv.tv_sec * 1000000) + tv.tv_usec);
	}*/

#if defined (ION_LWT)
int	sppclo(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
    char                    *endpointSpec = (char *)a1;
    char                    *sppCLAConfigStr = (char *)a1;
#else
int	main(int argc, char *argv[])
{
    char                    *endpointSpec = (argc > 1 ? argv[1] : NULL);
    char                    *sppCLAConfigStr = (argc > 2 ? argv[2] : NULL);
#endif
	unsigned char		*buffer;
	VOutduct		*vduct;
	PsmAddress		vductElt;
	Sdr			sdr;
	Outduct			outduct;
	Object			planDuctList;
	Object			planObj = 0;
	BpPlan			plan;
	//IonNeighbor		*neighbor = NULL;
	//PsmAddress		nextElt;
	Object			bundleZco;
	BpAncillaryData		ancillaryData;
	unsigned int		bundleLength;
	int			bytesSent = 0;
	void                    *funcHandle = NULL;
	int                     parsed_count = 0;
	int                     apid = 123;
	int                     seq_count = 0;
	int                     packet_type = 0;
	int                     sec_header_flag = 0;
	char                    *spacePacketConfigStr = NULL;
//        char                    *endpointSpec = NULL;
	char                    *sharedLibPath = NULL;
	char                    *ductBPConfig = NULL;
	const char              *delim = ";";
	char                    *tok = NULL;
	struct SppConfig *sppcfg;
	struct SppConfig sppcfgdefaults = {123,0,0,0,NULL,NULL};
	sppcfg = &sppcfgdefaults;

	if (sppCLAConfigStr == NULL)
	{
	    PUTS("Usage: sppclo {<remote node IPN> | \
@};{shared library path};{space packet config};[:<BP reliability]");
	    return 0;
	}
	tok = strtok(sppCLAConfigStr,delim);
	while (tok != NULL) {
	    parsed_count++;
	    switch (parsed_count)
	    {

	    case 1:
		sharedLibPath = tok;
		break;
	    case 2:
		spacePacketConfigStr = tok;
		break;
	    case 3:
		ductBPConfig = tok;
		break;
	    }
	    tok = strtok(NULL,delim);
	}

	if (sharedLibPath != NULL && spacePacketConfigStr != NULL)
	    printf("using shared library %s %s\n",sharedLibPath,spacePacketConfigStr);

	
	if (parsed_count != 2 && parsed_count != 3)
	{
	    putErrmsg("Space Packet CLA Configuration must be 2 strings of end point, shared library path, space packet configuration and optionally BP reliability.",sharedLibPath);
	    return -1;
	}

	funcHandle = dlopen(sharedLibPath, RTLD_NOW);

	if (funcHandle == NULL)
	{
	    putErrmsg("sppclo can not open shared protocol library.",sharedLibPath);
	    return -1;
	}

	openSPPFunctions(sppcfg,funcHandle);
	/*
	  apid
	  seq_count
	  packet_type
	  sec_header_flag
	*/

	parsed_count = sscanf(spacePacketConfigStr,"%d%*[,]%d%*[,]%d%*[,]%d",&apid,&seq_count,&packet_type,&sec_header_flag);

	if (parsed_count != 4 || parsed_count == 0)
	{
	    putErrmsg("Space Packet Configuration must be four values in the format %d,%d,%d,%d or omitted.",sharedLibPath);
	    return -1;
	}
	
	if (ductBPConfig == NULL)
	{
	    ancillaryData.flags |= BP_BEST_EFFORT;
	}

	if (bpAttach() < 0)
	{
		putErrmsg("sppclo can't attach to BP.", NULL);
		return -1;
	}

	buffer = MTAKE(SPPCLA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("No memory for SPP buffer in sppclo.", NULL);
		return -1;
	}

	findOutduct("spp", endpointSpec, &vduct, &vductElt);

	if (vductElt == 0)
	{
	    putErrmsg("No such spp duct.",endpointSpec);
		MRELEASE(buffer);
		return -1;
	}

	if (vduct->cloPid != ERROR && vduct->cloPid != sm_TaskIdSelf())
	{
		putErrmsg("CLO task is already started for this duct.",
				itoa(vduct->cloPid));
		MRELEASE(buffer);
		return -1;
	}

	/*	All command-line arguments are now validated.		*/

	

	//neighbor = NULL;
	sdr = getIonsdr();
	CHKZERO(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &outduct, sdr_list_data(sdr, vduct->outductElt),
			sizeof(Outduct));
	
	if (outduct.planDuctListElt)
	{
		planDuctList = sdr_list_list(sdr, outduct.planDuctListElt);
		planObj = sdr_list_user_data(sdr, planDuctList);
		if (planObj)
		{
			sdr_read(sdr, (char *) &plan, planObj, sizeof(BpPlan));
		}
	}

	sdr_exit_xn(sdr);

	/*	Set up signal handling.  SIGTERM is shutdown signal.	*/

	oK(sppcloSemaphore(&(vduct->semaphore)));
	isignal(SIGTERM, shutDownClo);

	/*	Can now begin transmitting to remote duct.		*/

	{
		char	memoBuf[1024];

		isprintf(memoBuf, sizeof(memoBuf),
				"[i] sppclo is running, spec = '%s'",
				endpointSpec);
		writeMemo(memoBuf);
	}

	while (!(sm_SemEnded(vduct->semaphore)))
	{
		if (bpDequeue(vduct, &bundleZco, &ancillaryData, 0) < 0)
		{
			putErrmsg("Can't dequeue bundle.", NULL);
			break;
		}

		if (bundleZco == 0)	/*	Outduct closed.		*/
		{
			writeMemo("[i] sppclo outduct closed.");
			sm_SemEnd(sppcloSemaphore(NULL));/*	Stop.	*/
			continue;
		}

		if (bundleZco == 1)	/*	Got a corrupt bundle.	*/
		{
			continue;	/*	Get next bundle.	*/
		}

		CHKZERO(sdr_begin_xn(sdr));
		bundleLength = zco_length(sdr, bundleZco);
		sdr_exit_xn(sdr);
		/* put sendBundleBySPP here */
		{
		    char	memoBuf[1024];

		    isprintf(memoBuf, sizeof(memoBuf),
			     "[i] sending bundle or trying to sppclo",
			     endpointSpec);
		    writeMemo(memoBuf);
		}

		if (sendBundleBySPP(bundleLength,bundleZco,buffer,sppcfg) < -1)
		{
		    putErrmsg("Unable to sendBundleBySPP",NULL);
		    return -1;
		}

		/* Remove this and add in a function call to mark bundles as abandoned*/
		if (bytesSent < bundleLength)
		{
			sm_SemEnd(sppcloSemaphore(NULL));/*	Stop.	*/
			continue;
		}

		/* Take out rate control for now... */


		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}
	if (funcHandle != NULL)
	{
	    dlclose(funcHandle);
	}
	writeErrmsgMemos();
	writeMemo("[i] sppclo duct has ended.");
	MRELEASE(buffer);
	ionDetach();
	return 0;
}
