# NAME

smlist - shared memory list management library

# SYNOPSIS

    #include "smlist.h"

    typedef int (*SmListCompareFn)
        (PsmPartition partition, PsmAddress eltData, void *argData);
    typedef void (*SmListDeleteFn)
        (PsmPartition partition, PsmAddress elt, void *argument);

    [see description for available functions]

# DESCRIPTION

The smlist library provides functions to create, manipulate
and destroy doubly-linked lists in shared memory.  As with lyst(3), 
smlist uses two types of objects, _list_ objects and
_element_ objects.  However, as these objects are stored in
shared memory which is managed by psm(3), pointers to these
objects are carried as PsmAddress values.  A list knows how
many elements it contains and what its first and last elements are.  
An element knows what list it belongs to and
the elements before and after it in its list.  An element
also knows its content, which is normally the PsmAddress of some
object in shared memory.

- PsmAddress sm\_list\_create(PsmPartition partition)

    Create a new list object without any elements in it, within the memory
    segment identified by _partition_.  Returns the PsmAddress of the list,
    or 0 on any error.

- void sm\_list\_unwedge(PsmPartition partition, PsmAddress list, int interval)

    Unwedge, as necessary, the mutex semaphore protecting shared access to the
    indicated list.  For details, see the explanation of the sm\_SemUnwedge()
    function in platform(3).

- int sm\_list\_clear(PsmPartition partition, PsmAddress list, SmListDeleteFn delete, void \*argument);

    Empty a list.  Frees each element of the list.  If the _delete_ function 
    is non-NULL, that function is called once for each freed element; when
    called, that function is passed the PsmAddress of the list element
    and the _argument_ pointer passed to sm\_list\_clear().  Returns 0 on success,
    \-1 on any error.

- int sm\_list\_destroy(PsmPartition partition, PsmAddress list, SmListDeleteFn delete, void \*argument);

    Destroy a list.  Same as sm\_list\_clear(), but additionally frees the list
    structure itself.  Returns 0 on success, -1 on any error.

- int sm\_list\_user\_data\_set(PsmPartition partition, PsmAddress list, PsmAddress userData);

    Set the value of a user data variable associated with the list as a whole.
    This value may be used for any purpose; it is typically used to store the
    PsmAddress of a shared memory block containing data (e.g., state data) which
    the user wishes to associate with the list.  Returns 0 on success, -1 on any
    error.

- PsmAddress sm\_list\_user\_data(PsmPartition partition, PsmAddress list);

    Return the value of the user data variable associated with the list as a
    whole, or 0 on any error.

- int sm\_list\_length(PsmPartition partition, PsmAddress list);

    Return the number of elements in the list.

- PsmAddress sm\_list\_insert(PsmPartition partition, PsmAddress list, PsmAddress data, SmListCompareFn compare, void \*dataBuffer);

    Create a new list element whose data value is _data_ and insert it
    into the given list.  If the _compare_ function is NULL, the new list element
    is simply appended to the list; otherwise, the new list element is inserted
    after the last element in the list whose data value is "less than or equal to"
    the data value of the new element (in _dataBuffer_) according to the
    collating sequence established by _compare_.  Returns the PsmAddress of
    the new element, or 0 on any error.

- PsmAddress sm\_list\_insert\_first(PsmPartition partition, PsmAddress list, PsmAddress data);
- PsmAddress sm\_list\_insert\_last(PsmPartition partition, PsmAddress list, PsmAddress data);

    Create a new list element and insert it at the start/end of a list.  Returns
    the PsmAddress of the new element on success, or 0 on any
    error.  Disregards any established sort order in the list.

- PsmAddress sm\_list\_insert\_before(PsmPartition partition, PsmAddress elt, PsmAddress data);
- PsmAddress sm\_list\_insert\_after(PsmPartition partition, PsmAddress elt, PsmAddress data);

    Create a new list element and insert it before/after a given element.
    Returns the PsmAddress of the new element on success, or 0
    on any error.  Disregards any established sort order in the list.

- int sm\_list\_delete(PsmPartition partition, PsmAddress elt, SmListDeleteFn delete, void \*argument);

    Delete an element from a list.  If the _delete_ function is non-NULL, that
    function is called upon deletion of _elt_; when called, that function is
    passed the PsmAddress of the list element and the _argument_
    pointer passed to sm\_list\_delete().  Returns 0 on success, -1 on any error.

