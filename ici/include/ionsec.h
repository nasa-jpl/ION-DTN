/*
	ionsec.h:	definition of the application programming
			interface for accessing the information inx
		       	ION's security database.

	Copyright (c) 2009, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

	Author: Scott Burleigh, Jet Propulsion Laboratory
	Modifications: TCSASSEMBLER, TopCoder

	Modification History:
	Date       Who     What
	9-24-13    TC      Added LtpXmitAuthRule and LtpRecvAuthRule
			   structures, added lists of ltpXmitAuthRule
			   and ltpRecvAuthRules in SecDB structure
									*/
#ifndef _SEC_H_
#define _SEC_H_

#include "ion.h"

#define	EPOCH_2000_SEC	946684800

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef ORIGINAL_BSP
typedef struct
{
	Object  securitySrcEid;		/* 	An sdrstring.	        */
	Object	securityDestEid;	/*	An sdrstring.		*/
	char	ciphersuiteName[32];	/*	NULL-terminated.	*/
	char	keyName[32];		/*	NULL-terminated.	*/
} BspBabRule;

typedef struct
{
	Object  securitySrcEid;		/* 	An sdrstring.	        */
	Object	securityDestEid;	/*	An sdrstring.		*/
	int	blockTypeNbr;	
	char	ciphersuiteName[32];	/*	NULL-terminated.	*/
	char	keyName[32];		/*	NULL-terminated.	*/
} BspPibRule;

typedef struct
{
	Object  securitySrcEid;		/* 	An sdrstring.	        */
	Object	securityDestEid;	/*	An sdrstring.		*/
	int	blockTypeNbr;	
	char	ciphersuiteName[32];	/*	NULL-terminated.	*/
	char	keyName[32];		/*	NULL-terminated.	*/
} BspPcbRule;
#else
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
#endif

/*		LTP authentication ciphersuite numbers			*/
#define LTP_AUTH_HMAC_SHA1_80	0
#define LTP_AUTH_RSA_SHA256	1
#define LTP_AUTH_NULL		255

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
#ifdef ORIGINAL_BSP
	Object	bspPibRules;		/*	SdrList of BspBibRule	*/
	Object	bspPcbRules;		/*	SdrList of BspBcbRule	*/
#else
	Object	bspBibRules;		/*	SdrList of BspBibRule	*/
	Object	bspBcbRules;		/*	SdrList of BspBcbRule	*/
#endif
	Object	ltpXmitAuthRules;	/*	SdrList LtpXmitAuthRule	*/
	Object	ltpRecvAuthRules;	/*	SdrList LtpRecvAuthRule	*/
} SecDB;

typedef struct
{
	PsmAddress	publicKeys;	/*	SM RB tree of PubKeyRef	*/
} SecVdb;

extern int	secInitialize();
extern int	secAttach();
extern Object	getSecDbObject();
extern SecVdb	*getSecVdb();
extern void	sec_clearBspRules(char *fromNode, char *toNode, char *blkType);

/*	*	Functions for managing public keys.			*/

extern void	sec_findPublicKey(uvast nodeNbr, BpTimestamp *effectiveTime,
			Object *keyAddr, Object *eltp);
extern int	sec_addPublicKey(uvast nodeNbr, BpTimestamp *effectiveTime,
			time_t assertionTime, int datLen, unsigned char *data);
extern int	sec_removePublicKey(uvast nodeNbr, BpTimestamp *effectiveTime);
extern int	sec_addOwnPublicKey(BpTimestamp *effectiveTime, int datLen,
			unsigned char *data);
extern int	sec_removeOwnPublicKey(BpTimestamp *effectiveTime);
extern int	sec_addPrivateKey(BpTimestamp *effectiveTime, int datLen,
			unsigned char *data);
extern int	sec_removePrivateKey(BpTimestamp *effectiveTime);

extern int	sec_get_public_key(uvast nodeNbr, BpTimestamp *effectiveTime,
			int *datBufferLen, unsigned char *datBuffer);
		/*	Retrieves the value of the public key that
		 *	was valid at "effectiveTime" for the node
		 *	identified by "nodeNbr" (which must not be
		 *	the local node).  The value is written into
		 *	datBuffer unless its length exceeds the length
		 *	of the buffer, which must be supplied in
		 *	*datBufferLen.
		 *
		 *	On success, returns the actual length of the
		 *	key.  If *datBufferLen is less than the
		 *	actual length of the key, returns 0 and
		 *	replaces buffer length in *datBufferLen with
		 *	the actual key length.  If the requested
		 *	key is not found, returns 0 and leaves the
		 *	value in *datBufferLen unchanged.  On
		 *	system failure returns -1.			*/

