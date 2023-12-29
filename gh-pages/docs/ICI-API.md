# Interplanetary Communications Infrasturcutre (ICI) APIs

## Background

For the reader's convenience, the description of the ICI package in the [ION Deployment Guide](./ION-Deployment-Guide.md) is repeated here:

>The ICI package in ION provides a number of core services that, from ION's point of view, implement what amounts to an extended POSIX-based operating system. ICI services include the following:
>
>Platform
>
> The platform system contains operating-system-sensitive code that enables ICI to present a single, consistent programming interface to those common operating system services that multiple ION modules utilize. For example, the platform system implements a standard semaphore abstraction that may invisibly be mapped to underlying POSIX semaphores, SVR4 IPC semaphores, Windows Events, or VxWorks semaphores, depending on which operating system the package is compiled for. The platform system also implements a standard shared-memory abstraction, enabling software running on operating systems both with and without memory protection to participate readily in ION's shared-memory-based computing environment.
>
>Personal Space Management (PSM)
>
> Although sound flight software design may prohibit the uncontrolled dynamic management of system memory, private management of assigned, fixed blocks of system memory is standard practice. Often that private management amounts to merely controlling the reuse of fixed-size rows in static tables, but such techniques can be awkward and may not make the most efficient use of available memory. The ICI package provides an alternative, called PSM, which performs high-speed dynamic allocation and recovery of variable-size memory objects within an assigned memory block of fixed size. A given PSM-managed memory block may be either private or shared memory.
>
>Memmgr
>
> The static allocation of privately-managed blocks of system memory for different purposes implies the need for multiple memory management regimes, and in some cases a program that interacts with multiple software elements may need to participate in the private shared-memory management regimes of each. ICI's memmgr system enables multiple memory managers -- for multiple privately-managed blocks of system memory -- to coexist within ION and be concurrently available to ION software elements.
>
>Lyst
>
>The lyst system is a comprehensive, powerful, and efficient system for managing doubly-linked lists in private memory. It is the model for a number of other list management systems supported by ICI; as noted earlier, linked lists are heavily used in ION inter-task communication.
>
>Llcv
>
>The llcv (Linked-List Condition Variables) system is an inter-thread communication abstraction that integrates POSIX thread condition variables (vice semaphores) with doubly-linked lists in private memory.
>
>Smlist
>
>Smlist is another doubly-linked list management service. It differs from lyst in that the lists it manages reside in shared (rather than private) DRAM, so operations on them must be semaphore-protected to prevent race conditions.
>
>SmRbt
>
>The SmRbt service provides mechanisms for populating and navigating "red/black trees" (RBTs) residing in shared DRAM. RBTs offer an alternative to linked lists: like linked lists they can be navigated as queues, but locating a single element of an RBT by its "key" value can be much quicker than the equivalent search through an ordered linked list.
>
>Simple Data Recorder (SDR)
>
>SDR is a system for managing non-volatile storage, built on exactly the same model as PSM. Put another way, SDR is a small and simple "persistent object" system or "object database" management system. It enables straightforward management of linked lists (and other data structures of arbitrary complexity) in non-volatile storage, notionally within a single file whose size is pre-defined and fixed.
>
>SDR includes a transaction mechanism that protects database integrity by ensuring that the failure of any database operation will cause all other operations undertaken within the same transaction to be backed out. The intent of the system is to assure retention of coherent protocol engine state even in the event of an unplanned flight computer reboot in the midst of communication activity.
>
>Sptrace
>
>The sptrace system is an embedded diagnostic facility that monitors the performance of the PSM and SDR space management systems. It can be used, for example, to detect memory "leaks" and other memory management errors.
>
>Zco
>
>ION's zco (zero-copy objects) system leverages the SDR system's storage flexibility to enable user application data to be encapsulated in any number of layers of protocol without copying the successively augmented protocol data unit from one layer to the next. It also implements a reference counting system that enables protocol data to be processed safely by multiple software elements concurrently -- e.g., a bundle may be both delivered to a local endpoint and, at the same time, queued for forwarding to another node -- without requiring that distinct copies of the data be provided to each element.
>
>Rfx
>
>The ION rfx (R/F Contacts) system manages lists of scheduled communication opportunities in support of a number of LTP and BP functions.
>
>Ionsec
>
>The IONSEC (ION security) system manages information that supports the implementation of security mechanisms in the other packages: security policy rules and computation keys.

In the following section, we will focus on a small subset of all available ICI APIs that will enable an external application to access and create data objects inside ION's SDR in the process of requesting and receiving BP services from ION. In order to write a fully functioning BP application, the user will generally need to use a combination of ICI APIs described below and [BP Service APIs](./BP-Service-API.md) described in a separate document.

## ION APIs

### Header

```c
#include "ion.h"
```

---------------

### ionAttach

Function Prototype

```c
extern int	ionAttach();
```

Parameters

