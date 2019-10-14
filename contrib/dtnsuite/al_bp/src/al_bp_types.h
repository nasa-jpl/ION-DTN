/********************************************************
 **  Authors: Michele Rodolfi, michele.rodolfi@studio.unibo.it
 **           Anna d'Amico, anna.damico@studio.unibo.it
 **           Davide Pallotti, davide.pallotti@studio.unibo.it
 **           Carlo Caini (DTNperf_3 project supervisor), carlo.caini@unibo.it
 **
 **
 **  Copyright (c) 2013, Alma Mater Studiorum, University of Bologna
 **  All rights reserved.
 ** This file contains the definition of all types specific to the al_bp.
 ********************************************************/

/*
 * bp_types.h
 *
 */

#ifndef BP_TYPES_H_
#define BP_TYPES_H_

#include "types.h"

#define AL_BP_MAX_ENDPOINT_ID 256
#define AL_BP_MAX_ORDINAL_NBR 254

/**
 * Bundle protocol implementation
 */
typedef enum
{
	BP_NONE = 0,
    BP_DTN,
    BP_ION,
	BP_IBR
} al_bp_implementation_t;

/**
 *	DTN eid Scheme type
 */
typedef enum
{
	CBHE_SCHEME = 0,
	DTN_SCHEME,
} al_bp_scheme_t;

/**
 * Specification of a bp endpoint id, i.e. a URI, implemented as a
 * fixed-length char buffer. Note that for efficiency reasons, this
 * fixed length is relatively small (256 bytes).
 *
 * The alternative is to use the string XDR type but then all endpoint
 * ids would require malloc / free which is more prone to leaks / bugs.
 */

typedef struct al_bp_endpoint_id_t {
	char uri[AL_BP_MAX_ENDPOINT_ID]; //256
} al_bp_endpoint_id_t;

/**
 * *************************************************************************CONTROLLA DESCRIZIONE
 * The basic handle for communication with the bp router.
 */
typedef int * al_bp_handle_t;

/**
 * BP timeouts are specified in seconds.
 */

typedef u32_t al_bp_timeval_t;

typedef struct al_bp_timestamp_t {
	u32_t secs;
	u32_t seqno;
} al_bp_timestamp_t;

typedef u32_t al_bp_reg_token_t;

/**
 * A registration cookie.
 */

typedef u32_t al_bp_reg_id_t;

/**
 * Registration state.
 */

typedef struct al_bp_reg_info_t {
	al_bp_endpoint_id_t endpoint;
	al_bp_reg_id_t regid;
	u32_t flags;
	u32_t replay_flags;
	al_bp_timeval_t expiration;
	boolean_t init_passive;
	al_bp_reg_token_t reg_token;
	struct {
		u32_t script_len;
		char *script_val;
	} script;
} al_bp_reg_info_t;

/**
 * Registration flags are a bitmask of the following:

 * Delivery failure actions (exactly one must be selected):
 *     BP_REG_DROP   - drop bundle if registration not active
 *     BP_REG_DEFER  - spool bundle for later retrieval
 *     BP_REG_EXEC   - exec program on bundle arrival
 *
 * Session flags:
 *     BP_SESSION_CUSTODY   - app assumes custody for the session
 *     BP_SESSION_PUBLISH   - creates a publication point
 *     BP_SESSION_SUBSCRIBE - create subscription for the session
 *
 * Other flags:
 *     BP_DELIVERY_ACKS - application will acknowledge delivered
 *                         bundles with bp_ack()
 *
 */

typedef enum al_bp_reg_flags_t {
	BP_REG_DROP = 1,
	BP_REG_DEFER = 2,
	BP_REG_EXEC = 3,
	BP_SESSION_CUSTODY = 4,
	BP_SESSION_PUBLISH = 8,
	BP_SESSION_SUBSCRIBE = 16,
	BP_DELIVERY_ACKS = 32,
} al_bp_reg_flags_t;

/**
 * Value for an unspecified registration cookie (i.e. indication that
 * the daemon should allocate a new unique id).
 */
#define BP_REGID_NONE 0

