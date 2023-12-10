# NAME

amsrc - CCSDS Asynchronous Message Service MIB initialization file

# DESCRIPTION

The Management Information Base (MIB) for an AMS communicating entity (either
**amsd** or an AMS application module) must contain enough information to
enable the entity to initiate participation in AMS message exchange, such
as the network location of the configuration server and the roles and message
subjects defined for some venture.

AMS entities automatically load their MIBs from initialization files at
startup.  When AMS is built with the -DNOEXPAT compiler option set, the
MIB initialization file must conform to the _amsrc_ syntax described
below; otherwise the _expat_ XML parsing library must be linked into
the AMS executable and the MIB initialization file must conform to the
_amsxml_ syntax described in amsxml(5).

The MIB initialization file lists _elements_ of MIB update information,
each of which may have one or more _attributes_.  An element may also
have sub-elements that are listed within the declaration of the parent
element, and so on.

The declaration of an element may occupy a single line of text in the
MIB initialization file or may extend across multiple lines.  A single-line
element declaration is indicated by a '\*' in the first character of the
line.  The beginning of a multi-line element declaration is indicated by
a '+' in the first character of the line, while the end of that declaration
is indicated by a '-' in the first character of the line.  In every case,
the type of element must be indicated by an element-type name beginning
in the second character of the line and terminated by whitespace.  Every
start-of-element line **must** be matched by a subsequent end-of-element
line that precedes the start of any other element that is not a nested
sub-element of this element.

