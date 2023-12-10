# NAME

amsmib - Asynchronous Message Service (AMS) MIB update utility

# SYNOPSIS

**amsmib** _application\_name_ _authority\_name_ _role\_name_ _continuum\_name_ _unit\_name_ _file\_name_

# DESCRIPTION

**amsmib** is a utility program that announces relatively brief Management
Information Base (MIB) updates to a select population of AMS modules.  Because
**amsd** processes may run AAMS modules in background threads, and because a
single MIB is shared in common among all threads of any process, **amsmib** may
update the MIBs used by registrars and/or configuration servers as well.

MIB updates can only be propagated to modules for which the subject "amsmib"
was defined in the MIB initialization files cited at module registration
time.  All ION AMS modules implicitly invite messages on subject "amsmib"
(from all modules registered in role "amsmib" in all continua of the same
venture) at registration time if subject "amsmib" and role "amsmib" are
defined in the MIB.

**amsmib** registers in the root cell of the message space identified by
_application\_name_ and _authority\_name_, within the local continuum.  It
registers in the role "amsmib"; if this role is not defined in the (initial)
MIB loaded by **amsmib** at registration time, then registration fails and
**amsmib** terminates.

**amsmib** then reads into a memory buffer up to 4095 bytes of MIB update
text from the file identified by _file\_name_.  The MIB update text must
conform to amsxml(5) or amsrc(5) syntax, depending on whether or not the
intended recipient modules were compiled with the -DNOEXPAT option.

**amsmib** then "announces" (see ams\_announce() in ams(3)) the contents of the
memory buffer to all modules of this same venture (identified by
_application\_name_ and _authority\_name_) that registered in the indicated
role, in the indicated unit of the indicated continuum.  If _continuum\_name_
is "" then the message will be sent to modules in all continua.  If
_role\_name_ is "" then all modules will be eligible to receive the message,
regardless of the role in which they registered.  If _unit\_name_ is "" (the
root unit) then all modules will be eligible to receive the message,
regardless of the unit in which they registered.

Upon reception of the announced message, each destination module will apply
all of the MIB updates in the content of the message, in exactly the same
way that its original MIB was loaded from the MIB initialization file when
the module started running.

If multiple modules are running in the same memory space (e.g., in different
threads of the same process, or in different tasks on the same VxWorks target)
then the updates will be applied multiple times, because all modules in the
same memory space share a single MIB.  MIB updates are idempotent, so this
is harmless (though some diagnostics may be printed).

Moreover, an **amsd** daemon will have a relevant "MIB update" module running
in a background thread if _application\_name_ and _authority\_name_ were cited 
on the command line that started the daemon (provided the role "amsd" was
defined in the initial MIB loaded at the time **amsd** began running).  The
MIB exposed to the configuration server and/or registrar running in that
daemon will likewise be updated upon reception of the announced message.

The name of the subject of the announced mib update message is "amsmib"; if
this subject is not defined in the (initial) MIB loaded by **amsmib** then
the message cannot be announced.  Nor can any potential recipient module
receive the message if subject "amsmib" is not defined in that module's MIB.

# EXIT STATUS

- "0"

    **amsmib** terminated normally.

- "1"

    An anomalous exit status, indicating that **amsmib** failed to register.

# FILES

A MIB initialization file with the applicable default name (see amsrc(5) and
amsxml(5)) must be present.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- amsmib subject undefined.

    The **amsmib** utility was unable to announce the MIB update message.

- amsmib domain role unknown.

    The **amsmib** utility was unable to announce the MIB update message.

- amsmib domain continuum unknown.

    The **amsmib** utility was unable to announce the MIB update message.

- amsmib domain unit unknown.

    The **amsmib** utility was unable to announce the MIB update message.

- amsmib can't open MIB file.

    The **amsmib** utility was unable to construct the MIB update message.

- MIB file length > 4096.

    The MIB update text file was too long to fit into the **amsmib** message buffer.

- Can't seek to end of MIB file.

    I/O error in processing the MIB update text file.

- Can't read MIB file.

    I/O error in processing the MIB update text file.

- amsmib can't announce 'amsmib' message.

    The **amsmib** utility was unable to announce the MIB update message, for
    reasons noted in the log file.

- amsmib can't register.

    The **amsmib** utility failed to register, for reasons noted in the log file.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

amsd(1), ams(3), amsrc(5), amsxml(5)
