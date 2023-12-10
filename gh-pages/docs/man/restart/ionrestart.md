# NAME

ionrestart - ION node restart utility

# SYNOPSIS

**ionrestart**

# DESCRIPTION

**ionrestart** is a utility task that is executed automatically when an ION
transaction fails, provided transaction reversibility is enabled (see
ionconfig(5)).  It should never need to be executed from the command line.

When an ION transaction is reversed, all changes made to the SDR non-volatile
heap in the course of the transaction are backed out but changes made to ION's
working memory are not.  (Forward logging of these changes to enable
automatic reversal on these relatively rare occasions is judged to impose
too much continuous processing overhead to be cost-justified.)  Because the
state of working memory is thereupon in conflict with information in the heap,
**ionrestart** is automatically invoked to reload all of working memory;
because this would obviously threaten the stability of all running ION tasks,
**ionrestart** gracefully terminates the tasks of the node (not only ION
daemons but also applications), then reloads working memory from the
recovered heap, and finally restarts the ION daemons.  Applications that
receive termination indications from **ionrestart** may choose to sleep for
a few seconds and then automatically re-initialize their own operations.

**ionrestart** will attempt to restart all core ION protocols including
LTP, BP, and CFDP, but any protocols which were not operating at the time
of the transaction reversal are not restarted.  Also, protocols that
**ionrestart** has not yet been adapted to cleanly terminate and restart
(including, at the time of this writing, BSSP and DPTC) are not restarted.

# EXIT STATUS

- "0"

    **ionrestart** terminated normally.

- "1"

    **ionrestart** failed, for reasons noted in the ion.log file; the task
    terminated.

# FILES

No configuration files are used beyond those required for normal ION
node initialization.

# ENVIRONMENT

No environment variables apply.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

ionadmin(1), ltpadmin(1), bpadmin(1), cfdpadmin(1)
