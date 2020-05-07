/*
 *	bpq.h:		definitions supporting implementation of
 *			the BP Quality of Service (QOS) block.
 *
 *	Copyright (c) 2019, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "bei.h" 

extern int	qos_offer(ExtensionBlock *, Bundle *);
extern void	qos_release(ExtensionBlock *);
extern int	qos_record(ExtensionBlock *, AcqExtBlock *);
extern int	qos_copy(ExtensionBlock *, ExtensionBlock *);
extern int	qos_processOnFwd(ExtensionBlock *, Bundle *, void *);
extern int	qos_processOnAccept(ExtensionBlock *, Bundle *, void *);
extern int	qos_processOnEnqueue(ExtensionBlock *, Bundle *, void *);
extern int	qos_processOnDequeue(ExtensionBlock *, Bundle *, void *);
extern int	qos_parse(AcqExtBlock *, AcqWorkArea *);
extern int	qos_check(AcqExtBlock *, AcqWorkArea *);
extern void	qos_clear(AcqExtBlock *);
