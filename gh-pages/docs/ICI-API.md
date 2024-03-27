# Interplanetary Communications Infrastructure (ICI) APIs

This section will focus on a subset of ICI APIs that enables an external application to create, manipulate, and access data objects inside ION's SDR.

## ICI APIs

### Header

```c
#include "ion.h"
```

### MTAKE & MRELEASE
```c
#define MTAKE(size)	allocFromIonMemory(__FILE__, __LINE__, size)
#define MRELEASE(addr)	releaseToIonMemory(__FILE__, __LINE__, addr)
```

* __MTAKE__ and __MRELEASE__ provide syntactically terse ways of calling `allocFromIonMemory` and `releaseToIonMemory`, the functional equivalent of `malloc` and `free` for ION. The allocated memory space comes from the ION working memory, which has been pre-allocated during the ION configuration.
* __FILE__ and __LINE__ provide the source file name and line number of the calling function and can assist in debugging and tracking down memory leaks.

-----------------

### ionAttach ###

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

Attached is the invoking task to ION infrastructure as previously established by running the ionadmin utility program. After successful execution, the handle to the ION SDR can be obtained by a separate API call. `putErrmsg` is an ION logging API, which will be described later in this document.

---------------

### ionDetach ###

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

-------------

### ionStartAttendant ###

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

Initializes the semaphore in `attendant` to block a pending ZCO space request. This is necessary to ensure that the invoking task cannot inject data into the Bundle Protocol Agent until SDR space has been allocated.

------------------

### ionStopAttendant

Function Prototype

```c
extern void	ionStopAttendant(ReqAttendant *attendant);
```

Parameters

* `*attendant`: pointer to a `ReqAttendant` structure that will be destroyed by this function.

Return Value

* 0: Success
* -1: Any error

Example Call

```c
ionStopAttendant(&attendant);
```

Description

Destroys the semaphore in `attendant`, preventing a potential resource leak. This is typically called at the end of a BP application after all user data have been injected into the SDR.

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

"Ends" the semaphore in attendant so that the task blocked on taking it is interrupted and may respond to an error or shutdown condition. This may be required when trying to quit a blocked user application while acquiring ZCO space.

------------------

### ionCreateZco

Function Prototype

```c
extern Object ionCreateZco(	ZcoMedium source,
			Object location,
			vast offset,
			vast length,
			unsigned char coarsePriority,
			unsigned char finePriority,
			ZcoAcct acct,
			ReqAttendant *attendant);
```

Parameters

Source: the type of ZCO to be created. Each source data object may be either a file, a "bulk" item in mass storage, an object in SDR heap space (identified by heap address stored in an "object reference" object in SDR heap), an array of bytes in SDR heap space (identified by heap address), or another ZCO.

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

* `location`: the location in the heap where a single extent of source data resides. The user application usually places this data via `sdr_malloc()`, which will be discussed later.
* `offset`: the offset within the source data object where the first byte of the ZCO should be placed.
* `length`: the length of the ZCO to be created.
* `coarsePriority`: this sets the bundle's Class of Service (COS) as an inherited feature from BPv6. Although COS is not specified in BPv7, ION API supports this feature when creating ZCOs. From the lowest to the highest priorities, it can be set to the following values:
	- `BP_BULK_PRIORITY` (value =0)
	- `BP_STD_PRIORITY` (value = 1), or 
	- `BP_EXPEDITED_PRIORITY` (value = 2)
* `finePriority`: this is inherited from BPv6 COS and is the finer grain priority level (level 0 to 254) within the `BP_STD_PRIORITY` class. The default value is 0.
* `acct`: The accounting category for the ZCO, which is either `ZcoInbound` (0), `ZcoOutbound` (1), or `ZcoUnknown` (2). If a ZCO is created for transmission to another node, this parameter is typically set to `ZcoOutbound`.
* `*attendant`: the semaphore that blocks the return of the function until the necessary resources have been allocated in the SDR for the ZCO

Return Value

