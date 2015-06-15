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
 **  11/08/12  E. Birrane     Redesign of messaging architecture.
 **  06/24/13  E. Birrane     Migrated from uint32_t to time_t.
 **  05/17/15  E. Birrane     Moved controls to ctrl.[h|c]. Updated TRL/SRL to DTNMP v0.1
 *****************************************************************************/

#ifndef _RULES_H_
#define _RULES_H_

#include "lyst.h"

#include "shared/utils/nm_types.h"

#include "shared/primitives/mid.h"

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
} rule_time_prod_desc_t;


/**
 * Associated Message Type(s): MSG_TYPE_CTRL_PERIOD_PROD
 *
 * Purpose:
 *
 * +--------+------------+--------+---------+
 * | Start  | Period (s) | Count  | Results |
 * | (TS)   | (SDNV)     | (SDNV) |   (MC)  |
 * +--------+------------+--------+---------+
 */
typedef struct {
    time_t time;     /**> The time to start the production.   */
    uvast period;   /**> The delay between productions.      */
    uvast count;    /**> The # times to produce the message. */
    Lyst     mids; /**> The MIDs to include in the report.  */

    /* Below is not serialized. */
    uint32_t countdown_ticks; /**> # ticks before next eval.  */
    rule_time_prod_desc_t desc; /**> SDR descriptor. */
} rule_time_prod_t;



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
} rule_pred_prod_descr_t;


/**
 * Associated Message Type(s): MSG_TYPE_CTRL_PRED_PROD
 *
 * Purpose:
 *
 * +--------+-----------+--------+---------+
 * | Start  | Predicate | Count  | Results |
 * | (TS)   |    (MC)   | (SDNV) |   (MC)  |
 * +--------+-----------+--------+---------+
 */
typedef struct {
    time_t time;       /**> The time to start the production. */
    Lyst predicate;    /**> The predicate driving report production.*/
    uvast count;       /**> The # times to produce the message. */
    Lyst contents;     /**> The MIDs to include in the report. */

    /* Below is not serialized. */
    uint32_t countdown_ticks;       /**> # ticks before next eval.  */
    rule_pred_prod_descr_t descObj; /**> SDR descriptor */
} rule_pred_prod_t;





/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */


/* Create functions. */
rule_time_prod_t *rule_create_time_prod_entry(time_t time,
							   				  uvast count,
											  uvast period,
											  Lyst contents);

rule_pred_prod_t *rule_create_pred_prod_entry(time_t time,
		   	   	   	   	   	   	   	   	      Lyst predicate,
		   	   	   	   	   	   	   	   	      uvast count,
		   	   	   	   	   	   	   	   	      Lyst contents);



/* Release functions.*/
void rule_release_time_prod_entry(rule_time_prod_t *msg);
void rule_release_pred_prod_entry(rule_pred_prod_t *msg);


/* Lyst functions. */
void rule_time_clear_lyst(Lyst *list, ResourceLock *mutex, int destroy);
void rule_pred_clear_lyst(Lyst *list, ResourceLock *mutex, int destroy);



#endif // _RULES_H_