extern int	sec_get_own_public_key(BpTimestamp *effectiveTime,
			int *datBufferLen, unsigned char *datBuffer);
		/*	Retrieves the value of the public key that was
		 *	valid at "effectiveTime" for the local node.
		 *	The value is written into datBuffer unless
		 *	its length exceeds the length of the buffer,
		 *	which must be supplied in *datBufferLen.
		 *
		 *	On success, returns the actual length of the
		 *	key.  If *datBufferLen is less than the
		 *	actual length of the key, returns 0 and
		 *	replaces buffer length in *datBufferLen with
		 *	the actual key length.  If the requested
		 *	key is not found, returns 0 and leaves the
		 *	value in *datBufferLen unchanged.  On
		 *	system failure returns -1.			*/

extern int	sec_get_private_key(BpTimestamp *effectiveTime,
			int *datBufferLen, unsigned char *datBuffer);
		/*	Retrieves the value of the private key that was
		 *	valid at "effectiveTime" for the local node.
		 *	The value is written into datBuffer unless
		 *	its length exceeds the length of the buffer,
		 *	which must be supplied in *datBufferLen.
		 *
		 *	On success, returns the actual length of the
		 *	key.  If *datBufferLen is less than the
		 *	actual length of the key, returns 0 and
		 *	replaces buffer length in *datBufferLen
		 *	with the actual key length.  If the
		 *	key is not found, returns 0 and leaves the
		 *	value in *datBufferLen unchanged.  On
		 *	system failure returns -1.			*/

/*	*	Functions for managing security information.		*/

extern void	sec_findKey(char *keyName, Object *keyAddr, Object *eltp);
extern int	sec_addKey(char *keyName, char *fileName);
extern int	sec_updateKey(char *keyName, char *fileName);
extern int	sec_removeKey(char *keyName);
extern int	sec_activeKey(char *keyName);
extern int	sec_addKeyValue(char *keyName, char *keyVal, uint32_t keyLen);

/*	Bundle Security Protocol Bundle Authentication Blocks		*/
extern int	sec_findBspBabRule(char *senderEid, char *destEid,
			Object *ruleAddr, Object *eltp);
extern int	sec_addBspBabRule(char *senderEid, char *destEid,
			char *ciphersuiteName, char *keyName);
extern int	sec_updateBspBabRule(char *senderEid, char *destEid,
			char *ciphersuiteName, char *keyName);
extern int	sec_removeBspBabRule(char *senderEid, char *destEid);

#ifdef ORIGINAL_BSP
/*	Bundle Security Protocol Payload Integrity Blocks		*/
extern int	sec_findBspPibRule(char *srcEid, char *destEid, int type,
			Object *ruleAddr, Object *eltp);
extern int	sec_addBspPibRule(char *srcEid, char *destEid, int type,
			char *ciphersuiteName, char *keyName);
extern int	sec_updateBspPibRule(char *srcEid, char *destEid, int type,
			char *ciphersuiteName, char *keyName);
extern int	sec_removeBspPibRule(char *srcEid, char *destEid, int type);

/*	Bundle Security Protocol Payload Confidentiality Blocks		*/
extern int	sec_findBspPcbRule(char *srcEid, char *destEid, int type,
			Object *ruleAddr, Object *eltp);
extern int	sec_addBspPcbRule(char *srcEid, char *destEid, int type,
			char *ciphersuiteName, char *keyName);
extern int	sec_updateBspPcbRule(char *srcEid, char *destEid, int type,
			char *ciphersuiteName, char *keyName);
extern int	sec_removeBspPcbRule(char *srcEid, char *destEid, int type);
#else
/*	Bundle Security Protocol Block Integrity Blocks			*/
extern int	sec_findBspBibRule(char *srcEid, char *destEid, int type,
			Object *ruleAddr, Object *eltp);
extern int	sec_addBspBibRule(char *srcEid, char *destEid, int type,
			char *ciphersuiteName, char *keyName);
extern int	sec_updateBspBibRule(char *srcEid, char *destEid, int type,
			char *ciphersuiteName, char *keyName);
extern int	sec_removeBspBibRule(char *srcEid, char *destEid, int type);

/*	Bundle Security Protocol Block Confidentiality Blocks		*/
extern int	sec_findBspBcbRule(char *srcEid, char *destEid, int type,
			Object *ruleAddr, Object *eltp);
extern int	sec_addBspBcbRule(char *srcEid, char *destEid, int type,
			char *ciphersuiteName, char *keyName);
extern int	sec_updateBspBcbRule(char *srcEid, char *destEid, int type,
			char *ciphersuiteName, char *keyName);
extern int	sec_removeBspBcbRule(char *srcEid, char *destEid, int type);
#endif

/*		LTP segment signing rules				*/
extern int	sec_findLtpXmitAuthRule(uvast ltpEngineId, Object *ruleAddr,
			Object *eltp);
extern int	sec_addLtpXmitAuthRule(uvast ltpEngineId,
			unsigned char ciphersuiteNbr, char *keyName);
