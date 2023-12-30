# Interplanetary Communications Infrasturcutre (ICI) APIs

In this section, we will focus on a small subset of all available ICI APIs that will enable an external application to access and create data objects inside ION's SDR in the process of requesting and receiving BP services from ION. 

In order to write a fully functioning BP application, the user will generally need to use a combination of ICI APIs described below and [BP Service APIs](./BP-Service-API.md) described in a separate document.

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

* `*attendant`: pointer to a `ReqAttendant` structure that will be initialized by this function.

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

* `*attendant`: pointer to a `ReqAttendant` structure that will be destroyed by this function.

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

### ionPauseAttendent

Function Prototype

```c
void ionPauseAttendant(ReqAttendant *attendant)
```

Parameters

* `*attendant`: pointer to a `ReqAttendant` structure.

Return Value

* None

Example Call

```c
ionStopAttendant(&attendant);
```

Description

"Ends" the semaphore in attendant so that the task that is blocked on taking it is interrupted and may respond to an error or shutdown condition. This may be required when trying to quitting a user application that is currently being blocked while acquiring ZCO space.

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
* `*attendant`: the semaphore that blocks return of the function until the necessary resources has been allocated in the SDR for the creation of the ZCO

Return Value

* *location of the ZCO*: success; the requested space has become available and the ZCO has been created to hold the user data
* ((Object) -1): the function has failed
* 0: either attendant was `NULL` and sufficient space for the first extent of the ZCO was not immediately available - OR - the function was interrupted by `ionPauseAttendant` before space for the ZCO became available.

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

## SDR Database & Heap APIs

SDR persistent data are referenced in application code by `Object` values and `Address` values, both of which are simply displacements (offsets) within SDR address space. The difference between the two is that an `Object` is always the address of a block of heap space returned by some call to `sdr_malloc`, while an `Address` can refer to any byte in the address space. That is, an `Address` is the SDR functional equivalent of a C pointer in DRAM, and some Addresses point to Objects.

The number of SDR-related APIs is large. Fortunately, there are only a few APIs that an external application will likely needs to use. The following is a list of the most commonly used APIs drawn from the Database I/O and Heap Space Management categories.

------------------

### Header

```c
#include "sdr.h"
```

---------------

### sdr_malloc

Function Prototype

```c
Object sdr_malloc(Sdr sdr, unsigned long size)
```

Parameters

* `sdr`: handle to the ION SDR obtained through `ionAttach` or `bp_attach`
* `size`: size of the block to allocate

Return Value

* address of the allocated space: success; the requested space has been allocated
* 0: unable to allocate

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

* `sdr`: handle to the ION SDR obtained through `ionAttach` or `bp_attach`
* `*from`: pointer to the location where size number of bytes of data are to be copied into the allocated space in the SDR
* `size`: size of the block to allocate

Return Value

* address of the allocated space: success
* 0: unable to allocate

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

### sdr_stow

Function Prototype

```c
Object sdr_stow(sdr, variable)
```

Parameters

* `sdr`: handle to the ION SDR obtained through `ionAttach` or `bp_attach`
* `variable`: the variable whose value is to be inserted into an SDR space

Return Value

* address of the allocated space: success
* 0: unable to allocate

Description

`sdr_stow` is a macro that uses `sdr_insert` to insert a copy of variable into the dataspace. The size of variable is used as the number of bytes to copy.

---------------

### sdr_object_length

Function Prototype

```c
int sdr_object_length(Sdr sdr, Object object)
```

Parameters

* `sdr`: handle to the ION SDR obtained through `ionAttach` or `bp_attach`
* `object`: the location of an application data Object at *object*

Return Value

* address of the allocated space: success
* 0: unable to allocate

Description

Returns the number of bytes of heap space allocated to the application data at *object*.

------------------

### sdr_free

Function Prototype

```c
void sdr_free(Sdr sdr, Object object)
```

Parameters

* `sdr`: handle to the ION SDR obtained through `ionAttach` or `bp_attach`
* `object`: the location of an application data Object at *object*

Return Value

* address of the allocated space: success
* 0: unable to allocate

Description

Frees the heap space occupied by an object at *object*. The space freed are put back into the SDR memory pool and will become available for subsequent re-allocation.

------------------

### sdr_read

Function Prototype

```c
void sdr_read(Sdr sdr, char *into, Address from, int length)
```

Parameters

* `sdr`: handle to the ION SDR obtained through `ionAttach` or `bp_attach`
* `*into`: the location in memory data should be read into
* `from`: this is a location in the SDR heap
* `length`: this is the size to be read

Return Value

* address of the allocated space: success
* 0: unable to allocate

Description

Copies length characters at from (a location in the indicated SDR) to the memory location given by into. The data are copied from the shared memory region in which the SDR resides, if any; otherwise they are read from the file in which the SDR resides.

------------------

### sdr_stage

Function Prototype

```c
void sdr_stage(Sdr sdr, char *into, Object from, int length)
```

Parameters

* `sdr`: handle to the ION SDR obtained through `ionAttach` or `bp_attach`
* `*into`: the location in memory data should be read into
* `from`: this is a location in the SDR heap which is occupied by an allocated object
* `length`: this is the size to be read

Return Value

* address of the allocated space: success
* 0: unable to allocate

Description

Like `sdr_read`, this function will copy length characters at from (a location in the heap of the indicated SDR) to the memory location given by into. Unlike `sdr_get`, `sdr_stage` requires that from be the address of some allocated object, not just any location within the heap. `sdr_stage`, when called from within a transaction, notifies the SDR library that the indicated object may be updated later in the transaction; this enables the library to retrieve the object's size for later reference in validating attempts to write into some location within the object. If length is zero, the object's size is privately retrieved by SDR but none of the object's content is copied into memory.

`sdr_get` is a macro that uses `sdr_read` to load variable from the SDR address given by heap_pointer; heap_pointer must be (or be derived from) a heap pointer as returned by `sdr_pointer`. The size of variable is used as the number of bytes to copy.

----------------

### sdr_write

Function Prototype

```c
void sdr_write(Sdr sdr, Address into, char *from, int length)

```

Parameters

* `sdr`: handle to the ION SDR obtained through `ionAttach` or `bp_attach`
* `*into`: the location in SDR heap where data should be written into
* `from`: this is a location in memory where data should copied from
* `length`: this is the size to be written

Return Value

* address of the allocated space: success
* 0: unable to allocate

Description

Like `sdr_read`, this function will copy length characters at from (a location in the heap of the indicated SDR) to the memory location given by into. Unlike `sdr_get`, `sdr_stage` requires that from be the address of some allocated object, not just any location within the heap. `sdr_stage`, when called from within a transaction, notifies the SDR library that the indicated object may be updated later in the transaction; this enables the library to retrieve the object's size for later reference in validating attempts to write into some location within the object. If length is zero, the object's size is privately retrieved by SDR but none of the object's content is copied into memory.

`sdr_get` is a macro that uses `sdr_read` to load variable from the SDR address given by heap_pointer; heap_pointer must be (or be derived from) a heap pointer as returned by `sdr_pointer`. The size of variable is used as the number of bytes to copy.

----------------

## Other less used APIs

See manual pages for `ion` and `sdr` for details on the full set of ICI APIs.

