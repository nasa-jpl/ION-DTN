/*
	bpsecadmin.c:	security database administration interface.


	Copyright (c) 2019, California Institute of Technology.	
	All rights reserved.
	Author: Scott Burleigh, Jet Propulsion Laboratory
	Modifications: TCSASSEMBLER, TopCoder

	Modification History:
	Date       Who     What
	9-24-13    TC      Added atouc helper function to convert char* to
			   unsigned char
	6-27-19	    SB	   Extracted from ionsecadmin.
	1-25-21    S. Heiner  Added initial BPSec Policy Rule Commands
*/


// TODO: Sub-parse objects - instantiate parsers per object to have end-of-object limits.

/*****************************************************************************
 *                              FILE INCLUSIONS                              *
 *****************************************************************************/

#include "bpsec.h"
#include "bpP.h"
#include "jsmn.h"
#include "bpsec_policy.h"
#include "bpsec_policy_eventset.h"
#include "bpsec_policy_event.h"
#include "bpsec_policy_rule.h"

/*****************************************************************************
 *                                 CONSTANTS                                 *
 *****************************************************************************/

#define RULE_ID_LEN 	(8)
#define MAX_JSMN_TOKENS (128)
#define MAX_RULE_ID		(255)


#define SEARCH_ALL 1
#define SEARCH_BEST 2

typedef struct {char *key; int value;} BpSecMap;


BpSecMap gRoleMap[] = {
	{"s",                BPRF_SRC_ROLE},
	{"source",           BPRF_SRC_ROLE},
	{"sec_source",       BPRF_SRC_ROLE},
	{"v",                BPRF_VER_ROLE},
	{"verifier",         BPRF_VER_ROLE},
	{"sec_verifier",     BPRF_VER_ROLE},
	{"a",                BPRF_ACC_ROLE},
	{"acceptor",         BPRF_ACC_ROLE},
	{"sec_acceptor",     BPRF_ACC_ROLE},
	{NULL,0}
};

BpSecMap gActionMap[] = {
	{"remove_sop",             BSLACT_REMOVE_SOP},
	{"remove_sop_target",      BSLACT_REMOVE_SOP_TARGET},
	{"remove_all_target_sops", BSLACT_REMOVE_ALL_TARGET_SOPS},
	{"do_not_forward",         BSLACT_DO_NOT_FORWARD},
	{"request_storage",        BSL_ACT_NOT_IMPLEMENTED}, //BSLACT_REQUEST_STORAGE},
	{"report_reason_code",     BSLACT_REPORT_REASON_CODE},
	{"override_target_bpcf",   BSL_ACT_NOT_IMPLEMENTED}, //BSLACT_OVERRIDE_TARGET_BPCF},
	{"override_sop_bpcf",      BSL_ACT_NOT_IMPLEMENTED}, //BSLACT_OVERRIDE_SOP_BPCF},
	{NULL,0}
};

BpSecMap gScParmMap[] = {
	{"key_file", CSI_PARM_KEYINFO},
	{"iv",       CSI_PARM_IV},
	{"salt",     CSI_PARM_SALT},
	{"icv",      CSI_PARM_ICV},
	{"intsig",   CSI_PARM_INTSIG},
	{"bek",      CSI_PARM_BEK},
	{"bekicv",   CSI_PARM_BEKICV},
	{NULL,0}
};


/*****************************************************************************
 *                              GLOBAL VARIABLES                             *
 *****************************************************************************/

PsmPartition gWm;


typedef struct
{
	int tokenCount;
	jsmntok_t tokens[MAX_JSMN_TOKENS];
	char *line;
} jsonObject;

/*****************************************************************************
 *                             FUNCTION PROTOTYPES                          *
 *****************************************************************************/


/*****************************************************************************
 *                             FUNCTION DEFINITIONS                          *
 *****************************************************************************/

static char	*_omitted()
{
	return "";
}

static int	_echo(int *newValue)
{
	static int	state = 0;

	if (newValue)
	{
		if (*newValue == 1)
		{
			state = 1;
		}
		else
		{
			state = 0;
		}
	}

	return state;
}

static void	printText(char *text)
{
	if (_echo(NULL))
	{
		writeMemo(text);
	}

	PUTS(text);
}

static void	handleQuit(int signum)
{
	printText("Please enter command 'q' to stop the program.");
}

static void	printSyntaxError(int lineNbr)
{
	char	buffer[80];

	isprintf(buffer, sizeof(buffer), "Syntax error at line %d of \
bpsecadmin.c", lineNbr);
	printText(buffer);
}

#define	SYNTAX_ERROR	printSyntaxError(__LINE__)

/*
 * TODO: update to show the acceptance of JSON
 */
static void	printUsage()
{
	PUTS("Valid commands are:");
	PUTS("\tq\tQuit");
	PUTS("\th\tHelp");
	PUTS("\t?\tHelp");
	PUTS("\tv\tPrint version of ION.");

	PUTS("Criteria fields for policyrule commands:");
	PUTS("\tFilter Criteria");
	PUTS("\t\t [role = <security policy role>, src = <source eid expression>, \
dest = <destination eid expression>, tgt = <target block type>, req_scid = \
<security context name>]");
	PUTS("\tSpecification Criteria");
	PUTS("\t\t [svc = <security service>, scid = <security context name>, \
sc_parms= <security context parameters>]");
	PUTS("\tEvent Criteria");
	PUTS("\t\t [event_set = <event set name> | event = <security operation \
event>, actions = <optional processing actions>, action_parms = <processing \
action parameters>]");


	PUTS("\ta\tAdd");
	PUTS("\t\tEvery eid expression must be a node identification \
expression, i.e., a partial eid expression ending in '*' or '~'.");
	PUTS("\t   a bibrule <source eid expression> <destination eid \
expression> <block type number> { '' | <ciphersuite name> <key name> }");
	PUTS("\t   a bcbrule <source eid expression> <destination eid \
expression> <block type number> { '' | <ciphersuite name> <key name> }");
	PUTS("\t   a eventset <event set name>");
	PUTS("\t   a event event_set=<event set name> event=<security \
operation event> actions=<optional processing actions> action_parms={ '' | \
<processing action parameters>}");
	PUTS("\t   a policyrule [filter criteria] [specification criteria] \
[event criteria]");

	PUTS("\tc\tChange");
	PUTS("\t   c bibrule <source eid expression> <destination eid \
expression> <block type number> { '' | <ciphersuite name> <key name> }");
	PUTS("\t   c bcbrule <source eid expression> <destination eid \
expression> <block type number> { '' | <ciphersuite name> <key name> }");
	PUTS("\t   c policyrule rule_id [filter criteria] [specification criteria]\
[event criteria]");

	PUTS("\td\tDelete");
	PUTS("\t   d bibrule <source eid expression> <destination eid \
	expression> <block type number>");
	PUTS("\t   d bcbrule <source eid expression> <destination eid \
	expression> <block type number>");
	PUTS("\t   d eventset <event set name>");
	PUTS("\t   d event event_set=<event set name> event=<security operation \
event> actions={ '' | <optional processing actions> ");
	PUTS("\t   d policyrule rule_id");

	PUTS("\ti\tInfo");
	PUTS("\t   i bibrule <source eid expression> <destination eid \
expression> <block type number>");
	PUTS("\t   i bcbrule <source eid expression> <destination eid \
expression> <block type number>");
	PUTS("\t   i eventset <event set name>");
	PUTS("\t   i policyrule [filter criteria]");

	PUTS("\tl\tList");
	PUTS("\t   l bibrule");
	PUTS("\t   l bcbrule");
	PUTS("\t   l eventset");

	PUTS("\tx\tClear BSP security rules.");
	PUTS("\t   x <security source eid> <security destination eid> \
{ 2 | 3 | 4 | ~ }");
	PUTS("\te\tEnable or disable echo of printed output to log file");
	PUTS("\t   e { 0 | 1 }");
	PUTS("\t#\tComment");
	PUTS("\t   # <comment text>");
}

/******************************************************************************
 *
 * \par Function Name: init
 *
 * \par Purpose: This function initializes the bpsecadmin utility, attaching
 *               to the security database and initializing ION working memory.
 *
 * \retval void
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/22/21   S. Heiner      Initial Implementation
 *****************************************************************************/
static void init()
{
	if (secInitialize() < 0)
	{
		printText("Can't initialize the ION security system.");
	}

	if (secAttach() != 0)
	{
		printText("Failed to attach to security database.");
	}

	SecVdb *vdb = getSecVdb();
	if (vdb == NULL)
	{
		printText("Failed to retrieve security database.");
	}

	gWm = getIonwm();

	if (bsl_all_init(gWm) < 1)
	{
		printText("Failed to initialize BPSec policy");
	}
}

