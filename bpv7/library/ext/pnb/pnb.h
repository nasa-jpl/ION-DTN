/*
 *	pnb.h:		definitions supporting implementation of
 *			the Previous Node Block (PNB).
 *
 *	Copyright (c) 2019, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "bei.h"

extern int	pnb_offer(ExtensionBlock *, Bundle *);
extern void	pnb_release(ExtensionBlock *);
extern int	pnb_record(ExtensionBlock *, AcqExtBlock *);
extern int	pnb_copy(ExtensionBlock *, ExtensionBlock *);
extern int	pnb_processOnFwd(ExtensionBlock *, Bundle *, void *);
extern int	pnb_processOnAccept(ExtensionBlock *, Bundle *, void *);
extern int	pnb_processOnEnqueue(ExtensionBlock *, Bundle *, void *);
extern int	pnb_processOnDequeue(ExtensionBlock *, Bundle *, void *);
extern int	pnb_parse(AcqExtBlock *, AcqWorkArea *);
extern int	pnb_check(AcqExtBlock *, AcqWorkArea *);
extern void	pnb_clear(AcqExtBlock *);
