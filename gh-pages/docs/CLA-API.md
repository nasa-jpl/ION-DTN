# Convergence Layer Adaptor - APIs

ION currently provides several CLAs, include LTP, TCP, UDP, STCP. However, it is possible to develop a customized CLA using ION's API. This document will describe the basic set of API used to develop a customized CLA.

## CLA APIs

### Header

```c
#include "bpP.h"
```

### bpDequeue

Function Prototype

```c
extern int bpDequeue(VOutduct *vduct,
					Object *outboundZco,
					BpAncillaryData *ancillaryData,
					int stewardship);
```

This function is invoked by a
convergence-layer output adapter
(outduct) daemon to get a bundle that
it is to transmit to some remote
convergence-layer input adapter (induct)
daemon.

The function first pops the next (only)
outbound bundle from the queue of
outbound bundles for the indicated duct.
If no such bundle is currently waiting
for transmission, it blocks until one
is [or until the duct is closed, at
which time the function returns zero
without providing the address of an
outbound bundle ZCO].

On obtaining a bundle, bpDequeue
does DEQUEUE processing on the bundle's
extension blocks; if this processing
determines that the bundle is corrupt,
the function returns zero while
providing 1 (a nonsense address) in
*bundleZco as the address of the
outbound bundle ZCO.  The CLO should
handle this result by simply calling
bpDequeue again.

bpDequeue then catenates (serializes)
the BP header information (primary
block and all extension blocks) in
the bundle and prepends that serialized
header to the source data of the
bundle's payload ZCO.  Then it
returns the address of that ZCO in
*bundleZco for transmission at the
convergence layer (possibly entailing
segmentation that would be invisible
to BP).

Requested quality of service for the
bundle is provided in *ancillaryData
so that the requested QOS can be
mapped to the QOS features of the
convergence-layer protocol.  For
example, this is where a request for
custody transfer is communicated
to BIBE when the outduct daemon is
one that does BIBE transmission.
The stewardship argument controls
the disposition of the bundle
following transmission.  Any value
other than zero indicates that the
outduct daemon is one that performs
"stewardship" procedures.  An outduct
daemon that performs stewardship
procedures will disposition the bundle
as soon as the results of transmission
at the convergence layer are known,
by calling one of two functions:
either bpHandleXmitSuccess or else
bpHandleXmitFailure.  A value of
zero indicates that the outduct
daemon does not perform stewardship
procedures and will not disposition
the bundle following transmission;
instead, the bpDequeue function itself
will assume that transmission at the
convergence layer will be successful
and will disposition the bundle on
that basis.

Return Values

* 0: on success
* -1: on failure

### bpHandleXmitSuccess

Function Prototype

```c
extern int		bpHandleXmitSuccess(Object zco);
```

This function is invoked by a
convergence-layer output adapter (an
outduct) on detection of convergence-
layer protocol transmission success.
It causes the serialized (catenated)
outbound bundle in zco to be destroyed,
unless some constraint (such as local
delivery of a copy of the bundle)
requires that bundle destruction
be deferred.

Return Values

* 1: if bundle success was handled
* 0: if bundle had already been destroyed
* -1: on system failure.

### bpHandleXmitFailure

Function Prototype

```c
extern int bpHandleXmitFailure(Object zco);
```

This function is invoked by a
convergence-layer output adapter (an
outduct) on detection of a convergence-
layer protocol transmission error.
It causes the serialized (catenated)
outbound bundle in zco to be queued
up for re-forwarding.
 
Return Values

* 1: if bundle failure was handled
* 0: if bundle had already been destroyed
* -1: on system failure.

### bpGetAcqArea

```c
extern AcqWorkArea	*bpGetAcqArea(VInduct *vduct);
```

Allocates a bundle acquisition work
area for use in acquiring inbound
bundles via the indicated duct. This is typically invoked
just once at the beginning of a CLA process initialization

Return Value

* `NULL`: on any failure

### bpReleaseAcqArea

```c
extern void		bpReleaseAcqArea(AcqWorkArea *workArea);
```

Releases dynamically allocated bundle acquisition work area.
This should be called before shutting down a CLA process.

Return Value

* `NULL`: on any failure

### bpBeginAcquisition

