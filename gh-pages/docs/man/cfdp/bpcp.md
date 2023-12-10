# NAME

bpcp - A remote copy utility for delay tolerant networks utilizing
NASA JPL's Interplanetary Overlay Network (ION)

# SYNOPSIS

**bpcp** \[-dqr | -v\] \[-L _bundle\_lifetime_\] \[-C _custody\_on/off_\] 
\[-S _class\_of\_service_\] \[_host1_:\]_file1_ ... \[_host2_:\]_file2_	

# DESCRIPTION

**bpcp** copies files between hosts utilizing NASA JPL's Interplanetary
Overlay Network (ION) to provide a delay tolerant network. File copies
from local to remote, remote to local, or remote to remote are permitted.
**bpcp** depends on ION to do any authentication or encryption of file transfers.
All covergence layers over which **bpcp** runs MUST be reliable.

The options are permitted as follows:

- ** -d**	Debug output. Repeat for increased verbosity.
- ** -q**	Quiet. Do not output status messages.
- ** -r**	Recursive.
- ** -v**	Display version information.
- ** -L** _bundle\_lifetime_

    Bundle lifetime in seconds. Default is 86400 seconds (1 day).

- ** -C** _BP\_custody_

    Acceptable values are ON/OFF,YES/NO,1/0. Default is OFF.

- ** -S** _class\_of\_service_

    Bundle Protocol Class of Service for this transfer. Available options are:

    - 0	Bulk Priority
    - 1	Standard Priority
    - 2	Expedited Priority

    Default is Standard Priority.

**bpcp** utilizes CFDP to preform the actual file transfers. This has several
important implications. First, ION's CFDP implementation requires that reliable
convergence layers be used to transfer the data. Second, file permissions are
not transferred. Files will be made executable on copy. Third, symbolic links
are ignored for local to remote transfers and their target is copied for remote
transfers. Fourth, all hosts must be specified using ION's IPN naming scheme.

In order to preform remote to local transfers or remote to remote transfers,
**bpcpd** must be running on the remote hosts. However, **bpcp** should NOT
be run simultaneously with **bpcpd** or **cfdptest**.

# EXIT STATUS

- "0"

    **bpcp** terminated normally.

- "1"

    **bpcp** terminated abnormally. Check console and the **ion.log** file for error messages.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bpcpd(1), ion(3), cfdptest(1)
