/*
 	ion.h:	private definitions supporting the implementation of
		protocols in the ION stack.

	Author: Scott Burleigh, JPL

	Copyright (c) 2007, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 									*/
#ifndef _ION_H_
#define _ION_H_

#include "platform.h"
#include "memmgr.h"
#include "sdr.h"
#include "smlist.h"
#include "smrbt.h"
#include "zco.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Allow the compile option -D to override this in the future */
#ifndef IONVERSIONNUMBER
/* As of 2016-02-10 the sourceforge version number is this: */
#define IONVERSIONNUMBER "ION OPEN SOURCE 3.6.0b"
#endif

/* Allow the compile option -D to override this in the future */
#ifndef STARTUP_TIMEOUT
/* When an admin program starts in ION, wait 15 seconds before
   considering the startup process 'hung'. */
#define STARTUP_TIMEOUT 15
#endif

#define	MAX_SPEED_MPH	(150000)
#define	MAX_SPEED_MPS	(MAX_SPEED_MPH / 3600)

#define	SENDER_NODE	(0)
#define	RECEIVER_NODE	(1)

#ifndef ION_SDR_MARGIN
#define	ION_SDR_MARGIN	(20)		/*	Percent.		*/
#endif
#ifndef ION_OPS_ALLOC
#define	ION_OPS_ALLOC	(40)		/*	Percent.		*/
#endif
#define	ION_SEQUESTERED	(ION_SDR_MARGIN + ION_OPS_ALLOC)

typedef struct
{
	int	wmKey;
	size_t	wmSize;
	char	*wmAddress;
	char	sdrName[MAX_SDR_NAME + 1];
	size_t	sdrWmSize;
	int	configFlags;
	size_t	heapWords;
	int	heapKey;
	size_t	logSize;
	int	logKey;
	char	pathName[MAXPATHLEN + 1];
} IonParms;

typedef struct
{
	unsigned int	term;		/*	In seconds.		*/
	unsigned int	cycles;		/*	0 = forever.		*/
	sm_SemId	semaphore;
	time_t		nextTimeout;
} IonAlarm;

/*	The IonDB lists of IonContacts and IonRanges are time-ordered,
 *	encyclopedic, and non-volatile.  With the passage of time their
 *	contents are propagated into the IonVdb lists of IonNodes and
 *	IonNeighbors.
 *
 *	Additionally, IonContacts for which either the fromNode or the
 *	toNode is the local node's own node number are used in
 *	congestion forecasting, by computation of maximum scheduled
 *	bundle space occupancy.						*/

typedef struct
{
	time_t		fromTime;	/*	As from time(2).	*/
	time_t		toTime;		/*	As from time(2).	*/
	uvast		fromNode;	/*	LTP engineID, a.k.a.	*/
	uvast		toNode;		/*	... BP CBHE nodeNbr.	*/
	size_t		xmitRate;	/*	In bytes per second.	*/
	float		confidence;	/*	Confidence in contact.	*/
	int		discovered;	/*	Boolean.		*/
} IonContact;

typedef struct
{
	time_t		fromTime;	/*	As from time(2).	*/
	time_t		toTime;		/*	As from time(2).	*/
	uvast		fromNode;	/*	LTP engineID, a.k.a.	*/
	uvast		toNode;		/*	... BP CBHE nodeNbr.	*/
	size_t		xmitRate;	/*	In bytes per second.	*/
} PastContact;

typedef struct
{
	time_t		fromTime;	/*	As from time(2).	*/
	time_t		toTime;		/*	As from time(2).	*/
	uvast		fromNode;	/*	LTP engineID, a.k.a.	*/
	uvast		toNode;		/*	... BP CBHE nodeNbr.	*/
	unsigned int	owlt;		/*	In seconds.		*/
} IonRange;

/*	The ION database is shared by BP, LTP, and RFX.			*/

