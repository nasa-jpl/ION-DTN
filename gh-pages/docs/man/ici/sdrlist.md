# NAME

sdrlist - Simple Data Recorder list management functions

# SYNOPSIS

    #include "sdr.h"

    typedef int (*SdrListCompareFn)(Sdr sdr, Address eltData, void *argData);
    typedef void (*SdrListDeleteFn)(Sdr sdr, Object elt, void *argument);

    [see description for available functions]

# DESCRIPTION

The SDR list management functions manage doubly-linked lists in managed
SDR heap space.  The functions manage two kinds of objects: lists and
list elements.  A list knows how many elements it contains and what its
start and end elements are.  An element knows what list it belongs to
and the elements before and after it in the list.  An element also
knows its content, which is normally the SDR Address of some object
in the SDR heap.  A list may be sorted, which speeds the process
of searching for a particular element.

- Object sdr\_list\_create(Sdr sdr)

    Creates a new list object in the SDR; the new list object initially 
    contains no list elements.  Returns the address of the new list, or 
    zero on any error.

- void sdr\_list\_destroy(Sdr sdr, Object list, SdrListDeleteFn fn, void \*arg)

    Destroys a list, freeing all elements of list.  If _fn_ is non-NULL,
    that function is called once for each freed element;
    when called, _fn_ is passed the Address that is the element's data and
    the _argument_ pointer passed to sdr\_list\_destroy().

    Do not use _sdr\_free_ to destroy an SDR list, as this would
    leave the elements of the list allocated yet unreferenced.

- int sdr\_list\_length(Sdr sdr, Object list)

    Returns the number of elements in the list, or -1 on any error.

- void sdr\_list\_user\_data\_set(Sdr sdr, Object list, Address userData)

    Sets the "user data" word of _list_ to _userData_.  Note that
    _userData_ is nominally an Address but can in fact be any value
    that occupies a single word.  It is typically used to point to an SDR
    object that somehow characterizes the list as a whole, such as a name.

- Address  sdr\_list\_user\_data(Sdr sdr, Object list)

    Returns the value of the "user data" word of _list_, or zero on any error.

- Object sdr\_list\_insert(Sdr sdr, Object list, Address data, SdrListCompareFn fn, void \*dataBuffer)

    Creates a new list element whose data value is _data_ and
    inserts that element into the list.  If _fn_ is NULL,
    the new list element is simply appended to the
    list; otherwise, the new list element is inserted
    after the last element in the list whose data value is
    "less than or equal to" the data value of the new element (in dataBuffer)
    according to the collating sequence established by _fn_.  Returns the address
    of the newly created element, or zero on any error.

- Object sdr\_list\_insert\_first(Sdr sdr, Object list, Address data)
- Object sdr\_list\_insert\_last(Sdr sdr, Object list, Address data)

    Creates a new element and inserts it at the front/end
    of the list.  This function should not be used to insert a new 
    element into any ordered list; use sdr\_list\_insert() instead.  
    Returns the address of the newly created list element on success,
    or zero on any error.

- Object sdr\_list\_insert\_before(Sdr sdr, Object elt, Address data)
- Object sdr\_list\_insert\_after(Sdr sdr, Object elt, Address data)

    Creates a new element and inserts it before/after the
    specified list element.  This function should not be
    used to insert a new element into any ordered list; use
    sdr\_list\_insert() instead.  Returns the address of the newly 
    created list element, or zero on any error.

- void sdr\_list\_delete(Sdr sdr, Object elt, SdrListDeleteFn fn, void \*arg)

    Delete _elt_ from the list it is in.
    If _fn_ is non-NULL, that function will be called upon deletion of
    _elt_; when called, that function is passed the Address that is the list
    element's data value and the _arg_ pointer passed to sdr\_list\_delete().

- Object sdr\_list\_first(Sdr sdr, Object list)
- Object sdr\_list\_last(Sdr sdr, Object list)

    Returns the address of the first/last element of _list_, or zero on
    any error.

- Object sdr\_list\_next(Sdr sdr, Object elt)
- Object sdr\_list\_prev(Sdr sdr, Object elt)

    Returns the address of the element following/preceding _elt_
    in that element's list, or zero on any error.

- Object sdr\_list\_search(Sdr sdr, Object elt, int reverse, SdrListCompareFn fn, void \*dataBuffer);

    Search a list for an element whose data matches the data in _dataBuffer_,
    starting at the indicated initial list element.  If the _compare_
    function is non-NULL, the list is assumed to be sorted
    in the order implied by that function and the function is automatically
    called once for each element of the list until it returns a value that is
    greater than or equal to zero (where zero indicates an exact match and a
    value greater than zero indicates that the list contains no matching
    element); each time _compare_ is called it is passed the Address that is
    the element's data value and the _dataBuffer_ value passed to sm\_list\_search().
    If _reverse_ is non-zero, then the list is searched in reverse order
    (starting at the indicated initial list element) and the search ends
    when _compare_ returns a value that is less than or equal to zero.  If
    _compare_ is NULL, then the entire list is searched (in either
    forward or reverse order, as directed) until an element is
    located whose data value is equal to ((Address) _dataBuffer_).  Returns
    the address of the matching element if one is found, 0 otherwise.

- Object sdr\_list\_list(Sdr sdr, Object elt)

    Returns the address of the list to which _elt_ belongs,
    or 0 on any error.

- Address sdr\_list\_data(Sdr sdr, Object elt)

    Returns the Address that is the data value of _elt_, or 0 on any error.

- Address sdr\_list\_data\_set(Sdr sdr, Object elt, Address data)

    Sets the data value for _elt_ to _data_, replacing the
    original value.  Returns the original data value for _elt_, or 0 on
    any error.  The original data value for _elt_ may or may not have
    been the address of an object in heap data space; even if it was, that
    object was NOT deleted.

    Warning: changing the data value of an element of an ordered list may ruin
    the ordering of the list.

# USAGE

When inserting elements or searching a list, the user may
optionally provide a compare function of the form:

    int user_comp_name(Sdr sdr, Address eltData, void *dataBuffer);

When provided, this function is automatically called by the sdrlist function
being invoked; when the function is called it is passed the content of a
list element (_eltData_, nominally the Address of an item in the SDR's
heap space) and an argument, _dataBuffer_, which is nominally the address
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

    void user_delete_name(Sdr sdr, Address eltData, void *argData)

When provided, this function is automatically called by the sdrlist function
being invoked; when the function is called it is passed the content of a
list element (_eltData_, nominally the Address of an item in the SDR's heap
space) and an argument, _argData_, which if non-NULL is normally the address
in local memory of a data item providing context for the list element deletion.
The user-supplied function performs any application-specific cleanup
associated with deleting the element, such as freeing the element's content
data item and/or other SDR heap space associated with the element.

# SEE ALSO

lyst(3), sdr(3), sdrstring(3), sdrtable(3), smlist(3)
