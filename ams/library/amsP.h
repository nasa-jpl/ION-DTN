/*
 	amsP.h:	private definitions supporting the implementation of
		Asynchronous Message Service.

	Author: Scott Burleigh, JPL

	Modification History:
	Date  Who What

	Copyright (c) 2005, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
 									*/
#ifndef _AMSP_H_
#define _AMSP_H_

#include "ams.h"
#include "amscommon.h"

#ifdef __cplusplus
extern "C" {
#endif

#define	NBR_OF_PRIORITY_LEVELS	(16)

/*	Flag values for XmitRule structures (message acceptance
 *	relationships).							*/

#define XMIT_IS_OKAY	0x00000001
#define RULE_CONFIRMED	0x00000002

/*	Rule type codes for XmitRule and MsgRule structures.		*/

#define SUBSCRIPTION	(1)
#define INVITATION	(2)

/*	*	Message transmission control structure	*	*	*/

/*	XmitRule regulates a message acceptance relationship, which is
 *	characterized by a module that accepts the messages on a given
 *	subject and the subject on which messages are accepted by that
 *	module.  The rule constrains senders of this messages, by role
 *	and/or unit, and it states the selected endpoint for sending
 *	messages on this subject to this module (based on the rule's
 *	implicit service mode, which was specified by the module), the
 *	priority and flow label applicable to transmissions governed
 *	by the rule, and whether or not the relationship is authorized.
 *
 *	There are two types of XmitRules: subscriptions and
 *	invitations.  They are identical in structure.
 *
 *	The XmitRules of either type, for a given message acceptance
 *	relationship (i.e., within a given FanModule and the
 *	corresponding SubjOfInterest) are ordered by descending
 *	stringency of role specification, i.e., role 0 ("all roles")
 *	is last.  The rules for a given role are ordered by ascending
 *	position in the unit hierarchy, i.e., unit 0 (the root unit)
 *	is last.
 *
 *	Note that, like the MsgRule structure, XmitRule notes the
 *	continuum number constraining the source of transmitted
 *	messages.  The XmitRules recorded by a module may well include
 *	rules that apply only to modules in other message spaces.  But
 *	knowing all these rules is consistent with the AMS design
 *	pattern of global distribution of global knowledge, and it
 *	is vital to the operation of RAMS gateways and venture
 *	visualization tools.						*/

typedef struct
{
	SubjOfInterest	*subject;	/*	Back-reference.		*/
	FanModule	*module;	/*	Back-reference.		*/
	int		roleNbr;	/*	Msg source constraint.	*/
	int		continuumNbr;	/*	Msg source constraint.	*/
	int		unitNbr;	/*	Msg source constraint.	*/
	int		priority;	/*	Requested by module.	*/
	unsigned char	flowLabel;	/*	Requested by module.	*/
	AmsEndpointP	amsEndpoint;	/*	Where to send messages.	*/
	int		flags;		/*	Booleans.		*/
} XmitRule;

/*	*	Message reception control structures	*	*	*/

/*	AmsInterface is a structure used by the LOCAL module (SAP) to
 *	send and receive AMS messages, using some transport service.	*/

typedef struct amsifst
{
	TransSvc	*ts;
	struct amssapst	*amsSap;	/*	Back-reference.		*/
	AmsDiligence	diligence;
	AmsSequence	sequence;
	char		*ept;		/*	Own endpoint name text.	*/
	Llcv		eventsQueue;
	pthread_t	receiver;
	void		*sap;		/*	e.g., socket FD		*/
} AmsInterface;

/*	An AmsEndpoint encapsulates the best endpoint for sending
 *	AMS messages to a REMOTE module on a specified service mode,
 *	as selected from among the endpoints in that module's delivery
 *	vector for that service mode, given the transport services
 *	that are locally available.					*/

typedef struct amsepst
{
	int		deliveryVectorNbr;
	AmsDiligence	diligence;	/*	Vector's service mode.	*/
	AmsSequence	sequence;	/*	Vector's service mode.	*/
	char		*ept;		/*	Raw, unparsed text.	*/
	void		*tsep;		/*	Xmit parms parsed out.	*/
	TransSvc	*ts;		/*	Use a tsif matching
						this ts when sending
						AMS msgs to this ep.	*/
} AmsEndpoint;

/*	A DeliveryVector lists the interfaces at which the LOCAL module
 *	(SAP) is prepared to receive messages at a given mode of
 *	service, in descending order of preference.  The listed
 *	interfaces are references to the interfaces in the SAP's
 *	interface pool - the amsTsifs array.				*/

typedef struct
{
	int		nbr;		/*	Vector number.		*/
	AmsDiligence	diligence;
	AmsSequence	sequence;
	Lyst		interfaces;	/*	(AmsInterface *)	*/
} DeliveryVector;

/*	MsgRule indicates which delivery vector (set of interfaces),
 *	priority, and flow label other modules are to use when sending
 *	messages on the indicated subject to the LOCAL module (the SAP)
 *	- if in fact such transmission is supported at all.  MsgRules
 *	are the source of the information encapsulated in other modules'
 *	XmitRules.
 *
 *	MsgRules are of two types - subscriptions and invitations -
 *	which are identical in structure.
 *
 *	The MsgRules for a unit are ordered by ascending subject
 *	number.  The rules for a given subject are ordered by
 *	descending stringency of role specification, i.e., role 0
 *	("all roles") is last.  The rules for a given subject/role are
 *	by descending stringency of continuum specification, i.e.,
 *	continuum 0 ("all continua") is last.  The rules for a given
 *	subject/role/continuum are ordered by ascending position in
 *	unit hierarchy, i.e., unit 0 (the root unit) is last.		*/

typedef struct
{
	Subject		*subject;
	int		roleNbr;	/*	Msg source constraint.	*/
	int		continuumNbr;	/*	Msg source constraint.	*/
	int		unitNbr;	/*	Msg source constraint.	*/
	int		priority;	/*	Requested by module.	*/
	int		flowLabel;	/*	Requested by module.	*/
	DeliveryVector	*vector;
} MsgRule;

/*	*	*	Service Access Point	*	*	*	*/

typedef enum
{
	AmsSapClosed = 0,
	AmsSapOpen,
	AmsSapCrashed
} AmsSapState;

typedef struct amssapst
{
	AmsSapState	state;
	pthread_t	primeThread;
	int		transportServiceCount;
	TransSvc	*transportServices[TS_INDEX_LIMIT + 1];
	AmsEventMgt	eventMgtRules;
	pthread_t	eventMgr;
	pthread_t	authorizedEventMgr;		

	Venture		*venture;
	Unit		*unit;
	AppRole		*role;			/*	In application.	*/
	MamsEndpoint	*csEndpoint;		/*	Config. server.	*/
	LystElt		csEndpointElt;
	MamsEndpoint	*rsEndpoint;		/*	Registrar.	*/
	int		moduleNbr;
	int		heartbeatsMissed;	/*	From registrar.	*/
	Lyst		delivVectors;		/*	(D...Vector *)	*/
	Lyst		subscriptions;		/*	(MsgRule *)	*/
	Lyst		invitations;		/*	(MsgRule *)	*/

	pthread_t	heartbeatThread;
	int		haveHeartbeatThread;
	pthread_t	mamsThread;
	int		haveMamsThread;

	Lyst		mamsEvents;		/*	(AmsEvt *)	*/
	struct llcv_str	mamsEventsCV_str;
	Llcv		mamsEventsCV;		/*	Inbound.	*/
	MamsInterface	mamsTsif;

	Lyst		amsEvents;		/*	(AmsEvt *)	*/
	struct llcv_str	amsEventsCV_str;
	Llcv		amsEventsCV;		/*	Inbound.	*/
	AmsInterface	amsTsifs[TS_INDEX_LIMIT + 1];
	LystElt		lastForPriority[NBR_OF_PRIORITY_LEVELS];
} AmsSAP;

/*	*	*	AMS event structures	*	*	*	*/

typedef struct
{
	int		continuumNbr;
	int		unitNbr;
	short		moduleNbr;
	short		subjectNbr;
	int		contextNbr;
	AmsMsgType	type;
	int		contentLength;
	char		*content;
	unsigned char	priority;
	unsigned char	flowLabel;
} AmsMsg;

typedef struct
{
	AmsStateType	stateType;
	AmsChangeType	changeType;
	int		unitNbr;
	int		moduleNbr;
	int		roleNbr;
	int		domainContinuumNbr;
	int		domainUnitNbr;
	int		subjectNbr;
	int		priority;
	unsigned char	flowLabel;
	AmsSequence	sequence;
	AmsDiligence	diligence;
} AmsNotice;

/*	*	*	Private function prototypes	*	*	*/

extern int	enqueueAmsMsg(AmsSAP *sap, unsigned char *buffer, int length);

#ifdef __cplusplus
}
#endif

#endif	/* _AMSP_H */
