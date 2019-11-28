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
#include "bpP.h"
#include "bei.h"

#ifdef __cplusplus
extern "C" {
#endif

#define bpsecBibRules	rules[1]
#define bpsecBcbRules	rules[2]

typedef struct
{
	Object 	 	securitySrcEid;	/* 	An sdrstring.	        */
	Object		destEid;	/*	An sdrstring.		*/
	BpBlockType	blockType;	
	char		ciphersuiteName[32];/*	NULL-terminated.	*/
	char		keyName[32];	/*	NULL-terminated.	*/
} BPsecBibRule;

typedef struct
{
	Object 	 	securitySrcEid;	/* 	An sdrstring.	        */
	Object		destEid;	/*	An sdrstring.		*/
	BpBlockType	blockType;	
	char		ciphersuiteName[32];/*	NULL-terminated.	*/
	char		keyName[32];	/*	NULL-terminated.	*/
} BPsecBcbRule;

extern void	sec_clearBPsecRules(char *fromNode, char *toNode,
			BpBlockType *blkType);

/*	*	Functions for managing security information.		*/

/*	Bundle Security Protocol Block Integrity Blocks			*/
extern int	sec_findBPsecBibRule(char *srcEid, char *destEid,
			BpBlockType type, Object *ruleAddr, Object *eltp);
extern int	sec_addBPsecBibRule(char *srcEid, char *destEid,
			BpBlockType type, char *ciphersuiteName, char *keyName);
extern int	sec_updateBPsecBibRule(char *srcEid, char *destEid,
			BpBlockType type, char *ciphersuiteName, char *keyName);
extern int	sec_removeBPsecBibRule(char *srcEid, char *destEid,
			BpBlockType type);

/*	Bundle Security Protocol Block Confidentiality Blocks		*/
extern int	sec_findBPsecBcbRule(char *srcEid, char *destEid,
			BpBlockType type, Object *ruleAddr, Object *eltp);
extern int	sec_addBPsecBcbRule(char *srcEid, char *destEid,
			BpBlockType type, char *ciphersuiteName, char *keyName);
extern int	sec_updateBPsecBcbRule(char *srcEid, char *destEid,
			BpBlockType type, char *ciphersuiteName, char *keyName);
extern int	sec_removeBPsecBcbRule(char *srcEid, char *destEid,
			BpBlockType type);

/*	*	Functions for retrieving security information.		*/

extern void	sec_get_bpsecBibRule(char *srcEid, char *destEid,
			BpBlockType *blkType, Object *ruleAddr, Object *eltp);
		/*	Finds the BIB rule that most narrowly applies
		 *	to the nodes identified by srcEid and destEid,
		 *	for blocks of the indicated type.  (NULL blkType
		 *	indicates "any block type".)  If applicable
		 *	rule is found, populates ruleAddr and eltp;
		 *	otherwise, sets *eltp to 0.			*/

extern Object	sec_get_bpsecBibRuleList();

extern void	sec_get_bpsecBcbRule(char *srcEid, char *destEid,
			BpBlockType *blkType, Object *ruleAddr, Object *eltp);
		/*	Finds the BCB rule that most narrowly applies
		 *	to the nodes identified by srcEid and destEid,
		 *	for blocks of the indicated type.  (NULL blkType
		 *	indicates "any block type".)  If applicable
		 *	rule is found, populates ruleAddr and eltp;
		 *	otherwise, sets *eltp to 0.			*/

extern Object	sec_get_bpsecBcbRuleList();

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

#ifdef __cplusplus
}
#endif

#endif  /* _BPSEC_H_ */