/**
 * Bundle delivery option flags. Note that multiple options may be
 * selected for a given bundle.
 *
 *     BP_DOPTS_NONE            - no custody, etc
 *     BP_DOPTS_CUSTODY         - custody xfer
 *     BP_DOPTS_DELIVERY_RCPT   - end to end delivery (i.e. return receipt)
 *     BP_DOPTS_RECEIVE_RCPT    - per hop arrival receipt
 *     BP_DOPTS_FORWARD_RCPT    - per hop departure receipt
 *     BP_DOPTS_CUSTODY_RCPT    - per custodian receipt
 *     BP_DOPTS_DELETE_RCPT     - request deletion receipt
 *     BP_DOPTS_SINGLETON_DEST  - destination is a singleton
 *     BP_DOPTS_MULTINODE_DEST  - destination is not a singleton
 *     BP_DOPTS_DO_NOT_FRAGMENT - set the do not fragment bit
 */

typedef enum al_bp_bundle_delivery_opts_t {
	BP_DOPTS_NONE = 0,
	BP_DOPTS_CUSTODY = 1,
	BP_DOPTS_DELIVERY_RCPT = 2,
	BP_DOPTS_RECEIVE_RCPT = 4,
	BP_DOPTS_FORWARD_RCPT = 8,
	BP_DOPTS_CUSTODY_RCPT = 16,
	BP_DOPTS_DELETE_RCPT = 32,
	BP_DOPTS_SINGLETON_DEST = 64,
	BP_DOPTS_MULTINODE_DEST = 128,
	BP_DOPTS_DO_NOT_FRAGMENT = 256,
} al_bp_bundle_delivery_opts_t;

/**
 * Bundle priority specifier.
 *     BP_PRIORITY_BULK      - lowest priority
 *     BP_PRIORITY_NORMAL    - regular priority
 *     BP_PRIORITY_EXPEDITED - important
 *     BP_PRIORITY_RESERVED  - TBD
 * Only for ION implementation there is ordinal number
 * 	   ordinal [0 - AL_MAX_ORDINAL_NBR]
 */

typedef enum al_bp_bundle_priority_enum {
	BP_PRIORITY_BULK = 0,
	BP_PRIORITY_NORMAL = 1,
	BP_PRIORITY_EXPEDITED = 2,
	BP_PRIORITY_RESERVED = 3,
} al_bp_bundle_priority_enum;

typedef struct al_bp_bundle_priority_t {
	al_bp_bundle_priority_enum priority;
	u32_t ordinal;
} al_bp_bundle_priority_t;

/**
 * Extension block.
 */

typedef struct al_bp_extension_block_t {
	u32_t type;
	u32_t flags;
	struct {
		u32_t data_len;
		char *data_val;
	} data;
} al_bp_extension_block_t;

/**
 * Bundle specifications. The delivery_regid is ignored when sending
 * bundles, but is filled in by the daemon with the registration
 * id where the bundle was received.
 */

typedef struct al_bp_bundle_spec_t {
	al_bp_endpoint_id_t source;
	al_bp_endpoint_id_t dest;
	al_bp_endpoint_id_t replyto;
	al_bp_bundle_priority_t priority;
	al_bp_bundle_delivery_opts_t dopts;
	al_bp_timeval_t expiration;
	al_bp_timestamp_t creation_ts;
	al_bp_reg_id_t delivery_regid;
	struct {
		u32_t blocks_len;
		al_bp_extension_block_t *blocks_val;
	} blocks;
	struct {
		u32_t metadata_len;
		al_bp_extension_block_t *metadata_val;
	} metadata;
	boolean_t unreliable;
	boolean_t critical;
	u32_t flow_label;
} al_bp_bundle_spec_t;

/**
 * The payload of a bundle can be sent or received either in a file,
 * in which case the payload structure contains the filename, or in
 * memory where the struct contains the data in-band, in the 'buf'
 * field.
 *
 * When sending a bundle, if the location specifies that the payload
 * is in a temp file, then the daemon assumes ownership of the file
 * and should have sufficient permissions to move or rename it.
 *
 * When receiving a bundle that is a status report, then the
 * status_report pointer will be non-NULL and will point to a
 * bp_bundle_status_report_t structure which contains the parsed fields
 * of the status report.
 *
 *     BP_PAYLOAD_MEM         - payload contents in memory
 *     BP_PAYLOAD_FILE        - payload contents in file
 *     BP_PAYLOAD_TEMP_FILE   - in file, assume ownership (send only)
 */

typedef enum al_bp_bundle_payload_location_t {
	BP_PAYLOAD_FILE = 0,
	BP_PAYLOAD_MEM = 1,
	BP_PAYLOAD_TEMP_FILE = 2,
} al_bp_bundle_payload_location_t;

/**
 * Bundle Status Report "Reason Code" flags
 */

