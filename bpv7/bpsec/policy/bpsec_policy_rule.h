/*****************************************************************************
 **
 ** File Name: bpsec_policy_rule.h
 **
 ** Description:
 **
 ** Notes:
 **
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/YYYY  AUTHOR         DESCRIPTION
 **  -------  ------------   ---------------------------------------------
 **  12/2020  E. Birrane     Initial implementation
 **
 *****************************************************************************/

#ifndef BPSEC_POLICY_RULE_H_
#define BPSEC_POLICY_RULE_H_

/*****************************************************************************
 *                              FILE INCLUSIONS                              *
 *****************************************************************************/

#include "bpsec_asb.h"
#include "bpsec_util.h"
#include "bpsec_policy.h"
#include "bpsec_policy_eventset.h"
#include "csi.h"
#include "sci.h"

/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  |
 * +--------------------------------------------------------------------------+
 */

#define BPSEC_RULE_DESCR_LEN (64)
#define BPSEC_RULE_SCORE_FULL (2)
#define BPSEC_RULE_SCORE_PARTIAL (1)

#define BPSEC_RULFLG_ANON_ES     (1)

#define BPRF_SRC_ROLE (0x01)
#define BPRF_VER_ROLE (0x02)
#define BPRF_ACC_ROLE (0x04)
#define BPRF_USE_BSRC (0x08)
#define BPRF_USE_BDST (0x10)
#define BPRF_USE_SSRC (0x20)
#define BPRF_USE_BTYP (0x40)
#define BPRF_USE_SCID (0x80)

#define BPSEC_MAX_NUM_RULES (255)

/*
 * +--------------------------------------------------------------------------+
 * |							  	MACROS  								  |
 * +--------------------------------------------------------------------------+
 */

#define BPSEC_RULE_ANON_ES(rule) (rule->flags & BPSEC_RULFLG_ANON_ES)

#define BPSEC_RULE_ROLE_IDX(rule) (rule->filter.flags & (BPRF_SRC_ROLE | BPRF_VER_ROLE | BPRF_ACC_ROLE))

#define BPSEC_RULE_BSRC_IDX(rule) (rule->filter.flags & BPRF_USE_BSRC)
#define BPSEC_RULE_BDST_IDX(rule) (rule->filter.flags & BPRF_USE_BDST)
#define BPSEC_RULE_SSRC_IDX(rule) (rule->filter.flags & BPRF_USE_SSRC)
#define BPSEC_RULE_BTYP_IDX(rule) (rule->filter.flags & BPRF_USE_BTYP)
#define BPSEC_RULE_SCID_IDX(rule) (rule->filter.flags & BPRF_USE_SCID)


#define BSLPOL_RULES() ((Lyst) psp(getIonwm(), getSecVdb()->bpsecPolicyRules))
#define BSLPOL_RULEIDX_BSRC() ((RadixTree) psp(getIonwm(), getSecVdb()->bpsecRuleIdxBySrc))
#define BSLPOL_RULEIDX_DEST() ((RadixTree) psp(getIonwm(), getSecVdb()->bpsecRuleIdxByDest))
#define BSLPOL_RULEIDX_SSRC() ((RadixTree) psp(getIonwm(), getSecVdb()->bpsecRuleIdxBySSrc))
#define BSLPOL_EID_DICT() ((RadixTree) psp(getIonwm(), getSecVdb()->bpsecEidDictionary))

/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */


/**
 * BPSec Policy Filters
 *
 * EID filters are a direct string match from the string representation of an EID
 *
 * To limit the overall size of strings in the system, these are pointers into a
 * structure holding EID strings.
 *
 * Flags: which items and what role;
 *
 * | BIT # |  Meaning if Set
 * +-------+------------------------------------------------------
 * |   0   | Rule is for a security source only
 * |   1   | Rule is for a security verifier only
 * |   2   | Rule is for a security acceptor only
 * |   3   | Bundle Source is part of the filter criteria
 * |   4   | Bundle Destination is part of the filter criteria
 * |   5   | Security Source is part of the filer criteria
 * |   6   | Block type is part of the filter criteria
 * |   7   | SC Id is part of the filter criteria (only for v/a roles)
 * +-------+-------------------------------------------------------
 *
 * EIDs: The EIDs used in this filter are pointers into a global dictionary of
 *       EIDs.  This dictionary is used to reduce the overall in-memory space
 *       associated with the BPSEC policy implementation.
 *
 * Lengths: The length of each string is stored to reduce the overall number of
 *          strlen calls used when processing security policy rules.
 *
 * Score: The specificity score of a filter. BPSec matches a single rule to a
 *        security block. The rule whose filter is most specific is chosen as
 *        the rule to be applied. This value is used to calculate the specificity
 *        score of a filter. This is a function of the number of items included
 *        in the filter and whether those items include prefix wildcards.
 *
 */
