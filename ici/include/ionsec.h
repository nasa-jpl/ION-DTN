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
	Object	recvEid;		/*	An sdrstring.		*/
	char	ciphersuiteName[32];	/*	NULL-terminated.	*/
	char	keyName[32];		/*	NULL-terminated.	*/
} BabTxRule;

typedef struct
{
	Object	xmitEid;		/*	An sdrstring.		*/
	char	ciphersuiteName[32];	/*	NULL-terminated.	*/
	char	keyName[32];		/*	NULL-terminated.	*/
} BabRxRule;

typedef struct
{
	Object	fromDestEid;		/*	An sdrstring.		*/
	Object	throughDestEid;		/*	An sdrstring.		*/
	char	ciphersuiteName[32];	/*	NULL-terminated.	*/
	char	keyName[32];		/*	NULL-terminated.	*/
	Object	securityDestEid;	/*	An sdrstring.		*/
} PibTxRule;

typedef struct
{
	Object	fromSourceEid;		/*	An sdrstring.		*/
	Object	throughSourceEid;	/*	An sdrstring.		*/
	char	ciphersuiteName[32];	/*	NULL-terminated.	*/
	char	keyName[32];		/*	NULL-terminated.	*/
	Object	securitySourceEid;	/*	An sdrstring.		*/
} PibRxRule;

typedef struct
{
	Object	fromDestEid;		/*	An sdrstring.		*/
	Object	throughDestEid;		/*	An sdrstring.		*/
	char	ciphersuiteName[32];	/*	NULL-terminated.	*/
	char	keyName[32];		/*	NULL-terminated.	*/
	Object	securityDestEid;	/*	An sdrstring.		*/
} PcbTxRule;

typedef struct
{
	Object	fromSourceEid;		/*	An sdrstring.		*/
	Object	throughSourceEid;	/*	An sdrstring.		*/
	char	ciphersuiteName[32];	/*	NULL-terminated.	*/
	char	keyName[32];		/*	NULL-terminated.	*/
	Object	securitySourceEid;	/*	An sdrstring.		*/
} PcbRxRule;

typedef struct
{
	Object	fromDestEid;		/*	An sdrstring.		*/
	Object	throughDestEid;		/*	An sdrstring.		*/
	int	blockTypeNbr;
	char	ciphersuiteName[32];	/*	NULL-terminated.	*/
	char	keyName[32];		/*	NULL-terminated.	*/
	Object	securityDestEid;	/*	An sdrstring.		*/
} EsbTxRule;

typedef struct
{
	Object	fromSourceEid;		/*	An sdrstring.		*/
	Object	throughSourceEid;	/*	An sdrstring.		*/
	int	blockTypeNbr;
	char	ciphersuiteName[32];	/*	NULL-terminated.	*/
	char	keyName[32];		/*	NULL-terminated.	*/
	Object	securitySourceEid;	/*	An sdrstring.		*/
} EsbRxRule;

typedef struct
{
	Object	keys;			/*	SdrList of SecKey	*/
	Object	babTxRules;		/*	SdrList of BabTxRule	*/
	Object	babRxRules;		/*	SdrList of BabRxRule	*/
	Object	pibTxRules;		/*	SdrList of PibTxRule	*/
	Object	pibRxRules;		/*	SdrList of PibRxRule	*/
	Object	pcbTxRules;		/*	SdrList of PcbTxRule	*/
	Object	pcbRxRules;		/*	SdrList of PcbRxRule	*/
	Object	esbTxRules;		/*	SdrList of EsbTxRule	*/
	Object	esbRxRules;		/*	SdrList of EsbRxRule	*/
} SecDB;

extern int		secInitialize();
extern int		secAttach();
extern Object		getSecDbObject();
extern void		ionClear(char *eid);

/*	*	Functions for managing security information.		*/

extern void	sec_findKey(char *keyName, Object *keyAddr, Object *eltp);
extern int	sec_addKey(char *keyName, char *fileName);
extern int	sec_updateKey(char *keyName, char *fileName);
extern int	sec_removeKey(char *keyName);

extern void	sec_findBabTxRule(char *eid, Object *ruleAddr, Object *eltp);
extern int	sec_addBabTxRule(char *eid, char *ciphersuiteName,
			char *keyName);
extern int	sec_updateBabTxRule(char *eid, char *ciphersuiteName,
			char *keyName);
extern int	sec_removeBabTxRule(char *eid);

extern void	sec_findBabRxRule(char *eid, Object *ruleAddr, Object *eltp);
extern int	sec_addBabRxRule(char *eid, char *ciphersuiteName,
			char *keyName);
extern int	sec_updateBabRxRule(char *eid, char *ciphersuiteName,
			char *keyName);
extern int	sec_removeBabRxRule(char *eid);

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
		 *	On success, returns the actual length the
		 *	key.  If *keyBufferLength is less than the
		 *	actual length of the key, returns 0 and
		 *	replaces buffer length in *keyBufferLength
		 *	with the actual key length.  If the named
		 *	key is not found, returns 0.  On system
		 *	failure returns -1.				*/

extern int	sec_get_babTxRule(char *eid, Object *ruleAddr, Object *eltp);
		/*	Finds the BAB transmission rule that most
		 *	narrowly applies to the endpoint identified
		 *	by eid.  If an applicable rule is found,
		 *	populates ruleAddr and eltp; otherwise, sets
		 *	*eltp to 0.  Returns -1 on system failure,
		 *	0 on success.					*/

extern int	sec_get_babRxRule(char *eid, Object *ruleAddr, Object *eltp);
		/*	Finds the BAB reception rule that most
		 *	narrowly applies to the endpoint identified
		 *	by eid.  If an applicable rule is found,
		 *	populates ruleAddr and eltp; otherwise, sets
		 *	*eltp to 0.  Returns -1 on system failure,
		 *	0 on success.					*/

#ifdef __cplusplus
}
#endif

#endif  /* _SEC_H_ */
