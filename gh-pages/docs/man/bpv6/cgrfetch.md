# NAME

cgrfetch - Visualize CGR simulations

# SYNOPSIS

**cgrfetch** \[_OPTIONS_\] _DEST-NODE_

# DESCRIPTION

**cgrfetch** uses CGR to simulate sending a bundle from the local node to
_DEST-NODE_. It traces the execution of CGR to generate graphs of the routes
that were considered and the routes that were ultimately chosen to forward
along. No bundle is sent during the simulation.

A JSON representation of the simulation is output to _OUTPUT-FILE_. The
representation includes parameters of the simulation and a structure for each
considered route, which in turn includes calculated parameters for the route and
an image of the contact graph.

The dot(1) tool from the Graphviz package is used to generate the contact graph
images and is required for cgrfetch(1). The base64(1) tool from coreutils is
used to embed the images in the JSON and is also required.

Note that a trace of the route computation logic performed by CGR is printed
to stderr; there is currently no cgrfetch option for redirecting this output
to a file.

# OPTIONS

- **DEST-NODE**

    The final destination to route to. To be useful, it should be a node that exists
    in the contact plan.

- **-q**

    Disable trace message output.

- **-j**

    Disable JSON output.

- **-m**

    Use a minimum-latency extended COS for the bundle. This ends up sending the
    bundle to all proximate nodes.

- **-t DISPATCH-OFFSET**

    Request a dispatch time of _DISPATCH-OFFSET_ seconds from the time the command
    is run (default: 0).

- **-e EXPIRATION-OFFSET**

    Set the bundle expiration time to _EXPIRATION-OFFSET_ seconds from the time the
    command is run (default: 3600).

- **-s BUNDLE-SIZE**

    Set the bundle payload size to _BUNDLE-SIZE_ bytes (default: 0).

- **-o OUTPUT-FILE**

    Send JSON to _OUTPUT-FILE_ (default: stdout).

- **-d PROTO:NAME**

    Use _PROTO_ as the outduct protocol and _NAME_ as the outduct name (default:
    udp:\*). Use **list** to list all available outducts.

# EXAMPLES

- cgrfetch 8

    Simulate CGR with destination node 8 and dispatch time equal to the current time.

- cgrfetch 8 -t 60

    Do the same with a dispatch time 60 seconds in the future.

- cgrfetch -d list

    List all available outducts.

# SEE ALSO

dot(1), base64(1)