typedef struct
{
	Object		contacts;	/*	SDR list: IonContact	*/
	Object		ranges;		/*	SDR list: IonRange	*/
	Object		contactLog[2];	/*	SDR list: PastContact	*/
	uvast		ownNodeNbr;
	size_t		productionRate;	/*	Bundles sent by apps.	*/
	size_t		consumptionRate;/*	Bundles rec'd by apps.	*/
	double		occupancyCeiling;
	double		maxForecastOccupancy;
	Object		alarmScript;	/*	Congestion alarm.	*/
	time_t		horizon;	/*	On congestion forecast.	*/
	int		deltaFromUTC;	/*	In seconds.		*/
	int		maxClockError;	/*	In seconds.		*/
	char		clockIsSynchronized;	/*	Boolean.	*/
	char		workingDirectoryName[256];
        IonParms        parmcopy;       /*	Copy of the ion config
						parms as asserted to
						ionadmin at startup.	*/
} IonDB;

/*	The IonVdb red-black tree of IonNodes, in volatile memory,
 *	contains objects representing all nodes in the network other
 *	than the local node.  Each IonNode has a list of "embargoes"
 *	(described below), plus a "routing object" that points to
 *	data that has structure and function specific to the routing
 *	system established for the bundle protocol agent.
 *
 *	The IonVdb also contains red-black trees that (a) index all
 *	contacts in the non-volatile database, by "from" node, "to"
 *	node, and time, and (b) support immediate lookup of the
 *	current one-way light time from any node to any other node,
 *	based on the range information in the non-volatile database.
 *	Both of these are provided to support route computation.
 *
 *	The IonVdb also contains a red-black tree of all of the local
 *	node's neighbors, characterizing the current state of
 *	communications between the local node and each neighbor.
 *
 *	An Embargo object identifies a neighboring node to which we
 *	no longer forward bundles destined for some destination node,
 *	because that neighboring node has recently refused custody of
 *	such bundles.  This refusal might have occurred because the
 *	neighboring node was short of storage capacity or wasn't
 *	able to compute a route to that destination node.  The
 *	existence of the embargo generally prevents consideration
 *	of this neighbor as proximate destination when routing to
 *	the affected destination node.  However, once each RTLT
 *	(between the local node and the embargoed neighbor), one
 *	custodial bundle may be routed to the embargoed neighbor as
 *	a "probe", to determine whether or not the neighbor is still
 *	refusing bundles destined for the associated IonNode.  Probe
 *	activity is initiated by scheduling IonProbe event objects.
 *	A list of all scheduled probes is included in the IonVdb.	*/

typedef struct
{
	uvast		nodeNbr;	/*	Of the embargoed node.	*/
	int		probeIsDue;	/*	Boolean.		*/
} Embargo;		/*	An uncooperative neighboring node.	*/

typedef struct
{
	uvast		nodeNbr;	/*	As from IonContact.	*/
	PsmAddress	embargoes;	/*	SM list: Embargo	*/
	PsmAddress	routingObject;	/*	Routing-dependent.	*/
} IonNode;		/*	A potential bundle destination node.	*/

typedef struct
{
	uvast		destNodeNbr;
	uvast		neighborNodeNbr;
	time_t		time;
} IonProbe;

/*	The IonVdb tree of IonNeighbors, in volatile memory, contains
 *	the *currrent* contact state of the local node.  BP uses this
 *	information for rate control.  LTP uses it for computation of
 *	timeout intervals for timers, and changes in this state
 *	trigger timer suspension and resumption.			*/

typedef struct
{
	size_t		nominalRate;	/*	In bytes per second.	*/
	vast		capacity;	/*	Bytes, current second.	*/
} Throttle;

typedef struct
{
	uvast		nodeNbr;	/*	As from IonContact.	*/
	size_t		xmitRate;	/*	Xmit *to* neighbor.	*/
	size_t		fireRate;	/*	Xmit *from* neighbor.	*/
	size_t		recvRate;	/*	Recv from neighbor.	*/
	size_t		prevXmitRate;	/*	Xmit *to* neighbor.	*/
	size_t		prevRecvRate;	/*	Recv from neighbor.	*/
	PsmAddress	node;		/*	Points to IonNode.	*/
	size_t		owltOutbound;	/*	In seconds.		*/
	size_t		owltInbound;	/*	In seconds.		*/
	Throttle	xmitThrottle;	/*	For rate control.	*/
} IonNeighbor;

