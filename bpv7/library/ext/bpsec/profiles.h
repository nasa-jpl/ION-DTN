/*****************************************************************************
 **
 ** File Name: profiles.h
 **
 ** Description: Definitions supporting the generic security profiles
 **              interface in the ION implementation of Bundle Security
 **              Protocol (bpsec).
 **
 **		 Note that "security context" is another way of saying
 **		 "security profile."
 **
 ** Notes:
 **   - This file was adapted from ciphersuite.h written by Scott Burleigh.
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **            S. Burleigh    Initial implementation as ciphersuite.h
 **  10/30/15  E. Birrane     Initial Implementation [Secure DTN
 **                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

#ifndef PROFILES_H_
#define PROFILES_H_

#include "bpsec_util.h"

/*
 * +--------------------------------------------------------------------------+
 * |   CONSTANTS  							      +
 * +--------------------------------------------------------------------------+
 */


/*
 * +--------------------------------------------------------------------------+
 * |  	MACROS 								      +
 * +--------------------------------------------------------------------------+
 */


/*
 * +--------------------------------------------------------------------------+
 * |    DATA TYPES  							      +
 * +--------------------------------------------------------------------------+
 */

typedef int	(*BibConstructFn)(uint16_t, ExtensionBlock *,
			BpsecOutboundBlock *);
typedef int	(*BibSignFn)(uint16_t, Bundle *, ExtensionBlock *,
			BpsecOutboundBlock *, BpsecOutboundTarget *,
			Object, char *);
typedef int	(*BibVerifyFn)(uint16_t, AcqWorkArea *, AcqExtBlock *,
			BpsecInboundBlock *, BpsecInboundTarget *,
			Object, char *);

typedef int	(*BcbConstructFn)(uint16_t, ExtensionBlock *,
			BpsecOutboundBlock *);
typedef int	(*BcbEncryptFn)(uint16_t, Bundle *, ExtensionBlock *,
			BpsecOutboundBlock *, BpsecOutboundTarget *,
			size_t, uvast *, char *);
typedef int	(*BcbDecryptFn)(uint16_t, AcqWorkArea *, AcqExtBlock *,
			BpsecInboundBlock *, BpsecInboundTarget *, char *);

/**
 * PROFILES
 *
 * Captures information necessary to process a security profile.
 * A security profile describes how a particular ciphersuite should
 * be used in the context of an SBSP block.
 *
 * profNbr    - The unique identifier for this profile.
 * profName   - Human-readable name for this ciphersuite.
 * suiteId    - The ciphersuite used by this profile.
 * construct/sign/verify - utility functions. A value of NULL indicates
 *                         that the generic profile functions should be used.
 */

typedef struct
{
	uint16_t	profNbr;	/*	A.K.A. context ID	*/
	char		*profName;
	uint16_t	suiteId;	/*	Ciphersuite number	*/
	BibConstructFn	construct;
	BibSignFn	sign;
	BibVerifyFn	verify;
} BibProfile;

typedef struct
{
	uint16_t	profNbr;	/*	A.K.A. context ID	*/
	char		*profName;
	uint16_t    	suiteId;	/*	Ciphersuite number	*/
	BcbConstructFn	construct;
	BcbEncryptFn	encrypt;
	BcbDecryptFn	decrypt;
} BcbProfile;

/*
 * +--------------------------------------------------------------------------+
 * |    FUNCTION PROTOTYPES  						      +
 * +--------------------------------------------------------------------------+
 */

extern BibProfile	*get_bib_prof_by_name(char *profName);
extern BibProfile	*get_bib_prof_by_number(int profNbr);

extern BcbProfile	*get_bcb_prof_by_name(char *profName);
extern BcbProfile	*get_bcb_prof_by_number(int profNbr);

#endif /* PROFILES_H_ */
