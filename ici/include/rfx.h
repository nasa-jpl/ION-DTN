/*

	rfx.h:	definition of the application programming interface
		for managing ION's time-ordered lists of contacts
		and ranges.

	Copyright (c) 2007, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/
#ifndef _RFX_H_
#define _RFX_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ion.h"

#define	RFX_NOTE_LEN	144

/*	*	Functions for inserting and removing contact notes.	*/

extern Object		rfx_insert_contact(time_t fromTime,
				time_t toTime,
				unsigned long fromNode,
				unsigned long toNode,
				unsigned long xmitRate);
			/*	Creates a new IonContact object,
				inserts that object into the time-
				ordered contacts list in the ION
				database, and returns the list element
				resulting from that insertion.

				(NOTE: in order to print this contact
				you have to use sdr_list_data() to
				extract the address of the note
				from the contact list element object.)

				Returns zero on any error.		*/

extern char		*rfx_print_contact(Object contact, char *buffer);
			/*	Prints the indicated IonContact
				object into buffer, which must be
				of length no less than RFX_NOTE_LEN.
				Returns buffer, or NULL on any error.	*/

extern int		rfx_remove_contact(time_t fromTime,
				unsigned long fromNode,
				unsigned long toNode);
			/*	Removes the indicated IonContact
				object from the time-ordered contacts
				list in the ION database.		*/

/*	*	Functions for inserting and removing range notes.	*/

extern Object		rfx_insert_range(time_t fromTime,
				time_t toTime,
				unsigned long fromNode,
				unsigned long toNode,
				unsigned int owlt);
			/*	Creates a new IonRange object,
				inserts that object into the time-
				ordered ranges list in the ION
				database, and returns the list element
				resulting from that insertion.

				(NOTE: in order to print this range
				you have to use sdr_list_data() to
				extract the address of the note
				from the range list element object.)

				Returns zero on any error.		*/

extern char		*rfx_print_range(Object range, char *buffer);
			/*	Prints the indicated IonRange
				object into buffer, which must be
				of length no less than RFX_NOTE_LEN.
				Returns buffer, or NULL on any error.	*/

extern int		rfx_remove_range(time_t fromTime,
				unsigned long fromNode,
				unsigned long toNode);
			/*	Removes the indicated IonRange
				object from the time-ordered ranges
				list in the ION database.		*/

/*	*	Functions for controlling the rfxclock.			*/

extern int		rfx_start();
extern int		rfx_system_is_started();
extern void		rfx_stop();

/*	*	Additional database management functions.		*/

extern IonNeighbor	*findNeighbor(IonVdb *ionvdb, unsigned long nodeNbr,
				PsmAddress *nextElt);

extern IonNeighbor	*addNeighbor(IonVdb *ionvdb, unsigned long nodeNbr,
				PsmAddress nextElt);

extern IonNode		*findNode(IonVdb *ionvdb, unsigned long nodeNbr,
				PsmAddress *nextElt);

extern IonNode		*addNode(IonVdb *ionvdb, unsigned long nodeNbr,
				PsmAddress nextElt);

extern IonOrigin	*findOrigin(IonNode *node,
				unsigned long neighborNodeNbr,
				PsmAddress *nextElt);

extern IonOrigin	*addOrigin(IonNode *node,
				unsigned long neighborNodeNbr,
				PsmAddress nextElt);

extern void		forgetXmit(IonNode *node, IonContact *contact);

extern int		addSnub(IonNode *node, unsigned long neighborNodeNbr);

extern void		removeSnub(IonNode *node,
				unsigned long neighborNodeNbr);

extern PsmAddress	postProbeEvent(IonNode *node, IonSnub *snub);

extern int		checkForCongestion();

#ifdef __cplusplus
}
#endif

#endif  /* _RFX_H_ */
