/*
 *	snid.h:		definitions supporting implementation of
 *			the Sending Node IDentification (SNID)
 *			extension block.
 *
 *	Copyright (c) 2014, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "bei.h" 

#define	EXTENSION_TYPE_SNID	65

extern int	snid_offer(ExtensionBlock *, Bundle *);
extern void	snid_release(ExtensionBlock *);
extern int	snid_record(ExtensionBlock *, AcqExtBlock *);
extern int	snid_copy(ExtensionBlock *, ExtensionBlock *);
extern int	snid_processOnFwd(ExtensionBlock *, Bundle *, void *);
extern int	snid_processOnAccept(ExtensionBlock *, Bundle *, void *);
extern int	snid_processOnEnqueue(ExtensionBlock *, Bundle *, void *);
extern int	snid_processOnDequeue(ExtensionBlock *, Bundle *, void *);
extern int	snid_acquire(AcqExtBlock *, AcqWorkArea *);
extern int	snid_check(AcqExtBlock *, AcqWorkArea *);
extern void	snid_clear(AcqExtBlock *);
