/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2021 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: sci.h
 **
 ** Namespace: bpsec_sci_
 **
 ** Description:
 **
 **     The BPSec security protocol used to secure BPv7 bundles defines
 **     the concept of a "security context" used to generate and consume
 **     cryptographic material associated with BPSec security blocks.
 **
 **     The ION Security Context Interface (SCI) provides methods for finding
 **     and using security contexts to perform security block processing.
 **
 ** Notes:
 **
 **     1. This interface does not, itself, implement cryptographic functions.
 **
 **     2. The security context "state" represents the state of a security
 **        context in the process of applying a security service at a node (such
 **        as when encrypting a target block). This state is used under the
 **        auspices of the single thread within the BPA tasked with the
 **        encryption. As such, this state is stored in regular heap memory and
 **        not in shared memory.
 **
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  08/03/21  E. Birrane     Initial implementation
 **
 *****************************************************************************/

#ifndef _SCI_H_
#define _SCI_H_

#include "platform.h"
#include "ion.h"
#include "lyst.h"
#include "bpP.h"
#include "csi.h"

#include "psm.h"
#include "smlist.h"
#include "sci_structs.h"

#include "bpsec_asb.h"



/**
 *
 * Security Context Interface (SCI)
 *
 * The security context interface defines the set of functions that must be
 * implemented by every ION security context instance.  These functions are
 * called as part of various SC-related processing within ION, such as:
 *
 * 1. When defining new policy rules to verify and store SC parameters.
 * 2. When processing a security block at a security verifier or acceptor.
 * 3. When creating security blocks at a security source.
 *
 * The purpose of each function in the SCI is as follows.
 *
 *
 * bpsec_sc_init        - Initialize the security context when the BPA is
 *                        started. This may include reading information
 *                        from disk, allocating buffers, etc... This is NOT
 *                        called per security block invocation - just once
 *                        on startup.
 *
 *
 * bpsec_sc_teardown    - Perform any finalization actions when the local
 *                        BPA is shutting down. This is the "opposite" of
 *                        the bpsec_sc_init function.
 *
 *
 * bpsec_sc_initOutboundASB - Initialize an outbound ASB when creating a new
 *                        security block at a security source. This function
 *                        populated a security block with shared information
 *                        such as the security context ID and the security
 *                        context parameters that should be kept in the
 *                        block.
 *
 *
 * bpsec_sc_stateInit   - Initializes a security context state that will be
 *                        used to process the operations in a security block.
 *                        This updates the state with the parameters and other
 *                        block-wide content, to include the number of
 *                        operations being processed by the block.
 *
 * bpsec_sc_stateIncr   - Increments the state to process the next operation in
 *                        a security block. This involves updating counters in
 *                        the state and clearing any results held from the
 *                        previous security operation.
 *
 * bpsec_sc_stateClear  - Clears any resources associated with state. The
 *                        sc_state structure itself is not freed because the
 *                        passed-in structure might represent a value stored on
 *                        the stack, or in heap, so it is up to the called to
 *                        determine how/when to release resources associated
 *                        with storing the sc_state structure itself, versus
 *                        its contents.
 *
 * TODO - We might need to pass in the security block itself, not just the ASB to pull in block elements like BPCF?
 *
 * bpsec_sc_procOutboundBlk - Process a security operation in an outbound
 *                        block. This function takes in an sc_state that has
 *                        been previously initialized with a call to
 *                        bpsec_sc_stateInit. This function is called by a
 *                        security source as part of adding a security operation
 *                        to a block.
 *
 *
 * bpsec_sc_procInboundBlk - Process a security operation in an inbound
 *                        block. This function takes in an sc_state that has
 *                        been previously initialized with a call to
 *                        bpsec_sc_stateInit. This function is called by a
 *                        security verifier or security acceptor as part of
 *                        processing an existing security operation in a
 *                        received security block.
 *
 * bpsec_sc_valMapGet   - Retrieve the value map which associated the various
 *                        sc_value functions used to process sc_values used by
 *                        the security context.
 */


