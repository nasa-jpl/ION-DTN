# NAME

smrbt - shared-memory red-black tree management library

# SYNOPSIS

    #include "smrbt.h"

    typedef int (*SmRbtCompareFn)
        (PsmPartition partition, PsmAddress nodeData, void *dataBuffer);
    typedef void (*SmRbtDeleteFn)
        (PsmPartition partition, PsmAddress nodeData, void *argument);

    [see description for available functions]

# DESCRIPTION

The smrbt library provides functions to create, manipulate
and destroy "red-black" balanced binary trees in shared memory.
smrbt uses two types of objects, _rbt_ objects and
_node_ objects; as these objects are stored in
shared memory which is managed by psm(3), pointers to these
objects are carried as PsmAddress values.  An rbt knows how
many nodes it contains and what its root node is.  
An node knows what rbt it belongs to and which nodes are its
parent and (up to 2) children.
A node also knows its content, which is normally the PsmAddress of some
object in shared memory.

- PsmAddress sm\_rbt\_create(PsmPartition partition)

    Create a new rbt object without any nodes in it, within the memory
    segment identified by _partition_.  Returns the PsmAddress of the rbt,
    or 0 on any error.

- void sm\_rbt\_unwedge(PsmPartition partition, PsmAddress rbt, int interval)

    Unwedge, as necessary, the mutex semaphore protecting shared access to the
    indicated rbt.  For details, see the explanation of the sm\_SemUnwedge()
    function in platform(3).

- int sm\_rbt\_clear(PsmPartition partition, PsmAddress rbt, SmRbtDeleteFn delete, void \*argument);

    Frees every node of the rbt, leaving the rbt empty.  If the _delete_ function 
    is non-NULL, that function is called once for each freed node; when
    called, that function is passed the PsmAddress that is the node's data
    and the _argument_ pointer passed to sm\_rbt\_clear().  Returns 0 on success,
    \-1 on any error.

- void sm\_rbt\_destroy(PsmPartition partition, PsmAddress rbt, SmRbtDeleteFn delete, void \*argument);

    Destroy an rbt.  Frees all nodes of the rbt as in sm\_rbt\_clear(), then
    frees the rbt structure itself.

- int sm\_rbt\_user\_data\_set(PsmPartition partition, PsmAddress rbt, PsmAddress userData);

    Set the value of a user data variable associated with the rbt as a whole.
    This value may be used for any purpose; it is typically used to store the
    PsmAddress of a shared memory block containing data (e.g., state data) which
    the user wishes to associate with the rbt.  Returns 0 on success, -1 on any
    error.

- PsmAddress sm\_rbt\_user\_data(PsmPartition partition, PsmAddress rbt);

    Return the value of the user data variable associated with the rbt as a
    whole, or 0 on any error.

- int sm\_rbt\_length(PsmPartition partition, PsmAddress rbt);

    Return the number of nodes in the rbt.

- PsmAddress sm\_rbt\_insert(PsmPartition partition, PsmAddress rbt, PsmAddress data, SmRbtCompareFn compare, void \*dataBuffer);

    Create a new rbt node whose data value is _data_ and insert it into
    _rbt_.  The nodes of an rbt are ordered by their notional "key" values;
    for this purpose, no two nodes may have the same key value.  The key value
    of a node is assumed to be some function of the content of _dataBuffer_,
    which is assumed to be a representation in memory of the data value
    indicated by _data_, and that function must be implicit in the _compare_
    function, which must not be NULL.  The new rbt node is inserted into
    the rbt in a tree location that preserves order in the tree, according to
    the collating sequence established by _compare_, and also ensures that
    no path (from root to leaf) in the tree is more than twice as long as
    any other path.  This makes searching the tree for a given data value
    quite rapid even if the number of nodes in the tree is very large.  Returns
    the PsmAddress of the new node, or 0 on any error.

- void sm\_rbt\_delete(PsmPartition partition, PsmAddress rbt, SmRbtCompareFn compare, void \*dataBuffer, SmRbtDeleteFn delete, void \*argument);

    Delete a node from _rbt_.  _compare_ must be the same function that was
    used to insert the node: the tree must be dynamically re-balanced upon node
    deletion, and the _compare_ function and the data value of the node that
    is to be deleted (as represented in memory in _dataBuffer_) are required for
    this purpose.  (Since the function descends the tree in search of the
    matching node anyway, in order to preserve balance, the address of the node
    itself is not needed.)

    If the _delete_ function is non-NULL, that function is called upon deletion
    of the indicated node.  When called, that function is passed the PsmAddress
    that is the node's data value and the _argument_ pointer passed to
    sm\_rbt\_delete().

    **NOTE** that this function does something highly devious to avoid extra
    tree-balancing complexity when node is deleted.  For details, see the code,
    but the main point is that deleting a node **WILL MOVE NODES WITHIN THE TREE**.
    After the deletion, the next node may not be the one that would have been
    reported if you passed the to-be-deleted node to sm\_rbt\_next() before
    calling sm\_rbt\_delete().  This is important: do not apply updates (no
    insertions, and especially no deletions) while you are traversing a
    red-black tree sequentially.  If you do, the result will not be what you
    expect.