* Location of the ZCO: on success, the requested space has become available, and the ZCO has been created to hold the user data
* `((Object) -1)`: the function has failed
* `0`: either the attendant was `NULL` and sufficient space for the first extent of the ZCO was not immediately available, or the function was interrupted by `ionPauseAttendant` before space for the ZCO became available.

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

This function provides a "blocking" implementation of admission control in ION. Like zco_create(), it constructs a zero-copy object (see zco(3)) that contains a single extent of source data residing at a location in the source, of which the initial offset number of bytes are omitted and the subsequent `length` bytes are included. By providing an attendant semaphore, initialized by `ionStartAttendant`, `ionCreateZco()` can be executed as a blocking call so long as the total amount of space in the `source` available for new ZCO formation is less than the length. `ionCreateZco()` operates by calling `ionRequestZcoSpace`, then pending on the semaphore in attendant as necessary before creating the ZCO and then populating it with the user's data.

---

## SDR Database & Heap APIs
SDR persistent data are referenced by object and address values in the application code, simply displacements (offsets) within the SDR address space. The difference between the two is that an `Object` is always the address of a block of heap space returned by some call to `sdr_malloc`, while an `Address` can refer to any byte in the SDR address space. An `Address` is the SDR functional equivalent of a C pointer; some Addresses point to actual Objects.

The number of SDR-related APIs is significant, and most are used by ION internally. Fortunately, there are only a few APIs that an external application will likely need to use. The following list of the most commonly used APIs is drawn from the _Database I/O_ and the _Heap Space Management_ API categories.

---

### Header

```c
#include "sdr.h"
```

---

### sdr_malloc

Function Prototype

```c
Object sdr_malloc(Sdr sdr, unsigned long size)
```

Parameters

* `sdr`: handle to the ION SDR obtained through `ionAttach` or `bp_attach`
* `size`: size of the block to allocate

Return Value

* address of the allocated space: Success. The requested space has been allocated
* 0: Failure

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

In this example, a 'critical section' has been implemented by API calls: `sdr_begin_xn` and `sdr_end_xn`. The critical section ensures that the SDR is not altered while the space allocation is in progress. These APIs will be explained later in this document. The `sdr_write` API is used to write data into the space acquired by `sdr_malloc`.

It may seem strange that failing to allocate space or write the data into the allocated space relies on checking the return value of `sdr_end_xn` instead of sdr_malloc and sdr_write functions. This is because when `sdr_end_xn` returns a negative value, it indicates that an SDR transaction was already terminated, which occurs when `sdr_malloc` or `sdr_write` fails. So, this is a convenient way to detect the failure of two calls simultaneously by checking the `sdr_end_xn` call return value.

Description

Allocates a block of space from the indicated SDR's heap. The maximum size is 1/2 of the maximum address space size (i.e., 2G for a 32-bit machine). Returns block address if successful, zero if block could not be allocated.

---

### sdr_insert

Function Prototype

```c
Object sdr_insert(Sdr sdr, char *from, unsigned long size)
```

Parameters

* `sdr`: handle to the ION SDR obtained through `ionAttach` or `bp_attach`
* `*from`: pointer to the location where several bytes (specified by `size`) of data are to be copied into the allocated space in the SDR
* `size`: size of the block to allocate

Return Value

* address of the allocated space: success
* 0: Failure

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

This function combines the action of `sdr_malloc` and `sdr_write`. It first uses `sdr_malloc` to obtain a block of space, and if this allocation is successful, it uses `sdr_write` to copy size bytes of data from memory into the newly allocated block.

---

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
* 0: Failure

Description

`sdr_stow` is a macro that uses `sdr_insert` to insert a copy of a variable into the dataspace. The size of the variable is used as the number of bytes to copy.

---

### sdr_object_length

Function Prototype

```c
int sdr_object_length(Sdr sdr, Object object)
```

Parameters

* `sdr`: handle to the ION SDR obtained through `ionAttach` or `bp_attach`
* `object`: the location of an application data object

Return Value

* address of the allocated space: success
* 0: Failure

Description

Returns the number of bytes of heap space allocated to the application data at *object*.

---

### sdr_free

Function Prototype

```c
void sdr_free(Sdr sdr, Object object)
```

