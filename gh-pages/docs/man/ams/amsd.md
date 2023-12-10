# NAME

amsd - AMS configuration server and/or registrar daemon

# SYNOPSIS

**amsd** { @ | _MIB\_source\_name_ } { . | @ | _config\_server\_endpoint\_spec_ } \[_application\_name_ _authority\_name_ _registrar\_unit\_name_\]

# DESCRIPTION

**amsd** is a background "daemon" task that functions as an AMS "configuration
server" in the local continuum, as an AMS "registrar" in a specified cell,
or both.

If _MIB\_source\_name_ is specified, it must name a MIB initialization file
in the correct format for **amsd**, either amsrc(5) or amsxml(5), depending on
whether or not -DNOEXPAT was set at compile time.  Otherwise **@** is required;
in this case, the built-in default MIB is loaded.

If this **amsd** task is **NOT** to run as a configuration server then the
second command-line argument must be a '.' character.  Otherwise the second
command-line argument must be either '@' or _config\_server\_endpoint\_spec_.
If '@' then the endpoint specification for this configuration server is
automatically computed as the default endpoint specification for the primary
transport service as noted in the MIB: "_hostname_:2357".

If an AMS module is **NOT** to be run in a background thread for this daemon
(enabling shutdown by amsstop(1) and/or runtime MIB update by amsmib(1)), 
then either the last three command-line arguments must be omitted or else the
"amsd" role must not be defined in the MIB loaded for this daemon.  Otherwise
the _application\_name_ and _authority\_name_ arguments are required and
the "amsd" role must be defined in the MIB.

If this **amsd** task is **NOT** to run as a registrar then the last
command-line argument must be omitted.  Otherwise the last three command-line
arguments are required and they must identify a unit in an AMS venture for
the indicated application and authority that is known to operate in the local
continuum, as noted in the MIB.  Note that the unit name for the "root unit"
of a venture is the zero-length string "".

# EXIT STATUS

- "0"

    **amsd** terminated without error.

- -1

    **amsd** terminated due to an anomaly as noted in the **ion.log** file.  If this
    termination was not commanded, investigate and solve the problem identified
    in the log file and restart **amsd**.

# FILES

If _MIB source name_ is specified, then a file of this name must be present.
Otherwise a MIB initialization file with the applicable default name (see
amsrc(5)) must be present.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- amsd can't load MIB.

    MIB initialization file was missing, unreadable, or invalid.

- amsd can't start CS.

    Configuration server initialization failed for reasons noted in ion.log file.

- amsd can't start RS.

    Registrar initialization failed for reasons noted in ion.log file.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

amsmib(1), amsstop(1), amsrc(5), amsxml(5)
