/*****************************************************************************
 **
 ** File Name: profiles.c
 **
 ** Description: Definitions supporting the generic security profiles
 **              interface in the ION implementation of Bundle Security
 **              Profile.
 **
 ** Notes:
 **   - This file was adapted from ciphersuite.c written by Scott Burleigh.
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **            S. Burleigh    Initial implementation as ciphersuite.c
 **  10/30/15  E. Birrane     Initial Implementation [Secure DTN
 **                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

#include "profiles.h"
#include "csi.h"

#if 0
/*		Bundle Authentication Block profiles		*/

static BabProfile	*_bab_profiles(int *count)
{
	static BabProfile	specs[] =
	{
		{	1, "BAB-HMAC-SHA1", CSTYPE_HMAC_SHA1, 1,
			NULL,
			NULL,
			NULL
		},

		{
			4, "BAB-HMAC-SHA256", CSTYPE_HMAC_SHA256, 1,
			NULL,
			NULL,
			NULL
		}

	};

	*count = sizeof specs / sizeof(BabProfile);
	return specs;
}

BabProfile	*get_bab_prof_by_name(char *profName)
{
	int		profCount;
	BabProfile	*prof = _bab_profiles(&profCount);
	int		i;

	CHKNULL(profName);
	for (i = 0; i < profCount; i++, prof++)
	{
		if (strcmp(prof->profName, profName) == 0)
		{
			return prof;
		}
	}

	return NULL;
}

BabProfile	*get_bab_prof_by_number(int profNbr)
{
	int		profCount;
	BabProfile	*prof = _bab_profiles(&profCount);
	int		i;

	CHKNULL(profNbr > 0);
	for (i = 0; i < profCount; i++, prof++)
	{
		if (prof->profNbr == profNbr)
		{
			return prof;
		}
	}

	return NULL;
}
#endif

/*		Block Integrity Block profiles			*/

static BibProfile	*_bib_profiles(int *count)
{
	static BibProfile	specs[] =
	{
		{	2, "BIB-HMAC-SHA256", CSTYPE_HMAC_SHA256, 0,
			NULL,
			NULL,
			NULL
		},
		{
			5, "BIB-HMAC-SHA384", CSTYPE_HMAC_SHA384, 0,
			NULL,
			NULL,
			NULL
		},
		{
			6, "BIB-ECDSA-SHA256", CSTYPE_ECDSA_SHA256, 0,
			NULL,
			NULL,
			NULL
		},
		{
			7, "BIB-ECDSA-SHA384", CSTYPE_ECDSA_SHA384, 0,
			NULL,
			NULL,
			NULL
		}

	};

	*count = sizeof specs / sizeof(BibProfile);
	return specs;
}

BibProfile	*get_bib_prof_by_name(char *profName)
{
	int		profCount;
	BibProfile	*prof = _bib_profiles(&profCount);
	int		i;

	CHKNULL(profName);
	for (i = 0; i < profCount; i++, prof++)
	{
		if (strcmp(prof->profName, profName) == 0)
		{
			return prof;
		}
	}

	return NULL;
}

BibProfile	*get_bib_prof_by_number(int profNbr)
{
	int		profCount;
	BibProfile	*prof = _bib_profiles(&profCount);
	int		i;

	CHKNULL(profNbr > 0);
	for (i = 0; i < profCount; i++, prof++)
	{
		if (prof->profNbr == profNbr)
		{
			return prof;
		}
	}

	return NULL;
}

/*		Block Confidentiality Block ciphersuites		*/

static BcbProfile	*_bcb_profiles(int *count)
{
	static BcbProfile	specs[] =
	{
		{	3, "BCB-ARC4-AES128", CSTYPE_ARC4, 0,
			NULL,
			NULL,
			NULL
		},
		{	8, "BCB-SHA256-AES128", CSTYPE_SHA256_AES128, 0,
			NULL,
			NULL,
			NULL
		},
		{	9, "BCB-SHA384-AES256", CSTYPE_SHA384_AES256, 0,
			NULL,
			NULL,
			NULL
		}

	};

	*count = sizeof specs / sizeof(BcbProfile);
	return specs;
}

BcbProfile	*get_bcb_prof_by_name(char *profName)
{
	int		profCount;
	BcbProfile	*prof = _bcb_profiles(&profCount);
	int		i;

	CHKNULL(profName);
	for (i = 0; i < profCount; i++, prof++)
	{
		if (strcmp(prof->profName, profName) == 0)
		{
			return prof;
		}
	}

	return NULL;
}

BcbProfile	*get_bcb_prof_by_number(int profNbr)
{
	int		profCount;
	BcbProfile	*prof = _bcb_profiles(&profCount);
	int		i;

	CHKNULL(profNbr > 0);
	for (i = 0; i < profCount; i++, prof++)
	{
		if (prof->profNbr == profNbr)
		{
			return prof;
		}
	}

	return NULL;
}
