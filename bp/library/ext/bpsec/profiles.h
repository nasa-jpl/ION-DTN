/*****************************************************************************
 **
 ** File Name: profiles.h
 **
 ** Description: Definitions supporting the generic security profiles
 **              interface in the ION implementation of Bundle Security
 **              Profile.
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
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */


/*
 * +--------------------------------------------------------------------------+
 * |							  	MACROS  								  +
 * +--------------------------------------------------------------------------+
 */


/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */

typedef int	(*BabConstructFn)(ExtensionBlock *, BpsecOutboundBlock *);
typedef int	(*BabSignFn)(Bundle *, ExtensionBlock *, BpsecOutboundBlock *);
typedef int	(*BabVerifyFn)(AcqWorkArea *, AcqExtBlock *);

typedef int	(*BibConstructFn)(ExtensionBlock *, BpsecOutboundBlock *);
typedef int	(*BibSignFn)(Bundle *, ExtensionBlock *, BpsecOutboundBlock *, uvast *);
typedef int	(*BibVerifyFn)(AcqWorkArea *, AcqExtBlock *, uvast *);

typedef int	(*BcbConstructFn)(ExtensionBlock *, BpsecOutboundBlock *);
typedef int	(*BcbEncryptFn)(Bundle *, ExtensionBlock *, BpsecOutboundBlock *, uvast *);
typedef int	(*BcbDecryptFn)(AcqWorkArea *, AcqExtBlock *, uvast *);

/**
 * PROFILES
 *
 * Captures information necessary to process a BAB Ciphersuite profile.
 * A BAB profile describes how a particular ciphersuite should be used
 * in the context of a BAB block (or blocks).
 *
 * profNbr    - The unique identifier for this profile.
 * profName   - Human-readable name for this ciphersuite.
 * suiteId    - The ciphersuite used by this profile.
 * blockPair  - Whether this BAB has a first/last or lone block.
 * construct/sign/verify - utility functions. A value of NULL indicates
 *                         that the generic BAB functions should be used.
 */
typedef struct
{
	uint16_t	profNbr;
	char		*profName;
	uint16_t	suiteId;
	uint8_t		blockPair;	/*	Boolean.		*/
	BabConstructFn	construct;
	BabSignFn	sign;
	BabVerifyFn	verify;
} BabProfile;

typedef struct
{
	uint16_t	profNbr;
	char		*profName;
	uint16_t	suiteId;
	uint8_t		blockPair;	/*	Boolean.		*/
	BibConstructFn	construct;
	BibSignFn	sign;
	BibVerifyFn	verify;
} BibProfile;

typedef struct
{
	uint16_t	profNbr;
	char		*profName;
	uint16_t    suiteId;
	uint8_t		blockPair;	/*	Boolean.		*/
	BcbConstructFn	construct;
	BcbEncryptFn	encrypt;
	BcbDecryptFn	decrypt;
} BcbProfile;



/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */

extern BabProfile	*get_bab_prof_by_name(char *profName);
extern BabProfile	*get_bab_prof_by_number(int profNbr);

extern BibProfile	*get_bib_prof_by_name(char *profName);
extern BibProfile	*get_bib_prof_by_number(int profNbr);

extern BcbProfile	*get_bcb_prof_by_name(char *profName);
extern BcbProfile	*get_bcb_prof_by_number(int profNbr);

#endif /* PROFILES_H_ */
