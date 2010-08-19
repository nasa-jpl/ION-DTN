/*
 	amscommon.h:	common definitions shared by libams, amsd,
			and the transport service adapters.

	Author: Scott Burleigh, JPL

	Modification History:
	Date  Who What

	Copyright (c) 2005, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
 									*/
#ifndef _AMSCOMMON_H_
#define _AMSCOMMON_H_

#include "platform.h"
#include "memmgr.h"
#include "psm.h"
#include "lyst.h"
#include "llcv.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INVITE_LEN	(9)
#define SUBSCRIBE_LEN	(9)

#define	MAX_APP_NAME	32
#define	MAX_AUTH_NAME	32
#define	MAX_UNIT_NAME	32
#define	MAX_ROLE_NAME	32
#define	MAX_ELEM_NAME	32
#define	MAX_SUBJ_NAME	32
#define	MAX_EP_NAME	255
#define	MAX_EP_SPEC	255

#define	AUTHENTICAT_LEN	(MAX_APP_NAME + MAX_AUTH_NAME + MAX_UNIT_NAME + 9)

#define	SUBJ_LIST_CT	257		/*	Must be prime for hash.	*/

#define	TS_INDEX_LIMIT	5

#ifndef MAX_CONTIN_NBR
#define	MAX_CONTIN_NBR	20
#endif

#ifndef MAX_VENTURE_NBR
#define	MAX_VENTURE_NBR	20
#endif

#ifndef MAX_UNIT_NBR
#define	MAX_UNIT_NBR	10
#endif

#ifndef MAX_NODE_NBR
#define	MAX_NODE_NBR	255
#endif

#ifndef MAX_ROLE_NBR
#define	MAX_ROLE_NBR	255
#endif

#ifndef MAX_SUBJ_NBR
#define	MAX_SUBJ_NBR	500
#endif

/*		Note: all intervals are in seconds.			*/

#ifndef N1_INTERVAL
#define	N1_INTERVAL	5
#endif

#ifndef N2_INTERVAL
#define	N2_INTERVAL	5
#endif

#ifndef N3_INTERVAL
#define	N3_INTERVAL	10
#endif

#define	N4_INTERVAL	(N3_INTERVAL * 2)

#ifndef N6_COUNT
#define	N6_COUNT	3
#endif

#define N5_INTERVAL	(N4_INTERVAL * N6_COUNT)

extern int		MaxContinNbr;
extern int		MaxVentureNbr;
extern int		MaxUnitNbr;
extern int		MaxModuleNbr;
extern int		MaxRoleNbr;
extern int		MaxSubjNbr;

#define	LOCK_MIB	pthread_mutex_lock(&mib->mutex)
#define	UNLOCK_MIB	pthread_mutex_unlock(&mib->mutex)

/*	Reason codes for "rejection" MAMS messages.			*/
#define	REJ_DUPLICATE	1
#define	REJ_NO_CENSUS	2
#define	REJ_CELL_FULL	3
#define	REJ_NO_UNIT	4

extern char		*rejectionMemos[];

/*	Memory management abstraction.					*/
extern int		amsMemory;
extern MemAllocator	amsmtake;
extern MemDeallocator	amsmrelease;
extern MemAtoPConverter	amsmatop;
extern MemPtoAConverter	amsmptoa;
#define MTAKE(size)	amsmtake(__FILE__, __LINE__, size)
#define MRELEASE(addr)	amsmrelease(__FILE__, __LINE__, addr)

extern char		*BadParmsMemo;
extern char		*NoMemoryMemo;

/*	Common event types.						*/
#define CRASH_EVT	11
#define MAMS_MSG_EVT	12
#define MSG_TO_SEND_EVT	13
#define	REJECTED_EVT	14

typedef void		*(*ThreadMain)(void *parms);

typedef struct amsevtst
{
	char		type;
	char		value[1];
} AmsEvt;

/*	*	AMS Management Information Base		*	*	*/

