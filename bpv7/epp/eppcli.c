/*
	eppcli.c:	BP EPP-based convergence-layer input
			daemon, designed to serve as an input
			duct.

	Author: Gregory Miles

	Copyright (c) 2025, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

									*/
#include "eppcla.h"
#include "ipnfw.h"
#include "dtn2fw.h"
#include <dlfcn.h>

/*	*	*	Main thread functions	*	*	*	*/

typedef struct
{
	VInduct			*vduct;
	int			running;
	encapsulation_indication_ptr	encapsulation_indication;
} ReceiverThreadParms;

static void	interruptThread(int signum)
{
	/* Tell the compiler that we are not using 'signum' */
	(void)signum;

	isignal(SIGTERM, interruptThread);
	ionKillMainThread("eppcli");
}

static void	*handleEncapsulationPackets(void *parm)
{
	/*	Main loop for EPP packet reception and handling.	*/

	ReceiverThreadParms *rtp = (ReceiverThreadParms *) parm;
	char		    *procName = "eppcli";
	AcqWorkArea	    *work;
	char		    *buffer = NULL;
	size_t		     bundleLength;
	int		     received_sdlp_channel;
	int		     received_epi;

	snooze(1);	/*	Let main thread become interruptible.	*/
	work = bpGetAcqArea(rtp->vduct);

	if (work == NULL)
	{
		putErrmsg("eppcli can't get acquisition work area.", NULL);
		ionKillMainThread(procName);
		return NULL;
	}

	buffer = MTAKE(EPPCLA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("eppcli can't get EPP buffer.", NULL);
		ionKillMainThread(procName);
		return NULL;
	}

	while (rtp->running)
	{
		bundleLength = (size_t)rtp->encapsulation_indication(buffer,
				&received_sdlp_channel, &received_epi);
		switch (bundleLength)
		{
		case -1:
			/* FALLTHROUGH */

		case 0:
			putErrmsg("Can't acquire bundle.", NULL);
			ionKillMainThread(procName);

			/* FALLTHROUGH */

		case 1:				/*	Normal stop.	*/
			rtp->running = 0;
			continue;

		default:
			break;			/*	Out of switch.	*/
		}

		/*
		 * Verify the EPI is correct for Bundle Protocol.
		 * Per CCSDS 734.20-O-1, EPI = 4 for BP.
		 */
		if (received_epi != EPP_BP_EPI)
		{
			char	errBuf[256];

			isprintf(errBuf, sizeof(errBuf),
					"Received EPP packet with unexpected "
					"EPI %d (expected %d), discarding.",
					received_epi, EPP_BP_EPI);
			writeMemo(errBuf);
			continue;
		}

		if (bpBeginAcq(work, 0, NULL) < 0
		|| bpContinueAcq(work, buffer, bundleLength, 0, 0) < 0
		|| bpEndAcq(work) < 0)
		{
			putErrmsg("Can't acquire bundle.", NULL);
			ionKillMainThread(procName);
			rtp->running = 0;
			continue;
		}

		sm_TaskYield();
	}

	writeErrmsgMemos();
	writeMemo("[i] eppcli receiver thread has ended.");
	bpReleaseAcqArea(work);
	MRELEASE(buffer);

	return NULL;
}

static int openEPPFunctions(ReceiverThreadParms *rtp, void *handle)
{

	/* Union to safely convert the pointer types per strict C standard */
	union {
		void *obj_ptr;
		encapsulation_indication_ptr func_ptr;
	} converter;

	/* Step 1: Get the generic object pointer from dlsym. */
	converter.obj_ptr = dlsym(handle, "encapsulation_indication");

	/* Step 2: Read the same memory as a function pointer. */
	rtp->encapsulation_indication = converter.func_ptr;

	if (rtp->encapsulation_indication == NULL)
	{
		putErrmsg("Could not find encapsulation_indication function.",
				dlerror());
		return -1;
	}

	return 0;
}

#if defined (ION_LWT)
int	eppcli(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	                *ductName = (char *)a1;
	char                    *sharedLibPath = (char *) a2;
#else
int	main(int argc, char *argv[])
{
	char 			*ductName = (argc > 1 ? argv[1] : NULL);
	char 			*sharedLibPath = (argc > 2 ? argv[2] : NULL);
#endif
	VInduct			*vduct;
	PsmAddress		vductElt;
	Sdr			sdr;
	Induct			duct;
	ClProtocol		protocol;
	ReceiverThreadParms	rtp;
	pthread_t		receiverThread;
	void                    *funcHandle = NULL;

	/* Argument Check and Usage Message */
	if (ductName == NULL || sharedLibPath == NULL)
	{
		PUTS("Usage: eppcli <duct_name> <shared_library_path>");
		return 0;
	}

	if (bpAttach() < 0)
	{
		putErrmsg("eppcli can't attach to BP.", NULL);
		return -1;
	}

	findInduct("epp", ductName, &vduct, &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("No such epp duct.", ductName);
		return -1;
	}

	funcHandle = dlopen(sharedLibPath, RTLD_NOW);
	if (!funcHandle)
	{
		putErrmsg("Error opening dlopen.", dlerror());
		return -1;
	}

	if (openEPPFunctions(&rtp, funcHandle) != 0)
	{
		putErrmsg("eppcli could not link to required EPP functions.",
				NULL);
		dlclose(funcHandle);
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

	if (vduct->cliPid != ERROR && vduct->cliPid != sm_TaskIdSelf())
	{
		putErrmsg("CLI task is already started for this duct.",
				itoa(vduct->cliPid));
		dlclose(funcHandle);
		return -1;
	}

	/*	All command-line arguments are now validated.		*/

	sdr = getIonsdr();
	CHKZERO(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &duct, sdr_list_data(sdr, vduct->inductElt),
			sizeof(Induct));
	sdr_read(sdr, (char *) &protocol, duct.protocol, sizeof(ClProtocol));
	sdr_exit_xn(sdr);

	rtp.vduct = vduct;


	/*	Set up signal handling; SIGTERM is shutdown signal.	*/

	ionNoteMainThread("eppcli");
	isignal(SIGTERM, interruptThread);

	/*	Start the receiver thread.				*/
	rtp.running = 1;

	if (pthread_begin(&receiverThread, NULL, handleEncapsulationPackets,
			&rtp))
	{
		putSysErrmsg("eppcli can't create receiver thread", NULL);
		dlclose(funcHandle);
		return -1;
	}

	{
		char	txt[500];

		isprintf(txt, sizeof(txt),
			"[i] eppcli is running for duct '%s'.", ductName);
		writeMemo(txt);
	}

	ionPauseMainThread(-1);

	/*	Time to shut down.					*/

	rtp.running = 0;

	pthread_join(receiverThread, NULL);

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
	writeMemo("[i] eppcli duct has ended.");
	ionDetach();
	return 0;
}
