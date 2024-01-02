# LTP Underlying Communications API

In the Licklider Transmission Protocol (LTP) Specification issued by [CCSDS 734.1-B-1](https://public.ccsds.org/Pubs/734x1b1.pdf), the elements of a LTP architecture is shown as follows:

![LTP Architecture](images/LTP-Architecture-CCSDS.png)

The `LTP Engine` and `MIB` is implemented and configured by ION, and the `Client Service Instance` is either BPv6 or BPv7. The storage is provided by the host system through the ICI APIs.

The `Underlying Communication Protocol` element is responsible for data and control message exchanges between two peered LTP Engines. It is not responsible for flow control, error correction/detection, and in-ordered delivery.

For a spacecraft, the LTP Engine will execute the LTP protocol logic and handing the LTP segments to the underlying communication services provided in the form of a simple UDP socket or a radio frequency/optical telecommunication system. In ION, the standard underlying communications protocol is UDP since it is widely available in terrestrial computer systems. In actual deployment, the UDP protocol may need to be substituted by a different ground-based or flight communications system.

In the document we describe a few essential APIs for any externally implemented underlying communication protocols to interface with LTP engine and perform the most basic tasks of (a) submitting a received LTP segments to the LTP Engine for processing and (b) acquiring an LTP segment from the LTP Engine for transmission to its peer.

## Connecting to the LTP Engine

There are several steps for an external application to connecting to LTP:

1. The ltp service must be running on the host system. The ltp service is started by the ION system and is configured by the `.ltprc` file processed `ltpadmin`. See the [Configuration File Tutorial](./Basic-Configuration-File-Tutorial.md) to understand how BP and LTP services are instantiated.
    * Typically, to ensure that the ltp service is running before the communications protocols try to connect to it, the underlying communication protocol service is invoked as part of LTP instantiation. See manual page for `ltprc` for more details.
2. The external application must make sure LTP is initialized by calling the `ltpInit()` API.
3. Once `ltpInit` called returned successfully, it must obtain access to ION SDR and detemine the associated LTP `span` (based on a peer engine number) for which communication service will be provisioned.  This is done by using the `findSpan()` API. A `span` defines the communication parameters between two LTP engine peers.
4. Acquire the semaphore used by the associated LTP engines - for the span - to indicate the availability of a segment for transmission. The presences of a valid semaphore is also indication that the span is currently active.
5. Use the `ltpDequeueOUtboundSegment` API to acquire each available segment from the LTP Engine for transmission to the peer entity.

In the following section we will describe the *private* APIs used by the underlying communication protocols. There are other APIs for external processes to use LTP as a reliable point-to-point data transmission service, but they are not described in this document; they are available in the manual pages.

## LTP API - for underlying comm protocol

### Header

```c
#include "ltpP.h"
```

### ltpInit

Function Prototype

```c
extern int	ltpInit(int estMaxExportSessions);
```

Parameters

* `estMaxExportSessions`: name of the endpoint

Return Value

* 0: success
* -1: any error

Example Call

```c
/*	Note that ltpadmin must be run before the first
 *	invocation of ltplso, to initialize the LTP database
 *	(as necessary) and dynamic database.*/

if (ltpInit(0) < 0)
{
    putErrmsg("aoslso can't initialize LTP.", NULL);
    
    /* user error handling routine here */
}
```

Description

This call attaches to ION and either initializes a new LTP database or loads the LTP database of an existing service. If the value of `estMaxExportSessions` is positive, the LTP service will be initialized with the specified maximum number of export sessions. If the value of `estMaxExportSessions` is zero or negative, then `ltpInit` will quit if no existing LTP service is found. 

**NOTE**: for the underlying communication protocol implementation, setting ltpInit(0) is appropriate since the intent is to load an existing LTP service only.

Once a LTP service is found, it loads the address to the LTP database object, as defined by `LtpDB` in `ltpP.h`.

#### LtpDB

```c
/* Database structure */

typedef struct
{
	uvast		ownEngineId;
	Sdnv		ownEngineIdSdnv;
	unsigned int	maxBacklog;
	Object		deliverables;	/*	SDR list: Deliverable	*/

	/*	estMaxExportSessions is used to compute the number
	 *	of rows in the export sessions hash table in the LTP
	 *	database.  If the summation of maxExportSessions over
	 *	all spans exceeds estMaxExportSessions, LTP export
	 *	session lookup performance may be compromised.		*/

	int		estMaxExportSessions;
	unsigned int	ownQtime;
	unsigned int	enforceSchedule;/*	Boolean.		*/
	double		maxBER;		/*	Max. bit error rate.	*/
	LtpClient	clients[LTP_MAX_NBR_OF_CLIENTS];
	unsigned int	sessionCount;
	Object		exportSessionsHash;
#if CLOSED_EXPORTS_ENABLED
	Object		closedExports;	/*	SDR list: CLosedExport	*/
#endif
	Object		deadExports;	/*	SDR list: ExportSession	*/
	Object		spans;		/*	SDR list: LtpSpan	*/
	Object		seats;		/*	SDR list: LtpSeat	*/
	Object		timeline;	/*	SDR list: LtpEvent	*/
	unsigned int	maxAcqInHeap;
	unsigned long	heapBytesReserved;
	unsigned long	heapBytesOccupied;
	unsigned long	heapSpaceBytesReserved;
	unsigned long	heapSpaceBytesOccupied;
} LtpDB;
```

This object hold the general information for LTP service in ION.



-----------------------

### findSpan

Function Prototype

```c
extern void	findSpan(uvast engineId, LtpVspan **vspan,
				PsmAddress *vspanElt);
```
Parameters

* `engineId`: The engine number of the peer engine for the given span
* `vspan`: pointer to the pointer of the volatile LTP span object that encapsulates the current state of the LTP span
* `vspanElt`: pointer to the address value stored in a list element that points to the volatile span object vspan

Return Value:



```c
sdr = getIonsdr();
CHKZERO(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
findSpan(remoteEngineId, &vspan, &vspanElt);
if (vspanElt == 0)
{
    sdr_exit_xn(sdr);
    putErrmsg("No such engine in database.", itoa(remoteEngineId));
    return 1;
}

if (vspan->lsoPid != ERROR && vspan->lsoPid != sm_TaskIdSelf())
{
    sdr_exit_xn(sdr);
    putErrmsg("LSO task is already started for this span.",
        itoa(vspan->lsoPid));
    return 1;
}
```