typedef enum
{
	RamsNoProtocol = 0,
	RamsBp,
	RamsUdp
} RamsNetProtocol;

typedef struct
{
	int		nbr;
	char		*name;
	RamsNetProtocol	gwProtocol;
	char		*gwEid;
	int		isNeighbor;	/*	Boolean.		*/
	char		*description;
} Continuum;

/*	MamsEndpoint encapsulates the endpoint to use for sending
 *	MAMS messages to a REMOTE entity, always using the primary
 *	transport service of the continuum.				*/

typedef struct
{
	char		*ept;		/*	Raw, unparsed text.	*/
	void		*tsep;		/*	Xmit parms parsed out.	*/

	/*	No need to indicate which MamsInterface to use when
	 *	sending messages to the tsep of this MamsEndpoint;
	 *	the sender always uses its own MamsInterface.		*/
} MamsEndpoint;

/*	TransSvc is a structure that encapsulates the machinery for
 *	exchanging messages via some transport service.			*/

typedef struct tsvcst	*TransSvcP;
typedef void		(*TsLoadFn)(TransSvcP ts);
typedef void		(*TsShutdownFn)(void *sap);

typedef int		(*TsCsepNameFn)(char *epSpec, char *epName);

typedef struct mamsifst	*MamsInterfaceP;
typedef int		(*TsMamsInitFn)(MamsInterfaceP tsif);

typedef int		(*TsMamsParseFn)(MamsEndpoint *endpoint);
typedef void		(*TsMamsClearFn)(MamsEndpoint *endpoint);
typedef int		(*TsSendMamsFn)(MamsEndpoint *endpoint,
				MamsInterfaceP tsif, char *msg, int msgLen);

typedef struct amsifst	*AmsInterfaceP;
typedef int		(*TsAmsInitFn)(AmsInterfaceP tsif, char *epspec);

typedef struct amsepst	*AmsEndpointP;
typedef int		(*TsAmsParseFn)(AmsEndpointP endpoint);
typedef void		(*TsAmsClearFn)(AmsEndpointP endpoint);

typedef struct amssapst	*AmsSapP;
typedef int		(*TsSendAmsFn)(AmsEndpointP endpoint, AmsSapP sap,
				unsigned char flowLabel,
				char *header, int headerLen,
				char *content, int contentLen);
typedef struct tsvcst
{
	char		*name;
	TsCsepNameFn	csepNameFn;
	TsMamsInitFn	mamsInitFn;
	ThreadMain	mamsReceiverFn;
	TsMamsParseFn	parseMamsEndpointFn;
	TsMamsClearFn	clearMamsEndpointFn;
	TsSendMamsFn	sendMamsFn;
	TsAmsInitFn	amsInitFn;
	ThreadMain	amsReceiverFn;
	TsAmsParseFn	parseAmsEndpointFn;
	TsAmsClearFn	clearAmsEndpointFn;
	TsSendAmsFn	sendAmsFn;
	TsShutdownFn	shutdownFn;
} TransSvc;

/*	MamsInterface is a structure used by the LOCAL entity to send
 *	and receive MAMS messages, using some transport service.	*/

typedef struct mamsifst
{
	TransSvc	*ts;
	int		ventureNbr;	/*	For authenticator.	*/
	int		unitNbr;	/*	For authenticator.	*/
	int		roleNbr;	/*	For authenticator.	*/
	char		*endpointSpec;	/*	Spec for own name.	*/
	char		*ept;		/*	Own endpoint name text.	*/
	Llcv		eventsQueue;
	pthread_t	receiver;
	void		*sap;		/*	e.g., socket FD		*/
} MamsInterface;

/*	AmsEpspec characterizes one means by which AMS modules on this
 *	host machine may receive AMS messages.				*/

typedef struct
{
	TransSvc	*ts;
	char		epspec[MAX_EP_SPEC + 1];
} AmsEpspec;

/*	AmsApp encapsulates global information about an application.	*/

