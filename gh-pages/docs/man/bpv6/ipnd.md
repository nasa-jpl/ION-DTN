# NAME

ipnd - ION IPND module

# DESCRIPTION

The **ipnd** daemon is the ION implementation of DTN IP Neighbor Discovery.
This module allows the node to send and receive beacon messages using 
unicast, multicast or broadcast IP addresses. Beacons are used for the
discovery of neighbors and may be used to advertise services that are 
present and available on nodes, such as routing algorithms or CLAs.

ION IPND module is configured using a \*.rc configuration file.  The name of
the configuration file must be passed as the sole command-line argument to
**ipnd** when the daemon is started.  Commands are interpreted line by line,
with exactly one command per line.  The formats and effects of the ION
**ipnd** management commands are described below.

# USAGE

ipnd _config\_file\_name_

# COMMANDS

- **1** 

    The **initialize** command.  This must be the first command.

- **#**

    Comment line.  Lines beginning with **#** are not interpreted.

- **e** { 1 | 0 }

    Echo control.  Setting echo to 1 causes all output printed by ipnd to
    be logged into ion.log.  Setting echo to 0 disables this behavior.
    Default is 1.

- **m eid** _eid_

    Local eid. This command sets the advertised BP endpoint ID by which the
    node will identify itself in beacon messages.

- **m announce period** { 1 | 0 }

    Announce period control. Setting to 1 causes all beacons messages sent
    to contain beacon period. Setting to 0 disables this behavior.
    Default is 1.

- **m announce eid** { 1 | 0 }

    Announce eid control. Setting to 1 causes all beacons messages sent to
    contain source eid. Setting to 0 disables this behavior. This should be 
    always set to 1. Default is 1.

- **m interval unicast** _interval_

    Unicast interval. This command sets the beacon messages period on unicast 
    transmissions. Time interval is expressed in seconds. Default is 5.

- **m interval multicast** _interval_

    Multicast interval. This command sets the beacon messages period on multicast 
    transmissions. Time interval is expressed in seconds. Default is 7.

- **m interval broadcast** _interval_

    Broadcastcast interval. This command sets the beacon messages period on
    broadcast transmissions. Time interval is expressed in seconds. Default is 11.

- **m multicast ttl** _ttl_

    Multicast ttl. This command sets the multicast outgoing beacon messages'
    time to live, in seconds. Default is 255.

- **m svcdef** _id_ _name_ _child\_name_:_child\_type_ ...

    Service definition. This command specifies definitions of "services", which
    are dynamically defined beacon message data structures indicating the
    capabilities of the beacon message sender.  _id_ is a service-identifying
    number in the range 128-255. _name_ is the name of the service type that
    is being defined. The definition of the structure of the service is a sequence
    of elements, each of which is a _name_**:**_type_ pair.  Each _child\_type_
    must be the name of a standard or previously defined service type.  Infinite
    recursion is supported.

- **a svcadv** _name_ _child\_name_:_child\_value_ ...

    Service advertising command. This command defines which services will 
    be advertised and with which values. All types of formats for values 
    are supported (e.g. 999, 0345 (octal), 0x999 (hex), -1e-9, 0.32, etc.).
    For a service that contains only a single element, it is not necessary to
    provide that element's name.  E.g. it is enough to write Booleans:true
    instead of Booleans:BooleanValues:B:true, as BooleanValues is the only
    child of Booleans and B is the only child of BooleanValues.

- **a listen** _IP\_address_

    Listen socket specification command. This command asserts, in the form
    _IP\_address_, the IP address of the socket at which the IPND daemon is to
    listen for incoming beacons; a default port number is used. The address can
    be an unicast, a multicast or a broadcast address. If a multicast address is
    provided all the configured unicast addresses will listen for multicast packets 
    in that group. If a broadcast address is provided all the unicast 
    addresses will listen for broadcasted packets.

- **a destination** _destination\_socket\_spec_

    Destination socket specification command. This command asserts the
    specification for a socket to which the IPND daemon is to send beacons. It
    can be an unicast, a multicast or a broadcast address.

- **s**

    The **start** command.  This command starts the IPND daemon for the local
    ION node.

# EXAMPLES

m scvdef 128 FooRouter Seed:SeedVal BaseWeight:WeightVal RootHash:bytes

Defines a new service called FooRouter comprising 3 elements. SeedVal 
and WeightVal are user defined services that must be already defined.

m svcdef 129 SeedVal   Value:fixed16

m svcdef 130 WeightVal Value:fixed16

m svcdef 128 FooRouter Seed:SeedVal BaseWeight:WeightVal RootHash:bytes

m svcdef 150 FixedValuesList F16:fixed16 F32:fixed32 F64:fixed64

m svcdef 131 VariableValuesList U64:uint64 S64:sint64

m svcdef 132 BooleanValues B:boolean

m svcdef 133 FloatValuesList F:float D:double

m svcdef 135 IntegersList FixedValues:FixedValuesList VariableValues:VariableValuesList

m svcdef 136 NumbersList Integers:IntegersList Floats:FloatValuesList

m svcdef 140 HugeService CLAv4:CLA-TCP-v4 Booleans:BooleanValues Numbers:NumbersList FR:FooRouter

a svcadv HugeService CLAv4:IP:10.1.0.10 CLAv4:Port:4444 Booleans:true FR:Seed:0x5432 FR:BaseWeight:13 FR:RootHash:BEEF Numbers:Integers:FixedValues:F16:0x16 Numbers:Integers:FixedValues:F32:0x32 Numbers:Integers:FixedValues:F64:0x1234567890ABCDEF Numbers:Floats:F:0.32 Numbers:Floats:D:-1e-6 Numbers:Integers:VariableValues:U64:18446744073704783380 Numbers:Integers:VariableValues:S64:-4611686018422619668

This shows how to define multiple nested services and how to advertise them.

# SEE ALSO

ion(3)