typedef struct
{
	uvast		fromNode;	/*	LTP engineID, a.k.a.	*/
	uvast		toNode;		/*	... BP CBHE nodeNbr.	*/
	time_t		fromTime;	/*	As from time(2).	*/
	time_t		toTime;		/*	As from time(2).	*/
	unsigned int	owlt;		/*	Current, in seconds.	*/
	Object		rangeElt;	/*	In iondb->ranges.	*/
} IonRXref;

typedef struct
{
	uvast		fromNode;	/*	LTP engineID, a.k.a.	*/
	uvast		toNode;		/*	... BP CBHE nodeNbr.	*/
	time_t		fromTime;	/*	As from time(2).	*/
	time_t		toTime;		/*	As from time(2).	*/
	size_t		xmitRate;	/*	In bytes per second.	*/
	float		confidence;	/*	Confidence in contact.	*/
	int		discovered;	/*	Boolean.		*/
	time_t		startXmit;	/*	Computed when inserted.	*/
	time_t		stopXmit;	/*	Computed when inserted.	*/
	time_t		startFire;	/*	Computed when inserted.	*/
	time_t		stopFire;	/*	Computed when inserted.	*/
	time_t		startRecv;	/*	Computed when inserted.	*/
	time_t		stopRecv;	/*	Computed when inserted.	*/
	time_t		purgeTime;	/*	Computed when inserted.	*/
	Object		contactElt;	/*	In iondb->contacts.	*/
	PsmAddress	routingObject;	/*	Routing-dependent.	*/
} IonCXref;

typedef enum
{
	IonStopImputedRange = 0,
	IonStopAssertedRange = 1,
	IonStopXmit = 2,
	IonStopFire = 3,
	IonStopRecv = 4,
	IonStartImputedRange = 16,
	IonStartAssertedRange = 17,
	IonStartXmit = 18,
	IonStartFire = 19,
	IonStartRecv = 20,
	IonPurgeContact = 21,
	IonAlarmTimeout = 31
} IonEventType;

typedef struct
{
	time_t		time;		/*	As from time(2).	*/
	IonEventType	type;
	PsmAddress	ref;		/*	A CXref or RXref addr.	*/
} IonEvent;

/*	These structures are used to implement flow-controlled ZCO
 *	space management for ION.					*/

typedef struct
{
	sm_SemId	semaphore;
} ReqAttendant;

typedef PsmAddress	ReqTicket;

typedef struct
{
	vast		fileSpaceNeeded;
	vast		bulkSpaceNeeded;
	vast		heapSpaceNeeded;
	sm_SemId	semaphore;
	int		secondsUnclaimed;
	unsigned char	coarsePriority;
	unsigned char	finePriority;
} Requisition;

/*	The volatile database object encapsulates the current volatile
 *	state of the database.						*/

typedef struct
{
	int		clockPid;	/*	For stopping rfxclock.	*/
	int		deltaFromUTC;	/*	In seconds.		*/
	struct timeval	lastEditTime;	/*	Add/del contacts/ranges	*/
	PsmAddress	nodes;		/*	SM RB tree: IonNode	*/
	PsmAddress	neighbors;	/*	SM RB tree: IonNeighbor	*/
	PsmAddress	contactIndex;	/*	SM RB tree: IonCXref	*/
	PsmAddress	rangeIndex;	/*	SM RB tree: IonRXref	*/
	PsmAddress	timeline;	/*	SM RB tree: IonEvent	*/
	PsmAddress	probes;		/*	SM list: IonProbe	*/
	PsmAddress	requisitions[2];/*	SM list: Requisition	*/
} IonVdb;

typedef struct
{
	unsigned int	totalCount;
	uvast		totalBytes;
	unsigned int	currentCount;
	uvast		currentBytes;
} Tally;

typedef struct
{
	unsigned int	seconds;
	unsigned int	count;
} BpTimestamp;

