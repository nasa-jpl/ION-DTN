/** \file contacts.h
 *
 *  \brief  This file provides the definition of the Contact type,
 *          of the ContactNode type and of the CtType, with all the declarations
 *          of the functions to manage the contact graph
 *
 *
 ** \copyright Copyright (c) 2020, Alma Mater Studiorum, University of Bologna, All rights reserved.
 **
 ** \par License
 **
 **    This file is part of Unibo-CGR.                                            <br>
 **                                                                               <br>
 **    Unibo-CGR is free software: you can redistribute it and/or modify
 **    it under the terms of the GNU General Public License as published by
 **    the Free Software Foundation, either version 3 of the License, or
 **    (at your option) any later version.                                        <br>
 **    Unibo-CGR is distributed in the hope that it will be useful,
 **    but WITHOUT ANY WARRANTY; without even the implied warranty of
 **    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **    GNU General Public License for more details.                               <br>
 **                                                                               <br>
 **    You should have received a copy of the GNU General Public License
 **    along with Unibo-CGR.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  \author Lorenzo Persampieri, lorenzo.persampieri@studio.unibo.it
 *
 *  \par Supervisor
 *       Carlo Caini, carlo.caini@unibo.it
 */

#ifndef SOURCES_CONTACTS_PLAN_CONTACTS_CONTACTS_H_
#define SOURCES_CONTACTS_PLAN_CONTACTS_CONTACTS_H_

#include <sys/time.h>
#include "../../library/commonDefines.h"
#include "../../library/list/list_type.h"
#include "../../library_from_ion/rbt/rbt_type.h"

#ifndef REVISABLE_CONFIDENCE
/**
 * \brief Set to 1 if you want to permits that contact's confidence can be changed.
 *        Set to 0 otherwise.
 */
#define REVISABLE_CONFIDENCE 1
#endif

#ifndef REVISABLE_XMIT_RATE
/**
 * \brief Set to 1 if you want to permits that contact's xmitRate (and MTVs) can be changed.
 *        Set to 0 otherwise.
 *
 * \par Notes:
 *          1. I suggest you to set this macro to 1 if you don't read (and update) the contact plan
 *             directly from a file but you read the contacts from another C struct by BP interface.
 */
#define REVISABLE_XMIT_RATE 1
#endif

#if (REVISABLE_CONFIDENCE && REVISABLE_XMIT_RATE)
#undef REVISABLE_CONTACT
#define REVISABLE_CONTACT 1
#endif

#define COPY_MTV 1
#define DO_NOT_COPY_MTV 0

typedef struct cgrContactNote ContactNote;

typedef enum
{
	TypeRegistration = 1,
	TypeScheduled,
	TypeSuppressed,
	TypePredicted,
	TypeHypothetical,
	TypeDiscovered
} CtType;

typedef struct
{
    /**
     * \brief Common region of both sender and receiver
     */
     unsigned long regionNbr;
	/**
	 * \brief Sender node (ipn node number)
	 */
	unsigned long long fromNode;
	/**
	 * \brief Receiver node (ipn node number)
	 */
	unsigned long long toNode;
	/**
	 * \brief Start transit time
	 */
	time_t fromTime;
	/**
	 * \brief Stop transmit time
	 */
	time_t toTime;
	/**
	 * \brief In bytes per second
	 */
	long unsigned int xmitRate;
	/**
	 * \brief Confidence that the contact will materialize
	 */
	float confidence;
	/**
	 * \brief Registration or Scheduled
	 */
	CtType type;
	/**
	 * \brief Remaining volume (for each level of priority)
	 */
	double mtv[3];
	/**
	 * \brief Used by Dijkstra's search
	 */
	ContactNote *routingObject;
	/**
	 * \brief List of ListElt data.
	 *
	 * \details Each citation is a pointer to the element
	 * of the hops list of the Route where the contact appears,
	 * and this element of the hops list points to this contact.
	 */
	List citations;
} Contact;