typedef struct
{
	uint8_t    flags;       /**< Information about how to use this filter. */

	PsmAddress bundle_src;  /**< EID Reference. (char *) */
	PsmAddress bundle_dest; /**< EID Reference. (char *) */
	PsmAddress sec_src;     /**< EID Reference. (char *) */

	uint16_t   bsrc_len;    /**< Length of bundle source EID.      */
	uint16_t   bdest_len;   /**< Length of bundle destination EID. */
	uint16_t   ssrc_len;    /**< Length of security source EID.    */

	uint8_t	   blk_type;    /**< Applicable block type         */
	int        scid;        /**< Required Security Context ID  */
	uint8_t	   score;       /**< Specificity score of the rule */
	uint8_t    svc;			/**< Security service			   */
} BpSecFilter;



/**
 * BPSec Security Context Configuration
 *
 * Each BPSec policy rule supports a series of zero or more security context
 * configurations that may be applied to various security contexts.
 *
 * This structure is currently hard-coded to the existing ION BPSec supported
 * parameters, which are very sparse.
 *
 * TODO: Update this structure to support a more generic interplay between
 *       security contexts and policy rules. User input from, for example, a
 *       security input file should be given to a security-context parameter
 *       parser which would create an appropriately-encoded version of the
 *       parameters for storage in the rule  for runtime.
 */

typedef struct
{
	uint16_t id;
	uint16_t length;
	PsmAddress addr;
} BpSecCtxParm;



/**
 * BPSec Policy Rules
 *
 * Policy in BPSec is specified as a series of "policy rules" which matches
 * security service actions to extension block lifecycle events.
 *
 * Policy rules have four components:
 *
 * 1. Identifying information for the rule.
 * 2. Filtering material to match the rule to extension blocks.
 * 3. Security context identification and configuration
 * 4. Identification of which security life-cycle events apply for this rule
 *
 * NOTE:
 *   - The encapsulated event set describes the notable events and actions
 *     used by this rule. This references a structure that exists either:
 *        (a) As a named, sharable event set managed in a global event set DB
 *        (b) As an anonymous rule-locked event set managed by this rule.
 *     The type of event set is identified by the rule flags.
 */
typedef struct
{
	char        desc[BPSEC_RULE_DESCR_LEN+1]; /**< User Descriptions.>  */
	uint8_t     idx;      /**< Index of rule in its vector storage.     */
	uint16_t    user_id;  /**< Unique User-supplied rule ID.            */
	uint8_t     flags;    /**< Administrative information for the rule. */
	BpSecFilter filter;   /**< Filter structure for matching to blocks. */
	PsmAddress  sc_parms; /**< SmList of BpSecCtxParm.                  */
	PsmAddress  eventSet; /**< Actionable events (BpSecPolEventSet *)   */
} BpSecPolRule;



/**
 * BPSec Policy Rule Radix Tree Entry
 *
 * The BPSec Policy Engine maintains multiple radix trees used to index lists
 * of rules matching certain criteria. Each node in a radix tree is represented by
 * a "rule entry" structure.
 *
 * Currently the rule entry structure is a Lyst of rules that all match the index
 * associated with the rule. This lyst has the following properties:
 *
 * 1. Each entry in this lyst is a pointer to a rule that s managed by the rules
 *    vector.
 * 2. The lyst entries are sorted in decreasing order by rule score. This means
 *    that, when searching for a rule, the moment you encounter a rule in this
 *    lyst with a score lower than your current best score, you can stop
 *    searching the lyst.
 * 3. For rules with the same score, this lyst is sub-sorted by ascending
 *    rule user_id.
 */
typedef struct
{
	PsmAddress rules; /**< Refs to items in rules vector sorted by rule score. */
} BpSecPolRuleEntry;


/**
 * BPSec Rule Search Tag
 *
 * The BPSec Rule Search Tag captures information associated with an extension
 * block encountered by the main BPSec engine. The information in this tag is
 * used to see if the filter associated with a policy rule will match (pass) the
 * information found in this tag.
 *
 * Since BpSecPolRules are stored in a shared memory area, the search tag is
 * augmented with the shared memory partition providing the context for the search.
 *
 */
typedef struct
{
	char *bsrc;
	uint16_t bsrc_len;  /**< Length of string WITHOUT NULL terminator. */

	char *bdest;
	uint16_t bdest_len;

	char *ssrc;
	uint16_t ssrc_len;

	int type;
	int scid;
	int role;
	int svc;

	char *es_name;
	uint16_t es_name_len;
} BpSecPolRuleSearchTag;


/**
 * BPSec Rule Search Best Tag
 *
 * This tag associated a search tag with the "one best rule" that matches the
 * information in the search tag.
 */
