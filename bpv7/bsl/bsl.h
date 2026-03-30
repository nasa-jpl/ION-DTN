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
//#include <SamplePolicyProvider.h>
//#include <DefaultSecContext.h>
//#include <DefaultSecContext_Private.h>
//#include <rfc9173.h>

/*	Needed?		*/
#include <inttypes.h>
#include <jansson.h>
/*	*	*	*/

#include <policy_provider/SamplePolicyProvider.h>
#include <security_context/rfc9173.h>

/*		---- BSL data structure summary ----
 *
 *	Each process using BP has a BSL AGENT.
 *
 *	An Agent retain's the process's application EID and
 *	security EID and it encapsulates four operational BSL
 *	CONTEXTS, i.e., policy context definitions - one for
 *	each BSL "location", that is, one for each of the four
 *	stages of bundle security processing.  
 *
 *	Each of the BSL contexts of an agent has a security POLICY
 *	(a.k.a. "policy provider") and also contains LIBRARY CONTEXT
 *	information used in enacting the corresponding stage of
 *	bundle security processing per that policy.
 *
 *	Each security policy includes an ID number (a PP ID) and
 *	comprises a list of policy invocation PREDICATES and a
 *	list of RULES (referencing those predicates) that are
 *	specific to that BSL context's processing stage.  
 *
 *	Each predicate indicates the source node(s), security source
 *	node(s), and destination node(s) to which the rule applies).
 *
 *	Each rule contains a reference to the governing predicate,
 *	a pointer to the applicable security context, and indications
 *	of the security operations role to which the rule applies,
 *	the affected security block type, the affected target block
 *	type, the applicable set of security parameter values, and
 *	the action that must be taken when this rule is found to
 *	apply to the bundle whose security is being investigated.
 *
 *	For implementation convenience, the Agent also has a RULE
 *	INFORMATION CATALOG (a.k.a. "rules registry") containing
 *	an array of sets of applicable security PARAMETER values
 *	that are needed in order to execute the various security
 *	rules. Each entry in the catalog is a set of 7 security
 *	policy parameter values.  There is one entry in the catalog
 *	for each rule in the security policy - and one rule for
 *	each active entry in the catalog - so the maximum length
 *	of the catalog is the limit on the number of rules that
 *	may be declared in a policy.
 *
 *	The library context of a BSL context consists of a policy
 *	registry and a security context registry.  Each entry in
 *	the policy registry contains a pointer to the BSL context's
 *	security policy together with three function pointers used
 *	in enacting that policy.  Each entry in the security context
 *	registry contains a pointer to the security context that
 *	will be applied in the enactment of the BSL context's
 *	security policy.						*/

#define MAX_RULES	100

typedef struct rule_params_s
{
	bool		active;

	/*		Params related to BIB				*/
	BSL_SecParam_t	*param_integ_scope_flag;
	BSL_SecParam_t	*param_sha_variant;

	/*		Params related to BCB				*/
	BSL_SecParam_t	*param_aad_scope_flag;
	BSL_SecParam_t	*param_init_vector;
	BSL_SecParam_t	*param_aes_variant;
	BSL_SecParam_t	*param_use_wrapped_key;

	/*		Params agnostic to BIB vs BCB			*/
	BSL_SecParam_t	*param_test_key;
} rule_params;

/*	Each BSL agent has a single catalog of rule information items.
 *	This catalog ("rules registry") does not contain the rules
 *	themselves; the rules are managed within BSL's PolicyRule
 *	objects residing in the policies of BSL contexts.  The
 *	catalog simply indicates which previously acquired security
 *	policy rules are currently in use, but additionally it contains
 *	an array of the rule_params objects containing the references
 *	to the parameter values that support each security policy rule.
 *	Identification numbers assigned to the rules during rule
 *	acquisition are used to index the "in use" indication array
 *	and the array of policy-specific parameter value reference
 *	sets.								*/

typedef struct rules_registry_s
{
	rule_params	param_sets[MAX_RULES];
	bool		in_use[MAX_RULES];
	int		registry_count;
} rules_registry;

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
	rules_registry	rulesRegistry;

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