Attributes are represented by whitespace-terminated &lt;name>=&lt;value>
expressions immediately following the element-type name on a '\*' or
'+' line.  An attribute value that contains whitespace must be enclosed
within a pair of single-quote (') characters.

Two types of elements are recognized in the MIB initialization file:
control elements and configuration elements.  A control element establishes
the update context within which the configuration elements nested within
it are processed, while a configuration element declares values for one
or more items of AMS configuration information in the MIB.

Note that an aggregate configuration element (i.e., one which may contain
other interior configuration elements; venture, for example) may be presented
outside of any control element, simply to establish the context in which 
subsequent control elements are to be interpreted.

# CONTROL ELEMENTS

- **ams\_mib\_init**

    Initializes an empty MIB.  This element must be declared prior to the
    declaration of any other element.

    Sub-elements: none

    Attributes:

    - continuum\_nbr

        Identifies the local continuum.

    - ptsname

        Identifies the primary transport service for the continuum.  Valid values
        include "dgr" and "udp".

    - pubkey

        This is the name of the public key used for validating the digital
        signatures of meta-AMS messages received from the configuration server
        for this continuum.  The value of this attribute (if present) must
        identify a key that has been loaded into the ION security database,
        nominally by ionsecadmin(1).

    - privkey

        This is the name of the private key used for constructing the digital
        signatures of meta-AMS messages sent by the configuration server
        for this continuum.  This attribute should **only** be present in
        the MIB initialization file for amsd().  The value of this attribute
        (if present) must identify a key that has been loaded into the ION
        security database, nominally by ionsecadmin(1).

- **ams\_mib\_add**

    This element contains a list of configuration items that are to be
    added to the MIB.

- **ams\_mib\_change**

    This element contains a list of configuration items that are to be
    revised in the MIB.

- **ams\_mib\_delete**

    This element contains a list of configuration items that are to be
    deleted from the MIB.

# CONFIGURATION ELEMENTS

- **continuum**

    Identifies a known remote continuum.

    Sub-elements: none

    Attributes:

    - nbr

        Identifies the remote continuum.

    - name

        Identifies the remote continuum.

    - neighbor

        1 if the continuum is a neighbor of the local continuum, zero otherwise.

    - desc

        A textual description of this continuum.

- **csendpoint**

    Identifies one of the network locations at which the configuration server
    may be reachable.  If the configuration server might be running at any one
    of several locations, the number of other locations that are preferred to
    this one is noted; in this case, csendpoints must be listed within the
    ams\_mib\_add element in descending order of preference, i.e., with the most
    preferred network location listed first.

    Sub-elements: none

    Attributes:

    - epspec

        Identifies the endpoint at which the configuration server may be
        reachable.  The endpoint specification must conform to the endpoint
        specification syntax defined for the continuum's primary transport
        service; see the AMS Blue Book for details.

    - after

        If present, indicates the number of other configuration server network
        locations that are considered preferable to this one.  This attribute is
        used to ensure that csendpoints are listed in descending order of
        preference.

- **amsendpoint**

    Normally the specifications of the transport service endpoints at which
    an AMS application module can receive messages are computed automatically
    using standard transport-service-specific rules.  However, in some cases
    it might be necessary for a module to receive messages at one or more
    non-standard endpoints; in these cases, amsendpoint elements can be
    declared in order to override the standard endpoint specification rules.

    Sub-elements: none

    Attributes:

    - tsname

        Identifies the transport service for which a non-standard endpoint
        specification is being supplied.

    - epspec

        Identifies an endpoint at which the application module will be reachable,
        in the context of the named transport service.  The endpoint specification
        must conform to the endpoint specification syntax defined for the named
        transport service; see the AMS Blue Book for details.

- **application**

    Identifies one of the applications supported within this continuum.

    Sub-elements: none

    Attributes:

    - name

        Identifies the application.

    - pubkey

        This is the name of the public key used for validating the digital
        signatures of meta-AMS messages received from the registrars for all
        cells of any message space in this continuum that is characterized by
        this application name.  The value of this attribute (if present) must
        identify a key that has been loaded into the ION security database,
        nominally by ionsecadmin(1).

    - privkey

        This is the name of the private key used for constructing the digital
        signatures of meta-AMS messages sent by the registrars for all cells
        of any message space in this continuum that is characterized by this
        application name.  This attribute should **only** be present in
        the MIB initialization file for amsd().  The value of this attribute
        (if present) must identify a key that has been loaded into the ION
        security database, nominally by ionsecadmin(1).

- **venture**

    Identifies one of the ventures operating within the local continuum.

    Sub-elements: role, subject, unit, msgspace

    Attributes:

    - nbr

        Identifies the venture.

    - appname

        Identifies the application addressed by this venture.

    - authname

        Identifies the authority under which the venture operates, distinguishing
        this venture from all other ventures that address the same application.

    - gweid

        Identifies the RAMS network endpoint ID of the RAMS gateway module for
        this venture's message space in the local continuum.  Gateway endpoint
        ID is expressed as &lt;protocol\_name>@&lt;eid\_string> where _protocol\_name_
        is either "bp" or "udp".  If protocol name is "bp" then _eid\_string_
        must be a valid Bundle Protocol endpoint ID; otherwise, _eid\_string_
        must be of the form &lt;hostname>:&lt;port\_number>.  If the gweid attribute
        is omitted, the RAMS gateway module's RAMS network endpoint ID defaults
        to "bp@ipn:&lt;local\_continuum\_number>.&lt;venture\_number>".

    - net\_config

        Has the value "tree" if the RAMS network supporting this venture is
        configured as a tree; otherwise "mesh", indicating that the RAMS network
        supporting this venture is configured as a mesh.

    - root\_cell\_resync\_period

        Indicates the number of seconds in the period on which resynchronization is
        performed for the root cell of this venture's message space in the local
        continuum.  If this attribute is omitted, resynchronization in that cell
        is disabled.

- **role**

    Identifies one of the functional roles in the venture that is the element
    that's currently being configured.

    Sub-elements: none

    Attributes:

    - nbr

        Identifies the role.

    - name

        Identifies the role.

    - authname

        Identifies the authority under which the venture operates, distinguishing
        this venture from all other ventures that address the same application.

    - pubkey

        This is the name of the public key used for validating the digital
        signatures of meta-AMS messages received from all application modules
        that register in this functional role.  The value of this attribute
        (if present) must identify a key that has been loaded into the ION
        security database, nominally by ionsecadmin(1).

    - privkey

        This is the name of the private key used for constructing the digital
        signatures of meta-AMS messages sent by all application modules that
        register in this functional role.  This attribute should **only** be
        present in the MIB initialization file for application modules that
        register in this role.  The value of this attribute (if present) must
        identify a key that has been loaded into the ION security database,
        nominally by ionsecadmin(1).

- **subject**

    Identifies one of the subjects on which messages may be sent, within
    the venture that is the element that's currently being configured.

    Sub-elements: sender, receiver

    Attributes:

    - nbr

        Identifies the subject.

    - name

        Identifies the subject.

    - desc

        A textual description of this message subject.

    - symkey

        This is the name of the symmetric key used for both encrypting and
        decrypting the content of messages on this subject; if omitted, messages
        on this subject are not encrypted by AMS.  If authorized senders and
        receivers are defined for this subject, then this attribute should
        **only** be present in the MIB initialization file for application
        modules that register in roles that appear in the subject's lists of
        authorized senders and/or receivers.  The value of this attribute
        (if present) must identify a key that has been loaded into the ION
        security database, nominally by ionsecadmin(1).

    - marshal

        This is the name associated with the content marshaling function
        defined for this message subject.  If present, whenever a message on
        this subject is issued the associated function is automatically called
        to convert the supplied content data to a platform-independent representation
        for transmission; this conversion occurs before any applicable content
        encryption is performed.  If omitted, content data are transmitted without
        conversion to a platform-independent representation.  Marshaling functions
        are defined in the marshalRules table in the marshal.c source file.

    - unmarshal

        This is the name associated with the content unmarshaling function
        defined for this message subject.  If present, whenever a message on
        this subject is received the associated function is automatically called
        to convert the transmitted content to local platform-specific
        representation; this conversion occurs after any applicable content
        decryption is performed.  If omitted, received content data are
        delivered without conversion to a local platform-specific representation.
        Unmarshaling functions are defined in the unmarshalRules table in the
        marshal.c source file.

- **sender**

    Identifies one of the roles in which application modules must register
    in order to be authorized senders of messages on the subject that is the
    element that's currently being configured.

    Sub-elements: none

    Attributes:

    - name

        Identifies the sender.  The value of this attribute must be the name of a role
        that has been defined for the venture that is currently being configured.

- **receiver**

    Identifies one of the roles in which application modules must register
    in order to be authorized receivers of messages on the subject that is the
    element that's currently being configured.

    Sub-elements: none

    Attributes:

    - name

        Identifies the receiver.  The value of this attribute must be the name of a
        role that has been defined for the venture that is currently being configured.

- **unit**

    Identifies one of the organizational units within the venture that is the
    element that's currently being configured.

    Sub-elements: none

    Attributes:

    - nbr

        Identifies the unit.

    - name

        Identifies the unit.

    - resync\_period

        Indicates the number of seconds in the period on which resynchronization is
        performed, for the cell of this venture's message space that is the portion
        of the indicated unit which resides in the local continuum.  If this attribute
        is omitted, resynchronization in that cell is disabled.

- **msgspace**

    Identifies one of the message spaces in remote continua that are encompassed
    by the venture that is the element that's currently being configured.

    Sub-elements: none

    Attributes:

    - nbr

        Identifies the remote continuum within which the message space operates.

    - gweid

        Identifies the RAMS network endpoint ID of the RAMS gateway module for
        this message space.  Gateway endpoint ID is expressed as
        &lt;protocol\_name>@&lt;eid\_string> where _protocol\_name_ is either "bp"
        or "udp".  If protocol name is "bp" then _eid\_string_ must be a
        valid Bundle Protocol endpoint ID; otherwise, _eid\_string_ must be
        of the form &lt;hostname>:&lt;port\_number>.  If the gweid attribute is omitted,
        the RAMS network endpoint ID of the message space's RAMS gateway
        module defaults to "bp@ipn:&lt;remote\_continuum\_number>.&lt;venture\_number>".

    - symkey

        This is the name of the symmetric key used for both encrypting and
        decrypting all messages to and from modules in the remote message
        space that are forwarded between the local RAMS gateway server and
        modules in the local message space; if omitted, these messages
        are not encrypted.  The value of this attribute (if present) must
        identify a key that has been loaded into the ION security database,
        nominally by ionsecadmin(1).

# EXAMPLE

\*ams\_mib\_init continuum\_nbr=2 ptsname=dgr

\+ams\_mib\_add

\*continuum nbr=1 name=apl desc=APL

\*csendpoint epspec=beaumont.stepsoncats.com:2357

\*application name=amsdemo

\+venture nbr=1 appname=amsdemo authname=test

\*role nbr=2 name=shell

\*role nbr=3 name=log

\*role nbr=4 name=pitch

\*role nbr=5 name=catch

\*role nbr=6 name=benchs

\*role nbr=7 name=benchr

\*role nbr=96 name=amsd

\*role nbr=97 name=amsmib

\*role nbr=98 name=amsstop

\*subject nbr=1 name=text desc='ASCII text'

\*subject nbr=2 name=noise desc='more ASCII text'

\*subject nbr=3 name=bench desc='numbered msgs'

\*subject nbr=97 name=amsmib desc='MIB updates'

\*subject nbr=98 name=amsstop desc='shutdown'

\*unit nbr=1 name=orbiters

\*unit nbr=2 name=orbiters.near

\*unit nbr=3 name=orbiters.far

\*msgspace nbr=2

\-venture

\-ams\_mib\_add

# SEE ALSO

amsxml(5)
