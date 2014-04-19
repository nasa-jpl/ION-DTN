/*
 *	bib_hmac_sha256.h:	definitions supporting implementation
 *				of the BIB_HMAC_SHA256 ciphersuite.
 *
 *	Copyright (c) 2014, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#ifndef BIB_HMAC_SHA256_H_
#define BIB_HMAC_SHA256_H_

#include "bspbib.h"
#include "ciphersuites.h" 

extern int	bib_hmac_sha256_construct(ExtensionBlock *,
			BspOutboundBlock *);
extern int	bib_hmac_sha256_sign(Bundle *, ExtensionBlock *,
			BspOutboundBlock *);
extern int	bib_hmac_sha256_verify(AcqWorkArea *wk, AcqExtBlock *blk);

#endif /* BIB_HMAC_SHA256_H */
