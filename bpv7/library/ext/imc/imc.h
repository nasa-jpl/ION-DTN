/*
 *	imc.h:		definitions supporting implementation of
 *			the IPN Multicast (IMC) block, which forwards
 *			the distribution of a multicast bundle to
 *			the members of the applicable multicast group.
 *
 *	Copyright (c) 2020, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "bei.h" 

extern int	imc_offer(ExtensionBlock *, Bundle *);
extern int	imc_serialize(ExtensionBlock *, Bundle *);
extern void	imc_release(ExtensionBlock *);
extern int	imc_record(ExtensionBlock *, AcqExtBlock *);
extern int	imc_copy(ExtensionBlock *, ExtensionBlock *);
extern int	imc_processOnFwd(ExtensionBlock *, Bundle *, void *);
extern int	imc_processOnAccept(ExtensionBlock *, Bundle *, void *);
extern int	imc_processOnEnqueue(ExtensionBlock *, Bundle *, void *);
extern int	imc_processOnDequeue(ExtensionBlock *, Bundle *, void *);
extern int	imc_parse(AcqExtBlock *, AcqWorkArea *);
extern int	imc_check(AcqExtBlock *, AcqWorkArea *);
extern void	imc_clear(AcqExtBlock *);
