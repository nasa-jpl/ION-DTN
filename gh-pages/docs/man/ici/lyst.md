# NAME

lyst - library for manipulating generalized doubly linked lists

# SYNOPSIS

    #include "lyst.h"

    typedef int  (*LystCompareFn)(void *s1, void *s2);
    typedef void (*LystCallback)(LystElt elt, void *userdata);

    [see description for available functions]

# DESCRIPTION

The "lyst" library uses two types of objects, _Lyst_ objects
and _LystElt_ objects.  A Lyst knows how many elements it contains, 
its first and last elements, the memory manager used
to create/destroy the Lyst and its elements, and how the elements are
sorted.  A LystElt knows its content (normally a pointer to an item
in memory), what Lyst it belongs to, and the LystElts before and after
it in that Lyst.

- Lyst lyst\_create(void)

    Create and return a new Lyst object without any elements in it.
    All operations performed on this Lyst will use the
    allocation/deallocation functions of the default memory
    manager "std" (see memmgr(3)).  Returns NULL on any failure.

- Lyst lyst\_create\_using(unsigned memmgrId)

    Create and return a new Lyst object without any elements in it.
    All operations performed on this Lyst will use the
    allocation/deallocation functions of the specified
    memory manager (see memmgr(3)).  Returns NULL on any failure.

- void lyst\_clear(Lyst list)

    Clear a Lyst, i.e. free all elements of _list_, calling the Lyst's
    deletion function if defined, but without destroying the Lyst itself.

- void lyst\_destroy(Lyst list)

    Destroy a Lyst.  Will free all elements of _list_, calling the Lyst's
    deletion function if defined.

- void lyst\_compare\_set(Lyst list, LystCompareFn compareFn)
- LystCompareFn lyst\_compare\_get(Lyst list)

    Set/get comparison function for specified Lyst.  Comparison 
    functions are called with two Lyst element data
    pointers, and must return a negative integer if first
    is less than second, 0 if both are equal, and a positive integer
    if first is greater than second (i.e., same return values as strcmp(3)).
    The comparison function is used by the
    lyst\_insert(), lyst\_search(), lyst\_sort(), and lyst\_sorted()
    functions.

- void lyst\_direction\_set(Lyst list, LystSortDirection direction)

    Set sort direction (either LIST\_SORT\_ASCENDING or
    LIST\_SORT\_DESCENDING) for specified Lyst.  If no comparison
    function is set, then this controls whether
    new elements are added to the end or beginning (respectively) 
    of the Lyst when lyst\_insert() is called.

- void lyst\_delete\_set(Lyst list, LystCallback deleteFn, void \*userdata)

    Set user deletion function for specified Lyst.  This
    function is automatically called whenever an element of the Lyst is deleted,
    to perform any user-required processing.  When automatically called,
    the deletion function is passed two arguments: the element being deleted
    and the _userdata_ pointer specified in the lyst\_delete\_set() call.

- void lyst\_insert\_set(Lyst list, LystCallback insertFn, void \*userdata)

    Set user insertion function for specified Lyst.  This
    function is automatically called whenever a Lyst element is
    inserted into the Lyst, to perform any user-required processing.
    When automatically called, the insertion function is passed two arguments:
    the element being inserted and the _userdata_ pointer specified in
    the lyst\_insert\_set() call.

- unsigned long lyst\_length(Lyst list)

    Return the number of elements in the Lyst.

- LystElt lyst\_insert(Lyst list, void \*data)

    Create a new element whose content is the pointer value _data_
    and insert it into the Lyst.  Uses the Lyst's comparison
    function to select insertion point, if defined; otherwise
    adds the new element at the beginning or end of the Lyst,
    depending on the Lyst sort direction setting.  Returns a
    pointer to the newly created element, or NULL on any failure.

- LystElt lyst\_insert\_first(Lyst list, void \*data)
- LystElt lyst\_insert\_last(Lyst list, void \*data)

    Create a new element and insert it at the beginning/end
    of the Lyst.  If these functions are used when inserting elements
    into a Lyst with a defined comparison function, then the Lyst may
    get out of order and future calls to lyst\_insert() can put new elements 
    in unpredictable locations.  Returns a pointer to
    the newly created element, or NULL on any failure.

- LystElt lyst\_insert\_before(LystElt element, void \*data)
- LystElt lyst\_insert\_after(LystElt element, void \*data)

    Create a new element and insert it before/after the
    specified element.  If these functions are used when inserting
    elements into a Lyst with a defined comparison function,
    then the Lyst may get out
    of order and future calls to lyst\_insert() can put new
    elements in unpredictable locations.  Returns a pointer
    to the newly created element, or NULL on any failure.

- void lyst\_delete(LystElt element)

    Delete the specified element from its Lyst and deallocate its memory.  
    Calls the user delete function if defined.

- LystElt lyst\_first(Lyst list)
- LystElt lyst\_last(Lyst list)

    Return a pointer to the first/last element of a Lyst.

- LystElt lyst\_next(LystElt element)
- LystElt lyst\_prev(LystElt element)

    Return a pointer to the element following/preceding the specified element.

- LystElt lyst\_search(LystElt element, void \*searchValue)

    Find the first matching element in a Lyst starting with
    the specified element.  Returns NULL if no matches are
    found.  Uses the Lyst's comparison function if defined,
    otherwise searches from the given element to the end of the Lyst.

- Lyst lyst\_lyst(LystElt element)

    Return the Lyst to which the specified element belongs.

- void\* lyst\_data(LystElt element)
- void\* lyst\_data\_set(LystElt element, void \*data)

    Get/set the pointer value content of the specified Lyst element.  The
    set routine returns the element's previous content, and the
    delete function is _not_ called.  If the lyst\_data\_set()
    function is used on an element of a Lyst with a defined comparison
    function, then the Lyst may get out of order and future calls to
    lyst\_insert() can put new elements in unpredictable locations.

- void lyst\_sort(Lyst list)

    Sort the Lyst based on the current comparison function
    and sort direction.  A stable insertion sort is used
    that is very fast when the elements are already in order.

- int lyst\_sorted(Lyst list)

    Determine whether or not the Lyst is sorted based on
    the current comparison function and sort direction.

- void lyst\_apply(Lyst list, LystCallback applyFn, void \*userdata)

    Apply the function _applyFn_ automatically to each element
    in the Lyst.  When automatically called, _applyFn_ is passed
    two arguments: a pointer to an element, and the _userdata_
    argument specified in the call to lyst\_apply().  _applyFn_
    should not delete or reorder the elements in the Lyst.

# SEE ALSO

memmgr(3), psm(3)