typedef struct
{
	char		*name;
	char		*publicKey;
	int		publicKeyLength;
	char		*privateKey;	/*	Only in registrar MIB.	*/
	int		privateKeyLength;
} AmsApp;

/*	AppRole characterizes one functional role in an instance of
 *	an application.							*/

typedef struct
{
	int		nbr;
	char		*name;
	char		*publicKey;
	int		publicKeyLength;
	char		*privateKey;	/*	Only in module's own MIB.*/
	int		privateKeyLength;
} AppRole;

/*	Modules send and receive the messages that are exchanged within
 *	an instance of an application.  Modules' references to subjects
 *	are encapsulated in SubjOfInterest structures.			*/

typedef struct
{
	struct subjst	*subject;
	LystElt		fanElt;		/*	(FanModule *)		*/
	Lyst		subscriptions;	/*	(XmitRule *)		*/
	Lyst		invitations;	/*	(XmitRule *)		*/
} SubjOfInterest;

/*	Module encapsulates information about a module in the message
 *	space as viewed by other modules and by the module's registrar.
 *
 *	Subjects of interest to this module (subscriptions and/or
 *	invitations) are listed in ascending subject number order.	*/

typedef struct
{
	int		unitNbr;	/*	ID of module's homecell.*/
	int		nbr;		/*	Of module, within cell.	*/
	AppRole		*role;		/*	Supplies name, keys.	*/
	MamsEndpoint	mamsEndpoint;	/*	Of module.		*/
	Lyst		amsEndpoints;	/*	(AmsEndpoint *)		*/
	Lyst		subjects;	/*	(SubjOfInterest *)	*/
	int		heartbeatsMissed;	/*	To Registrar.	*/
	int		confirmed;	/*	Boolean.		*/
} Module;


/*	MsgElement describes one component of a standardized message.	*/

typedef enum
{
	AmsNoElement = 0,
	AmsLong,
	AmsInt,
	AmsShort,
	AmsChar,
	AmsString
} ElementType;

typedef struct
{
	char		*name;
	ElementType	type;
	char		*description;
} MsgElement;

/*	Subjects identify the messages exchanged among the modules
 *	of an instance of an application.  Subjects' references to
 *	modules are encapsulated in FanModule structures.		*/

typedef struct
{
	Module		*module;
	SubjOfInterest	*subj;		/*	For access to lists.	*/
} FanModule;

/*	Fan modules are listed in ascending unit number (of cell),
 *	module number order.						*/

typedef struct subjst
{
	int		nbr;
	int		isContinuum;
	char		*name;
	char		*description;
	Lyst		elements;		/*	(MsgElement *)	*/
	Lyst		authorizedSenders;	/*	(AppRole *)	*/
	Lyst		authorizedReceivers;	/*	(AppRole *)	*/
	char		*symmetricKey;
	int		keyLength;
	Lyst		modules;		/*	(FanModule *)	*/
	LystElt		elt;		/*	In hashtable.		*/
} Subject;

/*	Cell encapsulates information about that portion of some unit
 *	(of some venture) that is within a single continuum.		*/

typedef struct
{
	struct unit_str	*unit;			/*	Parent.		*/
	MamsEndpoint	mamsEndpoint;		/*	Of registrar.	*/
	Module		*modules[MAX_NODE_NBR + 1];
	int		heartbeatsMissed;	/*	To CS.		*/
	int		resyncPeriod;		/*	In heartbeats.	*/
} Cell;

/*	Unit encapsulates information about one organizational unit
 *	of a venture.							*/

typedef struct unit_str
{
	int		nbr;
	char		*name;
	struct unit_str	*superunit;		/*	Parent unit.	*/
	LystElt		inclusionElt;		/*	In superunit.	*/
	Lyst		subunits;		/*	(Unit *)	*/

	/*	The cellData of the unit encapsulates information
	 *	about the portion of this unit that is within the
	 *	local continuum.					*/

	Cell		cellData;
	Cell		*cell;
} Unit;

