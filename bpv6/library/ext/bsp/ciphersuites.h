/*
 *	ciphersuites.h:		definitions supporting the generic 
 *				ciphersuite interface in the ION 
 *				implementation of Bundle Security
 *				Protocol.
 *
 *	Copyright (c) 2014, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#ifndef CIPHERSUITES_H_
#define CIPHERSUITES_H_

#include "bsputil.h" 

typedef int	(*BabConstructFn)(ExtensionBlock *, BspOutboundBlock *);
typedef int	(*BabSignFn)(Bundle *, ExtensionBlock *, BspOutboundBlock *);
typedef int	(*BabVerifyFn)(AcqWorkArea *, AcqExtBlock *);

typedef int	(*BibConstructFn)(ExtensionBlock *, BspOutboundBlock *);
typedef int	(*BibSignFn)(Bundle *, ExtensionBlock *, BspOutboundBlock *);
typedef int	(*BibVerifyFn)(AcqWorkArea *, AcqExtBlock *);

typedef int	(*BcbConstructFn)(ExtensionBlock *, BspOutboundBlock *);
typedef int	(*BcbEncryptFn)(Bundle *, ExtensionBlock *, BspOutboundBlock *);
typedef int	(*BcbDecryptFn)(AcqWorkArea *, AcqExtBlock *);

typedef struct
{
	int		csNbr;
	char		*csName;
	int		blockPair;	/*	Boolean.		*/
	BabConstructFn	construct;
	BabSignFn	sign;
	BabVerifyFn	verify;
} BabCiphersuite;

typedef struct
{
	int		csNbr;
	char		*csName;
	int		blockPair;	/*	Boolean.		*/
	BibConstructFn	construct;
	BibSignFn	sign;
	BibVerifyFn	verify;
} BibCiphersuite;

typedef struct
{
	int		csNbr;
	char		*csName;
	int		blockPair;	/*	Boolean.		*/
	BcbConstructFn	construct;
	BcbEncryptFn	encrypt;
	BcbDecryptFn	decrypt;
} BcbCiphersuite;

extern BabCiphersuite	*get_bab_cs_by_name(char *csName);
extern BabCiphersuite	*get_bab_cs_by_number(int csNbr);

extern BibCiphersuite	*get_bib_cs_by_name(char *csName);
extern BibCiphersuite	*get_bib_cs_by_number(int csNbr);

extern BcbCiphersuite	*get_bcb_cs_by_name(char *csName);
extern BcbCiphersuite	*get_bcb_cs_by_number(int csNbr);

#endif /* CIPHERSUITES_H_ */
