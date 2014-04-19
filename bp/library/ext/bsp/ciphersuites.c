/*
 *	ciphersuites.c:		implementation of generic ciphersuite
 *				interface for the Bundle Security
 *				Protocol in ION.
 *
 *	Copyright (c) 2014, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "ciphersuites.h"
#include "bab_hmac_sha1.h"
#include "bib_hmac_sha256.h"
#include "bcb_arc4.h"

/*		Bundle Authentication Block ciphersuites		*/

static BabCiphersuite	*_bab_ciphersuites(int *count)
{
	static BabCiphersuite	suites[] =
	{
		{	1, "BAB-HMAC-SHA1", 1,
			bab_hmac_sha1_construct,
			bab_hmac_sha1_sign,
			bab_hmac_sha1_verify				}
	};

	*count = sizeof suites / sizeof(BabCiphersuite);
	return suites;
}

BabCiphersuite	*get_bab_cs_by_name(char *csName)
{
	int		suiteCount;
	BabCiphersuite	*cs = _bab_ciphersuites(&suiteCount);
	int		i;

	CHKNULL(csName);
	for (i = 0; i < suiteCount; i++, cs++)
	{
		if (strcmp(cs->csName, csName) == 0)
		{
			return cs;
		}
	}

	return NULL;
}

BabCiphersuite	*get_bab_cs_by_number(int csNbr)
{
	int		suiteCount;
	BabCiphersuite	*cs = _bab_ciphersuites(&suiteCount);
	int		i;

	CHKNULL(csNbr > 0);
	for (i = 0; i < suiteCount; i++, cs++)
	{
		if (cs->csNbr == csNbr)
		{
			return cs;
		}
	}

	return NULL;
}

/*		Block Integrity Block ciphersuites			*/

static BibCiphersuite	*_bib_ciphersuites(int *count)
{
	static BibCiphersuite	suites[] =
	{
		{	2, "BIB-HMAC-SHA256", 0,
			bib_hmac_sha256_construct,
			bib_hmac_sha256_sign,
			bib_hmac_sha256_verify				}
	};

	*count = sizeof suites / sizeof(BibCiphersuite);
	return suites;
}

BibCiphersuite	*get_bib_cs_by_name(char *csName)
{
	int		suiteCount;
	BibCiphersuite	*cs = _bib_ciphersuites(&suiteCount);
	int		i;

	CHKNULL(csName);
	for (i = 0; i < suiteCount; i++, cs++)
	{
		if (strcmp(cs->csName, csName) == 0)
		{
			return cs;
		}
	}

	return NULL;
}

BibCiphersuite	*get_bib_cs_by_number(int csNbr)
{
	int		suiteCount;
	BibCiphersuite	*cs = _bib_ciphersuites(&suiteCount);
	int		i;

	CHKNULL(csNbr > 0);
	for (i = 0; i < suiteCount; i++, cs++)
	{
		if (cs->csNbr == csNbr)
		{
			return cs;
		}
	}

	return NULL;
}

/*		Block Confidentiality Block ciphersuites		*/

static BcbCiphersuite	*_bcb_ciphersuites(int *count)
{
	static BcbCiphersuite	suites[] =
	{
		{	3, "BCB-ARC4-AES128", 0,
			bcb_arc4_construct,
			bcb_arc4_encrypt,
			bcb_arc4_decrypt				}
	};

	*count = sizeof suites / sizeof(BcbCiphersuite);
	return suites;
}

BcbCiphersuite	*get_bcb_cs_by_name(char *csName)
{
	int		suiteCount;
	BcbCiphersuite	*cs = _bcb_ciphersuites(&suiteCount);
	int		i;

	CHKNULL(csName);
	for (i = 0; i < suiteCount; i++, cs++)
	{
		if (strcmp(cs->csName, csName) == 0)
		{
			return cs;
		}
	}

	return NULL;
}

BcbCiphersuite	*get_bcb_cs_by_number(int csNbr)
{
	int		suiteCount;
	BcbCiphersuite	*cs = _bcb_ciphersuites(&suiteCount);
	int		i;

	CHKNULL(csNbr > 0);
	for (i = 0; i < suiteCount; i++, cs++)
	{
		if (cs->csNbr == csNbr)
		{
			return cs;
		}
	}

	return NULL;
}
