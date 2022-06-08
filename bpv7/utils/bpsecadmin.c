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

#include "bpsecadmin_config.h"

/*****************************************************************************
 *                              GLOBAL VARIABLES                             *
 *****************************************************************************/

PsmPartition gWm;
char gUserText[USER_TEXT_LEN];
const char *singleCmdCodes = "#?1ehvq";     /* Command codes that are used
											 * on their own, not paired with
											 * additional JSON */

typedef struct
{
	int tokenCount;
	jsmntok_t tokens[MAX_JSMN_TOKENS];
	char *line;
} jsonObject;

/*****************************************************************************
 *                             FUNCTION DEFINITIONS                          *
 *****************************************************************************/

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

static void	bpsec_admin_printText(char *text)
{
	if (_echo(NULL))
	{
		writeMemo(text);
	}

	PUTS(text);
}

static void	bpsec_admin_handleQuit(int signum)
{
	bpsec_admin_printText("Please enter command 'q' to stop the program.");
}

#if 0
static void	printSyntaxError(int lineNbr)
{
	char	buffer[80];

	isprintf(buffer, sizeof(buffer), "Syntax error at line %d of \
		bpsecadmin.c", lineNbr);
	bpsec_admin_printText(buffer);
}
#endif

#define	SYNTAX_ERROR	printSyntaxError(__LINE__)

static void	bpsec_admin_printUsage()
{
	PUTS("Valid commands are:");
	PUTS("\tq\tQuit");
	PUTS("\th\tHelp");
	PUTS("\t?\tHelp");
	PUTS("\tv\tPrint version of ION.");
	PUTS("\te\tEnable or disable echo of printed output to log file");
	PUTS("\t   e { 0 | 1 }");
	PUTS("\t#\tComment");
	PUTS("\t   # <comment text>");

    PUTS("\n\n\tJSON Security Policy Commands");
    PUTS("\t--------------------------------");
    PUTS("\tJSON keys wrapped in ?'s are optional in a command.");
    PUTS("\tIf included, the key should be represented without the ?'s.");
	PUTS("\tEvery eid expression must be a node identification expression, i.e., a partial eid expression ending in '*'.");

    PUTS("\n\t   ADD\n");

    PUTS("\t   a { \"event\" :");
    PUTS("\t       {");
    PUTS("\t          \"es_ref\"   : \"<event set name>\",");
    PUTS("\t          \"event_id\" : \"<event name>\",");
    PUTS("\t          \"actions\"  : [{\"id\":\"<action>\", <parms if applicable>},...]");
    PUTS("\t       }");
    PUTS("\t     }\n");

    PUTS("\t   a { \"event_set\" :");
    PUTS("\t       {");
    PUTS("\t          \"?desc?\" : \"<description>\",");
    PUTS("\t          \"name\"   : \"<event set name>\"");
    PUTS("\t       }");
    PUTS("\t     }\n");

    PUTS("\t   a { \"policyrule\" :");
    PUTS("\t       {");
    PUTS("\t          \"?desc?\"  : \"<description>\",");
    PUTS("\t          \"es_ref\"  : \"<event set name>\",");
    PUTS("\t          \"filter\"  : ");
    PUTS("\t          {");
    PUTS("\t             \"?rule_id?\" : <rule id>,");
    PUTS("\t             \"role\"      : \"<security role>\", ");
    PUTS("\t             \"src\"       : \"<source eid expression>\", ");
    PUTS("\t             \"dest\"      : \"<destination eid expression>\",  (1 of src/dest/sec_src required)");
    PUTS("\t             \"sec_src\"   : \"<security source eid expression>\",");
    PUTS("\t             \"tgt\"       : <security target block type>,");
    PUTS("\t             \"?sc_id?\"   : <security context id> (specified here if role is Security Verifier or Acceptor)");
    PUTS("\t          },");
    PUTS("\t          \"spec\" :");
    PUTS("\t          {");
    PUTS("\t             \"svc\"       : \"<security service>\",");
    PUTS("\t             \"?sc_id?\"   : <security context id>, (specified here if role is Security Source)");
    PUTS("\t             \"?sc_parms?\": [{\"<parm1_id>\":\"<parm1_value>\"},...,{\"<parm_id>\":\"<parm_value>\"}]");
    PUTS("\t          }");
    PUTS("\t       }");
    PUTS("\t     }\n\n");

    PUTS("\t   DELETE\n");

    PUTS("\t   d { \"event\" : ");
    PUTS("\t       {");
    PUTS("\t         \"es_ref\"   : \"<event set name>\",");
    PUTS("\t         \"event_id\" : \"<event name>\"");
    PUTS("\t       }");
    PUTS("\t     }\n");

    PUTS("\t   d { \"event_set\" :");
    PUTS("\t       {");
    PUTS("\t         \"name\"    : \"<event set name>\"");
    PUTS("\t       }");
    PUTS("\t     }\n");

    PUTS("\t   d { \"policyrule\" :");
    PUTS("\t       {");
    PUTS("\t         \"rule_id\"  : <rule id>");
    PUTS("\t       }");
    PUTS("\t     }\n\n");

    PUTS("\t   FIND\n");
    PUTS("\t   f { \"policyrule\" : ");
    PUTS("\t       {");
    PUTS("\t          \"type\"    : \"all\" | \"best\",");
    PUTS("\t          \"src\"     : \"<source eid expression>\",");
    PUTS("\t          \"dest\"    : \"<destination eid expression>\",   (1 of src/dest/sec_src required)");
    PUTS("\t          \"sec_src\" : \"<security source eid expression>\",");
    PUTS("\t          \"?sc_id?\"  : <security context id>,");
    PUTS("\t          \"?role?\"  : \"<security role>\"");
    PUTS("\t       }");
    PUTS("\t     }\n\n");

    PUTS("\t   INFO\n");
    PUTS("\t   i { \"event_set\" :");
    PUTS("\t       {");
    PUTS("\t         \"name\"    : \"<event set name>\"");
    PUTS("\t       }");
    PUTS("\t     }\n");

    PUTS("\t   i { \"policyrule\":");
    PUTS("\t       {");
    PUTS("\t         \"rule_id\": <rule id>");
    PUTS("\t       }");
	PUTS("\t     }\n\n");

    PUTS("\t   LIST\n");
    PUTS("\t   l {\"event_set\"}");
    PUTS("\t   l {\"policyrule\"}");


}

static int bpsec_admin_attach(int state)
{

    if (secAttach() != 0)
    {
        putErrmsg("Failed to attach to security database.", NULL);
        return 0;
    }

    SecVdb *vdb = getSecVdb();
    if (vdb == NULL)
    {
        putErrmsg("Failed to retrieve security database.", NULL);
        return 0;
    }

    if((gWm = getIonwm()) == NULL)
    {
        putErrmsg("ION is not running.", NULL);
        return 0;
    }

    if(state == 1)
    {
        if (bsl_all_init(gWm) < 1)
        {
            putErrmsg("Failed to initialize BPSec policy.", NULL);
            return 0;
        }
    }

    return 1;
}

/******************************************************************************
 *
 * \par Function Name: bpsec_admin_init
 *
 * \par Purpose: This function initializes the bpsecadmin utility, attaching
 *               to the security database and initializing ION working memory.
 *
 * @retval int
 *****************************************************************************/
static int bpsec_admin_init()
{

    /* This will call ionAttach */
	if (secInitialize() < 0)
	{
		putErrmsg("Can't initialize the ION security system.", NULL);
		return 0;
	}

	return bpsec_admin_attach(1);
}

/******************************************************************************
 * @brief Retrieve the index of the next key field from a set of JSON tokens
 * 		  within the provided level.
 *
 * Example JSON: {K1 :
 * 					{K2 : V2,
 * 					 K3 :{K4:V4,
 * 					 	  K5:V5}
 * 					 K6: V6
 * 					}
 * 				  }
 * 		To retrieve the index of the first key in the command:
 * 			currentKeyIndex = -1
 * 			level = 0
 * 			return value = index of K1
 * 		To retrieve the key that follows K3 (at level 2):
 * 			currentKeyIndex = index of K3
 * 			level = 2 (the level of K3. K1 is at level 1).
 * 			return value = K6
 *
 *
 * @param[in] job        - Parsed JSON tokens.
 * @param[in] currKeyIdx - Current key index to search from.
 * @param[in] level      - Level of JSON tokens to be searched for the next key.
 *
 * @retval   -1 - Error. The next key could not be found.
 * @retval    0 - No more keys exist at this level.
 * @retval  >=1 - Index of the next key found at this level
 *****************************************************************************/

static int bpsec_admin_json_getNextKeyAtLevel(jsonObject job, int currKeyIdx, int level)
{
	CHKERR(currKeyIdx >= -1);

	int idx;

	/* Step 1: Increment the key index to find the key that occurs
	 * next (after) the current key. */
	currKeyIdx++;

	/* Step 2: Check if there are more keys to search in the JSON object. */
	if(currKeyIdx >= job.tokenCount)
	{
		/* Step 2.1: No more keys to search */
		return 0;
	}

	/* Step 3: Iterate over the JSON tokens from the current key index. */
	for (idx = currKeyIdx; idx < job.tokenCount; idx++)
	{

		/* Step 3.1: Check that the key found is at the level specified. */
		if (job.tokens[idx].parent == level)
		{
			/* Step 3.2: Next key index found. */
			return idx;
		}
	}

	return -1;
}

/******************************************************************************
 * @brief Retrieve the index of the value associated with a given key in a
 * 		  JSON policy command.
 * 		  The value index for a provided key is typically keyIdx+1. This
 * 		  function is useful in catching corner cases where a value is not
 * 		  present in the JSON.
 *
 * @param[in] job    - Parsed JSON tokens.
 * @param[in] keyIdx - The index of the key to retrieve the value index for.
 *
 * @retval  -1 - Error. The value index for the provided key was not found.
 * @retval >=0 - Index of the value associated with the key provided.
 *****************************************************************************/

static int bpsec_admin_json_getValueIdx(jsonObject job, int keyIdx)
{
	CHKERR(keyIdx >= 0);

	int idx;

	/* Step 1: Search the JSON tokens following the current key index to find
	 * its associated value. */
	for (idx = keyIdx; idx < job.tokenCount; idx++)
	{
		/* Step 2: The value for the key will have its parent index set to
		 * the key index. */
		if(job.tokens[idx].parent == keyIdx)
		{
			return idx;
		}
	}

	return -1;
}

