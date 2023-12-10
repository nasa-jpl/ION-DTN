# NAME

ramsgate - Remote AMS gateway daemon

# SYNOPSIS

**ramsgate** _application\_name_ _authority\_name_ \[_bundles\_TTL_\]

# DESCRIPTION

**ramsgate** is a background "daemon" task that functions as a Remote AMS
gateway.  _application\_name_ and _authority\_name_ must identify an AMS
venture that is known to operate in the local continuum, as noted in the
MIB for the **ramsgate** application module.

**ramsgate** will register as an application module in the root unit of
the indicated venture, so a configuration server for the local continuum
and a registrar for the root unit of the indicated venture (which may
both be instantiated in a single **amsd** daemon task) must be running
in order for **ramsgate** to commence operations.

**ramsgate** with communicate with other RAMS gateway modules in other
continua by means of the RAMS network protocol noted in the RAMS gateway
endpoint ID for the local continuum, as identified (explicitly or implicitly)
in the MIB.

If the RAMS network protocol is "bp" (i.e., the DTN Bundle Protocol), then
an ION Bundle Protocol node must be operating on the local computer and that
node must be registered in the BP endpoint identified by the RAMS gateway
endpoint ID for the local continuum.  Moreover, in this case the value of
_bundles\_TTL_ - if specified - will be taken as the lifetime in seconds that
is to be declared for all "bundles" issued by **ramsgate**; _bundles\_TTL_
defaults to 86400 seconds (one day) if omitted.

# EXIT STATUS

- "0"

    **ramsgate** terminated normally.

- "1"

    **ramsgate** failed, for reasons noted in the ion.log file; the task
    terminated.

# FILES

A MIB initialization file with the applicable default name (see amsrc(5))
must be present.

**ramsgate** records all "petitions" (requests for data on behalf of AMS
modules in other continua) in a file named "petition.log".  At startup,
the **ramsgate** daemon automatically reads and processes all petitions
in the petition.log file just as if they were received in real time.  **Note**
that this means that you can cause petitions to be, in effect, "pre-received"
by simply editing this file prior to startup.  This can be an especially
effective way to configure a RAMS network in which long signal propagation
times would otherwise retard real-time petitioning and thus delay the onset
of fully functional message exchange.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- ramsgate can't run.

    RAMS gateway functionality failed, for reasons noted in the ion.log file.

# BUGS

Note that the AMS design principle of receiving messages immediately and
enqueuing them for eventual ingestion by the application module - rather
than imposing application-layer flow control on AMS message traffic - enables
high performance but makes **ramsgate** vulnerable to message spikes.  Since
production and transmission of bundles is typically slower than AMS message
reception over TCP service, the ION working memory and/or heap space available
for AMS event insertion and/or bundle production can be quickly exhausted if
a high rate of application message production is sustained for a long enough
time.  Mechanisms for defending against this sort of failure are under study,
but for now the best mitigations are simply to (a) build with compiler option
\-DAMS\_INDUSTRIAL=1, (b) allocate as much space as possible to ION working
memory and SDR heap (see ionconfig(5)) and (c) limit the rate of AMS message
issuance.

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

amsrc(5), petition\_log(5)
