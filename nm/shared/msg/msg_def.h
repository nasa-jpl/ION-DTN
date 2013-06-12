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
 ** \file msg_def.h
 **
 **
 ** Description: Defines the data types associated with definition
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
 **  09/24/12  E. Birrane     Initial Implementation (in other files)
 **  11/02/12  E. Birrane     Redesign of messaging architecture.
 *****************************************************************************/

#ifndef MSG_DEF_H_
#define MSG_DEF_H_

#include <stdint.h>

#include "lyst.h"

#include "shared/utils/nm_types.h"
#include "shared/primitives/mid.h"
#include "shared/primitives/def.h"

#include "shared/msg/pdu.h"


/* Definition messages */
#define MSG_TYPE_DEF_CUST_RPT     (0x08)
#define MSG_TYPE_DEF_COMP_DATA    (0x09)
#define MSG_TYPE_DEF_MACRO        (0x0A)



/* Serialize functions. */
uint8_t *def_serialize_gen(def_gen_t *def, uint32_t *len);


/* Deserialize functions. */
def_gen_t *def_deserialize_gen(uint8_t *cursor,
		                       uint32_t size,
		                       uint32_t *bytes_used);


#endif // MSG_DEF_H_
