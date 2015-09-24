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
 ** \file msg_admin.h
 **
 **
 ** Description: Defines the data types associated with administrative
 **              messages within the protocol. Also, identify the functions
 **              that pack and unpack these messages.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  09/24/12  E. Birrane     Initial Implementation
 **  11/01/12  E. Birrane     Redesign of messaging architecture.
 *****************************************************************************/

#ifndef MSG_ADMIN_H_
#define MSG_ADMIN_H_

#include "stdint.h"

#include "lyst.h"

#include "shared/utils/nm_types.h"

#include "shared/primitives/mid.h"
#include "shared/primitives/admin.h"

#include "shared/msg/pdu.h"


/* Administrative messages */
#define MSG_TYPE_ADMIN_REG_AGENT  (0x00)
#define MSG_TYPE_ADMIN_RPT_POLICY (0x01)
#define MSG_TYPE_ADMIN_STAT_MSG   (0x02)


/* Serialize functions. */
uint8_t *msg_serialize_reg_agent(adm_reg_agent_t *msg, uint32_t *len);
uint8_t *msg_serialize_rpt_policy(adm_rpt_policy_t *msg, uint32_t *len);
uint8_t *msg_serialize_stat_msg(adm_stat_msg_t *msg, uint32_t *len);


/* Deserialize functions. */
adm_reg_agent_t *msg_deserialize_reg_agent(uint8_t *cursor,
		                                   uint32_t size,
		                                   uint32_t *bytes_used);

adm_rpt_policy_t *msg_deserialize_rpt_policy(uint8_t *cursor,
        									 uint32_t size,
        									 uint32_t *bytes_used);

adm_stat_msg_t   *msg_deserialize_stat_msg(uint8_t *cursor,
        								   uint32_t size,
        								   uint32_t *bytes_used);

#endif // MSG_ADMIN_H_
