/*
 	ams.h:	definitions supporting the implementation of AMS
		(Asynchronous Message Service) nodes.

	Author: Scott Burleigh, JPL

	Modification History:
	Date  Who What

	Copyright (c) 2005, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
 									*/
#ifndef _AMS_H_
#define _AMS_H_

#include "platform.h"
#include "lyst.h"

#ifdef __cplusplus
extern "C" {
#endif

#define THIS_CONTINUUM		(-1)
#define ALL_CONTINUA		(0)
#define ANY_CONTINUUM		(0)
#define ALL_SUBJECTS		(0)
#define ANY_SUBJECT		(0)
#define ALL_ROLES		(0)
#define ANY_ROLE		(0)

typedef enum
{
	AmsArrivalOrder = 0,
	AmsTransmissionOrder
} AmsSequence;

typedef enum
{
	AmsBestEffort = 0,
	AmsAssured
} AmsDiligence;

typedef struct amssapst	*AmsNode;
typedef struct amsevtst	*AmsEvent;

/*	AMS event types.						*/
#define	AMS_MSG_EVT		1
#define	TIMEOUT_EVT		2
#define	NOTICE_EVT		3
#define	USER_DEFINED_EVT	4

typedef enum
{
	AmsRegistrationState,
	AmsInvitationState,
	AmsSubscriptionState
} AmsStateType;

typedef enum
{
	AmsStateBegins = 1,
	AmsStateEnds
} AmsChangeType;

typedef enum
{
	AmsMsgUnary = 0,
	AmsMsgQuery,
	AmsMsgReply,
	AmsMsgNone
} AmsMsgType;

typedef void		(*AmsMsgHandler)(AmsNode node,
					void *userData,
					AmsEvent *eventRef,
					int continuumNbr,
					int unitNbr,
					int nodeNbr,
					int subjectNbr,
					int contentLength,
					char *content,
					int context,
					AmsMsgType msgType,
					int priority,
					unsigned char flowLabel);

typedef void		(*AmsRegistrationHandler)(AmsNode node,
					void *userData,
					AmsEvent *eventRef,
					int unitNbr,
					int nodeNbr,
					int roleNbr);

typedef void		(*AmsUnregistrationHandler)(AmsNode node,
					void *userData,
					AmsEvent *eventRef,
					int unitNbr,
					int nodeNbr);

typedef void		(*AmsInvitationHandler)(AmsNode node,
					void *userData,
					AmsEvent *eventRef,
					int unitNbr,
					int nodeNbr,
					int domainRoleNbr,
					int domainContinuumNbr,
					int domainUnitNbr,
					int subjectNbr,
					int priority,
					unsigned char flowLabel,
					AmsSequence sequence,
					AmsDiligence diligence);

typedef void		(*AmsDisinvitationHandler)(AmsNode node,
					void *userData,
					AmsEvent *eventRef,
					int unitNbr,
					int nodeNbr,
					int domainRoleNbr,
					int domainContinuumNbr,
					int domainUnitNbr,
					int subjectNbr);

typedef void		(*AmsSubscriptionHandler)(AmsNode node,
					void *userData,
					AmsEvent *eventRef,
					int unitNbr,
					int nodeNbr,
					int domainRoleNbr,
					int domainContinuumNbr,
					int domainUnitNbr,
					int subjectNbr,
					int priority,
					unsigned char flowLabel,
					AmsSequence sequence,
					AmsDiligence diligence);

typedef void		(*AmsUnsubscriptionHandler)(AmsNode node,
					void *userData,
					AmsEvent *eventRef,
					int unitNbr,
					int nodeNbr,
					int domainRoleNbr,
					int domainContinuumNbr,
					int domainUnitNbr,
					int subjectNbr);

typedef void		(*AmsUserEventHandler)(AmsNode node,
					void *userData,
					AmsEvent *eventRef,
					int code,
					int dataLength,
					char *data);

typedef void		(*AmsMgtErrHandler)(void *userData,
					AmsEvent *eventRef);

typedef struct
{
	AmsMsgHandler			msgHandler;
	void				*msgHandlerUserData;
	AmsRegistrationHandler		registrationHandler;
	void				*registrationHandlerUserData;
	AmsUnregistrationHandler	unregistrationHandler;
	void				*unregistrationHandlerUserData;
	AmsInvitationHandler		invitationHandler;
	void				*invitationHandlerUserData;
	AmsDisinvitationHandler		disinvitationHandler;
	void				*disinvitationHandlerUserData;
	AmsSubscriptionHandler		subscriptionHandler;
	void				*subscriptionHandlerUserData;
	AmsUnsubscriptionHandler	unsubscriptionHandler;
	void				*unsubscriptionHandlerUserData;
	AmsUserEventHandler		userEventHandler;
	void				*userEventHandlerUserData;
	AmsMgtErrHandler		errHandler;
	void				*errHandlerUserData;
} AmsEventMgt;

/*	Predefined term values for ams_query and ams_get_event.		*/
#define	AMS_POLL	(0)		/*	Return immediately.	*/
#define AMS_BLOCKING	(-1)		/*	Wait forever.		*/

extern int		ams_register(	char *mibSource,
					char *tsorder,
					char *mName,
					char *memory,
					unsigned mSize,
					char *applicationName,
					char *authorityName,
					char *unitName,
					char *roleName,
					AmsNode *node);
			/*	Arguments are:
			 *		name of source medium for MIB
			 *		overriding transport service
			 *			selection order (this
			 *			is normally NULL)
			 *		name of memory manager to be
			 *			used for AMS
			 *		size of private memory area to
			 *			be used for AMS
			 *		preallocated private memory to
			 *			be used for AMS
		 	 *  		name of application in which
			 *  			this node wants to
			 *  			participate
		 	 *  		name of authority in charge of
			 *  			the venture in which
			 *  			this node wants to
			 *  			participate
			 *  		name of specific unit, in the
			 *  			venture identified by
			 *  			application and authority
			 *  			names, in which this node
			 *  			wants to participate
			 *		name of node's functional role
			 *			within the application;
			 *			need not be unique
			 *		pointer to variable in which
			 *			address of AMS node
			 *			state will be returned
			 *
			 *	If mibSource is NULL, it defaults to
			 *	roleName.  mibSource is used to locate
			 *	the MIB for AMS nodes identified by
			 *	roleName; the MIB may reside in a file
			 *	in the current working directory, or
			 *	it may reside in an SDR, depending on
			 *	which MIB-loading module is linked in.
			 *
			 *	If mName is NULL, DRAM will be
			 *	dynamically allocated from system
			 *	memory (malloc/free) as needed.
			 *	Otherwise, DRAM will be dynamically
			 *	allocated (as needed) using the named
			 *	memory manager; if this memory manager
			 *	has not yet been initialized, the named
			 *	memory manager will be initialized to
			 *	allocate memory from the preallocated
			 *	memory pool identified by "memory"; if
			 *	"memory" is NULL, then a pool of size
			 *	mSize will be dynamically allocated
			 *	for this purposed from system memory
			 *	[just once, at initialization].
			 *
			 *	Returns 0 on success, -1 on any error.	*/

extern int		ams_unregister(	AmsNode node);

extern int		ams_get_node_nbr(AmsNode node);

extern int		ams_get_unit_nbr(AmsNode node);

extern Lyst		ams_list_msgspaces(AmsNode node);

extern int		ams_get_continuum_nbr();

extern int		ams_rams_net_is_tree();

extern int		ams_continuum_is_neighbor(int continuumNbr);

extern char		*ams_get_role_name(AmsNode node,
					int unitNbr,
					int nodeNbr);

extern int		ams_subunit_of(AmsNode node,
					int argUnitNbr,
					int refUnitNbr);

extern int		ams_lookup_unit_nbr(AmsNode node,
					char *unitName);

extern int		ams_lookup_role_nbr(AmsNode node,
					char *roleName);

extern int		ams_lookup_subject_nbr(AmsNode node,
					char *subjectName);

extern int		ams_lookup_continuum_nbr(AmsNode node,
					char *continuumName);

extern char		*ams_lookup_unit_name(AmsNode node,
					int unitNbr);

extern char		*ams_lookup_role_name(AmsNode node,
					int roleNbr);

extern char		*ams_lookup_subject_name(AmsNode node,
					int subjectNbr);

extern char		*ams_lookup_continuum_name(AmsNode node,
					int continuumNbr);

extern int		ams_invite(	AmsNode node,
					int roleNbr,
					int continuumNbr,
					int unitNbr,
					int subjectNbr,
					int priority,
					unsigned char flowLabel,
					AmsSequence sequence,
					AmsDiligence diligence);

extern int		ams_disinvite(	AmsNode node,
					int roleNbr,
					int continuumNbr,
					int unitNbr,
					int subjectNbr);

extern int		ams_subscribe(	AmsNode node,
					int roleNbr,
					int continuumNbr,
					int unitNbr,
					int subjectNbr,
					int priority,
					unsigned char flowLabel,
					AmsSequence sequence,
					AmsDiligence diligence);

extern int		ams_unsubscribe(AmsNode node,
					int roleNbr,
					int continuumNbr,
					int unitNbr,
					int subjectNbr);

extern int		ams_publish(	AmsNode node,
					int subjectNbr,
					int priority,
					unsigned char flowLabel,
					int contentLength,
					char *content,
					int context);

extern int		ams_send(	AmsNode node,
					int continuumNbr,
					int unitNbr,
					int nodeNbr,
					int subjectNbr,
					int priority,
					unsigned char flowLabel,
					int contentLength,
					char *content,
					int context);

extern int		ams_query(	AmsNode node,
					int continuumNbr,
					int unitNbr,
					int nodeNbr,
					int subjectNbr,
					int priority,
					unsigned char flowLabel,
					int contentLength,
					char *content,
					int context,
					int term,
					AmsEvent *event);

extern int		ams_reply(	AmsNode node,
					AmsEvent msg,
					int subjectNbr,
					int priority,
					unsigned char flowLabel,
					int contentLength,
					char *content);

extern int		ams_announce(	AmsNode node,
					int roleNbr,
					int continuumNbr,
					int unitNbr,
					int subjectNbr,
					int priority,
					unsigned char flowLabel,
					int contentLength,
					char *content,
					int context);

extern int		ams_post_user_event(AmsNode node,
					int userEventCode,
					int userEventDataLength,
					char *userEventData,
					int priority);

extern int		ams_get_event(	AmsNode node,
					int term,
					AmsEvent *event);

extern int		ams_get_event_type(AmsEvent event);

extern int		ams_parse_msg(	AmsEvent event,
					int *continuumNbr,
					int *unitNbr,
					int *nodeNbr,
					int *subjectNbr,
					int *contentLength,
					char **content,
					int *context,
					AmsMsgType *msgType,
					int *priority,
					unsigned char *flowLabel);

extern int		ams_parse_notice(AmsEvent event,
					AmsStateType *state,
					AmsChangeType *change,
					int *unitNbr,
					int *nodeNbr,
					int *roleNbr,
					int *domainContinuumNbr,
					int *domainUnitNbr,
					int *subjectNbr,
					int *priority,
					unsigned char *flowLabel,
					AmsSequence *sequence,
					AmsDiligence *diligence);

extern int		ams_parse_user_event(AmsEvent event,
					int *code,
					int *dataLength,
					char **data);

extern int		ams_recycle_event(AmsEvent event);

extern int		ams_set_event_mgr(AmsNode node,
					AmsEventMgt *rules);

extern void		ams_remove_event_mgr(AmsNode node);
#ifdef __cplusplus
}
#endif

#endif	/* _AMS_H */