/*	Venture encapsulates information about a venture (an instance
 *	of an application).						*/

typedef struct ventstr
{
	int		nbr;
	AmsApp		*app;		/*	Supplies name, keys.	*/
	char		*authorityName;
	AppRole		*roles[MAX_ROLE_NBR + 1];
	Subject		*subjects[MAX_SUBJ_NBR + 1];	/*	subj>0	*/
	Lyst		subjLysts[SUBJ_LIST_CT];/*	hash table	*/
	Unit		*units[MAX_UNIT_NBR + 1];

	/*	The msgspaces array enumerates all messages spaces
	 *	that are included in this venture, including the one
	 *	that is in the local continuum.				*/

	Subject		*msgspaces[MAX_CONTIN_NBR + 1];	/*	subj<0	*/
} Venture;

/*	The supported transport services listed in the MIB are in
 *	descending order of preference, i.e., in descending order of
 *	performance.							*/

typedef struct
{
	pthread_mutex_t	mutex;
	int		transportServiceCount;
	TransSvc	transportServices[TS_INDEX_LIMIT + 1];
	TransSvc	*pts;			/*	Primary TS.	*/
	Lyst		amsEndpointSpecs;	/*	(AmsEpspec *)	*/
	Continuum	*continua[MAX_CONTIN_NBR + 1];
	Lyst		applications;		/*	(AmsApp *)	*/
	Venture		*ventures[MAX_VENTURE_NBR + 1];
	Lyst		csEndpoints;		/*	(MamsEndpoint *)*/
	int		localContinuumNbr;
	RamsNetProtocol	localContinuumGwProtocol;
	char		*localContinuumGwEid;
	int		ramsNetIsTree;		/*	Boolean.	*/
	char		*csPublicKey;
	int		csPublicKeyLength;
	char		*csPrivateKey;		/*	Only for CS MIB.*/
	int		csPrivateKeyLength;
} AmsMib;

extern AmsMib		*mib;

/*	*	*	MAMS message structure	*	*	*	*/

typedef enum
{
	heartbeat = 1,
	rejection = 2,
	you_are_dead = 3,
	registrar_noted = 4,
	registrar_unknown = 5,
	reconnected = 6,
	announce_registrar = 7,
	invite = 8,
	disinvite = 9,
	cell_spec = 10,
	/*	Note: types 11-17 are reserved.				*/
	registrar_query = 18,
	module_registration = 19,
	you_are_in = 20,
	I_am_starting = 21,
	I_am_here = 22,
	declaration = 23,
	subscribe = 24,
	unsubscribe = 25,
	I_am_stopping = 26,
	reconnect = 27,
	cell_status = 28,
	module_has_started = 29,
	I_am_running = 30,
	module_status = 31
} MamsPduType;

typedef struct
{
	unsigned char	ventureNbr;
	unsigned short	unitNbr;
	unsigned char	roleNbr;
	MamsPduType	type;
	signed int	memo;
	time_t		timeTag;
	unsigned short	supplementLength;
	char		*supplement;
} MamsMsg;

/*	*	*	Private function prototypes	*	*	*/

extern int	initMemoryMgt(char *mName, char *memory, unsigned mSize);
extern int	loadMib(char *mibSource);

extern int	time_to_stop(Llcv llcv);
extern int	llcv_reply_received(Llcv llcv);

extern void	encryptUsingPublicKey(char *plaintext, int ptlen, char *key,
			int klen, char *cyphertext, int *ctlen);
extern void	decryptUsingPublicKey(char *cyphertext, int ctlen, char *key,
			int klen, char *plaintext, int *ptlen);
extern void	encryptUsingPrivateKey(char *plaintext, int ptlen, char *key,
			int klen, char *cyphertext, int *ctlen);
extern void	decryptUsingPrivateKey(char *cyphertext, int ctlen, char *key,
			int klen, char *plaintext, int *ptlen);
