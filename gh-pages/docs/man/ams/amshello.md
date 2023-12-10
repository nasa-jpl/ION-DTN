# NAME

amshello - Asynchronous Message Service (AMS) demo program for UNIX

# SYNOPSIS

**amshello**

# DESCRIPTION

**amshello** is a sample program designed to demonstrate that an entire
(very simple) distributed AMS application can be written in just a few
lines of C code.  When started, **amshello** forks a second process and
initiates transmission of a "Hello" text message from one process to the
other, after which both processes unregister and terminate.

The **amshello** processes will register as application modules in the
root unit of the venture identified by application name "amsdemo" and
authority name "test".  A configuration server for the local continuum
and a registrar for the root unit of that venture (which may both be
instantiated in a single **amsd** daemon task) must be running in order
for the **amshello** processes to run.

# EXIT STATUS

- "0"

    **amshello** terminated normally.

# FILES

A MIB initialization file with the applicable default name (see amsrc(5))
must be present.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

No diagnostics apply.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

amsrc(5)
