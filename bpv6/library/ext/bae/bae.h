/*
 *	bae.h:		definitions supporting implementation of
 *			the Bundle Age Extension(BAE) block.
 *
 *	Copyright (c) 2012, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "bei.h" 

#define	EXTENSION_TYPE_BAE	20

extern int	bae_offer(ExtensionBlock *, Bundle *);
extern void	bae_release(ExtensionBlock *);
extern int	bae_record(ExtensionBlock *, AcqExtBlock *);
extern int	bae_copy(ExtensionBlock *, ExtensionBlock *);
extern int	bae_processOnFwd(ExtensionBlock *, Bundle *, void *);
extern int	bae_processOnAccept(ExtensionBlock *, Bundle *, void *);
extern int	bae_processOnEnqueue(ExtensionBlock *, Bundle *, void *);
extern int	bae_processOnDequeue(ExtensionBlock *, Bundle *, void *);
extern int	bae_parse(AcqExtBlock *, AcqWorkArea *);
extern int	bae_check(AcqExtBlock *, AcqWorkArea *);
extern void	bae_clear(AcqExtBlock *);
