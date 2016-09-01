/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** \file admin.h
 **
 **
 ** Description: Administrative primitives.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  01/17/13  E. Birrane     Redesign of messaging architecture. (JHU/APL)
 **  06/24/13  E. Birrane     Migrated from uint32_t to time_t. (JHU/APL)
 **  06/30/16  E. Birrane     Update to AMP v0.3 (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/


#ifndef _ADMIN_H_
#define _ADMIN_H_


#include "../utils/nm_types.h"

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


/*
 * Associated Message Type: MSG_TYPE_ADMIN_REG_AGENT
 * Purpose: Notify manager of discovered agent.
 * +----------+
 * | Agent ID |
 * | [BLOB]   |
 * +----------+
 */
typedef struct {
	eid_t agent_id;       /**> ID of the agent being registered. */
} adm_reg_agent_t;


/*
 * Associated Message Type: MSG_TYPE_ADMIN_RPT_POLICY
 * Purpose: Set the reporting policy on an agent.
 *
 * +-------+--------+-------+-------+----------+
 * | Alert |  Warn  | Error |  Log  | Reserved |
 * +-------+--------+-------+-------+----------+
 * |   0   |    1   |    2  |    3  |  4 5 6 7 |
 * +-------+--------+-------+-------+----------+
 * LSB                                     MSB
 */
typedef struct {
    uint8_t mask;         /**> Reporting Policy Mask. */
} adm_rpt_policy_t;


/*
 * Associated Message Type: MSG_TYPE_ADMIN_STAT_MSG
 * Purpose: Report status from an agent.
 *  +------+------+------------+
 *  | Code | Time | Generators |
 *  |(MID) | (TS) |    (MC)    |
 *  +------+------+------------+
 */
typedef struct {
    mid_t *code;          /**> Status Code. */
	time_t time;          /**> Time when code occurred. */
	Lyst generators;      /**> MID list of generators. */
} adm_stat_msg_t;


/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */


/* Create functions. */
adm_reg_agent_t *msg_create_reg_agent(eid_t eid);

adm_rpt_policy_t *msg_create_rpt_policy(uint8_t mask);

adm_stat_msg_t *msg_create_stat_msg(mid_t *code,
								    time_t time,
									Lyst generators);

/* Release functions.*/
void msg_release_reg_agent(adm_reg_agent_t *msg);
void msg_release_rpt_policy(adm_rpt_policy_t *msg);
void msg_release_stat_msg(adm_stat_msg_t *msg);


#endif /* _ADMIN_H_ */
