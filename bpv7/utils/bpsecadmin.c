/*
	bpsecadmin.c:	security database administration interface.


	Copyright (c) 2019, California Institute of Technology.	
	All rights reserved.
	Author: Scott Burleigh, Jet Propulsion Laboratory
	Modifications: TCSASSEMBLER, TopCoder

TODO: Update JSON parser to use hierarchical parsers per JSON object.
TODO: Implement support for anonymous event sets in policyrules.

	Modification History:
	Date       Who        What
	9-24-13    TC          Added atouc helper function to convert char* to
			               unsigned char
	6-27-19	   SB	       Extracted from ionsecadmin.
	1-25-21    S. Heiner,  Added initial BPSec Policy Rule Commands and
	           E. Birrane  JSON parsing


*/


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

#define USER_TEXT_LEN   (1024)
#define SEC_ROLE_LEN    (15)
#define NUM_STR_LEN     (5)
#define GEN_PARM_LEN    (32)

#define BPSEC_SEARCH_ALL 1
#define BPSEC_SEARCH_BEST 2

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
	{"request_storage",        BSLACT_NOT_IMPLEMENTED}, //BSLACT_REQUEST_STORAGE},
	{"report_reason_code",     BSLACT_REPORT_REASON_CODE},
	{"override_target_bpcf",   BSLACT_NOT_IMPLEMENTED}, //BSLACT_OVERRIDE_TARGET_BPCF},
	{"override_sop_bpcf",      BSLACT_NOT_IMPLEMENTED}, //BSLACT_OVERRIDE_SOP_BPCF},
	{NULL,0}
};

BpSecMap gScParmMap[] = {
	{"key_name", CSI_PARM_KEYINFO},
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
char gUserText[USER_TEXT_LEN];

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

	PUTS("\n\ta\tAdd");
	PUTS("\t\tEvery eid expression must be a node identification \
expression, i.e., a partial eid expression ending in '*' or '~'.");
	PUTS("\t   a bibrule <source eid expression> <destination eid \
expression> <block type number> { '' | <ciphersuite name> <key name> }");
	PUTS("\t   a bcbrule <source eid expression> <destination eid \
expression> <block type number> { '' | <ciphersuite name> <key name> }");

	PUTS("\n\tc\tChange");
	PUTS("\t   c bibrule <source eid expression> <destination eid \
expression> <block type number> { '' | <ciphersuite name> <key name> }");
	PUTS("\t   c bcbrule <source eid expression> <destination eid \
expression> <block type number> { '' | <ciphersuite name> <key name> }");

	PUTS("\n\td\tDelete");
	PUTS("\t   d bibrule <source eid expression> <destination eid \
	expression> <block type number>");
	PUTS("\t   d bcbrule <source eid expression> <destination eid \
	expression> <block type number>");

	PUTS("\n\ti\tInfo");
	PUTS("\t   i bibrule <source eid expression> <destination eid \
expression> <block type number>");
	PUTS("\t   i bcbrule <source eid expression> <destination eid \
expression> <block type number>");

	PUTS("\n\tl\tList");
	PUTS("\t   l bibrule");
	PUTS("\t   l bcbrule");


	PUTS("\tx\tClear BSP security rules.");
	PUTS("\t   x <security source eid> <security destination eid> \
{ 2 | 3 | 4 | ~ }");
	PUTS("\te\tEnable or disable echo of printed output to log file");
	PUTS("\t   e { 0 | 1 }");
	PUTS("\t#\tComment");
	PUTS("\t   # <comment text>");


    PUTS("\n\n\tJSON Commands (Experimental)");
    PUTS("\t--------------------------------");
    PUTS("\tJSON keys wrapped in ?'s are optional in a command.");
    PUTS("\tIf included, the key should be represented without the ?'s.");
    PUTS("\n\t   ADD");
    PUTS("\t   a { \"event\" :");
    PUTS("\t       {");
    PUTS("\t          \"es_ref\" : \"<event set name>\",");
    PUTS("\t          \"event_id\" : \"<event name>\",");
    PUTS("\t          \"actions\" : [{\"id\":\"<action>\", <parms if applicable>},...]");
    PUTS("\t       }");
    PUTS("\t     }");
    PUTS("\t   a { \"event_set\" : {\"?desc?\" : \"<desc>\", \"name\" : \"<event set name>\"}}");
    PUTS("\t   a { \"policyrule\" :");
    PUTS("\t       {");
    PUTS("\t          \"?desc?\" : \"<description>\",");
    PUTS("\t          \"es_ref\" : \"<event set name>\",");
    PUTS("\t          \"filter\" : ");
    PUTS("\t          {");
    PUTS("\t             \"?rule_id?\" : \"<rule id>\",");
    PUTS("\t             \"role\"      : \"<security role>\", ");
    PUTS("\t             \"src\"       : \"<source eid expression>\",        \\");
    PUTS("\t             \"dest\"      : \"<destination eid expression>\",    ) (1 of src/dest/ssrc required)");
    PUTS("\t             \"ssrc\"      : \"<security source eid expression>\"/");
    PUTS("\t          },");
    PUTS("\t          \"spec\" :");
    PUTS("\t          {");
    PUTS("\t             \"svc\"      : \"<security service>\"");
    PUTS("\t             \"sc_parms\" : [{\"id\":\"<parm1_id>\",\"value\":\"<parm1_value\"},...]");
    PUTS("\t          }");
    PUTS("\t       }");
    PUTS("\t     }");

    PUTS("\n\t   DELETE");
    PUTS("\t   d { \"event\" : {\"es_ref\" : \"<event set name>\", \"event_id\" : \"<event name>\"}}");
    PUTS("\t   d { \"event_set\" : {\"name\" : \"<event set name>\"}}");
    PUTS("\t   d { \"policyrule\" : {\"rule_id\" : \"<rule id>\"}}");

    PUTS("\n\t   FIND");
    PUTS("\t   f { \"policyrule\" : ");
    PUTS("\t       {");
    PUTS("\t          \"type\" : \"all\" | \"best\",");
    PUTS("\t          \"src\"  : \"<source eid expression>\",         \\");
    PUTS("\t          \"dest\" : \"<destination eid expression>\",     ) (1 of src/dest/ssrc required)");
    PUTS("\t          \"ssrc\" : \"<security source eid expression>\"./");
    PUTS("\t          \"?scid?\" : <security context id>,");
    PUTS("\t          \"?role?\" : \"<security role>\"");
    PUTS("\t       }");
    PUTS("\t     }");

    PUTS("\n\t   INFO");
    PUTS("\t   i { \"event_set\" : {\"name\" : \"<event set name>\"}}");
    PUTS("\t   i { \"policyrule\" : <rule id>");

    PUTS("\n\t   LIST");
    PUTS("\t   l {\"type\" : \"eventset\" | \"policyrule\"}");


}

