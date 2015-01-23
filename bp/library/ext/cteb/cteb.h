/*
 *	cteb.h:		definitions supporting implementation of
 *			the Custody Transfer Enhancement Block (CTEB).
 *
 *  Copyright (c) 2010-2011, Regents of the University of Colorado.
 *  This work was supported by NASA contracts NNJ05HE10G, NNC06CB40C, and
 *  NNC07CB47C.
 *
 *	Author: Andrew Jenkins, University of Colorado
 */

#ifndef _CTEB_H_
#define _CTEB_H_

#include "bei.h"

#define	EXTENSION_TYPE_CTEB	10

typedef struct {
    unsigned int id;
} CtebScratchpad;

extern int	cteb_offer(ExtensionBlock *, Bundle *);
extern void	cteb_release(ExtensionBlock *);
extern int	cteb_record(ExtensionBlock *, AcqExtBlock *);
extern int	cteb_copy(ExtensionBlock *, ExtensionBlock *);
extern int	cteb_processOnFwd(ExtensionBlock *, Bundle *, void *);
extern int	cteb_processOnAccept(ExtensionBlock *, Bundle *, void *);
extern int	cteb_processOnEnqueue(ExtensionBlock *, Bundle *, void *);
extern int	cteb_processOnDequeue(ExtensionBlock *, Bundle *, void *);
extern int	cteb_parse(AcqExtBlock *, AcqWorkArea *);
extern int	cteb_check(AcqExtBlock *, AcqWorkArea *);
extern void	cteb_clear(AcqExtBlock *);

/* Looks in the bundle's extensions for a valid custody transfer enhancement
 * block.  If one is found, load the custody id.
 *
 * If calling before extensions have been recorded to SDR, AcqWorkArea should
 * be valid; otherwise, it should be NULL.
 *
 * Returns -1 if no valid CTEB is found. */
extern int  loadCtebScratchpad(Sdr, Bundle *, AcqWorkArea *, CtebScratchpad *);

#endif /* _CTEB_H_ */
