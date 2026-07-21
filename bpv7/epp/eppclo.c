/*
	eppclo.c:	BP Encapsulation Packet Protocol-based convergence-layer
			output daemon.

	Author: Gregory Miles JPL

	Copyright (c) 2025, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

									*/
#include "eppcla.h"
#include <dlfcn.h>

static sm_SemId		eppcloSemaphore(sm_SemId *semid)
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

	sm_SemEnd(eppcloSemaphore(NULL));
}


static int openEPPFunctions(struct EppConfig *eppcfg, void *handle)
{

	char *error = NULL;

	dlerror(); /* Clear existing error */

	/* Look up the init function */
	*(void **) (&eppcfg->init_sender) = dlsym(handle, "init_epp_sender");
	if ((error = dlerror()) != NULL)
	{
		fprintf(stderr, "dlsym error for init_epp_sender: %s\n", error);
		dlclose(handle);
		return -1;
	}

	/* Look up the finalize function */
	*(void **) (&eppcfg->finalize_sender) = dlsym(handle,
			"finalize_epp_sender");
	if ((error = dlerror()) != NULL)
	{
		fprintf(stderr, "dlsym error for finalize_epp_sender: %s\n",
				error);
		dlclose(handle);
		return -1;
	}

	/* Look up the encapsulation_request function */
	*(void **) (&eppcfg->encapsulation_request) = dlsym(handle,
			"encapsulation_request");
	if ((error = dlerror()) != NULL)
	{
		fprintf(stderr, "dlsym error for encapsulation_request: %s\n",
				error);
		dlclose(handle);
		return -1;
	}

	return 0;
}

/*	*	*	Main thread functions	*	*	*	*/

#if defined (ION_LWT)
int	eppclo(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char                    *endpointSpec = (char *)a1;
	char                    *sharedLibPath = (char *)a2;
	char                    *sdlpChannelStr = (char *)a3;
#else
int	main(int argc, char *argv[])
{
	char                    *endpointSpec = (argc > 1 ? argv[1] : NULL);
	char                    *sharedLibPath = (argc > 2 ? argv[2] : NULL);
	char                    *sdlpChannelStr = (argc > 3 ? argv[3] : NULL);
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
	int                     sdlp_channel = 0;
	struct EppConfig	*eppcfg;
	struct EppConfig	eppcfgdefaults = {0, EPP_BP_EPI, NULL, NULL,
					NULL};
	eppcfg = &eppcfgdefaults;

	/* Argument Check and Usage Message */
	if (endpointSpec == NULL || sharedLibPath == NULL
			|| sdlpChannelStr == NULL)
	{
		PUTS("Usage: eppclo <endpoint_spec> <shared_library_path> "
				"<sdlp_channel>");
		return 0;
	}

	if (sscanf(sdlpChannelStr, "%d", &sdlp_channel) != 1)
	{
		putErrmsg("SDLP channel must be an integer.", sdlpChannelStr);
		return -1;
	}

	eppcfg->sdlp_channel = sdlp_channel;

	/* Open EPP provider library */
	funcHandle = dlopen(sharedLibPath, RTLD_NOW);
	if (funcHandle == NULL)
	{
		putErrmsg("eppclo can not open shared protocol library.",
				sharedLibPath);
		return -1;
	}

	if (openEPPFunctions(eppcfg, funcHandle) != 0)
	{
		putErrmsg("eppclo could not link to all required EPP functions.",
				NULL);
		return -1;
	}

	/* Call provider initialization function if present. */
	typedef void (*init_func_t)(void);
	init_func_t init_func;
	void *init_sym = dlsym(funcHandle, "epp_provider_init");
	memcpy(&init_func, &init_sym, sizeof(init_sym));
	if (init_func)
	{
		init_func();
	}

	if (bpAttach() < 0)
	{
		putErrmsg("eppclo can't attach to BP.", NULL);
		return -1;
	}

	buffer = MTAKE(EPPCLA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("No memory for EPP buffer in eppclo.", NULL);
		return -1;
	}

	findOutduct("epp", endpointSpec, &vduct, &vductElt);

	if (vductElt == 0)
	{
		putErrmsg("No such epp duct.", endpointSpec);
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

	/* Call the init function pointer before starting the main loop. */
	eppcfg->init_sender();

	/*	Set up signal handling.  SIGTERM is shutdown signal.	*/
	oK(eppcloSemaphore(&(vduct->semaphore)));
	isignal(SIGTERM, shutDownClo);

	/*	Can now begin transmitting to remote duct.		*/

	{
		char	memoBuf[1024];

		isprintf(memoBuf, sizeof(memoBuf),
				"[i] eppclo is running, spec = '%s', "
				"sdlp_channel = %d",
				endpointSpec, sdlp_channel);
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
			writeMemo("[i] eppclo outduct closed.");
			sm_SemEnd(eppcloSemaphore(NULL));/*	Stop.	*/
			continue;
		}

		if (bundleZco == 1)	/*	Got a corrupt bundle.	*/
		{
			continue;	/*	Get next bundle.	*/
		}

		CHKZERO(sdr_begin_xn(sdr));
		bundleLength = zco_length(sdr, bundleZco);
		sdr_exit_xn(sdr);

		if ((bytesSent = sendBundleByEPP(bundleLength, bundleZco,
				buffer, eppcfg)) < 0)
		{
			putErrmsg("Unable to sendBundleByEPP", NULL);
			sm_SemEnd(eppcloSemaphore(NULL)); /*	Stop.	*/
			continue;
		}

		/*	Make sure other tasks have a chance to run.	*/
		sm_TaskYield();
	}

	/* Call the finalize function pointer after the loop ends. */
	eppcfg->finalize_sender();

	/* Call provider cleanup function if present. */
	typedef void (*cleanup_func_t)(void);
	cleanup_func_t cleanup_func;
	void	      *cleanup_sym = dlsym(funcHandle, "epp_provider_cleanup");
	memcpy(&cleanup_func, &cleanup_sym, sizeof(cleanup_sym));
	if (cleanup_func)
	{
		cleanup_func();
	}

	dlclose(funcHandle);

	writeErrmsgMemos();
	writeMemo("[i] eppclo duct has ended.");
	MRELEASE(buffer);
	ionDetach();
	return 0;
}
