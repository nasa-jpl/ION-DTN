/*

	ionsec.h:	definition of the application programming
			interface for accessing the information inx
		       	ION's security database.

	Copyright (c) 2009, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/
#ifndef _SEC_H_
#define _SEC_H_

#include "ion.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	char	name[32];		/*	NULL-terminated.	*/
	int	length;
	Object	value;
} SecKey;

typedef struct
{
	Object  senderEid;		/* 	An sdrstring.	        */
	Object	receiverEid;		/*	An sdrstring.		*/
	char	ciphersuiteName[32];	/*	NULL-terminated.	*/
	char	keyName[32];		/*	NULL-terminated.	*/
} BspBabRule;

#ifdef ORIGINAL_BSP
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

typedef struct
{
	Object	keys;			/*	SdrList of SecKey	*/
	Object	bspBabRules;		/*	SdrList of BspBabRule	*/
#ifdef ORIGINAL_BSP
	Object	bspPibRules;		/*	SdrList of BspBibRule	*/
	Object	bspPcbRules;		/*	SdrList of BspBcbRule	*/
#else
	Object	bspBibRules;		/*	SdrList of BspBibRule	*/
	Object	bspBcbRules;		/*	SdrList of BspBcbRule	*/
#endif
} SecDB;

extern int	secInitialize();
extern int	secAttach();
extern Object	getSecDbObject();
extern int	bspTypeToString(int bspType, char *retVal, int retValSize);
extern int	bspTypeToInt(char *bspType);
extern void	ionClear(char *srcEid, char *destEid, char *blockType);

/*	*	Functions for managing security information.		*/

extern void	sec_findKey(char *keyName, Object *keyAddr, Object *eltp);
extern int	sec_addKey(char *keyName, char *fileName);
extern int	sec_updateKey(char *keyName, char *fileName);
extern int	sec_removeKey(char *keyName);

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
		 *	key is not found, returns 0.  On system
		 *	failure returns -1.				*/

extern void	sec_get_bspBabRule(char *senderEid, char *receiverEid,
			Object *ruleAddr, Object *eltp);
		/*	Finds the BAB rule that most narrowly applies
		 *	to the nodes identified by senderEid and
		 *	receiverEid.  If an applicable rule is found,
		 *	populates ruleAddr and eltp; otherwise, sets
		 *	*eltp to 0.					*/

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