typedef enum al_bp_status_report_reason_t {
	BP_SR_REASON_NO_ADDTL_INFO = 0x00,
	BP_SR_REASON_LIFETIME_EXPIRED = 0x01,
	BP_SR_REASON_FORWARDED_UNIDIR_LINK = 0x02,
	BP_SR_REASON_TRANSMISSION_CANCELLED = 0x03,
	BP_SR_REASON_DEPLETED_STORAGE = 0x04,
	BP_SR_REASON_ENDPOINT_ID_UNINTELLIGIBLE = 0x05,
	BP_SR_REASON_NO_ROUTE_TO_DEST = 0x06,
	BP_SR_REASON_NO_TIMELY_CONTACT = 0x07,
	BP_SR_REASON_BLOCK_UNINTELLIGIBLE = 0x08,
} al_bp_status_report_reason_t;

/**
 * Bundle Status Report status flags that indicate which timestamps in
 * the status report structure are valid.
 */

typedef enum al_bp_status_report_flags_t {
	BP_STATUS_RECEIVED = 0x01,
	BP_STATUS_CUSTODY_ACCEPTED = 0x02,
	BP_STATUS_FORWARDED = 0x04,
	BP_STATUS_DELIVERED = 0x08,
	BP_STATUS_DELETED = 0x10,
	BP_STATUS_ACKED_BY_APP = 0x20,
} al_bp_status_report_flags_t;

/**
 * Type definition for a unique bundle identifier. Returned from bp_send
 * after the daemon has assigned the creation_secs and creation_subsecs,
 * in which case orig_length and frag_offset are always zero, and also in
 * status report data in which case they may be set if the bundle is
 * fragmented.
 */

typedef struct al_bp_bundle_id_t {
	al_bp_endpoint_id_t source;
	al_bp_timestamp_t creation_ts;
	u32_t frag_offset;
	u32_t orig_length;
} al_bp_bundle_id_t;

/**
 * Type definition for a bundle status report.
 */

typedef struct al_bp_bundle_status_report_t {
	al_bp_bundle_id_t bundle_id;
	al_bp_status_report_reason_t reason;
	al_bp_status_report_flags_t flags;
	al_bp_timestamp_t receipt_ts;
	al_bp_timestamp_t custody_ts;
	al_bp_timestamp_t forwarding_ts;
	al_bp_timestamp_t delivery_ts;
	al_bp_timestamp_t deletion_ts;
	al_bp_timestamp_t ack_by_app_ts;
} al_bp_bundle_status_report_t;

typedef struct al_bp_bundle_payload_t {
	al_bp_bundle_payload_location_t location;
	struct {
		u32_t filename_len;
		char *filename_val;
	} filename;
	struct {
		uint32_t buf_crc;
		u32_t buf_len;
		char *buf_val;
	} buf;
	al_bp_bundle_status_report_t *status_report;
} al_bp_bundle_payload_t;

/**
 * AL BP API error codes
 */
typedef enum al_bp_error_t
{
	//Both Error
	BP_SUCCESS = 0, /* ok */
	BP_ERRBASE,		/* Base error code */
	BP_ENOBPI,		/* error NO Bundle Protocol Implementation */
	BP_EINVAL, 		/* invalid argument */
	BP_ENULLPNTR,	/* operation on a null pointer */
	BP_EUNREG,		/* errot to unregister eid*/
	BP_ECONNECT, 	/* error connecting to server */
	BP_ETIMEOUT, 	/* operation timed out */
	BP_ESIZE, 	 	/* payload / eid too large */
	BP_ENOTFOUND, 	/* not found (e.g. reg) */
	BP_EINTERNAL, 	/* misc. internal error */
	BP_EBUSY, 	 	/* registration already in use */
	BP_ENOSPACE,	/* no storage space */
	BP_ENOTIMPL,	/* function not yet implemented */
	BP_EATTACH, 	/* error to attach bp protocol */
	BP_EBUILDEID,	/* error to buil a local eid */
	BP_EOPEN, 		/* error to open */
	BP_EREG,		/* error to register a eid */
	BP_EPARSEEID,	/* error to parse a endpoint id string */
	BP_ESEND,		/* error to send bundle*/
	BP_ERECV,		/* error to receive bundle*/
	BP_ERECVINT		/* reception interrupted*/
} al_bp_error_t;

/****************************************************************
 *
 *             HIGHER LEVEL TYPES
 *
 ****************************************************************/


typedef struct al_bp_bundle_object_t {
	al_bp_bundle_id_t * id;
	al_bp_bundle_spec_t * spec;
	al_bp_bundle_payload_t * payload;
} al_bp_bundle_object_t;

#endif /* BP_TYPES_H_ */
