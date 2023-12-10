# NAME

sdrhash - Simple Data Recorder hash table management functions

# SYNOPSIS

    #include "sdr.h"

    Object  sdr_hash_create        (Sdr sdr, int keyLength,
                                        int estNbrOfEntries,
                                        int meanSearchLength);
    int     sdr_hash_insert        (Sdr sdr, Object hash, char *key,
                                        Address value, Object *entry);
    int     sdr_hash_delete_entry  (Sdr sdr, Object entry);
    int     sdr_hash_entry_value   (Sdr sdr, Object hash, Object entry);
    int     sdr_hash_retrieve      (Sdr sdr, Object hash, char *key,
                                        Address *value, Object *entry);
    int     sdr_hash_count         (Sdr sdr, Object hash);
    int     sdr_hash_revise        (Sdr sdr, Object hash, char *key,
                                        Address value);
    int     sdr_hash_remove        (Sdr sdr, Object hash, char *key,
                                        Address *value);
    int     sdr_hash_destroy       (Sdr sdr, Object hash);

# DESCRIPTION

The SDR hash functions manage hash table objects in an SDR.  

Hash tables associate values with keys.  A value is always in the form of
an SDR Address, nominally the address of some stored object identified by
the associated key, but the actual significance of a value may be anything
that fits into a _long_.  A key is always an array of from 1 to
255 bytes, which may have any semantics at all.

Keys must be unique; no two distinct entries in an SDR hash table may have
the same key.  Any attempt to insert a duplicate entry in an SDR hash
table will be rejected.

All keys must be of the same length, and that length must be declared at
the time the hash table is created.  Invoking a hash table function with a
key that is shorter than the declared length will have unpredictable results.

An SDR hash table is an array of linked lists.  The location of a given
value in the hash table is automatically determined by computing a "hash"
of the key, dividing the hash by the number of linked lists in the array,
using the remainder as an index to the corresponding linked list, and
then sequentially searching through the list entries until the entry with
the matching key is found.

The number of linked lists in the array is automatically computed at the
time the hash table is created, based on the estimated maximum number of
entries you expect to store in the table and the mean linked list length
(i.e., mean search time) you prefer.  Increasing the maximum number of
entries in the table and decreasing the mean linked list length both tend
to increase the amount of SDR heap space occupied by the hash table.

- Object sdr\_hash\_create(Sdr sdr, int keyLength, int estNbrOfEntries, int meanSearchLength)

    Creates an SDR hash table.  Returns the SDR address of the new hash table
    on success, zero on any error.

- int sdr\_hash\_insert(Sdr sdr, Object hash, char \*key, Address value, Object \*entry)

    Inserts an entry into the hash table identified by _hash_.  On success,
    places the address of the new hash table entry in _entry_ and returns zero.
    Returns -1 on any error.

- int sdr\_hash\_delete\_entry(Sdr sdr, Object entry)

    Deletes the hash table entry identified by _entry_.  Returns zero on
    success, -1 on any error.

- Address sdr\_hash\_entry\_value(Sdr sdr, Object hash, Object entry)

    Returns the value of the hash table entry identified by _entry_.

- int sdr\_hash\_retrieve(Sdr sdr, Object hash, char \*key, Address \*value, Object \*entry)

    Searches for the value associated with _key_ in this hash table, storing it in
    _value_ if found.  If the entry matching _key_ was found, places the
    address of the hash table entry in _entry_ and returns 1.  Returns zero if
    no such entry exists, -1 on any other failure.

- int sdr\_hash\_count(Sdr sdr, Object hash)

    Returns the number of entries in the hash table identified by _hash_.

- int sdr\_hash\_revise(Sdr sdr, Object hash, char \*key, Address value)

    Searches for the hash table entry matching _key_ in this hash table,
    replacing the associated value with _value_ if found.  Returns 1 if the
    entry matching _key_ was found, zero if no such entry exists, -1 on
    any other failure.

- int sdr\_hash\_remove(Sdr sdr, Object hash, char \*key, Address \*value)

    Searches for the hash table entry matching _key_ in this hash table; if the
    entry is found, stores its value in _value_, deletes the entry, and returns
    1\.  Returns zero if no such entry exists, -1 on any other failure.

- void sdr\_hash\_destroy(Sdr sdr, Object hash);

    Destroys _hash_, destroying all entries in all linked lists of the
    array and destroying the hash table array structure itself.  DO NOT
    use sdr\_free() to destroy a hash table, as this would leave the hash
    table's content allocated yet unreferenced.

# SEE ALSO

sdr(3), sdrlist(3), sdrtable(3)
