/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
/*****************************************************************************
 **
 ** \file rules.h
 **
 **
 ** Description: This module contains the functions, structures, and other
 **              information necessary to describe Time-Based Rules (TRLs)
 **              and State-Based Rules (SRLs).
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  11/08/12  E. Birrane     Redesign of messaging architecture. (JHU/APL)
 **  06/24/13  E. Birrane     Migrated from uint32_t to time_t. (JHU/APL)
 **  05/17/15  E. Birrane     Moved controls to ctrl.[h|c]. Updated TRL/SRL to DTNMP v0.1  (Secure DTN - NASA: NNX14CS58P)
 **  06/26/15  E. Birrane     Updated structures/functs to reflect TRL/SRL naming.  (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#ifndef _RULES_H_
#define _RULES_H_

#include "lyst.h"

#include "../utils/nm_types.h"

#include "../primitives/mid.h"
#include "../primitives/expr.h"


/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */

/*
 * +--------------------------------------------------------------------------+
 * |							  	MACROS  								  +
 * +--------------------------------------------------------------------------+
 */


/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */



typedef struct
{
	Object itemObj;           /**> Serialized rule in an SDR. */
	uint32_t size;       /**> Size of rule in ruleObj.   */

	/* Descriptor Information. */
    int64_t  num_evals;       /**> # times left to eval rule. */
    uint32_t interval_ticks;  /**> # 1Hz ticks between evals. */
    eid_t    sender;          /**> Who sent this rule def.    */

    /* Below is not kept in the SDR. */
	Object descObj;   /** > This descriptor in SDR. */
} trl_desc_t;


typedef struct {
	mid_t *mid;     /**> MID identifier for this TRL.        */
    time_t time;    /**> The time to start the production.   */
    uvast period;   /**> The delay between productions.      */
    uvast count;    /**> The # times to produce the message. */
    Lyst  action;   /**> Macro to run when rule triggers  .  */

    /* Below is not serialized. */
    uint32_t countdown_ticks; /**> # ticks before next eval.  */
    trl_desc_t desc; /**> SDR descriptor. */
} trl_t;



typedef struct
{
	Object itemObj;           /**> Serialized rule in an SDR. */
	uint32_t size;            /**> Size of rule in ruleObj.   */

	/* Descriptor Information. */
    int64_t  num_evals;       /**> # times left to eval rule. */
    uint32_t interval_ticks;  /**> # 1Hz ticks between evals. */
    eid_t    sender;          /**> Who sent this rule def.    */

    /* Below is not kept in the SDR. */
	Object descObj;           /** > This descriptor in SDR. */
} srl_desc_t;


typedef struct {
	mid_t *mid;    /**> MID identifier for this SRL.            */
    time_t time;   /**> The time to start the production.       */
    expr_t *expr;     /**> The predicate driving report production.*/
    uvast count;   /**> The # times to produce the message.     */
    Lyst action;   /**> Macro to run when the rule triggers.    */

    /* Below is not serialized. */
    uint32_t countdown_ticks;       /**> # ticks before next eval.  */
    srl_desc_t desc; /**> SDR descriptor */
} srl_t;





/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */

srl_t*   srl_copy(srl_t *srl);
srl_t*   srl_create(mid_t *mid, time_t time, expr_t *expr, uvast count, Lyst action);
srl_t*   srl_deserialize(uint8_t *cursor, uint32_t size, uint32_t *bytes_used);
void     srl_lyst_clear(Lyst *list, ResourceLock *mutex, int destroy);
void     srl_release(srl_t *srl);
uint8_t* srl_serialize(srl_t *srl, uint32_t *len);

trl_t*   trl_create(mid_t *mid, time_t time, uvast period, uvast count, Lyst action);
trl_t*   trl_deserialize(uint8_t *cursor, uint32_t size, uint32_t *bytes_used);
void     trl_lyst_clear(Lyst *list, ResourceLock *mutex, int destroy);
void     trl_release(trl_t *trl);
uint8_t* trl_serialize(trl_t *trl, uint32_t *len);


#endif // _RULES_H_
