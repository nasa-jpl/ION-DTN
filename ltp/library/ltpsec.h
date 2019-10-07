/*
	ltpsec.h:	definition of the application programming
			interface for accessing the information in
		       	ION's LTP security database.

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
#ifndef _LTPSEC_H_
#define _LTPSEC_H_

#include "ionsec.h"

#ifdef __cplusplus
extern "C" {
#endif

#define	ltpXmitAuthRules	rules[3]
#define	ltpRecvAuthRules	rules[4]

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

#ifdef __cplusplus
}
#endif

#endif  /* _LTPSEC_H_ */
