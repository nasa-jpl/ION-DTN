/*

	bsl.h:	definition of the ION public structures and functions
		for accessing BPSec library functionality.  Exposed
		to the BPA.

	Copyright (c) 2025, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

	Author: Scott Burleigh
									*/
#ifndef BSL_H
#define BSL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "platform.h" // IWYU pragma: keep

#include <pthread.h>

#include <bsl/BPSecLib_Public.h>

/*	This structure holds the state of one of the four stages of
 *	security processing performed at this BP node - that is, the
 *	four BPA/BSL interaction points ("locations") at which BSL
 *	processing is performed by this process's BSL agent.		*/

typedef struct BslContext_s
{
	struct BSLP_PolicyProvider_s *policy;
	BSL_LibCtx_t		     *bsl;
	pthread_mutex_t		      mutex;
} BslContext;

/*	This structure holds the state of the BSL agent configured
 *	for this BP-enabled process.					*/

typedef struct BslAgent_s
{
	BSL_HostEID_t	app_eid;
	BSL_HostEID_t	sec_eid;

	/*	Terminology map:
	 *		location ==	bundle processing stage
	 *
	 *		APPIN ==	transmit
	 *		APPOUT ==	deliver
	 *		CLIN ==		receive
	 *		CLOUT ==	forward				*/

	BslContext	transmit;	/*	Bundle rec'd from app.	*/
	BslContext	forward;	/*	Bundle sent to CLA.	*/
	BslContext	receive;	/*	Bundle rec'd from CLA.	*/
	BslContext	deliver;	/*	Bundle sent to app.	*/

	int		initialized;	/*	Boolean.		*/
	int		keystoreReady;	/*	Boolean.		*/
} BslAgent;

/*		bsl functions accessible from the host process		*/

int	bslInitialize(BslAgent *agent);
int	bslProcess(BslAgent *agent, BslContext *ctx, BSL_PolicyLocation_e loc,
		void *workArea);
void	bslCleanup(BslAgent *agent);

#ifdef __cplusplus
}
#endif

#endif /* BSL_H */
