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

/*	*	Red-black tree ordering and deletion functions.	*	*/

extern int	rfx_order_ranges(PsmPartition partition, PsmAddress nodeData,
			void *dataBuffer);

extern int	rfx_order_contacts(PsmPartition partition, PsmAddress nodeData,
			void *dataBuffer);

extern int	rfx_order_events(PsmPartition partition, PsmAddress nodeData,
			void *dataBuffer);

extern void	rfx_erase_data(PsmPartition partition, PsmAddress nodeData,
			void *argument);

/*	*	Functions for inserting and removing contact notes.	*/

extern int		rfx_insert_contact(int regionIdx,
				time_t fromTime,
				time_t toTime,
				uvast fromNode,
				uvast toNode,
				size_t xmitRate,
				float confidence,
				PsmAddress *cxaddr);
			/*	Creates a new IonContact object,
				inserts that object into the contacts
				list of the applicable region in the
				ION database, and notes the address
				of the IonCXref object for that
				contact.  A toTime value of zero
				indicates that this is a "discovered"
				contact, for which the actual toTime
				on the database will be MAX_POSIX_TIME.
				The new IonCXref object address is zero
				if the contact was rejected.

				Returns zero on success, -1 on any
				system error.				*/

extern char		*rfx_print_contact(PsmAddress contact, char *buffer);
			/*	Prints the indicated IonCXref
				object into buffer, which must be
				of length no less than RFX_NOTE_LEN.
				Returns buffer, or NULL on any error.	*/

extern int		rfx_revise_contact(time_t fromTime,
				uvast fromNode,
				uvast toNode,
				size_t xmitRate,
				float confidence);
			/*	Revises the xmitRate of and possibly
			 *	our confidence in an existing contact.	*/

extern int		rfx_remove_contact(time_t *fromTime,
				uvast fromNode,
				uvast toNode);
			/*	Removes the indicated IonContact
				object from the time-ordered contacts
				list in the ION database.		*/

/*	*	Functions for inserting and removing range notes.	*/

extern int		rfx_insert_range(time_t fromTime,
				time_t toTime,
				uvast fromNode,
				uvast toNode,
				unsigned int owlt,
				PsmAddress *cxaddr);
			/*	Creates a new IonRange object,
				inserts that object into the ranges
				list in the ION database, and notes
				the address of the IonRXref entry for
				that range.  The new IonRXref object
				address is zero if the range was
				rejected.

				Returns zero on success, -1 on any
				system error.				*/

extern char		*rfx_print_range(PsmAddress range, char *buffer);
			/*	Prints the indicated IonRXref
				object into buffer, which must be
				of length no less than RFX_NOTE_LEN.
				Returns buffer, or NULL on any error.	*/

extern int		rfx_remove_range(time_t *fromTime,
				uvast fromNode,
				uvast toNode);
			/*	Removes the indicated IonRange
				object from the time-ordered ranges
				list in the ION database.		*/

/*	*	Functions for inserting and removing alarms.		*/

extern PsmAddress	rfx_insert_alarm(unsigned int term,
				unsigned int cycles);
			/*	Creates a new alarm object, inserts
			 *	a timeline event referencing that
			 *	object, and returns the alarm object's
			 *	address. Returns zero on any error.
			 *
			 *	If cycles is zero, the alarm recurs
			 *	indefinitely until it is removed.	*/

extern int		rfx_alarm_raised(PsmAddress alarmAddr);
			/*	Waits until alarm timeout expires,
			 *	then returns 1.  On any error, or
			 *	if the alarm is removed before its
			 *	timeout expires, returns 0.		*/

extern int		rfx_remove_alarm(PsmAddress alarmAddr);
			/*	Removes the indicated alarm from the
			 *	events timeline in the ION database.	*/

/*	*	Functions for controlling the rfxclock.			*/

extern int		rfx_start();
extern int		rfx_system_is_started();
extern void		rfx_stop();

/*	*	Additional database management functions.		*/

extern void		rfx_contact_state(uvast nodeNbr, size_t *secRemaining,
				size_t *xmitRate);

extern IonNeighbor	*findNeighbor(IonVdb *ionvdb, uvast nodeNbr,
				PsmAddress *nextElt);

extern IonNeighbor	*addNeighbor(IonVdb *ionvdb, uvast nodeNbr);

extern IonNeighbor	*getNeighbor(IonVdb *ionvdb, uvast nodeNbr);

extern IonNode		*findNode(IonVdb *ionvdb, uvast nodeNbr,
				PsmAddress *nextElt);

extern IonNode		*addNode(IonVdb *ionvdb, uvast nodeNbr);

extern int		addEmbargo(IonNode *node, uvast neighborNodeNbr);

extern void		removeEmbargo(IonNode *node, uvast neighborNodeNbr);

extern PsmAddress	postProbeEvent(IonNode *node, Embargo *embargo);

#ifdef __cplusplus
}
#endif

#endif  /* _RFX_H_ */
