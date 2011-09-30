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

#ifdef __cplusplus
extern "C" {
#endif

/* Allow the compile option -D to override this in the future */
#ifndef IONVERSIONNUMBER
/* As of 2011-09-26 the open channel release version number is this: */
#define IONVERSIONNUMBER "2.5.0"
#endif

/* Allow the compile option -D to override this in the future */
#ifndef STARTUP_TIMEOUT
/* When an admin program starts in ION, wait 15 seconds before
   considering the startup process 'hung'. */
#define STARTUP_TIMEOUT 15
#endif

#define	MAX_SPEED_MPH	(150000)
#define	MAX_SPEED_MPS	(MAX_SPEED_MPH / 3600)

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
	unsigned long	fromNode;	/*	LTP engineID, a.k.a.	*/
	unsigned long	toNode;		/*	... BP CBHE elementNbr.	*/
	unsigned long	xmitRate;	/*	In bytes per second.	*/
} IonContact;

typedef struct
{
	time_t		fromTime;	/*	As from time(2).	*/
	time_t		toTime;		/*	As from time(2).	*/
	unsigned long	fromNode;	/*	LTP engineID, a.k.a.	*/
	unsigned long	toNode;		/*	... BP CBHE elementNbr.	*/
	unsigned int	owlt;		/*	In seconds.		*/
} IonRange;

/*	The ION database is shared by BP, LTP, and RFX.			*/

typedef struct
{
	Object		contacts;	/*	SDR list: IonContact	*/
	Object		ranges;		/*	SDR list: IonRange	*/
	unsigned long	ownNodeNbr;
	long		productionRate;	/*	Bundles sent by apps.	*/
	long		consumptionRate;/*	Bundles rec'd by apps.	*/
	long		occupancyCeiling;
	long		receptionSpikeReserve;
	long		maxForecastOccupancy;
	long		maxForecastInTransit;
	long		currentOccupancy;
	Object		alarmScript;	/*	Congestion alarm.	*/
	time_t		horizon;	/*	On congestion forecast.	*/
	int		deltaFromUTC;	/*	In seconds.		*/
	int		maxClockError;	/*	In seconds.		*/
	char		workingDirectoryName[256];
} IonDB;

/*	The IonVdb list of IonNodes, in volatile memory, contains
 *	objects representing all nodes in the network other than the
 *	local node.  Each IonNode has lists of all of *its* neigbors
 *	(the origins list) and of references to all IonContacts for
 *	transmission to this node (the xmits list); each xmit refers
 *	to the origin that is the neighbor from which the indicated
 *	traffic will be transmitted.  All of this information is
 *	used for time-sensitive route computation.
 *
 *	The aggregate capacity noted in each xmit is the sum of the
 *	capacities of that xmit and all prior xmits that have not yet
 *	been purged from the database.  The capacity of an xmit is the
 *	product of its xmitRate and the difference between its toTime
 *	and its fromTime.
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
 *	IonProbe event objects.						*/

typedef struct
{
	unsigned long	nodeNbr;	/*	As from IonContact.	*/
	unsigned int	owlt;		/*	In seconds, current.	*/
} IonOrigin;

typedef struct
{
	PsmAddress	origin;		/*	Cross-ref to IonOrigin	*/
	time_t		fromTime;	/*	As from time(2).	*/
	time_t		toTime;		/*	As from time(2).	*/
	unsigned long	xmitRate;	/*	In bytes per second.	*/
	Scalar		aggrCapacity;	/*	Including this xmit.	*/
	time_t		mootAfter;	/*	As from time(2).	*/
	unsigned int	lastVisitor;
	time_t		visitHorizon;	/*	As from time(2).	*/
} IonXmit;

typedef struct
{
	unsigned long	nodeNbr;	/*	Of the snubbing node.	*/
	int		probeIsDue;	/*	Boolean.		*/
} IonSnub;

