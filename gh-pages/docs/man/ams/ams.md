# NAME

ams - CCSDS Asynchronous Message Service(AMS) communications library

# SYNOPSIS

    #include "ams.h"

    typedef void                (*AmsMsgHandler)(AmsModule module,
                                        void *userData,
                                        AmsEvent *eventRef,
                                        int continuumNbr,
                                        int unitNbr,
                                        int moduleNbr,
                                        int subjectNbr,
                                        int contentLength,
                                        char *content,
                                        int context,
                                        AmsMsgType msgType,
                                        int priority,
                                        unsigned char flowLabel);

    typedef void                (*AmsRegistrationHandler)(AmsModule module,
                                        void *userData,
                                        AmsEvent *eventRef,
                                        int unitNbr,
                                        int moduleNbr,
                                        int roleNbr);

    typedef void                (*AmsUnregistrationHandler)(AmsModule module,
                                        void *userData,
                                        AmsEvent *eventRef,
                                        int unitNbr,
                                        int moduleNbr);

    typedef void                (*AmsInvitationHandler)(AmsModule module,
                                        void *userData,
                                        AmsEvent *eventRef,
                                        int unitNbr,
                                        int moduleNbr,
                                        int domainRoleNbr,
                                        int domainContinuumNbr,
                                        int domainUnitNbr,
                                        int subjectNbr,
                                        int priority,
                                        unsigned char flowLabel,
                                        AmsSequence sequence,
                                        AmsDiligence diligence);

    typedef void                (*AmsDisinvitationHandler)(AmsModule module,
                                        void *userData,
                                        AmsEvent *eventRef,
                                        int unitNbr,
                                        int moduleNbr,
                                        int domainRoleNbr,
                                        int domainContinuumNbr,
                                        int domainUnitNbr,
                                        int subjectNbr);

    typedef void                (*AmsSubscriptionHandler)(AmsModule module,
                                        void *userData,
                                        AmsEvent *eventRef,
                                        int unitNbr,
                                        int moduleNbr,
                                        int domainRoleNbr,
                                        int domainContinuumNbr,
                                        int domainUnitNbr,
                                        int subjectNbr,
                                        int priority,
                                        unsigned char flowLabel,
                                        AmsSequence sequence,
                                        AmsDiligence diligence);

    typedef void                (*AmsUnsubscriptionHandler)(AmsModule module,
                                        void *userData,
                                        AmsEvent *eventRef,
                                        int unitNbr,
                                        int moduleNbr,
                                        int domainRoleNbr,
                                        int domainContinuumNbr,
                                        int domainUnitNbr,
                                        int subjectNbr);

    typedef void                (*AmsUserEventHandler)(AmsModule module,
                                        void *userData,
                                        AmsEvent *eventRef,
                                        int code,
                                        int dataLength,
                                        char *data);

    typedef void                (*AmsMgtErrHandler)(void *userData,
                                        AmsEvent *eventRef);

    typedef struct
    {
        AmsMsgHandler                   msgHandler;
        void                            *msgHandlerUserData;
        AmsRegistrationHandler          registrationHandler;
        void                            *registrationHandlerUserData;
        AmsUnregistrationHandler        unregistrationHandler;
        void                            *unregistrationHandlerUserData;
        AmsInvitationHandler            invitationHandler;
        void                            *invitationHandlerUserData;
        AmsDisinvitationHandler         disinvitationHandler;
        void                            *disinvitationHandlerUserData;
        AmsSubscriptionHandler          subscriptionHandler;
        void                            *subscriptionHandlerUserData;
        AmsUnsubscriptionHandler        unsubscriptionHandler;
        void                            *unsubscriptionHandlerUserData;
        AmsUserEventHandler             userEventHandler;
        void                            *userEventHandlerUserData;
        AmsMgtErrHandler                errHandler;
        void                            *errHandlerUserData;
    } AmsEventMgt;

    typedef enum
    {
        AmsArrivalOrder = 0,
        AmsTransmissionOrder
    } AmsSequence;

    typedef enum
    {
        AmsBestEffort = 0,
        AmsAssured
    } AmsDiligence;

    typedef enum
    {
        AmsRegistrationState,
        AmsInvitationState,
        AmsSubscriptionState
    } AmsStateType;

    typedef enum
    {
        AmsStateBegins = 1,
        AmsStateEnds
    } AmsChangeType;

    typedef enum
    {
        AmsMsgUnary = 0,
        AmsMsgQuery,
        AmsMsgReply,
        AmsMsgNone
    } AmsMsgType;

    [see description for available functions]