extern int	sec_updateLtpXmitAuthRule(uvast ltpEngineId,
			unsigned char ciphersuiteNbr, char *keyName);
extern int	sec_removeLtpXmitAuthRule(uvast ltpEngineId);

/*		LTP segment authentication rules			*/
extern int	sec_findLtpRecvAuthRule(uvast ltpEngineId, Object *ruleAddr,
			Object *eltp);
extern int	sec_addLtpRecvAuthRule(uvast ltpEngineId,
			unsigned char ciphersuiteNbr, char *keyName);
extern int	sec_updateLtpRecvAuthRule(uvast ltpEngineId,
			unsigned char ciphersuiteNbr, char *keyName);
extern int	sec_removeLtpRecvAuthRule(uvast ltpEngineId);

/*	*	Functions for retrieving security information.		*/

extern int	sec_get_key(char *keyName,
			int *keyBufferLength,
			char *keyValueBuffer);
		/*	Retrieves the value of the security key
		 *	identified by "keyName".  The value is
		 *	written into keyValueBuffer unless its
		 *	length exceeds the length of the buffer,
		 *	which must be supplied in *keyBufferLength.
		 *
		 *	On success, returns the actual length of
		 *	key.  If *keyBufferLength is less than the
		 *	actual length of the key, returns 0 and
		 *	replaces buffer length in *keyBufferLength
		 *	with the actual key length.  If the named
		 *	key is not found, returns 0 and leaves the
		 *	value in *keyBufferLength unchanged.  On
		 *	system failure returns -1.			*/

extern void	sec_get_bspBabRule(char *senderEid, char *receiverEid,
			Object *ruleAddr, Object *eltp);
		/*	Finds the BAB rule that most narrowly applies
		 *	to the nodes identified by senderEid and
		 *	receiverEid.  If an applicable rule is found,
		 *	populates ruleAddr and eltp; otherwise, sets
		 *	*eltp to 0.					*/

extern void	sec_get_bspBibRule(char *srcEid, char *destEid,
			int type, Object *ruleAddr, Object *eltp);
		/*	Finds the BIB rule that most narrowly applies
		 *	to the nodes identified by srcEid and destEid,
		 *	for blocks of the indicated type.  (Type zero
		 *	indicates "all block types".)  If applicable
		 *	rule is found, populates ruleAddr and eltp;
		 *	otherwise, sets *eltp to 0.			*/

extern Object	sec_get_bspBibRuleList();

extern void	sec_get_bspBcbRule(char *srcEid, char *destEid,
			int type, Object *ruleAddr, Object *eltp);
		/*	Finds the BCB rule that most narrowly applies
		 *	to the nodes identified by srcEid and destEid,
		 *	for blocks of the indicated type.  (Type zero
		 *	indicates "all block types".)  If applicable
		 *	rule is found, populates ruleAddr and eltp;
		 *	otherwise, sets *eltp to 0.			*/

extern Object	sec_get_bspBcbRuleList();

extern int	sec_get_bpsecNumKeys(int *size);
		/* Retrieves number of keys and maximum size
		 * of each key name.
		 */

extern void	sec_get_bpsecKeys(char *buffer, int length);
		/* Populates a PRE-ALLOCATED buffer of length len
		 * with bpsec key names. Key names are
		 * comma-separated as "K1,K2,K3".
		 */

extern int	sec_get_bpsecNumCSNames(int *size);
		/* Retrieves number of ciphersuites and maximum size
		 * of each ciphersuite name.
		 */

extern void	sec_get_bpsecCSNames(char *buffer, int length);
		/* Populates a PRE-ALLOCATED buffer of length len
		 * with bpsec ciphersuite names. ciphersuite names
		 * are comma-separated as "CS1,CS2,CS3".
		 */

extern int	sec_get_bpsecNumSrcEIDs(int *size);
		/* Retrieves number of rule src EIDs and maximum size
		 * of each EID name.
		 */

extern void	sec_get_bpsecSrcEIDs(char *buffer, int length);
		/* Populates a PRE-ALLOCATED buffer of length len
		 * with bpsec rule src EID names. Src names are
		 * comma-separated as "K1,K2,CS3".
		 */

#ifdef ORIGINAL_BSP
extern int	sec_get_bspPibTxRule(char *destEid, int blockTypeNbr,
			Object *ruleAddr, Object *eltp);
extern int	sec_get_bspPibRxRule(char *srcEid,  int blockTypeNbr,
			Object *ruleAddr, Object *eltp);
extern int	sec_get_bspPibRule(char *srcEid, char *destEid,
			int blockTypeNbr, Object *ruleAddr, Object *eltp);
extern int	sec_get_bspPcbRule(char *srcEid, char *destEid,
			int blockTypeNbr, Object *ruleAddr, Object *eltp);
#endif

#ifdef __cplusplus
}
#endif

#endif  /* _SEC_H_ */
