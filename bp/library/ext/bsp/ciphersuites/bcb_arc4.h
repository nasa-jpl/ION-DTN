/*
 *	bcb_arc4.h:	definitions supporting implementation of
 *			the BCB_ARC4 ciphersuite.
 *
 *	Copyright (c) 2014, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#ifndef BCB_ARC4_H_
#define BCB_ARC4_H_

#include "bspbcb.h"
#include "ciphersuites.h" 

extern int	bcb_arc4_construct(ExtensionBlock *, BspOutboundBlock *);
extern int	bcb_arc4_encrypt(Bundle *, ExtensionBlock *,
			BspOutboundBlock *);
extern int	bcb_arc4_decrypt(AcqWorkArea *wk, AcqExtBlock *blk);

#endif /* BCB_ARC4_H */