/******************************************************************************
 *
 * \par Function Name: jsonStrEqual
 *
 * \par Purpose: This function compares a jsmn token with a given string and
 * 				 determines if their contents are equivalent.
 *
 * \retval int -1  - Error.
 *              0  - JSON token contents are NOT equal to the provided string
 *             >0  - JSON token contents are equal to the provided string
 *
 * \param[in]  json		JSON string token was extracted from
 * \param[in]  tok      jsmn token to examine
 * \param[in]  str		String to compare jsmn token contents to
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  12/22/20   S. Heiner      Initial Implementation
 *****************************************************************************/
static int jsonStrEqual(const char *json, jsmntok_t *tok, const char *str)
{
  return (((tok->type == JSMN_STRING) && ((int)strlen(str) == tok->end - tok->start))
		  && (strncmp(json + tok->start, str, tok->end - tok->start) == 0));
}

/******************************************************************************
 *
 * \par Function Name: jsonStrCpy
 *
 * \par Purpose: This function copies a string from a line of JSON to the
 * 				 destination (str) provided by the user.
 *
 * \retval int -1  - Error.
 *              0  - The JSON string was not copied.
 *             >0  - The JSON string was successfully copied to the destination
 *
 * \param[in]      tok      jsmn token to copy the string from
 * \param[in]      maxLen	The maximum length of the destination string
 * \param[in]      json		JSON string token was extracted from
 * \param[in|out]  str		String to copy jsmn token contents to
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  12/30/20   S. Heiner      Initial Implementation
 *****************************************************************************/
static int jsonStrCpy(jsmntok_t *tok, int maxLen, char *json, char *str)
{
	if(tok->type == JSMN_STRING)
	{
		int len = MIN(tok->end - tok->start, maxLen);
		memset(str,0,maxLen);
		return ((len > 0) && (strncpy(str, json + tok->start, len)));
	}
	else
	{
		return 0;
	}
}

/******************************************************************************
 *
 * \par Function Name: jsonPrimitiveCpy
 *
 * \par Purpose: This function copies a primitive valuE from a line of JSON to
 *               the destination (str) provided by the user.
 *
 * \retval int -1  - Error.
 *              0  - The JSON primitive was not copied.
 *             >0  - The JSON primitive was successfully copied to the
 *                   destination
 *
 * \param[in]      tok      jsmn token to copy the primitive from
 * \param[in]      maxLen	The maximum length of the destination string
 * \param[in]      json		JSON string token was extracted from
 * \param[in|out]  str		String to copy jsmn token contents to
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  12/30/20   S. Heiner      Initial Implementation
 *****************************************************************************/
static int jsonPrimitiveCpy(jsmntok_t *tok, int maxLen, char *json, char *str)
{
	if(tok->type == JSMN_PRIMITIVE)
	{
		int len = MIN(tok->end - tok->start, maxLen);

		return (strncpy(str, json + tok->start, len)) ? 1 : 0;
	}
	return -1;
}

// Get index of item directly after key, IFF it is of correct type.
// END is the last index we will look for the KEY. Value can be end+1.
int json_get_typed_idx(jsonObject job, int start, int end, char *key, int type)
{
	int i;

	CHKZERO(key);

	if(end <= 0)
	{
		end = job.tokenCount - 1;
	}

	for (i = start; i <= end; i++)
	{
		if(jsonStrEqual(job.line, &(job.tokens[i]), key))
		{
			i++;
			return (job.tokens[i].type == type) ? i : 0;
		}
	}

	return 0;
}

// returned index is the index of the VALUE for the key, which is 1 plus the index of the key.
// So, idx can be end+1.
int json_get_primitive_value(jsonObject job, int start, int end, char *key, int max, char *value, int *idx)
{
	int i = 0;

	i = json_get_typed_idx(job, start, end, key, JSMN_PRIMITIVE);

	if(i > 0)
	{
		if(idx != NULL)
		{
			*idx = i;
		}
		return jsonPrimitiveCpy(&(job.tokens[i]), max, job.line, value);
	}

	return 0;
}

// value associated with NAME   starting at index i.
/*
 * Retrieves value of key item IFF the value is of the given type.
 *
 */

int json_get_str_value(jsonObject job, int start, int end, char *key, int max, char *value, int *idx)
{
	int i = 0;
	int result = 0;

	result = i = json_get_typed_idx(job, start, end, key, JSMN_STRING);

	if(i > 0)
	{
		if(idx != NULL)
		{
			*idx = i;
		}
		result = jsonStrCpy(&(job.tokens[i]), max, job.line, value);
	}

	return result;
}

char *json_get_new_str_value(jsonObject job, int start, int end, char *key, int max, int *idx)
{
	char *tmp = (char*) MTAKE(max+1);

	if(json_get_str_value(job, start, end, key, max, tmp, idx) <= 0)
	{
		MRELEASE(tmp);
		tmp = NULL;
	}

	return tmp;
}

int getMappedValue(BpSecMap map[], char *key)
{
	int idx = 0;

	while(map[idx].key != NULL)
	{
		if(strcmp(key, map[idx].key) == 0)
	    {
	      return map[idx].value;
	    }
	    idx++;
	}

	return 0;
}


/******************************************************************************
 *
 * \par Function Name: processEvent
 *
 * \par Purpose: Validate a provided event. This function checks that
 * 				 an event name token is:
 * 				 1) Present
 * 				 2) Within the imposed length guidelines
 * 				 3) Labeled with 'event_id'
 * 				 4) A supported security operation event
 * 				 If the provided event is valid, the function returns that
 * 				 event name.
 *
 * \retval int -1  - Error.
 *              0  - Event name is invalid
 *             >0  - Event name is valid, BpSecEventId is populated
 *
 * \param[in]  tokenCount  Number of tokens returned by the jsmn parser.
 * \param[in]  tokens      A pointer to all tokens found by the jsmn parser.
 * \param[in]  line        The command provided by the user.
 * \param[in|out]  event   The valid event ID.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  12/17/20   S. Heiner      Initial Implementation
 *****************************************************************************/
