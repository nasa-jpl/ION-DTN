/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2021 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 **	bpsecadmin_config.h:	The security database administration interface
 **							header file, including:
 **
 **							- Permitted key names for JSON cmd key-value pairs
 **							- Mandatory key sets for cmds
 **							- Optional key sets for cmds
 **							- Supported security policy commands
 **
******************************************************************************/

/*****************************************************************************
 *                              FILE INCLUSIONS                              *
 *****************************************************************************/

#include "bpP.h"
#include "jsmn.h"
#include "bpsec_policy.h"
#include "bpsec_policy_eventset.h"
#include "bpsec_policy_event.h"
#include "bpsec_policy_rule.h"
#include "sci.h"
#include "sc_value.h"

/*****************************************************************************
 *                                 CONSTANTS                                 *
 *****************************************************************************/

#define RULE_ID_LEN 	(8)
#define MAX_JSMN_TOKENS (128)
#define MAX_RULE_ID		(BPSEC_MAX_NUM_RULES)

#define USER_TEXT_LEN   (1024)
#define JSON_CMD_LEN    (2048)
#define JSON_KEY_LEN	(32)
#define JSON_VAL_LEN	(32)
#define SEC_ROLE_LEN    (15)
#define NUM_STR_LEN     (5)

#define BPSEC_SEARCH_ALL  1
#define BPSEC_SEARCH_BEST 2

#define BPSEC_UNSUPPORTED_SC (0x10000) /* BPSec security contexts ids must 
                                          be signed, 16-bit integers */

/* Permitted Key IDs (KID) for security policy command key fields.
 * Note that security policy commands are provided using JSON key-value
 * pairs. All key ids defined below follow the format KID_<name of
 * permitted key>. For example, KID_DESC indicates that the key identifier
 * 'desc' is supported by bpsecadmin.                                   */

#define KID_NAME			(0x000001) 	/*0000 0000 0000 0000 0000 0001*/
#define KID_DESC			(0x000002)	/*0000 0000 0000 0000 0000 0010*/
#define KID_ES_REF	        (0x000004)	/*0000 0000 0000 0000 0000 0100*/
#define KID_EVENT_ID 		(0x000008)	/*0000 0000 0000 0000 0000 1000*/
#define KID_ACTIONS 		(0x000010)	/*0000 0000 0000 0000 0001 0000*/
#define KID_ID				(0x000020)	/*0000 0000 0000 0000 0010 0000*/
#define KID_REASON_CODE		(0x000040)	/*0000 0000 0000 0000 0100 0000*/
#define KID_NEW_VALUE	    (0x000080)	/*0000 0000 0000 0000 1000 0000*/
#define KID_MASK		    (0x000100)	/*0000 0000 0000 0001 0000 0000*/
#define KID_FILTER			(0x000200)	/*0000 0000 0000 0010 0000 0000*/
#define KID_SRC				(0x000400)	/*0000 0000 0000 0100 0000 0000*/
#define KID_DEST			(0x000800)	/*0000 0000 0000 1000 0000 0000*/
#define KID_SEC_SRC			(0x001000)	/*0000 0000 0001 0000 0000 0000*/
#define KID_SPEC			(0x002000)	/*0000 0000 0010 0000 0000 0000*/
#define KID_ROLE			(0x004000)	/*0000 0000 0100 0000 0000 0000*/
#define KID_TGT				(0x008000)	/*0000 0000 1000 0000 0000 0000*/
#define KID_TYPE		    (0x010000)	/*0000 0001 0000 0000 0000 0000*/
#define KID_SC_ID			(0x020000)	/*0000 0010 0000 0000 0000 0000*/
#define KID_SVC				(0x040000)	/*0000 0100 0000 0000 0000 0000*/
#define KID_RULE_ID			(0x080000)	/*0000 1000 0000 0000 0000 0000*/
#define KID_SC_PARMS        (0x100000)	/*0001 0000 0000 0000 0000 0000*/
#define KID_POLICYRULE   	(0x200000)  /*0010 0000 0000 0000 0000 0000*/
#define	KID_EVENT_SET       (0x400000)  /*0100 0000 0000 0000 0000 0000*/
#define KID_EVENT           (0x800000)  /*1000 0000 0000 0000 0000 0000*/

/* Key name strings (KNS) associated with the key IDs defined above as
 * KID_ for security policy command key fields. */
