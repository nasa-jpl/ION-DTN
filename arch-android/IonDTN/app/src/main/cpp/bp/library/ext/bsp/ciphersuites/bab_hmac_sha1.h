/*
 *	bab_hmac_sha1.h:	definitions supporting implementation
 *				of the BAB_HMAC_SHA1 ciphersuite.
 *
 *	Copyright (c) 2014, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#ifndef BAB_HMAC_SHA1_H_
#define BAB_HMAC_SHA1_H_

#include "bspbab.h"
#include "ciphersuites.h" 

extern int	bab_hmac_sha1_construct(ExtensionBlock *, BspOutboundBlock *);
extern int	bab_hmac_sha1_sign(Bundle *, ExtensionBlock *,
			BspOutboundBlock *);
extern int	bab_hmac_sha1_verify(AcqWorkArea *wk, AcqExtBlock *blk);

#endif /* BAB_HMAC_SHA1_H */
