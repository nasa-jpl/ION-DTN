/*
 *	meb.h:		definitions supporting implementation of
 *			the Metadata extension block.
 *
 *	Copyright (c) 2014, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "bei.h" 

extern int	meb_offer(ExtensionBlock *, Bundle *);
extern void	meb_release(ExtensionBlock *);
extern int	meb_record(ExtensionBlock *, AcqExtBlock *);
extern int	meb_copy(ExtensionBlock *, ExtensionBlock *);
extern int	meb_processOnFwd(ExtensionBlock *, Bundle *, void *);
extern int	meb_processOnAccept(ExtensionBlock *, Bundle *, void *);
extern int	meb_processOnEnqueue(ExtensionBlock *, Bundle *, void *);
extern int	meb_processOnDequeue(ExtensionBlock *, Bundle *, void *);
extern int	meb_acquire(AcqExtBlock *, AcqWorkArea *);
extern int	meb_check(AcqExtBlock *, AcqWorkArea *);
extern void	meb_clear(AcqExtBlock *);
