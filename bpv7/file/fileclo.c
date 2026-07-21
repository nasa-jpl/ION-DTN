/*
	fileclo.c:	BP File-based convergence-layer output daemon.

	Author: ION Development Team

	Copyright (c) 2025, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

								*/
#include "filecla.h"
#include <dlfcn.h>

static sm_SemId		filecloSemaphore(sm_SemId *semid)
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

	sm_SemEnd(filecloSemaphore(NULL));
}


static int openFileFunctions(struct FileCloConfig *cfg, void *handle)
{
	char *error = NULL;

	dlerror(); /* Clear existing error */

	/* Look up the init function */
	*(void **) (&cfg->init_sender) = dlsym(handle, "init_file_sender");
	if ((error = dlerror()) != NULL)
	{
		fprintf(stderr, "dlsym error for init_file_sender: %s\n", error);
		dlclose(handle);
		return -1;
	}

	/* Look up the finalize function */
	*(void **) (&cfg->finalize_sender) = dlsym(handle,
			"finalize_file_sender");
	if ((error = dlerror()) != NULL)
	{
		fprintf(stderr, "dlsym error for finalize_file_sender: %s\n",
				error);
		dlclose(handle);
		return -1;
	}

	/* Look up the file_write_bundle function */
	*(void **) (&cfg->write_bundle) = dlsym(handle, "file_write_bundle");
	if ((error = dlerror()) != NULL)
	{
		fprintf(stderr, "dlsym error for file_write_bundle: %s\n",
				error);
		dlclose(handle);
		return -1;
	}

	return 0;
}

/*	*	*	Main thread functions	*	*	*	*/

#if defined (ION_LWT)
int	fileclo(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char                    *endpointSpec = (char *)a1;
	char                    *sharedLibPath = (char *)a2;
	char                    *outputFilePath = (char *)a3;
#else
int	main(int argc, char *argv[])
{
	char                    *endpointSpec = (argc > 1 ? argv[1] : NULL);
	char                    *sharedLibPath = (argc > 2 ? argv[2] : NULL);
	char                    *outputFilePath = (argc > 3 ? argv[3] : NULL);
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
	struct FileCloConfig	*cfg;
	struct FileCloConfig	cfgdefaults = {"", NULL, NULL, NULL};
	cfg = &cfgdefaults;

	/* Argument Check and Usage Message */
	if (endpointSpec == NULL || sharedLibPath == NULL
			|| outputFilePath == NULL)
	{
		PUTS("Usage: fileclo <duct_name> <shared_library_path> "
				"<output_file_path>");
		return 0;
	}

	/* Store file path in config */
	strncpy(cfg->file_path, outputFilePath, FILECLA_MAX_PATH - 1);
	cfg->file_path[FILECLA_MAX_PATH - 1] = '\0';

	/* Open file provider library */
	funcHandle = dlopen(sharedLibPath, RTLD_NOW);
	if (funcHandle == NULL)
	{
		putErrmsg("fileclo can not open shared provider library.",
				sharedLibPath);
		putErrmsg("dlopen error", dlerror());
		return -1;
	}

	if (openFileFunctions(cfg, funcHandle) != 0)
	{
		putErrmsg("fileclo could not link to all required file "
				"provider functions.", NULL);
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

	if (bpAttach() < 0)
	{
		putErrmsg("fileclo can't attach to BP.", NULL);
		return -1;
	}

	buffer = MTAKE(FILECLA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("No memory for file buffer in fileclo.", NULL);
		return -1;
	}

	findOutduct("file", endpointSpec, &vduct, &vductElt);

	if (vductElt == 0)
	{
		putErrmsg("No such file duct.", endpointSpec);
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
	if (cfg->init_sender(cfg->file_path) < 0)
	{
		putErrmsg("fileclo failed to initialize file sender.",
				cfg->file_path);
		MRELEASE(buffer);
		dlclose(funcHandle);
		return -1;
	}

	/*	Set up signal handling.  SIGTERM is shutdown signal.	*/
	oK(filecloSemaphore(&(vduct->semaphore)));
	isignal(SIGTERM, shutDownClo);

	/*	Can now begin transmitting to file.		*/

	{
		char	memoBuf[1024];

		isprintf(memoBuf, sizeof(memoBuf),
				"[i] fileclo is running, spec = '%s', "
				"output file = '%s'",
				endpointSpec, cfg->file_path);
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
			writeMemo("[i] fileclo outduct closed.");
			sm_SemEnd(filecloSemaphore(NULL));/*	Stop.	*/
			continue;
		}

		if (bundleZco == 1)	/*	Got a corrupt bundle.	*/
		{
			continue;	/*	Get next bundle.	*/
		}

		CHKZERO(sdr_begin_xn(sdr));
		bundleLength = zco_length(sdr, bundleZco);
		sdr_exit_xn(sdr);

		if ((bytesSent = sendBundleByFile(bundleLength, bundleZco,
				buffer, cfg)) < 0)
		{
			putErrmsg("Unable to sendBundleByFile", NULL);
			sm_SemEnd(filecloSemaphore(NULL)); /*	Stop.	*/
			continue;
		}

		/*	Make sure other tasks have a chance to run.	*/
		sm_TaskYield();
	}

	/* Call the finalize function pointer after the loop ends. */
	cfg->finalize_sender();

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
	writeMemo("[i] fileclo duct has ended.");
	MRELEASE(buffer);
	ionDetach();
	return 0;
}