#ifndef MTAKE
#define MTAKE(size)	allocFromIonMemory(__FILE__, __LINE__, size)
#define MRELEASE(addr)	releaseToIonMemory(__FILE__, __LINE__, addr)
#endif

extern void		*allocFromIonMemory(const char *, int, size_t);
extern void		releaseToIonMemory(const char *, int, void *);
extern void		*ionMemAtoP(uaddr);
extern uaddr		ionMemPtoA(void *);

extern int		ionInitialize(	IonParms *parms,
					uvast ownNodeNbr);
extern int		ionAttach();
extern void		ionDetach();
extern void		ionProd(	uvast fromNode,
					uvast toNode,
					size_t xmitRate,
					unsigned int owlt);
extern void		ionTerminate();

extern int		ionStartAttendant(ReqAttendant *attendant);
extern void		ionPauseAttendant(ReqAttendant *attendant);
extern void		ionResumeAttendant(ReqAttendant *attendant);
extern void		ionStopAttendant(ReqAttendant *attendant);
extern int		ionRequestZcoSpace(ZcoAcct acct,
					vast fileSpaceNeeded,
					vast bulkSpaceNeeded,
					vast heapSpaceNeeded,
					unsigned char coarsePriority,
					unsigned char finePriority,
					ReqAttendant *attendant,
					ReqTicket *ticket);
extern int		ionSpaceAwarded(ReqTicket ticket);
extern void		ionShred(	ReqTicket ticket);
extern Object		ionCreateZco(	ZcoMedium source,
					Object location,
					vast offset,
					vast length,
					unsigned char coarsePriority,
					unsigned char finePriority,
					ZcoAcct acct,
					ReqAttendant *attendant);
extern vast		ionAppendZcoExtent(Object zco,
					ZcoMedium source,
					Object location,
					vast offset,
					vast length,
					unsigned char coarsePriority,
					unsigned char finePriority,
					ReqAttendant *attendant);
extern int		ionSendZcoByTCP(int *sock, Object zco, char *buffer,
					int buflen);

extern Sdr		getIonsdr();
extern Object		getIonDbObject();
extern PsmPartition	getIonwm();
extern int		getIonMemoryMgr();
extern IonVdb		*getIonVdb();
extern char		*getIonWorkingDirectory();
extern uvast		getOwnNodeNbr();

extern int		startIonMemTrace(size_t size);
extern void		printIonMemTrace(int verbose);
extern void		clearIonMemTrace();
extern void		stopIonMemTrace();

#define	TIMESTAMPBUFSZ	20

extern int		setDeltaFromUTC(int newDelta);
extern time_t		getUTCTime();	/*	UTC scale, 1970 epoch.	*/
extern int		ionClockIsSynchronized();

extern time_t		readTimestampLocal(char *timestampBuffer,
					time_t referenceTime);
extern time_t		readTimestampUTC(char *timestampBuffer,
					time_t referenceTime);
extern void		writeTimestampLocal(time_t timestamp,
					char *timestampBuffer);
extern void		writeTimestampUTC(time_t timestamp,
					char *timestampBuffer);

#define extractSdnv(into, from, remnant) \
if (_extractSdnv(into, (unsigned char **) from, remnant, __LINE__) < 1) \
return 0
extern int		_extractSdnv(	uvast *into,
					unsigned char **from,
					int *nbrOfBytesRemaining,
					int lineNbr);
#define extractSmallSdnv(into, from, remnant) \
if (_extractSmallSdnv(into, (unsigned char **) from, remnant, __LINE__) < 1) \
return 0
extern int		_extractSmallSdnv(unsigned int *into,
					unsigned char **from,
					int *nbrOfBytesRemaining,
					int lineNbr);

extern int		ionLocked();

extern int		readIonParms(	char *configFileName,
					IonParms *parms);
extern void		printIonParms(	IonParms *parms);

extern void		ionNoteMainThread(char *procName);
extern void		ionPauseMainThread(int seconds);
extern void		ionKillMainThread(char *procName);

#ifdef __cplusplus
}
#endif

#endif  /* _ION_H_ */