extern void	encryptUsingSymmetricKey(char *plaintext, int ptlen, char *key,
			int klen, char *cyphertext, int *ctlen);
extern void	decryptUsingSymmetricKey(char *cyphertext, int ctlen, char *key,
			int klen, char *plaintext, int *ptlen);

extern LystElt	findApplication(char *appName);
extern LystElt	findElement(Subject *subject, char *elementName);
extern Venture	*lookUpVenture(char *appName, char *authName);
extern Subject	*lookUpSubject(Venture *venture, char *subjectName);
extern AppRole	*lookUpRole(Venture *venture, char *roleName);
extern Unit	*lookUpUnit(Venture *venture, char *unitName);
extern int	lookUpContinuum(char *continuumName);

extern LystElt	createApp(char *name, char *publicKey, int publicKeyLength,
			char *privateKey, int privateKeyLength);
extern LystElt	createElement(Subject *subj, char *name, ElementType type,
			char *description);
extern void	eraseSubject(Venture *venture, Subject *subj);
extern Subject	*createSubject(Venture *venture, int nbr, char *name,
			char *description, char *key, int keyLength);
extern void	eraseRole(Venture *venture, AppRole *role);
extern AppRole	*createRole(Venture *venture, int nbr, char *name,
			char *publicKey, int publicKeyLength,
			char *privateKey, int privateKeyLength);
extern void	eraseMsgspace(Venture *venture, Subject *subj);
extern Subject	*createMsgspace(Venture *venture, int continNbr, char *key,
			int keyLength);
extern void	eraseUnit(Venture *venture, Unit *unit);
extern Unit	*createUnit(Venture *venture, int nbr, char *name,
			int resyncPeriod);
extern void	eraseVenture(Venture *venture);
extern Venture	*createVenture(int nbr, char *appname, char *authname,
			int rootCellResyncPeriod);
extern Continuum
		*createContinuum(int nbr, char *name, char *gwEidString,
			int isNeighbor, char *description);
extern LystElt	createCsEndpoint(char *endpointSpec, LystElt nextElt);
extern LystElt	createAmsEpspec(char *tsname, char *endpointSpec);
extern void	eraseMib();
extern int	createMib(int nbr, char *geEidString, int ramsNetIsTree,
			char *ptsName, char *publicKey, int publicKeyLength,
			char *privateKey, int privateKeyLength);

extern unsigned short
		computeAmsChecksum(unsigned char *cursor, int pduLength);

extern int	enqueueMamsEvent(Llcv eventsQueue, AmsEvt *evt,
			char *ancillaryBlock, int responseNbr);
extern int	enqueueMamsCrash(Llcv eventsQueue, char *text);
extern int	enqueueMamsStubEvent(Llcv eventsQueue, int eventType);
extern void	recycleEvent(AmsEvt *evt);
extern void	destroyEvent(LystElt elt, void *userdata);

extern int	computeModuleId(int roleNbr, int unitNbr, int moduleNbr);
extern int	parseModuleId(int memo, int *roleNbr, int *unitNbr,
			int *moduleNbr);

extern char	*parseString(char **cursor, int *bytesRemaining, int *len);
extern int	constructMamsEndpoint(MamsEndpoint *ep, int epnLength,
			char *epnText);
extern void	clearMamsEndpoint(MamsEndpoint *ep);

extern int	rememberModule(Module *module, AppRole *role, int epnLength,
			char *epnText);
extern void	forgetModule(Module *module);

extern int	sendMamsMsg(MamsEndpoint *endpoint, MamsInterface *tsif,
			MamsPduType type, unsigned int memo,
			unsigned short supplementLength, char *supplement);
extern int	enqueueMamsMsg(Llcv eventsQueue, int length,
			unsigned char *content);

extern int	findConfigServer(MamsInterface *tsif, Llcv eventsQueue,
			MamsEndpoint **csEndpoint);

#ifdef __cplusplus
}
#endif

#endif	/* _AMSCOMMON_H */
