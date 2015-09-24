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
 ** \file msg_ctrl.h
 **
 **
 ** Description: Defines the serialization and de-serialization methods
 **              used to translate data types into byte streams associated
 **              with DTNMP messages, and vice versa.
 **
 ** Notes:
 **				 Not all fields of internal structures are serialized into/
 **				 deserialized from DTNMP messages.
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  11/04/12  E. Birrane     Redesign of messaging architecture.
 **  01/17/13  E. Birrane     Updated to use primitive types.
 *****************************************************************************/

#ifndef MSG_CTRL_H_
#define MSG_CTRL_H_

#include "stdint.h"

#include "lyst.h"

#include "shared/utils/nm_types.h"
#include "shared/primitives/mid.h"
#include "shared/primitives/rules.h"
#include "shared/msg/pdu.h"

/* Control messages */
#define MSG_TYPE_CTRL_PERIOD_PROD (0x18)
#define MSG_TYPE_CTRL_PRED_PROD   (0x19)
#define MSG_TYPE_CTRL_EXEC        (0x1A)

#define MAX_RULE_SIZE (1024)



/* Serialize functions. */
uint8_t *ctrl_serialize_time_prod_entry(rule_time_prod_t *msg, uint32_t *len);
uint8_t *ctrl_serialize_pred_prod_entry(rule_pred_prod_t *msg, uint32_t *len);
uint8_t *ctrl_serialize_exec(ctrl_exec_t *msg, uint32_t *len);


/* Deserialize functions. */
rule_time_prod_t *ctrl_deserialize_time_prod_entry(uint8_t *cursor,
		                       	   	   	   	   	   uint32_t size,
		                       	   	   	   	   	   uint32_t *bytes_used);

rule_pred_prod_t *ctrl_deserialize_pred_prod_entry(uint8_t *cursor,
		                       	   	   	   	       uint32_t size,
		                       	   	   	   	       uint32_t *bytes_used);

ctrl_exec_t *ctrl_deserialize_exec(uint8_t *cursor,
		                       	   uint32_t size,
		                       	   uint32_t *bytes_used);



#endif // MSG_CTRL_H_
