/*
	sppcli.c:	BP SPP-based convergence-layer input
			daemon, designed to serve as an input
			duct.

	Author: Gregory Miles

	Copyright (c) 2025, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

									*/
#include "sppcla.h"
#include "ipnfw.h"
#include "dtn2fw.h"
#include <dlfcn.h>

// static void	*handleSpacePackets(void *parm)
// {
// 	char *buffer;
// 	int   bundleLength;
//
// 	return 0;
// }

/*	*	*	Main thread functions	*	*	*	*/
typedef size_t (*packet_recv_ptr)(char*,int*);

typedef struct
{
	VInduct		*vduct;
	int		running;
	packet_recv_ptr packet_indication;
} ReceiverThreadParms;

static void	interruptThread(int signum)
{
	/* Tell the compiler that we are not using 'signum' */
	(void)signum;

	isignal(SIGTERM, interruptThread);
	ionKillMainThread("sppcli");
}

static void	*handleSpacePackets(void *parm)
{
	/*	Main loop for UDP datagram reception and handling.	*/

	ReceiverThreadParms *rtp = (ReceiverThreadParms *) parm;
	char		    *procName = "sppcli";
	AcqWorkArea	    *work;
	char		    *buffer = NULL;
	size_t		     bundleLength;
	int		     received_apid;

	snooze(1);	/*	Let main thread become interruptible.	*/
	work = bpGetAcqArea(rtp->vduct);

	if (work == NULL)
	{
		putErrmsg("sppcli can't get acquisition work area.", NULL);
		ionKillMainThread(procName);
		return NULL;
	}

	buffer = MTAKE(SPPCLA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("sppcli can't get SPP buffer.", NULL);
		ionKillMainThread(procName);
		return NULL;
	}

	while (rtp->running)
	{
		bundleLength = (size_t)rtp->packet_indication(buffer, &received_apid);
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
	writeMemo("[i] sppcli receiver thread has ended.");
	bpReleaseAcqArea(work);
	MRELEASE(buffer);

	return NULL;
}

/*
static int openSharedLibrary(char *sharedLibPath, void *handle)
{
	handle = dlopen(sharedLibPath, RTLD_NOW);
	if (!handle)
	{
		putErrmsg("Error opening dlopen.", dlerror());
		return -1;
	}
	return 0;
}
*/

static int openSPPFunctions(ReceiverThreadParms *rtp,void *handle)
{

	/* Union to safely convert the pointer types per strict C standard */
	union {
		void *obj_ptr;
		packet_recv_ptr func_ptr;
	} converter;

	/* Step 1: Get the generic object pointer from dlsym. */
	converter.obj_ptr = dlsym(handle, "packet_indication");

	/* Step 2: Read the same memory as a function pointer. This is standard-compliant. */
	rtp->packet_indication = converter.func_ptr;

	return 0;
}

#if defined (ION_LWT)
int	sppcli(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
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

	// Argument Check and Usage Message
	if (argc < 3)
	{
		PUTS("Usage: sppcli <duct_name> <shared_library_path>");
		return 0;
	}

	if (bpAttach() < 0)
	{
		putErrmsg("sppcli can't attach to BP.", NULL);
		return -1;
	}

	findInduct("spp", ductName, &vduct, &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("No such spp duct.", ductName);
		return -1;
	}

	funcHandle = dlopen(sharedLibPath, RTLD_NOW);
	if (!funcHandle)
	{
		putErrmsg("Error opening dlopen.", dlerror());
		return -1;
	}
	/*
	if (openSharedLibrary(sharedLibPath, funcHandle) == -1)
	{
		putErrmsg("sppcli can not open shared protocol library.",
				sharedLibPath);
		return -1;
	}*/

	openSPPFunctions(&rtp,funcHandle);

	/* Call provider initialization function if present. */
	typedef void (*init_func_t)(void);
	init_func_t init_func;
	void *init_sym = dlsym(funcHandle, "spp_provider_init");
	memcpy(&init_func, &init_sym, sizeof(init_sym));
	if (init_func)
	{
		init_func();
	}

	if (vduct->cliPid != ERROR && vduct->cliPid != sm_TaskIdSelf())
	{
		putErrmsg("CLI task is already started for this duct.",
				itoa(vduct->cliPid));
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

	ionNoteMainThread("sppcli");
	isignal(SIGTERM, interruptThread);

	/*	Start the receiver thread.				*/
	rtp.running = 1;

	if (pthread_begin(&receiverThread, NULL, handleSpacePackets, &rtp))
	{
		putSysErrmsg("sppcli can't create receiver thread", NULL);
		return -1;
	}

	{
		char	txt[500];

		isprintf(txt, sizeof(txt),
			"[i] sppcli is running for duct '%s'.", ductName);
		writeMemo(txt);
	}

	ionPauseMainThread(-1);

	/*	Time to shut down.					*/

	rtp.running = 0;

	pthread_join(receiverThread, NULL);

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
	writeMemo("[i] sppcli duct has ended.");
	ionDetach();
	return 0;
}
