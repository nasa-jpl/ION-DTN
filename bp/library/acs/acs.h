/*
	acs.h: definitions supporting the implementation of 
		Aggregate Custody Signals (ACS) for the bundle protocol.

	Authors: Andrew Jenkins, Sebastian Kuzminsky, 
				University of Colorado at Boulder

	Copyright (c) 2008-2011, Regents of the University of Colorado.
	This work was supported by NASA contracts NNJ05HE10G, NNC06CB40C, and
	NNC07CB47C.
 											*/

#ifndef _ACS_H_
#define _ACS_H_

#include "bpP.h"

#ifdef __cplusplus
extern "C" {
#endif

/*	Administrative record type.					*/
#define	BP_AGGREGATE_CUSTODY_SIGNAL	(4)

extern int	acsInitialize(long heapWordsRequested, int loglevel);
	/* This function initializes the ACS structures stored in SDR. */

extern int	acsAttach(void);
	/* This function connects the calling task to the database of ACS
	 * information stored in SDR.	*/

extern void acsDetach(void);
	/* This function detaches the calling task from the ACS SDR. */

extern int	parseAggregateCtSignal(void **acsptr, unsigned char *cursor,
			int unparsedBytes, int bundleIsFragment);
	/* This function takes a delivery that identifies itself as an ACS,
	 * and parses it, storing the result in acs. */

extern int	handleAcs(void *acs, BpDelivery *dlv, CtSignalCB handleCtSignal);
	/* This function takes a parsed ACS and applies the abstract custody
	 * signals within the ACS to the handleCtSignal callback and the
	 * custody database. */

extern int	offerNoteAcs(Bundle *bundle, AcqWorkArea *work, char *dictionary,
			int succeeded, BpCtReason reasonCode);
	/* This function attempts to note that the local ION node is signalling
	 * custody transfer for bundle according to succeeded and reasonCode.
	 *
	 * If calling before extensions have been recorded to SDR, work should be
	 * a valid AcqWorkArea; otherwise, work should be NULL.
	 *
	 * If the previous custodian is known to support ACS, then offerNoteAcs
	 * will note this signal via ACS mechanisms and return 1.
	 *
	 * If the previous custodian is not known to support ACS, then
	 * offerNoteAcs will return 0, and the caller should deliver a normal
	 * custody signal instead (via sendCtSignal()).			*/

extern int	sendAcs(Object);
	/* This function sends a pending custody signal; it's called when
	 * this custody signal is due on the Nagled custody signal timeline. */

extern int	destroyAcsMetadata(Bundle *bundle);
	/* This function destroys the metadata that ACS is storing for a bundle.
	 * It should be called whenever a bundle is destroyed.  It returns 1 if
	 * metadata was found and freed, 0 if no metadata was found, and -1 if
	 * there is an error. */

extern int	updateCustodianAcsDelay(const char *custodianEid,
			unsigned long acsDelay);
	/* This function updates the database of information about custodians,
	 * in particular, how long to delay ACS destined for that custodian. */

extern int	updateCustodianAcsSize(const char *custodianEid,
			unsigned long acsSize);
	/* This function updates the database of information about custodians,
	 * in particular, at what size of pending ACS the local bundle agent
	 * should stop appending, serialize and send to custodianEid. */

extern int	updateMinimumCustodyId(unsigned int minimumCustodyId);
	/* This function updates the ACS database to use the new minimumCustodyId
	 * as the next available custody ID. */

extern void listCustodianInfo(void (*printer)(const char *));
	/* This function passes a string to printer() for each custodian for which
	 * information is known.  This string is the information known about that
	 * custodian in human-readable form. */

#ifdef __cplusplus
}	/* extern "C" */
#endif

	
#endif /* _ACS_H_ */
