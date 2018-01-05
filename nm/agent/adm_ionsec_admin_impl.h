/****************************************************************************
 **
 ** File Name: adm_ionsec_admin_impl.h
 **
 ** Description: TODO
 **
 ** Notes: TODO
 **
 ** Assumptions: TODO
 **
 ** Modification History: 
 **  YYYY-MM-DD  AUTHOR           DESCRIPTION
 **  ----------  --------------   --------------------------------------------
 **  2018-01-05  AUTO             Auto-generated header file 
 **
 ****************************************************************************/

#ifndef ADM_IONSEC_ADMIN_IMPL_H_
#define ADM_IONSEC_ADMIN_IMPL_H_

/*   START CUSTOM INCLUDES HERE  */
#define	EPOCH_2000_SEC	946684800
/*		LTP authentication ciphersuite numbers			*/
#define LTP_AUTH_HMAC_SHA1_80	0
#define LTP_AUTH_RSA_SHA256	1
#define LTP_AUTH_NULL		255

/*   STOP CUSTOM INCLUDES HERE  */


#include "../shared/primitives/tdc.h"
#include "../shared/primitives/value.h"
#include "../shared/utils/utils.h"
#include "../shared/primitives/ctrl.h"
#include "../shared/primitives/table.h"

/*   START typeENUM */
typedef struct
{
	char		name[32];	/*	NULL-terminated.	*/
	int		length;
	Object		value;
} SecKey;				/*	Symmetric keys.		*/

typedef struct
{
	BpTimestamp	effectiveTime;
	int		length;
	Object		value;
} OwnPublicKey;

typedef struct
{
	BpTimestamp	effectiveTime;
	int		length;
	Object		value;
} PrivateKey;

typedef struct
{
	uvast		nodeNbr;
	BpTimestamp	effectiveTime;
	time_t		assertionTime;
	int		length;
	Object		value;
} PublicKey;				/*	Not used for Own keys.	*/

typedef struct
{
	uvast		nodeNbr;
	BpTimestamp	effectiveTime;
	Object		publicKeyElt;	/*	Ref. to PublicKey.	*/
} PubKeyRef;				/*	Not used for Own keys.	*/


typedef struct
{
	Object  senderEid;		/* 	An sdrstring.	        */
	Object	receiverEid;		/*	An sdrstring.		*/
	char	ciphersuiteName[32];	/*	NULL-terminated.	*/
	char	keyName[32];		/*	NULL-terminated.	*/
} BspBabRule;

typedef struct
{
	Object  securitySrcEid;		/* 	An sdrstring.	        */
	Object	destEid;		/*	An sdrstring.		*/
	int	blockTypeNbr;	
	char	ciphersuiteName[32];	/*	NULL-terminated.	*/
	char	keyName[32];		/*	NULL-terminated.	*/
} BspBibRule;

typedef struct
{
	Object  securitySrcEid;		/* 	An sdrstring.	        */
	Object	destEid;		/*	An sdrstring.		*/
	int	blockTypeNbr;	
	char	ciphersuiteName[32];	/*	NULL-terminated.	*/
	char	keyName[32];		/*	NULL-terminated.	*/
} BspBcbRule;

/*	LtpXmitAuthRule records an LTP segment signing rule for an
 *	identified remote LTP engine.  The rule specifies the
 *	ciphersuite to use for signing those segments and the
 *	name of the key that the indicated ciphersuite must use.	*/
typedef struct
{
	uvast		ltpEngineId;
	unsigned char	ciphersuiteNbr;
	char		keyName[32];
} LtpXmitAuthRule;

/*	LtpRecvAuthRule records an LTP segment authentication rule
 *	for an identified remote LTP engine.  The rule specifies
 *	the ciphersuite to use for authenticating segments and the
 *	name of the key that the indicated ciphersuite must use.	*/
typedef struct
{
	uvast		ltpEngineId;
	unsigned char	ciphersuiteNbr;
	char		keyName[32];
} LtpRecvAuthRule;

typedef struct
{
	Object	publicKeys;		/*	SdrList PublicKey	*/
	Object	ownPublicKeys;		/*	SdrList OwnPublicKey	*/
	Object	privateKeys;		/*	SdrList PrivateKey	*/
	time_t	nextRekeyTime;		/*	UTC			*/
	Object	keys;			/*	SdrList of SecKey	*/
	Object	bspBabRules;		/*	SdrList of BspBabRule	*/
	Object	bspBibRules;		/*	SdrList of BspBibRule	*/
	Object	bspBcbRules;		/*	SdrList of BspBcbRule	*/
	Object	ltpXmitAuthRules;	/*	SdrList LtpXmitAuthRule	*/
	Object	ltpRecvAuthRules;	/*	SdrList LtpRecvAuthRule	*/
} SecDB;

typedef struct
{
	PsmAddress	publicKeys;	/*	SM RB tree of PubKeyRef	*/
} SecVdb;

/*   STOP typeENUM  */
void name_adm_init_agent();


/******************************************************************************
 *                            Retrieval Functions                             *
 ******************************************************************************/

/*   START CUSTOM FUNCTIONS HERE */
static void	printBspBabRule(Object ruleAddr);
static void	printBspBibRule(Object ruleAddr);
static void printBspBcbRule(Object ruleAddr);
static void	printLtpRecvAuthRule(Object ruleAddr);
static void	printLtpXmitAuthRule(Object ruleAddr);
/*	*	Functions for managing security information.		*/

extern void	sec_findKey(char *keyName, Object *keyAddr, Object *eltp);
extern int	sec_addKey(char *keyName, char *fileName);
extern int	sec_updateKey(char *keyName, char *fileName);
extern int	sec_removeKey(char *keyName);
extern int	sec_activeKey(char *keyName);
extern int	sec_addKeyValue(char *keyName, char *keyVal, uint32_t keyLen);
/*   STOP CUSTOM FUNCTIONS HERE  */

void adm_ionsec_admin_setup();
void adm_ionsec_admin_cleanup();

/* Metadata Functions */
value_t adm_ionsec_admin_meta_name(tdc_t params);
value_t adm_ionsec_admin_meta_namespace(tdc_t params);

value_t adm_ionsec_admin_meta_version(tdc_t params);

value_t adm_ionsec_admin_meta_organization(tdc_t params);


/* Collect Functions */


/* Control Functions */
tdc_t* adm_ionsec_admin_ctrl_key_add(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ionsec_admin_ctrl_key_change(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ionsec_admin_ctrl_key_del(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ionsec_admin_ctrl_ltp_rx_rule_add(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ionsec_admin_ctrl_ltp_rx_rule_change(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ionsec_admin_ctrl_ltp_rx_rule_del(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ionsec_admin_ctrl_ltp_tx_rule_add(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ionsec_admin_ctrl_ltp_tx_rule_change(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ionsec_admin_ctrl_ltp_tx_rule_del(eid_t *def_mgr, tdc_t params, int8_t *status);


/* OP Functions */

#endif //ADM_IONSEC_ADMIN_IMPL_H_
