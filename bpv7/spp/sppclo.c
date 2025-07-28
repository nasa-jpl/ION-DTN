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

static int openSharedLibrary(char* sharedLibPath, void* handle)
{
    handle = dlopen(sharedLibPath, RTLD_LAZY);
    if (!handle)
    {
	putErrmsg("Error opening dlopen.", dlerror());
	return -1;
    }
    return 0;
}

static int openSPPFunctions(struct SppConfig* sppconfig,void *handle)
{

    // load two functions, one to build space packet and  packet request
    // any configuration will be handled by service provider
    sppconfig->build_space_packet = (build_space_packet_ptr)dlsym(handle,"build_space_packet");
    sppconfig->packet_request = (packet_request_ptr)dlsym(handle,"packet_request");
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
	char			*endpointSpec = (char *) a1;
	char                    *sharedLibPath = (char *) a2;
	char                    *spacePacketConfigStr = (char *)a2;
	char			*ductBPConfig = (char *) a3;
#else
int	main(int argc, char *argv[])
{
	char			*endpointSpec = (argc > 1 ? argv[1] : NULL);
	char                    *sharedLibPath = (argc > 2 ? argv[2] : NULL);
	char                    *spacePacketConfigStr = (argc > 3 ? argv[3] : NULL);
	char			*ductBPConfig = (argc > 4 ? argv[4] : NULL);
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
	int                     parsed_count;
	int                     apid = 123;
	int                     seq_count = 0;
	int                     packet_type = 0;
	int                     sec_header_flag = 0;
	struct SppConfig *sppcfg;
	struct SppConfig sppcfgdefaults = {123,0,0,0,NULL,NULL};
	sppcfg = &sppcfgdefaults;

	if (endpointSpec == NULL || sharedLibPath == NULL)
	{
	    PUTS("Usage: sppclo {<remote node IPN> | \
@} {shared library path} {space packet config} [:<BP reliability]");
	    return 0;
	}
	
	if (openSharedLibrary(sharedLibPath, funcHandle) == -1)
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

	writeErrmsgMemos();
	writeMemo("[i] sppclo duct has ended.");
	MRELEASE(buffer);
	ionDetach();
	return 0;
}