# DESCRIPTION

The ams library provides functions enabling application software to use AMS
to send and receive brief messages, up to 65000 bytes in length.  It conforms
to AMS Blue Book, including support for Remote AMS (RAMS).

In the ION implementation of RAMS, the "RAMS network protocol" may be either
the DTN Bundle Protocol (RFC 5050) or -- mainly for testing purposes -- the
User Datagram Protocol (RFC 768).  BP is the default.  When BP is used as
the RAMS network protocol, endpoints are by default assumed to conform to
the "ipn" endpoint identifier scheme with **node number** set to the AMS
**continuum number** and **service number** set to the AMS **venture number**.

Note that RAMS functionality is enabled by instantiating a **ramsgate** daemon,
which is simply an AMS application program that acts as a gateway between the
local AMS message space and the RAMS network.

AMS differs from other ION packages in that there is no utilization of
non-volatile storage (aside from the BP functionality in RAMS, if applicable).
Since there is no non-volatile AMS database, there is no AMS administration
program and there are no library functions for attaching to or detaching
from such a database.  AMS is instantiated by commencing operation of the
AMS real-time daemon **amsd**; once **amsd** is running, AMS application
programs ("modules") can be started.  All management of AMS operational
state is performed automatically in real time.

However, **amsd** and the AMS application programs all require
access to a common store of configuration data at startup in order to load
their Management Information Bases.  This configuration data must reside in
a readable file, which may take either of two forms: a file of XML statements
conforming to the scheme described in the amsxml(5) man page, or a file of
simple but less powerful configuration statements as described in the amsrc(5)
man page.  The **amsxml** alternative requires that the **expat** XML parsing
system be installed; the **amsrc** alternative was developed to simplify
deployment of AMS in environments in which **expat** is not readily supported.
Selection of the configuration file format is a compile-time decision,
implemented by setting (or not setting) the -DNOEXPAT compiler option.

The path name of the applicable configuration file may be passed as a
command-line parameter to **amsd** and as a registration function parameter
by any AMS application program.  Note, though, that **ramsgate** and the
AMS test and utility programs included in ION always assume that the
configuration file resides in the current working directory and that it is
named "mib.amsrc" if AMS was built with -DNOEXPAT, "amsmib.xml" otherwise.

The transport services that are made available to AMS communicating entities
are declared by the transportServiceLoaders array in the libams.c source
file.  This array is fixed at compile time.  The order of preference of the
transport services in the array is hard-coded, but the inclusion or omission 
of individual transport services is controlled by setting compiler options.
The "udp" transport service -- nominally the most preferred because it
imposes the least processing and transmission overhead -- is included by
setting the -DUDPTS option.  The "dgr" service is included by setting the
\-DDGRTS option.  The "vmq" (VxWorks message queue) service, supported only
on VxWorks, is included by setting the -DVMQTS option.  The "tcp" transport
service -- selected only when its quality of service is required -- is
included by setting the -DTCPTS option.

The operating state of any single AMS application program is managed in
an opaque AmsModule object.  This object is returned when the application
begins AMS operations (that is, registers) and must be provided as an
argument to most AMS functions.

- int ams\_register(char \*mibSource, char \*tsorder, char \*applicationName, char \*authorityName, char \*unitName, char \*roleName, AmsModule \*module)

    Registers the application within a cell (identified by _unitName_) of a
    message space that is that portion of the venture identified by
    _applicationName_ and _authorityName_ that runs within the local AMS
    continuum.  _roleName_ identifies the role that this application will
    perform in this venture.  The operating state of the registered application
    is returned in _module_.

    The application module's identifying parameters are validated against the
    configuration information in the applicable Management Information Base,
    which is automatically loaded from the file whose pathname is provided
    in _mibSource_.  If _mibSource_ is the zero-length string ("") then
    the default configuration file name is used as noted above.  If
    _mibSource_ is NULL then a rudimentary hard-coded default MIB, useful
    for basic testing purposes, is loaded.  This default MIB defines a single
    venture for application "amsdemo" and authority "test", using only the
    "dgr" transport service, with the configuration server residing on the
    local host machine; subject "text" and roles "shell", "log", "pitch",
    and "catch" are defined.

    The _tsorder_ argument is normally NULL.  If non-NULL it must be a
    NULL-terminated string of ASCII numeric digits '0' through '9' identifying
    an alternative transport service preference order that overrides the standard
    transport service preference order defined by the hard-coded array of
    transportServiceLoaders in the libams.c source file.  Each character of
    the _tsorder_ string must represent the index position of one of the
    transport services within the array.  For example, if services "udp", "dgr",
    "vmq", and "tcp" are all available in the array, a _tsorder_ string of "32" 
    would indicate that this application will only communicate using the tcp
    and vmq services; services 0 (udp) and 1 (dgr) will not be used, and tcp
    is preferred to vmq when both are candidate services for transmission of
    a given message.

    Returns 0 on success.  On any error, sets _module_ to NULL and returns -1.