typedef struct
{
	unsigned long	nodeNbr;	/*	As from IonContact.	*/
	PsmAddress	xmits;		/*	SM list: IonXmit	*/
	PsmAddress	origins;	/*	SM list: IonOrigin	*/
	PsmAddress	snubs;		/*	SM list: IonSnub	*/
} IonNode;

typedef struct
{
	unsigned long	destNodeNbr;
	unsigned long	neighborNodeNbr;
	time_t		time;
} IonProbe;

/*	The IonVdb list of IonNeighbors, in volatile memory, contains
 *	the *currrent* contact state of the local node.  BP uses this
 *	information for rate control.  LTP uses it for computation of
 *	timeout intervals for timers, and changes in this state
 *	trigger timer suspension and resumption.			*/

typedef struct
{
	unsigned long	nodeNbr;	/*	As from IonContact.	*/
	unsigned long	xmitRate;	/*	Xmit *to* neighbor.	*/
	unsigned long	fireRate;	/*	Xmit *from* neighbor.	*/
	unsigned long	recvRate;	/*	Recv from neighbor.	*/
	unsigned long	prevXmitRate;	/*	Xmit *to* neighbor.	*/
	unsigned long	prevRecvRate;	/*	Recv from neighbor.	*/
	PsmAddress	node;		/*	Points to IonNode.	*/
	unsigned int	owltInbound;	/*	In seconds.		*/
	unsigned int	owltOutbound;	/*	In seconds.		*/
} IonNeighbor;

/*	The volatile database object encapsulates the current volatile
 *	state of the database.						*/

typedef struct
{
	int		clockPid;	/*	For stopping rfxclock.	*/
	int		deltaFromUTC;	/*	In seconds.		*/
	PsmAddress	nodes;		/*	SM list: IonNode*	*/
	PsmAddress	neighbors;	/*	SM list: IonNeighbor*	*/
	PsmAddress	probes;		/*	SM list: IonProbe*	*/
} IonVdb;

#ifndef MTAKE
#define MTAKE(size)	allocFromIonMemory(__FILE__, __LINE__, size)
#define MRELEASE(addr)	releaseToIonMemory(__FILE__, __LINE__, addr)
#endif

extern void		*allocFromIonMemory(char *, int, size_t);
extern void		releaseToIonMemory(char *, int, void *);
extern void		*ionMemAtoP(unsigned long);
extern unsigned long	ionMemPtoA(void *);

extern int		ionInitialize(	IonParms *parms,
					unsigned long ownNodeNbr);
extern int		ionAttach();
extern void		ionDetach();
extern void		ionProd(	unsigned long fromNode,
					unsigned long toNode,
					unsigned long xmitRate,
					unsigned int owlt);
extern void		ionTerminate();

extern Sdr		getIonsdr();
extern Object		getIonDbObject();
extern PsmPartition	getIonwm();
extern int		getIonMemoryMgr();
extern IonVdb		*getIonVdb();
extern char		*getIonWorkingDirectory();
extern unsigned long	getOwnNodeNbr();

extern int		startIonMemTrace(int size);
extern void		printIonMemTrace(int verbose);
extern void		clearIonMemTrace();
extern void		stopIonMemTrace();

extern void		ionOccupy(	int size);
extern void		ionVacate(	int size);

#define	TIMESTAMPBUFSZ	20

extern int		setDeltaFromUTC(int newDelta);
extern time_t		getUTCTime();	/*	UTC scale, 1970 epoch.	*/

extern time_t		readTimestampLocal(char *timestampBuffer,
					time_t referenceTime);
extern time_t		readTimestampUTC(char *timestampBuffer,
					time_t referenceTime);
extern void		writeTimestampLocal(time_t timestamp,
					char *timestampBuffer);
extern void		writeTimestampUTC(time_t timestamp,
					char *timestampBuffer);

#define extractSdnv(into, from, remnant) \
if (_extractSdnv(into, (unsigned char **) from, remnant, __LINE__) < 1) return 0
extern int		_extractSdnv(	unsigned long *into,
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
