/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 **     This material may only be used, modified, or reproduced by or for the
 **       U.S. Government pursuant to the license rights granted under
 **          FAR clause 52.227-14 or DFARS clauses 252.227-7013/7014
 **
 **     For any other permissions, please contact the Legal Office at JHU/APL.
 ******************************************************************************/

/*****************************************************************************
 **
 ** \file rules.h
 **
 **
 ** Description:
 **
 **\todo Right now this is a dumping ground for common utilities across
 **\todo agent and manager. Need to make this a little more eloquent.
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  11/08/12  E. Birrane     Redesign of messaging architecture.
 **  06/24/13  E. Birrane     Migrated from uint32_t to time_t.
 *****************************************************************************/

#ifndef _RULES_H_
#define _RULES_H_

#include "lyst.h"

#include "shared/utils/nm_types.h"

#include "shared/primitives/mid.h"

/*#include "shared/msg/pdu.h"
#include "shared/msg/msg_def.h"
#include "shared/msg/msg_reports.h"
*/

/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */

#define DTNMP_RULE_EXEC_ALWAYS (-1)
#define CONTROL_ACTIVE (1)
#define CONTROL_INACTIVE (0)

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



typedef struct
{
	Object itemObj;           /**> Serialized ctrl in an SDR. */
	uint32_t size;            /**> Size of ctrl in ctrlObj.   */

	/* Descriptor Information. */
    eid_t    sender;          /**> Who sent this ctrl def.    */
    uint8_t  state;           /**> 0: INACTIVE, 1, ACTIVE     */

    /* Below is not kept in the SDR. */
	Object descObj;           /** > This descriptor in SDR. */
} ctrl_exec_desc_t;

/**
 * Associated Message Type(s): MSG_TYPE_CTRL_PERF_CTRL
 *
 * Purpose: Runs a control on the agent.
 *
 * +-------+-----------+
 * | Start |  Controls |
 * |  (TS) |    (MC)   |
 * +-------+-----------+
 */
typedef struct {
    time_t time;              /**> The time to run the control. */
    Lyst contents;            /**> The controls (macro) to run. */

    /* Below is not serialized. */
    uint32_t countdown_ticks; /**> # ticks before next eval.  */
    ctrl_exec_desc_t desc; /**> SDR descriptor. */

} ctrl_exec_t;



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

ctrl_exec_t *ctrl_create_exec(time_t time, Lyst contents);


/* Release functions.*/
void rule_release_time_prod_entry(rule_time_prod_t *msg);
void rule_release_pred_prod_entry(rule_pred_prod_t *msg);
void ctrl_release_exec(ctrl_exec_t *msg);


/* Lyst functions. */
void rule_time_clear_lyst(Lyst *list, ResourceLock *mutex, int destroy);
void rule_pred_clear_lyst(Lyst *list, ResourceLock *mutex, int destroy);
void ctrl_clear_lyst(Lyst *list, ResourceLock *mutex, int destroy);



#endif // _RULES_H_
