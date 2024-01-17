/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** \file pdu.h
 **
 **
 ** Description:
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  09/24/12  E. Birrane     Initial Implementation (JHU/APL)
 **  11/01/12  E. Birrane     Redesign of messaging architecture. (JHU/APL)
 **  06/25/13  E. Birrane     Renamed message "bundle" message "group". (JHU/APL)
 **  06/26/13  E. Birrane     Added group timestamp (JHU/APL)
 *****************************************************************************/

#ifndef DTNMPDU_H_
#define DTNMPDU_H_

#include "lyst.h"

#include "stdint.h"
#include "../utils/nm_types.h"



#define MSG_TYPE_RPT_DATA_LIST    (0x10)
#define MSG_TYPE_RPT_DATA_DEFS    (0x11)
#define MSG_TYPE_RPT_DATA_RPT     (0x12)
#define MSG_TYPE_RPT_PROD_SCHED   (0x13)


typedef struct
{
	/** The EID of the last node of this message. */
	eid_t senderEid;
    
    /** The EID of the message originator. */
    eid_t originatorEid;
    
    /** The EID of the message recipient. */
    eid_t recipientEid;    
    
} pdu_metadata_t;


typedef struct 
{
    /** Message type */
    uint8_t type;    /**> The opcode field identifying the message opcode. */
    uint8_t context; /**> Message category. */
    uint8_t ack;     /**> Whether the message must be ACK'd. */
    uint8_t nack;    /**> Whether the message must send error on failure. */
    uint8_t acl;     /**> Whether the message has an ACL appended to it. */
    uint8_t id;      /**> Combination of type and category. */
} pdu_header_t;


/*
 * \todo Support this...
 */
typedef struct
{

} pdu_acl_t;

typedef struct
{
	pdu_header_t *hdr;
	pdu_metadata_t meta;
	uint8_t *contents;
	uint32_t size;
	pdu_acl_t *acl;
} pdu_msg_t;


typedef struct
{
	Lyst msgs;
	time_t time;
} pdu_group_t;


pdu_group_t *pdu_create_empty_group();
pdu_group_t *pdu_create_group(pdu_msg_t *msg);

pdu_header_t *pdu_create_hdr(uint8_t id, uint8_t ack, uint8_t nack, uint8_t acl);


pdu_msg_t *pdu_create_msg(uint8_t id,
		                  uint8_t *data,
		                  uint32_t data_size,
		                  pdu_acl_t *acl);


void pdu_release_hdr(pdu_header_t *hdr);
void pdu_release_meta(pdu_metadata_t *meta);
void pdu_release_acl(pdu_acl_t *acl);
void pdu_release_msg(pdu_msg_t *pdu);
void pdu_release_group(pdu_group_t *group);


uint8_t *pdu_serialize_hdr(pdu_header_t *hdr, uint32_t *len);
uint8_t *pdu_serialize_acl(pdu_acl_t *acl, uint32_t *len);

uint8_t *pdu_serialize_msg(pdu_msg_t *msg, uint32_t *len);
uint8_t *pdu_serialize_group(pdu_group_t *group, uint32_t *len);


pdu_header_t *pdu_deserialize_hdr(uint8_t *cursor,
		                          uint32_t size,
		                          uint32_t *bytes_used);

pdu_acl_t *pdu_deserialize_acl(uint8_t *cursor,
		                       uint32_t size,
		                       uint32_t *bytes_used);


int pdu_add_msg_to_group(pdu_group_t *group, pdu_msg_t *msg);


#endif /* DTNMPDU_H_ */
