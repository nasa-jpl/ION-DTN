/*
	ionsec.h:	definition of the application programming
			interface for accessing the information in
			ION's global security database.

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

#ifndef SEC_H
#define SEC_H

#include "platform.h"
#include "psm.h"
#include "sdrxn.h"
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	char		name[32];	/*	NULL-terminated.	*/
	int		length;
	SdrObject	value;
} SecKey;				/*	Symmetric keys.		*/

typedef struct
{
	time_t		effectiveTime;
	int		length;
	SdrObject	value;
} OwnPublicKey;

typedef struct
{
	time_t		effectiveTime;
	int		length;
	SdrObject	value;
} PrivateKey;

typedef struct
{
	uvast		fqnn;
	time_t		effectiveTime;
	time_t		assertionTime;
	int		length;
	SdrObject	value;
} PublicKey;				/*	Not used for Own keys.	*/

typedef struct
{
	uvast		fqnn;
	time_t		effectiveTime;
	SdrObject	publicKeyElt;	/*	Ref. to PublicKey.	*/
} PubKeyRef;				/*	Not used for Own keys.	*/

typedef struct
{
	SdrObject publicKeys;	    /* SdrList PublicKey */
	SdrObject ownPublicKeys;    /* SdrList OwnPublicKey */
	SdrObject privateKeys;	    /* SdrList PrivateKey */
	time_t	  nextRekeyTime;    /* 1970 epoch time. */
	SdrObject keys;		    /* SdrList of SecKey */
	SdrObject rules[5];	    /* SdrLists of sec rules */
#if !USING_BSL
	SdrObject bpSecPolicyRules; /* Policy Engine Database */
	SdrObject bpSecEventSets;
#endif
} SecDB;

typedef struct
{
	PsmAddress publicKeys;	       /* SM RB tree of PubKeyRef */
#if !USING_BSL
	PsmAddress bpsecPolicyRules;   /* sm_list of BpSecPolRule */
	PsmAddress bpsecRuleIdxBySrc;  /* Radix tree of BpSecPolRule */
	PsmAddress bpsecRuleIdxByDest; /* Radix tree of BpSecPolRule */
	PsmAddress bpsecRuleIdxBySSrc; /* Radix tree of BpSecPolRule */
	PsmAddress bpsecEidDictionary; /* Radix tree of char[] */
	PsmAddress bpsecEventSet;      /* SM RB tree of BpSecPolEventSet */
#endif
} SecVdb;

extern int	secInitialize(void);
extern int	secAttach(void);
extern SdrObject getSecDbObject(void);
extern SecDB	*getSecConstants(void);
extern SecVdb	*getSecVdb(void);

/*
 * Functions for managing asymmetric cryptography.
 */

extern void	sec_findPublicKey(uvast fqnn, time_t effectiveTime,
			SdrObject *keyAddr, SdrObject *eltp);
extern int	sec_addPublicKey(uvast fqnn, time_t effectiveTime,
			time_t assertionTime, int datLen, unsigned char *data);
extern int	sec_removePublicKey(uvast fqnn, time_t effectiveTime);
extern int	sec_addOwnPublicKey(time_t effectiveTime, int datLen,
			unsigned char *data);
extern int	sec_removeOwnPublicKey(time_t effectiveTime);
extern int	sec_addPrivateKey(time_t effectiveTime, int datLen,
			unsigned char *data);
extern int	sec_removePrivateKey(time_t effectiveTime);

extern int	sec_get_public_key(uvast fqnn, time_t effectiveTime,
			int *datBufferLen, unsigned char *datBuffer);
		/*	Retrieves the value of the public key that
		 *	was valid at "effectiveTime" for the node
		 *	identified by "fqnn" (which must not be
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

extern int	sec_get_own_public_key(time_t effectiveTime, int *datBufferLen,
			unsigned char *datBuffer);
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

extern int	sec_get_private_key(time_t effectiveTime, int *datBufferLen,
			unsigned char *datBuffer);
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

/*
 * Functions for managing symmetric cryptography.
 */

extern void	sec_findKey(char *keyName, SdrObject *keyAddr, SdrObject *eltp);
extern int	sec_addKey(char *keyName, char *fileName, int keyLen);
extern int	sec_updateKey(char *keyName, char *fileName, int keyLen);
extern int	sec_removeKey(char *keyName);
extern int	sec_activeKey(char *keyName);
extern int	sec_addKeyValue(char *keyName, char *keyVal, uint32_t keyLen);

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

#ifdef __cplusplus
}
#endif

#endif /* SEC_H */