```c
extern int	bpBeginAcq(	AcqWorkArea *workArea,
				int authentic,
				char *senderEid);
```

This function is invoked by a
convergence-layer input adapter
to initiate acquisition of a new
bundle via the indicated workArea.
It initializes deserialization of
an array of bytes constituting a
single transmitted bundle.
The "authentic" Boolean and "senderEid"
string are knowledge asserted by the
convergence-layer input adapter
invoking this function: an assertion
of authenticity of the data being
acquired (e.g., per knowledge that
the data were received via a
physically secure medium) and, if
non-NULL, an EID characterizing the
node that send this inbound bundle.

Return Values

* 0: on success
* -1: on any failure				

### bpContinueAcq

```c
extern int	bpContinueAcq(	AcqWorkArea *workArea,
				char *bytes,
				int length,
				ReqAttendant *attendant,
				unsigned char priority);
```

This function continues acquisition
of a bundle as initiated by an
invocation of bpBeginAcq().  To
do so, it appends the indicated array
of bytes, of the indicated length, to
the byte array that is encapsulated
in workArea.

bpContinueAcq is an alternative to
bpLoadAcq, intended for use by
convergence-layer adapters that
incrementally acquire portions of
concatenated bundles into byte-array
buffers.  The function transparently
creates a zero-copy object for
acquisition of the bundle, if one
does not already exist, and appends
"bytes" to the source data of that
ZCO.

The behavior of bpContinueAcq when
currently available space for zero-
copy objects is insufficient to
contain this increment of bundle
source data depends on the value of
"attendant".  If "attendant" is NULL,
then bpContinueAcq will return 0 but
will flag the acquisition work area
for refusal of the bundle due to
resource exhaustion (congestion).
Otherwise, (i.e., "attendant" points
to a ReqAttendant structure, which 
MUST have already been initialized by
ionStartAttendant()), bpContinueAcq
will block until sufficient space
is available or the attendant is
paused or the function fails,
whichever occurs first.

"priority" is normally zero, but for
the TCPCL convergence-layer receiver
threads it is very high (255) because
any delay in allocating space to an
extent of TCPCL data delays the
processing of TCPCL control messages,
potentially killing TCPCL performance.

Return Values

* 0: on success (even if "attendant" was paused or the
acquisition work area is flagged
for refusal due to congestion)
* -1: on any failure.

### bpCancelAcq

```c
extern void		bpCancelAcq(	AcqWorkArea *workArea);
```

Cancels acquisition of a new
bundle via the indicated workArea,
destroying the bundle acquisition
ZCO of workArea.


### bpEndAcq

```c
extern int		bpEndAcq(	AcqWorkArea *workArea);
```
Concludes acquisition of a new
bundle via the indicated workArea.
This function is invoked after the
convergence-layer input adapter
has invoked either bpLoadAcq() or
bpContinueAcq() [perhaps invoking
the latter multiple times] such
that all bytes of the transmitted
bundle are now included in the
bundle acquisition ZCO of workArea.

Return Value

* 1: on success, the bundle has been 
fully acquired and dispatched (that is,
queued for delivery and/or forwarding).
In this case, the invoking convergence-
layer input adapter should simply
continue with the next cycle of
bundle acquisition, i.e., it should
call bpBeginAcq().

* 0: on any failure pertaining only to this bundle. 
The failure
is transient, applying only to the
bundle that is currently being
acquired.  In this case, the current
bundle acquisition has failed but
BP itself can continue; the invoking
convergence-layer input adapter
should simply continue with the next
cycle of bundle acquisition just as
if the return code had been 1.

* -1: on any other (i.e., system) failure.  

### bpLoadAcq (suitable for certain CLA types)

```c
extern int		bpLoadAcq(	AcqWorkArea *workArea,
					Object zco);
```
This function continues the acquisition
of a bundle as initiated by an
invocation of bpBeginAcq().  To
do so, it inserts the indicated
zero-copy object - containing
possibly multiple whole bundles in concatenated form -
into workArea.

bpLoadAcq is an alternative to
bpContinueAcq, intended for use
by convergence-layer adapters that
natively acquire concatenated
bundles into zero-copy objects such as LTP.

Return Value

* 0: on success
* -1: on any failure	

## Setting up a custom CLA in ION

_This section is under construction_