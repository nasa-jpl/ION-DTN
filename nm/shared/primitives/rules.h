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
 *****************************************************************************/

#ifndef _RULES_H_
#define _RULES_H_

#include "lyst.h"

#include "shared/utils/nm_types.h"

#include "shared/primitives/mid.h"

#include "shared/msg/pdu.h"
#include "shared/msg/msg_def.h"
#include "shared/msg/msg_reports.h"

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



/**EJBREFAcTOR
typedef struct {

    / * Number of times left to evaluate the rule. * /
    int64_t num_evals;

    / * Interval between rule evaluations in ticks. * /
    uint32_t interval_ticks;

    / * MIDs captured by this rule. * /
    Lyst mids;

    / * Decrementable time until next execution in ticks. * /
    uint32_t countdown_ticks;

    / *
     * Who sent us the rule
     * \todo: EJB Need to eventually associated rules to recipients
     * /
    eid_t sender;

    / * When to actually start the rule evaluation (0 = now) * /
    unsigned long offset;

} prod_rule_t;
**/



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
    uint32_t time;     /**> The time to start the production.   */
    uint64_t period;   /**> The delay between productions.      */
    uint64_t count;    /**> The # times to produce the message. */
    Lyst     mids; /**> The MIDs to include in the report.  */

    /* Below is not serialized. */

    int64_t  num_evals;       /**> # times left to eval rule. */
    uint32_t interval_ticks;  /**> # 1Hz ticks between evals. */
    uint32_t countdown_ticks; /**> # ticks before next eval.  */
    eid_t    sender;          /**> Who sent this rule def.    */

} rule_time_prod_t;


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
    uint32_t time;     /**> The time to start the production. */
    Lyst predicate;    /**> The predicate driving report production.*/
    uint64_t count;    /**> The # times to produce the message. */
    Lyst contents;     /**> The MIDs to include in the report. */

    /* Below is not serialized. */

    int64_t  num_evals;       /**> # times left to eval rule. */
    uint32_t interval_ticks;  /**> # 1Hz ticks between evals. */
    uint32_t countdown_ticks; /**> # ticks before next eval.  */
    eid_t    sender;          /**> Who sent this rule def.    */
} rule_pred_prod_t;


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
    uint32_t time;     /**> The time to run the control. */
    Lyst contents;     /**> The controls (macro) to run. */

    /* Below is no serialized. */
    uint32_t countdown_ticks; /**> # ticks before next eval.  */
    eid_t    sender;          /**> Who sent this ctrl def.    */
    uint8_t  state;           /**> 0: INACTIVE, 1, ACTIVE     */
} ctrl_exec_t;



/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */


/* Create functions. */
rule_time_prod_t *rule_create_time_prod_entry(uint32_t time,
							   				  uint64_t count,
											  uint64_t period,
											  Lyst contents);

rule_pred_prod_t *rule_create_pred_prod_entry(uint32_t time,
		   	   	   	   	   	   	   	   	      Lyst predicate,
		   	   	   	   	   	   	   	   	      uint64_t count,
		   	   	   	   	   	   	   	   	      Lyst contents);

ctrl_exec_t *ctrl_create_exec(uint32_t time, Lyst contents);


/* Release functions.*/
void rule_release_time_prod_entry(rule_time_prod_t *msg);
void rule_release_pred_prod_entry(rule_pred_prod_t *msg);
void ctrl_release_exec(ctrl_exec_t *msg);



#endif // _RULES_H_