Parameters

* `sdr`: handle to the ION SDR obtained through `ionAttach` or `bp_attach`
* `object`: the location of an application data Object

Return Value

* address of the allocated space: success
* 0: failure

Description

Frees the heap space occupied by an object at *object*. The space freed are put back into the SDR memory pool and will become available for subsequent re-allocation.

---

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
* 0: Failure

Description

Copies length characters from (a location in the indicated SDR) to the memory location given by into. The data are copied from the shared memory region in which the SDR resides, if any; otherwise, they are read from the file in which the SDR resides.

---

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
* 0: Failure

Description

Like `sdr_read`, this function will copy length characters from (a location in the heap of the indicated SDR) to the memory location given by into. Unlike `sdr_get`, `sdr_stage` requires that from be the address of some allocated object, not just any location within the heap. `sdr_stage`, when called from within a transaction, notifies the SDR library that the indicated object may be updated later in the transaction; this enables the library to retrieve the object's size for later reference in validating attempts to write into some location within the object. If the length is zero, the object's size is privately retrieved by SDR, but none of the object's content is copied into memory.

`sdr_get` is a macro that uses `sdr_read` to load variables from the SDR address given by heap_pointer; heap_pointer must be (or be derived from) a heap pointer as returned by `sdr_pointer`. The size of the variable is used as the number of bytes to copy.

---

### sdr_write

Function Prototype

```c
void sdr_write(Sdr sdr, Address into, char *from, int length)

```

Parameters

* `sdr`: handle to the ION SDR obtained through `ionAttach` or `bp_attach`
* `*into`: the location in the SDR heap where data should be written into
* `from`: this is a location in memory where data should copied from
* `length`: this is the size to be written

Return Value

* address of the allocated space: success
* 0: Failure

Description

Like `sdr_read`, this function will copy length characters from (a location in the heap of the indicated SDR) to the memory location given by into. Unlike `sdr_get`, `sdr_stage` requires that from be the address of some allocated object, not just any location within the heap. `sdr_stage`, when called from within a transaction, notifies the SDR library that the indicated object may be updated later in the transaction; this enables the library to retrieve the object's size for later reference in validating attempts to write into some location within the object. If length is zero, the object's size is privately retrieved by SDR but none of the object's content is copied into memory.

`sdr_get` is a macro that uses `sdr_read` to load variables from the SDR address given by heap_pointer; heap_pointer must be (or be derived from) a heap pointer as returned by `sdr_pointer`. The size of the variable is used as the number of bytes to copy.

---

## SDR Transaction APIs

The following APIs manage transactions by implementing a critical section in which SDR content cannot be modified.

### Header

```c
#include "sdrxn.h"
```

---

### sdr_begin_xn

Function Prototype

```c
int sdr_begin_xn(Sdr sdr)
```

Parameters

* `sdr`: handle to the ION SDR obtained through `ionAttach` or `bp_attach`

Return Value

* 1: Success
* 0: Failure

Description

Initiates a transaction. Returns 1 on success, 0 on any failure. Note that transactions are single-threaded; any task that calls `sdr_begin_xn` is suspended until all previously requested transactions have been ended or canceled.

---

### sdr_in_xn

Function Prototype

```c
int sdr_in_xn(Sdr sdr)
```

Parameters

* `sdr`: handle to the ION SDR obtained through `ionAttach` or `bp_attach`

Return Value

* 1: transaction in progress
* 0: no transaction in progress

Description

Returns 1 if called in the course of a transaction, 0 otherwise.

---

### sdr_exit_xn

Function Prototype

```c
void sdr_exit_xn(Sdr sdr)
```

Parameters

* `sdr`: handle to the ION SDR obtained through `ionAttach` or `bp_attach`

Return Value

* none

Description

Simply abandons the current transaction, ceasing the calling task's lock on ION. __MUST NOT be used if any dataspace modifications were performed during the transaction__; `sdr_end_xn` must be called instead to commit those modifications.

---

### sdr_cancel_xn

Function Prototype

```c
void sdr_cancel_xn(Sdr sdr)
```

Parameters

