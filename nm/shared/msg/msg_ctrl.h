/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
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
 **  11/04/12  E. Birrane     Redesign of messaging architecture. (JHU/APL)
 **  01/17/13  E. Birrane     Updated to use primitive types. (JHU/APL)
 *****************************************************************************/

#ifndef MSG_CTRL_H_
#define MSG_CTRL_H_

#include "stdint.h"

#include "lyst.h"

#include "../utils/nm_types.h"
#include "../primitives/mid.h"
#include "../primitives/rules.h"
#include "../msg/pdu.h"

/* Control messages */
#define MSG_TYPE_CTRL_PERIOD_PROD (0x18)
#define MSG_TYPE_CTRL_PRED_PROD   (0x19)
#define MSG_TYPE_CTRL_EXEC        (0x1A)

#define MAX_RULE_SIZE (1024)


typedef struct
{
	time_t ts;
	Lyst mc;
} msg_perf_ctrl_t;

/* Serialize functions. */

msg_perf_ctrl_t *msg_create_perf_ctrl(time_t ts, Lyst mc);
void msg_destroy_perf_ctrl(msg_perf_ctrl_t *ctrl);

uint8_t *msg_serialize_perf_ctrl(msg_perf_ctrl_t *ctrl, uint32_t *len);



/* Deserialize functions. */


msg_perf_ctrl_t *msg_deserialize_perf_ctrl(uint8_t *cursor,
		                                   uint32_t size,
		                                   uint32_t *bytes_used);

#endif // MSG_CTRL_H_
