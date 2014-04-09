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

/**
 * BAB Block Type Fields
 * TODO: Link this back to header files within BP to avoid code duplication.
 */
#define BSP_BAB_TYPE  0x02 /** pre-payload bab block type.  */
#define BSP_PIB_TYPE  0x03 /** BSP PIB block type.          */
#define BSP_PCB_TYPE  0x04 /** BSP PCB block type.          */
#define BSP_ESB_TYPE  0x09 /** BSP ESB block type.          */

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
	Object  fromDestEid;		/* 	Reserved for future use */
	Object  throughDestEid;		/* 	Reserved for future use */
	int	blockTypeNbr;	
	char	ciphersuiteName[32];	/*	NULL-terminated.	*/
	char	keyName[32];		/*	NULL-terminated.	*/
} BspPibRule;


typedef struct
{
	Object  securitySrcEid;		/* 	An sdrstring.	        */
	Object	securityDestEid;	/*	An sdrstring.		*/
	Object  fromDestEid;		/* 	Reserved for future use */
	Object  throughDestEid;		/* 	Reserved for future use */
	int	blockTypeNbr;	
	char	ciphersuiteName[32];	/*	NULL-terminated.	*/
	char	keyName[32];		/*	NULL-terminated.	*/
} BspPcbRule;

typedef struct
{
	Object  securitySrcEid;		/* 	An sdrstring.	        */
	Object	securityDestEid;	/*	An sdrstring.		*/
	Object  fromDestEid;		/* 	Reserved for future use */
	Object  throughDestEid;		/* 	Reserved for future use */
	int	blockTypeNbr;	
	char	ciphersuiteName[32];	/*	NULL-terminated.	*/
	char	keyName[32];		/*	NULL-terminated.	*/
} BspEsbRule;

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
	Object	bspPibRules;		/*	SdrList of BspPibRule	*/
	Object	bspPcbRules;		/*	SdrList of BspPcbRule	*/
	Object	bspEsbRules;		/*	SdrList of BspEsbRule	*/
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
extern int	bspTypeToString(int bspType, char *retVal, int retValSize);
extern int	bspTypeToInt(char *bspType);
extern void	ionClear(char *srcEid, char *destEid, char *blockType);

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

/*	*	Bundle Security Protocol Bundle Authentication Blocks	*/
extern int	sec_findBspBabRule(char *srcEid, char *destEid,
			Object *ruleAddr, Object *eltp);
extern int	sec_addBspBabRule(char *srcEid, char *destEid,
			char *ciphersuiteName, char *keyName);
extern int	sec_updateBspBabRule(char *srcEid, char *destEid,
			char *ciphersuiteName, char *keyName);
extern int	sec_removeBspBabRule(char *srcEid, char *destEid);

/*		Bundle Security Protocol Payload Integrity Blocks	*/
extern int	sec_findBspPibRule(char *srcEid, char *destEid, int type,
			Object *ruleAddr, Object *eltp);
extern int	sec_addBspPibRule(char *srcEid, char *destEid, int type,
			char *ciphersuiteName, char *keyName);
extern int	sec_updateBspPibRule(char *srcEid, char *destEid, int type,
			char *ciphersuiteName, char *keyName);
extern int	sec_removeBspPibRule(char *srcEid, char *destEid, int type);

/*		Bundle Security Protocol Payload Confidentiality Blocks	*/
extern int	sec_findBspPcbRule(char *srcEid, char *destEid, int type,
			Object *ruleAddr, Object *eltp);
extern int	sec_addBspPcbRule(char *srcEid, char *destEid, int type,
			char *ciphersuiteName, char *keyName);
extern int	sec_updateBspPcbRule(char *srcEid, char *destEid, int type,
			char *ciphersuiteName, char *keyName);
extern int	sec_removeBspPcbRule(char *srcEid, char *destEid, int type);

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

extern int	sec_get_bspBabRule(char *srcEid, char *destEid,
			Object *ruleAddr, Object *eltp);

		/*	Finds the BAB transmission rule that most
		 *	narrowly applies to the endpoint identified
		 *	by eid.  If an applicable rule is found,
		 *	populates ruleAddr and eltp; otherwise, sets
		 *	*eltp to 0.  Returns -1 on system failure,
		 *	0 on success.					*/

extern int	sec_get_bspPibTxRule(char *destEid, int blockTypeNbr,
			Object *ruleAddr, Object *eltp);
extern int	sec_get_bspPibRxRule(char *srcEid,  int blockTypeNbr,
			Object *ruleAddr, Object *eltp);
extern int	sec_get_bspPibRule(char *srcEid, char *destEid,
			int blockTypeNbr, Object *ruleAddr, Object *eltp);
extern int	sec_get_bspPcbRule(char *srcEid, char *destEid,
			int blockTypeNbr, Object *ruleAddr, Object *eltp);

#ifdef __cplusplus
}
#endif

#endif  /* _SEC_H_ */