* `sdr`: handle to the ION SDR obtained through `ionAttach` or `bp_attach`

Return Value

* none

Description

Cancels the current transaction. If reversibility is enabled for the SDR, canceling a transaction reverses all heap modifications performed during that transaction.

---

### sdr_end_xn

Function Prototype

```c
int sdr_end_xn(Sdr sdr)
```

Parameters

* `sdr`: handle to the ION SDR obtained through `ionAttach` or `bp_attach`

Return Value

* 0: transaction completed successfully
* -1: transaction unable to complete due to failed operations; transaction canceled

Description

Ends the current transaction. Returns 0 if the transaction was completed without any error; returns -1 if any operation performed in the course of the transaction failed, in which case the transaction was automatically canceled.

---

## SDR List management APIs

The SDR list management functions manage doubly-linked lists in managed SDR heap space. The functions manage two kinds of objects: lists and list elements. A list knows how many elements it contains and what its start and end elements are. An element knows what list it belongs to and the elements before and after it in the list. An element also knows its content, which is normally the SDR Address of some object in the SDR heap. A list may be sorted, which speeds the process of searching for a particular element.

### Header

```c
#include "sdr.h"

typedef int (*SdrListCompareFn)(Sdr sdr, Address eltData, void *argData);
typedef void (*SdrListDeleteFn)(Sdr sdr, Object elt, void *argument);
```

### Callback: SdrListCompareFn

### Callback: SDRListDEleteFn

USAGE

When inserting elements or searching a list, the user may optionally provide a compare function of the form:

```c
int user_comp_name(Sdr sdr, Address eltData, void *dataBuffer);
```

