# NAME

bpsecrc - BP security policy management commands file

# DESCRIPTION

BP security policy management commands are passed to **bpsecadmin** either
in a file of text lines or interactively at **bpsecadmin**'s command prompt
(:).  Commands are interpreted line-by line, with exactly one command per
line.  JSON commands may span multiple lines when provided as part of a config
file. The formats and effects of the BP security policy management commands 
are described below.

A parameter identifed as an _eid\_expr_ is an "endpoint ID expression."  For
all commands, whenever the last character of an endpoint ID expression is
the wild-card character '\*', an applicable endpoint ID "matches" this EID
expression if all characters of the endpoint ID expression prior to the last
one are equal to the corresponding characters of that endpoint ID.  Otherwise
an applicable endpoint ID "matches" the EID expression only when all characters
of the EID and EID expression are identical.

At present, ION supports a subset of the proposed "BPSec" security protocol
specification currently under consideration by the Internet Engineering
Steering Group.  Since BPSec is not yet a published standard, ION's
Bundle Protocol security mechanisms will not necessarily interoperate
with those of other BP implementations.  This is unfortunate but (we hope)
temporary, as BPSec represents a major improvement in bundle security.
Future releases of ION will implement the entire BPSec specification.

# COMMANDS

- **?**

    The **help** command.  This will display a listing of the commands and their
    formats.  It is the same as the **h** command.

- **#**

    Comment line.  Lines beginning with **#** are not interpreted.

- **e** { 1 | 0 }

    Echo control.  Setting echo to 1 causes all output printed by bpsecadmin to
    be logged as well as sent to stdout.  Setting echo to 0 disables this behavior.

- **v** 

    Version number.  Prints out the version of ION currently installed.  HINT:
    combine with **e 1** command to log the version number at startup.

- **a** { **event\_set** : { **name** : event set name, **desc** : (opt) description } }

    The **add event\_set** command. This command will add a named security operation
    event set to the local security policy database.

- **i** { **event\_set** : { **name** : event set name } }

    The **info event\_set** command for event sets displays the information the system
    maintains for a named event set. The security operation events and configured, 
    optional processing actions associated with that event set are shown.

- **d** { **event\_set** : { **name** : event set name } }

    The **delete event\_set** command deletes a named event set from the system. 
    A named event set cannot be deleted if it is referenced by a security policy 
    rule. All security policy rules associated with the named event set must be deleted 
    before the event set itself may be deleted.

- **l** {**event\_set**}

    The **list event\_set** command lists the names of all event sets defined in the 
    system.

- **a** { **event** : { 
	**es\_ref**   : event set name,
	**event\_id** : security operation event ID,
	**actions**  : \[ {**id**: processing action, (opt.) action parm key, (opt.) parm value}, ... ,
                    {**id**: processing action, (opt.) action parm key, (opt.) parm value} \] } }

    The **add event** command adds security operation event and associated optional 
    processing action(s) to an event set. Multiple processing actions can be specified 
    for a single security operation event.

- **d** { **event** : { 
    **es\_ref** : event set name,
	**event\_id** : security operation event ID } }

    The **delete event** command is used to delete optional processing actions from a
    named event set. This command currently deletes **all** of the optional processing
    actions associated with the security operation event provided.

- **a** { **policyrule** : {
	**desc** : (opt.) description,
	**filter** :
	{
	    **rule\_id** : Security policy rule id,
	    **role** : Security policy role,
	    **src** : Bundle source,
	    **dest** : Bundle destination
	    **sec\_src** : Security source
	    **tgt** : Security target block type,
	    **sc\_id** : Security context ID, 
	},
	**spec** : 
	{
	    **svc** : Security service,
	    **sc\_id** : Security context ID,
	    **sc\_parms** : \[ {SC parm ID, SC parm value }, ... ,
		                {SC parm ID, SC parm value } \] 
	},
	**es\_ref** : Event set name } }

    The **add policyrule** command adds a policy rule to the system, describing a 
    required security operation and the security policy role of the BPA applying 
    the policy statement. The above command adds a policy rule referencing a 
    named event set to the system.

- **d** { **policyrule** : { **rule\_id** : Security policy rule ID } }

    The **delete policyrule** command deletes the policy rule identified by its
    rule ID.

- **i** {**policyrule** : { **rule\_id** : Security policy rule ID } }

    The **info policyrule** command displays the information for the
    policy rule matching the provided ID.

- **f** {**policyrule** : 
    {**type** : **all | best**, 
	**src** : Bundle source,
	**dest** : Bundle destination, 
	**sec\_src** : Security source, 
	**sc\_id** : Security context ID,
	**role** : Security policy role } }

    The **find policyrule** command finds all policy rules matching the provided criteria 
    when type **all** is selected, and finds the single policy rule that is determined to be 
    the best match when type **best** is selected. 

- **l** {**policyrule**}

    The **list policyrule** command lists all policy rules currently 
    defined in the system.

- **h**

    The **help** command.  This will display a listing of the commands and their
    formats.  It is the same as the **?** command.

# SEE ALSO

bpsecadmin(1)