static int attach(int state)
{

    if (secAttach() != 0)
    {
        printText("Failed to attach to security database.");
        return 0;
    }

    SecVdb *vdb = getSecVdb();
    if (vdb == NULL)
    {
        printText("Failed to retrieve security database.");
        return 0;
    }


    if((gWm = getIonwm()) == NULL)
    {
        printText("ION is not running.");
        return 0;
    }

    if(state == 1)
    {
        if (bsl_all_init(gWm) < 1)
        {
            printText("Failed to initialize BPSec policy");
            return 0;
        }
    }

    return 1;
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
static int init()
{

    /* This will call ionAttach */
	if (secInitialize() < 0)
	{
		printText("Can't initialize the ION security system.");
		return 0;
	}

	return attach(1);
}


/******************************************************************************
 * @brief Find the index of a named value from a set of JSON tokens.
 *
 * @param[in]  job   - Parsed JSON tokens.
 * @param[in]  start - Start of the token range to search for the value index.
 * @param[in]  end   - End of the token range to search for the value index.
 * @param[in]  key   - The name of the value to search for.
 * @param[in]  type  - The expected type of the value
 *
 * @note
 * An "end" token range of 0 means to search all tokens.
 * \par
 * The expected type of the value (JSMN_PRIMITIVE, JSM_STRING, etc...) The
 * JSMN_PRIMITIVE type is used for for integers and characters.
 * \par
 * The index of the found value is always 1 + the index of its key. Since the
 * end parameter is the last token to search for the key, the idx value can
 * be end + 1.
 * \par
 * The found value is copied out of the JSON token and into the supplied value.
 *
 * @retval >0 - The index of the found value.
 * @retval 0 - The value was not found
 *****************************************************************************/
static int jsonGetTypedIdx(jsonObject job, int start, int end, char *key, int type)
{
	int i = 0;
	int key_len = 0;
	int token_len = 0;

	/* We need a key to search for. */
	if(key == NULL)
	{
		return -1;
	}

	key_len = strlen(key);

	/* Calculate end of token search. */
	if(end <= 0)
	{
		end = job.tokenCount - 1;
	}

	for (i = start; i <= end; i++)
	{
		/* If the current token matches the given key... */
		token_len = job.tokens[i].end - job.tokens[i].start;

		/* If we found the key... */
		if( (job.tokens[i].type == JSMN_STRING) &&
			(key_len == token_len) &&
			(strncmp(job.line + job.tokens[i].start, key, token_len) == 0))
		{
			/* The index after the key must be the value. */
			i++;

			/* The index is a match if the value is the expected type. */
			return (job.tokens[i].type == type) ? i : 0;
		}
	}

	return 0;
}



/******************************************************************************
 * @brief Extracts a named value of a given type from a set of JSON tokens.
 *
 * @param[in]  job   - The parsed JSON tokens.
 * @param[in]  start - The start of the token range to search for the key.
 * @param[in]  end   - The end of the token range to search for the key.
 * @param[in]  type  - The type of the value
 * @param[in]  key   - The name of the value to search for.
 * @param[in]  max   - The length of the value field holding the found result.
 * @param[out] value - The user-supplied field to hold the found result.
 * @param[out] idx   - The optional token index of the value.
 *
 * @note
 * An "end" token range of 0 means to search all tokens.
 * \par
 * The expected type of the value (JSMN_PRIMITIVE, JSM_STRING, etc...) The
 * JSMN_PRIMITIVE type is used for for integers and characters.
 * \par
 * The index of the found value is always 1 + the index of its key. Since the
 * end parameter is the last token to search for the key, the idx value can
 * be end + 1.
 * \par
 * The found value is copied out of the JSON token and into the supplied value.
 *
 * @retval 1 - The value was found in the JSON token range.
 * @retval 0 - The value was not found
 *****************************************************************************/

static int jsonGetTypedValue(jsonObject job, int start, int end, int type, char *key, int max, char *value, int *idx)
{
	int result = 0;
	int i = 0;

	/* Retrieve the index of the token holding the value. */
	i = jsonGetTypedIdx(job, start, end, key, type);

	if(i > 0)
	{
		/* Calculate the length of the found value. */
		int len = MIN(job.tokens[i].end - job.tokens[i].start, max);

		/* Clear the value field. */
		memset(value,0,max);

		/* Copy JSON segment into the value. */
		if(istrcpy(value, job.line+job.tokens[i].start, len+1) == NULL)
		{
#if 0
			result = -1;	/*	0 later overwrites -1.	*/
#endif
			return -1;
		}

		/* Store value token index if needed. */
		if(idx != NULL)
		{
			*idx = i;
		}

		/* Note success. */
		result = 1;
	}

	return result;
}



/******************************************************************************
 * @brief Returns deep copy of a value from a JSON string.
 *
 * @param[in]  job   - The parsed JSON tokens.
 * @param[in]  start - The start of the token range to search for the key.
 * @param[in]  end   - The end of the token range to search for the key.
 * @param[in]  type  - The type of the value
 * @param[in]  key   - The name of the value to search for.
 * @param[in]  max   - The length of the value field holding the found result.
 * @param[out] idx   - The optional token index of the value.
 *
 * @note
 * The returned char * MUST be released by the caller.
 *
 * @retval !NULL - The value that was found in the JSON token range.
 * @retval NULL  - The value was not found
 *****************************************************************************/

static char *jsonAllocStrValue(jsonObject job, int start, int end, char *key, int max, int *idx)
{
	char *tmp = (char*) MTAKE(max+1);

	if(jsonGetTypedValue(job, start, end, JSMN_STRING, key, max, tmp, idx) <= 0)
	{
		MRELEASE(tmp);
		tmp = NULL;
	}

	return tmp;
}


/******************************************************************************
 * @brief Returns a value from a key-map memory set
 *
 * @param[in]  map   - The specific map being queried
 * @param[in]  key   - The key being queried
 *
 * @note
 * This function serves as an associative array lookup for common mappings of
 * JSON string values to enumerated values.
 * \par
 * Keys in this array are static strings.  Values are integers.
 *
 * @retval !0 - The value
 * @retval 0  - The value was not found.
 *****************************************************************************/
static int getMappedValue(BpSecMap map[], char *key)
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
 * @brief Retrieves the event_id from a JSON string.
 *
 * @param[in]  job   - The parsed JSON tokens.
 * @param[out] event - The retrieved event enumeration
 *
 * @note
 * The expected JSON pair is "event_id" : "<event name>"
 *
 * @retval 1  - The event was found and validated.
 * @retval 0  - The event was not found or was invalid.
 * @retval -1 - Error extracting the event.
 *****************************************************************************/

static int getEventId(jsonObject job, BpSecEventId *event)
{
	char eventName[MAX_EVENT_LEN+1];
	int result = 1;

	CHKERR(event);

	if(jsonGetTypedValue(job, 1, 0, JSMN_STRING, "event_id", MAX_EVENT_LEN, eventName, NULL) > 0)
	{
		/* Check that provided event is supported */
		*event = bslevt_get_id(eventName);

		if(*event == unsupported)
		{
			isprintf(gUserText, USER_TEXT_LEN, "[?] event %s is not supported.", eventName);
			printText(gUserText);

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
 * @brief Sets an action bit in a bitmask associated with a named action
 *
 * @param[in]  action     - The name of the action being added to the mask
 * @param[out] actionMask - The updated action mask.
 *
 * @note
 * An action must be both defined AND map to a supported, implemented enumeration
 *
 * @retval !0 - The action enumeration OR'd into the mask
 * @retval 0  - The action was not added to the mask
 *****************************************************************************/

static int setAction(char *action, uint8_t *actionMask)
{
  int value = 0;

  /* Find the action value in the map of action values. */
  if((value = getMappedValue(gActionMap, action)) > 0)
  {
	  /* If the action was found but is not implemented...*/
	  if(value == BSLACT_NOT_IMPLEMENTED)
	  {
		isprintf(gUserText, USER_TEXT_LEN, "[i] Action %s currently not supported", action);
		printText(gUserText);

		value = 0;
	  }
	  else
	  {
		  *actionMask |= value;
	  }
  }
  else
  {
	  isprintf(gUserText, USER_TEXT_LEN, "[?] Unknown action %s", action);
	  printText(gUserText);
  }

  return value;
}



/******************************************************************************
 * @brief Retrieves a set of actions and parameters from a JSON object.
 *
 * @param[in]  job        - The parsed JSON tokens.
 * @param[out] actionMask - The mask of enabled actions
 * @param[out] parms      - Parameters associated with actions (if any)
 *
 * JSON structure
 * "actions" : { [ {"id":"action name", (opt parms)}, ...]}
 *
 * @note
 * The parms field is expected to have been allocated by the calling function.
 *
 * @retval  1 - Action(s) successfully processed - action(s) valid.
 * @retval  0 - Action(s) unsuccessfully processed - action(s) invalid.
 * @retval -1 - Error.
 *****************************************************************************/

static int getActions(jsonObject job, uint8_t *actionMask, BpSecEvtActionParms *parms)
{
	int start = 0;
	char actionStr[MAX_ACTION_LEN];
	int parmIdx = 0;
	char parmStr[64+1];
	int numParm = 0;
	int curAct = 0;

	/*
	 * Get the index of the start of the actions array. All key-value searches
	 * will be from this token onward.
	 */
	start = jsonGetTypedIdx(job, 1, 0, "actions", JSMN_ARRAY);

	if(start <= 0)
	{
		isprintf(gUserText, USER_TEXT_LEN, "[?] Cannot find actions array object", NULL);
		printText(gUserText);

		return -1;
	}

	/* Walk through tokens, looking for actions to add. */
	while (start < job.tokenCount)
	{
		/*
		 * All valid actions start with "id". If there are no more valid actions, we are done.
		 */
		if(jsonGetTypedValue(job, start, 0, JSMN_STRING, "id", MAX_ACTION_LEN, actionStr, &start) <= 0)
		{
			break;
		}

		/* Convert action name to an enumeration and update the action mask. */
		curAct = setAction(actionStr, actionMask);

		/* Process the action based on its enumeration. */
		switch(curAct)
		{
			/* If this is a report action, reas in the reason code to report. */
			case BSLACT_REPORT_REASON_CODE:
				if(jsonGetTypedValue(job, start, start+1, JSMN_PRIMITIVE, "reason_code", 64, parmStr, &start) > 0)
				{
				   parms[parmIdx++].asReason.reasonCode = atoi(parmStr);
				}
				else
				{
					isprintf(gUserText, USER_TEXT_LEN, "[x] No reason code supplied for action %d", curAct);
					printText(gUserText);

					return -1;
				}
				break;

			/* If this is an override action, get the mask/value for the override. */
			case BSLACT_OVERRIDE_TARGET_BPCF:
			case BSLACT_OVERRIDE_SOP_BPCF:
				numParm = 0;
				if(jsonGetTypedValue(job, start, start+3, JSMN_PRIMITIVE, "mask", 64, parmStr, NULL) > 0)
				{
				   parms[parmIdx].asOverride.mask = (uint64_t) strtol(parmStr, NULL, 16);
				   numParm++;
				}
				if(jsonGetTypedValue(job, start, start+3, JSMN_PRIMITIVE, "new_value", 64, parmStr, NULL) > 0)
				{
				   parms[parmIdx].asOverride.val = (uint64_t) strtol(parmStr, NULL, 16);
				   numParm++;
				}
				if(numParm != 2)
				{
					isprintf(gUserText, USER_TEXT_LEN, "[x] Incorrect parms for override action", NULL);
					printText(gUserText);

					parms[parmIdx].asOverride.val = parms[parmIdx].asOverride.mask = 0;
					return -1;
				}
				else
				{
					parmIdx++;
				}
				break;
			/* if the action is unknown, stop processing. The JSON is malformed. */
			case 0:
				isprintf(gUserText, USER_TEXT_LEN, "[?] Unknown action %s", actionStr);
				printText(gUserText);

				return 0;
				break;

			/* If the action is not implemented, skip and process other actions. */
			case BSLACT_NOT_IMPLEMENTED:
				isprintf(gUserText, USER_TEXT_LEN, "[?] Action %s not implemented", actionStr);
				printText(gUserText);
				break;
			default:
				break;
		}
	}
	return 1;
}



/******************************************************************************
 * @brief Retrieves the criteria associated with a policyrule filter.
 *
 * @param[in]   job   - The parsed JSON tokens.
 * @param[out]  bsrc  - Bundle source.
 * @param[out]  bdest - Bundle destination.
 * @param[out]  ssrc  - Security source.
 * @param[out]  type  - Security target block type.
 * @param[out]  role  - Security role.
 * @param[out]  sc_id - Security context ID.
 *
 * @note
 * All out parms are expected to have been pre-allocated by the calling function.
 * \par
 * All EID values are expected to be of MAX_EID_LEN length and memset to 0.
 * \par
 * role and sc_id remain 0 if their are not set in the JSON.
 * \par
 * type is set to -1 if its value is not set in the JSON
 *
 * @retval  1 - Filter criteria parsed successfully - parameters populated
 * @retval  0 - Filter criteria parsed unsuccessfully
 * @retval -1 - Error.
 *****************************************************************************/

static int getFilterCriteria(jsonObject job, char *bsrc, char *bdest, char *ssrc, int *type, int *role, int *sc_id)
{
	int result = 0;

	char num_str[NUM_STR_LEN];
	char role_str[SEC_ROLE_LEN];

	*role = 0;
	*type = -1;
	*sc_id = 0;

	if(jsonGetTypedValue(job, 1, 0, JSMN_STRING, "src", MAX_EID_LEN, bsrc, NULL) < 0)
	{
		printText("[?] Malformed bundle source provided");
		return 0;
	}

	if(jsonGetTypedValue(job, 1, 0, JSMN_STRING, "dest", MAX_EID_LEN, bdest, NULL) < 0)
	{
		printText("[?] Malformed bundle destination provided");
		return 0;
	}

	if(jsonGetTypedValue(job, 1, 0, JSMN_STRING, "sec_src", MAX_EID_LEN, ssrc, NULL) < 0)
	{
		printText("[?] Malformed security source provided");
		return 0;
	}

	if((result = jsonGetTypedValue(job, 1, 0, JSMN_PRIMITIVE, "tgt", NUM_STR_LEN, num_str, NULL)) < 0)
	{
		printText("[?] Malformed target block type provided");
		return 0;
	}
	else if (result > 0)
	{
		*type = atoi(num_str);
	}

	/*
	 * Role can be either a primitive character "v" or a string "verifier". Check for
	 * primitive first and if not found, try and read it as a string.
	 */
	if((result = jsonGetTypedValue(job, 1, 0, JSMN_PRIMITIVE, "role", SEC_ROLE_LEN, role_str, NULL)) <= 0)
	{
		result = jsonGetTypedValue(job, 1, 0, JSMN_STRING, "role", SEC_ROLE_LEN, role_str, NULL);
	}

	/* A result of 0 means the role was not found in the JSON, which is OK. */
	if(result < 0)
	{
		printText("[?] Malformed security role provided");
		return 0;
	}
	else if (result > 0)
	{
		*role = getMappedValue(gRoleMap, role_str);
	}

	/* A result of 0 means the sc_id was not found in the JSON, which is OK. */
	if((result = jsonGetTypedValue(job, 1, 0, JSMN_PRIMITIVE, "sc_id", NUM_STR_LEN, num_str, NULL)) < 0)
	{
		printText("[?] Malformed security context identifier provided");
		return 0;
	}
	else if (result > 0)
	{
		*sc_id = atoi(num_str);
	}


	if((*role == BPRF_SRC_ROLE) && (*sc_id == 0))
	{
		printText("[x] Security sources MUST specify a security context identifer.");
		return 0;
	}

	return 1;
}



/******************************************************************************
 * @brief Populates a policyrule filter object from a JSON string.
 *
 * @param[in]   job    - The parsed JSON tokens.
 * @param[out]  filter - The filter to populate.
 *
 * @note
 * A filter MUST contain at least one EID to be considered valid.
 * \par
 * The filter is expected to have been allocated by the calling function
 *
 * @retval  1 - Filter contents valid - BpSecFilter populated
 * @retval  0 - Filter contents invalid
 * @retval -1 - Error.
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
			printText("[?] Filter information invalid");
			return 0;
		}
	}

	printText("[?] Malformed filter criteria");
	return 0;
}




/******************************************************************************
 * @brief Retrieve a set of security context parameters from a JSON object
 *
 * @param[in]   job    - The parsed JSON tokens.
 *
 * @note
 * Currently, this is a lyst of sci_inbound_tlv structures.
 * \par
 * The created Lyst has a delete callback to help with lyst cleanup later
 * \par
 * TODO: Read parameters into a generic structure
 *
 * @retval  !NULL - Lyst of extracted security parameters
 * @retval  NULL  - Error extracting security parameters
 *****************************************************************************/

static PsmAddress getSecCtxtParms(jsonObject job)
{
	int i = 0;
	int start = 0;
	char curId[GEN_PARM_LEN];
	char curVal[GEN_PARM_LEN];
	PsmAddress result = 0;
	PsmAddress newParm = 0;
	PsmPartition partition = getIonwm();

	/* It is not an error to not have security context parms. */
	if((start = jsonGetTypedIdx(job, 1, 0, "sc_parms", JSMN_ARRAY)) <= 0)
	{
		return 0;
	}

	/* Create a shared memory list to hold the parms we did find. */
	result = sm_list_create(partition);
	CHKZERO(result);

	/* Start processing parms at the start of the sc_parms JSON object. */
	i = start;
	while(i > 0)
	{
		/*
		 * Get the next parm id. If we can't we will assume there are no more
		 * parms to be had.
		 */
		if(jsonGetTypedValue(job, i, 0, JSMN_STRING, "id", GEN_PARM_LEN, curId, &i) <= 0)
		{
			i = 0;
		}
		else
		{
			/* Init the parm Id */
			int type = getMappedValue(gScParmMap, curId);

			/* Read the parm value into a tmp variable. */
			if(jsonGetTypedValue(job, i, 0, JSMN_STRING, "value", GEN_PARM_LEN, curVal, &i) < 0)
			{
				isprintf(gUserText, USER_TEXT_LEN, "[x] Cannot parse sc_parm %s", curId);
				printText(gUserText);

				bslpol_scparms_destroy(partition, result);
				return 0;
			}

			if((newParm = bslpol_scparm_create(partition, type, strlen(curVal), curVal)) <= 0)
			{
                isprintf(gUserText, USER_TEXT_LEN, "[x] Cannot create parameter %s", curVal);
                printText(gUserText);

                bslpol_scparms_destroy(partition, result);
                return 0;
			}

			sm_list_insert_last(partition, result, newParm);
		}
	}

	return result;
}



/******************************************************************************
 * @brief Extract a policyrule ID from a JSON object
 *
 * @param[in]  job    - The parsed JSON tokens.
 * @param[out] ruleId - The extracted rule ID.
 *
 * @note
 * It is assumed that the caller allocated the ruleId being populated
 *
 * @retval -1 - Error
 * @retval  0 - Rule ID invalid
 * @retval  1 - Rule ID valid
 *****************************************************************************/

static int getRuleId(jsonObject job, uint16_t *ruleId)
{
	char id[RULE_ID_LEN];
	memset(id, '\0', sizeof(id));

	*ruleId = 0;

	if(jsonGetTypedValue(job, 1, 0, JSMN_STRING, "rule_id", RULE_ID_LEN, id, NULL) <= 0)
	{
		if(jsonGetTypedValue(job, 1, 0, JSMN_PRIMITIVE, "rule_id", RULE_ID_LEN, id, NULL) <= 0)
		{
		  return 0;
		}
	}

	*ruleId= atoi(id);
	return 1;
}



/******************************************************************************
 * @brief Generate an identifier for a policyrule
 *
 * @param[in]  job    - The parsed JSON tokens.
 * @param[out] ruleId - The extracted rule ID.
 *
 * @note
 * If present in the JSON tokens, the rule_id will be taken from there, if
 * the rule_id is not already defined in the system. Otherwise, a new
 * rule_id will be generated.
 *
 * @retval -1 - Error
 * @retval  0 - Rule ID invalid
 * @retval  1 - Rule ID valid
 *****************************************************************************/

static int getNewRuleId(jsonObject job, uint16_t *ruleId)
{
	CHKZERO(ruleId);

	/* Extract the ruleID from the JSON tokens. */
	getRuleId(job, ruleId);

	/*
	 * If the rule was not in the JSON objects then try and find the first
	 * available rule identifier.
	 */
	if(*ruleId <= 0)
	{
		int i = 0;
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

		isprintf(gUserText, USER_TEXT_LEN, "[x] No available rule IDs. Max # of %s reached.", MAX_RULE_ID-1);
		printText(gUserText);

		return 0;
	}

	/* Otherwise, check if user rule ID is already in use */
	else if (bslpol_rule_get_ptr(gWm, *ruleId) != NULL)
	{
		isprintf(gUserText, USER_TEXT_LEN, "[x] Rule %d already defined.", *ruleId);
		printText(gUserText);

		return 0;
	}

	return 1;
}



#if 0
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

#endif




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
 * @brief Process the adding of BPSec policy objects provided a JSON object
 *
 * @param[in]  job    - The parsed JSON tokens.
 *
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

	if((start = jsonGetTypedIdx(job, 1, 0, "event_set", JSMN_OBJECT)) > 0)
	{
		if(jsonGetTypedValue(job, start, 0, JSMN_STRING, "name", MAX_EVENT_SET_NAME_LEN, name, NULL))
		{
			if(bsles_add(gWm, name) < 0)
			{
				isprintf(gUserText, USER_TEXT_LEN, "[x] Error adding eventset %s ", name);
				printText(gUserText);
			}
		}
	}
	else if((start = jsonGetTypedIdx(job, 1, 0, "event", JSMN_OBJECT)) > 0)
	{
		if(jsonGetTypedValue(job, start, 0, JSMN_STRING, "es_ref", MAX_EVENT_SET_NAME_LEN, name, NULL))
		{
			if(bsles_get_ptr(gWm, name))
			{
				uint8_t actionMask = 0;
				BpSecEvtActionParms actionParms[BSLACT_MAX_PARM];
				BpSecEventId eventId = 0;

				memset(actionParms,0,sizeof(actionParms));

				if(getEventId(job, &eventId) < 0)
				{
					isprintf(gUserText, USER_TEXT_LEN, "[x] Malformed EventId for %s.", name);
					printText(gUserText);
				}
				else if(getActions(job, &actionMask, actionParms) < 0)
				{
					isprintf(gUserText, USER_TEXT_LEN, "[x] Malformed actions for %s.", name);
					printText(gUserText);
				}
				else if(bslevt_add(gWm, name, eventId, actionMask, actionParms) <= 0)
				{
					isprintf(gUserText, USER_TEXT_LEN, "[x] Error adding event %d to %s.", eventId, name);
					printText(gUserText);
				}
			}
			else
			{
				isprintf(gUserText, USER_TEXT_LEN, "[x] Eventset %s not found.", name);
				printText(gUserText);
			}
		}
		else
		{
			printText("[x] No es_ref in call to add event.");
		}
	}
	else if((start = jsonGetTypedIdx(job, 1, 0, "policyrule", JSMN_OBJECT)) > 0)
	{
		BpSecFilter filter;
		PsmAddress sci_parms = 0;
		uint16_t id = 0;
		PsmAddress esAddr = 0;
		char desc[BPSEC_RULE_DESCR_LEN+1];

		if (!parseFilter(job, &filter))
		{
			printText("[x] Filter criteria could not be processed");
		}
		else if (!getNewRuleId(job, &id))
		{
			printText("[x] Rule ID could not be processed");
		}
		else if(jsonGetTypedValue(job, start, 0, JSMN_STRING, "desc", BPSEC_RULE_DESCR_LEN, desc, NULL) < 0)
		{
			printText("[x] Error reading optional rule description.");
		}
		else if(jsonGetTypedValue(job, start, 0, JSMN_STRING, "es_ref", MAX_EVENT_SET_NAME_LEN, name, NULL) <= 0)
		{
			printText("[x] Missing Event set reference.");
		}
		else if((esAddr = bsles_get_addr(gWm, name)) == 0)
		{
			printText("[x] Undefined Event set.");
		}
		else if((sci_parms = getSecCtxtParms(job)) == 0)
		{
			printText("[x] Security context parameters could not be processed");
		}
		else
		{
			PsmAddress ruleAddr = 0;

			if((ruleAddr = bslpol_rule_create(gWm, desc, id, 0, filter, sci_parms, esAddr)) == 0)
			{
				isprintf(gUserText, USER_TEXT_LEN, "[x] Could not create rule %d.", id);
				printText(gUserText);
			}

			/* bslpol_rule_insert will free the rule if it cannot be added. */
			if(bslpol_rule_insert(gWm, ruleAddr, 1) <= 0)
			{
				isprintf(gUserText, USER_TEXT_LEN, "[x] Could not insert rule %d.", id);
				printText(gUserText);
			}
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


#if 0
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

	int start = 0;

	if (job.tokenCount < 2)
	{
		printText("Change what?");
		return;
	}

	if((start = jsonGetTypedIdx(job, 1, 0, "policyrule", JSMN_OBJECT)) > 0)
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

	SYNTAX_ERROR;
}

#endif



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
 * @brief Process the removal of BPSec policy objects provided a JSON object
 *
 * @param[in]  job    - The parsed JSON tokens.
 *
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

	if((start = jsonGetTypedIdx(job, 1, 0, "event_set", JSMN_OBJECT)) > 0)
	{
		if(jsonGetTypedValue(job, start, 0, JSMN_STRING, "name", MAX_EVENT_SET_NAME_LEN, name, NULL) > 0)
		{
			bsles_delete(gWm, name);
		}
		else
		{
			printText("[?] Missing event set name.");
		}
		return;
	}
	else if((start = jsonGetTypedIdx(job, 1, 0, "event", JSMN_OBJECT)) > 0)
	{
		if(jsonGetTypedValue(job, start, 0, JSMN_STRING, "es_ref", MAX_EVENT_SET_NAME_LEN, name, NULL) > 0)
		{
			BpSecEventId eventId = 0;
			if(getEventId(job, &eventId) > 0)
			{
				bslevt_delete(gWm, name, eventId);
			}
			else
			{
				isprintf(gUserText, USER_TEXT_LEN, "[x] Error removing event %d from %s.", eventId, name);
				printText(gUserText);
			}
		}
		else
		{
			printText("[?] Missing event set name.");
		}
		return;
	}
	else if((start = jsonGetTypedIdx(job, 1, 0, "policyrule", JSMN_OBJECT)) > 0)
	{
		uint16_t id = 0;
		if(getRuleId(job, &id) > 0)
		{
			bslpol_rule_remove_by_id(gWm, id);
		}
		else
		{
			printText("[?] Missing rule id.");
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


/******************************************************************************
 * @brief Prints an event object
 *
 * @param[in]   event - The event to be printed.
 *
 * @note
 *****************************************************************************/

static void printEvent(BpSecEvent *event)
{
	int idx = 0;
	int parmIdx = 0;

	char buf[2048];
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
				case BSLACT_NOT_IMPLEMENTED:
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



/******************************************************************************
 * @brief Prints an eventset name
 *
 * @param[in]   edPtr - The eventset whose name is to be printed.
 *
 * @note
 *****************************************************************************/

static void printEventsetName(BpSecEventSet *esPtr)
{
	char buf[MAX_EVENT_SET_NAME_LEN + 500]; //Max 255 named event sets
	memset(buf, '\0', sizeof(buf));

	isprintf(buf, sizeof(buf), "\nEventset name: %s\n Associated Policy Rules: %i\n",
			 esPtr->name, esPtr->ruleCount);

	printText(buf);
}



/******************************************************************************
 * @brief Prints an eventset
 *
 * @param[in]   esPtr - The eventset to be printed.
 *
 * @note
 *****************************************************************************/

static void printEventset(BpSecEventSet *esPtr)
{
	PsmAddress elt = 0;

	printEventsetName(esPtr);

	/* Print each event configured for the event set */
	for(elt = sm_list_first(gWm, esPtr->events); elt; elt = sm_list_next(gWm, elt))
	{
		BpSecEvent *event = (BpSecEvent *) psp(gWm, sm_list_data(gWm,elt));
		printEvent(event);
	}
}



/******************************************************************************
 * @brief Prints a policyrule.
 *
 * @param[in] rulePtr - The policyrule to be printed.
 * @param[in] verbose - Whether to print the full rule (1) or not (0)
 *
 * @note
 *****************************************************************************/

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

	isprintf(tmp, sizeof(tmp), "%s (Score: %d)", (strlen(rulePtr->desc) > 0) ? rulePtr->desc : "No Description", rulePtr->filter.score);
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
		isprintf(tmp, sizeof(tmp), " BDest \"%s\"", (char *) psp(gWm, rulePtr->filter.bundle_dest));
		strcat(buf,tmp);
	}

	if(rulePtr->filter.ssrc_len > 0)
	{
		isprintf(tmp, sizeof(tmp), " Ssrc \"%s\"", (char *) psp(gWm, rulePtr->filter.sec_src));
		strcat(buf,tmp);
	}

	if(BPSEC_RULE_BTYP_IDX(rulePtr))
	{
		isprintf(tmp, sizeof(tmp), " Type %i", rulePtr->filter.blk_type);
		strcat(buf,tmp);
	}

	if(BPSEC_RULE_SCID_IDX(rulePtr))
	{
		isprintf(tmp, sizeof(tmp), " ScID %i", rulePtr->filter.scid);
		strcat(buf,tmp);
	}
	strcat(buf,"\n\n");

	printText(buf);
}



/******************************************************************************
 * @brief Prints a Lyst of policyrules
 *
 * @param[in] rules   - The policyrules to be printed.
 * @param[in] verbose - Whether to print the full rule (1) or not (0)
 *
 * @note
 *****************************************************************************/

static void printRuleList(Lyst rules, int verbose)
{
	LystElt elt;

	/* lyst_length does a NULL check. */
	if(lyst_length(rules) <= 0)
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



/******************************************************************************
 * @brief Retrieves policyrule search criteria from JSON object
 *
 * @param[in]  job   - The parsed JSON tokens.
 * @param[in]  start - Starting search token index
 * @param[out] type  - Whether to retrieve ALL matches or the single BEST match
 * @param[out] tag   - The populates search criteria (search tag)
 *
 * @note
 * TODO: This is very close to getting a policyrule filter. Can we share a
 *       utility function?
 *
 * @retval -1 - Error
 * @retval  0 - Rule ID invalid
 * @retval  1 - Rule ID valid
 *****************************************************************************/

static int getFindCriteria(jsonObject job, int start, int *type, BpSecPolRuleSearchTag *tag)
{
	char tmp_str[GEN_PARM_LEN];
	int result = 0;

	CHKZERO(tag);

	/* Search the JSON object for the search type and process it. */
	if(jsonGetTypedValue(job, 1, 0, JSMN_STRING, "type", GEN_PARM_LEN, tmp_str, NULL) <= 0)
	{
		printText("[x] Search type missing.");
		return 0;
	}

	if(strcmp(tmp_str,"all") == 0)
	{
		*type = BPSEC_SEARCH_ALL;
	}
	else if(strcmp(tmp_str,"best") == 0)
	{
		*type = BPSEC_SEARCH_BEST;
	}
	else
	{
		isprintf(gUserText, USER_TEXT_LEN, "[x] unknown search type %s.", tmp_str);
		printText(gUserText);

		return 0;
	}

	/* Search the JSON object for various EIDs. */
	tag->bsrc = jsonAllocStrValue(job, start, 0, "src", MAX_EID_LEN, NULL);
	tag->bsrc_len = (tag->bsrc) ? istrlen(tag->bsrc, MAX_EID_LEN) : 0;

	tag->bdest = jsonAllocStrValue(job, start, 0, "dest", MAX_EID_LEN, NULL);
	tag->bdest_len = (tag->bdest) ? istrlen(tag->bdest, MAX_EID_LEN) : 0;

	tag->ssrc = jsonAllocStrValue(job, start, 0, "ssrc", MAX_EID_LEN, NULL);
	tag->ssrc_len = (tag->ssrc) ? istrlen(tag->ssrc, MAX_EID_LEN) : 0;

	/* Search for block type. A value of -1 indicates missing. */
	result = jsonGetTypedValue(job, 1, 0, JSMN_PRIMITIVE, "tgt", GEN_PARM_LEN, tmp_str, NULL);
	tag->type = (result > 0) ? atoi(tmp_str) : -1;

	// Role can be string or primitive.
	if((result = jsonGetTypedValue(job, 1, 0, JSMN_PRIMITIVE, "role", GEN_PARM_LEN, tmp_str, NULL)) <= 0)
	{
		result = jsonGetTypedValue(job, 1, 0, JSMN_STRING, "role", GEN_PARM_LEN, tmp_str, NULL);
	}
	tag->role = (result > 0) ? getMappedValue(gRoleMap, tmp_str) : 0;

	result = jsonGetTypedValue(job, 1, 0, JSMN_PRIMITIVE, "sc_id", GEN_PARM_LEN, tmp_str, NULL);
	tag->scid = (result > 0) ? atoi(tmp_str) : 0;

	return 1;
}



/******************************************************************************
 * @brief Processes a find command given a JSON parm object
 *
 * @param[in]  job   - The parsed JSON tokens.
 *
 * @note
 *****************************************************************************/

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

	if((start = jsonGetTypedIdx(job, 1, 0, "policyrule", JSMN_OBJECT)) > 0)
	{
		int type = 0;
		Lyst rules = NULL;
		BpSecPolRuleSearchTag tag;

		memset(&tag, 0, sizeof(tag));

		if(getFindCriteria(job, start, &type, &tag) <= 0)
		{
			printText("[x] Unable to find policyrule find criteria.");
			return;
		}

		switch(type)
		{
			case BPSEC_SEARCH_ALL:
				rules = bslpol_rule_get_all_match(gWm, tag);
				printRuleList(rules, 1);
				lyst_destroy(rules);
				break;

			case BPSEC_SEARCH_BEST:
				printRule(bslpol_rule_get_best_match(gWm, tag), 1);
				break;
			default:
				printText("[i] Unknown search type.");
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
 * @brief Processes a information command given a JSON parm object
 *
 * @param[in]  job   - The parsed JSON tokens.
 *
 * @note
 *****************************************************************************/

static void	executeInfoJson(jsonObject job)
{
	int start = 0;
	char name[MAX_EVENT_SET_NAME_LEN];
	memset(name, '\0', sizeof(name));

	if (job.tokenCount < 2)
	{
		printText("Information on what?");
		return;
	}

	if((start = jsonGetTypedIdx(job, 1, 0, "event_set", JSMN_OBJECT)) > 0)
	{
		if(jsonGetTypedValue(job, start, 0, JSMN_STRING, "name", MAX_EVENT_SET_NAME_LEN, name, NULL) > 0)
		{
			BpSecEventSet *esPtr = bsles_get_ptr(gWm, name);
			if(esPtr)
			{
				printEventset(esPtr);
			}
			else
			{
				isprintf(gUserText, USER_TEXT_LEN, "[?] Unknown event set %s", name);
				printText(gUserText);
			}
		}
		else
		{
			printText("[?] Missing event set name.");
		}
		return;
	}
	else if(jsonGetTypedValue(job, 1, 0, JSMN_PRIMITIVE, "policyrule", MAX_EVENT_SET_NAME_LEN, name, NULL) > 0)
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
 * @brief Processes a list command given a JSON parm object
 *
 * @param[in]  job   - The parsed JSON tokens.
 *
 * @note
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


	if(jsonGetTypedValue(job, 1, 0, JSMN_STRING, "type", MAX_EVENT_SET_NAME_LEN, name, NULL) < 0)
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
 * @brief Counts the number of times character c appears in a line.
 *
 * @param[in]  line    - A line of JSON text.
 * @param[in]  c       - A character to count.
 * @param[out] counter - The number of times the c appears in the line.
 *
 * @note
 * This is a helper function for parsing JSON objects. The character being
 * counted is usually object delimiters '{' or '}'.
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
 * @brief Extracts a command entered by the user to the bpsecadmin utility
 *        which uses JSON.
 *
 * @param[in]     line    - Line of JSON from which a command is extracted.
 * @param[in|out] jsonStr - The concatenated json command.
 * @param[in]     cmdFile - File from which line was extracted.
 * @param[in]     len     - Length of line.
 *
 * @note
 * This is a helper function for parsing JSON objects. The character being
 * counted is usually object delimiters '{' or '}'.
 *
 * @retval -1 - Error
 * @retval	0 - Command could not be retrieved.
 * @retval	1 - JSON command successfully retrieved.
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
	printText("[?] Invalid JSON syntax detected");
	return 0;
}



/******************************************************************************
 * @brief Process and execute a JSON command
 *
 * @param[in] line       - The JSON command to be executed.
 * @param[in] cmd        - (Optional) The single character command ('a',
 *                         'c', etc.) corresponding to the action the command
 *                         specifies in 'line'.
 * @param[in] cmdPresent - This value is set to true if the cmd field is
 *                         populated. The JSON command in 'line' does NOT
 *                         contain the command value if the cmdPresent
 *                         flag is set - instead, the value is found in the cmd
 *                         parameter.
 *
 * @note
 * This is a helper function for parsing JSON objects. The character being
 * counted is usually object delimiters '{' or '}'.
 *
 * @retval -1 - Error
 * @retval	0 - Command could not be successfully executed
 * @retval	1 - Command successfully executed.
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
			case '1':
			    if(init() < 0)
			    {
			        return -1;
			    }
			    return 0;
			case 'v':
				isprintf(buffer, sizeof(buffer), "%s", IONVERSIONNUMBER);
				printText(buffer);
				return 0;

			case 'a':
				if (attach(0) == 1)
				{
					executeAddJson(job);
				}
				return 0;
#if 0
			case 'c':
				if (attach(0) == 1)
				{
					executeChangeJson(job);
				}
				return 0;
#endif
			case 'd':
				if (attach(0) == 1)
				{
					executeDeleteJson(job);
				}
				return 0;

			case 'f':
				if (attach(0) == 1)
				{
					executeFindJson(job);
				}
				return 0;

			case 'i':
				if (attach(0) == 1)
				{
					executeInfoJson(job);
				}
				return 0;

			case 'l':
				if (attach(0) == 1)
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

        case '1':
            if(init() < 0)
            {
                return -1;
            }
            return 0;

		case 'a':
			if (attach(0) == 1)
			{
				executeAdd(tokenCount, tokens);
			}

			return 0;

		case 'c':
			if (attach(0) == 1)
			{
				executeChange(tokenCount, tokens);
			}

			return 0;

		case 'd':
			if (attach(0) == 1)
			{
				executeDelete(tokenCount, tokens);
			}

			return 0;

		case 'i':
			if (attach(0) == 1)
			{
				executeInfo(tokenCount, tokens);
			}

			return 0;

		case 'l':
			if (attach(0) == 1)
			{
				executeList(tokenCount, tokens);
			}

			return 0;

		case 'x':
			if (attach(0) == 1)
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

        if(init() <= 0)
        {
            return -1;
        }

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
