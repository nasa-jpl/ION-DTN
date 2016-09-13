/*
 	ams.h:	definitions supporting the implementation of AMS
		(Asynchronous Message Service) modules.

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

#define	MAX_AMS_CONTENT		(65000)

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

typedef struct amssapst		*AmsModule;
typedef struct amsevtst		*AmsEvent;

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

typedef void		(*AmsMsgHandler)(AmsModule module,
					void *userData,
					AmsEvent *eventRef,
					int continuumNbr,
					int unitNbr,
					int moduleNbr,
					int subjectNbr,
					int contentLength,
					char *content,
					int context,
					AmsMsgType msgType,
					int priority,
					unsigned char flowLabel);

typedef void		(*AmsRegistrationHandler)(AmsModule module,
					void *userData,
					AmsEvent *eventRef,
					int unitNbr,
					int moduleNbr,
					int roleNbr);

typedef void		(*AmsUnregistrationHandler)(AmsModule module,
					void *userData,
					AmsEvent *eventRef,
					int unitNbr,
					int moduleNbr);

typedef void		(*AmsInvitationHandler)(AmsModule module,
					void *userData,
					AmsEvent *eventRef,
					int unitNbr,
					int moduleNbr,
					int domainRoleNbr,
					int domainContinuumNbr,
					int domainUnitNbr,
					int subjectNbr,
					int priority,
					unsigned char flowLabel,
					AmsSequence sequence,
					AmsDiligence diligence);

typedef void		(*AmsDisinvitationHandler)(AmsModule module,
					void *userData,
					AmsEvent *eventRef,
					int unitNbr,
					int moduleNbr,
					int domainRoleNbr,
					int domainContinuumNbr,
					int domainUnitNbr,
					int subjectNbr);

typedef void		(*AmsSubscriptionHandler)(AmsModule module,
					void *userData,
					AmsEvent *eventRef,
					int unitNbr,
					int moduleNbr,
					int domainRoleNbr,
					int domainContinuumNbr,
					int domainUnitNbr,
					int subjectNbr,
					int priority,
					unsigned char flowLabel,
					AmsSequence sequence,
					AmsDiligence diligence);

typedef void		(*AmsUnsubscriptionHandler)(AmsModule module,
					void *userData,
					AmsEvent *eventRef,
					int unitNbr,
					int moduleNbr,
					int domainRoleNbr,
					int domainContinuumNbr,
					int domainUnitNbr,
					int subjectNbr);

typedef void		(*AmsUserEventHandler)(AmsModule module,
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
					char *applicationName,
					char *authorityName,
					char *unitName,
					char *roleName,
					AmsModule *module);
			/*	Arguments are:
			 *		name of source medium for MIB
			 *		overriding transport service
			 *			selection order (this
			 *			is normally NULL)
		 	 *  		name of application in which
			 *  			this module wants to
			 *  			participate
		 	 *  		name of authority in charge of
			 *  			the venture in which
			 *  			this module wants to
			 *  			participate
			 *  		name of specific unit, in the
			 *  			venture identified by
			 *  			application and authority
			 *  			names, in which this module
			 *  			wants to participate
			 *		name of module's functional role
			 *			within the application;
			 *			need not be unique
			 *		pointer to variable in which
			 *			address of AMS module
			 *			state will be returned
			 *
			 *	Returns 0 on success, -1 on any error.	*/

extern int		ams_unregister(	AmsModule module);

extern int		ams_get_module_nbr(AmsModule module);

extern int		ams_get_unit_nbr(AmsModule module);

extern Lyst		ams_list_msgspaces(AmsModule module);

extern int		ams_get_continuum_nbr();

extern int		ams_rams_net_is_tree(AmsModule module);

extern int		ams_msgspace_is_neighbor(AmsModule module,
					int continuumNbr);

extern char		*ams_get_role_name(AmsModule module,
					int unitNbr,
					int moduleNbr);

extern int		ams_subunit_of(AmsModule module,
					int argUnitNbr,
					int refUnitNbr);

extern int		ams_lookup_unit_nbr(AmsModule module,
					char *unitName);

extern int		ams_lookup_role_nbr(AmsModule module,
					char *roleName);

extern int		ams_lookup_subject_nbr(AmsModule module,
					char *subjectName);

extern int		ams_lookup_continuum_nbr(AmsModule module,
					char *continuumName);

extern char		*ams_lookup_unit_name(AmsModule module,
					int unitNbr);

extern char		*ams_lookup_role_name(AmsModule module,
					int roleNbr);

extern char		*ams_lookup_subject_name(AmsModule module,
					int subjectNbr);

extern char		*ams_lookup_continuum_name(AmsModule module,
					int continuumNbr);

extern int		ams_invite(	AmsModule module,
					int roleNbr,
					int continuumNbr,
					int unitNbr,
					int subjectNbr,
					int priority,
					unsigned char flowLabel,
					AmsSequence sequence,
					AmsDiligence diligence);

extern int		ams_disinvite(	AmsModule module,
					int roleNbr,
					int continuumNbr,
					int unitNbr,
					int subjectNbr);

extern int		ams_subscribe(	AmsModule module,
					int roleNbr,
					int continuumNbr,
					int unitNbr,
					int subjectNbr,
					int priority,
					unsigned char flowLabel,
					AmsSequence sequence,
					AmsDiligence diligence);

extern int		ams_unsubscribe(AmsModule module,
					int roleNbr,
					int continuumNbr,
					int unitNbr,
					int subjectNbr);

extern int		ams_publish(	AmsModule module,
					int subjectNbr,
					int priority,
					unsigned char flowLabel,
					int contentLength,
					char *content,
					int context);

extern int		ams_send(	AmsModule module,
					int continuumNbr,
					int unitNbr,
					int moduleNbr,
					int subjectNbr,
					int priority,
					unsigned char flowLabel,
					int contentLength,
					char *content,
					int context);

extern int		ams_query(	AmsModule module,
					int continuumNbr,
					int unitNbr,
					int moduleNbr,
					int subjectNbr,
					int priority,
					unsigned char flowLabel,
					int contentLength,
					char *content,
					int context,
					int term,
					AmsEvent *event);

extern int		ams_reply(	AmsModule module,
					AmsEvent msg,
					int subjectNbr,
					int priority,
					unsigned char flowLabel,
					int contentLength,
					char *content);

extern int		ams_announce(	AmsModule module,
					int roleNbr,
					int continuumNbr,
					int unitNbr,
					int subjectNbr,
					int priority,
					unsigned char flowLabel,
					int contentLength,
					char *content,
					int context);

extern int		ams_post_user_event(AmsModule module,
					int userEventCode,
					int userEventDataLength,
					char *userEventData,
					int priority);

extern int		ams_get_event(	AmsModule module,
					int term,
					AmsEvent *event);

extern int		ams_get_event_type(AmsEvent event);

extern int		ams_parse_msg(	AmsEvent event,
					int *continuumNbr,
					int *unitNbr,
					int *moduleNbr,
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
					int *moduleNbr,
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

extern int		ams_set_event_mgr(AmsModule module,
					AmsEventMgt *rules);

extern void		ams_remove_event_mgr(AmsModule module);
#ifdef __cplusplus
}
#endif

#endif	/* _AMS_H */
