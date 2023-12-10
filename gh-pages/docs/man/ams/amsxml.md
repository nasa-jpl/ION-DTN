# NAME

amsxml - CCSDS Asynchronous Message Service MIB initialization XML file

# DESCRIPTION

The Management Information Base (MIB) for an AMS communicating entity (either
**amsd** or an AMS application module) must contain enough information to
enable the entity to initiate participation in AMS message exchange, such
as the network location of the configuration server and the roles and message
subjects defined for some venture.

AMS entities automatically load their MIBs from initialization files at
startup.  When AMS is built with the -DNOEXPAT compiler option set, the
MIB initialization file must conform to the _amsrc_ syntax described
in amsrc(5); otherwise the _expat_ XML parsing library must be linked into
the AMS executable and the MIB initialization file must conform to the
_amsxml_ syntax described below.

The XML statements in the MIB initialization file constitute _elements_ of
MIB update information, each of which may have one or more _attributes_.  An
element may also have sub-elements that are listed within the declaration of
the parent element, and so on.

Two types of elements are recognized in the MIB initialization file:
control elements and configuration elements.  A control element establishes
the update context within which the configuration elements nested within
it are processed, while a configuration element declares values for one
or more items of AMS configuration information in the MIB.

For a discussion of the recognized control elements and configuration elements
of the MIB initialization file, see the amsrc(5) man page.  **NOTE**, though,
that all elements of an XML-based MIB initialization file **must** be
sub-elements of a single sub-element of the XML extension type **ams\_load\_mib**
in order for the file to be parsed successfully by expat.

# EXAMPLE

&lt;?xml version="1.0" standalone="yes"?>

&lt;ams\_mib\_load>

        <ams_mib_init continuum_nbr="2" ptsname="dgr"/>

        <ams_mib_add>

                <continuum nbr="1" name="apl" desc="APL"/>

                <csendpoint epspec="beaumont.stepsoncats.com:2357"/>

                <application name="amsdemo" />

                <venture nbr="1" appname="amsdemo" authname="test">

                        <role nbr="2" name="shell"/>

                        <role nbr="3" name="log"/>

                        <role nbr="4" name="pitch"/>

                        <role nbr="5" name="catch"/>

                        <role nbr="6" name="benchs"/>

                        <role nbr="7" name="benchr"/>

                        <role nbr="96" name="amsd"/>

                        <role nbr="97" name="amsmib"/>

                        <role nbr="98" name="amsstop"/>

                        <subject nbr="1" name="text" desc="ASCII text"/>

                        <subject nbr="2" name="noise" desc="more ASCII text"/>

                        <subject nbr="3" name="bench" desc="numbered msgs"/>

                        <subject nbr="97" name="amsmib" desc="MIB updates"/>

                        <subject nbr="98" name="amsstop" desc="shutdown"/>

                        <unit nbr="1" name="orbiters"/>

                        <unit nbr="2" name="orbiters.near"/>

                        <unit nbr="3" name="orbiters.far"/>

                        <msgspace nbr="2"/>

                </venture>

        </ams_mib_add>

&lt;/ams\_mib\_load>

# SEE ALSO

amsrc(5)
