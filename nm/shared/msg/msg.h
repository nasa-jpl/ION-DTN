/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** \file msg.h
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
 **  10/01/18  E. Birrane     Updated to AMP v0.5. Migrate from pdu.h. (JHU/APL)
 *****************************************************************************/

#ifndef _PDU_H_
#define _PDU_H_

#include "stdint.h"
#include "../utils/nm_types.h"
#include "../utils/vector.h"
#include "../primitives/ari.h"
#include "../primitives/report.h"
#include "../primitives/table.h"


#define MSG_DEFAULT_ENC_SIZE 1024

#define MSG_TYPE_UNK       (-1)
#define MSG_TYPE_REG_AGENT (0)
#define MSG_TYPE_RPT_SET   (1)
#define MSG_TYPE_PERF_CTRL (2)
#define MSG_TYPE_TBL_SET   (3)


#define MSG_HDR_FLG_OPCODE (0x7)
#define MSG_HDR_FLG_ACK    (0x8)
#define MSG_HDR_FLG_NACK   (0x10)
#define MSG_HDR_FLG_ACL    (0x20)

#define MSG_HDR_GET_OPCODE(flags) (flags & MSG_HDR_FLG_OPCODE)
#define MSG_HDR_GET_ACK(flags) (flags & MSG_HDR_FLG_ACK)
#define MSG_HDR_GET_NACK(flags) (flags & MSG_HDR_FLG_NACK)
#define MSG_HDR_GET_ACL(flags) (flags & MSG_HDR_FLG_ACL)

#define MSG_HDR_SET_OPCODE(flags, opcode) (flags |= (MSG_HDR_FLG_OPCODE & opcode))
#define MSG_HDR_SET_ACK(flags) (flags & MSG_HDR_FLG_ACK)
#define MSG_HDR_SET_NACK(flags) (flags & MSG_HDR_FLG_NACK)
#define MSG_HDR_SET_ACL(flags) (flags & MSG_HDR_FLG_ACL)





typedef struct
{
	uint8_t flags;
} msg_hdr_t;



typedef struct
{
	msg_hdr_t hdr;
	time_t start;
	ac_t *ac;
} msg_ctrl_t;


typedef struct
{
	msg_hdr_t hdr;
	eid_t agent_id;       /**> ID of the agent being registered. */
} msg_agent_t;


typedef struct
{
	msg_hdr_t hdr;
	vector_t rx;   /**> Recipients for the report. (char *)*/
	vector_t rpts; /**> (rpt_t *) */
} msg_rpt_t;

typedef struct
{
	msg_hdr_t hdr;
	vector_t rx; /**> (char *) */
	vector_t tbls; /**> (tbl_t *) */
} msg_tbl_t;

/*
 * Only support 1 name right now.
 */

typedef struct
{
	vector_t msgs;  /* (blob_t *) - serialized messages. */
	blob_t types;
	time_t time;
} msg_grp_t;


typedef struct
{
	eid_t senderEid;
	eid_t originatorEid;
	eid_t recipientEid;
} msg_metadata_t;

msg_hdr_t msg_hdr_deserialize(QCBORDecodeContext *it, int *success);

int msg_hdr_serialize(QCBOREncodeContext *encoder, msg_hdr_t hdr);


msg_agent_t* msg_agent_create();

msg_agent_t* msg_agent_deserialize(blob_t *data, int *success);

void         msg_agent_release(msg_agent_t *pdu, int destroy);

int msg_agent_serialize(QCBOREncodeContext *encoder, void *item);

blob_t*      msg_agent_serialize_wrapper(msg_agent_t *msg);

void         msg_agent_set_agent(msg_agent_t *msg, eid_t agent);

msg_ctrl_t* msg_ctrl_create();

msg_ctrl_t* msg_ctrl_create_ari(ari_t *id);

msg_ctrl_t* msg_ctrl_deserialize(blob_t *data, int *success);

void        msg_ctrl_release(msg_ctrl_t *msg, int destroy);

int msg_ctrl_serialize(QCBOREncodeContext *encoder, void *item);

blob_t*     msg_ctrl_serialize_wrapper(msg_ctrl_t *msg);



int        msg_rpt_add_rpt(msg_rpt_t *msg_rpt, rpt_t *rpt);

void       msg_rpt_cb_del_fn(void *item);

msg_rpt_t* msg_rpt_create(char *rx_name);

msg_rpt_t *msg_rpt_deserialize(blob_t *data, int *success);

void       msg_rpt_release(msg_rpt_t *pdu, int destroy);

int msg_rpt_serialize(QCBOREncodeContext *encoder, void *item);

blob_t*    msg_rpt_serialize_wrapper(msg_rpt_t *msg);


int        msg_tbl_add_tbl(msg_tbl_t *msg_tbl, tbl_t *tbl);

void       msg_tbl_cb_del_fn(void *item);

msg_tbl_t* msg_tbl_create(char *rx_name);

msg_tbl_t *msg_tbl_deserialize(blob_t *data, int *success);

void       msg_tbl_release(msg_tbl_t *pdu, int destroy);

int msg_tbl_serialize(QCBOREncodeContext *encoder, void *item);

blob_t*    msg_tbl_serialize_wrapper(msg_tbl_t *msg);



int        msg_grp_add_msg(msg_grp_t *grp, blob_t *msg, uint8_t type);

int        msg_grp_add_msg_agent(msg_grp_t *grp, msg_agent_t *msg);
int        msg_grp_add_msg_ctrl(msg_grp_t *grp, msg_ctrl_t *msg);
int        msg_grp_add_msg_rpt(msg_grp_t *grp, msg_rpt_t *msg);
int        msg_grp_add_msg_tbl(msg_grp_t *grp, msg_tbl_t *msg);

msg_grp_t* msg_grp_create(uint8_t length);

msg_grp_t* msg_grp_deserialize(blob_t *data, int *success);

int        msg_grp_get_type(msg_grp_t *grp, int idx);

void       msg_grp_release(msg_grp_t *group, int destroy);

int msg_grp_serialize(QCBOREncodeContext *encoder, void *item);

blob_t*    msg_grp_serialize_wrapper(msg_grp_t *msg_grp);









#endif /* _PDU_H_ */