struct cgrContactNote
{
	/**
	 * \brief Previous contact in the route, used to reconstruct the route at the end of
	 * the Dijkstra's search
	 */
	Contact *predecessor;
	/**
	 * \brief Best case arrival time to the toNode of the contact
	 */
	time_t arrivalTime;
	/**
	 * \brief Boolean used to identify each contact that belongs to the visited set
	 *
	 * \details Values
	 *          -  1  Contact already visited
	 *          -  0  Contact not visited
	 */
	int visited;
	/**
	 * \brief Flag used to identify each contact that belongs to the excluded set
	 */
	int suppressed;
	/**
	 * \brief Ranges sum to reach the toNode
	 */
	unsigned int owltSum;
	/**
	 * \brief Number of hops to reach this contact during Dijkstra's search.
	 */
	unsigned int hopCount;
	/**
	 * \brief Product of the confidence of each contacts in the path to
	 * reach this contact and of the contact's confidence itself
	 */
	float arrivalConfidence;
	/**
	 * \brief Flag to known if we already get the range for this contact during the Dijkstra's search.
	 *
	 * \details Values:
	 *          -  1  Range found at contact's start time
	 *          -  0  Range has yet to be searched
	 *          - -1  Range not found at the contact's start time
	 *
	 * \par Notes:
	 *          1.  Phase one always looks for the range at contact's start time.
	 */
	int rangeFlag;
	/**
	 * \brief The owlt of the range found.
	 */
	unsigned int owlt;
	/**
	 * \brief
	 */
	Contact *nextContactInDijkstraQueue;
};

#ifdef __cplusplus
extern "C"
{
#endif

extern int compare_contacts(void *first, void *second);
extern Contact* create_contact(unsigned long regionNbr, unsigned long long fromNode, unsigned long long toNode,
		time_t fromTime, time_t toTime, long unsigned int xmitRate, float confidence, CtType type);
extern void free_contact(void*);

extern int create_ContactsGraph();
extern void destroy_ContactsGraph();
extern void reset_ContactsGraph();

extern void removeExpiredContacts(time_t time);
extern void remove_contact_from_graph(unsigned long regionNbr, time_t *fromTime, unsigned long long fromNode,
		unsigned long long toNode);
extern void remove_contact_elt_from_graph(Contact *elt);
int add_contact_to_graph(unsigned long regionNbr, unsigned long long fromNode, unsigned long long toNode, time_t fromTime,
		time_t toTime, long unsigned int xmitRate, float confidence, int copyMTV, double mtv[]);
extern void discardAllRoutesFromContactsGraph();

extern Contact* get_contact(unsigned long regionNbr, unsigned long long fromNode, unsigned long long toNode, time_t fromTime,
		RbtNode **node);
extern Contact* get_first_contact(RbtNode **node);
extern Contact* get_first_contact_from_node(unsigned long regionNbr, unsigned long long fromNodeNbr, RbtNode **node);
extern Contact* get_first_contact_from_node_to_node(unsigned long regionNbr, unsigned long long fromNodeNbr,
		unsigned long long toNodeNbr, RbtNode **node);
extern Contact* get_next_contact(RbtNode **node);
extern Contact* get_prev_contact(RbtNode **node);
extern Contact * get_contact_with_time_tolerance(unsigned long regionNbr, unsigned long long fromNode, unsigned long long toNode, time_t fromTime, unsigned int tolerance);

#if REVISABLE_CONFIDENCE
extern int revise_confidence(unsigned long regionNbr, unsigned long long fromNode, unsigned long long toNode, time_t fromTime, float newConfidence);
#endif
#if REVISABLE_XMIT_RATE
extern int revise_xmit_rate(unsigned long regionNbr, unsigned long long fromNode, unsigned long long toNode, time_t fromTime, unsigned long int xmitRate, int copyMTV, double mtv[]);
#endif
#if REVISABLE_CONTACT
extern int revise_contact(unsigned long regionNbr, unsigned long long fromNode, unsigned long long toNode, time_t fromTime, float newConfidence, unsigned long int xmitRate, int copyMTV, double mtv[]);
#endif

extern int refill_mtv(unsigned long regionNbr, unsigned long long fromNode, unsigned long long toNode, time_t fromTime, unsigned int tolerance, unsigned int refillSize, int priority);

extern List get_known_regions();

#if (LOG == 1)
extern int printContactsGraph(FILE *file, time_t currentTime);
#else
#define printContactsGraph(file, currentTime) do {  } while(0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* SOURCES_CONTACTS_PLAN_CONTACTS_CONTACTS_H_ */
