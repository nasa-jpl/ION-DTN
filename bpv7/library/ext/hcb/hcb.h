/*
 *	hcb.h:		definitions supporting implementation of
 *			the Hop Count extension block.
 *
 *	Copyright (c) 2019, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "bei.h" 

extern int	hcb_offer(ExtensionBlock *, Bundle *);
extern void	hcb_release(ExtensionBlock *);
extern int	hcb_record(ExtensionBlock *, AcqExtBlock *);
extern int	hcb_copy(ExtensionBlock *, ExtensionBlock *);
extern int	hcb_processOnFwd(ExtensionBlock *, Bundle *, void *);
extern int	hcb_processOnAccept(ExtensionBlock *, Bundle *, void *);
extern int	hcb_processOnEnqueue(ExtensionBlock *, Bundle *, void *);
extern int	hcb_processOnDequeue(ExtensionBlock *, Bundle *, void *);
extern int	hcb_parse(AcqExtBlock *, AcqWorkArea *);
extern int	hcb_check(AcqExtBlock *, AcqWorkArea *);
extern void	hcb_clear(AcqExtBlock *);
