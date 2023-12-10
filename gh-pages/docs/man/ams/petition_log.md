# NAME

petition.log - Remote AMS petition log

# DESCRIPTION

The Remote AMS daemon **ramsgate** records all "petitions" (requests for data
on behalf of AMS modules in other continua) in a file named **petition.log**.
At startup, the **ramsgate** daemon automatically reads and processes all
petitions in the **petition.log** file just as if they were received in real
time, to re-establish the petition state that was in effect at the time the
**ramsgate** daemon shut down.  Note that this means that you can cause
petitions to be, in effect, "pre-received" by simply editing this file
prior to startup.  This can be an especially effective way to configure a
RAMS network in which long signal propagation times would otherwise retard
real-time petitioning and thus delay the onset of fully functional message
exchange.

Entries in **petition.log** are simple ASCII text lines, with parameters
separated by spaces. Each line of **petition.log** has the following parameters:

- protocolId

    This is a number that identifies the RAMS network protocol characterizing
    the network on which the petition was received: 1 == DTN Bundle Protocol, 2 = UDP.

- gatewayID

    This is a string that identifies the remote RAMS gateway node that issued
    this petition.

- controlCode

    This is a number that indicates whether the petition described by this line
    is one that is being asserted (2) or canceled (3).

- subject

    A number that identifies the subject of the traffic to which the petition
    pertains.

- continuumNumber

    Identifies the continuum for the domain of the petition.

- unitNumber

    Identifies the unit for the domain of the petition.

- roleNumber

    Identifies the role for the domain of the petition.

# SEE ALSO

ramsgate(1), ams(3)