* None.

Return Value

* 0: success
* -1: any error

Example Call

```c
if (ionAttach() < 0)
{
    putErrmsg("bpadmin can't attach to ION.", NULL);
    
    /* User calls error handling routine. */
}
```

Description

Attaches the invoking task to ION infrastructure as previously established by running the ionadmin utility program. After successful execution, the handle to the ION SDR can be obtained by a separte API call. `putErrmsg` is a ION logging API, which will be described later in this document.

---------------

### ionDetach

Function Prototype

```c
extern void	ionDetach();
```

Parameters

* None

Return Value

* None

Example Call

```c
ionDetach();
```

Description

Detaches the invoking task from ION infrastructure. In particular, releases handle allocated for access to ION's non-volatile database.

---------------

### ionTerminate

Function Prototype

```c
extern void ionTerminate();
```

Parameters

* None

Return Value

* None

Example Call

```c
ionTerminate();
```

Description

Shuts down the entire ION node, terminating all daemons. The state of the SDR will be destroyed during the termination process, even if the SDR heap is implemented in a non-volatile storage, such as a file.

---------------

### ionStartAttendant

Function Prototype

```c
extern int	ionStartAttendant(ReqAttendant *attendant);
```

Parameters

* `attendant`: pointer to a `ReqAttendant` structure that will be initialized by this function.

```c
typedef struct
{
	sm_SemId	semaphore;
} ReqAttendant;
```

Return Value

* 0: success
* -1: any error

Example Call

```c
if (ionStartAttendant(&attendant))
{
    putErrmsg("Can't initialize blocking transmission.", NULL);

    /* user implemented error handling routine */
}
```

Description

Initializes the semaphore in `attendant` so that it can be used for blocking a pending ZCO space request. This is necessary to ensure that the invoking task will not be able to inject data into BP service until SDR space is allocated.

---------------

### ionStopAttendant

Function Prototype

```c
extern void	ionStopAttendant(ReqAttendant *attendant);
```

Parameters

* `attendant`: pointer to a `ReqAttendant` structure that will be destroyed by this function.

Return Value

* 0: success
* -1: Any error

Example Call

```c
ionStopAttendant(&attendant);
```

Description

Destroys the semaphore in `attendant`, preventing a potential resource leak. This is typically called at the end of a BP application, after all user data have been injected into the SDR.

---------------

### ionCreateZco

Function Prototype

```c
extern Object		ionCreateZco(	ZcoMedium source,
					Object location,
					vast offset,
					vast length,
					unsigned char coarsePriority,
					unsigned char finePriority,
					ZcoAcct acct,
					ReqAttendant *attendant);
```

Parameters

* `source`: the type of ZCO to be created. Each source data object may be either a file, a "bulk" item in mass storage, an object in SDR heap space (identified by heap address stored in an "object reference" object in SDR heap), an array of bytes in SDR heap space (identified by heap address), or another ZCO.

```c
typedef enum
{
	ZcoFileSource = 1,
	ZcoBulkSource = 2,
	ZcoObjSource = 3,
	ZcoSdrSource = 4,
	ZcoZcoSource = 5
} ZcoMedium;
```

* `location`: the location in the heap where a single extent of source data resides. This data is usually placed there by the user application via the `sdr_malloc()` API which will be discussed later.
* `offset`: the offset within the source data object where the first byte of the ZCO should be placed.
* `length`: the length of the ZCO to be created.
* `coarsePriority`: this sets the basic class of service (COS) inherented from BPv6. Although COS is not specified in BPv7, ION API supports this feature when creating ZCOs. From lowest to the higher priority, it can be set to `BP_BULK_PRIORITY` (value =0), `BP_STD_PRIORITY` (value = 1), or `BP_EXPEDITED_PRIORITY` (value = 2).
* `finePriority`: this is inherented from BPv6 COS and it is the finer grain priority levels (level 0 to 254) within the class of `BP_STD_PRIORITY`. Typically this is set to the value of 0. 
* `acct`: The accounting category for the ZCO, it is either `ZcoInbound` (0), `ZcoOutbound` (1), or `ZcoUnknown` (2). If a ZCO is created for the purpose of transmission to another node, this parameter is typically set to `ZcoOutbound`.
* `attendant`: the semaphore that blocks return of the function until the necessary resources has been allocated in the SDR for the creation of the ZCO

Return Value

* *location of the ZCO*: success; the requested space has become available and the ZCO has been created to hold the user data
* `((Object) -1)`: the function has failed
* `0`: either attendant was `NULL` and sufficient space for the first extent of the ZCO was not immediately available - OR - the function was interrupted by `ionPauseAttendant` before space for the ZCO became available.

Example Call

