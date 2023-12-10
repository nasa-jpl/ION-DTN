# NAME

sdrtable - Simple Data Recorder table management functions

# SYNOPSIS

    #include "sdr.h"

    Object  sdr_table_create        (Sdr sdr, int rowSize, int rowCount);
    int     sdr_table_user_data_set (Sdr sdr, Object table, Address userData);
    Address sdr_table_user_data     (Sdr sdr, Object table);
    int     sdr_table_dimensions    (Sdr sdr, Object table, int *rowSize, 
                                        int *rowCount);
    int     sdr_table_stage         (Sdr sdr, Object table);
    Address sdr_table_row           (Sdr sdr, Object table, 
                                        unsigned int rowNbr);
    int     sdr_table_destroy       (Sdr sdr, Object table);

# DESCRIPTION

The SDR table functions manage table objects in the SDR.  An SDR
table comprises N rows of M bytes each, plus optionally one word
of user data (which is nominally the address of some other object
in the SDR's heap space).  When a table is created, the number of
rows in the table and the length of each row are specified; they remain
fixed for the life of the table.  The table functions merely
maintain information about the table structure and its location
in the SDR and calculate row addresses; other SDR functions such as
sdr\_read() and sdr\_write() are used to read and write the contents of
the table's rows.  In particular, the format of the rows of a
table is left entirely up to the user.

- Object sdr\_table\_create(Sdr sdr, int rowSize, int rowCount)

    Creates a "self-delimited table", comprising _rowCount_ rows of
    _rowSize_ bytes each, in the heap space of the indicated SDR.  Note
    that the content of the table, a two-dimensional array, is a single
    SDR heap space object of size (_rowCount_ x _rowSize_).  Returns
    the address of the new table on success, zero on any error.

- void sdr\_table\_user\_data\_set(Sdr sdr, Object table, Address userData)

    Sets the "user data" word of _table_ to _userData_.  Note that
    _userData_ is nominally an Address but can in fact be any value
    that occupies a single word.  It is typically used to point to an
    SDR object that somehow characterizes the table as a whole, such as an
    SDR string containing a name.

- Address sdr\_table\_user\_data(Sdr sdr, Object table)

    Returns the value of the "user data" word of _table_, or zero on any
    error.

- void sdr\_table\_dimensions(Sdr sdr, Object table, int \*rowSize, int \*rowCount)

    Reports on the row size and row count of the indicated table, as specified
    when the table was created.

- void sdr\_table\_stage(Sdr sdr, Object table)

    Stages _table_ so that the array it encapsulates may be updated; see the
    discussion of sdr\_stage() in sdr(3).  The effect of this function is
    the same as: 

        sdr_stage(sdr, NULL, (Object) sdr_table_row(sdr, table, 0), 0)

- Address sdr\_table\_row(Sdr sdr, Object table, unsigned int rowNbr)

    Returns the address of the _rowNbr_th row of _table_, for use in
    reading or writing the content of this row; returns -1 on any error.

- void sdr\_table\_destroy(Sdr sdr, Object table)

    Destroys _table_, releasing all bytes of all rows and destroying the
    table structure itself.  DO NOT use sdr\_free() to destroy a table, as
    this would leave the table's content allocated yet unreferenced.

# SEE ALSO

sdr(3), sdrlist(3), sdrstring(3)
