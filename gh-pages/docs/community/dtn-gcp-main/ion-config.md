# The ION Configuration File

Given to ionadmin either as a file or from the daemon command line, this file configures contacts for the ION node. We will assume that the local node's identification number is `1`.

This file specifies contact times and one-way light times between nodes. This is useful in deep-space scenarios: for instance, Mars may be 20 light-minutes away, or 8. Though only some transport protocols make use of this time (currently, only LTP), it must be specified for all links nonetheless. Times may be relative (prefixed with a + from current time) or absolute. Absolute times, are in the format `yyyy/mm/dd-hh:mm:ss`. By default, the contact-graph routing engine will make bundle routing decisions based on the contact information provided.

The configuration file lines are as follows:

## Initialize the ion node to be node number 1

````
1 1 ''
````

`1` refers to this being the initialization or `first` command.

`1` specifies the node number of this ion node. (IPN node 1).

`''` specifies the name of a file of configuration commands for the node's use of shared memory and other resources (suitable defaults are applied if you leave this argument as an empty string).

## Start the ION node

`s`

This will start the ION node. It mostly functions to officially "start" the node in a specific instant; it causes all of ION's protocol-independent background daemons to start running.

## Specify a transmission opportunity

````
a contact +1 +3600 1 1 100000
````

specifies a transmission opportunity for a given time duration between two connected nodes (or, in this case, a loopback transmission opportunity).

`a` adds this entry in the configuration table.

`contact` specifies that this entry defines a transmission opportunity.

`+1` is the start time for the contact (relative to when the s command is issued).

`+3600` is the end time for the contact (relative to when the s command is issued).

`1` is the source node number.

`1` is the destination node number.

`100000` is the maximum rate at which data is expected to be transmitted from the source node to the destination node during this time period (here, it is 100000 bytes / second).

## Specify a distance between nodes

````
a range +1 +3600 1 1 1
````

specifies a distance between nodes, expressed as a number of light seconds, where each element has the following meaning:

`a` adds this entry in the configuration table.

`range` declares that what follows is a distance between two nodes.

`+1` is the earliest time at which this is expected to be the distance between these two nodes (relative to the time s was issued).

`+3600` is the latest time at which this is still expected to be the distance between these two nodes (relative to the time s was issued).

`1` is one of the two nodes in question.

`1` is the other node.

`1` is the distance between the nodes, measured in light seconds, also sometimes called the "one-way light time" (here, one light second is the expected distance).


## Specify the maximum rate at which data will be produced by the node

````
m production 1000000
````

`m` specifies that this is a management command.

`production` declares that this command declares the maximum rate of data production at this ION node.

`1000000` specifies that at most 1000000 bytes/second will be produced by this node.


## Specify the maximum rate at which data can be consumed by the node

````
m consumption 1000000
````

`m` specifies that this is a management command.

`consumption` declares that this command declares the maximum rate of data consumption at this ION node.

`1000000` specifies that at most 1000000 bytes/second will be consumed by this node.


## Final configuration file - `host1.ionrc`

````
## begin ionadmin
1 1 ''
s
a contact +1 +3600 1 1 100000
a range +1 +3600 1 1 1
m production 1000000
m consumption 1000000
## end ionadmin
````
  