typedef int  (*bpsec_sc_init)            ();
typedef void (*bpsec_sc_teardown)        ();
typedef int  (*bpsec_sc_initOutboundASB) (void *def, Bundle *bundle, BpsecOutboundASB *asb, Sdr sdr, PsmPartition wm, PsmAddress parms);
typedef int  (*bpsec_sc_stateInit)       (PsmPartition wm, sc_state *state, unsigned char secBlkNum, void *sc_def, sc_role role, sc_action action, EndpointId src, PsmAddress pol_parms, Lyst blk_parms, int num_tgts);
typedef int  (*bpsec_sc_stateIncr)       (sc_state *state);
typedef void (*bpsec_sc_stateClear)      (sc_state *state);
typedef int  (*bpsec_sc_procOutboundBlk) (sc_state *state, Lyst outParms, Bundle *bundle, BpsecOutboundASB *asb, BpsecOutboundTargetResult *tgtResult);
typedef int  (*bpsec_sc_procInboundBlk)  (sc_state *state, AcqWorkArea *wk, BpsecInboundASB *asb, LystElt tgtBlkElt, BpsecInboundTargetResult *tgtResult);
typedef sc_value_map* (*bpsec_sc_valMapGet)();

/**
 * Security Context Definition
 *
 * This structure defines a security context object used by the ION BPSec
 * implementation to map the SC Interface to the functions implementing a
 * specific context.
 *
 * In addition to holding pointers to the SCI functions (as defined above)
 * this structure captures the name of the SC, it's ID, and the services
 * that it provides.
 *
 * The scId field holds the identifier for this security context. If this
 * value is >= 0, then it MUST reference a standardized security context,
 * as registered by IANA and found in the "BPSec Security Context
 * Identifiers" registry located at
 * https://www.iana.org/assignments/bundle/bundle.xhtml#bpsec-security-context
 *
 * Otherwise, the scId MUST be < 0 to represent a test or site-local value.
 *
 * The services field (scServices) is treated as a bitfield, with each
 * supported security service OR'd to this value.
 *
 */
typedef struct
{
    char scName[BPSEC_SC_NAME_MAX_LEN]; /** Name of security context.    */
    int  scId;                          /** Security context ID.         */
    int  scServices;                    /** Supported security services. */

    /* Executive Functions */
    bpsec_sc_init            scInit;
    bpsec_sc_initOutboundASB scInitOutboundASB;
    bpsec_sc_teardown        scTeardown;

    /* Processing Functions */
    bpsec_sc_stateInit       scStateInit;
    bpsec_sc_stateIncr       scStateIncr;
    bpsec_sc_stateClear      scStateClear;

    bpsec_sc_procOutboundBlk scProcOutBlk;
    bpsec_sc_procInboundBlk  scProcInBlk;

    bpsec_sc_valMapGet       scValMapGet;
} sc_Def;


/*****************************************************************************
 *                            Function Prototypes                            *
 *****************************************************************************/


int   bpsec_sci_defCount();
int   bpsec_sci_defFind(int sci_id, sc_Def *def);
int   bpsec_sci_idFind(char *sc_name, int *sc_id);

int   bpsec_sci_execInit();
void  bpsec_sci_execTeardown();

int   bpsec_sci_polParmAdd(PsmPartition wm, PsmAddress parms, sc_Def *def, char *key, char *value);
char* bpsec_sci_polParmPrint(PsmPartition wm, sc_Def *sc_def, PsmAddress pol_parms);


int   bpsec_sci_parmFilter(sc_state *state, sc_Def *def, PsmPartition wm,
		                    PsmAddress pol_parms, Lyst blk_parms);

int   bpsec_sci_initAsbFn(void *def, Bundle *bundle, BpsecOutboundASB *asb, Sdr sdr, PsmPartition wm, PsmAddress parms);
void  bpsec_sci_stateClear(sc_state *state);
int   bpsec_sci_stateIncr(sc_state *state);
int   bpsec_sci_stateInit(PsmPartition wm, sc_state *state, unsigned char secBlkNum, void *sc_def,
		                   sc_role role, sc_action action, EndpointId src, PsmAddress pol_parms,
						   Lyst blk_parms, int num_tgts);

int   bpsec_sci_multCheck(Sdr sdr, BpsecOutboundASB *asb, sc_Def *def,
		                  PsmPartition wm, PsmAddress parms);



#endif
