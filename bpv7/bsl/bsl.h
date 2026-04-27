/*

	bsl.h:	definition of the ION public structures and functions
		for accessing BPSec library functionality.  Exposed
		to the BPA.

	Copyright (c) 2025, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

	Author: Scott Burleigh
									*/
#ifndef _BSL_H_
#define _BSL_H_

#ifdef __cplusplus
extern "C" {
#endif

/*	Temporarily undefine ION's check macros (from platform.h) to
 *	allow BSL headers to define their own versions without error.	*/
#ifdef CHKVOID
#undef CHKVOID
#endif
#ifdef CHKNULL
#undef CHKNULL
#endif

#include <BPSecLib_Private.h>
#include <BPSecLib_Public.h>

/*	Immediately undefine BSL's deprecated versions.		*/
#undef CHKVOID
#undef CHKNULL

/*	Redefine using ION's versions (copied from platform.h).	*/
#define CHKVOID(e)    if (!(e) && (iEnd(#e)||1)) return
#define CHKNULL(e)    if (!(e) && (iEnd(#e)||1)) return NULL
#include <CryptoInterface.h>
#include <policy_provider/SamplePolicyProvider.h>
#include <policy_provider/SamplePolicyConfigParser.h>
#include <security_context/rfc9173.h>

/*	This structure holds the state of one of the four stages of
 *	security processing performed at this BP node - that is, the
 *	four BPA/BSL interaction points ("locations") at which BSL
 *	processing is performed by this process's BSL agent.		*/

typedef struct BslContext_s
{
	BSLP_PolicyProvider_t	*policy;
	BSL_LibCtx_t		*bsl;
	pthread_mutex_t		mutex;
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
} BslAgent;

/*		bsl functions accessible from the host process		*/

int	bslInitialize(BslAgent *agent);
int	bslProcess(BslAgent *agent, BslContext *ctx, BSL_PolicyLocation_e loc,
		void *workArea);
void	bslCleanup(BslAgent *agent);

#ifdef __cplusplus
}
#endif

#endif  /* _BSL_H_ */
