# The Bundle Protocol Configuration File

Given to bpadmin either as a file or from the daemon command line, this file configures the endpoints through which this node's Bundle Protocol Agent (BPA) will communicate. We will assume the local BPA's node number is 1; as for LTP, in ION node numbers are used to identify bundle protocol agents.

## Initialise the bundle protocol
````
1
````. 

`1` refers to this being the initialization or ''first'' command.

## Add support for a new Endpoint Identifier (EID) scheme
````
a scheme ipn 'ipnfw' 'ipnadminep'
````

`a` means that this command will add something.

`scheme` means that this command will add a scheme.

`ipn` is the name of the scheme to be added.

`'ipnfw'` is the name of the IPN scheme's forwarding engine daemon.

`'ipnadminep'` is the name of the IPN scheme's custody transfer management daemon.

## Establishes the BP node's membership in a BP endpoint

````
a endpoint ipn:1.0 q
````

`a` means that this command will add something.

`endpoint` means that this command adds an endpoint.

`ipn` is the scheme name of the endpoint.

`1.0` is the scheme-specific part of the endpoint. For the IPN scheme the scheme-specific part always has the form nodenumber:servicenumber. Each node must be a member of the endpoint whose node number is the node's own node number and whose service number is 0, indicating administrative traffic.

`q` means that the behavior of the engine, upon receipt of a new bundle for this endpoint, is to queue it until an application accepts the bundle. The alternative is to silently discard the bundle if no application is actively listening; this is specified by replacing q with x.


## Specify two more endpoints that will be used for test traffic

````
a endpoint ipn:1.1 q
a endpoint ipn:1.2 q
````

## Add support for a convergence-layer protocol

````
a protocol ltp 1400 100
````

`a` means that this command will add something.

`protocol` means that this command will add a convergence-layer protocol.

`ltp` is the name of the convergence-layer protocol.

`1400` is the estimated size of each convergence-layer protocol data unit (in bytes); in this case, the value is based on the size of a UDP/IP packet on Ethernet.

`100` is the estimated size of the protocol transmission overhead (in bytes) per convergence-layer procotol data unit sent.


## Add an induct, through which incoming bundles can be received from other nodes

````
a induct ltp 1 ltpcli
````

`a` means that this command will add something.

`induct` means that this command will add an induct.

`ltp` is the convergence layer protocol of the induct.

`1` is the identifier of the induct, in this case the ID of the local LTP engine.

`ltpcli` is the name of the daemon used to implement the induct.



## Add an outduct, through which outgoing bundles can be sent to other nodes

````
a outduct ltp 1 ltpclo
````

`a` means that this command will add something.

`outduct` means that this command will add an outduct.

`ltp` is the convergence layer protocol of the outduct.

`1` is the identifier of the outduct, the ID of the convergence-layer protocol induct of some remote node. 

`ltpclo` is the name of the daemon used to implement the outduct.


## Start the bundle engine including all daemons for the inducts and outducts
````
s
````

## Final configuration file - `host1.bprc`

````
## begin bpadmin
1
a scheme ipn 'ipnfw' 'ipnadminep'
a endpoint ipn:1.0 q
a endpoint ipn:1.1 q
a endpoint ipn:1.2 q
a protocol ltp 1400 100
a induct ltp 1 ltpcli
a outduct ltp 1 ltpclo
s
## end bpadmin
````
