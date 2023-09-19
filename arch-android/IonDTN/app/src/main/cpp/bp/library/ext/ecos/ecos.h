/*
 *	ecos.h:		definitions supporting implementation of
 *			the Extended Class of Service (ECOS) block.
 *
 *	Copyright (c) 2008, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "bei.h" 

#define	EXTENSION_TYPE_ECOS	19

extern int	ecos_offer(ExtensionBlock *, Bundle *);
extern void	ecos_release(ExtensionBlock *);
extern int	ecos_record(ExtensionBlock *, AcqExtBlock *);
extern int	ecos_copy(ExtensionBlock *, ExtensionBlock *);
extern int	ecos_processOnFwd(ExtensionBlock *, Bundle *, void *);
extern int	ecos_processOnAccept(ExtensionBlock *, Bundle *, void *);
extern int	ecos_processOnEnqueue(ExtensionBlock *, Bundle *, void *);
extern int	ecos_processOnDequeue(ExtensionBlock *, Bundle *, void *);
extern int	ecos_parse(AcqExtBlock *, AcqWorkArea *);
extern int	ecos_check(AcqExtBlock *, AcqWorkArea *);
extern void	ecos_clear(AcqExtBlock *);