- PsmAddress sm\_list\_first(PsmPartition partition, PsmAddress list);
- PsmAddress sm\_list\_last(PsmPartition partition, PsmAddress list);

    Return the PsmAddress of the first/last element in _list_, or 0 on any error.

- PsmAddress sm\_list\_next(PsmPartition partition, PsmAddress elt);
- PsmAddress sm\_list\_prev(PsmPartition partition, PsmAddress elt);

    Return the PsmAddress of the element following/preceding _elt_ in
    that element's list, or 0 on any error.

- PsmAddress sm\_list\_search(PsmPartition partition, PsmAddress elt, SmListCompareFn compare, void \*dataBuffer);

    Search a list for an element whose data matches the data in _dataBuffer_.  If
    the _compare_ function is non-NULL, the list is assumed to be sorted
    in the order implied by that function and the function is automatically
    called once for each element of the list until it returns a value that is
    greater than or equal to zero (where zero indicates an exact match and a
    value greater than zero indicates that the list contains no matching
    element); each time _compare_ is called it is passed the PsmAddress that is
    the element's data value and the _dataBuffer_ value passed to sm\_list\_search().
    If _compare_ is NULL, then the entire list is searched until an element is
    located whose data value is equal to ((PsmAddress) _dataBuffer_).
    Returns the PsmAddress of the matching element if one is found, 0 otherwise.

- PsmAddress sm\_list\_list(PsmPartition partition, PsmAddress elt);

    Return the PsmAddress of the list to which _elt_ belongs, or 0
    on any error.

- PsmAddress sm\_list\_data(PsmPartition partition, PsmAddress elt);

    Return the PsmAddress that is the data value for _elt_, or 0
    on any error.

- PsmAddress sm\_list\_data\_set(PsmPartition partition, PsmAddress elt, PsmAddress data);

    Set the data value for _elt_ to _data_, replacing the
    original value.  Returns the original data value for _elt_, or 0 on any
    error.  The original data value for _elt_ may or may not have
    been the address of an object in memory; even if it was, that object was
    NOT deleted.  

    Warning: changing the data value of an element of an ordered list may ruin
    the ordering of the list.

# USAGE

A user normally creates an element and adds it to a list by doing the following:

- `1`

    obtaining a shared memory block to contain the element's data;

- `2`

    converting the shared memory block's PsmAddress to a character pointer;

- `3`

    using that pointer to write the data into the shared memory block;

- `4`

    calling one of the _sm\_list\_insert_ functions to create the element 
    structure (which will include the shared memory block's PsmAddress) 
    and insert it into the list.

When inserting elements or searching a list, the user may
optionally provide a compare function of the form:

    int user_comp_name(PsmPartition partition, PsmAddress eltData, 
                       void *dataBuffer);

When provided, this function is automatically called by the smlist function
being invoked; when the function is called it is passed the content of a
list element (_eltData_, nominally the PsmAddress of an item in shared
memory) and an argument, _dataBuffer_, which is nominally the address
in local memory of some other item in the same format.
The user-supplied function normally compares some key values of the two
data items and returns 0 if they are equal, an integer less
than 0 if _eltData_'s key value is less than that of _dataBuffer_, and an
integer greater than 0 if _eltData_'s key value is greater than that of
_dataBuffer_.  These return values will produce a list in ascending order.  
If the user desires the list to be in descending
order, the function must reverse the signs of these return values.

When deleting an element or destroying a list, the user may
optionally provide a delete function of the form:

    void user_delete_name(PsmPartition partition, PsmAddress elt, void *argData)

When provided, this function is automatically called by the smlist function
being invoked; when the function is called it is passed the address of a
list element (_elt_ and an argument, _argData_, which if non-NULL is
normally the address
in local memory of a data item providing context for the list element deletion.
The user-supplied function performs any application-specific cleanup
associated with deleting the element, such as freeing the element's content
data item and/or other memory associated with the element.

# EXAMPLE

For an example of the use of smlist, see the file smlistsh.c
in the utils directory of ICI.

# SEE ALSO

lyst(3), platform(3), psm(3)
