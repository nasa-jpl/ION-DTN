/*
 *	phn.h:		definitions supporting implementation of
 *			the Previous Hop Node (PHN) block.
 *
 *	Copyright (c) 2009, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "bei.h"

#define	EXTENSION_TYPE_PHN	5

extern int	phn_offer(ExtensionBlock *, Bundle *);
extern void	phn_release(ExtensionBlock *);
extern int	phn_record(ExtensionBlock *, AcqExtBlock *);
extern int	phn_copy(ExtensionBlock *, ExtensionBlock *);
extern int	phn_processOnFwd(ExtensionBlock *, Bundle *, void *);
extern int	phn_processOnAccept(ExtensionBlock *, Bundle *, void *);
extern int	phn_processOnEnqueue(ExtensionBlock *, Bundle *, void *);
extern int	phn_processOnDequeue(ExtensionBlock *, Bundle *, void *);
extern int	phn_parse(AcqExtBlock *, AcqWorkArea *);
extern int	phn_check(AcqExtBlock *, AcqWorkArea *);
extern void	phn_clear(AcqExtBlock *);