```c
SdrObject bundleZco;

bundleZco = ionCreateZco(ZcoSdrSource, extent, 0, lineLength,
		BP_STD_PRIORITY, 0, ZcoOutbound, &attendant);
if (bundleZco == 0 || bundleZco == (Object) ERROR)
{
	putErrmsg("Can't create ZCO extent.", NULL);
	/* user implemented error handling routine goes here */
}
```

Description

This function provides a "blocking" implementation of admission control in ION. Like zco_create(), it constructs a zero-copy object (see zco(3)) that contains a single extent of source data residing at location in source, of which the initial offset number of bytes are omitted and the next length bytes are included. By providing an attendant semaphore, initialized by `ionStartAttendant`, ionCreateZco() can be execute as a blocking call so long as the total amount of space in `source` that is available for new ZCO formation is less than length. ionCreateZco() operates by calling `ionRequestZcoSpace`, then pending on the semaphore in attendant as necessary before creating the ZCO and then populating it with the user's data.

---------------

### Other less used ICI APIs

The following is a list of less-frequently used APIs by an external application. However, they might be valuable if the user wishes to implement or customize some of the behavior of the ION node or the external application requires more detailed control of ION behavior. Please see the manual page for `ion` for more details.

* ionProd
* ionPauseAttendant
* ionResumeAttendant
* ionRequestZcoSpace
* ionSpaceAwarded
* ionShred
* ionAppendZcoExtend
* char *getIonVersionNbr()
* getIonsdr()
* getIonwm()
* getIonMemoryMgr()
* ionLocked()
* getOwnNodeNbr()
* getCtime()
* ionClockIsSynchronized()
* writeTimestampLocal
* writeTimestampUTC
* readTimestampLocal
* readTimestampUTC

---------------

## SDR Heap Space Management APIs

SDR persistent data are referenced in application code by Object values and Address values, both of which are simply displacements (offsets) within SDR address space. The difference between the two is that an Object is always the address of a block of heap space returned by some call to sdr_malloc(), while an Address can refer to any byte in the address space. That is, an Address is the SDR functional equivalent of a C pointer in DRAM, and some Addresses point to Objects.

The number of SDR-related APIs is large. Fortunately, there are only a few APIs that an external application needs to use. The following is a list of the most commonly used APIs drawn from the Heap Space Management category.

### Header

```h
#include "sdr.h"
```

---------------

### sdr_malloc

Function Prototype

```c
Object sdr_malloc(Sdr sdr, unsigned long size)
```

Parameters

* sdr: handle to the ION SDR obtained through `ionAttach` or `bp_attach`
* size: size of the block to allocate

Return Value

* *address of the allocated space*: success; the requested space has been allocated
* `0`: unable to allocate

Example Call

```c
CHKZERO(sdr_begin_xn(sdr));
extent = sdr_malloc(sdr, lineLength);
if (extent)
{
	sdr_write(sdr, extent, text, lineLength);
}

if (sdr_end_xn(sdr) < 0)
{
	putErrmsg("No space for ZCO extent.", NULL);
	bp_detach();
	return 0;
}
```

In this example, a 'critical section' has been implemented by a pair of API calls: `sdr_begin_xn` and `sdr_end_xn`. The critical section is used to ensure that the SDR is not altered while the space allocation is in progress. These APIs will be explained later in this document. The `sdr_write` API is used to write data into the space acquired by `sdr_malloc`.

It may seem strange that the failure to allocate space or to write the data into the allocated space relies on checking the return value of `sdr_end_xn` instead sdr_malloc and sdr_write functions. This is because when `sdr_end_xn` returns negative value, it indicates that an SDR transaction was already terminated, which occurs when `sdr_malloc` or `sdr_write` fails. So this is a convenient way to detect the failure of two calls at the same time by checking the `sdr_end_xn` calls return value.

Description

Allocates a block of space from the indicated SDR's heap. The maximum size is 1/2 of the maximum address space size (i.e., 2G for a 32-bit machine). Returns block address if successful, zero if block could not be allocated.

---------------

### sdr_insert

Function Prototype

```c
Object sdr_insert(Sdr sdr, char *from, unsigned long size)
```

Parameters

* sdr: handle to the ION SDR obtained through `ionAttach` or `bp_attach`
* from: pointer to the location where size number of bytes of data are to be copied into the allocated space in the SDR
* size: size of the block to allocate

Return Value

* *address of the allocated space*: success; the requested space has been allocated and populated with the *size* number of bytes from memory location *from* 
* `0`: unable to allocate

Example Call

```c
CHKZERO(sdr_begin_xn(sdr));
extent = sdr_insert(sdr, text, lineLength);
if (sdr_end_xn(sdr) < 0)
{
	putErrmsg("No space for ZCO extent.", NULL);
	bp_detach();
	return 0;
}
```

Description

This function combines the action of `sdr_malloc` ans `sdr_write`. It first uses `sdr_malloc` to obtain a block of space of size size and, if this allocation is successful, uses `sdr_write` to copy size bytes of data from memory at from into the newly allocated block. 

---------------