When provided, this function is automatically called by the sdrlist function being invoked; when the function is called, it is passed the content of a list element (eltData, nominally the Address of an item in the SDR's heap space) and an argument, dataBuffer, which is nominally the address in the local memory of some other item in the same format. The user-supplied function normally compares some key values of the two data items. It returns 0 if they are equal, an integer less than 0 if eltData's key value is less than that of dataBuffer, and an integer greater than 0 if eltData's key value is greater than that of dataBuffer. These return values will produce a list in ascending order.
If the user desires the list to be in descending order, the function must reverse the signs of these return values.

When deleting an element or destroying a list, the user may optionally provide a delete function of the form:

```c
void user_delete_name(Sdr sdr, Address eltData, void *argData)
```

When provided, this function is automatically called by the sdrlist function being invoked; when the function is called, it is passed the content of a list element (eltData, nominally the Address of an item in the SDR's heap space) and an argument, argData, which if non-NULL is normally the address in the local memory of a data item providing context for the list element deletion. The user-supplied function performs any application-specific cleanup associated with deleting the element, such as freeing the element's content data item and/or other SDR heap space associated with the element.

---

### sdr_list_insert_first

### sdr_list_insert_last

Function Prototype

```c
Object sdr_list_insert_first(Sdr sdr, Object list, Address data)
Object sdr_list_insert_last(Sdr sdr, Object list, Address data)
```

Parameters

* `sdr`: handle to the ION SDR obtained through `ionAttach` or `bp_attach`
* `list`: a list
* `data`: an address in SDR

Return Value

* `address of newly created element`: success
* `0`: any error

Description

Creates a new element and inserts it at the front/end of the list. This function should not be used to insert a new element into any **ordered** list; use `sdr_list_insert`() instead.

---

### sdr_list_create

Function Prototype

```c
Object sdr_list_create(Sdr sdr)
```

Parameters

* `sdr`: handle to the ION SDR obtained through `ionAttach` or `bp_attach`

Return Value

* `address of newly created list`: Success
* `0`: any error

Description

Creates a new list object in the SDR; the new list object initially contains no list elements. Returns the address of the new list or zero on any error.

---

### sdr_list_length

Function Prototype

```c
int sdr_list_length(Sdr sdr, Object list)
```

Parameters

* `sdr`: handle to the ION SDR obtained through `ionAttach` or `bp_attach`
* `list`: a list in SDR

Return Value
`number of elements in the` list`: Success
* `-1`: any error

Description

Returns the number of elements in the list, or -1 on any error.

---

### sdr_list_destroy

Function Prototype

```c
void sdr_list_destroy(Sdr sdr, Object list, SdrListDeleteFn fn, void *arg)
```

Parameters

* `sdr`: handle to the ION SDR obtained through `ionAttach` or `bp_attach`
* `list`: a list in SDR to be destroyed
* `fn`: a sdrlist [delete function](#callback-sdrlistdeletefn)
* `arg`: arguments passed to the delete function

Return Value

* none

Description

Destroys a list, freeing all elements of the list. If fn is non-NULL, that function is called once for each freed element; when called, fn is passed the Address that is the element's data, and the argument pointer is passed to `sdr_list_destroy`. See the manual page for `sdrlist` for details on the form of the delete function sdrlist.

Do not use sdr_free to destroy an SDR list, as this would leave the elements of the list allocated yet unreferenced.

---

### sdr_list_user_data_set

Function Prototype

```c
void sdr_list_user_data_set(Sdr sdr, Object list, Address userData)
```

Parameters

* `sdr`: handle to the ION SDR obtained through `ionAttach` or `bp_attach`
* `list`: a list in SDR to be destroyed
* `userData`: a single word which is a SDR address

Return Value

* none

Description

Sets the "user data" word of list to userData. Note that userData is nominally an Address but can be any value that occupies a single word. It is typically used to point to an SDR object that somehow characterizes the list as a whole, such as a name.

---

### sdr_list_user_data

Function Prototype

```c
Address sdr_list_user_data(Sdr sdr, Object list)
```

Parameters

* `sdr`: handle to the ION SDR obtained through `ionAttach` or `bp_attach`
* `list`: a list in SDR to be destroyed

Return Value

* `value of the "user data" word of list`: Success
* `0`: any error

Description

Returns the value of the "user data" word of list, or zero on any error.

---

### sdr_list_insert

Function Prototype

```c
Object sdr_list_insert(Sdr sdr, Object list, Address data, SdrListCompareFn fn, void *dataBuffer)
```

Parameters

* `sdr`: handle to the ION SDR obtained through `ionAttach` or `bp_attach`
* `list`: a list in SDR to be destroyed
* `data`: address in SDR
* `fn`: a sdrlist [compare function](#callback-sdrlistcomparefn)
* `dataBuffer`: data pass to the compare function

Return Value

* `value of the "user data" word of list`: Success
* `0`: any error

Description

Creates a new list element whose data value is data and inserts that element into the list. If fn is NULL, the new list element is simply appended to the list; otherwise, the new list element is inserted after the last element in the list whose data value is "less than or equal to" the data value of the new element (in dataBuffer) according to the collating sequence established by fn. Returns the address of the newly created element or zero on any error.

---

### sdr_list_insert_before

### sdr_list_insert_after

Function Prototype

```c
Object sdr_list_insert_before(Sdr sdr, Object elt, Address data)
Object sdr_list_insert_after(Sdr sdr, Object elt, Address data)
```

Parameters

* `sdr`: handle to the ION SDR obtained through `ionAttach` or `bp_attach`
* `elt`: an element of a list in the SDR
* `data`: an address in SDR

Return Value

* `address of the newly created list element`: success
* `0`: any error

Description

Creates a new element and inserts it before/after the specified list element. This function should not be used to insert a new element into an ordered list; use `sdr_list_insert` instead. Returns the address of the newly created list element or zero on any error.

---

### sdr_list_delete

Function Prototype

```c
void sdr_list_delete(Sdr sdr, Object elt, SdrListDeleteFn fn, void *arg)
```

Parameters

* `sdr`: handle to the ION SDR obtained through `ionAttach` or `bp_attach`
* `elt`: an element of a list in the SDR
* `fn`: a sdr list [delete function](#callback-sdrlistdeletefn)
* `*arg`: argument passed to the delete function

Return Value

* none

Description

Delete elt from the list it is in. If fn is non-NULL, that function will be called upon deletion of elt; when called, that function is passed the Address that is the list element's data value and the arg pointer passed to `sdr_list_delete`.

---

### sdr_list_first

### sdr_list_last

Function Prototype

```c
Object sdr_list_first(Sdr sdr, Object list)
Object sdr_list_last(Sdr sdr, Object list)
```

Parameters

* `sdr`: handle to the ION SDR obtained through `ionAttach` or `bp_attach`
* `list`: address of a list in the SDR

Return Value

* `address to the first/last element`: success
* `0`: any error

Description

Returns the address of the first/last element of the list, or zero on any error.

---

### sdr_list_next

### sdr_list_prev

Function Prototype

```c
Object sdr_list_next(Sdr sdr, Object elt)
Object sdr_list_prev(Sdr sdr, Object elt)
```

Parameters

* `sdr`: handle to the ION SDR obtained through `ionAttach` or `bp_attach`
* `elt`: address of an element in a sdrlist

Return Value

* `address to the element following/preceding the element provided`: success
* `0`: any error

Description

Returns the address of the element following/preceding elt in that element's list or zero on any error.

---

### sdr_list_search

Function Prototype

```c
Object sdr_list_search(Sdr sdr, Object elt, int reverse, SdrListCompareFn fn, void *dataBuffer);
```

Parameters

* `sdr`: handle to the ION SDR obtained through `ionAttach` or `bp_attach`
* `elt`: address of an element in a sdrlist
* `reverse`: order of search
* `fn`: a sdrlist [compare function](#callback-sdrlistcomparefn)
* `*dataBuffer`: address of data to be used for search

Return Value

* `address to the matching element`: success
* `0`: any error

Description

Search a list for an element whose data matches the data in dataBuffer, starting at the indicated initial list element.

If the compare function is non-NULL, the list is assumed to be sorted in the order implied by that function, and the function is automatically called once for each element of the list until it returns a value that is greater than or equal to zero (where zero indicates an exact match and a value greater than zero indicates that the list contains no matching element); each time compare is called it is passed the Address that is the element's data value and the dataBuffer value passed to sm_list_search().
If the reverse is non-zero, then the list is searched in reverse order (starting at the indicated initial list element), and the search ends when the compare function returns a value that is less than or equal to zero. If the compare function is NULL, then the entire list is searched (in either forward or reverse order, as directed) until an element is located whose data value is equal to ((Address) dataBuffer). Returns the address of the matching element if one is found, 0 otherwise.

---

### sdr_list_list

Function Prototype

```c
Object sdr_list_list(Sdr sdr, Object elt)
```

Parameters

* `sdr`: handle to the ION SDR obtained through `ionAttach` or `bp_attach`
* `elt`: address of an element in a sdrlist

Return Value

* `address to the list to which elt belongs`: success
* `0`: any error

Description

Returns the address of the list to which elt belongs, or 0 on any error.

---

### sdr_list_data

Function Prototype

```c
Address sdr_list_data(Sdr sdr, Object elt)
```

Parameters

* `sdr`: handle to the ION SDR obtained through `ionAttach` or `bp_attach`
* `elt`: address of an element in a sdrlist

Return Value

* `address that is the data value of elt`: success
* `0`: any error

Description

Returns the Address that is the data value of elt, or 0 on any error.

---

### sdr_list_data_set

Function Prototype

```c
Address sdr_list_data_set(Sdr sdr, Object elt, Address data)
```

Parameters

* `sdr`: handle to the ION SDR obtained through `ionAttach` or `bp_attach`
* `elt`: address of an element in a sdrlist
* `data`: address of data in SDR

Return Value

* `original data value of elt`: success
* `0`: any error

Description

Sets the data value for elt to data, replacing the original value. Returns the original data value for elt, or 0 on any error. The original data value for elt may or may not have been the address of an object in heap data space; even if it was, that object was NOT deleted.

__Warning__: changing the data value of an element of an ordered list may ruin the ordering of the list.

---

## Other less used ICI APIs

There are many other less frequently used APIs. Please see the manual pages for the following:

 `ion`, `sdr`, `sdrlist`, `platform`, `lyst`, `psm`, `memmgr`, `sdrstring`, `sdrtable`, and `smlist`.
