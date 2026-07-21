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
	/* Tell the compiler that we are not using 'signum' */
	(void)signum;

	sm_SemEnd(sppcloSemaphore(NULL));
}


static int openSPPFunctions(struct SppConfig *sppconfig, void *handle)
{

	char *error = NULL;

	dlerror(); // Clear existing error

	// Look up the init function
	*(void **) (&sppconfig->init_sender) = dlsym(handle, "init_space_packet_sender");
	if ((error = dlerror()) != NULL)
	{
		fprintf(stderr, "dlsym error for init_space_packet_sender: %s\n", error);
		dlclose(handle);
		return -1;
	}

	// Look up the finalize function
	*(void **) (&sppconfig->finalize_sender) = dlsym(handle, "finalize_space_packet_sender");
	if ((error = dlerror()) != NULL)
	{
		fprintf(stderr, "dlsym error for finalize_space_packet_sender: %s\n", error);
		dlclose(handle);
		return -1;
	}

	// Look up the packet_request function
	*(void **) (&sppconfig->packet_request) = dlsym(handle, "packet_request");
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
	char                    *sharedLibPath = (argc > 2 ? argv[2] : NULL);
	char                    *spacePacketConfigStr = (argc > 3 ? argv[3] : NULL);
#endif
	unsigned char		*buffer;
	VOutduct		*vduct;
	PsmAddress		vductElt;
	Sdr			sdr;
	Outduct			outduct;
	SdrObject		planDuctList;
	SdrObject		planObj = 0;
	BpPlan			plan;
	SdrObject		bundleZco;
	BpAncillaryData		ancillaryData;
	unsigned int		bundleLength;
	int			bytesSent = 0;
	void                    *funcHandle = NULL;
	int                     parsed_count = 0;
	int                     apid = 123;
	int                     seq_count = 0;
	int                     packet_type = 0;
	int                     sec_header_flag = 0;
	struct SppConfig *sppcfg;
	struct SppConfig sppcfgdefaults = {123,0,0,0,NULL,NULL,NULL};
	sppcfg = &sppcfgdefaults;

	if (sharedLibPath != NULL && spacePacketConfigStr != NULL)
		printf("using shared library %s %s\n",sharedLibPath,spacePacketConfigStr);

	// Open SPP library
	funcHandle = dlopen(sharedLibPath, RTLD_NOW);
	if (funcHandle == NULL)
	{
		putErrmsg("sppclo can not open shared protocol library.",sharedLibPath);
		return -1;
	}

	if (openSPPFunctions(sppcfg, funcHandle) != 0)
	{
		putErrmsg("sppclo could not link to all required SPP functions.", NULL);
		return -1;
	}

	/* Call provider initialization function if present. */
	typedef void (*init_func_t)(void);
	init_func_t init_func;
	void *init_sym = dlsym(funcHandle, "spp_provider_init");
	memcpy(&init_func, &init_sym, sizeof(init_sym));
	if (init_func)
	{
		init_func();
	}

	parsed_count = sscanf(spacePacketConfigStr,"%d%*[,]%d%*[,]%d%*[,]%d",&apid,&seq_count,&packet_type,&sec_header_flag);

	if (parsed_count != 4 || parsed_count == 0)
	{
		putErrmsg("Space Packet Configuration must be four values in the format %d,%d,%d,%d or omitted.",sharedLibPath);
		return -1;
	}

	sppcfg->apid = apid;
	sppcfg->seq_count = seq_count;
	sppcfg->sec_header_flag = sec_header_flag;
	sppcfg->packet_type = packet_type;

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

	// Call the init function pointer before starting the main loop.
	sppcfg->init_sender();

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

		if ((bytesSent = sendBundleBySPP(bundleLength,bundleZco,buffer,sppcfg)) < 0)
		{
			putErrmsg("Unable to sendBundleBySPP",NULL);
			sm_SemEnd(sppcloSemaphore(NULL)); /*	Stop.	*/
			continue;
		}

		// Increment the sequence count for the next packet.
		sppcfg->seq_count++;

		// Check if the sequence count has exceeded the 14-bit limit.
		if (sppcfg->seq_count > SPP_MAX_SEQ_COUNT)
		{
			// Reset to 0 and log the wrap-around.
			sppcfg->seq_count = 0;
			writeMemo("[i] SPP sequence count wrapped around to 0.");
		}

		/* Remove this and add in a function call to mark bundles as abandoned*/
		if (bytesSent < 0 || (unsigned int)bytesSent < bundleLength)
		{
			sm_SemEnd(sppcloSemaphore(NULL));/*	Stop.	*/
			continue;
		}

		/*	Make sure other tasks have a chance to run.	*/
		sm_TaskYield();
	}

	// Call the finalize function pointer after the loop ends.
	sppcfg->finalize_sender();

	/* Call provider cleanup function if present. */
	typedef void (*cleanup_func_t)(void);
	cleanup_func_t cleanup_func;
	void	      *cleanup_sym = dlsym(funcHandle, "spp_provider_cleanup");
	memcpy(&cleanup_func, &cleanup_sym, sizeof(cleanup_sym));
	if (cleanup_func)
	{
		cleanup_func();
	}

	dlclose(funcHandle);

	writeErrmsgMemos();
	writeMemo("[i] sppclo duct has ended.");
	MRELEASE(buffer);
	ionDetach();
	return 0;
}
