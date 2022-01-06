/*
 *	ipt.h:		definitions supporting implementation of
 *			the Inter-Regional Routing Passageway Trace
 *			block, which preserves a trace of all
 *			passageway nodes traversed to date by a
 *			bundle whose source and destination nodes
 *			are in different regions.
 *
 *	Copyright (c) 2021, IPNGROUP.  ALL RIGHTS RESERVED.
 *
 *	Author: Scott Burleigh, IPNGROUP
 */

#include "bei.h" 

extern int	ipt_offer(ExtensionBlock *, Bundle *);
extern int	ipt_serialize(ExtensionBlock *, Bundle *);
extern void	ipt_release(ExtensionBlock *);
extern int	ipt_record(ExtensionBlock *, AcqExtBlock *);
extern int	ipt_copy(ExtensionBlock *, ExtensionBlock *);
extern int	ipt_processOnFwd(ExtensionBlock *, Bundle *, void *);
extern int	ipt_processOnAccept(ExtensionBlock *, Bundle *, void *);
extern int	ipt_processOnEnqueue(ExtensionBlock *, Bundle *, void *);
extern int	ipt_processOnDequeue(ExtensionBlock *, Bundle *, void *);
extern int	ipt_parse(AcqExtBlock *, AcqWorkArea *);
extern int	ipt_check(AcqExtBlock *, AcqWorkArea *);
extern void	ipt_clear(AcqExtBlock *);
