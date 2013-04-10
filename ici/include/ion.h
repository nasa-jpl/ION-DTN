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
/* As of 2013-03-13 the sourceforge version number is this: */
#define IONVERSIONNUMBER "ION OPEN SOURCE 3.1.2"
#endif

/* Allow the compile option -D to override this in the future */
#ifndef STARTUP_TIMEOUT
/* When an admin program starts in ION, wait 15 seconds before
   considering the startup process 'hung'. */
#define STARTUP_TIMEOUT 15
#endif

#define	MAX_SPEED_MPH	(150000)
#define	MAX_SPEED_MPS	(MAX_SPEED_MPH / 3600)

#ifndef ION_SDR_MARGIN
#define	ION_SDR_MARGIN	(20)		/*	Percent.		*/
#endif
#ifndef ION_OPS_ALLOC
#define	ION_OPS_ALLOC	(20)		/*	Percent.		*/
#endif
#define	ION_SEQUESTERED	(ION_SDR_MARGIN + ION_OPS_ALLOC)

typedef struct
{
	int	wmKey;
	long	wmSize;
	char	*wmAddress;
	char	sdrName[MAX_SDR_NAME + 1];
	int	configFlags;
	long	heapWords;
	int	heapKey;
	char	pathName[MAXPATHLEN + 1];
} IonParms;

typedef struct
{
	unsigned int	term;		/*	In seconds.		*/
	unsigned int	cycles;		/*	0 = forever.		*/
	int		(*proceed)(void *);
	void		*userData;
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
	unsigned int	xmitRate;	/*	In bytes per second.	*/
} IonContact;

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
	uvast		ownNodeNbr;
	long		productionRate;	/*	Bundles sent by apps.	*/
	long		consumptionRate;/*	Bundles rec'd by apps.	*/
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
 *	than the local node.  Each IonNode has a list of "snubs"
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
 *	An IonSnub object identifies a neighboring node that has
 *	refused custody of bundles destined for the associated
 *	IonNode, possibly because it wasn't able to compute a route
 *	to that destination node.  The existence of the snub generally
 *	prevents consideration of this neighbor as proximate
 *	destination when routing to the affected destination node.
 *	However, once each RTLT (between the local node and the
 *	snubbing neighbor), one custodial bundle may be routed to
 *	the snubbing neighbor as a "probe", to determine whether or
 *	not the neighbor is still refusing bundles destined for the
 *	associated IonNode.  Probe activity is initiated by scheduling
 *	IonProbe event objects.  A list of all scheduled probes is
 *	included in the IonVdb.						*/

typedef struct
{
	uvast		nodeNbr;	/*	Of the snubbing node.	*/
	int		probeIsDue;	/*	Boolean.		*/
} IonSnub;		/*	An uncooperative neighboring node.	*/

typedef struct
{
	uvast		nodeNbr;	/*	As from IonContact.	*/
	PsmAddress	snubs;		/*	SM list: IonSnub	*/
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
	uvast		nodeNbr;	/*	As from IonContact.	*/
	unsigned int	xmitRate;	/*	Xmit *to* neighbor.	*/
	unsigned int	fireRate;	/*	Xmit *from* neighbor.	*/
	unsigned int	recvRate;	/*	Recv from neighbor.	*/
	unsigned int	prevXmitRate;	/*	Xmit *to* neighbor.	*/
	unsigned int	prevRecvRate;	/*	Recv from neighbor.	*/
	PsmAddress	node;		/*	Points to IonNode.	*/
	unsigned int	owltInbound;	/*	In seconds.		*/
	unsigned int	owltOutbound;	/*	In seconds.		*/
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
	unsigned int	xmitRate;	/*	In bytes per second.	*/
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
} IonEventType;

typedef struct
{
	time_t		time;		/*	As from time(2).	*/
	IonEventType	type;
	PsmAddress	ref;		/*	A CXref or RXref addr.	*/
} IonEvent;

/*	The volatile database object encapsulates the current volatile
 *	state of the database.						*/

typedef struct
{
	int		clockPid;	/*	For stopping rfxclock.	*/
	int		deltaFromUTC;	/*	In seconds.		*/
	sm_SemId	zcoSemaphore;	/*	Signals availability.	*/
	int		zcoClaimants;	/*	# of waiting tasks.	*/
	int		zcoClaims;	/*	# of demands on ZCO.	*/
	time_t		lastEditTime;	/*	Add/del contacts/ranges	*/
	PsmAddress	nodes;		/*	SM RB tree: IonNode	*/
	PsmAddress	neighbors;	/*	SM RB tree: IonNeighbor	*/
	PsmAddress	contactIndex;	/*	SM RB tree: IonCXref	*/
	PsmAddress	rangeIndex;	/*	SM RB tree: IonRXref	*/
	PsmAddress	timeline;	/*	SM RB tree: IonEvent	*/
	PsmAddress	probes;		/*	SM list: IonProbe	*/
} IonVdb;

typedef struct
{
	unsigned int	totalCount;
	uvast		totalBytes;
	unsigned int	currentCount;
	uvast		currentBytes;
} Tally;

#ifndef MTAKE
#define MTAKE(size)	allocFromIonMemory(__FILE__, __LINE__, size)
#define MRELEASE(addr)	releaseToIonMemory(__FILE__, __LINE__, addr)
#endif

extern void		*allocFromIonMemory(char *, int, size_t);
extern void		releaseToIonMemory(char *, int, void *);
extern void		*ionMemAtoP(unsigned long);
extern unsigned long	ionMemPtoA(void *);

extern int		ionInitialize(	IonParms *parms,
					uvast ownNodeNbr);
extern int		ionAttach();
extern void		ionDetach();
extern void		ionProd(	uvast fromNode,
					uvast toNode,
					unsigned int xmitRate,
					unsigned int owlt);
extern void		ionTerminate();

extern Object		ionCreateZco(	ZcoMedium source,
					Object location,
					vast offset,
					vast length,
					int *control);
extern vast		ionAppendZcoExtent(Object zco,
					ZcoMedium source,
					Object location,
					vast offset,
					vast length,
					int *control);
extern void		ionCancelZcoSpaceRequest(int *control);

extern Sdr		getIonsdr();
extern Object		getIonDbObject();
extern PsmPartition	getIonwm();
extern int		getIonMemoryMgr();
extern IonVdb		*getIonVdb();
extern char		*getIonWorkingDirectory();
extern uvast		getOwnNodeNbr();

extern int		startIonMemTrace(int size);
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

extern void		ionSetAlarm(	IonAlarm *alarm, pthread_t *thread);
extern void		ionCancelAlarm(	pthread_t thread);

extern void		ionNoteMainThread(char *procName);
extern void		ionPauseMainThread(int seconds);
extern void		ionKillMainThread(char *procName);

#ifdef __cplusplus
}
#endif

#endif  /* _ION_H_ */
