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

In the following section, we will focus on a small subset of ICI APIs that will enable an external application to access and create data objects inside ION's SDR in the process of requesting and receiving BP services from ION. In order to write a fully functioning BP application, the user will generally need to use a combination of ICI APIs described below and [BP Service APIs](./BP-Service-API.md) described in a separate document.

## ICI API Reference

### Header

* ion.h
* platform.h

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

Initializes the semaphore in `attendant` so that it can be used for blocking ZCO space requisitions by `ionRequestZcoSpace()`. This is necessary to ensure that the invoking task will not be able to inject data into BP service until SDR space is allocated.

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

### TBD

Function Prototype

```c

```

Parameters

* None.

Return Value

* 0: success
* -1: Any error

Example Call


Description

---------------

### TBD

Function Prototype

```c

```

Parameters

* None.

Return Value

* 0: success
* -1: Any error

Example Call


Description

---------------

### TBD

Function Prototype

```c

```

Parameters

* None.

Return Value

* 0: success
* -1: Any error

Example Call


Description

---------------

## Logging, Status Message, Value Check APIs

Log to ion.log, standard way of writing formatted messages, short hand for checking for values such as ZERO, NULL, etc. 

### Header

* platform.h

---------------

### TBD

Function Prototype

```c

```

Parameters

* None.

Return Value

* 0: success
* -1: Any error

Example Call


Description

---------------