/******************************************************************************
 * @brief Retrieve a typed value from a set of JSON tokens using the provided
 *        index.
 *
 * @param[in]      job        - Parsed JSON tokens.
 * @param[in]      type       - Expected jsmn type of the value to extract.
 * @param[in]      idx        - Index of the value in the JSON tokens.
 * @param[in||out]  value      - The user-supplied field to hold the value.
 * @param[in]      size       - The size of the user-supplied field.
 *
 * @retval -1 - Error. The value was not extracted.
 * @retval  1 - Value successfully extracted from the provided JSON.
 *****************************************************************************/

static int bpsec_admin_json_getValueAtIdx(jsonObject job, jsmntype_t type, int idx, char *value, int size)
{
	/* Step 0: Sanity checks */
	CHKERR(idx >= 0);
	CHKERR(type >= 0);
	CHKERR(size >= 1);

	/* Step 1: Check that the index is in bounds. */
	if(idx < 0 || idx > job.tokenCount)
	{
		return -1;
	}

	/* Step 2: Check that the token type matches expected. */
	if(job.tokens[idx].type == type)
	{
		/* Step 3: Initialize the value field. */
		memset(value, '\0', size);

		/* Step 4: Obtain the length of the value from the JSON and
		 * check that the provided field can accommodate it */
		int len = job.tokens[idx].end - job.tokens[idx].start;

		if(len > size)
		{
			isprintf(gUserText, USER_TEXT_LEN, "Length of value exceeds permitted size. Length of value: %i. Max size: %i.", len, size);
			bpsec_admin_printText(gUserText);
			return -1;
		}

		/* Step 5: Copy JSON token value into the user-provided value field. */
		if(istrcpy(value, job.line+job.tokens[idx].start, len+1) == NULL)
		{
			isprintf(gUserText, USER_TEXT_LEN, "Command value was not successfully copied for JSON command at index %i.", idx);
			bpsec_admin_printText(gUserText);
			return -1;
		}

		return 1;
	}

	//isprintf(gUserText, USER_TEXT_LEN, "Command value was not found in provided JSON at index %i with type %i.", idx, type);
	//bpsec_admin_printText(gUserText);

	return -1;
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
 * @retval 0  - The value was not found
 *****************************************************************************/
static int bpsec_admin_json_getTypedIdx(jsonObject job, int start, int end, char *key, int type)
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

static int bpsec_admin_json_getTypedValue(jsonObject job, int start, int end, int type, char *key, int max, char *value, int *idx)
{
	int result = 0;
	int i = 0;

	/* Retrieve the index of the token holding the value. */
	i = bpsec_admin_json_getTypedIdx(job, start, end, key, type);

	if(i > 0)
	{
		/* Calculate the length of the found value. */
		int len = MIN(job.tokens[i].end - job.tokens[i].start, max);

		/* Clear the value field. */
		memset(value,0,max);

		/* Copy JSON segment into the value. */
		if(istrcpy(value, job.line+job.tokens[i].start, len+1) == NULL)
		{
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

static char *bpsec_admin_json_allocStrValue(jsonObject job, int start, int end, char *key, int max, int *idx)
{
	char *tmp = (char*) MTAKE(max+1);

	if(bpsec_admin_json_getTypedValue(job, start, end, JSMN_STRING, key, max, tmp, idx) <= 0)
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
static int bpsec_admin_getMappedValue(BpSecMap map[], char *key)
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
 * The expected JSON key-value pair is "event_id" : "<event name>"
 *
 * @retval  1  - The event was found and is valid.
 * @retval  0  - The event was not found or was invalid.
 * @retval -1  - Error extracting the event id.
 *****************************************************************************/

static int bpsec_admin_json_getEventId(jsonObject job, BpSecEventId *event)
{
	char eventName[MAX_EVENT_LEN+1];
	int result = 1;

	CHKERR(event);

	if(bpsec_admin_json_getTypedValue(job, 1, 0, JSMN_STRING, KNS_EVENT_ID, MAX_EVENT_LEN, eventName, NULL) > 0)
	{
		/* Check that provided event is supported */
		*event = bslevt_get_id(eventName);

		if(*event == unsupported)
		{
			isprintf(gUserText, USER_TEXT_LEN, "Security operation event %s is not supported.", eventName);
			bpsec_admin_printText(gUserText);

			result = 0;
		}
	}

	else
	{
		bpsec_admin_printText("Event format incorrect. Event name not found.");
		result = -1;
	}

	return result;
}

/******************************************************************************
 * @brief Using the text security service provided by the user, this function
 *        returns the security service number corresponding to the supported 
 *        value.
 *
 * @param[in]  svc_str  - User-provided string name of security service.
 *                        Ex: "bib-integrity"
 *
 * @retval -1  - The security service provided was invalid.
 * @retval >0  - The security service value.
 *****************************************************************************/

static int bpsec_admin_getSvc(char *svc_str)
{
	int svc = -1;

	/* Find the security service value in the map of security services. */
  	if((svc = bpsec_admin_getMappedValue(gSvcMap, svc_str)) > 0)
  	{
		return svc;
  	}
	else
	{
		isprintf(gUserText, USER_TEXT_LEN, "Security service %s unknown. Supported security services are: \n\tbib-integrity\n\tbcb-confidentiality", svc_str);
		bpsec_admin_printText(gUserText);

		svc = -1;
		return svc;
	}

	svc = -1;
  	return svc;
}



/******************************************************************************
 * @brief Sets an action bit in a bitmask associated with a named action
 *
 * @param[in]  action     - The name of the action being added to the mask
 * @param[out] actionMask - The updated action mask.
 *
 * @note
 * An action must be both defined AND map to a supported, implemented enumeration
 * to be set by this function.
 *
 * @retval !0 - The action enumeration OR'd into the mask
 * @retval 0  - The action was not added to the mask
 *****************************************************************************/

static int bpsec_admin_setAction(char *action, uint8_t *actionMask)
{
  int value = 0;

  /* Find the action value in the map of action values. */
  if((value = bpsec_admin_getMappedValue(gActionMap, action)) > 0)
  {
	  /* If the action was found but is not implemented...*/
	  if(value == BSLACT_NOT_IMPLEMENTED)
	  {
		isprintf(gUserText, USER_TEXT_LEN, "Action %s currently not supported.", action);
		bpsec_admin_printText(gUserText);

		value = 0;
	  }
	  else
	  {
		  *actionMask |= value;
	  }
  }

  else
  {
	  isprintf(gUserText, USER_TEXT_LEN, "Unknown action: %s.", action);
	  bpsec_admin_printText(gUserText);
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
 * @note 
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

static int bpsec_admin_json_getActions(jsonObject job, uint8_t *actionMask, BpSecEvtActionParms *parms)
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
	start = bpsec_admin_json_getTypedIdx(job, 1, 0, KNS_ACTIONS, JSMN_ARRAY);

	if(start <= 0)
	{
		isprintf(gUserText, USER_TEXT_LEN, "Cannot find processing actions array object in JSON.", NULL);
		bpsec_admin_printText(gUserText);

		return -1;
	}

	/* Walk through tokens, looking for actions to add. */
	while (start < job.tokenCount)
	{
		/*
		 * All valid actions start with "id". If there are no more valid actions, we are done.
		 */
		if(bpsec_admin_json_getTypedValue(job, start, 0, JSMN_STRING, KNS_ID, MAX_ACTION_LEN, actionStr, &start) <= 0)
		{
			break;
		}

		/* Convert action name to an enumeration and update the action mask. */
		curAct = bpsec_admin_setAction(actionStr, actionMask);

		/* Process the action based on its enumeration. */
		switch(curAct)
		{
			/* If this is a report action, reas in the reason code to report. */
			case BSLACT_REPORT_REASON_CODE:
				if(bpsec_admin_json_getTypedValue(job, start, start+1, JSMN_PRIMITIVE, KNS_REASON_CODE, 64, parmStr, &start) > 0)
				{
				   parms[parmIdx++].asReason.reasonCode = atoi(parmStr);
				}
				else
				{
					isprintf(gUserText, USER_TEXT_LEN, "No reason code supplied for action Report Reason Code %d.", curAct);
					bpsec_admin_printText(gUserText);

					return -1;
				}
				break;

			/* If this is an override action, get the mask/value for the override. */
			case BSLACT_OVERRIDE_TARGET_BPCF:
			case BSLACT_OVERRIDE_SOP_BPCF:
				numParm = 0;
				if(bpsec_admin_json_getTypedValue(job, start, start+3, JSMN_PRIMITIVE, KNS_MASK, 64, parmStr, NULL) > 0)
				{
				   parms[parmIdx].asOverride.mask = (uint64_t) strtol(parmStr, NULL, 16);
				   numParm++;
				}
				if(bpsec_admin_json_getTypedValue(job, start, start+3, JSMN_PRIMITIVE, KNS_NEW_VALUE, 64, parmStr, NULL) > 0)
				{
				   parms[parmIdx].asOverride.val = (uint64_t) strtol(parmStr, NULL, 16);
				   numParm++;
				}
				if(numParm != 2)
				{
					isprintf(gUserText, USER_TEXT_LEN, "Invalid parameters for override action. Check block processing flags.", NULL);
					bpsec_admin_printText(gUserText);

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
				isprintf(gUserText, USER_TEXT_LEN, "Unknown processing action %s.", actionStr);
				bpsec_admin_printText(gUserText);

				return 0;
				break;

			/* If the action is not implemented, skip and process other actions. */
			case BSLACT_NOT_IMPLEMENTED:
				isprintf(gUserText, USER_TEXT_LEN, "Action %s not implemented.", actionStr);
				bpsec_admin_printText(gUserText);
				break;
			default:
				break;
		}
	}
	return 1;
}


/******************************************************************************
 * @brief Retrieves and validates the security context ID from the JSON tokens.
 * 		  Will convert a security context name to ID if that security context
 *        is supported.
 *
 * @param[in]   job   - The parsed JSON tokens.
 * @param[out]  sc_id - Security context ID.
 *
 * @retval -1 - Provided security context was invalid/unsupported.
 * @retval  0 - sc_id was not present in the JSON. Note that this is NOT an error.
 * @retval  1 - sc_id was present and was validated.
 *****************************************************************************/

static int bpsec_admin_json_getScId(jsonObject job, int *sc_id)
{
	int result = -1;
	int input_sc_id = BPSEC_UNSUPPORTED_SC;
	char sc_id_num[NUM_STR_LEN];
	char sc_id_str[JSON_VAL_LEN];

	/* sc_id will default to unsupported (out of range).
	 * If this function returns -1, the initialized sc_id will not be used. */
	*sc_id = BPSEC_UNSUPPORTED_SC;

	/* The security context may be provided by ID (a primitive) */
	if((result = bpsec_admin_json_getTypedValue(job, 1, 0, JSMN_PRIMITIVE, KNS_SC_ID, NUM_STR_LEN, sc_id_num, NULL)) <= 0)
	{
		/* Or the security context may be identified by its name */
		if((result = bpsec_admin_json_getTypedValue(job, 1, 0, JSMN_STRING, KNS_SC_ID, JSON_VAL_LEN, sc_id_str, NULL)) <= 0)
		{
			/* Security context ID is missing - not provided as an int or string. 
			 * This is permitted for filter/find criteria. */
			return 0;
		}

		/* Get the SC ID associated with the string name provided */
		result = bpsec_sci_idFind(sc_id_str, &input_sc_id);

		/* If the SC ID is found, return it */
		if(result == 1)
		{
			*sc_id = input_sc_id;
			return 1;
		}
		/* Otherwise, the SC name provided is invalid */
		else
		{
			isprintf(gUserText, USER_TEXT_LEN, "Security context %s is not supported.", sc_id_str);
			bpsec_admin_printText(gUserText);
			return -1;
		}
	}
	else if (result > 0)
	{
		input_sc_id = atoi(sc_id_num);

		/* Lookup the provided SC ID to determine if it has a valid security context associated */
		if (bpsec_sci_defFind(input_sc_id, NULL) == 1)
		{
			/* Return 1 to indicate the SC ID lookup was successful */
			*sc_id = input_sc_id;
			return 1;
		}
		/* Otherwise, the SC ID provided is invalid */
		else
		{
			isprintf(gUserText, USER_TEXT_LEN, "Security context %d is not supported.", input_sc_id);
			bpsec_admin_printText(gUserText);
			return -1;
		}
	}

	/* Otherwise, the sc_id field is not present in the filter/find criteria, which
	 * is permitted in some cases. */
	return 0;
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
 * @param[out]  svc   - Security service.
 *
 * @note
 * All out parms are expected to have been pre-allocated by the calling function.
 * \par
 * All EID values are expected to be of MAX_EID_LEN length and memset to 0.
 * \par
 * role, sc_id, and svc remain 0 if they are not set in the JSON.
 * \par
 * sc_id and type are set to -1 if their values are not set in the JSON
 *
 * @retval  1 - Filter criteria parsed successfully - parameters populated
 * @retval  0 - Filter criteria parsed unsuccessfully
 * @retval -1 - Error.
 *****************************************************************************/

static int bpsec_admin_json_getFilterCriteria(jsonObject job, char *bsrc, char *bdest, char *ssrc, int *type, int *role, int *sc_id, int *svc)
{
	int result = 0;

	char num_str[NUM_STR_LEN];
	char role_str[SEC_ROLE_LEN];
	char svc_str[JSON_VAL_LEN];

	*role = 0;
	*type = -1;
	*sc_id = 0;
	*svc = 0;

	if(bpsec_admin_json_getTypedValue(job, 1, 0, JSMN_STRING, KNS_SRC, MAX_EID_LEN, bsrc, NULL) < 0)
	{
		bpsec_admin_printText("Malformed bundle source provided. Expected a string EID.");
		return 0;
	}

	if(bpsec_admin_json_getTypedValue(job, 1, 0, JSMN_STRING, KNS_DEST, MAX_EID_LEN, bdest, NULL) < 0)
	{
		bpsec_admin_printText("Malformed bundle destination provided. Expected a string EID.");
		return 0;
	}

	if(bpsec_admin_json_getTypedValue(job, 1, 0, JSMN_STRING, KNS_SEC_SRC, MAX_EID_LEN, ssrc, NULL) < 0)
	{
		bpsec_admin_printText("Malformed security source provided. Expected a string EID.");
		return 0;
	}

	if((result = bpsec_admin_json_getTypedValue(job, 1, 0, JSMN_PRIMITIVE, KNS_TGT, NUM_STR_LEN, num_str, NULL)) < 0)
	{
		bpsec_admin_printText("Malformed target block type provided. Expected a block type identifier (int).");
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
	if((result = bpsec_admin_json_getTypedValue(job, 1, 0, JSMN_PRIMITIVE, KNS_ROLE, SEC_ROLE_LEN, role_str, NULL)) <= 0)
	{
		result = bpsec_admin_json_getTypedValue(job, 1, 0, JSMN_STRING, KNS_ROLE, SEC_ROLE_LEN, role_str, NULL);
	}

	/* A result of 0 means the role was not found in the JSON, which is OK. */
	if(result < 0)
	{
		bpsec_admin_printText("Malformed security role provided. Supported roles are: \n\t\"sec_source\"\n\t\"sec_verifier\"\n\t\"sec_acceptor\"");
		return 0;
	}
	else if (result > 0)
	{
		*role = bpsec_admin_getMappedValue(gRoleMap, role_str);
	}

	/* A result of -1 means the sc_id in the JSON was invalid. */
	if((result = bpsec_admin_json_getScId(job, sc_id)) == -1)
	{
		bpsec_admin_printText("Malformed security context identifier provided.");
		return 0;
	}
	/* A result of 1 means that the sc_id was found and is valid. Result == 0 indicates that
	 * the sc_id field is missing, which is permitted in some cases. */
	/* Result is 0 if sc_id is missing */
	if((*role == BPRF_SRC_ROLE) && (result == 0))
	{
		bpsec_admin_printText("Security sources MUST specify a security context identifer (\"sc_id\").");
		return 0;
	}

	if((result = bpsec_admin_json_getTypedValue(job, 1, 0, JSMN_STRING, KNS_SVC, JSON_VAL_LEN, svc_str, NULL)) < 0)
	{
		bpsec_admin_printText("Malformed security service provided. Supported security services are: \n\tbib-integrity\n\tbcb-confidentiality");
		return 0;
	}
	else if (result > 0)
	{
		if ((*svc = bpsec_admin_getSvc(svc_str)) < 0)
		{
			/* If the security service cannot be mapped to a supported service,
			 * the bpsec_admin_getSvc function will print a detailed error message. */
			return 0;
		}
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

static int bpsec_admin_json_parseFilter(jsonObject job, BpSecFilter *filter)
{
	int type = -1;
	int role = 0;
	int sc_id = BPSEC_UNSUPPORTED_SC;
	int svc = 0;

	char bsrc[MAX_EID_LEN];
	char bdest[MAX_EID_LEN];
	char ssrc[MAX_EID_LEN];

	memset(bsrc, '\0', sizeof(bsrc));
	memset(bdest, '\0', sizeof(bdest));
	memset(ssrc, '\0', sizeof(ssrc));

	if (bpsec_admin_json_getFilterCriteria(job, bsrc, bdest, ssrc, &type, &role, &sc_id, &svc))
	{
		/* After parsing JSON, build filter for policy rule*/
		*filter = bslpol_filter_build(gWm, bsrc, bdest, ssrc, type, role, sc_id, svc);

		if(filter->flags)
		{
			return 1;
		}

		else
		{
			bpsec_admin_printText("Filter information for policy rule invalid.");
			return 0;
		}
	}

	bpsec_admin_printText("Malformed filter criteria found for policy rule.");
	return 0;
}


/******************************************************************************
 * @brief Retrieve a set of security context parameters from a JSON object
 *
 * @param[in]   job    - The parsed JSON tokens.
 *
 * @note
 * Currently, this is a lyst of sci_value structures.
 * \par
 * The created Lyst has a delete callback to help with lyst cleanup later
 * \par
 * TODO: Read parameters into a generic structure
 *
 * @retval  !NULL - Lyst of extracted security parameters
 * @retval  NULL  - Error extracting security parameters
 *****************************************************************************/

static PsmAddress bpsec_admin_json_getSecCtxtParms(jsonObject job, sc_Def *secCtx)
{
	int scParmIdx = 0;
	int scParmValIdx = 0;
	int scParmArrIdx = 0;
	int scKvPair = 0;
	char curId[JSON_KEY_LEN];
	char curVal[JSON_VAL_LEN];
	PsmAddress result = 0;
	PsmPartition wm = getIonwm();

	/* Step 1: Find the index of the security context parameter array. */
	if((scParmArrIdx = bpsec_admin_json_getTypedIdx(job, 1, 0, KNS_SC_PARMS, JSMN_ARRAY)) <= 0)
	{
		return 0;
	}

	/* Step 2: Create a shared memory list to hold the parms we did find. */
	result = sm_list_create(wm);
	CHKZERO(result);

	/* Step 3: Start processing parms at the start of the sc_parms JSON array. */
	scParmIdx = bpsec_admin_json_getNextKeyAtLevel(job, scParmArrIdx, scParmArrIdx+1);

	while(scParmIdx > 0)
	{
		/* 	Security context parms are provided in the form:
		 *  [{<parm id 1>, <value 1>},
		 *   {<parm id 2>, <value 2>},
		 *   ...
		 *   {<parm id n>, <value n>}]
		 */

		/* Step 3.1: Retrieve parm id. All parm ids must be strings */
		if(bpsec_admin_json_getValueAtIdx(job, JSMN_STRING, scParmIdx, curId, sizeof(curId)) <= 0)
		{
			/* If we can't retrieve the next parm id, assume there are no
			 * more parms to be processed. */
			scParmIdx = 0;
		}

		/* Step 3.2 Retrieve parm value index. */
		else
		{
			if((scParmValIdx = bpsec_admin_json_getValueIdx(job, scParmIdx)) < 0)
			{
				isprintf(gUserText, USER_TEXT_LEN, "Cannot find value for sc_parm id: %s", curId);
				bpsec_admin_printText(gUserText);
				sm_list_destroy(wm, result, bpsec_scv_smlistCbDel, NULL);
				return 0;
			}

			/* Step 3.3 Retrieve parm value. All parm values must be jsmn strings */
			if(bpsec_admin_json_getValueAtIdx(job, JSMN_STRING, scParmValIdx, curVal, sizeof(curVal)) <= 0)
			{
				isprintf(gUserText, USER_TEXT_LEN, "Cannot parse sc_parm %s.", curId);
				bpsec_admin_printText(gUserText);

				sm_list_destroy(wm, result, bpsec_scv_smlistCbDel, NULL);
				return 0;
			}

			//isprintf(gUserText, USER_TEXT_LEN, "Adding sc_parm %s with value %s", curId, curVal);
			//bpsec_admin_printText(gUserText);

			/* Step 4: Add the SCI parameter to shared memory */
			if(bpsec_sci_polParmAdd(wm, result, secCtx, curId, curVal) != 0)
			{
                isprintf(gUserText, USER_TEXT_LEN, "SCI cannot add sc_parm %s", curId);
				bpsec_admin_printText(gUserText);
				sm_list_destroy(wm, result, bpsec_scv_smlistCbDel, NULL);
				return 0;

			}

			/* Step 5: Advance to the next key-value pair of security context parameters.
			 * If we are at the end of parameters to process, scParkIdx is set to an out
			 * of bounds value, handled in step 3.1. */
			scKvPair = bpsec_admin_json_getNextKeyAtLevel(job, scParmValIdx, scParmArrIdx);
			scParmIdx = scKvPair+1; 
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

static int bpsec_admin_json_getRuleId(jsonObject job, uint16_t *ruleId)
{
	char id[RULE_ID_LEN];
	memset(id, '\0', sizeof(id));

	*ruleId = 0;

	if(bpsec_admin_json_getTypedValue(job, 1, 0, JSMN_STRING, KNS_RULE_ID, RULE_ID_LEN, id, NULL) <= 0)
	{
		if(bpsec_admin_json_getTypedValue(job, 1, 0, JSMN_PRIMITIVE, KNS_RULE_ID, RULE_ID_LEN, id, NULL) <= 0)
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

static int bpsec_admin_json_getNewRuleId(jsonObject job, uint16_t *ruleId)
{
	CHKZERO(ruleId);

	/* Extract the ruleID from the JSON tokens. */
	bpsec_admin_json_getRuleId(job, ruleId);

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

		isprintf(gUserText, USER_TEXT_LEN, "No available rule IDs. Max # of %s reached. \nDelete existing policy rules before adding a new rule.", MAX_RULE_ID-1);
		bpsec_admin_printText(gUserText);

		return 0;
	}

	/* Otherwise, check if user rule ID is already in use */
	else if (bslpol_rule_get_ptr(gWm, *ruleId) != NULL)
	{
		isprintf(gUserText, USER_TEXT_LEN, "Rule %d already defined. Add this rule using a different rule ID.", *ruleId);
		bpsec_admin_printText(gUserText);

		return 0;
	}

	return 1;
}


#if 0
/** Note that this is NOT dead code. This function is a placeholder for the
 *  implementation of anonymous event sets. */
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
 * TODO: Implement anonymous event sets.
 *****************************************************************************/
PsmAddress createAnonEventset(jsonObject job)
{

	writeMemo("[?] Anonymous event sets unsupported.");
	return 0;
}

#endif


/******************************************************************************
 * @brief Add a security policy event set from the provided JSON object.
 *
 * @param[in]  job  - The parsed JSON tokens.
 *
 *****************************************************************************/

static void	bpsec_admin_addEventSet(jsonObject job)
{
	int start = 0;
	char name[MAX_EVENT_SET_NAME_LEN];
	char desc[MAX_EVENT_SET_DESC_LEN];

	memset(name, '\0', sizeof(name));
	memset(desc, '\0', sizeof(desc));

	/* Step 1: Retrieve eventset name (required). */
	if(bpsec_admin_json_getTypedValue(job, start, 0, JSMN_STRING, KNS_NAME, MAX_EVENT_SET_NAME_LEN, name, NULL))
	{
		/* Step 2: Retrieve eventset description (optional). */
		bpsec_admin_json_getTypedValue(job, start, 0, JSMN_STRING, KNS_DESC, MAX_EVENT_SET_DESC_LEN, desc, NULL);
		
		/* Step 3: Add the eventset with name and description. */
		if(bsles_add(gWm, name, desc) < 0)
		{
			isprintf(gUserText, USER_TEXT_LEN, "Error adding eventset %s.", name);
			bpsec_admin_printText(gUserText);
		}
	}
	else
	{
		bpsec_admin_printText("Error adding named eventset.");
	}
	return;
}


/******************************************************************************
 * @brief Add a security policy event to an existing event set from the 
 * provided JSON object.
 *
 * @param[in]  job  - The parsed JSON tokens.
 *
 *****************************************************************************/

static void	bpsec_admin_addEvent(jsonObject job)
{
	int start = 0;
	char name[MAX_EVENT_SET_NAME_LEN];

	memset(name, '\0', sizeof(name));

	if(bpsec_admin_json_getTypedValue(job, start, 0, JSMN_STRING, KNS_ES_REF,
			MAX_EVENT_SET_NAME_LEN, name, NULL))
	{
		if(bsles_get_ptr(gWm, name))
		{
			uint8_t actionMask = 0;
			BpSecEvtActionParms actionParms[BSLACT_MAX_PARM];
			BpSecEventId eventId = 0;

			memset(actionParms, 0, sizeof(actionParms));

			if(bpsec_admin_json_getEventId(job, &eventId) < 0)
			{
				isprintf(gUserText, USER_TEXT_LEN, "Malformed security operation event id for eventset %s.", name);
				bpsec_admin_printText(gUserText);
			}
			else if(bpsec_admin_json_getActions(job, &actionMask, actionParms) < 0)
			{
				isprintf(gUserText, USER_TEXT_LEN, "Malformed processing action(s) for eventset %s.", name);
				bpsec_admin_printText(gUserText);
			}
			else if(bslevt_add(gWm, name, eventId, actionMask, actionParms) <= 0)
			{
				isprintf(gUserText, USER_TEXT_LEN, "Error adding security operation event %d to %s.", eventId, name);
				bpsec_admin_printText(gUserText);
			}
		}
		else
		{
			isprintf(gUserText, USER_TEXT_LEN, "Eventset %s not found.", name);
			bpsec_admin_printText(gUserText);
		}
	}
	else
	{
		bpsec_admin_printText("No \"es_ref\" in call to add event. Must include a named eventset.");
	}
}


/******************************************************************************
 * @brief Add a security policy rule from provided JSON object.
 *
 * @param[in]  job  - The parsed JSON tokens.
 *
 *****************************************************************************/

static void	bpsec_admin_addPolicyrule(jsonObject job)
{
	int 			start = 0;
	BpSecFilter 	filter;
	PsmAddress  	sci_parms = 0;
	uint16_t 		id = 0;
	PsmAddress 		esAddr = 0;
	char name[MAX_EVENT_SET_NAME_LEN];
	char desc[BPSEC_RULE_DESCR_LEN+1];

	memset(name, '\0', sizeof(name));
	memset(desc, '\0', sizeof(desc));

	if(!bpsec_admin_json_parseFilter(job, &filter))
	{
		bpsec_admin_printText("Filter criteria could not be processed.");
	}
	else if (!bpsec_admin_json_getNewRuleId(job, &id))
	{
		bpsec_admin_printText("Rule ID could not be processed.");
	}
	else if(bpsec_admin_json_getTypedValue(job, start, 0, JSMN_STRING, KNS_DESC, BPSEC_RULE_DESCR_LEN, desc, NULL) < 0)
	{
		isprintf(gUserText, USER_TEXT_LEN, "Error reading optional rule description. Ensure that description is within character limit of %d.", BPSEC_RULE_DESCR_LEN);
		bpsec_admin_printText(gUserText);
	}
	else if(bpsec_admin_json_getTypedValue(job, start, 0, JSMN_STRING, KNS_ES_REF, MAX_EVENT_SET_NAME_LEN, name, NULL) <= 0)
	{
		bpsec_admin_printText("Missing event set reference (\"es_ref\").");
	}
	else if((esAddr = bsles_get_addr(gWm, name)) == 0)
	{
		isprintf(gUserText, USER_TEXT_LEN, "Undefined event set %s.", name);
		bpsec_admin_printText(gUserText);
	}
	else
	{
		PsmAddress ruleAddr = 0;

		/* If the security context ID is provided by the security policy rule,
		   look up the security context definition to be used to add the security
		   policy parameters. */
		if(filter.flags & BPRF_USE_SCID)
		{
			sc_Def secCtx;
			int sci_lookup = bpsec_sci_defFind(filter.scid, &secCtx);

			if(sci_lookup <= 0)
			{
				isprintf(gUserText, USER_TEXT_LEN, "Unsupported security context %i", filter.scid);
				bpsec_admin_printText(gUserText);
				return;
			}

			if((sci_parms = bpsec_admin_json_getSecCtxtParms(job, &secCtx)) == 0)
			{
				bpsec_admin_printText("Security context parameters could not be processed.");
				return;
			}
		}
		/* If the security policy rule specifies security context parameters without identifying
		   the security context to use when processing them, it's an error. */
		else if ((!(filter.flags & BPRF_USE_SCID)) && (bpsec_admin_json_getTypedIdx(job, 1, 0, KNS_SC_PARMS, JSMN_ARRAY) > 0))
		{
			bpsec_admin_printText("Security context parameters provided without security context identifier.");
			bpsec_admin_printText("\"sc_id\" field must be present in policy rules providing \"sc_parms\".");
			return;
		}
		
		if((ruleAddr = bslpol_rule_create(gWm, desc, id, 0, filter, sci_parms, esAddr)) == 0)
		{
			isprintf(gUserText, USER_TEXT_LEN, "Could not create rule %d.", id);
			bpsec_admin_printText(gUserText);
		}

		/* bslpol_rule_insert will free the rule if it cannot be added. */
		if(bslpol_rule_insert(gWm, ruleAddr, 1) <= 0)
		{
			isprintf(gUserText, USER_TEXT_LEN, "Could not insert rule %d.", id);
			bpsec_admin_printText(gUserText);
		}
	}
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
 * TODO: Implement change option for policy rules.
 *****************************************************************************/
static void	executeChangeJson(jsonObject job)
{

	int start = 0;

	if (job.tokenCount < 2)
	{
		bpsec_admin_printText("Change what?");
		return;
	}

	if((start = bpsec_admin_json_getTypedIdx(job, 1, 0, "policyrule", JSMN_OBJECT)) > 0)
	{
		BpSecFilter filter;
		Lyst sci_parms = NULL;
		uint16_t id = 0;
		PsmAddress ruleAddr = 0;
		PsmAddress esAddr = 0;
		char name[MAX_EVENT_SET_NAME_LEN];

		memset(name, '\0', sizeof(name));

		if ((bpsec_admin_json_getRuleId(job, &id) > 0) &&
		    ((ruleAddr = bslpol_rule_get_addr(gWm, id)) > 0))
		{
			/* Delete existing rule */
			bslpol_rule_delete(gWm, ruleAddr);

			/* Create new rule with same ID as deleted rule */
			if (!bpsec_admin_json_parseFilter(job, &filter))
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
			else if((sci_parms = bpsec_admin_json_getSecCtxtParms(job)) == NULL)
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

/******************************************************************************
 * @brief Delete a named event set identified by the provided JSON object.
 *
 * @param[in]  job    - The parsed JSON tokens.
 *
 *****************************************************************************/

static void	bpsec_admin_deleteEventSet(jsonObject job)
{
	int start = 0;
	char name[MAX_EVENT_SET_NAME_LEN];
	memset(name, '\0', sizeof(name));

	if(bpsec_admin_json_getTypedValue(job, start, 0, JSMN_STRING, KNS_NAME, MAX_EVENT_SET_NAME_LEN, name, NULL) > 0)
	{
		if(bsles_delete(gWm, name) < 0)
		{
			isprintf(gUserText, USER_TEXT_LEN, "Error deleting eventset %s.", name);
			bpsec_admin_printText(gUserText);
		}
	}
	else
	{
		bpsec_admin_printText("Error deleting eventset.");
	}
	return;
}


/******************************************************************************
 * @brief Remove a security operation event from a named event set with both
 * identified by the provided JSON object.
 *
 * @param[in]  job    - The parsed JSON tokens.
 *
 *****************************************************************************/

static void	bpsec_admin_deleteEvent(jsonObject job)
{
	int start = 0;
	char name[MAX_EVENT_SET_NAME_LEN];
	memset(name, '\0', sizeof(name));

	if(bpsec_admin_json_getTypedValue(job, start, 0, JSMN_STRING, KNS_ES_REF,
			MAX_EVENT_SET_NAME_LEN, name, NULL) > 0)
	{
		BpSecEventId eventId = 0;
		if(bpsec_admin_json_getEventId(job, &eventId) > 0)
		{
			bslevt_delete(gWm, name, eventId);
		}
		else
		{
			isprintf(gUserText, USER_TEXT_LEN, "Error removing event %d from eventset %s.", eventId, name);
			bpsec_admin_printText(gUserText);
		}
	}
	else
	{
		bpsec_admin_printText("Missing event set name. Cannot delete event.");
	}
	return;
}

/******************************************************************************
 * @brief Remove a BPSec policy rule provided a JSON object.
 *
 * @param[in]  job    - The parsed JSON tokens.
 *
 *****************************************************************************/

static void	bpsec_admin_deletePolicyrule(jsonObject job)
{
	uint16_t id = 0;

	if(bpsec_admin_json_getRuleId(job, &id) > 0)
	{
		bslpol_rule_remove_by_id(gWm, id);
	}
	else
	{
		bpsec_admin_printText("Missing rule id. Cannot delete policy rule.");
	}
	return;
}

/******************************************************************************
 * @brief Prints an event object.
 *
 * @param[in] event - The event to be printed.
 *
 *****************************************************************************/

static void bpsec_admin_printEvent(BpSecEvent *event)
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

	bpsec_admin_printText(buf);
}


/******************************************************************************
 * @brief Prints an eventset name
 *
 * @param[in]  esPtr - The eventset whose name is to be printed.
 *
 *****************************************************************************/

static void bpsec_admin_printEventsetName(BpSecEventSet *esPtr)
{
	char buf[MAX_EVENT_SET_NAME_LEN + 500]; //Max 255 named event sets
	memset(buf, '\0', sizeof(buf));

	isprintf(buf, sizeof(buf), "\nEventset name: %s\n\tAssociated Policy Rules: %i",
			 esPtr->name, esPtr->ruleCount);

	bpsec_admin_printText(buf);
}


/******************************************************************************
 * @brief Prints an eventset description (optional field)
 *
 * @param[in]  esPtr - The eventset whose description is to be printed.
 *
 *****************************************************************************/

static void bpsec_admin_printEventsetDesc(BpSecEventSet *esPtr)
{
	char buf[MAX_EVENT_SET_DESC_LEN + 1]; 
	memset(buf, '\0', sizeof(buf));

	isprintf(buf, sizeof(buf), "\tDescription: %s\n", esPtr->desc);

	bpsec_admin_printText(buf);
}


/******************************************************************************
 * @brief Prints an eventset
 *
 * @param[in] esPtr - The eventset to be printed.
 *
 *****************************************************************************/

static void bpsec_admin_printEventset(BpSecEventSet *esPtr)
{
	PsmAddress elt = 0;

	bpsec_admin_printEventsetName(esPtr);
	bpsec_admin_printEventsetDesc(esPtr);

	/* Print each event configured for the event set */
	for(elt = sm_list_first(gWm, esPtr->events); elt; elt = sm_list_next(gWm, elt))
	{
		BpSecEvent *event = (BpSecEvent *) psp(gWm, sm_list_data(gWm,elt));
		bpsec_admin_printEvent(event);
	}
}


/******************************************************************************
 * @brief Prints a policyrule.
 *
 * @param[in] rulePtr - The policyrule to be printed.
 * @param[in] verbose - Whether to print the full rule (1) or not (0)
 *
 *****************************************************************************/

static void bpsec_admin_printPolicyrule(BpSecPolRule *rulePtr, int verbose)
{
	char buf[2048];
	char tmp[512];
	BpSecEventSet *esPtr = NULL;

	if(rulePtr == NULL)
	{
		bpsec_admin_printText("No Rule.\n");
		return;
	}

	memset(buf, '\0', sizeof(buf));

	isprintf(tmp, sizeof(tmp), "\nRule #%u - ", rulePtr->user_id);
	strcat(buf,tmp);

	isprintf(tmp, sizeof(tmp), "%s (Score: %d)", (strlen(rulePtr->desc) > 0) ? rulePtr->desc : "No Description", rulePtr->filter.score);
	strcat(buf,tmp);

	if(verbose == 0)
	{
		bpsec_admin_printText(buf);
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

	bpsec_admin_printText(buf);
}


/******************************************************************************
 * @brief Prints policy rules from a provided lyst.
 *
 * @param[in] rules   - The policyrules to be printed.
 * @param[in] verbose - Whether to print the full rule (1) or not (0)
 *
 *****************************************************************************/

static void bpsec_admin_printPolicyruleLyst(Lyst rules, int verbose)
{
	LystElt elt;

	/* lyst_length does a NULL check. */
	if(lyst_length(rules) <= 0)
	{
		bpsec_admin_printText("No policy rules defined.\n");
		return;
	}

	for(elt = lyst_first(rules); elt; elt = lyst_next(elt))
	{
		BpSecPolRule *rulePtr = (BpSecPolRule *) lyst_data(elt);
		bpsec_admin_printPolicyrule(rulePtr, verbose);
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

static int bpsec_admin_json_getFindCriteria(jsonObject job, int start, int *type, BpSecPolRuleSearchTag *tag)
{
	char tmp_str[JSON_VAL_LEN];
	int result = 0;
	int sc_id = 0;

	CHKZERO(tag);

	/* Search the JSON object for the search type and process it. */
	if(bpsec_admin_json_getTypedValue(job, 1, 0, JSMN_STRING, KNS_TYPE, JSON_VAL_LEN, tmp_str, NULL) <= 0)
	{
		bpsec_admin_printText("Search type missing. Include field \"type\" set to value \"all\" or \"best\".");
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
		isprintf(gUserText, USER_TEXT_LEN, "Unknown search type %s. Supported search types are \"all\" or \"best\".", tmp_str);
		bpsec_admin_printText(gUserText);

		return 0;
	}

	/* Search the JSON object for EIDs: bundle src, bundle dest, and security src. */
	tag->bsrc = bpsec_admin_json_allocStrValue(job, start, 0, KNS_SRC, MAX_EID_LEN, NULL);
	tag->bsrc_len = (tag->bsrc) ? istrlen(tag->bsrc, MAX_EID_LEN) : 0;

	tag->bdest = bpsec_admin_json_allocStrValue(job, start, 0, KNS_DEST, MAX_EID_LEN, NULL);
	tag->bdest_len = (tag->bdest) ? istrlen(tag->bdest, MAX_EID_LEN) : 0;

	tag->ssrc = bpsec_admin_json_allocStrValue(job, start, 0, KNS_SEC_SRC, MAX_EID_LEN, NULL);
	tag->ssrc_len = (tag->ssrc) ? istrlen(tag->ssrc, MAX_EID_LEN) : 0;

	/* Search for block type. A value of -1 indicates that the type was not provided. */
	result = bpsec_admin_json_getTypedValue(job, 1, 0, JSMN_PRIMITIVE, KNS_TGT, JSON_VAL_LEN, tmp_str, NULL);
	tag->type = (result > 0) ? atoi(tmp_str) : -1;

	/* Role can be a string or primitive value. */
	if((result = bpsec_admin_json_getTypedValue(job, 1, 0, JSMN_PRIMITIVE, KNS_ROLE, JSON_VAL_LEN, tmp_str, NULL)) <= 0)
	{
		result = bpsec_admin_json_getTypedValue(job, 1, 0, JSMN_STRING, KNS_ROLE, JSON_VAL_LEN, tmp_str, NULL);
	}
	/* If the security role was not provided, set to 0. */
	tag->role = (result > 0) ? bpsec_admin_getMappedValue(gRoleMap, tmp_str) : 0;

	/* Search for security context ID.  */
	/* A result of -1 means the sc_id in the JSON was invalid. */
	if((result = bpsec_admin_json_getScId(job, &sc_id)) == -1)
	{
		bpsec_admin_printText("Malformed security context identifier provided.");
		return 0;
	}
	/* A result of 1 means the SC ID is valid (supported by the BPA) */
	else if(result == 1){
		tag->scid = sc_id;
	}
	/* A value of 0 indicates that the sc_id was not provided. This is permitted behavior
	   - a find command does not have to specify an SC ID to be valid. */
	else {
		tag->scid = BPSEC_UNSUPPORTED_SC;
	}

	/* Search for an event set name. If none is found, set to NULL */
	tag->es_name = bpsec_admin_json_allocStrValue(job, start, 0, KNS_ES_REF, MAX_EVENT_SET_NAME_LEN, NULL);
	tag->es_name_len = (tag->es_name) ? istrlen(tag->es_name, MAX_EVENT_SET_NAME_LEN) : 0;

	/* Search for a security service. If none is found, set to 0 */
	if((result = bpsec_admin_json_getTypedValue(job, 1, 0, JSMN_STRING, KNS_SVC, JSON_VAL_LEN, tmp_str, NULL)) < 0)
	{
		bpsec_admin_printText("Malformed security service provided. Supported security services are: \n\tbib-integrity\n\tbcb-confidentiality");
		return 0;
	}
	else if (result > 0)
	{
		int svc = 0;
		if ((svc = bpsec_admin_getSvc(tmp_str)) < 0)
		{
			/* If the security service cannot be mapped to a supported service,
			 * the bpsec_admin_getSvc function will print a detailed error message. */
			return 0;
		}
		tag->svc = svc;
	}
	else
	{
		tag->svc = 0;
	}

	return 1;
}


/******************************************************************************
 * @brief Processes a find command given a JSON object.
 *
 * @param[in]  job   - The parsed JSON tokens.
 *****************************************************************************/

static void	bpsec_admin_findPolicyrule(jsonObject job)
{
	int start = 0;
	int type = 0;
	Lyst rules = NULL;
	BpSecPolRuleSearchTag tag;

	memset(&tag, 0, sizeof(tag));

	if(bpsec_admin_json_getFindCriteria(job, start, &type, &tag) <= 0)
	{
		bpsec_admin_printText("Unable to populate policy rule find criteria.");
		return;
	}

	switch(type)
	{
		case BPSEC_SEARCH_ALL:
			rules = bslpol_rule_get_all_match(gWm, tag);
			bpsec_admin_printPolicyruleLyst(rules, 1);
			lyst_destroy(rules);
			break;

		case BPSEC_SEARCH_BEST:
			bpsec_admin_printPolicyrule(bslpol_rule_find_best_match(gWm, tag), 1);
			break;
		default:
			bpsec_admin_printText("Unknown find type. Supported types are: \"best\" or \"all\".");
			break;
	}

	return;
}


/******************************************************************************
 * @brief Processes a information command for an event set given a JSON object.
 *
 * @param[in]  job   - The parsed JSON tokens.
 *****************************************************************************/

static void	bpsec_admin_infoEventSet(jsonObject job)
{
	int start = 0;

	char name[MAX_EVENT_SET_NAME_LEN];
	char desc[MAX_EVENT_SET_DESC_LEN];

	memset(name, '\0', sizeof(name));
	memset(desc, '\0', sizeof(desc));

	if(bpsec_admin_json_getTypedValue(job, start, 0, JSMN_STRING, KNS_NAME, MAX_EVENT_SET_NAME_LEN, name, NULL) > 0)
	{
		BpSecEventSet *esPtr = bsles_get_ptr(gWm, name);
		if(esPtr)
		{
			bpsec_admin_printEventset(esPtr);
		}
		else
		{
			isprintf(gUserText, USER_TEXT_LEN, "No info for unknown event set %s.", name);
			bpsec_admin_printText(gUserText);
		}
	}
	else
	{
		bpsec_admin_printText("Error displaying eventset info.");
	}

	return;
}

/******************************************************************************
 * @brief Processes a information command for a policy rule given a JSON object.
 *
 * @param[in]  job  - The parsed JSON tokens.
 *****************************************************************************/

static void	bpsec_admin_infoPolicyrule(jsonObject job)
{
	uint16_t id = 0;

	if(bpsec_admin_json_getRuleId(job, &id) > 0)
	{
		bpsec_admin_printPolicyrule(bslpol_rule_get_ptr(gWm, id), 1);
	}
	else
	{
		bpsec_admin_printText("Missing policy rule id from info command.");
	}
	return;
}


/******************************************************************************
 * @brief Processes an event set list command given a JSON parm object
 *
 * @param[in]  job   - The parsed JSON tokens.
 *****************************************************************************/

static void	bpsec_admin_listEventSet(jsonObject job)
{
	char name[MAX_EVENT_SET_NAME_LEN];
	char desc[MAX_EVENT_SET_DESC_LEN];

	memset(name, '\0', sizeof(name));
	memset(desc, '\0', sizeof(desc));

	Lyst eventsets = bsles_get_all(getIonwm());

	if (eventsets != NULL)
	{
		LystElt elt;

		/* For each element in the lyst (event set), print the eventset name */
		for(elt = lyst_first(eventsets); elt; elt = lyst_next(elt))
		{
			BpSecEventSet *esPtr = (BpSecEventSet *) lyst_data(elt);
			bpsec_admin_printEventsetName(esPtr);
			bpsec_admin_printEventsetDesc(esPtr);
		}
		lyst_destroy(eventsets);
	}

	return;
}


/******************************************************************************
 * @brief Processes a policyrule list command given a JSON object.
 *
 * @param[in]  job   - The parsed JSON tokens.
 *****************************************************************************/

static void	bpsec_admin_listPolicyrule(jsonObject job)
{
	PsmAddress elt = 0;

	/* Print each event configured for the event set */
	for(elt = sm_list_first(gWm, getSecVdb()->bpsecPolicyRules); elt; elt = sm_list_next(gWm, elt))
	{
		BpSecPolRule *rule = (BpSecPolRule *) psp(gWm, sm_list_data(gWm,elt));
		bpsec_admin_printPolicyrule(rule, 0);
	}
	return;
}


/******************************************************************************
 * @brief Determines the type of security policy command given. The security
 *        policy command is uniquely identified using two pieces of
 *        information:
 *        	1) The command code identifying the action to take ('a' for add,
 *        		'd' for delete, etc.).
 *        	2) The command type identifier, policyrule, event_set, or event,
 *        	    indicating the policy structure to use with the command.
 *
 * @param[in] cmdCode - Single character action identifier for the command.
 * @param[in] job     - JSON object holding parsed command tokens.
 *
 * @retval   SecPolCmd enum
 *****************************************************************************/

SecPolCmd bpsec_admin_json_getSecPolCmd(char *cmdCode, jsonObject job)
{
	CHKERR(cmdCode != NULL);

	/* Step 1: Get the key at the first level of the JSON object, which is the
	 * command type identifier. */
	int cmdTypeIdx = bpsec_admin_json_getNextKeyAtLevel(job, -1, 0);

	if(cmdTypeIdx < 0)
	{
		bpsec_admin_printText("Malformed security policy command. \n"
				"Hint: Supported command types are: \n"
				"\t \"event_set\" \n\t \"event\" \n\t \"policyrule\"\n");
		return invalid;
	}

	char cmdType[JSON_KEY_LEN];
	memset(cmdType, '\0', sizeof(cmdType));

	/* Step 1.1: The command type identifier must be a string. */
	if(bpsec_admin_json_getValueAtIdx(job, JSMN_STRING, cmdTypeIdx, cmdType, sizeof(cmdType)) <= 0)
	{
		bpsec_admin_printText("Malformed security policy command. Command type must be a string.\n"
				"Hint: Supported command types are: \n"
				"\t \"event_set\" \n\t \"event\" \n\t \"policyrule\"\n");
		return invalid;
	}

	/* Step 2: Determine supported command type identifiers for the
	 * command code provided. */
	switch (cmdCode[0])
	{
		/* Add command */
		case 'a':
			if(strcmp(cmdType, KNS_POLICYRULE) == 0)
			{
				return add_policyrule;
			}
			else if(strcmp(cmdType, KNS_EVENT_SET) == 0)
			{
				return add_event_set;
			}
			else if(strcmp(cmdType, KNS_EVENT) == 0)
			{
				return add_event;
			}
			else
			{
				bpsec_admin_printText("Malformed security policy add command. \n"
						"Supported command types are: \n"
						"\t \"event_set\" \t \"event\" \t \"policyrule\"");
				return invalid;
			}
		/* Delete command */
		case 'd':
			if(strcmp(cmdType, KNS_POLICYRULE) == 0)
			{
				return delete_policyrule;
			}
			else if(strcmp(cmdType, KNS_EVENT_SET) == 0)
			{
				return delete_event_set;
			}
			else if(strcmp(cmdType, KNS_EVENT) == 0)
			{
				return delete_event;
			}
			else
			{
				bpsec_admin_printText("Malformed security policy delete command. \n"
						"Supported command types are: \n"
						"\t \"event_set\" \t \"event\" \t \"policyrule\"");
				return invalid;
			}
		/* Find command */
		case 'f':
			if(strcmp(cmdType, KNS_POLICYRULE) == 0)
			{
				return find_policyrule;
			}
			else
			{
				bpsec_admin_printText("Malformed security policy find command. \n"
						"Supported command types are: \n"
						"\t \"policyrule\"");
				return invalid;
			}
		/* Info command */
		case 'i':
			if(strcmp(cmdType, KNS_POLICYRULE) == 0)
			{
				return info_policyrule;
			}
			else if(strcmp(cmdType, KNS_EVENT_SET) == 0)
			{
				return info_event_set;
			}
			else
			{
				bpsec_admin_printText("Malformed security policy info command. \n"
						"Supported command types are: \n"
						"\t \"event_set\" \t \"policyrule\"");
				return invalid;
			}
		/* List command */
		case 'l':
			if(strcmp(cmdType, KNS_POLICYRULE) == 0)
			{
				return list_policyrule;
			}
			else if(strcmp(cmdType, KNS_EVENT_SET) == 0)
			{
				return list_event_set;
			}
			else
			{
				bpsec_admin_printText("Malformed security policy list command. \n"
						"Supported command types are: \n"
						"\t \"event_set\" \t \"policyrule\"");
				return invalid;
			}
	}

	bpsec_admin_printText("Malformed security policy command. \nCheck security policy user's manual for more information.");
	return invalid;
}

/******************************************************************************
 * @brief Checks the structure of a security policy JSON command. A valid
 *        security policy command must have:
 *    	  1. At least three JSON tokens.
 *        2. A command body typed as a JSON object.
 *        3. A command type (policyrule, event_set, event) typed as a string.
 *
 * @param[in]  job  - The JSON command to be validated, composed of parsed
 *                    JSON tokens
 *
 * @retval -1 - Error
 * @retval	0 - JSON command structure is incorrect.
 * @retval	1 - JSON command structure is correct.
 *****************************************************************************/

int bpsec_admin_json_checkCmd(jsonObject job)
{
	/* Step 1: Check that there are enough tokens for the policy command */
	if (job.tokenCount < 2)
	{
		return -1;
	}

	/* Step 2: Check that the command contents were parsed correctly, with
	 * 		- the root object typed as a JSON object
	 * 		- the second token, the command type, as a string */
	if(job.tokens[0].type == JSMN_OBJECT && job.tokens[1].type == JSMN_STRING)
	{
		return 1;
	}

	return -1;
}

/******************************************************************************
 * @brief Parses the provided string using jsmn to create a JSON object (JOb).
 *
 * @param[in]      json - The JSON string to parse.
 * @param[in|out]  job  - The JSON object to hold the parsed JSON tokens.
 *
 * @retval -1 - Error
 * @retval	1 - JSON commands parsed successfully.
 *****************************************************************************/

int bpsec_admin_json_parseJob(char *json, jsonObject *job)
{
	/* Step 0: Sanity checks */
	CHKERR(json != NULL);

	jsmn_parser p;

	/* Step 0: Sanity checks and jsmn parser initialization */
	jsmn_init(&p);

	/* Step 1: Parse the JSON command with jsmn */
	job->line = json;
	job->tokenCount = jsmn_parse(&p, job->line, strlen(job->line), job->tokens,
				 sizeof(job->tokens) / sizeof(job->tokens[0]));

	/* Step 2: Check that parsing was successful. If the jsmn token count
	 * is less than 0, processing was unsuccessful. If the token count exceeds
	 * the maximum number of permitted tokens, the JSON command is not
	 * processed/executed */
	if (job->tokenCount < 0 || job->tokenCount > MAX_JSMN_TOKENS)
	{
		return -1;
	}

	return 1;
}


/******************************************************************************
 * @brief Retrieves a JSON command from a script read by the bpsecadmin
 *        utility. This function may be called multiple times for the
 *        same JSON command if it is composed from multiple lines in the
 *        script. This function processes a single line of the script at a time.
 *
 * @param[in]     line    - Line of JSON from the script being processed.
 * @param[in|out] jsonStr - The concatenated JSON command.
 *
 * @note
 * This is a helper function for parsing JSON objects. It is expected that the
 * JSON string (jsonStr) argument is initialized (empty) when the function is
 * called for the first line of the command, and that the same variable is used
 * to accumulate the JSON command if it spans multiple lines in the script
 * being processed.
 *
 * @retval -1 - Error
 * @retval	0 - JSON command is not valid - may be incomplete.
 * @retval	1 - JSON command has been retrieved and is valid.
 *****************************************************************************/

int bpsec_admin_json_getCmd(char *line, char *jsonStr)
{
	/* Step 0: Sanity checks. */
	CHKERR(line != NULL);

	jsonObject  job;

	/* Step 1: Strip any leading whitespace from the line */
	while (isspace((int) *line))
	{
		line++;
	}

	/* Step 2: Check the length of the potential concatenated string. If the
	 * command length would be longer than jsonStr can accommodate (JSON_CMD_LEN)
	 * return an error. */
	if((strlen(line) + strlen(jsonStr)) > JSON_CMD_LEN)
	{
		isprintf(gUserText, USER_TEXT_LEN, "Security policy command exceeds permitted length %d.", JSON_CMD_LEN);
		bpsec_admin_printText(gUserText);
		return -1;
	}

	/* Step 3: Add the contents of the new line to the JSON command string */
	strcat(jsonStr, line);

	/* Step 4: Parse the JSON command string using jsmn to check syntax. Start at
	 * jsonStr+1 to skip the command code, which is executed in processJson() but
	 * is not considered to be part of the JSON command body. */
	if (bpsec_admin_json_parseJob(jsonStr+1, &job))
	{
		/* Step 5: JSON syntax is valid. Determine if the command structure
		 * is valid, containing a primitive for command code and JSON object
		 * for command content */
		if(bpsec_admin_json_checkCmd(job) == 1)
		{
			return 1;
		}

		/* Otherwise, valid JSON was provided but it does not fit the
		 * necessary format for security policy commands. */
		return 0;
	}

	/* JSON is not valid. The retrieval of the next line in the script may
	 * be necessary to form a complete command. */
	return 0;
}


/******************************************************************************
 * @brief Set a flag in the 32-bit mask for security policy command keys. The
 *        flag to be set is identified by the string value passed as the key.
 *
 * @param[in]     key     - String key from the security policy command that
 *                          indicates which flag should be set.
 * @param[in|out] keyMask - Mask with bits set for each of the keys found in
 *                          the security policy command
 *
 * @retval   0 - Key was not found. Bit was not set.
 * @retval   1 - Key was found. Bit was set.
 *****************************************************************************/

int bpsec_admin_setKeyFlag(char *key, uint32_t *keyMask)
{
  int value = 0;

  /* Find the action value in the map of action values. */
  if((value = bpsec_admin_getMappedValue(gKeyWords, key)) > 0)
  {
	  *keyMask |= value;
  }
  else
  {
	  isprintf(gUserText, USER_TEXT_LEN, "Unknown key in security policy command %s. \nCheck security policy user's manual for supported keys.", key);
	  bpsec_admin_printText(gUserText);
  }

  return value;
}


/******************************************************************************
 * @brief Check a security policy command's keys. This check ensures that all
 * 		  mandatory key fields are present in the command and all other keys
 * 		  that are present are supported as optional for that command.
 *
 * @param[in] cmdId      - Security policy command ID
 * @param[in] cmdKeyMask - Mask with bits set for each of the keys found in
 *                         the security policy command
 *
 * @retval   0 - Command keys are invalid.
 * @retval   1 - Command keys are valid.
 *****************************************************************************/

int bpsec_admin_json_checkKeys(SecPolCmd cmdId, uint32_t cmdKeyMask)
{
	CHKERR(cmdId);

	switch (cmdId)
	{
		case add_event_set:
			if((HAS_MANDATORY_KEYS(cmdKeyMask, MAND_ES_ADD_KEYS)) &&
				!(HAS_INVALID_KEYS(cmdKeyMask, MAND_ES_ADD_KEYS, OPT_ES_ADD_KEYS)))
			{
				return 1;
			}
			return 0;
		case delete_event_set:
			if((HAS_MANDATORY_KEYS(cmdKeyMask,MAND_ES_DEL_KEYS)) &&
				!(HAS_INVALID_KEYS(cmdKeyMask, MAND_ES_DEL_KEYS, 0)))
			{
				return 1;
			}
			return 0;
		case info_event_set:
			if((HAS_MANDATORY_KEYS(cmdKeyMask,MAND_ES_INFO_KEYS)) &&
				!(HAS_INVALID_KEYS(cmdKeyMask, MAND_ES_INFO_KEYS, 0)))
			{
				return 1;
			}
			return 0;
		case list_event_set:
			if((HAS_MANDATORY_KEYS(cmdKeyMask,MAND_ES_LIST_KEYS)) &&
				!(HAS_INVALID_KEYS(cmdKeyMask, MAND_ES_LIST_KEYS, 0)))
			{
				return 1;
			}
			return 0;
		case add_event:
			if((HAS_MANDATORY_KEYS(cmdKeyMask,MAND_EVENT_ADD_KEYS)) &&
				!(HAS_INVALID_KEYS(cmdKeyMask, MAND_EVENT_ADD_KEYS, 0)))
			{
				return 1;
			}
			return 0;
		case delete_event:
			if((HAS_MANDATORY_KEYS(cmdKeyMask,MAND_EVENT_DEL_KEYS)) &&
				!(HAS_INVALID_KEYS(cmdKeyMask, MAND_EVENT_DEL_KEYS, 0)))
			{
				return 1;
			}
			return 0;
		case add_policyrule:
			if((HAS_MANDATORY_KEYS(cmdKeyMask,MAND_RULE_ADD_KEYS)) &&
				!(HAS_INVALID_KEYS(cmdKeyMask, MAND_RULE_ADD_KEYS, OPT_RULE_ADD_KEYS)))
			{
				return 1;
			}
			return 0;
		case delete_policyrule:
			if((HAS_MANDATORY_KEYS(cmdKeyMask,MAND_RULE_DEL_KEYS)) &&
				!(HAS_INVALID_KEYS(cmdKeyMask, MAND_RULE_DEL_KEYS, 00)))
			{
				return 1;
			}
			return 0;
		case info_policyrule:
			if((HAS_MANDATORY_KEYS(cmdKeyMask,MAND_RULE_INFO_KEYS)) &&
				!(HAS_INVALID_KEYS(cmdKeyMask, MAND_RULE_INFO_KEYS, 0)))
			{
				return 1;
			}
			return 0;
		case find_policyrule:
			if((HAS_MANDATORY_KEYS(cmdKeyMask, MAND_RULE_FIND_KEYS)) &&
				!(HAS_INVALID_KEYS(cmdKeyMask, MAND_RULE_FIND_KEYS, OPT_RULE_FIND_KEYS)))
			{
				return 1;
			}
			return 0;
		case list_policyrule:
			if((HAS_MANDATORY_KEYS(cmdKeyMask, MAND_RULE_LIST_KEYS)) &&
				!(HAS_INVALID_KEYS(cmdKeyMask, MAND_RULE_LIST_KEYS, 0)))
			{
				return 1;
			}
			return 0;
		default:
			return 0;
	}
	return -1;
}

/******************************************************************************
 * @brief Process a JSON security policy command to validate the keys.
 *
 * @param[in] cmdType - The security policy command type.
 * @param[in]     job - The JSON object holding the parsed JSON tokens.
 *
 * @retval <=0 - Command is invalid.
 * @retval  >0 - Command is valid and can be executed.
 *****************************************************************************/

int bpsec_admin_json_validateCmd(SecPolCmd cmdType, jsonObject job){

	int keyIdx = bpsec_admin_json_getNextKeyAtLevel(job, -1, 0);
	int valIdx = 0;
	uint32_t cmdKeyMask = 0;
	char key[JSON_KEY_LEN];
	char val[JSON_VAL_LEN];

	memset(key, '\0', sizeof(key));
	memset(val, '\0', sizeof(val));

	/* Step 1: Get and skip the command type index.
	 * 		Supported command types are:
	 * 			event_set
	 * 			event
	 * 			policyrule                       */
	keyIdx = bpsec_admin_json_getNextKeyAtLevel(job, keyIdx, 2);

	/* Step 2: Retrieve all keys at level 2 of the JSMN tokens.
	 * 		   For each of the level two keys: */
	while(keyIdx > 0)
	{
		/* Step 2.1: Retrieve key field. All keys must be strings */
		if(bpsec_admin_json_getValueAtIdx(job, JSMN_STRING, keyIdx, key, sizeof(key)) <= 0)
		{
			bpsec_admin_printText("Malformed key field provided. All keys must be strings.");
			return -1;
		}

		/* Step 2.2: Set the flag for the key found */
		if(bpsec_admin_setKeyFlag(key, &cmdKeyMask) <= 0)
		{
			/* Step 2.2.1 If the key is not recognized, error has occurred.
			 * The error message is printed to user above in bpsec_admin_setKeyFlag */
			return -1;
		}

		/* Step 2.3 If the key's value is a string,
		 * check that it is not a keyword */
		if((valIdx = bpsec_admin_json_getValueIdx(job, keyIdx)) >= 0)
		{
			if(job.tokens[valIdx].type == JSMN_STRING)
			{
				/* Step 2.3.1 Retrieve the value */
				if(bpsec_admin_json_getValueAtIdx(job, JSMN_STRING, valIdx, val, sizeof(val)) <= 0)
				{
					isprintf(gUserText, USER_TEXT_LEN, "Malformed value in security policy command for key %s.", key);
					bpsec_admin_printText(gUserText);
					return -1;
				}

				/* Step 2.3.2 Check that the value is not a reserved key word */
				if((bpsec_admin_getMappedValue(gKeyWords, val)) != 0)
				{
					isprintf(gUserText, USER_TEXT_LEN, "Value in security policy command is reserved keyword %s.", val);
					bpsec_admin_printText(gUserText);
					return -1;
				}

				memset(val, '\0', sizeof(val));
			}
		}
		/* Step 2.3.3 If there is not a value associated with the key, the security
		 * policy command is malformed. */
		else
		{
			isprintf(gUserText, USER_TEXT_LEN, "Security policy command missing value for key %s.", key);
			bpsec_admin_printText(gUserText);
			return -1;
		}

		/* Step 2.4 Get the next level 2 key */
		keyIdx = bpsec_admin_json_getNextKeyAtLevel(job, keyIdx, 2);
	}

	/* Step 3: With the fully populated mask of keys found in the
	 * command, check that all mandatory keys are present and all
	 * other keys are approved as optional keys for that command */
	if(bpsec_admin_json_checkKeys(cmdType, cmdKeyMask) != 1)
	{
		char invalid_cmd[JSON_CMD_LEN];
		memset(invalid_cmd, '\0', sizeof(invalid_cmd));

		if(bpsec_admin_json_getValueAtIdx(job, JSMN_OBJECT, 0, invalid_cmd, sizeof(invalid_cmd)) > 0)
		{
			isprintf(gUserText, USER_TEXT_LEN, "Malformed security policy command: \n\t %s \n Invalid key field provided.", invalid_cmd);
			bpsec_admin_printText(gUserText);
			return -1;
		}
		else
		{
			bpsec_admin_printText("Malformed security policy command: Invalid key field provided.");
			return -1;
		}
	}

	/* All keys provided in the security policy command are correct */
	return 1;
}


/******************************************************************************
 * @brief Process and execute a bpsecadmin command
 *
 * @param[in] line - The command to be executed, as JSON or a single character.
 *
 * @retval -1 - Error
 * @retval	0 - Command could not be executed.
 * @retval	1 - Command successfully executed.
 *****************************************************************************/

int bpsec_admin_executeCmd(char *line)
{
	/* Step 0: Sanity checks. */
	CHKERR(line != NULL);

	char		buffer[80];
	jsonObject  job;
	char        cmdCode[1];

	/* Step 1: Retrieve the command code (char) from the policy command */

	memset(cmdCode, '\0', sizeof(cmdCode));

	/* Step 1.1: Strip any leading whitespace */
	while (isspace((int) *line))
	{
		line++;
	}

	/* Step 1.2: Check for case where the command code has been omitted
	 * from the security policy command. In this case, the first char would
	 * be '{' indicating the start of the JSON command. */
	if(strchr("{", line[0]) != NULL)
	{
		bpsec_admin_printText("Security policy command missing command code.");
		bpsec_admin_printText("Hint: 'a', 'd', 'f', etc.\nEnter 'h' for more information.");
		return -1;
	}

	/* Step 1.3: Populate the command code field (length is 1 + 1)*/
	if(istrcpy(cmdCode, line, 2) == NULL)
	{
		bpsec_admin_printText("Security policy command code malformed.");
		bpsec_admin_printText("Hint: 'a', 'd', 'f', etc.\nEnter 'h' for more information.");
		return -1;
	}

	/* Step 1.4: Advance cursor past command code, now pointing to
	 * JSON command contents */
	line++;

	/* Step 1.5: Strip any remaining whitespace */
	while (isspace((int) *line))
	{
		line++;
	}

	/* Step 2: Execute any single char commands ('h', 'q', etc.) */
	switch (cmdCode[0])
	{
		case 0:			/*	Empty line.		*/
		case '#':		/*	Comment.		*/
			return 0;
		case '?':		/*  Help command.   */
		case 'h':
			bpsec_admin_printUsage();
			return 0;
		case '1':		/*  Init command    */
			if(bpsec_admin_init() < 0)
			{
				return -1;
			}
			return 0;
		case 'v':		/*  Version command. */
			isprintf(buffer, sizeof(buffer), "%s", IONVERSIONNUMBER);
			bpsec_admin_printText(buffer);
			return 0;
		case 'q':       /* Quit command.     */
			return 0;
	}

	/* Step 3: If the command code provided requires additional data, it is
	 * provided as JSON. Parse the JSON command with jsmn. If the cursor is
	 * at the end of the command, there is no JSON to parse and the command code
	 * itself is the only portion of the policy command to process
	 * (Ex: user may have entered 'q') */
	if(*line != '\0')
	{
		if(bpsec_admin_json_parseJob(line, &job) < 1)
		{
			bpsec_admin_printText("Malformed security policy command detected. Failed to parse JSON.");
			return -1;
		}
	}
	else
	{
		bpsec_admin_printText("Malformed security policy command. \n "
				"Hint: Enter 'h' to see supported commands.");
		return -1;
	}

	/* Step 3.1: Verify that the provided JSON matches the expected
	 * structure for a security policy command. */
	if(bpsec_admin_json_checkCmd(job) < 1)
	{
		bpsec_admin_printText("Malformed security policy command. \n"
				"Hint: Supported command types are: \n"
				"\t \"event_set\" \n\t \"event\" \n\t \"policyrule\"");
		return 0;
	}

	/* Step 3.2: Retrieve the security policy command type */
	SecPolCmd cmd_id = bpsec_admin_json_getSecPolCmd(cmdCode, job);

	if(cmd_id == invalid)
	{
		isprintf(buffer, sizeof(buffer), "Malformed security policy command code %c.\nHint: Enter 'h' to see supported commands.", cmdCode[0]);
		bpsec_admin_printText(buffer);
		return 0;
	}

	/* Step 3.3: Validate all command key fields */
	if(bpsec_admin_json_validateCmd(cmd_id, job) < 1)
	{
		/* Detailed error messages printed to user when validating key fields */
		return 0;
	}

	/* Step 3.4: Attach to the security database */
	if(bpsec_admin_attach(0) != 1)
	{
		/* Detailed error message printed to user in attach() */
		return 0;
	}

	/* Step 4: Execute security policy command */
	switch (cmd_id)
	{
	case add_event_set:
	{
		bpsec_admin_addEventSet(job);
		return 0;
	}
	case delete_event_set:
	{
		bpsec_admin_deleteEventSet(job);
		return 0;
	}
	case info_event_set:
	{
		bpsec_admin_infoEventSet(job);
		return 0;
	}
	case list_event_set:
	{
		bpsec_admin_listEventSet(job);
		return 0;
	}
	case add_event:
	{
		bpsec_admin_addEvent(job);
		return 0;
	}
	case delete_event:
	{
		bpsec_admin_deleteEvent(job);
		return 0;
	}
	case add_policyrule:
	{
		bpsec_admin_addPolicyrule(job);
		return 0;
	}
	case delete_policyrule:
	{
		bpsec_admin_deletePolicyrule(job);
		return 0;
	}
	case info_policyrule:
	{
		bpsec_admin_infoPolicyrule(job);
		return 0;
	}
	case find_policyrule:
	{
		bpsec_admin_findPolicyrule(job);
		return 0;
	}
	case list_policyrule:
	{
		bpsec_admin_listPolicyrule(job);
		return 0;
	}
	default:
		bpsec_admin_printText("Invalid command.  Enter '?' or 'h' for help.");
		return 0;
	}

	return 0;
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
	char	line[USER_TEXT_LEN];
	char 	jsonStr[JSON_CMD_LEN];

	memset(jsonStr, '\0', sizeof(jsonStr));
	memset(line, '\0', sizeof(line));

	if (cmdFileName == NULL)		/*	Interactive.	*/
	{
#ifdef FSWLOGGER
		return 0;			/*	No stdout.	*/
#else
		cmdFile = fileno(stdin);
		isignal(SIGINT, bpsec_admin_handleQuit);

        if(bpsec_admin_init() <= 0)
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

			bpsec_admin_executeCmd(line);
		}
#endif
	}
	else  /*	Scripted.	*/
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
				if (igets(cmdFile, line, sizeof(line), &len) == NULL)
				{
					if (len == 0)
					{

						/* If an incomplete security policy command (held
						 * in jsonStr) exists when we reach EOF, that command
						 * is invalid */
						if(strlen(jsonStr) != 0)
						{
							bpsec_admin_printText("Malformed security policy command detected. JSON incomplete.");
						}
						break;	/*	Loop.	*/
					}

					putErrmsg("igets failed.", NULL);
					break;		/*	Loop.	*/
				}

				/* If the line is empty or a comment, ignore */
				if (len == 0 || line[0] == '#')
				{
					continue;
				}

				/* If the line is a single, supported command code,
				 * process that command to execute it. */
				else if (strchr(singleCmdCodes, line[0]) != NULL)
				{
					bpsec_admin_executeCmd(line);
				}

				/* Otherwise, the line is *added* to the JSON command. The
				 * line may contain a complete security policy command, or
				 * that command may span several lines in the file.  */
				else
				{
					/* Pass the line to the JSON command accumulator. */
					int retval = bpsec_admin_json_getCmd(line, jsonStr);

					/* If this line is a complete security policy command,
					 * process that command. */
					if (retval == 1)
					{
						bpsec_admin_executeCmd(jsonStr);

						/* Clear the accumulated JSON string to begin
						 * populating the next command. */
						memset(jsonStr, '\0', sizeof(jsonStr));
					}
					else if (retval == -1)
					{
						bpsec_admin_printText("Invalid JSON detected.");
						memset(jsonStr, '\0', sizeof(jsonStr));
					}
					/* Otherwise, add the next line to the concatenated JSON command
					 * in jsonStr. */
					else
					{
						continue;
					}
				}
			}
			close(cmdFile);
		}
	}

	writeErrmsgMemos();
	bpsec_admin_printText("Stopping bpsecadmin.");
	ionDetach();
	return 0;
}