- int ams\_unregister(AmsModule module)

    Reverses the operation of ams\_unregister(), destroying _module_.  Returns
    0 on success, -1 on any error.

- int ams\_invite(AmsModule module, int roleNbr, int continuumNbr, int unitNbr, int subjectNbr, int priority, unsigned char flowLabel, AmsSequence sequence, AmsDiligence diligence)

    Announces this module's agreement to receive messages on the subject
    identified by _subjectNbr_.

    The invitation is extended only to modules registered in the role identified
    by _roleNbr_ (where 0 indicates "all roles"), operating in the continuum
    identifed by _continuumNbr_ (where 0 indicates "all continua"), and
    registered within the unit identified by _unitNbr_ (where 0 indicates
    the venture's root unit) or any of that unit's subunits.  These parameters
    define the "domain" of the invitation.

    Such messages should be sent at the priority indicated by _priority_ with
    flow label as indicated by _flowLabel_ and with quality of service as
    indicated by _sequence_ and _diligence_.  _priority_ must be an integer
    in the range 1-15, where priority 1 indicates the greatest urgency.  Flow
    labels are passed through to transport services and are opaque to AMS itself;
    in the absence of defined flow labels, a value of 0 is typically used.  These
    parameters define the "class of service" of the invitation.

    Returns 0 on success, -1 on any error.

- int ams\_disinvite(AmsModule module, int roleNbr, int continuumNbr, int unitNbr, int subjectNbr)

    Rescinds the invitation characterized by the indicated subject and
    domain.  Returns 0 on success, -1 on any error.

- int ams\_subscribe(AmsModule module, int roleNbr, int continuumNbr, int unitNbr, int subjectNbr, int priority, unsigned char flowLabel, AmsSequence sequence, AmsDiligence diligence)

    Announces this module's subscription to messages on the indicated subject,
    constrained by the indicated domain, and transmitted subject to the indicated
    class of service.  Note that subscriptions differ from invitations in that 
    reception of these messages is actively solicited, not just permitted.

    Returns 0 on success, -1 on any error.

- int ams\_unsubscribe(AmsModule module, int roleNbr, int continuumNbr, int unitNbr, int subjectNbr)

    Cancels the subscription characterized by the indicated subject and
    domain.  Returns 0 on success, -1 on any error.

- int ams\_publish(AmsModule module, int subjectNbr, int priority, unsigned char flowLabel, int contentLength, char \*content, int context)

    Publishes _contentLength_ bytes of data starting at _content_ as the content
    of a message that is sent to all modules whose subscriptions to _subjectNbr_
    are characterized by a domain that includes this module.  _priority_ and
    _flowLabel_, if non-zero, override class of service as requested in the
    subscriptions.  _context_ is an opaque "hint" to the receiving modules;
    its use is application-specific.

    Returns 0 on success, -1 on any error.

- int ams\_send(AmsModule module, int continuumNbr, int unitNbr, int moduleNbr, int subjectNbr, int priority, unsigned char flowLabel, int contentLength, char \*content, int context)

    Sends _contentLength_ bytes of data starting at _content_ as the content
    of a message that is transmitted privately to the module in the continuum
    identified by _continuumNbr_ (where 0 indicates "the local continuum") that
    is identified by _unitNbr_ and _moduleNbr_ -- provided that _module_ is
    in the domain of one of that module's invitations on _subjectNbr_.
    _priority_ and _flowLabel_, if non-zero, override class of service as
    requested in the invitation.  _context_ is an opaque "hint" to the receiving
    module; its use is application-specific.

    Returns 0 on success, -1 on any error.

- int ams\_query(AmsModule module, int continuumNbr, int unitNbr, int moduleNbr, int subjectNbr, int priority, unsigned char flowLabel, int contentLength, char \*content, int context, int term, AmsEvent \*event)

    Sends a message exactly is described above for ams\_send(), but additionally
    suspends the delivery and processing of newly received messages until either
    (a) a "reply" message sent in response to this message is received or (b) the
    time interval indicated by _term_, in seconds, expires.  The event (reply or
    timeout) that ends the suspension of processing is provided in _event_ (as
    if from ams\_get\_event() when the function returns. 

    If _term_ is AMS\_BLOCKING then the timeout interval is indefinite; only
    reception of a reply message enables the function to return.  If _term_ is
    AMS\_POLL then the function returns immediately, without waiting for a reply
    message.

    Returns 0 on success, -1 on any error.

- int ams\_reply(AmsModule module, AmsEvent msg, int subjectNbr, int priority, unsigned char flowLabel, int contentLength, char \*content)

    Sends a message exactly is described above for ams\_send(), except that the
    destination of the message is the sender of the message identified by _msg_
    and the "context" value included in the message is the context that was
    provided in _msg_.  This message is identified as a "reply" message that
    will end the processing suspension resulting from transmission of _msg_ if
    that message was issued by means of ams\_query() rather than ams\_send().

    Returns 0 on success, -1 on any error.

- int ams\_announce(AmsModule module, int roleNbr, int continuumNbr, int unitNbr, int subjectNbr, int priority, unsigned char flowLabel, int contentLength, char \*content, int context)

    Sends a message exactly is described above for ams\_send(), except that one
    copy of the message is sent to every module in the domain of this function
    (role, continuum, unit) whose invitation for messages on this subject is
    itself characterized by a domain that includes the the sending module, rather
    than to any specific module.  

    Returns 0 on success, -1 on any error.

- int ams\_get\_event(AmsModule module, int term, AmsEvent \*event)

    Returns in _event_ the next event in the queue of AMS events pending delivery
    to this module.  If the event queue is empty at the time this function is
    called, processing is suspended until either an event is queued or the time
    interval indicated by _term_, in seconds, expires.  See ams\_query() above
    for the semantics of _term_.  When the function returns on expiration of
    _term_, an event of type TIMEOUT\_EVT is returned in _event_.  Otherwise
    the event will be of type AMS\_MSG\_EVT (indicating arrival of a message),
    NOTICE\_EVT (indicating a change in the configuration of the message space),
    or USER\_DEFINED\_EVT (indicating that application code posted an event).

    The nature of the event returned by ams\_get\_event() can be determined by
    passing _event_ to ams\_get\_event\_type() as described below.  Event type can
    then be used to determine whether the information content of the event
    must be obtained by calling ams\_parse\_msg(), ams\_parse\_notice(), or
    ams\_parse\_user\_event().  

    In any case, the memory occupied by _event_ must be released after the
    event object is no longer needed.  The ams\_recycle\_event() function is
    invoked for this purpose.

    Returns 0 on success, -1 on any error.

- int ams\_get\_event\_type(AmsEvent event)

    Returns the event type of _event_, or -1 on any error.

- int ams\_parse\_msg(AmsEvent event, int \*continuumNbr, int \*unitNbr, int \*moduleNbr, int \*subjectNbr, int \*contentLength, char \*\*content, int \*context, AmsMsgType \*msgType, int \*priority, unsigned char \*flowLabel);

    Extracts all relevant information pertaining to the AMS message encapsulated
    in _event_, populating the indicated fields.  Must only be called when
    the event type of _event_ is known to be AMS\_MSG\_EVT.

    Returns 0 on success, -1 on any error.

- int ams\_parse\_notice(AmsEvent event, AmsStateType \*state, AmsChangeType \*change, int \*unitNbr, int \*moduleNbr, int \*roleNbr, int \*domainContinuumNbr, int \*domainUnitNbr, int \*subjectNbr, int \*priority, unsigned char \*flowLabel, AmsSequence \*sequence, AmsDiligence \*diligence)

    Extracts all relevant information pertaining to the AMS configuration change
    notice encapsulated in _event_, populating the relevant fields.  Must only
    be called when the event type of _event_ is known to be NOTICE\_EVT.

    Note that different fields will be populated depending on the nature of the
    notice in _event_.  _state_ will be set to AmsRegistrationState,
    AmsInvitationState, or AmsSubscription state depending on whether the
    notice pertains to a change in module registration, a change in invitations,
    or a change in subscriptions.  _change_ will be set to AmsStateBegins or
    AmsStateEnds depending on whether the notice pertains to the initiation or
    termination of a registration, invitation, or subscription.

    Returns 0 on success, -1 on any error.

- int ams\_post\_user\_event(AmsModule module, int userEventCode, int userEventDataLength, char \*userEventData, int priority)

    Posts a "user event" whose content is the _userEventDataLength_ bytes of
    data starting at _userEventData_.  _userEventCode_ is an application-specific
    value that is opaque to AMS.  _priority_ determines the event's position in
    the queue of events pending delivery to this module; it may be any integer
    in the range 0-15, where 0 indicates the greatest urgency.  (Note that user
    events can be delivered ahead of all message reception events if necessary.)

    Returns 0 on success, -1 on any error.

- int ams\_parse\_user\_event(AmsEvent event, int \*code, int \*dataLength, char \*\*data)

    Extracts all relevant information pertaining to the user event encapsulated
    in _event_, populating the indicated fields.  Must only be called when
    the event type of _event_ is known to be USER\_DEFINED\_EVT.

    Returns 0 on success, -1 on any error.

- int ams\_recycle\_event(AmsEvent event)

    Releases all memory occupied by _event_.  Returns 0 on success, -1 on any
    error.

- int ams\_set\_event\_mgr(AmsModule module, AmsEventMgt \*rules)

    Starts a background thread that processes events queued for this module,
    handling each event in the manner indicated by _rules_.  Returns 0 on
    success, -1 on any error.

- void ams\_remove\_event\_mgr(AmsModule module)

    Terminates the background thread established to process events queued for
    this module.  Returns 0 on success, -1 on any error.

- int ams\_get\_module\_nbr(AmsModule module)

    Returns the module number assigned to this module upon registration, or -1
    on any error.

- int ams\_get\_unit\_nbr(AmsModule module)

    Returns the unit number assigned to the unit within which this module
    registered, or -1 on any error.

- Lyst ams\_list\_msgspaces(AmsModule module)

    Returns a dynamically allocated linked list of all message spaces identified
    in the MIB for this module, or -1 on any error.  See lyst(3) for operations
    that can be performed on the returned linked list.

- int ams\_get\_continuum\_nbr()

    Returns the continuum number assigned to the continuum within which this
    module operates, or -1 on any error.

- int ams\_rams\_net\_is\_tree(AmsModule module)

    Returns 1 if the RAMS net for the venture in which this module is registered
    is configured as a tree, 0 if that RAMS net is configured as a mesh, -1 on
    any error.

- int ams\_continuum\_is\_neighbor(int continuumNbr)

    Returns 1 if _continuumNbr_ identifies a continuum whose RAMS gateways
    are immediate neighbors (within the applicable RAMS networks) of the
    RAMS gateways in the local continuum.  Returns 0 otherwise.

- char \*ams\_get\_role\_name(AmsModule module, int unitNbr, int moduleNbr)

    Returns the name of the role in which the module identified by _unitNbr_ and
    _moduleNbr_ registered, or NULL on any error.

- int ams\_subunit\_of(AmsModule module, int argUnitNbr, int refUnitNbr)

    Returns 1 if _argUnitNbr_ identifies a unit that is wholly contained within
    the unit identified by _refUnitNbr_, in the venture within which this
    module is registered.  Returns 0 otherwise.

- int ams\_lookup\_unit\_nbr(AmsModule module, char \*unitName)

    Returns the number assigned to the unit identified by _unitName_, in
    the venture within which this module is registered, or -1 on any error.

- int ams\_lookup\_role\_nbr(AmsModule module, char \*roleName)

    Returns the number assigned to the role identified by _roleName_, in
    the venture within which this module is registered, or -1 on any error.

- int ams\_lookup\_subject\_nbr(AmsModule module, char \*subjectName)

    Returns the number assigned to the subject identified by _subjectName_, in
    the venture within which this module is registered, or -1 on any error.

- int ams\_lookup\_continuum\_nbr(AmsModule module, char \*continuumName)

    Returns the number of the continuum identified by _continuumName_, or -1
    on any error.

- char \*ams\_lookup\_unit\_name(AmsModule module, int unitNbr)

    Returns the name of the unit identified by _unitNbr_, in
    the venture within which this module is registered, or -1 on any error.

- char \*ams\_lookup\_role\_name(AmsModule module, int roleNbr)

    Returns the name of the role identified by _roleNbr_, in
    the venture within which this module is registered, or -1 on any error.

- char \*ams\_lookup\_subject\_name(AmsModule module, int subjectNbr)

    Returns the name of the subject identified by _subjectNbr_, in
    the venture within which this module is registered, or -1 on any error.

- char \*ams\_lookup\_continuum\_name(AmsModule module, int continuumNbr)

    Returns the name of the continuum identified by _continuumNbr_, or -1
    on any error.

# SEE ALSO

amsd(1), ramsgate(1), amsxml(5), amsrc(5)