static int getEventId(jsonObject job, BpSecEventId *event)
{
	char eventName[MAX_EVENT_LEN+1];
	int result = 1;

	CHKERR(event);

	if(json_get_str_value(job, 1, 0, "event_id", MAX_EVENT_LEN, eventName, NULL) > 0)
	{
		/* Check that provided event is supported */
		*event = bslevt_get_id(eventName);

		if(*event == unsupported)
		{
			writeMemo("[?] Event is unsupported");
			result = 0;
		}
	}
	else
	{
		writeMemo("[?] Event format incorrect");
		result = -1;
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name: setAction
 *
 * \par Purpose: Given an optional processing action, this function sets the
 * 				 appropriate bit in the provided action mask to indicate that
 * 				 the action has been enabled.
 *
 * \retval - int 1 - Provided action is supported and bit in actionMask set.
 * 				 0 - An actionMask bit was not set.
 *
 * \param[in]        action - The optional processing action to enable.
 * \parm[in|out] actionMask - Currently configured optional processing actions.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  12/17/20   S. Heiner      Initial Implementation
 *****************************************************************************/
// Returns action value...
static int setAction(char *action, uint8_t *actionMask)
{
  int value = 0;

  if((value = getMappedValue(gActionMap, action)) > 0)
  {
	  if(value == BSL_ACT_NOT_IMPLEMENTED)
	  {
		  writeMemoNote("[i] Action currently not supported", action);
	  }
	  else
	  {
		  *actionMask |= value;
	  }
  }
  else
  {
	  writeMemoNote("[?] Unknown action", action);
  }

  return value;
}

/******************************************************************************
 *
 * \par Function Name: processActions
 *
 * \par Purpose: This function processes user-provided action(s). The actions
 *               are first validated. Then, each valid action's bit is set in
 *               the actionMask provided. All actions must be valid, meaning
 *               that each action provided corresponds to a supported action,
 *               in order for this function to return success.
 *
 * \retval int -1 Error
 * 				0 Action(s) unsuccessfully processed - action(s) invalid.
 * 				1 Action(s) successfully processed - action(s) valid.
 *
 * \param[in]  tokenCount  Number of tokens returned by the jsmn parser.
 * \param[in]  tokens      A pointer to all tokens found by the jsmn parser.
 * \param[in]  line        The command provided by the user.
 * \param[in|out]  event   The valid event name.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  12/17/20   S. Heiner      Initial Implementation
 *****************************************************************************/
// Presumes parms has been allocated...
static int getActions(jsonObject job, uint8_t *actionMask, BpSecEvtActionParms *parms)
{
	int i = 0;
	int start = 0;
	int actionLen = 0;
	char actionStr[MAX_ACTION_LEN+1];
	int parmIdx = 0;
	char parmStr[64+1];
	int numParm = 0;
	int curAct = 0;

	/* Get the index of the start of the actions array. */
	start = json_get_typed_idx(job, 1, 0, "actions", JSMN_ARRAY);

	if(start <= 0)
	{
		writeMemo("[?] Error while parsing action(s)");
		return -1;
	}

	memset(parmStr,0,sizeof(parmStr));

	while (i < job.tokenCount)
	{
		/* Grab the next action in the array. */
		if((actionLen = json_get_str_value(job, start, 0, "id", MAX_ACTION_LEN, actionStr, &start)) <= 0)
		{
			return 0;
		}
		curAct = setAction(actionStr, actionMask);
		switch(curAct)
		{
			case BSLACT_REPORT_REASON_CODE:
				if(json_get_primitive_value(job, start, start+1, "reason_code", 64, parmStr, &start) > 0)
				{
				   parms[parmIdx++].asReason.reasonCode = atoi(parmStr);
				}
				break;
			case BSLACT_OVERRIDE_TARGET_BPCF:
			case BSLACT_OVERRIDE_SOP_BPCF:
				numParm = 0;
				if(json_get_primitive_value(job, start, start+3, "mask", 64, parmStr, NULL) > 0)
				{
				   parms[parmIdx].asOverride.mask = (uint64_t) atol(parmStr);
				   numParm++;
				}
				if(json_get_primitive_value(job, start, start+3, "new_value", 64, parmStr, NULL) > 0)
				{
				   parms[parmIdx].asOverride.val = (uint64_t) atol(parmStr);
				   numParm++;
				}
				if(numParm != 2)
				{
					writeMemo("[?] Incorrect number of action parms");
					parms[parmIdx].asOverride.val = parms[parmIdx].asOverride.mask = 0;
					return -1;
				}
				else
				{
					parmIdx++;
				}
				break;
			case 0:
				writeMemo("[?] Unknown action");
				return 0;
				break;
			case BSL_ACT_NOT_IMPLEMENTED:
				writeMemoNote("[?] Action not implemented",actionStr);
				return 0;
				break;
			default:
				break;
		}
	}
	return 1;
}


/******************************************************************************
 *
 * \par Function Name: getFilterCriteria
 *
 * \par Purpose: This function processes user-provided filter criteria and
 * 				 populates the given parameters with their values.
 *
 * \retval int -1 Error
 * 				0 Filter criteria parsed unsuccessfully
 * 				1 Filter criteria parsed successfully - parameters populated
 *
 * \param[in]  tokenCount  Number of tokens returned by the jsmn parser.
 * \param[in]  tokens      A pointer to all tokens found by the jsmn parser.
 * \param[in]  line        The command provided by the user.
 * \param[in|out]  bsrc    Bundle source.
 * \param[in|out]  bdest   Bundle destination.
 * \param[in|out]  ssrc    Security source.
 * \param[in|out]  type    Security target block type.
 * \param[in|out]  role    Security role.
 * \param[in|out]  sc_id   Security context ID.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/05/21   S. Heiner      Initial Implementation
 *****************************************************************************/
static int getFilterCriteria(jsonObject job, char *bsrc, char *bdest, char *ssrc, int *type, int *role, int *sc_id)
{
	int result = 0;

	char num_str[4];
	char role_str[13];

	*role = 0;
	*type = -1;
	*sc_id = 0;

	if(json_get_str_value(job, 1, 0, "src", MAX_EID_LEN, bsrc, NULL) < 0)
	{
		writeMemo("[?] Malformed bundle source provided");
		return 0;
	}

	if(json_get_str_value(job, 1, 0, "dest", MAX_EID_LEN, bdest, NULL) < 0)
	{
		writeMemo("[?] Malformed bundle destination provided");
		return 0;
	}

	if(json_get_str_value(job, 1, 0, "sec_src", MAX_EID_LEN, ssrc, NULL) < 0)
	{
		writeMemo("[?] Malformed security source provided");
		return 0;
	}

	if((result = json_get_primitive_value(job, 1, 0, "tgt", 3, num_str, NULL)) < 0)
	{
		writeMemo("[?] Malformed target block type provided");
		return 0;
	}
	else if (result > 0)
	{
		*type = atoi(num_str);
	}

	// Result can be string or primitive.
	if((result = json_get_primitive_value(job, 1, 0, "role", 13, role_str, NULL)) <= 0)
	{
		result = json_get_str_value(job, 1, 0, "role", 13, role_str, NULL);
	}

	if(result < 0)
	{
		writeMemo("[?] Malformed security role provided");
		return 0;
	}
	else if (result > 0)
	{
		*role = getMappedValue(gRoleMap, role_str);
	}

	if((result = json_get_primitive_value(job, 1, 0, "sc_id", 3, num_str, NULL)) < 0)
	{
		writeMemo("[?] Malformed security context identifier provided");
		return 0;
	}
	else if (result > 0)
	{
		*sc_id = atoi(num_str);
	}


	return 1;
}

/******************************************************************************
 *
 * \par Function Name: parseFilter
 *
 * \par Purpose: This function processes user-provided filter contents and
 * 				 populates a pointer to BPsecFilter. A filter MUST contain at
 * 				 least one EID to be considered valid.
 *
 * \retval int -1 Error
 * 				0 Filter contents invalid
 * 				1 Filter contents valid - BpSecFilter populated
 *
 * \param[in]  tokenCount  Number of tokens returned by the jsmn parser.
 * \param[in]  tokens      A pointer to all tokens found by the jsmn parser.
 * \param[in]  line        The command provided by the user.
 * \param[in|out]  filter  Pointer to BPsecFilter to be populated if filter
 * 						   contents are valid.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  12/28/20   S. Heiner      Initial Implementation
 *****************************************************************************/
static int parseFilter(jsonObject job, BpSecFilter *filter)
{
	int type = -1;
	int role = 0;
	int sc_id = 0;

	char bsrc[MAX_EID_LEN];
	char bdest[MAX_EID_LEN];
	char ssrc[MAX_EID_LEN];

	memset(bsrc, '\0', sizeof(bsrc));
	memset(bdest, '\0', sizeof(bdest));
	memset(ssrc, '\0', sizeof(ssrc));

	if (getFilterCriteria(job, bsrc, bdest, ssrc, &type, &role, &sc_id))
	{
		/* After parsing JSON, build filter for policy rule*/
		*filter = bslpol_filter_build(gWm, bsrc, bdest, ssrc, type, role, sc_id);

		if(filter->flags)
		{
			return 1;
		}

		else
		{
			writeMemo("[?] Filter information invalid");
			return 0;
		}
	}

	writeMemo("[?] Malformed filter criteria");
	return 0;
}

static void sc_parm_del(LystElt elt, void *arg)
{
	void *data = lyst_data(elt);
	if(data)
	{
		MRELEASE(data);
	}
}

/******************************************************************************
 *
 * \par Function Name: getSecCtxtParms
 *
 * \par Purpose: This function processes the security context parameters
 *               provided by a user when creating a security policy rule.
 *               A Lyst of security context parameters, type sci_inbound_tlv,
 *               is created and returned if all parameters are formatted and
 *               parsed successfully.
 *
 * \retval int -1 Error
 * 				0 Security context parameter(s) malformed.
 * 				1 Security context parameter(s) successfully processed.
 *
 * \param[in]  tokenCount      Number of tokens returned by the jsmn parser.
 * \param[in]  tokens          Pointer to all tokens found by the jsmn parser.
 * \param[in]  line            The command provided by the user.
 * \param[in|out] parms        Lyst of each sci_inbound_tlv parsed from JSON.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/04/21   S. Heiner      Initial Implementation
 *****************************************************************************/
static Lyst getSecCtxtParms(jsonObject job)
{
	int i = 0;
	int start = 0;
	char curId[32+1];
	char curVal[32+1];
	sci_inbound_tlv *curParm = NULL;
	Lyst parms = NULL;

	if((start = json_get_typed_idx(job, 1, 0, "sc_parms", JSMN_ARRAY)) <= 0)
	{
		return NULL;
	}

	parms = lyst_create();
	CHKNULL(parms);
	lyst_delete_set(parms,sc_parm_del,NULL);

	i = start;
	while(i > 0)
	{
		// Get the next SC Parm id.
		if(json_get_str_value(job, i, 0, "id", 32, curId, &i) <= 0)
		{
			i = 0;
		}
		else
		{
			curParm = (sci_inbound_tlv*) MTAKE(sizeof(sci_inbound_tlv));
			CHKNULL(curParm);
			memset(curParm, 0, sizeof(sci_inbound_tlv));

			/* Init the parm Id */
			curParm->id = getMappedValue(gScParmMap, curId);

			/* Read the parm value into a tmp variable. */
			if((curParm->length = json_get_str_value(job, i, 0, "value", 32, curVal, &i)) < 0)
			{
				MRELEASE(curParm);
				lyst_destroy(parms);
				return NULL;
			}

			/* Allocate space for variable in the sc parm and copy it in. */
			if((curParm->value = MTAKE(curParm->length + 1)) == NULL)
			{
				MRELEASE(curParm);
				lyst_destroy(parms);
				return NULL;
			}
			memset(curParm->value, 0, curParm->length+1);
			memcpy(curParm->value, curVal, curParm->length);

			lyst_insert(parms, curParm);
		}
	}

	return parms;
}



/******************************************************************************
 *
 * \par Function Name: getRuleId
 *
 * \par Purpose: This function extracts a user-provided rule ID from a JSON
 *               command.
 *
 * \retval int -1 Error
 * 				0 Rule ID invalid
 * 				1 Rule ID valid - pointer updated.
 *
 * \param[in]  tokenCount  Number of tokens returned by the jsmn parser.
 * \param[in]  tokens      A pointer to all tokens found by the jsmn parser.
 * \param[in]  line        The command provided by the user.
 * \param[in|out]  ruleID  The rule ID
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/05/21   S. Heiner      Initial Implementation
 *****************************************************************************/
static int getRuleId(jsonObject job, uint16_t *ruleId)
{
	int idLen = 0;
	char id[RULE_ID_LEN+1];
	memset(id, '\0', sizeof(id));

	if((idLen = json_get_str_value(job, 1, 0, "rule_id", RULE_ID_LEN, id, NULL)) <= 0)
	{
		return 0;
	}

	*ruleId= atoi(id);
	return 1;
}

/******************************************************************************
 *
 * \par Function Name: getNewRuleId
 *
 * \par Purpose: This function processes a user-provided rule ID. If the rule
 * 				 ID is formatted correctly and is not already assigned to an
 * 				 existing rule, the ruleID pointer is updated by
 * 				 the function.
 *
 * \retval int -1 Error
 * 				0 Rule ID invalid
 * 				1 Rule ID valid - pointer updated.
 *
 * \param[in]  tokenCount  Number of tokens returned by the jsmn parser.
 * \param[in]  tokens      A pointer to all tokens found by the jsmn parser.
 * \param[in]  line        The command provided by the user.
 * \param[in|out]  ruleID  The valid rule ID
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  12/28/20   S. Heiner      Initial Implementation
 *****************************************************************************/
static int getNewRuleId(jsonObject job, uint16_t *ruleId)
{

	/* If a rule ID has not been provided by the user, generate one */
	if(!getRuleId(job, ruleId))
	{
		int i;
		for(i = 1; i < MAX_RULE_ID; i++)
		{
			/*
			 * Use first-available rule ID that is not yet associated with
			 * a policy rule.
			 */
			if (bslpol_rule_get_ptr(gWm, i) == NULL)
			{
				*ruleId = i;
				return 1;
			}
		}

		writeMemo("[?] No available rule IDs - max number of policy "
				"rules have been defined");
		return 0;
	}

	/* Otherwise, check if user rule ID is already in use */
	else
	{
		if (bslpol_rule_get_ptr(gWm, *ruleId) == NULL)
		{
			return 1;
		}

		else
		{
			writeMemo("[?] Rule ID already in use");
			return 0;
		}
	}

	return -1;
}

/******************************************************************************
 *
 * \par Function Name: createAnonEventset
 *
 * \par Purpose: This function creates an anonymous event set to be associated
 *               with a single security policy rule.
 *
 * \retval PsmAddress - anonymous event set
 *
 * \param[in]  tokenCount      Number of tokens returned by the jsmn parser.
 * \param[in]  tokens          Pointer to all tokens found by the jsmn parser.
 * \param[in]  line            The command provided by the user containing the
 *                             details of the anonymous event set to be
 *                             constructed.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/22/21   S. Heiner      Initial Implementation
 *****************************************************************************/
PsmAddress createAnonEventset(jsonObject job)
{

	writeMemo("[?] Anonymous event sets unsupported.");
	return 0;
}

static void	executeAdd(int tokenCount, char **tokens)
{
	char	*keyName = "";

	if (tokenCount < 2)
	{
		printText("Add what?");
		return;
	}

	if (strcmp(tokens[1], "bibrule") == 0)
	{
		switch (tokenCount)
		{
		case 7:
			keyName = tokens[6];
			break;

		case 6:
			keyName = _omitted();
			break;

		default:
			SYNTAX_ERROR;
			return;
		}

		sec_addBPsecBibRule(tokens[2], tokens[3], atoi(tokens[4]),
				tokens[5], keyName);
		return;
	}

	if (strcmp(tokens[1], "bcbrule") == 0)
	{
		switch (tokenCount)
		{
		case 7:
			keyName = tokens[6];
			break;

		case 6:
			keyName = _omitted();
			break;

		default:
			SYNTAX_ERROR;
			return;
		}

		sec_addBPsecBcbRule(tokens[2], tokens[3], atoi(tokens[4]),
				tokens[5], keyName);
		return;
	}

	SYNTAX_ERROR;
}

/******************************************************************************
 *
 * \par Function Name: executeAddJson
 *
 * \par Purpose: This function executes the add command provided by the user if
 * 				 JSON is present.
 *
 * \param[in]  tokenCount  The number of parsed jsmn tokens.
 * \param[in]  tokens      jsmn token(s).
 * \param[in]  line        'Add' command using JSON syntax.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  12/30/20   S. Heiner      Initial Implementation
 *****************************************************************************/
static void	executeAddJson(jsonObject job)
{
	int start = 0;
	char name[MAX_EVENT_SET_NAME_LEN];

	memset(name, '\0', sizeof(name));

	if (job.tokenCount < 2)
	{
		printText("Add what?");
		return;
	}

	if((start = json_get_typed_idx(job, 1, 0, "event_set", JSMN_OBJECT)) > 0)
	{
		if(json_get_str_value(job, start, 0, "name", MAX_EVENT_SET_NAME_LEN, name, NULL))
		{
			if(bsles_add(gWm, name) < 0)
			{
				writeMemoNote("[?] Error adding eventset: ", name);
			}
		}
	}
	else if((start = json_get_typed_idx(job, 1, 0, "event", JSMN_OBJECT)) > 0)
	{
		if(json_get_str_value(job, start, 0, "es_ref", MAX_EVENT_SET_NAME_LEN, name, NULL))
		{
			if(bsles_get_ptr(gWm, name))
			{
				uint8_t actionMask = 0;
				BpSecEvtActionParms actionParms[BSLACT_MAX_PARM];
				BpSecEventId eventId = 0;

				memset(actionParms,0,sizeof(actionParms));

				if(getEventId(job, &eventId) < 0)
				{
					writeMemoNote("[?] Malformed EventId for: ", name);
				}
				else if(getActions(job, &actionMask, actionParms) < 0)
				{
					writeMemoNote("[?] Malformed actions for: ", name);
				}
				else if(bslevt_add(gWm, name, eventId, actionMask, actionParms) <= 0)
				{
					writeMemoNote("[?] Cannot add event for: ", name);
				}
			}
		}
	}
	else if((start = json_get_typed_idx(job, 1, 0, "policyrule", JSMN_OBJECT)) > 0)
	{
		BpSecFilter filter;
		Lyst sci_parms = NULL;
		uint16_t id = 0;
		PsmAddress esAddr = 0;
		char desc[BPSEC_RULE_DESCR_LEN+1];

		if (!parseFilter(job, &filter))
		{
			writeMemo("[?] Filter criteria could not be processed");
		}
		else if (!getNewRuleId(job, &id))
		{
			writeMemo("[?] Rule ID could not be processed");
		}
		else if(json_get_str_value(job, start, 0, "desc", BPSEC_RULE_DESCR_LEN, desc, NULL) < 0)
		{
			writeMemo("[x] Error reading optional rule description.");
		}
		else if(json_get_str_value(job, start, 0, "es_ref", MAX_EVENT_SET_NAME_LEN, name, NULL) <= 0)
		{
			writeMemo("[?] Missing Event set reference.");
		}
		else if((esAddr = bsles_get_addr(gWm, name)) == 0)
		{
			writeMemo("[?] Undefined Event set.");
		}
		else if((sci_parms = getSecCtxtParms(job)) == NULL)
		{
			writeMemo("[?] Security context parameters could not be processed");
		}
		else
		{
			PsmAddress ruleAddr = 0;

			if((ruleAddr = bslpol_rule_create(gWm, desc, id, 0, filter, sci_parms, esAddr)) == 0)
			{
				writeMemo("[?] Could not create rule");
			}

			if(bslpol_rule_insert(gWm, ruleAddr) <= 0)
			{
				writeMemo("[?] Could not insert rule");
			}

			lyst_destroy(sci_parms);
		}
	}
	else
	{
		SYNTAX_ERROR;
	}
}

static void	executeChange(int tokenCount, char **tokens)
{
	char	*keyName;

	if (tokenCount < 2)
	{
		printText("Change what?");
		return;
	}

	if (strcmp(tokens[1], "bibrule") == 0)
	{
		switch (tokenCount)
		{
		case 7:
			keyName = tokens[6];
			break;

		case 6:
			keyName = _omitted();
			break;

		default:
			SYNTAX_ERROR;
			return;
		}

		sec_updateBPsecBibRule(tokens[2], tokens[3], atoi(tokens[4]),
				tokens[5], keyName);
		return;
	}

        if (strcmp(tokens[1], "bcbrule") == 0)
        {
                switch (tokenCount)
                {
                case 7:
                        keyName = tokens[6];
                        break;

                case 6:
                        keyName = _omitted();
                        break;

                default:
                        SYNTAX_ERROR;
                        return;
		}

                sec_updateBPsecBcbRule(tokens[2], tokens[3], atoi(tokens[4]),
                                tokens[5], keyName);
                return;
        }

	SYNTAX_ERROR;
}

/******************************************************************************
 *
 * \par Function Name: executeChangeJson
 *
 * \par Purpose: This function executes the change command provided by the user
 *               if JSON is present.
 *
 * \param[in]  tokenCount  The number of parsed jsmn tokens.
 * \param[in]  tokens      jsmn token(s).
 * \param[in]  line        'Change' command using JSON syntax.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  12/30/20   S. Heiner      Initial Implementation
 *****************************************************************************/
static void	executeChangeJson(jsonObject job)
{
#if 0
	int start = 0;

	if (job.tokenCount < 2)
	{
		printText("Change what?");
		return;
	}

	if((start = json_get_typed_idx(job, 1, 0, "policyrule", JSMN_OBJECT)) > 0)
	{
		BpSecFilter filter;
		Lyst sci_parms = NULL;
		uint16_t id = 0;
		PsmAddress ruleAddr = 0;
		PsmAddress esAddr = 0;
		char name[MAX_EVENT_SET_NAME_LEN];

		memset(name, '\0', sizeof(name));

		if ((getRuleId(job, &id) > 0) &&
		    ((ruleAddr = bslpol_rule_get_addr(gWm, id)) > 0))
		{
			/* Delete existing rule */
			bslpol_rule_delete(gWm, ruleAddr);

			/* Create new rule with same ID as deleted rule */
			if (!parseFilter(job, &filter))
			{
				writeMemo("[?] Filter criteria could not be processed");
			}
			else if(json_get_str_value(job, start, 0, "es_ref", MAX_EVENT_SET_NAME_LEN, name, NULL) <= 0)
			{
				writeMemo("[?] Missing Event set reference.");
			}
			else if((esAddr = bsles_get_addr(gWm, name)) == 0)
			{
				writeMemo("[?] Undefined Event set.");
			}
			else if((sci_parms = getSecCtxtParms(job)) == NULL)
			{
				writeMemo("[?] Security context parameters could not be processed");
			}
			else
			{
				if(bslpol_rule_create(gWm, id, 0, filter, sci_parms, esAddr) == 0)
				{
					writeMemo("[?] Could not create rule");
				}

				lyst_destroy(sci_parms);
			}
		}
	}
#endif
	SYNTAX_ERROR;
}

static void	executeDelete(int tokenCount, char **tokens)
{
	if (tokenCount < 3)
	{
		printText("Delete what?");
		return;
	}

	if (strcmp(tokens[1], "bibrule") == 0)
	{
		if (tokenCount != 5)
		{
			SYNTAX_ERROR;
			return;
		}
		sec_removeBPsecBibRule(tokens[2], tokens[3], atoi(tokens[4]));
		return;
	}

	if (strcmp(tokens[1], "bcbrule") == 0)
	{
		if (tokenCount != 5)
		{
			SYNTAX_ERROR;
			return;
		}
		sec_removeBPsecBcbRule(tokens[2], tokens[3], atoi(tokens[4]));
		return;
	}
	SYNTAX_ERROR;
}

/******************************************************************************
 *
 * \par Function Name: executeDeleteJson
 *
 * \par Purpose: This function executes the delete command provided by the
 *               user if JSON is present.
 *
 * \param[in]  tokenCount  The number of parsed jsmn tokens.
 * \param[in]  tokens      jsmn token(s).
 * \param[in]  line        'Delete' command using JSON syntax.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  12/30/20   S. Heiner      Initial Implementation
 *****************************************************************************/
static void	executeDeleteJson(jsonObject job)
{
	int start = 0;
	char name[MAX_EVENT_SET_NAME_LEN];
	memset(name, '\0', sizeof(name));

	if (job.tokenCount < 2)
	{
		printText("Delete what?");
		return;
	}

	if((start = json_get_typed_idx(job, 1, 0, "event_set", JSMN_OBJECT)) > 0)
	{
		if(json_get_str_value(job, start, 0, "name", MAX_EVENT_SET_NAME_LEN, name, NULL) > 0)
		{
			bsles_delete(gWm, name);
		}
		else
		{
			writeMemo("[?] Missing event set name.");
		}
		return;
	}
	else if((start = json_get_typed_idx(job, 1, 0, "event", JSMN_OBJECT)) > 0)
	{
		if(json_get_str_value(job, start, 0, "es_ref", MAX_EVENT_SET_NAME_LEN, name, NULL) > 0)
		{
			BpSecEventId eventId = 0;
			if(getEventId(job, &eventId) > 0)
			{
				bslevt_delete(gWm, name, eventId);
			}
		}
		else
		{
			writeMemo("[?] Missing event set name.");
		}
		return;
	}
	else if((start = json_get_typed_idx(job, 1, 0, "policyrule", JSMN_OBJECT)) > 0)
	{
		uint16_t id = 0;
		if(getRuleId(job, &id) > 0)
		{
			bslpol_rule_remove_by_id(gWm, id);
		}
		else
		{
			writeMemo("[?] Missing rule id.");
		}
		return;
	}


	SYNTAX_ERROR;
}

static void	printBPsecBibRule(Object ruleAddr)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(BPsecBibRule, rule);
	char	srcEidBuf[SDRSTRING_BUFSZ], destEidBuf[SDRSTRING_BUFSZ];
	char	buf[512];

	GET_OBJ_POINTER(sdr, BPsecBibRule, rule, ruleAddr);
	sdr_string_read(sdr, srcEidBuf, rule->securitySrcEid);
	sdr_string_read(sdr, destEidBuf, rule->destEid);
	isprintf(buf, sizeof(buf), "rule src eid '%.255s' dest eid '%.255s' \
type '%d' ciphersuite '%.31s' key name '%.31s'", srcEidBuf, destEidBuf,
		rule->blockType, rule->profileName, rule->keyName);
	printText(buf);
}

static void     printBPsecBcbRule(Object ruleAddr)
{
        Sdr     sdr = getIonsdr();
                OBJ_POINTER(BPsecBcbRule, rule);
        char    srcEidBuf[SDRSTRING_BUFSZ], destEidBuf[SDRSTRING_BUFSZ];
        char    buf[512];

        GET_OBJ_POINTER(sdr, BPsecBcbRule, rule, ruleAddr);
        sdr_string_read(sdr, srcEidBuf, rule->securitySrcEid);
        sdr_string_read(sdr, destEidBuf, rule->destEid);
        isprintf(buf, sizeof(buf), "rule src eid '%.255s' dest eid '%.255s' \
type '%d' ciphersuite '%.31s' key name '%.31s'", srcEidBuf, destEidBuf,
		rule->blockType, rule->profileName, rule->keyName);
        printText(buf);
}

static void printEvent(BpSecEvent *event)
{
	int idx = 0;
	int parmIdx = 0;

	char buf[512];
	char tmp[128];
	memset(buf, '\0', sizeof buf);


	isprintf(tmp, sizeof(tmp), "Event: %s\nActions:\n", bslevt_get_name(event->id));
	strcat(buf,tmp);

	while(gActionMap[idx].key != NULL)
	{
		if(event->action_mask & gActionMap[idx].value)
		{
			switch(gActionMap[idx].value)
			{
				case BSLACT_REPORT_REASON_CODE:
					isprintf(tmp, sizeof(tmp), "- Report with reason code %i\n",
						     event->action_parms[parmIdx].asReason.reasonCode);
					strcat(buf,tmp);
					parmIdx++;
					break;
				case BSLACT_OVERRIDE_TARGET_BPCF:
				case BSLACT_OVERRIDE_SOP_BPCF:
					isprintf(tmp, sizeof(tmp), "- %s\n\t Mask: %ll \n\t New values:%ll \n",
							gActionMap[idx].key,
							event->action_parms[parmIdx].asOverride.mask,
							event->action_parms[parmIdx].asOverride.val);
					strcat(buf,tmp);
					parmIdx++;
					break;
				case BSL_ACT_NOT_IMPLEMENTED:
					break;
				default:
					isprintf(tmp, sizeof(tmp), "  %s\n", gActionMap[idx].key);
					strcat(buf, tmp);
			}
		}
		idx++;
	}

	printText(buf);
}

static void printEventsetName(BpSecEventSet *esPtr)
{
	char buf[MAX_EVENT_SET_NAME_LEN + 500]; //Max 255 named event sets
	memset(buf, '\0', sizeof(buf));

	isprintf(buf, sizeof(buf), "\nEventset name: %s\n Associated Policy Rules: %i\n",
			 esPtr->name, esPtr->ruleCount);

	printText(buf);
}

static void printEventset(BpSecEventSet *esPtr)
{
	PsmAddress elt;

	printEventsetName(esPtr);

	/* Print each event configured for the event set */
	for(elt = sm_list_first(gWm, esPtr->events); elt; elt = sm_list_next(gWm, elt))
	{
		BpSecEvent *event = (BpSecEvent *) psp(gWm, sm_list_data(gWm,elt));
		printEvent(event);
	}
}

static void printRule(BpSecPolRule *rulePtr, int verbose)
{
	char buf[2048];
	char tmp[512];
	BpSecEventSet *esPtr = NULL;

	if(rulePtr == NULL)
	{
		printText("No Rule.\n");
		return;
	}

	memset(buf, '\0', sizeof(buf));

	isprintf(tmp, sizeof(tmp), "\nRule #%u - ", rulePtr->user_id);
	strcat(buf,tmp);

	isprintf(tmp, sizeof(tmp), "%s", (strlen(rulePtr->desc) > 0) ? rulePtr->desc : "No Description");
	strcat(buf,tmp);

	if(verbose == 0)
	{
		printText(buf);
		return;
	}

	strcat(buf,"\n\tRoles: ");
    if(rulePtr->filter.flags & BPRF_SRC_ROLE)
    {
		isprintf(tmp, sizeof(tmp), "Source ", NULL);
		strcat(buf,tmp);
    }
    if(rulePtr->filter.flags & BPRF_VER_ROLE)
    {
		isprintf(tmp, sizeof(tmp), "Verifier ", NULL);
		strcat(buf,tmp);
    }
    if(rulePtr->filter.flags & BPRF_ACC_ROLE)
    {
		isprintf(tmp, sizeof(tmp), "Acceptor ", NULL);
		strcat(buf,tmp);
    }

	esPtr = (BpSecEventSet *) psp(gWm, rulePtr->eventSet);
	isprintf(tmp, sizeof(tmp), "\n\tEventset: %s", esPtr->name);
	strcat(buf,tmp);


	/* Print filter criteria for the rule */
	strcat(buf,"\n\tFilter:");

	if(rulePtr->filter.bsrc_len > 0)
	{
		isprintf(tmp, sizeof(tmp), " BSrc \"%s\"", (char *) psp(gWm, rulePtr->filter.bundle_src));
		strcat(buf,tmp);
	}

	if(rulePtr->filter.bdest_len > 0)
	{
		isprintf(tmp, sizeof(tmp), "BDest \"%s\"", (char *) psp(gWm, rulePtr->filter.bundle_dest));
		strcat(buf,tmp);
	}

	if(rulePtr->filter.ssrc_len > 0)
	{
		isprintf(tmp, sizeof(tmp), "Ssrc \"%s\"", (char *) psp(gWm, rulePtr->filter.sec_src));
		strcat(buf,tmp);
	}

	if(BPSEC_RULE_BTYP_IDX(rulePtr))
	{
		isprintf(tmp, sizeof(tmp), "Type %i", rulePtr->filter.blk_type);
		strcat(buf,tmp);
	}

	if(BPSEC_RULE_SCID_IDX(rulePtr))
	{
		isprintf(tmp, sizeof(tmp), "ScID %i", rulePtr->filter.scid);
		strcat(buf,tmp);
	}
	strcat(buf,"\n\n");

	printText(buf);
}

static void printRuleList(Lyst rules, int verbose)
{
	LystElt elt;

	if(rules == NULL)
	{
		printText("No Rules.\n");
		return;
	}

	for(elt = lyst_first(rules); elt; elt = lyst_next(elt))
	{
		BpSecPolRule *rulePtr = (BpSecPolRule *) lyst_data(elt);
		printRule(rulePtr, verbose);
	}
}


static int getFindCriteria(jsonObject job, int start, int *type, BpSecPolRuleSearchTag *tag)
{
	char tmp_str[32+1];
	int result = 0;

	CHKZERO(tag);

	if(json_get_str_value(job, 1, 0, "type", 32, tmp_str, NULL) <= 0)
	{
		printText("Search type missing.");
		writeMemo("[?] Search type missing.");
		return 0;
	}
	if(strcmp(tmp_str,"all") == 0)
	{
		*type = SEARCH_ALL;
	}
	else if(strcmp(tmp_str,"best") == 0)
	{
		*type = SEARCH_BEST;
	}
	else
	{
		printText("Unknown search type.");
		writeMemo("[?] Unknown search type.");
		return 0;
	}

	tag->bsrc = json_get_new_str_value(job, start, 0, "src", MAX_EID_LEN, NULL);
	tag->bsrc_len = (tag->bsrc) ? istrlen(tag->bsrc, MAX_EID_LEN) : 0;

	tag->bdest = json_get_new_str_value(job, start, 0, "dest", MAX_EID_LEN, NULL);
	tag->bdest_len = (tag->bdest) ? istrlen(tag->bdest, MAX_EID_LEN) : 0;

	tag->ssrc = json_get_new_str_value(job, start, 0, "ssrc", MAX_EID_LEN, NULL);
	tag->ssrc_len = (tag->ssrc) ? istrlen(tag->ssrc, MAX_EID_LEN) : 0;

	result = json_get_primitive_value(job, 1, 0, "tgt", 32, tmp_str, NULL);
	tag->type = (result > 0) ? atoi(tmp_str) : -1;

	// Role can be string or primitive.
	if((result = json_get_primitive_value(job, 1, 0, "role", 32, tmp_str, NULL)) <= 0)
	{
		result = json_get_str_value(job, 1, 0, "role", 32, tmp_str, NULL);
	}

	tag->role = (result > 0) ? getMappedValue(gRoleMap, tmp_str) : 0;

	result = json_get_primitive_value(job, 1, 0, "sc_id", 3, tmp_str, NULL);
	tag->scid = (result > 0) ? atoi(tmp_str) : 0;

	return 1;
}

static void	executeFindJson(jsonObject job)
{
	int start = 0;
	char name[MAX_EVENT_SET_NAME_LEN];
	memset(name, '\0', sizeof(name));

	if (job.tokenCount < 2)
	{
		printText("Delete what?");
		return;
	}

	if((start = json_get_typed_idx(job, 1, 0, "policyrule", JSMN_OBJECT)) > 0)
	{
		int type = 0;
		Lyst rules = NULL;
		BpSecPolRuleSearchTag tag;

		memset(&tag, 0, sizeof(tag));

		if(getFindCriteria(job, start, &type, &tag) <= 0)
		{
			return;
		}

		switch(type)
		{
			case SEARCH_ALL:
				rules = bslpol_rule_get_all_match(gWm, tag);
				printRuleList(rules, 1);
				lyst_destroy(rules);
				break;

			case SEARCH_BEST:
				printRule(bslpol_rule_get_best_match(gWm, tag), 1);
				break;
			default:
				printText("Unknown type.");
				break;
		}

		return;
	}

	SYNTAX_ERROR;

}

static void	executeInfo(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	Object		addr;
	Object		elt;

	if (tokenCount < 2)
	{
		printText("Information on what?");
		return;
	}

	if (tokenCount > 5)
	{
		SYNTAX_ERROR;
		return;
	}

	if (strcmp(tokens[1], "bibrule") == 0)
	{
		if (tokenCount != 5)
		{
			SYNTAX_ERROR;
			return;
		}
		CHKVOID(sdr_begin_xn(sdr));
		sec_findBPsecBibRule(tokens[2], tokens[3], atoi(tokens[4]),
				&addr, &elt);
		if (elt == 0)
		{
			printText("BIB rule not found.");
		}
		else
		{
			printBPsecBibRule(addr);
		}

		sdr_exit_xn(sdr);
		return;
	}

    if (strcmp(tokens[1], "bcbrule") == 0)
	{
    	if (tokenCount != 5)
		{
			SYNTAX_ERROR;
			return;
		}
    	CHKVOID(sdr_begin_xn(sdr));
		sec_findBPsecBcbRule(tokens[2], tokens[3], atoi(tokens[4]),
				&addr, &elt);
		if (elt == 0)
		{
			printText("BCB rule not found.");
		}
		else
		{
			printBPsecBcbRule(addr);
		}

		sdr_exit_xn(sdr);
        return;
     }
	SYNTAX_ERROR;
}

/******************************************************************************
 *
 * \par Function Name: executeInfoJson
 *
 * \par Purpose: This function executes the information command provided by
 *               the user if JSON is present.
 *
 * \param[in]  tokenCount  The number of parsed jsmn tokens.
 * \param[in]  tokens      jsmn token(s).
 * \param[in]  line        'Info' command using JSON syntax.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  12/30/20   S. Heiner      Initial Implementation
 *****************************************************************************/
static void	executeInfoJson(jsonObject job)
{
	int start = 0;
	char name[MAX_EVENT_SET_NAME_LEN+1];
	memset(name, '\0', sizeof(name));

	if (job.tokenCount < 2)
	{
		printText("Information on what?");
		return;
	}

	if((start = json_get_typed_idx(job, 1, 0, "event_set", JSMN_OBJECT)) > 0)
	{
		if(json_get_str_value(job, start, 0, "name", MAX_EVENT_SET_NAME_LEN, name, NULL) > 0)
		{
			BpSecEventSet *esPtr = bsles_get_ptr(gWm, name);
			if(esPtr)
			{
				printEventset(esPtr);
			}
			else
			{
				writeMemoNote("[?] Unknown event set:", name);
			}
		}
		else
		{
			writeMemo("[?] Missing event set name.");
		}
		return;
	}
	else if(json_get_primitive_value(job, 1, 0, "policyrule", MAX_EVENT_SET_NAME_LEN, name, NULL) > 0)
	{
		printRule(bslpol_rule_get_ptr(gWm, atoi(name)), 1);
		return;
	}


	SYNTAX_ERROR;
}

static void	executeList(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	OBJ_POINTER(SecDB, db);
	Object	elt;
	Object	obj;

	if (tokenCount < 2)
	{
		printText("List what?");
		return;
	}

	if (tokenCount != 2)
	{
		SYNTAX_ERROR;
		return;
	}

	GET_OBJ_POINTER(sdr, SecDB, db, getSecDbObject());
	if (strcmp(tokens[1], "bibrule") == 0)
	{
		CHKVOID(sdr_begin_xn(sdr));
		for (elt = sdr_list_first(sdr, db->bpsecBibRules); elt;
				elt = sdr_list_next(sdr, elt))
		{
			obj = sdr_list_data(sdr, elt);
			printBPsecBibRule(obj);
		}

		sdr_exit_xn(sdr);
		return;
	}

	if (strcmp(tokens[1], "bcbrule") == 0)
	{
		CHKVOID(sdr_begin_xn(sdr));
		for (elt = sdr_list_first(sdr, db->bpsecBcbRules); elt;
				elt = sdr_list_next(sdr, elt))
		{
			obj = sdr_list_data(sdr, elt);
			printBPsecBcbRule(obj);
		}

		sdr_exit_xn(sdr);
		return;
     }

	SYNTAX_ERROR;
}

/******************************************************************************
 *
 * \par Function Name: executeListJson
 *
 * \par Purpose: This function executes the list command provided by the user
 * 				 if JSON is present.
 *
 * \param[in]  tokenCount  The number of parsed jsmn tokens.
 * \param[in]  tokens      jsmn token(s).
 * \param[in]  line        'List' command using JSON syntax.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  12/30/20   S. Heiner      Initial Implementation
 *****************************************************************************/
static void	executeListJson(jsonObject job)
{
	char name[MAX_EVENT_SET_NAME_LEN];
	memset(name, '\0', sizeof(name));

	if (job.tokenCount < 2)
	{
		printText("List what?");
		return;
	}


	if(json_get_str_value(job, 1, 0, "type", MAX_EVENT_SET_NAME_LEN, name, NULL) < 0)
	{
		printText("Malformed Request.");
	}

	if(strcmp(name,"event_set") == 0)
	{
		Lyst eventsets = bsles_get_all(getIonwm());
		if (eventsets != NULL)
		{
			LystElt elt;

			/* For each element in the lyst (event set), print the eventset name */
			for(elt = lyst_first(eventsets); elt; elt = lyst_next(elt))
			{
				BpSecEventSet *esPtr = (BpSecEventSet *) lyst_data(elt);
				printEventsetName(esPtr);
			}
			lyst_destroy(eventsets);
		}

		return;
	}
	else if(strcmp(name,"policyrule") == 0)
	{
		PsmAddress elt = 0;

		/* Print each event configured for the event set */
		for(elt = sm_list_first(gWm, getSecVdb()->bpsecPolicyRules); elt; elt = sm_list_next(gWm, elt))
		{
			BpSecPolRule *rule = (BpSecPolRule *) psp(gWm, sm_list_data(gWm,elt));
			printRule(rule, 0);
		}
		return;
	}

	SYNTAX_ERROR;
}

static void	switchEcho(int tokenCount, char **tokens)
{
	int	state;

	if (tokenCount < 2)
	{
		printText("Echo on or off?");
		return;
	}

	switch (*(tokens[1]))
	{
	case '0':
		state = 0;
		break;

	case '1':
		state = 1;
		break;

	default:
		printText("Echo on or off?");
		return;
	}

	oK(_echo(&state));
}

/******************************************************************************
 *
 * \par Function Name: setOpenCounter
 *
 * \par Purpose: This is a helper function for parsing JSON commands. The
 * 				 function determines the number of open braces ('{') in
 * 				 the line of JSON passed in.
 *
 * \param[in]      line         The line of JSON to be parsed for brackets.
 * \param[in|out]  openCounter  The count of open brackets to be updated.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  12/30/20   S. Heiner      Initial Implementation
 *****************************************************************************/
void setCounter(char *line, char c, int *counter)
{
	char *cursor = strchr(line, c);

	while (cursor != NULL)
	{
		(*counter)++;
		cursor = strchr(cursor+1, c);
	}
}


/******************************************************************************
 *
 * \par Function Name: getJson
 *
 * \par Purpose: This function extracts a command entered by the user to the
 * 				 bpsecadmin utility which uses JSON.
 *
 * \retval int -1 Error
 * 				0 Command could not be retrieved.
 * 				1 JSON command successfully retrieved.
 *
 * \param[in]      line         The line of JSON from which a command is
 *                              extracted.
 * \param[in|out]  jsonStr      The concatenated json command.
 * \param[in]      cmdFile      File from which line was extracted.
 * \param[in]      len          Length of line.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  12/30/20   S. Heiner      Initial Implementation
 *****************************************************************************/
int getJson(char *line, char *jsonStr, int cmdFile, int len)
{
	int openCounter = 0;
	int closeCounter = 0;

	setCounter(line, '{', &openCounter);
	setCounter(line, '}', &closeCounter);

	strcpy(jsonStr, line);

	/* Keep gathering lines until we get a closing brace for JSON */
	while (openCounter != closeCounter)
	{

		if (igets(cmdFile, line, 1024, &len) == NULL)
		{
			if (len == 0)
			{
				break;	/*	Loop.	*/
			}

			putErrmsg("igets failed.", NULL);
			break;		/*	Loop.	*/
		}

		setCounter(line, '{', &openCounter);
		setCounter(line, '}', &closeCounter);

		/* Accumulate the JSON line contents */
		strcat(jsonStr, line);
	}

	if (openCounter == closeCounter)
	{
		/* End of JSON - successfully enclosed in braces */
		return 1;
	}

	/* Brace mismatch encountered */
	writeMemo("[?] Invalid JSON syntax detected");
	return 0;
}

/******************************************************************************
 *
 * \par Function Name: processJson
 *
 * \par Purpose: This function processes and executes a JSON command provided
 *               by the user to the bpsecadmin utility.
 *
 * \retval int -1 Error
 * 				0 Command could not be successfully executed
 * 				1 JCommand successfully executed.
 *
 * \param[in]    line       The JSON command to be executed.
 * \param[in]    cmd        (Optional) The single character command ('a',
 * 						    'c', etc.) corresponding to the action the command
 * 						    specifies in 'line'.
 * \param[in]    cmdPresent This value is set to true if the cmd field is
 * 							populated. The JSON command in 'line' does NOT
 * 							contain the command value if the cmdPresent
 * 							flag is set - instead, the value is found in the cmd
 * 							parameter.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  12/30/20   S. Heiner      Initial Implementation
 *****************************************************************************/
int processJson(char *line, char cmd, int cmdPresent)
{
	char 		*cursor;
	char		buffer[80];
	jsmn_parser p;
	jsonObject  job;
	char        action;

	jsmn_init(&p);

	job.line = line;
	job.tokenCount = jsmn_parse(&p, job.line, strlen(job.line), job.tokens,
				 sizeof(job.tokens) / sizeof(job.tokens[0]));

	if (job.tokenCount < 0 || job.tokenCount > MAX_JSMN_TOKENS)
	{
		writeMemo("[?] Failed to parse JSON");
		return -1;
	}

	/* Skip over any leading whitespace */
	cursor = line;
	while (isspace((int) *cursor))
	{
		cursor++;
	}

	action = (cmdPresent) ? cmd : cursor[0];

		/* Command code provided*/
		switch (action)
		{
			case 0:			/*	Empty line.		*/
			case '#':		/*	Comment.		*/
				return 0;

			case '?':
			case 'h':
				printUsage();
				return 0;
			case 'v':
				isprintf(buffer, sizeof(buffer), "%s", IONVERSIONNUMBER);
				printText(buffer);
				return 0;

			case 'a':
				if (secAttach() == 0)
				{
					executeAddJson(job);
				}
				return 0;

			case 'c':
				if (secAttach() == 0)
				{
					executeChangeJson(job);
				}
				return 0;

			case 'd':
				if (secAttach() == 0)
				{
					executeDeleteJson(job);
				}
				return 0;

			case 'f':
				if (secAttach() == 0)
				{
					executeFindJson(job);
				}
				return 0;

			case 'i':
				if (secAttach() == 0)
				{
					executeInfoJson(job);
				}
				return 0;

			case 'l':
				if (secAttach() == 0)
				{
					executeListJson(job);
				}
				return 0;
		}


	return 1;
}

int	processLine(char *line, int lineLength, char *jsonStr, int cmdFile)
{
	int			tokenCount;
	char		*cursor;
	int			i;
	char		*tokens[9];
	char		buffer[80];
	BpBlockType	blockType;

	tokenCount = 0;
	for (cursor = line, i = 0; i < 9; i++)
	{
		if (*cursor == '\0')
		{
			tokens[i] = NULL;
		}
		else
		{
			findToken(&cursor, &(tokens[i]));
			if (tokens[i])
			{
				tokenCount++;
			}
		}
	}

	if (tokenCount == 0)
	{
		return 0;
	}

	/*	Skip over any trailing whitespace.			*/
	while (isspace((int) *cursor))
	{
		cursor++;
	}

	/*	Make sure we've parsed everything.			*/
	if (*cursor != '\0')
	{
		printText("Too many tokens.");
		return 0;
	}

	/*	Have parsed the command.  Now execute it.		*/
	if (tokenCount == 1 && ((*(tokens[0]) == 'a') || (*(tokens[0]) == 'c') ||
				(*(tokens[0]) == 'd') || (*(tokens[0]) == 'i') ||
				(*(tokens[0]) == 'l')))
	{
		int len;
		char cmd = *(tokens[0]);
		igets(cmdFile, line, 1024, &len);
		getJson(line, jsonStr, cmdFile, len);
		processJson(jsonStr, cmd, 1);
		return 0;
	}

	switch (*(tokens[0]))		/*	Command code.		*/
	{
		case 0:			/*	Empty line.		*/
		case '#':		/*	Comment.		*/
			return 0;

		case '?':
		case 'h':
			printUsage();
			return 0;

		case 'v':
			isprintf(buffer, sizeof(buffer), "%s", IONVERSIONNUMBER);
			printText(buffer);
			return 0;

		case 'a':
			if (secAttach() == 0)
			{
				executeAdd(tokenCount, tokens);
			}

			return 0;

		case 'c':
			if (secAttach() == 0)
			{
				executeChange(tokenCount, tokens);
			}

			return 0;

		case 'd':
			if (secAttach() == 0)
			{
				executeDelete(tokenCount, tokens);
			}

			return 0;

		case 'i':
			if (secAttach() == 0)
			{
				executeInfo(tokenCount, tokens);
			}

			return 0;

		case 'l':
			if (secAttach() == 0)
			{
				executeList(tokenCount, tokens);
			}

			return 0;

		case 'x':
			if (secAttach() == 0)
			{
			   	if (tokenCount > 4)
				{
					SYNTAX_ERROR;
				}
				else if (tokenCount == 4)
				{
					blockType = (BpBlockType)
					atoi(tokens[3]);
					sec_clearBPsecRules(tokens[1],
							tokens[2], &blockType);
				}
				else if (tokenCount == 3)
				{
					sec_clearBPsecRules(tokens[1],
							tokens[2], NULL);
				}
				else if (tokenCount == 2)
				{
					sec_clearBPsecRules(tokens[1], "~",
							NULL);
				}
				else
				{
					sec_clearBPsecRules("~", "~", NULL);
				}
			}

			return 0;

		case 'e':
			switchEcho(tokenCount, tokens);
			return 0;
	
		case 'q':
			return -1;	/*	End program.		*/

		default:
			printText("Invalid command.  Enter '?' for help.");
			return 0;
	}
}

#if defined (ION_LWT)
int	bpsecadmin(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*cmdFileName = (char *) a1;
#else
int	main(int argc, char **argv)
{
	char	*cmdFileName = (argc > 1 ? argv[1] : NULL);
#endif
	int		cmdFile;
	int		len;
	char	line[1024];
	char 	jsonStr[2048];

	if (cmdFileName == NULL)		/*	Interactive.	*/
	{
#ifdef FSWLOGGER
		return 0;			/*	No stdout.	*/
#else
		cmdFile = fileno(stdin);
		isignal(SIGINT, handleQuit);
		init();
		while (1)
		{
			printf(": ");
			fflush(stdout);
			if (igets(cmdFile, line, sizeof(line), &len) == NULL)
			{
				if (len == 0)
				{
					break;
				}

				putErrmsg("igets failed.", NULL);
				break;		/*	Out of loop.	*/
			}

			if (len == 0)
			{
				continue;
			}

			if (strchr(line, '{') != NULL) /* JSON */
			{
				char cmd = '\0';
				getJson(line, jsonStr, cmdFile, len);
				processJson(jsonStr, cmd, 0);
			}

			else if (processLine(line, len, jsonStr, cmdFile))
			{
				break;		/*	Out of loop.	*/
			}
		}
#endif
	}
	else					/*	Scripted.	*/
	{
		cmdFile = iopen(cmdFileName, O_RDONLY, 0777);
		if (cmdFile < 0)
		{
			PERROR("Can't open command file");
		}
		else
		{
			while (1)
			{
				if (igets(cmdFile, line, sizeof(line), &len)
						== NULL)
				{
					if (len == 0)
					{
						break;	/*	Loop.	*/
					}

					putErrmsg("igets failed.", NULL);
					break;		/*	Loop.	*/
				}

				if (len == 0
				|| line[0] == '#')	/*	Comment.*/
				{
					continue;
				}

				if (strchr(line, '{') != NULL) /* JSON */
				{
					char cmd = '\0';
					getJson(line, jsonStr, cmdFile, len);
					processJson(jsonStr, cmd, 0);
				}

				else if (processLine(line, len, jsonStr, cmdFile))
				{
					break;	/*	Out of loop.	*/
				}
			}

			close(cmdFile);
		}
	}

	writeErrmsgMemos();
	printText("Stopping bpsecadmin.");
	ionDetach();
	return 0;
}
