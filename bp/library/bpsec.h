/*
	bpsec.h:	definition of the application programming
			interface for accessing the information in
		       	ION's BP security database.

	Copyright (c) 2019, California Institute of Technology.
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
#ifndef _BPSEC_H_
#define _BPSEC_H_

#include "ionsec.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ORIGINAL_BSP
#define	bspBabRules	rules[0]
#define bspPibRules	rules[1]
#define bspPcbRules	rules[2]
#else
#define bspBibRules	rules[1]
#define bspBcbRules	rules[2]
#endif

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

extern void	sec_clearBspRules(char *fromNode, char *toNode, char *blkType);

/*	*	Functions for managing security information.		*/

#ifdef ORIGINAL_BSP
/*	Bundle Security Protocol Bundle Authentication Blocks		*/
extern int	sec_findBspBabRule(char *senderEid, char *destEid,
			Object *ruleAddr, Object *eltp);
extern int	sec_addBspBabRule(char *senderEid, char *destEid,
			char *ciphersuiteName, char *keyName);
extern int	sec_updateBspBabRule(char *senderEid, char *destEid,
			char *ciphersuiteName, char *keyName);
extern int	sec_removeBspBabRule(char *senderEid, char *destEid);

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

#ifdef ORIGINAL_BSP
extern void	sec_get_bspBabRule(char *senderEid, char *receiverEid,
			Object *ruleAddr, Object *eltp);
		/*	Finds the BAB rule that most narrowly applies
		 *	to the nodes identified by senderEid and
		 *	receiverEid.  If an applicable rule is found,
		 *	populates ruleAddr and eltp; otherwise, sets
		 *	*eltp to 0.					*/

extern int	sec_get_bspPibTxRule(char *destEid, int blockTypeNbr,
			Object *ruleAddr, Object *eltp);
extern int	sec_get_bspPibRxRule(char *srcEid,  int blockTypeNbr,
			Object *ruleAddr, Object *eltp);
extern int	sec_get_bspPibRule(char *srcEid, char *destEid,
			int blockTypeNbr, Object *ruleAddr, Object *eltp);
extern int	sec_get_bspPcbRule(char *srcEid, char *destEid,
			int blockTypeNbr, Object *ruleAddr, Object *eltp);
#else
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

extern int	sec_get_sbspNumKeys(int *size);
		/* Retrieves number of keys and maximum size
		 * of each key name.
		 */

extern void	sec_get_sbspKeys(char *buffer, int length);
		/* Populates a PRE-ALLOCATED buffer of length len
		 * with sbsp key names. Key names are
		 * comma-separated as "K1,K2,K3".
		 */

extern int	sec_get_sbspNumCSNames(int *size);
		/* Retrieves number of ciphersuites and maximum size
		 * of each ciphersuite name.
		 */

extern void	sec_get_sbspCSNames(char *buffer, int length);
		/* Populates a PRE-ALLOCATED buffer of length len
		 * with sbsp ciphersuite names. ciphersuite names
		 * are comma-separated as "CS1,CS2,CS3".
		 */

extern int	sec_get_sbspNumSrcEIDs(int *size);
		/* Retrieves number of rule src EIDs and maximum size
		 * of each EID name.
		 */

extern void	sec_get_sbspSrcEIDs(char *buffer, int length);
		/* Populates a PRE-ALLOCATED buffer of length len
		 * with sbsp rule src EID names. Src names are
		 * comma-separated as "K1,K2,CS3".
		 */
#endif

#ifdef __cplusplus
}
#endif

#endif  /* _BPSEC_H_ */
