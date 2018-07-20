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

#include "sbsp_util.h"

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

#if 0
typedef int	(*BabConstructFn)(ExtensionBlock *, SbspOutboundBlock *);
typedef int	(*BabSignFn)(Bundle *, ExtensionBlock *, SbspOutboundBlock *);
typedef int	(*BabVerifyFn)(AcqWorkArea *, AcqExtBlock *);
#endif

typedef int	(*BibConstructFn)(ExtensionBlock *, SbspOutboundBlock *);
typedef int	(*BibSignFn)(Bundle *, ExtensionBlock *, SbspOutboundBlock *,
			uvast *);
typedef int	(*BibVerifyFn)(AcqWorkArea *, AcqExtBlock *, uvast *);

typedef int	(*BcbConstructFn)(ExtensionBlock *, SbspOutboundBlock *);
typedef int	(*BcbEncryptFn)(Bundle *, ExtensionBlock *, SbspOutboundBlock *,
			size_t, uvast *);
typedef int	(*BcbDecryptFn)(AcqWorkArea *, AcqExtBlock *, uvast *);

/**
 * PROFILES
 *
 * Captures information necessary to process a Ciphersuite profile.
 * A Ciphersuite profile describes how a particular ciphersuite should
 * be used in the context of an SBSP block.
 *
 * profNbr    - The unique identifier for this profile.
 * profName   - Human-readable name for this ciphersuite.
 * suiteId    - The ciphersuite used by this profile.
 * blockPair  - Whether this profile has a first/last or lone block.
 * construct/sign/verify - utility functions. A value of NULL indicates
 *                         that the generic profile functions should be used.
 */
#if 0
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
#endif

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
	uint16_t    	suiteId;
	uint8_t		blockPair;	/*	Boolean.		*/
	BcbConstructFn	construct;
	BcbEncryptFn	encrypt;
	BcbDecryptFn	decrypt;
} BcbProfile;


/*
 * +--------------------------------------------------------------------------+
 * |    FUNCTION PROTOTYPES  						      +
 * +--------------------------------------------------------------------------+
 */
#if 0
extern BabProfile	*get_bab_prof_by_name(char *profName);
extern BabProfile	*get_bab_prof_by_number(int profNbr);
#endif

extern BibProfile	*get_bib_prof_by_name(char *profName);
extern BibProfile	*get_bib_prof_by_number(int profNbr);

extern BcbProfile	*get_bcb_prof_by_name(char *profName);
extern BcbProfile	*get_bcb_prof_by_number(int profNbr);

#endif /* PROFILES_H_ */