- PsmAddress sm\_rbt\_first(PsmPartition partition, PsmAddress rbt);
- PsmAddress sm\_rbt\_last(PsmPartition partition, PsmAddress rbt);

    Return the PsmAddress of the first/last node in _rbt_, or 0 on any error.

- PsmAddress sm\_rbt\_next(PsmPartition partition, PsmAddress node);
- PsmAddress sm\_rbt\_prev(PsmPartition partition, PsmAddress node);

    Return the PsmAddress of the node following/preceding _node_ in
    that node's rbt, or 0 on any error.

    **NOTE** that the red-black tree node insertion and deletion functions 
    **WILL MOVE NODES WITHIN THE TREE**.
    This is important: do not apply updates (no insertions, and especially no
    deletions) while you are traversing a red-black tree sequentially, using
    sm\_rbt\_next() or sm\_rbt\_prev().  If you do, the result will not be what you
    expect.

- PsmAddress sm\_rbt\_search(PsmPartition partition, PsmAddress rbt, SmRbtCompareFn compare, void \*dataBuffer, PsmAddress \*successor);

    Search _rbt_ for a node whose data matches the data in _dataBuffer_.
    _compare_ must be the same function that was used to insert all nodes
    into the tree.  The tree is searched until a node is found whose data
    value is "equal" (according to _compare_) to the data value represented
    in memory in _dataBuffer_, or until it is known that there is no such
    node in the tree.  If the matching node is found, the PsmAddress of that
    node is returned and _\*successor_ is set to zero.  Otherwise, zero is
    returned and _\*successor_ is set to the PsmAddress of the first node in
    the tree whose key value is greater than the key value of _dataBuffer_,
    according to _compare_, or to zero if there is no such successor node.

- PsmAddress sm\_rbt\_rbt(PsmPartition partition, PsmAddress node);

    Return the PsmAddress of the rbt to which _node_ belongs, or 0
    on any error.

- PsmAddress sm\_rbt\_data(PsmPartition partition, PsmAddress node);

    Return the PsmAddress that is the data value for _node_, or 0
    on any error.

# USAGE

A user normally creates an node and adds it to a rbt by doing the following:

- `1`

    obtaining a shared memory block to contain the node's data;

- `2`

    converting the shared memory block's PsmAddress to a character pointer;

- `3`

    using that pointer to write the data into the shared memory block;

- `4`

    calling the _sm\_rbt\_insert_ function to create the node structure (which
    will include the shared memory block's PsmAddress) and insert it into the rbt.

When inserting or deleting nodes or searching a rbt, the user must
provide a compare function of the form:

    int user_comp_name(PsmPartition partition, PsmAddress node, 
                       void *dataBuffer);

This function is automatically called by the smrbt function being invoked;
when the function is called it is passed the data content of an rbt node
(_node_, nominally the PsmAddress of an item in shared memory) and an
argument, _dataBuffer_, which is nominally the address in local memory
of some other data item in the same format.  The user-supplied function
normally compares some key values of the two data items and returns 0 if
they are equal, an integer less than 0 if _node_'s key value is less
than that of _dataBuffer_, and an integer greater than 0 if _node_'s
key value is greater than that of _dataBuffer_.  These return values
will produce an rbt in ascending order.  

When deleting an node or destroying a rbt, the user may
optionally provide a delete function of the form:

    void user_delete_name(PsmPartition partition, PsmAddress node, 
                          void *argData)

When provided, this function is automatically called by the smrbt function
being invoked; when the function is called it is passed the content of a
rbt node (_node_, nominally the PsmAddress of an item in shared
memory) and an argument, _argData_, which if non-NULL is normally the address
in local memory of a data item providing context for the rbt node deletion.
The user-supplied function performs any application-specific cleanup
associated with deleting the node, such as freeing the node's content
data item and/or other memory associated with the node.

# EXAMPLE

For an example of the use of smrbt, see the file smrbtsh.c
in the utils directory of ICI.

# SEE ALSO

smrbtsh(1), platform(3), psm(3)