typedef struct
{
	BpSecPolRuleSearchTag  search;
	BpSecPolRule          *best_rule;
} BpSecPolRuleSearchBestTag;



/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */

/* Rule Filter Functions */
BpSecFilter bslpol_filter_build(PsmPartition partition, char *bsrc, char *bdest, char *ssrc, int type, int role, int scid, int svc);
void        bslpol_filter_score(PsmPartition partition, BpSecFilter *filter);

/* Rule Processing for Bundle. */
int bslpol_proc_applyReceiverPolRule(AcqWorkArea *wk, BpSecPolRule *polRule, int service,
                                     AcqExtBlock *secBlk, BpsecInboundASB *asb, BpsecInboundTargetResult *tgtResult,
                                     sc_Def *def, LystElt *tgtBlkElt, size_t *tgtBlkOrigLen);

int bslpol_proc_applySenderPolRule(Bundle *bundle, BpBlockType secBlkType, BpSecPolRule *polRule, int tgtNum);



/* General Rule Processing Functions */
BpSecPolRule* bslpol_rule_applies(Bundle *bundle, BpBlockType tgtType, int bpaRole);
PsmAddress    bslpol_rule_create(PsmPartition partition, char *desc, uint16_t id, uint8_t flags, BpSecFilter filter, PsmAddress sec_parms, PsmAddress events);
void          bslpol_rule_delete(PsmPartition partition, PsmAddress ruleAddr);
PsmAddress    bslpol_rule_get_addr(PsmPartition partition, int user_id);
Lyst          bslpol_rule_get_all_match(PsmPartition partition, BpSecPolRuleSearchTag criteria);
BpSecPolRule* bslpol_rule_find_best_match(PsmPartition partition, BpSecPolRuleSearchTag tag);
BpSecPolRule* bslpol_rule_get_best_match(PsmPartition partition, BpSecPolRuleSearchTag criteria);
BpSecPolRule* bslpol_rule_get_ptr(PsmPartition partition, int user_id);
BpSecPolRule* bslpol_get_sender_rule(Bundle *bundle, BpBlockType sopType, BpBlockType tgtType);
BpSecPolRule* bslpol_get_receiver_rule(AcqWorkArea *work, unsigned char tgtNum, int scid);

int           bslpol_rule_insert(PsmPartition partition, PsmAddress ruleAddr, int remember);
int           bslpol_rule_matches(PsmPartition partition, BpSecPolRule *rulePtr, BpSecPolRuleSearchTag *tag);
int           bslpol_rule_remove(PsmPartition partition, PsmAddress ruleAddr);
int           bslpol_rule_remove_by_id(PsmPartition partition, int user_id);

/* Security context parameter functions. */
PsmAddress bslpol_scparm_create(PsmPartition partition, int type, int length, void *value);
PsmAddress bslpol_scparm_find(PsmPartition partition, PsmAddress parms, int type);

void       bslpol_scparms_destroy(PsmPartition partition, PsmAddress addr);
Address    bslpol_scparms_persist(PsmPartition partition, char *cursor, PsmAddress parms, int *bytes_left);
Address    bslpol_scparms_restore(PsmPartition partition, PsmAddress *parms, char *cursor, int *bytes_left);
int        bslpol_scparms_size(PsmPartition partition, PsmAddress parms);

/* Rule Persistence Functions. */
int     bslpol_sdr_rule_forget(PsmPartition wm, PsmAddress ruleAddr);
int     bslpol_sdr_rule_persist(PsmPartition wm, PsmAddress ruleAddr);
int     bslpol_sdr_rule_restore(PsmPartition vm, BpSecPolicyDbEntry entry);
int     bslpol_sdr_rule_size(PsmPartition wm, PsmAddress ruleAddr);


/* Rule Searching Functions */
int bslpol_search_tag_best(PsmPartition partition, BpSecPolRuleSearchBestTag *tag, PsmAddress ruleAddr);

/* Call-backs for working with various data structures. */

int  bslpol_cb_rule_compare_idx(PsmPartition partition, PsmAddress eltData, void *insertData);
int  bslpol_cb_rule_compare_score(PsmPartition partition, PsmAddress eltData, void *dataBuffer);
int  bslpol_cb_rulelyst_compare_score(BpSecPolRule *r1, BpSecPolRule *r2);
int  bslpol_cb_ruleradix_insert(PsmPartition partition, PsmAddress *entryAddr, PsmAddress itemAddr);
int  bslpol_cb_ruleradix_remove(PsmPartition partition, PsmAddress entryAddr, void *tag);
int  bslpol_cb_ruleradix_search_best(PsmPartition partition, PsmAddress entryAddr, BpSecPolRuleSearchBestTag *tag);

void bslpol_cb_smlist_delete(PsmPartition partition, PsmAddress eltAddr, void *tag);

#endif /* BPSEC_POLICY_RULE_H_ */
