/*
 *	snw.h:		definitions supporting implementation of
 *			the Spray and Wait (SNW) block, which
 *			propagates forwarding permits.
 *
 *	Copyright (c) 2019, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "bei.h" 

extern int	snw_offer(ExtensionBlock *, Bundle *);
extern void	snw_release(ExtensionBlock *);
extern int	snw_record(ExtensionBlock *, AcqExtBlock *);
extern int	snw_copy(ExtensionBlock *, ExtensionBlock *);
extern int	snw_processOnFwd(ExtensionBlock *, Bundle *, void *);
extern int	snw_processOnAccept(ExtensionBlock *, Bundle *, void *);
extern int	snw_processOnEnqueue(ExtensionBlock *, Bundle *, void *);
extern int	snw_processOnDequeue(ExtensionBlock *, Bundle *, void *);
extern int	snw_parse(AcqExtBlock *, AcqWorkArea *);
extern int	snw_check(AcqExtBlock *, AcqWorkArea *);
extern void	snw_clear(AcqExtBlock *);