#define KNS_NAME 		"name"
#define KNS_DESC 		"desc"
#define KNS_ES_REF 		"es_ref"
#define KNS_EVENT_ID 	"event_id"
#define KNS_ACTIONS 	"actions"
#define KNS_ID 			"id"
#define KNS_REASON_CODE "reason_code"
#define KNS_NEW_VALUE 	"new_value"
#define KNS_MASK 		"mask"
#define KNS_FILTER 		"filter"
#define KNS_SRC 		"src"
#define KNS_DEST 		"dest"
#define KNS_SEC_SRC 	"sec_src"
#define KNS_SPEC 		"spec"
#define KNS_ROLE 		"role"
#define KNS_TGT 		"tgt"
#define KNS_TYPE		"type"
#define KNS_SC_ID 		"sc_id"
#define KNS_SVC 		"svc"
#define KNS_RULE_ID 	"rule_id"
#define KNS_SC_PARMS 	"sc_parms"
#define KNS_POLICYRULE 	"policyrule"
#define KNS_EVENT_SET 	"event_set"
#define KNS_EVENT 		"event"
#define KNS_VALUE		"value" //To remove with 4.2 update

/* Security policy commands are composed of JSON key-value pairs. The
 * keys can be identified as one of three types: mandatory, optional,
 * or invalid. The keys which match these types are different for
 * each security policy command.
 * 		Mandatory keys: Must be present for the command to be valid.
 * 		Optional keys:  May be present in the command. These keys are
 * 		                not required.
 * 		Invalid keys:   Any keys that are not mandatory or optional
 * 		                for the command. The presence of any key other
 * 		                than a mandatory/optional key causes the command
 * 		                to be invalid. */

/* Mandatory key names for event set commands */
#define MAND_ES_ADD_KEYS      (KID_NAME)
#define MAND_ES_INFO_KEYS     (KID_NAME)
#define MAND_ES_DEL_KEYS      (KID_NAME)
#define MAND_ES_LIST_KEYS     (0)

/* Optional key names for event set commands */
#define OPT_ES_ADD_KEYS       (KID_DESC)

/* Mandatory key names for event commands */
#define MAND_EVENT_ADD_KEYS   (KID_ES_REF | KID_EVENT_ID | KID_ACTIONS)
#define MAND_EVENT_DEL_KEYS   (KID_ES_REF | KID_EVENT_ID)

/* Mandatory key names for policy rule commands */
#define MAND_RULE_ADD_FILTER_KEYS (KID_ROLE | KID_TGT)
#define MAND_RULE_ADD_SPEC_KEYS   (KID_SVC)
#define MAND_RULE_ADD_KEYS        (KID_FILTER | KID_SPEC | KID_ES_REF)
#define MAND_RULE_DEL_KEYS		  (KID_RULE_ID)
#define MAND_RULE_INFO_KEYS       (KID_RULE_ID)
#define MAND_RULE_FIND_KEYS		  (KID_TYPE)
#define MAND_RULE_LIST_KEYS	      (0)

/* Optional key names for policy rule commands */
#define OPT_RULE_ADD_KEYS      (KID_DESC | KID_RULE_ID | KID_ROLE | KID_TGT| KID_SRC | KID_DEST | KID_SEC_SRC | KID_SVC | KID_SC_ID | KID_SC_PARMS)
#define OPT_RULE_FIND_KEYS	   (KID_SRC | KID_DEST | KID_SEC_SRC | KID_SC_ID | KID_ROLE | KID_TGT | KID_SVC | KID_ES_REF)

/* Optional key names to identify security policy commands */
#define OPT_POLICY_KEYS        (KID_POLICYRULE | KID_EVENT_SET | KID_EVENT)

#define HAS_MANDATORY_KEYS(cmdKeys, mandMask) ((cmdKeys & mandMask) == mandMask)
#define HAS_INVALID_KEYS(cmdKeys, mandMask, optMask) ((~(mandMask | optMask)) & cmdKeys)

/* Supported security policy commands */
typedef enum
{
	invalid = 0,
	add_event_set,
	delete_event_set,
	info_event_set,
	list_event_set,
	add_event,
	delete_event,
	add_policyrule,
	delete_policyrule,
	info_policyrule,
	find_policyrule,
	list_policyrule
} SecPolCmd;

typedef struct {char *key; int value;} BpSecMap;

extern BpSecMap gSvcMap[];
extern BpSecMap gRoleMap[];
extern BpSecMap gActionMap[];
extern BpSecMap gScParmMap[];
extern BpSecMap gKeyWords[];