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
 **  09/24/12  E. Birrane     Initial Implementation
 **  11/01/12  E. Birrane     Redesign of messaging architecture.
 *****************************************************************************/

#ifndef DTNMPDU_H_
#define DTNMPDU_H_

#include "lyst.h"

#include "stdint.h"
#include "shared/utils/nm_types.h"





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
} pdu_bundle_t;

/*
 * DTNMP PDU Structure
 * /
typedef struct
{
    
    / ** Meta-data associated with this message * /
    pdu_metadata_t meta;
    
    / ** Message Header * /
    pdu_header_t hdr;
    
	/ ** The received data buffer. * /
	uint8_t *content;
    
    / ** Number of bytes in the data buffer. * /
    uint32_t data_size;
  
} pdu;
*/


pdu_bundle_t *pdu_create_bundle();
pdu_bundle_t *pdu_create_bundle_arg(pdu_msg_t *msg);

pdu_header_t *pdu_create_hdr(uint8_t id, uint8_t ack, uint8_t nack, uint8_t acl);


pdu_msg_t *pdu_create_msg(uint8_t id,
		                  uint8_t *data,
		                  uint32_t data_size,
		                  pdu_acl_t *acl);


void pdu_release_hdr(pdu_header_t *hdr);
void pdu_release_meta(pdu_metadata_t *meta);
void pdu_release_acl(pdu_acl_t *acl);
void pdu_release_msg(pdu_msg_t *pdu);
void pdu_release_bundle(pdu_bundle_t *bundle);


uint8_t *pdu_serialize_hdr(pdu_header_t *hdr, uint32_t *len);
uint8_t *pdu_serialize_acl(pdu_acl_t *acl, uint32_t *len);

uint8_t *pdu_serialize_msg(pdu_msg_t *msg, uint32_t *len);
uint8_t *pdu_serialize_bundle(pdu_bundle_t *bundle, uint32_t *len);


pdu_header_t *pdu_deserialize_hdr(uint8_t *cursor,
		                          uint32_t size,
		                          uint32_t *bytes_used);

pdu_acl_t *pdu_deserialize_acl(uint8_t *cursor,
		                       uint32_t size,
		                       uint32_t *bytes_used);


int pdu_add_msg_to_bundle(pdu_bundle_t *bundle, pdu_msg_t *msg);


/***
/ * Functions to unmarshall PDU values * /
prod_rule *createProdRule(pdu* cur_pdu);


nm_custom_report* createCustomReport(pdu *rcv_pdu);

nm_report *createDataReport(pdu* cur_pdu);


/ * Functions to bild PDUs * /
uint8_t *buildProdRulePDU(int offset, int period, int evals, Lyst mids, int mid_size, int* msg_len);


uint8_t *buildReportDefPDU(mid_t *report_id, Lyst mids, uint32_t mid_size, int *msg_len);

****/

#endif /* DTNMPDU_H_ */
