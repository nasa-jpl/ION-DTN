/*
	filecli.c:	BP File-based convergence-layer input
			daemon, designed to serve as an input
			duct.

	Author: ION Development Team

	Copyright (c) 2025, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

								*/
#include "filecla.h"
#include "ipnfw.h"
#include "dtn2fw.h"
#include <dlfcn.h>

/*	*	*	Main thread functions	*	*	*	*/

typedef struct
{
	VInduct			*vduct;
	int			running;
	file_read_bundle_ptr	read_bundle;
} ReceiverThreadParms;

static void	interruptThread(int signum)
{
	/* Tell the compiler that we are not using 'signum' */
	(void)signum;

	isignal(SIGTERM, interruptThread);
	ionKillMainThread("filecli");
}

static void	*handleFileBundles(void *parm)
{
	/*	Main loop for file bundle reception and handling.	*/

	ReceiverThreadParms *rtp = (ReceiverThreadParms *) parm;
	char		    *procName = "filecli";
	AcqWorkArea	    *work;
	char		    *buffer = NULL;
	size_t		     bundleLength;

	snooze(1);	/*	Let main thread become interruptible.	*/
	work = bpGetAcqArea(rtp->vduct);

	if (work == NULL)
	{
		putErrmsg("filecli can't get acquisition work area.", NULL);
		ionKillMainThread(procName);
		return NULL;
	}

	buffer = MTAKE(FILECLA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("filecli can't get file buffer.", NULL);
		ionKillMainThread(procName);
		return NULL;
	}

	while (rtp->running)
	{
		bundleLength = (size_t)rtp->read_bundle(buffer);
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
	writeMemo("[i] filecli receiver thread has ended.");
	bpReleaseAcqArea(work);
	MRELEASE(buffer);

	return NULL;
}

static int openFileFunctions(struct FileCliConfig *cfg,
		ReceiverThreadParms *rtp, void *handle)
{
	char *error = NULL;

	/* Union to safely convert the pointer types per strict C standard */
	union {
		void *obj_ptr;
		file_read_bundle_ptr func_ptr;
	} converter;

	union {
		void *obj_ptr;
		init_file_receiver_ptr func_ptr;
	} init_converter;

	union {
		void *obj_ptr;
		finalize_file_receiver_ptr func_ptr;
	} finalize_converter;

	dlerror(); /* Clear existing error */

	/* Look up init_file_receiver */
	init_converter.obj_ptr = dlsym(handle, "init_file_receiver");
	cfg->init_receiver = init_converter.func_ptr;
	if ((error = dlerror()) != NULL)
	{
		fprintf(stderr, "dlsym error for init_file_receiver: %s\n",
				error);
		return -1;
	}

	/* Look up finalize_file_receiver */
	finalize_converter.obj_ptr = dlsym(handle, "finalize_file_receiver");
	cfg->finalize_receiver = finalize_converter.func_ptr;
	if ((error = dlerror()) != NULL)
	{
		fprintf(stderr, "dlsym error for finalize_file_receiver: %s\n",
				error);
		return -1;
	}

	/* Look up file_read_bundle */
	converter.obj_ptr = dlsym(handle, "file_read_bundle");
	rtp->read_bundle = converter.func_ptr;
	cfg->read_bundle = converter.func_ptr;
	if ((error = dlerror()) != NULL)
	{
		fprintf(stderr, "dlsym error for file_read_bundle: %s\n",
				error);
		return -1;
	}

	return 0;
}

#if defined (ION_LWT)
int	filecli(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	                *ductName = (char *)a1;
	char                    *sharedLibPath = (char *) a2;
	char                    *inputFilePath = (char *) a3;
	char                    *pollIntervalStr = (char *) a4;
#else
int	main(int argc, char *argv[])
{
	char 			*ductName = (argc > 1 ? argv[1] : NULL);
	char 			*sharedLibPath = (argc > 2 ? argv[2] : NULL);
	char			*inputFilePath = (argc > 3 ? argv[3] : NULL);
	char			*pollIntervalStr = (argc > 4 ? argv[4] : NULL);
#endif
	VInduct			*vduct;
	PsmAddress		vductElt;
	Sdr			sdr;
	Induct			duct;
	ClProtocol		protocol;
	ReceiverThreadParms	rtp;
	pthread_t		receiverThread;
	void                    *funcHandle = NULL;
	struct FileCliConfig	cfg;
	int			pollInterval = 1000; /* default 1 second */

	memset(&cfg, 0, sizeof(cfg));

	/* Argument Check and Usage Message */
	if (ductName == NULL || sharedLibPath == NULL || inputFilePath == NULL)
	{
		PUTS("Usage: filecli <duct_name> <shared_library_path> "
				"<input_file_path> [poll_interval_ms]");
		return 0;
	}

	/* Store file path in config */
	strncpy(cfg.file_path, inputFilePath, FILECLA_MAX_PATH - 1);
	cfg.file_path[FILECLA_MAX_PATH - 1] = '\0';

	/* Parse optional poll interval */
	if (pollIntervalStr != NULL)
	{
		if (sscanf(pollIntervalStr, "%d", &pollInterval) != 1)
		{
			putErrmsg("Poll interval must be an integer.",
					pollIntervalStr);
			return -1;
		}
	}
	cfg.poll_interval = pollInterval;

	if (bpAttach() < 0)
	{
		putErrmsg("filecli can't attach to BP.", NULL);
		return -1;
	}

	findInduct("file", ductName, &vduct, &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("No such file duct.", ductName);
		return -1;
	}

	funcHandle = dlopen(sharedLibPath, RTLD_NOW);
	if (!funcHandle)
	{
		putErrmsg("Error opening dlopen.", dlerror());
		return -1;
	}

	if (openFileFunctions(&cfg, &rtp, funcHandle) != 0)
	{
		putErrmsg("filecli could not link to required file provider "
				"functions.", NULL);
		dlclose(funcHandle);
		return -1;
	}

	/* Call provider initialization function if present. */
	typedef void (*init_func_t)(void);
	init_func_t init_func;
	void *init_sym = dlsym(funcHandle, "file_provider_init");
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

	/* Initialize the file receiver */
	if (cfg.init_receiver(cfg.file_path, cfg.poll_interval) < 0)
	{
		putErrmsg("filecli failed to initialize file receiver.",
				cfg.file_path);
		dlclose(funcHandle);
		return -1;
	}

	rtp.vduct = vduct;

	/*	Set up signal handling; SIGTERM is shutdown signal.	*/

	ionNoteMainThread("filecli");
	isignal(SIGTERM, interruptThread);

	/*	Start the receiver thread.				*/
	rtp.running = 1;

	if (pthread_begin(&receiverThread, NULL, handleFileBundles, &rtp))
	{
		putSysErrmsg("filecli can't create receiver thread", NULL);
		cfg.finalize_receiver();
		dlclose(funcHandle);
		return -1;
	}

	{
		char	txt[500];

		isprintf(txt, sizeof(txt),
			"[i] filecli is running for duct '%s', "
			"input file = '%s', poll interval = %d ms.",
			ductName, cfg.file_path, cfg.poll_interval);
		writeMemo(txt);
	}

	ionPauseMainThread(-1);

	/*	Time to shut down.					*/

	rtp.running = 0;

	pthread_join(receiverThread, NULL);

	/* Finalize the file receiver */
	cfg.finalize_receiver();

	/* Call provider cleanup function if present. */
	typedef void (*cleanup_func_t)(void);
	cleanup_func_t cleanup_func;
	void	      *cleanup_sym = dlsym(funcHandle, "file_provider_cleanup");
	memcpy(&cleanup_func, &cleanup_sym, sizeof(cleanup_sym));
	if (cleanup_func)
	{
		cleanup_func();
	}

	dlclose(funcHandle);

	writeErrmsgMemos();
	writeMemo("[i] filecli duct has ended.");
	ionDetach();
	return 0;
}
