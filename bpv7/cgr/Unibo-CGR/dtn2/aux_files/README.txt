This directory contains a few auxiliary files necessary to enable the use of Unibo-CGR in DTN2/DTNME.
They are taken from homonimous original files, or are directly derived from them. Therefore they maintain their original copyright and license.

To include Unibo-CGR in DTN2/DTNME insert Unibo-CGR directory under ./servlib/routing/. 
Then copy these auxilairy files in the following directories ("." is DTNME's root directory)

BundleRouter.cc:            ./servlib/routing/
UniboCGRBundleRouter.cc:    ./servlib/routing/
UniboCGRBundleRouter.h:     ./servlib/routing/
Makefile:                   ./servlib/

The auxiliary file "contact-plan.txt" contains an example of contact plan and the syntax of the instructions accepted. It must be placed in the directory from which the dtnd daemon is launched.

Compile and install the usual way.

DTN2 configuration file
To enable Unibo-CGR, you must select it among the available routers in the config file. As CGR recognizes only ipn scheme addresses, you should add (not replace the dtn EID, which is compulsory in DTN2) an ipn EID to all your nodes, and optionally enable the ipn advertising. If the EID of a bundle destination follows the dtn scheme, Unibo-CGR cannot apply CGR and simply reverts to static routing. 
For the user's convenience, we list below the corresponding instructions to be set in the the configuration file.

# Set the algorithm used for dtn routing.
#
# route set type [uniboCGR |static | flood | neighborhood | linkstate | external]
#
route set type uniboCGR

# Set the local administrative id of this node. The default just uses
# the internet hostname plus the appended string ".dtn" to make it
# clear that the hostname isn't really used for DNS lookups.
#
route local_eid "dtn://[info hostname].dtn"
route local_eid_ipn "ipn:your_node_number.0"

# Parameter Tuning
param set announce_ipn true

For use and limitations in DTN2/DTNME, see the general Unibo-CGR README.txt 






