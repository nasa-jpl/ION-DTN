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

extern PsmAddress	rfx_insert_contact(time_t fromTime,
				time_t toTime,
				uvast fromNode,
				uvast toNode,
				unsigned int xmitRate);
			/*	Creates a new IonContact object,
				inserts that object into the contacts
				list in the ION database, and returns
				the address of the IonCXref object for
				that contact.

				Returns zero on any error.		*/

extern char		*rfx_print_contact(PsmAddress contact, char *buffer);
			/*	Prints the indicated IonCXref
				object into buffer, which must be
				of length no less than RFX_NOTE_LEN.
				Returns buffer, or NULL on any error.	*/

extern int		rfx_remove_contact(time_t fromTime,
				uvast fromNode,
				uvast toNode);
			/*	Removes the indicated IonContact
				object from the time-ordered contacts
				list in the ION database.		*/

/*	*	Functions for inserting and removing range notes.	*/

extern PsmAddress	rfx_insert_range(time_t fromTime,
				time_t toTime,
				uvast fromNode,
				uvast toNode,
				unsigned int owlt);
			/*	Creates a new IonRange object,
				inserts that object into the ranges
				list in the ION database, and returns
				the address of the IonRXref entry for
				that range.

				Returns zero on any error.		*/

extern char		*rfx_print_range(PsmAddress range, char *buffer);
			/*	Prints the indicated IonRXref
				object into buffer, which must be
				of length no less than RFX_NOTE_LEN.
				Returns buffer, or NULL on any error.	*/

extern int		rfx_remove_range(time_t fromTime,
				uvast fromNode,
				uvast toNode);
			/*	Removes the indicated IonRange
				object from the time-ordered ranges
				list in the ION database.		*/

/*	*	Functions for controlling the rfxclock.			*/

extern int		rfx_start();
extern int		rfx_system_is_started();
extern void		rfx_stop();

/*	*	Additional database management functions.		*/

extern IonNeighbor	*findNeighbor(IonVdb *ionvdb, uvast nodeNbr,
				PsmAddress *nextElt);

extern IonNeighbor	*addNeighbor(IonVdb *ionvdb, uvast nodeNbr);

extern IonNode		*findNode(IonVdb *ionvdb, uvast nodeNbr,
				PsmAddress *nextElt);

extern IonNode		*addNode(IonVdb *ionvdb, uvast nodeNbr);

extern int		addSnub(IonNode *node, uvast neighborNodeNbr);

extern void		removeSnub(IonNode *node, uvast neighborNodeNbr);

extern PsmAddress	postProbeEvent(IonNode *node, IonSnub *snub);

#ifdef __cplusplus
}
#endif

#endif  /* _RFX_H_ */
